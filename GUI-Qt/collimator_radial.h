#ifndef COLLIMATOR_RADIAL_H
#define COLLIMATOR_RADIAL_H

#include "basemodule.h"

namespace Ui {
class Collimator_radial;
}

class Collimator_radial : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Collimator_radial(BaseModule *parent = nullptr);
    ~Collimator_radial();

private:
    Ui::Collimator_radial *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // COLLIMATOR_RADIAL_H
