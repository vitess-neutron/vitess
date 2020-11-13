#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modultable.h"
#include "help.h"
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QProcess>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QScrollArea>
#include "yaml-cpp/yaml.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

        void showSelectedModul(int);
        void changeModulWidget(QString modul,int row);

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
        void on_pushCheck_clicked();
        void on_pushOutdir_clicked();
        void on_pushIndir_clicked();
        void on_pushStart_clicked();
        void on_pushKill_clicked();
        void on_pushStop_clicked();

        void BufferSize_triggered();
        void minNeutWeight_triggered();

private:
    Ui::MainWindow *ui;
    ModulTable* modultab;
    QFormLayout *formLayout;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *lEdit;
    QComboBox *cBox;

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
    QMap<QString, QStringList> mapModul;
    QMap <QString,QMap<QString,QStringList>> mapVitess;
    QMap<QString,QString> Module;
    QMap<QString,QScrollArea *> modulGui;

    QList<QLineEdit  *> allLineEdits;
    QList<QComboBox *>  allComboBoxes;
    QList < QProcess *> procList;
    QStringList fList,strList,cmdList;
    QStringList typeList = {"file","string", "float", "int", "combo"};
    QString instrumentName;
    QString syspar;
    QString str;
    QString nBuffer;
    QString MinWght;
    int minWidth;
    bool pipeActive = false;

    YAML::Node config, configChildren;

    void getHeader(QTextStream& out);
    void loadHeader(YAML::Node& config);
    void getModulParam(YAML::Node& config,QString modulName);
    void writeHeader(YAML::Node& config);
    void saveFile(QString instrumentName);
};

#endif // MAINWINDOW_H
