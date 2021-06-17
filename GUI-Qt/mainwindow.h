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
        void visualActive();

        void on_InDir_editingFinished();
        void on_OutDir_editingFinished();


private:
    Ui::MainWindow *ui;

    Parameter *paramWin;
    QMap <QString, Parameter *> paramWindow;

    Big *bigOutput;
    Progress *progDial;
    QString VitessDir;
    QString instrumentFile;
    QString logFname;
    QString fGeom;
    QString cModul;
    ModulTable* modultab;
    QGridLayout *gridLayout;
    QScrollArea *scrollArea;

    QMap<QString, QString> mapHeader = {
        {"RndSeed" , "--Z" },
//        {"RndNoGen", "???"},
        {"bGravity", "--G" },
        {"nBuffer" , "--B" },
        {"MinWght" , "--U" },
        {"InDir"   , "--i" },
        {"OutDir"  , "--o" },
    };

    QMap<QString, QMap<QString,QString>> ModulParam;
    QMap <QString,QMap<QString,QMap<QString,QString>>> Module;

    QMap<QString,QStringList> ModulFiles;
    QMap<QString,QScrollArea *> ModulGui;

    QMap<QString,int> modindex;

    QTimer *tVisual;
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
    int visualRepete;
    int minWidth;
    bool pipeActive = false;
    YAML::Node config, configChildren;
    YAML::Node curModul;

    void getHeader(QTextStream& out);
    void loadHeader(YAML::Node& config);
    void designModul(QString modulName);
    void getModulParameter(YAML::Node& config,QString modulName);
    void writeHeader(YAML::Node& config);
    void saveFile(QString instrumentFile);
    void readCurModul(YAML::Node& curModule,int index);
    void pasteCurModul(YAML::Node curModule,int index);
    void startPipe();
    void progress();
    void toolCommand(QString prog);
    void closeEvent(QCloseEvent *ev);
    void Visualization();
};

#endif // MAINWINDOW_H
