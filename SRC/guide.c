#ifndef GUIDE_PARALLEL_C
#define GUIDE_PARALLEL_C

/********************************************************************************************/
/*  VITESS module 'guide'                                                                   */
/*                                                                                          */
/* This module simulates a guide of various geometries consisting of linear sections        */
/*    (The 'helper threads' option is realized in this module)                              */
/*                                                                                          */
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
/* 2.23  Jun 2010  A. Houben      Additional guide planes by extra rotation of top/bottom or*/
/* (guide.c)                      left/right planes by given angle a < 90deg around x axis  */
/*                                 --> a = 45deg --> octagon shape; a = 60deg --> hexagon   */
/* 3.1   Feb 2010  M. Fromme      helper threads                                            */
/* 3.2   Mar 2011  K. Lieutenant  Gaussian waviness distribution, length of abutment loss   */
/* 3.3   Feb 2012  K. Lieutenant  visualization                                             */
/* 3.4   Jul 2012  K. Lieutenant  exact ellipse calculations                                */
/* 3.5   Nov 2013  K. Lieutenant  m-values as input as alternative to reflectivity files    */
/* 3.6   Nov 2013  K. Lieutenant  corrections for R < 0                                     */
/* 3.7   Jun 2016  A. Houben      Reflection plot options: statistics per plane             */
/* 3.7a  Mar 2017  A. Houben      color filter                                              */
/* 3.7b  May 2018  A. Houben      Correct assignment of reflectivity files for              */
/*                                non-shape-by-file geometries used with add. plane angles  */
/* 3.8   Jul 2019  K. Lieutenant  Tidying up and length compression in visualization        */
/* 3.9   Jul 2019  K. Lieutenant  Curved divergent/convergent guide                         */
/* 3.10  Sep 2019  K. Lieutenant  new standard reflectivities                               */
/* 3.11  Mar 2020  A. Szakal      Calculating prompt gamma production, storing in MCPL file */
/* 3.12  Feb 2021  K. Lieutenant  corrections 'LengthFromFile' and coating from R(Q) file   */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"
#include "string.h"
#include "threadHelper.h"
#include "mcpl.h"
#include "guide.h"
#include "mathfunctions.h"


/******************************/
/** Prototypes               **/
/******************************/
// original code and visualisation
void     OwnInit   (int argc, char *argv[]);           // reads input parameters and sets global variables
void     showAndCompleteSetup(void);                   // evaluates and writes the input parameters
void     processNeutron(int neutron_i, int thread_i);  // processes 1 neutron trajectory
void     OwnCleanup();                                 // does module specific cleanup
void     SetGeometry(char* sColor);                    // fills the structure stGeometry for visualization

ReflFile *GetReflData(double m, char *Filename, FILE *file); // gets reflectivity data by either reading or calculating the data
void     LoadReflFile(ReflFile *pReflFile);                  // loads Reflectivity data from file
void     CalcReflData(ReflFile* pReflFile);                  // calculates reflectivity data from m-value
double   Height    (double length);                          // calculates height of the guide (depending on guide shape option)
double   Width     (double length);                          // calculates width of the guide (depending on guide shape option)
double   GetLengthFromFile(FILE *file);                      // determines length of the guide from the guide shape file

double   PathThroughGuideGravOrder1(int thread_i, Neutron *pThisNeutron, NeutronGuide guide,  // calculates flight pat within the guide
                                    GuidePiece *Pce, ReflCond *RefOut, long iPiece);

char*    FullInName (const char* filename);  // from init.c
char*    FullOutName(const char* filename);  // from init.c


// functions for threading
static void   flushOutput   (void);
static Plane* copyWalls (Plane *g);
static void   addThreadOutput(int thread_i, char *s, int len);
static void   CountMessageThread (int thread_i, VtMsgID code, TotalID id);
static void   MergeThreadBins (BINDATA **bin, int pixcount);
static void   MergeBins ();
static FILE*  tryOpen(const char *fn, const char *s);
static void   allocRdata (ReflFile ***p, int c);


// functions for coating optimization
void     WriteReflParam(ReflCond *RefOut, int thread_i, int Mode, Neutron *pNeutron, GuidePiece *Pce,
                        VtGdeWall ThisCollision, double degangular, double reflectivity);
void     PrintMaximalM(double *RData, long i);
int      FindIndexXY(double Xval, double Yval, int *ibinX, int *ibinY, int iplane);

static int    writeReflPix (BINDATA *pix, int datarange);
static void   writeBindata (void);
static void   applyPixStat(ReflCond *RefOut, double ValProb, NeutronEx *bp, NeutronEx *rp, BINDATA *pix);
static void   addPixStat(BINDATA *bpix,  BINDATA *rpix);
static void   doBinDetail(ReflCond *RefOut, NeutronEx *rp, double ValProb, int ibinX, int ibinY, BINDATA **ppix);

void     DoBin(ReflCond *RefOut, int thread_i);
GetVal   SetValueFunction(const VtPlotPar key);
void     GetKeyName      (const VtPlotPar key, char* buf);

double (*GetValueX)(ReflCond *RefOut, int cNeut) = NULL;
double (*GetValueY)(ReflCond *RefOut, int cNeut) = NULL;
double (*GetProb  )(ReflCond *RefOut, int cNeut) = NULL;


// Function for gamma calculation
void CalcGammaAndNeutron(double incI, double reflectivity, double wl, double mVal, // Calculates the gamma production
                         double mirrMaxM, Escaping_MCPL_particles* pOutParticles); // according to Kolvetov's work


/******************************/
/** Global variables         **/
/******************************/

// REMEMBER: Do not write to a global variable in process_neutron,
// with the only exception of arrays, where the thread index is used
// to restrict write access to a part for that thread only!

extern long iModuleId;


// input parameters
// ----------------
// geometry and coating
VtGdeShape eGuideShapeY=VT_CONSTANT,  // -Y   [-]   shape of the guide in horizontal direction
           eGuideShapeZ=VT_CONSTANT;  // -Z   [-]   shape of the guide in vertical direction
const char *ShapeFileName             // -S   [-]   name of the file containing the sizes of the guide, output or input file
          = "guide_shape.dat";
long   nPieces   = 1;              // -N   [-]   number of guide pieces
double GuideEntrWidth = 0.0,       // -w  [cm]   height of the guide exit
       GuideEntrHeight = 0.0,      // -h  [cm]   height of the guide entrance
       GuideExitWidth = 0.0,       // -W  [cm]   width of the guide exit
       GuideExitHeight = 0.0,      // -H  [cm]   width of the guide entrance
       piecelength = 0.0,          // -p  [cm]   length of 1 piece of the guide
       Radius  = 0.0,              // -R   [m]   radius of a guide that is curved (in horizontal plane)
       D_Foc2Y = 0.0,              // -f  [cm]   pos. behind guide of 2nd focal point in horizontal direction (for elliptic shape)
       D_Foc2Z = 0.0,              // -F  [cm]   pos. behind guide of 2nd focal point in vertical direction (for elliptic shape)
       MValGenL = 1.0,             // -L   [-]   m-value for left wall
       MValGenR = 1.0,             // -Q   [-]   m-value for right wall
       MValGenTB = 1.0;            // -G   [-]   m-value for top and bottom wall

char  *ReflFileNameL = NULL;       // -i   [-]   name of the reflectivity file for the left plane
char  *ReflFileNameR = NULL;       // -I   [-]   name of the reflectivity file for the right plane
char  *ReflFileNameT = NULL;       // -j   [-]   name of the reflectivity file for the top plane
char  *ReflFileNameB = NULL;       // -J   [-]   name of the reflectivity file for the bottom plane

// special features
short  keyAbut   =  0;             // -a   [-]   flag: abutment loss  0: no  1: yes
long   nChannels =  1,             // -b   [-]   number of (vertical) channels
       nColour   = -1;             // -g   [-]   colour necessary for the trajectory to be regarded (-1 means all). Others are passed unchanged to the next module.
short  AddToColor = 0;             // -A   [-]   value added to the color on each reflection
double spacer   = 0.0,             // -s  [cm]   width of the blades dividing the channels
       AbutLen  = 0.0,             // -l  [cm]   area around the connection of guide segments, where neutrons are absorbed
       WaviAmpl = 0.0,             // -r  [deg]  maximal (VT_WAVI_RECT) or RMS (VT_WAVI_GAUSS) deviation from normal of the waviness of the guide surface
       MuScat   = 0.0,             // -M [1/cm]  total macroscopic scattering cross-section
       MuAbs    = 0.0,             // -m [1/cm]  macroscopic absorption cross-section for 1.798 Ang
       rotplane = 0.0;             // -n  [deg]  rotation angle to create additional planes  (see help file for details)
VtWaviDistr eWaviDistr=VT_WAVI_RECT; //    [-]   shape of the waviness distribution   VT_WAVI_RECT   VT_WAVI_GAUSS

// Reflection list
char  *ReflParamFileName=NULL;      // -o   [-]   Name of the file for a reflection list. Giving a filename activiates this option.
int    nReflMinCnt  = 0;            // -e   [-]   Minimum number of reflections within the guide for reflection list output
int    nReflMaxCnt  = 0;            // -E   [-]   Maximum number of reflections within the guide for reflection list output
int    nReflMinCntY = 0;            // -c   [-]   Minimum number of reflections on the horizontal guide for reflection list output
int    nReflMaxCntY = 0;            // -C   [-]   Maximum number of reflections on the horizontal guide for reflection list output
int    nReflMinCntZ = 0;            // -d   [-]   Minimum number of reflections on the vertical guide for reflection list output
int    nReflMaxCntZ = 0;            // -D   [-]   Maximum number of reflections on the vertical guide for reflection list output
VtListVbs eReflVbs=VT_LSTM_NO;      // -v   [-]   Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated.
VtListPar eReflPar=VT_LIST_PASS_LF; /* -O   [-]   Trajectories to be written out:
                                                 1 = only those leaving the guide;
                                                 2 = all successfull reflections; no matter if the trajectory reaches the guide end
                                                 3 = only those with at least one successful scattering event
                                                     (tracjectory may end with an unsuccessfull event)
                                                 4 = all
                                                 a negative number adds a line feed between each trajectory */
// Reflection plot
char      *ReflPlotFileName=NULL;    // -P   [-]   name of the file for writing teflectiond fot plotting. Giving a filename activiates this option.
VtPlotFilt ePlotFilt = VT_PLOT_ALL;  // -B   [-]   Plot filter:  0 = any   1 = only scattered   2 = only died
VtPlotPar KeyX = dKeyPositionX;      // -t   [-]   key defining x parameter for reflection plot
VtPlotPar KeyY = dKeym;              // -T   [-]   key defining y parameter for reflection plot
VtPlotPar KeyProb = dKeyProbability; // -V   [-]   key for f(x,y) for reflection plot
long   nbinsX=1000,                  // -k   [-]   number of X bins in reflection plot
       nbinsY= 100;                  // -K   [-]   number of Y bins in reflection plot
double MinX=    0.0,                 // -x  [var]  lower bound of X range in reflection plot
       MaxX= 1000.0,                 // -X  [var]  upper bound of X range in reflection plot
       MinY=    0.0,                 // -u  [var]  lower bound of Y range in reflection plot
       MaxY=   10.0;                 // -U  [var]  upper bound of Y range in reflection plot

// MCP Loutput
char  *MCPLParamFileName=NULL;     // -Z   [-]   name of the file for the gamma radiation


// Parameters determined from input parameters
// -------------------------------------------
mcpl_outfile_t* pReflMCPLThread;   //      [-]   file for writing the MCPL output
FILE  *pReflParam = NULL;          //      [-]   file for writing each reflection
FILE  *pReflPlot = NULL;           //      [-]   file for writing each reflection as plot
FILE  *pReflL = NULL;              //      [-]   file for describing left plane of guide
FILE  *pReflR = NULL;              //      [-]   file for describing right plane of guide
FILE  *pReflT = NULL;              //      [-]   file for describing top plane of guide
FILE  *pReflB = NULL;              //      [-]   file for describing bottom plane of guide
char sRefFileFullL[512] = "";      //            full path to reflectivity files
char sRefFileFullR[512] = "";
char sRefFileFullT[512] = "";
char sRefFileFullB[512] = "";

long   nSpacers  = 0,              //      [-]   number of blades dividing the (vertical) channels
       nPlanes   = 4;              //      [-]   number of walls (4 for rectangular cross-section)
int    keyAddPlane = 0;            //      [-]   additional planes: 0 = none, 1 = top/bottom, 2 = left/right

double GuideMaxHeight=0.0,         //            max. height and width of guide for elliptic shape
       GuideMaxWidth=0.0,          //
       D_Foc1Y=0.0,                //            pos. of 1st focal point in horizontal direction (for elliptic shape)
       D_Foc1Z=0.0,                //            pos. of 1st focal point in vertical direction (for elliptic shape)
       LcntrY =0.0,                //            centre positions of ellipse
       LcntrZ =0.0,                //
       SAxisY =0.0, LAxisY = 0.0,  //            short and long axes of the ellipses
       SAxisZ =0.0, LAxisZ = 0.0,  //
       AparY = 0.0,                //            factor of quadratic term in parabola
       AparZ = 0.0,                //
       TotalLength=0.0,            //            total length of the guide
       DeltaX=0.0, DeltaY=0.0,     //            length in x- and y-direction of the total guide
       beta=0.0,   beta_ges=0.0;   //            angle of declination between 2 pieces  and of the total guide
double XpceZero = 0.0;             //     [cm]   x position of 1st guide piece
double startPoint=0.0,             //            beginning of the elliptic guide, (not needed here)
       endPoint  =0.0;             //            end of the elliptic guide,       (not needed here)
double AreaY=0.0, AreaZ=0.0;       //    [cm^2]  Approximate area of guide plane
GuidePiece *pPieces;               //            Holds piece Informations. Replaces Xpce, Ypce, Zpce
VtGdeWall eGwExit = GW_EXIT,       //      [-]   structure describing the entrance plane
          eGwInit = GW_INIT;       //      [-]   structure describing the exit plane

extern VectorType BegPosS,         //     [cm]   end position of prev. section = origin of this section in absolute co-ordinate system
                  BegPosM;         //     [cm]   end position of prev. module = origin of this module in absolute co-ordinate system

extern double RotMatrixS[3][3],    //            matrix to rotate from abs. co-ordinate system to co-ordinate system of last section
              RotY, RotZ;          //            hor. and vert. rotation of the local co-ordinate system relative to the absolute one

// Extended FROM FILE
ReflFile* pReflFiles  = {NULL};    //      [-]   structure containing information about a reflectivity file
long      nReflFiles  = 0;         //      [-]   number of reflectivity files read or calculated
long      maxReflFiles= 0;         //      [-]   maximum number of reflectivity files (used for memory allocation)

// Reflection Plot variables
double *bpostX,  *bpostY;          //      [-]   limits of the bins
BINDATA **binXY, **binX, **binY;

// MCPL related globals
mcpl_particle_t* particleT;        //            Each Thread have an own temporary particle*/
double** RotMatrixSThread;         //            The thread's own rotation vector*/
double** BegPosSThread;            //            The thread's own position vector*/
short    sMCPLWrite=0;             //            Write MCPL file? 0: NO; 1: only gamma; 2: only neutron; 3: gamma and neutron */
Escaping_MCPL_particles* pMCPLparticles; //      for returning results from CalcGammaAndNeutron() func.
//SERIAL double RotMatrixSThread[3][3];
//SERIAL double BegPosSThread[3];

// Constants
// ---------
// Gamma energies for the gamma production output of MCPL
double gamma_energy [] = {0.15, 0.2, 0.3, 0.4, 0.5, 0.6, 0.8, 1.0,   1.5,
                          2.0,  3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0};
double gamma_fractionNi[]={0.003722, 0.015513, 0.048432, 0.047485, 0.198059, 0.002809,
                           0.003142, 0.06579,  0.054534, 0.028872, 0.063882, 0.050286,
                           0.035567, 0.080326, 0.156571, 0.130355, 0.534153, 0.000558,
                           0.000260};
double gamma_fractionTi[]={0.009326, 0.0,      0.001938, 0.302632, 0.000882, 0.000668,
                           0.000914, 0.023046, 0.947104, 0.232257, 0.086418, 0.162878,
                           0.11209, 0.013419, 0.0875051, 0.009083, 0.001923, 0.002189,
                           0.000332};


/******************************/
/** Static variables         **/
/******************************/

// We need a separate guide variable per thread
// because each writes to it
static NeutronGuide TGuide[MAXWORKER];

// These variables are declared static globally,
// because they are written in main and read in processNeutron:
static ReflCond GRefOut[MAXWORKER];
static double GdXpce,  // length of a piece incl. diff. in y- or z- position
  GdDelY, GdDelZ,      // difference in y- or z-position of a piece
  RotMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}},
  dCosBetH = 1.0,
  dSinBetH = 0.0;     // cos(beta/2) and sin(beta/2)
static NeutronGuide Guide;

// storage for temp. output of threads, if reflections are to be printed to a file
static int perThreadMaxOut, rThreadOCount[MAXWORKER];
static char *rThreadO[MAXWORKER];

// avoid locks, so store errors per thread
static int     C_M[2][MAXWORKER];
static TotalID C_I[2][MAXWORKER];



/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  int i=0, needPreRand=0;

  /******************************************/
  /** Initialisation and parameter input   **/
  /******************************************/
  _eModule=MCN_GUIDE;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "3.12a");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp     = TRUE;

  // allocate for planes + exit plane
  if (! (Guide.Wall = (Plane*) calloc(nPlanes+1, sizeof(Plane))))
    myExit("ERROR: Not enough memory for guide data!\n");

  showAndCompleteSetup();

  TGuide[0].Wall = Guide.Wall;
  for (i=1; i<=NThreads; i++)
    TGuide[i].Wall = copyWalls(Guide.Wall);

  // needPreRand value:
  // 100 would mean pre-compute 100 random numbers per neutron
  // -1 (any negative value) means use an individual random number generator per thread
  needPreRand = ((NThreads && WaviAmpl) || sMCPLWrite) ? -1 : 0;

  if (NThreads > 0 && pReflParam)
  {
    int allsize;
    char *buf;
    allsize = BufferSize*256; // 256 byte per neutron
    if (! (buf = (char*) malloc(allsize)))
      myExit("ERROR: Not enough memory for reflection data!\n");
    perThreadMaxOut = allsize / NThreads;
    for (i=0; i<NThreads; i++)
    {
      rThreadO[i] = buf;
      buf += perThreadMaxOut;
    }
    processPipedNeutronsWithOutput(NThreads, processNeutron, flushOutput, 1, needPreRand);
  }
  else
  {
    processPipedNeutrons(NThreads, processNeutron, 1, needPreRand);
  }

  /* set description for instrument plot */
  beta_ges = (nPieces-1)*beta;
  if (Radius != 0.0)
  {
    DeltaX = Radius*sin(beta_ges)       + 0.5*piecelength*(cos(beta_ges)+1.0);
    DeltaY = Radius*(1.0-cos(beta_ges)) + 0.5*piecelength* sin(beta_ges);
  }
  else
  {
    DeltaX = TotalLength;
    DeltaY = 0.0;
  }

  SetGeometry("green");                         // write geometry data for visualization
  OwnCleanup();
  Cleanup(DeltaX,DeltaY,0.0, beta_ges, 0.0);

  return 0;
}


/******************************/
/** General subroutines      **/
/******************************/

/******************************************************************************/
// 'OwnInit' reads the input parameter and checks the values
/******************************************************************************/
void OwnInit   (int argc, char *argv[])
{
  long  i,j;
  char *arg = NULL, sLine[512];
  FILE *pFile = NULL;
  char  sRefFileL[512] = "", sRefFileR[512] = "", sRefFileT[512] = "", sRefFileB[512] = "";
  double MValueL = 0.0, MValueR = 0.0, MValueT = 0.0, MValueB = 0.0;
  ReflFile *pRefFileLast;
  double bintervalX = 1.0, bintervalY = 1.0;
  int ibinX, ibinY;
  int k;
  char *sFullName = NULL;

  MsgInit();
  InitVector(BegPosM);
  InitVector(BegPosS);

  // guide parameter character usage:
  //free                                                                                                  y
  //used a A b B c C d D e E f F g G h H i I j J k K l L m M n  N o O p P q Q r R s S t T u U v V w W x X   Y z Z

  for (i=1; i<argc; i++)
  {
    if (argv[i][0] == '+') continue;  //these have been processed by Init()

    arg = &argv[i][2];
    switch (argv[i][1])
    {
    case 'i':  /* left plane */
      pReflL = tryOpen( (ReflFileNameL = arg), "coating of left plane");
      break;
    case 'I':  /* right plane */
      pReflR = tryOpen( (ReflFileNameR = arg), "coating of right plane");
      break;
    case 'j':    /* top plane */
      pReflT = tryOpen( (ReflFileNameT = arg), "coating of top plane");
      break;
    case 'J':    /* bottom plane */
      pReflB = tryOpen( (ReflFileNameB = arg), "coating of bottom plane");
      break;
    case 'o':  /* Reflection parameter writeout */
      if ((pReflParam = OpenInputFile(arg, FALSE, "w")))
      {
        fprintf(pReflParam,
                "#____ID____ Scattered plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z\n"
                "#    1           2      3       4      5          6          7        8     9    10     11      12         13           14         15         16         17        18        19       20   21   22 \n");
        ReflParamFileName=arg;
      }
      break;

    case 'z':
      //MCPL realted starts
      sMCPLWrite=3;
      //MCPL output
      pReflMCPLThread = (mcpl_outfile_t*) malloc((NThreads+1)*sizeof(mcpl_outfile_t));
      particleT = (mcpl_particle_t*) malloc((NThreads+1)*sizeof(mcpl_particle_t));
      pMCPLparticles  = (Escaping_MCPL_particles*) malloc((NThreads+1)*sizeof(Escaping_MCPL_particles));

      //Initialize RotMatrixM and BegPosM
      double BlnLen;
      ReadInstrData(iModuleId, BegPosM, &BlnLen, &RotZ, &RotY, sInstrInfIn);
      CopyVector(BegPosM, BegPosS);
      FillRMatrixZY(RotMatrixM, RotY, RotZ);

      //allocate memory for RotMatrixSThread and BegPosSThread
      //PARALLLEL
      RotMatrixSThread = (double**) malloc(3*(NThreads+1)*sizeof(double*));
      for (k=0; k<3*(NThreads+1);k++)
          RotMatrixSThread[k] = (double*) malloc(3*sizeof(double));
      //allocate memory for RotMatrixSThread and BegPosSThread
      BegPosSThread = (double**) malloc((NThreads+1)*sizeof(double*));
      for (k=0; k<(NThreads+1);k++)
          BegPosSThread[k] = (double*) malloc(3*sizeof(double));

      MCPLParamFileName=arg;
      char mcpl_tmp_filename[1024];
      sFullName = FullOutName(arg);
      for (k = 0; k <= NThreads; ++k) {
        if (k == 0) {
          sprintf(mcpl_tmp_filename, "%s.mcpl", sFullName);
          remove(mcpl_tmp_filename);
        } else {
          sprintf(mcpl_tmp_filename, "%s.tmp%d.mcpl", sFullName, k);
        }

        pReflMCPLThread[k] = mcpl_create_outfile(mcpl_tmp_filename);

        // Init the Threads own tmp particle
        memset(particleT + k, '\0', sizeof(mcpl_particle_t));
      }
      free(sFullName);
      sFullName = NULL;
      //End of MCPL related code
      break;

    case 'O':
      eReflPar = (VtListPar) atoi(arg); /* Trajectories to be written out:
                                   1 = only those leaving the guide;
                                   2 = all successfull reflections; no matter if the trajectory reaches the guide end
                                   3 = only those with at least one successful scattering event (tracjectory may end with an unsuccessfull event)
                                   4 = all
                                   a negative number adds a line feed between each trajectory */
      break;
    case 'B':
      ePlotFilt = (VtPlotFilt) atoi(arg); /* Trajectories to be binned:
                                   0 = all
                                   1 = only scattered;
                                   2 = only dies
                                   WARNING: This is influenced by keyReflParam too. Set keyReflParam to all to get all events.
                                */
      break;
    case 'P':  /* Reflection parameter plot */
      if ((pReflPlot = OpenInputFile(arg, FALSE, "w")))
      {
        fprintf(pReflPlot,
                "#   X          Y        counts   Mode    0       5       10    RefCount RCy RCz  ____ID____ plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc  color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z   WeightSum\n"
                "#   1          2         3=C     4=S    5=C     6=C     7=C    8=A      9=A 10=A  11=1N     12=1N 13=A      14=A  15=A           16=A     17=A  18=1N  19=A   20=A   21=A     22=S           23=A       24=A       25=A       26=A      27=A      28=A      29=A 30=A 31=A     32=S   \n"
                "#1N = defined by first neutron in bin; A = avaraged; S = summed up; C = events counted; Mode: 0=Scattered, 5=No interaction, 10=Died\n"
                "#Data blocks: ==Data== holds 2D binning, ==DataX== and ==DataY== are 1D binnings, ==Data<Number>==, ==DataX<Number>==, ==DataY<Number>== the same analysis, but done seperately per guide plane with 1 = top, 2 = bottom, 3 = left, 4 = right, >=5 same order for each plane created by the additional planes option\n");
        ReflPlotFileName=arg;
      }
      break;
    case 'v':    /* Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated. */
      eReflVbs = (VtListVbs) atoi(arg);
      break;
    case 'e':    /* Minimum number of reflections within the guide for reflection list output */
      nReflMinCnt = atoi(arg);
      break;
    case 'E':    /* Maximum number of reflections within the guide for reflection list output */
      nReflMaxCnt = atoi(arg);
      break;
    case 'c':    /* Minimum number of reflections on the horizontal guide for reflection list output */
      nReflMinCntY = atoi(arg);
      break;
    case 'C':    /* Maximum number of reflections on the horizontal guide for reflection list output */
      nReflMaxCntY = atoi(arg);
      break;
    case 'd':    /* Minimum number of reflections on the vertical guide for reflection list output */
      nReflMinCntZ = atoi(arg);
      break;
    case 'D':    /* Maximum number of reflections on the vertical guide for reflection list output */
      nReflMaxCntZ = atoi(arg);
      break;

    case 'S':    /* shape file */
      ShapeFileName=arg;
      break;

    case 'h':
      GuideEntrHeight = atof(arg);
      break;
    case 'H':
      GuideExitHeight = atof(arg);
      break;
    case 'w':
      GuideEntrWidth = atof(arg);
      break;
    case 'W':
      GuideExitWidth = atof(arg);
      break;

    case 'L':
      MValGenL  = atof(arg);
      break;
    case 'Q':
      MValGenR  = atof(arg);
      break;
    case 'G':
      MValGenTB = atof(arg);
      break;

    case 'f':
      D_Foc2Y = atof(arg);      // distance of focus point behind guide exit
      break;
    case 'F':
      D_Foc2Z = atof(arg);      // distance of focus point behind guide exit
      break;

    case 'n':
      rotplane = atof(arg); /* Additional planes: rot angle */
      if (fabs(rotplane) >= 90.0)
      {
        rotplane = 0.0;
      }
      else if (fabs(rotplane) > 0.0)
      {
        double rot;
        keyAddPlane = rotplane > 0.0 ? 1 : 2;
        rot = rotplane;
        while (fabs(rot) < 90.0)
        {
          rot += rotplane;
          nPlanes += 4;
        }
        eGwExit = (VtGdeWall)  nPlanes;
        eGwInit = (VtGdeWall) (nPlanes+1);
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
      eGuideShapeY = (VtGdeShape) atol(arg); /*                 1: (linearly) converging or diverging */
      break;                    /*                 2: curved (circular)                  */
    case 'Z':                   /*                 3: parabolic                          */
      eGuideShapeZ = (VtGdeShape) atol(arg); /*                 4: elliptic                           */
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
      spacer = atof(arg); /* width of the blades dividing the channels in cm */
      break;

    case 'a':
      keyAbut = (short) atoi(arg); /* key for abutment loss  0: no (default), 1: yes  */
      break;
    case 'l':
      AbutLen = atof(arg);  /* area around the connection of guide segments where neutrons are absorbed */
      break;
    case 'A':
      AddToColor = atoi(arg); /* value added to the color for each reflection  */
      break;
    case 'q':
      eWaviDistr = (VtWaviDistr) atoi(arg);  /* enum: waviness distribution: 1: rectangular (default), 2: Gaussian  */
      break;
    case 'r':
      WaviAmpl  =  atof(arg)*M_PI/180.0;   /* Maximal or RMS angle of deviation from normal in rad */
      WaviAmpl  =  tan(WaviAmpl);
      break;

    case 'k':
      nbinsX = atol(arg); /* number of X bins */
      break;
    case 'K':
      nbinsY = atol(arg); /* number of Y bins */
      break;

    case 'x':
      MinX = atof(arg);   /* lower bound of X range */
      break;
    case 'X':
      MaxX = atof(arg);   /* upper bound of X range */
      break;

    case 'u':
      MinY = atof(arg);   /* lower bound of Y range */
      break;
    case 'U':
      MaxY = atof(arg);   /* upper bound of Y range */
      break;

    case 't':    /* Key for x parameter */
      KeyX = (VtPlotPar) atoi(arg);
      break;
    case 'T':    /* Key for y parameter */
      KeyY = (VtPlotPar) atoi(arg);
      break;

    case 'V':    /* Probability weighting key */
      KeyProb = (VtPlotPar) atoi(arg);
      break;

    case 'g':
      nColour = atol(arg);       /*  excludes all neutrons with diff. Colour, if nColour >= 0 */
      break;

    default:
      myExit1("ERROR: Unknown command option: %s\n",argv[i]);
    }
  }

  /* Input checks */
  /* ------------ */

  /* consistency of abutment loss parameters */
  if (keyAbut==ON  && AbutLen==0.0) AbutLen=0.5;
  if (keyAbut==OFF && AbutLen > 0.0)
    Error("Inconsistent abutment loss parameters");

  /* combination: orientation - shape */
  if (eGuideShapeZ==VT_CURVED || eGuideShapeZ==VT_LIN_CURV)
    Error("Vertical curving of the guide not supported");

  /* Radius  */
  if (eGuideShapeY==VT_CURVED || eGuideShapeY==VT_LIN_CURV)
  {
    if (Radius==0.0)
    { beta=0.0;
      Error("Radius of curvature is missing");
    }
    else
    { // curved guide
      beta = 2.0*asin(piecelength/(2.0*Radius));
      dCosBetH = cos(beta/2.0);
      dSinBetH = sin(beta/2.0);
      FillRotMatrixZ(RotMatrix, beta);
    }
  }
  else
  { if (Radius!=0.0)
      Error("Curvature of guide only supported in options 'curved' and 'linear+curved', please delete radius or switch to one of these options");
  }

  /* Exit and entrance size */
  if (eGuideShapeZ==VT_CONSTANT)
  {
    if (GuideExitHeight != GuideEntrHeight)
      Error("In guides of constant height, exit and entrance height have to be equal");
  }
  else if (GuideExitHeight== 0.0)
  {
    Error("You must enter the height of the guide exit");
  }

  if (eGuideShapeY==VT_CONSTANT || eGuideShapeY==VT_CURVED)
  {
    if (GuideExitWidth != GuideEntrWidth)
      Error("In curved and constant guides, exit and entrance width have to be equal");
  }
  else if (GuideExitWidth == 0.0)
  {
    Error("You must enter the width of the guide exit");
  }

  /* number of pieces */
  if (eGuideShapeZ==VT_PARABOLIC || eGuideShapeZ==VT_ELLIPTIC ||
      eGuideShapeY==VT_PARABOLIC || eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_CURVED)
  { if (nPieces <= 1)
      Error("More than 1 piece is necessary for this shape");
  }

  if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
  {
    pFile = OpenInputFile(ShapeFileName, FALSE, "r");
    if (pFile)
    {
      nPieces      = LinesInFile(pFile) - 1;
      TotalLength  = GetLengthFromFile(pFile);
      maxReflFiles = (nPieces+1) * 4;
    }
    else
    { myExit1("ERROR: Input file %s could not be read !\n", ShapeFileName);
    }
  }
  else
  {
    TotalLength  = nPieces*piecelength;
    maxReflFiles = 4;
  }

  pReflFiles = (ReflFile*) calloc(maxReflFiles, sizeof(ReflFile));
  if (!pReflFiles)
    myExit("ERROR: Not enough memory for reflecitvity data!\n");
  memset(pReflFiles, '\0', maxReflFiles * sizeof(ReflFile));

  /* bottom plane */
  if (pReflB == NULL && pReflT != NULL)
  {
    pReflB = pReflT;
    ReflFileNameB = ReflFileNameT;
    fprintf(LogFilePtr,"\nNOTE: coating of top wall also used for bottom wall \n");
  }

  /* Init bin arrays */
  GetValueX = SetValueFunction(KeyX);
  GetValueY = SetValueFunction(KeyY);
  GetProb   = SetValueFunction(KeyProb);

  if (pReflPlot)
  {
    bpostX = (double*) calloc(nbinsX+1, sizeof(double));
    bpostY = (double*) calloc(nbinsY+1, sizeof(double));
    // allocate storage for each thread
    binX    = (BINDATA**) calloc((NThreads+1)*(nbinsX+1)*(nPlanes+1), sizeof(BINDATA*));
    binY    = (BINDATA**) calloc((NThreads+1)*(nbinsY+1)*(nPlanes+1), sizeof(BINDATA*));
    binXY   = (BINDATA**) calloc((NThreads+1)*(INDEX(nbinsX, nbinsY, nPlanes)+1), sizeof(BINDATA*));

    bintervalX = (MaxX - MinX) / (double)nbinsX;
    bintervalY = (MaxY - MinY) / (double)nbinsY;

    for(ibinX = 0; ibinX<=nbinsX; ibinX++)
      bpostX[ibinX] = MinX + bintervalX*ibinX;
    for(ibinY = 0; ibinY<=nbinsY; ibinY++)
      bpostY[ibinY] = MinY + bintervalY*ibinY;
  }

  /* Calculation of height, width and channel-width of beginning and end of pieces */
  pPieces = (GuidePiece*) calloc(nPieces+1, sizeof(GuidePiece));

  if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
  {
    pRefFileLast = GetReflData(MValGenL, ReflFileNameL, pReflL);

    for (j=0; j <= nPieces; j++)
    {
      ReadLine(pFile, sLine, sizeof(sLine)-1);

      sRefFileL[0] = sRefFileR[0] = sRefFileT[0] = sRefFileB[0] = '\0';
      sscanf(sLine, "%lf %lf %lf %s %s %s %s", &pPieces[j].Xpce, &pPieces[j].Ypce, &pPieces[j].Zpce,
             (char *)&sRefFileL, (char *)&sRefFileR, (char *)&sRefFileT, (char *)&sRefFileB);
      MValueL = atof(sRefFileL); if (MValueL > 0.0) strcpy(sRefFileL, " ");
      MValueR = atof(sRefFileR); if (MValueR > 0.0) strcpy(sRefFileR, " ");
      MValueT = atof(sRefFileT); if (MValueT > 0.0) strcpy(sRefFileT, " ");
      MValueB = atof(sRefFileB); if (MValueB > 0.0) strcpy(sRefFileB, " ");

      pPieces[j].Xpce *= 100.0;
      if (j==0) XpceZero = pPieces[0].Xpce;
      pPieces[j].Xpce -= XpceZero;
      if (eGuideShapeY==VT_FROM_FILE) pPieces[j].Ypce *= 0.5;
      else pPieces[j].Ypce =  Width(pPieces[j].Xpce)/2.0;
      if (eGuideShapeZ==VT_FROM_FILE) pPieces[j].Zpce *= 0.5;
      else pPieces[j].Zpce = Height(pPieces[j].Xpce)/2.0;
      pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
      if (pPieces[j].Wchan <= 0.0)
        Error("Geometry impossible. Channel width gets zero (or less)");
      if (j>0)
      {
        AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }

      allocRdata(&(pPieces[j].RData), nPlanes);

      /* Calculate Area for this reflectivity file; only valid for squared guide */
      if (j > 0)
      {
        if (pPieces[j-1].RData[GW_LEFT])
          pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_RIGHT])
          pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_TOP])
          pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_BOTTOM])
          pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }

      /* Either load standard reflectivity file or use userdefined one */
      switch (sRefFileL[0]) {
      case ':':
        // "use previous"
        pPieces[j].RData[GW_LEFT] = pRefFileLast;
        break;
      case '\0':
        // standard files
        pPieces[j].RData[GW_LEFT] = GetReflData(MValGenL, ReflFileNameL, pReflL);
        break;
      case ' ':
        // m-value from file
        pPieces[j].RData[GW_LEFT] = GetReflData(MValueL, NULL, NULL);
        break;
      default:
        TotalPath(sRefFileFullL, sRefFileL, "", IN_DIR);
        // file from file
        pPieces[j].RData[GW_LEFT] = GetReflData(0.0, sRefFileFullL, NULL);
        pRefFileLast = pPieces[j].RData[GW_LEFT];
        break;
      }

      switch (sRefFileR[0]) {
      case ':':
        pPieces[j].RData[GW_RIGHT] = pRefFileLast;
        break;
      case '\0':
        pPieces[j].RData[GW_RIGHT] = GetReflData(MValGenR, ReflFileNameR, pReflR);
        break;
      case ' ':
        pPieces[j].RData[GW_RIGHT] = GetReflData(MValueR, NULL, NULL);
        break;
      default:
        TotalPath(sRefFileFullR, sRefFileR, "", IN_DIR);
        pPieces[j].RData[GW_RIGHT] = GetReflData(0.0, sRefFileFullR, NULL);
        pRefFileLast = pPieces[j].RData[GW_RIGHT];
        break;
      }

      switch (sRefFileT[0]) {
      case ':':
        pPieces[j].RData[GW_TOP] = pRefFileLast;
        break;
      case '\0':
        pPieces[j].RData[GW_TOP] = GetReflData(MValGenTB, ReflFileNameT, pReflT);
        break;
      case ' ':
        pPieces[j].RData[GW_TOP] = GetReflData(MValueR, NULL, NULL);
        break;
      default:
        TotalPath(sRefFileFullT, sRefFileT, "", IN_DIR);
        pPieces[j].RData[GW_TOP] = GetReflData(0.0, sRefFileFullT, NULL);
        pRefFileLast = pPieces[j].RData[GW_TOP];
        break;
      }

      switch (sRefFileB[0]) {
      case ':':
        pPieces[j].RData[GW_BOTTOM] = pRefFileLast;
        break;
      case '\0':
        pPieces[j].RData[GW_BOTTOM] = GetReflData(MValGenTB, ReflFileNameB, pReflB);
        break;
      case ' ':
        pPieces[j].RData[GW_BOTTOM] = GetReflData(MValueR, NULL, NULL);
        break;
      default:
        TotalPath(sRefFileFullB, sRefFileB, "", IN_DIR);
        pPieces[j].RData[GW_BOTTOM] = GetReflData(0.0, sRefFileFullB, NULL);
        pRefFileLast = pPieces[j].RData[GW_BOTTOM];
        break;
      }

      /* for additional planes use pointers. WARNING please use same order of assingment as in other functions */
      if (nPlanes > 4)
      {
        double rot = rotplane;
        int cPlane = GW_RIGHT;
        while (fabs(rot) < 90.0 && cPlane < eGwExit)
        {
          switch (keyAddPlane)
          {
            case 1:
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_TOP];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_TOP];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_BOTTOM];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_BOTTOM];
              break;
            case 2:
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_LEFT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_LEFT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_RIGHT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_RIGHT];
              break;
          }
          rot += rotplane;
        }
      }
    }

    TotalLength = RoundP(pPieces[nPieces].Xpce - pPieces[0].Xpce, 7);
    GuideEntrWidth = pPieces[0].Ypce*2.;
    GuideEntrHeight = pPieces[0].Zpce*2.;
    GuideExitWidth = pPieces[nPieces].Ypce*2.;
    GuideExitHeight = pPieces[nPieces].Zpce*2.;
    piecelength  = TotalLength / (double)nPieces; /* Use piecelength with care in the case of nonequidistant planes */
  }
  else
  {
    pFile = OpenOutputFile(ShapeFileName, FALSE, "w+");

    for (j=0; j <= nPieces; j++)
    {
      pPieces[j].Xpce  = j*piecelength;
      pPieces[j].Ypce  = Width (pPieces[j].Xpce)/2.0;
      pPieces[j].Zpce  = Height(pPieces[j].Xpce)/2.0;
      pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
      if (pPieces[j].Wchan <= 0.0)
        Error("Geometry impossible. Channel width gets zero (or less)");
      if (pFile)
      {
        if (j==0)
        {
          fprintf(pFile,
                  "# length [m]  width [cm]  height [cm] \n"
                  "#-------------------------------------\n");
        } else
        {
          AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
          AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        }
        //fprintf(pFile, "%10.4f  %10.6f  %10.6f  %10.6f\n", pPieces[j].Xpce/100.0, 2.0*pPieces[j].Ypce, 2.0*pPieces[j].Zpce, Max(0.0, Min(beta_ges, beta*(j-0.5)*180.0/M_PI)));
        fprintf(pFile, "%10.4f  %10.6f  %10.6f\n", pPieces[j].Xpce/100.0, 2.0*pPieces[j].Ypce, 2.0*pPieces[j].Zpce);
      }

      allocRdata(&(pPieces[j].RData), nPlanes);

      /* Calculate Area for this reflectivity file; only valid for squared guide */
      if (j > 0)
      {
        if (pPieces[j-1].RData[GW_LEFT])
          pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_RIGHT])
          pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_TOP])
          pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
        if (pPieces[j-1].RData[GW_BOTTOM])
          pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
      }

      /* Load reflectivity file or reuse already loaded file */
      pPieces[j].RData[GW_LEFT]   = GetReflData(MValGenL,  ReflFileNameL, pReflL);
      pPieces[j].RData[GW_RIGHT]  = GetReflData(MValGenR,  ReflFileNameR, pReflR);
      pPieces[j].RData[GW_TOP]    = GetReflData(MValGenTB, ReflFileNameT, pReflT);
      pPieces[j].RData[GW_BOTTOM] = GetReflData(MValGenTB, ReflFileNameB, pReflB);

      if (nPlanes > 4)
      {
        double rot = rotplane;
        int cPlane = GW_RIGHT;
        while (fabs(rot) < 90.0 && cPlane < eGwExit)
        {
          switch (keyAddPlane)
          {
            case 1:
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_TOP];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_TOP];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_BOTTOM];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_BOTTOM];
              break;
            case 2:
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_LEFT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_LEFT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_RIGHT];
              pPieces[j].RData[++cPlane] = pPieces[j].RData[GW_RIGHT];
              break;
          }
          rot += rotplane;
        }
      }
    }
  }
  if (pFile)
    fclose(pFile);
}


/******************************************************************************/
// 'showAndCompleteSetup' evaluates and writes the input parameters
/******************************************************************************/
void showAndCompleteSetup()
{
  int i;

  fprintf(LogFilePtr, "\nTotal length of guide   : %8.3f  m\n", TotalLength/100.);
  if (nChannels > 1)
    fprintf(LogFilePtr, " with %ld channels", nChannels);
  fprintf(LogFilePtr, "Width x Height          : %8.4f  x %7.4f cm^2", GuideEntrWidth, GuideEntrHeight);
  if (GuideExitWidth != GuideEntrWidth || GuideExitHeight != GuideEntrHeight)
    fprintf(LogFilePtr, " -> %7.4f x %7.4f cm^2", GuideExitWidth, GuideExitHeight);
  fprintf(LogFilePtr, "\n\nHorizontal: ");

  switch (eGuideShapeY)
  {
    case VT_ELLIPTIC:
      fprintf(LogFilePtr, "elliptic shape\n"
              " maximal width     :%9.4f cm  at %10.5f m from entrance\n", GuideMaxWidth, LcntrY/100.);
      fprintf(LogFilePtr, " long half axis    :%10.5f m\n", LAxisY/100.);
      fprintf(LogFilePtr, " focal points      :%10.5f m from entrance, %10.5f m after exit\n", D_Foc1Y/100., D_Foc2Y/100.);
      break;
    case VT_PARABOLIC:
      fprintf(LogFilePtr, "parabolic shape    : focal point:%10.5f m after exit\n",
              (sq(GuideEntrWidth)*AparY-TotalLength-1.0/AparY/16.0) / 100.);
      break;
    case VT_CURVED  :
      fprintf(LogFilePtr, "curved guide\n");
      break;
    case VT_LIN_CURV:
      fprintf(LogFilePtr, "linearly diverging or converging curved guide\n");
      break;
    case VT_FROM_FILE:
      fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
      fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
      break;
    case VT_CONSTANT:
    case VT_LINEAR  :
      if      (GuideExitWidth > GuideEntrWidth) fprintf(LogFilePtr, "linearly diverging\n");
      else if (GuideExitWidth < GuideEntrWidth) fprintf(LogFilePtr, "linearly converging\n");
      else    fprintf(LogFilePtr, "constant width\n");
      break;
  }
  fprintf(LogFilePtr, " area (top+bottom) :%8.3f m^2\n", AreaY*2./1e4);

  fprintf(LogFilePtr, "Vertical  : ");
  switch (eGuideShapeZ)
  {
    case VT_ELLIPTIC:
      fprintf(LogFilePtr, "elliptic shape\n"
              " max. height       :%9.4f cm  at %10.5f m from entrance\n", GuideMaxHeight, LcntrZ/100.);
      fprintf(LogFilePtr, " long half axis    :%10.5f m\n", LAxisZ/100.);
      fprintf(LogFilePtr, " focal points      :%10.5f m from entrance, %10.5f m after exit\n", D_Foc1Z/100., D_Foc2Z/100.);
      break;
    case VT_PARABOLIC:
      fprintf(LogFilePtr, "parabolic shape    : focal point:%10.5f m after exit\n",
              (sq(GuideEntrHeight)*AparZ-TotalLength-1.0/AparZ/16.0)/100.);
      break;
    case VT_CURVED  :
    case VT_LIN_CURV:
      fprintf(LogFilePtr, "WARNING: vertically curved guide not supported\n");
      break;
    case VT_FROM_FILE:
      fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
      fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
      break;
    case VT_CONSTANT:
    case VT_LINEAR  :
      if      (GuideExitHeight > GuideEntrHeight) fprintf(LogFilePtr, "linearly diverging\n");
      else if (GuideExitHeight < GuideEntrHeight) fprintf(LogFilePtr, "linearly converging\n");
      else    fprintf(LogFilePtr, "constant height\n");
      break;
    default: ;
  }
  fprintf(LogFilePtr, " area (left+right) :%8.3f m^2\n", AreaZ*2./1e4);

  if (Radius != 0.0)  // curved guide
    fprintf(LogFilePtr,"\n%ld kink(s) with an angle of %8.4f deg  each\n", nPieces-1, 180.0/M_PI*beta);

  for(i=0; i < nReflFiles; i++)
  {
    if (pReflFiles[i].filename)
    {
      fprintf(LogFilePtr,"\nReflectivity file  : %s\n", pReflFiles[i].filename);
      if (pReflFiles[i].pfile)
      {
        PrintMaximalM(pReflFiles[i].Rdata, pReflFiles[i].maxdata);
      } else {
        Error("Reflectivity file was not found!\n");
      }
      fprintf(LogFilePtr,  " surface area      :%8.3f m^2\n", pReflFiles[i].area/1.e4);
    }
  }

  if (keyAddPlane != 0)
    fprintf(LogFilePtr,"\nAdditional planes used. WARNING: All areas are calculated with rectangular cross section! \n");

  if (AbutLen > 0.0)
    fprintf(LogFilePtr,"\nAbutment loss area :%8.3f cm\n", AbutLen);

  if (WaviAmpl == 0.0)
  {
    fprintf(LogFilePtr,"The walls have no waviness\n");
  }
  else
  {
    fprintf(LogFilePtr,"The walls have a waviness of %10.3e deg\n", atan(WaviAmpl)*180.0/M_PI);
    if (eWaviDistr==VT_WAVI_GAUSS)
      fprintf(LogFilePtr,"rms Gaussian distribution\n");
    else
      fprintf(LogFilePtr,"max. rectangular distribution\n");
  }

  /****************************************************************************************/
  /* Set up the parameters of the planes from the input data...                           */
  /****************************************************************************************/
  {
    double Length1, Length2; // length of a piece incl. diff. in z- or y-position resp.

    GdXpce = piecelength;
    GdDelY = (GuideExitWidth  - GuideEntrWidth)  / (2.0*nPieces);
    GdDelZ = (GuideExitHeight - GuideEntrHeight) / (2.0*nPieces);
    Length1 = sqrt(GdXpce*GdXpce+GdDelZ*GdDelZ);
    Length2 = sqrt(GdXpce*GdXpce+GdDelY*GdDelY);

    /* top plane */
    Guide.Wall[GW_TOP].A =  GdDelZ/Length1;
    Guide.Wall[GW_TOP].B =  0.0;
    Guide.Wall[GW_TOP].C = -GdXpce/Length1;
    Guide.Wall[GW_TOP].D = -Guide.Wall[GW_TOP].C * (GuideEntrHeight/2.0);

    /* bottom plane */
    Guide.Wall[GW_BOTTOM].A = -GdDelZ/Length1;
    Guide.Wall[GW_BOTTOM].B =  0.0;
    Guide.Wall[GW_BOTTOM].C = -GdXpce/Length1;
    Guide.Wall[GW_BOTTOM].D =  Guide.Wall[GW_BOTTOM].C * (GuideEntrHeight/2.0);

    /* left plane */
    Guide.Wall[GW_LEFT].A = -GdDelY/Length2;
    Guide.Wall[GW_LEFT].B =  GdXpce/Length2;
    Guide.Wall[GW_LEFT].C =  0.0;
    Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B * (GuideEntrWidth/2.0);

    /* right plane */
    Guide.Wall[GW_RIGHT].A =  GdDelY/Length2;
    Guide.Wall[GW_RIGHT].B =  GdXpce/Length2;
    Guide.Wall[GW_RIGHT].C =  0.0;
    Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B * (GuideEntrWidth/2.0);

    /* exit plane */
    Guide.Wall[eGwExit].A =  1.0;
    Guide.Wall[eGwExit].B =  0.0;
    Guide.Wall[eGwExit].C =  0.0;
    Guide.Wall[eGwExit].D = -GdXpce;
  }
}


/******************************************************************************/
// Callback routine 'processNeutron' is responsible to process neutron i;
// thread_i indicates the tread which is performing, in case of serial execution this is thread 0
/******************************************************************************/
void processNeutron(int neutron_i, int thread_i)
{
  ReflCond *PRefOut;
  long   j, kChan;
  short  test;
  Plane  *gW;
  Neutron *myneutron;
  NeutronGuide guide;
  double TimeOF1=0.0, TimeOF2=0.0,
         dDelYr =0.0, dDelYl=0.0,     // difference in y-position of the left and right side of a piece resp.
         Length1=0.0,                 // length of a piece incl. diff. in z-position resp.
         Length2r =0.0,Length2l=0.0,  // length of a piece incl. diff. in y-position. for left and right side of a piece resp.
         right_beg=0.0,left_beg=0.0,  // right and left position of the beginning of a channel of a piece
         right_end=0.0,left_end=0.0;  // right and left position of the end of a channel of a piece

  double dXpce = GdXpce;
  // double dDelY = GdDelY;
  double dDelZ = GdDelZ;
  int i, l;

  myneutron = InputNeutrons + neutron_i;

  if (IsEOB(myneutron)==TRUE)
  {
    WriteNeutronParallel(myneutron, thread_i);
  }
  else
  {
    if (nColour!=-1 && nColour!=myneutron->Color) goto dump; //Wrong color neutrons will be written!

    /* myneutron->Position.X = 0.0;   !!!!!!!! */
    CopyVector(BegPosM, BegPosS);
    memcpy(RotMatrixS, RotMatrixM, sizeof(RotMatrixS));

    // Initialize the matrices for parallel MCPL output
    if (sMCPLWrite !=0)
    {
      CopyVector(BegPosM, BegPosSThread[thread_i]);
      for (i=0; i<3; i++)
      {
        for (l=0; l<3; l++)
        {
          RotMatrixSThread[thread_i*3+i][l] = RotMatrixM[i][l];
        }
      }
      FillRMatrixZY((MatrixPtr)RotMatrixSThread[3*thread_i], 0, 0);  //TMP solution because RotMatrixM is not initialized.
    }

    // Check to see if the neutron is initially in the entrance to the guide
    // ---------------------------------------------------------------------
    if (fabs(myneutron->Position[1]) > GuideEntrWidth/2.0 ||
        fabs(myneutron->Position[2]) > GuideEntrHeight/2.0)
    { // WriteIAP(myneutron, VT_OUTSIDE);
      return;
    }

    test = TRUE;
    kChan = 0;
    TimeOF1 = 0.0;
    TimeOF2 = 0.0;

    if (TotalLength == 0.0) goto zerolength;

    guide = TGuide[thread_i];
    gW = &(guide.Wall[0]);

    /************** start channel option *********************/
    if (nChannels > 1)
    {
      int k;
      for (k=0; k < nChannels; k++)
      {
        right_beg = -GuideEntrWidth/2.0 + k*(pPieces[0].Wchan + spacer);
        left_beg  = right_beg + pPieces[0].Wchan;

        if (    (right_beg < myneutron->Position[1])
             && (left_beg  > myneutron->Position[1]))
        {
          kChan = k;
          myneutron->Color = (short)(k+1);
          /* left and right walls of guide exchanged by the channel walls             */
          /* for elliptical parabolic shape, this has to be calculated for each piece */
          if (eGuideShapeY!=VT_PARABOLIC && eGuideShapeY!=VT_ELLIPTIC && eGuideShapeY!=VT_FROM_FILE) {
            right_end = -GuideExitWidth/2.0 + kChan*(pPieces[nPieces].Wchan + spacer);
            left_end  =  right_end + pPieces[nPieces].Wchan;
            dDelYr    =  right_end - right_beg;
            dDelYl    =  left_end  - left_beg;
            Length2r  =  sqrt(dXpce*dXpce+dDelYr*dDelYr);
            Length2l  =  sqrt(dXpce*dXpce+dDelYl*dDelYl);
            gW[GW_LEFT].A = -dDelYl/Length2l;
            gW[GW_LEFT].B =  dXpce /Length2l;
            gW[GW_LEFT].D =  gW[GW_LEFT].B*(-left_beg);
            gW[GW_RIGHT].A = -dDelYr/Length2r;
            gW[GW_RIGHT].B =  dXpce /Length2r;
            gW[GW_RIGHT].D =  gW[GW_RIGHT].B*(-right_beg);
          }
          break;
        }
      }
      if (k==nChannels)
      { WriteIAP(myneutron, VT_OUTSIDE); /* neutron blocked by spacer */
        return;
      }
    }
    /************** end channel option **************************/


    /****************************************************************************************/
    /* Pass a pointer to the neutron and the guide structure variable to a subroutine to do */
    /* the donkey work. The return value is the total length of the flight path through the */
    /* guide, or -1.0 if it missed all plates and the exit (should be impossible).          */
    /****************************************************************************************/

    PRefOut = GRefOut + thread_i;
    PRefOut->RefCount = 0;
    PRefOut->RefCountY = 0;
    PRefOut->RefCountZ = 0;
    PRefOut->insert_at = 0;
    PRefOut->cneutrons = 0;

    for (j=0; j < nPieces; j++)
    {
      CHECK;

      // In case of several pieces:
      //   planes must be adjusted for each piece (depending on the guide shape)
      if (nPieces > 1)
      {
        dXpce = pPieces[j+1].Xpce - pPieces[j].Xpce;

        switch (eGuideShapeY)
        {
          case VT_CURVED:
          case VT_LIN_CURV:
            /* last piece has an output plane normal to the guide direction, the others are tilted  */
            if (j == nPieces-1)
            {
              gW[eGwExit].A =  1.0;
              gW[eGwExit].B =  0.0;
              gW[eGwExit].D = -dXpce;
            }
            else
            {
              gW[eGwExit].A =  dCosBetH;
              gW[eGwExit].B =  dSinBetH;
              gW[eGwExit].D = -gW[eGwExit].A * dXpce;
            }
            if (eGuideShapeY==VT_LIN_CURV)
            { /* left and right walls are moved  */
              gW[GW_LEFT].D  = -gW[GW_LEFT].B  * pPieces[j].Ypce;
              gW[GW_RIGHT].D =  gW[GW_RIGHT].B * pPieces[j].Ypce;
            }
            break;

          case VT_LINEAR:
            /* left and right walls are moved  */
            gW[GW_LEFT].D  = -gW[GW_LEFT].B * pPieces[j].Ypce;
            gW[GW_RIGHT].D = gW[GW_RIGHT].B * pPieces[j].Ypce;
            break;

          case VT_PARABOLIC:
          case VT_ELLIPTIC:
          case VT_FROM_FILE:
            /* left and right walls are moved  */
            if (nChannels > 1)
            {
              right_beg = -pPieces[j].Ypce   + kChan*(pPieces[j].Wchan + spacer);
              left_beg  =  right_beg + pPieces[j].Wchan;
              right_end = -pPieces[j+1].Ypce + kChan*(pPieces[j+1].Wchan + spacer);
              left_end  =  right_end + pPieces[j+1].Wchan;
              dDelYr    = right_end - right_beg;
              dDelYl    = left_end  - left_beg;
              Length2r  = sqrt(dXpce*dXpce+dDelYr*dDelYr);
              Length2l  = sqrt(dXpce*dXpce+dDelYl*dDelYl);
            }
            else
            {
              right_beg = -pPieces[j].Ypce;
              left_beg  =  pPieces[j].Ypce;
              dDelYl    =  pPieces[j+1].Ypce-pPieces[j].Ypce;
              dDelYr    = -dDelYl;
              Length2l  = Length2r = sqrt(dXpce*dXpce+dDelYr*dDelYr);
            }
            gW[GW_LEFT].A = -dDelYl/Length2l;
            gW[GW_LEFT].B =  dXpce /Length2l;
            gW[GW_RIGHT].A = -dDelYr/Length2r;
            gW[GW_RIGHT].B =  dXpce /Length2r;

            gW[GW_LEFT].D = -gW[GW_LEFT].B *   left_beg;
            gW[GW_RIGHT].D =  gW[GW_RIGHT].B *(-right_beg);

            gW[eGwExit].D = -dXpce;
            break;
          default:;
        }

        switch (eGuideShapeZ)
        {
          case VT_PARABOLIC:
          case VT_ELLIPTIC:
          case VT_FROM_FILE:
            /* new shift is calculated to move walls */
            dDelZ   = pPieces[j+1].Zpce-pPieces[j].Zpce;
            Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
            gW[GW_TOP].A =  dDelZ/Length1;
            gW[GW_TOP].C = -dXpce/Length1;
            gW[GW_BOTTOM].A = -dDelZ/Length1;
            gW[GW_BOTTOM].C = -dXpce/Length1;
            gW[GW_TOP].D = -gW[GW_TOP].C * pPieces[j].Zpce;
            gW[GW_BOTTOM].D = gW[GW_BOTTOM].C * pPieces[j].Zpce;
            break;
          case VT_LINEAR:
            /* top and bottom walls are moved */
            gW[GW_TOP].D = -gW[GW_TOP].C * pPieces[j].Zpce;
            gW[GW_BOTTOM].D = gW[GW_BOTTOM].C * pPieces[j].Zpce;
            break;
          default:;
        }
      }

      // Option: Oktogonaler Guide
      if (nPlanes > 4)
      {
        double cx, sx, rot;
        long cPlane;
        rot = rotplane;
        cPlane = GW_RIGHT;
        while (fabs(rot) < 90.0 && cPlane < eGwExit)
        {
          cx = cos(rot/180.*M_PI);
          sx = sin(rot/180.*M_PI);
          switch (keyAddPlane)
          {
            case 1:
              cPlane++;
              gW[cPlane].A = gW[GW_TOP].A;
              gW[cPlane].B = gW[GW_TOP].B* cx + gW[GW_TOP].C*-sx;
              gW[cPlane].C = gW[GW_TOP].B* sx + gW[GW_TOP].C* cx;
              gW[cPlane].D = gW[GW_TOP].D;
              cPlane++;
              gW[cPlane].A = gW[GW_TOP].A;
              gW[cPlane].B = gW[GW_TOP].B* cx + gW[GW_TOP].C* sx;
              gW[cPlane].C = gW[GW_TOP].B*-sx + gW[GW_TOP].C* cx;
              gW[cPlane].D = gW[GW_TOP].D;
              cPlane++;
              gW[cPlane].A = gW[GW_BOTTOM].A;
              gW[cPlane].B = gW[GW_BOTTOM].B* cx + gW[GW_BOTTOM].C*-sx;
              gW[cPlane].C = gW[GW_BOTTOM].B* sx + gW[GW_BOTTOM].C* cx;
              gW[cPlane].D = gW[GW_BOTTOM].D;
              cPlane++;
              gW[cPlane].A = gW[GW_BOTTOM].A;
              gW[cPlane].B = gW[GW_BOTTOM].B* cx + gW[GW_BOTTOM].C* sx;
              gW[cPlane].C = gW[GW_BOTTOM].B*-sx + gW[GW_BOTTOM].C* cx;
              gW[cPlane].D = gW[GW_BOTTOM].D;
              break;
            case 2:
              cPlane++;
              gW[cPlane].A = gW[GW_LEFT].A;
              gW[cPlane].B = gW[GW_LEFT].B* cx + gW[GW_LEFT].C*-sx;
              gW[cPlane].C = gW[GW_LEFT].B* sx + gW[GW_LEFT].C* cx;
              gW[cPlane].D = gW[GW_LEFT].D;
              cPlane++;
              gW[cPlane].A = gW[GW_LEFT].A;
              gW[cPlane].B = gW[GW_LEFT].B* cx + gW[GW_LEFT].C* sx;
              gW[cPlane].C = gW[GW_LEFT].B*-sx + gW[GW_LEFT].C* cx;
              gW[cPlane].D = gW[GW_LEFT].D;
              cPlane++;
              gW[cPlane].A = gW[GW_RIGHT].A;
              gW[cPlane].B = gW[GW_RIGHT].B* cx + gW[GW_RIGHT].C*-sx;
              gW[cPlane].C = gW[GW_RIGHT].B* sx + gW[GW_RIGHT].C* cx;
              gW[cPlane].D = gW[GW_RIGHT].D;
              cPlane++;
              gW[cPlane].A = gW[GW_RIGHT].A;
              gW[cPlane].B = gW[GW_RIGHT].B* cx + gW[GW_RIGHT].C* sx;
              gW[cPlane].C = gW[GW_RIGHT].B*-sx + gW[GW_RIGHT].C* cx;
              gW[cPlane].D = gW[GW_RIGHT].D;
              break;
          }
          rot += rotplane;
        }

        /****************************************************************************************/
        /* Check to see if the neutron is initially in the entrance to the guide...             */
        /****************************************************************************************/
        if (j == 0)
        {
          double d = 0;
          int k;
          for (k=0; k < eGwExit; k++)
          {
            d = (gW[k].B*myneutron->Position[1] +
                 gW[k].C*myneutron->Position[2] +
                 gW[k].D) / gW[k].D;
            if (d < 0)
              break;
          }
          if (d < 0)
          {
            test=FALSE;
            WriteIAP(myneutron, VT_OUTSIDE);
            break; // quit npieces loop
          }
        }
      }

      if (eReflVbs != VT_LSTM_NO && j == 0)
        WriteReflParam(PRefOut, thread_i, 5, myneutron, &pPieces[j], eGwInit, 0., 0.);
      //if (j == 0)
      //  WriteIAP(myneutron, VT_ENTERED);

      // donkey work routine
      TimeOF1 = PathThroughGuideGravOrder1(thread_i, myneutron, guide, &pPieces[j], PRefOut, j);

      if (eReflVbs == VT_LSTM_EDGE && j == nPieces-1)
        WriteReflParam(PRefOut, thread_i, 5, myneutron, &pPieces[j], eGwExit, 0., 0.);

      if (TimeOF1 == -1.0)
      {
        test=FALSE;
        break; // trajectory is lost, quit npieces loop
      }

      /****************************************************************************************/
      /* Update the coordinates.                                                              */
      /****************************************************************************************/
      myneutron->Position[0] -= dXpce;

      /* Position vector for interaction points adjusted  */
      if (bVisTraj)
      {
        VectorType Shift={0.0,0.0,0.0};  /* Shift of end position  */

        Shift[0]= dXpce;
        RotBackVector(RotMatrixS, Shift);    // Transforms Shift from Section coord. syst. to Guide coord. system
        AddVector(BegPosS, Shift);           // Increments BegPosS with Shift
      }

      // Update BegPosS for absolute position calculations for MCPL output
      if (sMCPLWrite !=0)
      {
          VectorType Shift={dXpce, 0.0, 0.0};
          RotBackVector((MatrixPtr)RotMatrixSThread[3*thread_i], Shift);
          AddVector(BegPosSThread[thread_i], Shift);
          //SERIAL RotBackVector(RotMatrixSThread, Shift);
          //SERIAL AddVector(BegPosSThread, Shift);
      }
      //End of matrix update

      /* For curved guide: frame rotated for next piece, but not after last piece */
      if (Radius != 0.0)
      {
        /* horizontal position and flight direction adjusted */
        if (j < nPieces-1)
        {
          RotVector(RotMatrix, myneutron->Position);
          RotVector(RotMatrix, myneutron->Vector);
          RotVector(RotMatrix, myneutron->Spin);

          /* Rotation matrix for interaction points adjusted  */
          if (bVisTraj)
            FillRMatrixZY(RotMatrixS, RotY, RotZ+(j+1)*beta);

          /* RotMatrixSThread must be updated for MCPL */
          if (sMCPLWrite!=0)
          {
              FillRMatrixZY((MatrixPtr)RotMatrixSThread[3*thread_i], RotY, RotZ+(j+1)*beta);
              //SERIAL FillRMatrixZY(RotMatrixSThread, RotY, RotZ+(j+1)*beta);
          }
        }
      }

      TimeOF2 += TimeOF1;

    }  // end j nPieces loop

    if ( (pReflParam || pReflPlot) &&
         (abs(PRefOut->RefCount) >= nReflMinCnt && (abs(PRefOut->RefCount) <= nReflMaxCnt || nReflMaxCnt == 0)) &&
         (PRefOut->RefCountY >= nReflMinCntY && (PRefOut->RefCountY <= nReflMaxCntY || nReflMaxCntY == 0)) &&
         (PRefOut->RefCountZ >= nReflMinCntZ && (PRefOut->RefCountZ <= nReflMaxCntZ || nReflMaxCntZ == 0)) )
    {
      int condition;
      switch (abs(eReflPar))
      {
        case 1:  condition = (PRefOut->RefCount > 0); break;
        case 2:
        case 3:  condition = (PRefOut->RefCount != -1 && PRefOut->RefCount != 0); break;
        default: condition = 1;
      }
      if (condition)
      {
        if (pReflParam)
        {
          char *s = PRefOut->Output;
          if (NThreads <= 0 || thread_i <= 0)
          {
            // in serial mode, or if we are thread 0: print to file
            fprintf(pReflParam, eReflPar < 0 ? "%s\n" : "%s", s);
          }
          else
          {
            int len = PRefOut->insert_at;
            if (eReflPar < 0)
            {
              s[len] = '\n';
              len++;
            }
            addThreadOutput(thread_i, s, len);
          }
        }
        if (pReflPlot && PRefOut->neutrons)
          DoBin(PRefOut, thread_i);
      }
    }

    if (test==FALSE) return;

    if (fabs(myneutron->Position[1]) > 0.5*GuideExitWidth ||
        fabs(myneutron->Position[2]) > 0.5*GuideExitHeight)
    {
      CountMessageThread(thread_i, GUID_OUT_OF_EXIT, myneutron->ID);
      WriteIAP(myneutron, VT_OUT_OF_WND);
      return;
    }

   zerolength:
    /****************************************************************************************/
    /* Add the time needed to travel all guide and writeout this trajectory                 */
    /****************************************************************************************/
    {
      double pathlen;            // total neutron pathlength in the guide
      Neutron Output = *myneutron;

      pathlen = V_FROM_LAMBDA(Output.Wavelength)*TimeOF2;

      Output.Position[0]=0.0;
      Output.Time += TimeOF2;
      Output.Probability *= exp(-(MuScat+MuAbs*Output.Wavelength/1.798)*pathlen);

      WriteNeutronParallel(&Output, thread_i);
    }
    goto my_exit;
   dump:
   {
      Neutron Output = *myneutron;
      WriteNeutronParallel(&Output, thread_i);
   }
 my_exit:;
  }
}


/************************************************************************************/
// 'OwnCleanup' collects data from threads, writes out messages and coating results
/************************************************************************************/
void OwnCleanup()
{
  if (NThreads > 0)
  {
    int i,count;
    for (i=0; i<NThreads; i++)
    {
      if ((count = C_M[0][i]))
        CountMessageID_C(GUID_NO_PLANE,    C_I[0][i], count);
      if ((count = C_M[1][i]))
        CountMessageID_C(GUID_OUT_OF_EXIT, C_I[1][i], count);
    }
    if (pReflPlot)
      MergeBins();
    printMCStatistic(LogFilePtr);
  }

  /* print error that might have occurred many times */
  PrintMessage(GUID_OUT_OF_EXIT, "", ON);
  PrintMessage(GUID_NO_PLANE, "", ON);

  fprintf(LogFilePtr,"\n");

  if (pReflParam)
    fclose(pReflParam);

  if (sMCPLWrite!=0)
  {
    char mcpl_tmp_filename[1024];
    char mcpl_outfilename[1024];
    int i;
    //Close MCPL files
    for (i=0; i<NThreads+1; ++i)
    {
        mcpl_close_outfile(pReflMCPLThread[i]);
    }
    //Merge MCPL files
    sprintf(mcpl_outfilename, "%s.mcpl", FullOutName(MCPLParamFileName));
    fprintf(LogFilePtr, "MCPL outfile: %s \n", mcpl_outfilename);
    for (i=1;i<=NThreads;++i)
    {
      sprintf(mcpl_tmp_filename, "%s.tmp%d.mcpl", FullOutName(MCPLParamFileName), i);
      if (mcpl_can_merge(mcpl_outfilename, mcpl_tmp_filename) )
      {
        mcpl_merge_inplace(mcpl_outfilename, mcpl_tmp_filename);
        if (remove(mcpl_tmp_filename) == 1)
          fprintf(LogFilePtr, "Delete unsuccesfull: %s \n",mcpl_tmp_filename);
      }
      else
      { fprintf(LogFilePtr, "NOT MERGEABLE: %s -> %s\n",mcpl_tmp_filename, mcpl_outfilename);
      }
    }
  }

  if (pReflPlot)
  {
    writeBindata();
    fclose(pReflPlot);
  }
}


/******************************************************************************/
// 'Set Geometry' fills the structure stGeometry for Visualization
/******************************************************************************/
void SetGeometry(char* sColor)
{
  int    k;
  double GdPcePos, GdPcePos0=0.0;   // length from beginning of the guide to the center of the actual element, 0. element

  // Visualisation of the guide geometry
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.pHull = (VtHull*)calloc(nPieces, sizeof(VtHull));
    stGeometry.nHulls = nPieces;

    for (k=0; k < nPieces; k++)
    {
      stGeometry.pHull[k].Length     = (pPieces[k+1].Xpce - pPieces[k].Xpce);
      stGeometry.pHull[k].vNormal[1] = atan(k * beta);
      stGeometry.pHull[k].vNormal[2] = 0.0;
      stGeometry.pHull[k].vNormal[0] = sqrt(1.0 - sq(stGeometry.pHull[k].vNormal[1]) - sq(stGeometry.pHull[k].vNormal[2]));
      stGeometry.pHull[k].WidthIn    = 2.0 * BlowUp * pPieces[k].Ypce;
      stGeometry.pHull[k].HeightIn   = 2.0 * BlowUp * pPieces[k].Zpce;
      stGeometry.pHull[k].WidthOut   = 2.0 * BlowUp * pPieces[k+1].Ypce;
      stGeometry.pHull[k].HeightOut  = 2.0 * BlowUp * pPieces[k+1].Zpce;

      GdPcePos = 0.5 * (pPieces[k].Xpce + pPieces[k + 1].Xpce);
      if (Radius != 0.0)
      {
        if (k == 0)
          GdPcePos0 = GdPcePos;
        stGeometry.pHull[k].vCntr[0] = Radius *        sin((GdPcePos - GdPcePos0) / Radius) + GdPcePos0;
        stGeometry.pHull[k].vCntr[1] = Radius * (1.0 - cos((GdPcePos - GdPcePos0) / Radius));
      }
      else
      {
        stGeometry.pHull[k].vCntr[0] = GdPcePos;
        stGeometry.pHull[k].vCntr[1] = 0.0;
      }
      stGeometry.pHull[k].vCntr[2] = 0.0;
    }
  }

  return;
}


/******************************************************************************/
// Gets reflectivity data by either reading or calculating the data
/******************************************************************************/
ReflFile* GetReflData(double MValue, char *sFilename, FILE *file)
{
  int iFile = 0;    // index of already loaded reflectivity files

  if (sFilename==NULL && MValue==0.0)
    return NULL;

  // check if this coating already exists in the list
  for (iFile = 0; iFile < nReflFiles; iFile++)
  {
    // if the name is given, m values are ignored, compare filenames
    if (sFilename!=NULL)
    { if (pReflFiles[iFile].filename != NULL)
       #ifdef _MSC_VER
         if (_stricmp(pReflFiles[iFile].filename, sFilename)==0)
           return &pReflFiles[iFile];
       #else
         if (strcasecmp(pReflFiles[iFile].filename, sFilename)==0)
           return &pReflFiles[iFile];
       #endif
    }
    // otherwise m-values are compared
    else
    {
      if (MValue > 0.0 && pReflFiles[iFile].MValue==MValue)
        return &pReflFiles[iFile];
    }
  }

  // not found: add one data set
  nReflFiles++;
  iFile = nReflFiles - 1;
  if (sFilename != NULL)
  { pReflFiles[iFile].filename = sFilename;
    if (file) pReflFiles[iFile].pfile = file;
    LoadReflFile(&pReflFiles[iFile]);
  }
  else
  { pReflFiles[iFile].MValue = MValue;
    CalcReflData(&pReflFiles[iFile]);
  }
  return &pReflFiles[iFile];
}


/******************************************************************************/
// Loads Reflectivity data from file. Give pReflFile as input
/******************************************************************************/
void   LoadReflFile(ReflFile* pReflFile)
{
  long   count = 0, i=0,
         nCols = 0, nRows=0;
  int    nVals;          // number of Q and R values in the 2-column-file R(Q)
  char   sBuffer[CHAR_BUF_SMALL]="";

  if (pReflFile && pReflFile->filename)
  {
    if (pReflFile->pfile == NULL)
      pReflFile->pfile = OpenInputFile(pReflFile->filename, TRUE, "r");

    if (pReflFile->pfile != NULL)
    {
      nCols = ColumnsInFile(pReflFile->pfile);
      nRows = LinesInFile  (pReflFile->pfile);

      if (nCols==2 && nRows > 1)
      // Reflectivity file as R(Q) assumed
      {
        double aQ[ROFQ_MAX], aR[ROFQ_MAX];  // Q and reflectivity values given in the file
        nVals = ReadRofQ(pReflFile->pfile, aQ, aR);
        pReflFile->maxdata = NumDataPtsQ(aQ[nVals-1]);
        pReflFile->Rdata = (double*) calloc(pReflFile->maxdata, sizeof(double));
        pReflFile->MValue  = 0.0;
        SetReflData(pReflFile->Rdata, aQ, aR, nVals);
      }
      else
      // Reflectivity file as R(theta) corr. to 0.000,0.001, 0.002, ... deg, ref. wavelength 1 A, 10 values per line
      {
        pReflFile->Rdata = (double*) calloc(10*nRows, sizeof(double));
        pReflFile->MValue=0.0;
        for(count=0; count < nRows; count++)
        { ReadLine(pReflFile->pfile, sBuffer, sizeof(sBuffer)-1);
          i += StrgScanLF(sBuffer, &pReflFile->Rdata[10*count], 10*(nRows-count), 0);
        }
        pReflFile->maxdata = i;
      }

      fclose(pReflFile->pfile);
    }
  }
}


/******************************************************************************/
// calculates reflectivity data from m-value
/******************************************************************************/
void CalcReflData(ReflFile* pReflFile)
{
  double Angle,
         AngleMax;
  int    i=0;

  AngleMax = 0.1*(pReflFile->MValue+0.2);

  pReflFile->pfile   = NULL;
  pReflFile->filename= NULL;
  pReflFile->maxdata = (long)    ceil(1000*AngleMax)+1;
  pReflFile->Rdata   = (double*) calloc(pReflFile->maxdata, sizeof(double));

  for (Angle=0.000; Angle <= AngleMax; Angle+=0.001)
  {
    pReflFile->Rdata[i] = ReflTypical(QbyRefl(1.0, Angle), pReflFile->MValue);  // table designed for 1 Ang neutrons
    i++;
  }

  return;
}


/******************************************************************************/
// calculates height or width of the guide (depending on guide shape option)
/******************************************************************************/
double Height(double dLength)
{
  double dHeight=0.0,
         FocDistZ,     // half distance of focal points of the ellipse
         L_end;        // end of parabel

  switch (eGuideShapeZ)
  {
    case VT_CONSTANT:
    case VT_CURVED:
      dHeight = GuideEntrHeight;
      break;

    case VT_LINEAR:
      dHeight = GuideEntrHeight + (GuideExitHeight-GuideEntrHeight)/TotalLength * dLength;
      break;

    case VT_PARABOLIC:
      AparZ   = TotalLength/(sq(GuideEntrHeight) - sq(GuideExitHeight));
      L_end   = AparZ * sq(GuideEntrHeight);
      dHeight = sqrt((L_end-dLength)/AparZ);
      break;

    case VT_ELLIPTIC:
      CalculateEllipseParameters(GuideEntrHeight, GuideExitHeight, TotalLength / 100.0, D_Foc2Z / 100.0,
                                 &LAxisZ, &SAxisZ, &startPoint, &endPoint);
      FocDistZ       = 100.0 * sqrt(sq(LAxisZ) - sq(SAxisZ)); // m -> cm
      GuideMaxHeight = 200.0 * SAxisZ;                        // m -> cm
      LAxisZ        *= 100.0;                                 // m -> cm
      LcntrZ         = TotalLength - FocDistZ + D_Foc2Z;
      D_Foc1Z        = 2.0*FocDistZ - TotalLength - D_Foc2Z;
      dHeight        = GuideMaxHeight * sqrt(1.0 - sq((dLength-LcntrZ)/LAxisZ));
      break;

    default:
      Error("Shape unknown");
  }

  // test for proper value
  if (dHeight == dHeight+1.0       // test for infinity
      || dHeight!=dHeight)         // test for 'not a number'
    Error("Height of the guide element could not be determined");

  return dHeight;
}

double Width(double dLength)
{
  double dWidth=0.0,
         FocDistY,      // half distance of focal points of the ellipse
         L_end;         // end of parabel or 2nd part of ellipse (center to exit)

  switch (eGuideShapeY)
  {
    case VT_CONSTANT:
    case VT_CURVED:
      dWidth = GuideEntrWidth;
      break;

    case VT_LINEAR:
    case VT_LIN_CURV:
      dWidth = GuideEntrWidth + (GuideExitWidth-GuideEntrWidth)/TotalLength * dLength;
      break;

    case VT_PARABOLIC:
      AparY  = TotalLength/(sq(GuideEntrWidth) - sq(GuideExitWidth));
      L_end  = AparY * sq(GuideEntrWidth);
      dWidth = sqrt((L_end-dLength)/AparY);
      break;

    case VT_ELLIPTIC:
      CalculateEllipseParameters(GuideEntrWidth, GuideExitWidth, TotalLength / 100.0, D_Foc2Y / 100.0,
                                 &LAxisY, &SAxisY, &startPoint, &endPoint);
      FocDistY      = 100.0*sqrt(sq(LAxisY) - sq(SAxisY)); // m -> cm
      GuideMaxWidth = 200.0*SAxisY;                        // m -> cm
      LAxisY       *= 100.0;                               // m -> cm
      LcntrY        = TotalLength - FocDistY + D_Foc2Y;
      D_Foc1Y       = 2.0*FocDistY - TotalLength - D_Foc2Y;
      dWidth        = GuideMaxWidth * sqrt(1.0 - sq((dLength-LcntrY)/LAxisY));
      break;

    default:
      Error("Shape unknown");
  }

  // test for proper value
  if (dWidth == dWidth+1.0       // test for infinity
      || dWidth!=dWidth)         // test for 'not a number'
    Error("Width of the guide element could not be determined");

  return dWidth;
}


/******************************************************************************/
// determines length of the guide from the guide shape file
/******************************************************************************/
double GetLengthFromFile(FILE *pFile)
{
  char   sLine[512]="";
  double Xstart=0,
         Xend  =0;
  int    j=0;
  double tempX, tempY, tempZ;
  char   sTmp1[30]="", sTmp2[30]="", sTmp3[30]="", sTmp4[30]="";

  fpos_t position;
  fgetpos (pFile, &position);

  for (j=0; j <= nPieces; j++)
  {
    ReadLine(pFile, sLine, sizeof(sLine)-1);

    sscanf(sLine, "%lf %lf %lf %s %s %s %s", &tempX, &tempY, &tempZ,
                                             sTmp1,  sTmp2,  sTmp3, sTmp4);

    if (j == 0)       Xstart = 100.0*tempX;
    if (j == nPieces) Xend   = 100.0*tempX;
  }

  fsetpos(pFile, &position);

  return (Xend - Xstart);
}


/******************************************************************************/
// calculates flight pat within the guide
/******************************************************************************/
double PathThroughGuideGravOrder1(int thread_i,
                                  Neutron *pThisNeutron, NeutronGuide guide,
                                  GuidePiece *Pce, ReflCond *RefOut, long iPiece)
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
  /* Significantly rewritten by Sergey Manoshin Feb 2001                             */
  /* Note! The function returns the Time Of Flight                                   */
  /***********************************************************************************/

  int     datanumber,
          iColl=0,          // index of collisions
          k = GW_INIT;
  VtGdeWall ThisCollision = GW_INIT;
  double  degangular, ThisReflectivity=0.;
  double  TimeOF,         // time of flight to any wall
          TimeOFmin,      // shortest time of flight to a wall (found so far)
          TimeOFTotal=0.0;// total time of flight through the guide segment
  double  HitDist,        // distance between point of reflection and end of guide segment
          DOTP;
  VectorType vWallN,      // normal to the plane wall
             vWaviN;      // normal to the wall with its waviness
  Neutron TempNeutron, NearestNeutron; // Local copies of the actual trajectory for loops


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
    for (k=GW_TOP; k < eGwInit; k++) { /* GW_TOP = 0, GW_INIT = 5 */

      /***********************************************************************************/
      /* Find the point where this neutron trajectory intercepts the current plane       */
      /***********************************************************************************/

      /*Save current neutron, because the function 'NeutronPlaneIntersectionGrav' has
        modified trajectory data  */
      CopyNeutron(pThisNeutron, &TempNeutron);

      if (keygrav == 1)
        TimeOF = NeutronPlaneIntersectionGrav(&TempNeutron, guide.Wall[k]);
      else
        TimeOF = NeutronPlaneIntersection1   (&TempNeutron, guide.Wall[k]);

      /***********************************************************************************/
      /* If this intercept point is behind the neutrons current position, pass control to*/
      /* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */
      /***********************************************************************************/
      if ((TimeOF<=0.0) || (TempNeutron.Position[0] < pThisNeutron->Position[0]))
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
      ThisCollision = (VtGdeWall) k;
    }

    /***********************************************************************************/
    /* Having looped through all five planes, the current values of NearestNeutron,    */
    /* TimeOFmin and ThisCollision, reflect the coordinates, distance and index        */
    /* of the neutrons interaction with a guide wall. If this guide wall is index 4    */
    /*(i.e. the exit window) reset the neutron coordinates to this point, add the path */
    /* length to this point to the running total and return that total.                */
    /***********************************************************************************/

    if (ThisCollision == eGwExit)
    {
      if (NearestNeutron.Vector[0] < 0.0)
        return -1.0;

      /*  Neutrons hitting the surface close to the guide exit are removed,
          but not in the last segment (iPiece=nPieces-1)                          */
      if (iPiece!=nPieces-1 && AbutLen > 0.0)
      {
        HitDist = TimeOFmin * V_FROM_LAMBDA(NearestNeutron.Wavelength);
        if (HitDist <= 0.5*AbutLen)
        { WriteIAP(&NearestNeutron, VT_ABSORBED);
          return -1.0;
        }
      }

      if (NearestNeutron.Probability < wei_min)
      { WriteIAP(&NearestNeutron, VT_ABSORBED);
        return -1.0;
      }

      pThisNeutron->Position[0] = NearestNeutron.Position[0];
      pThisNeutron->Position[1] = NearestNeutron.Position[1];
      pThisNeutron->Position[2] = NearestNeutron.Position[2];
      pThisNeutron->Vector  [2] = NearestNeutron.Vector  [2];
      pThisNeutron->Vector  [0] = sqrt(1.0 - sq(pThisNeutron->Vector[1])
                                           - sq(pThisNeutron->Vector[2]));
      pThisNeutron->Probability = NearestNeutron.Probability;

      if (eReflVbs == VT_LSTM_YES && RefOut)
        WriteReflParam(RefOut, thread_i, 5, pThisNeutron, Pce, ThisCollision, 0., 0.);

      if (iPiece == nPieces-1)
      {
        WriteIAP(pThisNeutron, VT_EXITED);
      }
      else
      {
       // WriteIAP(pThisNeutron, VT_TRANSIT);
      }
      TimeOFTotal += TimeOFmin;
      return TimeOFTotal;
    }
    else
    { /*  Abutment loss:
          Neutrons hitting the surface close to the entrance of a guide segment are removed,
          but not in the first segment (iPiece=0); only the first collision (iColl=1) can be considered */
      if (NearestNeutron.Vector[0] < 0.0 || NearestNeutron.Position[0] > TotalLength*1.1)
        return -1.0;
      iColl++;
      if (iPiece!=0 && iColl==1 && AbutLen > 0.0)
      {
        HitDist = TimeOFmin * V_FROM_LAMBDA(NearestNeutron.Wavelength);
        if (HitDist <= 0.5*AbutLen)
        { WriteIAP(&NearestNeutron, VT_ABSORBED);
          return -1.0;
        }
      }
    }

    /***********************************************************************************/
    /* If the angle of intersection of the flight path and the guide wall exceeds the  */
    /* critical angle of the guide, the neutron is absorbed.                           */
    /* Otherwise the probability is reduced by the reflectivity of the plane.          */
    /***********************************************************************************/

    vWallN[0] = guide.Wall[ThisCollision].A;
    vWallN[1] = guide.Wall[ThisCollision].B;
    vWallN[2] = guide.Wall[ThisCollision].C;

    /* Normalize normal vector to the reflection plane */
    if (LengthVector(vWallN) == 0.0)
      return -1.0;
    NormVector(vWallN);

    /* influence of rough surface */
    if (WaviAmpl == 0.0)
      CopyVector(vWallN, vWaviN);
    else
      /* rough surface must not alter the side from which the neutron comes */
      do
      {
        double V[3];
        ran_dir_3d_par(V, thread_i);
        vWaviN[0] = vWallN[0] + WaviAmpl*V[0];
        vWaviN[1] = vWallN[1] + WaviAmpl*V[1];
        vWaviN[2] = vWallN[2] + WaviAmpl*V[2];

        /* Renormalize vector */
        if (LengthVector(vWaviN) == 0.0) return -1.0;
        NormVector(vWaviN);
      }
      while (ScalarProduct(NearestNeutron.Vector, vWallN) * ScalarProduct(NearestNeutron.Vector, vWaviN) < 0.0);

    /* angle between normal vector and neutron flight direction (in degree) */
    degangular = fabs(90 - AngleVectors(NearestNeutron.Vector, vWaviN));

    /* Determine number of reflectivity value in reflectivty file */
    datanumber = (int)(degangular*1000.0 / NearestNeutron.Wavelength);

    /* Choose the reflectivity file/value and multiply probability by reflectivity value */
    if ((int)ThisCollision < eGwExit)
    {
      if (Pce->RData[ThisCollision]==NULL || datanumber >= Pce->RData[ThisCollision]->maxdata)
      {
        if (RefOut)
          WriteReflParam(RefOut, thread_i, 10, &NearestNeutron, Pce, ThisCollision, degangular, 0.);
        WriteIAP(&NearestNeutron, VT_ABSORBED);
        return -1.0;
      }
      else
      { ThisReflectivity = ReflInterpol(NearestNeutron.Wavelength, degangular, Pce->RData[ThisCollision]->Rdata, Pce->RData[ThisCollision]->maxdata);
        //ThisReflectivity = Pce->RData[ThisCollision]->Rdata[datanumber];
      }
    }
    else
    {
      CountMessageThread(thread_i, GUID_NO_PLANE, NearestNeutron.ID);
      return -1.0;
    }

    NearestNeutron.Probability *= ThisReflectivity;

    if (NearestNeutron.Probability < wei_min)
    {
      NearestNeutron.Probability = 0.;
      if (RefOut)
        WriteReflParam(RefOut, thread_i, 10, &NearestNeutron, Pce, ThisCollision, degangular, ThisReflectivity);
      WriteIAP(&NearestNeutron, VT_ABSORBED);
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
    pThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*vWaviN[0];
    pThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*vWaviN[1];
    pThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*vWaviN[2];

    while (ScalarProduct(pThisNeutron->Vector, vWallN) * ScalarProduct(NearestNeutron.Vector, vWallN) > 0.0)
    {
      pThisNeutron->Vector[0] -= 2.0*DOTP*vWaviN[0];
      pThisNeutron->Vector[1] -= 2.0*DOTP*vWaviN[1];
      pThisNeutron->Vector[2] -= 2.0*DOTP*vWaviN[2];
    }
    NormVector(pThisNeutron->Vector);

    /* CopyVector(NearestNeutron.Position, pThisNeutron->Position); */
    pThisNeutron->Position[0] = NearestNeutron.Position[0];
    pThisNeutron->Position[1] = NearestNeutron.Position[1];
    pThisNeutron->Position[2] = NearestNeutron.Position[2];

    pThisNeutron->Probability = NearestNeutron.Probability;

    if(ThisCollision == GW_TOP || ThisCollision == GW_BOTTOM)
      pThisNeutron->Color += AddToColor;
    else if(ThisCollision == GW_LEFT || ThisCollision == GW_RIGHT)
      pThisNeutron->Color += AddToColor*100;

    TimeOFTotal += TimeOFmin;

    // Graphical output
    if (RefOut) WriteReflParam(RefOut, thread_i, 0, pThisNeutron, Pce, ThisCollision, degangular, ThisReflectivity);

    NearestNeutron.Probability *= (1.0/ThisReflectivity - 1.0);       // intensity loss by reflection (for IAP output)
    WriteIAP(&NearestNeutron, VT_REFLECTED);
  }
}


/*******************************************************/
/** Code for Helper Threads                           **/
/*******************************************************/

// Callback routine flushOutput is called after a chunk of read neutrons have been
// processed by pararallel threads.
// For module guide this means to write to the pReflParam file if any data have been recorded
// by threads 1,2,... ; output from thread 0 directly goes to pReflParam.
// ----------------------------------------------------------------------------------------------
void flushOutput()
{
  int c, n;
  if (NThreads <= 0 || pReflParam == 0) return;
  for (n=0; n < NThreads; n++)
  {
    if ((c = rThreadOCount[n]))
    {
      fwrite(rThreadO[n], 1, c, pReflParam);
      rThreadOCount[n] = 0;
    }
  }
}


static Plane* copyWalls (Plane *g)
{
  Plane *c;
  int sz = (nPlanes+1) * sizeof(Plane);
  c = (Plane*) malloc(sz);
  memcpy(c, g, sz);
  return c;
}


static void addThreadOutput(int thread_i, char *s, int len)
{
  int n,c;
  if (len <= 0) return;
  n = thread_i - 1;  // n will be 0 for thread 1
  c = rThreadOCount[n];
  if ((rThreadOCount[n] = c + len) > perThreadMaxOut)
    myExit2("!!! temp reflection buffer size %d too small for thread %d !!!\n", perThreadMaxOut, thread_i);
  memcpy(rThreadO[n] + c, s, len);
}


static void CountMessageThread (int thread_i, VtMsgID code, TotalID id)
{
  int i;
  if (thread_i-- <= 0)
  {
    CountMessageID(code, id);
    return;
  }

  i = code == GUID_NO_PLANE ? 0 : 1;

  if (! C_M[i][thread_i])
    C_I[i][thread_i] = id;
  C_M[i][thread_i]++;
}


static void MergeThreadBins (BINDATA **bin, int pixcount)
{
  // merge neutron path statistics of threads n=1,2,..,NThreads to that of thread 0
  int i,n;
  BINDATA **thread_bin, *tpi, *bpi;

  thread_bin = bin;

  for (n=1; n<=NThreads; n++)
  {
    thread_bin += pixcount;
    for (i=0; i<pixcount; i++)
    {
      if ((tpi = thread_bin[i]))
      {
        // we have data for pixel i of thread n
        if ((bpi = bin[i]))
        {
          // we have data for that pixel of thread 0, too
          addPixStat(bpi, tpi);
        }
        else
        {
          // just use thread n's data for bin
          bin[i] = tpi;
        }
      }
    }
  }
}


static void MergeBins ()
{
  MergeThreadBins(binX, nbinsX*(nPlanes+1));
  MergeThreadBins(binY, nbinsY*(nPlanes+1));
  MergeThreadBins(binXY, nbinsX*nbinsY*(nPlanes+1));  //should better use INDEX!
}


static FILE* tryOpen(const char *sFilename, const char *sPurpose)
{
  FILE* pFile=OpenInputFile(sFilename, FALSE, "r");
  if (pFile==NULL)
  { fprintf(LogFilePtr, "ERROR: File %s containing %s could not be opened\n", sFilename, sPurpose);
    exit(-1);
  }

  return pFile;
}


static void allocRdata (ReflFile ***p, int c)
{
  ReflFile **np;
  if (*p) return;
  np = (ReflFile**) calloc(c, sizeof(ReflFile*));
  if (np)
    *p = np;
  else
    myExit("ERROR: Not enough memory for reflectivity of planes!\n");
}


/*******************************************************/
/** Coating optimization                              **/
/*******************************************************/

void WriteReflParam(ReflCond *RefOut, int thread_i, int Mode, Neutron *pNeutron, GuidePiece *Pce,
                    VtGdeWall ThisCollision, double degangular, double reflectivity)
{
  const char *fstr="%c%c%09lu     %c     %3d   %8.5f %6.2f %12.5f %8.4f %8.4f  %c %5d  %7.3f %8.5f %11.3e"
    "  %10.4f %10.4f %10.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n";
  double     DivY, DivZ, mVal, Qz;
  char       buffer[256];

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

  buffer[0] = 0;

  if (pReflParam)
  {
    int c;
    switch (Mode)
    {
      case 10: c = 'F'; break; // Neutron died
      case  5: c = '-'; break; // GW_EXIT
      case  0: c = 'T'; break; // Scattered
      default: c = 0;
    }
    if (c)
      sprintf(buffer, fstr,
              pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo,
              c, ThisCollision, degangular, mVal, reflectivity, DivY, DivZ,
              pNeutron->Debug,       pNeutron->Color,
              pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
              pNeutron->Position[0]+Pce->Xpce, pNeutron->Position[1], pNeutron->Position[2],
              pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
              pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]);
  }

  // Start MCPL output
  // Calculate absolute position
  double absPos[3]={pNeutron->Position[0], pNeutron->Position[1], pNeutron->Position[2]};
  if(sMCPLWrite)
  {
    int j;
    RotBackVector((MatrixPtr)RotMatrixSThread[thread_i*3], absPos);
    //SERIAL RotBackVector(RotMatrixSThread, absPos);

    for (j=0; j<3; j++)
      absPos[j] = BegPosSThread[thread_i][j] + absPos[j];
  }
  // Gamma output
  if (sMCPLWrite==1 || sMCPLWrite==3)
  {
    //Calculate gamma and neutron intensities
    if(ThisCollision == GW_TOP || ThisCollision == GW_BOTTOM)
    {
      CalcGammaAndNeutron(pNeutron->Probability, reflectivity, pNeutron->Wavelength, mVal,
                          MValGenTB, pMCPLparticles+thread_i);
    }
    else if(ThisCollision == GW_LEFT)
    {
      CalcGammaAndNeutron(pNeutron->Probability, reflectivity, pNeutron->Wavelength, mVal,
                          MValGenL, pMCPLparticles+thread_i);
    }
    else if(ThisCollision == GW_RIGHT)
    {
      CalcGammaAndNeutron(pNeutron->Probability, reflectivity, pNeutron->Wavelength, mVal,
                          MValGenR, pMCPLparticles+thread_i);
    }
    else
    {
        fprintf(LogFilePtr, "Problem with m-value calculation in MCPL output! Using m-value of right side.\n\n");
      CalcGammaAndNeutron(pNeutron->Probability, reflectivity, pNeutron->Wavelength, mVal,
                          MValGenR, pMCPLparticles+thread_i);
    }

    double cosTheta, cosPhi, sinTheta, sinPhi, phi;
    int i;
    for (i=0; i<MAX_GAMMA_NUM; i++)
    {
      particleT[thread_i].position[0] = absPos[0];
      particleT[thread_i].position[1] = absPos[1];
      particleT[thread_i].position[2] = absPos[2];

      cosTheta=(VranPar(thread_i)-0.5)*2;
      sinTheta=sqrt(1-pow(cosTheta,2));
      phi=VranPar(thread_i)*2*M_PI;
      sinPhi=sin(phi);
      cosPhi=cos(phi);
      particleT[thread_i].direction[0] = sinTheta*cosPhi;
      particleT[thread_i].direction[1] = sinTheta*sinPhi;
      particleT[thread_i].direction[2] = cosTheta;

      particleT[thread_i].ekin = gamma_energy[i];
      particleT[thread_i].time = pNeutron->Time;
      particleT[thread_i].pdgcode = 22;
      particleT[thread_i].weight = (pMCPLparticles+thread_i)->gammaInt[i];
      //Add gamma particle
      mcpl_add_particle( pReflMCPLThread[thread_i], &particleT[thread_i]);
    }
  }

  //Escaping neutron output
  if ((sMCPLWrite==2) || (sMCPLWrite==3))
  {
      double dir[3]={pNeutron->Vector[0], pNeutron->Vector[1], pNeutron->Vector[2]};
      RotBackVector((MatrixPtr)RotMatrixSThread[thread_i*3], dir);

      particleT[thread_i].position[0] = absPos[0];
      particleT[thread_i].position[1] = absPos[1];
      particleT[thread_i].position[2] = absPos[2];
      particleT[thread_i].direction[0] = dir[0];
      particleT[thread_i].direction[1] = dir[1];
      particleT[thread_i].direction[2] = dir[2];

      particleT[thread_i].ekin = 1.0e-12 * ENERGY_FROM_LAMBDA(pNeutron->Wavelength);
      particleT[thread_i].time = pNeutron->Time;
      particleT[thread_i].pdgcode = 2112;
      particleT[thread_i].weight = pNeutron->Probability;
      //Add neutron particle
      mcpl_add_particle( pReflMCPLThread[thread_i], &particleT[thread_i]);
  }
  //End MCPL output

  if ((Mode == 0) || (Mode == 5 && RefOut->RefCount >= 0) || (Mode != 0 && abs(eReflPar) > 2))
  {
    NeutronEx *rp;
    int slen, to_alloc;

    slen = strlen(buffer);
    if (slen) {
      // add string to Output
      to_alloc = 1 + (slen > allocText ? slen : allocText);
      if (RefOut->Output == 0)
      {
        if (!(RefOut->Output = malloc(to_alloc)))
          myExit("unable to malloc refl string\n");
        RefOut->alloc_text = to_alloc;
        RefOut->insert_at = 0;
      }
      else if (RefOut->insert_at + slen >= RefOut->alloc_text)
      {
        RefOut->alloc_text += to_alloc;
        if (!(RefOut->Output = realloc(RefOut->Output, RefOut->alloc_text)))
          myExit("unable to realloc refl string\n");
      }
      memcpy(RefOut->Output + RefOut->insert_at, buffer, slen+1);
      RefOut->insert_at += slen;
    }

    // add neutron to reflection storage /* neutrons */
    if (((int)ThisCollision < eGwExit) &&
      ((ePlotFilt == VT_PLOT_ALL) || ((ePlotFilt == VT_PLOT_SCAT) && (Mode == 0)) || ((ePlotFilt == VT_PLOT_DIED) && (Mode == 10))))
    {
      // first fetch enough space for that
      if (RefOut->alloc_neutrons == 0)
      {
        if (!(RefOut->neutrons = malloc(allocNeutrons * sizeof(NeutronEx))))
          myExit("unable to malloc refl storage\n");
        RefOut->alloc_neutrons = allocNeutrons;
      }
      else if (RefOut->alloc_neutrons <= RefOut->cneutrons)
      {
        RefOut->alloc_neutrons += allocNeutrons;
        if (!(RefOut->neutrons = realloc(RefOut->neutrons, RefOut->alloc_neutrons * sizeof(NeutronEx))))
          myExit("unable to realloc refl storage\n");
      }

      rp = &(RefOut->neutrons[RefOut->cneutrons]);
      CopyNeutron(pNeutron, &(rp->neutron));
      rp->neutron.Position[0] += Pce->Xpce + XpceZero;
      rp->ThisCollision = ThisCollision;
      rp->degangular = degangular;
      rp->m = mVal;
      rp->reflectivity = reflectivity;
      rp->DivY = DivY;
      rp->DivZ = DivZ;
      rp->Mode = Mode;
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

void PrintMaximalM(double *RData, long i)
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


int FindIndexXY(double Xval, double Yval, int *ibinX, int *ibinY, int iplane)
{
  int ix, iy;
  *ibinX = ix = (int)((Xval - MinX) / ((MaxX - MinX) / (double)nbinsX));
  if (ix >= nbinsX)
    return -1;
  *ibinY = iy = (int)((Yval - MinY) / ((MaxY - MinY) / (double)nbinsY));
  if (iy >= nbinsY)
    return -1;

  return INDEX(ix, iy, iplane);
}


static int writeReflPix (BINDATA *pix, int datarange)
{ //datarange: 0 = XY; 1 = X; 2 = Y
  double ProbSum;
  NeutronEx *bp;

  const char *fstr="%10.4f %10.4f %10d %5.1f %7d %7d %7d %5.2f %5.2f %5.2f %c%c%09lu %3d   %8.5f %6.2f %12.5f %8.4f %8.4f"
    "  %c %5.2f  %7.3f %8.5f %11.3e  %10.4f %10.4f %10.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f %11.3e\n";

  // Generate averages
  bp = &(pix->ndata);
  if (pix->ProbSum <= 0.) return 0;
  ProbSum = pix->ProbSum;
  bp->degangular          /= ProbSum;
  bp->m                   /= ProbSum;
  bp->reflectivity        /= ProbSum;
  bp->DivY                /= ProbSum;
  bp->DivZ                /= ProbSum;
  bp->neutron.Time        /= ProbSum;
  bp->neutron.Wavelength  /= ProbSum;
  bp->neutron.Position[0] /= ProbSum;
  bp->neutron.Position[1] /= ProbSum;
  bp->neutron.Position[2] /= ProbSum;
  bp->neutron.Vector[0]   /= ProbSum;
  bp->neutron.Vector[1]   /= ProbSum;
  bp->neutron.Vector[2]   /= ProbSum;
  bp->neutron.Spin[0]     /= ProbSum;
  bp->neutron.Spin[1]     /= ProbSum;
  bp->neutron.Spin[2]     /= ProbSum;
  pix->RefCount           /= ProbSum;
  pix->RefCountY          /= ProbSum;
  pix->RefCountZ          /= ProbSum;

  fprintf(pReflPlot, fstr,
          (datarange!=2)?pix->X:0.0, (datarange!=1)?pix->Y:0.0, pix->Counts,
          ((double)(bp->Mode)/ProbSum),
          pix->Mode0             , pix->Mode5             , pix->Mode10,
          pix->RefCount          , pix->RefCountY         , pix->RefCountZ,
          bp->neutron.ID.IDGrp[0], bp->neutron.ID.IDGrp[1], bp->neutron.ID.IDNo,
          bp->ThisCollision      , bp->degangular         , bp->m,
          bp->reflectivity       , bp->DivY               , bp->DivZ,
          bp->neutron.Debug      ,
          ((double)bp->neutron.Color)/ProbSum,
          bp->neutron.Time       , bp->neutron.Wavelength , bp->neutron.Probability,
          bp->neutron.Position[0], bp->neutron.Position[1], bp->neutron.Position[2],
          bp->neutron.Vector[0]  , bp->neutron.Vector[1]  , bp->neutron.Vector[2],
          bp->neutron.Spin[0]    , bp->neutron.Spin[1]    , bp->neutron.Spin[2],
          ProbSum
          );
  return 1;
}


static void writeBindata ()
{
  int ibinXY, ibinX, ibinY, iplane, cout;
  BINDATA *pix;
  char buf[4][40];

  memset(buf, 0, 4*40); // clean initialisation

  GetKeyName(KeyX, buf[0]);
  GetKeyName(KeyY, buf[1]);
  GetKeyName(KeyProb, buf[2]);

  fprintf(pReflPlot, "#BinX:%s   BinY:%s   Weight:%s\n#==Data==\n", buf[0], buf[1], buf[2]);
  for (iplane = -1; iplane < nPlanes; iplane++)
  {
    if (iplane!=-1) fprintf(pReflPlot, "\n#==Data%d==\n", iplane+1);
    for (ibinX = 0; ibinX < nbinsX; ibinX++)
    {
      cout = 0;
      for (ibinY = 0; ibinY < nbinsY; ibinY++)
      {
        ibinXY = INDEX(ibinX, ibinY, iplane);
        if ((pix = binXY[ibinXY])) cout+=writeReflPix(pix, 0);
      }
      if (eReflPar < 0 && cout>0) fprintf(pReflPlot,"\n");
    }

    if (iplane==-1)
    { fprintf(pReflPlot, "\n#==XData==\n");
    }
    else
    { fprintf(pReflPlot, "\n#==XData%d==\n", iplane+1);
    }
    for (ibinX=0; ibinX < nbinsX; ibinX++)
      if ((pix = binX[ibinX + nbinsX*(iplane+1)]))
        writeReflPix(pix, 1);

    if (eReflPar < 0) fprintf(pReflPlot,"\n");

    if (iplane==-1)
    { fprintf(pReflPlot, "\n#==YData==\n");
    }
    else
    { fprintf(pReflPlot, "\n#==YData%d==\n", iplane+1);
    }
    for (ibinY=0; ibinY < nbinsY; ibinY++)
      if ((pix = binY[ibinY + nbinsY*(iplane+1)]))
        writeReflPix(pix, 2);

    if (eReflPar < 0) fprintf(pReflPlot,"\n");
  }
}


static void applyPixStat(ReflCond *RefOut, double ValProb, NeutronEx *bp, NeutronEx *rp, BINDATA *pix)
{
  bp->degangular          += ValProb * rp->degangular;
  bp->m                   += ValProb * rp->m;
  bp->reflectivity        += ValProb * rp->reflectivity;
  bp->DivY                += ValProb * rp->DivY;
  bp->DivZ                += ValProb * rp->DivZ;
  bp->Mode                += (int)(ValProb * rp->Mode);
  bp->neutron.Color       += (short)ValProb * rp->neutron.Color;
  bp->neutron.Time        += ValProb * rp->neutron.Time;
  bp->neutron.Wavelength  += ValProb * rp->neutron.Wavelength;
  bp->neutron.Probability += ValProb * rp->neutron.Probability;
  bp->neutron.Position[0] += ValProb * rp->neutron.Position[0];
  bp->neutron.Position[1] += ValProb * rp->neutron.Position[1];
  bp->neutron.Position[2] += ValProb * rp->neutron.Position[2];
  bp->neutron.Vector[0]   += ValProb * rp->neutron.Vector[0];
  bp->neutron.Vector[1]   += ValProb * rp->neutron.Vector[1];
  bp->neutron.Vector[2]   += ValProb * rp->neutron.Vector[2];
  bp->neutron.Spin[0]     += ValProb * rp->neutron.Spin[0];
  bp->neutron.Spin[1]     += ValProb * rp->neutron.Spin[1];
  bp->neutron.Spin[2]     += ValProb * rp->neutron.Spin[2];

  pix->ProbSum   += ValProb;
  pix->RefCount  += ValProb * abs(RefOut->RefCount);
  pix->RefCountY += ValProb * RefOut->RefCountY;
  pix->RefCountZ += ValProb * RefOut->RefCountZ;

  switch (rp->Mode)
  {
    case 0:  pix->Mode0++;  break;
    case 5:  pix->Mode5++;  break;
    case 10: pix->Mode10++; break;
  }

  pix->Counts++;
}


static void addPixStat(BINDATA *bpix,  BINDATA *rpix)
{
  NeutronEx *bp, *rp;
  bp = &(bpix->ndata);
  rp = &(rpix->ndata);
  bp->degangular          += rp->degangular;
  bp->m                   += rp->m;
  bp->reflectivity        += rp->reflectivity;
  bp->DivY                += rp->DivY;
  bp->DivZ                += rp->DivZ;
  bp->Mode                += rp->Mode;
  bp->neutron.Color       += rp->neutron.Color;
  bp->neutron.Time        += rp->neutron.Time;
  bp->neutron.Wavelength  += rp->neutron.Wavelength;
  bp->neutron.Probability += rp->neutron.Probability;
  bp->neutron.Position[0] += rp->neutron.Position[0];
  bp->neutron.Position[1] += rp->neutron.Position[1];
  bp->neutron.Position[2] += rp->neutron.Position[2];
  bp->neutron.Vector[0]   += rp->neutron.Vector[0];
  bp->neutron.Vector[1]   += rp->neutron.Vector[1];
  bp->neutron.Vector[2]   += rp->neutron.Vector[2];
  bp->neutron.Spin[0]     += rp->neutron.Spin[0];
  bp->neutron.Spin[1]     += rp->neutron.Spin[1];
  bp->neutron.Spin[2]     += rp->neutron.Spin[2];

  bpix->ProbSum   += rpix->ProbSum;
  bpix->RefCount  += rpix->RefCount;
  bpix->RefCountY += rpix->RefCountY;
  bpix->RefCountZ += rpix->RefCountZ;

  bpix->Mode0  += rpix->Mode0;
  bpix->Mode5  += rpix->Mode5;
  bpix->Mode10 += rpix->Mode10;

  bpix->Counts +=  rpix->Counts;
}


static void doBinDetail(ReflCond *RefOut, NeutronEx *rp, double ValProb, int ibinX, int ibinY, BINDATA **ppix)
{
  BINDATA *pix;
  NeutronEx *bp;

  if ((pix = *ppix))
  {
    bp = &(pix->ndata);
  }
  else
  {
    *ppix = pix = (BINDATA *) calloc(1, sizeof(BINDATA));
    pix->X = (bpostX[ibinX] + bpostX[ibinX+1]) / 2.0;
    pix->Y = (bpostY[ibinY] + bpostY[ibinY+1]) / 2.0;
    bp = &(pix->ndata);
    bp->neutron.ID = rp->neutron.ID;
    bp->neutron.Debug = rp->neutron.Debug;
    bp->ThisCollision = rp->ThisCollision;
  }

  applyPixStat(RefOut, ValProb, bp, rp, pix);
}


void DoBin(ReflCond *RefOut, int thread_i)
{
  int ibinX, ibinY, ibinXY, cNeut;
  double ValX, ValY, ValProb;
  NeutronEx *rp;

  for (cNeut = 0; cNeut < RefOut->cneutrons; cNeut++)
  {
    ValX = GetValueX(RefOut, cNeut);
    ValY = GetValueY(RefOut, cNeut);
    ibinXY = FindIndexXY(ValX, ValY, &ibinX, &ibinY, -1);
    if (ibinXY < 0) continue;
    ValProb = GetProb(RefOut, cNeut);

    rp = &(RefOut->neutrons[cNeut]);

    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binX  + ibinX  + thread_i*nbinsX*(nPlanes+1));
    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binY  + ibinY  + thread_i*nbinsY*(nPlanes+1));
    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binXY + ibinXY + thread_i*nbinsX*nbinsY*(nPlanes+1));
    //if (rp->ThisCollision < GW_EXIT) {

    ibinXY = FindIndexXY(ValX, ValY, &ibinX, &ibinY, rp->ThisCollision);
    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binX  + ibinX  + (rp->ThisCollision+1)*nbinsX + thread_i*nbinsX*(nPlanes+1));
    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binY  + ibinY  + (rp->ThisCollision+1)*nbinsY + thread_i*nbinsY*(nPlanes+1));
    doBinDetail(RefOut, rp, ValProb, ibinX, ibinY,
                binXY + ibinXY + thread_i*nbinsX*nbinsY*(nPlanes+1));
    //}
  }
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


GetVal SetValueFunction(const VtPlotPar key)
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

  switch (key)
  {
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


void GetKeyName(const VtPlotPar key, char* buf)
{
  switch (key)
  {
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


/***************************************************************************************************
 *Calculates the produced gamma and transmitted neutron intensity according to NIMA 922(2019)p.98-107
 * Input parameters:
 *    -incI         :  Incident neutron intensuty before reflection [1/s]
 *    -reflectivity :  Reflectivity of mirror [1]
 *    -wl           :  neutron wavelength [Angstroem]
 *    -mVal         :  m-value of reflevtion
 *    -mirrMaxM     :  m-value of mirror
 *    -pOutParticles:  Pointer to a Escaping_MCPL_particles struct
****************************************************************************************************/
void CalcGammaAndNeutron(double incI, double reflectivity, double wl, double mVal,
                                            double mirrMaxM, Escaping_MCPL_particles* pOutParticles)
{
  double Niabs, Tiabs;
  double lossNeutronI = incI * (1 - reflectivity);
  int i;

  // Total reflection
  if (mVal<=1)
  {
    double Niabswl=0.1348;
    Niabs=Niabswl*wl*lossNeutronI;
    Tiabs=0;

    if ((pOutParticles->neutronInt=lossNeutronI-Niabs-Tiabs)<0)
    { // check if absorption is larger than the loss
      double q;
      q=lossNeutronI/(Niabs+Tiabs);
      Niabs*=q;
      Tiabs*=q;
      pOutParticles->neutronInt=0;
    }

    for (i=0; i<MAX_GAMMA_NUM; ++i)
    {
        pOutParticles->gammaInt[i] = Niabs*gamma_fractionNi[i] + Tiabs*gamma_fractionTi[i];
    }
  }

  // Supermirror reflection
  if ( (mVal>1) && (mVal<=mirrMaxM) )
  {
    Niabs = (0.005 + 0.005 * (mVal - 1))*incI;
    Tiabs =  0.0045 * (mVal - 1)*incI;
    if ((pOutParticles->neutronInt=lossNeutronI-Niabs-Tiabs)<0)
    { // absorption is larger than the loss
      double q;
      q=lossNeutronI/(Niabs+Tiabs);
      Niabs*=q;
      Tiabs*=q;
      pOutParticles->neutronInt=0;
    }
    for (i=0; i<MAX_GAMMA_NUM; ++i)
    {
      pOutParticles->gammaInt[i] = Niabs*gamma_fractionNi[i] + Tiabs*gamma_fractionTi[i];
    }
  }

  // Neutron escape
  if(mVal>mirrMaxM){
    Niabs=0.0025 * (mirrMaxM +0.1) * (mirrMaxM +0.1)/mVal*incI;
    Tiabs=0.00225 * (mirrMaxM - 0.9) * (mirrMaxM + 0.1)/mVal*incI;

    if ((pOutParticles->neutronInt=lossNeutronI-Niabs-Tiabs)<0){ // absorption is larger than the loss
      double q;
      q=lossNeutronI/(Niabs+Tiabs);
      Niabs*=q;
      Tiabs*=q;
      pOutParticles->neutronInt=0;
    }
    for (i=0; i<MAX_GAMMA_NUM; ++i){
      pOutParticles->gammaInt[i] = Niabs*gamma_fractionNi[i] + Tiabs*gamma_fractionTi[i];
    }
  }
}


#endif
