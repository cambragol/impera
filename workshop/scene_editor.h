#pragma once
#include "map_editor.h"
#include "narrative_editor.h"
namespace Workshop {
struct SceneStep {
    int offset, opcode;
    QByteArray bytes;
    QString title;
};
class SceneDocument {
  public:
    static QStringList names();
    static QStringList stages();
    static QByteArray map(const QByteArray &bytes, int scene, int stage = 0);
    static QByteArray changeMap(const QByteArray &bytes, int scene, int stage,
                                const QByteArray &cells);
    static QVector<SceneStep> steps(const QByteArray &bytes);
    static void validate(const QByteArray &bytes);
    static QByteArray changeSteps(const QByteArray &bytes, const QVector<QByteArray> &steps);
    static QPoint affectedCell(const QByteArray &bytes, int step);
    static QByteArray preview(const QByteArray &bytes, int step,
                              QVector<MapCanvas::Actor> *actors = nullptr, int *stage = nullptr);
};
QWidget *sceneEditor(Project *, const QString &, TextCommit, QMap<QString, int> *,
                     std::function<bool()> *);
} // namespace Workshop
