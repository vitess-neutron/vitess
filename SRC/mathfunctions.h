#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#include <math.h>

void SolveQuarticEquation(double a, double b, double c, double d, double* solutions, short switchSign);
double CalculateEllipsePoint(double x, double longAxis, double shortAxis, double sign);
#ifdef __cplusplus
short CalculateEllipseParametersFromStartAndExitWidths(double w1, double w2, double length, double dist, double &longAxis, double &shortAxis, double& startPoint, double& endPoint);
extern "C" 
#endif
short C_CalculateEllipseParameters(double w1, double w2, double length, double dist, double* longAxis, double* shortAxis, double* startPoint, double* endPoint);
double ImprovePrecision(double x, double y, double a, double b, double c, double d);
double CheckSolution(double x, double a, double b, double c, double d);

double RandomLorentzian(double mean, double gamma);

#endif
