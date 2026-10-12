#pragma once
#include "map_editor.h"
namespace Workshop {
struct ContentsTarget;
enum class MapItemKind {
    Npc,
    Monster,
    Sign,
    Container,
    HiddenContents,
    Horse,
    Skiff,
    Frigate,
    Carpet,
    Trigger,
    Item,
    SleepingFigure
};
struct MapItemEdit {
    QMap<QString, QByteArray> changes;
    int slot = -1;
    QString mode;
};
QString mapItemName(MapItemKind kind);
QList<MapItemKind> availableMapItems(const Project &, const QString &resource);
MapItemEdit addMapItem(const Project &, const QString &resource, int page, QPoint cell,
                       MapItemKind kind, int appearance = -1, int conversation = -1,
                       const QString &name = "New NPC");
QMap<QString, QByteArray> deleteMapItem(const Project &, const QString &resource, int page,
                                        const QString &mode, int slot, QPoint cell);
QMap<QString, QByteArray> moveSignTerrain(const Project &, const QString &, int page, QPoint from,
                                          QPoint to);
QByteArray addContentsAt(const Project &, ContentsTarget target, QPoint cell, bool container);
QString actorName(int shape);
QList<int> monsterShapes();
} // namespace Workshop
