#ifndef CHOPPER_DISC_H
#define CHOPPER_DISC_H

#include <QWidget>

namespace Ui {
class Chopper_disc;
}

class Chopper_disc : public QWidget
{
    Q_OBJECT

public:
    explicit Chopper_disc(QWidget *parent = nullptr);
    ~Chopper_disc();

private:
    Ui::Chopper_disc *ui;
};

#endif // CHOPPER_DISC_H
