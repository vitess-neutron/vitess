#ifndef DETECTOR_H
#define DETECTOR_H

#include "basemodule.h"

namespace Ui {
class Detector;
}
class Detector : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Detector(BaseModule *parent = nullptr);
    ~Detector();

private:
    Ui::Detector *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // DETECTOR_H
