#include "guide.h"
#include "ui_guide.h"

Guide::Guide(BaseModule *parent) :
    BaseModule(parent),
    ui(new Ui::Guide)
{
    ui->setupUi(this);
    foreach(QString entry, map.keys())
        if (this->findChild<QLineEdit *>(entry))
        {
           QLineEdit *lEdit = this->findChild<QLineEdit *>(entry);
           setValidator( lEdit,map[entry]);
           connect(this->findChild<QLineEdit *>(entry), SIGNAL(textChanged(const QString &)),this,
                                                        SLOT(checkIsValide()));
           allLineEdits += this->findChild<QLineEdit *>(entry);
        }
    allComboBoxes = this->findChildren<QComboBox*>();
    modName = this->objectName().toStdString();
    for(int i=1; i<8 ;i++)
    {
       QPushButton* button = this->findChild<QPushButton*>("pushBrowse_" + QString::number(i));
       QPushButton* buttonN = this->findChild<QPushButton*>("pushBrowseN_" + QString::number(i));
       connect(button,SIGNAL(clicked()),this,SLOT(openFile()));
       connect(buttonN,SIGNAL(clicked()),this,SLOT(saveFile()));
    }
}

Guide::~Guide()
{
    delete ui;
}
void Guide::writePipe(QTextStream& out)
{
    modulePipe(out,map);
}

void Guide::writeCmd(QString& cmd)
{
    bool flag;
    foreach(QString entry, map.keys())
    {
      cmd += " " + map[entry][0];
      flag = false;
      if ( filter.indexOf(entry)>=0)
          cmd += mapFilter[this->findChild<QComboBox *>(entry)->currentText()];
      else {
          foreach (QLineEdit* child, allLineEdits)
          {
             if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
             {
                flag = true;
                cmd += child->text();
                break;
             }
          }
          if (!flag)                                                               //if key is no lineEdit
             foreach (QComboBox* child, allComboBoxes)
                if ( entry.indexOf(child->objectName()) == 0 )                   //if key is lineEdit objectname
                {
                    cmd += QString::number(child->currentIndex());
                    break;
                }
      }
    }
}

void Guide::writeValues(YAML::Node& config)
{
    writeModule(config);
}

void Guide::readValues(YAML::Node& config)
{
    readModule(config);
}

void Guide::openFile()
{
    QString fileName = openFileName();
    int i = sender()->objectName().right(1).toInt();
    this->findChild<QLineEdit *>(buttonToFile[i-1])->setText(fileName);
    cout << fileName.toStdString() << endl;
}
void Guide::saveFile()
{
    QString fileName = saveFileName();
    int i = sender()->objectName().right(1).toInt();
    this->findChild<QLineEdit *>(buttonToFile[i-1])->setText(fileName);
    cout << fileName.toStdString() << endl;
}
