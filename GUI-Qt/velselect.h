#ifndef VELSELECT_H
#define VELSELECT_H

#include "basemodule.h"

namespace Ui {
class Velselect;
}

class Velselect : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Velselect(BaseModule *parent = nullptr);
    ~Velselect();

private:
    Ui::Velselect *ui;

    QMap<QString,QStringList> map = {
        {"Radius"  , {"-r","float","0.001",""}},
        {"Length"  , {"-l","float","0.001",""}},
        {"Spacer"  , {"-d","float","0.0","1000.0"}},
        {"Freq"    , {"-s","float","",""}},
        {"Twist"   , {"-c","float","",""}},
        {"DistAxle", {"-o","float","",""}},
        {"winnum"  , {"-w","int","1",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // VELSELECT_H
