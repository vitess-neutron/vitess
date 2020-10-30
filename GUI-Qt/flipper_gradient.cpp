#include "flipper_gradient.h"
#include "ui_flipper_gradient.h"

Flipper_gradient::Flipper_gradient(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Flipper_gradient)
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

Flipper_gradient::~Flipper_gradient()
{
    delete ui;
}
void Flipper_gradient::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Flipper_gradient::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Flipper_gradient::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Flipper_gradient::readValues(YAML::Node& config)
{
    readModule(config);
}

void Flipper_gradient::on_BrowsePol_clicked()
{
    QString fileName = openFileName();
    ui->PolFileO->setText(fileName);
}

void Flipper_gradient::on_BrowseField_clicked()
{
    QString fileName = openFileName();
    ui->FldFileO->setText(fileName);
}
