#include "engine_tables.h"
#include "engine_table_names.h"
#include "inventory_names.h"
#include "mod/engine_tables.h"
#include "overlay_inventory.h"
#include <QtWidgets>
using U5::require;
namespace Workshop {
static const unsigned char *raw(const QByteArray &b) {
    return reinterpret_cast<const unsigned char *>(b.constData());
}
void TableDocument::validate(const QByteArray &b) {
    require(b.isEmpty() || U5_ValidateTables(raw(b), b.size()),
            "Invalid engine table schema, index, ordering or value");
}
int TableDocument::value(const QByteArray &b, int table, int index) {
    validate(b);
    auto spec = U5_TableSpec(table);
    require(spec && index >= 0 && unsigned(index) < spec->rows * spec->columns,
            "Invalid table entry");
    return U5_TableValue(raw(b), b.size(), table, index, spec->defaults[index]);
}
QByteArray TableDocument::change(const QByteArray &b, int table, int index, int val, bool inherit) {
    validate(b);
    require(U5_TableEntryValid(table, index, val), "Table value or reference is out of bounds");
    QMap<unsigned, unsigned> entries;
    if (!b.isEmpty())
        for (int pos = 10; pos < b.size(); pos += 5)
            entries[(unsigned(uchar(b[pos])) << 16) | U5::word(b, pos + 1)] = U5::word(b, pos + 3);
    unsigned key = (unsigned(table) << 16) | unsigned(index);
    if (inherit)
        entries.remove(key);
    else
        entries[key] = val;
    if (entries.isEmpty())
        return {};
    QByteArray out("IRULES01", 8);
    out.append(char(entries.size()));
    out.append(char(entries.size() >> 8));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        out.append(char(it.key() >> 16));
        out.append(char(it.key()));
        out.append(char(it.key() >> 8));
        out.append(char(it.value()));
        out.append(char(it.value() >> 8));
    }
    validate(out);
    return out;
}
static QString location(int i) {
    return i >= 0 && i < 40 ? QString::fromUtf8(tableLocationNames[i]) : "Unassigned location";
}
QString TableDocument::entryLabel(int table, int index) {
    if (table == 1)
        return QString::fromUtf8(engineEquipmentNames[index]);
    if (table >= 7)
        return location(index);
    int columns = table == 6 ? 1 : 8, row = index / columns;
    if (table == 5) {
        columns = 4;
        row = index / 4;
    }
    int type = table == 2 || table == 3 ? 4 : table == 4 ? 0 : table == 5 ? 5 : 2;
    int shop = type * 16 + row;
    QString name = QString::fromUtf8(tableMerchantNames[shop]);
    QString result =
        name.isEmpty() ? "Unassigned shop (reserved)" : location(int(tableShopLocations[shop]) - 1);
    if (!name.isEmpty())
        result += " · " + name;
    if (table == 2 || table == 3)
        result += " · " + QString::fromUtf8(tableReagentNames[index % 8]);
    if (table == 4)
        result += QString(" · Stock position %1").arg(index % 8 + 1);
    if (table == 5)
        result += " · " + QStringList{"Keys (3)", "Gems (4)", "Torches (5)", "Reserved"}[index % 4];
    return result;
}
static bool overridden(const QByteArray &b, int table, int index) {
    if (b.isEmpty())
        return false;
    for (int p = 10; p < b.size(); p += 5)
        if (uchar(b[p]) == table && U5::word(b, p + 1) == unsigned(index))
            return true;
    return false;
}
QWidget *tableEditor(Project *project, const QString &resource, TextCommit commit,
                     QMap<QString, int> *state, std::function<bool()> *flush) {
    auto root = new QWidget;
    root->setObjectName("engineTableEditor");
    auto layout = new QVBoxLayout(root);
    auto tabs = new QTabWidget;
    layout->addWidget(tabs);
    auto editor = new QWidget;
    auto el = new QVBoxLayout(editor);
    auto note = new QLabel("Verified engine tables · Changes are exported as RULES.TBL, not "
                           "ineffective DATA.OVL bytes. Prices use the original intelligence "
                           "adjustment; omitted entries inherit compiled defaults.");
    note->setWordWrap(true);
    el->addWidget(note);
    auto kind = new QComboBox;
    kind->setObjectName("engineTableSelector");
    for (auto &spec : U5_EngineTables)
        kind->addItem(QString::fromUtf8(spec.name), spec.id);
    kind->setToolTip(
        "Choose a verified table. Each override has a typed, versioned runtime path in Impera.");
    el->addWidget(kind);
    auto table = new QTableWidget;
    table->setObjectName("engineTableEntries");
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"Item / location", "Value", "Source"});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setToolTip("Select an entry to override or restore its inherited compiled default. "
                      "Reserved fields remain read-only.");
    el->addWidget(table, 1);
    auto actions = new QHBoxLayout;
    auto value = new QSpinBox;
    value->setObjectName("engineTableValue");
    value->setToolTip(
        "Bounded value in gold, quantity or world coordinate, according to the selected table.");
    actions->addWidget(value);
    auto equipment = new QComboBox;
    equipment->setObjectName("engineStockItem");
    equipment->addItem("End of stock list / no item", 255);
    for (int i = 0; i < 48; ++i)
        equipment->addItem(QString::fromUtf8(engineEquipmentNames[i]), i);
    equipment->setToolTip("Equipment reference; End of stock list hides subsequent positions. The "
                          "final terminator is preserved.");
    actions->addWidget(equipment);
    auto apply = new QPushButton("Apply override");
    apply->setObjectName("applyEngineTable");
    apply->setToolTip("Override this entry. Undo restores the preceding value and inheritance.");
    actions->addWidget(apply);
    auto revert = new QPushButton("Use default");
    revert->setObjectName("inheritEngineTable");
    revert->setToolTip("Remove this specific override and inherit the engine's compiled default.");
    actions->addWidget(revert);
    actions->addStretch();
    el->addLayout(actions);
    auto preview = new QLabel;
    preview->setObjectName("engineTablePreview");
    preview->setWordWrap(true);
    el->addWidget(preview);
    auto select = [=] {
        int id = kind->currentData().toInt(), row = table->currentRow();
        auto spec = U5_TableSpec(id);
        bool enabled = row >= 0;
        if (enabled && id == 5 && (row >= 12 || row % 4 == 3))
            enabled = false;
        if (enabled && id == 4 && row % 8 == 7)
            enabled = false;
        if (enabled && id == 6 && row >= 3)
            enabled = false;
        value->setEnabled(enabled);
        equipment->setEnabled(enabled);
        apply->setEnabled(enabled);
        revert->setEnabled(enabled);
        value->setVisible(id != 4);
        equipment->setVisible(id == 4);
        value->setRange(0, spec->maximum);
        if (row < 0)
            return;
        (*state)["tables/row"] = row;
        int v = TableDocument::value(project->data("RULES.TBL"), id, row);
        value->setValue(v);
        equipment->setCurrentIndex(equipment->findData(v));
        QString explanation;
        if (id == 1)
            explanation = QString("Base price %1 gold. At intelligence 20, buy: %2 gold; sell: %3 "
                                  "gold. Original rounding and adjustments are retained.")
                              .arg(v)
                              .arg(v + v * 40 / 100)
                              .arg(20 * v * 3 / 100 + 1);
        else if (id == 2 || id == 3) {
            int price = TableDocument::value(project->data("RULES.TBL"), 2, row),
                qty = TableDocument::value(project->data("RULES.TBL"), 3, row);
            explanation = QString("Bundle: %1 reagents for base price %2 gold. A zero price omits "
                                  "it from this shop's list.")
                              .arg(qty)
                              .arg(price);
        } else if (id == 4)
            explanation = v == 255
                              ? "End marker: subsequent stock positions do not appear in this shop."
                              : "The shop offers " + QString::fromUtf8(engineEquipmentNames[v]) +
                                    " at the equipment table's adjusted price.";
        else if (id >= 7)
            explanation = QString("%1 entrance at (%2,%3). Enter and exit commands use this "
                                  "coordinate; paint matching terrain in Maps too. Moving an "
                                  "entrance does not move terrain automatically.")
                              .arg(location(row))
                              .arg(TableDocument::value(project->data("RULES.TBL"), 7, row))
                              .arg(TableDocument::value(project->data("RULES.TBL"), 8, row));
        else
            explanation = QString("Base price %1 gold. Original purchase rules and intelligence "
                                  "adjustment are retained.")
                              .arg(v);
        if (!enabled)
            explanation = "Reserved / terminator position: no editable runtime value.";
        preview->setText(explanation);
    };
    auto load = [=] {
        QSignalBlocker blocker(table);
        int id = kind->currentData().toInt();
        auto spec = U5_TableSpec(id);
        auto bytes = project->data("RULES.TBL");
        TableDocument::validate(bytes);
        table->setRowCount(spec->rows * spec->columns);
        for (int i = 0; i < table->rowCount(); ++i) {
            int v = TableDocument::value(bytes, id, i);
            auto item = new QTableWidgetItem(TableDocument::entryLabel(id, i));
            item->setToolTip(
                QString("%1 · Entry %2 · legacy candidate address 0x%3 (Advanced inventory)")
                    .arg(QString::fromUtf8(spec->symbol))
                    .arg(i)
                    .arg(spec->address, 0, 16));
            table->setItem(i, 0, item);
            table->setItem(i, 1,
                           new QTableWidgetItem(
                               id == 4 ? (v == 255 ? "End of list"
                                                   : QString::fromUtf8(engineEquipmentNames[v]))
                                       : QString::number(v)));
            table->setItem(i, 2,
                           new QTableWidgetItem(overridden(bytes, id, i) ? "Mod override"
                                                                         : "Compiled default"));
        }
        table->selectRow(qBound(0, state->value("tables/row", 0), table->rowCount() - 1));
        select();
    };
    QObject::connect(kind, qOverload<int>(&QComboBox::currentIndexChanged), root, [=](int) {
        (*state)["tables/kind"] = kind->currentData().toInt();
        (*state)["tables/row"] = 0;
        load();
    });
    QObject::connect(table, &QTableWidget::itemSelectionChanged, root, select);
    auto change = [=](bool inherit) {
        int id = kind->currentData().toInt(), index = table->currentRow();
        if (index < 0)
            return;
        int v = id == 4 ? equipment->currentData().toInt() : value->value();
        auto bytes = TableDocument::change(project->data("RULES.TBL"), id, index, v, inherit);
        if (commit({{"RULES.TBL", bytes}},
                   inherit ? "Use compiled table default" : "Override engine table"))
            load();
    };
    QObject::connect(apply, &QPushButton::clicked, root, [=] { change(false); });
    QObject::connect(revert, &QPushButton::clicked, root, [=] { change(true); });
    tabs->addTab(editor, "Shops and locations");
    auto inventory = new QWidget;
    auto il = new QVBoxLayout(inventory);
    auto context = new QLabel(
        "Coverage audit: only the Britannia chunk index is loaded from DATA.OVL. Verified concepts "
        "above have companion overrides. Other compiled tables, DOS-only layouts and unresolved "
        "spans are explicitly identified; candidate offsets require edition verification.");
    context->setWordWrap(true);
    il->addWidget(context);
    auto search = new QLineEdit;
    search->setObjectName("overlayInventorySearch");
    search->setPlaceholderText("Search table, consumer or classification…");
    search->setToolTip(
        "Filter the complete compiled-symbol, encoded-string and unresolved-span inventory.");
    il->addWidget(search);
    auto coverage = new QTreeWidget;
    coverage->setObjectName("overlayCoverage");
    coverage->setHeaderLabels({"Table / span", "Classification", "Advanced details / consumers"});
    coverage->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    QByteArray inventoryBytes;
    for (auto part : overlayInventoryParts)
        inventoryBytes += part;
    auto manifest = QJsonDocument::fromJson(inventoryBytes).object();
    for (auto value : manifest["entries"].toArray()) {
        auto e = value.toObject();
        QStringList refs;
        for (auto ref : e["references"].toArray()) {
            auto r = ref.toObject();
            QStringList lines;
            for (auto v : r["lines"].toArray())
                lines.append(QString::number(v.toInt()));
            refs.append(r["file"].toString() + ":" + lines.join(','));
        }
        QString detail =
            QString("%1 · candidate file offset 0x%2 · %3 bytes · %4\n%5")
                .arg(e["type"].toString())
                .arg(e["offset"].toInt(), 0, 16)
                .arg(e["length"].isNull() ? "unresolved" : QString::number(e["length"].toInt()))
                .arg(e["purpose"].toString())
                .arg(refs.join("; "));
        auto item = new QTreeWidgetItem(coverage,
                                        {e["symbol"].toString(), e["category"].toString(), detail});
        item->setToolTip(
            2, detail + (e["verifiedSpan"].toBool()
                             ? "\nVerified DOS span"
                             : "\nSpan/address-space unverified; not an editable runtime promise"));
    }
    for (auto value : manifest["unresolvedSpans"].toArray()) {
        auto e = value.toObject();
        new QTreeWidgetItem(
            coverage,
            {QString("Gap at 0x%1").arg(e["offset"].toInt(), 0, 16), "unresolved",
             QString("%1 bytes: unknown code, padding, aliases or data").arg(e["length"].toInt())});
    }
    coverage->setToolTip("Runtime coverage inventory. Native literals/tables are not loaded from "
                         "arbitrary overlay bytes.");
    il->addWidget(coverage, 1);
    QObject::connect(search, &QLineEdit::textChanged, root, [=](const QString &s) {
        for (int i = 0; i < coverage->topLevelItemCount(); ++i) {
            auto item = coverage->topLevelItem(i);
            item->setHidden(
                !(item->text(0) + item->text(1) + item->text(2)).contains(s, Qt::CaseInsensitive));
        }
    });
    tabs->addTab(inventory, "Overlay coverage inventory");
    tabs->setCurrentIndex(resource == "DATA.OVL" ? 1 : 0);
    kind->setCurrentIndex(qBound(0, state->value("tables/kind", 1) - 1, 7));
    load();
    *flush = [] { return true; };
    return root;
}
} // namespace Workshop
