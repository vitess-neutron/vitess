#ifndef SPACEWINDOW_H
#define SPACEWINDOW_H

#include "basemodule.h"

namespace Ui {
class Spacewindow;
}

class Spacewindow : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Spacewindow(BaseModule *parent = nullptr);
    ~Spacewindow();

private slots:
    void on_BrowseOut_clicked();

    void on_BrowseIn_clicked();

private:
    Ui::Spacewindow *ui;
    QMap<QString,QStringList> map = {
        {"bCircWnd", {"-R","combo",}},
        {"bBeamStp", {"-S","combo",}},
        {"bRemOthr", {"-d","combo",}},
        {"TreatCol", {"-f","float","",""}},
        {"Distance", {"-l","float","0.0",""}},
        {"Hmin"    , {"-h","float","",""}},
        {"Hmax"    , {"-H","float","",""}},
        {"Wmin"    , {"-w","float","",""}},
        {"Wmax"    , {"-W","float","",""}},
        {"Radius"  , {"-r","float","",""}},
        {"CntrY"   , {"-y","float","",""}},
        {"CntrZ"   , {"-z","float","",""}},
        {"RotAng"  , {"-A","float","",""}},
        {"PhiMin"  , {"-p","float","",""}},
        {"PhiMax"  , {"-P","float","",""}},
        {"ThickOut", {"-t","float","",""}},
        {"ThickIn" , {"-T","float","",""}},
        {"FileOut" , {"-C","file","",""}},
        {"FileIn"  , {"-m","file","",""}},
        {"eMatrial", {"-c","combo",}},
    };

    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SPACEWINDOW_H
