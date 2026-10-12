#pragma once
#include <QImage>
#include <QWidget>
#include <QVector>
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
    void append(uint8_t ch);
    void flush();
    void backspace(int count);
    void paintCursor(uint8_t ch);
    void selectFont(int index);
    QImage image() const;
    const QByteArray &indexedPixels() const { return pixels; }
    const TextWindow &cursor() const { return window; }
    const QVector<PreviewGlyph> &glyphs() const { return rendered; }
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
    static void pixel(void *, int, int, uint8_t);
};
class NativeTextViewport : public QWidget {
  public:
    explicit NativeTextViewport(QWidget *parent = nullptr);
    void setImage(const QImage &image);
    void clear();
    QSize sizeHint() const override;
    QRect displayRect() const;

  protected:
    void paintEvent(QPaintEvent *) override;

  private:
    QImage frame;
};
} // namespace Dialogue
