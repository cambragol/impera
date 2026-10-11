#include "dialogue_preview.h"
#include <functional>
namespace Dialogue {
QVector<PreviewGlyph> layoutTalkWords(const QByteArray &styled) {
    U5::require(styled.size() <= 16384, "Dialogue preview exceeds supported bounds");
    QVector<PreviewGlyph> result;
    QByteArray word;
    int x = 0, y = 0, lastColumn = 0;
    auto print = [&](int character, bool runic) {
        if (character == '\n') {
            x = 0;
            ++y;
        } else {
            result.append({x, y, character, runic});
            if (++x > 17) {
                x = 0;
                ++y;
            }
        }
    };
    auto flush = [&] {
        for (unsigned char byte : word) {
            const int character = byte & 127;
            if (x != 0 || character != ' ') {
                if (lastColumn < 15 || character != '\n') {
                    if (character != '\n')
                        lastColumn = x;
                    print(character, !(byte & 128));
                } else
                    lastColumn = 0;
            }
        }
        word.clear();
    };
    // Port of TALK_0574_BuildOrPrintWord and TALK_04e2_PrintWord. In
    // particular, a full 16-byte word flushes before consuming the next byte.
    // This unusual behavior is tested against the extracted engine functions.
    for (unsigned char byte : styled) {
        if (word.size() != 16) {
            word.append(char(byte));
            if (byte != 0x8a && byte != 0xa0)
                continue;
            if (x + word.size() >= 18)
                print('\n', false);
        }
        flush();
    }
    flush();
    return result;
}
GamePreview previewResponse(const QByteArray &bytes, int entry, const QByteArray &font,
                            const QByteArray &runes, const Sandbox &state) {
    U5::require(font.size() == 1024, "Game preview requires the original 1024-byte IBM.CH font");
    U5::require(bytes.size() <= 1024, "Conversation response exceeds supported bounds");
    GamePreview result;
    QByteArray styled;
    int mask = 255, lastLiteral = 0;
    bool dictionaryBoundary = false, stopped = false;
    auto literal = [&](int value) {
        if (value == 0xa2 && lastLiteral == 0xa2)
            return;
        if (value == 0x8d)
            value = 0x8a;
        lastLiteral = value & mask;
        if (dictionaryBoundary)
            styled += char(0xa0);
        styled += char(lastLiteral);
        dictionaryBoundary = false;
    };
    auto appendLiteral = [&](const QString &text) {
        for (QChar ch : text) {
            U5::require(ch.unicode() >= 32 && ch.unicode() < 127,
                        "Preview names must contain DOS ASCII characters");
            literal(ch.unicode() | 128);
        }
    };
    const bool description = entry == 1;
    if (description)
        appendLiteral("You see ");
    else {
        appendLiteral("\"");
        if (entry == 0)
            appendLiteral("My name is ");
    }
    int prefix = bytes.size() >= 2 && U5::byte(bytes, 0) == 144 ? 2 : 0;
    for (const auto &block : blocks(bytes.mid(prefix))) {
        const int code = block.opcode;
        if (code < 0) {
            for (unsigned char byte : block.bytes) {
                if (byte <= 128) {
                    QString expanded = U5::decodeText(QByteArray(1, char(byte)));
                    U5::require(expanded.startsWith('<') && expanded.endsWith('>') &&
                                    !expanded.startsWith("<Byte"),
                                "Unknown dictionary word");
                    expanded = expanded.mid(1, expanded.size() - 2);
                    styled += char(0xa0);
                    for (QChar ch : expanded)
                        styled += char(ch.unicode() | 128);
                    dictionaryBoundary = true;
                } else
                    literal(byte);
            }
        } else if (code == 129)
            appendLiteral(state.avatar.left(8));
        else if (code == 142)
            mask ^= 128;
        else if (code == 130 || code == 132 || code == 255 || (code >= 145 && code <= 159)) {
            result.notes.append("Stops at: " + actionName(code) +
                                ". Use Playtest to follow branches.");
            stopped = true;
            break;
        } else if (code == 136) {
            appendLiteral("What is thy name?\"");
            result.notes.append("Stops for player name input. Continue in Playtest.");
            stopped = true;
            break;
        } else if (code == 140 || code == 254) {
            bool taken =
                code == 140 ? state.introduced : state.karma >= int(U5::byte(block.bytes, 1));
            result.notes.append(taken
                                    ? "Conditional branch taken; continue in Playtest."
                                    : "Conditional branch not taken with the current Setup state.");
            if (taken) {
                stopped = true;
                break;
            }
        } else if (code == 133) {
            int gold = 0;
            for (int i = 1; i <= 3; ++i)
                gold = gold * 10 + (U5::byte(block.bytes, i) & 127) - '0';
            result.notes.append(
                QString("Payment: %1 gold. Playtest calculates state changes.").arg(gold));
        } else if (code == 143 || code == 131)
            result.notes.append(actionName(code) +
                                ": preview shows text on both sides of this control.");
        else
            result.notes.append(actionName(code) + ": world effects are not rendered.");
    }
    if (!stopped && !description)
        appendLiteral("\"");
    result.glyphs = layoutTalkWords(styled);
    int rows = 1;
    for (const auto &glyph : result.glyphs)
        rows = qMax(rows, glyph.y + 1);
    result.image = QImage(144, rows * 8, QImage::Format_RGB32);
    result.image.fill(Qt::black);
    for (const auto &glyph : result.glyphs) {
        const bool missingRunes = glyph.runic && runes.size() != 1024;
        const auto &charset = glyph.runic && !missingRunes ? runes : font;
        if (missingRunes &&
            !result.notes.contains("RUNES.CH unavailable; runic text is shown in the normal font."))
            result.notes.append("RUNES.CH unavailable; runic text is shown in the normal font.");
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                if (U5::byte(charset, glyph.character * 8 + y) & (128 >> x))
                    result.image.setPixel(glyph.x * 8 + x, glyph.y * 8 + y,
                                          missingRunes ? qRgb(255, 180, 80) : qRgb(255, 255, 255));
    }
    return result;
}
} // namespace Dialogue
