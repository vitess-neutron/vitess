#ifndef MONITOR2D_H
#define MONITOR2D_H
#include "basemodule.h"

namespace Ui {
class Monitor2D;
}

class Monitor2D : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Monitor2D(BaseModule *parent = nullptr);
    ~Monitor2D();

private slots:
    void on_Browse_clicked();

private:
    Ui::Monitor2D *ui;

    QMap<QString,QStringList> map = {
        {"MonFile" , {"-O","file","",""}},
        {"IdParA"  , {"-X","combo",}},
        {"nBinParA", {"-x","int","",""}},
        {"MinParA" , {"-w","float","",""}},
        {"MaxParA" , {"-W","float","",""}},
        {"IdParB"  , {"-Y","combo",}},
        {"nBinParB", {"-y","int","",""}},
        {"MinParB" , {"-f","float","",""}},
        {"MaxParB" , {"-F","float","",""}},
        {"bProbWt" , {"-p","combo",}},
        {"bExclude", {"-e","combo",}},
        {"eFormat" , {"-F","combo",}},
        {"LmbdMin" , {"-l","float","",""}},
        {"LmbdMax" , {"-L","float","",""}},
        {"IdFilt1" , {"-I","combo",}},
        {"IdFilt2" , {"-J","combo",}},
        {"FiltComb", {"-C","combo",}},
        {"MinFilt1", {"-u","float","",""}},
        {"MaxFilt1", {"-U","float","",""}},
        {"MinFilt2", {"-v","float","",""}},
        {"Maxfilt2", {"-V","float","",""}},
        {"bPolAna" , {"-P","combo",}},
        {"PolDirX" , {"-r","float","",""}},
        {"PolDirY" , {"-s","float","",""}},
        {"PolDirZ" , {"-t","float","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // MONITOR2D_H
