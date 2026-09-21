#include "dialogs.h"
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
class RuleDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        if (auto line = qobject_cast<QLineEdit *>(editor)) line->setMaxLength(std::numeric_limits<int>::max());
        return editor;
    }
};
QMessageBox::StandardButton askSave(QWidget *parent) {
    return QMessageBox::question(parent, "Save coloring settings?", "Save the changes to these coloring settings?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
}
}
QIcon colorIcon(const QColor &color) {
    QPixmap swatch(24, 24);
    swatch.fill(color);
    QPainter painter(&swatch);
    painter.setPen(Qt::gray);
    painter.drawRect(0, 0, 23, 23);
    return QIcon(swatch);
}
RuleTableModel::RuleTableModel(const QVector<ColorRule> &rules, QObject *parent) : QAbstractTableModel(parent), values(rules) {}
int RuleTableModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : values.size(); }
int RuleTableModel::columnCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : 2; }
QVariant RuleTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= values.size()) return {};
    const auto &rule = values[index.row()];
    if (role == Qt::DecorationRole && index.column() == 1) return colorIcon(rule.color);
    if (role == Qt::DisplayRole || role == Qt::EditRole) return index.column() == 0 ? rule.text : rule.color.name();
    if (role == Qt::ToolTipRole) return index.column() == 0 ? QString("Double-click to edit the text") : QString("Click to choose a color");
    return {};
}
QVariant RuleTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) return section == 0 ? QString("Text") : QString("Color");
    return QAbstractTableModel::headerData(section, orientation, role);
}
Qt::ItemFlags RuleTableModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index) | (index.isValid() && index.column() == 0 ? Qt::ItemIsEditable : Qt::NoItemFlags);
}
bool RuleTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= values.size() || role != Qt::EditRole) return false;
    if (index.column() == 0) values[index.row()].text = value.toString();
    else {
        QColor color(value.toString());
        if (!color.isValid()) return false;
        color.setAlpha(255);
        values[index.row()].color = color;
    }
    emit dataChanged(index, index);
    return true;
}
void RuleTableModel::add() {
    const int row = values.size();
    beginInsertRows({}, row, row);
    values.append({QString(), QColor("#ffdc80")});
    endInsertRows();
}
void RuleTableModel::remove(int row) {
    if (row < 0 || row >= values.size()) return;
    beginRemoveRows({}, row, row);
    values.removeAt(row);
    endRemoveRows();
}
RulesDialog::RulesDialog(const QString &title, const QVector<ColorRule> &rules, bool blocks, QWidget *parent)
    : QDialog(parent), model(new RuleTableModel(rules, this)), original(rules) {
    setWindowTitle(title);
    resize(640, 450);
    auto layout = new QVBoxLayout(this);
    auto help = new QLabel(blocks
        ? "Match the beginning of a paragraph (case-sensitive). A blank line ends the block. The longest prefix wins; ties use the later rule."
        : "Match text within a line, ignoring case. Spaces are allowed. Later rules win when phrases overlap.", this);
    help->setWordWrap(true);
    layout->addWidget(help);
    auto table = new QTableView(this);
    table->setModel(model);
    table->setItemDelegate(new RuleDelegate(table));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    table->setColumnWidth(1, 115);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    layout->addWidget(table);
    connect(table, &QTableView::clicked, this, [this](const QModelIndex &index) {
        if (index.column() != 1) return;
        const QColor color = QColorDialog::getColor(QColor(model->data(index, Qt::EditRole).toString()), this, "Choose highlight color");
        if (color.isValid()) model->setData(index, color.name());
    });
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto add = buttons->addButton("Add rule", QDialogButtonBox::ActionRole);
    auto remove = buttons->addButton("Remove selected", QDialogButtonBox::ActionRole);
    connect(add, &QPushButton::clicked, this, [this, table] {
        model->add();
        const QModelIndex index = model->index(model->rowCount() - 1, 0);
        table->setCurrentIndex(index);
        table->scrollTo(index);
        table->edit(index);
    });
    connect(remove, &QPushButton::clicked, this, [this, table] {
        QModelIndexList selected = table->selectionModel()->selectedRows();
        std::sort(selected.begin(), selected.end(), [](const QModelIndex &a, const QModelIndex &b) { return a.row() > b.row(); });
        for (const auto &index : selected) model->remove(index.row());
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
QVector<ColorRule> RulesDialog::rules() const { return model->rules(); }
void RulesDialog::done(int) {
    // Commit an active cell editor before comparing against the original model.
    if (QWidget *focused = focusWidget()) focused->clearFocus();
    if (model->rules() == original) { QDialog::done(QDialog::Rejected); return; }
    const auto answer = askSave(this);
    if (answer == QMessageBox::Cancel) return;
    if (answer == QMessageBox::Discard) { QDialog::done(QDialog::Rejected); return; }
    for (const auto &rule : model->rules()) {
        if (rule.text.isEmpty() || rule.text.contains('\n') || rule.text.contains('\r') || rule.text.contains(QChar(0x2028)) || rule.text.contains(QChar(0x2029))) {
            QMessageBox::warning(this, "Invalid rule", "Rules must contain nonempty text on a single line. Edit or remove the empty rule before saving.");
            return;
        }
    }
    QDialog::done(QDialog::Accepted);
}
FindColorDialog::FindColorDialog(const QColor &color, QWidget *parent) : QDialog(parent), original(color), selected(color) {
    setWindowTitle("Find Color");
    auto layout = new QVBoxLayout(this);
    auto button = new QPushButton(colorIcon(selected), selected.name(), this);
    button->setAccessibleName("Find highlight color");
    layout->addWidget(button);
    connect(button, &QPushButton::clicked, this, [this, button] {
        const QColor changed = QColorDialog::getColor(selected, this, "Find highlight color");
        if (!changed.isValid()) return;
        selected = changed;
        button->setIcon(colorIcon(selected));
        button->setText(selected.name());
    });
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
void FindColorDialog::done(int) {
    if (selected == original) { QDialog::done(QDialog::Rejected); return; }
    const auto answer = askSave(this);
    if (answer != QMessageBox::Cancel) QDialog::done(answer == QMessageBox::Save ? QDialog::Accepted : QDialog::Rejected);
}
ThemeColorsDialog::ThemeColorsDialog(const AppSettings &settings, QWidget *parent)
    : QDialog(parent), originalDay(settings.dayColors), originalNight(settings.nightColors),
      selectedDay(originalDay), selectedNight(originalNight) {
    setWindowTitle("Day / Night Colors");
    auto layout = new QVBoxLayout(this);
    auto form = new QFormLayout;
    layout->addLayout(form);
    auto add = [this, form](const QString &label, QColor *color) {
        auto button = new QPushButton(colorIcon(*color), color->name(), this);
        button->setAccessibleName(label);
        form->addRow(label, button);
        connect(button, &QPushButton::clicked, this, [this, button, color, label] {
            const QColor changed = QColorDialog::getColor(*color, this, label);
            if (!changed.isValid()) return;
            *color = changed;
            button->setIcon(colorIcon(*color));
            button->setText(color->name());
        });
    };
    add("Day: editor background", &selectedDay.editor);
    add("Day: menus and window", &selectedDay.window);
    add("Day: selection", &selectedDay.selection);
    add("Night: editor background", &selectedNight.editor);
    add("Night: menus and window", &selectedNight.window);
    add("Night: selection", &selectedNight.selection);
    auto hint = new QLabel("Text colors adapt automatically to keep text readable on the chosen backgrounds.", this);
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}
void ThemeColorsDialog::done(int) {
    if (selectedDay == originalDay && selectedNight == originalNight) { QDialog::done(QDialog::Rejected); return; }
    const auto answer = QMessageBox::question(this, "Save theme colors?", "Save these Day and Night colors?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer != QMessageBox::Cancel) QDialog::done(answer == QMessageBox::Save ? QDialog::Accepted : QDialog::Rejected);
}
