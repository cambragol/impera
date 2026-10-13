#include "dialogue_surface.h"
namespace {
constexpr int TokenObject = QTextFormat::UserObject + 1;
constexpr int TokenId = QTextFormat::UserProperty + 1;
constexpr int TokenCode = QTextFormat::UserProperty + 2;
constexpr int TokenLabel = QTextFormat::UserProperty + 3;
QString tokenLabel(int code) { return code == 129 ? "Avatar name" : "Rune toggle"; }
QString spanKey(int before, int after) {
    return QString::number(before) + "/" + QString::number(after);
}
} // namespace
QSizeF DialogueTokenRenderer::intrinsicSize(QTextDocument *doc, int, const QTextFormat &format) {
    QFontMetricsF metrics(doc->defaultFont());
    return {metrics.horizontalAdvance((format.hasProperty(TokenLabel)
                                           ? format.property(TokenLabel).toString()
                                           : tokenLabel(format.property(TokenCode).toInt()))) +
                16,
            metrics.height() + 4};
}
void DialogueTokenRenderer::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc,
                                       int, const QTextFormat &format) {
    painter->save();
    painter->setPen(QColor(115, 150, 185));
    painter->setBrush(QColor(42, 61, 82));
    painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 4, 4);
    painter->setFont(doc->defaultFont());
    painter->setPen(Qt::white);
    painter->drawText(rect, Qt::AlignCenter,
                      (format.hasProperty(TokenLabel)
                           ? format.property(TokenLabel).toString()
                           : tokenLabel(format.property(TokenCode).toInt())));
    painter->restore();
}
DialogueTextSurface::DialogueTextSurface(const QVector<Dialogue::Block> &blocks, QWidget *parent)
    : QTextEdit(parent) {
    setObjectName("conversationInlineText");
    setAcceptRichText(false);
    setAcceptDrops(false);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTabChangesFocus(true);
    setAccessibleName("Speech with protected Avatar-name and rune controls");
    auto renderer = new DialogueTokenRenderer(this);
    document()->documentLayout()->registerHandler(TokenObject, renderer);
    loading = true;
    QTextCursor cursor(document());
    QByteArray textBytes;
    int before = -1;
    auto appendText = [&](int after) {
        const QString text = Dialogue::plain(textBytes);
        originalSpans.insert(spanKey(before, after), {text, textBytes});
        cursor.insertText(text, QTextCharFormat());
        textBytes.clear();
    };
    for (const auto &block : blocks) {
        if (block.opcode < 0)
            textBytes += block.bytes;
        else {
            const int id = nextToken++;
            appendText(id);
            tokens[id] = block.bytes;
            expectedTokens.append(id);
            insertTokenObject(cursor, id, block.opcode);
            before = id;
        }
    }
    appendText(-1);
    setUndoRedoEnabled(false); // Workshop owns document undo across actions and paragraphs.
    refreshFormatting();
    validDocument = QTextDocumentFragment(document());
    loading = false;
    connect(this, &QTextEdit::textChanged, this, [this] { checkEdit(); });
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this,
            [this] { fit(); });
    connect(this, &QTextEdit::cursorPositionChanged, this, [this] {
        if (textCursor().hasSelection())
            return;
        auto cursor = textCursor();
        cursor.setCharFormat(QTextCharFormat());
        setTextCursor(cursor);
    });
    fit();
}
void DialogueTextSurface::insertTokenObject(QTextCursor &cursor, int id, int opcode) {
    QTextCharFormat format;
    format.setObjectType(TokenObject);
    format.setProperty(TokenId, id);
    format.setProperty(TokenCode, opcode);
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
    cursor.setCharFormat(QTextCharFormat());
}
QByteArray DialogueTextSurface::bytes() const {
    QString text;
    QByteArray result;
    QVector<int> found;
    int before = -1;
    auto appendText = [&](int after) {
        const auto original = originalSpans.constFind(spanKey(before, after));
        result += original != originalSpans.cend() && original->text == text ? original->bytes
                                                                             : Dialogue::text(text);
        text.clear();
    };
    for (int position = 0; position < document()->characterCount() - 1; ++position) {
        QTextCursor cursor(document());
        cursor.setPosition(position);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        const auto ch = document()->characterAt(position);
        if (ch == QChar::ObjectReplacementCharacter) {
            const int id = cursor.charFormat().property(TokenId).toInt();
            U5::require(cursor.charFormat().objectType() == TokenObject && tokens.contains(id),
                        "Invalid inline dialogue control");
            appendText(id);
            result += tokens[id];
            found.append(id);
            before = id;
        } else
            text += ch == QChar::ParagraphSeparator ? QChar('\n') : ch;
    }
    appendText(-1);
    U5::require(found == expectedTokens,
                "Use the token menu to remove or insert dialogue controls");
    return result;
}
void DialogueTextSurface::checkEdit() {
    if (loading)
        return;
    // Protect control objects even from cut, IME, accessibility or programmatic
    // document edits. Invalid ASCII is retained as a visible draft for validation.
    QVector<int> found;
    bool valid = true;
    for (int position = 0; position < document()->characterCount() - 1; ++position)
        if (document()->characterAt(position) == QChar::ObjectReplacementCharacter) {
            QTextCursor cursor(document());
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            const int id = cursor.charFormat().property(TokenId).toInt();
            valid &= cursor.charFormat().objectType() == TokenObject && tokens.contains(id) &&
                     cursor.charFormat().property(TokenCode).toInt() ==
                         (tokens.contains(id) ? int(U5::byte(tokens[id], 0)) : -1);
            found.append(cursor.charFormat().property(TokenId).toInt());
        }
    if (!valid || found != expectedTokens) {
        const int position = textCursor().position();
        loading = true;
        QTextCursor restore(document());
        restore.select(QTextCursor::Document);
        restore.insertFragment(validDocument);
        QTextCursor cursor(document());
        cursor.setPosition(qMin(position, document()->characterCount() - 1));
        setTextCursor(cursor);
        loading = false;
        return;
    }
    loading = true;
    refreshFormatting();
    loading = false;
    validDocument = QTextDocumentFragment(document());
    fit();
    if (edited)
        edited();
}
void DialogueTextSurface::refreshFormatting() {
    bool runic = false;
    int begin = 0;
    auto formatSpan = [&](int end) {
        if (end <= begin)
            return;
        QTextCursor cursor(document());
        cursor.setPosition(begin);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        QTextCharFormat format;
        format.setFontItalic(runic);
        format.setForeground(runic ? QColor(105, 181, 145) : palette().color(QPalette::Text));
        cursor.mergeCharFormat(format);
    };
    for (int i = 0; i < document()->characterCount() - 1; ++i) {
        if (document()->characterAt(i) != QChar::ObjectReplacementCharacter)
            continue;
        formatSpan(i);
        QTextCursor cursor(document());
        cursor.setPosition(i);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        const int code = cursor.charFormat().property(TokenCode).toInt();
        if (code == 142) {
            runic = !runic;
            QTextCharFormat format;
            format.setProperty(TokenLabel, runic ? "Begin runes" : "End runes");
            cursor.mergeCharFormat(format);
        }
        begin = i + 1;
    }
    formatSpan(document()->characterCount() - 1);
}
void DialogueTextSurface::insertToken(int opcode) {
    U5::require(opcode == 129 || opcode == 142, "Unsupported inline control");
    if (selectionContainsToken())
        return;
    auto cursor = textCursor();
    int ordinal = 0;
    for (int i = 0; i < cursor.selectionStart(); ++i)
        ordinal += document()->characterAt(i) == QChar::ObjectReplacementCharacter;
    const int id = nextToken++;
    tokens[id] = Dialogue::action(opcode);
    expectedTokens.insert(ordinal, id);
    loading = true;
    cursor.removeSelectedText();
    insertTokenObject(cursor, id, opcode);
    setTextCursor(cursor);
    loading = false;
    checkEdit();
}
bool DialogueTextSurface::selectionContainsToken() const {
    auto cursor = textCursor();
    for (int i = cursor.selectionStart(); i < cursor.selectionEnd(); ++i)
        if (document()->characterAt(i) == QChar::ObjectReplacementCharacter)
            return true;
    return false;
}
void DialogueTextSurface::keyPressEvent(QKeyEvent *event) {
    const auto cursor = textCursor();
    const bool modifies = event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete ||
                          !event->text().isEmpty() || event->matches(QKeySequence::Cut) ||
                          event->matches(QKeySequence::Paste);
    if (modifies && selectionContainsToken() && !event->matches(QKeySequence::Copy)) {
        event->accept();
        return;
    }
    if (!cursor.hasSelection() &&
        ((event->key() == Qt::Key_Backspace && cursor.position() > 0 &&
          document()->characterAt(cursor.position() - 1) == QChar::ObjectReplacementCharacter) ||
         (event->key() == Qt::Key_Delete &&
          document()->characterAt(cursor.position()) == QChar::ObjectReplacementCharacter))) {
        event->accept();
        return;
    }
    QTextEdit::keyPressEvent(event);
}
QMimeData *DialogueTextSurface::createMimeDataFromSelection() const {
    auto data = new QMimeData;
    QString text;
    const auto selected = textCursor();
    for (int position = selected.selectionStart(); position < selected.selectionEnd(); ++position) {
        const auto ch = document()->characterAt(position);
        if (ch == QChar::ObjectReplacementCharacter) {
            QTextCursor cursor(document());
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            text += "[" + tokenLabel(cursor.charFormat().property(TokenCode).toInt()) + "]";
        } else
            text += ch == QChar::ParagraphSeparator ? QChar('\n') : ch;
    }
    data->setText(text); // External paste is always speech, never executable control tags.
    return data;
}
void DialogueTextSurface::insertFromMimeData(const QMimeData *source) {
    if (!selectionContainsToken())
        insertPlainText(source->text());
}
void DialogueTextSurface::contextMenuEvent(QContextMenuEvent *event) {
    auto menu = createStandardContextMenu();
    auto insert = menu->addMenu("Insert inline control");
    insert->addAction("Avatar name", this, [this] { insertToken(129); });
    insert->addAction("Rune toggle", this, [this] { insertToken(142); });
    auto cursor = cursorForPosition(event->pos());
    int position = cursor.position();
    if (document()->characterAt(position) != QChar::ObjectReplacementCharacter && position > 0)
        --position;
    if (document()->characterAt(position) == QChar::ObjectReplacementCharacter) {
        cursor.setPosition(position);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        const int id = cursor.charFormat().property(TokenId).toInt();
        menu->addAction("Remove this inline control", this, [this, cursor, id]() mutable {
            expectedTokens.removeOne(id);
            loading = true;
            cursor.removeSelectedText();
            loading = false;
            checkEdit();
        });
    }
    menu->exec(event->globalPos());
    delete menu;
}
void DialogueTextSurface::fit() {
    setFixedHeight(qMin(16384, qMax(42, qCeil(document()->size().height()) + 6)));
}
void DialogueTextSurface::resizeEvent(QResizeEvent *event) {
    QTextEdit::resizeEvent(event);
    fit();
}
void DialogueTextSurface::focusInEvent(QFocusEvent *event) {
    QTextEdit::focusInEvent(event);
    if (activated)
        activated();
}
