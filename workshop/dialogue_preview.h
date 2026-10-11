#pragma once
#include "dialogue.h"
#include <QImage>
namespace Dialogue {
struct PreviewGlyph {
    int x = 0, y = 0, character = 0;
    bool runic = false;
};
// Styled bytes use the same high-bit font selection as TALK_04e2_PrintWord.
QVector<PreviewGlyph> layoutTalkWords(const QByteArray &styled);
struct GamePreview {
    QImage image;
    QVector<PreviewGlyph> glyphs;
    QStringList notes;
};
GamePreview previewResponse(const QByteArray &bytes, int entry, const QByteArray &font,
                            const QByteArray &runes, const Sandbox &state);
} // namespace Dialogue
