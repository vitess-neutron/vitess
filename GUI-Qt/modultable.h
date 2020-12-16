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
    QVector<bool> disableFlag;
    int linenum;
    void loadModule(QString text);
    void cleanModules();
    void addNewRow();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

signals:
    void arrowPressed(int row);
    void changedComboVal(QString text, int curRow);
    void removeCombo(int index);
    void insertCombo(int index);

private slots:
    void comboModulItemChanged(QString);
    void arrowButtonPressed(bool);
    void showContextMenu(const QPoint&);
    void disableModule();
    void enableModule();
    void enableAllModules();
    void removeModule();
    void insertModule();
    void infoModule();
private:
    Ui::ModulTable *ui;
    QStringList modNames;
    QVector<QComboBox *> comboModule;
    QVector<QToolButton *> arrowButton;
    QIcon *arrow;
    int minWidth;
    int oldRow;
    QMenu *context;
    QHeaderView *header;
};

#endif // MODULTABLE_H
