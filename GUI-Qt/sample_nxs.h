#ifndef SAMPLE_NXS_H
#define SAMPLE_NXS_H

#include "basemodule.h"

namespace Ui {
class Sample_nxs;
}

class Sample_nxs : public BaseModule
{
    Q_OBJECT

public:
     Q_INVOKABLE explicit Sample_nxs(BaseModule *parent = nullptr);
    ~Sample_nxs();

private slots:
    void on_Browse_clicked();

private:
    Ui::Sample_nxs *ui;
    QMap<QString,QStringList> map = {
        {"bIncScat", {"-I","combo",}},
        {"bTrtAll" , {"-a","combo",}},
        {"bOnlyT"  , {"-Z","combo",}},
        {"nColor"  , {"-c","int","-1",""}},
        {"Repete"  , {"-A","int","1",""}},
        {"Theta"   , {"-d","float","",""}},
        {"DelTheta", {"-D","float","",""}},
        {"Phi"     , {"-p","float","",""}},
        {"DelPhi"  , {"-P","float","",""}},
    // #  {"SmplFile", {"-S","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_NXS_H
