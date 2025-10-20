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
/* 1.8    Nov 2013  D. Nekrassov   M-values as input                                           */
/* 1.8a   Feb 2018  K. Lieutenant  silicon data for 0.4 Ang added                             */
/* 1.9    Feb 2020  K. Lieutenant  tidy up, new reflectivity calculation, new file handling   */
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
  extern int gselec; /* choose the output 1 - display only, 2 - file only,
          3 - both, defined in cpgplot.c */
#endif


#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "bender_inter_data.h"
#include "bender.h"


/******************************/
/** Inline Functions         **/
/******************************/
FILE * openNFile (char *name) {return OpenInputFile(name, TRUE, "r");}


/*************************************************/
/** Prototypes of 'init' and internal functions **/
/*************************************************/
char* FullOutName(const char* filename);                    // adds output dir to file name

void OwnInit(int argc, char *argv[]);                       // reads input parameters and initializes global variables
int  LoadReflFile(FILE* pReflFile, double* pData, const char* sWall, const char* sSpin);
void FillReflContainer(double array[1000], double m);

void CreateVisualisationGeometryCurvedChannels  (double xStart, double xEnd, double yStart, double yEnd, double dYcirc, double radius, double entranceHeight, double dZ);
void CreateVisualisationGeometryStraightChannels(double xStart, double xEnd, double yStart, double yEnd, double entranceHeight, double dZ);
void CreateVisualisationGeometryTopBottom(double x1Start, double x1End, double y1Start, double y1End, double dY1circ, double radius1,
                                          double x2Start, double x2End, double y2Start, double y2End, double dY2circ, double radius2,
                                          double entranceHeight, double dZ);

void DefineTriangle(VtTriangle* triangle, VectorType v1, VectorType v2, VectorType v3);


/******************************/
/** Global Variables         **/
/******************************/
const double lengthGeomPiece = 50.; //Length of a geometry element in cm a surface consists of for x3d visualisation
int   numberRectangles;
int   numberTriangles;
long   NumberOfSurfaces;

long   ntfr=0, ntfl=0, ntfs=0;
double X2, Y2, COSB, SINB; /* for defining base circle */

long  keypol= 0;
long  qspin = 0;         /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
long  keymaterial0=6;    /* Material of bender channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
long  keymaterial1=1;    /* IN LEFT : Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
long  keymaterial2=1;    /* IN RIGHT: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */

long   bAbsTransCrit=0;  /* behaviour of not reflected neutrons (0 = absorbed) */

double BenderEntranceHeight=0.0, BenderExitHeight=0.0, BenderEntranceWidth=0.0;
double BenderExitWidth=0.0, EntranceHelp=0.0, rightend=0.0, leftend=0.0 ,spacer=0.0, channelwidth=0.0;
double Radius=0.0, length=0.0;
double surfacerough=0.0;  /* The parameter which characterized bender surface rought */
double beta=0.0;
static double rdatalup[1000], rdatarup[1000], rdatatbup[1000];
static double rdataldo[1000], rdatardo[1000], rdatatbdo[1000];
double TimeOF1;
double rdate[N_SURF_M3];
double transm0[1001]; /* file, which describe transmission of material of bender channel */
double transm1[1001], transm2[1001]; /* file, which describe transmission of material between channels */
double WAVL[MAX_MU], WAVR[MAX_MU], WAVS[MAX_MU], MUL[MAX_MU], MUR[MAX_MU], MUS[MAX_MU];
double surradius[N_SURF], entrancediscenter[N_SURF], exitdiscenter[N_SURF];

double XRL[N_SURF], YRL[N_SURF]; /* for calculating converging surface */
double XENL[N_SURF], XEXL[N_SURF], YENL[N_SURF], YEXL[N_SURF], RADL[N_SURF];
double XCENL[N_SURF], YCENL[N_SURF], XTMPL[N_SURF], YTMPL[N_SURF], ALPHAL[N_SURF];

double XRR[N_SURF], YRR[N_SURF]; /* for calculating converging surface */
double XENR[N_SURF], XEXR[N_SURF], YENR[N_SURF], YEXR[N_SURF], RADR[N_SURF];
double XCENR[N_SURF], YCENR[N_SURF], XTMPR[N_SURF], YTMPR[N_SURF], ALPHAR[N_SURF];

double mNumber[2][3];

double TMP1, TMP2, TMP3, TMP4; /* Temporary for surfaces tests variables */

long   keytest = 0; /* Key for test geometry of bender */
long   keyVisAct=0;

double entdismin, entdismax;
double extdismin, extdismax;
double disabut=0.0; /* Abutment distance, cm */


char    *ReflFileNamelup=NULL;
char     *ReflFileNameldo=NULL;

char    *TransFileName0=NULL;
char    *TransFileName1=NULL;
char    *TransFileName2=NULL;

char    *ReflFileNamerup=NULL;
char    *ReflFileNamerdo=NULL;

char    *ReflFileNametbup=NULL;
char    *ReflFileNametbdo=NULL;

char    *SurfacesFileName=NULL;
char     *AsciiFileName=NULL;


BenderChannel  BenderCh;
Bender        BenderMy;

Neutron  Output;

FILE  *refl_filelup=NULL; /* file for describing left surfaces of bender, spin up */
FILE  *refl_fileldo=NULL; /* file for describing left surface of bender, spin down */

FILE  *trans_file0=NULL; /* file for describing of transmission of material of bender channel  */
FILE  *trans_file1=NULL; /* file for describing of transmission of bender left surface (see from entrance) */
FILE  *trans_file2=NULL; /* file for describing of transmission of bender right surface(see from entrance) */

FILE  *refl_filerup=NULL; /* file for describing right surfaces of bender, spin up */
FILE  *refl_filerdo=NULL; /* file for describing right surfaces of bender, spin down */

FILE  *refl_filetbup=NULL; /* file for describing top and bottom planes of bender, spin up */
FILE  *refl_filetbdo=NULL; /* file for describing top and bottom planes of bender, spin down */

FILE  *surfaces_file=NULL; /* file for describing of surfaces of converging bender*/
FILE  *AsciiFile=NULL; /* file for output characteristics of bender surfaces */

  /* MF: visualisation for Windows and generation of file for the picture */
#ifdef DO_WIN32
  const char *GraphDev = "bender.ps";
#else
  const char *GraphDev = "bender.png";
#endif


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
  long  i, j, numberch = 0;

  mNumber[0][0] = -1; mNumber[0][1] = -1; mNumber[0][2] = -1;
  mNumber[1][0] = -1; mNumber[1][1] = -1; mNumber[1][2] = -1;

#ifdef VT_GRAPH
  gselec = 1 ; /* Activate visualisation device -screen */
#endif

  // reading of input data and initialisation
  // ----------------------------------------
  _eModule=MCN_BENDER;

  Init(argc, argv,_eModule);
  PrintModuleName(_eModule, "1.9");
  OwnInit(argc, argv);

  /* test geometry of bender */
  if (keytest == 1)
    GeometryTestBender(BenderMy, XENR, YENR, XENL, YENL, XEXR, YEXR, XEXL, YEXL,
                       BenderEntranceHeight, BenderExitHeight, beta, NumberOfSurfaces);

  /*  set allowed spin direction: qspin must be  0 or 1 or 2 ONLY */
  if (qspin < 0 || qspin > 2) qspin = 0;
  if (qspin == 0) fprintf(LogFilePtr,"Magnetic field direction - AXIS OX \n");
  if (qspin == 1) fprintf(LogFilePtr,"Magnetic field direction - AXIS OY \n");
  if (qspin == 2) fprintf(LogFilePtr,"Magnetic field direction - AXIS OZ \n");

  if(keygrav == 1)
    fprintf(LogFilePtr,"Inside bender gravity is enabled if the planes have no curvature!! \n");
  else
    fprintf(LogFilePtr,"Inside bender gravity is disabled \n");

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

        if (fabs(InputNeutrons[i].Position[2])>BenderEntranceHeight/2.0) continue;

        /****************choose the channel******************/
        /* include thickness  */

        for (j=1;j<=(NumberOfSurfaces-1);j++)
        {
          rightend=YENR[j];
          leftend=YENL[j];
          if((rightend<InputNeutrons[i].Position[1])&&(leftend>InputNeutrons[i].Position[1]))
          {
            numberch = j;
            break;
          }
        }

        //        fprintf(LogFilePtr,"J end =  %d  %d  \n",j, numberch);

        if(j==NumberOfSurfaces)
          continue;  /*neutron blocked by spacer*/


        /* Check the quantization of polarization */

        if (keypol == 1)
          if (fabs(InputNeutrons[i].Spin[qspin]) != 1.0)
          {
            fprintf(LogFilePtr,"ERROR: Illegal Spin Quantisation!!! Check the Spin value \n");
            exit(-1);
          }



        /******************************************************************************************/
        /* Pass a pointer to the neutron and the Bender structure variable to a subroutine to do  */
        /* the donkey work. The return value is the total value of the time of flight through the */
        /* Bender, or -1.0 if it missed all plates and the exit (should be impossible).           */
        /******************************************************************************************/

        /* Choose the behavior of neutrons between channels */

        if (bAbsTransCrit == 0)
        {
          /* Neutrons travel WITHOUT crosstalk between channels */
          TimeOF1 = PathThroughChannelGravOrder2(&InputNeutrons[i], BenderMy, BenderCh, numberch, NumberOfSurfaces, wei_min, disabut,
          rdatalup, rdatarup, rdatatbup, rdataldo, rdatardo, rdatatbdo, surfacerough,
          keygrav, keypol, qspin,
          entrancediscenter, exitdiscenter, spacer);
        }
        else
        {
          /* Neutrons travel WITH crosstalk between channels */
          TimeOF1 = PathThroughBenderGravOrder2(&InputNeutrons[i], BenderMy, BenderCh, numberch, NumberOfSurfaces, wei_min, disabut,
                                                rdatalup, rdatarup, rdatatbup, rdataldo, rdatardo, rdatatbdo, surfacerough,
                                                keygrav, keypol, qspin,
                                                entrancediscenter, exitdiscenter, spacer,
                                                keymaterial0, keymaterial1, keymaterial2,

          WAVS, MUS, ntfs,
          WAVL, MUL, ntfl,
          WAVR, MUR, ntfr);
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


        //      fprintf(LogFilePtr,"Out x = %f  y = %f  z = %f \n",Output.Position[0], Output.Position[1], Output.Position[2]);


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

  if (AsciiFile!=NULL)
    fclose(AsciiFile);

  return(0);
}


void  OwnInit(int argc, char *argv[])
{
  long i, count;
  long  k, counter;
#ifdef VT_GRAPH
  double temp1=0.0, temp2=0.0;
#endif
  double dX, dZ,
         X1, Y1, xr, yr; /* for defining base circle */

  // OwnInit
  for(i=1; i<argc; i++)
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
        refl_filelup = openNFile((ReflFileNamelup = arg));
        LoadReflFile(refl_filelup,  rdatalup, "left",  "up");
        break;

      case 'I':
        refl_fileldo = openNFile((ReflFileNameldo = arg));
        LoadReflFile(refl_fileldo,  rdataldo, "left", "down");
        break;

      case 'm':
        refl_filerup = openNFile((ReflFileNamerup = arg));
        LoadReflFile(refl_filerup,  rdatarup, "right", "up");
        break;

      case 'M':
        refl_filerdo = openNFile((ReflFileNamerdo = arg));
        LoadReflFile(refl_filerdo,  rdatardo, "right", "down");
        break;

      case 'u':
        SurfacesFileName = arg;
        break;

      case 'k':
        refl_filetbup = openNFile((ReflFileNametbup = arg));
        LoadReflFile(refl_filetbup, rdatatbup,"top and bottom", "up");
        break;

      case 'K':
        refl_filetbdo = openNFile((ReflFileNametbdo = arg));
        LoadReflFile(refl_filetbdo, rdatatbdo,"top and bottom", "down");
        break;

      case 'T':
        trans_file1 = openNFile((TransFileName1 = arg));
        break;

      case 'O':
        trans_file2 = openNFile((TransFileName2 = arg));
        break;

      case 'A':
        AsciiFileName = arg;
        break;

      case 'h':
        BenderEntranceHeight =  atof(arg); /* in cm */
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
        keymaterial0 = atol(arg);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;

      case 'C':
        trans_file0 = openNFile((TransFileName0 = arg));
        break;


      case 'z':
        keymaterial1 = atol(arg);  /* IN LEFT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;

      case 'w':
        keymaterial2 = atol(arg);  /* IN RIGHT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
        break;

      case 'p':
        keypol = atol(arg); /* using(1) or no(0) polarization */
        break;

      case 't':
        keytest = atol(arg); /* test activated - 1 or disactivated -0 */
        break;

      case 'o':
  #ifdef VT_GRAPH
        gselec = atol(&argv[i][2]);
  #endif
        break;

      case 'y':
        keyVisAct = atof(arg);   /* for visualiztion */
  #ifdef VT_GRAPH
        do_visualise = keyVisAct != 0;
  #endif
        break;

      case 'r':
        surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
        surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
        surfacerough  =  tan(surfacerough);
        break;

      case 'g':
        bAbsTransCrit =  atol(arg);    /* behaviour of not reflected neutrons  */
        break;

      case 'V':
        qspin  =  atol(arg);  /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
        break;

      default:
        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n", a);
        exit(-1);
        break;
    }
  }


  /* Open surface for reading */

  if (SurfacesFileName)
  {
     surfaces_file = openNFile(SurfacesFileName);
  }
  else
  {
    fprintf(LogFilePtr, "ERROR: no surface file name given\n");
    exit(-1);
  }

  if (bAbsTransCrit != 0)
  {
    if (keymaterial0 == 0)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Material transmission characteristics reading from file\n");
    }

    if (keymaterial0 == 1)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Gadolinium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
    }

    if (keymaterial0 == 2)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Cadmium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial0 == 3)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Bor10 \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial0 == 4)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Eu \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial0 == 5)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Silicon \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.5 .. 20 A, please correct if nesessary  \n");
    }

    if (keymaterial0 == 6)
    {
      fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Vacuum \n");
    }

    if ((keymaterial0 != 0)&&(keymaterial0 != 1)&&(keymaterial0 != 2)&&(keymaterial0 != 3)&&(keymaterial0 != 4)&&(keymaterial0 != 5)&&(keymaterial0 != 6))
    {
      fprintf(LogFilePtr,"ERROR: MATERIAL OF BENDER CHANNES: No material: EXIT! Correct -c option \n");
      exit(-1);
    }


    if (keymaterial1 == 0)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material transmission characteristics reading from file\n");
    }

    if (keymaterial1 == 1)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Gadolinium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
    }

    if (keymaterial1 == 2)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Cadmium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial1 == 3)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Bor10 \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial1 == 4)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Eu \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial1 == 5)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Silicon \n");
      fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
    }

    if (keymaterial1 == 6)
    {
      fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Vacuum \n");
    }

    if ((keymaterial1 != 0)&&(keymaterial1 != 1)&&(keymaterial1 != 2)&&(keymaterial1 != 3)&&(keymaterial1 != 4)&&(keymaterial1 != 5)&&(keymaterial1 != 6))
    {
      fprintf(LogFilePtr,"ERROR: IN LEFT SIDE OF CHANNEL(see from entrance): No material between channels: EXIT! Correct -z option \n");
      exit(-1);
    }

    if (keymaterial2 == 0)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material transmission characteristics read from file \n");
    }

    if (keymaterial2 == 1)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Gadolinium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
    }

    if (keymaterial2 == 2)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Cadmium \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial2 == 3)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Bor10 \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial2 == 4)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Eu \n");
      fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
    }

    if (keymaterial2 == 5)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Silicon \n");
      fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
    }

    if (keymaterial2 == 6)
    {
      fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Vacuum \n");
    }

  if ((keymaterial2 != 0)&&(keymaterial2 != 1)&&(keymaterial2 != 2)&&(keymaterial2 != 3)&&(keymaterial2 != 4)&&(keymaterial2 != 5)&&(keymaterial2 != 6))
    {
      fprintf(LogFilePtr,"ERROR: IN RIGHT SIDE OF CHANNEL(see from entrance): No material between channels: EXIT! Correct -w option \n");
      exit(-1);
    }
  }


  if(disabut > 0.0)
  {
    fprintf(LogFilePtr,"Inside bender abutment loss is enabled \n");
    fprintf(LogFilePtr,"Abutment distance =  %f cm \n",disabut);
  }
  else
  {
    fprintf(LogFilePtr,"Inside bender abutment loss is disabled \n");
  }

  if(keypol == 1)
  {
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is enabled \n");
  }
  else
  {
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is disabled \n");
  }

  if(surfacerough == 0.0)
  {
    fprintf(LogFilePtr,"The reflected surface is an ideal mirror \n");
  }
  else
  {
    fprintf(LogFilePtr,"The reflected surface is rough \n");
  }

  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e \n", wei_min);


  /* Read bender data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, a.s.o., deg,  */
  /* reference wavelength 1 A */

  /* initial initialization */

  /* for(i=0;i<1000; i++) */
  /* { */
  /*   rdatalup[i]=0.0; */
  /*   rdatarup[i]=0.0; */
  /*   rdatatbup[i]=0.0; */
  /*   rdataldo[i]=0.0; */
  /*   rdatardo[i]=0.0; */
  /*   rdatatbdo[i]=0.0; */
  /* } */


  for(i=0;i<=1000; i++)
  {
    transm1[i]=0.0;
    transm2[i]=0.0;
    transm0[i]=0.0;
  }

  for(i=0; i<MAX_MU; i++)
  {
    WAVL[i] = 0.0;
    WAVR[i] = 0.0;
    WAVS[i] = 0.0;
    MUL[i] = 0.0;
    MUR[i] = 0.0;
    MUS[i] = 0.0;
  }

  for(i=0; i<=N_SURF_S; i++)
  {
    entrancediscenter[i] = 0.0;
    exitdiscenter[i] = 0.0;
    surradius[i] = 0.0;
  }

  for(i=0; i<=N_SURF_M3_S; i++)
    rdate[i] = 0.0;

  if(BenderEntranceHeight == 0.0)
  {
    fprintf(LogFilePtr,"ERROR: You must enter the height of the bender\n");
    exit(0);
  }

  if(Radius < 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Value of radius is negative\n");
    exit(0);
  }

  if(length <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Valus length is incorrect, negative or zero\n");
    exit(0);
  }

  if(BenderExitHeight == 0.0)
    BenderExitHeight = BenderEntranceHeight;

  /* Fill reflectivity values for all walls, spin up and down, if files are not given */
  if (!refl_filelup)  FillReflContainer(rdatalup,  mNumber[0][0]);
  if (!refl_filerup)  FillReflContainer(rdatarup,  mNumber[0][1]);
  if (!refl_filetbup) FillReflContainer(rdatatbup, mNumber[0][2]);
  if (!refl_fileldo)  FillReflContainer(rdataldo,  mNumber[1][0]);
  if (!refl_filerdo)  FillReflContainer(rdatardo,  mNumber[1][1]);
  if (!refl_filetbdo) FillReflContainer(rdatatbdo, mNumber[1][2]);


  if (bAbsTransCrit != 0)
  {
    if (keymaterial0 == 0)
    {
      /* Read transmission file for  bender channel */

      if (TransFileName0 !=NULL)
      {
        for(count=1; count<=1000; count++)
        {
          if (fscanf(trans_file0,"%lf",&transm0[count])==EOF)
          break;
        }

        fclose(trans_file0);

        k = 1;
        ntfs = (long)((count-1)/2);
        for(i = 1; i <= ntfs; i++)
        {
          WAVS[i] = transm0[k];
          MUS[i] = transm0[k+1];
          if ((MUS[i] < 0.0)||(WAVS[i] <= 0.0))
          {
            fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of the bender channel, EXIT!\n");
            exit(-1);
          }
          k = k + 2;
        }

        /* check the input data */
        for(i = 1; i <= (ntfs-1); i++)
        {
          if (WAVS[i+1] <= WAVS[i])
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of the bender channel \n");
            fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
            exit(-1);
          }
        }

        //      fprintf(LogFilePtr,"transmfile for surface count = %d  num = %d \n", count, ntfs);
        //      for(i = 1; i <= ntfs; i++)
        //      fprintf(LogFilePtr," %f   %f  \n",WAVS[i], MUS[i]);
        fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAVS[1], WAVS[ntfs]);

      }
      else
      fprintf(LogFilePtr,"No file, which description transmission of bender channel \n");
    }

    if (keymaterial1 == 0)
    {
      /* Read transmission file for left bender surface */

      if (TransFileName1 !=NULL)
      {
        for(count=1; count<=1000; count++)
        {
          if (fscanf(trans_file1,"%lf",&transm1[count])==EOF)
            break;
        }

        fclose(trans_file1);

        k = 1;
        ntfl = (long)((count-1)/2);
        for(i = 1; i <= ntfl; i++)
        {
          WAVL[i] = transm1[k];
          MUL[i] = transm1[k+1];
          if ((MUL[i] < 0.0)||(WAVL[i] <= 0.0))
          {
            fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of left bender surface. EXIT!\n");
            exit(-1);
          }
          k = k + 2;
        }

        /* check the input data */
        for(i = 1; i <= (ntfl-1); i++)
        {
          if (WAVL[i+1] <= WAVL[i])
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of left bender surfaces \n");
            fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
            exit(-1);
          }
        }

        //      fprintf(LogFilePtr,"transmfile for left count = %d  num = %d \n", count, ntfl);
        //      for(i = 1; i <= ntfl; i++)
        //      fprintf(LogFilePtr," %f   %f  \n",WAVL[i], MUL[i]);
        fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAVL[1], WAVL[ntfl]);
      }
      else
      {
        fprintf(LogFilePtr,"No file, which description transmission of LEFT bender surface of channel \n");
      }
    }

    if (keymaterial2 == 0)
    {
      /* Read transmission file for left bender surface */

      if (TransFileName2 !=NULL)
      {
        for(count=1; count<=1000; count++)
        {
          if (fscanf(trans_file2,"%lf",&transm2[count])==EOF)
            break;
        }

        fclose(trans_file2);

        k = 1;
        ntfr = (long)((count-1)/2);

        for(i = 1; i <= ntfr; i++)
        {
          WAVR[i] = transm2[k];
          MUR[i] = transm2[k+1];
          if ((MUR[i] < 0.0)||(WAVR[i] <= 0.0))
          {
            fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of right bender surfaces. EXIT!\n");
            exit(-1);
          }
          k = k + 2;
        }

        /* check the input data */
        for(i = 1; i <= (ntfr-1); i++)
        {
          if (WAVR[i+1] <= WAVR[i])
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of right bender surfaces \n");
            fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
            exit(-1);
          }
        }

        //      fprintf(LogFilePtr,"transmfile for right count = %d  num = %d \n", count, ntfr);
        //      for(i = 1; i <= ntfr; i++)
        //      fprintf(LogFilePtr," %f   %f  \n",WAVR[i], MUR[i]);
        fprintf(LogFilePtr,"Minimum and maximum wavelength must be  %f ... %f\n",WAVR[1], WAVR[ntfr]);
      }
      else
      fprintf(LogFilePtr,"No file, which description transmission of RIGHT bender surface of channel \n");
    }

  }

  if (SurfacesFileName !=NULL)
  {
    for(counter = 1; counter <= N_SURF_M3_S; counter++)
      if (fscanf(surfaces_file,"%lf",&rdate[counter])==EOF)
        break;

    fclose(surfaces_file);
  }
  else
  {
    fprintf(LogFilePtr,"\n ERROR: No Surface data. Check the data in the file. \n");
    exit(-1);
  }

  if (AsciiFileName==NULL)
  {
    int len = strlen(SurfacesFileName);
    AsciiFileName = (char*)malloc(len+1);
    memcpy(AsciiFileName, SurfacesFileName, len-3);
    AsciiFileName[len-3] = '\0';
    strcat(AsciiFileName, "Log");
    fprintf(LogFilePtr,"NOTE: No name of the bender information file was given. Output is written to %s\n", AsciiFileName);
  }
  AsciiFile = OpenOutputFile(AsciiFileName, FALSE, "wt");

  NumberOfSurfaces = (long)((counter-1)/3);

  /*  fprintf(LogFilePtr,"Number of in %d \n", (counter-1)); */
  fprintf(LogFilePtr,"Number of surfaces: %ld\n", NumberOfSurfaces);

  k = 1;
  for(i = 1; i <= NumberOfSurfaces; i++)
  {
    entrancediscenter[i] = rdate[k];
    exitdiscenter[i] = rdate[k+1];
    surradius[i] = rdate[k+2];
    /* Automaticly disable gravity, if plane have a curvature */
    if (surradius[i] != 0.0)
    {
      keygrav = 0;
    }
    k = k + 3;
  }

  if ((bAbsTransCrit != 0)&&(NumberOfSurfaces == 2))
  {
    Warning("Having only one channel and choosing transmission between channels does not make sense. \n");
  }

  /* define bender entrance and exit width and radius of curvature */


  entdismin =  9999999.0;
  entdismax = -9999999.0;

  extdismin =  9999999.0;
  extdismax = -9999999.0;

  for(i = 1; i <= NumberOfSurfaces; i++)
  {
    if (entrancediscenter[i] <= entdismin)
    {
      entdismin = entrancediscenter[i];
    }

    if (entrancediscenter[i] >= entdismax)
    {
      entdismax = entrancediscenter[i];
    }

    if (exitdiscenter[i] <= extdismin)
    {
      extdismin = exitdiscenter[i];
    }

    if (exitdiscenter[i] >= extdismax)
    {
      extdismax = exitdiscenter[i];
    }
  }

  fprintf(LogFilePtr,"entrance: max = %f  min = %f \n", entdismax, entdismin);
  fprintf(LogFilePtr,"exit    : max = %f  min = %f \n", extdismax, extdismin);

  /* KL: correction fabs(x - y) instead of fabs(x) + fabs(y) */

  BenderEntranceWidth = fabs(entdismax - entdismin);
  BenderExitWidth     = fabs(extdismax - extdismin);

  fprintf(LogFilePtr,"BENW= %f  BEXW= %f \n",BenderEntranceWidth,BenderExitWidth);


  /****************************************************************************************/
  /* Set up the parameters of the surfaces from the InputNeutrons data...                 */
  /****************************************************************************************/

  //Number of geometry elements for visualisation
  numberRectangles = ((int) ((length / lengthGeomPiece)+1.))*NumberOfSurfaces*2;
  numberTriangles  = ((int) ((length / lengthGeomPiece)+1 + NumberOfSurfaces)*4);

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
  dZ = (BenderExitHeight - BenderEntranceHeight)/2.0;

  BenderCh.Surf[0].A = 0.0;
  BenderCh.Surf[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].C = 0.0;
  BenderCh.Surf[0].D = 0.0;
  BenderCh.Surf[0].E = 0.0;
  BenderCh.Surf[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].W = -BenderCh.Surf[0].F*(BenderEntranceHeight/2.0);
  BenderCh.Surf[0].P = 0.0;
  BenderCh.Surf[0].Q = 0.0;
  BenderCh.Surf[0].R = 0.0;

  BenderMy.SurfTopBottom[0].A = 0.0;
  BenderMy.SurfTopBottom[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[0].C = 0.0;
  BenderMy.SurfTopBottom[0].D = 0.0;
  BenderMy.SurfTopBottom[0].E = 0.0;
  BenderMy.SurfTopBottom[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[0].W = -BenderMy.SurfTopBottom[0].F*(BenderEntranceHeight/2.0);
  BenderMy.SurfTopBottom[0].P = 0.0;
  BenderMy.SurfTopBottom[0].Q = 0.0;
  BenderMy.SurfTopBottom[0].R = 0.0;


  dZ=-dZ;

  BenderCh.Surf[1].A = 0.0;
  BenderCh.Surf[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].C = 0.0;
  BenderCh.Surf[1].D = 0.0;
  BenderCh.Surf[1].E = 0.0;
  BenderCh.Surf[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].W = BenderCh.Surf[1].F*(BenderEntranceHeight/2.0);
  BenderCh.Surf[1].P = 0.0;
  BenderCh.Surf[1].Q = 0.0;
  BenderCh.Surf[1].R = 0.0;

  BenderMy.SurfTopBottom[1].A = 0.0;
  BenderMy.SurfTopBottom[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[1].C = 0.0;
  BenderMy.SurfTopBottom[1].D = 0.0;
  BenderMy.SurfTopBottom[1].E = 0.0;
  BenderMy.SurfTopBottom[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[1].W = BenderMy.SurfTopBottom[1].F*(BenderEntranceHeight/2.0);
  BenderMy.SurfTopBottom[1].P = 0.0;
  BenderMy.SurfTopBottom[1].Q = 0.0;
  BenderMy.SurfTopBottom[1].R = 0.0;



  /* Begin to build curved or straight surfaces */

  for(i = 1; i <= (NumberOfSurfaces-1); i++)
  {

    /* calculating the center of surface on given entrance, exit points and radius
        of curvature */
    /* calculating poins for circle FOR LEFT SURFACES*/

    /* fprintf(LogFilePtr,"Make channel %d \n",i); */

    XENL[i] = X1;
    YENL[i] = Y1 + (entrancediscenter[i+1]-0.5*spacer);

    XEXL[i] = X2 - (sin(beta))*(exitdiscenter[i+1]-0.5*spacer);
    YEXL[i] = Y2 + (cos(beta))*(exitdiscenter[i+1]-0.5*spacer);
    RADL[i] = surradius[i+1];

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

      BenderMy.SurfLeft[i].A = 1.0;
      BenderMy.SurfLeft[i].B = -2.0*XRL[i];
      BenderMy.SurfLeft[i].C = 1.0;
      BenderMy.SurfLeft[i].D = -2.0*YRL[i];
      BenderMy.SurfLeft[i].E = 0.0;
      BenderMy.SurfLeft[i].F = 0.0;
      BenderMy.SurfLeft[i].W = XRL[i]*XRL[i] + YRL[i]*YRL[i] - (RADL[i])*(RADL[i]);
      BenderMy.SurfLeft[i].P = 0.0;
      BenderMy.SurfLeft[i].Q = 0.0;
      BenderMy.SurfLeft[i].R = 0.0;

      CreateVisualisationGeometryCurvedChannels(XENL[i], XEXL[i], YENL[i], YEXL[i], Y2 + YENL[i]*(cos(beta) - 1.), RADL[i], BenderEntranceHeight, dZ*2.);

    }
    else
    {
      /*  Straight Line  */
      BenderMy.SurfLeft[i].A = 0.0;
      BenderMy.SurfLeft[i].B = YENL[i] - YEXL[i];
      BenderMy.SurfLeft[i].C = 0.0;
      BenderMy.SurfLeft[i].D = XEXL[i] - XENL[i];
      BenderMy.SurfLeft[i].E = 0.0;
      BenderMy.SurfLeft[i].F = 0.0;
      BenderMy.SurfLeft[i].W = YEXL[i]*XENL[i] - YENL[i]*XEXL[i];
      BenderMy.SurfLeft[i].P = 0.0;
      BenderMy.SurfLeft[i].Q = 0.0;
      BenderMy.SurfLeft[i].R = 0.0;

      CreateVisualisationGeometryStraightChannels(XENL[i],  XEXL[i],  YENL[i], YEXL[i], BenderEntranceHeight, dZ*2.);
    }

    /* FOR RIGHT SURFACES */

    XENR[i] = X1;
    YENR[i] = Y1 + (entrancediscenter[i]+0.5*spacer);

    XEXR[i] = X2 - (sin(beta))*(exitdiscenter[i]+0.5*spacer);
    YEXR[i] = Y2 + (cos(beta))*(exitdiscenter[i]+0.5*spacer);
    RADR[i] = surradius[i];

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


      BenderMy.SurfRight[i].A = 1.0;
      BenderMy.SurfRight[i].B = -2.0*XRR[i];
      BenderMy.SurfRight[i].C = 1.0;
      BenderMy.SurfRight[i].D = -2.0*YRR[i];
      BenderMy.SurfRight[i].E = 0.0;
      BenderMy.SurfRight[i].F = 0.0;
      BenderMy.SurfRight[i].W = XRR[i]*XRR[i] + YRR[i]*YRR[i] - (RADR[i])*(RADR[i]);
      BenderMy.SurfRight[i].P = 0.0;
      BenderMy.SurfRight[i].Q = 0.0;
      BenderMy.SurfRight[i].R = 0.0;


      TMP3 = sqrt((XRR[i]-XENR[i])*(XRR[i]-XENR[i]) + (YRR[i]-YENR[i])*(YRR[i]-YENR[i]));
      TMP4 = sqrt((XRR[i]-XEXR[i])*(XRR[i]-XEXR[i]) + (YRR[i]-YEXR[i])*(YRR[i]-YEXR[i]));

      CreateVisualisationGeometryCurvedChannels(XENR[i], XEXR[i], YENR[i], YEXR[i], Y2 + YENR[i]*(cos(beta) - 1.), RADR[i], BenderEntranceHeight, dZ*2.);

    }
    else
    {
      /*  Straight line  */
      BenderMy.SurfRight[i].A = 0.0;
      BenderMy.SurfRight[i].B = YENR[i] - YEXR[i];
      BenderMy.SurfRight[i].C = 0.0;
      BenderMy.SurfRight[i].D = XEXR[i] - XENR[i];
      BenderMy.SurfRight[i].E = 0.0;
      BenderMy.SurfRight[i].F = 0.0;
      BenderMy.SurfRight[i].W = YEXR[i]*XENR[i] - YENR[i]*XEXR[i];
      BenderMy.SurfRight[i].P = 0.0;
      BenderMy.SurfRight[i].Q = 0.0;
      BenderMy.SurfRight[i].R = 0.0;

      CreateVisualisationGeometryStraightChannels(XENR[i], XEXR[i], YENR[i],  YEXR[i], BenderEntranceHeight, dZ*2.);
    }

    /*fprintf(LogFilePtr,"\n LEFT data: XEN = %e  YEN = %e  %e", XENL[i], YENL[i], RADL[i]);
      fprintf(LogFilePtr,"\n LEFT data: XEX = %e  YEX = %e  %e", XEXL[i], YEXL[i], RADL[i]);
      if (RADL[i] != 0.0)
        fprintf(LogFilePtr,"\nL DIST CI %e %e %e al = %e",TMP1, TMP2, RADL[i], ALPHAL[i]);

      fprintf(LogFilePtr,"\n RIGT data: XEN = %e  YEN = %e  %e", XENR[i], YENR[i], RADR[i]);
      fprintf(LogFilePtr,"\n RIGT data: XEX = %e  YEX = %e  %e", XEXR[i], YEXR[i], RADR[i]);
      if (RADR[i] != 0.0)
        fprintf(LogFilePtr,"\nR DIST CI %e %e %e al = %e",TMP1, TMP2, RADR[i], ALPHAR[i]); */

  /*  Exit surface  */

    BenderMy.SurfExit[i].A = 0.0;
    BenderMy.SurfExit[i].B = cos(beta);
    BenderMy.SurfExit[i].C = 0.0;
    BenderMy.SurfExit[i].D = sin(beta);
    BenderMy.SurfExit[i].E = 0.0;
    BenderMy.SurfExit[i].F = 0.0;
    BenderMy.SurfExit[i].W = -1.0*(X2*cos(beta) + Y2*sin(beta));
    BenderMy.SurfExit[i].P = 0.0;
    BenderMy.SurfExit[i].Q = 0.0;
    BenderMy.SurfExit[i].R = 0.0;

  }

  i = NumberOfSurfaces - 1;
  CreateVisualisationGeometryTopBottom(XENL[i], XEXL[i], YENL[i], YEXL[i], Y2 + YENL[i]*(cos(beta) - 1.), RADL[i],
                                       XENR[1], XEXR[1], YENR[1], YEXR[1], Y2 + YENR[1]*(cos(beta) - 1.), RADR[1],
                                       BenderEntranceHeight, dZ*2.);

  if (AsciiFile!=NULL)
  {
    /* Output in file some of parameters of surfaces */
    fprintf(AsciiFile,"******************* UNIVERSAL BENDER module ************************ \n");
    fprintf(AsciiFile,"********** INFORMATION FOR FABRICATE OF CONVERGING BENDER ********** \n");
    fprintf(AsciiFile,"******************************************************************** \n");
    fprintf(AsciiFile,"***USER*INPUT*DATA***\n");
    for(i = 1; i <= NumberOfSurfaces; i++)
    {
      fprintf(AsciiFile,"Displace data: N = %ld  EN = %e cm  EX = %e cm  RAD = %e cm \n", i,
      entrancediscenter[i], exitdiscenter[i], surradius[i]);
    }
    fprintf(AsciiFile,"Bender Entrance Width = %f cm ;Bender Exit Width = %f cm \n",BenderEntranceWidth,BenderExitWidth);
    fprintf(AsciiFile,"Bender Entrance Height = %f cm ;Bender Exit Height = %f cm \n",BenderEntranceHeight,BenderExitHeight);
    fprintf(AsciiFile,"Bender Length = %f cm; Bender surfaces thickness = %f cm \n",length,spacer);

    if (Radius != 0.0)
    {
      fprintf(AsciiFile,"Base bender axis - circle\n");
      fprintf(AsciiFile,"Radius of Curvature for base bender axis = %f cm \n",Radius);
      fprintf(AsciiFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",X1, Y1);
      fprintf(AsciiFile,"EXIT: Base points x = %f cm  y = %f cm \n",X2, Y2);
      fprintf(AsciiFile,"The coordinates of center of base circle x = %f cm  y = %f cm \n",xr,yr);
    }
    else
    {
      fprintf(AsciiFile,"Base bender axis - line\n");
      fprintf(AsciiFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",X1, Y1);
      fprintf(AsciiFile,"EXIT: Base points x = %f cm  y = %f cm \n",X2, Y2);
    }

    fprintf(AsciiFile,"****************************SURFACES******************************* \n");
    fprintf(AsciiFile,"******************************************************************* \n");
    fprintf(AsciiFile,"*****************THICKNESS*OF*SURFACES*IS*INCLUDED***************** \n");


    for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {
      fprintf(AsciiFile,"Begin Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",    XENR[i], YENR[i]);

      fprintf(AsciiFile,"Begin Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",    XENL[i], YENL[i]);
      fprintf(AsciiFile,"------------------------------------------------------------------------------\n");

      fprintf(AsciiFile,"Exit Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",    XEXR[i], YEXR[i]);

      fprintf(AsciiFile,"Exit Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",    XEXL[i], YEXL[i]);

      fprintf(AsciiFile,"------------------------------------------------------------------------------\n");
      fprintf(AsciiFile,"==============================================================================\n");
    }

    fprintf(AsciiFile,"==============================================================================\n");


    for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {
      fprintf(AsciiFile,"Center of RIGHT surface %ld X, Y; Radius\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",      XRR[i], YRR[i], RADR[i]);

      fprintf(AsciiFile,"Center of LEFT surface %ld X, Y; Radius\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",      XRL[i], YRL[i], RADL[i]);

      fprintf(AsciiFile,"------------------------------------------------------------------------------\n");

    }
  }

#ifdef VT_GRAPH
  if (do_visualise)
  {
    /* visualise bender surfaces */

    /* choose device for output visualisation */

    if (!((gselec == 1)||(gselec == 2)||(gselec == 3)))
    {
      fprintf(LogFilePtr,"ERROR: Incorrect output device, correct option -o, value 1,2 or 3 \n");
      exit(-1);
    }


    if (gselec == 1)
    {
      fprintf(LogFilePtr,"Open visual output device - display\n");
    }

    if (gselec == 2)
    {
      fprintf(LogFilePtr,"Open visual output device - file \n");
    }

    if (gselec == 3)
    {
      fprintf(LogFilePtr,"Open visual output device - display+file \n");
    }


    /* open graphical device */
    if (cpgopen(FullOutName(GraphDev)) < 1)
    {
      fprintf(LogFilePtr,"ERROR: I cannot open plot device \n");
      exit(-1);
    }
    fprintf(LogFilePtr,"Number of visualised neutrons = %ld \n", BufferSize);

    /* KL: improvement: using a scale that brings the total bender on the screen */
    cpgenv(0.0,1.2*(length),
    1.2*Min(entdismin,extdismin+Y2),1.2*Max(entdismax,extdismax+Y2),0,0);

    cpgsfs(2);

    cpgsch(1.2);


    if (bAbsTransCrit == 0)
    {
      /* Neutrons are travels WITHOUT crosstalk between channels */
      cpglab("X, cm ; Red Line - Device Axis","Y, cm", NumberOfSurfaces == 2 ?
      "Guide Surface Visualisation" :
      "Bender Surfaces Visualisation WITHOUT crosstalk between channels");
    }
    else
    {
      /* Neutrons are travels WITH crosstalk between channels */

      cpglab("X, cm ; Red Line - Device Axis","Y, cm", NumberOfSurfaces == 2 ?
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

    for(i = 1; i <= (NumberOfSurfaces-1); i++)
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

        if (i == (NumberOfSurfaces-1))
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

        if (i == (NumberOfSurfaces-1))
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
  {
    fprintf(LogFilePtr,"m-Value below 0 is given! Module stops!\n");
    exit(-1);
  }

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
            double entranceHeight, double dZ)

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

  sprintf(sVisDescrpt, "%s:yellow", sModuleName);
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
