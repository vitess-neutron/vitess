#ifndef SOURCE_H
#define SOURCE_H

#include "basemodule.h"
#include "moderator.h"

namespace Ui {
class Source;
}

class Source : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Source(BaseModule *parent = nullptr);
    ~Source();
//    void writePipe(QTextStream& out);

private slots:

    void on_eType_currentIndexChanged(int index);

    void on_ModPara_clicked();

    void on_pushRayTracing_clicked();

    void on_pushModFile_clicked();

private:
    Ui::Source *ui;
    QMap<QString,QStringList> map = {
        { "nTraj"    , {"-n" ,"float","",""}},
        { "Lmin"     , {"-m" ,"float","",""}},
        { "Lmax"     , {"-M" ,"float","",""}},
//        { "Tmin"     , {"-t" ,"float","",""}},
//        { "Tmax"     , {"-T" ,"float","",""}},
        { "Divmin"   , {"-y" ,"float","",""}},
        { "Divmax"   , {"-z" ,"float","",""}},
        { "eDir"     , {"-d" ,"combo",}},
        { "TDist"    , {"-D" ,"float","",""}},
        { "TWidth"   , {"-w" ,"float","",""}},
        { "THeight"  , {"-h" ,"float","",""}},
        { "TofDist"  , {"-s" ,"float","",""}},
//        { "TofMin"   , {"-f" ,"float","",""}},
//        { "TofMax"   , {"-F" ,"float","",""}},
        { "TimeMeas" , {"-A" ,"float","",""}},
//        { "LmbdWant" , {"-W" ,"float","",""}},
        { "PolVecX"  , {"-X" ,"float","",""}},
        { "PolVecY"  , {"-Y" ,"float","",""}},
        { "PolVecZ"  , {"-V" ,"float","",""}},
        { "PolDeg"   , {"-P" ,"float","",""}},
//        { "TrcFile"  , {"-r" ,"file","",""}},
        { "eTrcMode" , {"-k" ,"combo",}},
//        { "SrcName"  , {"-N" ,"combo",}},
        { "eType"    , {"-S" ,"combo",}},
//        { "Freq"    , {"-R","float","",""}},
//        { "Power"   , {"-L","float","",""}},
//        { "Length"  , {"-p","float","",""}},
        { "Decl"     , {"-i" ,"float","",""}},
        { "ModFile"  , {"-a" ,"float","",""}},
   };

    // entries for source yaml part
    QStringList sourceEntry = {"Beamline","Decl"};
    QStringList sourceCombo = {"SrcName","eType","DataVsn"};
    QList<QLineEdit *> sourceList;
    QList<QComboBox *> sourceBox;

    Moderator moderator;
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SOURCE_H
