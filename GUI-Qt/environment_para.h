#ifndef ENVIRONMENT_PARA_H
#define ENVIRONMENT_PARA_H

#include "basedialog.h"

namespace Ui {
class Environment_para;
}

class Environment_para : public BaseDialog
{
    Q_OBJECT

public:
    explicit Environment_para(BaseDialog *parent = nullptr);
    ~Environment_para();

private slots:
    void on_Close_clicked();


private:
    Ui::Environment_para *ui;
    QMap<QString,QStringList> mapLimit = {
        {"UCV"      , {"float","0.001","",}},
        {"Height"   , {"float","0.001","",}},
        {"Diameter" , {"float","0.001","",}},
        {"Thicknes" , {"float","0.001","",}},
        {"MuAbs"    , {"float","0.0","",}},
        {"MuScaTot" , {"float","0.0","",}},
        {"MuScaInc" , {"float","0.0","",}},
    };
};

#endif // ENVIRONMENT_PARA_H
