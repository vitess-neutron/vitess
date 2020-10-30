#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QVector>
#include <QProcess>
#include "modultable.h"
//#include "beamstop.h"
//#include "detector.h"
//#include "filter.h"
#include "flipper_coil.h"
#include "flipper_gradient.h"
#include "frame.h"
//#include "collimator.h"
//#include "collimator_radial.h"
#include "chopper_disc.h"
//#include "chopper_fermi_str.h"
//#include "chopper_fermi_cur.h"
//#include "capture_flux.h"
#include "guide.h"
#include "monitor1d.h"
#include "monitor2d.h"
#include "monochr_analyser.h"
#include "monochromator.h"
#include "polariser_he3.h"
#include "polariser_sm.h"
#include "precessionfield.h"
#include "rotating_field.h"
#include "resonator_drabkin.h"
#include "source.h"
#include "space.h"
#include "slit.h"
#include "sample_environment.h"
#include "sample_elasticisotr.h"
#include "sample_nxs.h"
#include "sample_powder.h"
#include "sample_reflectom.h"
#include "sample_sans.h"
#include "spacewindow.h"
#include "sample_inelast.h"
#include "sample_singcryst.h"
#include "sample_s_q.h"
#include "sm_ensemble.h"
#include "velselect.h"

#include "dummy.h"
#include "help.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{    
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QWidget *newModule;

private slots:
//    void comboModulItemChanged(QString,int);
    void comboModulItemChanged(int);
//    void comboModulItemChangedVal(QString);
    void comboModulItemChangedVal(QString,int);
//    void comboModulItemChangedValue(QString,int);

    void removeModule(int);
    void insertModule(int);

    void finishedLast();

    void on_actionLoad_triggered();

    void on_actionSave_as_triggered();

    void on_actionSave_triggered();

    void on_actionNewInst_triggered();

    void on_actionExit_triggered();

    void on_actionGeneral_Information_triggered();

    void on_actionTutorial_triggered();

    void on_pushFresh_clicked();

    void on_pushClear_clicked();

    void on_pushSave_clicked();

    void on_pushDryrun_clicked();

    void BufferSize_triggered();

    void minNeutWeight_triggered();

    void on_pushCheck_clicked();

    void on_pushOutdir_clicked();

    void on_pushIndir_clicked();

    void on_pushStart_clicked();

    void on_pushKill_clicked();

    void on_pushStop_clicked();

private:
    Ui::MainWindow *ui;
    ModulTable* modultab;
    bool pipeActive = false;
    QString instrumentName;
    QList<QLineEdit *> allLineEdits;
    QList<QComboBox *> allComboBoxes;
    QList<QWidget *> allModulWidgets;

    QMap<QString, QStringList> map = {
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

    YAML::Node config, configChildren;
    QVector<int> modindex;
    QString userName, pwd;
    QString InDir, OutDir;
    QString syspar;
    QString nBuffer;
    QString MinWght;
    QList < QProcess *> procList;
    QStringList cmdList;
    void getHeader(QTextStream& out);
    void writeHeader(YAML::Node& config);
    void loadHeader(YAML::Node& config);
};

#endif // MAINWINDOW_H
