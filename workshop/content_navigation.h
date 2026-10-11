#pragma once
#include "project.h"
namespace Workshop {
// Semantic navigation destinations reference Project; they never own editable bytes.
struct ContentDestination {
    QString id, workspace, group, title, resource;
    int page = -1;
};
QVector<ContentDestination> contentDestinations(const Project &project);
QStringList contentWorkspaces();
} // namespace Workshop
