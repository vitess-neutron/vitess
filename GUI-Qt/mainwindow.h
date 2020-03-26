#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "modultable.h"
#include "beamstop.h"
#include "detector.h"

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
    //    ModulTable *modultab;
    ModulTable* modultab;
};

#endif // MAINWINDOW_H
