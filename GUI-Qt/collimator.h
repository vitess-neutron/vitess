#ifndef COLLIMATOR_H
#define COLLIMATOR_H

#include "basemodule.h"

namespace Ui {
class Collimator;
}

class Collimator : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Collimator(BaseModule *parent = nullptr);
    ~Collimator();

private:
    Ui::Collimator *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // COLLIMATOR_H
