#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modultable.h"
#include "beamstop.h"
#include "detector.h"
#include "filter.h"
#include "collimator.h"
#include "chopper_fermi_str.h"
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
    void comboModulItemChanged(QString);

private:
    Ui::MainWindow *ui;
    Beamstop  *beamstop;
    Detector *detector;
    Filter *filter;
    Collimator *collimator;
    Chopper_fermi_str *chopper_fermi_str;
    ModulTable* modultab;

};

#endif // MAINWINDOW_H
