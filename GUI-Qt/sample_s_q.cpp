#include "sample_s_q.h"
#include "ui_sample_s_q.h"

Sample_s_q::Sample_s_q(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sample_s_q)
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

Sample_s_q::~Sample_s_q()
{
    delete ui;
}
void Sample_s_q::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sample_s_q::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sample_s_q::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Sample_s_q::readValues(YAML::Node& config)
{
    readModule(config);
}

void Sample_s_q::on_pushParameter_clicked()
{
    param.setWindowTitle("Parameter sample_s_q");
    param.exec();
}

void Sample_s_q::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->SmplFile->setText(fileName);
}
