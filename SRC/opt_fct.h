#ifndef OPTFCT_F_H
#define OPTFCT_F_H

#include <stdio.h>

#include "opt_defs.h"

// External function delivering the F(P; T)
// ----------------------------------------
extern short  FitFctPc  (double F[IMAX+1], const double X[IMAX+1], const double P[NMAX+1], const int nPts, const short nPar);
extern short  OptFctPc  (const double X[IMAX+1], const int nPts, const short  mMin, const short mMax, const short nPar);
extern short  OptFctGrid(const double X[IMAX+1], const int nPts, const short  mMin, const short mMax, const short nPar, char* sGridOpt);

// Basic functions
// ---------------
void   FctF         (double F[IMAX+1],       const int jFct);
short  Calc1Fct     (double F[IMAX+1],       const double P[NMAX+1],    const short m);
short  CalcAllFctsG (const double P[NMAX+1], const double delP[NMAX+1], const short bF0);
short  CalcAllFcts  (const short  mMin,      const short  mMax);
double SquareSum    (const double F[IMAX+1], const short  bPrint);
double ChiSquared   (const double F[IMAX+1], const double sigma, const short bPrint);
double Chi2FromQ    (const double Q,         const double sigma);
double QFromChi2    (const double Chi2,      const double sigma);
void   PrintP       (const double P[NMAX+1], const short bNl);
short  ReadParameter(char* pId, char* sParameter, FILE* pFile);

// Functions of the gradient methods
// ---------------------------------
void   Differentiate(double Nm[NMAX+1][NMAX+1], double r [NMAX+1], const double delP[NMAX+1]);
void   Invert       (double Ni[NMAX+1][NMAX+1], double Nm[NMAX+1][NMAX+1]);

#endif
