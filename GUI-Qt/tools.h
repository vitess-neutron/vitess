#ifndef TOOLS_H
#define TOOLS_H
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QFormLayout>
#include <QStyleFactory>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QMap>
#include <iostream>
#include <fstream>

#include "string.h"
#include "yaml-cpp/yaml.h"

static QStringList typeList = {"file","string", "float", "int", "combo","switch","window"};
static QStringList strList;
static QString instrumentDir;

static QMap<QString,QString> mapParam = {
    {"type", ""},
    {"descr", ""},
    {"tooltip", ""},
    {"default", ""},
    {"index", ""},
    {"min", ""},
    {"max", ""},
    {"column", ""},
    {"prefix", ""},

};
void getWidgetDesign(QString parName,QMap<QString,QString> mapParameter,
                     QGridLayout *gridLayout,int &row,int &index);
void pythonScript(QString instDir,QStringList cmdList);
void shellScript(QString instDir,QStringList cmdList);

#endif // TOOLS_H
