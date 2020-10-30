#ifndef SM_ENSEMBLE_H
#define SM_ENSEMBLE_H

#include "basemodule.h"
#include "mirror.h"

namespace Ui {
class Sm_ensemble;
}

class Sm_ensemble : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sm_ensemble(BaseModule *parent = nullptr);
    ~Sm_ensemble();

private slots:

    void on_Mirror_clicked();

private:
    Ui::Sm_ensemble *ui;
    Mirror param;
    QMap<QString,QStringList> map = {
        {"eMatMirr", {"-S","combo",}},
        {"bColor"  , {"-R","combo",}},
        {"eSpinDir", {"-Q","combo",}},
        {"OutX"    , {"-r","float","",""}},
        {"OutY"    , {"-s","float","",""}},
        {"OutZ"    , {"-t","float","",""}},
        {"OutHor"  , {"-h","float","",""}},
        {"OutVert" , {"-v","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SM_ENSEMBLE_H
