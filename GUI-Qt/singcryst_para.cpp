#include "singcryst_para.h"
#include "ui_singcryst_para.h"

Singcryst_para::Singcryst_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Singcryst_para)
{
    ui->setupUi(this);
}

Singcryst_para::~Singcryst_para()
{
    delete ui;
}
