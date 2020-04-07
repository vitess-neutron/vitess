#ifndef CAPTURE_FLUX_H
#define CAPTURE_FLUX_H

#include <QWidget>

namespace Ui {
class Capture_flux;
}

class Capture_flux : public QWidget
{
    Q_OBJECT

public:
    explicit Capture_flux(QWidget *parent = nullptr);
    ~Capture_flux();

private:
    Ui::Capture_flux *ui;
};

#endif // CAPTURE_FLUX_H
