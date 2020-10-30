#ifndef REFLECTOM_PARA_H
#define REFLECTOM_PARA_H

#include <QDialog>

namespace Ui {
class Reflectom_para;
}

class Reflectom_para : public QDialog
{
    Q_OBJECT

public:
    explicit Reflectom_para(QWidget *parent = nullptr);
    ~Reflectom_para();

private:
    Ui::Reflectom_para *ui;
};

#endif // REFLECTOM_PARA_H
