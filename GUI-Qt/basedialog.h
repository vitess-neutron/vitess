#ifndef BASEDIALOG_H
#define BASEDIALOG_H

#include <iostream>
#include <QDialog>
#include "yaml-cpp/yaml.h"
#include <QLineEdit>
#include <QComboBox>
#include <QList>

namespace Ui {
class BaseDialog;
}

class BaseDialog : public QDialog
{
    Q_OBJECT

public slots:
    void checkIsValide();

public:
    explicit BaseDialog(QWidget *parent = nullptr);
    ~BaseDialog();
    void writeParameter(YAML::Node& config,std::string moduleName);
    QList<QLineEdit *> allLineEdits;
    QList<QComboBox *> allComboBoxes;
    std::string modName;
    void setValidator(QLineEdit *lEdit, QMap <QString,QStringList> &map);
    void setValidatorFloat();
    QPalette palette;
    QLineEdit *testEdit;

private:
    Ui::BaseDialog *ui;
};

#endif // BASEDIALOG_H
