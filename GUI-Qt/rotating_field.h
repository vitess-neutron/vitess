#ifndef ROTATING_FIELD_H
#define ROTATING_FIELD_H

#include "basemodule.h"

namespace Ui {
class Rotating_field;
}

class Rotating_field : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Rotating_field(BaseModule *parent = nullptr);
    ~Rotating_field();

private slots:
    void on_BrowseAmpl_clicked();

    void on_BrowsePol_clicked();

    void on_BrowseField_clicked();

private:
    Ui::Rotating_field *ui;
    QStringList FileEntry =
        {"AmplFile","PolFileO","FldFileO"};

    QMap<QString,QStringList> map = {
        {"Depth"   ,  {"-X","float","",""}},
        {"Width"   ,  {"-Y","float","",""}},
        {"Height"  ,  {"-V","float","",""}},
        {"PosX"    ,  {"-k","float","",""}},
        {"PosY"    ,  {"-l","float","",""}},
        {"PosZ"    ,  {"-m","float","",""}},
        {"OutX"    ,  {"-p","float","",""}},
        {"OutY"    ,  {"-r","float","",""}},
        {"OutZ"    ,  {"-s","float","",""}},
        {"AnglHor" ,  {"-i","float","",""}},
        {"nDomainX",  {"-C","int","",""}},
        {"nDomainY",  {"-D","int","",""}},
        {"nDomainZ",  {"-E","int","",""}},
        {"AmplFile",  {"-t","file","",""}},
        {"AmplFldR",  {"-d","float","",""}},
        {"ADevFldR",  {"-a","float","",""}},
        {"FreqFldR",  {"-w","float","",""}},
        {"FDevFldR",  {"-b","float","",""}},
        {"PhaseIni",  {"-z","float","",""}},
        {"eRotAxis",  {"-M","combo",}},
        {"eAmplDis",  {"-e","combo",}},
        {"eFreqDis",  {"-v","combo",}},
        {"bTofPhas",  {"-n","combo",}},
        {"FieldX"  ,  {"-I","float","",""}},
        {"FieldY"  ,  {"-A","float","",""}},
        {"FieldZ"  ,  {"-K","float","",""}},
        {"AmplFldR",  {"-q","float","",""}},
        {"Wavelen" ,  {"-W","float","",""}},
        {"bPolOut" ,  {"-S","combo",}},
        {"bAutoClc",  {"-x","combo",}},
        {"bBootStr",  {"-T","combo",}},
        {"PolFileO",  {"-O","file","",""}},
        {"FldFileO",  {"-N","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // ROTATING_FIELD_H
