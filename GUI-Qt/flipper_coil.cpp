#include "flipper_coil.h"
#include "ui_flipper_coil.h"

Flipper_coil::Flipper_coil(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Flipper_coil)
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

Flipper_coil::~Flipper_coil()
{
    delete ui;
}

void Flipper_coil::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Flipper_coil::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Flipper_coil::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Flipper_coil::readValues(YAML::Node& config)
{
    readModule(config);
}
