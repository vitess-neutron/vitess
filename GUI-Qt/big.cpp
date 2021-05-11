#include "big.h"
#include "ui_big.h"
#include <iostream>
Big::Big(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Big)
{
    ui->setupUi(this);
    verscrollBar = ui->tBrowser->verticalScrollBar();
    connect( verscrollBar, SIGNAL( rangeChanged(int,int) ), this, SLOT( scrolltoBottom(int,int) ) );
}

Big::~Big()
{
    delete ui;
}

void Big::setBrowserText(QString text)
{
    this->raise();
    ui->tBrowser->setText(text);
    ui->tBrowser->verticalScrollBar()->setValue(verscrollBar->maximum());
}

void Big::on_pushSmall_clicked()
{
    emit small();
    close();
}

void Big::closeEvent( QCloseEvent *ev)
{
   on_pushSmall_clicked();
}

void Big::on_pushClear_clicked()
{
    ui->tBrowser->clear();
    emit clear();
}


void Big::scrolltoBottom(int min,int max)
{
    verscrollBar->setValue(verscrollBar->maximum());
}
