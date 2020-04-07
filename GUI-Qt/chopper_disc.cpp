#include "chopper_disc.h"
#include "ui_chopper_disc.h"

Chopper_disc::Chopper_disc(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Chopper_disc)
{
    ui->setupUi(this);
}

Chopper_disc::~Chopper_disc()
{
    delete ui;
}
