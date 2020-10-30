#include "monitor1d.h"
#include "ui_monitor1d.h"
#include "QMessageBox"
#include <iostream>

using namespace std;

Monitor1D::Monitor1D(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Monitor1D)
{
    ui->setupUi(this);
    foreach(QString entry, map.keys())
        if (this->findChild<QLineEdit *>(entry))
        {
           QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
           setValidator( lEdit,map[entry]);
           connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                        SLOT(checkIsValide()));
           allLineEdits += this->findChild<QLineEdit *>(entry);
        }
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Monitor1D::~Monitor1D()
{
    delete ui;
}
void Monitor1D::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Monitor1D::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Monitor1D::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Monitor1D::readValues(YAML::Node& config)
{
    readModule(config);
}

void Monitor1D::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->MonFile->setText(fileName);

}

void Monitor1D::on_Edit_clicked()
{

}
