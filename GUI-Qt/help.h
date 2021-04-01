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
    void defaultHelp();

private slots:
    void on_comboBox_activated(const QString &arg1);

private:
    Ui::Help *ui;
    QString helpDir;
    QStringList helpFiles;
    void writeHelp(int);
};

#endif // HELP_H
