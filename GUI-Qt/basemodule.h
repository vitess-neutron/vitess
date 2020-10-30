#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <iostream>

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QList>
#include "QTextStream"
#include "yaml-cpp/yaml.h"
#include "string.h"
#include "defines.h"

using namespace std;

class BaseModule : public QWidget
{
    Q_OBJECT
public slots:
    void checkIsValide();

public:
    explicit BaseModule(QWidget *parent = nullptr);
    ~BaseModule();
    virtual void writeValues(YAML::Node& config) = 0;
    virtual void readValues(YAML::Node& config) = 0;
    virtual void writePipe(QTextStream& out) = 0;
    virtual void writeCmd(QString& cmd) = 0;

    QString openFileName();
    QString saveFileName();
    void writeModule(YAML::Node& config);
    void readModule(YAML::Node& config);
//    void modulePipe(QTextStream& out,QMap<QString,QString>& map);
    void modulePipe(QTextStream& out,QMap<QString,QStringList>& map);
//    void moduleCmd(QString& cmd,QMap<QString,QString>& map);
    void moduleCmd(QString& cmd,QMap<QString,QStringList>& map);
//    void setValidator(QLineEdit *lEdit, QMap <QString,QStringList> &map);
    void setValidator(QLineEdit *lEdit, QStringList& Limits);
//    void setValidatorFloat();

    QList<QLineEdit *> allLineEdits;
    QList<QComboBox *> allComboBoxes;
    std::string modName;
    QPalette palette;
    QLineEdit *testEdit;
    const QDoubleValidator *vali;
    QString type, min, max;
private:
    bool flag;
    double val;
};

#endif // BASEMODULE_H
