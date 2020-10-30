#ifndef SPACE_H
#define SPACE_H

#include "basemodule.h"

namespace Ui {
class Space;
}

class Space : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Space(BaseModule *parent = nullptr);
    ~Space();

private:
    Ui::Space *ui;
    QMap<QString,QStringList> map = {
        {"Distance", {"-d","float","",""}},
        {"MuScat"  , {"-M","float","",""}},
        {"MuAbs"   , {"-m","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SPACE_H
