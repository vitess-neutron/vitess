#include "spacewindow.h"
#include "ui_spacewindow.h"

Spacewindow::Spacewindow(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Spacewindow)
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

Spacewindow::~Spacewindow()
{
    delete ui;
}
void Spacewindow::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Spacewindow::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Spacewindow::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Spacewindow::readValues(YAML::Node& config)
{
    readModule(config);
}

void Spacewindow::on_BrowseOut_clicked()
{
    QString fileName = openFileName();
    ui->FileOut->setText(fileName);

}

void Spacewindow::on_BrowseIn_clicked()
{
    QString fileName = openFileName();
    ui->FileIn->setText(fileName);

}
