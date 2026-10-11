#include "narrative_editor.h"
#include "text_preview.h"
#include <QtWidgets>
#include <memory>
using U5::require;
namespace Workshop {
QVector<const U5NarrativeEntry *> narrativeEntries(const QString &resource) {
    QVector<const U5NarrativeEntry *> result;
    for (const auto &entry : U5NarrativeEntries)
        if (resource == entry.resource)
            result.append(&entry);
    return result;
}
static QByteArray originalText(const QByteArray &bytes, const U5NarrativeEntry &entry) {
    require(entry.offset < unsigned(bytes.size()), "Narrative entry is outside the resource");
    int end = bytes.indexOf('\0', entry.offset);
    require(end >= 0, "Narrative entry has no terminator");
    auto text = bytes.mid(entry.offset, end - entry.offset);
    auto terminated = text + char(0);
    require(U5_NarrativeTextValid(&entry,
                                  reinterpret_cast<const unsigned char *>(terminated.constData()),
                                  terminated.size()),
            "Narrative entry has unsupported characters, tokens or length");
    return text;
}
static QMap<QString, QByteArray> overrides(const Project &project) {
    QMap<QString, QByteArray> out;
    if (project.data("TEXT.OVR").isEmpty())
        return out;
    const auto &b = project.data("TEXT.OVR");
    int valid;
    U5_FindNarrativeText(reinterpret_cast<const unsigned char *>(b.constData()), b.size(), nullptr,
                         &valid);
    require(valid, "Invalid or unsupported narrative overrides");
    for (const auto &entry : U5NarrativeEntries) {
        const char *text = U5_FindNarrativeText(
            reinterpret_cast<const unsigned char *>(b.constData()), b.size(), entry.id, &valid);
        if (text)
            out.insert(entry.id, QByteArray(text));
    }
    return out;
}
QByteArray narrativeText(const Project &project, const U5NarrativeEntry &entry) {
    auto map = overrides(project);
    return map.contains(entry.id) ? map[entry.id]
                                  : originalText(project.data(entry.resource), entry);
}
int narrativeBudget(const QByteArray &bytes, const U5NarrativeEntry &entry) {
    auto text = originalText(bytes, entry);
    int end = entry.offset + text.size() + 1, limit = bytes.size();
    for (auto next : narrativeEntries(entry.resource))
        if (next->offset > entry.offset)
            limit = qMin(limit, int(next->offset));
    // Expand only into verified zero padding. Unidentified non-prose bytes remain untouched.
    while (end < limit && bytes[end] == 0)
        ++end;
    return qMin(end - int(entry.offset), int(U5_NarrativeLimit(&entry)));
}
static const QStringList tags = {"shop", "keeper", "time", "price", "quantity", "item", "customer"};
QString narrativeSource(const QByteArray &bytes, bool shop) {
    QString out;
    const char *symbols = "#$@%^&*";
    for (auto c : bytes) {
        unsigned v = uchar(c);
        if (shop && v >= 128)
            out += QString("[word:%1]").arg(v, 2, 16, QChar('0'));
        else if (shop && strchr(symbols, v))
            out += "[" + tags[strchr(symbols, v) - symbols] + "]";
        else if (c == '[')
            out += "[[";
        else
            out += QChar(v);
    }
    return out;
}
QByteArray encodeNarrative(const QString &text, bool shop) {
    QByteArray out;
    for (int i = 0; i < text.size(); ++i) {
        if (text.mid(i, 2) == "[[") {
            out += '[';
            ++i;
            continue;
        }
        if (shop && text[i] == '[') {
            int end = text.indexOf(']', i);
            require(end > i, "Unclosed substitution token");
            auto tag = text.mid(i + 1, end - i - 1);
            int index = tags.indexOf(tag);
            if (index >= 0)
                out += "#$@%^&*"[index];
            else {
                require(tag.startsWith("word:") && tag.size() == 7, "Unknown shop token");
                bool ok;
                unsigned v = tag.mid(5).toUInt(&ok, 16);
                require(ok && v >= 128 && v <= 255 && U5ShopPhrases[v - 128],
                        "Unknown phrase token");
                out += char(v);
            }
            i = end;
            continue;
        }
        auto c = text[i].unicode();
        require(c == 10 || c == 13 || (c >= 32 && c <= 126),
                "Use DOS ASCII text and supported substitution tokens");
        out += char(c);
    }
    return out;
}
static void appendWord(QByteArray &b, unsigned value) {
    for (int i = 0; i < 4; ++i)
        b += char(value >> (i * 8));
}
QMap<QString, QByteArray> changeNarrative(const Project &project, const U5NarrativeEntry &entry,
                                          const QString &source, bool extended) {
    auto text = encodeNarrative(source, strcmp(entry.resource, "SHOPPE.DAT") == 0);
    auto terminated = text + char(0);
    require(U5_NarrativeTextValid(&entry,
                                  reinterpret_cast<const unsigned char *>(terminated.constData()),
                                  terminated.size()),
            "Text exceeds the runtime limit or contains invalid tokens");
    auto map = overrides(project);
    QMap<QString, QByteArray> changes;
    if (extended)
        map[entry.id] = text;
    else {
        auto bytes = project.data(entry.resource);
        int budget = narrativeBudget(bytes, entry);
        require(terminated.size() <= budget,
                QString("This entry allows %1 bytes including its terminator. Shorten it or enable "
                        "extended text.")
                    .arg(budget));
        // Clear only the previous text; all padding and opaque data outside it are preserved.
        int oldLength = originalText(bytes, entry).size() + 1;
        bytes.replace(entry.offset, qMax(oldLength, int(terminated.size())),
                      terminated + QByteArray(qMax(0, oldLength - int(terminated.size())), 0));
        changes[entry.resource] = bytes;
        map.remove(entry.id);
    }
    QByteArray companion("ITXT0001", 8);
    appendWord(companion, map.size());
    for (auto it = map.begin(); it != map.end(); ++it) {
        auto id = it.key().toLatin1();
        appendWord(companion, id.size());
        companion += id;
        appendWord(companion, it.value().size() + 1);
        companion += it.value();
        companion += char(0);
    }
    // Keep a valid empty companion after removing the final override; package version 2 signals
    // compatibility.
    if (!map.isEmpty() || !project.data("TEXT.OVR").isEmpty())
        changes["TEXT.OVR"] = companion;
    return changes;
}
void validateNarrative(const Project &project, const QString &resource) {
    if (resource == "TEXT.OVR") {
        const auto map = overrides(project);
        for (const auto &entry : U5NarrativeEntries)
            if (map.contains(entry.id))
                require(project.resources.contains(entry.resource),
                        "Narrative override refers to an absent source document");
    } else {
        for (auto entry : narrativeEntries(resource)) {
            auto text = originalText(project.data(resource), *entry);
            int boundary = project.data(resource).size();
            for (auto next : narrativeEntries(resource))
                if (next->offset > entry->offset)
                    boundary = qMin(boundary, int(next->offset));
            require(int(entry->offset) + text.size() < boundary,
                    "Narrative entry overlaps the next fixed entry");
        }
    }
}
QWidget *narrativeEditor(Project *project, const QString &resource, TextCommit commit,
                         QMap<QString, int> *state, std::function<bool()> *flush) {
    validateNarrative(*project, resource);
    auto root = new QWidget;
    root->setObjectName("narrativeWorkspace");
    auto layout = new QVBoxLayout(root);
    auto intro = new QLabel("Edit the story text; engine rules and character outcomes remain "
                            "read-only. Unidentified data is preserved in Advanced details.");
    intro->setWordWrap(true);
    layout->addWidget(intro);
    auto filter = new QLineEdit;
    filter->setPlaceholderText("Find an entry or service…");
    filter->setToolTip("Filter by virtue, page, shop service or message context.");
    layout->addWidget(filter);
    auto split = new QSplitter;
    layout->addWidget(split, 1);
    auto list = new QListWidget;
    list->setObjectName("narrativeEntries");
    list->setToolTip(
        "Choose the text used by this scene. Shared shop entries list all their contexts.");
    split->addWidget(list);
    auto right = new QWidget;
    auto form = new QVBoxLayout(right);
    split->addWidget(right);
    split->setStretchFactor(1, 2);
    auto context = new QLabel;
    context->setWordWrap(true);
    form->addWidget(context);
    auto karmaRow = new QWidget;
    auto karmaLayout = new QHBoxLayout(karmaRow);
    karmaLayout->setContentsMargins(0, 0, 0, 0);
    auto speaker = new QComboBox;
    speaker->addItems({"Lord British's dream", "Blackthorn"});
    speaker->setToolTip(
        "These speakers share the lower four karma speeches; their highest speech differs.");
    auto karma = new QSpinBox;
    karma->setRange(0, 100);
    karma->setToolTip("Select the speech the engine would use for this sample karma value.");
    karmaLayout->addWidget(new QLabel("Speaker:"));
    karmaLayout->addWidget(speaker);
    karmaLayout->addWidget(new QLabel("Sample karma:"));
    karmaLayout->addWidget(karma);
    form->addWidget(karmaRow);
    karmaRow->setVisible(resource == "KARMA.DAT");
    auto text = new QPlainTextEdit;
    text->setObjectName("narrativeText");
    text->setMinimumHeight(140);
    text->setToolTip("Edit ASCII text. Shop tokens insert runtime values; [[ produces a literal "
                     "opening bracket.");
    form->addWidget(text, 1);
    bool shop = resource == "SHOPPE.DAT";
    auto tokenRow = new QWidget;
    auto tokenLayout = new QHBoxLayout(tokenRow);
    tokenLayout->setContentsMargins(0, 0, 0, 0);
    for (auto tag : tags) {
        auto b = new QPushButton(tag);
        b->setToolTip("Insert the " + tag + " value supplied by the game.");
        tokenLayout->addWidget(b);
        QObject::connect(b, &QPushButton::clicked, root,
                         [=] { text->insertPlainText("[" + tag + "]"); });
    }
    form->addWidget(tokenRow);
    tokenRow->setVisible(shop);
    auto phraseRow = new QWidget;
    auto phraseLayout = new QHBoxLayout(phraseRow);
    phraseLayout->setContentsMargins(0, 0, 0, 0);
    auto phrase = new QComboBox;
    phrase->setToolTip("Original compressed shop words. Insert one to save bytes while preserving "
                       "the engine’s spacing rules.");
    for (unsigned i = 0; i < sizeof(U5ShopPhrases) / sizeof(U5ShopPhrases[0]); ++i)
        if (U5ShopPhrases[i])
            phrase->addItem(QString::fromLatin1(U5ShopPhrases[i]), int(i + 128));
    auto insertPhrase = new QPushButton("Insert word");
    insertPhrase->setToolTip("Insert the selected compressed word token at the text cursor.");
    phraseLayout->addWidget(new QLabel("Compressed word:"));
    phraseLayout->addWidget(phrase, 1);
    phraseLayout->addWidget(insertPhrase);
    form->addWidget(phraseRow);
    phraseRow->setVisible(shop);
    QObject::connect(insertPhrase, &QPushButton::clicked, root, [=] {
        text->insertPlainText(
            QString("[word:%1]").arg(phrase->currentData().toInt(), 2, 16, QChar('0')));
    });
    auto samples = new QWidget;
    auto samplesLayout = new QFormLayout(samples);
    auto values = std::make_shared<QVector<QLineEdit *>>();
    QStringList defaults = {"The Golden Ankh", "Julia", "morning", "100", "2", "Sword", "Avatar"};
    for (int i = 0; i < tags.size(); ++i) {
        auto field = new QLineEdit(defaults[i]);
        field->setToolTip("Preview value for [" + tags[i] +
                          "]. The runtime supplies the actual value.");
        samplesLayout->addRow(tags[i] + ":", field);
        values->append(field);
    }
    auto samplesBox = new QGroupBox("Sample shop values");
    auto samplesBoxLayout = new QVBoxLayout(samplesBox);
    samplesBoxLayout->addWidget(samples);
    samplesBox->setCheckable(true);
    samplesBox->setChecked(false);
    samples->hide();
    QObject::connect(samplesBox, &QGroupBox::toggled, samples, &QWidget::setVisible);
    form->addWidget(samplesBox);
    samplesBox->setVisible(shop);
    auto extended = new QCheckBox("Extended text (requires narrative-enabled Impera)");
    extended->setObjectName("narrativeExtended");
    extended->setToolTip(
        "Store text in a versioned mod companion without moving the original DAT entry. Newer "
        "Impera checks it before the original text. Older engines reject this package.");
    form->addWidget(extended);
    auto budget = new QLabel;
    budget->setWordWrap(true);
    form->addWidget(budget);
    auto artwork = new QLabel;
    artwork->setAlignment(Qt::AlignCenter);
    artwork->setToolTip("Original ending artwork, positioned by the engine's page tables. Text "
                        "preview below is approximate; validate the final scene in Impera.");
    form->addWidget(artwork);
    artwork->setVisible(resource == "END.DAT");
    auto previewInput = new QPlainTextEdit(root);
    previewInput->hide();
    previewInput->setProperty("sceneHeight", 200);
    auto preview = textPreviewPanel(project, previewInput,
                                    shop || resource == "KARMA.DAT" || resource == "MISCMSG.DAT" ||
                                            resource == "ENDMSG.DAT"
                                        ? 144
                                        : 320);
    form->addWidget(preview);
    if (resource == "QUESTION.DAT" || resource == "END.DAT") {
        auto mode = preview->findChild<QComboBox *>();
        if (mode)
            mode->setCurrentIndex(2);
    }
    auto apply = new QPushButton("Apply text");
    apply->setObjectName("applyNarrative");
    apply->setToolTip("Validate and save this entry as one undoable change. No text is truncated.");
    form->addWidget(apply);
    auto entries = std::make_shared<QVector<const U5NarrativeEntry *>>(narrativeEntries(resource));
    auto current = std::make_shared<int>(-1);
    auto busy = std::make_shared<bool>(false);
    auto update = [=] {
        if (*current < 0)
            return;
        auto entry = (*entries)[*current];
        try {
            auto bytes = encodeNarrative(text->toPlainText(), shop);
            int limit = extended->isChecked() ? int(U5_NarrativeLimit(entry))
                                              : narrativeBudget(project->data(resource), *entry);
            budget->setText(QString("%1 / %2 bytes including terminator. %3")
                                .arg(bytes.size() + 1)
                                .arg(limit)
                                .arg(bytes.size() + 1 > limit
                                         ? "Too long: shorten the text or enable extended text."
                                         : "Layout overflow is separate from this byte limit."));
            budget->setStyleSheet(bytes.size() + 1 > limit ? "color:#e07070" : "");
            apply->setEnabled(bytes.size() + 1 <= limit);
            QString rendered = QString::fromLatin1(bytes);
            if (shop) {
                QVector<QByteArray> storage;
                const char *sampleValues[7];
                for (auto field : *values)
                    storage.append(field->text().toLatin1());
                for (int i = 0; i < 7; ++i)
                    sampleValues[i] = storage[i].constData();
                char out[1500];
                require(
                    U5_ExpandShopText(reinterpret_cast<const unsigned char *>(bytes.constData()),
                                      out, sizeof(out), sampleValues),
                    "Expanded shop preview exceeds the 1500-byte runtime output limit");
                rendered = QString::fromLatin1(out);
            }
            previewInput->setPlainText(rendered);
        } catch (const std::exception &e) {
            budget->setText(QString::fromUtf8(e.what()));
            budget->setStyleSheet("color:#e07070");
            apply->setEnabled(false);
        }
    };
    auto caption = [=](const U5NarrativeEntry *entry) {
        QString label = QString::fromLatin1(entry->label);
        if (resource == "MISCMSG.DAT" || resource == "ENDMSG.DAT") {
            auto snippet = QString::fromLatin1(narrativeText(*project, *entry)).simplified();
            if (!snippet.isEmpty())
                label += "\n" + snippet.left(55) + (snippet.size() > 55 ? "…" : "");
        }
        return label;
    };
    auto save = [=]() -> bool {
        if (*current < 0 || !text->document()->isModified())
            return true;
        try {
            require(apply->isEnabled(), budget->text());
            auto entry = (*entries)[*current];
            if (!commit(
                    changeNarrative(*project, *entry, text->toPlainText(), extended->isChecked()),
                    "Edit " + QString::fromLatin1(entry->label)))
                return false;
            text->document()->setModified(false);
            list->item(*current)->setText(caption(entry));
            return true;
        } catch (const std::exception &e) {
            QMessageBox::warning(root, "Cannot apply narrative text", QString::fromUtf8(e.what()));
            return false;
        }
    };
    if (flush)
        *flush = save;
    for (auto entry : *entries) {
        auto item = new QListWidgetItem(caption(entry), list);
        item->setToolTip(QString("%1\nConsumer: %2\nStable entry: %3\nFile offset: 0x%4")
                             .arg(entry->label, entry->resource, entry->id)
                             .arg(entry->offset, 0, 16));
        if (resource == "END.DAT") {
            int page = list->count() - 1;
            try {
                auto g = U5::readGraphics(U5D_3df4[page] ? "END2.16" : "END1.16",
                                          project->data(U5D_3df4[page] ? "END2.16" : "END1.16"));
                item->setIcon(QPixmap::fromImage(g.images[U5D_3dee[page]]));
            } catch (const std::exception &) {
            }
        }
    }
    list->setIconSize(QSize(80, 50));
    QObject::connect(list, &QListWidget::currentRowChanged, root, [=](int row) {
        if (*busy || row < 0)
            return;
        if (!save()) {
            QSignalBlocker blocker(list);
            list->setCurrentRow(*current);
            return;
        }
        *busy = true;
        *current = row;
        (*state)[resource + "/narrative"] = row;
        auto entry = (*entries)[row];
        text->setPlainText(narrativeSource(narrativeText(*project, *entry), shop));
        text->document()->setModified(false);
        extended->setChecked(overrides(*project).contains(entry->id));
        QString description = QString::fromLatin1(entry->label);
        if (resource == "QUESTION.DAT" && QString(entry->id).startsWith("creation/") &&
            QString(entry->id).count('/') == 2) {
            auto parts = QString(entry->id).split('/');
            int a = parts[1].toInt(), b = parts[2].toInt();
            description +=
                QString("\nChoice A: %1 (+%2 INT, +%3 DEX, +%4 STR)\nChoice B: %5 (+%6 INT, +%7 "
                        "DEX, +%8 STR)\nThese outcomes are engine rules, not editable text.")
                    .arg(U5Virtues[a])
                    .arg(U5D_5164[a])
                    .arg(U5D_516c[a])
                    .arg(U5D_5174[a])
                    .arg(U5Virtues[b])
                    .arg(U5D_5164[b])
                    .arg(U5D_516c[b])
                    .arg(U5D_5174[b]);
        }
        context->setText(description);
        if (resource == "MISCMSG.DAT") {
            bool runic = entry->offset == 0x3ab + 0x57f || entry->offset == 0x3ab + 0x5d1 ||
                         entry->offset == 0x3ab + 0x60c || entry->offset == 0x3ab + 0x680;
            if (auto panel = root->findChild<QWidget *>("textPreviewPanel"))
                if (auto font = panel->findChild<QComboBox *>())
                    font->setCurrentIndex(runic ? 1 : 0);
        }
        if (resource == "END.DAT") {
            try {
                auto name = U5D_3df4[row] ? "END2.16" : "END1.16";
                auto g = U5::readGraphics(name, project->data(name));
                QImage image(320, 200, QImage::Format_RGB32);
                image.fill(Qt::black);
                QPainter p(&image);
                p.drawImage(U5D_3dfa[row], U5D_3e00[row], g.images[U5D_3dee[row]]);
                p.end();
                artwork->setPixmap(QPixmap::fromImage(
                    image.scaled(320, 200, Qt::KeepAspectRatio, Qt::FastTransformation)));
            } catch (const std::exception &e) {
                artwork->setText(QString::fromUtf8(e.what()));
            }
        }
        *busy = false;
        update();
    });
    QObject::connect(text, &QPlainTextEdit::textChanged, root, update);
    QObject::connect(extended, &QCheckBox::toggled, root, [=] {
        if (!*busy)
            text->document()->setModified(true);
        update();
    });
    for (auto field : *values)
        QObject::connect(field, &QLineEdit::textChanged, root, [=] { update(); });
    QObject::connect(apply, &QPushButton::clicked, root, [=] {
        save();
        update();
    });
    QObject::connect(filter, &QLineEdit::textChanged, root, [=](const QString &query) {
        for (int i = 0; i < list->count(); ++i)
            list->item(i)->setHidden(list->item(i)->data(Qt::UserRole + 1).toBool() ||
                                     !list->item(i)->text().contains(query, Qt::CaseInsensitive));
    });
    auto karmaUpdate = [=] {
        int index = qMin(4, karma->value() / 20);
        if (speaker->currentIndex() == 0 && index == 4)
            index = 5;
        list->setCurrentRow(index);
    };
    QObject::connect(karma, qOverload<int>(&QSpinBox::valueChanged), root, [=] { karmaUpdate(); });
    QObject::connect(speaker, qOverload<int>(&QComboBox::currentIndexChanged), root,
                     [=] { karmaUpdate(); });
    list->setCurrentRow(qBound(0, state->value(resource + "/narrative"), list->count() - 1));
    return root;
}
} // namespace Workshop
