#include "sample_reflectom.h"
#include "ui_sample_reflectom.h"

Sample_reflectom::Sample_reflectom(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_reflectom)
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
}

Sample_reflectom::~Sample_reflectom()
{
    delete ui;
}

void Sample_reflectom::on_pushEdit_clicked()
{
    param.exec();
}
void Sample_reflectom::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_reflectom::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_reflectom::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_reflectom::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_reflectom::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}

void Sample_reflectom::on_BrowseRefl_clicked()
{
    QString fileName = openFileName();
    ui->ReflFile->setText(fileName);
}
