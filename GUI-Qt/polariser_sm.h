#ifndef POLARISER_SM_H
#define POLARISER_SM_H

#include "basemodule.h"
#include "polar_sm_para.h"

namespace Ui {
class Polariser_sm;
}

class Polariser_sm : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Polariser_sm(BaseModule *parent = nullptr);
    ~Polariser_sm();

private slots:

    void on_BrowseUpRef_clicked();

    void on_BrowseDownRef_clicked();

    void on_BrowsePara_clicked();

    void on_PushEdit_clicked();

private:
    Ui::Polariser_sm *ui;
    Polar_sm_para param;
    QMap<QString,QStringList> map = {
        {"sReflUp" ,  {"-U","file","",""}},
        {"sReflDn" ,  {"-D","file","",""}},
        {"PosX"    ,  {"-a","float","",""}},
        {"PosY"    ,  {"-b","float","",""}},
        {"PosZ"    ,  {"-c","float","",""}},
        {"AnglHor" ,  {"-H","float","",""}},
        {"AnglVert",  {"-V","float","",""}},
        {"OutX"    ,  {"-R","float","",""}},
        {"OutY"    ,  {"-E","float","",""}},
        {"OutZ"    ,  {"-G","float","",""}},
        {"OutHor"  ,  {"-h","float","",""}},
        {"OutVert" ,  {"-v","float","",""}},
        //        {"Depth"   ,   "?"},
        //        {"Width"   ,   "?"},
        //        {"Height"  ,   "?"},
        //        {"FldGdeX" ,   "?"},
        //        {"FldGdeY" ,   "?"},
        //        {"FldGdeZ" ,   "?"},
        //        {"AnaDirX" ,   "?"},
        //        {"AnaDirY" ,   "?"},
        //        {"AnaDirZ" ,   "?"},
        //        {"nChan"   ,   "?"},
        //        {"Thicknes",   "?"},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // POLARISER_SM_H
