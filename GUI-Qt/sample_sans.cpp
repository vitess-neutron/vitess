#include "sample_sans.h"
#include "ui_sample_sans.h"

Sample_sans::Sample_sans(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_sans)
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

Sample_sans::~Sample_sans()
{
    delete ui;
}

void Sample_sans::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_sans::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_sans::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_sans::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_sans::on_Edit_clicked()
{
    param.exec();

}

void Sample_sans::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);

}
