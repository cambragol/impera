#include "dialogue_editor.h"
#include "dialogue_preview.h"
#include "native_text_viewport.h"
#include "native_dialogue_session.h"
#include "dialogue_surface.h"
#include "content_navigation.h"
#include <memory>
using namespace Dialogue;
static constexpr int InsertParagraphPause = 1000;
static constexpr int InsertSpeechText = 1001;
static constexpr int InsertNarrationText = 1002;
static QVector<Block> paragraphBlocks(const QByteArray &bytes) {
    QVector<Block> result;
    for (const auto &block : blocks(bytes)) {
        if (block.opcode >= 0) {
            result.append(block);
            continue;
        }
        int begin = 0;
        while (begin < block.bytes.size()) {
            int end = block.bytes.indexOf(QByteArray::fromHex("8d8d"), begin);
            if (end < 0)
                end = block.bytes.size();
            else {
                end += 2;
                while (end < block.bytes.size() && U5::byte(block.bytes, end) == 141)
                    ++end;
            }
            result.append({block.bytes.mid(begin, end - begin), -1});
            begin = end;
        }
    }
    return result;
}
static bool speechAt(const QVector<Block> &entryBlocks, int index, bool implicitQuotes) {
    bool open = implicitQuotes;
    int lastLiteral = implicitQuotes ? 162 : 0;
    for (int i = 0; i < index; ++i) {
        const auto &block = entryBlocks[i];
        if (block.opcode < 0) {
            for (unsigned char ch : block.bytes) {
                if (ch == 162 && lastLiteral != 162)
                    open = !open;
                if (ch > 128)
                    lastLiteral = ch;
            }
        } else if (block.opcode == 136) {
            open = false;
            lastLiteral = 162;
        }
    }
    if (entryBlocks[index].bytes.startsWith(char(162)) && lastLiteral != 162)
        open = !open;
    return open;
}
static QPushButton *button(QBoxLayout *layout, const QString &text,
                           const std::function<void()> &fn) {
    auto b = new QPushButton(text);
    layout->addWidget(b);
    QObject::connect(b, &QPushButton::clicked, b, fn);
    return b;
}
static QString caption(const QByteArray &bytes) {
    QString s;
    try {
        for (auto b : blocks(bytes))
            if (b.opcode < 0)
                s += plain(b.bytes);
            else if (b.opcode == 129)
                s += "{Avatar}";
    } catch (...) {
    }
    return s.simplified().left(65);
}
static int actionAfterStateActions(const QVector<Block> &entryBlocks, int index) {
    while (index < entryBlocks.size() &&
           (entryBlocks[index].opcode == 134 || entryBlocks[index].opcode == 140))
        ++index;
    return index < entryBlocks.size() ? entryBlocks[index].opcode : -1;
}
static int handoffSpacing(int opcode) {
    if (opcode == 130)
        return 1;
    return opcode == 132 || opcode == 136 || opcode == 143 || opcode == 255 ||
                   (opcode >= 145 && opcode <= 159)
               ? 2
               : 0;
}
static QByteArray managedSpacing(const QByteArray &textBytes, int followingAction) {
    if ((followingAction == 131 || handoffSpacing(followingAction)) &&
        textBytes.endsWith(QByteArray::fromHex("8d8d")))
        return QByteArray::fromHex("8d8d");
    if (followingAction == 130 && textBytes.endsWith(char(141)))
        return QByteArray(1, char(141));
    return {};
}
static bool paragraphSpacingBefore(const QVector<Block> &entryBlocks, int actionIndex) {
    int textBlock = actionIndex - 1;
    while (textBlock >= 0 &&
           (entryBlocks[textBlock].opcode == 134 || entryBlocks[textBlock].opcode == 140))
        --textBlock;
    return textBlock >= 0 && entryBlocks[textBlock].opcode < 0 &&
           entryBlocks[textBlock].bytes.endsWith(QByteArray::fromHex("8d8d"));
}
static QString displayedActionName(const QVector<Block> &entryBlocks, int index) {
    if (entryBlocks[index].opcode == 131)
        return paragraphSpacingBefore(entryBlocks, index) ? "Pause after paragraph"
                                                          : "Brief pause (no line break)";
    return actionName(entryBlocks[index].opcode);
}
// Keyword chips wrap without forcing the conversation column wider than the window.
class KeywordFlowLayout : public QLayout {
  public:
    explicit KeywordFlowLayout(QWidget *parent) : QLayout(parent) {
        setContentsMargins(0, 0, 0, 0);
    }
    ~KeywordFlowLayout() override {
        while (auto item = takeAt(0))
            delete item;
    }
    void addItem(QLayoutItem *item) override { items.append(item); }
    int count() const override { return items.size(); }
    QLayoutItem *itemAt(int index) const override { return items.value(index); }
    QLayoutItem *takeAt(int index) override {
        return index >= 0 && index < items.size() ? items.takeAt(index) : nullptr;
    }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return place(QRect(0, 0, width, 0), false); }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize size;
        for (auto item : items) {
            auto minimum = item->minimumSize();
            if (item->widget())
                minimum = minimum.boundedTo(item->widget()->maximumSize());
            size = size.expandedTo(minimum);
        }
        return size;
    }
    void setGeometry(const QRect &rect) override {
        QLayout::setGeometry(rect);
        place(rect, true);
    }

  private:
    QList<QLayoutItem *> items;
    int place(const QRect &rect, bool install) const {
        int x = rect.x(), y = rect.y(), height = 0;
        for (auto item : items) {
            auto size = item->sizeHint();
            if (item->widget())
                size = size.boundedTo(item->widget()->maximumSize());
            size.setWidth(qMin(size.width(), qMax(1, rect.width())));
            if (x > rect.x() && x + size.width() > rect.right() + 1) {
                x = rect.x();
                y += height + 4;
                height = 0;
            }
            if (install)
                item->setGeometry(QRect(QPoint(x, y), size));
            x += size.width() + 6;
            height = qMax(height, size.height());
        }
        return y - rect.y() + height;
    }
};
// Text paragraphs share the outer document scroll area; no nested scrollbars.
class DialogueParagraph : public QPlainTextEdit {
  public:
    std::function<void()> activated;
    explicit DialogueParagraph(const QString &text, QWidget *parent)
        : QPlainTextEdit(text, parent) {
        setFrameShape(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTabChangesFocus(true);
        connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, [this] { fit(); });
    }
    void fit() {
        const int lines = qMax(1, qRound(document()->documentLayout()->documentSize().height()));
        setFixedHeight(qMin(16384, lines * fontMetrics().lineSpacing() +
                                       int(document()->documentMargin() * 2) + 8));
    }

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QPlainTextEdit::resizeEvent(event);
        fit();
    }
    void focusInEvent(QFocusEvent *event) override {
        QPlainTextEdit::focusInEvent(event);
        if (activated)
            activated();
    }
};
ConversationEditor::ConversationEditor(
    Project *p, const QString &r, std::function<void(const QByteArray &, const QString &)> cb,
    std::function<void(const QString &, const QString &)> nameCallback, int npc, int entry,
    std::function<void(int, int)> choose, QWidget *parent)
    : QWidget(parent), project(p), resource(r), commit(cb), selection(choose),
      rename(nameCallback) {
    setObjectName("conversationEditor");
    conversations = U5::readDialogue(p->data(r));
    auto layout = new QVBoxLayout(this);
    auto heading = new QHBoxLayout;
    auto title = new QLabel("<b>Conversations</b>");
    heading->addWidget(title);
    heading->addStretch();
    search = new QLineEdit;
    search->setPlaceholderText("Search NPC names, topics and dialogue…");
    search->setObjectName("conversationSearch");
    heading->addWidget(search, 1);
    layout->addLayout(heading);
    auto split = new QSplitter;
    layout->addWidget(split, 1);
    auto npcPane = new QWidget;
    npcPane->setObjectName("conversationBrowser");
    auto npcLayout = new QVBoxLayout(npcPane);
    npcLayout->setContentsMargins(0, 0, 0, 0);
    npcLayout->addWidget(new QLabel("Location:"));
    locationFilter = new QComboBox;
    locationFilter->setObjectName("conversationLocationFilter");
    locationFilter->addItem("All locations", -1);
    locationFilter->setToolTip(
        "Filter NPCs by their top-level location, across all floors and schedule times. Combines "
        "with search; your current conversation draft stays open.");
    QString stem = r.left(r.size() - 4);
    QByteArray schedules =
        p->resources.contains(stem + ".NPC") ? p->data(stem + ".NPC") : QByteArray();
    QMap<int, QString> locationNames;
    if (!schedules.isEmpty()) {
        auto map =
            p->resources.contains(stem + ".DAT") ? p->data(stem + ".DAT") : QByteArray(32768, 0);
        for (auto page : U5::mapPages(stem + ".DAT", map))
            locationNames[page.settlement] = page.name.section(" — ", 0, 0);
        for (auto it = locationNames.cbegin(); it != locationNames.cend(); ++it)
            locationFilter->addItem(it.value(), it.key());
    }
    locationFilter->addItem("Unassigned conversations", -2);
    npcLayout->addWidget(locationFilter);
    npcs = new QTreeWidget;
    npcs->setObjectName("conversationNpcs");
    npcs->setHeaderLabels({"NPC / dialogue ID"});
    npcs->setMinimumWidth(140);
    npcLayout->addWidget(npcs, 1);
    split->addWidget(npcPane);
    for (int i = 0; i < conversations.size(); ++i) {
        auto d = parse(conversations[i].bytes);
        QString name = d.entries.isEmpty() ? "Unnamed" : Dialogue::displayName(d.entries[0].bytes);
        if (name.isEmpty())
            name = "Unnamed";
        auto item = new QTreeWidgetItem(npcs, {name + QString(" · %1").arg(conversations[i].id)});
        item->setData(0, Qt::UserRole, i);
        QVariantList usedLocations;
        for (auto town : locationNames.keys())
            for (int slot = 0; slot < 32; ++slot)
                if (town * 576 + 544 + slot < schedules.size() &&
                    U5::byte(schedules, town * 576 + 512 + slot) != 0 &&
                    U5::byte(schedules, town * 576 + 544 + slot) == conversations[i].id &&
                    !usedLocations.contains(town))
                    usedLocations.append(town);
        item->setData(0, Qt::UserRole + 1, usedLocations);
        item->setToolTip(0, QString("%1 · dialogue ID %2").arg(r).arg(conversations[i].id));
    }
    auto navigation = new QWidget;
    navigation->setObjectName("conversationNavigation");
    auto outlineToggle = button(heading, "Topics",
                                [navigation] { navigation->setVisible(!navigation->isVisible()); });
    outlineToggle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    outlineToggle->setToolTip("Show or hide the conversation outline to make more editing space.");
    auto nav = new QVBoxLayout(navigation);
    nav->setContentsMargins(0, 0, 0, 0);
    outline = new QTreeWidget;
    outline->setObjectName("conversationOutline");
    outline->setHeaderLabels({"Conversation"});
    nav->addWidget(outline, 1);
    auto add = new QHBoxLayout;
    button(add, "+ Topic", [this] { addTopic(false); });
    button(add, "+ Question", [this] {
        if (flush())
            attempt([&] { install(addQuestion(doc), "Add question"); });
    });
    nav->addLayout(add);
    auto ops = new QHBoxLayout;
    replyButton = button(ops, "+ Answer", [this] { addTopic(true); });
    deleteButton = button(ops, "Delete", [this] { deleteSelected(); });
    nav->addLayout(ops);
    auto move = new QHBoxLayout;
    upButton = button(move, "Move up", [this] { moveTopic(-1, false); });
    downButton = button(move, "Move down", [this] { moveTopic(1, false); });
    duplicateButton = button(move, "Duplicate", [this] { moveTopic(0, true); });
    nav->addLayout(move);
    nameButton = button(nav, "Name question", [this] { renameQuestion(); });
    split->addWidget(navigation);
    auto right = new QWidget;
    auto detail = new QVBoxLayout(right);
    detail->setContentsMargins(0, 0, 0, 0);
    context = new QLabel;
    context->setWordWrap(true);
    detail->addWidget(context);
    aliases = new QLineEdit(this);
    aliases->setObjectName("conversationAliases");
    aliases->setPlaceholderText("Keywords, separated by commas");
    aliases->hide(); // Transactional backing field; keyword chips are the visible UI.
    keywordChips = new QWidget;
    keywordChips->setObjectName("conversationKeywords");
    detail->addWidget(keywordChips);
    connect(aliases, &QLineEdit::textEdited, this, [this] {
        if (!loading) {
            aliasesDraft = true;
            changed();
        }
    });
    connect(aliases, &QLineEdit::editingFinished, this, [this] { aliasesChanged(); });
    tabs = new QTabWidget;
    detail->addWidget(tabs, 3);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    blocksWidget = new QWidget;
    blocksLayout = new QVBoxLayout(blocksWidget);
    blocksLayout->setAlignment(Qt::AlignTop);
    blocksLayout->setSizeConstraint(QLayout::SetMinimumSize);
    scroll->setWidget(blocksWidget);
    tabs->setObjectName("conversationAuthoring");
    tabs->addTab(scroll, "Conversation");
    tabs->tabBar()->hide();
    auto advanced = new QWidget;
    auto advancedLayout = new QVBoxLayout(advanced);
    sourceScope = new QComboBox;
    sourceScope->setObjectName("conversationSourceScope");
    sourceScope->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    sourceScope->setMinimumContentsLength(22);
    sourceScope->addItems(
        {"Selected entry (lossless tagged source)", "Whole conversation (advanced)"});
    advancedLayout->addWidget(sourceScope);
    source = new QPlainTextEdit;
    source->setObjectName("dialogueText");
    advancedLayout->addWidget(source, 1);
    tabs->addTab(advanced, "Advanced source");
    button(advancedLayout, "Validate and apply source", [this] { applyDraft(); });
    connect(sourceScope, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (loading)
            return;
        if (!flush()) {
            QSignalBlocker block(sourceScope);
            sourceScope->setCurrentIndex(wholeSource ? 1 : 0);
            return;
        }
        wholeSource = index == 1;
        refreshSource();
    });
    connect(source, &QPlainTextEdit::textChanged, this, [this] {
        if (!loading) {
            sourceDraft = true;
            changed();
        }
    });
    auto actionPane = new QWidget;
    actionPane->setObjectName("conversationActionPane");
    auto actions = new QVBoxLayout(actionPane);
    actions->setAlignment(Qt::AlignTop);
    actions->addWidget(new QLabel("Available actions"));
    auto available = new QListWidget;
    available->setObjectName("conversationAvailableActions");
    auto addAvailable = [available](const QString &name, int code) {
        auto item = new QListWidgetItem(name, available);
        item->setData(Qt::UserRole, code);
        item->setToolTip("Insert " + name.toLower() + " at the end of the selected response.");
        return item;
    };
    addAvailable("Speech Text", InsertSpeechText)
        ->setToolTip("Add spoken text with quotation marks supplied automatically.");
    addAvailable("Narration Text", InsertNarrationText)
        ->setToolTip("Add unquoted narration between spoken paragraphs.");
    for (int code : {129, 142, 143, 130})
        addAvailable(actionName(code), code);
    addAvailable("Pause after paragraph", InsertParagraphPause)
        ->setToolTip("Add paragraph spacing, then pause briefly before continuing.");
    addAvailable("Brief pause (no line break)", 131)
        ->setToolTip("Pause briefly without changing the text layout.");
    for (int code : {132, 133, 134, 136, 137, 138, 139, 140, 254, 255})
        addAvailable(actionName(code), code);
    addAvailable("Explicit line break", 141);
    addAvailable("Enter question", 145);
    connect(available, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        addBlock(item->data(Qt::UserRole).toInt() == -1, -1, -1, item->data(Qt::UserRole).toInt());
    });
    auto insertShortcut = new QShortcut(QKeySequence(Qt::Key_Return), available);
    insertShortcut->setContext(Qt::WidgetShortcut);
    connect(insertShortcut, &QShortcut::activated, this, [available] {
        if (auto item = available->currentItem())
            if (item->flags() & Qt::ItemIsEnabled)
                emit available->itemClicked(item);
    });
    actions->addWidget(available, 1);
    button(actions, "Game preview", [this] { openAuxiliary(0); });
    button(actions, "Playtest", [this] { openAuxiliary(2); });
    auto advancedButton = button(actions, "Advanced…", [this] {
        if (flush())
            tabs->setCurrentIndex(tabs->currentIndex() == 0 ? 1 : 0);
    });
    advancedButton->setToolTip("Switch between visual editing and lossless tagged source.");
    connect(tabs, &QTabWidget::currentChanged, advancedButton, [advancedButton](int index) {
        advancedButton->setText(index == 0 ? "Advanced…" : "Back to conversation");
    });
    actionPane->setMinimumWidth(190);
    actionPane->setMaximumWidth(260);
    budget = new QLabel;
    budget->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    connect(budget, &QLabel::linkActivated, this, [this] { openAuxiliary(1); });
    detail->addWidget(budget);
    status = new QLabel;
    status->setWordWrap(true);
    status->setObjectName("conversationStatus");
    detail->addWidget(status);
    auxiliary = new QDialog(this, Qt::Window);
    auxiliary->setObjectName("conversationTools");
    auxiliary->setWindowTitle("Conversation tools");
    auxiliary->resize(700, 560);
    auto auxiliaryLayout = new QVBoxLayout(auxiliary);
    previewTabs = new QTabWidget;
    auxiliaryLayout->addWidget(previewTabs);
    connect(previewTabs, &QTabWidget::currentChanged, this, [this] {
        if (!loading && auxiliary->isVisible())
            updateDiagnostics();
    });
    auto dos = new QWidget;
    auto dosLayout = new QVBoxLayout(dos);
    previewNotes = new QLabel;
    previewNotes->setWordWrap(true);
    previewNotes->setObjectName("conversationPreviewNotes");
    dosLayout->addWidget(previewNotes);
    dosImage = new NativeTextViewport;
    dosLayout->addWidget(dosImage, 1);
    previewTabs->addTab(dos, "Game preview");
    diagnostics = new QListWidget;
    diagnostics->setObjectName("conversationDiagnostics");
    previewTabs->addTab(diagnostics, "Diagnostics");
    connect(diagnostics, &QListWidget::itemClicked, this, [this](QListWidgetItem *i) {
        int e = i->data(Qt::UserRole).toInt();
        if (e >= 0 && flush())
            loadEntry(e);
    });
    auto test = new QWidget;
    auto testLayout = new QVBoxLayout(test);
    auto setup = new QWidget;
    setup->setObjectName("conversationTestSetup");
    auto setupLayout = new QVBoxLayout(setup);
    auto testToolbar = new QHBoxLayout;
    auto setupButton =
        button(testToolbar, "Setup", [setup] { setup->setVisible(!setup->isVisible()); });
    setupButton->setToolTip("Configure the sandbox's starting state, then Restart.");
    button(testToolbar, "Restart", [this] { resetSimulation(); });
    showTrace = new QCheckBox("Show actions");
    showTrace->setObjectName("conversationShowTrace");
    testToolbar->addWidget(showTrace);
    playtestStatus = new QLabel;
    playtestStatus->setObjectName("conversationPlaytestStatus");
    playtestStatus->setWordWrap(true);
    testToolbar->addWidget(playtestStatus, 1);
    testLayout->addLayout(testToolbar);
    auto stateRow = new QHBoxLayout;
    avatar = new QLineEdit("Avatar");
    avatar->setPlaceholderText("Party names, comma-separated");
    avatar->setToolTip("First name is the Avatar. Name recognition uses the "
                       "first four characters of any listed party member.");
    stateRow->addWidget(avatar);
    gold = new QSpinBox;
    gold->setRange(0, 9999);
    gold->setValue(100);
    gold->setPrefix("Gold ");
    stateRow->addWidget(gold);
    karma = new QSpinBox;
    karma->setRange(0, 99);
    karma->setValue(50);
    karma->setPrefix("Karma ");
    stateRow->addWidget(karma);
    setupLayout->addLayout(stateRow);
    auto stateRow2 = new QHBoxLayout;
    introduced = new QCheckBox("Introduced");
    seed = new QSpinBox;
    seed->setObjectName("conversationSeed");
    seed->setRange(0, 65535);
    seed->setValue(42);
    seed->setPrefix("Seed ");
    stateRow2->addWidget(introduced);
    stateRow2->addWidget(seed);
    partySize = new QSpinBox;
    partySize->setRange(1, 6);
    partySize->setValue(3);
    partySize->setPrefix("Party ");
    stateRow2->addWidget(partySize);
    setupLayout->addLayout(stateRow2);
    auto npcRow = new QHBoxLayout;
    npcRow->addWidget(new QLabel("NPC"));
    testNpc = new QComboBox;
    testNpc->setObjectName("conversationTestNpc");
    npcRow->addWidget(testNpc, 1);
    setupLayout->addLayout(npcRow);
    connect(testNpc, &QComboBox::currentIndexChanged, this, [this] {
        if (!project->resources.contains("INIT.GAM"))
            return;
        try {
            auto initial =
                nativeSetup(*project, resource, npcIndex, testNpc->currentData().toInt());
            avatar->setText(initial.state.partyNames.join(", "));
            gold->setValue(initial.state.gold);
            karma->setValue(initial.state.karma);
            partySize->setValue(initial.state.partySize);
            introduced->setChecked(initial.state.introduced);
        } catch (const std::exception &error) {
            status->setText(QString::fromUtf8(error.what()));
        }
    });
    testLayout->addWidget(setup);
    setup->hide();
    transcript = new QTreeWidget;
    transcript->setHeaderLabels({"Conversation"});
    transcript->setObjectName("conversationTranscript");
    auto testBody = new QHBoxLayout;
    playtestImage = new NativeTextViewport;
    playtestImage->setObjectName("conversationNativeViewport");
    playtestImage->keyInput = [this](int key) {
        if (nativeSession)
            nativeSession->sendKey(key);
    };
    testBody->addWidget(playtestImage, 1);
    testBody->addWidget(transcript);
    testLayout->addLayout(testBody, 1);
    transcript->setMaximumWidth(260);
    transcript->hide();
    connect(transcript, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *i) {
        if (nativeStale)
            return;
        int e = i->data(0, Qt::UserRole).toInt();
        const int offset = i->data(0, Qt::UserRole + 2).toInt();
        if (e >= 0 && flush()) {
            loadEntry(e);
            const auto bs = paragraphBlocks(doc.entries[e].bytes.mid(entryPrefix(e)));
            int begin = entryPrefix(e), block = -1;
            for (int n = 0; n < bs.size(); ++n) {
                if (offset >= begin && offset < begin + bs[n].bytes.size())
                    block = n;
                begin += bs[n].bytes.size();
            }
            for (auto widget : blocksWidget->findChildren<QFrame *>())
                if (widget->property("dialogueEntry").isValid() &&
                    widget->property("dialogueEntry").toInt() == e &&
                    (widget->property("dialogueBlock").toInt() == block ||
                     (widget->property("dialogueBlockEnd").isValid() &&
                      widget->property("dialogueBlock").toInt() <= block &&
                      widget->property("dialogueBlockEnd").toInt() > block))) {
                    widget->setFrameShape(QFrame::Box);
                    widget->setFocusPolicy(Qt::StrongFocus);
                    widget->setFocus();
                    if (auto view = qobject_cast<QScrollArea *>(tabs->widget(0)))
                        view->ensureWidgetVisible(widget);
                }
        }
    });
    connect(showTrace, &QCheckBox::toggled, this, [this](bool show) {
        transcript->setVisible(show);
        for (int i = 0; i < transcript->topLevelItemCount(); ++i) {
            auto item = transcript->topLevelItem(i);
            item->setHidden(!show && item->data(0, Qt::UserRole + 1).toBool());
        }
    });
    previewTabs->addTab(test, "Test Conversation");
    auto close = new QDialogButtonBox(QDialogButtonBox::Close);
    auxiliaryLayout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, auxiliary, &QDialog::hide);
    auxiliary->installEventFilter(this);
    split->addWidget(right);
    split->addWidget(actionPane);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 0);
    split->setStretchFactor(2, 1);
    split->setStretchFactor(3, 0);
    split->setSizes({170, 235, 650, 210});
    timer.setSingleShot(true);
    timer.setInterval(650);
    connect(&timer, &QTimer::timeout, this, [this] { applyDraft(); });
    connect(outline, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *item, QTreeWidgetItem *previous) {
                if (loading || !item)
                    return;
                int e = item->data(0, Qt::UserRole).toInt();
                if (e < 0)
                    return;
                if (!flush()) {
                    QSignalBlocker block(outline);
                    outline->setCurrentItem(previous);
                    return;
                }
                loadEntry(e);
            });
    connect(npcs, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *item, QTreeWidgetItem *previous) {
                if (loading || !item)
                    return;
                if (!flush()) {
                    QSignalBlocker block(npcs);
                    npcs->setCurrentItem(previous);
                    return;
                }
                loadNpc(item->data(0, Qt::UserRole).toInt());
            });
    connect(search, &QLineEdit::textChanged, this, [this] { filterNpcs(); });
    connect(locationFilter, &QComboBox::currentIndexChanged, this, [this] { filterNpcs(); });
    if (!conversations.isEmpty()) {
        loading = true;
        npcs->setCurrentItem(npcs->topLevelItem(qBound(0, npc, int(conversations.size() - 1))));
        loading = false;
        loadNpc(qBound(0, npc, int(conversations.size() - 1)), entry);
    }
}
void ConversationEditor::filterNpcs() {
    const auto query = search->text();
    const int location = locationFilter->currentData().toInt();
    for (int i = 0; i < npcs->topLevelItemCount(); ++i) {
        auto item = npcs->topLevelItem(i);
        auto conversation = conversations[item->data(0, Qt::UserRole).toInt()];
        auto assigned = item->data(0, Qt::UserRole + 1).toList();
        bool matchesLocation =
            location == -1 || (location == -2 ? assigned.isEmpty() : assigned.contains(location));
        bool matchesText =
            item->text(0).contains(query, Qt::CaseInsensitive) ||
            U5::decodeText(conversation.bytes).contains(query, Qt::CaseInsensitive) ||
            caption(conversation.bytes).contains(query, Qt::CaseInsensitive);
        item->setHidden(!matchesLocation || !matchesText);
    }
}
bool ConversationEditor::attempt(const std::function<void()> &fn) {
    try {
        fn();
        return true;
    } catch (const std::exception &e) {
        status->setText(QString("Cannot apply: %1").arg(QString::fromUtf8(e.what())));
        return false;
    }
}
void ConversationEditor::setCharacterIdentity(int character) {
    int index = testNpc->findData(character);
    if (index >= 0)
        testNpc->setCurrentIndex(index);
}
void ConversationEditor::loadTestContext() {
    if (!testNpc)
        return;
    testNpc->clear();
    testNpc->addItem("Conversation only", -2);
    for (const auto &npc : Workshop::characterDestinations(*project))
        if (npc.character > 0 && npc.character % 32 != 0 && npc.resource == resource &&
            npc.conversation == npcIndex)
            testNpc->addItem(npc.title + " — " + npc.group, npc.character);
    if (testNpc->count() == 2)
        testNpc->setCurrentIndex(1);
}
void ConversationEditor::loadNpc(int i, int entry) {
    npcIndex = i;
    doc = parse(conversations[i].bytes);
    loadTestContext();
    ++nativeGeneration;
    nativeSession.reset();
    nativeStale = false;
    transcript->clear();
    buildOutline(entry);
    loadEntry(qBound(0, entry, int(doc.entries.size() - 1)));
}
void ConversationEditor::buildOutline(int selectEntry) {
    if (selectEntry == doc.tail) selectEntry = 0;
    loading = true;
    outline->clear();
    auto root = [&](QString s) {
        auto i = new QTreeWidgetItem(outline, {s});
        i->setData(0, Qt::UserRole, -1);
        i->setExpanded(true);
        return i;
    };
    auto row = [&](QTreeWidgetItem *p, QString s, int e) {
        auto i = new QTreeWidgetItem(p, {s});
        i->setData(0, Qt::UserRole, e);
        if (e == selectEntry)
            outline->setCurrentItem(i);
        return i;
    };
    if (!doc.structuralError.isEmpty()) {
        row(root("Unrecognized structure"), "Whole conversation (Advanced)", 0);
        loading = false;
        return;
    }
    auto basics = root("Basics");
    QStringList names = {"Name", "Description", "Greeting (introduced)", "Job / Work", "Farewell"};
    for (int e = 0; e < 5; ++e)
        row(basics, names[e], e);
    auto label = [&](const Rule &r) {
        QStringList keys;
        for (int e : r.keywords)
            keys << keyword(doc.entries[e].bytes);
        return keys.join(" / ");
    };
    auto topics = root("Topics");
    for (auto r : doc.topics)
        row(topics, label(r), r.response);
    auto questions = root("Questions");
    for (auto q : doc.questions) {
        auto group = row(questions, questionName(q.label), q.prompt);
        group->setExpanded(true);
        for (auto r : q.replies)
            row(group, label(r), r.response);
        if (q.fallback >= 0)
            row(group, "Otherwise / fallback", q.fallback);
    }
    for (int e : doc.rawEntries)
        row(root("Unrecognized bytes"), QString("Raw entry %1 (Advanced)").arg(e), e);
    loading = false;
}
void ConversationEditor::loadEntry(int e) {
    if (e == doc.tail) e = 0;
    if (e < 0 || e >= doc.entries.size())
        return;
    loading = true;
    selected = e;
    prefix = 0;
    for (auto q : doc.questions)
        if (q.prompt == e)
            prefix = 2;
    if (!doc.structuralError.isEmpty())
        wholeSource = true;
    {
        QSignalBlocker block(sourceScope);
        sourceScope->setCurrentIndex(wholeSource ? 1 : 0);
    }
    refreshSource();
    sourceDraft = false;
    aliasesDraft = false;
    pending = false;
    aliases->clear();
    aliases->hide();
    auto show = [&](const QVector<Rule> &rs) {
        for (auto r : rs)
            if (r.response == e) {
                QStringList keys;
                for (auto k : r.keywords)
                    keys << keyword(doc.entries[k].bytes);
                aliases->setText(keys.join(", "));
                // Keywords are shown as chips; keep the backing field for transactional edits.
            }
    };
    show(doc.topics);
    for (auto q : doc.questions)
        show(q.replies);
    if (auto old = keywordChips->layout()) {
        while (auto item = old->takeAt(0)) {
            if (auto widget = item->widget()) {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
        delete old;
    }
    auto keywordLayout = new QHBoxLayout(keywordChips);
    keywordLayout->setContentsMargins(0, 0, 0, 0);
    if (selectedQuestion() < 0)
        buildKeywords(e, keywordLayout);
    keywordChips->setVisible(selectedQuestion() < 0 && !aliases->text().isEmpty());
    QStringList basics = {"Name", "Description", "Greeting", "Job / Work", "Farewell"};
    QString message = e < basics.size() ? basics[e] : "Response";
    const int question = selectedQuestion();
    if (question >= 0)
        message = questionName(doc.questions[question].label);
    context->setToolTip(QString());
    QString npcFile = resource.left(resource.size() - 4) + ".NPC";
    if (project->resources.contains(npcFile)) {
        auto data = project->data(npcFile);
        QStringList locations;
        auto pages = project->resources.contains(resource.left(resource.size() - 4) + ".DAT")
                         ? U5::mapPages(resource.left(resource.size() - 4) + ".DAT",
                                        project->data(resource.left(resource.size() - 4) + ".DAT"))
                         : QVector<U5::MapPage>{};
        for (int town = 0; town < 8; ++town)
            for (int n = 0; n < 32; ++n)
                if (town * 576 + 544 + n < data.size() &&
                    U5::byte(data, town * 576 + 512 + n) != 0 &&
                    U5::byte(data, town * 576 + 544 + n) == conversations[npcIndex].id) {
                    QString location = QString("Settlement %1").arg(town + 1);
                    for (auto page : pages)
                        if (page.settlement == town) {
                            location = page.name.section(" — ", 0, 0);
                            break;
                        }
                    locations << QString("%1 · NPC %2").arg(location).arg(n);
                }
        if (!locations.isEmpty())
            context->setToolTip("Used by: " + locations.join("; "));
    }
    context->setTextFormat(Qt::PlainText);
    context->setText(message);
    status->clear();
    buildBlocks();
    selection(npcIndex, e);
    updateDiagnostics();
    loading = false;
}
int ConversationEditor::entryPrefix(int entry) const {
    for (const auto &question : doc.questions)
        if (question.prompt == entry)
            return 2;
    return 0;
}
int ConversationEditor::selectedQuestion() const {
    for (int i = 0; i < doc.questions.size(); ++i) {
        const auto &q = doc.questions[i];
        if (selected == q.prompt || selected == q.fallback)
            return i;
        for (const auto &reply : q.replies)
            if (reply.response == selected)
                return i;
    }
    return -1;
}
void ConversationEditor::activateEntry(int entry) {
    if (loading || entry == selected)
        return;
    // Question sections are one document. Changing focus must not flush or discard
    // another paragraph's incomplete draft.
    selected = entry;
    prefix = entryPrefix(entry);
    if (!sourceDraft)
        refreshSource();
    selection(npcIndex, entry);
    QSignalBlocker blocked(outline);
    QTreeWidgetItemIterator it(outline);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toInt() == entry)
            outline->setCurrentItem(*it);
        ++it;
    }
    updateOutlineActions();
}
void ConversationEditor::openAuxiliary(int tab) {
    if (!flush())
        return;
    previewTabs->setCurrentIndex(tab);
    auxiliary->show();
    updateDiagnostics();
    auxiliary->raise();
    auxiliary->activateWindow();
    if (tab == 2 && !nativeSession)
        resetSimulation();
    if (tab == 2)
        playtestImage->setFocus();
}
void ConversationEditor::updateOutlineActions() {
    auto available = findChild<QListWidget *>("conversationAvailableActions");
    if (available) {
        for (int i = 0; i < available->count(); ++i) {
            auto item = available->item(i);
            const int code = item->data(Qt::UserRole).toInt();
            const bool enabled =
                selected >= 0 && selected != doc.tail && !doc.rawEntries.contains(selected) &&
                doc.structuralError.isEmpty() &&
                (!doc.questions.isEmpty() || (code != 140 && code != 254 && code != 145));
            item->setFlags(enabled ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags);
        }
    }
    bool rule = false;
    QVector<Rule> rules = doc.topics;
    for (const auto &q : doc.questions)
        rules += q.replies;
    int at = -1;
    for (int i = 0; i < rules.size(); ++i)
        if (rules[i].response == selected) {
            rule = true;
            at = i;
        }
    const int question = selectedQuestion();
    replyButton->setEnabled(question >= 0);
    nameButton->setEnabled(question >= 0);
    duplicateButton->setEnabled(rule);
    deleteButton->setEnabled(rule || (question >= 0 && doc.questions[question].prompt == selected));
    // Keep reply ordering within its own question, never across question boundaries.
    const auto local = question >= 0 ? doc.questions[question].replies : doc.topics;
    at = -1;
    for (int i = 0; i < local.size(); ++i)
        if (local[i].response == selected)
            at = i;
    upButton->setEnabled(at > 0);
    downButton->setEnabled(at >= 0 && at + 1 < local.size());
}
void ConversationEditor::replaceKeywords(int entry, const QStringList &keys) {
    if (!flush())
        return;
    attempt([&] {
        QVector<Rule> rules = doc.topics;
        for (const auto &q : doc.questions)
            rules += q.replies;
        for (const auto &rule : rules)
            if (rule.response == entry) {
                install(replaceAliases(doc, rule, keys), "Edit response keywords",
                        rule.keywords.first() + keys.size() * 2 - 1);
                return;
            }
    });
}
void ConversationEditor::buildKeywords(int entry, QBoxLayout *layout) {
    QVector<Rule> rules = doc.topics;
    for (const auto &q : doc.questions)
        rules += q.replies;
    for (const auto &rule : rules)
        if (rule.response == entry) {
            QStringList keys;
            for (int key : rule.keywords)
                keys.append(keyword(doc.entries[key].bytes));
            auto container = new QWidget;
            auto chips = new KeywordFlowLayout(container);
            layout->addWidget(container, 1);
            chips->addWidget(new QLabel("When asked:"));
            for (int i = 0; i < keys.size(); ++i) {
                auto chip = new QToolButton;
                chip->setText(keys[i]);
                chip->setMaximumWidth(200);
                chip->setProperty("dialogueKeyword", true);
                chip->setToolTip(keys[i] +
                                 " — edit this keyword or remove it from the shared response.");
                chip->setPopupMode(QToolButton::InstantPopup);
                auto menu = new QMenu(chip);
                menu->addAction("Edit keyword…", this, [this, entry, keys, i] {
                    bool ok;
                    auto value =
                        QInputDialog::getText(this, "Edit keyword", "What the player types",
                                              QLineEdit::Normal, keys[i], &ok);
                    if (ok) {
                        auto next = keys;
                        next[i] = value;
                        replaceKeywords(entry, next);
                    }
                });
                auto remove = menu->addAction("Remove keyword", this, [this, entry, keys, i] {
                    auto next = keys;
                    next.removeAt(i);
                    replaceKeywords(entry, next);
                });
                remove->setEnabled(keys.size() > 1);
                chip->setMenu(menu);
                chips->addWidget(chip);
            }
            auto add = new QPushButton("+ Keyword");
            chips->addWidget(add);
            connect(add, &QPushButton::clicked, this, [this, entry, keys] {
                bool ok;
                auto value = QInputDialog::getText(this, "Add keyword", "What the player types",
                                                   QLineEdit::Normal, {}, &ok);
                if (ok) {
                    auto next = keys;
                    next.append(value);
                    replaceKeywords(entry, next);
                }
            });
            add->setToolTip("Keywords share this response and are checked in their stored order.");
            break;
        }
}
void ConversationEditor::buildBlocks() {
    while (auto i = blocksLayout->takeAt(0)) {
        if (i->widget()) {
            i->widget()->hide();
            i->widget()->setParent(nullptr);
            i->widget()->deleteLater();
        }
        delete i;
    }
    readers.clear();
    entryShapes.clear();
    readerRanges.clear();
    entryErrors.clear();
    currentBlocks.clear();
    if (!doc.structuralError.isEmpty() || selected == doc.tail ||
        doc.rawEntries.contains(selected)) {
        blocksLayout->addWidget(new QLabel("Advanced source only"));
        tabs->setCurrentIndex(1);
        return;
    }
    const int question = selectedQuestion();
    auto section = [this](int entry, const QString &title, bool collapsible) {
        auto group = new QGroupBox(title.size() > 45 ? title.left(42) + "…" : title);
        group->setToolTip(title);
        group->setProperty("dialogueSectionEntry", entry);
        group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        auto layout = new QVBoxLayout(group);
        layout->setAlignment(Qt::AlignTop);
        layout->setSizeConstraint(QLayout::SetMinimumSize);
        if (collapsible) {
            group->setCheckable(true);
            group->setChecked(entry == selected);
        }
        auto body = new QWidget;
        auto content = new QVBoxLayout(body);
        content->setSizeConstraint(QLayout::SetMinimumSize);
        content->setContentsMargins(0, 0, 0, 0);
        auto keywords = new QHBoxLayout;
        buildKeywords(entry, keywords);
        content->addLayout(keywords);
        buildEntryBlocks(entry, content);
        auto insert = new QHBoxLayout;
        button(insert, "+ Speech", [this, entry] { addBlock(true, entry); });
        button(insert, "+ Narration",
               [this, entry] { addBlock(false, entry, -1, InsertNarrationText); });
        button(insert, "Actions →", [this, entry] {
            activateEntry(entry);
            findChild<QListWidget *>("conversationAvailableActions")->setFocus();
        });
        insert->addStretch();
        if (collapsible) {
            auto move = [this, entry](int delta, bool duplicate) {
                if (flush()) {
                    activateEntry(entry);
                    moveTopic(delta, duplicate);
                }
            };
            button(insert, "Move up", [move] { move(-1, false); });
            button(insert, "Move down", [move] { move(1, false); });
            button(insert, "Remove answer", [this, entry] {
                if (flush()) {
                    activateEntry(entry);
                    deleteSelected();
                }
            });
            body->setVisible(entry == selected);
            connect(group, &QGroupBox::toggled, this, [this, entry, body](bool expanded) {
                body->setVisible(expanded);
                if (expanded)
                    activateEntry(entry);
            });
        }
        content->addLayout(insert);
        layout->addWidget(body);
        blocksLayout->addWidget(group);
    };
    if (question >= 0) {
        const auto q = doc.questions[question];
        section(q.prompt, "Prompt", false);
        blocksLayout->addWidget(new QLabel("Answers — checked in order"));
        for (const auto &reply : q.replies) {
            QStringList keys;
            for (int key : reply.keywords)
                keys.append(keyword(doc.entries[key].bytes));
            section(reply.response, keys.join(" / "), true);
        }
        auto add = button(blocksLayout, "+ Add answer", [this] { addTopic(true); });
        add->setToolTip("Add an answer to this question, after its existing answers.");
        if (q.fallback >= 0)
            section(q.fallback, "Otherwise", false);
        auto refs = new QVBoxLayout;
        auto referencesBox = new QWidget;
        referencesBox->setLayout(refs);
        refs->addWidget(new QLabel("Used by"));
        for (int entry : references(doc, q.label)) {
            QString title = caption(doc.entries[entry].bytes);
            if (title.isEmpty())
                title = "Response with a branch";
            button(refs, title, [this, entry] {
                if (flush())
                    loadEntry(entry);
            });
        }
        if (references(doc, q.label).isEmpty())
            refs->addWidget(new QLabel("No incoming references"));
        blocksLayout->addWidget(referencesBox);
    } else
        buildEntryBlocks(selected, blocksLayout);
    updateOutlineActions();
}
void ConversationEditor::buildEntryBlocks(int entry, QVBoxLayout *sectionLayout) {
    auto engineText = [sectionLayout](const QString &text) {
        auto label = new QLabel(text);
        label->setObjectName("conversationEngineText");
        label->setTextFormat(Qt::PlainText);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        label->setWordWrap(true);
        sectionLayout->addWidget(label);
    };
    if (entry == 0 || entry == 1)
        engineText(entry == 0 ? "My name is " : "You see ");
    const auto entryBlocks = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
    for (const auto &block : entryBlocks)
        entryShapes[entry].append(block.opcode);
    if (entry == selected)
        currentBlocks = entryBlocks;
    bool runic = false;
    for (int index = 0; index < entryBlocks.size(); ++index) {
        const auto b = entryBlocks[index];
        // Interleaved text, variables and rune controls form a single editable
        // paragraph with protected inline objects, not separate text panes.
        int end = index;
        bool inlineControls = false;
        while (end < entryBlocks.size() &&
               (entryBlocks[end].opcode < 0 || entryBlocks[end].opcode == 129 ||
                entryBlocks[end].opcode == 142)) {
            inlineControls |= entryBlocks[end].opcode >= 0;
            ++end;
            if (entryBlocks[end - 1].opcode < 0 &&
                entryBlocks[end - 1].bytes.endsWith(QByteArray::fromHex("8d8d")))
                break;
        }
        if (inlineControls) {
            auto displayed = entryBlocks.mid(index, end - index);
            QByteArray spacing, opening, closing;
            const int followingAction = actionAfterStateActions(entryBlocks, end);
            if (displayed.last().opcode < 0) {
                spacing = managedSpacing(displayed.last().bytes, followingAction);
                if (spacing.isEmpty() && end < entryBlocks.size() && entryBlocks[end].opcode < 0 &&
                    displayed.last().bytes.endsWith(QByteArray::fromHex("8d8d")))
                    spacing = QByteArray::fromHex("8d8d");
                displayed.last().bytes.chop(spacing.size());
            }
            const bool speech = speechAt(entryBlocks, index, entry != 1);
            if (speech && displayed.first().opcode < 0 &&
                displayed.first().bytes.startsWith(char(162))) {
                opening = displayed.first().bytes.left(1);
                displayed.first().bytes.remove(0, 1);
            }
            if (speech && displayed.last().opcode < 0 &&
                displayed.last().bytes.endsWith(char(162))) {
                closing = displayed.last().bytes.right(1);
                displayed.last().bytes.chop(1);
            }
            auto surface = new DialogueTextSurface(displayed);
            surface->setAccessibleName(speech ? "Speech Text" : "Narration Text");
            surface->setProperty("dialogueTextKind", speech ? "speech" : "narration");
            surface->setProperty("dialogueEntry", entry);
            surface->setProperty("dialogueBlock", index);
            surface->setProperty("dialogueBlockEnd", end);
            surface->activated = [this, entry] { activateEntry(entry); };
            surface->edited = [this, entry] {
                activateEntry(entry);
                changed();
            };
            readers[entry].append([surface, spacing, opening, closing] {
                return opening + surface->bytes() + closing + spacing;
            });
            readerRanges[entry].append({index, end});
            auto paragraph = new QWidget;
            auto row = new QHBoxLayout(paragraph);
            row->setContentsMargins(0, 0, 0, 0);
            auto kindLabel = new QLabel(speech ? "Speech" : "Narration");
            kindLabel->setObjectName("conversationTextKind");
            row->addWidget(kindLabel, 0, Qt::AlignTop);
            row->addWidget(surface, 1);
            sectionLayout->addWidget(paragraph);
            index = end - 1;
            continue;
        }
        auto group = new QFrame;
        group->setObjectName(b.opcode < 0 ? "conversationParagraph" : "conversationAction");
        group->setProperty("dialogueEntry", entry);
        group->setProperty("dialogueBlock", index);
        group->setProperty("dialogueOpcode", b.opcode);
        group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        auto layout = new QVBoxLayout(group);
        layout->setAlignment(Qt::AlignTop);
        layout->setSizeConstraint(QLayout::SetMinimumSize);
        layout->setContentsMargins(4, 2, 4, 2);
        auto tools = new QHBoxLayout;
        auto summary = new QToolButton;
        summary->setText(displayedActionName(entryBlocks, index));
        summary->setToolButtonStyle(Qt::ToolButtonTextOnly);
        summary->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        summary->setAccessibleName(displayedActionName(entryBlocks, index));
        if (b.opcode >= 0)
            tools->addWidget(summary, 1);
        else
            summary->setParent(group);
        if (b.opcode < 0)
            summary->hide();
        auto more = new QToolButton;
        more->setText("⋯");
        more->setAccessibleName("Paragraph or action menu");
        more->setPopupMode(QToolButton::InstantPopup);
        auto menu = new QMenu(more);
        menu->addAction("Insert Speech Text before", this,
                        [this, entry, index] { addBlock(true, entry, index); });
        menu->addAction("Insert Narration Text before", this, [this, entry, index] {
            addBlock(false, entry, index, InsertNarrationText);
        });
        menu->addAction("Insert action before…", this,
                        [this, entry, index] { addBlock(false, entry, index); });
        auto up = menu->addAction("Move up", this,
                                  [this, entry, index] { mutateBlocks(entry, index, -1, false); });
        up->setEnabled(index > 0);
        auto down = menu->addAction("Move down", this,
                                    [this, entry, index] { mutateBlocks(entry, index, 1, false); });
        down->setEnabled(index + 1 < entryBlocks.size());
        if (b.opcode >= 0) {
            menu->addAction("Copy action", this, [this, entry, index] {
                int target = index;
                if (!attempt([&] { target = resolveDisplayedBlock(entry, index); }) || !flush())
                    return;
                auto data = new QMimeData;
                const auto bs = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
                data->setData("application/x-impera-dialogue-action", bs[target].bytes);
                data->setText(actionName(bs[target].opcode));
                QApplication::clipboard()->setMimeData(data);
            });
        }
        menu->addAction("Paste action before", this, [this, entry, index] {
            const auto data =
                QApplication::clipboard()->mimeData()->data("application/x-impera-dialogue-action");
            if (!data.isEmpty())
                attempt([&] {
                    const auto copied = blocks(data);
                    U5::require(
                        copied.size() == 1 && copied[0].opcode > 0 && copied[0].opcode != 135 &&
                            copied[0].opcode != 144,
                        "Clipboard must contain one supported action, not conversation structure");
                    insertBlock(entry, index, data);
                });
        });
        menu->addAction(b.opcode < 0 ? "Remove paragraph" : "Remove action", this,
                        [this, entry, index] { mutateBlocks(entry, index, 0, true); });
        more->setMenu(menu);
        if (b.opcode >= 0) {
            tools->addWidget(more);
            layout->addLayout(tools);
        } else {
            delete tools;
        }
        if (b.opcode < 0) {
            QByteArray visible = b.bytes, spacing;
            const int followingAction = actionAfterStateActions(entryBlocks, index + 1);
            spacing = managedSpacing(visible, followingAction);
            if (spacing.isEmpty() && index + 1 < entryBlocks.size() &&
                entryBlocks[index + 1].opcode < 0 && visible.endsWith(QByteArray::fromHex("8d8d")))
                spacing = QByteArray::fromHex("8d8d");
            visible.chop(spacing.size());
            const bool speech = speechAt(entryBlocks, index, entry != 1);
            QByteArray opening, closing;
            if (speech && visible.startsWith(char(162))) {
                opening = visible.left(1);
                visible.remove(0, 1);
            }
            if (speech && visible.endsWith(char(162))) {
                closing = visible.right(1);
                visible.chop(1);
            }
            auto edit = new DialogueParagraph(plain(visible), group);
            edit->setAccessibleName(speech ? "Speech Text" : "Narration Text");
            edit->setProperty("dialogueTextKind", speech ? "speech" : "narration");
            if (visible.isEmpty() && !spacing.isEmpty())
                group->hide();
            edit->setObjectName("conversationBlockText");
            edit->setProperty("dialogueEntry", entry);
            edit->setProperty("dialogueBlock", index);
            edit->activated = [this, entry] { activateEntry(entry); };
            if (runic) {
                edit->setAccessibleName("Runic text");
                auto runeLabel = new QLabel("Runic text");
                layout->addWidget(runeLabel);
            }
            edit->fit();
            auto paragraphRow = new QHBoxLayout;
            auto kindLabel = new QLabel(speech ? "Speech" : "Narration");
            kindLabel->setObjectName("conversationTextKind");
            paragraphRow->addWidget(kindLabel, 0, Qt::AlignTop);
            paragraphRow->addWidget(edit, 1);
            paragraphRow->addWidget(more, 0, Qt::AlignTop);
            layout->addLayout(paragraphRow);
            auto original = edit->toPlainText();
            readers[entry].append([b, edit, original, spacing, opening, closing] {
                return edit->toPlainText() == original
                           ? b.bytes
                           : opening + text(edit->toPlainText()) + closing + spacing;
            });
            connect(edit, &QPlainTextEdit::textChanged, this, [this] { changed(); });
        } else if (b.opcode == 133 || b.opcode == 134 || b.opcode == 140 || b.opcode == 254 ||
                   (b.opcode >= 145 && b.opcode <= 159)) {
            auto row = new QHBoxLayout;
            auto value = new QSpinBox;
            int v = 0;
            if (b.opcode == 133) {
                for (int j = 1; j <= 3; ++j)
                    v = v * 10 + (U5::byte(b.bytes, j) & 127) - '0';
                value->setRange(0, 999);
                value->setPrefix("Gold ");
            } else if (b.opcode == 134) {
                v = U5::byte(b.bytes, 1) & 127;
                value->setRange(0, 127);
                value->setPrefix("Inventory code ");
            } else if (b.opcode == 254) {
                v = U5::byte(b.bytes, 1);
                value->setRange(1, 255);
                value->setPrefix("Karma ≥ ");
            }
            value->setValue(v);
            if (b.opcode == 133 || b.opcode == 254)
                row->addWidget(value);
            else
                value->hide();
            value->setParent(group);
            auto itemChoice = new QComboBox(group);
            if (b.opcode == 134) {
                auto options = inventoryOptions();
                for (auto it = options.begin(); it != options.end(); ++it)
                    itemChoice->addItem(it.value(), it.key());
                int item = itemChoice->findData(v);
                if (item < 0) {
                    itemChoice->addItem(QString("Raw operand %1 (advanced)").arg(v), v);
                    item = itemChoice->count() - 1;
                }
                itemChoice->setCurrentIndex(item);
                row->addWidget(itemChoice, 1);
            } else
                itemChoice->hide();
            auto dest = new QComboBox;
            dest->setParent(group);
            if (b.opcode == 140)
                dest->addItem("Return to ordinary topics", 0);
            for (auto q : doc.questions)
                dest->addItem(questionName(q.label), q.label);
            int target = b.opcode == 140                      ? int(U5::byte(b.bytes, 1)) - 144
                         : b.opcode == 254                    ? int(U5::byte(b.bytes, 2)) - 144
                         : b.opcode >= 145 && b.opcode <= 159 ? b.opcode - 144
                                                              : 0;
            if (b.opcode == 140 && U5::byte(b.bytes, 1) == 255)
                target = 0;
            int found = dest->findData(target);
            if (found < 0) {
                dest->addItem(QString("Missing Q%1").arg(target), target);
                found = dest->count() - 1;
            }
            dest->setCurrentIndex(found);
            if (b.opcode != 133 && b.opcode != 134)
                row->addWidget(dest, 1);
            else
                dest->hide();
            auto form = new QWidget(group);
            auto formLayout = new QVBoxLayout(form);
            formLayout->setContentsMargins(0, 0, 0, 0);
            formLayout->addLayout(row);
            layout->addWidget(form);
            form->hide();
            connect(summary, &QToolButton::clicked, this, [this, entry, form] {
                activateEntry(entry);
                form->setVisible(!form->isVisible());
            });
            auto updateSummary = [this, summary, b, value, itemChoice, dest] {
                QString label = actionName(b.opcode);
                if (b.opcode == 133)
                    label += QString(": %1 gold").arg(value->value());
                else if (b.opcode == 134)
                    label += ": " + itemChoice->currentText();
                else if (b.opcode == 254)
                    label =
                        QString("If karma ≥ %1 → %2").arg(value->value()).arg(dest->currentText());
                else if (b.opcode == 140)
                    label = "If introduced → " + dest->currentText();
                else
                    label = "Enter question → " + dest->currentText();
                summary->setText(label);
                summary->setToolTip(label + " — click to edit the parameters.");
            };
            updateSummary();
            connect(value, &QSpinBox::valueChanged, summary, [updateSummary] { updateSummary(); });
            connect(itemChoice, &QComboBox::currentIndexChanged, summary,
                    [updateSummary] { updateSummary(); });
            connect(dest, &QComboBox::currentIndexChanged, summary,
                    [updateSummary] { updateSummary(); });
            readers[entry].append([b, value, itemChoice, dest, v, target] {
                int t = dest->currentData().toInt(),
                    n = b.opcode == 134 ? itemChoice->currentData().toInt() : value->value();
                if (n == v && t == target)
                    return b.bytes;
                return action(b.opcode >= 145 && b.opcode <= 159 ? 144 + t : b.opcode, n, t);
            });
            connect(itemChoice, &QComboBox::currentIndexChanged, this, [this, entry] {
                activateEntry(entry);
                changed();
            });
            connect(value, &QSpinBox::valueChanged, this, [this, entry] {
                activateEntry(entry);
                changed();
            });
            connect(dest, &QComboBox::currentIndexChanged, this, [this, entry] {
                activateEntry(entry);
                changed();
            });
        } else {
            summary->setToolTip(b.opcode == 131
                                    ? paragraphSpacingBefore(entryBlocks, index)
                                          ? "Add paragraph spacing, then pause briefly."
                                          : "Pause briefly without changing the text layout."
                                    : "Use the action menu to copy, move or explicitly remove "
                                      "this control.");
            if (b.opcode == 142) {
                runic = !runic;
                summary->setText(runic ? "Begin runic text" : "End runic text");
            }
            connect(summary, &QToolButton::clicked, this, [this, entry] { activateEntry(entry); });
            readers[entry].append([b] { return b.bytes; });
        }
        readerRanges[entry].append({index, index + 1});
        sectionLayout->addWidget(group);
    }
    auto error = new QLabel;
    error->setWordWrap(true);
    error->setObjectName("conversationEntryError");
    error->setStyleSheet("color: #ec9696;");
    error->hide();
    entryErrors[entry] = error;
    sectionLayout->addWidget(error);
}

void ConversationEditor::changed() {
    if (loading)
        return;
    pending = true;
    if (sourceDraft) {
        status->setText("Source changes pending");
        timer.stop();
    } else {
        status->setText("Changes pending");
        timer.start();
    }
}
bool ConversationEditor::applyDraft() {
    timer.stop();
    if (!pending)
        return true;
    bool ok = attempt([&] {
        QByteArray bytes;
        if (sourceDraft) {
            auto encoded = U5::encodeText(source->toPlainText());
            if (wholeSource)
                bytes = encoded;
            else {
                auto e = doc.entries[selected];
                int length = e.bytes.size() + (e.terminated ? 1 : 0);
                bytes = doc.bytes().left(e.offset) + encoded + doc.bytes().mid(e.offset + length);
            }
        } else {
            auto next = doc;
            for (auto it = readers.cbegin(); it != readers.cend(); ++it) {
                QByteArray bytes = next.entries[it.key()].bytes.left(entryPrefix(it.key()));
                try {
                    for (const auto &read : it.value())
                        bytes += read();
                    if (auto error = entryErrors.value(it.key()))
                        error->hide();
                } catch (const std::exception &exception) {
                    if (auto error = entryErrors.value(it.key())) {
                        error->setText(QString::fromUtf8(exception.what()));
                        error->show();
                    }
                    throw;
                }
                next.entries[it.key()].bytes = bytes;
            }
            bytes = next.bytes();
        }
        bool wasSource = sourceDraft || aliasesDraft;
        int nextSelected = selected;
        auto next = parse(bytes);
        if (aliasesDraft) {
            QVector<Rule> rs = next.topics;
            for (auto q : next.questions)
                rs += q.replies;
            for (auto r : rs)
                if (r.response == selected) {
                    QStringList keys;
                    for (auto s : aliases->text().split(','))
                        keys << s.trimmed();
                    bytes = replaceAliases(next, r, keys);
                    nextSelected = r.keywords.first() + keys.size() * 2 - 1;
                    next = parse(bytes);
                    break;
                }
        }
        for (auto issue : next.issues()) {
            if (issue.error && entryErrors.contains(issue.entry)) {
                entryErrors[issue.entry]->setText(issue.message);
                entryErrors[issue.entry]->show();
            }
            U5::require(!issue.error, issue.message);
        }
        bool shapeChanged = false;
        if (!wasSource)
            for (auto it = entryShapes.cbegin(); it != entryShapes.cend(); ++it) {
                int prefix = 0;
                for (const auto &q : next.questions)
                    if (q.prompt == it.key())
                        prefix = 2;
                QVector<int> shape;
                for (const auto &block : paragraphBlocks(next.entries[it.key()].bytes.mid(prefix)))
                    shape.append(block.opcode);
                shapeChanged |= shape != it.value();
            }
        auto focused = QApplication::focusWidget();
        int focusEntry = -1, focusBlock = -1, focusPosition = 0;
        if (focused && focused->property("dialogueEntry").isValid()) {
            focusEntry = focused->property("dialogueEntry").toInt();
            focusBlock = focused->property("dialogueBlock").toInt();
            if (auto text = qobject_cast<QTextEdit *>(focused))
                focusPosition = text->textCursor().position();
            if (auto text = qobject_cast<QPlainTextEdit *>(focused))
                focusPosition = text->textCursor().position();
        }
        auto entries = conversations;
        entries[npcIndex].bytes = bytes;
        auto encoded = U5::writeDialogue(entries);
        commit(encoded, "Edit conversation");
        conversations = entries;
        doc = next;
        selected = qBound(0, nextSelected, int(doc.entries.size() - 1));
        pending = false;
        sourceDraft = false;
        aliasesDraft = false;
        sourceBaseline = bytes;
        refreshSource();
        nativeStale = true;
        transcript->clear();
        status->setText("Changes applied");
        buildOutline(selected);
        if (wasSource || shapeChanged) {
            loadEntry(qBound(0, selected, int(doc.entries.size() - 1)));
            if (shapeChanged && !wasSource) {
                for (auto widget : blocksWidget->findChildren<QWidget *>()) {
                    if (!widget->property("dialogueEntry").isValid() ||
                        widget->property("dialogueEntry").toInt() != focusEntry ||
                        widget->property("dialogueBlock").toInt() != focusBlock)
                        continue;
                    QTextDocument *document = nullptr;
                    if (auto text = qobject_cast<QTextEdit *>(widget))
                        document = text->document();
                    if (auto text = qobject_cast<QPlainTextEdit *>(widget))
                        document = text->document();
                    if (!document)
                        continue;
                    QTextCursor cursor(document);
                    cursor.setPosition(qMin(focusPosition, document->characterCount() - 1));
                    if (auto text = qobject_cast<QTextEdit *>(widget))
                        text->setTextCursor(cursor);
                    if (auto text = qobject_cast<QPlainTextEdit *>(widget))
                        text->setTextCursor(cursor);
                    widget->setFocus();
                    break;
                }
            }
        } else
            updateDiagnostics();
    });
    return ok;
}
bool ConversationEditor::flush() {
    if (applyDraft())
        return true;
    auto result =
        QMessageBox::question(this, "Invalid conversation draft",
                              "This draft cannot be applied. Correct it, or discard the draft to "
                              "continue.",
                              QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result == QMessageBox::Discard) {
        pending = false;
        sourceDraft = false;
        loadEntry(selected);
        return true;
    }
    return false;
}
void ConversationEditor::install(const QByteArray &bytes, const QString &description, int entry) {
    auto next = parse(bytes);
    for (auto i : next.issues())
        U5::require(!i.error, i.message);
    auto entries = conversations;
    entries[npcIndex].bytes = bytes;
    auto encoded = U5::writeDialogue(entries);
    commit(encoded, description);
    conversations = entries;
    doc = next;
    nativeStale = true;
    transcript->clear();
    pending = false;
    buildOutline(entry < 0 ? selected : entry);
    loadEntry(qBound(0, entry < 0 ? selected : entry, int(doc.entries.size() - 1)));
}
void ConversationEditor::updateDiagnostics() {
    issues = doc.issues();
    diagnostics->clear();
    for (auto i : issues) {
        auto item =
            new QListWidgetItem((i.error ? "Error: " : "Warning: ") + i.message, diagnostics);
        item->setData(Qt::UserRole, i.entry);
        item->setToolTip(QString("Entry %1").arg(i.entry));
    }
    budget->setText(
        QString("<a href=\"issues\">%3 issues</a> · %1 / 1024 bytes · %2 / 15 questions")
            .arg(doc.bytes().size())
            .arg(doc.questions.size() + (doc.tail >= 0 ? 1 : 0))
            .arg(issues.size()));
    if (selected >= 0 && selected < doc.entries.size() && auxiliary->isVisible() &&
        previewTabs->currentIndex() == 0) {
        try {
            Sandbox state;
            state.avatar = avatar->text().section(',', 0, 0).trimmed();
            if (state.avatar.isEmpty())
                state.avatar = "Avatar";
            state.introduced = introduced->isChecked();
            state.karma = karma->value();
            state.gold = gold->value();
            const auto preview = previewResponse(
                doc.entries[selected].bytes, selected,
                project->resources.contains("IBM.CH") ? project->data("IBM.CH") : QByteArray(),
                project->resources.contains("RUNES.CH") ? project->data("RUNES.CH") : QByteArray(),
                state);
            dosImage->setImage(preview.image);
            previewNotes->setText(preview.notes.join("\n"));
        } catch (const std::exception &error) {
            dosImage->clear();
            previewNotes->setText(QString::fromUtf8(error.what()));
        }
    }
}
int ConversationEditor::resolveDisplayedBlock(int entry, int block) const {
    if (block < 0 || !readerRanges.contains(entry))
        return block;
    const auto &ranges = readerRanges[entry];
    const auto &fields = readers[entry];
    int current = 0;
    for (int i = 0; i < ranges.size(); ++i) {
        if (block <= ranges[i].first)
            return current;
        const auto expanded = paragraphBlocks(fields[i]());
        if (block < ranges[i].second)
            return current + qMin(block - ranges[i].first, int(expanded.size()));
        current += expanded.size();
    }
    return current;
}
void ConversationEditor::mutateBlocks(int entry, int block, int delta, bool remove) {
    if (!attempt([&] { block = resolveDisplayedBlock(entry, block); }))
        return;
    if (!flush() || !doc.structuralError.isEmpty() || entry == doc.tail ||
        doc.rawEntries.contains(entry))
        return;
    attempt([&] {
        auto bs = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
        U5::require(block >= 0 && block < bs.size(), "This block no longer exists");
        if (bs[block].opcode == 131 || handoffSpacing(bs[block].opcode)) {
            int textBlock = block - 1;
            while (textBlock >= 0 &&
                   (bs[textBlock].opcode == 134 || bs[textBlock].opcode == 140))
                --textBlock;
            if (textBlock >= 0 && bs[textBlock].opcode < 0) {
                const auto spacing = managedSpacing(bs[textBlock].bytes, bs[block].opcode);
                bs[textBlock].bytes.chop(spacing.size());
                bs[block].bytes.prepend(spacing);
            }
        }
        if (remove)
            bs.removeAt(block);
        else {
            const int next = block + delta;
            if (next < 0 || next >= bs.size())
                return;
            bs.swapItemsAt(block, next);
        }
        auto next = doc;
        QByteArray bytes = next.entries[entry].bytes.left(entryPrefix(entry));
        for (const auto &b : bs)
            bytes += b.bytes;
        next.entries[entry].bytes = bytes;
        install(next.bytes(), remove ? "Remove response block" : "Move response block", entry);
    });
}
void ConversationEditor::insertBlock(int entry, int before, const QByteArray &bytes,
                                     bool resolved) {
    if (!resolved && !attempt([&] { before = resolveDisplayedBlock(entry, before); }))
        return;
    if (!flush() || !doc.structuralError.isEmpty() || entry == doc.tail ||
        doc.rawEntries.contains(entry))
        return;
    attempt([&] {
        U5::require(!bytes.contains(char(0)), "Clipboard cannot contain structural separators");
        auto addition = blocks(bytes);
        U5::require(!addition.isEmpty(), "Empty clipboard block");
        auto bs = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
        const int at = before < 0 ? int(bs.size()) : before;
        U5::require(at >= 0 && at <= bs.size(), "Insertion position no longer exists");
        if (addition.size() == 1 && handoffSpacing(addition[0].opcode)) {
            int textBlock = at - 1;
            while (textBlock >= 0 && (bs[textBlock].opcode == 134 || bs[textBlock].opcode == 140))
                --textBlock;
            if (textBlock >= 0 && bs[textBlock].opcode < 0 &&
                !plain(bs[textBlock].bytes).remove('"').trimmed().isEmpty()) {
                int existing = 0;
                const auto &textBytes = bs[textBlock].bytes;
                while (existing < textBytes.size() &&
                       U5::byte(textBytes, textBytes.size() - 1 - existing) == 141)
                    ++existing;
                const int required = handoffSpacing(addition[0].opcode);
                if (existing < required)
                    bs[textBlock].bytes += QByteArray(required - existing, char(141));
            } else if (textBlock >= 0 &&
                       (bs[textBlock].opcode == 129 || bs[textBlock].opcode == 142)) {
                addition.prepend({QByteArray(handoffSpacing(addition[0].opcode), char(141)), -1});
            }
        }
        for (int i = 0; i < addition.size(); ++i)
            bs.insert(at + i, addition[i]);
        auto next = doc;
        QByteArray result = next.entries[entry].bytes.left(entryPrefix(entry));
        for (const auto &b : bs) {
            result += b.bytes;
        }
        next.entries[entry].bytes = result;
        install(next.bytes(), "Insert response block", entry);
    });
}
void ConversationEditor::addBlock(bool isText, int entry, int before, int requestedCode) {
    if (entry < 0)
        entry = selected;
    if (!attempt([&] { before = resolveDisplayedBlock(entry, before); }))
        return;
    if (!flush() || !doc.structuralError.isEmpty() || entry == doc.tail ||
        doc.rawEntries.contains(entry))
        return;
    attempt([&] {
        if (before < 0) {
            const auto existing = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
            if (!existing.isEmpty()) {
                const int terminal = existing.last().opcode;
                if (terminal == 130 || terminal == 255 || (terminal >= 145 && terminal <= 159))
                    before = existing.size() - 1;
            }
        }
        QByteArray bytes;
        if (isText || requestedCode == InsertSpeechText || requestedCode == InsertNarrationText) {
            const auto existing = paragraphBlocks(doc.entries[entry].bytes.mid(entryPrefix(entry)));
            const int at = before < 0 ? int(existing.size()) : before;
            U5::require(at >= 0 && at <= existing.size(), "Paragraph position no longer exists");
            int offset = entryPrefix(entry);
            for (int i = 0; i < at; ++i)
                offset += existing[i].bytes.size();
            const bool narration = requestedCode == InsertNarrationText;
            bool prompt = false;
            for (const auto &q : doc.questions)
                prompt |= q.prompt == entry;
            auto next = doc;
            next.entries[entry].bytes = insertTextParagraph(
                doc.entries[entry].bytes, offset,
                narration ? ParagraphKind::Narration : ParagraphKind::Speech,
                narration ? "New narration." : "New speech.", entry != 1, !prompt);
            install(next.bytes(), narration ? "Insert narration text" : "Insert speech text",
                    entry);
            return;
        } else {
            int code = requestedCode;
            if (code == -1) {
                QDialog dlg(this);
                dlg.setWindowTitle("Insert action or variable");
                auto layout = new QVBoxLayout(&dlg);
                auto choice = new QComboBox;
                choice->setObjectName("conversationActionChoice");
                for (int code : {129, 142, 130, InsertParagraphPause, 131, 132, 133, 134, 136, 137,
                                 138, 139, 140, 143, 254, 255}) {
                    if ((code == 140 || code == 254) && doc.questions.isEmpty())
                        continue;
                    choice->addItem(code == InsertParagraphPause ? "Pause after paragraph"
                                    : code == 131                ? "Brief pause (no line break)"
                                                                 : actionName(code),
                                    code);
                }
                choice->addItem("Explicit line break", 141);
                if (!doc.questions.isEmpty())
                    choice->addItem("Enter question", 145);
                layout->addWidget(choice);
                auto buttons =
                    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                layout->addWidget(buttons);
                connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
                if (dlg.exec() != QDialog::Accepted)
                    return;
                code = choice->currentData().toInt();
            }
            U5::require(!doc.questions.isEmpty() || (code != 140 && code != 254 && code != 145),
                        "Add a question before inserting a branch to it");
            const int label = doc.questions.isEmpty() ? 0 : doc.questions.first().label;
            if (code == 141)
                bytes = text("\n");
            else if (code == InsertParagraphPause)
                bytes = text("\n\n") + action(131);
            else
                bytes = action(code == 145 ? 144 + label : code,
                               code == 134   ? 65
                               : code == 254 ? 50
                                             : 0,
                               label);
        }
        insertBlock(entry, before, bytes, true);
    });
}
void ConversationEditor::aliasesChanged() {
    if (loading)
        return;
    QVector<Rule> rs = doc.topics;
    for (auto q : doc.questions)
        rs += q.replies;
    for (auto r : rs)
        if (r.response == selected) {
            QStringList keys;
            for (auto k : r.keywords)
                keys << keyword(doc.entries[k].bytes);
            if (aliases->text() != keys.join(", ")) {
                aliasesDraft = true;
                pending = true;
                applyDraft();
            }
            break;
        }
}

void ConversationEditor::addTopic(bool reply) {
    if (!flush() || !doc.structuralError.isEmpty())
        return;
    attempt([&] {
        int before = doc.questions.isEmpty() ? (doc.tail >= 0 ? doc.tail : doc.entries.size())
                                             : doc.questions.first().prompt;
        if (reply) {
            int index = -1;
            for (int i = 0; i < doc.questions.size(); ++i) {
                auto q = doc.questions[i];
                int end = i + 1 < doc.questions.size() ? doc.questions[i + 1].prompt
                          : doc.tail >= 0              ? doc.tail
                                                       : doc.entries.size();
                if (selected >= q.prompt && selected < end)
                    index = i;
            }
            U5::require(index >= 0, "Select a question or one of its replies first");
            before = index + 1 < doc.questions.size() ? doc.questions[index + 1].prompt
                     : doc.tail >= 0                  ? doc.tail
                                                      : doc.entries.size();
        }
        bool ok;
        QString key = QInputDialog::getText(this, reply ? "Add reply" : "Add topic",
                                            "What the player types", QLineEdit::Normal, {}, &ok);
        if (!ok)
            return;
        install(insertRule(doc, before, key), reply ? "Add question reply" : "Add topic",
                before + 1);
    });
}
void ConversationEditor::deleteSelected() {
    if (!flush() || !doc.structuralError.isEmpty())
        return;
    attempt([&] {
        for (auto q : doc.questions)
            if (q.prompt == selected) {
                auto refs = references(doc, q.label);
                int target = 0;
                if (!refs.isEmpty()) {
                    QStringList options;
                    QVector<int> labels;
                    for (auto x : doc.questions)
                        if (x.label != q.label) {
                            options << questionName(x.label);
                            labels << x.label;
                        }
                    U5::require(!labels.isEmpty(),
                                "This question has callers. Remove their branch actions "
                                "before deleting the last question.");
                    bool ok;
                    auto answer = QInputDialog::getItem(
                        this, "Referenced question",
                        QString("%1 callers must be retargeted before deletion.").arg(refs.size()),
                        options, 0, false, &ok);
                    if (!ok)
                        return;
                    target = labels[options.indexOf(answer)];
                }
                if (QMessageBox::question(this, "Delete question",
                                          "Delete the prompt, fallback and all replies?",
                                          QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
                    return;
                install(removeQuestion(doc, q.label, target), "Delete question", 0);
                return;
            }
        QVector<Rule> rs = doc.topics;
        for (auto q : doc.questions)
            rs += q.replies;
        for (auto r : rs)
            if (r.response == selected) {
                auto n = doc;
                for (int i = r.keywords.first(); i <= r.response; ++i)
                    n.entries.removeAt(r.keywords.first());
                install(n.bytes(), "Delete topic/reply", 0);
                return;
            }
        U5::require(false, "Fixed fields, fallbacks and structural tails cannot be deleted");
    });
}
void ConversationEditor::resetSimulation() {
    if (!flush())
        return;
    for (const auto &issue : doc.issues())
        if (issue.error) {
            status->setText("Resolve conversation errors before testing.");
            return;
        }
    NativeSetup config;
    if (project->resources.contains("INIT.GAM")) {
        try {
            config = nativeSetup(*project, resource, npcIndex, testNpc->currentData().toInt());
        } catch (const std::exception &error) {
            status->setText(QString::fromUtf8(error.what()));
            return;
        }
    }
    config.state.partyNames = avatar->text().split(',', Qt::SkipEmptyParts);
    for (auto &name : config.state.partyNames)
        name = name.trimmed();
    config.state.avatar =
        config.state.partyNames.isEmpty() ? "Avatar" : config.state.partyNames.first();
    config.state.introduced = introduced->isChecked();
    config.state.gold = gold->value();
    config.state.karma = karma->value();
    config.state.partySize = partySize->value();
    config.seed = uint16_t(seed->value());
    const auto generation = ++nativeGeneration;
    nativeSession.reset();
    nativeStale = false;
    transcript->clear();
    playtestImage->clear();
    nativeSession = std::make_unique<NativeDialogueSession>(
        doc.bytes(), project->resources.contains("IBM.CH") ? project->data("IBM.CH") : QByteArray(),
        project->resources.contains("RUNES.CH") ? project->data("RUNES.CH") : QByteArray(), config);
    connect(
        nativeSession.get(), &NativeDialogueSession::frameReady, this,
        [this, generation](const QImage &image) {
            if (generation == nativeGeneration)
                playtestImage->setImage(image);
        },
        Qt::QueuedConnection);
    connect(
        nativeSession.get(), &NativeDialogueSession::actionReady, this,
        [this, generation](const QString &text, int entry, int offset) {
            if (generation == nativeGeneration && !nativeStale)
                appendTranscript({{text, entry, true, offset}});
        },
        Qt::QueuedConnection);
    connect(
        nativeSession.get(), &NativeDialogueSession::finished, this,
        [this, generation](const QString &message) {
            if (generation == nativeGeneration) {
                status->setText(message);
                playtestStatus->setText(message);
            }
        },
        Qt::QueuedConnection);
    connect(
        nativeSession.get(), &NativeDialogueSession::stateReady, this,
        [this, generation](const QStringList &changes) {
            if (generation != nativeGeneration || nativeStale)
                return;
            for (const auto &change : changes)
                appendTranscript({{change, -1, true, -1}});
        },
        Qt::QueuedConnection);
    status->setText("Playtest running");
    playtestStatus->setText("Running");
    nativeSession->start();
    playtestImage->setFocus();
}
void ConversationEditor::appendTranscript(const QVector<Line> &lines) {
    for (auto l : lines) {
        auto previous = transcript->topLevelItem(transcript->topLevelItemCount() - 1);
        if (!l.trace && previous && !previous->data(0, Qt::UserRole + 1).toBool() &&
            previous->data(0, Qt::UserRole).toInt() == l.entry) {
            previous->setText(0, previous->text(0) + l.text);
            continue;
        }
        auto item = new QTreeWidgetItem(transcript, {(l.trace ? "↳ " : "") + l.text});
        item->setData(0, Qt::UserRole, l.entry);
        item->setData(0, Qt::UserRole + 1, l.trace);
        item->setData(0, Qt::UserRole + 2, l.offset);
        item->setHidden(l.trace && !showTrace->isChecked());
        if (l.trace)
            item->setForeground(0, QBrush(QColor("#94b9e7")));
    }
    transcript->scrollToBottom();
}

QString ConversationEditor::questionName(int label) const {
    QString key =
        resource + "/" + QString::number(conversations[npcIndex].id) + "/" + QString::number(label);
    QString name = project->dialogueNames.value(key);
    if (name.isEmpty()) {
        int i = doc.question(label);
        if (i >= 0)
            name = caption(doc.entries[doc.questions[i].prompt].bytes.mid(2));
    }
    return name.isEmpty() ? QString("Question %1").arg(label) : name;
}
void ConversationEditor::renameQuestion() {
    if (!flush())
        return;
    int label = 0;
    for (int i = 0; i < doc.questions.size(); ++i) {
        auto q = doc.questions[i];
        int end = i + 1 < doc.questions.size() ? doc.questions[i + 1].prompt
                  : doc.tail >= 0              ? doc.tail
                                               : doc.entries.size();
        if (selected >= q.prompt && selected < end)
            label = q.label;
    }
    if (!label) {
        status->setText("Select a question or one of its replies first.");
        return;
    }
    bool ok;
    QString name = QInputDialog::getText(this, "Name question",
                                         "Project-only name (does not change game dialogue)",
                                         QLineEdit::Normal, questionName(label), &ok);
    if (!ok)
        return;
    if (name.size() > 120) {
        status->setText("Question names are limited to 120 characters.");
        return;
    }
    rename(resource + "/" + QString::number(conversations[npcIndex].id) + "/" +
               QString::number(label),
           name);
    buildOutline(selected);
    loadEntry(selected);
}
void ConversationEditor::moveTopic(int delta, bool duplicate) {
    if (!flush())
        return;
    attempt([&] {
        QVector<Rule> rules = doc.topics;
        for (auto q : doc.questions)
            for (auto r : q.replies)
                if (r.response == selected)
                    rules = q.replies;
        int index = -1;
        for (int i = 0; i < rules.size(); ++i)
            if (rules[i].response == selected)
                index = i;
        U5::require(index >= 0, "Select a topic or question reply");
        auto r = rules[index];
        auto next = doc;
        int begin = r.keywords.first(), end = r.response + 1;
        auto part = doc.entries.mid(begin, end - begin);
        if (duplicate) {
            for (int i = 0; i < part.size(); ++i)
                next.entries.insert(end + i, part[i]);
            install(next.bytes(), "Duplicate topic/reply", end + part.size() - 1);
            return;
        }
        int other = index + delta;
        if (other < 0 || other >= rules.size())
            return;
        auto neighbor = rules[other];
        int at = delta < 0 ? neighbor.keywords.first() : neighbor.response + 1 - (end - begin);
        for (int i = begin; i < end; ++i)
            next.entries.removeAt(begin);
        for (int i = 0; i < part.size(); ++i)
            next.entries.insert(at + i, part[i]);
        install(next.bytes(), "Move topic/reply", at + part.size() - 1);
    });
}

void ConversationEditor::refreshSource() {
    QSignalBlocker blocked(source);
    sourceBaseline = wholeSource ? conversations[npcIndex].bytes
                                 : doc.entries[selected].bytes + (doc.entries[selected].terminated
                                                                      ? QByteArray(1, char(0))
                                                                      : QByteArray());
    source->setPlainText(U5::decodeText(sourceBaseline));
}

void ConversationEditor::focusCharacter() {
    findChild<QWidget *>("conversationBrowser")->hide();
    search->hide();
}

void ConversationEditor::hideEvent(QHideEvent *event) {
    if (auxiliary)
        auxiliary->hide();
    QWidget::hideEvent(event);
}

ConversationEditor::~ConversationEditor() {
    ++nativeGeneration;
    nativeSession.reset();
}
bool ConversationEditor::eventFilter(QObject *object, QEvent *event) {
    if (object == auxiliary && (event->type() == QEvent::Hide || event->type() == QEvent::Close)) {
        ++nativeGeneration;
        nativeSession.reset();
    }
    return QWidget::eventFilter(object, event);
}
