#include "powder_para.h"
#include "ui_powder_para.h"

Powder_para::Powder_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Powder_para)
{
    ui->setupUi(this);
}

Powder_para::~Powder_para()
{
    delete ui;
}
