#include "sample_elasticisotr.h"
#include "ui_sample_elasticisotr.h"
#include <QtGlobal>
#include <limits>

Sample_elasticisotr::Sample_elasticisotr(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_elasticisotr)
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
    allComboBoxes = allComboBoxes + param.findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
}

Sample_elasticisotr::~Sample_elasticisotr()
{
    delete ui;
}

void Sample_elasticisotr::on_pushEdit_clicked()
{
    param.exec();
}
void Sample_elasticisotr::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_elasticisotr::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_elasticisotr::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_elasticisotr::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_elasticisotr::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}
