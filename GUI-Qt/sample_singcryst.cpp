#include "sample_singcryst.h"
#include "ui_sample_singcryst.h"

Sample_singcryst::Sample_singcryst(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_singcryst)
{
    ui->setupUi(this);
//    allLineEdits =this->findChildren<QLineEdit*>();
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
    ui->D_distr->addItem("LORENTZIAN",QVariant(VtDistr::LORENTZIAN));
    ui->D_distr->addItem("GAUSSIAN",QVariant(VtDistr::GAUSSIAN));
}

Sample_singcryst::~Sample_singcryst()
{
    delete ui;
}

void Sample_singcryst::on_pushEdit_clicked()
{
    param.exec();
}
void Sample_singcryst::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_singcryst::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_singcryst::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_singcryst::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_singcryst::on_BrowseSmpl_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}

void Sample_singcryst::on_BrowseRefl_clicked()
{
    QString fileName = openFileName();
    ui->StrFile->setText(fileName);
}
