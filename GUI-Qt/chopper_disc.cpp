#include "chopper_disc.h"
#include "ui_chopper_disc.h"

Chopper_disc::Chopper_disc(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Chopper_disc)
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

Chopper_disc::~Chopper_disc()
{
    delete ui;
}
void Chopper_disc::writePipe(QTextStream& out)
{
      modulePipe(out,map);
}

void Chopper_disc::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Chopper_disc::writeValues(YAML::Node& config)
{
    writeModule(config);
    param.writePara(config,modName);
}

void Chopper_disc::readValues(YAML::Node& config)
{
    readModule(config);
    YAML::Node Windows = config["window"];
    std::cout << "Window size:" << Windows.size() << "type: " << Windows.Type() << std::endl;
    param.readPara(Windows);
}

void Chopper_disc::on_pushButton_clicked()
{
        param.exec();
}
