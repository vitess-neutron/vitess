#ifndef BIG_H
#define BIG_H

#include <QWidget>
#include <QScrollBar>

namespace Ui {
class Big;
}

class Big : public QWidget
{
    Q_OBJECT

public:
//    explicit Big(QString browserText, QWidget *parent = nullptr);
    explicit Big(QWidget *parent = nullptr);
    ~Big();


private slots:
    void setBrowserText(QString text);
    void scrolltoBottom(int min,int max);
    void on_pushSmall_clicked();

    void on_pushClear_clicked();

private:
    Ui::Big *ui;
    QString text;
    QScrollBar *verscrollBar;
    void closeEvent(QCloseEvent *ev);

signals:
    //void small(QString test);
    void small();
    void clear();
};

#endif // BIG_H
