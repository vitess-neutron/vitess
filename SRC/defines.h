#ifndef DEFINES_H
#define DEFINES_H

/***********************/
/** Definitions       **/
/***********************/

#define MN          1.6749284E-27
#define G           9.80665
#define K           1.380662E-23
#define NA          6.022137E23
#define H_P         6.6260696E-34
#define L_2_E       81805.048
#define E_C         1.6021773E-19
#define THETA_NI    0.09894   // the old value 0.099138° corresponds to QC_NI=0.021743 1/Ang
#define QC_NI       0.0217
#define NEUTRON_ID  2112

#define TRUE 		    1
#define FALSE 		  0
#define MISSING 	 -1

#define UP          1
#define DOWN        0

#define SPIN_UP     1
#define SPIN_UNDEF  0
#define SPIN_DOWN  -1

#define ON          1
#define OFF         0

#define NO  0
#define YES 1

#define NN  0

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

#define VT_EOF -1

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
#define PATH_LEN            128  // maximal length of path
#define NAME_LEN            256  // maximal length of path + filename

#define MAX_ULONG    4294967295 //  4.295e09  // 2^32 - 1

#define NO_TRACING      0
#define WRITE_TRC_FILES 1
#define ONLY_TRC_TRAJ   2

typedef double VectorType[3];
typedef double DoublePair[2];


/***********************/
/** Enums             **/
/***********************/

// General
// -------
typedef enum
{
  NOT_EXIST = -1,  // module is completely ignored
  INACTIVE  =  0,  // module is replaced by space, i.e. instrument length is kept constant
  ACTIVE    =  1,  // module is treated normally
  FIRST_PAR =  2,  // first of an array of parallel modules: lost neutrons are written with their input values, neutrons that passed are written with x > 0
  MIDDLE_PAR=  3,  // module in between other parallel mod.: neutrons with x > 0 are ignored, lost neutrons are written with their input values, neutrons that passed are written with x > 0
  LAST_PAR  =  4   // last of an array of parallel modules : neutrons with x > 0 are ignored, otherwise neutrons are treated normally
}
VtModAct;

typedef enum
{
  LORENTZIAN = 1, 
  GAUSSIAN   = 2
}
VtDistr;

typedef enum
{
  MCN_COMP_UNKNOWN =   0,
	MCN_SOURCE       = 100,
	MCN_SRC_SMPL     = 110,
	MCN_SRC_CWS      = 120,
	MCN_SRC_TOF      = 130,
	MCN_SRC_SP       = 140,
	MCN_SRC_LP       = 150,
	MCN_READ_IN      = 190,
	MCN_SPACE        = 200,
	MCN_SLIT         = 210,
	MCN_WINDOW       = 220,
	MCN_WND_MULT     = 222,
	MCN_GRID         = 224,
	MCN_LENSE        = 230,
	MCN_MIRROR       = 240,
	MCN_MIRROR_POL   = 242,
	MCN_MIRROR_ELLI  = 244,
	MCN_SM_ENSEMBLE  = 250,
	MCN_COLLIMATOR   = 260,
	MCN_COLL_SOLLER  = 262,
	MCN_COLL_RADIAL  = 264,
	MCN_COLL_VIRT    = 266,
	MCN_GUIDE        = 270,
	MCN_GUIDE_IDEAL  = 275,
	MCN_BENDER       = 280,
	MCN_CHOP_DISC    = 310,
	MCN_CHOP_FERMI   = 320,
	MCN_VEL_SELECT   = 330,
	MCN_MONO_ANA     = 350,
	MCN_MONOCHROM    = 352,
	MCN_POL_HE3      = 410,
	MCN_POL_SM       = 412,
	MCN_FLIP_COIL    = 420,
	MCN_FLIP_GRAD    = 422,
	MCN_RES_DRABKIN  = 430,
	MCN_FIELD_PREC   = 450,
	MCN_FIELD_ROT    = 460,
	MCN_FIELD_SESANS = 470,
	MCN_CAPTURE      = 510,
	MCN_BEAMSTOP     = 520,
	MCN_SMPL_ENVIRO  = 530,
	MCN_DETECTOR     = 540,
	MCN_WRITEOUT     = 590,
	MCN_SMPL_EL_ISO  = 610,
	MCN_SMPL_INELAST = 620,
	MCN_SMPL_SNGL_X  = 630,
	MCN_SMPL_POWDER  = 640,
	MCN_SMPL_S_Q     = 650,
	MCN_SMPL_NXS     = 660,
	MCN_SMPL_SANS    = 670,
	MCN_SMPL_REFL    = 680,
	MCN_FRAME        = 710,
	MCN_FILTER       = 720,
	MCN_RESET        = 730,
	MCN_VISUAL       = 740,
	MCN_MONITOR1     = 800,
	MCN_MON1         = 810,
	MCN_MON1_BRL     = 820,
	MCN_MON1_POL     = 840,
	MCN_MONITOR2     = 850,
	MCN_MON2_POS     = 860,
	MCN_MON2_DIV     = 870,
	MCN_MON2_KDIV    = 872,
	MCN_MON2_RDIV    = 874,
	MCN_MON2_POSDIV  = 876,
	MCN_MON2_WLDIV   = 878,
	MCN_MON2_TOFWL   = 880,
	MCN_MON2_POL_POS = 890,
	MCN_EVAL1_ELAST  = 900,
	MCN_EVAL1_SANS   = 910,
	MCN_EVAL1_INELAST= 920,
	MCN_EVAL2_ELAST  = 950,
	MCN_RUNTIME      = 980,
	MCN_TOOL_A2B     = 1010,
	MCN_TOOL_CAS     = 1020,
	MCN_TOOL_CHOP    = 1030,
	MCN_TOOL_DEF_DIR = 1040,
	MCN_TOOL_DIR_VIEW= 1050,
	MCN_TOOL_GEN_BAT = 1055,
	MCN_TOOL_GEN_COAT= 1060,
	MCN_TOOL_GEN_EXTR= 1070,
	MCN_TOOL_GEN_SURF= 1080,
	MCN_TOOL_STD_DEV = 1090,
	MCN_TOOL_LAT_DST = 1100,
	MCN_TOOL_GUIDE   = 1110,
	MCN_TOOL_DST_TIME= 1120,
	MCN_TOOL_ANLZ_2D = 1130,
  MCN_OPT_MAIN     = 1200,
  MCN_OPT_FOM      = 1210,
  MCN_OPT_PIPE     = 1220,
}
McCompID;

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
VtWaviDistr;

// monochromator
// -------------
typedef enum
{
  SINGLE_CE     = 1,
  CE_ARRAY_CALC = 2,
  CE_ARRAY_FILE = 3
}
VtMonoGeom;

typedef enum
{
  REFL_MONO   = 1,
  TRANSM_MONO = 2
}
VtMonoType;

typedef enum
{
  CONST_LMBD = 1,
  SPHERICAL  = 2,
  VERT_CYL   = 3,
  DBL_FOC    = 4
}
VtMonoFocus;

// monitors
// --------
typedef enum
{
  NO_MON_PAR = 0,
  MON_LAMBDA = 1,
  MON_TIME   = 2,
  MON_DIV_Y  = 3,
  MON_DIV_Z  = 4,
  MON_Y      = 5,
  MON_Z      = 6,
  MON_ENERGY = 7,
  MON_DIV_YZ = 8,
}
VtMonPar;

typedef enum
{
  NO_PAR   =  0,
  POS_X    = 17,   
  POS_Y    =  1,
  POS_Z    =  2,
  DIV_Y    =  3,
  DIV_Z    =  4,
  LAMBDA   =  5,
  ENERGY   =  6,
  TIME     =  7,
  K_Y      =  8,
  K_Z      =  9,
  POS_R    = 10,
  POS_PHI  = 11,
  DIR_PHI  = 15,
  DIR_THETA= 16,
  COL_VERT = 12,
  COL_HOR  = 13,
  COLOR    = 14,
}
VtPar;

typedef enum
{
  NO_NORM   = 0,   // no normalization
  NORM_SIZE = 1,   // normalization by bin size 
  NORM_REF  = 2    // relative to reference file
}
VtMonNorm;

typedef enum
{
  BRL_ABS    = 1,  // absolute brilliance
  BRL_TRANSF = 2,  // brilliance transfer, i.e. relative to reference file
  BRL_PCT    = 3   // brilliance within 1 percent DelLmbd/Lmbd
}
VtBrlNorm;

typedef enum
{
  NO_FORMAT =-1,
  MATRIX    = 0,
  XYZ       = 1,
  MATR_CMPT = 2,
  XYZ_CMPT  = 3
}
VtFormat2D;

// reading and writing trajectories
// --------------------------------
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

typedef enum
{ VT_VITESS_FMT = 1,
  VT_MCSTAS_FMT = 2,
  VT_MCPL_FMT   = 3,
  VT_MCNP_FMT   = 4,
  VT_MCNPX_FMT  = 5
}
VtPrgFormat;

typedef enum
{ VT_EXPONENTIAL = 0,
  VT_FLOAT       = 1,
  VT_BINARY      = 2
}
VtDataFormat;

typedef enum
{ VT_BLANK     = 0,
  VT_TABULATOR = 1
}
VtSeparator;


/***********************/
/** Structures        **/
/***********************/

// General
// -------
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

// Samples
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

// Trajectories
// ------------
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
	double         Weight;
	VectorType     Position;
	VectorType     Speed;
	double         Time;
	VectorType     Spin;
}
McNeutron;

typedef struct
{
	VectorType     Position;
	VectorType     Vector;
  double         Energy;
	double         Counts;
	double         Shakes;
}
McnpNeutron;

typedef struct
{
  double         History;
  double         ID;
  double         Counts;
  double         Energy;
	double         Shakes;
	VectorType     Position;
	VectorType     Vector;
  double         Unknown;
}
McnpxNeutron;

// Visualization
// -------------
typedef struct
{
  VectorType vPosBeg;
  VectorType vPosEnd;
}
VtLine;

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

// obsolete
// --------
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

#endif

