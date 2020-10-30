#include "monochr_analyser.h"
#include "ui_monochr_analyser.h"

Monochr_analyser::Monochr_analyser(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Monochr_analyser)
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
    ui->d_sprOpt->addItem("LORENTZIAN",QVariant(VtDistr::LORENTZIAN));
    ui->d_sprOpt->addItem("GAUSSIAN",QVariant(VtDistr::GAUSSIAN));
    ui->eMonGeom->addItem("SINGLE_CE",QVariant(VtMonoGeom::SINGLE_CE));
    ui->eMonGeom->addItem("CE_ARRAY_CALC",QVariant(VtMonoGeom::CE_ARRAY_CALC));
    ui->eMonGeom->addItem("CE_ARRAY_FILE",QVariant(VtMonoGeom::CE_ARRAY_FILE));
    ui->eFocGeom->addItem("CONST_LMBD",QVariant(VtMonoFocus::CONST_LMBD));
    ui->eFocGeom->addItem("SPHERICAL",QVariant(VtMonoFocus::SPHERICAL));
    ui->eFocGeom->addItem("VERT_CYL",QVariant(VtMonoFocus::VERT_CYL));
    ui->eFocGeom->addItem("DBL_FOC",QVariant(VtMonoFocus::DBL_FOC));
}

Monochr_analyser::~Monochr_analyser()
{
    delete ui;
}
void Monochr_analyser::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Monochr_analyser::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Monochr_analyser::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Monochr_analyser::readValues(YAML::Node& config)
{
    readModule(config);
}

void Monochr_analyser::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->ParFile->setText(fileName);
}

void Monochr_analyser::on_EditPara_clicked()
{
    param.exec();
}

void Monochr_analyser::on_BrowseGeom_clicked()
{
    QString fileName = openFileName();
    ui->GeomFile->setText(fileName);
}
