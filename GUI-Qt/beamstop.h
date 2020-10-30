#ifndef BEAMSTOP_H
#define BEAMSTOP_H

#include "basemodule.h"

namespace Ui {
class Beamstop;
}

class Beamstop : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Beamstop(BaseModule *parent = nullptr);
    ~Beamstop();

private:
    Ui::Beamstop *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void setValiFloat();
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // BEAMSTOP_H
