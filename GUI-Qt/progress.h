#ifndef PROGRESS_H
#define PROGRESS_H
#include <QWidget>
#include <QProgressDialog>
#include <QTimer>

class Progress : public QObject
{
    Q_OBJECT
public:
    explicit Progress(int modnum, QVector<bool> disableVec,QString logFname, QObject *parent = 0);
    ~Progress();
    QProgressDialog *pd;

signals:

public slots:
    void perform();
private:
    QVector<bool> disableFlag;
    QString logFile;
    int steps;
//    QProgressDialog *pd;
    QTimer *t;

};

#endif // PROGRESS_H
