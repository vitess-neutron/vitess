#include "precessionfield.h"
#include "ui_precessionfield.h"

Precessionfield::Precessionfield(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Precessionfield)
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

Precessionfield::~Precessionfield()
{
    delete ui;
}
void Precessionfield::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Precessionfield::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Precessionfield::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Precessionfield::readValues(YAML::Node& config)
{
    readModule(config);
}

void Precessionfield::on_BrowseField_clicked()
{
    QString fileName = openFileName();
    ui->FieldMap->setText(fileName);
}
