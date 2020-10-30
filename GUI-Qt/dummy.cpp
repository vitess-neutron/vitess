#include "dummy.h"
#include "ui_dummy.h"

Dummy::Dummy(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Dummy)
{
    ui->setupUi(this);
}

Dummy::~Dummy()
{
    delete ui;
}
