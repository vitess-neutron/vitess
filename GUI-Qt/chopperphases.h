#ifndef CHOPPERPHASES_H
#define CHOPPERPHASES_H

#include <QWidget>
#include <QMap>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
namespace Ui {
class ChopperPhases;
}

class ChopperPhases : public QWidget
{
    Q_OBJECT

public:
    explicit ChopperPhases(QString modHeader,QWidget *parent = nullptr);
    ~ChopperPhases();

private slots:
    void on_Calculate_clicked();
    void on_Quit_clicked();

private:
    Ui::ChopperPhases *ui;
    QMap<QString, QString> chopPhas =
    {
        {"roundsMin"   , "-s" },
        {"pulseLen"    , "-p" },
        {"centerPulse" , "-d" },
        {"distSource"  , "-C" },
        {"chopperApp"  , "-a" },
        {"wavelen"     , "-W" },
        {"waveRange"   , "-w" },
    };
    QString cmd;
};

#endif // CHOPPERPHASES_H
