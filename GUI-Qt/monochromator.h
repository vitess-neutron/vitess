#ifndef MONOCHROMATOR_H
#define MONOCHROMATOR_H

#include "basemodule.h"
#include "monochromator_para.h"

namespace Ui {
class Monochromator;
}

class Monochromator : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Monochromator(BaseModule *parent = nullptr);
    ~Monochromator();

private slots:
    void on_BrowsePara_clicked();
    void on_EditPara_clicked();
    void on_BrowseGeom_clicked();

private:
    Ui::Monochromator *ui;
    Monochromator_para param;
    QMap<QString,QStringList> map = {
        {"eMonGeom", {"-O","combo",}},
        {"eMonMode", {"-X","combo",}},
        {"bTransm" , {"-o","combo",}},
        {"d_sprOpt", {"-d","combo",}},
        {"MosaicH" , {"-m","float","",""}},
        {"MosaicV" , {"-M","float","",""}},
        {"d_fwhm"  , {"-D","float","",""}},
        {"Reflect" , {"-R","float","",""}},
        {"nRepete" , {"-A","int","1",""}},
        {"Freq"    , {"-f","float","",""}},
        {"Zeta0"   , {"-z","float","",""}},
        {"MuScat"  , {"-c","float","",""}},
        {"MuAbs"   , {"-C","float","",""}},
        {"GeomFile", {"-G","file","",""}},
        {"eFocGeom", {"-g","combo",}},
        {"NumCEhor", {"-H","int","1",""}},
        {"NumCEvrt", {"-V","int","1",""}},
        {"RadH"    , {"-s","float","",""}},
        {"RadV"    , {"-r","float","",""}},
        {"PsiBot"  , {"-a","float","",""}},
        {"GapH"    , {"-h","float","",""}},
        {"GapV"    , {"-v","float","",""}},
        {"DevH"    , {"-t","float","",""}},
        {"DevV"    , {"-T","float","",""}},
      // # {"ParFile" , {"-P","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // MONOCHROMATOR_H
