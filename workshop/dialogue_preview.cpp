#include "dialogue_preview.h"
#include <functional>
namespace Dialogue {
QVector<PreviewGlyph> layoutTalkWords(const QByteArray &styled) {
    U5::require(styled.size() <= 16384, "Dialogue preview exceeds supported bounds");
    struct Layout {
        QVector<PreviewGlyph> result;
        int x = 0, y = 0, font = 0;
        uint8_t word[16]{}, length = 0, last = 0;
    } layout;
    ImperaWords words{};
    words.buffer = layout.word;
    words.length = &layout.length;
    words.lastColumn = &layout.last;
    words.host = &layout;
    words.column = [](void *host) { return static_cast<Layout *>(host)->x; };
    words.font = [](void *host, int index) { static_cast<Layout *>(host)->font = index; };
    words.print = [](void *host, uint16_t ch) {
        auto &s = *static_cast<Layout *>(host);
        if (ch == '\n') {
            s.x = 0;
            ++s.y;
        } else {
            s.result.append({s.x, s.y, ch, s.font != 0});
            if (++s.x > 17) {
                s.x = 0;
                ++s.y;
            }
        }
    };
    for (unsigned char ch : styled)
        IMPERA_WordAppend(&words, ch);
    IMPERA_WordFlush(&words);
    return layout.result;
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
    // A selected response starts in a cleared window, unlike a full session.
    NativeTextWindow window(font, runes, 0);
    for (unsigned char ch : styled)
        window.append(ch);
    window.flush();
    result.glyphs = window.glyphs();
    result.image = window.image();
    return result;
}
} // namespace Dialogue
