#ifndef PARAMETER_H
#define PARAMETER_H

#include <QWidget>
#include "tools.h"

namespace Ui {
class Parameter;
}

class Parameter : public QWidget
{
    Q_OBJECT

public:
    explicit Parameter(QWidget *parent = nullptr);
    ~Parameter();

private:
    Ui::Parameter *ui;
    QScrollArea *winScrollArea;
    QGridLayout *gridLayout;
    QMap<QString, QMap<QString,QString>> mapModule;
    void getModulSubParameter(YAML::Node& config,QString modulName);
    QPalette palette;
    QString fname;
public:
    void designParameterWin(QString filename);

private slots:
    void on_numberEdit_returnPressed();
    void on_butMinus_clicked();
    void on_butPlus_clicked();
    void browseBut_clicked();
    void checkIsValide();
    void on_pushClose_clicked();
};

#endif // PARAMETER_H
