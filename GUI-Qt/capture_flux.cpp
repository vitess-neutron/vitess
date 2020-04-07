#include "capture_flux.h"
#include "ui_capture_flux.h"

Capture_flux::Capture_flux(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Capture_flux)
{
    ui->setupUi(this);
}

Capture_flux::~Capture_flux()
{
    delete ui;
}
