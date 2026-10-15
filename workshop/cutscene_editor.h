#pragma once
#include "mod/cutscene_placement.h"
#include "narrative_editor.h"
namespace Workshop {
struct CutscenePage {
    int layout = 0;
    QString text;
    U5CutscenePlacement placement = {};
};
bool operator==(const CutscenePage &, const CutscenePage &);
QVector<CutscenePage> cutscenePages(const Project &, int scene);
QByteArray changeCutscene(const Project &, int scene, const QVector<CutscenePage> &);
QImage previewCutscene(const Project &, int scene, const CutscenePage &);
QWidget *cutsceneEditor(Project *, const QString &, TextCommit, QMap<QString, int> *,
                        std::function<bool()> *);
} // namespace Workshop
