#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modultable.h"
#include "help.h"
#include "parameter.h"
#include "chrystanalyzer.h"
#include "chopperphases.h"
#include "progress.h"
#include "big.h"
#include "convert.h"

#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QProcess>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QProgressDialog>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadInstrument(QString fName);
protected:
    bool eventFilter(QObject *obj, QEvent *ev);

signals:
    void big(QString test);

private slots:

//        void showTextBrowser(QString test);
        void showTextBrowser();
        void showSelectedModul(int);
        void changeModulWidget(QString modul,int row);
        void changeParamWidget(QString filename,QString initName);

        void removeModule(int);
        void insertModule(int);

        void finishedLast();
        void finishedSort();

        void checkIsValide();

        void on_actionLoad_triggered();
        void on_actionSave_as_triggered();
        void on_actionSave_triggered();
        void on_actionNewInst_triggered();
        void on_actionExit_triggered();
        void on_actionGeneral_Information_triggered();
        void on_actionTutorial_triggered();
        void on_actionUser_Interface_triggered();
        void on_actionOptimization_triggered();
        void on_actionPlot_File_triggered();
        void on_action2D_Plot_File_triggered();
        void on_actionPy_Python_script_triggered();
        void on_actionBat_shell_triggered();
        void on_actionCopy_Module_Parameters_triggered();
        void on_actionPaste_Module_Parameters_triggered();
        void on_actionShow_inf_File_triggered();
        void on_actionSet_Instrument_Name_triggered();

        void on_actionConvert_Ascii_to_Binary_triggered();
        void on_actionDefine_Direction_triggered();
        void on_actionGenerate_Mirror_Files_triggered();
        void on_actionGenerate_Surface_Files_triggered();
        void on_actionGenerate_Extraction_System_triggered();
        void on_actionGuide_Shape_triggered();
        void on_actionCryst_Analayzer_Spectrom_triggered();
        void on_actionCompute_Chopper_Phases_triggered();


        void on_pushFresh_clicked();
        void on_pushClear_clicked();
        void on_pushSave_clicked();
        void on_pushDryrun_clicked();
        void on_pushCheck_clicked();
        void on_pushOutdir_clicked();
        void on_pushIndir_clicked();
        void on_pushStart_clicked();
        void on_pushKill_clicked();
        void on_pushStop_clicked();

        void browseBut_clicked();
        void editBut_clicked();
        void paramBut_clicked();
        void BufferSize_triggered();
        void minNeutWeight_triggered();
        void helpTools_triggered();
        void helpModules_triggered();


        void on_pushBig_clicked();

        void on_pushVisual_clicked();
        void testActive();

        void on_InDir_editingFinished();
        void on_OutDir_editingFinished();

private:
    Ui::MainWindow *ui;

    Parameter *paramWin;
    QMap <QString, Parameter *> paramWindow;

    Big *bigOutput;

    QString VitessDir;
    QString instrumentName;
    QString logFname;
    QString fGeom;
    ModulTable* modultab;
    QGridLayout *gridLayout;
    QScrollArea *scrollArea;

    typedef int (*convert_ptr)(const char *);
    QMap<QString, int (*)(const char *)> functionMap = {
    //   source
    //   {"SrcName",  reinterpret_cast<convert_ptr>(&SrcName_Txt2ID)},
         {"SrcName",  (convert_ptr) &SrcName_Txt2ID},
         {"eKind",    (convert_ptr) &SrcKind_Txt2ID},
         {"eDir",     (convert_ptr) &Direct_Txt2ID},
         {"eTrcMode", (convert_ptr) &Trace_Txt2ID},
         {"DataVsn",  (convert_ptr) &ModVsn_Txt2ID},
    //   moderator
         {"eShape",   (convert_ptr) &ModShape_Txt2ID},
         {"eType",    (convert_ptr) &ModType_Txt2ID},
         {"eTS",      (convert_ptr) &TS_Txt2ID},
    //   writeout
         {"ePrgFmt",  (convert_ptr) &PrgFormat_Txt2ID},
         {"eDatFmt",  (convert_ptr) &DataFormat_Txt2ID},
         {"eSepFmt",  (convert_ptr) &Separator_Txt2ID},
    //   monitor
         {"ePar",     (convert_ptr) &Mon1Par_Txt2ID},
         {"eNorm",    (convert_ptr) &MonNorm_Txt2ID},
         {"eFormat",  (convert_ptr) &Format2D_Txt2ID},
         {"ePar",     (convert_ptr) &Mon1Par_Txt2ID},
         {"eParA",    (convert_ptr) &MonPar_Txt2ID},
         {"eParB",    (convert_ptr) &MonPar_Txt2ID},
         {"eParC",    (convert_ptr) &MonPar_Txt2ID},
         {"eBrl",     (convert_ptr) &BrlNorm_Txt2ID},
         {"eParBrl",  (convert_ptr) &BrlPar_Txt2ID},
     //   read_in
         {"eInPrgf",  (convert_ptr) &PrgFormat_Txt2ID},
         {"eInForm",  (convert_ptr) &DataFormat_Txt2ID},
    //   guide
         {"eShapeY",  (convert_ptr) &GdeShape_Txt2ID},
         {"eShapeZ",  (convert_ptr) &GdeShape_Txt2ID},
         {"eLstPar",  (convert_ptr) &ListPar_Txt2ID},
         {"eLstGeom", (convert_ptr) &ListVbs_Txt2ID},
         {"bPlotPar", (convert_ptr) &PlotFilt_Txt2ID},
         {"ePlotX",   (convert_ptr) &PlotPar_Txt2ID},
         {"ePlotY",   (convert_ptr) &PlotPar_Txt2ID},
         {"ePlotPrb", (convert_ptr) &PlotPar_Txt2ID},
    //   frame
         {"Sequence", (convert_ptr) &TfmnSeq_Txt2ID},
    //   spacewindow
         {"eCircWnd", (convert_ptr) &Shape_Txt2ID},
         {"eMatrial", (convert_ptr) &WndAbs_Txt2ID},
    //   sample
         {"Mode", (convert_ptr) &MeasMode_Txt2ID},
         {"RotAxis", (convert_ptr) &Axis_Txt2ID},
};


    QMap<QString, QStringList> mapHeader = {
        {"RndSeed" , {"--Z"}, },
//        {"RndNoGen", {"???"}},
        {"bGravity", {"--G"},},
        {"nBuffer" , {"--B"},},
        {"MinWght" , {"--U"},},
        {"InDir"   , {"--i"}},
//        {"InDir"   , {"--P",}},
        {"OutDir"  , {"--o",}},
//        {"LogFile" , {"--L",}},
//        {"Modnum"  , {"--N",}},
    };
    QMap<QString,QString> helpTools = {
        { "Generate Series", "sim_series"},
        { "Convert Ascii to Binary" , "ascii2bin"},
        { "Define Direction", "define_direction"},
        { "Generate Mirror Files", "mirror_coating"},
        { "Generate Surface Files", "surface_file"},
        { "Generate Extraction System",  "gener_bispectral"},
        { "Cryst.Analyzer Spectrom.", "crysanalyzerspec"},
        { "Compute Chopper Phases", "chop_phases"}
    };
    QMap<QString, QMap<QString,QString>> mapModule;
    QMap <QString,QMap<QString,QMap<QString,QString>>> mapVitess;

    QMap<QString,QStringList> Module;
    QMap<QString,QScrollArea *> modulGui;

    QMap<QString,int> modindex;

    QTimer *t;
    QElapsedTimer timer;
    QProgressDialog *dialog;

    QList<QLineEdit  *> allLineEdits;
    QList<QComboBox *>  allComboBoxes;
    QList<QCheckBox *>  allCheckBoxes;
    QList<QPushButton *>  allPushButtons;
    QList < QProcess *> procList;
    QStringList fList,cmdList;
    
    QString syspar;
    QString str;
    QString nBuffer;
    QString MinWght;
    QPalette palette;
    int sim;
    int minWidth;
    bool pipeActive = false;
    YAML::Node config, configChildren;
    YAML::Node curModul;

    void getHeader(QTextStream& out);
    void loadHeader(YAML::Node& config);
    void designModul(QString modulName);
    void getModulParameter(YAML::Node& config,QString modulName);
    void writeHeader(YAML::Node& config);
    void saveFile(QString instrumentName);
    void readCurModul(YAML::Node& curModule,int index);
    void pasteCurModul(YAML::Node curModule,int index);
    void progress();
    void toolCommand(QString prog);
    void closeEvent(QCloseEvent *ev);
    void Visualization(int i);
};

#endif // MAINWINDOW_H
