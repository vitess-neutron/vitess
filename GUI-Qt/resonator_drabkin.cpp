#include "resonator_drabkin.h"
#include "ui_resonator_drabkin.h"

Resonator_drabkin::Resonator_drabkin(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Resonator_drabkin)
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

Resonator_drabkin::~Resonator_drabkin()
{
    delete ui;
}
void Resonator_drabkin::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Resonator_drabkin::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Resonator_drabkin::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Resonator_drabkin::readValues(YAML::Node& config)
{
    readModule(config);
}

void Resonator_drabkin::on_BrowsePol_clicked()
{
    QString fileName = openFileName();
    ui->PolFileO->setText(fileName);
}

void Resonator_drabkin::on_BrowseField_clicked()
{
    QString fileName = openFileName();
    ui->FldFileO->setText(fileName);
}
