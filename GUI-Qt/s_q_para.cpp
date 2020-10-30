#include "s_q_para.h"
#include "ui_s_q_para.h"

S_q_para::S_q_para(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::S_q_para)
{
    ui->setupUi(this);
}

S_q_para::~S_q_para()
{
    delete ui;
}
