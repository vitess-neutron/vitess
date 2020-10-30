#include "sample_nxs.h"
#include "ui_sample_nxs.h"

Sample_nxs::Sample_nxs(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_nxs)
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

Sample_nxs::~Sample_nxs()
{
    delete ui;
}
void Sample_nxs::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_nxs::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_nxs::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_nxs::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_nxs::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->StrFile->setText(fileName);
}
