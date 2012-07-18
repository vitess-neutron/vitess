#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#include <math.h>


void SolveQuarticEquation(double a, double b, double c, double d, double* solutions, short switchSign);
#ifdef __cplusplus
extern "C" 
#endif
short C_CalculateEllipseParameters(double w1, double w2, double length, double dist, double* longAxis, double* shortAxis);


#endif
