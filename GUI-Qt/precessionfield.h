#ifndef PRECESSIONFIELD_H
#define PRECESSIONFIELD_H

#include "basemodule.h"

namespace Ui {
class Precessionfield;
}

class Precessionfield : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Precessionfield(BaseModule *parent = nullptr);
    ~Precessionfield();

private slots:
    void on_BrowseField_clicked();

private:
    Ui::Precessionfield *ui;
    QMap<QString,QStringList> map = {
        {"FieldMap",  {"-P","file","",""}},
        {"eOption" ,  {"-O","combo",}},
        {"PosX"    ,  {"-k","float","",""}},
        {"PosY"    ,  {"-l","float","",""}},
        {"PosZ"    ,  {"-m","float","",""}},
        {"AnglHor" ,  {"-i","float","",""}},
        {"AnglVert",  {"-j","float","",""}},
        {"Depth"   ,  {"-X","float","",""}},
        {"Width"   ,  {"-Y","float","",""}},
        {"Height"  ,  {"-V","float","",""}},
        {"FieldX"  ,  {"-T","float","",""}},
        {"FieldY"  ,  {"-G","float","",""}},
        {"FieldZ"  ,  {"-H","float","",""}},
        {"OutX"    ,  {"-p","float","",""}},
        {"OutY"    ,  {"-r","float","",""}},
        {"OutZ"    ,  {"-s","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // PRECESSIONFIELD_H
