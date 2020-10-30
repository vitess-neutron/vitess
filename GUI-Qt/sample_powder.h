#ifndef SAMPLE_POWDER_H
#define SAMPLE_POWDER_H

#include "basemodule.h"
#include "powder_para.h"

namespace Ui {
class Sample_powder;
}

class Sample_powder : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_powder(BaseModule *parent = nullptr);
    ~Sample_powder();

private slots:
    void on_pushEdit_clicked();

    void on_Browse_clicked();

private:
    Ui::Sample_powder *ui;
    Powder_para param;
    QMap<QString,QStringList> map = {
        {"bIncScat", {"-I","combo",}},
        {"bTrtAll" , {"-a","combo",}},
        {"nColor"  , {"-c","int","",""}},
        {"Repete"  , {"-A","int","1",""}},
        {"Theta"   , {"-d","float","",""}},
        {"DelTheta", {"-D","float","",""}},
        {"Phi"     , {"-p","float","",""}},
        {"DelPhi"  , {"-P","float","",""}},
        {"SmplFile", {"-S","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_POWDER_H
