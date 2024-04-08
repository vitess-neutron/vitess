//=============================================================================
// File:    main.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    2021
// Purpose: Main program to start application
//=============================================================================

#include "mainwindow.h"
#include <QApplication>

//starts the application
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    //set style of application
    w.centralWidget()->setStyleSheet("QWidget {background-color:lightcyan;}"
                                    " QLineEdit {background-color:lightyellow;}"
                                    " QPushButton {background-color:lightgray;}");

    //call programm with instrumentfilename as argument
    if (argc >1)
    {
        QString fName(argv[1]);
        w.loadInstrument(fName);
    }

    return a.exec();
}
