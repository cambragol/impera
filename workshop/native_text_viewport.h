#pragma once
#include <QImage>
#include <QWidget>
#include <QVector>
#include <functional>
#include "text/text_window.h"
namespace Dialogue {
struct PreviewGlyph {
    int x = 0, y = 0, character = 0;
    bool runic = false;
};
/* Session-local fonts and native indexed pixels. No project data is mutated. */
class NativeTextWindow {
  public:
    NativeTextWindow(const QByteArray &normal, const QByteArray &runes, int row = 12);
    void print(uint16_t ch);
    void printString(const char *);
    void setPosition(int x, int y) { IMPERA_TextPosition(&window, x, y); }
    void setRows(int rows) { window.bottom = uint8_t(window.top + rows - 1); }
    uint16_t *advanceFlag() { return &advance; }
    void setSleeper(std::function<void(int)> fn);
    void setObserver(std::function<void(const QString &, int)> fn) { observer = std::move(fn); }
    void append(uint8_t ch);
    void flush();
    void backspace(int count);
    void paintCursor(uint8_t ch);
    void selectFont(int index);
    QImage image() const;
    const QByteArray &indexedPixels() const { return pixels; }
    const TextWindow &cursor() const { return window; }
    const QVector<PreviewGlyph> &glyphs() const { return rendered; }
    void setRecordGlyphs(bool enabled) { recordGlyphs = enabled; }
    NativeTextWindow(const NativeTextWindow &) = delete;
    NativeTextWindow &operator=(const NativeTextWindow &) = delete;

  private:
    TextWindow window{}, *current = &window;
    QByteArray fonts[2], pixels;
    uint8_t *charset = nullptr, saved[24]{}, word[16]{}, length = 0, lastColumn = 0;
    uint16_t advance = 1, underline = 0, center = 0, inverse = 0;
    uint8_t fg = 15, bg = 0;
    ImperaText text{};
    ImperaWords words{};
    QVector<PreviewGlyph> rendered;
    bool recordGlyphs = true;
    std::function<void(int)> sleeper;
    std::function<void(const QString &, int)> observer;
    static void pixel(void *, int, int, uint8_t);
};
class NativeTextViewport : public QWidget {
    Q_OBJECT
  public:
    explicit NativeTextViewport(QWidget *parent = nullptr);
    void setImage(const QImage &image);
    void setConversationFrame(const QImage &);
    void setZoom(double scale); // Zero fits the available area; positive values never reflow.
    double zoom() const { return scale; }
    const QImage &logicalImage() const { return presentation; }
    void clear();
    QSize sizeHint() const override;
    QRect displayRect() const;
    std::function<void(int)> keyInput;

  protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool event(QEvent *) override;

  private:
    QImage frame, backdrop, presentation;
    double scale = 0;
    void compose();
    void updateMinimumSize();
    QSize logicalSize() const;
};
} // namespace Dialogue
