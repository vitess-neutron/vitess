#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/******************************/
/** Definitions              **/
/******************************/

#ifdef _MSC_VER
#define M_PI            3.14159265358979323846  /* pi */
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#endif

#define MN          1.6749284E-27
#define G           9.80665
#define K           1.380662E-23
#define NA          6.022137E23
#define H           6.626076E-34
#define L_2_E       81805.048
#define E_C         1.6021773E-19

#define TRUE 		1
#define FALSE 		0

#define UP          1
#define DOWN        0

#define ON          1
#define OFF         0

#define GUIDEFLIGHT 1

#define MAX_COLLISIONS      100
#define MAX_CHOPPER_WINDOWS  10
#define LAMBDA_MIN            0.001
#define LAMBDA_MAX          100.0

#define BUFFER_SIZE       10000
#define CHAR_BUF_LENGTH    1024
#define CHAR_BUF_LARGE     5120
#define CHAR_BUF_SMALL      256

#ifdef RND_SIMPLE
  #ifdef WINDOWS
    #define Vran() rand()
  #else
    #define Vran() random()
  #endif
#else  
  #define Vran() gsl_rng_uniform (vit_gsl_rng)
#endif

typedef enum
{	VT_CUBE    = 1,
	VT_CYL     = 2,
	VT_SPHERE  = 3,
	VT_HOL_CYL = 4
}
SampleGeom;


typedef enum
{
	VT_SOURCE      =   1,
	VT_GUIDE       =  11,
	VT_BENDER      =  12,
	VT_COLLIMATOR  =  13,
	VT_RAD_COLLIM  =  14,
	VT_SM_ENSEMBLE =  15,
	VT_SPACE       =  20,
	VT_WINDOW      =  21,
	VT_WND_MULT    =  22,
	VT_GRID        =  23,
	VT_SLIT        =  24,
	VT_CHOP_DISC   =  31,
	VT_CHOP_FERMI  =  32,
	VT_VEL_SELECT  =  41,
	VT_MONOC_ANALY =  45,
	VT_POL_HE3     =  51,
	VT_POL_SM      =  52,
	VT_POL_MIRROR  =  53,
	VT_FLIP_COIL   =  55,
	VT_FLIP_GRAD   =  56,
	VT_RES_DRABKIN =  59,
	VT_PREC_FIELD  =  60,
	VT_ROT_FIELD   =  61,
	VT_SESANS_FIELD=  62,
	VT_DETECTOR    =  71,
	VT_SMPL_EL_ISO =  81,
	VT_SMPL_INELAST=  83,
	VT_SMPL_SING_X =  84,
	VT_SMPL_POWDER =  85,
	VT_SMPL_S_Q    =  86,
	VT_SMPL_SANS   =  87,
	VT_SMPL_REFL   =  89,
	VT_SMPL_ENVIRON=  90,
	VT_MONITOR_1   = 101,
	VT_MONITOR_2   = 102,
	VT_MON_POL_1   = 103,
	VT_MON_POL_POS = 104,
	VT_CAPTURE     = 110,
	VT_EVAL_ELAST  = 111,
	VT_EVAL_ELAST2 = 222,
	VT_EVAL_INELAST= 112,
	VT_VISUAL      = 121,
	VT_FRAME       = 131,
	VT_WRITEOUT    = 141,
	VT_RESET       = 142,
	VT_TOOL        = 999
}
VtModID;


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
	VtModID  eModule;
	double   dWPar;    /* width, ...             */
	double   dHPar;    /* height, end width, ... */
	double   dRPar;    /* radius, ...            */
	long     nNumber;  /* number of ....         */
	short    eType;    /* shape, mon. par., ...  */
	char*    pDescr;   /* material, ...          */
}
ModProp;



/******************************/
/** Prototypes               **/
/******************************/

double ENERGY_FROM_LAMBDA(double x);
double LAMBDA_FROM_ENERGY(double x);
double ENERGY_FROM_V   (double x);
double V_FROM_LAMBDA   (double x);
double LAMBDA_FROM_V(double x);

double MonteCarlo (double x, double y);
double DistrGauss(double Module, double Sigma);

double sq   (double Value);                        /* = Value*Value*/
double atan0(double a, double b);
double Round(double value);
void   Exchange(double* pValue1, double* pValue2);
double Min(double value1, double value2);
double Max(double value1, double value2);
long   mini(long value1, long value2);
long   maxi(long value1, long value2);

double SolidAngle   (const double dHorAngle, const double dVertAngle);

void   CopyVector   (const VectorType Src, VectorType Dest);
long   MAXV         (const VectorType Vector);
double LengthVector (const VectorType Vector);
double DistVector   (const VectorType Vec1, const VectorType Vec2);
double ScalarProduct(const VectorType Vec1, const VectorType Vec2);
double AngleVectors (VectorType v1, VectorType v2);
double Area(VectorType v1, VectorType v2);
short  NormVector      (VectorType Vector);
void   AddVector       (VectorType Value,  const VectorType Add);
void   SubVector       (VectorType Value,  const VectorType Sub);
void   MultiplyByScalar(VectorType Vector, const double Scalar);
void   RotVector       (double RotMatrix[3][3], VectorType Vector);
void   RotBackVector   (double RotMatrix[3][3], VectorType Vector);
void   FillRMatrixZY   (double RotMatrix[3][3], double roty, double rotz);

FILE * fileOpen(const char *name, char *mode);
void   Error(const char *text);
void   Warning(const char *text);
void   Abort();

int    ReadLine(FILE* pFile, char* pLine, int nStrLen);
void   ReadParString(FILE *fpt, char *stringvar);
double ReadParF(FILE *fpt);
int    ReadParI(FILE *fpt);
void   ReadParComment(FILE *fpt);

void   StrgCopy  (char* sCopy, const char* sOrigin, int nLen);
#ifdef VERS26
void   StrgLShift(char* sStr, int kWidth);
#endif
long   StrgScanLF(const char* sStr, double* pTable, const int nMax, const int nStart);

#endif

