#ifndef SERIES_H
#define SERIES_H

#include <QWidget>
#include "yaml-cpp/yaml.h"

namespace Ui {
class Series;
}

class Series : public QWidget
{
    Q_OBJECT

public:
    explicit Series(QWidget *parent = nullptr);
    ~Series();
    void getSerieVariablen(YAML::Node& config);
    void loadSerieVariablen(YAML::Node& config);

signals:
//    void startSeries(QStringList seriesVar ,QVector <QVector <QString>> table,
//                     QStringList copyFiles, QString copyDir);


    void startSeries(QStringList seriesVar ,QVector <QVector <QString>> table,
                     QStringList selSelect, QStringList copyFiles, QString copyDir);

private slots:
    void setSeriesVariable(QString text, QString val);
    void on_iterations_valueChanged(int arg1);

    void on_seriesTable_cellChanged(int row, int column);

    void on_sectionClicked(int index);

    void on_pushStart_clicked();

    void on_pushCancel_clicked();

    void on_pushBrowse_clicked();

    void on_pushScript_clicked();

private:
    Ui::Series *ui;
    QStringList horiHeader;
    QStringList seriesVar;
    int modNr;
    bool ok;
    void setVertHeader();
};

#endif // SERIES_H
