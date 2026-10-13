#include "scene_editor.h"
#include "mod/creation_scene.h"
#include <memory>
using U5::require;
namespace Workshop {
QStringList SceneDocument::names() {
    return {"Blackthorn imprisonment", "Shrine ritual", "Codex interaction", "Final rescue",
            "Character creation"};
}
QStringList SceneDocument::stages() {
    return {"The Summoning", "The Journey", "The Arrival", "The Welcoming"};
}
static const QStringList operations = {
    "Place actor",          "Remove actor",        "Move actor",        "Wait",
    "Open moongate",        "Close moongate",      "Change background", "Reveal actor",
    "Disappearance effect", "Restart sequence",    "Change terrain",    "Lightning effect",
    "Clear actors",         "Move actor and wait", "Begin repeat",      "End repeat"};
static const unsigned char *raw(const QByteArray &b) {
    return reinterpret_cast<const unsigned char *>(b.constData());
}
QByteArray SceneDocument::map(const QByteArray &b, int scene, int stage) {
    require(scene >= 0 && scene < 5 && stage >= 0 && stage < 4, "Invalid scene/background");
    int offset = scene < 4 ? scene * 176 : 704 + stage * 128, width = scene < 4 ? 11 : 19,
        rows = scene < 4 ? 11 : 4, stride = scene < 4 ? 16 : 32;
    require(b.size() >= offset + rows * stride, "Scene background is truncated");
    QByteArray out;
    for (int y = 0; y < rows; ++y)
        out += b.mid(offset + y * stride, width);
    return out;
}
QByteArray SceneDocument::changeMap(const QByteArray &b, int scene, int stage,
                                    const QByteArray &cells) {
    auto old = map(b, scene, stage);
    require(cells.size() == old.size(), "Wrong scene dimensions");
    auto out = b;
    int offset = scene < 4 ? scene * 176 : 704 + stage * 128, width = scene < 4 ? 11 : 19,
        stride = scene < 4 ? 16 : 32;
    for (int y = 0; y < cells.size() / width; ++y)
        out.replace(offset + y * stride, width, cells.mid(y * width, width));
    return out;
}
QVector<SceneStep> SceneDocument::steps(const QByteArray &b) {
    require(b.size() > 1216 && b.size() <= 2704,
            "Character creation block must fit the engine's 2000-byte buffer");
    QVector<SceneStep> out;
    for (int pos = 1216; pos < b.size();) {
        int op = uchar(b[pos]), size = U5_CreationStepSize(op);
        require(size <= b.size() - pos, "Truncated creation instruction");
        out.append(
            {pos, op, b.mid(pos, size),
             op < 16 ? operations[op] : QString("Unsupported operation %1 (read-only)").arg(op)});
        auto &step = out.back();
        if (op == 0)
            step.title += QString(" %1 at (%2,%3)")
                              .arg(uchar(step.bytes[1]) + 1)
                              .arg(uchar(step.bytes[3]))
                              .arg(uchar(step.bytes[4]));
        else if (op == 1 || op == 7 || op == 8)
            step.title += QString(" %1").arg(uchar(step.bytes[1]) + 1);
        else if (op == 2 || op == 13) {
            auto dir = uchar(step.bytes[2]);
            step.title += QString(" %1 · %2")
                              .arg(uchar(step.bytes[1]) + 1)
                              .arg(dir < 4 ? QStringList{"North", "East", "South", "West"}[dir]
                                           : "Invalid direction");
        } else if (op == 3)
            step.title += QString(" · %1 ticks").arg(uchar(step.bytes[1]));
        else if (op == 6) {
            int stage = uchar(step.bytes[1]);
            step.title += QString(" · %1").arg(stage < 4 ? stages()[stage] : "Invalid background");
        } else if (op == 14)
            step.title += QString(" · %1 times").arg(uchar(step.bytes[1]));
        pos += size;
        if (op == 9)
            return out;
    }
    throw std::runtime_error("Creation sequence has no restart instruction");
}
void SceneDocument::validate(const QByteArray &b) {
    steps(b);
    U5CreationPreview state;
    require(U5_CreationPreview(raw(b) + 704, b.size() - 704, ~0u, &state),
            "Invalid creation sequence: check actors, coordinates, waits and repeat pairs");
}
QByteArray SceneDocument::changeSteps(const QByteArray &b, const QVector<QByteArray> &s) {
    auto old = steps(b);
    for (auto &step : old)
        require(step.opcode < 16,
                "Unknown operations prevent sequence editing; original bytes must be preserved");
    QByteArray stream;
    for (auto &step : s) {
        require(!step.isEmpty() && uchar(step[0]) < 16 &&
                    step.size() == int(U5_CreationStepSize(uchar(step[0]))),
                "Invalid instruction size");
        stream += step;
    }
    require(!s.isEmpty() && uchar(s.back()[0]) == 9, "Keep Restart sequence as the final step");
    for (int i = 0; i < s.size() - 1; ++i)
        require(uchar(s[i][0]) != 9, "Restart must be the final step");
    auto out = b.left(1216) + stream + b.mid(old.back().offset + old.back().bytes.size());
    validate(out);
    return out;
}
QByteArray SceneDocument::preview(const QByteArray &b, int index, QVector<MapCanvas::Actor> *actors,
                                  int *stage) {
    auto list = steps(b);
    require(index >= 0 && index < list.size(), "Invalid preview step");
    U5CreationPreview state;
    require(U5_CreationPreview(raw(b) + 704, b.size() - 704, list[index].offset - 704, &state),
            "Cannot preview this instruction safely");
    if (stage)
        *stage = state.stage;
    if (actors) {
        actors->clear();
        for (int i = 0; i < 32; ++i)
            if (state.actors[i].tile) {
                auto a = state.actors[i];
                actors->append({a.x, a.y, a.tile, i, QString()});
            }
    }
    return QByteArray(reinterpret_cast<const char *>(state.terrain), 76);
}
QPoint SceneDocument::affectedCell(const QByteArray &b, int index) {
    auto list = steps(b);
    require(index >= 0 && index < list.size(), "Invalid scene step");
    auto step = list[index];
    U5CreationPreview state;
    require(U5_CreationPreview(raw(b) + 704, b.size() - 704, step.offset - 704, &state),
            "Cannot locate this step safely");
    if (step.opcode == 0 || step.opcode == 1 || step.opcode == 2 || step.opcode == 7 ||
        step.opcode == 8 || step.opcode == 13 || step.opcode == 11) {
        int id = uchar(step.bytes[step.opcode == 11 ? 2 : 1]);
        return {state.actors[id].x, state.actors[id].y};
    }
    if (step.opcode == 4)
        return {uchar(step.bytes[1]), uchar(step.bytes[2])};
    if (step.opcode == 5) {
        for (int i = index - 1; i >= 0; --i)
            if (list[i].opcode == 4)
                return {uchar(list[i].bytes[1]), uchar(list[i].bytes[2])};
    }
    if (step.opcode == 10)
        return {uchar(step.bytes[2]), uchar(step.bytes[3])};
    return {-1, -1};
}
QWidget *sceneEditor(Project *project, const QString &resource, TextCommit commit,
                     QMap<QString, int> *state, std::function<bool()> *flush) {
    if (resource != "MISCMAPS.DAT")
        return narrativeEditor(project, resource, commit, state, flush);
    SceneDocument::validate(project->data(resource));
    auto root = new QWidget;
    auto layout = new QVBoxLayout(root);
    auto bar = new QHBoxLayout;
    auto scenes = new QComboBox;
    scenes->setObjectName("sceneSelector");
    scenes->addItems(SceneDocument::names());
    scenes->setToolTip(
        "Choose a stored special-scene background or the character-creation animation.");
    auto stages = new QComboBox;
    stages->setObjectName("sceneBackground");
    stages->addItems(SceneDocument::stages());
    stages->setToolTip("Choose one of the four character-creation backgrounds. Titles and ambient "
                       "animation are fixed engine behavior.");
    bar->addWidget(new QLabel("Scene:"));
    bar->addWidget(scenes, 1);
    bar->addWidget(stages);
    layout->addLayout(bar);
    auto tabs = new QTabWidget;
    tabs->setObjectName("sceneTabs");
    layout->addWidget(tabs, 1);
    auto mapPage = new QWidget;
    auto mapLayout = new QVBoxLayout(mapPage);
    auto tools = new QHBoxLayout;
    auto canvas = new MapCanvas;
    canvas->setObjectName("sceneMap");
    canvas->tiles = U5::readGraphics("TILES.16", project->data("TILES.16")).images;
    auto group = new QButtonGroup(root);
    group->setExclusive(true);
    for (int tool : {MapCanvas::Pencil, MapCanvas::Eyedropper, MapCanvas::Fill,
                     MapCanvas::Rectangle, MapCanvas::Select, MapCanvas::Pan}) {
        auto button = new QToolButton;
        button->setIcon(MapCanvas::toolIcon(tool));
        button->setCheckable(true);
        button->setChecked(tool == 0);
        group->addButton(button, tool);
        button->setToolTip(
            QStringList{"Pencil: paint terrain", "Eyedropper: pick terrain", "Pan: move the view",
                        "Inspect", "Select: copy a terrain region", "Rectangle: paint a rectangle",
                        "Fill: replace connected terrain"}[tool]);
        tools->addWidget(button);
        QObject::connect(button, &QToolButton::clicked, root, [=] { canvas->tool = tool; });
    }
    canvas->chooseTool = [=](int tool) {
        canvas->tool = tool;
        if (auto b = group->button(tool))
            b->setChecked(true);
    };
    auto zoom = new QComboBox;
    zoom->addItems({"1×", "2×", "3×", "4×"});
    zoom->setCurrentIndex(1);
    zoom->setToolTip("Change preview magnification without changing stored pixels.");
    tools->addWidget(zoom);
    tools->addStretch();
    mapLayout->addLayout(tools);
    QObject::connect(zoom, &QComboBox::currentIndexChanged, root, [=](int i) {
        canvas->zoom = i + 1;
        canvas->resizeMap();
    });
    auto split = new QSplitter;
    auto scroll = new MapView(canvas);
    scroll->setAlignment(Qt::AlignCenter);
    split->addWidget(scroll);
    auto palette = new QListWidget;
    palette->setObjectName("sceneTiles");
    palette->setViewMode(QListView::IconMode);
    palette->setIconSize({32, 32});
    palette->setGridSize({42, 42});
    palette->setMaximumWidth(250);
    palette->setMinimumWidth(170);
    palette->setToolTip("Choose terrain to paint. Hover a tile for its name.");
    for (int i = 0; i < 256; ++i) {
        auto item = new QListWidgetItem(QPixmap::fromImage(canvas->tiles[i]), QString(), palette);
        item->setToolTip(MapDocument::tileName(i));
    }
    palette->setCurrentRow(1);
    QObject::connect(palette, &QListWidget::currentRowChanged, root,
                     [=](int i) { canvas->brush = i; });
    canvas->pick = [=](int i) { palette->setCurrentRow(i); };
    split->addWidget(palette);
    mapLayout->addWidget(split, 1);
    tabs->addTab(mapPage, "Map");
    auto messages = new QWidget;
    auto messageLayout = new QVBoxLayout(messages);
    tabs->addTab(messages, "Messages");
    auto nestedFlush = std::make_shared<std::function<bool()>>();
    auto messageWidget = std::make_shared<QWidget *>(nullptr);
    auto sequence = new QWidget;
    auto sequenceLayout = new QVBoxLayout(sequence);
    auto fixed = new QLabel;
    fixed->setWordWrap(true);
    sequenceLayout->addWidget(fixed);
    auto sequenceSplit = new QSplitter;
    auto list = new QListWidget;
    list->setObjectName("creationSteps");
    list->setToolTip("Select an instruction to preview its first execution. Repeat blocks run to "
                     "completion when previewing a later step.");
    sequenceSplit->addWidget(list);
    auto detail = new QWidget;
    auto form = new QVBoxLayout(detail);
    auto title = new QLabel;
    title->setWordWrap(true);
    form->addWidget(title);
    auto fields = new QFormLayout;
    form->addLayout(fields);
    auto operands = std::make_shared<QVector<QSpinBox *>>();
    for (int i = 0; i < 4; ++i) {
        auto f = new QSpinBox;
        f->setRange(0, 255);
        operands->append(f);
        fields->addRow("Parameter:", f);
    }
    auto direction = new QComboBox;
    direction->addItems({"North", "East", "South", "West"});
    direction->setToolTip("Direction the selected actor moves by one tile.");
    fields->addRow("Direction:", direction);
    auto background = new QComboBox;
    background->addItems(SceneDocument::stages());
    background->setToolTip("Background, fixed scene title and ambient animation to load.");
    fields->addRow("Background:", background);
    QObject::connect(direction, &QComboBox::currentIndexChanged, root,
                     [=](int i) { (*operands)[1]->setValue(i); });
    QObject::connect(background, &QComboBox::currentIndexChanged, root,
                     [=](int i) { (*operands)[0]->setValue(i + 1); });
    auto apply = new QPushButton("Apply step");
    apply->setObjectName("applyCreationStep");
    apply->setToolTip("Apply parameters only if the complete sequence remains safe to execute.");
    form->addWidget(apply);
    auto preview = new MapCanvas;
    preview->setObjectName("creationPreview");
    preview->tiles = canvas->tiles;
    preview->side = 19;
    preview->rows = 4;
    preview->zoom = 2;
    preview->tool = MapCanvas::InspectNpc;
    preview->setToolTip("Static state after the selected step; intermediate effects, audio and "
                        "precise timing are engine-controlled.");
    preview->resizeMap();
    auto pvScroll = new QScrollArea;
    pvScroll->setWidget(preview);
    form->addWidget(pvScroll, 1);
    auto status = new QLabel;
    status->setWordWrap(true);
    form->addWidget(status);
    auto locate = new QPushButton("Go to affected background");
    locate->setToolTip("Open the background and cell affected by the selected instruction.");
    form->addWidget(locate);
    sequenceSplit->addWidget(detail);
    sequenceLayout->addWidget(sequenceSplit, 1);
    auto actions = new QHBoxLayout;
    auto opChoice = new QComboBox;
    for (int i = 0; i < 16; ++i)
        if (i != 9 && i != 11)
            opChoice->addItem(operations[i], i);
    opChoice->setToolTip("Choose a verified operation to insert before the selected step. "
                         "Lightning's reserved operand is retained only in existing steps.");
    actions->addWidget(opChoice);
    auto add = new QPushButton("Insert");
    auto remove = new QPushButton("Delete");
    auto up = new QPushButton("Move up");
    auto down = new QPushButton("Move down");
    for (auto b : {add, remove, up, down}) {
        actions->addWidget(b);
        b->setToolTip("Change instruction order; invalid actor references, unmatched repeats or "
                      "out-of-bounds movement are rejected without changing the project.");
    }
    sequenceLayout->addLayout(actions);
    tabs->addTab(sequence, "Sequence");
    auto error = [=](const std::exception &e) {
        QMessageBox::warning(root, "Cannot edit scene", QString::fromUtf8(e.what()));
    };
    auto currentScene = std::make_shared<int>(-1);
    auto currentSteps = std::make_shared<QVector<SceneStep>>();
    auto stepDirty = std::make_shared<bool>(false);
    auto stepFlush = std::make_shared<std::function<bool()>>();
    for (auto field : *operands)
        QObject::connect(field, &QSpinBox::valueChanged, root, [=] { *stepDirty = true; });
    auto refresh = std::make_shared<std::function<void(int)>>();
    auto rebuild = std::make_shared<std::function<void()>>();
    *refresh = [=](int row) {
        if (row < 0 || row >= currentSteps->size())
            return;
        (*state)["scene/step"] = row;
        *stepDirty = false;
        auto step = (*currentSteps)[row];
        title->setText(step.title);
        QStringList labels;
        switch (step.opcode) {
        case 0:
            labels = {"Actor (1–32)", "Tile", "Column (0–18)", "Row (0–3)"};
            break;
        case 1:
        case 7:
        case 8:
            labels = {"Actor (1–32)"};
            break;
        case 2:
        case 13:
            labels = {"Actor (1–32)", "Direction (0 N, 1 E, 2 S, 3 W)"};
            break;
        case 3:
            labels = {"Wait ticks"};
            break;
        case 4:
            labels = {"Column (0–18)", "Row (0–3)"};
            break;
        case 6:
            labels = {"Background (1–4)"};
            break;
        case 10:
            labels = {"Tile", "Column (0–18)", "Row (0–3)"};
            break;
        case 11:
            labels = {"Reserved byte (preserved)", "Actor (1–32)"};
            break;
        case 14:
            labels = {"Repeat count"};
            break;
        }
        bool known = true;
        for (auto &s : *currentSteps)
            if (s.opcode >= 16)
                known = false;
        for (int i = 0; i < 4; ++i) {
            auto f = (*operands)[i];
            fields->labelForField(f)->setVisible(i < labels.size());
            f->setVisible(i < labels.size());
            if (i < labels.size()) {
                auto label = labels[i];
                static_cast<QLabel *>(fields->labelForField(f))->setText(label + ":");
                bool one = label.startsWith("Actor") || label.startsWith("Background");
                int max = label.startsWith("Actor")                                  ? 32
                          : label.startsWith("Background")                           ? 4
                          : label.startsWith("Column")                               ? 18
                          : label.startsWith("Row") || label.startsWith("Direction") ? 3
                                                                                     : 255;
                QSignalBlocker blocker(f);
                f->setRange(one || label.contains("ticks") || label.contains("count") ? 1 : 0, max);
                f->setValue(uchar(step.bytes[i + 1]) + (one ? 1 : 0));
                f->setToolTip(label);
                f->setEnabled(known && !(step.opcode == 11 && i == 0));
            }
        }
        bool hasDirection = step.opcode == 2 || step.opcode == 13, hasBackground = step.opcode == 6;
        direction->setVisible(hasDirection);
        fields->labelForField(direction)->setVisible(hasDirection);
        background->setVisible(hasBackground);
        fields->labelForField(background)->setVisible(hasBackground);
        direction->setEnabled(known);
        background->setEnabled(known);
        if (hasDirection) {
            QSignalBlocker b(direction);
            direction->setCurrentIndex(uchar(step.bytes[2]));
            (*operands)[1]->hide();
            fields->labelForField((*operands)[1])->hide();
        }
        if (hasBackground) {
            QSignalBlocker b(background);
            background->setCurrentIndex(uchar(step.bytes[1]));
            (*operands)[0]->hide();
            fields->labelForField((*operands)[0])->hide();
        }
        apply->setEnabled(known && !labels.isEmpty());
        for (auto b : {add, remove, up, down})
            b->setEnabled(known);
        remove->setEnabled(known && step.opcode != 9);
        up->setEnabled(known && row > 0 && step.opcode != 9);
        down->setEnabled(known && row + 1 < currentSteps->size() - 1);
        try {
            int stage;
            preview->ids =
                SceneDocument::preview(project->data(resource), row, &preview->actors, &stage);
            preview->update();
            status->setText(QString("Background %1. Static preview after step %2; visual effects "
                                    "and audio remain coded.%3")
                                .arg(stage + 1)
                                .arg(row + 1)
                                .arg(known ? "" : " Unknown operations: sequence is read-only."));
            locate->setEnabled(stage >= 0);
        } catch (const std::exception &e) {
            status->setText(e.what());
            locate->setEnabled(false);
        }
    };
    *rebuild = [=] {
        *currentSteps = SceneDocument::steps(project->data(resource));
        QSignalBlocker block(list);
        int row = qBound(0, state->value("scene/step"), int(currentSteps->size()) - 1);
        list->clear();
        for (auto &s : *currentSteps) {
            auto item =
                new QListWidgetItem(QString("%1. %2").arg(list->count() + 1).arg(s.title), list);
            item->setToolTip(QString("%1\nOriginal instruction bytes: %2\nFile offset: 0x%3")
                                 .arg(s.title, QString::fromLatin1(s.bytes.toHex(' ')))
                                 .arg(s.offset, 0, 16));
        }
        list->setCurrentRow(row);
        (*refresh)(row);
    };
    auto bytes = [=] {
        QVector<QByteArray> out;
        for (auto &s : *currentSteps)
            out.append(s.bytes);
        return out;
    };
    *stepFlush = [=]() -> bool {
        if (!*stepDirty)
            return true;
        try {
            auto all = bytes();
            int row = state->value("scene/step");
            auto &s = all[row];
            for (int i = 0; i < s.size() - 1; ++i) {
                if (s[0] == 11 && i == 0)
                    continue;
                auto label = static_cast<QLabel *>(fields->labelForField((*operands)[i]))->text();
                s[i + 1] =
                    char((*operands)[i]->value() -
                         ((label.startsWith("Actor") || label.startsWith("Background")) ? 1 : 0));
            }
            if (!commit({{resource, SceneDocument::changeSteps(project->data(resource), all)}},
                        "Edit creation step"))
                return false;
            *currentSteps = SceneDocument::steps(project->data(resource));
            *stepDirty = false;
            return true;
        } catch (const std::exception &e) {
            error(e);
            return false;
        }
    };
    auto currentTab = std::make_shared<int>(qBound(0, state->value("scene/tab"), 2));
    QObject::connect(tabs, &QTabWidget::currentChanged, root, [=](int tab) {
        if (!(*stepFlush)() || (*nestedFlush && !(*nestedFlush)())) {
            QSignalBlocker blocker(tabs);
            tabs->setCurrentIndex(*currentTab);
            return;
        }
        *currentTab = tab;
        (*state)["scene/tab"] = tab;
    });
    QObject::connect(list, &QListWidget::currentRowChanged, root, [=](int row) {
        if (!(*stepFlush)()) {
            QSignalBlocker blocker(list);
            list->setCurrentRow(state->value("scene/step"));
            return;
        }
        (*refresh)(row);
    });
    auto store = [=](QVector<QByteArray> steps, const QString &description, int selected = -1) {
        try {
            if (commit({{resource, SceneDocument::changeSteps(project->data(resource), steps)}},
                       description)) {
                if (selected >= 0)
                    (*state)["scene/step"] = selected;
                (*rebuild)();
            }
        } catch (const std::exception &e) {
            error(e);
        }
    };
    QObject::connect(apply, &QPushButton::clicked, root, [=] {
        if ((*stepFlush)())
            (*rebuild)();
    });
    QObject::connect(add, &QPushButton::clicked, root, [=] {
        if (!(*stepFlush)())
            return;
        auto all = bytes();
        int op = opChoice->currentData().toInt();
        QByteArray step(U5_CreationStepSize(op), 0);
        step[0] = char(op);
        if (op == 3 || op == 14)
            step[1] = 1;
        int row = list->currentRow();
        if (op == 14) {
            if ((*currentSteps)[row].opcode == 9)
                return;
            all.insert(row + 1, QByteArray(1, char(15)));
        }
        all.insert(row, step);
        store(all, "Insert creation step");
    });
    QObject::connect(remove, &QPushButton::clicked, root, [=] {
        if (!(*stepFlush)())
            return;
        auto all = bytes();
        all.removeAt(list->currentRow());
        store(all, "Delete creation step");
    });
    QObject::connect(up, &QPushButton::clicked, root, [=] {
        if (!(*stepFlush)())
            return;
        auto all = bytes();
        int row = list->currentRow();
        all.swapItemsAt(row, row - 1);
        store(all, "Reorder creation steps", row - 1);
    });
    QObject::connect(down, &QPushButton::clicked, root, [=] {
        if (!(*stepFlush)())
            return;
        auto all = bytes();
        int row = list->currentRow();
        all.swapItemsAt(row, row + 1);
        store(all, "Reorder creation steps", row + 1);
    });
    QObject::connect(locate, &QPushButton::clicked, root, [=] {
        int stage;
        QVector<MapCanvas::Actor> actors;
        SceneDocument::preview(project->data(resource), list->currentRow(), &actors, &stage);
        stages->setCurrentIndex(stage);
        auto cell = SceneDocument::affectedCell(project->data(resource), list->currentRow());
        if (cell.x() >= 0)
            canvas->keyboardCell = cell;
        canvas->update();
        tabs->setCurrentIndex(0);
        canvas->setFocus();
    });
    auto loadMap = [=] {
        int scene = *currentScene;
        canvas->side = scene < 4 ? 11 : 19;
        canvas->rows = scene < 4 ? 11 : 4;
        canvas->ids = SceneDocument::map(project->data(resource), scene, stages->currentIndex());
        canvas->original = SceneDocument::map(project->resources[resource].original, scene,
                                              stages->currentIndex());
        canvas->resizeMap();
    };
    canvas->commit = [=](const QByteArray &cells) {
        try {
            bool ok =
                commit({{resource, SceneDocument::changeMap(project->data(resource), *currentScene,
                                                            stages->currentIndex(), cells)}},
                       "Paint scene background");
            if (ok && *currentScene == 4)
                (*refresh)(list->currentRow());
            return ok;
        } catch (const std::exception &e) {
            error(e);
            return false;
        }
    };
    QObject::connect(stages, &QComboBox::currentIndexChanged, root, [=](int i) {
        canvas->cancelStroke();
        (*state)["scene/background"] = i;
        if (*currentScene >= 0)
            loadMap();
    });
    QObject::connect(scenes, &QComboBox::currentIndexChanged, root, [=](int scene) {
        if (!(*stepFlush)() || (*nestedFlush && !(*nestedFlush)())) {
            QSignalBlocker block(scenes);
            scenes->setCurrentIndex(*currentScene);
            return;
        }
        canvas->cancelStroke();
        *currentScene = scene;
        (*state)["scene/current"] = scene;
        stages->setVisible(scene == 4);
        loadMap();
        if (*messageWidget) {
            messageLayout->removeWidget(*messageWidget);
            delete *messageWidget;
            *messageWidget = nullptr;
        }
        *nestedFlush = {};
        QString msg = scene == 3 ? "ENDMSG.DAT" : scene == 4 ? "QUESTION.DAT" : "MISCMSG.DAT";
        auto wrapper = new QWidget;
        auto ml = new QVBoxLayout(wrapper);
        if (project->resources.contains(msg)) {
            auto editor = narrativeEditor(project, msg, commit, state, nestedFlush.get());
            ml->addWidget(editor);
            if (scene < 3) {
                auto entries = editor->findChild<QListWidget *>("narrativeEntries");
                int first = -1;
                auto catalog = narrativeEntries(msg);
                for (int i = 0; i < catalog.size(); ++i) {
                    bool show = QString::fromLatin1(catalog[i]->label)
                                    .startsWith(scene == 0   ? "Blackthorn"
                                                : scene == 1 ? "Shrine"
                                                             : "Codex");
                    entries->item(i)->setData(Qt::UserRole + 1, !show);
                    entries->item(i)->setHidden(!show);
                    if (show && first < 0)
                        first = i;
                }
                entries->setCurrentRow(first);
            }
        } else
            ml->addWidget(new QLabel("This project does not contain " + msg + "."));
        *messageWidget = wrapper;
        messageLayout->addWidget(wrapper);
        list->setVisible(scene == 4);
        detail->setVisible(scene == 4);
        for (auto b : {add, remove, up, down})
            b->setVisible(scene == 4);
        opChoice->setVisible(scene == 4);
        fixed->setText(
            scene == 4
                ? "The animation repeats until a key is pressed. Edit verified instructions; actor "
                  "numbers are local to this scene. No text opcode exists."
                : "This scene's actor placement, dialogue choices and timing are fixed engine "
                  "code. Its stored terrain and messages can be edited in the other tabs.");
    });
    if (flush)
        *flush = [=] {
            canvas->cancelStroke();
            return (*stepFlush)() && (!*nestedFlush || (*nestedFlush)());
        };
    (*rebuild)();
    stages->setCurrentIndex(qBound(0, state->value("scene/background"), 3));
    {
        QSignalBlocker block(scenes);
        scenes->setCurrentIndex(-1);
    }
    scenes->setCurrentIndex(qBound(0, state->value("scene/current"), 4));
    tabs->setCurrentIndex(*currentTab);
    return root;
}
} // namespace Workshop
