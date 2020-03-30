#ifndef MODULTABLE_H
#define MODULTABLE_H

#include <QWidget>
#include <QComboBox>
#include <QToolButton>
#include <QSignalMapper>
namespace Ui {
class ModulTable;
}

class ModulTable : public QWidget
{
    Q_OBJECT

public:
    explicit ModulTable(QWidget *parent = nullptr);
    ~ModulTable();
    QVector<QComboBox *> comboModule;
    QVector<QToolButton *> arrowButton;
    QIcon *arrow;
    QString modullist[3]={"--inactive--","Beamstop","Detector"};
    QStringList module[1000];
    int linenum;
    QSignalMapper mapper;

signals:
    void changedCombo(QString text);

private:
    Ui::ModulTable *ui;

private slots:
    void comboModulItemChanged(QString);
    void arrowButtonClicked(int row);
};

#endif // MODULTABLE_H
