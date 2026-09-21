#pragma once
#include "settings.h"
#include <QAbstractTableModel>
#include <QDialog>
#include <QIcon>

QIcon colorIcon(const QColor &color);
class RuleTableModel : public QAbstractTableModel {
public:
    explicit RuleTableModel(const QVector<ColorRule> &rules, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    void add();
    void remove(int row);
    QVector<ColorRule> rules() const { return values; }
private:
    QVector<ColorRule> values;
};

class RulesDialog : public QDialog {
public:
    RulesDialog(const QString &title, const QVector<ColorRule> &rules, bool blocks, QWidget *parent);
    QVector<ColorRule> rules() const;
protected:
    void done(int result) override;
private:
    RuleTableModel *model;
    QVector<ColorRule> original;
};
class FindColorDialog : public QDialog {
public:
    explicit FindColorDialog(const QColor &color, QWidget *parent);
    QColor color() const { return selected; }
protected:
    void done(int result) override;
private:
    QColor original, selected;
};
class ThemeColorsDialog : public QDialog {
public:
    explicit ThemeColorsDialog(const AppSettings &settings, QWidget *parent);
    ThemeColors dayColors() const { return selectedDay; }
    ThemeColors nightColors() const { return selectedNight; }
protected:
    void done(int result) override;
private:
    ThemeColors originalDay, originalNight, selectedDay, selectedNight;
};
