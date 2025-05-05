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
#define THETA_NI    0.09894   // the old value 0.099138 deg corresponds to QC_NI=0.021743 1/Ang
#define QC_NI       0.0217
#define NEUTRON_ID  2112

#define TRUE 		    1
#define FALSE 		  0
#define MISSING 	 -1
#define UNUSED     -1

#define UP          1
#define DOWN        0

#define SPIN_UP     1
#define SPIN_UNDEF  0
#define SPIN_DOWN  -1

#define ON          1
#define OFF         0

#define NO  0
#define YES 1

#define DATE_STD   0
#define DATE_US    1

#define NN  0

#define VT_EOF -1

#define GUIDEFLIGHT 1

#define ANY_ORDER  -1
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

#define FREQ_B_RATIO         18.324717
#define FREQUENCY_FROM_FIELD(x)  ( 18.324717 * x ) /* rad*kHz from Gauss */

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
	MCN_TEMPLATE     =  50,
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
  MCN_PRISM        = 231,
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
	MCN_SCREEN       = 550,
	MCN_WRITEOUT     = 590,
	MCN_SMPL_EL_ISO  = 610,
	MCN_SMPL_INELAST = 620,
	MCN_SMPL_SNGL_X  = 630,
	MCN_SMPL_POWDER  = 640,
	MCN_SMPL_S_Q     = 650,
	MCN_SMPL_NXS     = 660,
	MCN_SMPL_SANS    = 670,
	MCN_SMPL_REFL    = 680,
  MCN_SMPL_NCRYSTAL= 690,
	MCN_FRAME        = 710,
	MCN_FILTER       = 720,
	MCN_FILTER2D     = 722,
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
	MCN_TOOL_GEN_HKL = 1065,
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
  MCN_SOURCE_VAE   = 1230,
}
McCompID;

// reason for writing interaction point
typedef enum
{	
  VT_NO_REASON  =-1,
  VT_CREATED    = 0,    // source
  VT_OUTSIDE    = 1,    // guide
  VT_OUT_OF_WND = 2,    // slit
  VT_PASSED     = 3,    // chopper, slit
  VT_ENTERED    = 4,    // guide, field, component
  VT_TRANSIT    = 5,    // from one guide segment to the next
  VT_TRANSMITTED= 6,    // mirror, window, channel wall
  VT_REFLECTED  = 7,    // guide or mirror surface
  VT_SCATTERED  = 8,    // sample
  VT_ABSORBED   = 9,    // chopper, guide, collimator
  VT_EXITED     =10,    // guide, field, component
  VT_DETECTED   =15,    // detector
  VT_FILTERED   =20,    // filter and others
  VT_NO_DATA    =99,    // window, bender etc.
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
  NO_ROT_AX= ' ',
  VT_ROT_X = 'X',
  VT_ROT_Y = 'Y',
  VT_ROT_Z = 'Z'
} 
VtRotAxis;

// Scattering axis
typedef enum
{
  NO_AXIS=-1,
  X_AXIS = 0,
  Y_AXIS = 1,
  Z_AXIS = 2,
} 
VtAxis;

// Orientation
typedef enum
{
  NO_ORIENT  =-1,
  HORIZONTAL = 0,
  VERTICAL   = 1
} 
VtOrient;

typedef enum
{	
  VT_NO_DIR = 0,
	VT_IN     = 1,
	VT_OUT    = 2,
	VT_INSIDE = 3,
	VT_BEYOND = 4
}
VtDir;

// Frame generation
typedef enum
{
  VT_NO_FRAME   =-1,
  VT_FRAME_STD  = 0,
  VT_FRAME_USER = 1
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

// Instrument geometry
typedef enum
{
  VT_NO_I_GEOM   =-1,
  VT_DIRECT_GEOM = 0,
  VT_INVERT_GEOM  = 1
}
VtInstGeom;


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

// random sampling
typedef enum
{
  NO_SAMPLING = 0,
  SAMPLING    = 1
}
VtSampling;

// data format of the program
typedef enum
{ VT_VITESS_FMT = 1,
  VT_MCSTAS_FMT = 2,
  VT_MCPL_FMT   = 3,
  VT_MCNPX_FMT  = 4,
  VT_MCNP6_FMT  = 5,
  VT_SSW_FMT = 6,
  VT_KDS_FMT = 7,
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
  VT_NO_MAT    = -1,
  VT_ABS_IDEAL =  0,
	VT_ABS_GD    =  1,
	VT_ABS_B10   =  2,
	VT_ABS_CD    =  3,
	VT_ABS_EU    =  4,
  VT_ABS_FILE  = 10
}
VtAbsMat;

// absorber material
typedef enum
{	
  VT_NO_WABS    =-1,
  VT_WABS_FILE  = 0,
	VT_WABS_GD    = 1,
  VT_WABS_CD    = 2,
	VT_WABS_B10   = 3,
  VT_WABS_EU    = 4,
	VT_WABS_SI    = 5,
	VT_WABS_VAC   = 6,
  VT_WABS_IDEAL = 99,
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

// mirror material
typedef enum
{	
	VT_NO_MIRR_MAT =-1,
  VT_MIRR_OTHER  = 0,
  VT_MIRR_SI     = 1,
  VT_MIRR_SAPPH  = 2,
  VT_MIRR_GLASS  = 3,
  VT_MIRR_B4C    = 4,
  VT_MIRR_VACUUM = 6
}
VtMirrMat;

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


// Monochromators and choppers
// ---------------------------
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

// monochromator movement
typedef enum
{
  VT_MONO_FIX = 0,    // no movement
  VT_MONO_ROT = 1,    // rotation about a vertical axis
  VT_MONO_PST = 2,    // rotation about a horizontal axis    
  VT_MONO_OSC = 3,    // horizontal oscillation (for Doppler shift)
}
VtMonoMove;

// focusing options
typedef enum
{
  NO_FOCUSING= 0,
  CONST_LMBD = 1,
  SPHERICAL  = 2,
  VERT_CYL   = 3,
  DBL_FOC    = 4
}
VtMonoFocus;

// shape of Fermi chopper channels
typedef enum
{	
  VT_NO_CHN_SHAPE=-1,
  VT_CHN_STR     = 0,
  VT_CHN_IDEAL   = 1,
  VT_CHN_CIRC    = 2
}
VtChnlShape;

// shape of Fermi chopper channels
typedef enum
{	
	VT_NO_FERMI_TYPE=0,
	VT_FERMI_STR    =1,
	VT_FERMI_CURV   =2
}
VtFermiType;

// Devices for polarisation
// ------------------------
/* source of polarization 
typedef enum
{
  VT_NO_POL_SRC  =-1,
  VT_POL_FR_FILE = 0,
  VT_POL_AS_FCT  = 1
}
VtPolSrc;*/

// Samples
// -------
// samnple geometry
typedef enum
{	
  VT_NO_GEOM = 0,
  VT_CUBE    = 1,
	VT_CYL     = 2,
	VT_SPHERE  = 3,
	VT_HOL_CYL = 4
}
VtSmplGeom;

/*
typedef enum
{
  VT_NO_PTCL       = ' ',
  VT_PTCL_SPHERE   = 'S',
  VT_PTCL_POLY_SPH = 'D',
  VT_PTCL_ELLIPS   = 'E',
  VT_PTCL_CYL      = 'C',
  VT_PTCL_EPIPED   = 'P'
  VT_ISOTROPIC     = 'I',
}
VtPtclGeom;*/
typedef enum
{
  VT_NO_PTCL       = 0,
  VT_PTCL_SPHERE   = 1,
  VT_PTCL_POLY_SPH = 2,
  VT_PTCL_ELLIPS   = 3,
  VT_PTCL_CYL      = 4,
  VT_PTCL_EPIPED   = 5,
  VT_ISOTROPIC     = 6
}
VtPtclGeom;

typedef enum
{
  VT_NO_SRC  = ' ',
  VT_FR_FILE = 'D',
  VT_AS_FCT  = 'F'
}
VtDataSrc;

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
  VT_NO_DET_GEOM=-1,
  VT_DET_CYL    = 1,
  VT_DET_FLAT   = 2
}
VtDetGeom;

// type
typedef enum
{
  VT_NO_DET_TYPE=-1,
  VT_DET_TUBE   = 0,
  VT_DET_AREA   = 1
}
VtDetType;

// tube shape
typedef enum
{	
  VT_NO_TUBE_SHAPE=-1,
	VT_TUBE_CIRCLE  = 0,
	VT_TUBE_SQUARE  = 1
}
VtTubeShape;

// module usage
typedef enum
{
  VT_NO_DET_USE=-1,
  VT_DET_REAL  = 0,
  VT_MON_ONLY  = 1,
  VT_GRID_OFF  = 2,
  VT_EFF_OFF   = 3
}
VtDetUse;

// absorbing detector material
typedef enum
{
  VT_NO_ABS_MAT=-1,
  VT_GAS_BF3   = 0,
  VT_GAS_HE3   = 1,
  VT_SOLID_B10 = 2,
  VT_SOLID_LI6 = 3,
  VT_ABS_OTHER = 5
}
VtDetAbs;


// Ensembles and Unions
// --------------------
// mirror data format
typedef enum
{	
  VT_MIRR_NO_FMT =-1,
  VT_MIRR_FMT_OLD= 0,
  VT_MIRR_FMT_NEW= 1,
  VT_MIRR_STL_CAD= 2,
}
VtMirrFormat;


// Monitors, Filter and Evaluation
// -------------------------------
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

// 2D monitor and filter parameter
typedef enum
{
  NO_MON2_PAR = 0,
  MON2_POS    = 1,
  MON2_DIV    = 2,
}
VtMon2Par;

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
  POS_THETA= 18,
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
  VT_ENERGY  =8,
  VT_POS_R   =9
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
  XYZ_CMPT     = 3,
  MATR_INT     = 4
}
VtFormat2D;

// combination of filter parameters
typedef enum
{
  NO_FCOMB   =-1,   
  OR_OR_OR   = 0,
  AND_AND_AND= 1,
  AND_OR_AND = 2,
}
VtFiltComb;

// evaluation parameter for eval_elast2
typedef enum
{
  VT_NO_EVAL   =0,
  VT_EVAL_DSP  =1,
  VT_EVAL_Q    =2,
  VT_EVAL_ANGLE=3,
  VT_EVAL_LMBD =4,
}
VtEvalPar;

// evaluation combination for eval_elast2
typedef enum
{
  VT_NO_ECOMB = 0,   
  VT_SCA_LMBD = 1,   // scattering angle and wavelength
  VT_SCA_TOF  = 2,   // scattering angle and TOF
}
VtEvalComb;

// sort mode for eval_elast2
typedef enum
{
  VT_NO_SORT    = 0,
  VT_SORT_X     = 1,
  VT_SORT_X_R   =-1,
  VT_SORT_Y     = 2,
  VT_SORT_Y_R   =-2,
  VT_SORT_INT   = 3,
  VT_SORT_INT_R =-3,
  VT_SORT_CTS   = 4,
  VT_SORT_CTS_R =-4,
}
VtEvalSort;

// angle selection mode for eval_elast2
typedef enum
{
  VT_NO_SEL  =-1,
  VT_SEL_DIR = 0,  // selection by direction
  VT_SEL_POS = 1,  // selection by position
}
VtAngleSel;


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
	char           sParID[2];    // e.g. "-Z" for '--Z', "A " for '-A' 
	unsigned long  iCompID;      // ID of the component/module
	char           cChange;      // 'C' for parameter change, 'R' for 'reset'
	short          iModuleNo;    // position in the row of modules in the pipe
	double         Value;        // double parameter value
	long           nValue;       // integer parameter value
  long           bInteger;
	double         Unused;
	VectorType     vUnused1;
	VectorType     vUnused2;
	VectorType     vUnused3;
}
ParChange;

typedef union
{
  Neutron   Traj;
  ParChange Trigger;
}
VtEvent;

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
  double         DirX;
  double         DirY;
  double         Surface;
}
Mcnp6Neutron;

typedef struct
{
  int  nBytes;
  char sID      [8];
  int  iNum1    [2];
  char sPrg     [8],
       sVsn     [5];
  char sDatePrg [9];
  char sDateEnd [9],
       sTimeEnd[10];
  char sDateBeg [9],
       sTimeBeg [9];
  char sTitle  [80];
  int  iNum2   [12];
  char sRest  [300];
}
Mcnp6Header;


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
  VectorType vCntr;     // Center of the prism
  VectorType vNormal;   // Normal vector for orientation
  VectorType vVertices[6];  // Vertices for two triangular faces (3 bottom + 3 top)
  double     PrismHeight;   // Distance between the triangular faces (vertical height of the prism)
  double     BaseWidth;     // Width of the triangular base (x direction)
  double     BaseHeight;    // Height of the triangular base (z direction)
  double     rotAngle;      // Rotation angle around the central axis
}
VtPrism;

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
  VtPrism*     pPrism;
  int          nPrisms;
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

