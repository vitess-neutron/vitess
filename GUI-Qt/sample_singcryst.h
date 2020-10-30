#ifndef SAMPLE_SINGCRYST_H
#define SAMPLE_SINGCRYST_H

#include "basemodule.h"
#include "singcryst_para.h"

namespace Ui {
class Sample_singcryst;
}

class Sample_singcryst : public BaseModule
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Sample_singcryst(BaseModule *parent = nullptr);
    ~Sample_singcryst();

private slots:
    void on_pushEdit_clicked();

    void on_BrowseSmpl_clicked();

    void on_BrowseRefl_clicked();

private:
    Ui::Sample_singcryst *ui;
    Singcryst_para param;
    QMap<QString,QStringList> map = {
        {"SmplFile" , {"-P","file","",""}},
        {"StrFile" , {"-S","file","",""}},
        {"d_spread", {"-d","float","",""}},
        {"d_distr" , {"-o","combo",}},
    };
    void writeValues(YAML::Node& config);
    void readValues(YAML::Node& config);
    void writePipe(QTextStream& out);
    void writeCmd(QString& cmd);
};

#endif // SAMPLE_SINGCRYST_H
