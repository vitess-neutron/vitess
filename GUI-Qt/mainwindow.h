#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modultable.h"
#include "help.h"
#include "parameter.h"

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

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:

        void showSelectedModul(int);
        void changeModulWidget(QString modul,int row);
        void changeParamWidget(QString filename,QString initName);

        void removeModule(int);
        void insertModule(int);

        void finishedLast();

        void checkIsValide();

        void on_actionLoad_triggered();
        void on_actionSave_as_triggered();
        void on_actionSave_triggered();
        void on_actionNewInst_triggered();
        void on_actionExit_triggered();
        void on_actionGeneral_Information_triggered();
        void on_actionTutorial_triggered();
        void on_actionPlot_File_triggered();
        void on_action2D_Plot_File_triggered();
        void on_actionPy_Python_script_triggered();
        void on_actionBat_shell_triggered();
        void on_actionCopy_Module_Parameters_triggered();
        void on_actionPaste_Module_Parameters_triggered();


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

private:
    Ui::MainWindow *ui;

    Parameter paramWin;
    QMap <QString, Parameter *> paramWindow;

    QString VitessDir; 
    QString instrumentName;

    ModulTable* modultab;
    QGridLayout *gridLayout;
    QScrollArea *scrollArea;

    QMap<QString, QStringList> mapHeader = {
        {"RndSeed" , {"--Z"}, },
//        {"RndNoGen", {"???"}},
        {"bGravity", {"--G"},},
        {"nBuffer" , {"--B"},},
        {"MinWght" , {"--U"},},
//        {"InDir"   , {"--I"}},
        {"InDir"   , {"--P",}},
//        {"OutDir"  , {"--O",}},
//        {"LogFile" , {"--L",}},
//        {"Modnum"  , {"--N",}},
    };
    QMap<QString, QMap<QString,QString>> mapModule;
    QMap <QString,QMap<QString,QMap<QString,QString>>> mapVitess;

    QMap<QString,QStringList> Module;
    QMap<QString,QScrollArea *> modulGui;

    QMap<QString,int> modindex;

    QElapsedTimer *timer;
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
};

#endif // MAINWINDOW_H
