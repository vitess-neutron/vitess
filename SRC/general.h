#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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
#define VT_WINDOWS
# define M_PI            3.14159265358979323846  /* pi */
# define M_PI_2          1.57079632679489661923  /* pi/2 */
# define ISNAN(x) _isnan(x)
#else
# define ISNAN(x) isnan(x)
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

#define MN          1.6749284E-27
#define G           9.80665
#define K           1.380662E-23
#define NA          6.022137E23
#define H_P         6.6260696E-34
#define L_2_E       81805.048
#define E_C         1.6021773E-19
#define THETA_NI    0.099138
#define QC_NI       0.0217
#define NEUTRON_ID  2112

#define TRUE 		    1
#define FALSE 		  0

#define UP          1
#define DOWN        0

#define SPIN_UP     1
#define SPIN_UNDEF  0
#define SPIN_DOWN  -1

#define ON          1
#define OFF         0

#define GUIDEFLIGHT 1

#define ANY_COLOR  -1
#define NO_COLOR    0

#define MOD_THML    1
#define MOD_COLD    2

#define MAX_COLLISIONS      100
#define MAX_CHOPPER_WINDOWS  10
#define LAMBDA_MIN            0.001
#define LAMBDA_MAX          100.0

#define BUFFER_SIZE       50000
#define CHAR_BUF_LENGTH    1024
#define CHAR_BUF_LARGE     5120
#define CHAR_BUF_SMALL      256
#define ROFQ_MAX            512

#define MAX_ULONG    4294967295 //  4.295e09  // 2^32 - 1

#define NO_TRACING      0
#define WRITE_TRC_FILES 1
#define ONLY_TRC_TRAJ   2

//#define DEBUG 1

#ifdef RND_SIMPLE
# ifdef WINDOWS
#  define Vran() rand()
# else
#  define Vran() random()
# endif
#else
# define Vran() gsl_rng_uniform (vit_gsl_rng)
#endif

    
#include "common.h"

typedef enum
{	VT_CUBE    = 1,
	VT_CYL     = 2,
	VT_SPHERE  = 3,
	VT_HOL_CYL = 4
}
SampleGeom;


typedef enum
{	
	VT_RECTANGULAR = 1,
	VT_GAUSSIAN    = 2,
}
VtDistr;


typedef enum
{	
	VT_CREATED   = 0,    // source
	VT_OUTSIDE   = 1,    // guide
	VT_OUT_OF_WND= 2,    // slit
	VT_PASSED    = 3,    // chopper, slit
	VT_ENTERED   = 4,    // guide
	VT_TRANSIT   = 5,    // from one guide segment to the next
	VT_REFLECTED = 6,    // guide or mirror surface
	VT_SCATTERED = 7,    // sample
	VT_ABSORBED  = 8,    // chopper, guide, collimator
	VT_EXITED    = 9,    // guide
	VT_DETECTED  = 10,   // detector
}
VtReason;


typedef double VectorType[3];
typedef double DoublePair[2];


/******************************/
/** Structures               **/
/******************************/

typedef struct
{
	double X,Y,Z;
}
CartesianPoint;


typedef struct
{
	double	A, B, C, D;
}
Plane;


typedef struct
{
	double  A, B, C, D, E, F, W, P, Q, R;
}
SurfaceSecond;


typedef struct
{
	char           IDGrp[2];
	unsigned long  IDNo;
}
TotalID;


typedef struct
{
	TotalID        ID;
	char           Debug;
	short          Color;
	double         Time;
	double         Wavelength;
	double         Probability;
	VectorType     Position;
	VectorType     Vector;
	VectorType     Spin;
}
Neutron;


typedef struct
{
  float          pos[3];
  float          lambda;
  float          weight;
	TotalID        id;
	short          color;
  VtReason       reason;
	short          spin;
}
VtTrajPoint;


typedef struct
{
      double height, width, thickness;
}
CubeType;

typedef struct
{
      double height, r;
}
CylinderType;

typedef struct
{
      double r;
}
BallType;

typedef struct
{
      double h_out, h_in, r_out, r_in;
}
HolCylType;


typedef union
{
    CubeType     Cube;
    CylinderType Cyl;
    BallType     Ball;
    HolCylType   HCyl;
}
SampleGeomType;


typedef struct
{
  SampleGeom Type;
  VectorType Position;
  VectorType Direction;
  SampleGeomType SG;
}
SampleType;


typedef struct
{
  VectorType vPosBeg;
  VectorType vPosEnd;
}
VtLine;

typedef struct
{
  VectorType vCntr;
  VectorType vNormal;
  double     Width;
  double     Height;
  double     rotAngle;
}
VtRectangle;

typedef struct
{
  VectorType vEdges[3];
}
VtTriangle;

typedef struct
{
  VectorType vCntr;
  VectorType vNormal;
  double     Width;
  double     Height;
  double     InnerWidth;
  double     InnerHeight;
}
VtOpenRect;

typedef struct
{
  VectorType vCntr;
  VectorType vNormal;
  double     Radius;
  double     AngleBeg;
  double     AngleEnd;
}
VtCircle;

typedef struct
{
  VectorType vCntr;
  VectorType vNormal;
  double     Length;
  double     Width;
  double     Height;
  double     rotAngle;
}
VtCuboid;

typedef struct
{
  VectorType vCntr;
  VectorType vNormal;
  double     Length;
  double     WidthIn;
  double     WidthOut;
  double     HeightIn;
  double     HeightOut;
  double     rotAngle;
}
VtHull;

typedef struct
{
  VectorType vCntr;
  VectorType vSymAxis;
  double     Length;
  double     Radius;
}
VtCylinder;

typedef struct
{
  VectorType vCntr;
  VectorType vSymAxis;
  double     Length;
  double     Radius;
  double     InnerRadius;
}
VtHolCyl;

typedef struct
{
  VectorType vCntr;
  double     Radius;
}
VtSphere;

typedef struct
{
  VectorType vCntr;
  VectorType vSymAxis;
  double     Length;
  double     Width;
  double     Height;
  double     Xlow;
  double     Xhigh;
}
VtEllipsoid;

typedef struct
{
  VectorType vCntr;
  VectorType vSymAxis;
  double     Radius;
  double     Width;
  double     Height;
  double     Phi;
  double     OpenAngle;
}
VtCylSlice;


typedef struct
{
  McCompID      eModule;
  VtLine*      pLine;
  int          nLines;
  VtRectangle* pRectangle;
  int          nRectangles;
  VtTriangle*  pTriangle;
  int          nTriangles;
  VtOpenRect*  pOpenRect;
  int          nOpenRects;
  VtCircle*    pCircle;
  int          nCircles;
  VtCuboid*    pCuboid;
  int          nCuboids;
  VtHull*      pHull;
  int          nHulls;
  VtCylinder*  pCylinder;
  int          nCylinders;
  VtHolCyl*    pHolCyl;
  int          nHolCyls;
  VtEllipsoid* pEllipsoid;
  int          nEllipsoids;
  VtSphere*    pSphere;
  int          nSpheres;
  VtCylSlice*  pCylSlice;
  int          nCylSlices;
  char*        pDescr;   /* description   */
}
VtModGeom;



typedef struct
{
  McCompID eModule;
  double   dWPar;    /* width, ...             */
  double   dHPar;    /* height, end width, ... */
  double   dRPar;    /* radius, ...            */
  long     nNumber;  /* number of ....         */
  short    eType;    /* shape, mon. par., ...  */
}
ModProp;


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

void   RotVector         (double RotMatrix[3][3], VectorType Vector);
void   RotBackVector     (double RotMatrix[3][3], VectorType Vector);
void   FillRMatrixZY     (double RotMatrix[3][3], const double roty, const double rotz);
void   CartesianToEulerZY(VectorType Vector, double *roty,  double *rotz);
void   EulerToCartesianZY(VectorType Vector, double *roty,  double *rotz);

FILE * fileOpen(const char *name, const char *mode);
void   Error  (const char *text);
void   Warning(const char *text);
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

void  ChangeSlash(char* pStr);

#endif

