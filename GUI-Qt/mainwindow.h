#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "modultable.h"
#include "beamstop.h"
#include "detector.h"
#include "filter.h"
#include "flipper_coil.h"
#include "collimator.h"
#include "chopper_disc.h"
#include "chopper_fermi_str.h"
#include "capture_flux.h"


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
    ModulTable* modultab;

};

#endif // MAINWINDOW_H
