#include "polariser_he3.h"
#include "ui_polariser_he3.h"

Polariser_he3::Polariser_he3(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Polariser_he3)
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

Polariser_he3::~Polariser_he3()
{
    delete ui;
}
void Polariser_he3::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Polariser_he3::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Polariser_he3::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Polariser_he3::readValues(YAML::Node& config)
{
    readModule(config);
}

void Polariser_he3::on_BrowsePol_clicked()
{
    QString fileName = openFileName();
    ui->sPolFil->setText(fileName);
}

void Polariser_he3::on_BrowseTrans_clicked()
{
    QString fileName = openFileName();
    ui->sTrnsFil->setText(fileName);
}
