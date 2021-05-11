#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
   // w.centralWidget()->setStyleSheet("QPushButton {background-color:lightgreen;}");
    w.centralWidget()->setStyleSheet("QWidget {background-color:lightcyan;}"
                                    " QLineEdit {background-color:lightyellow;}"
                                    " QPushButton {background-color:lightgray;}");
    //w.centralWidget()->setStyleSheet("background-color:lightgreen");
    if (argc >1)
    {
        QString fName(argv[1]);
        std::cout << "fName: " << fName.toStdString() << std::endl;
        w.loadInstrument(fName);
    }

    return a.exec();
}
