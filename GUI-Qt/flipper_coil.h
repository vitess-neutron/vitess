#ifndef FLIPPER_COIL_H
#define FLIPPER_COIL_H

#include <QWidget>

namespace Ui {
class Flipper_coil;
}

class Flipper_coil : public QWidget
{
    Q_OBJECT

public:
    explicit Flipper_coil(QWidget *parent = nullptr);
    ~Flipper_coil();

private:
    Ui::Flipper_coil *ui;
};

#endif // FLIPPER_COIL_H
