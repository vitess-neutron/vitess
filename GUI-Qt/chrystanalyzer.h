#ifndef CHRYSTANALYZER_H
#define CHRYSTANALYZER_H

#include <QWidget>

namespace Ui {
class Chrystanalyzer;
}

class Chrystanalyzer : public QWidget
{
    Q_OBJECT

public:
    explicit Chrystanalyzer(QStringList modulSpec,QWidget *parent = nullptr);
    ~Chrystanalyzer();

private slots:
    void on_Cancel_clicked();
    void on_Execute_clicked();

private:
    Ui::Chrystanalyzer *ui;
    QString vdir;
};

#endif // CHRYSTANALYZER_H
