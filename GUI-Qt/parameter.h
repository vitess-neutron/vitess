#ifndef PARAMETER_H
#define PARAMETER_H

#include <QWidget>
#include <QPlainTextEdit>
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
    QMap<QString, QMap<QString,QString>> mapModule;
    void designParameterWin(QString filename);
    void loadFile(QString filename);
    void saveData(YAML::Node& config,std::string key,QString param,QString parFile);
    QString instInDir;

private:
    Ui::Parameter *ui;
    QScrollArea *winScrollArea;
    QGridLayout *gridLayout;
    QPalette palette;
    QString initFile;
    QStringList multipleWin = {"moderator","chopper"};
    QList<QLineEdit *>  allLineEdits;
    QList<QComboBox *>  allComboBoxes;
    QList<QCheckBox *>  allCheckBoxes;
    void getModulSubParameter(YAML::Node& config,QString modulName);
    void getData(YAML::Node& configWin, int i);

signals:
    void changedParamWidget(QString fileName, QString initName);

private slots:
    void on_numberEdit_returnPressed();
    void on_butMinus_clicked();
    void on_butPlus_clicked();
    void browseBut_clicked();
    void editBut_clicked();
    void checkIsValide();
    void on_pushClose_clicked();
    void on_pushSave_clicked();
};

#endif // PARAMETER_H
