#include "flipper_coil.h"
#include "ui_flipper_coil.h"

Flipper_coil::Flipper_coil(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Flipper_coil)
{
    ui->setupUi(this);
}

Flipper_coil::~Flipper_coil()
{
    delete ui;
}
