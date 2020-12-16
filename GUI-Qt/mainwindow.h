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
#include <QButtonGroup>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QStyleFactory>
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

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:

        void showSelectedModul(int);
        void changeModulWidget(QString modul,int row);

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

        void BufferSize_triggered();
        void minNeutWeight_triggered();

private:
    Ui::MainWindow *ui;

    QString VitessDir = "/home/jcns/Downloads/vitess3.4";

    ModulTable* modultab;
    QFormLayout *formLayout;
    QGridLayout *gridLayout;
    QLabel *label,*headerLabel;
    QLineEdit *lEdit;
    QComboBox *cBox;
    QCheckBox *checkBox;
    QButtonGroup *groupBox;
    QPushButton *browseBut, *editBut;
    QValidator *validator;
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
    QMap<QString, QStringList> mapModul;
    QMap <QString,QMap<QString,QStringList>> mapVitess;
//    QMap<QString,QString> Module;
    QMap<QString,QStringList> Module;
    QMap<QString,QScrollArea *> modulGui;

    QMap<QString,int> modindex;

    QList<QLineEdit  *> allLineEdits;
    QList<QComboBox *>  allComboBoxes;
    QList<QCheckBox *>  allCheckBoxes;
    QList < QProcess *> procList;
    QStringList fList,strList,cmdList;
    QStringList typeList = {"file","string", "float", "int", "combo","switch"};
    QString instrumentName;
    QString syspar;
    QString str;
    QString nBuffer;
    QString MinWght;
    QPalette palette;
    int minWidth;
    bool pipeActive = false;
    bool flag,ok;
    YAML::Node config, configChildren;

    QString openFileName();
    void getHeader(QTextStream& out);
    void loadHeader(YAML::Node& config);
    void designModul(QString modulName);
    void getModulParam(YAML::Node& config,QString modulName);
    void writeHeader(YAML::Node& config);
    void saveFile(QString instrumentName);
};

#endif // MAINWINDOW_H
