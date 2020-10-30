#ifndef CHOPPER_FERMI_CUR_H
#define CHOPPER_FERMI_CUR_H

#include "basemodule.h"

namespace Ui {
class Chopper_fermi_cur;
}

class Chopper_fermi_cur : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Chopper_fermi_cur(BaseModule *parent = nullptr);
    ~Chopper_fermi_cur();

private:
    Ui::Chopper_fermi_cur *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // CHOPPER_FERMI_CUR_H
