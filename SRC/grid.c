/**********************************************************************************************/
/*  VITESS module 'grid'                                                                      */
/*                                                                                            */
/* This module simulates a rectangular plate having N x M rectangular of circulalar apertures */
/*   (non-ideal absorption of the plate can be considered)                                    */
/*                                                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to  */
/* the authors.                                                                               */
/*                                                                                            */
/* Written by Manoshin Sergey in Jun 2003 for VSANS simulations, HMI Berlin                    */
/* Born from module spacewindow_multiple                                                      */
/*                                                                                            */
/* 1.00  Jun 2003  S. Manoshin    initial version with possibility of material simulation     */
/* 1.01  Jul 2004  S. Manoshin    Color tracking was added                                    */
/* 1.02  Aug 2004  S. Manoshin    Add deviation of distance and shifts of the grid elements   */
/*                                Auto calculation for gravity monochromator and grid system  */
/*                                Add reducing of grid sizes for convergent gridset system    */
/* 1.1   Oct 2004  S. Manoshin    Add and correct the deviation of grid system                */
/*                                DistanceDev, ShiftHorDev, ShiftVerDev, Pos and Size  hole   */
/* 1.2   Aug 2019  K. Lieutenant  tidy up and visualization                                   */
/* 1.2a  Sep 2022  K. Lieutenant  visualization corrected and minor improvements              */
/* 1.3   Dec 2024  K. Lieutenant  color setting corrected and log file output improved        */
/* 1.4   Mar 2026  K. Lieutenant  use of improved functions in 'bender_inter_data'            */
/**********************************************************************************************/
#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"
#include "convert.h"
#include "message.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit     (int argc, char *argv[]);    // Reads input parameters and sets them as global variables
void  EvalInput   ();                          // Analyses input parameters and prepares attenuation
short ReadGridFile();                          // Reads the file describing the grid geometry
void  SetGeometry (char* sColor, int nHoles);  // Fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
char   *sCollFileName=NULL;     // -I  [-]  file describing the grid geometry
char   *sTransFileName=NULL;    // -C  [-]  file describing the transmission of the grid material
VtWndMat eMaterialO            // -c  [-]  material of the grids: 0 - from file, 1 - gadolinium, 2 - cadmium,  3 - Bor10,
           =VT_WND_IDEAL;      //                                 4 - Eu,        5 - Silicon,   99 - ideal absorber
VtShape eKeyShape=VT_NO_SHAPE;  // -N  [-]  Form of grid elements 0 - square form; 1 - circle form
long    eKeyColorTrack=0;       // -K  [-]  Activate color tracking = cross-talk analysis, default no (0)

double  Distance=0.0,           // -D  [cm] distance from the previous item of the grid
        ShiftHor=0.0,           // -d  [cm]  horizontal and
        ShiftVer=0.0,           // -e  [cm]  vertical shift of the grid element
        Thickness=0.1,          // -t  [cm]  thickness of the plate
        OuterA=0.0,             // -a  [cm]  radius or width of the plate
        OuterB=0.0;             // -b  [cm]  height of the plate

double  DistanceDev =0.0,       // -X  [cm]  distance and deviation
        ShiftHorDev =0.0,       // -y  [cm]  horizontal and
        ShiftVerDev =0.0,       // -q  [cm]  vertical displacement of a grid element
        WinRadiusDev=0.0,       // -h  [cm]  deviation of hole size
         WinCenterDev=0.0;       // -H  [cm]  deviation of hole position

double  DistanceAbs = 0.0;      // -M  [cm]  absolute distance from first grid element, for simulation of system of grid, [cm]
double  TotalLength = 0.0;      // -m  [cm]  length of the grid system = half the lenngth from first element to detector
double  WaveMonoch  = 0.0;      // -n  [Ang] standard wavelength for monochromatisation, if zero disactivated


// Variables determined from input parameters, files or trajectory data
long    NumberOfHoles=0;                  // number of holes in the grid element
double  WAVS[MAX_MU],                     // lambda and µ-values and thickness of the grid material
        MUS [MAX_MU];
long    nValF=0;                          // number of attenuation values in file (describing absorption in grid material)
double  winsize   [1001],                 // arrays of hole positions and sizes
        ywincenter[1001],
        zwincenter[1001];
double  OuterRadius=0.0;                  // outer radius for spherical holes
Plane    Endpoint,                         // Planes through beginning and end of the grid element
        EndpointCol;                      // Endpoint.D: distance from origin to grid along x-axis        [cm]



/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  long    i=0,               // index of trajectories
          j=0,               // index of holes
          k=0;               // counter of cross-talk events
  long    key_abs=1;         // key absorb: 1 - absorption,  0 - transmission
  short    current_color=0,   // color of the current trajectory (= number of the previous hole that the neutron passed)
          current_hole=0;    // number of the hole, where the neutron passed
  double  dist_squared=0.0;  // distance from the point of impact to the center of the grid
  double  TimeOF=0.0;        // TOF of neutron from origin to grid element


  double  NewPositionY=0.0,  // neutron position in co-ordinate system  rotated by 'rotang'
          NewPositionZ=0.0;

  double  VelocityReal=0.0,  // velocity of the current neutron
          N_Wavelength,      // wavelength of the current neutron
          mu=0.0,            // attenuation coefficient of the material, that the neutron traverses
          prob=0.0;          // probability of traversing the material

  Neutron Output;            // trajectory as it is written to the output

  /******************************************/
  /** Initialisation and parameter input   **/
  /******************************************/
  InitNeutron(&Output);

  _eModule=MCN_GRID;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.4");
  OwnInit(argc, argv);
  MsgInit();
  NumberOfHoles=ReadGridFile();
  EvalInput();

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp = TRUE;

  DECLARE_ABORT

  /******************************/
  /** Loop over trajectories   **/
  /******************************/
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        if (InputNeutrons[i].Vector[0] <= 0.0) continue;
        if (InputNeutrons[i].Wavelength == 0.0) continue;
        VelocityReal = (V_FROM_LAMBDA(InputNeutrons[i].Wavelength));
        if (VelocityReal <= 0.0) continue;
        current_color = InputNeutrons[i].Color;

        // Move neutron to beginning of grid element with gravity effect and calculate Time of Flight
        // ------------------------------------------------------------------------------------------
        CopyNeutron(&InputNeutrons[i], &Output);

        if (keygrav == 1)
        {
          TimeOF = NeutronPlaneIntersectionGrav(&Output, Endpoint);
        }
        else
        {
          TimeOF = NeutronPlaneIntersection1(&Output, Endpoint);
        }
        Output.Time += (double)TimeOF;

        /* windows test */
        NewPositionY = Output.Position[1] - ShiftHor;
        NewPositionZ = Output.Position[2] - ShiftVer;

        key_abs = 0;      // not absorbed
        current_hole = 0; // default: outside window

        // shape of grid elements: 0 - square form
        if (eKeyShape==VT_SQUARE)
        {
          if ((-0.5*OuterA < NewPositionY)&&(0.5*OuterA > NewPositionY)&&(-0.5*OuterB < NewPositionZ)&&(0.5*OuterB > NewPositionZ))
          {
            /*  Square form */
            key_abs = 1;            // absorbed
            for(j=1; j<=NumberOfHoles; j++)
            {
              if ((-0.5*winsize[j] < (NewPositionY-ywincenter[j]))&&   // here the size means half of the side length of a square
                  ( 0.5*winsize[j] > (NewPositionY-ywincenter[j]))&&
                  (-0.5*winsize[j] < (NewPositionZ-zwincenter[j]))&&
                  ( 0.5*winsize[j] > (NewPositionZ-zwincenter[j])))
              {
                key_abs = 0 ;       // not absorbed
                current_hole = j ;
              }
            }
          }
        }
        else
        {
          /*  1 - circle form  */
          dist_squared = NewPositionY*NewPositionY + NewPositionZ*NewPositionZ;
          if (dist_squared <= OuterRadius*OuterRadius)
          {
            key_abs = 1;            // absorbed
            for(j=1; j<=NumberOfHoles; j++)
            {
              dist_squared = (NewPositionY - ywincenter[j])*(NewPositionY - ywincenter[j]) +
                             (NewPositionZ - zwincenter[j])*(NewPositionZ - zwincenter[j]);
              if (dist_squared <= winsize[j]*winsize[j])
              {
                key_abs = 0;      // not absorbed
                current_hole = j ;
              }
            }
          }
        }

        if (current_hole==0)
          WriteIAP(&Output, VT_OUTSIDE);
        else if (key_abs==TRUE)
          WriteIAP(&Output, VT_OUT_OF_WND);
        else
          WriteIAP(&Output, VT_PASSED);

        // Move neutron to end of grid element with gravity effect and calculate Time of Flight
        // ------------------------------------------------------------------------------------
        if (keygrav == 1)
        {
          TimeOF = NeutronPlaneIntersectionGrav(&Output, EndpointCol);
        }
        else
        {
          TimeOF = NeutronPlaneIntersection1(&Output, EndpointCol);
        }
        Output.Time += TimeOF;


        // In case of absorption: Attenuation in the grid material
        // -------------------------------------------------------
        if (key_abs == 1)
        {
          if (eMaterialO == VT_WND_IDEAL)
            continue;
          /* Attenuation during pass through grid material */
          N_Wavelength = Output.Wavelength;
          mu = Interpol(N_Wavelength, WAVS, MUS, nValF);
          if (mu == -10000.0)
          { CountMessageID(WNDO_L_RANGE_TOO_SMALL, Output.ID);
            prob = 0.0;
          }
          else if (mu == 10000.0)
          { prob = 0.0;
          }
          else
          { prob = exp(-mu*TimeOF*VelocityReal);
          }
          Output.Probability = Output.Probability*prob;
          if (Output.Probability <= wei_min)
            continue;
        }

        // neutron trajectories outside disk
        if (current_hole == 0)
        {
          CountMessageID(WND_PASSED_OUTSIDE, Output.ID);
        }
        else
        { // crosstalk check
          if (eKeyColorTrack == TRUE)
          {
            if (current_hole != current_color && current_color != 0)
            {
              CountMessageID(WND_CROSS_TALK, Output.ID);
              k++;
              if (k < 6)
                fprintf(LogFilePtr,"Warning: Crosstalk of trajectories found:  grid position: %8.3f  current hole: %2d   previous hole: %2d\n", DistanceAbs, current_hole, current_color);
            }
          }
        }

        Output.Color = current_hole;
        WriteIAP(&Output, VT_PASSED);

        Output.Position[0] = 0.0;
        WriteNeutron(&Output);
      }
    }
  }

/******************************************************************************/
/* Finish: print parameters, write geometry and instrument file, free memory  */
/******************************************************************************/
my_exit:
  PrintMessage(WNDO_L_RANGE_TOO_SMALL, sTransFileName, ON);
  PrintMessage(WND_PASSED_OUTSIDE, "", ON);
  PrintMessage(WND_CROSS_TALK,     "", ON);
  fprintf(LogFilePtr, "\n");

  SetGeometry("grey", NumberOfHoles);
  Cleanup((Thickness+Distance), 0.0, 0.0, 0.0, 0.0);

  return(0);
}


/**************************************************************/
/** Reads input parameters and sets them as global variables **/
/**************************************************************/
void   OwnInit   (int argc, char *argv[])
{
  int i=0,
      eMat=-1;        // key for outer material as saved or read from GUI

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        // Geometry
        case 'I':
          sCollFileName=&argv[i][2];
          break;

        case 'D':
          Distance =  atof(&argv[i][2]);
          break;
        case 't':
          Thickness = atof(&argv[i][2]);
          break;

        case 'a':
          OuterA = atof(&argv[i][2]);
          break;
        case 'b':
          OuterB = atof(&argv[i][2]);
          break;

        case 'e':
          ShiftVer = atof(&argv[i][2]);
          break;
        case 'd':
          ShiftHor = atof(&argv[i][2]);
          break;

        case 'N':
          eKeyShape = (VtShape) atoi(&argv[i][2]); /* Form of grid elements 0 - square form; 1 - circle form */
          break;
        case 'K':
          eKeyColorTrack = atol(&argv[i][2]);
          break;

        // Grid material
        case 'C':
          sTransFileName=&argv[i][2];
          break;
        case 'c':
          eMat = atoi(&argv[i][2]);      // Material of window frame: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber
          if (eMat==6)
            eMaterialO = VT_WND_IDEAL;   // inconsistency: value '6' used for vacuum in 'bender' und 'bender_inter_data' (i.e. for 'Attenuation()')
          else                           // and for ideal absorption here, in 'window' and 'window_mult'
            eMaterialO = (VtWndMat) eMat;
          break;

        // Deviations
        case 'q':
          ShiftVerDev = atof(&argv[i][2]);
          break;
        case 'y':
          ShiftHorDev = atof(&argv[i][2]);
          break;

        case 'h':
          WinRadiusDev = atof(&argv[i][2]);
          break;
        case 'H':
          WinCenterDev = atof(&argv[i][2]);
          break;

        case 'X':
          DistanceDev =  atof(&argv[i][2]);
          break;

        // Gravity monochromator option
        case 'M':
          DistanceAbs =  atof(&argv[i][2]);
          break;
        case 'm':
          TotalLength =  atof(&argv[i][2]);
          break;
        case 'n':
          WaveMonoch =  atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
}


/********************************************************/
/** Analyses input parameters and prepares attenuation **/
/********************************************************/
void  EvalInput()
{
  char    sLine[CHAR_BUF_SMALL]="",
          sShape[14]="";
  FILE   *trans_file=NULL;   // file describing the transmission of the grid material
  long    i=0;               // index of arrays for wavelength and attenuation
  double  TimeOF=0.0;        // TOF of neutron from origin to window
  Plane   Pcalc;             // Pcalc.D: distance to window along x-axis        [cm]
  Neutron Ncalc;             // Neutron trajctory

  // Checks
  // ------
  if ((2.0*TotalLength) <= DistanceAbs)
  {
    fprintf(LogFilePtr,"EXIT: Incorrect absolute distance and total length of the grid system \n");
    exit(-1);
  }

  if (WaveMonoch < 0.0)
  {
    fprintf(LogFilePtr,"Wavelength for monhromatisation must be positive!!! \n");
    exit(-1);
  }

  if (DistanceAbs < 0.0)
  {
    fprintf(LogFilePtr,"Absolute distance must be positive!!! \n");
    exit(-1);
  }

  if (TotalLength <= 0.0)
  {
    fprintf(LogFilePtr,"Total length of gridset must be positive!!! \n");
    exit(-1);
  }

  if (Thickness < 0.0)
    Error("Thickness of the disk < 0.0");
  if (Thickness == 0.0 && eMaterialO!=VT_WND_IDEAL)
    Error("Thickness of the disk can only be zero for ideal absorber");

  if (DistanceDev < 0.0)
  {
    fprintf(LogFilePtr,"Deviation of distance must be positive!!! \n");
    exit(-1);
  }

  if (ShiftHorDev < 0.0)
  {
    fprintf(LogFilePtr,"Deviation of horizontal shift must be positive!!! \n");
    exit(-1);
  }

  if (ShiftVerDev < 0.0)
  {
    fprintf(LogFilePtr,"Deviation of vertical shift must be positive!!! \n");
    exit(-1);
  }

  if (WinRadiusDev < 0.0)
  {
    fprintf(LogFilePtr,"Deviation of size of hole must be positive!!! \n");
    exit(-1);
  }

  if (WinCenterDev < 0.0)
  {
    fprintf(LogFilePtr,"Deviation of position of hole must be positive!!! \n");
    exit(-1);
  }

  // Inits
  // -----
  memset(&Ncalc,  '\0', sizeof(Neutron));

  for (i=0; i < MAX_MU; i++)
  { WAVS[i] = 0.0;
    MUS[i]  = 0.0;
  }

  // Settings and printing of parameters
  // -----------------------------------
  Shape_ID2Txt(sShape, eKeyShape);
  fprintf(LogFilePtr, "Form of the disk and holes      : %s\n", sShape);
  fprintf(LogFilePtr, "Number of holes                 : %4ld \n", NumberOfHoles);

  InitPlane  (&Endpoint);    Endpoint.D    = -1.0*Distance;
  InitPlane  (&EndpointCol); EndpointCol.D = -1.0*(Distance+Thickness);
  fprintf(LogFilePtr, "Distance + thickness of the disk: %8.3f cm  \n", -EndpointCol.D);

  if(keygrav == 1)
    fprintf(LogFilePtr,"Gravity is enabled \n");
  else
    fprintf(LogFilePtr,"Gravity is disabled \n");

  if (eKeyColorTrack == 1)
  {
    fprintf(LogFilePtr, "Tracking of crosstalk is activated => Ideal absorption assumed  \n");
    // Warning("This feature only works correctly, if the neutron trajectories have color 0 at the first grid");
    eMaterialO = VT_WND_IDEAL ;
  }

  if (eKeyShape == VT_CIRCLE)
    OuterRadius = OuterA;

  switch (eMaterialO)
  { case VT_WND_FILE :  fprintf(LogFilePtr, "Transmission characteristics read from file %s\n", sTransFileName); break;
    case VT_WND_GD   :  fprintf(LogFilePtr, "Window frame material: Gadolinium \n");   nValF = Gadolinium (WAVS, MUS, MAX_MU); break;
    case VT_WND_CD   :  fprintf(LogFilePtr, "Window frame material: Cadmium    \n");   nValF = Cadmium    (WAVS, MUS, MAX_MU); break;
    case VT_WND_B10  :  fprintf(LogFilePtr, "Window frame material: Bor10      \n");   nValF = Bor10      (WAVS, MUS, MAX_MU); break;
    case VT_WND_EU   :  fprintf(LogFilePtr, "Window frame material: Eu         \n");   nValF = Eu         (WAVS, MUS, MAX_MU); break;
    case VT_WND_SI   :  fprintf(LogFilePtr, "Window frame material: Silicon    \n");   nValF = Silicon    (WAVS, MUS, MAX_MU); break;
    case VT_WND_IDEAL:  fprintf(LogFilePtr, "Ideal absorption set in window frame\n"); nValF = IdealAbsorp(WAVS, MUS);         break;
    default: fprintf(LogFilePtr, "\n"); Error("No valid material chosen for window frame");
  }


  if (DistanceAbs > 0.0)
  {
    // fprintf(LogFilePtr,"----------FOR GRIDSET simulations ONLY=========\n");
    fprintf(LogFilePtr, "Length from first grid to focal point: %8.2f cm\n", 2.0*TotalLength);
    fprintf(LogFilePtr, "Absolute distance from first grid    : %8.2f cm\n", DistanceAbs);

    if (WaveMonoch > 0.0)
    {
      fprintf(LogFilePtr, "Gravity monochromatisation is activated for %5.2f Ang, the grids are shifted down\n", WaveMonoch);

      /* Calculate vertical shifting */
      Ncalc.Position[0] = 0.0 ;
      Ncalc.Position[1] = 0.0 ;
      Ncalc.Position[2] = 0.0 ;
      Ncalc.Vector[0] = 1.0 ;
      Ncalc.Vector[1] = 0.0 ;
      Ncalc.Vector[2] = 0.0 ;
      Ncalc.Wavelength = WaveMonoch ;

      Pcalc.A = 1.0;
      Pcalc.B = 0.0;
      Pcalc.C = 0.0;
      Pcalc.D = -1.0*DistanceAbs;

      TimeOF = NeutronPlaneIntersectionGrav(&Ncalc, Pcalc);

      ShiftVer = Ncalc.Position[2] ;

      // fprintf(LogFilePtr,"WARNING: Overriding! New value for the down shifting  %f  cm \n", ShiftVer);
    }
    // fprintf(LogFilePtr,"---------------------------------==========================\n");
  }

  if (DistanceDev > 0.0)
  {
    fprintf(LogFilePtr,"Deviation of distance is activated  +-  %f  cm \n", DistanceDev);
    Distance   =   Distance +  (MonteCarlo(-1.0, 1.0)*DistanceDev) ;
  }

  if (ShiftHorDev > 0.0)
  {
    fprintf(LogFilePtr,"Deviation of horizontal shift is activated  +-  %f  cm \n", ShiftHorDev);
    ShiftHor   =   ShiftHor +  (MonteCarlo(-1.0, 1.0)*ShiftHorDev) ;
    fprintf(LogFilePtr,"New value for horizontal shift = %f cm \n", ShiftHor);
  }

  if (ShiftVerDev > 0.0)
  {
    fprintf(LogFilePtr,"Deviation of vertical shift is activated  +-  %f cm \n", ShiftVerDev);
    ShiftVer   =   ShiftVer +  (MonteCarlo(-1.0, 1.0)*ShiftVerDev) ;
    fprintf(LogFilePtr,"New value for vertical shift = %f cm \n", ShiftVer);
  }


  if (eMaterialO == VT_WND_FILE)
  {
    // Read transmission file for grid element
    if (sTransFileName != NULL)
    {
      trans_file = OpenParameterFile2(sTransFileName, "transmission data", "r");
      i = 0;
      while (ReadLine(trans_file, sLine, sizeof(sLine) - 1) > 0) 
      {
        sscanf(sLine, "%lf %lf", &WAVS[i], &MUS[i]);
        i++;
      }
      nValF = i;
      fclose(trans_file);

      /* check the input data */
      for (i = 1; i <= (nValF - 1); i++) {
        if (WAVS[i + 1] <= WAVS[i]) {
          fprintf(LogFilePtr, "MISTAKE: incorrect data in the transmission file of the grid material \n");
          fprintf(LogFilePtr, "The wavelength values (1st column) must be in ascending order!!!\n");
          exit(-1);
        }
      }
    } else {
      Error("No file name given describing the transmission of the window frame\n");
    }
  }
}


/**************************************************************/
/** Reads the file describing the grid geometry              **/
/**************************************************************/
short ReadGridFile()
{
  char   sLine[CHAR_BUF_SMALL]="";
  FILE  *coll_file=NULL;         // file describing the grid geometry
  double rdate[3]={0.0,0.0,0.0}; // values for each grid hole
  int    i=0;                    // counter of lines
  long   nHoles=0;               // number of holes

  /* Input data from the file describing the grid system */
  if (sCollFileName !=NULL)
  {
    coll_file = OpenParameterFile2(sCollFileName, "grid arrangement", "r");
    while (ReadLine(coll_file, sLine, sizeof(sLine) - 1) == TRUE) {
      StrgScanLF(sLine, rdate, 3, 0);
      i++;

      if (DistanceAbs > 0.0) {
        /* reducing the grid sizes according the converging to the detector */
        ywincenter[i] = ((rdate[0]) * (2.0 * TotalLength - DistanceAbs) / (2.0 * TotalLength)) +
                        (MonteCarlo(-1.0, 1.0) * WinCenterDev);
        zwincenter[i] = ((rdate[1]) * (2.0 * TotalLength - DistanceAbs) / (2.0 * TotalLength)) +
                        (MonteCarlo(-1.0, 1.0) * WinCenterDev);
        winsize[i] = ((rdate[2]) * (2.0 * TotalLength - DistanceAbs) / (2.0 * TotalLength)) +
                     (MonteCarlo(0.0, 1.0) * WinRadiusDev);
      } else {
        ywincenter[i] = rdate[0] + (MonteCarlo(-1.0, 1.0) * WinCenterDev);
        zwincenter[i] = rdate[1] + (MonteCarlo(-1.0, 1.0) * WinCenterDev);
        winsize[i] = rdate[2] + (MonteCarlo(0.0, 1.0) * WinRadiusDev);
      }
    }
    nHoles = i;

    fclose(coll_file);
  }
  else
  {
    Error("Name of the file describing the grid arrangement missing.");
  }

  return (nHoles);
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void  SetGeometry(char* sColor, int nHoles)
{
  int j;
  double InnerRadius=0.0,                   // sizes of the plate
         InnerWidth=0.0, InnerHeight=0.0;

  // Geometry data
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    for (j=1; j <= nHoles; j++)
    {
      if (eKeyShape==VT_CIRCLE)
      { InnerRadius = fmax(InnerRadius, sqrt(sq(ywincenter[j]) + sq(zwincenter[j])) + winsize[j]);
      }
      else
      { InnerWidth  = fmax(InnerWidth,  2.0*(ywincenter[j] + 0.5*winsize[j]));
        InnerHeight = fmax(InnerHeight, 2.0*(zwincenter[j] + 0.5*winsize[j]));
      }
    }

    if (eKeyShape==VT_SQUARE)   // square
    {
      stGeometry.nHulls   = nHoles+1;
      stGeometry.pHull    = calloc(nHoles+1,   sizeof(VtHull));

      // whole plate
      stGeometry.pHull[0].WidthIn    = BlowUp * InnerWidth;
      stGeometry.pHull[0].WidthOut   = BlowUp * OuterA;
      stGeometry.pHull[0].HeightIn   = BlowUp * InnerHeight;
      stGeometry.pHull[0].HeightOut  = BlowUp * OuterB;
      stGeometry.pHull[0].Length     = Thickness;
      stGeometry.pHull[0].vCntr[0]   = (Distance + stGeometry.pHull[0].Length/2.);
      stGeometry.pHull[0].vCntr[1]   = 0.0;
      stGeometry.pHull[0].vCntr[2]   = 0.0;
      stGeometry.pHull[0].vNormal[0] = 1.0;
      stGeometry.pHull[0].vNormal[1] = 0.0;
      stGeometry.pHull[0].vNormal[2] = 0.0;

      for (j=1; j<=nHoles; j++)
      {
        stGeometry.pHull[j].WidthIn   = BlowUp * winsize[j];
        stGeometry.pHull[j].WidthOut  = BlowUp * winsize[j] * 1.1;
        stGeometry.pHull[j].HeightIn  = BlowUp * winsize[j];
        stGeometry.pHull[j].HeightOut = BlowUp * winsize[j] * 1.1;
        stGeometry.pHull[j].Length    = Thickness;
        stGeometry.pHull[j].vCntr[0]  = (Distance + stGeometry.pHull[0].Length/2.);
        stGeometry.pHull[j].vCntr[1]  = ywincenter[j];
        stGeometry.pHull[j].vCntr[2]  = zwincenter[j];
        stGeometry.pHull[j].vNormal[0]= 1.0;
        stGeometry.pHull[j].vNormal[1]= 0.0;
        stGeometry.pHull[j].vNormal[2]= 0.0;
      }
    }
    else
    {
      stGeometry.nHolCyls = nHoles+1;
      stGeometry.pHolCyl  = calloc(nHoles+1, sizeof(VtHolCyl));

      // whole plate
      stGeometry.pHolCyl[0].Radius      = BlowUp * OuterRadius;
      stGeometry.pHolCyl[0].InnerRadius = BlowUp * InnerRadius;
      stGeometry.pHolCyl[0].Length      = Thickness;
      stGeometry.pHolCyl[0].vCntr[0]    = (Distance + stGeometry.pHolCyl[0].Length/2.);
      stGeometry.pHolCyl[0].vCntr[1]    = 0.0;
      stGeometry.pHolCyl[0].vCntr[2]    = 0.0;
      stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
      stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
      stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;

      for (j=1; j<=nHoles; j++)
      {
        stGeometry.pHolCyl[j].InnerRadius = BlowUp * winsize[j];
        stGeometry.pHolCyl[j].Radius      = BlowUp * winsize[j] * 1.1;
        stGeometry.pHolCyl[j].Length      = Thickness;
        stGeometry.pHolCyl[j].vCntr[0]    = (Distance + stGeometry.pHolCyl[0].Length/2.);
        stGeometry.pHolCyl[j].vCntr[1]    = BlowUp * ywincenter[j];
        stGeometry.pHolCyl[j].vCntr[2]    = BlowUp * zwincenter[j];
        stGeometry.pHolCyl[j].vSymAxis[0] = 1.0;
        stGeometry.pHolCyl[j].vSymAxis[1] = 0.0;
        stGeometry.pHolCyl[j].vSymAxis[2] = 0.0;
      }
    }
  }
  return;
}
