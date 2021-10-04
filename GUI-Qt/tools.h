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
 {"RotAxis", reinterpret_cast<convert_char_ptr>(&Axis_Txt2ID)},
//moderator
 {"eShape",   reinterpret_cast<convert_char_ptr>(&ModShape_Txt2ID)},

};

static QMap<QString, int (*)(const char *)> functionMap = {
//   source
//     {"SrcName",  (convert_ptr) &SrcName_Txt2ID},
     {"SrcName",  reinterpret_cast<convert_ptr>(&SrcName_Txt2ID)},
     {"eKind",    reinterpret_cast<convert_ptr>(&SrcKind_Txt2ID)},
     {"eDir",     reinterpret_cast<convert_ptr>(&Direct_Txt2ID)},
     {"eTrcMode", reinterpret_cast<convert_ptr>(&Trace_Txt2ID)},
     {"DataVsn",  reinterpret_cast<convert_ptr>(&ModVsn_Txt2ID)},
//   moderator
//     {"eShape",   reinterpret_cast<convert_ptr>(&ModShape_Txt2ID)},
     {"eType",    reinterpret_cast<convert_ptr>(&ModType_Txt2ID)},
     {"eTS",      reinterpret_cast<convert_ptr>(&TS_Txt2ID)},
//   writeout
     {"ePrgFmt",  reinterpret_cast<convert_ptr>(&PrgFormat_Txt2ID)},
     {"eDatFmt",  reinterpret_cast<convert_ptr>(&DataFormat_Txt2ID)},
     {"eSepFmt",  reinterpret_cast<convert_ptr>(&Separator_Txt2ID)},
//   monitor
     {"ePar",     reinterpret_cast<convert_ptr>(&Mon1Par_Txt2ID)},
     {"eNorm",    reinterpret_cast<convert_ptr>(&MonNorm_Txt2ID)},
     {"eFormat",  reinterpret_cast<convert_ptr>(&Format2D_Txt2ID)},
     {"ePar",     reinterpret_cast<convert_ptr>(&Mon1Par_Txt2ID)},
     {"eParA",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eParB",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eParC",    reinterpret_cast<convert_ptr>(&MonPar_Txt2ID)},
     {"eBrl",     reinterpret_cast<convert_ptr>(&BrlNorm_Txt2ID)},
     {"eParBrl",  reinterpret_cast<convert_ptr>(&BrlPar_Txt2ID)},
 //   read_in
     {"eInPrgf",  reinterpret_cast<convert_ptr>(&PrgFormat_Txt2ID)},
     {"eInForm",  reinterpret_cast<convert_ptr>(&DataFormat_Txt2ID)},
//   guide
     {"eShapeY",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eShapeZ",  reinterpret_cast<convert_ptr>(&GdeShape_Txt2ID)},
     {"eLstPar",  reinterpret_cast<convert_ptr>(&ListPar_Txt2ID)},
     {"eLstGeom", reinterpret_cast<convert_ptr>(&ListVbs_Txt2ID)},
     {"bPlotPar", reinterpret_cast<convert_ptr>(&PlotFilt_Txt2ID)},
     {"ePlotX",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotY",   reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
     {"ePlotPrb", reinterpret_cast<convert_ptr>(&PlotPar_Txt2ID)},
//   frame
     {"Sequence", reinterpret_cast<convert_ptr>(&TfmnSeq_Txt2ID)},
//   spacewindow
     {"eCircWnd", reinterpret_cast<convert_ptr>(&Shape_Txt2ID)},
     {"eMatrial", reinterpret_cast<convert_ptr>(&WndAbs_Txt2ID)},
//   sample
     {"Mode", reinterpret_cast<convert_ptr>(&MeasMode_Txt2ID)},
 //    {"RotAxis", reinterpret_cast<convert_ptr>(&Axis_Txt2ID)},
     {"SmplGeom",reinterpret_cast<convert_ptr>(&SmpleGeom_Txt2ID)},
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
