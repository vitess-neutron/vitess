#ifndef DUMMY_H
#define DUMMY_H

#include <QWidget>

namespace Ui {
class Dummy;
}

class Dummy : public QWidget
{
    Q_OBJECT

public:
    explicit Dummy(QWidget *parent = nullptr);
    ~Dummy();

private:
    Ui::Dummy *ui;
};

#endif // DUMMY_H
