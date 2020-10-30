#include "monochromator.h"
#include "ui_monochromator.h"

Monochromator::Monochromator(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Monochromator)
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

    //set items with values from define.h
    ui->d_sprOpt->addItem("LORENTZIAN",QVariant(VtDistr::LORENTZIAN));
    ui->d_sprOpt->addItem("GAUSSIAN",QVariant(VtDistr::GAUSSIAN));
    ui->eMonGeom->addItem("SINGLE_CE",QVariant(VtMonoGeom::SINGLE_CE));
    ui->eMonGeom->addItem("CE_ARRAY_CALC",QVariant(VtMonoGeom::CE_ARRAY_CALC));
    ui->eMonGeom->addItem("CE_ARRAY_FILE",QVariant(VtMonoGeom::CE_ARRAY_FILE));
    ui->eMonMode->addItem("REFL_MONO",QVariant(VtMonoType::REFL_MONO));
    ui->eMonMode->addItem("TRANSM_MONO",QVariant(VtMonoType::TRANSM_MONO));
    ui->eFocGeom->addItem("CONST_LMBD",QVariant(VtMonoFocus::CONST_LMBD));
    ui->eFocGeom->addItem("SPHERICAL",QVariant(VtMonoFocus::SPHERICAL));
    ui->eFocGeom->addItem("VERT_CYL",QVariant(VtMonoFocus::VERT_CYL));
    ui->eFocGeom->addItem("DBL_FOC",QVariant(VtMonoFocus::DBL_FOC));
}

Monochromator::~Monochromator()
{
    delete ui;
}

void Monochromator::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Monochromator::writeCmd(QString& cmd)
{
    moduleCmd(cmd,map);
}

void Monochromator::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Monochromator::readValues(YAML::Node& config)
{
    readModule(config);
}

void Monochromator::on_BrowsePara_clicked()
{
    QString fileName = openFileName();
    ui->ParFile->setText(fileName);
}

void Monochromator::on_EditPara_clicked()
{
    param.exec();
}

void Monochromator::on_BrowseGeom_clicked()
{
    QString fileName = openFileName();
    ui->GeomFile->setText(fileName);
}
