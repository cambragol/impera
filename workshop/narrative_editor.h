#pragma once
#include "location_text.h"
#include "mod/narrative.h"
namespace Workshop {
QVector<const U5NarrativeEntry *> narrativeEntries(const QString &resource);
QByteArray narrativeText(const Project &project, const U5NarrativeEntry &entry);
int narrativeBudget(const QByteArray &bytes, const U5NarrativeEntry &entry);
QString narrativeSource(const QByteArray &bytes, bool shop);
QByteArray encodeNarrative(const QString &text, bool shop);
QMap<QString, QByteArray> changeNarrative(const Project &project, const U5NarrativeEntry &entry,
                                          const QString &text, bool extended);
void validateNarrative(const Project &project, const QString &resource);
QWidget *narrativeEditor(Project *project, const QString &resource, TextCommit commit,
                         QMap<QString, int> *state, std::function<bool()> *flush);
} // namespace Workshop
