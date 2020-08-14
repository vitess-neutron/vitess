#ifndef GUIDE_ELLIPTIC_H
#define GUIDE_ELLIPTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

extern "C" 
{
  #include "init.h"
  #include "softabort.h"
  #include "general.h"
}

#include "mathvector.h"
#include "mathfunctions.h"


/*** Global variables ***/

int shapeHor=-1;
int shapeVer=-1;

double longAxisHor=-1;
double longAxisVer=-1;
double shortAxisHor=-1;
double shortAxisVer=-1;
double lengthGuide=-1;

double startWidth=-1;
double endWidth=-1;
double startHeight=-1;
double endHeight=-1;

double startPoint=0;
double endPoint=0;
double vertOffset=0.;

double distToFocusHor=0;
double distToFocusVer=-1;

double slopeStraightHor=0;
double slopeStraightVer=0;

std::string reflFileNameTop="";
std::string reflFileNameBottom="";
std::string reflFileNameRight="";
std::string reflFileNameLeft="";

FILE* fReflFileTopPointer=0;
FILE* fReflFileBottomPointer=0;
FILE* fReflFileLeftPointer=0;
FILE* fReflFileRightPointer=0;

int neutronsKilledStraight = 0;
int neutronsKilledParabolic = 0;
int simultaneousCollisions = 0;
int badNeutrons = 0;
int changeColor = 0;

typedef struct
{
  FILE   *pfile;
  const char   *filename;
  double *Rdata;
  long   maxdata;
  double area;
}
ReflFile;

double mNumber[4];
// 0: left, 1: right, 2: top, 3: bottom
ReflFile reflContainer[4];

std::string shapeFile="";
FILE* fShapeFilePointer=0;

MathVector* gravityDirection=0;


/*** Methods ***/
void OwnInit(int argc, char *argv[]);
void LoadReflFile(ReflFile* pReflFile);
void FillReflContainer(ReflFile* reflStruct, double m);
void OwnCleanup();
void SetGeometryData(char* sColor);

int ProcessNeutron(Neutron* n);

bool PropagateStraightTrajectory(Neutron* n, double &dist, double xMin, int plane, int shape);
bool PropagateParabolicTrajectory(Neutron* n, double &dist, double xMin, int plane, int shape);

void IntersectStraightTrajectoryWithEllipse(double longAxis, double shortAxis, double b, double m, double xMin, double &x, double &y);
void IntersectParabolicTrajectoryWithEllipse(double longAxis, double shortAxis, double a0, double a1, double a2, double xMin, double& x, double& y, bool switchSign);
void IntersectTrajectoryWithLinearShape(double slopeFromShape, double shapeWidthAtZero, double a0, double a1, double a2, double xMin, double &x, double &y);

double CalculateAngleAfterReflectionEllipse(double longAxis, double shortAxis, double a1, double a2, double x, int plane, bool positive);
double CalculateAngleAfterReflectionLinear(double slopeFromShape, double a1, double a2, double x, bool positive);
double CalculateGuidePoint(double x, int dir, double sign = 1.);

void WriteIAPEllGuide(Neutron *n, VtReason eReason);

#endif
