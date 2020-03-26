#ifndef DETECTOR_H
#define DETECTOR_H

#include <QScrollArea>

namespace Ui {
class Detector;
}

class Detector : public QScrollArea
{
    Q_OBJECT

public:
    explicit Detector(QWidget *parent = nullptr);
    ~Detector();

private:
    Ui::Detector *ui;
};

#endif // DETECTOR_H
