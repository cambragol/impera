#include "content_navigation.h"
#include "dialogue.h"
#include "map_editor.h"
#include "resource_document.h"
#include <algorithm>
namespace Workshop {
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
            title = "Illustration " + name.section('.', 0, 0).toLower();
            if (name == "TILES.16")
                title = "Terrain and object tiles";
            break;
        case EditorKind::Fonts:
            workspace = "Artwork & Fonts";
            if (name.endsWith(".HCS"))
                title += " (Hercules)";
            break;
        case EditorKind::State:
        case EditorKind::WorldObjects:
            workspace = "New Game";
            break;
        case EditorKind::Tables:
            workspace = "Game Rules";
            break;
        case EditorKind::Story:
        case EditorKind::Narrative:
        case EditorKind::Scenes:
            workspace = "Story & Scenes";
            break;
        default:
            workspace = "Mod Overview";
            group = "Compatibility details";
            break;
        }
        result.append({"content/" + name, workspace, group, title, name, -1});
    }
    return result;
}
} // namespace Workshop
