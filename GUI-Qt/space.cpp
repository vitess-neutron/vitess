#include "space.h"
#include "ui_space.h"

Space::Space(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Space)
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

Space::~Space()
{
    delete ui;
}
void Space::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Space::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Space::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Space::readValues(YAML::Node& config)
{
    readModule(config);
}
