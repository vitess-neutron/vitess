#include "progress.h"
#include <QFileInfo>

Progress::Progress(int modnum, QVector<bool> disableVec, QString logFname,  QObject *parent)
    : QObject(parent),steps(0)
{
    disableFlag = disableVec;
    logFile = logFname;
    pd = new QProgressDialog("Progress of pipe","",0,modnum);
    pd->setCancelButton(nullptr);
    pd->setValue(1);
    t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &Progress::perform);
    t->start(0);
}

Progress::~Progress()
{
}

void Progress::perform()
{
    if (disableFlag[steps])
            steps++;
    else
    {
      QFileInfo fi (logFile+QString::number(steps+1));
      if (fi.size() > 0)
      {
         pd->setValue(steps+1);
         steps++;
      }
      if (steps >= pd->maximum())
      {
        t->stop();
        pd->close();
      }
    }
}
