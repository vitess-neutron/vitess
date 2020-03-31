#ifndef COLLIMATOR_H
#define COLLIMATOR_H

#include <QWidget>

namespace Ui {
class Collimator;
}

class Collimator : public QWidget
{
    Q_OBJECT

public:
    explicit Collimator(QWidget *parent = nullptr);
    ~Collimator();

private:
    Ui::Collimator *ui;
};

#endif // COLLIMATOR_H
