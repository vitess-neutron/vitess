#ifndef FRAME_H
#define FRAME_H

#include "basemodule.h"

namespace Ui {
class Frame;
}

class Frame : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Frame(BaseModule *parent = nullptr);
    ~Frame();

private:
    Ui::Frame *ui;
    QMap<QString,QStringList> map = {
        {"Sequence", {"-S","combo",}},
        {"MirrX"   , {"-i","combo",}},
        {"MirrY"   , {"-j","combo",}},
        {"MirrZ"   , {"-k","combo",}},
        {"RotZ"    , {"-H","float","",""}},
        {"RotY"    , {"-V","float","",""}},
        {"RotX"    , {"-A","float","",""}},
        {"TransX"  , {"-x","float","",""}},
        {"TransY"  , {"-y","float","",""}},
        {"TransZ"  , {"-z","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
}
;

#endif // FRAME_H
