//=============================================================================
// File:    big.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: Bigger seperate window for output area
//=============================================================================

#include "big.h"
#include "ui_big.h"
#include <iostream>
Big::Big(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Big)
{
    ui->setupUi(this);
    verscrollBar = ui->tBrowser->verticalScrollBar();
    //scroll always to bottom if range of textbrowser scrollbar changed,
    //perhaps during maesurement
    connect( verscrollBar, SIGNAL( rangeChanged(int,int) ), this, SLOT( scrolltoBottom(int,int) ) );
}

Big::~Big()
{
    delete ui;
}

//write in textbrowser
void Big::setBrowserText(QString text)
{
    this->raise();
    ui->tBrowser->setText(text);
    //scroll to bottom if text changed during big window is shown
    ui->tBrowser->verticalScrollBar()->setValue(verscrollBar->maximum());
}

//show textbrowser in mainWindow
void Big::on_pushSmall_clicked()
{
    emit small();
    close();
}

//close big window
void Big::closeEvent( QCloseEvent *ev)
{
   on_pushSmall_clicked();
   QWidget::closeEvent(ev);
}

//clear output area
void Big::on_pushClear_clicked()
{
    ui->tBrowser->clear();
    emit clear();
}


void Big::scrolltoBottom(int min,int max)
{
    //scroll always to bottom of textbrowser if range changed
    verscrollBar->setValue(verscrollBar->maximum());
}
