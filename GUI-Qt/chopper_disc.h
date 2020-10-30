#ifndef CHOPPER_DISC_H
#define CHOPPER_DISC_H

#include "basemodule.h"
#include "chopper_para.h"

namespace Ui {
class Chopper_disc;
}

class Chopper_disc : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Chopper_disc(BaseModule *parent = nullptr);
    ~Chopper_disc();

private slots:
    void on_pushButton_clicked();

private:
    Ui::Chopper_disc *ui;
    QMap<QString,QStringList> map = {
        {"Rpm"   ,{ "-s","float","",""}},
        {"Offset",{ "-o","float","",""}},
        {"Dist"  ,{ "-l","float","",""}},
        {"eAbs"  ,{ "-g","combo",}},
        {"bZeroT",{ "-z","combo",}},
        {"bPass" ,{ "-p","combo",}},
        {"bColor",{ "-c","combo",}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
    Chopper_para param;
};

#endif // CHOPPER_DISC_H
