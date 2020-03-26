#include "beamstop.h"
#include "ui_beamstop.h"

Beamstop::Beamstop(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Beamstop)
{
    ui->setupUi(this);
}

Beamstop::~Beamstop()
{
    delete ui;
}
