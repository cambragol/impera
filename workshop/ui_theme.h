#pragma once
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace Workshop {
inline void applyTheme(QApplication &app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    p.setColor(QPalette::Window, QColor("#18202e"));
    p.setColor(QPalette::WindowText, QColor("#e5ebf5"));
    p.setColor(QPalette::Base, QColor("#101723"));
    p.setColor(QPalette::AlternateBase, QColor("#202b3a"));
    p.setColor(QPalette::Text, QColor("#e5ebf5"));
    p.setColor(QPalette::PlaceholderText, QColor("#a7b5cc"));
    p.setColor(QPalette::Button, QColor("#293549"));
    p.setColor(QPalette::ButtonText, QColor("#e5ebf5"));
    p.setColor(QPalette::Highlight, QColor("#527fc4"));
    p.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(p);
    app.setStyleSheet("QToolBar{spacing:8px;padding:8px;}QPushButton{padding:7px "
                      "12px;}QLineEdit,QComboBox,QSpinBox{padding:5px;}QTabBar::tab{padding:9px "
                      "14px;}QTreeWidget{border:0;}QSplitter::handle{background:#2d3a50;}QLineEdit:"
                      "focus,QComboBox:focus,QSpinBox:focus,QPushButton:focus,QCheckBox:focus{"
                      "border:1px solid #85b9ff;}");
}
} // namespace Workshop
