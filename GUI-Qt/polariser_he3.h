#ifndef POLARISER_HE3_H
#define POLARISER_HE3_H

#include "basemodule.h"

namespace Ui {
class Polariser_he3;
}

class Polariser_he3 : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Polariser_he3(BaseModule *parent = nullptr);
    ~Polariser_he3();

private slots:
    void on_BrowsePol_clicked();

    void on_BrowseTrans_clicked();

private:
    Ui::Polariser_he3 *ui;

    QMap<QString,QStringList> map = {
        {"bAnaCalc",  {"-a","combo",}},
        {"PolPcnt" ,  {"-b","float","",""}},
        {"Xsection",  {"-c","float","",""}},
        {"Density" ,  {"-d","float","",""}},
        {"sPolFil" ,  {"-P","file","",""}},
        {"sTrnsFil",  {"-T","file","",""}},
        {"PosX"    ,  {"-k","float","",""}},
        {"PosY"    ,  {"-l","float","",""}},
        {"PosZ"    ,  {"-m","float","",""}},
        {"Length"  ,  {"-X","float","",""}},
        {"Radius"  ,  {"-Y","float","",""}},
        {"FldGdeX" ,  {"-G","float","",""}},
        {"FldGdeY" ,  {"-H","float","",""}},
        {"FldGdeZ" ,  {"-K","float","",""}},
        {"FldPolX" ,  {"-M","float","",""}},
        {"FldPolY" ,  {"-N","float","",""}},
        {"FldPolZ" ,  {"-O","float","",""}},
        {"OutX"    ,  {"-p","float","",""}},
        {"OutY"    ,  {"-r","float","",""}},
        {"OutZ"    ,  {"-s","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // POLARISER_HE3_H
