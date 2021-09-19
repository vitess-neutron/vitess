//=============================================================================
// File:    progress.cpp
// Author:  Lydia Fleischhauer-Fuß <l.fleischhauer-fuss@fz-juelich.de>
// Date:    03.Sep.2021
// Purpose:
//=============================================================================

#include "progress.h"
#include <QFileInfo>
#include <QTime>

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
    t->start(10);
}

Progress::~Progress()
{
}

void Progress::perform()
{
    if (steps >= disableFlag.count()-1)
    {
      t->stop();
      pd->close();
    }
    else if (disableFlag[steps])
        steps++;
    else
    {
      QFileInfo fi (logFile+QString::number(steps+1));
      if (fi.size() > 0)
      {
         pd->setValue(pd->value() + 1);
         steps++;
      }
    }
}
