#ifndef CHOPPER_PARA_H
#define CHOPPER_PARA_H

#include "yaml-cpp/yaml.h"
#include <QDialog>

namespace Ui {
class Chopper_para;
}

class Chopper_para : public QDialog
{
    Q_OBJECT

public:
    explicit Chopper_para(QWidget *parent = nullptr);
    ~Chopper_para();
    void writePara(YAML::Node& config,std::string moduleName);
    void readPara(YAML::Node& config);

private:
    Ui::Chopper_para *ui;
};

#endif // CHOPPER_PARA_H
