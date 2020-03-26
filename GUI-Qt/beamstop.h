#ifndef BEAMSTOP_H
#define BEAMSTOP_H

#include <QWidget>

namespace Ui {
class Beamstop;
}

class Beamstop : public QWidget
{
    Q_OBJECT

public:
    explicit Beamstop(QWidget *parent = nullptr);
    ~Beamstop();

private:
    Ui::Beamstop *ui;
};

#endif // BEAMSTOP_H
