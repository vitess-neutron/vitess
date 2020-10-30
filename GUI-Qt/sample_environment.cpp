#include "sample_environment.h"
#include "ui_sample_environment.h"

Sample_environment::Sample_environment(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_environment)
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

Sample_environment::~Sample_environment()
{
    delete ui;
}
void Sample_environment::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_environment::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_environment::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_environment::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_environment::on_Edit_clicked()
{
    param.exec();
}

void Sample_environment::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->ParFile->setText(fileName);
}
