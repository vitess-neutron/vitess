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
    {"options", ""},
    {"enum", ""},
    {"hidden", ""},
    {"plottable", ""},
    {"filename", ""},
};
//possible entries for parameter type definition
static QStringList typeList = {"file","string", "float", "int", "combo","switch","window", "title"};

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
     {"eDirInOut",   reinterpret_cast<convert_ptr>(&DirInOut_Txt2ID)},
     {"eFrame",   reinterpret_cast<convert_ptr>(&FrameGen_Txt2ID)},
     {"eShape",   reinterpret_cast<convert_ptr>(&Shape_Txt2ID)},

     {"eDistr",   reinterpret_cast<convert_ptr>(&Distr_Txt2ID)},
     {"eAbsorbMat",  reinterpret_cast<convert_ptr>(&AbsMat_Txt2ID)},
     {"eInstrGeom", reinterpret_cast<convert_ptr>(&InstGeom_Txt2ID)},
//   source + moderator
     {"eSrcName",  reinterpret_cast<convert_ptr>(&SrcName_Txt2ID)},
     {"sSrcName",  reinterpret_cast<convert_ptr>(&SrcName_Txt2ID)},
     {"eSrcKind",    reinterpret_cast<convert_ptr>(&SrcKind_Txt2ID)},
     {"eSrcType", reinterpret_cast<convert_ptr>(&SrcType_Txt2ID)},
     {"eDirMode",  reinterpret_cast<convert_ptr>(&Direct_Txt2ID)},
     {"eTraceMode", reinterpret_cast<convert_ptr>(&Trace_Txt2ID)},
     {"eDataVsn", reinterpret_cast<convert_ptr>(&ModVsn_Txt2ID)},
     {"eModType", reinterpret_cast<convert_ptr>(&ModType_Txt2ID)},
     {"eTS",      reinterpret_cast<convert_ptr>(&TS_Txt2ID)},
//   read_in + writeout
     {"ePrgFormat",  reinterpret_cast<convert_ptr>(&PrgFormat_Txt2ID)},
     {"eDatFormat",  reinterpret_cast<convert_ptr>(&DataFormat_Txt2ID)},
     {"eSepFormat",  reinterpret_cast<convert_ptr>(&Separator_Txt2ID)},
     {"bRandSamp",   reinterpret_cast<convert_ptr>(&Sampling_Txt2ID)},
//   frame
     {"eSequence", reinterpret_cast<convert_ptr>(&TfmnSeq_Txt2ID)},
//   guide + mirrors
     {"eShapeY",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eShapeZ",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eListPar",  reinterpret_cast<convert_ptr>(&ListPar_Txt2ID)},
     {"eListGeom", reinterpret_cast<convert_ptr>(&ListVbs_Txt2ID)},
     {"ePlotPar", reinterpret_cast<convert_ptr>(&PlotFilt_Txt2ID)},
     {"ePlotX",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotY",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotProb", reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"eMirrMat", reinterpret_cast<convert_ptr>(&MirrMat_Txt2ID)},
//   monochromator + choppers
     {"eMonoTyp", reinterpret_cast<convert_ptr>(&MonoType_Txt2ID)},
     {"eMonoArrange", reinterpret_cast<convert_ptr>(&MonoArrange_Txt2ID)},
     {"eMonoFoc", reinterpret_cast<convert_ptr>(&MonoFocus_Txt2ID)},
     {"eChanShape",  reinterpret_cast<convert_ptr>(&ChnlShape_Txt2ID)},
//   windows + collimators
     {"eMatOut",  reinterpret_cast<convert_ptr>(&WndAbs_Txt2ID)},
//   sample + sample environment
     {"eSmplGeom", reinterpret_cast<convert_ptr>(&SmplGeom_Txt2ID)},
     {"ePtclGeom", reinterpret_cast<convert_ptr>(&PtclGeom_Txt2ID)},
     {"eDataSrc", reinterpret_cast<convert_ptr>(&DataSrc_Txt2ID)},
     {"eMeasMod", reinterpret_cast<convert_ptr>(&MeasMode_Txt2ID)},
     {"eSubstrMat", reinterpret_cast<convert_ptr>(&MirrMat_Txt2ID)},
//   detector
     {"eDetGeom", reinterpret_cast<convert_ptr>(&DetGeom_Txt2ID)},
     {"eDetType", reinterpret_cast<convert_ptr>(&DetType_Txt2ID)},
     {"eDetUse",  reinterpret_cast<convert_ptr>(&DetUse_Txt2ID)},
     {"eDetMat",  reinterpret_cast<convert_ptr>(&DetAbs_Txt2ID)},
     {"eDetXsec", reinterpret_cast<convert_ptr>(&TubeShape_Txt2ID)},
//   monitor
     {"eMonPar", reinterpret_cast<convert_ptr>(&Mon1Par_Txt2ID)},
     {"eMonNorm", reinterpret_cast<convert_ptr>(&MonNorm_Txt2ID)},
     {"eFormat",  reinterpret_cast<convert_ptr>(&Format2D_Txt2ID)},
     {"ePar",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"ePar1",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"ePar2",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eBrlNorm", reinterpret_cast<convert_ptr>(&BrlNorm_Txt2ID)},
     {"eBrlPar",  reinterpret_cast<convert_ptr>(&BrlPar_Txt2ID)},
//   filter + eval
     {"eFilterPar1", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFilterPar2", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFilterPar3", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFilterPar4", reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eFilterCombi", reinterpret_cast<convert_ptr>(&FiltComb_Txt2ID)},
     {"eFilt2D",   reinterpret_cast<convert_ptr>(&Mon2Par_Txt2ID)},
     {"eEvalPar", reinterpret_cast<convert_ptr>(&EvalPar_Txt2ID)},
     {"eEvalCombi", reinterpret_cast<convert_ptr>(&EvalComb_Txt2ID)},
     {"eSort", reinterpret_cast<convert_ptr>(&EvalSort_Txt2ID)},
     {"eScatAngle", reinterpret_cast<convert_ptr>(&AngleSel_Txt2ID)},
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
