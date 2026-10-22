#pragma once
#include "location_text.h"
namespace Workshop {
class TableDocument {
  public:
    static void validate(const QByteArray &);
    static int value(const QByteArray &, int table, int index);
    static QByteArray change(const QByteArray &, int table, int index, int value,
                             bool inherit = false);
    static QString entryLabel(int table, int index, const Project *project = nullptr);
};
QWidget *tableEditor(Project *, const QString &, TextCommit, QMap<QString, int> *,
                     std::function<bool()> *);
} // namespace Workshop
