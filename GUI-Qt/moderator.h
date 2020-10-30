#ifndef MODERATOR_H
#define MODERATOR_H

#include "yaml-cpp/yaml.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

namespace Ui {
class Moderator;
}

class Moderator : public QDialog
{
    Q_OBJECT

public:
    explicit Moderator(QWidget *parent = nullptr);
    ~Moderator();
    void writePara(YAML::Node& config,std::string moduleName);
    void readPara(YAML::Node& config);
    void writePipe(QTextStream& out);

private slots:
    void on_nrMod_valueChanged(int arg1);

private:
    Ui::Moderator *ui;
    QMap<QString,QString> map = {
    };
    QWidgetList ModWindows;
    QStringList StrEdit = {"ModName","LmbdFile","L_T_File","TimeFile"};
    QList<QLineEdit *> lEdits;
    QList<QComboBox *> cBoxes;
    bool flag;
};

#endif // MODERATOR_H
