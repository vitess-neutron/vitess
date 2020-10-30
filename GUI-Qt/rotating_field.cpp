#include "rotating_field.h"
#include "ui_rotating_field.h"

Rotating_field::Rotating_field(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Rotating_field)
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

Rotating_field::~Rotating_field()
{
    delete ui;
}
void Rotating_field::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Rotating_field::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Rotating_field::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Rotating_field::readValues(YAML::Node& config)
{
    readModule(config);
}

void Rotating_field::on_BrowseAmpl_clicked()
{
    QString fileName = openFileName();
    this->findChild<QLineEdit *>(FileEntry[0])->setText(fileName);

}

void Rotating_field::on_BrowsePol_clicked()
{
    QString fileName = openFileName();
    this->findChild<QLineEdit *>(FileEntry[1])->setText(fileName);

}

void Rotating_field::on_BrowseField_clicked()
{
    QString fileName = openFileName();
    this->findChild<QLineEdit *>(FileEntry[2])->setText(fileName);

}
