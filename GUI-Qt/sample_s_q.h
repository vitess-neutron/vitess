#ifndef SAMPLE_S_Q_H
#define SAMPLE_S_Q_H

#include "basemodule.h"
#include "s_q_para.h"

namespace Ui {
class Sample_s_q;
}

class Sample_s_q : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_s_q(BaseModule *parent = nullptr);
    ~Sample_s_q();

private slots:
    void on_pushParameter_clicked();

    void on_Browse_clicked();

private:
    Ui::Sample_s_q *ui;
    S_q_para param;
    QMap<QString,QStringList> map = {
        {"bIncScat", {"-I","combo",}},
        {"Repete"  , {"-A","int","",""}},
        {"Theta"   , {"-D","float","",""}},
        {"DelTheta", {"-d","float","",""}},
        {"Phi"     , {"-P","float","",""}},
        {"DelPhi"  , {"-p","float","",""}},
        {"Freq"    , {"-f","float","",""}},
        {"Offset"  , {"-o","float","",""}},
        {"SmplFile", {"-S","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_S_Q_H
