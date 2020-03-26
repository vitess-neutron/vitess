#include "detector.h"
#include "ui_detector.h"

Detector::Detector(QWidget *parent) :
    QScrollArea(parent),
    ui(new Ui::Detector)
{
    ui->setupUi(this);
}

Detector::~Detector()
{
    delete ui;
}
