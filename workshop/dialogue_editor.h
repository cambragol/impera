#pragma once
#include "dialogue.h"
#include "project.h"
#include <QtWidgets>
#include <functional>
namespace Dialogue {
class NativeTextViewport;
}
class ConversationEditor : public QWidget {
  public:
    ConversationEditor(Project *project, const QString &resource,
                       std::function<void(const QByteArray &, const QString &)> commit,
                       std::function<void(const QString &, const QString &)> rename, int npc,
                       int entry, std::function<void(int, int)> selection,
                       QWidget *parent = nullptr);
    bool flush();
    void focusCharacter();

  protected:
    void hideEvent(QHideEvent *) override;

  private:
    Project *project;
    QString resource;
    QVector<U5::Conversation> conversations;
    Dialogue::Document doc;
    std::function<void(const QByteArray &, const QString &)> commit;
    std::function<void(int, int)> selection;
    std::function<void(const QString &, const QString &)> rename;
    QString questionName(int label) const;
    void renameQuestion();
    void moveTopic(int delta, bool duplicate);
    QTreeWidget *npcs, *outline;
    QLineEdit *search, *aliases;
    QLabel *budget, *context, *status;
    QTabWidget *tabs, *previewTabs;
    QPlainTextEdit *source;
    QDialog *auxiliary = nullptr;
    QWidget *keywordChips;
    QCheckBox *showTrace;
    QPushButton *replyButton, *deleteButton, *upButton, *downButton, *duplicateButton, *nameButton;
    QComboBox *sourceScope, *locationFilter;
    void filterNpcs();
    bool wholeSource = false;
    void refreshSource();
    Dialogue::NativeTextViewport *dosImage;
    QLabel *previewNotes;
    QWidget *blocksWidget;
    QVBoxLayout *blocksLayout;
    QTimer timer;
    QMap<int, QVector<std::function<QByteArray()>>> readers;
    QVector<Dialogue::Block> currentBlocks;
    QMap<int, QVector<int>> entryShapes;
    QMap<int, QVector<QPair<int, int>>> readerRanges;
    int resolveDisplayedBlock(int entry, int block) const;
    QMap<int, QLabel *> entryErrors;
    QVector<Dialogue::Issue> issues;
    QListWidget *diagnostics;
    QTreeWidget *transcript;
    QLineEdit *input, *avatar;
    QCheckBox *introduced, *announce;
    QSpinBox *gold, *karma, *partySize;
    std::unique_ptr<Dialogue::Simulator> simulation;
    int npcIndex = -1, selected = -1, prefix = 0;
    bool loading = false, pending = false, sourceDraft = false, aliasesDraft = false;
    QByteArray sourceBaseline;
    void loadNpc(int index, int entry = 0);
    void buildOutline(int selectEntry);
    void loadEntry(int entry);
    void buildBlocks();
    void buildEntryBlocks(int entry, QVBoxLayout *layout);
    void buildKeywords(int entry, QBoxLayout *layout);
    void replaceKeywords(int entry, const QStringList &keywords);
    void activateEntry(int entry);
    int entryPrefix(int entry) const;
    int selectedQuestion() const;
    void updateOutlineActions();
    void openAuxiliary(int tab);
    void insertBlock(int entry, int before, const QByteArray &bytes, bool resolved = false);
    void changed();
    bool applyDraft();
    void install(const QByteArray &bytes, const QString &description, int selectEntry = -1);
    void updateDiagnostics();
    void mutateBlocks(int entry, int block, int delta, bool remove);
    void addBlock(bool isText, int entry = -1, int before = -1, int requestedCode = -1);
    void aliasesChanged();
    void addTopic(bool reply);
    void deleteSelected();
    void resetSimulation();
    void appendTranscript(const QVector<Dialogue::Line> &lines);
    bool attempt(const std::function<void()> &operation);
};
