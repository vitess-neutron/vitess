#ifndef MONITOR1D_H
#define MONITOR1D_H
#include "basemodule.h"

namespace Ui {
class Monitor1D;
}

class Monitor1D : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Monitor1D(BaseModule *parent = nullptr);
    ~Monitor1D();

private slots:
    void on_Browse_clicked();

    void on_Edit_clicked();

private:
    Ui::Monitor1D *ui;

    QMap<QString,QStringList> map = {
        {"MonFile" , {"-O","float","",""}},
        {"IdParA"  , {"-X","combo",}},
        {"nBinParA", {"-x","int","",""}},
        {"MinParA" , {"-w","float","",""}},
        {"MaxParA" , {"-W","float","",""}},
        {"IdParB"  , {"-Y","combo",}},
        {"nBinParB", {"-y","int","",""}},
        {"MinParB" , {"-f","float","",""}},
        {"MaxParB" , {"-F","float","",""}},
        {"IdParC"  , {"-Z","combo",}},
        {"nBinParC", {"-z","int","",""}},
        {"MinParC" , {"-g","float","",""}},
        {"MaxParC" , {"-G","float","",""}},
        {"bProbWt" , {"-p","combo",}},
        {"bExclude", {"-e","combo",}},
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

#endif // MONITOR1D_H
