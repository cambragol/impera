#pragma once
#include "map_editor.h"
#include "mod/contents.h"
namespace Workshop {
struct ContentsTarget {
    int kind = 0, map = 0, floor = 0;
};
ContentsTarget contentsTarget(const QString &resource, const U5::MapPage &page, int index);
QVector<ContentsDef> readContents(const QByteArray &bytes);
QByteArray writeContents(const QVector<ContentsDef> &defs, const QByteArray &previous);
QVector<ContentsDef> inheritedContents(ContentsTarget target);
QString contentsItemName(ContentsItem item);
QVector<ContentsDef> mapContents(const Project &project, ContentsTarget target);
bool contentsHit(const Project &project, ContentsTarget target, QPoint cell);
class ContentsEditor : public QWidget {
  public:
    using Commit = std::function<bool(const QMap<QString, QByteArray> &, const QString &)>;
    ContentsEditor(Project *project, ContentsTarget target, MapCanvas *canvas, Commit commit,
                   QWidget *parent = nullptr);
    void selectCell(QPoint cell);
    void refresh();

  private:
    Project *project;
    ContentsTarget target;
    MapCanvas *canvas;
    Commit commit;
    QVector<ContentsDef> entries;
    ContentsDef selected{};
    QPoint cell{0, 0};
    bool loading = false;
    QLabel *title, *origin;
    QComboBox *mode, *interaction, *refill;
    QCheckBox *locked, *trapped;
    QSpinBox *lockDifficulty, *trapDifficulty;
    QTableWidget *items;
    QPushButton *add, *remove, *deleteContainer;
    QWidget *properties;
    QListWidget *locations;
    void load();
    void apply();
    bool change(QVector<ContentsDef> defs, const QString &title);
    void removeContainer();
    void addItem();
    void removeItem();
    bool move(int entry, QPoint destination);
};
} // namespace Workshop
