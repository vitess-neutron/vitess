#ifndef INELAST_PARA_H
#define INELAST_PARA_H

#include "basedialog.h"


namespace Ui {
class Inelast_para;
}

class Inelast_para : public BaseDialog
{
    Q_OBJECT

public:
    explicit Inelast_para(BaseDialog *parent = nullptr);
    ~Inelast_para();
//    void writePara(YAML::Node& config,std::string moduleName);

    //    void readPara(YAML::Node& config);

private:
    Ui::Inelast_para *ui;    
    QMap<QString,QStringList> mapLimit = {
        {"ScatLmbd" , {"float","0.001","",}},
        {"ScatHor"  , {"float","-180.0","180.0",}},
        {"ScatVert" , {"float","-180.0","180.0",}},
        {"VarHor"   , {"float","0.0","180.0",}},
        {"VarVert"  , {"float","0.0","180.0",}},
        {"MuScaTot" , {"float","0.001","",}},
        {"MuAbs"    , {"float","0.001","",}},
        {"Height"   , {"float","0.001","",}},
        {"Width"    , {"float","0.001","",}},
        {"SizePar1" , {"float","0.001","",}},
        {"LmbdInit" , {"float","0.001","",}},
    };
};

#endif // INELAST_PARA_H
