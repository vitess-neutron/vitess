#ifndef FILTER_H
#define FILTER_H

#include "basemodule.h"

namespace Ui {
class Filter;
}

class Filter : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Filter(BaseModule *parent = nullptr);
    ~Filter();

private:
    Ui::Filter *ui;
    QMap<QString,QString> map = {
        {"test", "testval"}
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // FILTER_H
