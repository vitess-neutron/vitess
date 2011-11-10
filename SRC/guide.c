/********************************************************************************************/
/*  VITESS module guide                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.xx   Sep 1999  D. Wechsler                                                             */
/*                                                                                          */
/*                  Rewritten by Manoshin Sergey Feb 2001 for include GRAVITY               */
/*                  Fixed some major bugs... Manoshin Sergey 28.02.01.                      */
/*                  Add key -g for gravity off or on                                        */
/*                  Add key -a for abutment error on or off                                 */
/*                  Add possibility for simulate guide with different coated matherial in   */
/*                  left and right and top and bottom planes of guide:                      */
/*                  first reflectivity file describe left plane of guide -i                 */
/*                  second reflectivity file describe right plane of guide -I               */
/*                  third reflectivity file describe top and bottom planes of guide -j      */
/* 2.3              add key -r for simulation the rough reflecting surface                  */
/*                                                                                          */
/* 2.4   Dec 2001  K. Lieutenant  adaption to changes in YTSDefs and wei_min as general     */
/*                                parameter, improvement in printing                        */
/* 2.5   Jan 2002  K. Lieutenant  reorganisation                                            */
/* 2.6   Apr 2003  K. Lieutenant  more precise calculation of curved guide; changed output  */
/*                                OwnInit() and other changes in style                      */
/* 2.6a  Jun 2003  K. Lieutenant  small corrections: output of waviness, free memory, 'maxi'*/
/* 2.7   Jul 2003  K. Lieutenant  correction: loss of trajectories by check 'previous coll.'*/
/*                                correction: wrong direction because of high waviness for  */
/*                                            straight guides                               */
/*                                condition: trajectory must end inside exit plane          */
/* 2.7a  Jan 2004  K. Lieutenant  4 different coatings                                      */
/* 2.7b  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                              */
/* 2.8   Jan 2004  K. Lieutenant  correction: wrong direction because of high waviness      */
/* 2.9   Feb 2004  K. Lieutenant  'FullParName'; 'message' included                         */
/* 2.10  Mar 2004  K. Lieutenant  parabolic and elliptic shape                              */
/* 2.11  Oct 2004  K. Lieutenant  curvature to the right by negative radius                 */
/* 2.12  May 2005  K. Lieutenant  elliptic shape by focus point                             */
/* 2.13  May 2008  K. Lieutenant  shape defined in file                                     */
/* 2.14  Oct 2008  K. Lieutenant  attenuation included                                      */
/* 2.15  Aug 2009  A. Houben      Simple calculation of the guide area                      */
/* 2.16  Aug 2009  A. Houben      Write out reflection parameters of each trajectory        */
/*                                (data is complementary to traceing and writeout)          */
/* 2.17  Sep 2009  A. Houben      Extended writeout of reflection parameters                */
/* 2.18  Sep 2009  A. Houben      Changes to shape defined by file & some minor things      */
/* 2.19  Oct 2009  A. Houben      Shape by file for nonequidistant planes & minor things    */
/*                                (introduced rounding of XYZ positions but left commented) */
/* 2.20  Dez 2009  A. Houben      -FROM FILE mode allows to give mirror filenames           */
/*                                -GuidePieces are managed by array of struct GuidePiece    */
/*                                -Mirror files are requested/loaded by GetReflFile and     */
/*                                 stored in array of structs. Filename is key for reuse.   */
/* 2.21  Jan 2010  A. Houben      Bin data with arbitrary parameters like x pos, m, ...     */
/* 2.22  Feb 2010  M. Fromme      helper threads                                            */
/* 2.23  Jun 2010  A. Houben      Additional guide planes by extra rotation of top/bottom or*/
/*                                 left/right planes by given angle a < 90deg around x axis */
/*                                 --> a = 45deg --> octagon shape; a = 60deg --> hexagon   */
/* 2.24  Oct 2010  K. Lieutenant  Gaussian waviness distribution, length of abutment loss   */
/********************************************************************************************/

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"
#include "string.h"
#include "threadHelper.h"

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);
double gsl_ran_gaussian(const gsl_rng * r, const double sigma);

//#define INDEX(x,y) (x*(nbinsX)+y)
#define INDEX(x,y) (x*(nbinsY)+y)


/******************************/
/** Structures and Enums     **/
/******************************/

/* GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT must be 0 to 3 */
typedef enum
{ GW_TOP      = 0,
  GW_BOTTOM   = 1,
  GW_LEFT     = 2,
  GW_RIGHT    = 3,
  //GW_EXIT     = 4,
  //GW_INIT     = 5,
}
eGuideWall;
int GW_EXIT = 4;
int GW_INIT = 5;

typedef struct
{
  Neutron    neutron;
  eGuideWall ThisCollision;
  double     degangular;
  double     m;
  double     reflectivity;
  double     DivY;
  double     DivZ;
  int        Mode;      /* 0 = Scattered, 5 = GW_EXIT (not saved), 10 = Died */
}
NeutronEx;

typedef struct
{
  double    X;
  double    Y;
  NeutronEx ndata;
  long      Counts;
  double    RefCount;   /* Counts the number of reflections. 
                           If the last reflection did not occure for any reason, 
                           RefCount is increased by one, and multiplied by -1 */
  double    RefCountY;  /* Number of reflection on horizontal guide planes */
  double    RefCountZ;  /* Number of reflection on vertical guide planes */
  int       Mode10;
  int       Mode5;
  int       Mode0;
  double    ProbSum;
} BINDATA;

typedef struct
{
  int       RefCount;   /* Counts the number of reflections. 
                           If the last reflection did not occure for any reason, 
                           RefCount is increased by one, and multiplied by -1 */
  int       RefCountY;  /* Number of reflection on horizontal guide planes */
  int       RefCountZ;  /* Number of reflection on vertical guide planes */
  char      *Output;    /* One line of text for each reflection */
  NeutronEx *neutrons;  /* List of neutron trajectory states */
  int       cneutrons;
}
ReflCond;

typedef struct
{
  //double  CriticalAngle;
  //double  CutoffAngle;
  //Plane	Wall[5];
  Plane* Wall;
}
NeutronGuide;

typedef enum
{ VT_CONSTANT = 0,
  VT_LINEAR   = 1,
  VT_CURVED   = 2,
  VT_PARABOLIC= 3,
  VT_ELLIPTIC = 4,
  VT_FROM_FILE= 5,
}
VtShape;

typedef struct
{
  FILE   *pfile;
  char   *filename;
  double *Rdata;
  long   maxdata;
  double area;
}
ReflFile;

typedef struct
{
  double Xpce, Ypce, Zpce,   /* list of x-pos., width and height at beginning and end of pieces */
         Wchan;              /* list of widths of channel at beginning and end of each piece */
  //ReflFile *RData[4];      /* use GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT */
  ReflFile **RData;          /* use GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT, etc */
}
GuidePiece;

/******************************/
/** Prototypes               **/
/******************************/

void   OwnInit   (int argc, char *argv[]);
void   OwnCleanup();
ReflFile *GetReflFile(char *Filename, FILE *file);
void   LoadReflFile(ReflFile *pReflFile);
double Height    (double length);
double Width     (double length);
double PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide *ThisGuide, double  wei_min,
           GuidePiece *Pce, double surfacerough, long keygrav, double AbutLen, ReflCond *RefOut, long iPiece);
void   WriteReflParam(ReflCond *RefOut, int Mode, Neutron *pNeutron, NeutronGuide *ThisGuide, GuidePiece *Pce, 
           eGuideWall ThisCollision, double degangular, double reflectivity);
void   PrintMaximalM(double *RData, long i);
int    FindIndexXY(double *Xval, double *Yval, int *ibinX, int *ibinY);
void   DoBin(ReflCond *RefOut);
//double GetValProb(const double *ValProb, const int Key);

typedef double(*GetVal)(ReflCond *RefOut, int cNeut);
GetVal SetValueFunction(const int key);
void GetKeyName(const int key, char* buf);

double (*GetValueX)(ReflCond *RefOut, int cNeut) = NULL;
double (*GetValueY)(ReflCond *RefOut, int cNeut) = NULL;
double (*GetProb  )(ReflCond *RefOut, int cNeut) = NULL;


/******************************/
/** Global variables         **/
/******************************/

short  keyabut  =0;           /* key for abutment loss 0: no  1: yes */
long   nPieces  =1,
       nChannels=1,
       nSpacers =0,
       nPlanes  =4;
short  AddToColor = 0;
int    keyReflParam = -1;     /* Trajectories to be written out:
                                 1 = only those leaving the guide;
                                 2 = all successfull reflections; no matter if the trajectory reaches the guide end
                                 3 = only those with at least one successful scattering event (tracjectory may end with an unsuccessfull event)
                                 4 = all
                                 a negative number adds a line feed between each trajectory */
int    keyPlotParam = 0;      /* Plot filter:
                                 0 = any
                                 1 = only scattered
                                 2 = only died*/
int    keyReflMinCnt = 0;     /* Minimum number of reflections within the guide for reflection list output */
int    keyReflMaxCnt = 0;     /* Maximum number of reflections within the guide for reflection list output */
int    keyReflMinCntY = 0;    /* Minimum number of reflections on the horizontal guide for reflection list output */
int    keyReflMaxCntY = 0;    /* Maximum number of reflections on the horizontal guide for reflection list output */
int    keyReflMinCntZ = 0;    /* Minimum number of reflections on the vertical guide for reflection list output */
int    keyReflMaxCntZ = 0;    /* Maximum number of reflections on the vertical guide for reflection list output */
int    keyReflVerbose = 0;    /* Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated. */
int    keyAddPlane = 0;       /* Additional planes: 0 = none, 1 = top/bottom, 2 = left/right */

double GuideEntranceHeight=0.0,
       GuideEntranceWidth=0.0,
       GuideExitHeight=0.0,
       GuideExitWidth=0.0,
       GuideMaxHeight=0.0,   /* max. height and width of guide for elliptic shape */
       GuideMaxWidth=0.0,
       FocusY=0.0,D_Foc1Y=0.0, /* pos. of focus points for elliptic shape in horizontal dir. */
       FocusZ=0.0,D_Foc1Z=0.0, /* pos. of focus points for elliptic shape in vertical dir.   */
       PhiAnfY =90.0,        /* phases for elliptic shape */
       PhiAnfZ =90.0,
       LcntrY  = 0.0,        /* centre positions of ellipse */
       LcntrZ  = 0.0,
       AxisY   = 0.0,        /* long axes of ellipse */
       AxisZ   = 0.0,
       AparY   = 0.0,        /* factor of quadratic term in parabola  */
       AparZ   = 0.0,
       Radius  = 0.0,
       piecelength=0.0,      /* length of 1 piece of the guide */
       dTotalLength,         /* total length of the guide  */
       dDeltaX, dDeltaY,     /* length in x- and y-direction of the total guide  */
       beta, beta_ges,       /* angle of declination between 2 pieces  and of the total guide */
       spacer  =0.0,
       AbutLen =0.0,         /* area around the connection of guide segments, where neutrons are absorbed */
       surfacerough=0.0,     /* parameter which characterizes the waviness of the guide surface */
       MuScat=0.0,           /* total macroscopic scattering coeff. in 1/cm */
       MuAbs =0.0,           /* macroscopic absorption coeff. in 1/cm */
       rotplane = 0.0;       /* Additional planes: rotation angle */
double AreaY=0., AreaZ=0.;   /* Approximate area of guide planes in cm**2 */
GuidePiece *pPieces;         /* Holds piece Informations. Replaces Xpce, Ypce, Zpce */

VtShape eGuideShapeY=1,      /* shape of guide in y- and z-direction */
        eGuideShapeZ=1;
VtDistr eWaviDistr=VT_RECTANGULAR;  /* shape of the waviness distribution */

const char  *ShapeFileName="guide_shape.dat";
char  *ReflParamFileName=NULL;
char  *ReflPlotFileName=NULL;
char  *ReflFileNameL=NULL;
char  *ReflFileNameR=NULL;
char  *ReflFileNameT=NULL;
char  *ReflFileNameB=NULL;

FILE  *pReflParam=NULL; /* file for writing each reflection */
FILE  *pReflPlot=NULL;  /* file for writing each reflection as plot */
FILE  *pReflL=NULL; /* file for describing left plane of guide */
FILE  *pReflR=NULL; /* file for describing right plane of guide */
FILE  *pReflT=NULL; /* file for describing top plane of guide */
FILE  *pReflB=NULL; /* file for describing bottom plane of guide */

/* Extended FROM FILE */
ReflFile *pReflFiles = {NULL};
long     cReflFiles  = 0;

/* bining keys */
#define KeyNone            0
#define iKeyMode           1
#define iKeyMode0          2
#define iKeyMode5          3
#define iKeyMode10         4
#define dKeyRefCount       5
#define dKeyRefCountY      6
#define dKeyRefCountZ      7
#define iKeyThisCollision  8
#define dKeydegangular     9
#define dKeym             10
#define dKeyreflectivity  11
#define dKeyDivY          12
#define dKeyDivZ          13
#define iKeyColor         14
#define dKeyTime          15
#define dKeyWavelength    16
#define dKeyProbability   17
#define dKeyPositionX     18
#define dKeyPositionY     19
#define dKeyPositionZ     20
#define dKeyVectorX       21
#define dKeyVectorY       22
#define dKeyVectorZ       23
#define dKeySpinX         24
#define dKeySpinY         25
#define dKeySpinZ         26

/* Reflection Plot */
long   nbinsX=1000, nbinsY=100;
double MinX=0., MaxX=10000., MinY=0., MaxY=10.;
double *bpostX = {NULL};                 /* limits of the bins                                           */
double *bpostY = {NULL};                 /* limits of the bins                                           */
BINDATA **bin = {NULL};
//#ifdef bn
BINDATA **binX = {NULL};
BINDATA **binY = {NULL};
//#endif
int    KeyX = dKeyPositionX;
int    KeyY = dKeym;
int    KeyProb = dKeyProbability;
double XpceZero = 0.;
/**********************/

/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  /********************************************************************************************/
  /* This module reads in a file of neutron structures, and defines a neutron guide as a set  */
  /* of five infinite planes with a global critical angle. It outputs the coordinates and time*/
  /* displacement of any neutrons that pass through the guide without being absorbed.			*/
  /*                                                                                          */
  /* Anything not directly commented is an InputNeutrons or an output routine.                        */
  /********************************************************************************************/
  long   i, j, k, kChan, cPlane=3;
  short  test;

  double pathlen,            /* total neutron pathlength in the guide              */
         dXpce,              /* length of a piece incl. diff. in y- or z- position */
         dDelY,  dDelZ,      /* difference in y- or z-position of a piece          */
         dDelYr, dDelYl,     /* difference in y-position of the left and right side of a piece resp. */
         Length1, Length2,   /* length of a piece incl. diff. in z- or y-position resp. */
         Length2r,Length2l,  /* length of a piece incl. diff. in y-position.
                                   for left and right side of a piece resp. */
         right_beg,left_beg, /* right and left position of the beginning of a channel of a piece */
         right_end,left_end, /* right and left position of the end of a channel of a piece */
         rot = 0.0;          /* rotation for AddPlanes */
  double dCosBetH = 1.0,
         dSinBetH = 0.0,     /* cos(beta/2) and sin(beta/2)            */
         TimeOF1, TimeOF2,
         RotMatrix[3][3]={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  double cx, sx, d = 0;

  NeutronGuide Guide;
  Neutron      Output;
  ReflCond     RefOut = {0};


  /* Initialisation */
  Init(argc, argv, VT_GUIDE);
  print_module_name("guide 2.24");
  OwnInit(argc, argv);

  /* planes */
  Guide.Wall = calloc(nPlanes+1, sizeof(Plane)); //one more for exit plane
  if (!Guide.Wall) { fprintf(LogFilePtr,"ERROR: Not enough memory for guide data!\n");
    exit(-1);
  }

  /* Writing to log file */
  fprintf(LogFilePtr, "\nTotal length of guide   : %8.3f  m\n", dTotalLength/100.);
  if (nChannels > 1)
    fprintf(LogFilePtr, " with %ld channels", nChannels);
  fprintf(LogFilePtr, "Width x Height          : %8.3f  x %7.3f cm²", GuideEntranceWidth, GuideEntranceHeight);
  if (GuideExitWidth != GuideEntranceWidth || GuideExitHeight != GuideEntranceHeight)
    fprintf(LogFilePtr, " -> %7.3f x %7.3f cm²", GuideExitWidth, GuideExitHeight);
  fprintf(LogFilePtr, "\n\nHorizontal: ");
  switch (eGuideShapeY)
  { case VT_ELLIPTIC:
      fprintf(LogFilePtr, "elliptic shape\n");
      fprintf(LogFilePtr, " maximal width     :%8.3f cm  at %8.2f m from entrance\n", GuideMaxWidth, LcntrY/100.);
      fprintf(LogFilePtr, " long half axis    :%8.3f m\n", AxisY/100.);
      fprintf(LogFilePtr, " focal points      :%8.3f m from entrance, %8.3f m after exit\n", D_Foc1Y/100., FocusY/100.);
      break;
    case VT_PARABOLIC:
      fprintf(LogFilePtr, "parabolic shape    : focal point:%8.3f m after exit\n", 
                          (sq(GuideEntranceWidth)*AparY-dTotalLength-1.0/AparY/16.0)/100.);
      break;
    case VT_CURVED  :
      fprintf(LogFilePtr, "curved guide\n");
      break;
    case VT_FROM_FILE:
      fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
      fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
      break;
    case VT_CONSTANT:
    case VT_LINEAR  :
      if      (GuideExitWidth > GuideEntranceWidth) fprintf(LogFilePtr, "linearly diverging\n");
      else if (GuideExitWidth < GuideEntranceWidth) fprintf(LogFilePtr, "linearly converging\n");
      else    fprintf(LogFilePtr, "constant width\n");
      break;
  }
  fprintf(LogFilePtr, " area (top+bottom) :%8.3f m²\n", AreaY*2./1e4);
  fprintf(LogFilePtr, "Vertical  : ");
  switch (eGuideShapeZ)
  { case VT_ELLIPTIC:
      fprintf(LogFilePtr, "elliptic shape\n");
      fprintf(LogFilePtr, " max. height       :%8.3f cm  at %8.2f m from entrance\n", GuideMaxHeight, LcntrZ/100.);
      fprintf(LogFilePtr, " long half axis    :%8.3f m\n", AxisZ/100.);
      fprintf(LogFilePtr, " focal points      :%8.3f m from entrance, %8.3f m after exit\n", D_Foc1Z/100., FocusZ/100.);
      break;
    case VT_PARABOLIC:
      fprintf(LogFilePtr, "parabolic shape    : focal point:%8.3f m after exit\n", 
                          (sq(GuideEntranceHeight)*AparZ-dTotalLength-1.0/AparZ/16.0)/100.);
      break;
    case VT_CURVED  :
      fprintf(LogFilePtr, "WARNING: vertically curved guide not supported\n"); 
      break;
    case VT_FROM_FILE:
      fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
      fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
      break;
    case VT_CONSTANT:
    case VT_LINEAR  :
      if      (GuideExitHeight > GuideEntranceHeight) fprintf(LogFilePtr, "linearly diverging\n");
      else if (GuideExitHeight < GuideEntranceHeight) fprintf(LogFilePtr, "linearly converging\n");
      else    fprintf(LogFilePtr, "constant height\n");
      break;
  default: ;
  }
  fprintf(LogFilePtr, " area (left+right) :%8.3f m²\n", AreaZ*2./1e4);

  if (Radius != 0.0)  /* curved guide */
  {	beta = 2.0*asin(piecelength/(2.0*Radius));
    dCosBetH = cos(beta/2.0);
    dSinBetH = sin(beta/2.0);
    FillRotMatrixZ(RotMatrix, beta);
    fprintf(LogFilePtr,"\n%ld kink(s) with an angle of %8.4f deg  each", nPieces-1, 180.0/M_PI*beta);
  }
  else
  {	beta = 0.0;
  }

  for(i=0; i < cReflFiles; i++)
  {
    if (pReflFiles[i].filename != NULL) {
      fprintf(LogFilePtr,"\nReflectivity file  : %s\n", pReflFiles[i].filename);
      if (pReflFiles[i].pfile != NULL) {
        PrintMaximalM(pReflFiles[i].Rdata, pReflFiles[i].maxdata);
      } else {
        fprintf(LogFilePtr,"WARNING: Case of zero reflectivity for this file! Most probably the file was not found!\n");
      }
      fprintf(LogFilePtr,  " surface area      :%8.3f m²\n", pReflFiles[i].area/1.e4);
    } else {
      break;
    }
  }

  if (keyAddPlane != 0)
    fprintf(LogFilePtr,"\nAdditional planes used. WARNING: All areas are calculated with rectangular cross section! \n");

  if (AbutLen > 0.0)
    fprintf(LogFilePtr,"\nAbutment loss area :%8.3f cm\n", AbutLen);

  if (surfacerough == 0.0)
  { fprintf(LogFilePtr,"The walls have no waviness \n");
  }
  else
  {	fprintf(LogFilePtr,"The walls have a waviness of %10.3e° ", atan(surfacerough)*180.0/M_PI);	
    if (eWaviDistr==VT_GAUSSIAN)
      fprintf(LogFilePtr,"rms Gaussian distribution \n");
    else
      fprintf(LogFilePtr,"max. rectangular distribution \n");
  }
  fflush(LogFilePtr);



  /****************************************************************************************/
  /* Set up the parameters of the planes from the input data...                           */
  /****************************************************************************************/

  dXpce = piecelength;
  dDelY = (GuideExitWidth  - GuideEntranceWidth)  / (2.0*nPieces);
  dDelZ = (GuideExitHeight - GuideEntranceHeight) / (2.0*nPieces);
  Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
  Length2 = sqrt(dXpce*dXpce+dDelY*dDelY);

  /* top plane */
  Guide.Wall[GW_TOP].A =  dDelZ/Length1;
  Guide.Wall[GW_TOP].B =  0.0;
  Guide.Wall[GW_TOP].C = -dXpce/Length1;
  Guide.Wall[GW_TOP].D = -Guide.Wall[GW_TOP].C * (GuideEntranceHeight/2.0);

  /* bottom plane */
  Guide.Wall[GW_BOTTOM].A = -dDelZ/Length1;
  Guide.Wall[GW_BOTTOM].B =  0.0;
  Guide.Wall[GW_BOTTOM].C = -dXpce/Length1;
  Guide.Wall[GW_BOTTOM].D =  Guide.Wall[GW_BOTTOM].C * (GuideEntranceHeight/2.0);

  /* left plane */
  Guide.Wall[GW_LEFT].A = -dDelY/Length2;
  Guide.Wall[GW_LEFT].B =  dXpce/Length2;
  Guide.Wall[GW_LEFT].C =  0.0;
  Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B * (GuideEntranceWidth/2.0);

  /* right plane */
  Guide.Wall[GW_RIGHT].A =  dDelY/Length2;
  Guide.Wall[GW_RIGHT].B =  dXpce/Length2;
  Guide.Wall[GW_RIGHT].C =  0.0;
  Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B * (GuideEntranceWidth/2.0);

  /* exit plane */
  Guide.Wall[GW_EXIT].A =  1.0;
  Guide.Wall[GW_EXIT].B =  0.0;
  Guide.Wall[GW_EXIT].C =  0.0;
  Guide.Wall[GW_EXIT].D = -dXpce;
  // A*x + B*y + C*z + D = 0

  /*****************************************************/

  DECLARE_ABORT

  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      test = TRUE;
      kChan = 0;
      TimeOF1 = 0.0;
      TimeOF2 = 0.0;

      /*	InputNeutrons[i].Position.X = 0.0;   !!!!!!!! */
      /****************************************************************************************/
      /* Check to see if the neutron is initially in the entrance to the guide...             */
      /****************************************************************************************/
      if (fabs(InputNeutrons[i].Position[1]) > GuideEntranceWidth/2.0)  continue;
      if (fabs(InputNeutrons[i].Position[2]) > GuideEntranceHeight/2.0) continue;

      if (dTotalLength == 0.0) goto zerolength;

      /************** start bender option *********************/
      if (nChannels > 1)
      {
        for (k=0; k < nChannels; k++)
        {
          right_beg = -GuideEntranceWidth/2.0 + k*(pPieces[0].Wchan + spacer);
          left_beg  = right_beg + pPieces[0].Wchan;

          if (   (right_beg < InputNeutrons[i].Position[1])
              && (left_beg  > InputNeutrons[i].Position[1]))
          {
            kChan = k;
            InputNeutrons[i].Color = (short)(k+1);
            /* left and right walls of guide exchanged by the channel walls             */
            /* for elliptical parabolic shape, this has to be calculated for each piece */
            if (eGuideShapeY!=VT_PARABOLIC && eGuideShapeY!=VT_ELLIPTIC && eGuideShapeY!=VT_FROM_FILE)
            {	right_end = -GuideExitWidth/2.0 + kChan*(pPieces[nPieces].Wchan + spacer);
              left_end  =  right_end + pPieces[nPieces].Wchan;
              dDelYr    =  right_end - right_beg;
              dDelYl    =  left_end  - left_beg;
              Length2r  =  sqrt(dXpce*dXpce+dDelYr*dDelYr);
              Length2l  =  sqrt(dXpce*dXpce+dDelYl*dDelYl);
              Guide.Wall[GW_LEFT].A = -dDelYl/Length2l;
              Guide.Wall[GW_LEFT].B =  dXpce /Length2l;
              Guide.Wall[GW_LEFT].D =  Guide.Wall[GW_LEFT].B*(-left_beg);
              Guide.Wall[GW_RIGHT].A = -dDelYr/Length2r;
              Guide.Wall[GW_RIGHT].B =  dXpce /Length2r;
              Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B*(-right_beg);
            }
            break;
          }
        }
        if(k==nChannels) continue;  /* neutron blocked by spacer */
      }
      /************** end bender option **************************/


      /****************************************************************************************/
      /* Pass a pointer to the neutron and the guide structure variable to a subroutine to do */
      /* the donkey work. The return value is the total length of the flight path through the */
      /* guide, or -1.0 if it missed all plates and the exit (should be impossible).          */
      /****************************************************************************************/
      RefOut.RefCount = 0;
      RefOut.RefCountY = 0;
      RefOut.RefCountZ = 0;
      if (RefOut.Output != NULL) free(RefOut.Output);
      RefOut.Output = NULL;
      if (RefOut.neutrons != NULL) free(RefOut.neutrons);
      RefOut.cneutrons = 0;
      RefOut.neutrons = NULL;
      for(j=0; j < nPieces; j++)
      {
        CHECK

        /* In case of several pieces:
           planes must be adjusted for each piece (depending on the guide shape) */
        if (nPieces > 1)
        {
          dXpce = pPieces[j+1].Xpce - pPieces[j].Xpce;

          switch (eGuideShapeY)
          {	case VT_CURVED:
              /* last piece has an output plane normal to the guide direction, the others are tilted  */
              if (j == nPieces-1)
              { Guide.Wall[GW_EXIT].A =  1.0;
                Guide.Wall[GW_EXIT].B =  0.0;
                Guide.Wall[GW_EXIT].D = -dXpce;
              }
              else
              { Guide.Wall[GW_EXIT].A =  dCosBetH;
                Guide.Wall[GW_EXIT].B =  dSinBetH;
                Guide.Wall[GW_EXIT].D = -Guide.Wall[GW_EXIT].A * dXpce;
              }
              break;

            case VT_LINEAR:
              /* left and right walls are moved  */
              Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B * pPieces[j].Ypce;
              Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B * pPieces[j].Ypce;
              break;

            case VT_PARABOLIC:
            case VT_ELLIPTIC:
            case VT_FROM_FILE:
              /* left and right walls are moved  */
              if (nChannels > 1)
              {	right_beg = -pPieces[j].Ypce   + kChan*(pPieces[j].Wchan + spacer);
                left_beg  =  right_beg + pPieces[j].Wchan;
                right_end = -pPieces[j+1].Ypce + kChan*(pPieces[j+1].Wchan + spacer);
                left_end  =  right_end + pPieces[j+1].Wchan;
                dDelYr    = right_end - right_beg;
                dDelYl    = left_end  - left_beg;
                Length2r  = sqrt(dXpce*dXpce+dDelYr*dDelYr);
                Length2l  = sqrt(dXpce*dXpce+dDelYl*dDelYl);
              }
              else
              {	right_beg = -pPieces[j].Ypce;
                left_beg  =  pPieces[j].Ypce;
                dDelYl    =  pPieces[j+1].Ypce-pPieces[j].Ypce;
                dDelYr    = -dDelYl;
                Length2l  = Length2r = sqrt(dXpce*dXpce+dDelYr*dDelYr);
              }
              Guide.Wall[GW_LEFT].A = -dDelYl/Length2l;
              Guide.Wall[GW_LEFT].B =  dXpce /Length2l;
              Guide.Wall[GW_RIGHT].A = -dDelYr/Length2r;
              Guide.Wall[GW_RIGHT].B =  dXpce /Length2r;

              Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B *   left_beg;
              Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B *(-right_beg);

              Guide.Wall[GW_EXIT].D = -dXpce;
              break;
            default:
              ;
          }

          switch (eGuideShapeZ)
          {
            case VT_PARABOLIC:
            case VT_ELLIPTIC:
            case VT_FROM_FILE:
              /* new shift is calculated to move walls */
              dDelZ   = pPieces[j+1].Zpce-pPieces[j].Zpce;
              Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
              Guide.Wall[GW_TOP].A =  dDelZ/Length1;
              Guide.Wall[GW_TOP].C = -dXpce/Length1;
              Guide.Wall[GW_BOTTOM].A = -dDelZ/Length1;
              Guide.Wall[GW_BOTTOM].C = -dXpce/Length1;
              /* no break at this point !!! */
            case VT_LINEAR:
              /* top and bottom walls are moved */
              Guide.Wall[GW_TOP].D = -Guide.Wall[GW_TOP].C * pPieces[j].Zpce;
              Guide.Wall[GW_BOTTOM].D =  Guide.Wall[GW_BOTTOM].C * pPieces[j].Zpce;
              break;
            default:
              ;
          }
        }

        if (nPlanes > 4) {
          rot = rotplane;
          cPlane = GW_RIGHT;
          while (rot < 90.0 && cPlane < GW_EXIT) {
            cx = cos(rot/180.*M_PI);
            sx = sin(rot/180.*M_PI);
            switch (keyAddPlane) {
              case 1:
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_TOP].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_TOP].B* cx + Guide.Wall[GW_TOP].C*-sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_TOP].B* sx + Guide.Wall[GW_TOP].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_TOP].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_TOP].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_TOP].B* cx + Guide.Wall[GW_TOP].C* sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_TOP].B*-sx + Guide.Wall[GW_TOP].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_TOP].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_BOTTOM].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_BOTTOM].B* cx + Guide.Wall[GW_BOTTOM].C*-sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_BOTTOM].B* sx + Guide.Wall[GW_BOTTOM].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_BOTTOM].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_BOTTOM].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_BOTTOM].B* cx + Guide.Wall[GW_BOTTOM].C* sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_BOTTOM].B*-sx + Guide.Wall[GW_BOTTOM].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_BOTTOM].D;
                break;
              case 2:
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_LEFT].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_LEFT].B* cx + Guide.Wall[GW_LEFT].C*-sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_LEFT].B* sx + Guide.Wall[GW_LEFT].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_LEFT].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_LEFT].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_LEFT].B* cx + Guide.Wall[GW_LEFT].C* sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_LEFT].B*-sx + Guide.Wall[GW_LEFT].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_LEFT].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_RIGHT].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_RIGHT].B* cx + Guide.Wall[GW_RIGHT].C*-sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_RIGHT].B* sx + Guide.Wall[GW_RIGHT].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_RIGHT].D;
                cPlane++;
                Guide.Wall[cPlane].A = Guide.Wall[GW_RIGHT].A;
                Guide.Wall[cPlane].B = Guide.Wall[GW_RIGHT].B* cx + Guide.Wall[GW_RIGHT].C* sx;
                Guide.Wall[cPlane].C = Guide.Wall[GW_RIGHT].B*-sx + Guide.Wall[GW_RIGHT].C* cx;
                Guide.Wall[cPlane].D = Guide.Wall[GW_RIGHT].D;
                break;
            }
            rot += rotplane;
          }

          /****************************************************************************************/
          /* Check to see if the neutron is initially in the entrance to the guide...             */
          /****************************************************************************************/
          if (j == 0) {
            for (k=0; k < GW_EXIT; k++) {
              d = (Guide.Wall[k].B*InputNeutrons[i].Position[1] + 
                 Guide.Wall[k].C*InputNeutrons[i].Position[2] + 
                 Guide.Wall[k].D)/Guide.Wall[k].D;
              if (d < 0) {
                break;
              }
            }
            if (d < 0) {
              test=FALSE;
              j=nPieces;
              continue;
            }
          }
        }

        //if (!test) continue;

        if (keyReflVerbose != 0 && j == 0)
          WriteReflParam(&RefOut, 5, &(InputNeutrons[i]), &Guide, &pPieces[j], GW_INIT, 0., 0.);
        TimeOF1 = PathThroughGuideGravOrder1(&(InputNeutrons[i]), &Guide, wei_min, &pPieces[j], surfacerough, keygrav, AbutLen, &RefOut, j);
        if (keyReflVerbose == 2 && j == nPieces-1)
          WriteReflParam(&RefOut, 5, &(InputNeutrons[i]), &Guide, &pPieces[j], GW_EXIT, 0., 0.);
  			
        if (TimeOF1 == -1.0) /* trajectory is lost */
        {	test=FALSE;
          j=nPieces;
          continue;
        }

        /****************************************************************************************/
        /* Update the coordinates.                                                              */
        /****************************************************************************************/

        InputNeutrons[i].Position[0] -= dXpce;

        /* For curved guide: frame rotated for next piece, but not after last piece */
        if (Radius != 0.0 && j < nPieces-1)
        {
          /* horizontal position and flight direction adjusted */
          RotVector(RotMatrix, InputNeutrons[i].Position);
          RotVector(RotMatrix, InputNeutrons[i].Vector);
          RotVector(RotMatrix, InputNeutrons[i].Spin);
        }
        TimeOF2 += TimeOF1;
      }
  		
      if (pReflParam != NULL)
      {
        if ((abs(RefOut.RefCount) >= keyReflMinCnt && (abs(RefOut.RefCount) <= keyReflMaxCnt || keyReflMaxCnt == 0)) &&
          (RefOut.RefCountY >= keyReflMinCntY && (RefOut.RefCountY <= keyReflMaxCntY || keyReflMaxCntY == 0)) &&
          (RefOut.RefCountZ >= keyReflMinCntZ && (RefOut.RefCountZ <= keyReflMaxCntZ || keyReflMaxCntZ == 0)) &&
          RefOut.Output != NULL)
        {
          switch (abs(keyReflParam))
          {
          case 1:
            if (RefOut.RefCount > 0) {
              fprintf(pReflParam, "%s", RefOut.Output);
              if (keyReflParam < 0) fprintf(pReflParam, "\n");
            }
            break;
          case 2: //Difference beetween 2 and 3 is created in WriteReflParam
            if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
              fprintf(pReflParam, "%s", RefOut.Output);
              if (keyReflParam < 0) fprintf(pReflParam, "\n");
            }
            break;
          case 3:
            if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
              fprintf(pReflParam, "%s", RefOut.Output);
              if (keyReflParam < 0) fprintf(pReflParam, "\n");
            }
            break;
          case 4:
            fprintf(pReflParam, "%s", RefOut.Output);
            if (keyReflParam < 0) fprintf(pReflParam, "\n");
            break;
          }
        }
      }
      if (pReflPlot != NULL && RefOut.neutrons != NULL) {
        if ((abs(RefOut.RefCount) >= keyReflMinCnt && (abs(RefOut.RefCount) <= keyReflMaxCnt || keyReflMaxCnt == 0)) &&
          (RefOut.RefCountY >= keyReflMinCntY && (RefOut.RefCountY <= keyReflMaxCntY || keyReflMaxCntY == 0)) &&
          (RefOut.RefCountZ >= keyReflMinCntZ && (RefOut.RefCountZ <= keyReflMaxCntZ || keyReflMaxCntZ == 0)))
        {
          switch (abs(keyReflParam))
          {
          case 1:
            if (RefOut.RefCount > 0) {
              DoBin(&RefOut);
            }
            break;
          case 2:
            if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
              DoBin(&RefOut);
            }
            break;
          case 3:
            if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
              DoBin(&RefOut);
            }
            break;
          case 4:
            DoBin(&RefOut);
            break;
          }
        }
      }
      if (test==FALSE) continue;

      if (fabs(InputNeutrons[i].Position[1]) > 0.5*GuideExitWidth ||
          fabs(InputNeutrons[i].Position[2]) > 0.5*GuideExitHeight)
      {
        CountMessageID(GUID_OUT_OF_EXIT, InputNeutrons[i].ID);
        continue;
      }

  zerolength:
      /****************************************************************************************/
      /* Add the time needed to travel all guide and writeout this trajectory                 */
      /****************************************************************************************/
      Output = InputNeutrons[i];

      pathlen = V_FROM_LAMBDA(Output.Wavelength)*TimeOF2;

      Output.Position[0]=0.0;
      Output.Time += TimeOF2;
      Output.Probability*=exp(-(MuScat+MuAbs*Output.Wavelength/1.798)*pathlen);

      WriteNeutron(&Output);
    }
  } //ReadNeutrons

  my_exit:
  // Output of Results
  fflush(LogFilePtr);

  // bin data 
  if (pReflPlot != NULL)
  {
    int ibinXY, ibinX, ibinY, cout;
    //fprintf(pReflPlot, "#   X          Y        counts   Mode  0   5   10  RefCount RCy RCz  ____ID____ plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc  color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z  WeightSum\n"
	    //           "#   1          2          3      4=A  4:A      5=A 6=A  6:1N       7:1N  8:A       9:A   10:A           11:A     12:A  13:1N  14:A   15:A   16:A     17:S           18:A       19:A       20:A       21:A      22:A      23:A      24:A 25:A 26:A\n"
    const char *fstr="%10.4f %10.4f %10d %5.1f %7d %7d %7d %5.2f %5.2f %5.2f %c%c%09lu %3d   %8.5f %6.2f %12.5f %8.4f %8.4f  %c %5.2f  %7.3f %8.5f %11.3e  %10.4f %10.4f %10.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f %11.3e\n";
    char buf[3][40];

    memset(buf, 0, 3*40); // clean initialisation

    GetKeyName(KeyX, buf[0]);
    GetKeyName(KeyY, buf[1]);
    GetKeyName(KeyProb, buf[2]);
    fprintf(pReflPlot, "#BinX:%s   BinY:%s   Weight:%s\n", buf[0], buf[1], buf[2]);
    fprintf(pReflPlot, "#==Data==\n");
    
    
    
    //for (ibinXY = 0; ibinXY < INDEX(nbinsX, nbinsY); ibinXY++)
    for (ibinX = 0; ibinX < nbinsX; ibinX++)
    {
      cout = 0;
      for (ibinY = 0; ibinY < nbinsY; ibinY++)
      {
        ibinXY = INDEX(ibinX, ibinY);
        if (bin[ibinXY] != NULL)
        {// Generate averages
          if (bin[ibinXY]->ProbSum > 0.) {
            bin[ibinXY]->ndata.degangular          /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.m                   /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.reflectivity        /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.DivY                /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.DivZ                /= bin[ibinXY]->ProbSum;
            //bin[ibinXY]->ndata.Mode                /= bin[ibinXY]->ProbSum; //see below
            //bin[ibinXY]->ndata.neutron.Color       /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Time        /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Wavelength  /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Position[0] /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Position[1] /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Position[2] /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Vector[0]   /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Vector[1]   /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Vector[2]   /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Spin[0]     /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Spin[1]     /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->ndata.neutron.Spin[2]     /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->RefCount                  /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->RefCountY                 /= bin[ibinXY]->ProbSum;
            bin[ibinXY]->RefCountZ                 /= bin[ibinXY]->ProbSum;

            cout++;
            fprintf(pReflPlot, fstr, 
              bin[ibinXY]->X                        , bin[ibinXY]->Y                        , bin[ibinXY]->Counts, 
              ((double)(bin[ibinXY]->ndata.Mode)/bin[ibinXY]->ProbSum), 
              bin[ibinXY]->Mode0                    , bin[ibinXY]->Mode5                    , bin[ibinXY]->Mode10                   ,
              bin[ibinXY]->RefCount                 , bin[ibinXY]->RefCountY                , bin[ibinXY]->RefCountZ,
              bin[ibinXY]->ndata.neutron.ID.IDGrp[0], bin[ibinXY]->ndata.neutron.ID.IDGrp[1], bin[ibinXY]->ndata.neutron.ID.IDNo,
              bin[ibinXY]->ndata.ThisCollision      , bin[ibinXY]->ndata.degangular         , bin[ibinXY]->ndata.m, 
              bin[ibinXY]->ndata.reflectivity       , bin[ibinXY]->ndata.DivY               , bin[ibinXY]->ndata.DivZ,
              bin[ibinXY]->ndata.neutron.Debug      , 
              ((double)bin[ibinXY]->ndata.neutron.Color)/bin[ibinXY]->ProbSum,
              bin[ibinXY]->ndata.neutron.Time       , bin[ibinXY]->ndata.neutron.Wavelength , bin[ibinXY]->ndata.neutron.Probability,
              bin[ibinXY]->ndata.neutron.Position[0], bin[ibinXY]->ndata.neutron.Position[1], bin[ibinXY]->ndata.neutron.Position[2],
              bin[ibinXY]->ndata.neutron.Vector[0]  , bin[ibinXY]->ndata.neutron.Vector[1]  , bin[ibinXY]->ndata.neutron.Vector[2],
              bin[ibinXY]->ndata.neutron.Spin[0]    , bin[ibinXY]->ndata.neutron.Spin[1]    , bin[ibinXY]->ndata.neutron.Spin[2],
              bin[ibinXY]->ProbSum
              );
          }
          //free(bin[ibinXY]);
          //bin[ibinXY] = NULL;
        }
      }
      if (keyReflParam<0 && cout>0) fprintf(pReflPlot,"\n");
    }

//#ifdef bn
    fprintf(pReflPlot, "\n#==XData==\n");
    for (ibinX = 0; ibinX < nbinsX; ibinX++)
    {
      //cout = 0;
      if (binX[ibinX] != NULL)
      {// Generate averages
        if (binX[ibinX]->ProbSum > 0.) {
          binX[ibinX]->ndata.degangular          /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.m                   /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.reflectivity        /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.DivY                /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.DivZ                /= binX[ibinX]->ProbSum;
          //binX[ibinX]->ndata.Mode                /= binX[ibinX]->ProbSum; //see below
          //binX[ibinX]->ndata.neutron.Color       /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Time        /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Wavelength  /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Position[0] /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Position[1] /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Position[2] /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Vector[0]   /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Vector[1]   /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Vector[2]   /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Spin[0]     /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Spin[1]     /= binX[ibinX]->ProbSum;
          binX[ibinX]->ndata.neutron.Spin[2]     /= binX[ibinX]->ProbSum;
          binX[ibinX]->RefCount                  /= binX[ibinX]->ProbSum;
          binX[ibinX]->RefCountY                 /= binX[ibinX]->ProbSum;
          binX[ibinX]->RefCountZ                 /= binX[ibinX]->ProbSum;

          //cout++;
          fprintf(pReflPlot, fstr, 
            binX[ibinX]->X                        , 0.                                    , binX[ibinX]->Counts, 
            ((double)(binX[ibinX]->ndata.Mode)/binX[ibinX]->ProbSum), 
            binX[ibinX]->Mode0                    , binX[ibinX]->Mode5                    , binX[ibinX]->Mode10                   ,
            binX[ibinX]->RefCount                 , binX[ibinX]->RefCountY                , binX[ibinX]->RefCountZ,
            binX[ibinX]->ndata.neutron.ID.IDGrp[0], binX[ibinX]->ndata.neutron.ID.IDGrp[1], binX[ibinX]->ndata.neutron.ID.IDNo,
            binX[ibinX]->ndata.ThisCollision      , binX[ibinX]->ndata.degangular         , binX[ibinX]->ndata.m, 
            binX[ibinX]->ndata.reflectivity       , binX[ibinX]->ndata.DivY               , binX[ibinX]->ndata.DivZ,
            binX[ibinX]->ndata.neutron.Debug      , 
            ((double)binX[ibinX]->ndata.neutron.Color)/binX[ibinX]->ProbSum,
            binX[ibinX]->ndata.neutron.Time       , binX[ibinX]->ndata.neutron.Wavelength , binX[ibinX]->ndata.neutron.Probability,
            binX[ibinX]->ndata.neutron.Position[0], binX[ibinX]->ndata.neutron.Position[1], binX[ibinX]->ndata.neutron.Position[2],
            binX[ibinX]->ndata.neutron.Vector[0]  , binX[ibinX]->ndata.neutron.Vector[1]  , binX[ibinX]->ndata.neutron.Vector[2],
            binX[ibinX]->ndata.neutron.Spin[0]    , binX[ibinX]->ndata.neutron.Spin[1]    , binX[ibinX]->ndata.neutron.Spin[2],
            binX[ibinX]->ProbSum
            );
        }
      }
      //if (keyReflParam<0 && cout>0) fprintf(pReflPlot,"\n");
    }

    if (keyReflParam<0) fprintf(pReflPlot,"\n");
    fprintf(pReflPlot, "\n#==YData==\n");
    for (ibinY = 0; ibinY < nbinsY; ibinY++)
    {
      //cout = 0;
      if (binY[ibinY] != NULL)
      {// Generate averages
        if (binY[ibinY]->ProbSum > 0.) {
          binY[ibinY]->ndata.degangular          /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.m                   /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.reflectivity        /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.DivY                /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.DivZ                /= binY[ibinY]->ProbSum;
          //binY[ibinY]->ndata.Mode                /= binY[ibinY]->ProbSum; //see below
          //binY[ibinY]->ndata.neutron.Color       /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Time        /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Wavelength  /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Position[0] /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Position[1] /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Position[2] /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Vector[0]   /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Vector[1]   /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Vector[2]   /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Spin[0]     /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Spin[1]     /= binY[ibinY]->ProbSum;
          binY[ibinY]->ndata.neutron.Spin[2]     /= binY[ibinY]->ProbSum;
          binY[ibinY]->RefCount                  /= binY[ibinY]->ProbSum;
          binY[ibinY]->RefCountY                 /= binY[ibinY]->ProbSum;
          binY[ibinY]->RefCountZ                 /= binY[ibinY]->ProbSum;

          //cout++;
          fprintf(pReflPlot, fstr, 
            0.                                    , binY[ibinY]->Y                        , binY[ibinY]->Counts, 
            ((double)(binY[ibinY]->ndata.Mode)/binY[ibinY]->ProbSum), 
            binY[ibinY]->Mode0                    , binY[ibinY]->Mode5                    , binY[ibinY]->Mode10                   ,
            binY[ibinY]->RefCount                 , binY[ibinY]->RefCountY                , binY[ibinY]->RefCountZ,
            binY[ibinY]->ndata.neutron.ID.IDGrp[0], binY[ibinY]->ndata.neutron.ID.IDGrp[1], binY[ibinY]->ndata.neutron.ID.IDNo,
            binY[ibinY]->ndata.ThisCollision      , binY[ibinY]->ndata.degangular         , binY[ibinY]->ndata.m, 
            binY[ibinY]->ndata.reflectivity       , binY[ibinY]->ndata.DivY               , binY[ibinY]->ndata.DivZ,
            binY[ibinY]->ndata.neutron.Debug      , 
            ((double)binY[ibinY]->ndata.neutron.Color)/binY[ibinY]->ProbSum,
            binY[ibinY]->ndata.neutron.Time       , binY[ibinY]->ndata.neutron.Wavelength , binY[ibinY]->ndata.neutron.Probability,
            binY[ibinY]->ndata.neutron.Position[0], binY[ibinY]->ndata.neutron.Position[1], binY[ibinY]->ndata.neutron.Position[2],
            binY[ibinY]->ndata.neutron.Vector[0]  , binY[ibinY]->ndata.neutron.Vector[1]  , binY[ibinY]->ndata.neutron.Vector[2],
            binY[ibinY]->ndata.neutron.Spin[0]    , binY[ibinY]->ndata.neutron.Spin[1]    , binY[ibinY]->ndata.neutron.Spin[2],
            binY[ibinY]->ProbSum
            );
        }
      }
      //if (keyReflParam<0 && cout>0) fprintf(pReflPlot,"\n");
    }
//#endif

    /*if (bin != NULL) {
      free(bin);
      bin = NULL;
    }*/
  }

  //if (RefOut.Output != NULL) free(RefOut.Output);
  //if (RefOut.neutrons != NULL) free(RefOut.neutrons);
  OwnCleanup();
  Cleanup(sqrt(sq(dTotalLength)-sq(dDeltaY)),dDeltaY,0.0, beta_ges, 0.0);

  return(0);
}

void   DoBin(ReflCond *RefOut)
{
  int ibinX, ibinY, ibinXY, cNeut;
  double ValX, ValY, ValProb;

  for (cNeut = 0; cNeut < RefOut->cneutrons; cNeut++) {
    //ibinXY = FindIndexXY(&RefOut->neutrons[cNeut].neutron.Position[0], &RefOut->neutrons[cNeut].m, &ibinX, &ibinY);
    ValX = GetValueX(RefOut, cNeut);
    ValY = GetValueY(RefOut, cNeut);
    ValProb = GetProb(RefOut, cNeut);
    ibinXY = FindIndexXY(&ValX, &ValY, &ibinX, &ibinY);
    /*if (RefOut->neutrons[cNeut].Mode == 5)
    {
      ibinXY = ibinXY;
    }*/
    if (ibinXY >= 0)
    {
      if (bin[ibinXY] == NULL)
      {
        bin[ibinXY] = (BINDATA *)malloc(sizeof(BINDATA));
        memset(bin[ibinXY], 0, sizeof(BINDATA));
        /*if (bLogBinningX)
          bin[ibinXY]->X = sqrt((bpostX[ibinX])*(bpostX[ibinX+1]));
        else*/
          bin[ibinXY]->X = ((bpostX[ibinX])+(bpostX[ibinX+1]))/2.0;
        /*if (bLogBinningY)
          bin[ibinXY]->Y = sqrt((bpostY[ibinY])*(bpostY[ibinY+1]));
        else*/
          bin[ibinXY]->Y = ((bpostY[ibinY])+(bpostY[ibinY+1]))/2.0;
    		
        //memcpy(&bin[ibinXY]->ndata, &RefOut->neutrons[cNeut], sizeof(NeutronEx));
        memcpy(&bin[ibinXY]->ndata.neutron.ID, &RefOut->neutrons[cNeut].neutron.ID, sizeof(TotalID));
        bin[ibinXY]->ndata.neutron.Debug = RefOut->neutrons[cNeut].neutron.Debug;
        bin[ibinXY]->ndata.ThisCollision = RefOut->neutrons[cNeut].ThisCollision;
      } //else { //sum up, avarage will be generated by division through counts
        //bin[ibinXY]->ndata.degangular          += GetValProb(&ValProb, dKeydegangular) * RefOut->neutrons[cNeut].degangular;
        bin[ibinXY]->ndata.degangular          += ValProb * RefOut->neutrons[cNeut].degangular;
        bin[ibinXY]->ndata.m                   += ValProb * RefOut->neutrons[cNeut].m;
        bin[ibinXY]->ndata.reflectivity        += ValProb * RefOut->neutrons[cNeut].reflectivity;
        bin[ibinXY]->ndata.DivY                += ValProb * RefOut->neutrons[cNeut].DivY;
        bin[ibinXY]->ndata.DivZ                += ValProb * RefOut->neutrons[cNeut].DivZ;
        bin[ibinXY]->ndata.Mode                += (int)(ValProb * RefOut->neutrons[cNeut].Mode);
        bin[ibinXY]->ndata.neutron.Color       += (short)ValProb * RefOut->neutrons[cNeut].neutron.Color;
        bin[ibinXY]->ndata.neutron.Time        += ValProb * RefOut->neutrons[cNeut].neutron.Time;
        bin[ibinXY]->ndata.neutron.Wavelength  += ValProb * RefOut->neutrons[cNeut].neutron.Wavelength;
        bin[ibinXY]->ndata.neutron.Probability += ValProb * RefOut->neutrons[cNeut].neutron.Probability;
        bin[ibinXY]->ndata.neutron.Position[0] += ValProb * RefOut->neutrons[cNeut].neutron.Position[0];
        bin[ibinXY]->ndata.neutron.Position[1] += ValProb * RefOut->neutrons[cNeut].neutron.Position[1];
        bin[ibinXY]->ndata.neutron.Position[2] += ValProb * RefOut->neutrons[cNeut].neutron.Position[2];
        bin[ibinXY]->ndata.neutron.Vector[0]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[0];
        bin[ibinXY]->ndata.neutron.Vector[1]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[1];
        bin[ibinXY]->ndata.neutron.Vector[2]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[2];
        bin[ibinXY]->ndata.neutron.Spin[0]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[0];
        bin[ibinXY]->ndata.neutron.Spin[1]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[1];
        bin[ibinXY]->ndata.neutron.Spin[2]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[2];
      //}
      bin[ibinXY]->ProbSum   += ValProb;
      bin[ibinXY]->RefCount  += ValProb * abs(RefOut->RefCount);
      bin[ibinXY]->RefCountY += ValProb * RefOut->RefCountY;
      bin[ibinXY]->RefCountZ += ValProb * RefOut->RefCountZ;
      switch (RefOut->neutrons[cNeut].Mode) {
        case 0:
          bin[ibinXY]->Mode0++;
          break;
        case 5:
          bin[ibinXY]->Mode5++;
          break;
        case 10:
          bin[ibinXY]->Mode10++;
          break;
      }

      bin[ibinXY]->Counts++;
      //bin[ibinXY]->Int += prob;

      //bintc += prob;
    //}

//#ifdef bn
      //binX
      if (ibinX >= 0 && ibinX <= nbinsX)
      {
        if (binX[ibinX] == NULL)
        {
          binX[ibinX] = (BINDATA *)malloc(sizeof(BINDATA));
          memset(binX[ibinX], 0, sizeof(BINDATA));
          /*if (bLogBinningX)
            bin[ibinXY].X = sqrt((bpostX[ibinX])*(bpostX[ibinX+1]));
          else*/
            binX[ibinX]->X = ((bpostX[ibinX])+(bpostX[ibinX+1]))/2.0;
      		
          //memcpy(&bin[ibinXY]->ndata, &RefOut->neutrons[cNeut], sizeof(NeutronEx));
          memcpy(&binX[ibinX]->ndata.neutron.ID, &RefOut->neutrons[cNeut].neutron.ID, sizeof(TotalID));
          binX[ibinX]->ndata.neutron.Debug = RefOut->neutrons[cNeut].neutron.Debug;
          binX[ibinX]->ndata.ThisCollision = RefOut->neutrons[cNeut].ThisCollision;
        } //else { //sum up, avarage will be generated by division through counts
          //bin[ibinXY].ndata.degangular          += GetValProb(&ValProb, dKeydegangular) * RefOut->neutrons[cNeut].degangular;
          binX[ibinX]->ndata.degangular          += ValProb * RefOut->neutrons[cNeut].degangular;
          binX[ibinX]->ndata.m                   += ValProb * RefOut->neutrons[cNeut].m;
          binX[ibinX]->ndata.reflectivity        += ValProb * RefOut->neutrons[cNeut].reflectivity;
          binX[ibinX]->ndata.DivY                += ValProb * RefOut->neutrons[cNeut].DivY;
          binX[ibinX]->ndata.DivZ                += ValProb * RefOut->neutrons[cNeut].DivZ;
          binX[ibinX]->ndata.Mode                += (int)(ValProb * RefOut->neutrons[cNeut].Mode);
          binX[ibinX]->ndata.neutron.Color       += (short)ValProb * RefOut->neutrons[cNeut].neutron.Color;
          binX[ibinX]->ndata.neutron.Time        += ValProb * RefOut->neutrons[cNeut].neutron.Time;
          binX[ibinX]->ndata.neutron.Wavelength  += ValProb * RefOut->neutrons[cNeut].neutron.Wavelength;
          binX[ibinX]->ndata.neutron.Probability += ValProb * RefOut->neutrons[cNeut].neutron.Probability;
          binX[ibinX]->ndata.neutron.Position[0] += ValProb * RefOut->neutrons[cNeut].neutron.Position[0];
          binX[ibinX]->ndata.neutron.Position[1] += ValProb * RefOut->neutrons[cNeut].neutron.Position[1];
          binX[ibinX]->ndata.neutron.Position[2] += ValProb * RefOut->neutrons[cNeut].neutron.Position[2];
          binX[ibinX]->ndata.neutron.Vector[0]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[0];
          binX[ibinX]->ndata.neutron.Vector[1]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[1];
          binX[ibinX]->ndata.neutron.Vector[2]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[2];
          binX[ibinX]->ndata.neutron.Spin[0]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[0];
          binX[ibinX]->ndata.neutron.Spin[1]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[1];
          binX[ibinX]->ndata.neutron.Spin[2]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[2];
        //}
        binX[ibinX]->ProbSum   += ValProb;
        binX[ibinX]->RefCount  += ValProb * abs(RefOut->RefCount);
        binX[ibinX]->RefCountY += ValProb * RefOut->RefCountY;
        binX[ibinX]->RefCountZ += ValProb * RefOut->RefCountZ;
        switch (RefOut->neutrons[cNeut].Mode) {
          case 0:
            binX[ibinX]->Mode0++;
            break;
          case 5:
            binX[ibinX]->Mode5++;
            break;
          case 10:
            binX[ibinX]->Mode10++;
            break;
        }

        binX[ibinX]->Counts++;
      }

      //binY
      if (ibinY >= 0 && ibinY <= nbinsY)
      {
        if (binY[ibinY] == NULL)
        {
          binY[ibinY] = (BINDATA *)malloc(sizeof(BINDATA));
          memset(binY[ibinY], 0, sizeof(BINDATA));
          /*if (bLogBinningY)
            bin[ibinXY].Y = sqrt((bpostY[ibinY])*(bpostY[ibinY+1]));
          else*/
            binY[ibinY]->Y = ((bpostY[ibinY])+(bpostY[ibinY+1]))/2.0;
      		
          //memcpy(&bin[ibinXY]->ndata, &RefOut->neutrons[cNeut], sizeof(NeutronEx));
          memcpy(&binY[ibinY]->ndata.neutron.ID, &RefOut->neutrons[cNeut].neutron.ID, sizeof(TotalID));
          binY[ibinY]->ndata.neutron.Debug = RefOut->neutrons[cNeut].neutron.Debug;
          binY[ibinY]->ndata.ThisCollision = RefOut->neutrons[cNeut].ThisCollision;
        } //else { //sum up, avarage will be generated by division through counts
          //bin[ibinXY].ndata.degangular          += GetValProb(&ValProb, dKeydegangular) * RefOut->neutrons[cNeut].degangular;
          binY[ibinY]->ndata.degangular          += ValProb * RefOut->neutrons[cNeut].degangular;
          binY[ibinY]->ndata.m                   += ValProb * RefOut->neutrons[cNeut].m;
          binY[ibinY]->ndata.reflectivity        += ValProb * RefOut->neutrons[cNeut].reflectivity;
          binY[ibinY]->ndata.DivY                += ValProb * RefOut->neutrons[cNeut].DivY;
          binY[ibinY]->ndata.DivZ                += ValProb * RefOut->neutrons[cNeut].DivZ;
          binY[ibinY]->ndata.Mode                += (int)(ValProb * RefOut->neutrons[cNeut].Mode);
          binY[ibinY]->ndata.neutron.Color       += (short)ValProb * RefOut->neutrons[cNeut].neutron.Color;
          binY[ibinY]->ndata.neutron.Time        += ValProb * RefOut->neutrons[cNeut].neutron.Time;
          binY[ibinY]->ndata.neutron.Wavelength  += ValProb * RefOut->neutrons[cNeut].neutron.Wavelength;
          binY[ibinY]->ndata.neutron.Probability += ValProb * RefOut->neutrons[cNeut].neutron.Probability;
          binY[ibinY]->ndata.neutron.Position[0] += ValProb * RefOut->neutrons[cNeut].neutron.Position[0];
          binY[ibinY]->ndata.neutron.Position[1] += ValProb * RefOut->neutrons[cNeut].neutron.Position[1];
          binY[ibinY]->ndata.neutron.Position[2] += ValProb * RefOut->neutrons[cNeut].neutron.Position[2];
          binY[ibinY]->ndata.neutron.Vector[0]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[0];
          binY[ibinY]->ndata.neutron.Vector[1]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[1];
          binY[ibinY]->ndata.neutron.Vector[2]   += ValProb * RefOut->neutrons[cNeut].neutron.Vector[2];
          binY[ibinY]->ndata.neutron.Spin[0]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[0];
          binY[ibinY]->ndata.neutron.Spin[1]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[1];
          binY[ibinY]->ndata.neutron.Spin[2]     += ValProb * RefOut->neutrons[cNeut].neutron.Spin[2];
        //}
        binY[ibinY]->ProbSum   += ValProb;
        binY[ibinY]->RefCount  += ValProb * abs(RefOut->RefCount);
        binY[ibinY]->RefCountY += ValProb * RefOut->RefCountY;
        binY[ibinY]->RefCountZ += ValProb * RefOut->RefCountZ;
        switch (RefOut->neutrons[cNeut].Mode) {
          case 0:
            binY[ibinY]->Mode0++;
            break;
          case 5:
            binY[ibinY]->Mode5++;
            break;
          case 10:
            binY[ibinY]->Mode10++;
            break;
        }

        binY[ibinY]->Counts++;
      }
//#endif
    }
  }
}

/*double GetValProb(const double *ValProb, const int Key)
{
  if (Key == KeyProb) return 1.0; else return *ValProb;
}*/


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit   (int argc, char *argv[])
{
  long  i,j, cPlane;
  char  *arg=NULL, sLine[512];
  FILE* pFile=NULL;
  char sRefFileL[512] = "", sRefFileR[512] = "", sRefFileT[512] = "", sRefFileB[512] = "";
  ReflFile *pRefFileLast;
  double bintervalX=1.0, bintervalY=1.0, rot=0.0;
  int ibinX, ibinY;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      arg=&argv[i][2];   //free                                         k K   L                   Q                        
      switch(argv[i][1]) //used a A b B c C d D e E f F g G h H i I j J     l   m M n N o O p P q   r R s S t T u U v V w W x X y Y z Z
      {
        case 'i':  /* left plane */
          if( (pReflL = fopen(FullParName(arg),"r"))==NULL)
          {	fprintf(LogFilePtr,"ERROR: File %s containing coating of left plane could not be opened\n",arg);
            exit(-1);
          }
          ReflFileNameL=arg;
          break;
        case 'I':  /* right plane */
          if( (pReflR = fopen(FullParName(arg),"r"))==NULL)
          {	fprintf(LogFilePtr,"ERROR: File %s containing coating of right plane could not be opened\n",arg);
            exit(-1);
          }
          ReflFileNameR=arg;
          break;
        case 'j':    /* top plane */
          if( (pReflT = fopen(FullParName(arg),"r"))==NULL)
          {
            fprintf(LogFilePtr,"ERROR: File %s containing coating of top plane could not be opened\n",arg);
            exit(-1);
          }
          ReflFileNameT=arg;
          break;
        case 'J':    /* bottom plane */
          if( (pReflB = fopen(FullParName(arg),"r"))==NULL)
          {
            fprintf(LogFilePtr,"ERROR: File %s containing coating of bottom plane could not be opened\n",arg);
            exit(-1);
          }
          ReflFileNameB=arg;
          break;
        case 'o':  /* Reflection parameter writeout */
          if( (pReflParam = fopen(FullParName(arg),"w"))!=NULL)
          {	/*fprintf(LogFilePtr,"ERROR: File %s for creating reflection parameter output could not be created\n",arg);
            exit(-1);*/
            fprintf(pReflParam, "#____ID____ Scattered plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z\n"
                                "#    1           2      3       4      5          6          7        8     9    10     11      12         13           14         15         16         17        18        19       20   21   22 \n");
            ReflParamFileName=arg;
          }
          break;
        case 'O':
          keyReflParam = atoi(arg); /* Trajectories to be written out:
                                       1 = only those leaving the guide;
                                       2 = all successfull reflections; no matter if the trajectory reaches the guide end
                                       3 = only those with at least one successful scattering event (tracjectory may end with an unsuccessfull event)
                                       4 = all
                                       a negative number adds a line feed between each trajectory */
          break;
        case 'B':
          keyPlotParam = atoi(arg); /* Trajectories to be binned:
                                       0 = all
                                       1 = only scattered;
                                       2 = only dies
                                       WARNING: This is influenced by keyReflParam too. Set keyReflParam to all to get all events.
                                       */
          break;
        case 'P':  /* Reflection parameter plot */
          if( (pReflPlot = fopen(FullParName(arg),"w"))!=NULL)
          {	/*fprintf(LogFilePtr,"ERROR: File %s for creating reflection parameter output could not be created\n",arg);
            exit(-1);*/
            fprintf(pReflPlot, "#   X          Y        counts   Mode    0       5       10    RefCount RCy RCz  ____ID____ plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc  color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z   WeightSum\n"
                               "#   1          2         3=C     4=S    5=C     6=C     7=C    8=A      9=A 10=A  11=1N     12=1N 13=A      14=A  15=A           16=A     17=A  18=1N  19=A   20=A   21=A     22=S           23=A       24=A       25=A       26=A      27=A      28=A      29=A 30=A 31=A     32=S   \n"
                               "#1N = defined by first neutron in bin; A = avaraged; S = summed up; C = events counted; Mode: 0=Scattered, 5=No interaction, 10=Died\n");

            ReflPlotFileName=arg;
          }
          break;
        case 'v':    /* Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated. */
          keyReflVerbose = atoi(arg); 
          break;
        case 'e':    /* Minimum number of reflections within the guide for reflection list output */
          keyReflMinCnt = atoi(arg); 
          break;
        case 'E':    /* Maximum number of reflections within the guide for reflection list output */
          keyReflMaxCnt = atoi(arg); 
          break;
        case 'c':    /* Minimum number of reflections on the horizontal guide for reflection list output */
          keyReflMinCntY = atoi(arg); 
          break;
        case 'C':    /* Maximum number of reflections on the horizontal guide for reflection list output */
          keyReflMaxCntY = atoi(arg); 
          break;
        case 'd':    /* Minimum number of reflections on the vertical guide for reflection list output */
          keyReflMinCntZ = atoi(arg); 
          break;
        case 'D':    /* Maximum number of reflections on the vertical guide for reflection list output */
          keyReflMaxCntZ = atoi(arg); 
          break;

        case 'S':    /* shape file */
          ShapeFileName=arg;
          break;

        case 'h':
          GuideEntranceHeight =  atof(arg);
          break;
        case 'H':
          GuideExitHeight = atof(arg);
          break;
        case 'w':
          GuideEntranceWidth = atof(arg);
          break;
        case 'W':
          GuideExitWidth = atof(arg);
          break;

        case 'f':
          FocusY = atof(arg);      /* distance of focus point behind guide exit */
          break;
        case 'F':
          FocusZ = atof(arg);      /* distance of focus point behind guide exit */
          break;
        case 'z':
          PhiAnfZ = atof(arg);    /* Phase of ellipse for height at guide entrance (in deg) */
          PhiAnfZ *= M_PI/180.0;  /* 90 deg means: max. width of ellipse       */
          break;
        case 'y':
          PhiAnfY = atof(arg);    /* Phase of ellipse for width at guide entrance (in deg) */
          PhiAnfY *= M_PI/180.0;
          break;
      	
        case 'n':
          rotplane = atof(arg); /* Additional planes: rot angle */
          if (fabs(rotplane) >= 90.0) {
            rotplane = 0.0;
          } else if (fabs(rotplane) > 0.0) {
            if (rotplane > 0.0) keyAddPlane = 1; else keyAddPlane = 2;
            rot = rotplane;
            while (rot < 90.0) {
              rot += rotplane;
              nPlanes += 4;
            }
            GW_EXIT = nPlanes;
            GW_INIT = GW_EXIT+1;
          }
          break;
        case 'N':
          nPieces = atol(arg); /* number of pieces */
          break;
        case 'R':
          Radius =  atof(arg); /* in m */
          Radius *= 100.0;
          break;
        case 'p':
          piecelength = atof(arg); /* length of 1 piece of guide in cm */
          break;
        case 'Y':                   /* Shape of guide: 0: constant                           */
          eGuideShapeY = atol(arg); /*                 1: (linearly) converging or diverging */
          break;                    /*                 2: curved (circular)                  */
        case 'Z':                   /*                 3: parabolic                          */
          eGuideShapeZ = atol(arg); /*                 4: elliptic                           */
          break;

        case 'M':
          MuScat =  atof(arg); /* macroscopic scattering coeff. in 1/cm */
          break;
        case 'm':
          MuAbs  =  atof(arg); /* macroscopic absorption coeff. in 1/cm */
          break;

        case 'b':
          nChannels = atol(arg); /* bender: No. of channels  */
          nSpacers  = nChannels - 1;
          break;
        case 's':
          spacer  = atof(arg);  /* width of bender channel border in cm */
          break;

        case 'a':
          keyabut = (short) atoi(arg); /* key for abutment loss  0: no (default), 1: yes  */
          break;
        case 'l':
          AbutLen = atof(arg);  /* area around the connection of guide segments where neutrons are absorbed */
          break;
        case 'A':
          AddToColor =  atoi(arg); /* Adds value to color for each reflection  */
          break;
        case 'q':
          eWaviDistr  = atoi(arg);  /* enum: waviness distribution: 1: rectangular (default), 2: Gaussian  */
          break;
        case 'r':
          surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
          surfacerough  =  surfacerough*M_PI/180.0; /*Convert from degree to radian */
          surfacerough  =  tan(surfacerough);
          break;

        case 'k':
          nbinsX = atol(arg); /* number of bins */
          break;

        case 'K':
          nbinsY = atol(arg); /* number of bins */
          break;

        case 'x':
          MinX = atof(arg);   /* lower bound of X bin */
          break;

        case 'X':
          MaxX = atof(arg);   /* upper bound of X bin */
          break;

        case 'u':
          MinY = atof(arg);   /* lower bound of Y bin */
          break;

        case 'U':
          MaxY = atof(arg);   /* upper bound of Y bin */
          break;

        case 't':    /* Key for x bin */
          KeyX = atoi(arg);
          break;

        case 'T':    /* Key for y bin */
          KeyY = atoi(arg); 
          break;

        case 'V':    /* Probability weighting key */
          KeyProb = atoi(arg); 
          break;

        default:
          fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  /* Input checks */
  /* ------------ */
  /* consistency in abutment loss parameters */
  if (keyabut==ON  && AbutLen==0.0) AbutLen=0.5;  
  if (keyabut==OFF && AbutLen > 0.0)  
    Error("Inconsistent abutment loss parameters");

  /* combination: orientation - shape */
  if (eGuideShapeZ==VT_CURVED)
    Error("Vertical curving of the guide not supported");

  /* Radius  */
  if (eGuideShapeY==VT_CURVED)
  {	if (Radius==0.0)
      Error("Radius of curvature is missing");
  }
  else
  {	if (Radius!=0.0)
      Error("Curvature of guide only supported in option 'curved', please delete radius or switch to 'curved'");
  }

  /* Exit and entrance size */
  if (eGuideShapeZ==VT_CONSTANT)
  {	if (GuideExitHeight != GuideEntranceHeight)
      Error("In guides of constant height, exit and entrance height have to be equal");
    else
      GuideExitHeight = GuideEntranceHeight;
  }
  else
  {	if (GuideExitHeight== 0.0)
      Error("You must enter the height of the guide exit");
  }

  if (eGuideShapeY==VT_CONSTANT || eGuideShapeY==VT_CURVED)
  {	if (GuideExitWidth != GuideEntranceWidth)
      Error("In curved and constant guides, exit and entrance width have to be equal");
    else
      GuideExitWidth  = GuideEntranceWidth;
  }
  else
  {	if (GuideExitWidth == 0.0)
      Error("You must enter the width of the guide exit");
  }

  if ((eGuideShapeY==VT_ELLIPTIC && PhiAnfY < 0.5*M_PI && GuideExitWidth  > GuideEntranceWidth) ||
      (eGuideShapeZ==VT_ELLIPTIC && PhiAnfZ < 0.5*M_PI && GuideExitHeight > GuideEntranceHeight)   )
  {
    Error("The ellipse must widen at the guide entrance (angle > 90 deg) to achieve a exit width larger than the entrance width");
  }

  /* number of pieces */
  /* ---------------- */
  if (   eGuideShapeZ==VT_PARABOLIC || eGuideShapeZ==VT_ELLIPTIC
      || eGuideShapeY==VT_PARABOLIC || eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_CURVED)
  {	if (nPieces <= 1)
      Error("More than 1 piece is necessary for this shape");
  }

  if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
  {	
    pFile = fopen(FullParName(ShapeFileName), "r");
    if (pFile != NULL)
    {	nPieces = LinesInFile(pFile) - 1;
      cReflFiles = (nPieces+1) * 4;
    }
    else
    {	fprintf(LogFilePtr,"ERROR: Input file %s could not be read !\n", ShapeFileName);
      exit(-1);
    }
  } else {
    cReflFiles = 4;
  }

  pReflFiles = calloc(cReflFiles, sizeof(ReflFile));
  if (!pReflFiles) { fprintf(LogFilePtr,"ERROR: Not enough memory for reflecitvity data!\n");
    exit(-1);
  }

  /* left plane */
  if (pReflL == NULL)
  {	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the left wall \n");
  }
  /* right plane */
  if (pReflR == NULL)
  {	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the right wall \n");
  }
  /* top plane */
  if (pReflT == NULL)
  {	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the top wall \n");
  }
  /* bottom plane */
  if (pReflB == NULL)
  {	pReflB = pReflT;
    ReflFileNameB = ReflFileNameT;
    fprintf(LogFilePtr,"\nNOTE: coating of top wall also used for bottom wall \n");
  }

  /* Init bin arrays */
  GetValueX = SetValueFunction(KeyX);
  GetValueY = SetValueFunction(KeyY);
  GetProb   = SetValueFunction(KeyProb);

  if (pReflPlot != NULL)
  {
    bpostX = malloc(sizeof(double)*(nbinsX+1));
    memset(bpostX, 0, sizeof(double)*(nbinsX+1));
    bpostY = malloc(sizeof(double)*(nbinsY+1));
    memset(bpostY, 0, sizeof(double)*(nbinsY+1));
    bin = malloc(sizeof(BINDATA*)*(INDEX(nbinsX, nbinsY)+1));
    memset(bin, 0, sizeof(BINDATA*)*(INDEX(nbinsX, nbinsY)+1));
  //#ifdef bn
    //binX = calloc((nbinsX+1), sizeof(BINDATA));
    binX = malloc(sizeof(BINDATA*)*(nbinsX+1));
    memset(binX, 0, sizeof(BINDATA*)*(nbinsX+1));
    //binY = calloc((nbinsY+1), sizeof(BINDATA*));
    binY = malloc(sizeof(BINDATA*)*(nbinsY+1));
    memset(binY, 0, sizeof(BINDATA*)*(nbinsY+1));
  //#endif

    bintervalX = (MaxX - MinX) / (double)nbinsX;
    bintervalY = (MaxY - MinY) / (double)nbinsY;

    for(ibinX = 0; ibinX<=nbinsX; ibinX++)
      bpostX[ibinX] = MinX + bintervalX*ibinX;
    for(ibinY = 0; ibinY<=nbinsY; ibinY++)
      bpostY[ibinY] = MinY + bintervalY*ibinY;
  }

  /* Calculation of height, width and channel-width of beginning and end of pieces */
  /* ----------------------------------------------------------------------------- */
  /*Xpce  = calloc(nPieces+1, sizeof(double));
  Ypce  = calloc(nPieces+1, sizeof(double));
  Zpce  = calloc(nPieces+1, sizeof(double));
  Wchan = calloc(nPieces+1, sizeof(double));*/
  pPieces = calloc(nPieces+1, sizeof(GuidePiece));
  pRefFileLast = GetReflFile(ReflFileNameL, pReflL);

  if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
  {	
    for(j=0; j <= nPieces; j++)
    {	
      ReadLine(pFile, sLine, sizeof(sLine)-1);
  		
      sRefFileL[0] = '\0'; sRefFileR[0] = '\0'; sRefFileT[0] = '\0'; sRefFileB[0] = '\0';
      sscanf(sLine, "%lf %lf %lf %s %s %s %s", &pPieces[j].Xpce, &pPieces[j].Ypce, &pPieces[j].Zpce, (char *)&sRefFileL, (char *)&sRefFileR, (char *)&sRefFileT, (char *)&sRefFileB);
      pPieces[j].Xpce *= 100.0;
      if (j==0) XpceZero = pPieces[0].Xpce;
      pPieces[j].Xpce -= XpceZero;
      pPieces[j].Ypce *=   0.5;
      pPieces[j].Zpce *=   0.5;
      /*pPieces[j].Xpce = RoundP(pPieces[j].Xpce, 7);
      pPieces[j].Ypce = RoundP(pPieces[j].Ypce, 7);
      pPieces[j].Zpce = RoundP(pPieces[j].Zpce, 7);*/
      pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
      if (pPieces[j].Wchan <= 0.0)
        Error("Geometry impossible. Channel width gets zero (or less)");
      if (j>0) {
        AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }
  		
      /* alloc */
      pPieces[j].RData = calloc(nPlanes, sizeof(ReflFile*));
      if (!pPieces[j].RData) { fprintf(LogFilePtr,"ERROR: Not enough memory for reflectivity of planes!\n");
        exit(-1);
      }

      /* Calculate Area for this reflectivity file */
      if (j > 0) {
        if (pPieces[j-1].RData[GW_LEFT]!=NULL)
          pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_RIGHT]!=NULL) 
          pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_TOP]!=NULL) 
          pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_BOTTOM]!=NULL)
          pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }
  		
      /* Either load standard reflectivity file or use userdefined one */
      switch (sRefFileL[0]) {
        case ':':  pPieces[j].RData[GW_LEFT]   = pRefFileLast;break;
        case '\0': pPieces[j].RData[GW_LEFT]   = GetReflFile(ReflFileNameL, pReflL);break;
        default:   pPieces[j].RData[GW_LEFT]   = GetReflFile(FullParName((char *)&sRefFileL), NULL);
          pRefFileLast = pPieces[j].RData[GW_LEFT];
          break;
      }
  		

      switch (sRefFileR[0]) {
        case ':':  pPieces[j].RData[GW_RIGHT]  = pRefFileLast;break;
        case '\0': pPieces[j].RData[GW_RIGHT]  = GetReflFile(ReflFileNameR, pReflR);break;
        default:   pPieces[j].RData[GW_RIGHT]  = GetReflFile(FullParName((char *)&sRefFileR), NULL);
          pRefFileLast = pPieces[j].RData[GW_RIGHT];
          break;
      }
  		

      switch (sRefFileT[0]) {
        case ':':  pPieces[j].RData[GW_TOP]    = pRefFileLast;break;
        case '\0': pPieces[j].RData[GW_TOP]    = GetReflFile(ReflFileNameT, pReflT);break;
        default:   pPieces[j].RData[GW_TOP]    = GetReflFile(FullParName((char *)&sRefFileT), NULL);
          pRefFileLast = pPieces[j].RData[GW_TOP];
          break;
      }
  		

      switch (sRefFileB[0]) {
        case ':':  pPieces[j].RData[GW_BOTTOM] = pRefFileLast;break;
        case '\0': pPieces[j].RData[GW_BOTTOM] = GetReflFile(ReflFileNameB, pReflB);break;
        default:   pPieces[j].RData[GW_BOTTOM] = GetReflFile(FullParName((char *)&sRefFileB), NULL);
          pRefFileLast = pPieces[j].RData[GW_BOTTOM];
          break;
      }

      /* for additional planes use pointers. WARNING please use same order of assingment as in other functions */
      if (nPlanes > 4) {
        rot = rotplane;
        cPlane = GW_RIGHT;
        while (rot < 90.0 && cPlane < GW_EXIT) {
          switch (keyAddPlane) {
            case 1:
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_TOP];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_TOP];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_BOTTOM];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_BOTTOM];
              break;
            case 2:
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_LEFT];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_LEFT];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_RIGHT];
              cPlane++;
              pPieces[j].RData[cPlane] = pPieces[j].RData[GW_RIGHT];
              break;
          }
          rot += rotplane;
        }
      }
    }
    dTotalLength = RoundP(pPieces[nPieces].Xpce - pPieces[0].Xpce, 7);
    GuideEntranceWidth = pPieces[0].Ypce*2.;
    GuideEntranceHeight = pPieces[0].Zpce*2.;
    GuideExitWidth = pPieces[nPieces].Ypce*2.;
    GuideExitHeight = pPieces[nPieces].Zpce*2.;
    piecelength  = dTotalLength / (double)nPieces; /* Use piecelength with care in the case of nonequidistant planes */
  }
  else
  {
    pFile = fopen(FullParName(ShapeFileName), "w+");

    dTotalLength = nPieces*piecelength;

    for(j=0; j <= nPieces; j++)
    {	
      pPieces[j].Xpce  = j*piecelength;
      pPieces[j].Ypce  = Width (pPieces[j].Xpce)/2.0;
      pPieces[j].Zpce  = Height(pPieces[j].Xpce)/2.0;
      pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
      if (pPieces[j].Wchan <= 0.0)
        Error("Geometry impossible. Channel width gets zero (or less)");
      if (pFile != NULL)
      {	if (j==0)
        {	fprintf(pFile, "# length [m]  width [cm]  height [cm]   reflectivity filenames (left, right, top, bottom) \n");
          fprintf(pFile, "#-----------------------------------------------------------------------------------------\n");
        } else {
          AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
          AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        }
        fprintf(pFile, "%10.3f  %10.4f  %10.4f\n", pPieces[j].Xpce/100.0, 2.0*pPieces[j].Ypce, 2.0*pPieces[j].Zpce);
      }
      
      /* alloc */
      pPieces[j].RData = calloc(nPlanes, sizeof(ReflFile*));
      if (!pPieces[j].RData) { fprintf(LogFilePtr,"ERROR: Not enough memory for reflectivity of planes!\n");
        exit(-1);
      }
      
      /* Calculate Area for this reflectivity file */
      if (j > 0) {
        if (pPieces[j-1].RData[GW_LEFT]!=NULL)
          pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_RIGHT]!=NULL) 
          pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_TOP]!=NULL) 
          pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_BOTTOM]!=NULL)
          pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }

      /* Load reflectivity file or reuse already loaded file */
      pPieces[j].RData[GW_LEFT]   = GetReflFile(ReflFileNameL, pReflL);
      pPieces[j].RData[GW_RIGHT]  = GetReflFile(ReflFileNameR, pReflR);
      pPieces[j].RData[GW_TOP]    = GetReflFile(ReflFileNameT, pReflT);
      pPieces[j].RData[GW_BOTTOM] = GetReflFile(ReflFileNameB, pReflB);
    }
  }
  if (pFile != NULL)
    fclose(pFile);
}

/* Read guide data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, ... deg,  */
/* reference wavelength 1 A */
ReflFile *GetReflFile(char *Filename, FILE *file)
{
  //GetReflFile = NULL;
  long  cFiles = 0;
  if (Filename == NULL) return NULL;

  for (cFiles = 0; cFiles < cReflFiles; cFiles++) {
    if (pReflFiles[cFiles].filename != NULL) {
#ifdef _MSC_VER
      if (_stricmp(pReflFiles[cFiles].filename, Filename)==0) {
        return &pReflFiles[cFiles];
#else
      if (strcasecmp(pReflFiles[cFiles].filename, Filename)==0) {
        return &pReflFiles[cFiles];
#endif
      }
    } else {
      break;
    }
  }
  if (pReflFiles[cFiles].filename == NULL) {
    pReflFiles[cFiles].filename = Filename;
    if (file != NULL) pReflFiles[cFiles].pfile = file;
    LoadReflFile(&pReflFiles[cFiles]);
    return &pReflFiles[cFiles];
  }

  return NULL;
}

/* Load Reflectivity data from file. Give pReflFile as input */
void   LoadReflFile(ReflFile *pReflFile)
{
  long   count = 0, i = 0, nLines = 0;
  char   sBuffer[512]="";

  if (pReflFile!=NULL) {
    if (pReflFile->filename != NULL) {
      if (pReflFile->pfile == NULL) pReflFile->pfile = fopen(FullParName(pReflFile->filename), "r");
      if (pReflFile->pfile != NULL) {
        nLines = LinesInFile(pReflFile->pfile);
        pReflFile->maxdata = nLines * 10;
        pReflFile->Rdata = calloc(pReflFile->maxdata, sizeof(double));
        for(count=0; count < nLines; count++) {
          ReadLine(pReflFile->pfile, sBuffer, sizeof(sBuffer)-1);
          i += StrgScanLF(sBuffer, &pReflFile->Rdata[10*count], pReflFile->maxdata-10*count, 0);
        }
        fclose(pReflFile->pfile);
      }
    }
  }
}

/* own cleanup of the guide module */
/* --------------------------------*/
void OwnCleanup()
{
  //long cFiles = 0;

  /* print error that might have occured many times */
  PrintMessage(GUID_OUT_OF_EXIT, "", ON);
  PrintMessage(GUID_NO_PLANE, "", ON);

  fprintf(LogFilePtr," \n");

  /* set description for instrument plot */
  stPicture.dWPar = GuideEntranceWidth;
  stPicture.dHPar = GuideExitWidth;
  stPicture.dRPar = Radius/100.0;
  stPicture.eType = (short)(10*eGuideShapeY + eGuideShapeZ);
  if (nChannels > 1)
    stPicture.nNumber = - nChannels;
  else
    stPicture.nNumber = nPieces;

  beta_ges = (nPieces-1)*beta;
  if (Radius != 0.0)
  {	dDeltaX = Radius*sin(beta_ges)       + 0.5*piecelength*(cos(beta_ges)+1.0);
    dDeltaY = Radius*(1.0-cos(beta_ges)) + 0.5*piecelength* sin(beta_ges);
  }
  else
  {	dDeltaX = dTotalLength;
    dDeltaY = 0.0;
  }

  /*Non necessary
  if (pPieces!=NULL) free(pPieces);

  if (pReflFiles!=NULL) {
    for (cFiles = 0; cFiles < cReflFiles; cFiles++)
    {
      if (pReflFiles[cFiles].Rdata!=NULL) free(pReflFiles[cFiles].Rdata);
    }
    free(pReflFiles);
  }*/
  if (pReflParam!=NULL) fclose(pReflParam);
  if (pReflPlot!=NULL) fclose(pReflPlot);
}/* End OwnCleanup */


double Height(double dLength)
{
  double dHeight=0.0,
         L_end,           /* end of parabel or 2nd part of ellipse (center to exit) */
         eps,             /* correction value =(b*b)/(2a*a)        */
         Phi,
         Phi_anf,Phi_end; /* phases in ellipse                     */

  switch (eGuideShapeZ)
  {
    case VT_CONSTANT:
    case VT_CURVED:
      dHeight = GuideEntranceHeight;
      break;
    case VT_LINEAR:
      dHeight = GuideEntranceHeight + (GuideExitHeight-GuideEntranceHeight)/dTotalLength * dLength;
      break;
    case VT_PARABOLIC:
      AparZ   = dTotalLength/(sq(GuideEntranceHeight) - sq(GuideExitHeight));
      L_end   = AparZ * sq(GuideEntranceHeight);
      dHeight = sqrt((L_end-dLength)/AparZ);
      break;
    case VT_ELLIPTIC:
      /* first approximation */
      AxisZ   = 0.5*fabs((sq(dTotalLength+FocusZ)*sq(GuideExitHeight) - sq(FocusZ*GuideEntranceHeight))
                         /(FocusZ*sq(GuideEntranceHeight) - (dTotalLength+FocusZ)*sq(GuideExitHeight)));
      L_end   = AxisZ - FocusZ;
      LcntrZ  = dTotalLength - L_end;
      Phi_anf = acos(-LcntrZ/AxisZ);
      Phi_end = acos(L_end/AxisZ);
      GuideMaxHeight = GuideEntranceHeight/sin(Phi_anf);
      /* second approximation */
      eps     = 0.5 * sq(GuideMaxHeight/AxisZ);
      AxisZ   = 0.5*fabs((1.0+eps)*(sq(dTotalLength+FocusZ)*sq(GuideExitHeight) - sq(FocusZ*GuideEntranceHeight))
                         /(FocusZ*sq(GuideEntranceHeight) - (dTotalLength+FocusZ)*sq(GuideExitHeight) + eps*AxisZ*(sq(GuideEntranceHeight)-sq(GuideExitHeight)) ));
      L_end   = AxisZ/(1.0+eps) - FocusZ;
      LcntrZ  = dTotalLength - L_end;
      Phi_anf = acos(-LcntrZ/AxisZ);
      Phi_end = acos(L_end/AxisZ);
      GuideMaxHeight = GuideEntranceHeight/sin(Phi_anf);
      D_Foc1Z = LcntrZ - AxisZ/(1.0+eps);

      Phi     = acos((dLength - LcntrZ)/AxisZ);
      dHeight = GuideMaxHeight*sin(Phi);
      break;
    default:
      Error("Shape unknown");
  }

  return dHeight;
}

double Width(double dLength)
{
  double dWidth=0.0,
         L_end,           /* end of parabel or 2nd part of ellipse (center to exit) */
         eps,             /* correction value =(b*b)/(2a*a)        */
         Phi,
         Phi_anf,Phi_end; /* phases in ellipse                     */

  switch (eGuideShapeY)
  {
    case VT_CONSTANT:
    case VT_CURVED:
      dWidth = GuideEntranceWidth;
      break;
    case VT_LINEAR:
      dWidth = GuideEntranceWidth + (GuideExitWidth-GuideEntranceWidth)/dTotalLength * dLength;
      break;
    case VT_PARABOLIC:
      AparY  = dTotalLength/(sq(GuideEntranceWidth) - sq(GuideExitWidth));
      L_end  = AparY * sq(GuideEntranceWidth);
      dWidth = sqrt((L_end-dLength)/AparY);
      break;
    case VT_ELLIPTIC:
      /* first approximation */
      AxisY   = 0.5*fabs((sq(dTotalLength+FocusY)*sq(GuideExitWidth) - sq(FocusY*GuideEntranceWidth))
                         /(FocusY*sq(GuideEntranceWidth) - (dTotalLength+FocusY)*sq(GuideExitWidth)));
      L_end   = AxisY - FocusY;
      LcntrY  = dTotalLength - L_end;
      Phi_anf = acos(-LcntrY/AxisY);
      Phi_end = acos(L_end/AxisY);
      GuideMaxWidth = GuideEntranceWidth/sin(Phi_anf);
      /* second approximation */
      eps     = 0.5 * sq(GuideMaxWidth/AxisY);
      AxisY   = 0.5*fabs((1.0+eps)*(sq(dTotalLength+FocusY)*sq(GuideExitWidth) - sq(FocusY*GuideEntranceWidth))
                         /(FocusY*sq(GuideEntranceWidth) - (dTotalLength+FocusY)*sq(GuideExitWidth) + eps*AxisY*(sq(GuideEntranceWidth)-sq(GuideExitWidth)) ));
      L_end   = AxisY/(1.0+eps) - FocusY;
      LcntrY  = dTotalLength - L_end;
      Phi_anf = acos(-LcntrY/AxisY);
      Phi_end = acos(L_end/AxisY);
      GuideMaxWidth = GuideEntranceWidth/sin(Phi_anf);
      D_Foc1Y = LcntrY - AxisY/(1.0+eps);

      Phi     = acos((dLength - LcntrY)/AxisY);
      dWidth = GuideMaxWidth*sin(Phi);
      break;
    default:
      Error("Shape unknown");
  }

  return dWidth;
}


double PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide *ThisGuide, double  wei_min,
                                  GuidePiece *Pce, double surfacerough, long keygrav, double AbutLen, ReflCond *RefOut, long iPiece)
{
  /***********************************************************************************/
  /* This routine calculates the trajectory a neutron follows through a simple       */
  /* neutron guide. It accepts two structured variables; a pointer to a neutron      */
  /* structure and a simple Guide structure. This latter consists simply of four     */
  /* infinite planes describing the two walls floor and ceiling of the guide and a   */
  /* fifth infinite plane at the exit of the guide. The structure has an assosciated */
  /* critical angle; any neutron that intercepts a wall at an angle greater than this*/
  /* is absorbed.                                                                    */
  /* Neutron flight by parabolic trajectories with GRAVITY                           */
  /* Significant Rewrited by Manoshin Sergey Feb 2001                                */
  /* Note! The function is return Time Of Flight                                     */
  /***********************************************************************************/

  int     datanumber,
          iColl=0;      // index of collisions
  eGuideWall k = GW_INIT, ThisCollision = GW_INIT;
  double  degangular, ThisReflectivity=0.;
  double  TimeOF, TimeOFmin;
  double  TimeOFTotal=0.0;
  double  HitDist, DOTP;
  double  VX, VY, VZ, AngleWavi;
  double  RotMat[3][3];   
  VectorType vWallN,  /* normal to the plane wall             */
             vWaviN;  /* normal to the wall with its waviness */
  Neutron TempNeutron, NearestNeutron; /* Local copies of actual trajectory for loops */


  /***********************************************************************************/
  /* The main loop here is continuous: the neutron will continue to bounce around,   */
  /* until it is absorbed or intercepts with the exit plane.                         */
  /***********************************************************************************/

  while(TRUE)
  {
    TimeOFmin = 99999999999999999.9;

    /***********************************************************************************/
    /* Loop through all five planes....                                                */
    /***********************************************************************************/
    for(k=GW_TOP;k<GW_INIT;k++) /* GW_TOP = 0, GW_INIT = 5, i.e. maximum, GW_EXIT = GW_INIT-1 */
    {
      /***********************************************************************************/
      /* Find the point where this neutron trajectory intercepts the current plane       */
      /***********************************************************************************/

      /*Save current neutron, because the function 'NeutronPlaneIntersectionGrav' has
        modified trajectory data  */
      CopyNeutron(ThisNeutron, &TempNeutron);

      if (keygrav == 1)
        TimeOF = NeutronPlaneIntersectionGrav(&TempNeutron, ThisGuide->Wall[k]);
      else
        TimeOF = NeutronPlaneIntersection1   (&TempNeutron, ThisGuide->Wall[k]);

      /***********************************************************************************/
      /* If this intercept point is behind the neutrons current position, pass control to*/
      /* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */
      /***********************************************************************************/
      if ((TimeOF<=0.0) || (TempNeutron.Position[0] < ThisNeutron->Position[0]))
        continue;

      /***********************************************************************************/
      /* If this calculated distance is not the shortest so far, return to the top of the*/
      /* loop.  TimeOF -> min                                                            */
      /***********************************************************************************/
      if (TimeOF > TimeOFmin)
        continue;

      /***********************************************************************************/
      /* The intercept of the neutron with this wall is the nearest so far, so accept it */
      /* temporarily.                                                                    */
      /***********************************************************************************/

      CopyNeutron(&TempNeutron, &NearestNeutron);
      TimeOFmin = TimeOF;
      ThisCollision = k;
    }

    /***********************************************************************************/
    /* Having looped through all five planes, the current values of NearestNeutron,    */
    /* TimeOFmin and ThisCollision, reflect the coordinates, distance and index        */
    /* of the neutrons interaction with a guide wall. If this guide wall is index 4    */
    /*(i.e. the exit window) reset the neutron coordinates to this point, add the path */
    /* length to this point to the running total and return that total.                */
    /***********************************************************************************/

    if(ThisCollision == GW_EXIT)
    {
      if(NearestNeutron.Vector[0] < 0.0)
        return(-1.0);

      /*  Neutrons hitting the surface close to the guide exit are removed,
          but not in the last segment (iPiece=nPieces-1)                          */
      if (iPiece!=nPieces-1 && AbutLen > 0.0)
      {
        HitDist = TimeOFmin * V_FROM_LAMBDA(NearestNeutron.Wavelength);
        if (HitDist <= 0.5*AbutLen)
          return(-1.0);
      }

      if (NearestNeutron.Probability < wei_min)
        return(-1.0);

      ThisNeutron->Position[0] = NearestNeutron.Position[0];
      ThisNeutron->Position[1] = NearestNeutron.Position[1];
      ThisNeutron->Position[2] = NearestNeutron.Position[2];
      ThisNeutron->Vector  [2] = NearestNeutron.Vector  [2];
      ThisNeutron->Vector  [0] = sqrt(1.0 - sq(ThisNeutron->Vector[1])
                                          - sq(ThisNeutron->Vector[2]));
      ThisNeutron->Probability = NearestNeutron.Probability;

      TimeOFTotal =  TimeOFTotal + TimeOFmin;
      if (keyReflVerbose == 1)
        WriteReflParam(RefOut, 5, ThisNeutron, ThisGuide, Pce, ThisCollision, 0., 0.);

      return TimeOFTotal;
    }
    else
    { 
      /*  Neutrons hitting the surface close to the entrance of a guide segment are removed,
          but not in the first segment (iPiece=0); only the first collision (iColl=1) can be considered */
      iColl++;
      if (iPiece!=0 && iColl==1 && AbutLen > 0.0)
      {
        HitDist = TimeOFmin * V_FROM_LAMBDA(NearestNeutron.Wavelength);
        if (HitDist <= 0.5*AbutLen)
          return(-1.0);
      }
    }


    /***********************************************************************************/
    /* If the angle of intersection of the flight path and the guide wall exceeds the  */
    /* critical angle of the guide, the neutron is absorbed.                           */
    /* Otherwise the probability is reduced by the reflectivity of the plane.          */
    /***********************************************************************************/

    vWallN[0] = ThisGuide->Wall[ThisCollision].A;
    vWallN[1] = ThisGuide->Wall[ThisCollision].B;
    vWallN[2] = ThisGuide->Wall[ThisCollision].C;

    /* Normalize normal vector to the reflection plane */
    if (LengthVector(vWallN) == 0.0)
      return(-1.0);
    else
      NormVector(vWallN);

    /* influence of rough surface */
    if (surfacerough == 0.0)
    {	CopyVector(vWallN, vWaviN);
    }
    else
    { /* rough surface must not alter the side from which the neutron comes */
      do
      {	/* Gaussian distribution */
        if (eWaviDistr==VT_GAUSSIAN)
        {	
          CopyVector(vWallN, vWaviN);

          // Rotate about y-axis if top or bottom plane
          if (vWallN[1]==0)          // top, bottom
          {	
            AngleWavi = gsl_ran_gaussian(vit_gsl_rng, surfacerough);
            FillRotMatrixY(RotMat, AngleWavi);
            RotVector(RotMat, vWaviN); 
          }
          // Rotate about z-axis if left or right plane
          else if (vWallN[2]==0)     // left, right
          {	
            AngleWavi = gsl_ran_gaussian(vit_gsl_rng, surfacerough);
            FillRotMatrixZ(RotMat, AngleWavi);
            RotVector(RotMat, vWaviN);  
          }

          // Rotate about x-axis
          AngleWavi = gsl_ran_gaussian(vit_gsl_rng, surfacerough);
          FillRotMatrixX(RotMat, AngleWavi);
          RotVector(RotMat, vWaviN);
        }
        else
        /* Rectangular distribution */
        {	gsl_ran_dir_3d(vit_gsl_rng, &VX, &VY, &VZ);
          vWaviN[0] = vWallN[0] + surfacerough*VX;
          vWaviN[1] = vWallN[1] + surfacerough*VY;
          vWaviN[2] = vWallN[2] + surfacerough*VZ;
        }

          /* Renormalize normal vector */
          if (LengthVector(vWaviN) == 0.0)
            return(-1.0);
          else
            NormVector(vWaviN);
        }
      while (  ScalarProduct(NearestNeutron.Vector, vWallN)
              * ScalarProduct(NearestNeutron.Vector, vWaviN) < 0.0);
    }

    /* angle between normal vector and neutron flight direction (in degree) */
    degangular = fabs(90 - AngleVectors(NearestNeutron.Vector, vWaviN));

    /* Determine number of reflectivity value in reflectivty file */
    datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength));
    if (datanumber < 0) {
      CountMessageID(ALL_NEGATIVE_INT, NearestNeutron.ID);
      return -1.0;
    }

    /* Choose the reflectivity file/value and multiply probability by reflectivity value */
    //if (ThisCollision == GW_TOP || ThisCollision == GW_BOTTOM || ThisCollision == GW_LEFT || ThisCollision == GW_RIGHT) {
    if (ThisCollision < GW_EXIT) {
      if (Pce->RData[ThisCollision]==NULL || datanumber >= Pce->RData[ThisCollision]->maxdata) {
        WriteReflParam(RefOut, 10, &NearestNeutron, ThisGuide, Pce, ThisCollision, degangular, 0.);
        return -1.0;
      } else {
        ThisReflectivity = Pce->RData[ThisCollision]->Rdata[datanumber]; }
    } else {
      CountMessageID(GUID_NO_PLANE, NearestNeutron.ID);
      return -1.0;
    }

    NearestNeutron.Probability *= ThisReflectivity;

    if (NearestNeutron.Probability < wei_min){
      NearestNeutron.Probability = 0.;
      WriteReflParam(RefOut, 10, &NearestNeutron, ThisGuide, Pce, ThisCollision, degangular, ThisReflectivity);
      return -1.0;
    }


    /***********************************************************************************/
    /* Calculate the direction of the reflected neutron.                               */
    /* Set the neutron coordinates to coordinates of the collision                     */
    /* Correct if waviness has prevented a change in flight direction                  */
    /* calculate new count rate                                                        */
    /* return to the begining of the loop and find the next collision                  */
    /***********************************************************************************/

    DOTP = ScalarProduct(vWaviN, NearestNeutron.Vector);

    /* Reflection must alter direction relative to wall orientation */
    ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*vWaviN[0];
    ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*vWaviN[1];
    ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*vWaviN[2];

    while (  ScalarProduct(ThisNeutron->Vector,   vWallN)
           * ScalarProduct(NearestNeutron.Vector, vWallN) > 0.0)
    {
      ThisNeutron->Vector[0] -= 2.0*DOTP*vWaviN[0];
      ThisNeutron->Vector[1] -= 2.0*DOTP*vWaviN[1];
      ThisNeutron->Vector[2] -= 2.0*DOTP*vWaviN[2];
    }
    NormVector(ThisNeutron->Vector);

    /* CopyVector(NearestNeutron.Position, ThisNeutron->Position); */
    ThisNeutron->Position[0] = NearestNeutron.Position[0];
    ThisNeutron->Position[1] = NearestNeutron.Position[1];
    ThisNeutron->Position[2] = NearestNeutron.Position[2];

    ThisNeutron->Probability = NearestNeutron.Probability;
    ThisNeutron->Color += AddToColor;

    TimeOFTotal +=  TimeOFmin;
    WriteReflParam(RefOut, 0, ThisNeutron, ThisGuide, Pce, ThisCollision, degangular, ThisReflectivity);
  }
}

void   WriteReflParam(ReflCond *RefOut, int Mode, Neutron *pNeutron, NeutronGuide *ThisGuide, GuidePiece *Pce, 
                      eGuideWall ThisCollision, double degangular, double reflectivity)
{
 //fprintf(pReflParam, "#____ID____ Scattered plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z\n");
  const char *fstr = "%c%c%09lu     %c     %3d   %8.5f %6.2f %12.5f %8.4f %8.4f  %c %5d  %7.3f %8.5f %11.3e  %10.4f %10.4f %10.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n";
  double     DivY, DivZ, mVal, Qz;
  char       buffer[256] = "";

  if (!RefOut) return;
  if (pReflParam==NULL && pReflPlot==NULL) return;

  DivY = (double)atan2(pNeutron->Vector[1], pNeutron->Vector[0]);
    DivY *= 180.0/M_PI;
    if ((pNeutron->Vector[1]==0.0) && (pNeutron->Vector[0]==0.0))
      DivY = 0.0;

    DivZ = (double)atan2(pNeutron->Vector[2], pNeutron->Vector[0]);
    DivZ *= 180.0/M_PI;
    if ((pNeutron->Vector[2]==0.0) && (pNeutron->Vector[0]==0.0))
      DivZ = 0.0;

  Qz = 4.*M_PI/pNeutron->Wavelength*sin(degangular*M_PI/180.);
  mVal = Qz/0.02174;

  if (pReflParam!=NULL)
  {
    int Scattered;
    switch (Mode)
    {
      case 10: Scattered = 'F'; break; // Neutron died
      case  5: Scattered = '-'; break; // GW_EXIT
      case  0: Scattered = 'T'; break; // Scattered
      default: Scattered = 0;
    }
    if (Scattered)
    {
      sprintf(buffer, fstr,
        pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo,
        Scattered, ThisCollision, degangular, mVal, reflectivity, DivY, DivZ,
        pNeutron->Debug,       pNeutron->Color,
        pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
        pNeutron->Position[0]+Pce->Xpce, pNeutron->Position[1], pNeutron->Position[2],
        pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
        pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]
        );
    }
  }

  if (RefOut != NULL)
  {
    if ((Mode == 0) || (Mode == 5 && RefOut->RefCount >= 0) || (Mode != 0 && abs(keyReflParam) > 2))
    {
      char    *tmp = NULL;
      NeutronEx *tmpneutrons = NULL;
      size_t curlen = (RefOut->Output)?strlen((RefOut->Output)):0;

      if((tmp = realloc(RefOut->Output,curlen+strlen(buffer)+1)) != NULL)
      {
        if (curlen == 0) *tmp = '\0';
        RefOut->Output = tmp;
        strcat(RefOut->Output, buffer);
      }
  		
      /* neutrons */
      if ((ThisCollision < GW_EXIT) && 
        ((keyPlotParam == 0) || ((keyPlotParam == 1) && (Mode == 0)) || ((keyPlotParam == 2) && (Mode == 10)))) {
        if (RefOut->cneutrons == 0)
        {
          RefOut->neutrons = malloc(sizeof(NeutronEx));
          //CopyNeutron(pNeutron, &RefOut->neutrons[0].neutron);
        } else {
          if((tmpneutrons = realloc(RefOut->neutrons, sizeof(NeutronEx)*(RefOut->cneutrons+1))) != NULL)
          {
            RefOut->neutrons = tmpneutrons;
            //CopyNeutron(pNeutron, &RefOut->neutrons[RefOut->cneutrons].neutron);
          }
        }
        CopyNeutron(pNeutron, &RefOut->neutrons[RefOut->cneutrons].neutron);
        RefOut->neutrons[RefOut->cneutrons].neutron.Position[0] += Pce->Xpce + XpceZero;
        /*if (RefOut->neutrons[RefOut->cneutrons].neutron.Position[0] > 2850) {
          RefOut->neutrons[RefOut->cneutrons].degangular = degangular;
        }*/
        RefOut->neutrons[RefOut->cneutrons].ThisCollision = ThisCollision;
        RefOut->neutrons[RefOut->cneutrons].degangular = degangular;
        RefOut->neutrons[RefOut->cneutrons].m = mVal;
        RefOut->neutrons[RefOut->cneutrons].reflectivity = reflectivity;
        RefOut->neutrons[RefOut->cneutrons].DivY = DivY;
        RefOut->neutrons[RefOut->cneutrons].DivZ = DivZ;
        RefOut->neutrons[RefOut->cneutrons].Mode = Mode;
        RefOut->cneutrons++;
      }
    }

    if (Mode != 5)
    {
      RefOut->RefCount++;
      if (ThisCollision <= GW_BOTTOM)       // GW_TOP || GW_BOTTOM
        RefOut->RefCountZ++;
      else if (ThisCollision <= GW_RIGHT)   // GW_LEFT || GW_RIGHT
        RefOut->RefCountY++;     
      if (Mode != 0) RefOut->RefCount *= -1;
    }
  }
}

void   PrintMaximalM(double *RData, long i)
{
  long count;
  for(count=i-1; count >= 0; count--)
    if (RData[count] != 0.)
      break;
  if (count >= 0)
    fprintf(LogFilePtr," maximal defined m : %8.2f\n", sin(count/180000.*M_PI)*4*M_PI/0.02174);
  else
    fprintf(LogFilePtr," maximal defined m : absorber\n");
}

int    FindIndexXY(double *Xval, double *Yval, int *ibinX, int *ibinY)
{
  *ibinX = (int)((*Xval - MinX) / ((MaxX - MinX) / (double)nbinsX));
  *ibinY = (int)((*Yval - MinY) / ((MaxY - MinY) / (double)nbinsY));

  //FIXME: If ibin = nbins because yval = miny then the event is not binned!
  if (*ibinX < nbinsX && *ibinY < nbinsY)
    return INDEX(*ibinX, *ibinY);
  else
    return -1;
}

double GetValueNone            (ReflCond *RefOut, int cNeut) { return (double)1.0; }
double GetValueKeyMode         (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].Mode; }
double GetValueKeyRefCount     (ReflCond *RefOut, int cNeut) { return (double)RefOut->RefCount; }
double GetValueKeyRefCountY    (ReflCond *RefOut, int cNeut) { return (double)RefOut->RefCountY; }
double GetValueKeyRefCountZ    (ReflCond *RefOut, int cNeut) { return (double)RefOut->RefCountZ; }
double GetValueKeyThisCollision(ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].ThisCollision; }
double GetValueKeydegangular   (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].degangular; }
double GetValueKeym            (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].m; }
double GetValueKeyreflectivity (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].reflectivity; }
double GetValueKeyDivY         (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].DivY; }
double GetValueKeyDivZ         (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].DivZ; }
double GetValueKeyColor        (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Color; }
double GetValueKeyTime         (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Time; }
double GetValueKeyWavelength   (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Wavelength; }
double GetValueKeyProbability  (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Probability; }
double GetValueKeyPositionX    (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Position[0]; }
double GetValueKeyPositionY    (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Position[1]; }
double GetValueKeyPositionZ    (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Position[2]; }
double GetValueKeyVectorX      (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Vector[0]; }
double GetValueKeyVectorY      (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Vector[1]; }
double GetValueKeyVectorZ      (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Vector[2]; }
double GetValueKeySpinX        (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Spin[0]; }
double GetValueKeySpinY        (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Spin[1]; }
double GetValueKeySpinZ        (ReflCond *RefOut, int cNeut) { return (double)RefOut->neutrons[cNeut].neutron.Spin[2]; }


GetVal SetValueFunction(const int key)
{
  /*
    #define iKeyMode           1
    #define iKeyMode0          2
    #define iKeyMode5          3
    #define iKeyMode10         4
    #define dKeyRefCount       5
    #define dKeyRefCountY      6
    #define dKeyRefCountZ      7
    #define iKeyThisCollision  8
    #define dKeydegangular     9
    #define dKeym             10
    #define dKeyreflectivity  11
    #define dKeyDivY          12
    #define dKeyDivZ          13
    #define iKeyColor         14
    #define dKeyTime          15
    #define dKeyWavelength    16
    #define dKeyProbability   17
    #define dKeyPositionX     18
    #define dKeyPositionY     19
    #define dKeyPositionZ     20
    #define dKeyVectorX       21
    #define dKeyVectorY       22
    #define dKeyVectorZ       23
    #define dKeySpinX         24
    #define dKeySpinY         25
    #define dKeySpinZ         26
  */

  switch (key) {
    case iKeyMode:          return &GetValueKeyMode;
    case iKeyMode0:         return &GetValueNone;
    case iKeyMode5:         return &GetValueNone;
    case iKeyMode10:        return &GetValueNone;
    case dKeyRefCount:      return &GetValueKeyRefCount;
    case dKeyRefCountY:     return &GetValueKeyRefCountY;
    case dKeyRefCountZ:     return &GetValueKeyRefCountZ;
    case iKeyThisCollision: return &GetValueKeyThisCollision;
    case dKeydegangular:    return &GetValueKeydegangular;
    case dKeym:             return &GetValueKeym;
    case dKeyreflectivity:  return &GetValueKeyreflectivity;
    case dKeyDivY:          return &GetValueKeyDivY;
    case dKeyDivZ:          return &GetValueKeyDivZ;
    case iKeyColor:         return &GetValueKeyColor;
    case dKeyTime:          return &GetValueKeyTime;
    case dKeyWavelength:    return &GetValueKeyWavelength;
    case dKeyProbability:   return &GetValueKeyProbability;
    case dKeyPositionX:     return &GetValueKeyPositionX;
    case dKeyPositionY:     return &GetValueKeyPositionY;
    case dKeyPositionZ:     return &GetValueKeyPositionZ;
    case dKeyVectorX:       return &GetValueKeyVectorX;
    case dKeyVectorY:       return &GetValueKeyVectorY;
    case dKeyVectorZ:       return &GetValueKeyVectorZ;
    case dKeySpinX:         return &GetValueKeySpinX;
    case dKeySpinY:         return &GetValueKeySpinY;
    case dKeySpinZ:         return &GetValueKeySpinZ;
    default:                return &GetValueNone;
  }
}

void GetKeyName(const int key, char* buf)
{
  switch (key) {
    case iKeyMode:          sprintf(buf, "%s:%d", "Mode"                    , key); break;
    case iKeyMode0:         sprintf(buf, "%s:%d", "Mode0"                   , key); break;
    case iKeyMode5:         sprintf(buf, "%s:%d", "Mode5"                   , key); break;
    case iKeyMode10:        sprintf(buf, "%s:%d", "Mode10"                  , key); break;
    case dKeyRefCount:      sprintf(buf, "%s:%d", "RefCount"                , key); break;
    case dKeyRefCountY:     sprintf(buf, "%s:%d", "RefCountY"               , key); break;
    case dKeyRefCountZ:     sprintf(buf, "%s:%d", "RefCountZ"               , key); break;
    case iKeyThisCollision: sprintf(buf, "%s:%d", "Plane"                   , key); break;
    case dKeydegangular:    sprintf(buf, "%s:%d", "refangle"                , key); break;
    case dKeym:             sprintf(buf, "%s:%d", "m_Ni"                    , key); break;
    case dKeyreflectivity:  sprintf(buf, "%s:%d", "reflectivity"            , key); break;
    case dKeyDivY:          sprintf(buf, "%s:%d", "DivY"                    , key); break;
    case dKeyDivZ:          sprintf(buf, "%s:%d", "DivZ"                    , key); break;
    case iKeyColor:         sprintf(buf, "%s:%d", "Color"                   , key); break;
    case dKeyTime:          sprintf(buf, "%s:%d", "TOF"                     , key); break;
    case dKeyWavelength:    sprintf(buf, "%s:%d", "lambda (Wavelength)"     , key); break;
    case dKeyProbability:   sprintf(buf, "%s:%d", "count rate (Probability)", key); break;
    case dKeyPositionX:     sprintf(buf, "%s:%d", "pos_x"                   , key); break;
    case dKeyPositionY:     sprintf(buf, "%s:%d", "pos_y"                   , key); break;
    case dKeyPositionZ:     sprintf(buf, "%s:%d", "pos_z"                   , key); break;
    case dKeyVectorX:       sprintf(buf, "%s:%d", "dir_x"                   , key); break;
    case dKeyVectorY:       sprintf(buf, "%s:%d", "dir_y"                   , key); break;
    case dKeyVectorZ:       sprintf(buf, "%s:%d", "dir_z"                   , key); break;
    case dKeySpinX:         sprintf(buf, "%s:%d", "sp_x"                    , key); break;
    case dKeySpinY:         sprintf(buf, "%s:%d", "sp_y"                    , key); break;
    case dKeySpinZ:         sprintf(buf, "%s:%d", "sp_z"                    , key); break;
    default:                sprintf(buf, "%s:%d", "None"                    , key); break;
  }
}

