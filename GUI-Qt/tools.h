#ifndef TOOLS_H
#define TOOLS_H
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QFormLayout>
#include <QStyleFactory>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QMap>
#include <iostream>
#include <fstream>
#include "convert.h"
#include "string.h"
#include "yaml-cpp/yaml.h"


static QStringList strList;
static QString instrumentDir;
static QString instrumentInDir, instrumentOutDir;

//definition entries for single parameter
static QMap<QString,QString> mapParam = {
    {"type", ""},
    {"descr", ""},
    {"tooltip", ""},
    {"default", ""},
    {"index", ""},
    {"min", ""},
    {"max", ""},
    {"column", ""},
    {"prefix", ""},
};
//possible entries for parameter type definition 
static QStringList typeList = {"file","string", "float", "int", "combo","switch","window"};

typedef int (*convert_ptr)(const char *);
typedef char (*convert_char_ptr)(const char *);

//search vitess enum values matching the text in comboBoxes
//use functions in convert.c 

static QMap<QString, char (*)(const char *)> functionMapChar = {
//sample
 {"eRotAxis", reinterpret_cast<convert_char_ptr>(&RotAxis_Txt2ID)},
//moderator
 {"eModShp",   reinterpret_cast<convert_char_ptr>(&ModShape_Txt2ID)},

};

static QMap<QString, int (*)(const char *)> functionMap = {
//   general
     {"eAxis",    reinterpret_cast<convert_ptr>(&Axis_Txt2ID)},
     {"eOrient",  reinterpret_cast<convert_ptr>(&Orient_Txt2ID)},
     {"eDirIO",   reinterpret_cast<convert_ptr>(&DirInOut_Txt2ID)},
     {"eFrame",   reinterpret_cast<convert_ptr>(&FrameGen_Txt2ID)},
     {"eShape",   reinterpret_cast<convert_ptr>(&Shape_Txt2ID)},

     {"eDistr",   reinterpret_cast<convert_ptr>(&Distr_Txt2ID)},
     {"eAbsMat",  reinterpret_cast<convert_ptr>(&AbsMat_Txt2ID)},
     {"eInstGeo", reinterpret_cast<convert_ptr>(&InstGeom_Txt2ID)},
//   source + moderator
     {"SrcName",  reinterpret_cast<convert_ptr>(&SrcName_Txt2ID)},
     {"eKind",    reinterpret_cast<convert_ptr>(&SrcKind_Txt2ID)},
     {"eSrcType", reinterpret_cast<convert_ptr>(&SrcType_Txt2ID)},
     {"eDirect",  reinterpret_cast<convert_ptr>(&Direct_Txt2ID)},
     {"eTrcMode", reinterpret_cast<convert_ptr>(&Trace_Txt2ID)},
     {"eDataVsn", reinterpret_cast<convert_ptr>(&ModVsn_Txt2ID)},
     {"eModType", reinterpret_cast<convert_ptr>(&ModType_Txt2ID)},
     {"eTS",      reinterpret_cast<convert_ptr>(&TS_Txt2ID)},
//   read_in + writeout
     {"ePrgFmt",  reinterpret_cast<convert_ptr>(&PrgFormat_Txt2ID)},
     {"eDatFmt",  reinterpret_cast<convert_ptr>(&DataFormat_Txt2ID)},
     {"eSepFmt",  reinterpret_cast<convert_ptr>(&Separator_Txt2ID)},
//   frame
     {"eSequenc", reinterpret_cast<convert_ptr>(&TfmnSeq_Txt2ID)},
//   guide + mirrors
     {"eShapeY",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eShapeZ",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eLstPar",  reinterpret_cast<convert_ptr>(&ListPar_Txt2ID)},
     {"eLstGeom", reinterpret_cast<convert_ptr>(&ListVbs_Txt2ID)},
     {"bPlotPar", reinterpret_cast<convert_ptr>(&PlotFilt_Txt2ID)},
     {"ePlotX",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotY",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotPrb", reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"eMirrMat", reinterpret_cast<convert_ptr>(&MirrMat_Txt2ID)},
//   monochromator + choppers
     {"eMonoTyp", reinterpret_cast<convert_ptr>(&MonoType_Txt2ID)},
     {"eMonoArr", reinterpret_cast<convert_ptr>(&MonoArrange_Txt2ID)},
     {"eMonoFoc", reinterpret_cast<convert_ptr>(&MonoFocus_Txt2ID)},
     {"eChnShp",  reinterpret_cast<convert_ptr>(&ChnlShape_Txt2ID)},
//   windows + collimators
     {"eFrmMat",  reinterpret_cast<convert_ptr>(&WndAbs_Txt2ID)},
//   sample + sample environment
     {"eSmplGeo", reinterpret_cast<convert_ptr>(&SmplGeom_Txt2ID)},
     {"ePtclGeo", reinterpret_cast<convert_ptr>(&PtclGeom_Txt2ID)},
     {"eDataSrc", reinterpret_cast<convert_ptr>(&DataSrc_Txt2ID)},
     {"eMeasMod", reinterpret_cast<convert_ptr>(&MeasMode_Txt2ID)},
     {"eSubsMat", reinterpret_cast<convert_ptr>(&MirrMat_Txt2ID)},
//   detector
     {"eDetGeom", reinterpret_cast<convert_ptr>(&DetGeom_Txt2ID)},
     {"eDetType", reinterpret_cast<convert_ptr>(&DetType_Txt2ID)},
     {"eDetUse",  reinterpret_cast<convert_ptr>(&DetUse_Txt2ID)},
     {"eDetMat",  reinterpret_cast<convert_ptr>(&DetAbs_Txt2ID)},
     {"eDetXsec", reinterpret_cast<convert_ptr>(&TubeShape_Txt2ID)},
//   monitor
     {"eMon1Par", reinterpret_cast<convert_ptr>(&Mon1Par_Txt2ID)},
     {"eMonNorm", reinterpret_cast<convert_ptr>(&MonNorm_Txt2ID)},
     {"eFormat",  reinterpret_cast<convert_ptr>(&Format2D_Txt2ID)},
     {"eParA",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eParB",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eBrlNorm", reinterpret_cast<convert_ptr>(&BrlNorm_Txt2ID)},
     {"eBrlPar",  reinterpret_cast<convert_ptr>(&BrlPar_Txt2ID)},
//   filter + eval
     {"eFltPar1", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFltPar2", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFltPar3", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFltPar4", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFltComb", reinterpret_cast<convert_ptr>(&FiltComb_Txt2ID)},
     {"eFlt2D",   reinterpret_cast<convert_ptr>(&Mon2Par_Txt2ID)},
     {"eEvalPar", reinterpret_cast<convert_ptr>(&EvalPar_Txt2ID)},
     {"eEvalCmb", reinterpret_cast<convert_ptr>(&EvalComb_Txt2ID)},
     {"eEvalSrt", reinterpret_cast<convert_ptr>(&EvalSort_Txt2ID)},
     {"eEvalSel", reinterpret_cast<convert_ptr>(&AngleSel_Txt2ID)},
};

//map connects entries in menu help tools to files in WWW directory 
static QMap<QString,QString> helpTools = {
    { "Generate Series", "sim_series"},
    { "Convert Ascii to Binary" , "ascii2bin"},
    { "Define Direction", "define_direction"},
    { "Generate Mirror Files", "mirror_coating"},
    { "Generate Surface Files", "surface_file"},
    { "Generate Extraction System",  "gener_bispectral"},
    { "Cryst.Analyzer Spectrom.", "crysanalyzerspec"},
    { "Compute Chopper Phases", "chop_phases"}
};

//list of moduls displayed in help menu
static QStringList helpModul = {
    "beamstop",
    "chopper", "collimator",
    "detector",
    "evaluation",
    "filter", "flipper", "frame",
    "guide",
    "magnetic_field", "mirror", "monitor", "monochromator",
    "optical_elements",
    "polariser",
    "resonator_drabkin",
    "sample", "sample_environment", "sm_ensemble", "source", "spacewindow",
    "trajectories",
    "velselect",
};
void getWidgetDesign(QString parName,QMap<QString,QString> mapParameter,
                     QGridLayout *gridLayout,int &row,int &index);
void pythonScript(QString instDir,QStringList cmdList, QString logfile);
void shellScript(QString instDir,QStringList cmdList, QString logfile);

#endif // TOOLS_H
