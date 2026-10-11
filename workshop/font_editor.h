#pragma once
#include "location_text.h"
namespace Workshop {
struct FontDocument {
    static int stride(const QString &name);
    static QSize dimensions(const QString &name);
    static void validate(const QString &, const QByteArray &);
    static QImage glyph(const QString &, const QByteArray &, int index);
    static QByteArray replace(const QString &, const QByteArray &, int index, const QImage &);
    static QString label(int index);
    static QImage sample(const QString &, const QByteArray &, const QString &);
};
QWidget *fontEditor(Project *, const QString &, TextCommit, QMap<QString, int> *,
                    std::function<bool()> *);
} // namespace Workshop
