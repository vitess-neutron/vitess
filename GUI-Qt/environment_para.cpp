#include "environment_para.h"
#include "ui_environment_para.h"

//#include <QRegExpValidator>


Environment_para::Environment_para(BaseDialog *parent) :
      BaseDialog(parent),
      ui(new Ui::Environment_para)
{
    ui->setupUi(this);
    allLineEdits =this->findChildren<QLineEdit*>();
    setValidatorFloat();
    foreach(QString entry, mapLimit.keys())
    {
        QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
        setValidator( lEdit,mapLimit);
        std::cout << entry.toStdString() << std::endl;
        connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                     SLOT(checkIsValide()));
    }

}

Environment_para::~Environment_para()
{
    delete ui;
}

void Environment_para::on_Close_clicked()
{
    this->close();
}

