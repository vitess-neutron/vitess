#include "slit.h"
#include "ui_slit.h"

Slit::Slit(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Slit)
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

Slit::~Slit()
{
    delete ui;
}
void Slit::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Slit::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Slit::writeValues(YAML::Node& config)
{
    writeModule(config);
}
void Slit::readValues(YAML::Node& config)
{
    readModule(config);
}
