#include "sm_ensemble.h"
#include "ui_sm_ensemble.h"

Sm_ensemble::Sm_ensemble(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Sm_ensemble)
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

Sm_ensemble::~Sm_ensemble()
{
    delete ui;
}
void Sm_ensemble::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Sm_ensemble::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Sm_ensemble::writeValues(YAML::Node& config)
{
    writeModule(config);
    param.writePara(config,modName);
}

void Sm_ensemble::readValues(YAML::Node& config)
{
    readModule(config);
    YAML::Node NewNode = config["Mirror"];
    param.readPara(NewNode);
}

void Sm_ensemble::on_Mirror_clicked()
{
    param.show();
}
