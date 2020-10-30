#ifndef PARAMETER_ENV_H
#define PARAMETER_ENV_H

#include <QWidget>

namespace Ui {
class Parameter_env;
}

class Parameter_env : public QWidget
{
    Q_OBJECT

public:
    explicit Parameter_env(QWidget *parent = nullptr);
    ~Parameter_env();

private slots:

    void on_Save_clicked();

    void on_Saveas_clicked();

    void on_Close_clicked();

private:
    Ui::Parameter_env *ui;
};

#endif // PARAMETER_ENV_H
