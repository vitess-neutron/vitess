#ifndef ELASTICISOTR_PARA_H
#define ELASTICISOTR_PARA_H

#include "basedialog.h"

namespace Ui {
class Elasticisotr_para;
}

class Elasticisotr_para : public BaseDialog
{
    Q_OBJECT

public:
    explicit Elasticisotr_para(BaseDialog *parent = nullptr);
    ~Elasticisotr_para();
    Ui::Elasticisotr_para *ui;
private slots:

private:
//    Ui::Elasticisotr_para *ui;
    QMap<QString,QStringList> mapLimit = {
        {"Height"   , {"float","0.001","",}},
        {"SizePar1" , {"float","0.001","",}},
        {"Width"    , {"float","0.001","",}},
        {"MuAbs"    , {"float","0.001","",}},
        {"MuScaTot" , {"float","0.001","",}},
    };
};

#endif // ELASTICISOTR_PARA_H
