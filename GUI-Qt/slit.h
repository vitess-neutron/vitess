#ifndef SLIT_H
#define SLIT_H

#include "basemodule.h"

namespace Ui {
class Slit;
}

class Slit : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Slit(BaseModule *parent = nullptr);
    ~Slit();

private:
    Ui::Slit *ui;
    QMap<QString,QStringList> map = {
        {"Distance", {"-d","float","0.0",""}},
        {"Width"   , {"-W","float","0.0",""}},
        {"Height"  , {"-H","float","0.0",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SLIT_H
