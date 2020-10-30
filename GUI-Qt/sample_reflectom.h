#ifndef SAMPLE_REFLECTOM_H
#define SAMPLE_REFLECTOM_H

#include "basemodule.h"
#include "reflectom_para.h"

namespace Ui {
class Sample_reflectom;
}

class Sample_reflectom : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_reflectom(BaseModule *parent = nullptr);
    ~Sample_reflectom();

private slots:
    void on_pushEdit_clicked();

    void on_BrowsePara_clicked();

    void on_BrowseRefl_clicked();

private:
    Ui::Sample_reflectom *ui;
    Reflectom_para param;
    QMap<QString,QStringList> map = {
        {"bIncScat", {"-B","combo",}},
        {"bOffSpec", {"-o","combo",}},
        {"Mode"    , {"-O","combo",}},
        {"SmplFile", {"-P","file","",""}},
        {"ReflFile", {"-I","file","",""}},
        {"RotAxis ", {"-R","combo",}},
        {"RotAngle", {"-a","float","",""}},
        {"MuIncSca", {"-X","float","",""}},
        {"DetWidth", {"-p","float","",""}},
        {"DetHite" , {"-t","float","",""}},
        {"DetDist" , {"-d","float","",""}},
        {"StoNArea", {"-S","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_REFLECTOM_H
