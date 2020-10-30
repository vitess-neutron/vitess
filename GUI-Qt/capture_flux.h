#ifndef CAPTURE_FLUX_H
#define CAPTURE_FLUX_H

#include "basemodule.h"

namespace Ui {
class Capture_flux;
}

class Capture_flux : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Capture_flux(BaseModule *parent = nullptr);
    ~Capture_flux();

private:
    Ui::Capture_flux *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
}
;

#endif // CAPTURE_FLUX_H
