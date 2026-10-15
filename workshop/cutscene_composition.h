#pragma once
#include "cutscene_editor.h"
#include "mod/cutscene_composition.h"
namespace Workshop {
struct CutsceneImage {
    QString name;
    QImage image;
    QPoint position;
};
struct CutsceneTextBox {
    QRect bounds;
    QString text;
    bool flow = false;
    U5CutsceneLayout layout = {};
};
struct CutsceneComposition {
    QVector<CutsceneImage> images;
    QVector<CutsceneTextBox> texts;
};
CutsceneImage importCutscenePng(const QString &path);
QVector<CutsceneComposition> compositionPages(const Project &, int scene, bool inherit = true);
QByteArray changeComposition(const Project &, int scene, const QVector<CutsceneComposition> &);
QImage previewComposition(const Project &, const CutsceneComposition &,
                          QStringList *issues = nullptr);
QWidget *compositionEditor(Project *, const QString &, TextCommit, QMap<QString, int> *,
                           std::function<bool()> *);
} // namespace Workshop
