#ifndef SANS_PARA_H
#define SANS_PARA_H

#include "yaml-cpp/yaml.h"
#include <QDialog>

namespace Ui {
class Sans_para;
}

class Sans_para : public QDialog
{
    Q_OBJECT

public:
    explicit Sans_para(QWidget *parent = nullptr);
    ~Sans_para();
    void writePara(YAML::Node& config,std::string moduleName);

private:
    Ui::Sans_para *ui;
};

#endif // SANS_PARA_H
