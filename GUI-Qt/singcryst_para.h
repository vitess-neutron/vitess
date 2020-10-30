#ifndef SINGCRYST_PARA_H
#define SINGCRYST_PARA_H

#include <QDialog>

namespace Ui {
class Singcryst_para;
}

class Singcryst_para : public QDialog
{
    Q_OBJECT

public:
    explicit Singcryst_para(QWidget *parent = nullptr);
    ~Singcryst_para();

private:
    Ui::Singcryst_para *ui;
};

#endif // SINGCRYST_PARA_H
