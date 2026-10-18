#pragma once
#include "project.h"
namespace Workshop {
inline int npcCharacter(int town, int slot) {
    return slot < 32 ? town * 32 + slot : 256 + town * 32768 + slot;
}
inline int npcTown(int character) {
    return character < 256 ? character / 32 : (character - 256) / 32768;
}
inline int npcSlot(int character) {
    return character < 256 ? character % 32 : (character - 256) % 32768;
}

// Semantic navigation destinations reference Project; they never own editable bytes.
struct ContentDestination {
    QString id, workspace, group, title, resource;
    int page = -1;
    int character = -1, conversation = -1, settlement = -1;
    QString scheduleResource, provenance;
    QMap<QString, int> selection;
    QStringList sources;
};
QVector<ContentDestination> characterDestinations(const Project &project);
QVector<ContentDestination> contentDestinations(const Project &project);
QStringList contentWorkspaces();
QString artworkTitle(const QString &resource);
QMap<QString, QStringList> contentChanges(const Project &, const QVector<ContentDestination> &);
bool matchesSelection(const ContentDestination &, const QMap<QString, int> &state);
} // namespace Workshop
