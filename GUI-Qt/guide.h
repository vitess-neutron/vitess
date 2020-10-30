#ifndef GUIDE_H
#define GUIDE_H

#include "basemodule.h"

namespace Ui {
class Guide;
}

class Guide : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Guide(BaseModule *parent = nullptr);
    ~Guide();
private slots:
    void openFile();
    void saveFile();

private:
    Ui::Guide *ui;

    QMap<QString,QStringList> map = {
        {"eShapeY" , {"-Y","combo",}},
        {"eShapeZ" , {"-Z","combo",}},
        {"sShapeFN", {"-S","file","",""}},
        {"EntrWdth", {"-H","float","",""}},
        {"EntrHite", {"-h","float","",""}},
        {"ExitWdth", {"-W","float","",""}},
        {"ExitHite", {"-w","float","",""}},
        {"PieceLen", {"-p","float","",""}},
        {"nPieces" , {"-N","int","1",""}},
        {"Radius"  , {"-R","float","",""}},
        {"D_Foc2Y" , {"-f","float","",""}},
        {"D_Foc2Z" , {"-F","float","",""}},
        {"m_Left"  , {"-L","float","",""}},
        {"m_Right" , {"-Q","float","",""}},
        {"m_TopBot", {"-G","float","",""}},
        {"sReflL"  , {"-i","file","",""}},
        {"sReflR"  , {"-I","file","",""}},
        {"sReflB"  , {"-J","file","",""}},
        {"nChannel", {"-b","int","1",""}},
        {"BladeThi", {"-s","float","",""}},
        {"MuTotSca", {"-M","float","",""}},
        {"MuAbs"   , {"-m","float","",""}},
        {"nColour" , {"-g","int","-1",""}},
        {"AddToCo" , {"-A","int","",""}},
        {"bAbutLos", {"-a","combo",}},
        {"AbutLen" , {"-l","float","",""}},
        {"eWaviDis", {"-q","combo",}},
        {"Waviness", {"-r","float","",""}},
        {"RotPlane", {"-n","float","",""}},
        {"sLstFN"  , {"-o","file","",""}},
        {"eLstPar" , {"-O","combo",}},
        {"bLstVbs" , {"-v","combo",}},
        {"nLstMin" , {"-e","int","",""}},
        {"nLstMax" , {"-E","int","",""}},
        {"nLstMinY", {"-c","int","",""}},
        {"nLstMaxY", {"-C","int","",""}},
        {"nLstMinZ", {"-d","int","",""}},
        {"nLstMaxZ", {"-D","int","",""}},
        {"sPlotFN" , {"-P","float","",""}},
        {"bPlotPar", {"-B","combo",}},
        {"ePlotX"  , {"-t","combo",}},
        {"ePlotY"  , {"-T","combo",}},
        {"ePlotPrb", {"-V","combo",}},
        {"nPlotX"  , {"-k","int","",""}},
        {"nPlotY"  , {"-K","int","",""}},
        {"xPlotMin", {"-x","float","",""}},
        {"xPlotMax", {"-X","float","",""}},
        {"yPlotMin", {"-u","float","",""}},
        {"yPlotMax", {"-U","float","",""}},
    };
    QMap<QString,QString> mapFilter = {
        {"NONE"        , "0"},
        {"SCATTERED"   , "1"},
        {"REF. ANGLE"  , "9"},
        {"M"           , "10"},
        {"REFLECTIVITY", "11"},
        {"DIV_Y"       , "12"},
        {"DIV_Z"       , "13"},
        {"COLOR"       , "14"},
        {"TOF"         , "15"},
        {"WAVELENGTH"  , "16"},
        {"PROBABILITY" , "17"},
        {"POS_X"       , "18"},
        {"POS_Y"       , "19"},
        {"POS_Z"       , "20"},
        {"VEC_X"       , "21"},
        {"VEC_Y"       , "22"},
        {"VEC_Z"       , "23"},
        {"SPIN_X"      , "24"},
        {"SPIN_Y"      , "25"},
        {"SPIN_Z"      , "26"},
    };
    QStringList filter = {"ePlotX","ePlotY","ePlotPrb"};
    QStringList buttonToFile =
        {"sShapeFN","sReflL","sReflR","sReflT","sReflB","sLstFN","sPlotFN"};

    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // GUIDE_H
