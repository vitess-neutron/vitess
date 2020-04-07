#ifndef MODULTABLE_H
#define MODULTABLE_H

#include <QWidget>
#include <QComboBox>
#include <QToolButton>
#include <QStringList>
#include <QHeaderView>

namespace Ui {
class ModulTable;
}

class ModulTable : public QWidget
{
    Q_OBJECT

public:
    explicit ModulTable(QStringList s1,QWidget *parent = nullptr);
    ~ModulTable();

    QStringList module[1000];
    int linenum;

signals:
    void changedCombo(QString text);

private slots:
    void comboModulItemChanged(QString);
    void arrowButtonPressed(bool);
    void showContextMenu(const QPoint&);
    void removeRow(int);
    void insertModule();
    void removeModule();
    void infoModule();

private:
    Ui::ModulTable *ui;
    QStringList modNames;
    void addNewRow();
    QVector<QComboBox *> comboModule;
    QVector<QToolButton *> arrowButton;
    QIcon *arrow;
    int minWidth;
    QMenu *context;
    QHeaderView *header;
};

#endif // MODULTABLE_H
