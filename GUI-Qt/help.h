#ifndef HELP_H
#define HELP_H

#include <QDialog>

namespace Ui {
class Help;
}

class Help : public QDialog
{
    Q_OBJECT

public:
    explicit Help(QWidget *parent = nullptr);
    ~Help();
    void guiHelp();
    void showHelp(int);

private slots:

    void on_comboBox_activated(int index);

private:
    Ui::Help *ui;
    QString helpDir;
    QStringList helpFiles;
};

#endif // HELP_H
