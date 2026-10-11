#include "content_navigation.h"
#include "dialogue.h"
#include "map_editor.h"
#include "resource_document.h"
#include "scene_editor.h"
#include <algorithm>
namespace Workshop {
QString artworkTitle(const QString &name) {
    static const QMap<QString, QString> titles = {
        {"TILES.16", "Terrain, objects and characters"},
        {"ITEMS.16", "Dungeon item illustrations"},
        {"CREATE.16", "Character creation artwork"},
        {"TEXT.16", "Story text framing"},
        {"ULTIMA.16", "Title artwork"},
        {"STARTSC.16", "Title scene artwork"},
        {"ENDSC.16", "Final rescue artwork"}};
    if (titles.contains(name)) return titles[name];
    const auto stem = name.section('.', 0, 0);
    for (const auto prefix : {QString("STORY"), QString("END"), QString("DNG"), QString("MON")}) {
        bool ok = false;
        int number = stem.mid(prefix.size()).toInt(&ok);
        if (stem.startsWith(prefix) && ok) {
            QString purpose = prefix == "STORY" ? "Introduction illustrations"
                            : prefix == "END" ? "Ending illustrations"
                            : prefix == "DNG" ? "Dungeon scenery" : "Dungeon creature illustrations";
            return purpose + QString(" — set %1").arg(prefix == "MON" ? number + 1 : number);
        }
    }
    return "Artwork collection — " + stem;
}
bool matchesSelection(const ContentDestination &d, const QMap<QString, int> &state) {
    for (auto it = d.selection.begin(); it != d.selection.end(); ++it)
        if (state.value(it.key(), 0) != it.value()) return false;
    return true;
}
QStringList contentWorkspaces() {
    return {"World",      "Characters", "Story & Scenes", "Artwork & Fonts",
            "Game Rules", "New Game",   "Mod Overview"};
}
QVector<ContentDestination> characterDestinations(const Project &project) {
    QVector<ContentDestination> result;
    for (const QString stem : {"TOWNE", "CASTLE", "KEEP", "DWELLING"}) {
        const QString talk = stem + ".TLK", schedule = stem + ".NPC", map = stem + ".DAT";
        QVector<U5::Conversation> conversations;
        if (project.resources.contains(talk)) {
            try {
                conversations = U5::readDialogue(project.data(talk));
            } catch (...) {
            }
        }
        QStringList names;
        for (const auto &conversation : conversations) {
            QString name;
            try {
                auto doc = Dialogue::parse(conversation.bytes);
                if (!doc.entries.isEmpty()) {
                    // Name entries may continue with flags and name-recognition actions.
                    // Display literal name text only; preserve all commands in the editor.
                    for (const auto &block : Dialogue::blocks(doc.entries[0].bytes)) {
                        if (block.opcode >= 0) break;
                        name += Dialogue::plain(block.bytes);
                    }
                    name = name.simplified();
                }
            } catch (...) {
            }
            names.append(name.isEmpty() ? "Unnamed character" : name);
        }
        QMap<int, QString> locations;
        if (project.resources.contains(map)) {
            try {
                for (const auto &page : U5::mapPages(map, project.data(map)))
                    if (page.settlement >= 0)
                        locations[page.settlement] = page.name.section(" — ", 0, 0);
            } catch (...) {
            }
        }
        const QByteArray data =
            project.resources.contains(schedule) ? project.data(schedule) : QByteArray();
        QMap<int, QStringList> users;
        if (data.size() == 4608) {
            for (int town = 0; town < 8; ++town)
                for (int slot = 0; slot < 32; ++slot) {
                    if (U5::byte(data, town * 576 + 512 + slot) == 0)
                        continue;
                    int id = U5::byte(data, town * 576 + 544 + slot), conversation = -1;
                    for (int i = 0; i < conversations.size(); ++i)
                        if (int(conversations[i].id) == id) {
                            conversation = i;
                            break;
                        }
                    const QString location =
                        locations.value(town, QString("%1 location %2").arg(stem).arg(town + 1));
                    QString name = conversation >= 0 ? names[conversation]
                                                     : QString("Character %1").arg(slot + 1);
                    ContentDestination d{
                        QString("character/%1/%2/%3").arg(stem).arg(town).arg(slot), "Characters",
                        location, name, conversation >= 0 ? talk : schedule};
                    d.character = town * 32 + slot;
                    d.conversation = conversation;
                    d.settlement = town;
                    d.scheduleResource = schedule;
                    d.provenance = QString("%1 · character %2 · conversation number %3")
                                       .arg(location)
                                       .arg(slot + 1)
                                       .arg(id);
                    result.append(d);
                    if (conversation >= 0)
                        users[conversation].append(d.provenance);
                }
        }
        for (int i = 0; i < conversations.size(); ++i) {
            if (!users.contains(i)) {
                ContentDestination d{
                    QString("conversation/%1/%2").arg(stem).arg(conversations[i].id), "Characters",
                    data.size() == 4608 ? "Unassigned conversations" : "Unverified assignments", names[i], talk};
                d.conversation = i;
                d.provenance = data.size() == 4608
                    ? "No scheduled character uses this conversation. It may be referenced by special game logic."
                    : "Schedule data is missing or damaged; character assignments cannot be verified.";
                result.append(d);
            }
        }
        for (auto &d : result)
            if (d.resource == talk && d.conversation >= 0 && users.contains(d.conversation))
                d.provenance +=
                    QString("\n%1%2 scheduled character(s) use this conversation:\n%3")
                        .arg(users[d.conversation].size() > 1 ? "Shared dialogue: " : "")
                        .arg(users[d.conversation].size())
                        .arg(users[d.conversation].join("\n"));
    }
    return result;
}
QVector<ContentDestination> contentDestinations(const Project &project) {
    QVector<ContentDestination> result = characterDestinations(project);
    bool usableScenes = false;
    if (project.resources.contains("MISCMAPS.DAT")) {
        try {
            SceneDocument::validate(project.data("MISCMAPS.DAT"));
            usableScenes = true;
        } catch (...) {
            // Keep independent message editors available when their scene bank is damaged.
        }
    }
    for (auto it = project.resources.begin(); it != project.resources.end(); ++it) {
        const auto name = it.key();
        const auto c = capability(name);
        if (MapDocument::supported(name)) {
            const auto pages = U5::mapPages(name, project.data(name));
            for (int i = 0; i < pages.size(); ++i) {
                const auto &p = pages[i];
                QString group = p.settlement >= 0       ? "Settlements"
                                : name.endsWith(".CBT") ? "Combat maps"
                                                        : "World maps";
                QString title = p.name;
                // Settlement floors belong in the editor's floor selector, not another tree.
                if (p.settlement >= 0) {
                    int main = i;
                    for (int j = 0; j < pages.size(); ++j)
                        if (pages[j].settlement == p.settlement &&
                            qAbs(pages[j].floor) < qAbs(pages[main].floor))
                            main = j;
                    if (main != i)
                        continue;
                    title = title.section(" — ", 0, 0);
                } else if (name.endsWith(".CBT")) {
                    title = QString("%1 encounter %2")
                                .arg(name == "BRIT.CBT" ? "Outdoor" : "Dungeon")
                                .arg(i + 1);
                }
                result.append(
                    {QString("world/%1/%2").arg(name).arg(p.settlement >= 0 ? p.settlement : i),
                     "World", group, title, name, i});
            }
            continue;
        }
        QString workspace, group = c.group, title = c.title;
        switch (c.editor) {
        case EditorKind::Dungeon:
            workspace = "World";
            group = "Dungeons";
            break;
        case EditorKind::LocationText:
            workspace = "World";
            group = "Location text";
            break;
        case EditorKind::Conversation:
        case EditorKind::Schedule:
            // Damaged or empty banks remain accessible without inventing characters.
            if (std::any_of(result.cbegin(), result.cend(), [&](const auto &d) {
                    return d.resource == name || d.scheduleResource == name;
                }))
                continue;
            workspace = "Characters";
            group = "Unavailable character data";
            break;
        case EditorKind::Artwork:
            workspace = "Artwork & Fonts";
            title = artworkTitle(name);
            group = name == "TILES.16" ? "Game tiles" : "Illustrations";
            break;
        case EditorKind::Fonts:
            workspace = "Artwork & Fonts";
            if (name.endsWith(".HCS"))
                title += " (Hercules)";
            break;
        case EditorKind::State:
            workspace = "New Game";
            group = "Starting game";
            title = "Starting party and world state";
            break;
        case EditorKind::WorldObjects:
            if (name == "INIT.OOL" && project.resources.contains("UNDER.OOL")) continue;
            workspace = "New Game";
            group = "Starting objects";
            title = name == "BRIT.OOL" ? "Britannia objects" : "Underworld objects";
            break;
        case EditorKind::Tables:
            workspace = name == "DATA.OVL" ? "Mod Overview" : "Game Rules";
            group = name == "DATA.OVL" ? "Compatibility details" : "Shops and locations";
            title = name == "DATA.OVL" ? "Engine compatibility coverage" : "Prices, stock and location entrances";
            break;
        case EditorKind::Scenes:
            for (int scene = 0; scene < SceneDocument::names().size(); ++scene) {
                ContentDestination d{"scene/" + QString::number(scene),
                    scene == 4 ? "New Game" : "Story & Scenes", "Scenes",
                    SceneDocument::names()[scene], name};
                d.selection["scene/current"] = scene;
                d.sources = {name, scene == 4 ? "QUESTION.DAT" : scene == 3 ? "ENDMSG.DAT" : "MISCMSG.DAT"};
                result.append(d);
            }
            continue;
        case EditorKind::Story:
        case EditorKind::Narrative:
            if (usableScenes &&
                QStringList{"QUESTION.DAT", "MISCMSG.DAT", "ENDMSG.DAT"}.contains(name)) continue;
            workspace = name == "QUESTION.DAT" ? "New Game" : name == "SHOPPE.DAT" ? "Game Rules" : "Story & Scenes";
            group = name == "SHOPPE.DAT" ? "Shops and locations" : "Stories and messages";
            if (name == "SHOPPE.DAT") title = "Shop and service dialogue";
            if (name == "KARMA.DAT") title = "Dreams and virtue speeches";
            break;
        default:
            workspace = "Mod Overview";
            group = "Compatibility details";
            break;
        }
        ContentDestination d{"content/" + name, workspace, group, title, name, -1};
        d.sources = {name};
        if (name == "UNDER.OOL" && project.resources.contains("INIT.OOL")) d.sources.append("INIT.OOL");
        result.append(d);
    }
    return result;
}
} // namespace Workshop
