#include "monitor2d.h"
#include "ui_monitor2d.h"

using namespace std;

Monitor2D::Monitor2D(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Monitor2D)
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
    ui->setupUi(this);
}

Monitor2D::~Monitor2D()
{
    delete ui;
}
void Monitor2D::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Monitor2D::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Monitor2D::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Monitor2D::readValues(YAML::Node& config)
{
    readModule(config);
}

void Monitor2D::on_Browse_clicked()
{
    QString fileName = openFileName();
    ui->MonFile->setText(fileName);

}
