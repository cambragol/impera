#pragma once
#include "narrative_editor.h"
#include <QTextEdit>
namespace Workshop {
QStringList shopServices();
QStringList shopEntryServices(const U5NarrativeEntry &entry);
void loadShopText(QTextEdit *editor, const QByteArray &bytes);
QByteArray shopTextBytes(const QTextDocument &document);
QWidget *shopEditor(Project *project, TextCommit commit, QMap<QString, int> *state,
                    std::function<bool()> *flush);
} // namespace Workshop
