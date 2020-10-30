#ifndef CHOPPER_FERMI_STR_H
#define CHOPPER_FERMI_STR_H

#include "basemodule.h"

namespace Ui {
class Chopper_fermi_str;
}

class Chopper_fermi_str : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Chopper_fermi_str(BaseModule *parent = nullptr);
    ~Chopper_fermi_str();

private:
    Ui::Chopper_fermi_str *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // CHOPPER_FERMI_STR_H
