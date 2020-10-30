#ifndef S_Q_PARA_H
#define S_Q_PARA_H

#include "yaml-cpp/yaml.h"
#include <QDialog>

namespace Ui {
class S_q_para;
}

class S_q_para : public QDialog
{
    Q_OBJECT

public:
    explicit S_q_para(QWidget *parent = nullptr);
    ~S_q_para();
    void writePara(YAML::Node& config,std::string moduleName);

private:
    Ui::S_q_para *ui;
};

#endif // S_Q_PARA_H
