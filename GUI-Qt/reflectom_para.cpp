#include "reflectom_para.h"
#include "ui_reflectom_para.h"

Reflectom_para::Reflectom_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Reflectom_para)
{
    ui->setupUi(this);
}

Reflectom_para::~Reflectom_para()
{
    delete ui;
}
