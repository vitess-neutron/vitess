/**********************************************************************************************/
/*  VITESS module 'bender'                                                                    */
/*                                                                                            */
/* This module simulates a bender of circular or straight channels with channel cross-talk    */
/*   (bender is horizontally bent, absorption inside channels can be considered               */
/*    exit width and height can differ from entrance width and height                         */
/*    magnetic field is vertical, direction up  )                                             */
/*                                                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to  */
/* the authors.                                                                               */
/*                                                                                            */
/* 1.00  Jun 2001  S. Manoshin     initial version                                            */
/* 1.01  Dec 2001  K. Lieutenant   new  : surface file generation, softabort, loss in bender  */
/*                                 impr.: graphics acceleration, new scale for the picture    */
/*                                 corr.: transformation, calc. of bender width and step size */
/*                                 simpl: all radii identical                                 */
/*                 M. Fromme       graphics for Windows                                       */
/* 1.02  Jan 2002  K. Lieutenant   reorganisation, radii centered                             */
/* 1.03  Mar 2002  S. Manoshin     Feature: if radius of curvature zero => straight           */
/*                                 line instead circle for bender surfaces and axis.          */
/*                                 Also surfaces radius is negative possible                  */
/*                                 New option: Spin quantisation (Sx,Sy,Sz) is defined by     */
/*                                 user -V option; 0,1,2 - axis 0X, OY, OZ;                   */
/*                                 Some reorganisation of data structure, fixed graphics bugs */
/*                                 Test part of program moved in separated function and       */
/*                                 included comparing test value with near zero               */
/*                                 New option: -t : 1 - test activated 0 - test disactivated  */
/*                                 Fixed bug in visualize part                                */
/* 1.1alpha Apr 2002  S. Manoshin  Neutron crosstalk between channels under construction      */
/* 1.1beta  May 2002  S. Manoshin  Initial version: neutrons are travels via bender channel   */
/*                                 WITH CROSSTALK Between channels; Under testing             */
/*                                 bAbsTransCrit: Behaviour of not reflected neutrons         */
/*                                 REORGANISATION, add some comments, abutment length add     */
/*                                 The module is divided into 3 functions and main program    */
/*                                 Neutron, which pass via all extreme surfaces - ABSORBING   */
/*                                     top, bottom, right, left                                */
/*                                 Add materials for neutron flux attenuation between         */
/*                                 bender channels                                            */
/* 1.3alpha June 2002 S. Manoshin  Add possibility to read transmission characteristics of    */
/*                                 materials between channels from file                       */
/*                                 Generation surface part is remove from module              */
/*                                 Add neutron flux attenuation inside bender channels        */
/* 1.3beta  Jul 2002  S. Manoshin  Pre-Realization in the VITESS 2.3                          */
/* 1.4      Jul 2002  S. Manoshin  Realization in the VITESS 2.3                              */
/* 1.5      Mar 2003  S. Manoshin  Fixed bug for gravity, gravity may apply ONLY for first    */
/*                                 order planes                                               */
/*                                 Number of channels (or planes) now defined, not fixed!     */
/* 1.6      Oct 2003  S. Manoshin  Corrected some mistakes with output: Interpolation         */
/*                                 func. Improve checking of input datas for interpolation    */
/* 1.7      Feb 2004  S. Manoshin  Visualise only first 10000 trajectories,                   */
/*                                 if visualisation was activated                             */
/*                                 Choose the output device : screen, file or both            */
/*                                 New external variable gselec                               */
/* 1.8    Nov 2013  D. Nekrassov   M-values as input                                          */
/* 1.8a   Feb 2018  K. Lieutenant  silicon data for 0.4 Ang added                             */
/* 1.9    Feb 2020  K. Lieutenant  tidy up, new reflectivity calculation, new file handling   */
/* 1.10   Dec 2025  K. Lieutenant  attenuation in channel separated from channel cross-over   */
/* 1.11   Mar 2026  K. Lieutenant  tidy up (parameter description, renaming, ...)             */
/* 1.12   Mar 2026  K. Lieutenant  use of improved functions in 'bender_inter_data'           */
/**********************************************************************************************/


#include <string.h>
#include <math.h>

#ifdef VT_GRAPH
# include "cpgplot.h"
  double timev, timevmin, timevmax;  /* */
  double timestep; /* step for all surfaces */
  double timestep1; /* step for extreme (left and right) surfaces */
  extern int do_visualize;
  int do_visualise; /* default : no visualisation */
  long number_vis_tr=0; /* counter : number of trajectories, which was visualised */
  long  cancel_vis=0; /* cancel visualisation */
  extern int gselec;           //    -o      [-]  flag: output of the internal visualization: 1: display only  2: file only  3: both   (defined in cpgplot.c)
#endif


#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "message.h"
#include "bender_inter_data.h"
#include "bender.h"


/******************************/
/** Inline Functions         **/
/******************************/
static FILE *openNFile(char *name)
{
  return OpenParameterFile(name, TRUE, "r");
}


/*************************************************/
/** Prototypes of 'init' and internal functions **/
/*************************************************/
void OwnInit(int argc, char *argv[]);                                                     // reads input parameters and initializes global variables
void EvalInput();                                                                         // Analyses input parameters and prepares attenuation
void SetGeometry(char* sColor);                                                           // calculates data for both visualization tools
int  LoadReflFile(FILE* pReflFile, double* pData, const char* sWall, const char* sSpin);  // reads a reflectivity file
void FillReflContainer(double array[1000], double m);

// subroutines to calculate the visualization data
void CreateVisualisationGeometryCurvedChannels  (double xStart, double xEnd, double yStart, double yEnd, double dYcirc, double radius, double entranceHeight, double dZ);
void CreateVisualisationGeometryStraightChannels(double xStart, double xEnd, double yStart, double yEnd, double entranceHeight, double dZ);
void CreateVisualisationGeometryTopBottom(double x1Start, double x1End, double y1Start, double y1End, double dY1circ, double radius1,
                                          double x2Start, double x2End, double y2Start, double y2End, double dY2circ, double radius2,
                                          double entranceHeight, double dZ, char* sColor);

void DefineTriangle(VtTriangle* triangle, VectorType v1, VectorType v2, VectorType v3);


/******************************/
/** Global Variables         **/
/******************************/

// Input parameters
// ----------------
double BenderEntrHeight=0.0,      //    -h     [cm]  Height of the entrance of the bender
       BenderExitHeight=0.0;      //    -H     [cm]  Height of the exit of the bender
double Radius=0.0,                //    -R     [cm]  Radius of curvature of the base circle, i.e. the center of the bender (if zero, plane surfaces are assumed)
       length=0.0,                //    -l     [cm]  Length of the bender
       spacer=0.0;                //    -s     [cm]  Thickness of the material dividing the bender into channels

double mNumber[2][3];             // -b -B -d   [-]  m value of the left, right, top/bottom wall for spin-up 
                                  // -e -E -f   [-]    and spin-down neutrons
VtWndMat eChnlMat=VT_WND_VAC,    //    -c      [-]  enum: material of bender channels     : 0: from file, 5: Silicon, 6: Vacuum
         eAbsMatL=VT_WND_GD,     //    -z      [-]  enum: absorbing material on left side : 0: from file, 1: gadolinium, 2: cadmium, 3: Bor10, 4: Eu, 6: Vacuum 
         eAbsMatR=VT_WND_GD;     //    -w      [-]  enum: absorbing material on right side: 0: from file, 1: gadolinium, 2: cadmium, 3: Bor10, 4: Eu, 6: Vacuum 

short  bAbsTransCrit=0;           //    -g      [-]  flag: behaviour of not reflected neutrons: 0: absorbed  1: possible passage to neighboring channels
short  keytest  =0;               //    -t      [-]  flag: test of the bender geometry          0: no  1: yes
short  keyVisAct=0;               //    -y      [-]  flag: internal visualization (in addition to the general instrument and trajectory visualization:  0: no  1: yes
short  keypol   =0;               //    -p      [-]  flag: usage of polarization, i.e. separate reflectivity files used
short  qspin    =0;               //    -V      [-]  axis for spin quantisation: 0: X axis     1: Y axis   2: Z axis

double disabut=0.0;               //    -a     [cm]  Length of the abutment loss area at the exit of the bender
double surfacerough=0.0;          //    -r     [deg] Surface waviness: maximal angle 'zeta' of deviation from normal in degree - converted to tan(zeta)

char  *ReflFileNameUpL=NULL;      //    -i      [-]  Name of the reflectivity file for the left plane and spin-up neutrons 
char  *ReflFileNameUpR=NULL;      //    -m      [-]  Name of the reflectivity file for the right plane and spin-up neutrons
char  *ReflFileNameUpTB=NULL;     //    -k      [-]  Name of the reflectivity file for top and bottom plane and spin-up neutrons
char  *ReflFileNameDownL=NULL;      //    -I      [-]  Name of the reflectivity file for the left plane and spin-down neutrons 
char  *ReflFileNameDownR=NULL;      //    -M      [-]  Name of the reflectivity file for the right plane and spin-down neutrons
char  *ReflFileNameDownTB=NULL;     //    -K      [-]  Name of the reflectivity file for top and bottom plane and spin-down neutrons

char  *TransFileName0=NULL;       //    -C      [-]  Name of the file characterizing the attenuation as a function of wavelength for the channel material  
char  *TransFileName1=NULL;       //    -T      [-]  Name of the file characterizing the attenuation of the material behind the reflecting surface on the left side of the channel
char  *TransFileName2=NULL;       //    -O      [-]  Name of the file characterizing the attenuation of the material behind the reflecting surface on the right side of the channel

char  *SurfacesFileName=NULL;     //    -u      [-]  Name of the input file defining the positions of all bender channels at entry and exit and their radii
char  *sInfoFileName=NULL;        //    -A      [-]  Name of the output file containing information about the bender geometry

// Parameters determined from input parameters
// -------------------------------------------
double EntrMin=9999999.0, EntrMax=-9999999.0,
       ExitMin=9999999.0, ExitMax=-9999999.0;
double BenderEntrWidth=0.0,       // Width of the entrance of the bender
       BenderExitWidth=0.0;       // Width of the exit of the bender

BenderChannel BenderCh;
Bender        MyBender;

FILE  *pReflFileUpL =NULL;        // Reflectivity file for the left plane and spin-up neutrons 
FILE  *pReflFileUpR =NULL;        // Reflectivity file for the right plane and spin-up neutrons
FILE  *pReflFileUpTB=NULL;        // Rreflectivity file for top and bottom plane and spin-up neutrons
FILE  *pReflFileDownL =NULL;      // Reflectivity file for the left plane and spin-down neutrons 
FILE  *pReflFileDownR =NULL;      // Reflectivity file for the right plane and spin-down neutrons
FILE  *pReflFileDownTB=NULL;      // Reflectivity file for top and bottom plane and spin-down neutrons
                                  
FILE  *pAttenFileChnl=NULL;       // File characterizing the attenuation of of the bender channel material  
FILE  *pAttenFileL=NULL;          // File characterizing the attenuation of the material behind the reflecting surface on the left side of the channel
FILE  *pAttenFileR=NULL;          // File characterizing the attenuation of the material behind the reflecting surface on the right side of the channel 
                                  
FILE  *pSurfaceFile=NULL;         // Input file defining the positions of all bender channels at entry and exit and their radii
FILE  *pInfoFile=NULL;            // Output file containing information about the bender geometry 

// internal visualisation for Windows and generation of file for the picture */
#ifdef DO_WIN32
  const char *GraphDev = "bender.ps";
#else
  const char *GraphDev = "bender.png";
#endif
char fullGraphDev[CHAR_BUF_SMALL];

// geometry data
double beta=0.0;
double surfaceradius[N_SURF], entrdiscenter[N_SURF], exitdiscenter[N_SURF];
long   nSurfaces;

// reflectivity data
static double aReflUpL  [1000], aReflUpR  [1000], aReflUpTB  [1000];  
static double aReflDownL[1000], aReflDownR[1000], aReflDownTB[1000];

// attenuation data
long   nMuValR=0, nMuValL=0, nMuValChnl=0;
double aLmbdL[MAX_MU+1], aLmbdR[MAX_MU+1], aLmbdChnl[MAX_MU+1],
       aMuL  [MAX_MU+1], aMuR  [MAX_MU+1], aMuChnl  [MAX_MU+1];

// visualization data
int    numberRectangles;
int    numberTriangles;

double X2, Y2, COSB, SINB;     /* for defining base circle */

double XRL[N_SURF], YRL[N_SURF]; /* for calculating converging surface */
double XENL[N_SURF], XEXL[N_SURF], YENL[N_SURF], YEXL[N_SURF], RADL[N_SURF];
double XCENL[N_SURF], YCENL[N_SURF], XTMPL[N_SURF], YTMPL[N_SURF], ALPHAL[N_SURF];

double XRR[N_SURF], YRR[N_SURF]; /* for calculating converging surface */
double XENR[N_SURF], XEXR[N_SURF], YENR[N_SURF], YEXR[N_SURF], RADR[N_SURF];
double XCENR[N_SURF], YCENR[N_SURF], XTMPR[N_SURF], YTMPR[N_SURF], ALPHAR[N_SURF];


// Other parameters
// ----------------
const double lengthGeomPiece = 50.0;  // Length of a geometry element in cm a surface consists of for x3d visualisation


/******************************/
/**      MAIN Program        **/
/******************************/

int main(int argc, char *argv[])
{
  /********************************************************************************************/
  /* This module reads in a file of neutron structures, and defines a neutron Bender as a set  */
  /* of five infinite planes with a global critical angle. It outputs the coordinates and time */
  /* displacement of any neutrons that pass through the Bender without being absorbed.         */
  /*                                                                                           */
  /* Anything not directly commented is an InputNeutrons or an output routine.                 */
  /********************************************************************************************/
  long     i=0, j=0, 
           jChan =0;   // number of the channel, into which the neutron enters
  double   TimeOF1 =0.0;
  double   rightend=0.0, leftend=0.0;
  Neutron  Output;

#ifdef VT_GRAPH
  gselec = 1 ; /* Activate visualisation device -screen */
#endif

  // reading of input data and initialisation
  // ----------------------------------------
  _eModule=MCN_BENDER;
  InitNeutron(&Output);

  Init(argc, argv,_eModule);
  PrintModuleName(_eModule, "1.12");
  OwnInit  (argc, argv);
  MsgInit  ();
  EvalInput();
  SetGeometry("yellow");

  /* test geometry of bender */
  if (keytest == 1)
    GeometryTestBender(MyBender, XENR, YENR, XENL, YENL, XEXR, YEXR, XEXL, YEXL,
                       BenderEntrHeight, BenderExitHeight, beta, nSurfaces);

  /*  set allowed spin direction: qspin must be  0 or 1 or 2 ONLY */
  if (qspin < 0 || qspin > 2) qspin = 0;
  if (qspin == 0) fprintf(LogFilePtr,"Magnetic field direction - AXIS OX \n");
  if (qspin == 1) fprintf(LogFilePtr,"Magnetic field direction - AXIS OY \n");
  if (qspin == 2) fprintf(LogFilePtr,"Magnetic field direction - AXIS OZ \n");

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp = TRUE;

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons())
  {
    for(i=0; i<NumNeutGot; i++)
    {

  #ifdef VT_GRAPH
      if (do_visualise)
      {
        if  (number_vis_tr == BufferSize)
        {
          do_visualise = 0 != 0; /* stop visualisation */
          cancel_vis = 1;
          fprintf(LogFilePtr,"Visualisation is stopped\n");
        }
      }
  #endif

      CHECK

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        TimeOF1 = 0.0;

        /*  InputNeutrons[i].Position.X = 0.0;   !!!!!!!! */
        /****************************************************************************************/
        /* Check to see if the neutron is initially in the entrance to the bender...             */
        /****************************************************************************************/

        if (fabs(InputNeutrons[i].Position[2])>BenderEntrHeight/2.0) continue;

        /****************choose the channel******************/
        /* include thickness  */

        for (j=1;j<=(nSurfaces-1);j++)
        {
          rightend=YENR[j];
          leftend=YENL[j];
          if((rightend<InputNeutrons[i].Position[1])&&(leftend>InputNeutrons[i].Position[1]))
          {
            jChan = j;
            break;
          }
        }

        if(j==nSurfaces)
          continue;  /*neutron blocked by spacer*/

        /* Check the quantization of polarization */
        if (keypol == 1 && fabs(InputNeutrons[i].Spin[qspin]) != 1.0)
          Error("Illegal spin quantisation. Check the Spin value");

        /******************************************************************************************/
        /* Pass a pointer to the neutron and the Bender structure variable to a subroutine to do  */
        /* the donkey work. The return value is the total value of the time of flight through the */
        /* Bender, or -1.0 if it missed all plates and the exit (should be impossible).           */
        /******************************************************************************************/

        /* Choose the behavior of neutrons between channels */

        if (bAbsTransCrit == 0)
        {
          /* Neutrons travel WITHOUT crosstalk between channels */
          TimeOF1 = PathThroughChannelGravOrder2(&InputNeutrons[i], MyBender, BenderCh,
                                                 jChan,        disabut,
                                                 aReflUpL,     aReflUpR,      aReflUpTB,
                                                 aReflDownL,   aReflDownR,    aReflDownTB,
                                                 surfacerough, keypol,        qspin,
                                                 entrdiscenter,exitdiscenter, spacer,
                                                 eChnlMat,     aLmbdChnl,     aMuChnl,  nMuValChnl);
        }
        else
        {
          /* Neutrons travel WITH crosstalk between channels */
          TimeOF1 = PathThroughBenderGravOrder2(&InputNeutrons[i], MyBender, BenderCh,
                                                jChan,        nSurfaces,     disabut,
                                                aReflUpL,     aReflUpR,      aReflUpTB,
                                                aReflDownL,   aReflDownR,    aReflDownTB,
                                                surfacerough, keypol,        qspin,
                                                entrdiscenter,exitdiscenter, spacer,
                                                eChnlMat,     aLmbdChnl,     aMuChnl,   nMuValChnl,
                                                eAbsMatL,     aLmbdL,        aMuL,      nMuValL,
                                                eAbsMatR,     aLmbdR,        aMuR,      nMuValR);
        }

        if(TimeOF1 == -1.0)  continue;
        if(TimeOF1 == -10000.0) exit(-1);

        /****************************************************************************************/
        /* Transform the coordinates.                              */
        /* X must be always renormalized to zero...                                                  */
        /****************************************************************************************/

        /* KL: correction: recursion found in calculation of InputNeutrons[i].Position[1], ...Vector[1]
          InputNeutrons[i].Position[0], ...Vector[0]  were already changed !           */

        Output = InputNeutrons[i];

        /* KL: correction: transformation: move coordinate system to the center of rotation
                        rotate
                            move coordinate system back       */
        /*          InputNeutrons[i].Position[1] -= Radius;
        Output.Position[0] =  (InputNeutrons[i].Position[0])*COSB + (InputNeutrons[i].Position[1])*SINB;
        Output.Position[1] = -(InputNeutrons[i].Position[0])*SINB + (InputNeutrons[i].Position[1])*COSB;
        Output.Position[1] += Radius;  */

        /* SM: Similar */

        Output.Position[0] =  (InputNeutrons[i].Position[0]-X2)*COSB + (InputNeutrons[i].Position[1]-Y2)*SINB;
        Output.Position[1] = -(InputNeutrons[i].Position[0]-X2)*SINB + (InputNeutrons[i].Position[1]-Y2)*COSB;

        Output.Vector[0] =  (InputNeutrons[i].Vector[0])*COSB + (InputNeutrons[i].Vector[1])*SINB;
        Output.Vector[1] = -(InputNeutrons[i].Vector[0])*SINB + (InputNeutrons[i].Vector[1])*COSB;

        if (fabs(Output.Position[2])>BenderExitHeight/2.0) continue;

        /****************************************************************************************/
        /* Add the time needed to travel inside Bender.                                   */
        /****************************************************************************************/
        Output.Time = Output.Time + TimeOF1;
        /****************************************************************************************/
        /* Count this as a success.                                                             */
        /****************************************************************************************/

  #ifdef VT_GRAPH
        if (do_visualise)
        {
          number_vis_tr = number_vis_tr + 1;
        }
  #endif

        WriteNeutron(&Output);
      }
    }
  }

 my_exit:

#ifdef VT_GRAPH
  if ((do_visualise)||(cancel_vis == 1))
  {
    fprintf(LogFilePtr,"Close graphical window\n");
    cpgclos();
  }
#endif

  Cleanup(X2, Y2, 0.0, beta, 0.0);

  if (pInfoFile!=NULL)
    fclose(pInfoFile);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  long  i=0;

  // initialization
  // --------------
  mNumber[0][0] = -1; mNumber[0][1] = -1; mNumber[0][2] = -1;
  mNumber[1][0] = -1; mNumber[1][1] = -1; mNumber[1][2] = -1;

  for (i=0;i<1000; i++)
  { 
    aReflUpL[i] =0.0; aReflDownL[i]=0.0; 
    aReflUpR[i] =0.0; aReflDownR[i]=0.0; 
    aReflUpTB[i]=0.0; aReflDownTB[i]=0.0;
  }

  for (i=0; i<MAX_MU; i++)
  {
    aLmbdL[i]   =0.0; aMuL[i]   =0.0;
    aLmbdR[i]   =0.0; aMuR[i]   =0.0;
    aLmbdChnl[i]=0.0; aMuChnl[i]=0.0;
  }

  for (i=0; i<=N_SURF_S; i++)
  {
    entrdiscenter[i] = 0.0;
    exitdiscenter[i] = 0.0;
    surfaceradius[i] = 0.0;
  }

  // Reading parameters
  // --------------
  for (i=1; i<argc; i++)
  {
    char *a, *arg;
    a = argv[i];
    if (*a != '-') continue;
    arg = a + 2;
    switch(a[1])
    {
      case 'b':  /* up, left plane */
        mNumber[0][0] = atof(&argv[i][2]);
        break;
      case 'B':  /* up, right plane */
        mNumber[0][1] = atof(&argv[i][2]);
        break;
       case 'd':  /* up, top/bottom plane */
        mNumber[0][2] = atof(&argv[i][2]);
        break;

      case 'e':  /* down, left plane */
        mNumber[1][0] = atof(&argv[i][2]);
        break;
      case 'E':  /* down, right plane */
        mNumber[1][1] = atof(&argv[i][2]);
        break;
       case 'f':  /* down, top/bottom plane */
        mNumber[1][2] = atof(&argv[i][2]);
        break;

      case 'i':
        pReflFileUpL = openNFile((ReflFileNameUpL = arg));
        LoadReflFile(pReflFileUpL,  aReflUpL, "left",  "up");
        break;
      case 'm':
        pReflFileUpR = openNFile((ReflFileNameUpR = arg));
        LoadReflFile(pReflFileUpR,  aReflUpR, "right", "up");
        break;
      case 'k':
        pReflFileUpTB = openNFile((ReflFileNameUpTB = arg));
        LoadReflFile(pReflFileUpTB, aReflUpTB,"top and bottom", "up");
        break;
      case 'I':
        pReflFileDownL = openNFile((ReflFileNameDownL = arg));
        LoadReflFile(pReflFileDownL,  aReflDownL, "left", "down");
        break;
      case 'M':
        pReflFileDownR = openNFile((ReflFileNameDownR = arg));
        LoadReflFile(pReflFileDownR,  aReflDownR, "right", "down");
        break;
      case 'K':
        pReflFileDownTB = openNFile((ReflFileNameDownTB = arg));
        LoadReflFile(pReflFileDownTB, aReflDownTB,"top and bottom", "down");
        break;

      case 'u':
        SurfacesFileName = arg;
        break;
      case 'A':
        sInfoFileName = arg;
        break;

      case 'h':
        BenderEntrHeight =  atof(arg); /* in cm */
        break;
      case 'H':
        BenderExitHeight = atof(arg);  /* in cm */
        break;

      case 'R':
        Radius =  atof(arg); /* in cm */
        break;
      case 'l':
        length = atof(arg); /* length  of bender in cm */
        break;
      case 's':
        spacer =  atof(arg); /* width of bender channel border in cm */
        break;

      case 'a':
        disabut = atof(arg); /* abutment length, cm*/
        break;

      case 'c':
        eChnlMat = (VtWndMat)atoi(arg);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;
      case 'z':
        eAbsMatL = (VtWndMat)atoi(arg);  /* IN LEFT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;
      case 'w':
        eAbsMatR = (VtWndMat)atoi(arg);  /* IN RIGHT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;

      case 'C':
        pAttenFileChnl = openNFile((TransFileName0 = arg));
        break;
      case 'T':
        pAttenFileL = openNFile((TransFileName1 = arg));
        break;
      case 'O':
        pAttenFileR = openNFile((TransFileName2 = arg));
        break;

      case 'p':
        keypol =  (short) atoi(arg); /* Polarisaton: 1: separate reflectivity files  0: spin-up files als used for spin-down */
        break;

      case 't':
        keytest = (short) atoi(arg); /* test:  1: activated 0: deactivated */
        break;

      case 'o':
  #ifdef VT_GRAPH
        gselec = atoi(&argv[i][2]);
  #endif
        break;

      case 'y':
        keyVisAct = (short) atoi(arg);   /* for visualiztion */
  #ifdef VT_GRAPH
        do_visualise = keyVisAct != 0;
  #endif
        break;

      case 'r':
        surfacerough  =  atof(arg); /* Maximal angle of deviation from normal in degree */
        surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
        surfacerough  =  tan(surfacerough);
        break;

      case 'g':
        bAbsTransCrit = (short) atoi(arg);    /* behaviour of not reflected neutrons  */
        break;

      case 'V':
        qspin  =  (short) atoi(arg);          /*  magnetic field direction: 0: X axis  1: Y axis 2: Z axis */
        break;

      default:
        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n", a);
        exit(-1);
        break;
    }
  }

  return;
}


/*****************************************************************/
/** Checks and analyzes input parameters and writes to log file **/
/*****************************************************************/
void EvalInput()
{
  short keygrav_off=FALSE;    // flag: gravity cannot be handled by the mathematical functions used
  long  i=0;
  long  k=0, counter=0;
  double rdate[N_SURF_M3];

  // initialization
  // --------------
  for (i=0; i<=N_SURF_M3_S; i++)
    rdate[i] = 0.0;

  // checks of input parameters
  // --------------------------
  // Open surface file
  if (SurfacesFileName)
     pSurfaceFile = openNFile(SurfacesFileName);
  else
    Error("no surface file name given");

  // Attenuation inside channel
  if ((eChnlMat != VT_WND_FILE) && (eChnlMat != VT_WND_SI) && (eChnlMat != VT_WND_VAC))
    Error("No proper material for the bender channel chosen");
  else
    WriteMatInfo(eChnlMat, "Channel material");

  // Attenuation by transition from one channel to the next
  if (bAbsTransCrit != 0)
  {
    WriteMatInfo (eAbsMatL, "Material of left side of channel");
    WriteMatInfo (eAbsMatR, "Material of right side of channel");
  }

  // Special options
  if (disabut > 0.0)
    fprintf(LogFilePtr,"Inside bender abutment loss is enabled, abutment distance = %f6.3 cm \n",disabut);
  else
    fprintf(LogFilePtr,"Inside bender abutment loss is disabled \n");

  if (keypol == 1)
    fprintf(LogFilePtr,"The dependance of reflectivity on neutron polarization enabled \n");
  else
    fprintf(LogFilePtr,"The dependance of reflectivity on neutron polarization is disabled \n");

  if (surfacerough == 0.0)
    fprintf(LogFilePtr,"The reflecting surface is ideally smooth \n");
  else
    fprintf(LogFilePtr,"The reflecting surface is rough \n");

  // Guide dimensions
  if (BenderEntrHeight == 0.0)
    Error("You must enter the height of the bender\n");
 
  if (Radius < 0.0)
    Error("Value of radius must not be negative");

  if (length <= 0.0)
    Error("Bender length must not be negative or zero");

  if (BenderExitHeight == 0.0)
    BenderExitHeight = BenderEntrHeight;

  /* Fill reflectivity values for all walls, spin up and down, if files are not given */
  if (!pReflFileUpL)  FillReflContainer(aReflUpL,  mNumber[0][0]);
  if (!pReflFileUpR)  FillReflContainer(aReflUpR,  mNumber[0][1]);
  if (!pReflFileUpTB) FillReflContainer(aReflUpTB, mNumber[0][2]);
  if (!pReflFileDownL)  FillReflContainer(aReflDownL,  mNumber[1][0]);
  if (!pReflFileDownR)  FillReflContainer(aReflDownR,  mNumber[1][1]);
  if (!pReflFileDownTB) FillReflContainer(aReflDownTB, mNumber[1][2]);

  /* Read transmission for the bender channel */
  nMuValChnl = ReadAttenWnd(eChnlMat, aLmbdChnl, aMuChnl, MAX_MU, TransFileName0);

  /* If cross-over bwtween channels is considered, read transmission for the absorbing layerbender channel */
  if (bAbsTransCrit != 0)
  {
    nMuValL = ReadAttenWnd(eAbsMatL, aLmbdL, aMuL, MAX_MU, TransFileName1);
    nMuValR = ReadAttenWnd(eAbsMatR, aLmbdR, aMuR, MAX_MU, TransFileName2);
  }

  /* Read the bender geometry, i.e. the surfaces of the bender channels */
  if (SurfacesFileName !=NULL)
  {
    for(counter = 1; counter <= N_SURF_M3_S; counter++)
      if (fscanf(pSurfaceFile,"%lf",&rdate[counter])==EOF)
        break;

    fclose(pSurfaceFile);
  }
  else
  {
    fprintf(LogFilePtr,"\n ERROR: No Surface data. Check the data in the file. \n");
    exit(-1);
  }

  nSurfaces = (long)((counter-1)/3);
  fprintf(LogFilePtr,"Number of surfaces: %ld\n", nSurfaces);

  k = 1;
  for(i = 1; i <= nSurfaces; i++)
  {
    entrdiscenter[i] = rdate[k];
    exitdiscenter[i] = rdate[k+1];
    surfaceradius[i] = rdate[k+2];

    /* Automaticly disable gravity, if plane have a curvature */
    if (surfaceradius[i]!=0.0 && keygrav==1)
    {
      keygrav     = 0;
      keygrav_off = TRUE;    
    }
    k = k + 3;
  }

  /* Handling of gravity */
  if (keygrav==1)
  { fprintf(LogFilePtr, "Inside bender gravity is enabled\n");
  }
  else
  { if (keygrav_off==TRUE)
      fprintf(LogFilePtr, "Inside bender gravity is disabled for mathematical reasons");
    else
      fprintf(LogFilePtr, "Inside bender gravity is disabled by user");
  }

  if ((bAbsTransCrit != 0) && (nSurfaces == 2))
    Warning("Having only one channel and choosing transmission between channels does not make sense.");

  /* determine the bender entrance and exit width */
  for (i = 1; i <= nSurfaces; i++)
  {
    if (entrdiscenter[i] <= EntrMin) EntrMin = entrdiscenter[i];
    if (entrdiscenter[i] >= EntrMax) EntrMax = entrdiscenter[i];
    if (exitdiscenter[i] <= ExitMin) ExitMin = exitdiscenter[i];
    if (exitdiscenter[i] >= ExitMax) ExitMax = exitdiscenter[i];
  }

  BenderEntrWidth = fabs(EntrMax - EntrMin);
  BenderExitWidth = fabs(ExitMax - ExitMin);

  fprintf(LogFilePtr, "entrance: %8.4f - %8.4f cm\n", EntrMin, EntrMax);
  fprintf(LogFilePtr, "exit    : %8.4f - %8.4f cm\n", ExitMin, ExitMax);
  fprintf(LogFilePtr, "width   : %8.4f ->%8.4f cm\n", BenderEntrWidth, BenderExitWidth);

  return;
}


/*******************************************************************************************/
/* Calculates surfaces from the input parameters and the data for both visualization tools */
/*******************************************************************************************/
void SetGeometry(char* sColor)
{
  long  i=0;
  double dX, dZ,
         X1, Y1, xr, yr;              // to define the base circle
  double TMP1, TMP2, TMP3, TMP4;      /* Temporary for surfaces tests variables */
#ifdef VT_GRAPH
  double temp1=0.0, temp2=0.0;
#endif

  //Number of geometry elements for visualisation
  numberRectangles = ((int) ((length / lengthGeomPiece)+1.))*nSurfaces*2;
  numberTriangles  = ((int) ((length / lengthGeomPiece)+1 + nSurfaces)*4);

   if (bVisInstr)
   {
     stGeometry.pRectangle = calloc(numberRectangles, sizeof(VtRectangle));
     stGeometry.pTriangle = calloc(numberTriangles, sizeof(VtTriangle));
   }

  /* Define base circle or line */
  beta = 0.0;
  if (Radius != 0)
  {
    /* base circle */
    beta = length/Radius;

    /* the entrance center point*/
    X1 = 0.0;
    Y1 = 0.0;

    /* the origin of base circle */
    xr = X1;
    yr = Y1 + Radius;

    /* the exit center point */
    X2 = xr + Radius*sin(beta);
    Y2 = yr - Radius*cos(beta);
  }
  else
  {
    /* base line */
    beta = 0.0;

    /* the entrance center point*/
    X1 = 0.0;
    Y1 = 0.0;

    /* the origin of base circle, not actually */
    xr = 0.0;
    yr = 0.0;

    /* the exit center point */
    X2 = X1 + length;
    Y2 = 0.0;
  }


  /* for transform of system of coordimate */
  COSB = cos(beta);
  SINB = sin(beta);

  /* NOTE! X2, Y2, beta is base for TRANSFORM system of coordimats */
  /*  fprintf(LogFilePtr,"entp X1= %f  Y1= %f beta= %f \n", X1,Y1,beta);
      fprintf(LogFilePtr,"exip X2= %f  Y2= %f beta= %f \n", X2,Y2,beta);
      fprintf(LogFilePtr,"cent base ci xr= %f yr= %f \n",xr,yr); */

  TMP1 = sqrt((xr-X1)*(xr-X1) + (yr-Y1)*(yr-Y1));
  TMP2 = sqrt((xr-X2)*(xr-X2) + (yr-Y2)*(yr-Y2));
  //  fprintf(LogFilePtr,"DISTBC  %f  %f\n",TMP1, TMP2);

  /* top and bottom planes, possible converging or diverging */

  dX = length;
  dZ = (BenderExitHeight - BenderEntrHeight)/2.0;

  BenderCh.Surf[0].A = 0.0;
  BenderCh.Surf[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].C = 0.0;
  BenderCh.Surf[0].D = 0.0;
  BenderCh.Surf[0].E = 0.0;
  BenderCh.Surf[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].W = -BenderCh.Surf[0].F*(BenderEntrHeight/2.0);
  BenderCh.Surf[0].P = 0.0;
  BenderCh.Surf[0].Q = 0.0;
  BenderCh.Surf[0].R = 0.0;

  MyBender.SurfTopBottom[0].A = 0.0;
  MyBender.SurfTopBottom[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  MyBender.SurfTopBottom[0].C = 0.0;
  MyBender.SurfTopBottom[0].D = 0.0;
  MyBender.SurfTopBottom[0].E = 0.0;
  MyBender.SurfTopBottom[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  MyBender.SurfTopBottom[0].W = -MyBender.SurfTopBottom[0].F*(BenderEntrHeight/2.0);
  MyBender.SurfTopBottom[0].P = 0.0;
  MyBender.SurfTopBottom[0].Q = 0.0;
  MyBender.SurfTopBottom[0].R = 0.0;

  dZ=-dZ;

  BenderCh.Surf[1].A = 0.0;
  BenderCh.Surf[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].C = 0.0;
  BenderCh.Surf[1].D = 0.0;
  BenderCh.Surf[1].E = 0.0;
  BenderCh.Surf[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].W = BenderCh.Surf[1].F*(BenderEntrHeight/2.0);
  BenderCh.Surf[1].P = 0.0;
  BenderCh.Surf[1].Q = 0.0;
  BenderCh.Surf[1].R = 0.0;

  MyBender.SurfTopBottom[1].A = 0.0;
  MyBender.SurfTopBottom[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  MyBender.SurfTopBottom[1].C = 0.0;
  MyBender.SurfTopBottom[1].D = 0.0;
  MyBender.SurfTopBottom[1].E = 0.0;
  MyBender.SurfTopBottom[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  MyBender.SurfTopBottom[1].W = MyBender.SurfTopBottom[1].F*(BenderEntrHeight/2.0);
  MyBender.SurfTopBottom[1].P = 0.0;
  MyBender.SurfTopBottom[1].Q = 0.0;
  MyBender.SurfTopBottom[1].R = 0.0;

  /* Begin to build curved or straight surfaces */
  for(i = 1; i <= (nSurfaces-1); i++)
  {

    /* calculating the center of surface on given entrance, exit points and radius
        of curvature */
    /* calculating poins for circle FOR LEFT SURFACES*/

    /* fprintf(LogFilePtr,"Make channel %d \n",i); */

    XENL[i] = X1;
    YENL[i] = Y1 + (entrdiscenter[i+1]-0.5*spacer);

    XEXL[i] = X2 - (sin(beta))*(exitdiscenter[i+1]-0.5*spacer);
    YEXL[i] = Y2 + (cos(beta))*(exitdiscenter[i+1]-0.5*spacer);
    RADL[i] = surfaceradius[i+1];

    if (RADL[i] != 0.0)
    {
      ALPHAL[i] = atan2((YEXL[i]-YENL[i]),(XEXL[i]-XENL[i]));

      XCENL[i] = 0.5*(XENL[i]+XEXL[i]);
      YCENL[i] = 0.5*(YENL[i]+YEXL[i]);
      XTMPL[i] = 0.0;
      YTMPL[i] = sqrt(RADL[i]*RADL[i]-0.25*(XENL[i]-XEXL[i])*(XENL[i]-XEXL[i])-0.25*(YENL[i]-YEXL[i])*(YENL[i]-YEXL[i]));

      /* feature for radius is negative    */
      YTMPL[i] = YTMPL[i]*RADL[i]/(fabs(RADL[i]));

      /* calculate the center of circle via transform of coordinate system  */
      XRL[i] = XTMPL[i]*cos(ALPHAL[i]) - YTMPL[i]*sin(ALPHAL[i]) + XCENL[i];
      YRL[i] = XTMPL[i]*sin(ALPHAL[i]) + YTMPL[i]*cos(ALPHAL[i]) + YCENL[i];

      TMP1 = sqrt((XRL[i]-XENL[i])*(XRL[i]-XENL[i]) + (YRL[i]-YENL[i])*(YRL[i]-YENL[i]));
      TMP2 = sqrt((XRL[i]-XEXL[i])*(XRL[i]-XEXL[i]) + (YRL[i]-YEXL[i])*(YRL[i]-YEXL[i]));

      MyBender.SurfLeft[i].A = 1.0;
      MyBender.SurfLeft[i].B = -2.0*XRL[i];
      MyBender.SurfLeft[i].C = 1.0;
      MyBender.SurfLeft[i].D = -2.0*YRL[i];
      MyBender.SurfLeft[i].E = 0.0;
      MyBender.SurfLeft[i].F = 0.0;
      MyBender.SurfLeft[i].W = XRL[i]*XRL[i] + YRL[i]*YRL[i] - (RADL[i])*(RADL[i]);
      MyBender.SurfLeft[i].P = 0.0;
      MyBender.SurfLeft[i].Q = 0.0;
      MyBender.SurfLeft[i].R = 0.0;

      CreateVisualisationGeometryCurvedChannels(XENL[i], XEXL[i], YENL[i], YEXL[i], Y2 + YENL[i]*(cos(beta) - 1.), RADL[i], BenderEntrHeight, dZ*2.);
    }
    else
    {
      /*  Straight Line  */
      MyBender.SurfLeft[i].A = 0.0;
      MyBender.SurfLeft[i].B = YENL[i] - YEXL[i];
      MyBender.SurfLeft[i].C = 0.0;
      MyBender.SurfLeft[i].D = XEXL[i] - XENL[i];
      MyBender.SurfLeft[i].E = 0.0;
      MyBender.SurfLeft[i].F = 0.0;
      MyBender.SurfLeft[i].W = YEXL[i]*XENL[i] - YENL[i]*XEXL[i];
      MyBender.SurfLeft[i].P = 0.0;
      MyBender.SurfLeft[i].Q = 0.0;
      MyBender.SurfLeft[i].R = 0.0;

      CreateVisualisationGeometryStraightChannels(XENL[i],  XEXL[i],  YENL[i], YEXL[i], BenderEntrHeight, dZ*2.);
    }

    /* FOR RIGHT SURFACES */
    XENR[i] = X1;
    YENR[i] = Y1 + (entrdiscenter[i]+0.5*spacer);

    XEXR[i] = X2 - (sin(beta))*(exitdiscenter[i]+0.5*spacer);
    YEXR[i] = Y2 + (cos(beta))*(exitdiscenter[i]+0.5*spacer);
    RADR[i] = surfaceradius[i];

    if (RADR[i] != 0.0)
    {
      ALPHAR[i] = atan2((YEXR[i]-YENR[i]),(XEXR[i]-XENR[i]));

      XCENR[i] = 0.5*(XENR[i]+XEXR[i]);
      YCENR[i] = 0.5*(YENR[i]+YEXR[i]);
      XTMPR[i] = 0.0;
      YTMPR[i] = sqrt(RADR[i]*RADR[i]-0.25*(XENR[i]-XEXR[i])*(XENR[i]-XEXR[i])-0.25*(YENR[i]-YEXR[i])*(YENR[i]-YEXR[i]));

      /* feature for radius is negative    */

      YTMPR[i] = YTMPR[i]*RADR[i]/(fabs(RADR[i]));

      /* calculate the center of circle via transform of coordinate system*/

      XRR[i] = XTMPR[i]*cos(ALPHAR[i]) - YTMPR[i]*sin(ALPHAR[i]) + XCENR[i];
      YRR[i] = XTMPR[i]*sin(ALPHAR[i]) + YTMPR[i]*cos(ALPHAR[i]) + YCENR[i];

      MyBender.SurfRight[i].A = 1.0;
      MyBender.SurfRight[i].B = -2.0*XRR[i];
      MyBender.SurfRight[i].C = 1.0;
      MyBender.SurfRight[i].D = -2.0*YRR[i];
      MyBender.SurfRight[i].E = 0.0;
      MyBender.SurfRight[i].F = 0.0;
      MyBender.SurfRight[i].W = XRR[i]*XRR[i] + YRR[i]*YRR[i] - (RADR[i])*(RADR[i]);
      MyBender.SurfRight[i].P = 0.0;
      MyBender.SurfRight[i].Q = 0.0;
      MyBender.SurfRight[i].R = 0.0;

      TMP3 = sqrt((XRR[i]-XENR[i])*(XRR[i]-XENR[i]) + (YRR[i]-YENR[i])*(YRR[i]-YENR[i]));
      TMP4 = sqrt((XRR[i]-XEXR[i])*(XRR[i]-XEXR[i]) + (YRR[i]-YEXR[i])*(YRR[i]-YEXR[i]));

      CreateVisualisationGeometryCurvedChannels(XENR[i], XEXR[i], YENR[i], YEXR[i], Y2 + YENR[i]*(cos(beta) - 1.), RADR[i], BenderEntrHeight, dZ*2.);
    }
    else
    {
      /*  Straight line  */
      MyBender.SurfRight[i].A = 0.0;
      MyBender.SurfRight[i].B = YENR[i] - YEXR[i];
      MyBender.SurfRight[i].C = 0.0;
      MyBender.SurfRight[i].D = XEXR[i] - XENR[i];
      MyBender.SurfRight[i].E = 0.0;
      MyBender.SurfRight[i].F = 0.0;
      MyBender.SurfRight[i].W = YEXR[i]*XENR[i] - YENR[i]*XEXR[i];
      MyBender.SurfRight[i].P = 0.0;
      MyBender.SurfRight[i].Q = 0.0;
      MyBender.SurfRight[i].R = 0.0;

      CreateVisualisationGeometryStraightChannels(XENR[i], XEXR[i], YENR[i],  YEXR[i], BenderEntrHeight, dZ*2.);
    }

 
  /*  Exit surface  */
    MyBender.SurfExit[i].A = 0.0;
    MyBender.SurfExit[i].B = cos(beta);
    MyBender.SurfExit[i].C = 0.0;
    MyBender.SurfExit[i].D = sin(beta);
    MyBender.SurfExit[i].E = 0.0;
    MyBender.SurfExit[i].F = 0.0;
    MyBender.SurfExit[i].W = -1.0*(X2*cos(beta) + Y2*sin(beta));
    MyBender.SurfExit[i].P = 0.0;
    MyBender.SurfExit[i].Q = 0.0;
    MyBender.SurfExit[i].R = 0.0;
  }

  i = nSurfaces - 1;
  CreateVisualisationGeometryTopBottom(XENL[i], XEXL[i], YENL[i], YEXL[i], Y2 + YENL[i]*(cos(beta) - 1.), RADL[i],
                                       XENR[1], XEXR[1], YENR[1], YEXR[1], Y2 + YENR[1]*(cos(beta) - 1.), RADR[1],
                                       BenderEntrHeight, dZ*2.0, sColor);

  /* Write the bender geometry to the file 'sInfoFileName' 
  if (sInfoFileName==NULL)
  {
    long len = strlen(SurfacesFileName);
    sInfoFileName = (char*)malloc(len+1);
    memcpy(sInfoFileName, SurfacesFileName, len-3);
    sInfoFileName[len-3] = '\0';
    strcat(sInfoFileName, "inf");
  } */
  if (sInfoFileName!=NULL)
    pInfoFile = OpenOutputFile(sInfoFileName, FALSE, "wt");

  if (pInfoFile!=NULL)
  {
    /* Output in file some of parameters of surfaces */
    fprintf(pInfoFile,"******************* UNIVERSAL BENDER module ************************ \n");
    fprintf(pInfoFile,"********** INFORMATION FOR FABRICATE OF CONVERGING BENDER ********** \n");
    fprintf(pInfoFile,"******************************************************************** \n");
    fprintf(pInfoFile,"***USER*INPUT*DATA***\n");
    for(i = 1; i <= nSurfaces; i++)
    {
      fprintf(pInfoFile,"Displace data: N = %ld  EN = %e cm  EX = %e cm  RAD = %e cm \n", i,
      entrdiscenter[i], exitdiscenter[i], surfaceradius[i]);
    }
    fprintf(pInfoFile,"Bender Entrance Width = %f cm ;Bender Exit Width = %f cm \n",BenderEntrWidth,BenderExitWidth);
    fprintf(pInfoFile,"Bender Entrance Height = %f cm ;Bender Exit Height = %f cm \n",BenderEntrHeight,BenderExitHeight);
    fprintf(pInfoFile,"Bender Length = %f cm; Bender surfaces thickness = %f cm \n",length,spacer);

    if (Radius != 0.0)
    {
      fprintf(pInfoFile,"Base bender axis - circle\n");
      fprintf(pInfoFile,"Radius of Curvature for base bender axis = %f cm \n",Radius);
      fprintf(pInfoFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",X1, Y1);
      fprintf(pInfoFile,"EXIT: Base points x = %f cm  y = %f cm \n",X2, Y2);
      fprintf(pInfoFile,"The coordinates of center of base circle x = %f cm  y = %f cm \n",xr,yr);
    }
    else
    {
      fprintf(pInfoFile,"Base bender axis - line\n");
      fprintf(pInfoFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",X1, Y1);
      fprintf(pInfoFile,"EXIT: Base points x = %f cm  y = %f cm \n",X2, Y2);
    }

    fprintf(pInfoFile,"****************************SURFACES******************************* \n");
    fprintf(pInfoFile,"******************************************************************* \n");
    fprintf(pInfoFile,"*****************THICKNESS*OF*SURFACES*IS*INCLUDED***************** \n");

    for(i = 1; i <= (nSurfaces-1); i++)
    {
      fprintf(pInfoFile,"Begin Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  \n",    XENR[i], YENR[i]);

      fprintf(pInfoFile,"Begin Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  \n",    XENL[i], YENL[i]);
      fprintf(pInfoFile,"------------------------------------------------------------------------------\n");

      fprintf(pInfoFile,"Exit Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  \n",    XEXR[i], YEXR[i]);

      fprintf(pInfoFile,"Exit Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  \n",    XEXL[i], YEXL[i]);

      fprintf(pInfoFile,"------------------------------------------------------------------------------\n");
      fprintf(pInfoFile,"==============================================================================\n");
    }

    fprintf(pInfoFile,"==============================================================================\n");

    for(i = 1; i <= (nSurfaces-1); i++)
    {
      fprintf(pInfoFile,"Center of RIGHT surface %ld X, Y; Radius\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",      XRR[i], YRR[i], RADR[i]);

      fprintf(pInfoFile,"Center of LEFT surface %ld X, Y; Radius\n",i);
      fprintf(pInfoFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",      XRL[i], YRL[i], RADL[i]);

      fprintf(pInfoFile,"------------------------------------------------------------------------------\n");

    }
    fprintf(LogFilePtr,"NOTE: Bender information is written to %s\n", sInfoFileName);
  }

#ifdef VT_GRAPH
  if (do_visualise)
  {
    /* visualise bender surfaces */

    /* choose device for output visualisation */
    if (!((gselec == 1)||(gselec == 2)||(gselec == 3)))
      Error("Incorrect output device, correct option -o, value 1,2 or 3");

    if (gselec == 1)
      fprintf(LogFilePtr,"Open visual output device - display\n");

    if (gselec == 2)
      fprintf(LogFilePtr,"Open visual output device - file \n");

    if (gselec == 3)
      fprintf(LogFilePtr,"Open visual output device - display+file \n");

    /* open graphical device */
    TotalPath(fullGraphDev, GraphDev, "", OUT_DIR);
    if (cpgopen(fullGraphDev) < 1)
    {
      fprintf(LogFilePtr,"ERROR: I cannot open plot device \n");
      exit(-1);
    }
    fprintf(LogFilePtr,"Number of visualised neutrons = %ld \n", BufferSize);

    /* KL: improvement: using a scale that brings the total bender on the screen */
    cpgenv(0.0,1.2*(length),
    1.2*Min(EntrMin,ExitMin+Y2),1.2*Max(EntrMax,ExitMax+Y2),0,0);

    cpgsfs(2);
    cpgsch(1.2);

    if (bAbsTransCrit == 0)
    {
      /* Neutrons travel WITHOUT crosstalk between channels */
      cpglab("X, cm ; Red Line - Device Axis","Y, cm", nSurfaces == 2 ?
      "Guide Surface Visualisation" :
      "Bender Surfaces Visualisation WITHOUT crosstalk between channels");
    }
    else
    {
      /* Neutrons travel WITH crosstalk between channels */
      cpglab("X, cm ; Red Line - Device Axis","Y, cm", nSurfaces == 2 ?
      "Guide Surface Visualisation" :
      "Bender Surfaces Visualisation WITH crosstalk between channels ");
    }

    cpgsci(2);
    cpgpt1(X1,Y1,-1);
    cpgpt1(X2,Y2,-1);

    /* Draw bender axis */
    if (Radius != 0.0)
    {
      /*  draw cirlcle axis */
      timevmin = 0.0;
      timevmax = 2*M_PI;
      timestep = (timevmax - timevmin)/5000.0;

      for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
      {
        temp1 = xr + Radius*cos(timev);
        temp2 = yr + Radius*sin(timev);
        cpgpt1(temp1,temp2,-1);
      }
    }
    else
    {
      /* draw line axis */
      cpgmove(X1,Y1);
      cpgdraw(1.2*(length),Y2);
    }

    /* visualize entrance and exit line */
    for(i = 1; i <= (nSurfaces-1); i++)
    {
      cpgsci(1);
      /* set poins */
      cpgpt1(XENL[i], YENL[i], 22);
      cpgpt1(XEXL[i], YEXL[i], 22);

      cpgpt1(XENR[i], YENR[i], 22);
      cpgpt1(XEXR[i], YEXR[i], 22);

      /* entrance line*/
      cpgmove(XENL[i],YENL[i]);
      cpgdraw(XENR[i],YENR[i]);

      /* exit line */
      cpgmove(XEXL[i],YEXL[i]);
      cpgdraw(XEXR[i],YEXR[i]);

      /* draw left surfaces */
      if (RADL[i] != 0.0)
      {
        /* SM: revised */
        if (RADL[i] > 0.0)
        {
          /* KL: acceleration of the graphics (see above) */
          timevmin = 2*M_PI - (acos((XENL[i] - XRL[i]) / (RADL[i]))); /* 0.0;      */
          timevmax = 2*M_PI - (acos((XEXL[i] - XRL[i]) / (RADL[i]))); /* 2.0*M_PI; */
          timestep = (timevmax - timevmin) / 100.0;                  /* 0.0001,   /(RADL[i]+RADR[i]) */
          timestep1 = timestep/2.0;
        }
        else
        {
          timevmin = acos((XEXL[i] - XRL[i])/fabs(RADL[i]));
          timevmax = acos((XENL[i] - XRL[i])/fabs(RADL[i]));
          timestep = (timevmax - timevmin) / 100.0;
          timestep1 = timestep/2.0;
        }

        /* draw all left surfaces*/
        cpgsci(4);
        for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
        {
          temp1 = XRL[i] + (fabs(RADL[i]))*cos(timev);
          temp2 = YRL[i] + (fabs(RADL[i]))*sin(timev);
          cpgpt1(temp1,temp2,-1);
        }

        /* draw extreme left surfaces */
        if (i == (nSurfaces-1))
        {
          cpgsci(1);
          for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
          {
            temp1 = XRL[i] + (fabs(RADL[i]))*cos(timev);
            temp2 = YRL[i] + (fabs(RADL[i]))*sin(timev);
            cpgpt1(temp1,temp2,-1);
          }
        }

      }
      else
      {
        /*  draw all surfaces lines   */
        cpgsci(4);
        cpgmove(XENL[i],YENL[i]);
        cpgdraw(XEXL[i],YEXL[i]);

        /*    draw left extreme surface line */
        if (i == (nSurfaces-1))
        {
          cpgsci(1);
          cpgmove(XENL[i],YENL[i]);
          cpgdraw(XEXL[i],YEXL[i]);
        }
      }

      /* draw right surfaces */

      if (RADR[i] != 0.0)
      {
        /* SM: revised */
        if (RADR[i] > 0.0)
        {
          /* KL: acceleration of the graphics (see above) */
          timevmin = 2*M_PI - (acos((XENR[i] - XRR[i]) / (RADR[i]))); /* 0.0;      */
          timevmax = 2*M_PI - (acos((XEXR[i] - XRR[i]) / (RADR[i]))); /* 2.0*M_PI; */
          timestep = (timevmax - timevmin) / 100.0;                  /* 0.0001,   /(RADL[i]+RADR[i]) */
          timestep1 = timestep/2.0;
        }
        else
        {
          timevmin = acos((XEXR[i] - XRR[i])/fabs(RADR[i]));
          timevmax = acos((XENR[i] - XRR[i])/fabs(RADR[i]));
          timestep = (timevmax - timevmin) / 100.0;
          timestep1 = timestep/2.0;
        }

        /* draw all right surfaces*/
        cpgsci(5);
        for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
        {
          temp1 = XRR[i] + (fabs(RADR[i]))*cos(timev);
          temp2 = YRR[i] + (fabs(RADR[i]))*sin(timev);
          cpgpt1(temp1,temp2,-1);
        }

        /* draw extreme right surface */
        if (i == 1)
        {
          cpgsci(2);
          for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
          {
            temp1 = XRR[i] + (fabs(RADR[i]))*cos(timev);
            temp2 = YRR[i] + (fabs(RADR[i]))*sin(timev);
            cpgpt1(temp1,temp2,-1);
          }
        }
      }
      else
      {
        /*  draw all right surfaces line  */
        cpgsci(5);
        cpgmove(XENR[i],YENR[i]);
        cpgdraw(XEXR[i],YEXR[i]);

        /*    draw right extreme surface line */
        if (i == 1)
        {
          cpgsci(2);
          cpgmove(XENR[i],YENR[i]);
          cpgdraw(XEXR[i],YEXR[i]);
        }
      }

    }
    /* end visualise bender surfaces */
  }
#endif
}


/*******************************************************/
/** Reads reflectivity data from file                 **/
/*******************************************************/
int LoadReflFile(FILE* pReflFile, double* pData, const char* sWall, const char* sSpin)
{
  short rc;
  int  nLines, iLine;
  char sBuffer[100];

  if (pReflFile != NULL)
  {
    nLines = LinesInFile(pReflFile);

    for(iLine=0; iLine < nLines; iLine++)
    {
      ReadLine(pReflFile, sBuffer, sizeof(sBuffer)-1);
      StrgScanLF(sBuffer, &pData[10*iLine], 10, 0);
    }
    fclose(pReflFile);
    rc=TRUE;
  }
  else
  { fprintf(LogFilePtr,"case of no reflectivity for %s surfaces of bender for spin %s neutrons \n", sWall, sSpin);
    rc=FALSE;
  }
  return rc;
}



void FillReflContainer(double array[1000], double m)
{
  int i;
  double lambda = 1./THETA_NI;

  if (m < 0)
    Error("m-Value below 0 is given! Module stops");

  for (i = 0; i < 1000; i++) array[i] = ReflTypical(QbyRefl(lambda, (double)i*0.01), m);

  return;

}


void CreateVisualisationGeometryCurvedChannels(double xStart, double xEnd, double yStart, double yEnd, double dYcirc, double radius, double entranceHeight, double dZ)
{

  double angle;
  double angleNorm;
  double angleElem;
  double totalLength;
  int nElements;
  int i;
  double x1, x2, y1, y2;
  double dY;
  double deltaY1, deltaY2;

  if (!bVisInstr) return;

  // xStart/=CmprFact; xEnd/=CmprFact;

  // fprintf(LogFilePtr,"xStart %f, xEnd %f, yStart %f, yEnd %f, radius %f, entrance height %f, dZ %f \n", xStart, xEnd, yStart, yEnd, radius, entranceHeight, dZ);

  angle = asin((xEnd - xStart)/radius);
  totalLength = radius*angle;

  deltaY1 = fabs(yEnd - yStart);
  deltaY2 = fabs(1. - cos(angle))*radius;

  // fprintf(LogFilePtr,"deltaY1 %f, deltaY2 %f  \n", deltaY1, deltaY2);

  if (deltaY1 >= deltaY2)
  {
    totalLength = sqrt(pow(totalLength, 2)  + pow(deltaY1 - deltaY2, 2));
  }
  else
  {
    totalLength = sqrt(pow(totalLength, 2)  - pow(deltaY1 - deltaY2, 2));
  }

  nElements = (int) (totalLength/lengthGeomPiece);

  x1 = xStart;
  y1 = yStart;

  // fprintf(LogFilePtr,"dY before %f, total length %f  \n", dYcirc, totalLength);

  // Take into account a possible converging
  dY = yEnd - (yStart + dYcirc);

  // fprintf(LogFilePtr,"dY %f \n", dY);

  angleElem = 0.;
  angleNorm = 0.;

  for (i = 0; i < nElements; i++)
  {
    double height;

    angleElem += 2.*asin(lengthGeomPiece/(2.*radius));

    y2 = yStart + radius*(1. - cos(angleElem)) + ((i + 1.)*lengthGeomPiece)*dY/totalLength;
    x2 = sqrt(pow(lengthGeomPiece, 2) - pow(y2 - y1, 2)) + x1; //xStart + radius*sin(angleElem);


    stGeometry.pRectangle[stGeometry.nRectangles].vCntr[0] = (x1 + x2)/2.0;
    stGeometry.pRectangle[stGeometry.nRectangles].vCntr[1] = (y1 + y2)/2.0*BlowUp;
    stGeometry.pRectangle[stGeometry.nRectangles].vCntr[2] = 0;

    angleNorm = atan(-(y2 - y1)*BlowUp/(x2 - x1));

    stGeometry.pRectangle[stGeometry.nRectangles].vNormal[0] = sin(angleNorm);
    stGeometry.pRectangle[stGeometry.nRectangles].vNormal[1] = cos(angleNorm);
    stGeometry.pRectangle[stGeometry.nRectangles].vNormal[2] = 0;

    stGeometry.pRectangle[stGeometry.nRectangles].Width = lengthGeomPiece;

    height = entranceHeight + ((1.0*i + 0.5)*lengthGeomPiece)*dZ/totalLength;
    stGeometry.pRectangle[stGeometry.nRectangles].Height = height*BlowUp;
    stGeometry.pRectangle[stGeometry.nRectangles].rotAngle = 0.;

    stGeometry.nRectangles++;

    x1 = x2;
    y1 = y2;

  }

  //angleElem += 2.*asin((totalLength - nElements*lengthGeomPiece)/(2.*radius));

  x2 = xEnd;
  y2 = yEnd;

  stGeometry.pRectangle[stGeometry.nRectangles].vCntr[0] = (x1 + x2)/2.;
  stGeometry.pRectangle[stGeometry.nRectangles].vCntr[1] = (y1 + y2)/2.0*BlowUp;
  stGeometry.pRectangle[stGeometry.nRectangles].vCntr[2] = 0;

  angleNorm = atan(-(y2 - y1)*BlowUp/(x2 - x1));

  stGeometry.pRectangle[stGeometry.nRectangles].vNormal[0] = sin(angleNorm);
  stGeometry.pRectangle[stGeometry.nRectangles].vNormal[1] = cos(angleNorm);
  stGeometry.pRectangle[stGeometry.nRectangles].vNormal[2] = 0;

  stGeometry.pRectangle[stGeometry.nRectangles].Width    = totalLength - nElements*lengthGeomPiece;
  stGeometry.pRectangle[stGeometry.nRectangles].Height   = (entranceHeight + dZ)*BlowUp;
  stGeometry.pRectangle[stGeometry.nRectangles].rotAngle = 0.;

  stGeometry.nRectangles++;

  return;

}


void CreateVisualisationGeometryStraightChannels(double xStart, double xEnd, double yStart, double yEnd, double entranceHeight, double dZ)
{

  VectorType v1 = {xStart, BlowUp * yStart, -BlowUp * entranceHeight/2.};
  VectorType v2 = {xStart, BlowUp * yStart,  BlowUp * entranceHeight/2.};
  VectorType v3 = {xEnd,   BlowUp * yEnd,    BlowUp *(entranceHeight + dZ)/2.};

  if (!bVisInstr) return;

  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v1, v2, v3);

  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][0] = xStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][1] = yStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][2] = -entranceHeight/2.; */

  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][0] = xStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][1] = yStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][2] = entranceHeight/2.; */

  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][0] = xEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][1] = yEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][2] = (entranceHeight + dZ)/2.; */

  stGeometry.nTriangles++;

  v2[0]  = xEnd;
  v2[1]  = yEnd*BlowUp;   // orig.: xEnd
  v2[2] *= -1.0;

  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v1, v2, v3);
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][0] = xStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][1] = yStart; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[0][2] = -entranceHeight/2.; */

  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][0] = xEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][1] = yEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[1][2] = (entranceHeight + dZ)/2.; */

  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][0] = xEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][1] = yEnd; */
  /* stGeometry.pTriangle[stGeometry.nTriangles].vEdges[2][2] = -(entranceHeight + dZ)/2.; */

  stGeometry.nTriangles++;

  return;

}



void CreateVisualisationGeometryTopBottom(double x1Start, double x1End, double y1Start, double y1End, double dY1circ, double radius1,
                                          double x2Start, double x2End, double y2Start, double y2End, double dY2circ, double radius2,
                                          double entranceHeight, double dZ, char* sColor)

{

  double angle1, angle2;
  double angleElem1, angleElem2;
  double totalLength1, totalLength2;
  int nElements1, nElements2;
  int minElements, maxElements;

  int i, j;
  double x11=0.0, x12=0.0, y11=0.0, y12=0.0, x21=0.0, x22=0.0, y21=0.0, y22=0.0;
  double dY1, dY2;
  double deltaY1, deltaY2;

  VectorType v[4];

  double height11, height12, height21, height22;

  if (!bVisInstr) return;

  // outer left surface
  angle1 = asin((x1End - x1Start)/radius1);
  totalLength1 = radius1*angle1;
  nElements1 = (int) (totalLength1/lengthGeomPiece);
  x11 = x1Start;
  y11 = y1Start;

  deltaY1 = fabs(y1End - y1Start);
  deltaY2 = fabs(1. - cos(angle1))*radius1;

  if (deltaY1 >= deltaY2)
  {
    totalLength1 = sqrt(pow(totalLength1, 2)  + pow(deltaY1 - deltaY2, 2));
  }
  else
  {
    totalLength1 = sqrt(pow(totalLength1, 2)  - pow(deltaY1 - deltaY2, 2));
  }

  // outer right surface
  angle2 = asin((x2End - x2Start)/radius2);
  totalLength2 = radius2*angle2;
  nElements2 = (int) (totalLength2/lengthGeomPiece);
  x21 = x2Start;
  y21 = y2Start;

  deltaY1 = fabs(y2End - y2Start);
  deltaY2 = fabs(1. - cos(angle2))*radius2;

  if (deltaY1 >= deltaY2)
  {
    totalLength2 = sqrt(pow(totalLength2, 2)  + pow(deltaY1 - deltaY2, 2));
  }
  else
  {
    totalLength2 = sqrt(pow(totalLength2, 2)  - pow(deltaY1 - deltaY2, 2));
  }

   // Take into account a possible converging
  dY1 = y1End - (y1Start + dY1circ);
  dY2 = y2End - (y2Start + dY2circ);

  angleElem1 = 0.;
  angleElem2 = 0.;

  if (nElements1 <= nElements2)
  {
    minElements = nElements1;
    maxElements = nElements2;
  } else {
    minElements = nElements2;
    maxElements = nElements1;
  }

  for (i = 0; i < minElements; i++)
  {
    angleElem1 += 2.*asin(lengthGeomPiece/(2.*radius1));
    angleElem2 += 2.*asin(lengthGeomPiece/(2.*radius2));

    //    x12 = x1Start + radius1*sin(angleElem1);
    y12 = y1Start + radius1*(1. - cos(angleElem1)) + ((i + 1.)*lengthGeomPiece)*dY1/totalLength1;
    x12 = sqrt(pow(lengthGeomPiece, 2) - pow(y12 - y11, 2)) + x11;
    //    x22 = x2Start + radius2*sin(angleElem2);
    y22 = y2Start + radius2*(1. - cos(angleElem2)) + ((i + 1.)*lengthGeomPiece)*dY2/totalLength2;
    x22 = sqrt(pow(lengthGeomPiece, 2) - pow(y22 - y21, 2)) + x21;

    height11 = entranceHeight + 1.0*i*lengthGeomPiece*dZ/totalLength1;
    height12 = entranceHeight + 1.0*(i+1)*lengthGeomPiece*dZ/totalLength1;
    height21 = entranceHeight + 1.0*i*lengthGeomPiece*dZ/totalLength2;
    height22 = entranceHeight + 1.0*(i+1)*lengthGeomPiece*dZ/totalLength2;

    //bottom
    v[0][0] = x11; v[0][1] = y11*BlowUp; v[0][2] = -height11/2.0*BlowUp;
    v[1][0] = x12; v[1][1] = y12*BlowUp; v[1][2] = -height12/2.0*BlowUp;
    v[2][0] = x21; v[2][1] = y21*BlowUp; v[2][2] = -height21/2.0*BlowUp;
    v[3][0] = x22; v[3][1] = y22*BlowUp; v[3][2] = -height22/2.0*BlowUp;

    DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
    stGeometry.nTriangles++;

    DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
    stGeometry.nTriangles++;

    //top
    for (j=0; j<4; j++) v[j][2] *= -1.;

    DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
    stGeometry.nTriangles++;

    DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
    stGeometry.nTriangles++;

    x11 = x12;
    y11 = y12;

    x21 = x22;
    y21 = y22;

  }

  if (minElements != maxElements)
  {
    for (i = minElements; i < maxElements; i++)
    {
      if (nElements1 < nElements2)
      {
        angleElem2 += 2.*asin(lengthGeomPiece/(2.*radius2));

        //  x22 = x2Start + radius2*sin(angleElem2);
        y22 = y2Start + radius2*(1. - cos(angleElem2)) + ((i + 1.)*lengthGeomPiece)*dY2/totalLength2;
        x22 = sqrt(pow(lengthGeomPiece, 2) - pow(y22 - y21, 2)) + x21;

        height11 = entranceHeight + 1.0*(nElements1-1)*lengthGeomPiece*dZ/totalLength1;
        height12 = entranceHeight + 1.0*nElements1*lengthGeomPiece*dZ/totalLength1;
        height21 = entranceHeight + 1.0*i*lengthGeomPiece*dZ/totalLength2;
        height22 = entranceHeight + 1.0*(i+1)*lengthGeomPiece*dZ/totalLength2;

      }
      else
      {

        angleElem1 += 2.*asin(lengthGeomPiece/(2.*radius1));

        //  x12 = x1Start + radius1*sin(angleElem1);
        y12 = y1Start + radius1*(1. - cos(angleElem1)) + ((i + 1.)*lengthGeomPiece)*dY1/totalLength1;
        x12 = sqrt(pow(lengthGeomPiece, 2) - pow(y12 - y11, 2)) + x11;

        height11 = entranceHeight + 1.0*i*lengthGeomPiece*dZ/totalLength1;
        height12 = entranceHeight + 1.0*(i+1)*lengthGeomPiece*dZ/totalLength1;
        height21 = entranceHeight + 1.0*(nElements2 - 1)*lengthGeomPiece*dZ/totalLength2;
        height22 = entranceHeight + 1.0*nElements2*lengthGeomPiece*dZ/totalLength2;

      }

      v[0][0] = x11; v[0][1] = y11*BlowUp; v[0][2] = -height11/2.0*BlowUp;
      v[1][0] = x12; v[1][1] = y12*BlowUp; v[1][2] = -height12/2.0*BlowUp;
      v[2][0] = x21; v[2][1] = y21*BlowUp; v[2][2] = -height21/2.0*BlowUp;
      v[3][0] = x22; v[3][1] = y22*BlowUp; v[3][2] = -height22/2.0*BlowUp;

      //bottom
      DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
      stGeometry.nTriangles++;

      DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
      stGeometry.nTriangles++;

      //top
      for (j=0; j<4; j++) v[j][2] *= -1.;

      DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
      stGeometry.nTriangles++;

      DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
      stGeometry.nTriangles++;

      if (nElements1 < nElements2)
      {
        x21 = x22;
        y21 = y22;
      }
      else
      {
        x11 = x12;
        y11 = y12;
      }
    }
  }


  //Last piece (< 50cm)
  x12 = x1End;
  y12 = y1End;

  x22 = x2End;
  y22 = y2End;

  height11 = entranceHeight + nElements1*lengthGeomPiece*dZ/totalLength1;
  height12 = entranceHeight + dZ;
  height21 = entranceHeight + nElements2*lengthGeomPiece*dZ/totalLength2;
  height22 = entranceHeight + dZ;

  v[0][0] = x11; v[0][1] = y11*BlowUp; v[0][2] = -height11/2.0*BlowUp;
  v[1][0] = x12; v[1][1] = y12*BlowUp; v[1][2] = -height12/2.0*BlowUp;
  v[2][0] = x21; v[2][1] = y21*BlowUp; v[2][2] = -height21/2.0*BlowUp;
  v[3][0] = x22; v[3][1] = y22*BlowUp; v[3][2] = -height22/2.0*BlowUp;

  //bottom
  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
  stGeometry.nTriangles++;

  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
  stGeometry.nTriangles++;

  //top
  for (j=0; j<4; j++) v[j][2] *= -1.;

  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[0], v[1], v[2]);
  stGeometry.nTriangles++;

  DefineTriangle(&(stGeometry.pTriangle[stGeometry.nTriangles]), v[1], v[2], v[3]);
  stGeometry.nTriangles++;

  sprintf(sVisDescrpt, "%s:%sy", sModuleName, sColor);
  stGeometry.pDescr  =  sVisDescrpt;
  stGeometry.eModule = _eModule;

  return;

}



void DefineTriangle(VtTriangle* triangle, VectorType v1, VectorType v2, VectorType v3)
{

  CopyVector(v1, triangle->vEdges[0]);
  CopyVector(v2, triangle->vEdges[1]);
  CopyVector(v3, triangle->vEdges[2]);

}
