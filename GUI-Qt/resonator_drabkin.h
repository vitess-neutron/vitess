#ifndef RESONATOR_DRABKIN_H
#define RESONATOR_DRABKIN_H

#include "basemodule.h"

namespace Ui {
class Resonator_drabkin;
}

class Resonator_drabkin : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Resonator_drabkin(BaseModule *parent = nullptr);
    ~Resonator_drabkin();

private slots:

    void on_BrowsePol_clicked();

    void on_BrowseField_clicked();

private:
    Ui::Resonator_drabkin *ui;

    QMap<QString,QStringList> map = {
        {"Depth"    ,  {"-X","float","",""}},
        {"Width"    ,  {"-Y","float","",""}},
        {"Height"   ,  {"-V","float","",""}},
        {"PosX"     ,  {"-k","float","",""}},
        {"PosY"     ,  {"-l","float","",""}},
        {"PosZ"     ,  {"-m","float","",""}},
        {"OutX"     ,  {"-p","float","",""}},
        {"OutY"     ,  {"-r","float","",""}},
        {"OutZ"     ,  {"-s","float","",""}},
        {"nDomainX" ,  {"-C","int","",""}},
        {"nDomainY" ,  {"-D","int","",""}},
        {"nDomainZ" ,  {"-E","int","",""}},
        {"AmplFldP" ,  {"-d","float","",""}},
        {"eAxis"    ,  {"-M","combo",}},
        {"eDisFldP" ,  {"-v","combo",}},
        {"eAmplFldP",  {"-e","combo",}},
        {"ADevFldP" ,  {"-a","float","",""}},
        {"ASigFldP" ,  {"-x","float","",""}},
        {"FieldX"   ,  {"-I","float","",""}},
        {"FieldY"   ,  {"-A","float","",""}},
        {"FieldZ"   ,  {"-K","float","",""}},
        {"AmplFldR" ,  {"-q","float","",""}},
        {"bPolOut"  ,  {"-S","combo",}},
        {"PolFileO" ,  {"-O","file","",""}},
        {"FldFileO" ,  {"-N","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // RESONATOR_DRABKIN_H
