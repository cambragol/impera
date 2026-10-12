#pragma once
#include "dialogue.h"
#include <QImage>
#include "native_text_viewport.h"
namespace Dialogue {
// Legacy 18-column layout oracle. Pixel output uses the real 16x13 renderer.
QVector<PreviewGlyph> layoutTalkWords(const QByteArray &styled);
struct GamePreview {
    QImage image;
    QVector<PreviewGlyph> glyphs;
    QStringList notes;
};
GamePreview previewResponse(const QByteArray &bytes, int entry, const QByteArray &font,
                            const QByteArray &runes, const Sandbox &state);
} // namespace Dialogue
