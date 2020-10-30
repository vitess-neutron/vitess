#ifndef SAMPLE_ENVIRONMENT_H
#define SAMPLE_ENVIRONMENT_H

#include "basemodule.h"
#include "environment_para.h"

namespace Ui {
class Sample_environment;
}

class Sample_environment : public BaseModule
{
        Q_OBJECT

public:
     Q_INVOKABLE explicit Sample_environment(BaseModule *parent = nullptr);
    ~Sample_environment();

private slots:
    void on_Edit_clicked();
    void on_Browse_clicked();

private:
    Ui::Sample_environment *ui;
    Environment_para param;
    QMap<QString,QStringList> map = {
        {"ParFile" , {"-F","file","","",}},
        {"nColor"  , {"-c","int","0","",}},
        {"PosMainX", {"-x","float","","",}},
        {"PosMainY", {"-y","float","","",}},
        {"PosMainZ", {"-z","float","","",}},
        {"eDirEnv" , {"-r","combo",}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_ENVIRONMENT_H
