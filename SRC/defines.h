#ifndef DEFINES_H
#define DEFINES_H

/***********************/
/** Definitions       **/
/***********************/

#define MN          1.6749284E-27
#define G           9.80665
#define KB          1.380662E-23
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

#define BUFFER_SIZE       10000
#define CHAR_BUF_LENGTH    1024
#define CHAR_BUF_LARGE     5120
#define CHAR_BUF_SMALL      256
#define CHAR_BUF_XS         128
#define ROFQ_MAX            512
#define PATH_LEN            128  // maximal length of path
#define NAME_LEN            256  // maximal length of path + filename

#define MAX_ULONG    4294967295 //  4.295e09  // 2^32 - 1

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */

typedef double VectorType[3];
typedef double DoublePair[2];


/***********************/
/** Enums             **/
/***********************/

// General
// -------

// Random number generator
typedef enum
{
  VT_RAN3    = 1, 
  VT_TAUS    = 2,
  VT_GFSR4   = 3,
  VT_MT19937 = 4,
  VT_RANLUX  = 5 
}
VtRndGen;

// Module ID
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

// reason for writing interaction point
typedef enum
{	
  VT_NO_REASON =-1,
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

// directory type
typedef enum
{ 
  NO_DIR    =-1,
  PAR_DIR   = 0,
  INSTL_DIR = 1,
  IN_DIR    = 2,
  OUT_DIR   = 3
}
VtDirType;


// Axis
typedef enum
{
  X_AXIS = 'X',
  Y_AXIS = 'Y',
  Z_AXIS = 'Z'
} 
VtAxis;

// Orientation
typedef enum
{
  HORIZONTAL = 0,
  VERTICAL   = 1
} 
VtOrient;

// Frame generation
typedef enum
{
  VT_FRAME_STD  = 1,
  VT_FRAME_USER = 2
} 
VtFrameGen;

// Shape
typedef enum
{	
	VT_NO_SHAPE =-1,
	VT_SQUARE   = 0,
	VT_CIRCLE   = 1
}
VtShape;

// Component Status
typedef enum
{
  NOT_EXIST  = -1,  // module is completely ignored
  INACTIVE   =  0,  // module is replaced by space, i.e. instrument length is kept constant
  ACTIVE     =  1,  // module is treated normally
  FIRST_PART =  2,  // first of an array of parallel modules: lost neutrons are written with their input values, neutrons that passed are written with x > 0
  MIDDLE_PART=  3,  // module in between other parallel mod.: neutrons with x > 0 are ignored, lost neutrons are written with their input values, neutrons that passed are written with x > 0
  LAST_PART  =  4   // last of an array of parallel modules : neutrons with x > 0 are ignored, otherwise neutrons are treated normally
}
VtCompAct;

// Distribution function
typedef enum
{
  LORENTZIAN = 1, 
  GAUSSIAN   = 2
}
VtDistr;


// Source and Moderators
// ---------------------
// name of source
typedef enum
{
  ANYSOURCE=-1,
  ESS      = 1,
  SNS      = 2,
  ISIS     = 3,
  CSNS     = 4,
  IPNS     = 5,
  HBS      = 6,
  ILL      =10,
  HMI      =11,
  FRM2     =12
}
VtSrcName;

// type of source:  simple, ... pulsed ... (Vitess 4)
typedef enum
{
  NO_SRC_KIND=0,
  SRC_SIMPLE =1,
  SRC_CWS    =2,
  SRC_PULSED =3,
  SRC_ISIS   =4,
  SRC_ESS    =5
}
VtSrcKind;

// Type of source: CWS, SPSS, LPSS         (Vitess 3)
typedef enum
{
  NO_TYPE=0,
  CWS    =1,
  SPSS   =2,
  LPSS   =3
}
VtSrcType;

// Target station
typedef enum
{
  VT_NO_TS=0,
  VT_TS_1 =1,
  VT_TS_2 =2
}
VtTS;

typedef enum
{
  NO_MOD_TYPE=0,
  POISONED   =1,   /* moderator decoupled poisoned        */
  DECOUPLED  =2,   /* moderator decoupled unpoisoned      */
  COUPLED    =3,   /* moderator coupled                   */
  MULT_SPEC  =4    /* effective spectrum of a moderator consisting of a cold and thermal part      */
}
VtModType;

// window or moderator shape
typedef enum
{	
  VT_MOD_SQUARE = 'R',
	VT_MOD_CIRCLE = 'C'
}
VtModShape;

// definition of flight direction 
typedef enum
{	VT_DIVERGENCE = 0,
	VT_REAL_WND   = 1,
	VT_VIRT_WND   = 2,
}
VtDirect;

// ESS version
typedef enum
{
  NO_VERSION      =0,
  MEZEI_2001      =1,
  ZANINI_2012     =2,
  SCHOENFELDT_2013=3,
  VARHEIGHT_2013  =4,
  BUTTERFLY2_2015 =5,
  BUTTERFLY1_2016 =6
}
EssModVsn;


// Reading and Writing Trajectories
// --------------------------------
// tracing options
typedef enum
{	
  NO_TRACING     = 0,
  WRITE_TRC_FILES= 1,
  ONLY_TRC_TRAJ  = 2
}
VtTrace;

// data format of the program
typedef enum
{ VT_VITESS_FMT = 1,
  VT_MCSTAS_FMT = 2,
  VT_MCPL_FMT   = 3,
  VT_MCNPX_FMT  = 4,
  VT_MCNP6_FMT  = 5
}
VtPrgFormat;

// format used to store trajctories: float, exponential or binary
typedef enum
{ VT_EXPONENTIAL = 0,
  VT_FLOAT       = 1,
  VT_BINARY      = 2
}
VtDataFormat;

// choice of separator in trajectory table
typedef enum
{ VT_BLANK     = 0,
  VT_TABULATOR = 1
}
VtSeparator;


// Frame
// -----
typedef enum
{	
	VT_NO_SEQ= 0,
	VT_RTM   = 1,
	VT_RMT   = 2,
	VT_TRM   = 3,
	VT_TMR   = 4,
	VT_MTR   = 5,
	VT_MRT   = 6
}
VtTfmnSeq;


// Windows + Collimators
// ---------------------
// window absorber material
typedef enum
{	
  VT_WABS_FILE  = 0,
	VT_WABS_GD    = 1,
  VT_WABS_CD    = 2,
	VT_WABS_B10   = 3,
  VT_WABS_EU    = 4,
	VT_WABS_SI    = 5,
  VT_WABS_IDEAL = 6
}
VtWndAbs;

// oscillation (of the radial collimator)
typedef enum
{	
  VT_OSC_OFF   = 0,
	VT_RND_PHASE = 1,
}
VtOscill;

// Shape of multiple windows
typedef enum
{	
	VT_MWND_AUTO   = 0,
	VT_MWND_CIRCLE = 1,
	VT_MWND_SQUARE = 2
}
VtMultWndShape;


// Guides
// ------
// guide walls : top, bottom ... 
typedef enum
{ GW_TOP      = 0,
  GW_BOTTOM   = 1,
  GW_LEFT     = 2,
  GW_RIGHT    = 3,
  GW_EXIT     = 4,
  GW_INIT     = 5
}
VtGdeWall;

// guide shape
typedef enum
{ 
  VT_CONSTANT = 0,
  VT_LINEAR   = 1,
  VT_CURVED   = 2,
  VT_PARABOLIC= 3,
  VT_ELLIPTIC = 4,
  VT_FROM_FILE= 5,
  VT_LIN_CURV = 6,
}
VtGdeShape;

// waviness distribution
typedef enum
{	
	VT_WAVI_RECT  = 1,
	VT_WAVI_GAUSS = 2,
}
VtWaviDistr;

// reflection list parameter
typedef enum
{	
	VT_LIST_PASS    =  1,
	VT_LIST_PASS_LF = -1,
	VT_LIST_REFL    =  2,
	VT_LIST_REFL_LF = -2,
	VT_LIST_T1SR    =  3,
	VT_LIST_T1SR_LF = -3,
	VT_LIST_ALL     =  4,
	VT_LIST_ALL_LF  = -4,
}
VtListPar;

// additional output for reflection list
typedef enum
{	
	VT_LSTM_NO   = 0,
	VT_LSTM_YES  = 1,
	VT_LSTM_EDGE = 2,
}
VtListVbs;

// reflection plot parameter
typedef enum
{	
  KeyNone          =  0,
  iKeyMode         =  1,
  iKeyMode0        =  2,
  iKeyMode5        =  3,
  iKeyMode10       =  4,
  dKeyRefCount     =  5,
  dKeyRefCountY    =  6,
  dKeyRefCountZ    =  7,
  iKeyThisCollision=  8,
  dKeydegangular   =  9,
  dKeym            = 10,
  dKeyreflectivity = 11,
  dKeyDivY         = 12,
  dKeyDivZ         = 13,
  iKeyColor        = 14,
  dKeyTime         = 15,
  dKeyWavelength   = 16,
  dKeyProbability  = 17,
  dKeyPositionX    = 18,
  dKeyPositionY    = 19,
  dKeyPositionZ    = 20,
  dKeyVectorX      = 21,
  dKeyVectorY      = 22,
  dKeyVectorZ      = 23,
  dKeySpinX        = 24,
  dKeySpinY        = 25,
  dKeySpinZ        = 26
}
VtPlotPar;

// reflection plot filter
typedef enum
{	
	VT_PLOT_ALL  = 0,
	VT_PLOT_SCAT = 1,
	VT_PLOT_DIED = 2,
}
VtPlotFilt;


// Monochromator
// -------------
// monochromator arrangement
typedef enum
{
  SINGLE_CE     = 1,
  CE_ARRAY_CALC = 2,
  CE_ARRAY_FILE = 3
}
VtMonoArrange;

// monochromator geometry
typedef enum
{
  REFL_MONO   = 1,
  TRANSM_MONO = 2
}
VtMonoType;

// focusing options
typedef enum
{
  CONST_LMBD = 1,
  SPHERICAL  = 2,
  VERT_CYL   = 3,
  DBL_FOC    = 4
}
VtMonoFocus;


// Samples
// -------
// samnple geometry
typedef enum
{	
  VT_CUBE    = 1,
	VT_CYL     = 2,
	VT_SPHERE  = 3,
	VT_HOL_CYL = 4
}
VtSmplGeom;

// measuring mode (sample_reflectom)
typedef enum
{	
  VT_SAMPLE    = 1,
	VT_REFERENCE = 2,
}
VtMeasMode;


// Detector
// --------
// geometry
typedef enum
{
  VT_DET_CYL  = 1,
  VT_DET_FLAT = 2
}
VtDetGeom;

// type
typedef enum
{
  VT_DET_TUBE = 0,
  VT_DET_AREA = 1
}
VtDetType;

// tube shape
typedef enum
{	
	VT_TUBE_CIRCLE = 0,
	VT_TUBE_SQUARE = 1
}
VtTubeShape;

// module usage
typedef enum
{
  VT_DET_REAL = 0,
  VT_MON_ONLY = 1,
  VT_GRID_OFF = 2
}
VtDetUse;

// absorbing detector material
typedef enum
{
  VT_GAS_BF3   = 0,
  VT_GAS_HE3   = 1,
  VT_SOLID_B10 = 2,
  VT_SOLID_LI6 = 3,
  VT_ABS_OTHER = 5
}
VtDetAbs;


// Monitors
// --------
// monitor parameter for mon1 and monpol1
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
VtMon1Par;

// monitor parameter for monitor1D and monitor2D
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
VtMonPar;

// normalization options
typedef enum
{
  NO_NORM       = 0,   // no normalization
  NORM_BIN_SIZE = 1,   // normalization by bin size 
  NORM_REF_FILE = 2    // relative to reference file
}
VtMonNorm;

typedef enum
{
  VT_NOT_DEF =0,
  VT_LAMBDA  =1,
  VT_TIME    =2,
  VT_POS_Y   =3,
  VT_POS_Z   =4,
  VT_DIV_HOR =5,
  VT_DIV_VERT=6,
  VT_DIV_RAD =7,
  VT_ENERGY  =8
}
VtBrlPar;

typedef enum
{
  BRL_ABS    = 1,  // absolute brilliance
  BRL_TRANSF = 2,  // brilliance transfer, i.e. relative to reference file
  BRL_PCT    = 3   // brilliance within 1 percent DelLmbd/Lmbd
}
VtBrlNorm;

typedef enum
{
  NO_2D_FORMAT =-1,
  MATRIX       = 0,
  XYZ          = 1,
  MATR_CMPT    = 2,
  XYZ_CMPT     = 3
}
VtFormat2D;


// Tools
// -----
typedef enum
{	
	VT_REFL_STD = 1,
	VT_M_R_COL  = 2,
	VT_Q_R_COL  = 3,
	VT_SN_QUD   = 4,
	VT_PAR_IN   = 5,
}
VtInMod;


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
  VtSmplGeom   Type;
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
McnpxNeutron;

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
Mcnp6Neutron;

// choppers
// --------
typedef struct
{
	double  Pos;
	double  Left, Right;
	double  Bottom;
	double  Opening;
}
ChopperWindow;

typedef struct
{
	short          NumberOfWindows;
	CartesianPoint Centre;          /* centre of the chopper in the coordinate system of the beamline [cm] */
	double         Radius;          /* radius of the chopper  [cm] */
	double         Frequency;       /* rot.freq 2*pi*60*rpm  */
	double         Angle;           /* orientation of the center of beamline in the chopper system */
	ChopperWindow  *Window;
}
Chopper;


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


#endif

