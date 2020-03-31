#ifndef CHOPPER_FERMI_STR_H
#define CHOPPER_FERMI_STR_H

#include <QWidget>

namespace Ui {
class Chopper_fermi_str;
}

class Chopper_fermi_str : public QWidget
{
    Q_OBJECT

public:
    explicit Chopper_fermi_str(QWidget *parent = nullptr);
    ~Chopper_fermi_str();

private:
    Ui::Chopper_fermi_str *ui;
};

#endif // CHOPPER_FERMI_STR_H
