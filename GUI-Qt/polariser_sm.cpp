#include "polariser_sm.h"
#include "ui_polariser_sm.h"

Polariser_sm::Polariser_sm(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Polariser_sm)
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
    allLineEdits = allLineEdits + param.findChildren<QLineEdit*>();
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Polariser_sm::~Polariser_sm()
{
    delete ui;
}
void Polariser_sm::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Polariser_sm::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Polariser_sm::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Polariser_sm::readValues(YAML::Node& config)
{
    readModule(config);
}

void Polariser_sm::on_BrowseUpRef_clicked()
{
    QString fileName = openFileName();
    ui->sReflUp->setText(fileName);
}

void Polariser_sm::on_BrowseDownRef_clicked()
{
    QString fileName = openFileName();
    ui->sReflDn->setText(fileName);
}

void Polariser_sm::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->sParam->setText(fileName);
}

void Polariser_sm::on_PushEdit_clicked()
{
    param.exec();
}
