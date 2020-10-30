#include "monochromator_para.h"
#include "ui_monochromator_para.h"

Monochromator_para::Monochromator_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Monochromator_para)
{
    ui->setupUi(this);
    this->setWindowTitle("Monochromator parameter");
}

Monochromator_para::~Monochromator_para()
{
    delete ui;
}
