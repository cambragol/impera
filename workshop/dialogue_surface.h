#pragma once
#include "dialogue.h"
#include <QtWidgets>
#include <functional>
// Protected inline objects use the document's native caret/selection geometry.
class DialogueTokenRenderer : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
  public:
    using QObject::QObject;
    QSizeF intrinsicSize(QTextDocument *, int, const QTextFormat &) override;
    void drawObject(QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &) override;
};
class DialogueTextSurface : public QTextEdit {
    Q_OBJECT
  public:
    DialogueTextSurface(const QVector<Dialogue::Block> &, QWidget *parent = nullptr);
    QByteArray bytes() const;
    void insertToken(int opcode);
    std::function<void()> edited, activated;

  protected:
    void resizeEvent(QResizeEvent *) override;
    void focusInEvent(QFocusEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;
    QMimeData *createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData *) override;

  private:
    struct Span {
        QString text;
        QByteArray bytes;
    };
    QMap<QString, Span> originalSpans;
    QMap<int, QByteArray> tokens;
    QVector<int> expectedTokens;
    int nextToken = 0;
    bool loading = false;
    QTextDocumentFragment validDocument;
    bool selectionContainsToken() const;
    void checkEdit();
    void refreshFormatting();
    void fit();
    void insertTokenObject(QTextCursor &, int id, int opcode);
};
