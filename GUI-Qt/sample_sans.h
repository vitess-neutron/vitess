#ifndef SAMPLE_SANS_H
#define SAMPLE_SANS_H

#include "basemodule.h"
#include "sans_para.h"

namespace Ui {
class Sample_sans;
}

class Sample_sans : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_sans(BaseModule *parent = nullptr);
    ~Sample_sans();

private slots:
    void on_Edit_clicked();
    void on_Browse_clicked();

private:
    Ui::Sample_sans *ui;
    Sans_para param;
    QMap<QString,QStringList> map = {
        {"bIncScat", {"-I","combo",}},
        {"Repete"  , {"-A","int","1",""}},
        {"MaxTheta", {"-M","float","",""}},
        {"SmplFile", {"-S","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_SANS_H
