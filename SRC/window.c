/*********************************************************************************************/
/*  VITESS module 'window'                                                               */
/*                                                                                           */
/* This module simulates an aperture or window of circular or rectangular shape              */
/*   (attenuation inside and non-ideal absorption outside the window can be considered)      */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*       June 1999  D. Wechsler                                                              */
/* 1.00  Feb  2001  S. Manoshin     include of gravity effect                                */
/* 1.01  June 2001  K. Lieutenant   parameter S to simulate a beamstop + SOFTABORT           */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation                                           */
/* 2.00  Jun  2003  S. Manoshin     Add material for window frame                            */
/* 2.10  Mar  2004  S. Manoshin     Add material for inner part of window                    */
/* 2.21  Jul  2004  S. Manoshin     Corrected some bugs for thick window                     */
/* 2.22  Jan  2002  K. Lieutenant   correction:  position after beamstop                     */
/* 2.23  May  2010  A. Houben       "Rotation" of square window by counter rot of neutron pos*/
/* 2.24  Apr  2012  A. Houben       Treat only neutrons with a given color and phi angle     */
/* 2.25  Aug  2019  K. Lieutenant   tidy up and compression option for visualization         */
/* 3.0   Apr  2023  K. Lieutenant   re-written                                               */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"
#include "matrix.h"
#include "message.h"
#include "convert.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);        // reads input parameters and initializes global variables
void  EvalInput();                            // Analyses input parameters and prepares attenuation
void  SetGeometry(char* sColor);              // fills the structure stGeometry for visualization
short IsOutOfWindow(double y, double z);      // checks if the neutron is outside the window
VtDir GetDir(VectorType Pos, VectorType Dir); // checks if the neutron is moving inward or outward


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
VtShape  eWndShape=VT_NO_SHAPE; // -R  [-]   Window shape: VT_CIRCLE circular, VT_SQUARE rectangular
short    bBeamStop=FALSE,       // -S  [-]   Flag: beamstop        TRUE: beamstop, FALSE normal window
                                // -F  [-]   Flag: keep frame of previous module  (par. 'bOldFrame' from init.c, default for beam stop)
         bRemoveOtherCol=TRUE,  // -d  [-]   Flag: Neutrons that are not treated are removed
         TreatColor = -1;       // -f  [-]   Treat only neutrons with this color
double   DistMove =0.0;         // -l  [cm]  Distance from origin to the window (along the x-axis)
double   heightmin=0.0,         // -h  [cm]  z-coordinate: bottom of rectangular window
         heightmax=0.0,         // -H  [cm]  z-coordinate: top of rectangular window
         widthmin =0.0,         // -w  [cm]  y-coordinate: lower frame value of rectangular window
         widthmax =0.0,         // -W  [cm]  y-coordinate: higher frame value of rectangular window
         winradius=0.0,         // -r  [cm]  radius of circular window
         ywincenter=0.0,        // -y  [cm]  y coordinate: center of window
         zwincenter=0.0,        // -z  [cm]  z coordinate: center of window
         rotang = 0.0;          // -A  [rad] Rotation angle (input parameter in [deg])
double   minPhi=-1.0,           // -p  [deg] min. and
         maxPhi=370.0;          // -P  [deg] max. angle in y-z-plane
double   ThicknessO=0.0,        // -t  [cm]  thickness of the frame material
         ThicknessI=0.0;        // -T  [cm]  thickness of the pane material
char    *sTransFileNameO=NULL;  // -C   [-]  file describing the transmission of the window frame material
char    *sTransFileNameI=NULL;  // -m   [-]  file describing the transmission of the material in the open part of window
VtWndAbs eMaterialO             // -c   [-]  Window frame material: 0 - from file, 1 - gadolinium, 2 - cadmium,  3 - Bor10,
          =VT_NO_WABS;          //                                  4 - europium,  5 - silicon,   99 - ideal absorber


// Variables determined from input parameters or trajectory data
short  bPane=FALSE;             //           flag: window pane material exists
FILE  *pTransFileO=NULL;        //           pointer to window frame transmission file
FILE  *pTransFileI=NULL;        //           pointer to window pane transmission file
double WavO[MAX_MU],            //           wavelength and attenuation values of the frame material
       MuO [MAX_MU],
       WavI[MAX_MU],            //           wavelength and attenuation values of the pane material
       MuI [MAX_MU],
       Thickness=0.0;           //           maximum of inner and outer thickness
long   nValFO=0;                //           number of attenuation values in file (for window frame material)
long   nValFI=0;                //           number of attenuation values in file (for window pane material)
Plane  BegPoint,                //           Plane through beginning of the window in the frame of the origin
       EndPoint;                //           Plane through end of the window in the frame of the beginning of the window
VectorType SizeRI={0.0,0.0,0.0},//           dimension of inner material of a rectangular window with certain thickness
           SizeRC={0.0,0.0,0.0},//           dimension of the rectangular channel through the outer material
           SizeCI={0.0,0.0,0.0},//           dimension of inner material of a circular window with certain thickness
           SizeCC={0.0,0.0,0.0},//           dimension of the circular channel through the outer material
           Center={0.0,0.0,0.0};//           center of the window in the frame of the beginning of the window


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short   rcI=FALSE,          // flag: neutron passes through inner material
          rcC=FALSE,          // flag: neutron intersects channel through outer material
          bOutOfWndBeg=FALSE; // flag: neutron out of entrance area
  long    i=0;                // index of trajectories
  double  lenI=0.0,           // path lengths inside inner material
          lenC=0.0,           //    channel,
          lenO=0.0,           //    outer material,
          lenT=0.0;           //    and total length inside window
  double  TofBeg=0.0,         // TOF of neutron from origin to window entry
          TofEnd=0.0;         // TOF of neutron to pass through the window material
  double  Phi=0.0;            // phi angle of the current trajectory
  double  lambda=0.0,         // wavelength of the current neutron
          muI=0.0,  muO=0.0,  // attenuation coefficient of the inner and outer material, which the neutron traverses
          probI=1.0,probO=1.0;// probabilities of traversing the materials

  VectorType PosBeg={0.0,0.0,0.0},  // window entry
             Pos1  ={0.0,0.0,0.0},  // channel entry
             Pos2  ={0.0,0.0,0.0},  // pane entry
             Pos3  ={0.0,0.0,0.0},  // pane exit
             Pos4  ={0.0,0.0,0.0},  // channel exit
             DirBeg={1.0,0.0,0.0};  // direction at window entry
  Neutron    OutNeutron;            // trajectory as it is written to the output

  // Initialisation
  // --------------
  InitNeutron(&OutNeutron);

  _eModule=MCN_WINDOW;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "3.0");
  OwnInit(argc, argv);
  MsgInit();
  EvalInput();

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp = TRUE;

  DECLARE_ABORT

  // Loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      // Only write out event if EOB line is found or if , otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        lambda = InputNeutrons[i].Wavelength;

        // Remove neutrons with wrong color, direction or wavelength
        // ----------------------------------------------------------
        if (TreatColor >= 0 && InputNeutrons[i].Color != TreatColor && bRemoveOtherCol==TRUE)
        {
          WriteIAP(&InputNeutrons[i], VT_FILTERED);
          continue;
        }

        if (bOldFrame==FALSE)
        {
          if (InputNeutrons[i].Vector[0] <= 0.0 || lambda <= 0.0)
          {
            WriteIAP(&InputNeutrons[i], VT_FILTERED);
            continue;
          }
        }
        /* else
        { OutNeutron = InputNeutrons[i];
        }*/

        //   Move neutron to window entrance with or without gravity effect and calculate Time of Flight (ms)
        // ----------------------------------------------------------------------------------------
        if (keygrav == 1)
          TofBeg = NeutronPlaneIntersectionGrav(&InputNeutrons[i], BegPoint);
        else
          TofBeg = NeutronPlaneIntersection1(&InputNeutrons[i], BegPoint);

        InputNeutrons[i].Time += TofBeg;

        // Write intersection point
        WriteIAP(&InputNeutrons[i], VT_ENTERED);

        // Propagate trajectories withh wrong color to exit, if wished
        if (TreatColor >= 0 && InputNeutrons[i].Color != TreatColor && bRemoveOtherCol==FALSE)
        {
          CopyNeutron(&InputNeutrons[i], &OutNeutron);

          if (keygrav == 1)
            TofEnd = NeutronPlaneIntersectionGrav(&OutNeutron, EndPoint);
          else
            TofEnd = NeutronPlaneIntersection1(&OutNeutron, EndPoint);
          OutNeutron.Time += TofEnd;
          goto write_traj;
        }

        // restrict divergence range
        if (minPhi >= 0.0 && maxPhi <= 360.0)
        {
          Phi  = (double)atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[2])*180.0/M_PI+180.;
          if (Phi < minPhi || Phi > maxPhi)
          {
            WriteIAP(&InputNeutrons[i], VT_ABSORBED);
            continue;
          }
        }

        // Test whether window is hit
        bOutOfWndBeg = IsOutOfWindow(InputNeutrons[i].Position[1], InputNeutrons[i].Position[2]);

        // Case 1: zero thickness of window or beamstop
        if (Thickness == 0.0)
        {
          // ok, if out of beamstop or in window
          if ((bBeamStop==TRUE  && bOutOfWndBeg==TRUE) ||   // outside beamstop
              (bBeamStop==FALSE && bOutOfWndBeg==FALSE))    // inside window
          {
            CopyNeutron(&InputNeutrons[i], &OutNeutron);
          }
          else
          {
            WriteIAP(&InputNeutrons[i], VT_OUT_OF_WND);
            continue;
          }
        }

        // case 2: thick beamstop
        else if (bBeamStop==TRUE)
        {
          // calculate the path length inside the inner material and the attenuation
          CopyVector(InputNeutrons[i].Position, PosBeg);
          CopyVector(InputNeutrons[i].Vector,   DirBeg);
          SubVector (PosBeg, Center);

          if (eWndShape==VT_CIRCLE)
            rcI=IntersectionWithHorCyl(ThicknessI, 2.0*winradius, PosBeg, DirBeg, Pos2, Pos3);
          else
            rcI=IntersectionWithRectangular(SizeRI, PosBeg, DirBeg, Pos2, Pos3);

          if (rcI==TRUE)
          {
            lenI = DistVector(Pos2, Pos3);
            muI = Interpolation(lambda, VT_WABS_FILE, WavI, MuI, nValFI);

            if (muI == -10000.0)
            { CountMessageID(WNDI_L_RANGE_TOO_SMALL, InputNeutrons[i].ID);
              probI = 0.0;
            }
            else
            { probI = exp(-muI*lenI);
            }
          }
          else          // beamstop is not hit
          { probI = 1.0;
          }

          // Propagate the neutron to end of the window
          CopyNeutron(&InputNeutrons[i], &OutNeutron);

          if (keygrav == 1)
            TofEnd = NeutronPlaneIntersectionGrav(&OutNeutron, EndPoint);
          else
            TofEnd = NeutronPlaneIntersection1(&OutNeutron, EndPoint);

          OutNeutron.Time        += TofEnd;
          OutNeutron.Probability *= probI;
        }

        // case 3: thick window
        else
        {
          // calculate the path length within the inner material and the path length through the channel
          CopyVector(InputNeutrons[i].Position, PosBeg);
          CopyVector(InputNeutrons[i].Vector,   DirBeg);
          SubVector (PosBeg, Center);

          if (eWndShape==VT_CIRCLE)
          { rcC=IntersectionWithHorCyl(Thickness,  2.0*winradius, PosBeg, DirBeg, Pos1, Pos4);      // channel
            if (bPane)
              rcI=IntersectionWithHorCyl(ThicknessI, 2.0*winradius, PosBeg, DirBeg, Pos2, Pos3);      // inner material
          }
          else
          { rcC=IntersectionWithRectangular(SizeRC, PosBeg, DirBeg, Pos1, Pos4);
            if (bPane)
              rcI=IntersectionWithRectangular(SizeRI, PosBeg, DirBeg, Pos2, Pos3);
          }

          if (rcI==TRUE)
            lenI = DistVector(Pos2, Pos3);
          else
            lenI = 0.0;

          if (rcC==TRUE)
            lenC = DistVector(Pos1, Pos4);
          else
            lenC = 0.0;

          // Propagate the neutron to end of the window
          CopyNeutron(&InputNeutrons[i], &OutNeutron);

          if (keygrav == 1)
            TofEnd = NeutronPlaneIntersectionGrav(&OutNeutron, EndPoint);
          else
            TofEnd = NeutronPlaneIntersection1(&OutNeutron, EndPoint);
          OutNeutron.Time += TofEnd;

          // calculate the length through the outer material
          lenT = TofEnd * V_FROM_LAMBDA(lambda);
          lenO = lenT - lenC;

          // complete absorption if length through ideal absorber is greater zero
          if (eMaterialO == VT_WABS_IDEAL && lenO > 1.0e-06)     // allowing for rounding errors
          {
            WriteIAP(&OutNeutron, VT_ABSORBED);
            OutNeutron.Probability = 0.0;
            continue;
          }
          // calculation of attenuation
          else
          {
            if (bPane && lenI > 0.0)
            {
              muI = Interpolation(lambda, VT_WABS_FILE, WavI, MuI, nValFI);
              if (muI == -10000.0)
              { CountMessageID(WNDI_L_RANGE_TOO_SMALL, InputNeutrons[i].ID);
                WriteIAP(&OutNeutron, VT_NO_DATA);
                OutNeutron.Probability = 0.0;
                continue;
              }
              else
              { probI = exp(-muI*lenI);
              }
            }
            else
            { probI = 1.0;
            }

            if (lenO > 1.0e-06)
            {
              muO = Interpolation(lambda, eMaterialO, WavO, MuO, nValFO);
              if (muO == -10000.0)
              { CountMessageID(WNDO_L_RANGE_TOO_SMALL, OutNeutron.ID);
                WriteIAP(&OutNeutron, VT_NO_DATA);
                OutNeutron.Probability = 0.0;
                continue;
              }
              else
              { probO = exp(-muO*lenO);
              }
            }
            else
            { probO = 1.0;
            }
            OutNeutron.Probability *= (probI * probO);
          }
        }

      write_traj:
        WriteIAP(&OutNeutron, VT_EXITED);

        if (bOldFrame==FALSE)
          OutNeutron.Position[0]=0.0;

        WriteNeutron(&OutNeutron);
      }
    }
  }

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
my_exit:
  if (TreatColor >= 0)
  { fprintf(LogFilePtr,"Only neutrons with color %hd are treated, ", TreatColor);
    if (bRemoveOtherCol) fprintf(LogFilePtr,"others are removed\n");
    else                 fprintf(LogFilePtr,"others are propagated to the end of the window\n");
  }

  PrintMessage(WNDI_L_RANGE_TOO_SMALL, sTransFileNameI, ON);
  PrintMessage(WNDO_L_RANGE_TOO_SMALL, sTransFileNameO, ON);

  // fills the structure stGeometry for visualization
  SetGeometry("blue");

  Cleanup(DistMove+Thickness/2.0, 0.0, 0.0, 0.0, 0.0);

  return(0);
}


/**************************************************************/
/** Reads input parameters and sets them as global variables **/
/**************************************************************/
void  OwnInit(int argc, char *argv[])
{
  int   i=0, j=0,
        eMat=-1;        // key for outer material as saved or read from GUI
  short bOFrame=FALSE;  // default for window 'new frame'

  // initialize
  bOldFrame = -1;      /* no default for frame in general */

  for(j=0; j<MAX_MU; j++)
  {
    WavO[j] = 0.0;
    MuO [j] = 0.0;
    WavI[j] = 0.0;
    MuI [j] = 0.0;
  }

  // read parameters
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
      case 'l':
        DistMove = atof(&argv[i][2]);
        break;

      case 'S':
        bBeamStop = (short) atol(&argv[i][2]);
        bOFrame   = TRUE;   /* default for beamstop 'prev. frame' */
        break;

      case 'F':
        bOldFrame = (short) atol(&argv[i][2]);
        break;

      case 'R':
        if (strlen(&argv[i][2]) > 1)
          eWndShape = Shape_Txt2ID(&argv[i][2]);    // text given   VITESS 4
        else
          eWndShape = (VtShape) atoi(&argv[i][2]);  // ID given     VITESS 3
        break;

      case 'h':
        heightmin =  atof(&argv[i][2]);
        break;
      case 'w':
        widthmin = atof(&argv[i][2]);
        break;

      case 'H':
        heightmax =  atof(&argv[i][2]);
        break;
      case 'W':
        widthmax = atof(&argv[i][2]);
        break;
      case 'A':
        rotang = atof(&argv[i][2])*M_PI/180.;
        break;

      case 'r':
        winradius = atof(&argv[i][2]);
        break;
      case 'y':
        ywincenter = atof(&argv[i][2]);
        break;
      case 'z':
        zwincenter = atof(&argv[i][2]);
        break;

      case 'c':
        eMat = atoi(&argv[i][2]);      // Material of window frame: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber
        if (eMat==6)
          eMaterialO = VT_WABS_IDEAL;  // inconsistency: value '6' used for vacuum in 'bender' und 'bender_inter_data' (i.e. for 'Interpolation()')
        else                           // and for ideal absorption here, in 'grid' and 'window_mult'
          eMaterialO = (VtWndAbs) eMat;
        break;

      case 'C':
        sTransFileNameO=&argv[i][2];
        break;
      case 'm':
        sTransFileNameI=&argv[i][2];
        break;

      case 't':
        ThicknessO = atof(&argv[i][2]);
        /* in cm, outer material */
        break;
      case 'T':
        ThicknessI = atof(&argv[i][2]);
        /* in cm, inner material */
        break;
      case 'f':
        sscanf(&(argv[i][2]),"%hd", &TreatColor);
        break;
      case 'd':
        sscanf(&(argv[i][2]),"%hd", &bRemoveOtherCol);
        break;
      case 'p':
        minPhi = atof(&argv[i][2]);
        /* in deg, min angle in yz plane */
        break;
      case 'P':
        maxPhi = atof(&argv[i][2]);
        if (maxPhi < 0.0)
          maxPhi = 370.0;
        /* in deg, max angle in yz plane */
        break;

      default:
        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
        exit(-1);
        break;
      }
    }
  }

  // take default value for frame, if not explicitely set
  if (bOldFrame==-1)
    bOldFrame=bOFrame;

  if (maxPhi < minPhi)
    Error("Maximal phi angle must not be smaller than minimal phi angle");

  // Checks
  // ------
  if (DistMove < 0.0 && bOldFrame == FALSE)
    Error("Length of space must be >= 0.0");

  if (ThicknessO < 0.0 || ThicknessI < 0.0)
    Error("Value for thickness < 0.0");

  // Set thicknesses and material
  if (bBeamStop==TRUE)
  {
    eMaterialO = VT_WABS_VAC;
    if (ThicknessO > 0.0 && ThicknessO != ThicknessI)
      Note("Outer thickness not relevant, is ignored");
    ThicknessO = ThicknessI;
    Thickness  = ThicknessI;
  }
  else
  {
    if (ThicknessO < ThicknessI)
      Error("The inner material cannot be thicker than the outer material");

    Thickness = Max(ThicknessO, ThicknessI);
    if (ThicknessO == 0.0)
      eMaterialO = VT_WABS_IDEAL;
  }

  if (DistMove < 0.5 * Thickness)
    Error("The position of the window must be at least half the thickness material");

  return;
}


/********************************************************/
/** Analyses input parameters and prepares attenuation **/
/********************************************************/
void EvalInput()
{
  char sLine[CHAR_BUF_SMALL]="";
  long i=0,                        // index of arrays for wavelength and attenuation
       nVal=0;                   // number of wavelength and attenuation values in the array

  // Fill structures defining geometry and planes
  InitPlane(&BegPoint);
  InitPlane(&EndPoint);
  BegPoint.D = -1.0 *(DistMove - 0.5*Thickness);
  EndPoint.D = -1.0 *(DistMove + 0.5*Thickness);

  SizeRI[0] = ThicknessI;
  SizeRI[1] = widthmax  - widthmin;
  SizeRI[2] = heightmax - heightmin;
  SizeRC[0] = ThicknessO;
  SizeRC[1] = widthmax  - widthmin;
  SizeRC[2] = heightmax - heightmin;

  SizeCI[0] = 2.0*winradius;
  SizeCI[1] = 0.0;
  SizeCI[2] = ThicknessI;
  SizeCC[0] = 2.0*winradius;
  SizeCC[1] = 0.0;
  SizeCC[2] = ThicknessO;

  Center[0] = DistMove;
  if (eWndShape==VT_SQUARE)
  {
    Center[1] = (widthmax  + widthmin) /2.0;
    Center[2] = (heightmax + heightmin)/2.0;
  }
  else if (eWndShape==VT_CIRCLE)
  {
    Center[1] = ywincenter;
    Center[2] = zwincenter;
  }
  else
  { Error("Window shape not (correctly) defined");
  }

  // output text
  // -----------
  if (bBeamStop)
     fprintf(LogFilePtr,"Beamstop ");
  else
     fprintf(LogFilePtr,"Window ");
  if (eWndShape==VT_CIRCLE)
    fprintf(LogFilePtr,"of %5.2f cm diameter in a distance of %6.2f cm", 2.0*winradius, DistMove);
  else
    fprintf(LogFilePtr,"of %5.2f x %5.2f cm (W x H) in a distance of %6.2f cm",
            widthmax-widthmin, heightmax-heightmin, DistMove);
  if (eWndShape==VT_SQUARE && rotang > 0.0)
     fprintf(LogFilePtr," rotated by %6.2f deg", rotang*180.0/M_PI);
  fprintf(LogFilePtr," \n");

  if (Thickness > 0.0)
    fprintf(LogFilePtr,"Thickness: %7.2f cm \n", Thickness);

  fprintf(LogFilePtr,"Outer material: ");
  switch (eMaterialO)
  {
    case VT_WABS_FILE : fprintf(LogFilePtr, "Transmission characteristics read from file %s\n", sTransFileNameO); break;
    case VT_WABS_GD   : fprintf(LogFilePtr, "Gadolinium \n"); Gadolinium(WavO, MuO, &nVal); break;
    case VT_WABS_CD   : fprintf(LogFilePtr, "Cadmium    \n"); Cadmium   (WavO, MuO, &nVal); break;
    case VT_WABS_B10  : fprintf(LogFilePtr, "Bor10      \n"); Bor10     (WavO, MuO, &nVal); break;
    case VT_WABS_EU   : fprintf(LogFilePtr, "Eu         \n"); Eu        (WavO, MuO, &nVal); break;
    case VT_WABS_SI   : fprintf(LogFilePtr, "Silicon    \n"); Silicon   (WavO, MuO, &nVal); break;
    case VT_WABS_VAC  : fprintf(LogFilePtr, "vacuum \n");                                    break;
    case VT_WABS_IDEAL: fprintf(LogFilePtr, "Ideal absorber \n");                           break;
    case VT_NO_WABS   : fprintf(LogFilePtr, "no outer material\n");                         break;
    default: fprintf(LogFilePtr, "\n"); Error("No valid value for material ID (option -c)");
  }

  // window frame material from file
  // -------------------------------
  if (eMaterialO == VT_WABS_FILE)
  {
    // Read transmission file for window frame
    if (sTransFileNameO !=NULL)
    {
      pTransFileO = OpenParameterFile(sTransFileNameO, FALSE, "r");
      if (pTransFileO!=NULL)
      {
        i=0;
        while (ReadLine(pTransFileO, sLine, CHAR_BUF_SMALL-1) > 0)
        { i++;
          sscanf(sLine, "%lf %lf", &WavO[i], &MuO[i]);
        }
        nVal  =i;
        nValFO=nVal;
        fclose(pTransFileO);

        /* check the input data */
        for(i = 1; i <= (nValFO-1); i++)
        {
          if (WavO[i+1] < WavO[i])
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in the transmission file '%s' of the outer material\n", sTransFileNameO);
            fprintf(LogFilePtr,"The wavelength values must be in increasing order! \n");
            exit(-1);
          }
        }
      }
      else
      { Error("Transmission file could not be opened");
      }
    }
    else
    {
      Error("No file name given describing the transmission of the outer material\n");
    }
  }

  if (ThicknessO > 0 && ThicknessO != Thickness)
    fprintf(LogFilePtr, "Thickness of outer material: %5.2f cm \n", ThicknessO);
  if (eMaterialO != VT_WABS_IDEAL && eMaterialO != VT_NO_WABS && eMaterialO != VT_WABS_VAC)
    fprintf(LogFilePtr, "Usable wavelength range    : %5.2f - %5.2f Ang \n", WavO[1], WavO[nValFO]);

  // window pane material from file
  // ------------------------------
  if (sTransFileNameI != NULL  && ThicknessI > 0.0)
  {
    fprintf(LogFilePtr,"Material transmission characteristics of inner material read from file:  %s \n", sTransFileNameI);
    bPane = TRUE; /* activate this material */

    pTransFileI = OpenParameterFile(sTransFileNameI,FALSE, "r");
    if (pTransFileI!=NULL)
    { i=0;
      while (ReadLine(pTransFileI, sLine, CHAR_BUF_SMALL-1) > 0)
      { i++;
        sscanf(sLine, "%lf %lf", &WavI[i], &MuI[i]);
      }
      nValFI=i;
      fclose(pTransFileI);

      /* check the input data */
      for(i = 1; i <= (nValFI-1); i++)
      {
        if (WavI[i+1] < WavI[i])
        {
          fprintf(LogFilePtr,"ERROR: incorrect data in the transmission file0 '%s' of the inner material:\n", sTransFileNameI);
          fprintf(LogFilePtr,"The wavelength values must be in increasing order! \n");
          exit(-1);
        }
      }
    }
    else
    { Error("Transmission file could not be opened");
    }

    if (ThicknessI != Thickness)
      fprintf(LogFilePtr, "Thickness of inner material   : %5.2f cm \n", ThicknessI);
    fprintf(LogFilePtr, "Usable wavelength range       : %5.2f - %5.2f Ang \n", WavI[1], WavI[nValFI]);
  }
  return;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{

 // Geometry data
  if (bVisInstr && eWndShape!=VT_NO_SHAPE)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    if (eWndShape==VT_CIRCLE)
    {
      // frame
      stGeometry.nHolCyls = 1;
      stGeometry.pHolCyl  = calloc(1, sizeof(VtHolCyl));
      stGeometry.pHolCyl[0].Length      = Thickness;
      stGeometry.pHolCyl[0].InnerRadius = BlowUp * winradius;
      stGeometry.pHolCyl[0].Radius      = BlowUp * winradius * 3.0;
      stGeometry.pHolCyl[0].vCntr[0]    = DistMove;
      stGeometry.pHolCyl[0].vCntr[1]    = BlowUp * Center[1];
      stGeometry.pHolCyl[0].vCntr[2]    = BlowUp * Center[2];
      stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
      stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
      stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;

      // pane
      if (bPane==TRUE && ThicknessI > 0.0 && ThicknessI < ThicknessO)
      { stGeometry.nCylinders = 1;
        stGeometry.pCylinder  = calloc(stGeometry.nCylinders, sizeof(VtCylinder));
        stGeometry.pCylinder[0].Length      = ThicknessI;
        stGeometry.pCylinder[0].Radius      = BlowUp * winradius;
        stGeometry.pCylinder[0].vCntr[0]    = DistMove;
        stGeometry.pCylinder[0].vCntr[1]    = BlowUp * Center[1];
        stGeometry.pCylinder[0].vCntr[2]    = BlowUp * Center[2];
        stGeometry.pCylinder[0].vSymAxis[0] = 1.0;
        stGeometry.pCylinder[0].vSymAxis[1] = 0.0;
        stGeometry.pCylinder[0].vSymAxis[2] = 0.0;
      }
    }
    else
    {
      // frame
      stGeometry.nHulls = 1;
      stGeometry.pHull = calloc(1, sizeof(VtHull));
      stGeometry.pHull[0].Length    = Thickness;
      stGeometry.pHull[0].WidthIn   = BlowUp * (widthmax  - widthmin);
      stGeometry.pHull[0].WidthOut  = BlowUp * (widthmax  - widthmin) * 3.0;
      stGeometry.pHull[0].HeightIn  = BlowUp * (heightmax - heightmin);
      stGeometry.pHull[0].HeightOut = BlowUp * (heightmax - heightmin) * 3.0;
      stGeometry.pHull[0].vCntr[0]  = DistMove;
      stGeometry.pHull[0].vCntr[1]  = BlowUp * Center[1];
      stGeometry.pHull[0].vCntr[2]  = BlowUp * Center[2];
      stGeometry.pHull[0].vNormal[0]= 1.0;
      stGeometry.pHull[0].vNormal[1]= 0.0;
      stGeometry.pHull[0].vNormal[2]= 0.0;
      stGeometry.pHull[0].rotAngle  = rotang * 180.0/M_PI;

      // pane
      if (bPane==TRUE && ThicknessI > 0.0 && ThicknessI < ThicknessO)
      { stGeometry.nCuboids = 1;
        stGeometry.pCuboid = calloc(stGeometry.nCuboids, sizeof(VtCuboid));
        stGeometry.pCuboid[0].Length     = ThicknessI;
        stGeometry.pCuboid[0].Width      = BlowUp * (widthmax  - widthmin) ;
        stGeometry.pCuboid[0].Height     = BlowUp * (heightmax - heightmin);
        stGeometry.pCuboid[0].vCntr[0]   = DistMove;
        stGeometry.pCuboid[0].vCntr[1]   = BlowUp * Center[1];
        stGeometry.pCuboid[0].vCntr[2]   = BlowUp * Center[2];
        stGeometry.pCuboid[0].vNormal[0] = 1.0;
        stGeometry.pCuboid[0].vNormal[1] = 0.0;
        stGeometry.pCuboid[0].vNormal[2] = 0.0;
        stGeometry.pCuboid[0].rotAngle   = rotang * 180.0/M_PI;
      }

      // FillRotMatrixZY(rotMatrixPi2, 0, M_PI_2);
      // RotVector(rotMatrixPi2, stGeometry.pCuboid[0].vNormal);
    }
  }
}


/*******************************************************/
/** checks if the neutron is inside the window        **/
/*******************************************************/
short IsOutOfWindow(double Y, double Z)
{
  short  bOut=FALSE;
  double DistSquared=0.0,
         Ynew=0.0, Znew=0.0;

  if (eWndShape==VT_SQUARE && rotang != 0.0)
  {   /*x' = x cos f - y sin f
        y' = y cos f + x sin f */
    Ynew = Y * cos(-rotang) - Z * sin(-rotang);
    Znew = Y * sin(-rotang) + Z * cos(-rotang);
  }
  else
  {
    Ynew = Y;
    Znew = Z;
  }

  if (eWndShape==VT_CIRCLE)
  {  DistSquared = sq(Ynew - ywincenter)
                + sq(Znew - zwincenter);
    if (DistSquared > sq(winradius))
      bOut=TRUE;
  }
  else
  {  if (Ynew < widthmin  || Ynew > widthmax ||
        Znew < heightmin || Znew > heightmax  )
      bOut=TRUE;
  }

  return bOut;
}

/*******************************************************/
/** checks if the neutron is moving inward or outward **/
/*******************************************************/
VtDir GetDir(VectorType Pos, VectorType Dir)
{
  VtDir eDir=VT_NO_DIR;

  if (eWndShape==VT_CIRCLE)
  {
    double r = sqrt(sq(Pos[1]) + sq(Pos[2]) / winradius);

    if (r==1.0) // passes through wall
    { if (Pos[1]*Dir[1] + Pos[2]*Dir[2] > 0.0)
        eDir = VT_OUT;
      else
        eDir = VT_IN;
    }
    else if (r < 1.0) // passes through top or bottom wall
    { eDir = VT_INSIDE;
    }
    else
    { eDir = VT_BEYOND;
    }
  }
  else
  {
    if (fabs(Pos[1])==SizeRI[1]) // passes through left or right wall
    { if (Pos[1]*Dir[1] > 0.0)
        eDir = VT_OUT;
      else
        eDir = VT_IN;
    }
    else if (fabs(Pos[2])==SizeRI[2]) // passes through top or bottom wall
    { if (Pos[2]*Dir[2] > 0.0)
        eDir = VT_OUT;
      else
        eDir = VT_IN;
    }
    else if (IsOutOfWindow(Pos[1], Pos[2])==TRUE) // passes through top or bottom wall
    { eDir = VT_BEYOND;
    }
    else
    { eDir = VT_INSIDE;
    }
  }

  return eDir;
}
