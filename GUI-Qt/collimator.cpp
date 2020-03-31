#include "collimator.h"
#include "ui_collimator.h"

Collimator::Collimator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Collimator)
{
    ui->setupUi(this);
}

Collimator::~Collimator()
{
    delete ui;
}
