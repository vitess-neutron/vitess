#include "sample_powder.h"
#include "ui_sample_powder.h"

Sample_powder::Sample_powder(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_powder)
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

Sample_powder::~Sample_powder()
{
    delete ui;
}

void Sample_powder::on_pushEdit_clicked()
{
    param.exec();
}
void Sample_powder::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_powder::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_powder::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_powder::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_powder::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}
