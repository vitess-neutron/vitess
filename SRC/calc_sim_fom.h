#ifndef CALC_SIM_H
#define CALC_SIM_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "opt_defs.h"

#define NMAX_H   8     // 1/2 * NMAX

/*********************************************/
/* prototypes                                */
/*********************************************/
short  ExtFunction (double F[IMAX+1], const double X[IMAX+1], const double P[NMAX+1], const int nPts, const short nPar);
short  ExtFunctions(const double X[IMAX+1], const int nPts, const short  mMin, const short mMax, const short nPar);

#endif