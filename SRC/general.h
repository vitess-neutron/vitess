#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/** Definitions              **/
/******************************/

#ifdef WIN32
# define VINLINE __inline
#else
# define VINLINE inline
#endif

#ifdef _MSC_VER
# include <float.h>
# define VT_WINDOWS
# define M_PI     3.14159265358979323846  /* pi */
# define M_PI_2   1.57079632679489661923  /* pi/2 */
# define ISNAN(x) _isnan(x)
# define cSlash   '\\'
#else
# define ISNAN(x) isnan(x)
# define cSlash   '/'
#endif

#ifdef  _MSC_VER
/* The Microsoft visual C++ compiler spews about 1000 warnings during */
/* compilation of gnuplot. The following lines disable most of them.  */
# pragma warning(disable: 4018 4056 4244 4305 4706 4761 4756 4996)
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
# if _MSC_VER <= 1200
    typedef unsigned int uintptr_t;
# endif
#endif

#ifdef RND_SIMPLE
# ifdef WINDOWS
#  define Vran() rand()
# else
#  define Vran() random()
# endif
#else
# define Vran() gsl_rng_uniform (vit_gsl_rng)
#endif

    
#include "defines.h"

/******************************/
/** Prototypes               **/
/******************************/

double ENERGY_FROM_LAMBDA(const double lmbd);  // Ang -> meV
double LAMBDA_FROM_ENERGY(const double E);     // meV -> Ang
double ENERGY_FROM_V     (const double v);
double V_FROM_ENERGY     (const double E);
double LAMBDA_FROM_V     (const double v);
double V_FROM_LAMBDA     (const double lmbd);

double Lambda2E (const double lmbd);           // Ang -> meV
double E2Lambda (const double E);              // meV -> Ang

double ReflAngle(const double lambda, const double Q);       // [Ang], [1/Ang] -> [deg]
double QbyRefl  (const double lambda, const double ThetaD);  // [Ang], [deg]   -> [1/Ang]

double MonteCarlo (const double x, const double y);
double DistrGauss(double Module, double Sigma);

double Radians (const double angleD);
double Degrees (const double angleR);
double sq      (const double Value);                        // Value*Value
double atan0   (const double a, const double b);
double Round   (const double value);
double RoundP  (const double value, const int decimal);
void   Exchange(double* pValue1, double* pValue2);
double Min     (const double value1, const double value2);
double Max     (const double value1, const double value2);
long   mini    (const long   value1, const long   value2);
long   maxi    (const long   value1, const long   value2);

double SolidAngle    (const double dHorAngle,     const double dVertAngle);
double TrueSolidAngle(const double dHorAngle,     const double dVertAngle);
double ReflSNT       (char* sTxt, const double Q, const double m, const short bPrint);
double ReflTypical   (            const double Q, const double m);
double ReflTypicalT  (char* sTxt, const double Q, const double m, const short bPrint);
double ReflMirrT     (char* sTxt, const double Q, const double m, const double R0,    const double Rm, const double W, const double Qc, const short bPrint);
int    ReadRofQ      (FILE* pReflFile,                 double* aQ,          double* aR);
int    NumDataPtsQ   (const double Q);   
int    NumDataPtsM   (const double m,             const double  Qc,   const double  W);
void   SetReflData   (double* pReflDat,           const double* aQ,   const double* aR, const int nVals);
double InterpolM     (const double m,             const double* aM,   const double* aR, const int nVals);
double InterpolQ     (const double Q,             const double* aQ,   const double* aR, const int nVals);
double ReflInterpol  (const double Lambda,        const double Angle, const double* Rdata, long nData);

/* Functions using VectorType */
void   InitVector    (VectorType Vector);
void   CopyVector    (const VectorType Src, VectorType Dest);
long   MAXV          (const VectorType Vector);
double LengthVector  (const VectorType Vector);
double DistVector    (const VectorType Vec1, const VectorType Vec2);
double ScalarProduct (const VectorType Vec1, const VectorType Vec2);
double AngleVectors  (const VectorType v1, const VectorType v2);
double Area            (const VectorType v1, const VectorType v2);
short  NormVector      (VectorType Vector);
void   AddVector       (VectorType Value,  const VectorType Add);
void   SubVector       (VectorType Value,  const VectorType Sub);
void   MultiplyByScalar(VectorType Vector, const double Scalar);

void   InitPlane       (Plane* pPlane);

/* Basic matrix operation */
void   Init3x3Matrix     (double Matrix   [3][3]);
void   InitRotMatrix     (double RotMatrix[3][3]);
void   RotVector         (double RotMatrix[3][3], VectorType Vector);
void   RotBackVector     (double RotMatrix[3][3], VectorType Vector);
void   FillRMatrixZY     (double RotMatrix[3][3], const double roty, const double rotz);
void   CartesianToEulerZY(VectorType Vector, double *roty,  double *rotz);
void   EulerToCartesianZY(VectorType Vector, double *roty,  double *rotz);

FILE*  fileOpen          (const char* sName, const char* sMode);
FILE*  fileOpen2         (const char* sName, const char* sMode, const char* sContent);

void   Error  (const char *text);
void   Warning(const char *text);
void   Note   (const char *text);
void   Abort  ();
void   Wait   (float WaitTime);

long   LinesInFile  (FILE* pFile);
long   ColumnsInFile(FILE* pFile);
int    ReadLine     (FILE* pFile, char* pLine, int nStrLen);
void   ReadParString(FILE *fpt, char *stringvar);
double ReadParF(FILE *fpt);
int    ReadParI(FILE *fpt);
void   ReadParComment(FILE *fpt);

void   StrgCopy  (char* sCopy, const char* sOrigin, int nLen);
void   StrgLShift(char* sStr, int kWidth);
long   StrgScanLF(const char* sStr, double* pTable, const int nMax, const int nStart);

void   GetActDate(char* sDate);                      // Gets current date from system  
void   GetActTime(char* sTime);                      // Gets current time from system  

void   ChangeSlash(char* pStr);
void   AddSlash   (char* pStr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

