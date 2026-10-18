#pragma once
#include "formats.h"
#include <QtWidgets>
#include <algorithm>
#include <functional>
#include <climits>

namespace Workshop {
inline QVector<QRgb> artworkColors(const U5::Graphics &graphics) {
    if (graphics.pngResource.isEmpty())
        return U5::palette();
    QSet<QRgb> unique;
    for (const auto &image : graphics.images)
        for (int y = 0; y < image.height(); y++)
            for (int x = 0; x < image.width(); x++)
                unique.insert(image.pixel(x, y));
    auto colors = unique.values().toVector();
    std::sort(colors.begin(), colors.end());
    return colors;
}
inline QRgb paletteColor(QRgb rgba, const QVector<QRgb> &colors) {
    if (!qAlpha(rgba) || colors.isEmpty())
        return 0;
    QRgb best = colors.first();
    int distance = INT_MAX;
    for (auto color : colors) {
        int r = qRed(rgba) - qRed(color), g = qGreen(rgba) - qGreen(color),
            b = qBlue(rgba) - qBlue(color);
        int next = r * r + g * g + b * b;
        if (next < distance) {
            distance = next;
            best = color;
        }
    }
    return qRgba(qRed(best), qGreen(best), qBlue(best), qAlpha(rgba));
}
inline QImage paletteImage(const QImage &source, const QVector<QRgb> &colors) {
    auto result = source.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < result.height(); y++)
        for (int x = 0; x < result.width(); x++)
            result.setPixel(x, y, paletteColor(result.pixel(x, y), colors));
    return result;
}
// Both artwork workspaces use the complete palette of the selected source sheet.
class ColorPalette : public QWidget {
    QGridLayout *grid;
    QVector<QToolButton *> swatches;
    QRgb selected = qRgb(255, 255, 255);
    int columns;

  public:
    std::function<void(QRgb)> changed;
    explicit ColorPalette(int columnCount, QWidget *parent = nullptr)
        : QWidget(parent), columns(columnCount) {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        auto scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setMaximumHeight(118);
        scroll->setMinimumHeight(78);
        auto content = new QWidget;
        grid = new QGridLayout(content);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        scroll->setWidget(content);
        layout->addWidget(scroll);
    }
    void select(QRgb rgba) {
        selected = rgba;
        for (auto b : swatches)
            b->setChecked(b->property("rgba").toUInt() == rgba);
    }
    void setColors(QVector<QRgb> colors, bool transparent) {
        while (auto item = grid->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        swatches.clear();
        if (transparent && !colors.contains(0))
            colors.append(0);
        int index = 0;
        for (QRgb rgba : colors) {
            auto b = new QToolButton;
            b->setCheckable(true);
            b->setFixedSize(30, 30);
            b->setProperty("rgba", quint32(rgba));
            b->setToolTip(
                qAlpha(rgba)
                    ? "Draw with this palette color (" + QColor::fromRgba(rgba).name() +
                          "). Black is opaque."
                    : "Draw transparent pixels to reveal the ground behind a transparent sprite.");
            b->setAccessibleName(b->toolTip());
            b->setStyleSheet(QString("QToolButton {background:%1;border:1px solid #8290a0;} "
                                     "QToolButton:checked {border:3px solid #ffffff;}")
                                 .arg(qAlpha(rgba) ? QColor::fromRgba(rgba).name() : "#333e4e"));
            if (!qAlpha(rgba))
                b->setText("×");
            grid->addWidget(b, index / columns, index % columns);
            index++;
            swatches.append(b);
            connect(b, &QToolButton::clicked, this, [=] {
                select(rgba);
                if (changed)
                    changed(rgba);
            });
        }
        select(selected);
    }
};
} // namespace Workshop
