#ifndef MONOCHROMATOR_PARA_H
#define MONOCHROMATOR_PARA_H

#include <QDialog>

namespace Ui {
class Monochromator_para;
}

class Monochromator_para : public QDialog
{
    Q_OBJECT

public:
    explicit Monochromator_para(QWidget *parent = nullptr);
    ~Monochromator_para();

private:
    Ui::Monochromator_para *ui;
};

#endif // MONOCHROMATOR_PARA_H
