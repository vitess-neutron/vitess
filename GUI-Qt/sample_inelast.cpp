#include "sample_inelast.h"
#include "ui_sample_inelast.h"

Sample_inelast::Sample_inelast(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_inelast)
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

Sample_inelast::~Sample_inelast()
{
    delete ui;
}

void Sample_inelast::on_pushEdit_clicked()
{
    param.exec();
}
void Sample_inelast::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_inelast::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_inelast::writeValues(YAML::Node& config)
{
    writeModule(config);    //in basemodule
}

void Sample_inelast::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_inelast::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}
