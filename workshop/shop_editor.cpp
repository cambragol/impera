#include "shop_editor.h"
#include <QtWidgets>
#include <QTextObjectInterface>
#include <algorithm>
using U5::require;
namespace {
constexpr int ValueObject = QTextFormat::UserObject + 81;
constexpr int ValueProperty = QTextFormat::UserProperty + 81;
constexpr int GroupProperty = QTextFormat::UserProperty + 82;
constexpr int RawProperty = QTextFormat::UserProperty + 83;
constexpr int DisplayProperty = QTextFormat::UserProperty + 84;
const QStringList valueNames{"Shop name", "Keeper name", "Time of day",  "Price",
                             "Quantity",  "Item name",   "Customer name"};
const char *symbols = "#$@%^&*";
QString normalized(QString text) { return text.replace("\r\n", "\n").replace('\r', '\n'); }
QString serviceName(QString label) {
    return label.section(" / ", 0, 0).section(" - ", 0, 0).section(':', 0, 0).trimmed();
}
QStringList services(const U5NarrativeEntry &entry) {
    QStringList result;
    for (auto label : QString::fromLatin1(entry.label).split(" / ")) {
        auto service = serviceName(label);
        if (!result.contains(service))
            result.append(service);
    }
    return result;
}
QString responseName(const U5NarrativeEntry &entry, const QString &service) {
    QStringList result;
    for (auto label : QString::fromLatin1(entry.label).split(" / "))
        if (serviceName(label) == service) {
            auto title = label.mid(service.size()).trimmed();
            if (title.startsWith(':') || title.startsWith('-'))
                title = title.mid(1).trimmed();
            if (!result.contains(title))
                result.append(title);
        }
    return result.join(" / ");
}
class ValueRenderer : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
  public:
    explicit ValueRenderer(QObject *parent) : QObject(parent) {}
    QSizeF intrinsicSize(QTextDocument *doc, int, const QTextFormat &format) override {
        auto font = doc->defaultFont();
        QFontMetricsF metrics(font);
        return {
            metrics.horizontalAdvance(valueNames.value(format.property(ValueProperty).toInt())) +
                16,
            metrics.height() + 4};
    }
    void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int,
                    const QTextFormat &format) override {
        p->save();
        p->setPen(QColor("#88b9e8"));
        p->setBrush(QColor("#294561"));
        p->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 3, 3);
        p->setFont(doc->defaultFont());
        p->setPen(QColor("#e0efff"));
        p->drawText(rect, Qt::AlignCenter,
                    valueNames.value(format.property(ValueProperty).toInt()));
        p->restore();
    }
};
void insertValue(QTextEdit *editor, int value) {
    QTextCharFormat format;
    format.setObjectType(ValueObject);
    format.setProperty(ValueProperty, value);
    auto cursor = editor->textCursor();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
    cursor.setCharFormat(QTextCharFormat());
    editor->setTextCursor(cursor);
}
QByteArray literal(QString text) {
    QByteArray out;
    for (auto c : text) {
        auto v = c.unicode();
        require(v == 10 || v == 13 || (v >= 32 && v <= 126),
                "Use ordinary ASCII text and inserted values.");
        require(!strchr(symbols, v), "Insert shop values using the insertion pane.");
        out.append(char(v));
    }
    return out;
}
} // namespace
namespace Workshop {
QStringList shopEntryServices(const U5NarrativeEntry &entry) { return services(entry); }
QStringList shopServices() {
    QStringList result;
    for (auto entry : narrativeEntries("SHOPPE.DAT"))
        for (auto service : services(*entry))
            if (!result.contains(service))
                result.append(service);
    result.sort(Qt::CaseInsensitive);
    return result;
}
static void appendShopText(QTextCursor &cursor, const QByteArray &bytes) {
    for (int i = 0; i < bytes.size();) {
        int v = uchar(bytes[i]);
        if (v < 128 && strchr(symbols, v)) {
            QTextCharFormat value;
            value.setObjectType(ValueObject);
            value.setProperty(ValueProperty, int(strchr(symbols, v) - symbols));
            cursor.insertText(QString(QChar::ObjectReplacementCharacter), value);
            cursor.setCharFormat(QTextCharFormat());
            ++i;
            continue;
        }
        QByteArray raw;
        QString display;
        if (v >= 128) {
            require(U5ShopPhrases[v - 128], "Invalid shop word");
            raw = bytes.mid(i, 1);
            display = " " + QString::fromLatin1(U5ShopPhrases[v - 128]);
            if (i + 1 < bytes.size() && uchar(bytes[i + 1]) < 128)
                display += ' ';
            ++i;
        } else {
            int start = i++;
            while (i < bytes.size() && uchar(bytes[i]) < 128 && !strchr(symbols, uchar(bytes[i])))
                ++i;
            raw = bytes.mid(start, i - start);
            display = normalized(QString::fromLatin1(raw));
        }
        QTextCharFormat format;
        format.setProperty(GroupProperty, QUuid::createUuid().toString());
        format.setProperty(RawProperty, raw);
        format.setProperty(DisplayProperty, display);
        cursor.insertText(display, format);
    }
}
void loadShopText(QTextEdit *editor, const QByteArray &bytes) {
    QSignalBlocker block(editor);
    editor->clear();
    QTextCursor cursor(editor->document());
    appendShopText(cursor, bytes);
    cursor.movePosition(QTextCursor::Start);
    cursor.setCharFormat(QTextCharFormat());
    editor->setTextCursor(cursor);
    editor->document()->setModified(false);
}
static QByteArray shopTextRange(const QTextDocument &doc, int start, int end) {
    struct Segment {
        QByteArray raw;
        QString expected, actual;
        bool phrase = false;
    };
    QVector<Segment> parts;
    int position = start;
    while (position < end) {
        QTextCursor cursor(const_cast<QTextDocument *>(&doc));
        cursor.setPosition(position);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        auto format = cursor.charFormat();
        QString text = cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');
        if (text == QString(QChar::ObjectReplacementCharacter)) {
            int value = format.property(ValueProperty).toInt();
            require(format.objectType() == ValueObject && value >= 0 && value < 7,
                    "Unsupported embedded value");
            parts.append({QByteArray(1, symbols[value]), text, text, false});
            ++position;
            continue;
        }
        const auto group = format.property(GroupProperty).toString();
        ++position;
        while (position < end && !group.isEmpty()) {
            cursor.setPosition(position);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            auto next = cursor.charFormat();
            if (next.objectType() == ValueObject ||
                next.property(GroupProperty).toString() != group)
                break;
            text += cursor.selectedText().replace(QChar::ParagraphSeparator, '\n');
            ++position;
        }
        auto raw = format.property(RawProperty).toByteArray();
        auto expected = format.property(DisplayProperty).toString();
        bool preserved = !group.isEmpty() && expected == text;
        parts.append({preserved ? raw : literal(text), expected, text,
                      preserved && raw.size() == 1 && uchar(raw[0]) >= 128});
    }
    QByteArray result;
    for (int i = 0; i < parts.size(); ++i) {
        auto &part = parts[i];
        if (part.phrase) {
            const bool space = i + 1 < parts.size() && !parts[i + 1].raw.isEmpty() &&
                               uchar(parts[i + 1].raw[0]) < 128;
            const auto display = " " +
                                 QString::fromLatin1(U5ShopPhrases[uchar(part.raw[0]) - 128]) +
                                 (space ? " " : "");
            result += display == part.actual ? part.raw : literal(part.actual);
        } else
            result += part.raw;
    }
    return result;
}
QByteArray shopTextBytes(const QTextDocument &doc) {
    return shopTextRange(doc, 0, doc.characterCount() - 1);
}
class ShopTextEdit : public QTextEdit {
  public:
    using QTextEdit::QTextEdit;

  protected:
    QMimeData *createMimeDataFromSelection() const override {
        auto mime = new QMimeData;
        mime->setText(textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n'));
        try {
            auto cursor = textCursor();
            mime->setData("application/x-impera-shop-text",
                          shopTextRange(*document(), cursor.selectionStart(), cursor.selectionEnd()));
        } catch (...) { /* Invalid drafts can still be copied as ordinary text. */
        }
        return mime;
    }
    void insertFromMimeData(const QMimeData *mime) override {
        if (mime->hasFormat("application/x-impera-shop-text")) {
            auto bytes = mime->data("application/x-impera-shop-text");
            auto terminated = bytes + char(0);
            const U5NarrativeEntry entry = {"SHOPPE.DAT", 0, "", ""};
            if (U5_NarrativeTextValid(
                    &entry, reinterpret_cast<const unsigned char *>(terminated.constData()),
                    terminated.size())) {
                auto cursor = textCursor();
                cursor.beginEditBlock();
                cursor.removeSelectedText();
                appendShopText(cursor, bytes);
                cursor.endEditBlock();
                cursor.setCharFormat(QTextCharFormat());
                setTextCursor(cursor);
                return;
            }
        }
        QTextEdit::insertFromMimeData(mime);
    }
};
QWidget *shopEditor(Project *project, TextCommit commit, QMap<QString, int> *state,
                    std::function<bool()> *flush) {
    validateNarrative(*project, "SHOPPE.DAT");
    auto root = new QWidget;
    root->setObjectName("shopDialogueWorkspace");
    auto layout = new QVBoxLayout(root);
    auto heading = new QLabel;
    heading->setObjectName("shopDialogueHeading");
    layout->addWidget(heading);
    auto scope = new QLabel;
    scope->setObjectName("shopDialogueScope");
    layout->addWidget(scope);
    auto split = new QSplitter;
    layout->addWidget(split, 1);
    auto left = new QWidget;
    auto leftLayout = new QVBoxLayout(left);
    auto search = new QLineEdit;
    search->setPlaceholderText("Find a response…");
    search->setToolTip("Find greetings, refusals or service responses.");
    leftLayout->addWidget(search);
    auto tree = new QTreeWidget;
    tree->setObjectName("shopResponses");
    tree->setHeaderHidden(true);
    leftLayout->addWidget(tree);
    split->addWidget(left);
    auto center = new QWidget;
    auto centerLayout = new QVBoxLayout(center);
    auto text = new ShopTextEdit;
    text->setObjectName("shopDialogueText");
    text->setAcceptRichText(false);
    text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    text->document()->documentLayout()->registerHandler(ValueObject, new ValueRenderer(text));
    centerLayout->addWidget(text, 1);
    auto status = new QLabel;
    status->setObjectName("shopDialogueStatus");
    status->setWordWrap(true);
    centerLayout->addWidget(status);
    split->addWidget(center);
    auto right = new QWidget;
    auto rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(new QLabel("Insert value"));
    for (int i = 0; i < valueNames.size(); ++i) {
        auto b = new QPushButton(valueNames[i]);
        b->setObjectName("shopInsertValue" + QString::number(i));
        b->setToolTip("Insert the " + valueNames[i].toLower() + " supplied by the game.");
        rightLayout->addWidget(b);
        QObject::connect(b, &QPushButton::clicked, root, [=] {
            insertValue(text, i);
            text->setFocus();
        });
    }
    rightLayout->addStretch();
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    split->setSizes({230, 700, 180});
    auto entries =
        std::make_shared<QVector<const U5NarrativeEntry *>>(narrativeEntries("SHOPPE.DAT"));
    auto current = std::make_shared<int>(-1);
    auto busy = std::make_shared<bool>(false);
    const auto serviceNames = shopServices();
    auto service = serviceNames.value(
        qBound(0, state->value("SHOPPE.DAT/service"), int(serviceNames.size() - 1)));
    auto save = std::make_shared<std::function<bool()>>();
    *save = [=] {
        if (*busy || *current < 0 || !text->document()->isModified())
            return true;
        try {
            auto entry = (*entries)[*current];
            auto bytes = shopTextBytes(*text->document());
            QByteArray terminated = bytes + char(0);
            require(U5_NarrativeTextValid(
                        entry, reinterpret_cast<const unsigned char *>(terminated.constData()),
                        terminated.size()),
                    "Response is too long or contains an unsupported value.");
            const char *values[7] = {"The Golden Ankh", "Julia", "morning", "100", "2",
                                     "Sword",           "Avatar"};
            char expanded[1500];
            require(
                U5_ExpandShopText(reinterpret_cast<const unsigned char *>(terminated.constData()),
                                  expanded, sizeof(expanded), values),
                "Expanded response is too long.");
            if (bytes != narrativeText(*project, *entry)) {
                *busy = true;
                bool ok = commit(
                    changeNarrative(*project, *entry, narrativeSource(bytes, true),
                                    bytes.size() + 1 >
                                        narrativeBudget(project->data("SHOPPE.DAT"), *entry)),
                    "Edit shop response");
                *busy = false;
                if (!ok)
                    return false;
            }
            text->document()->setModified(false);
            status->clear();
            return true;
        } catch (const std::exception &error) {
            *busy = false;
            status->setText(QString::fromUtf8(error.what()));
            status->setStyleSheet("color:#e07070");
            return false;
        }
    };
    if (flush)
        *flush = [save] { return (*save)(); };
    auto timer = new QTimer(root);
    timer->setSingleShot(true);
    timer->setInterval(500);
    QObject::connect(text, &QTextEdit::textChanged, root, [=] {
        if (!*busy)
            timer->start();
    });
    QObject::connect(timer, &QTimer::timeout, root, [save] { (*save)(); });
    QMap<QString, QTreeWidgetItem *> groups;
    for (int i = 0; i < entries->size(); ++i) {
        auto entry = (*entries)[i];
        if (!services(*entry).contains(service))
            continue;
        auto name = responseName(*entry, service);
        QString group = name.contains("Greeting", Qt::CaseInsensitive)   ? "Greetings"
                        : name.contains("Farewell", Qt::CaseInsensitive) ? "Farewells"
                        : name.contains("Refusal", Qt::CaseInsensitive)  ? "Refusals"
                        : service == "Equipment"                         ? "Item descriptions"
                                                                         : "Service responses";
        if (!groups.contains(group))
            groups[group] = new QTreeWidgetItem(tree, {group});
        auto item = new QTreeWidgetItem(groups[group], {name});
        item->setData(0, Qt::UserRole, i);
        item->setToolTip(0, QString::fromLatin1(entry->label));
    }
    tree->expandAll();
    QObject::connect(tree, &QTreeWidget::currentItemChanged, root,
                     [=](QTreeWidgetItem *item, QTreeWidgetItem *old) {
                         if (!item || !item->data(0, Qt::UserRole).isValid())
                             return;
                         if (!(*save)()) {
                             QSignalBlocker block(tree);
                             tree->setCurrentItem(old);
                             return;
                         }
                         *busy = true;
                         *current = item->data(0, Qt::UserRole).toInt();
                         (*state)["SHOPPE.DAT/narrative"] = *current;
                         auto entry = (*entries)[*current];
                         heading->setText(service + " — " + responseName(*entry, service));
                         scope->setText(service == "Equipment"
                                            ? "Shared item description"
                                            : "Shared " + service.toLower() + " dialogue");
                         scope->setToolTip(QString::fromLatin1(entry->label));
                         loadShopText(text, narrativeText(*project, *entry));
                         status->clear();
                         *busy = false;
                     });
    QObject::connect(search, &QLineEdit::textChanged, root, [=](const QString &query) {
        for (auto group : groups) {
            bool visible = false;
            for (int i = 0; i < group->childCount(); ++i) {
                auto item = group->child(i);
                bool match = item->text(0).contains(query, Qt::CaseInsensitive);
                item->setHidden(!match);
                visible |= match;
            }
            group->setHidden(!visible);
        }
    });
    QTreeWidgetItem *first = nullptr, *selected = nullptr;
    for (auto group : groups)
        for (int i = 0; i < group->childCount(); ++i) {
            auto item = group->child(i);
            if (!first)
                first = item;
            if (item->data(0, Qt::UserRole).toInt() == state->value("SHOPPE.DAT/narrative", -1))
                selected = item;
        }
    if (tree->topLevelItemCount() && tree->topLevelItem(0)->childCount())
        first = tree->topLevelItem(0)->child(0);
    tree->setCurrentItem(selected ? selected : first);
    return root;
}
} // namespace Workshop
#include "shop_editor.moc"
