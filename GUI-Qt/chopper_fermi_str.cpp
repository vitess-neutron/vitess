#include "chopper_fermi_str.h"
#include "ui_chopper_fermi_str.h"

Chopper_fermi_str::Chopper_fermi_str(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chopper_fermi_str)
{
    ui->setupUi(this);
}

Chopper_fermi_str::~Chopper_fermi_str()
{
    delete ui;
}
