#pragma once
#include "formats.h"
#include <QtWidgets>
#include <algorithm>
#include <functional>

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
// Both artwork workspaces use the complete palette of the selected source sheet.
class ColorPalette : public QWidget {
    QGridLayout *grid;
    QVector<QToolButton *> swatches;
    QToolButton *custom = nullptr;
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
        if (custom)
            custom->setStyleSheet(QString("background:%1;border:1px solid #8290a0;")
                                      .arg(QColor::fromRgba(rgba).name()));
    }
    void setColors(QVector<QRgb> colors, bool transparent, bool customColors) {
        while (auto item = grid->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        swatches.clear();
        custom = nullptr;
        if (transparent && !colors.contains(0))
            colors.append(0);
        int index = 0;
        for (QRgb rgba : colors) {
            auto b = new QToolButton;
            b->setCheckable(true);
            b->setFixedSize(30, 30);
            b->setProperty("rgba", quint32(rgba));
            b->setToolTip(qAlpha(rgba) ? QColor::fromRgba(rgba).name() : "Transparent");
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
        if (customColors) {
            custom = new QToolButton;
            custom->setFixedSize(30, 30);
            custom->setText("…");
            custom->setToolTip("Choose a custom color.");
            grid->addWidget(custom, index / columns, index % columns);
            connect(custom, &QToolButton::clicked, this, [this, transparent] {
                auto c = QColorDialog::getColor(QColor::fromRgba(selected), this, "Brush color",
                                                transparent ? QColorDialog::ShowAlphaChannel
                                                            : QColorDialog::ColorDialogOptions{});
                if (c.isValid()) {
                    select(c.rgba());
                    if (changed)
                        changed(c.rgba());
                }
            });
        }
        select(selected);
    }
};
} // namespace Workshop
