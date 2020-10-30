#include "elasticisotr_para.h"
#include "ui_elasticisotr_para.h"

Elasticisotr_para::Elasticisotr_para(BaseDialog *parent) :
    BaseDialog(parent),
    ui(new Ui::Elasticisotr_para)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    foreach(QString entry, mapLimit.keys())
    {
       QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
       setValidator( lEdit,mapLimit);
       connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                    SLOT(checkIsValide()));
    }

}

Elasticisotr_para::~Elasticisotr_para()
{
    delete ui;
}

