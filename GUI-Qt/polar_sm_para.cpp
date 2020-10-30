#include "polar_sm_para.h"
#include "ui_polar_sm_para.h"

Polar_sm_para::Polar_sm_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Polar_sm_para)
{
    ui->setupUi(this);
}

Polar_sm_para::~Polar_sm_para()
{
    delete ui;
}
