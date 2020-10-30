#include "frame.h"
#include "ui_frame.h"

Frame::Frame(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Frame)
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

Frame::~Frame()
{
    delete ui;
}
void Frame::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Frame::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Frame::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Frame::readValues(YAML::Node& config)
{
    readModule(config);
}
