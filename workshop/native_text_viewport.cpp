#include "native_text_viewport.h"
#include "formats.h"
#include <QPainter>
namespace Dialogue {
NativeTextWindow::NativeTextWindow(const QByteArray &normal, const QByteArray &runes, int row)
    : pixels(128 * 104, char(0)) {
    U5::require(normal.size() == 1024, "Preview requires IBM.CH");
    U5::require(runes.isEmpty() || runes.size() == 1024, "Invalid RUNES.CH font");
    U5::require(row >= 0 && row <= 12, "Invalid text cursor row");
    // Force independent storage; effects temporarily modify individual glyphs.
    fonts[0] = QByteArray(normal.constData(), normal.size());
    fonts[1] = QByteArray(runes.constData(), runes.size());
    window = {24, 11, 39, 23, 0, uint8_t(row), 15, 0};
    charset = reinterpret_cast<uint8_t *>(fonts[0].data());
    text.window = &current;
    text.font = &charset;
    text.savedGlyph = saved;
    text.advance = &advance;
    text.underline = &underline;
    text.center = &center;
    text.inverse = &inverse;
    text.foreground = &fg;
    text.background = &bg;
    text.host = this;
    text.draw = [](void *host, uint8_t *font, int offset, uint8_t f, uint8_t b, int x, int y) {
        auto *self = static_cast<NativeTextWindow *>(host);
        self->rendered.append({x - self->window.left, y - self->window.top, offset / 8,
                               font == reinterpret_cast<uint8_t *>(self->fonts[1].data())});
        IMPERA_TextGlyph(font, offset, f, b, x * 8, y * 8, host, pixel);
    };
    text.clear = [](void *host, int x1, int y1, int x2, int y2, uint8_t color) {
        for (int y = y1; y <= y2; ++y)
            for (int x = x1; x <= x2; ++x)
                pixel(host, x, y, color);
    };
    text.scroll = [](void *host, int x1, int y1, int x2, int y2, int amount) {
        auto *self = static_cast<NativeTextWindow *>(host);
        IMPERA_TextScroll(reinterpret_cast<uint8_t *>(self->pixels.data()), 128, x1 - 192, y1 - 88,
                          x2 - 192, y2 - 88, amount, nullptr, nullptr);
    };
    words.buffer = word;
    words.length = &length;
    words.lastColumn = &lastColumn;
    words.host = this;
    words.column = [](void *host) {
        return int(static_cast<NativeTextWindow *>(host)->window.currentX);
    };
    words.font = [](void *host, int index) {
        static_cast<NativeTextWindow *>(host)->selectFont(index);
    };
    words.print = [](void *host, uint16_t ch) { static_cast<NativeTextWindow *>(host)->print(ch); };
}
void NativeTextWindow::pixel(void *host, int x, int y, uint8_t color) {
    auto *self = static_cast<NativeTextWindow *>(host);
    x -= 192;
    y -= 88;
    if (x >= 0 && x < 128 && y >= 0 && y < 104)
        self->pixels[y * 128 + x] = char(color);
}
void NativeTextWindow::selectFont(int index) {
    U5::require(index >= 0 && index < 2 && fonts[index].size() == 1024,
                "Runic preview requires RUNES.CH");
    charset = reinterpret_cast<uint8_t *>(fonts[index].data());
}
void NativeTextWindow::print(uint16_t ch) { IMPERA_TextPrint(&text, ch); }
void NativeTextWindow::append(uint8_t ch) { IMPERA_WordAppend(&words, ch); }
void NativeTextWindow::flush() { IMPERA_WordFlush(&words); }
void NativeTextWindow::backspace(int count) { IMPERA_TextBackspace(&text, count); }
void NativeTextWindow::paintCursor(uint8_t ch) {
    const auto savedAdvance = advance;
    advance = 0;
    print(ch);
    advance = savedAdvance;
}
QImage NativeTextWindow::image() const {
    QImage image(128, 104, QImage::Format_RGB32);
    for (int y = 0; y < 104; ++y)
        for (int x = 0; x < 128; ++x)
            image.setPixel(x, y,
                           0xff000000u | IMPERA_TextPalette[uint8_t(pixels[y * 128 + x]) & 15]);
    return image;
}
NativeTextViewport::NativeTextViewport(QWidget *parent) : QWidget(parent) {
    setObjectName("nativeDialogueViewport");
    setMinimumSize(128, 125);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
void NativeTextViewport::setImage(const QImage &image) {
    U5::require(image.size() == QSize(128, 104), "Invalid native text framebuffer");
    frame = image;
    update();
}
void NativeTextViewport::clear() {
    frame = QImage();
    update();
}
QSize NativeTextViewport::sizeHint() const { return QSize(384, 375); }
QRect NativeTextViewport::displayRect() const {
    // One DOS pixel is 5:6. Resize presentation only, never the logical window.
    const double scale = qMin(width() / 128.0, height() / (104.0 * 1.2));
    const QSize size(qRound(128 * scale), qRound(104 * 1.2 * scale));
    return QRect(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}
void NativeTextViewport::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (!frame.isNull())
        painter.drawImage(displayRect(), frame);
}
} // namespace Dialogue
