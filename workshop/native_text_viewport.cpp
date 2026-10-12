#include "native_text_viewport.h"
#include "formats.h"
#include "text/game_frame.h"
#include <QPainter>
#include <QKeyEvent>
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
        if (self->recordGlyphs)
            self->rendered.append({x - self->window.left, y - self->window.top, offset / 8,
                                   font == reinterpret_cast<uint8_t *>(self->fonts[1].data())});
        IMPERA_TextGlyph(font, offset, f, b, x * 8, y * 8, host, pixel);
        if (self->observer)
            self->observer("glyph", offset / 8);
    };
    text.clear = [](void *host, int x1, int y1, int x2, int y2, uint8_t color) {
        for (int y = y1; y <= y2; ++y)
            for (int x = x1; x <= x2; ++x)
                pixel(host, x, y, color);
        auto *self = static_cast<NativeTextWindow *>(host);
        if (self->observer)
            self->observer("fill", 0);
    };
    text.scroll = [](void *host, int x1, int y1, int x2, int y2, int amount) {
        auto *self = static_cast<NativeTextWindow *>(host);
        IMPERA_TextScroll(reinterpret_cast<uint8_t *>(self->pixels.data()), 128, x1 - 192, y1 - 88,
                          x2 - 192, y2 - 88, amount, nullptr, nullptr);
        if (self->observer)
            self->observer("scroll", amount);
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
void NativeTextWindow::printString(const char *str) { IMPERA_TextString(&text, str); }
void NativeTextWindow::setSleeper(std::function<void(int)> fn) {
    sleeper = std::move(fn);
    text.speedFix = 1;
    text.sleep = [](void *host, int ms) {
        auto &self = *static_cast<NativeTextWindow *>(host);
        if (self.sleeper)
            self.sleeper(ms);
    };
}
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
    setFocusPolicy(Qt::StrongFocus);
}
void NativeTextViewport::setImage(const QImage &image) {
    U5::require(image.size() == QSize(128, 104), "Invalid native text framebuffer");
    frame = image;
    compose();
}
void NativeTextViewport::setConversationFrame(const QImage &image) {
    U5::require(image.size() == QSize(IMPERA_TALK_FRAME_WIDTH, IMPERA_TALK_FRAME_HEIGHT),
                "Invalid native conversation frame");
    backdrop = image;
    updateMinimumSize();
    compose();
}
void NativeTextViewport::compose() {
    presentation = backdrop.isNull() ? frame : backdrop.copy();
    if (!backdrop.isNull() && !frame.isNull()) {
        QPainter painter(&presentation);
        painter.drawImage(IMPERA_TALK_FRAME_TEXT_X, IMPERA_TALK_FRAME_TEXT_Y, frame);
    }
    update();
}
void NativeTextViewport::clear() {
    frame = QImage();
    compose();
}
QSize NativeTextViewport::logicalSize() const {
    return backdrop.isNull() ? QSize(128, 104) : backdrop.size();
}
void NativeTextViewport::setZoom(double value) {
    U5::require(value == 0 || (value >= 1 && value <= 5), "Invalid Playtest zoom");
    scale = value;
    updateMinimumSize();
    update();
}
void NativeTextViewport::updateMinimumSize() {
    const auto logical = logicalSize();
    double factor = scale > 0 ? scale : 1;
    setMinimumSize(qRound(logical.width() * factor), qRound(logical.height() * 1.2 * factor));
    updateGeometry();
}
QSize NativeTextViewport::sizeHint() const {
    const auto logical = logicalSize();
    double factor = scale > 0 ? scale : 3;
    return QSize(qRound(logical.width() * factor), qRound(logical.height() * 1.2 * factor));
}
QRect NativeTextViewport::displayRect() const {
    // One DOS pixel is 5:6. Resize presentation only, never the logical window.
    const auto logical = logicalSize();
    const double factor =
        scale > 0 ? scale
                  : qMin(width() / double(logical.width()), height() / (logical.height() * 1.2));
    const QSize size(qRound(logical.width() * factor), qRound(logical.height() * 1.2 * factor));
    return QRect(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}
void NativeTextViewport::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (!presentation.isNull())
        painter.drawImage(displayRect(), presentation);
}
void NativeTextViewport::keyPressEvent(QKeyEvent *event) {
    int code = 0;
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        code = 13;
        break;
    case Qt::Key_Backspace:
        code = 8;
        break;
    case Qt::Key_Escape:
        code = 27;
        break;
    case Qt::Key_Left:
        code = 257;
        break;
    case Qt::Key_Right:
        code = 258;
        break;
    case Qt::Key_Up:
        code = 259;
        break;
    case Qt::Key_Down:
        code = 260;
        break;
    case Qt::Key_Home:
        code = 256 | 211;
        break;
    case Qt::Key_End:
        code = 256 | 212;
        break;
    case Qt::Key_PageUp:
        code = 256 | 213;
        break;
    case Qt::Key_PageDown:
        code = 256 | 214;
        break;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        code = 9;
        break;
    default:
        if (event->text().size() == 1 && event->text()[0].unicode() >= 32 &&
            event->text()[0].unicode() < 127)
            code = event->text()[0].unicode();
    }
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_B:
            code = 2;
            break;
        case Qt::Key_E:
            code = 5;
            break;
        case Qt::Key_K:
            code = 11;
            break;
        case Qt::Key_M:
            code = 13;
            break;
        case Qt::Key_O:
            code = 15;
            break;
        case Qt::Key_S:
            code = 19;
            break;
        case Qt::Key_V:
            code = 22;
            break;
        default:
            code = 26;
            break;
        }
    }
    if (keyInput && code) {
        keyInput(code);
        event->accept();
    } else
        QWidget::keyPressEvent(event);
}
bool NativeTextViewport::event(QEvent *event) {
    if (keyInput && event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    if (keyInput && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
            keyPressEvent(key);
            return true;
        }
    }
    return QWidget::event(event);
}
} // namespace Dialogue
