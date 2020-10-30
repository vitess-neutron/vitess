#ifndef MONOCHR_ANALYSER_H
#define MONOCHR_ANALYSER_H

#include "basemodule.h"
#include "monochromator_para.h"

namespace Ui {
class Monochr_analyser;
}

class Monochr_analyser : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Monochr_analyser(BaseModule *parent = nullptr);
    ~Monochr_analyser();

private slots:
    void on_BrowsePara_clicked();
    void on_EditPara_clicked();
    void on_BrowseGeom_clicked();

private:
    Ui::Monochr_analyser *ui;
    Monochromator_para param;
    QMap<QString,QStringList> map = {
        {"eMonGeom", {"-O","combo",}},
        {"nRepete" , {"-A","int","1",""}},
        {"MosaicH" , {"-m","float","",""}},
        {"MosaicV" , {"-M","float","",""}},
        {"d_fwhm"  , {"-D","float","",""}},
        {"Reflect" , {"-R","float","",""}},
        {"d_sprOpt", {"-d","combo",}},
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
    // #{"ParFile" , {"-P","file","",""}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // MONOCHR_ANALYSER_H
