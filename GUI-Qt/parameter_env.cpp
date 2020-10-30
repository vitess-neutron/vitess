#include "parameter_env.h"
#include "ui_parameter_env.h"

Parameter_env::Parameter_env(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Parameter_env)
{
    ui->setupUi(this);
}

Parameter_env::~Parameter_env()
{
    delete ui;
}


void Parameter_env::on_Save_clicked()
{

}

void Parameter_env::on_Saveas_clicked()
{

}

void Parameter_env::on_Close_clicked()
{

}
