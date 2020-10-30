#ifndef SAMPLE_ELASTICISOTR_H
#define SAMPLE_ELASTICISOTR_H

#include "basemodule.h"
#include "elasticisotr_para.h"

namespace Ui {
class Sample_elasticisotr;
}

class Sample_elasticisotr : public BaseModule
{
    Q_OBJECT

public:
     Q_INVOKABLE explicit Sample_elasticisotr(BaseModule *parent = nullptr);
    ~Sample_elasticisotr();

private slots:
    void on_pushEdit_clicked();

    void on_BrowsePara_clicked();

private:
    Ui::Sample_elasticisotr *ui;
    Elasticisotr_para param;
    QMap<QString,QStringList> map = {
        {"bColor"  , {"-c","int","-1","",}},
        {"Repete"  , {"-A","int","1","",}},
        {"SmplFile", {"-P","file","","",}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);

};

#endif // SAMPLE_ELASTICISOTR_H
