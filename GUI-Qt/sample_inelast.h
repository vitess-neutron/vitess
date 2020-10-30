#ifndef SAMPLE_INELAST_H
#define SAMPLE_INELAST_H

#include "basemodule.h"
#include "inelast_para.h"

namespace Ui {
class Sample_inelast;
}

class Sample_inelast : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_inelast(BaseModule *parent = nullptr);
    ~Sample_inelast();

private slots:
    void on_pushEdit_clicked();

    void on_BrowsePara_clicked();

private:
    Ui::Sample_inelast *ui;
    Inelast_para param;
   QMap<QString,QStringList> map = {
        {"Repete"  , {"-A","int","1",""}},
        {"SmplFile", {"-P","file","",""}},
        {"P1"      , {"-a","float","",""}},
        {"P2"      , {"-b","float","0.0",""}},
        {"P3"      , {"-c","float","0.0001",""}},
        {"P4"      , {"-d","float","",""}},
        {"D1"      , {"-x","float","",""}},
        {"D2"      , {"-y","float","",""}},
        {"D3"      , {"-z","float","",""}},
        {"Temp"    , {"-z","float","0.0001",""}},
        {"bBoseFac", {"-D","combo",}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_INELAST_H
