#ifndef FLIPPER_COIL_H
#define FLIPPER_COIL_H

#include "basemodule.h"

namespace Ui {
class Flipper_coil;
}

class Flipper_coil : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Flipper_coil(BaseModule *parent = nullptr);
    ~Flipper_coil();

private:
    Ui::Flipper_coil *ui;
    QMap<QString,QStringList> map = {
        {"PosX"    ,  {"-k","float","",""}},
        {"PosY"    ,  {"-l","float","",""}},
        {"PosZ"    ,  {"-m","float","",""}},
        {"CoilDir" ,  {"-y","combo",}},
        {"AnglHor" ,  {"-i","float","",""}},
        {"AnglVert",  {"-j","float","",""}},
        {"Depth"   ,  {"-X","float","",""}},
        {"Width"   ,  {"-Y","float","",""}},
        {"Height"  ,  {"-V","float","",""}},
        {"FldGuide",  {"-G","float","",""}},
        {"FldCoil" ,  {"-H","float","",""}},
        {"Thicknes",  {"-t","float","0.0",""}},
        {"nDomainX",  {"-N","int","0","99"}},
        {"OutX"    ,  {"-p","float","",""}},
        {"OutY"    ,  {"-r","float","",""}},
        {"OutZ"    ,  {"-s","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // FLIPPER_COIL_H
