#include "content_navigation.h"
#include "map_editor.h"
#include "resource_document.h"
namespace Workshop {
QStringList contentWorkspaces() {
    return {"World",      "Characters", "Story & Scenes", "Artwork & Fonts",
            "Game Rules", "New Game",   "Mod Overview"};
}
QVector<ContentDestination> contentDestinations(const Project &project) {
    QVector<ContentDestination> result;
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
        case EditorKind::Schedule: {
            workspace = "Characters";
            QString region = name.startsWith("TOWNE")    ? "Towns"
                             : name.startsWith("CASTLE") ? "Castles and villages"
                             : name.startsWith("KEEP")   ? "Keeps"
                                                         : "Dwellings";
            title =
                region + (c.editor == EditorKind::Conversation ? " conversations" : " schedules");
            break;
        }
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
