#ifndef FLIPPER_GRADIENT_H
#define FLIPPER_GRADIENT_H

#include "basemodule.h"

namespace Ui {
class Flipper_gradient;
}

class Flipper_gradient : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Flipper_gradient(BaseModule *parent = nullptr);
    ~Flipper_gradient();

private slots:
    void on_BrowsePol_clicked();

    void on_BrowseField_clicked();

private:
    Ui::Flipper_gradient *ui;

    QMap<QString,QStringList> map = {
        {"Depth"   ,  { "-X","float","",""}},
        {"Width"   ,  { "-Y","float","",""}},
        {"Height"  ,  { "-V","float","",""}},
        {"PosX"    ,  { "-k","float","",""}},
        {"PosY"    ,  { "-l","float","",""}},
        {"PosZ"    ,  { "-m","float","",""}},
        {"OutX"    ,  { "-p","float","",""}},
        {"OutY"    ,  { "-r","float","",""}},
        {"OutZ"    ,  { "-s","float","",""}},
        {"AnglHor" ,  { "-i","float","",""}},
        {"nDomainX",  { "-C","int","",""}},
        {"nDomainY",  { "-D","int","",""}},
        {"nDomainZ",  { "-E","int","",""}},
        {"AmplFldR",  { "-d","float","",""}},
        {"ADevFldR",  { "-a","float","",""}},
        {"FreqFldR",  { "-w","float","",""}},
        {"FDevFldR",  { "-b","float","",""}},
        {"PhaseIni",  { "-z","float","",""}},
        {"eRotAxis",  { "-M","combo",}},
        {"eFctRotF",  { "-h","combo",}},
        {"eDirRotF",  { "-y","combo",}},
        {"eAmplDis",  { "-e","combo",}},
        {"eFreqDis",  { "-v","combo",}},
        {"bTofPhas",  { "-n","combo",}},
        {"eFctGdeF",  { "-u","combo",}},
        {"eDirGdeF",  { "-t","combo",}},
        {"FldIniX" ,  { "-I","float","",""}},
        {"FldIniY" ,  { "-A","float","",""}},
        {"FldIniZ" ,  { "-K","float","",""}},
        {"FldFinX" ,  { "-P","float","",""}},
        {"FldFinY" ,  { "-Q","float","",""}},
        {"FldFinZ" ,  { "-R","float","",""}},
        {"AmplFldR",  { "-q","float","",""}},
        {"bPolOut" ,  { "-S","combo",}},
        {"PolFileO",  { "-O","file","",""}},
        {"FldFileO",  { "-N","file","",""}},
    };

    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // FLIPPER_GRADIENT_H
