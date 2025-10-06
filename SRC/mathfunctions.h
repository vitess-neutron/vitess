#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

void SolveQuarticEquation(double a, double b, double c, double d, double *solutions, short switchSign);
double CalculateEllipsePoint(double x, double longAxis, double shortAxis, double sign);
int CalculateEllipseParameters(double w1, double w2, double length, double dist, double *longAxis,
                               double *shortAxis, double *startPoint, double *endPoint);
int CalculateEllipseParametersFromStartAndExitWidths(double w1, double w2, double length, double dist,
                                                     double *longAxis, double *shortAxis, double *startPoint,
                                                     double *endPoint);
double ImprovePrecision(double x, double y, double a, double b, double c, double d);
double CheckSolution(double x, double a, double b, double c, double d);

double RandomLorentzian(double mean, double gamma);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* MATHFUNCTIONS_H */
