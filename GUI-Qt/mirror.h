#ifndef MIRROR_H
#define MIRROR_H

#include "yaml-cpp/yaml.h"
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include "QTextStream"
#include "string.h"

using namespace std;

namespace Ui {
class Mirror;
}

class Mirror : public QWidget
{
    Q_OBJECT

public:
    explicit Mirror(QWidget *parent = nullptr);
    ~Mirror();
    void writePara(YAML::Node& config,std::string moduleName);
    void readPara(YAML::Node& config);

private slots:

    void on_nrWin_valueChanged(int arg1);

private:
    Ui::Mirror *ui;
    QWidgetList MirrorWindows;
    QList<QLineEdit *> lEdits;
    QList<QComboBox *> cBoxes;
    bool flag;
};

#endif // MIRROR_H
