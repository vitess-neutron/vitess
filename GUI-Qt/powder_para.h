#ifndef POWDER_PARA_H
#define POWDER_PARA_H

#include <QDialog>

namespace Ui {
class Powder_para;
}

class Powder_para : public QDialog
{
    Q_OBJECT

public:
    explicit Powder_para(QWidget *parent = nullptr);
    ~Powder_para();

private:
    Ui::Powder_para *ui;
};

#endif // POWDER_PARA_H
