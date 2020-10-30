#ifndef POLAR_SM_PARA_H
#define POLAR_SM_PARA_H

#include <QDialog>

namespace Ui {
class Polar_sm_para;
}

class Polar_sm_para : public QDialog
{
    Q_OBJECT

public:
    explicit Polar_sm_para(QWidget *parent = nullptr);
    ~Polar_sm_para();

private:
    Ui::Polar_sm_para *ui;
};

#endif // POLAR_SM_PARA_H
