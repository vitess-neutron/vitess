/*********************************************************************************************/
/*  VITESS module  SPACE and CHOPPER                                                         */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00  Jun 1999  D. Wechsler                                                               */
/* 1.01  Mar 2001  S. Manoshin    include of gravity effect                                  */
/* 1.02  Jun 2001  K. Lieutenant  SOFTABORT                                                  */
/* 1.03  Jan 2002  G. Zsigmond    weighted center of beam                                    */
/* 1.04  Jan 2002  K. Lieutenant  reorganisation                                             */
/* 1.05  Apr 2003  K. Lieutenant  horizontal distance                                        */
/* 1.06  Nov 2003  K. Lieutenant  chopper sets colour to window number  and                  */
/*                                choice: treatment of neutrons outside chopper              */
/* 1.07  Jan 2004  K. Lieutenant  changes for 'instrument.dat'; correction 'time to zero'    */
/*                                new: absorption by Bor-10, absorption by Gd changed        */
/* 1.08  Feb 2004  K. Lieutenant  'message.h', 'ERROR' and 'FullParName' included; output of */
/*                                parameter file data; optimal phase into 'instrument.inf'   */
/* 1.09  Nov 2005  K. Lieutenant  option: equivalent windows added                           */
/* 1.10  Mar 2011  K. Lieutenant  correction: side deviation                                 */
/* 1.11  Jan 2012  K. Lieutenant  visualization                                              */
/* 1.12  Feb 2020  K. Lieutenant  new central visualization parameters                       */
/* 1.13  Aug 2020  K. Lieutenant  time of arrival can be set by random choice                */
/*********************************************************************************************/

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "bender_inter_data.h"
#include "message.h"


/******************************/
/** Structures               **/
/******************************/
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


/******************************/
/** Prototypes               **/
/******************************/
void           OwnInit         (int argc, char *argv[]);    // Reads input parameters and sets global variables
void           ReadChopperData ();                          // Reads chopper parameters from file
void           SetGeometry     (char* sColor);              // Fills the structure stGeometry for visualization 
void           OwnCleanup      ();                          // Does module specific cleanup

unsigned short BlockedByChopper(Neutron* pNeutron);         // Checks if neutron is blocked by the chopper
double         RedAngle        (double angle, short dir);   // reduces the angle to [0°, 360°] or [-360°, 0°] (dir = -1)
double         ModPhase        (double phase, int nSect);   // reduces the phase from a full circle to a section of a circle
double         CalcMinTrnd     (void);                      // calculates a proper value for the lower end of the time interval of the random time of arrival


/***********************************/
/** global and static variables   **/
/***********************************/
McCompID _eModule=MCN_CHOP_DISC;

// Input parameters
char	*ChopperFileName=NULL;     // -C  [-]    name of the file describing the chopper disc
double Rpm=0.0,                  // -s [1/min] rotation speed 
       ChopperInitialOffset=0.0, // -o [deg]   orientation of the chopper at t=0 
       Distance=0.0;             // -l  [cm]   distance of the chopper from the end of the previous component 
short  eAbsMaterial = FALSE,     // -g  [-]    enum: absorption in chopper: 0: ideal  1:Gadolinium  2: Bor 
       bRndTof      = FALSE,     // -r  [-]    flag: time of arrival is set by a random choice within the period of the chopper disc
       bZeroTime    = FALSE,     // -z  [-]    flag: chopper sets neutron time close to zero after the chopper
       bPassOutside = TRUE,      // -p  [-]    flag: neutrons can pass outside the chopper 
       bSetColour   = FALSE;     // -c  [-]    flag: chopper sets colour to window number 

// Deactivated in GUI
int    NumEquWnds   = 1;         // -n  [-]    number of equivalent windows used to generate pulses, deactivated in GUI

// Variables determined from input parameters or file data
double Period   =0.0;            //     [ms]   time for 1 revolution of the disc
double Frequency=0.0;            //            rot.freq 2*pi*60*rpm  
double TrndMin  =0.0,            //     [ms]   min. max. value for randomized arrival time
       TrndMax  =0.0; 
double Angle    =0.0;            //            orientation of the center of beamline in the chopper system 
FILE	*ChopperFile=NULL;         //     [-]    pointer to the file describing the chopper disc
Plane	 Endpoint;                 //     [-]    struture describing the chopper position and orientation

// Variables read from file
// Chopper ThisChopper;          //            all chopper data        
short         NumberWindows=0;   // -N  [-]    number of choppers
double        Radius =0.0;       // -R  [cm]   radius of the chopper  [cm]  
double        CenterX=0.0,       //     [cm]   fixed
              CenterY=0.0,       // -Y  [cm] 
              CenterZ=0.0;       // -Z  [cm]   centre of the chopper in the coordinate system of the beamline [cm]  
ChopperWindow *Window;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long   i=0;

  double mu  =0.0,    /* absorption coeffient of the absorbing material [1/cm] */
         prob=0.0;    /* resulting attenuation inside absorbing material       */

  double TimeOF=0.0,
         AveTimeOF=0.0,                 /* average time of flight to chopper weighted by count rate */
         SumProb =0.0,                   /* sum of count rates of all traj. reaching the chopper */
         CtrBeamX=0.0, 
         CtrBeamY=0.0,
         CtrBeamZ=0.0;                   /* center of beam of all traj. reaching the chopper at chopper weighted by count rate */
         Neutron OutNeutron;

  // initialisation
  // --------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.13");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
  bLengthCmpr = TRUE;

  /* Reading chopper file  */
  ReadChopperData();
  if (bRndTof) 
  { TrndMin = CalcMinTrnd();
    TrndMax = TrndMin+Period;
  }

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      // Submit both the Neutron and the plane to a subroutine and find the intercept 
      // ----------------------------------------------------------------------------
      if (InputNeutrons[i].Position[0] > -Endpoint.D)
      CountMessageID(ALL_BEHIND_COMPONENT, InputNeutrons[i].ID);

      // 	Move neutron to window with gravity effect and calculate Time of Flight (ms)
      // -----------------------------------------------------------------------------
      if (keygrav == 1)
      {
        TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
      }
      else
      {
        TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
      }

      if (bRndTof)
        InputNeutrons[i].Time = MonteCarlo(TrndMin, TrndMax);
      else
        InputNeutrons[i].Time += (double)TimeOF;

      OutNeutron = InputNeutrons[i];

      AveTimeOF += OutNeutron.Probability*OutNeutron.Time;
      CtrBeamX  += OutNeutron.Probability*OutNeutron.Position[0];
      CtrBeamY  += OutNeutron.Probability*OutNeutron.Position[1];
      CtrBeamZ  += OutNeutron.Probability*OutNeutron.Position[2];
      SumProb   += OutNeutron.Probability;

      OutNeutron.Position[0]=0.0;

      // submit the neutron to a subroutine that works out if the chopper gets in the way
      // --------------------------------------------------------------------------------
      if (BlockedByChopper(&OutNeutron))
      {
        /* non perfect absorption */

        switch (eAbsMaterial)
        {	/* ideally absorbing material */
          case 0:
            WriteIAP(&InputNeutrons[i], VT_ABSORBED);
            continue;
            break;

          /* gadolinium */
          case 1:
            if (OutNeutron.Wavelength <= 0.35)
            {	prob = -0.1843*OutNeutron.Wavelength  + 1.0128;
              if (prob > 0.961) prob = 0.961;
            }
            else if ( OutNeutron.Wavelength < 6.0)
            {	double *pLmbdList=NULL, *pMuList=NULL;

              mu   = Interpolation(OutNeutron.Wavelength, 1, pLmbdList, pMuList, 44);
              prob = exp(-mu*0.02);  /* typical thickness 2 x 100 um */
            }
            else
            {	prob = 0.0;
            }
            break;

          /* Bor-10 */
          case 2:
            if (OutNeutron.Wavelength < 0.29)
            {	double eV, mcnp;

              eV   = 1.0e-06*ENERGY_FROM_LAMBDA(OutNeutron.Wavelength);
              mcnp = 612.07/sqrt(eV);
              mu   = mcnp * NA * 2.46E-24 / 10.811;
              prob = exp(-mu*0.05);  /* typical thickness 2 x 250 um */
            }
            else if ( OutNeutron.Wavelength < 6.0)
            {	double *pLmbdList=NULL, *pMuList=NULL;

              mu   = Interpolation(OutNeutron.Wavelength, 3, pLmbdList, pMuList, 44);
              prob = exp(-mu*0.05);  /* typical thickness 2 x 250 um */
            }
            else
            {	prob = 0.0;
            }
            break;

          default:
            Error("This kind of absorption is not supported");
        }
        OutNeutron.Probability *= prob;
      }

      if (OutNeutron.Probability < 0.0)
      {
        Error("NeutronProbability < 0");
      }
      else if (OutNeutron.Probability < wei_min)
      {
        WriteIAP(&InputNeutrons[i], VT_ABSORBED);
        continue;
      }
      else
      {	WriteNeutron(&OutNeutron);
        WriteIAP(&InputNeutrons[i], VT_PASSED);
      }
    }
  }

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
my_exit:
  if (SumProb != 0.0)
  {
    CtrBeamX = CtrBeamX/SumProb;
    CtrBeamY = CtrBeamY/SumProb;
    CtrBeamZ = CtrBeamZ/SumProb;
    AveTimeOF = AveTimeOF/SumProb;
    fprintf(LogFilePtr,"Center of beam before the chopper: X = %f cm Y = %f cm Z = %f cm TOF = %f ms \n",CtrBeamX, CtrBeamY, CtrBeamZ, AveTimeOF);
  }
  else
  {
    Warning("No neutron hit the chopper\n");
  }

  SetGeometry("blue");                       // write geometry data for visualization
  OwnCleanup();                              // write messages, free memory
  Cleanup(-Endpoint.D,0.0,0.0, 0.0,0.0);     // print intensity, write instrument.inf, 

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit   (int argc, char *argv[])
{
  long   i=0;

  Endpoint.A = 1.0;
  Endpoint.B = 0.0;
  Endpoint.C = 0.0;
  Endpoint.D = 0.0;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'C':
          ChopperFileName = &argv[i][2];
          ChopperFile = OpenInputFile(ChopperFileName, FALSE, "r");
          if (ChopperFile==NULL)
          {
            fprintf(LogFilePtr, "ERROR: Chopper parameter file %s could not be opened\n", ChopperFileName);
            exit(-1);
          }
          break;

        case 'o':										/*Offset [deg] */
          ChopperInitialOffset = atof(&argv[i][2]);
          ChopperInitialOffset = 2.0*M_PI*ChopperInitialOffset/360.0;
          break;

        case 's':
          Rpm = atof(&argv[i][2]);			          		/* Rounds per minute  */
          Frequency = 2.0*M_PI*Rpm/60.0;  /* Frequency in rad/s */
          Period = 60.0 * 1000.0 / Rpm;               /* period in ms       */
          break;

        case 'l':
          Distance   = atof(&argv[i][2]);
          Endpoint.D = -Distance;
          break;


        case 'g':
          /* g unequal 0 => non perfect chopper absorption activated */
          eAbsMaterial =  (short) atoi(&argv[i][2]);
          switch (eAbsMaterial)
          {	
            case 0: fprintf(LogFilePtr,"Ideal absorption in chopper disk assumed\n"); break;
            case 1: fprintf(LogFilePtr,"Absorption of Gd in chopper disk assumed\n"); break;
            case 2: fprintf(LogFilePtr,"Absorption of Bor-10 in chopper disk assumed\n"); break;
            default: Error("This kind of absorption is not supported");
          }
          break;

        case 'r':
          /* r unequal 0 => chopper sets time to zero */
          bRndTof = (short) atoi(&argv[i][2]);
          if (bRndTof)
          fprintf(LogFilePtr,"Time of arrival is set by a random choice within the period of the chopper\n");
          break;

        case 'z':
          /* z unequal 0 => chopper sets time close to zero */
          bZeroTime = (short) atoi(&argv[i][2]);
          if (bZeroTime)
          fprintf(LogFilePtr,"Time is set close to zero after the chopper\n");
          break;

        case 'c':
          /* c unequal 0 => chopper sets colour to window number */
          bSetColour =  (short) atoi(&argv[i][2]);
          if (bSetColour)
          fprintf(LogFilePtr,"Colours of trajectories will be set to window number\n");
          break;

        case 'p':
          /* z unequal 0 => chopper sets time to zero */
          bPassOutside = (short) atoi(&argv[i][2]);
          if (bPassOutside==FALSE)
          fprintf(LogFilePtr,"Neutrons passing outside the chopper are removed\n");
          break;

        // not yet activated
        case 'R':										/* radius of the chopper */
          Radius = atof(&argv[i][2]);
          break;

        case 'Y':										/* y-component of the centre of the chopper */
          CenterY = atof(&argv[i][2]);
          break;
        case 'Z':										/* z-component of the centre of the chopper */
          CenterZ = atof(&argv[i][2]);
          break;

        // deactivated
        case 'n':										/* no of windows for pulse generation */
          NumEquWnds = atoi(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  return;
}


/*******************************************************/
/** Read chopper file                                 **/
/*******************************************************/
void ReadChopperData() 
{
  short  k=0;
  int    iv=0;
  char   Buffer[CHAR_BUF_LENGTH]="";
  double WindowHeight=0.0;

  // reads data of the chopper
  fgets(Buffer,100,ChopperFile);
  sscanf(Buffer,"%d", &iv);
  NumberWindows = (short) iv;

  fgets(Buffer,100,ChopperFile);
  sscanf(Buffer,"%lf",&Radius);
  Radius = fabs(Radius);

  fgets(Buffer,100,ChopperFile);
  sscanf(Buffer,"%lf %lf", &CenterZ, &CenterY);

  // Allocates memory for the chopper window data and initializes them
  if ((Window = (ChopperWindow *)malloc(NumberWindows*sizeof(ChopperWindow)))==NULL)
    Error("Out of memory whilst reading chopper data");

  for (k=0; k < NumberWindows; k++) 
  {
    Window[k].Pos     = 0.0;  Window[k].Opening = 0.0;
    Window[k].Left    = 0.0;  Window[k].Right   = 0.0;
    Window[k].Bottom  = 0.0;
  }

  // writes out chopper data
  Angle = atan2(-CenterY, -CenterZ);
  fprintf(LogFilePtr, "Radius of chopper           :  %6.2f         cm\n", Radius);
  fprintf(LogFilePtr, "Center of chopper axle (Z,Y): (%+6.2f,%+6.2f) cm\n", CenterZ, CenterY);
  fprintf(LogFilePtr, "Chopper open at t=0 (without offset) for a window at %-5.1f deg\n", Angle*180/M_PI);

  // data of the windows
  for (k=0; k < NumberWindows; k++) 
  {
    if (fgets(Buffer, 100, ChopperFile)==NULL) 
    {
      fprintf(LogFilePtr,"ERROR: File %s does not contain %d window definitions\n", ChopperFileName, NumberWindows);
      fclose(ChopperFile);
      exit(-1);
    }
    sscanf(Buffer,"%lf %lf %lf %lf %lf", &Window[k].Pos,  &WindowHeight, &Window[k].Opening, &Window[k].Left, &Window[k].Right);

    fprintf(LogFilePtr, "Window %d: Position: %7.2f deg   Aperture: %6.2f deg   Height: %6.2f cm\n",
	                      k+1, Window[k].Pos, Window[k].Opening, WindowHeight);

    if (Window[k].Left > 0.0 || Window[k].Right > 0.0)
      fprintf(LogFilePtr, "  Deviation: %6.2f deg left, %6.2f deg right\n", Window[k].Left, Window[k].Right);

    Window[k].Pos     *= M_PI/180.0;
    Window[k].Opening *= M_PI/180.0;
    Window[k].Left    *= M_PI/180.0;
    Window[k].Right   *= M_PI/180.0;

    Window[k].Bottom   = Radius - WindowHeight;
  }

  fclose(ChopperFile);
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  long   nModuleNo;   /* number of the previous module (not needed) */
  short  dir=0,
         k;           /* index of the chopper window  */
  double time,        /* time of flight*/
         phi_wnd,     /* orientation of the window after TOF 'time' or at t=0 [deg] */
         phi_red;     /* phi_ges reduced to a value in [0,360[ */
  double TimeMeas,    /* measuring time     (from simulation.inf, not needed here) */
         LmbdWanted,  /* desired wavelength (from simulation.inf)                  */
         Freq,        /* source frequency   (from simulation.inf, not needed here) */
         Length,      /* length of the instrument until chopper module */
         RotZ, RotY;  /* orientation of the output of the previous component (not needed) */
  VectorType EndPos;   /* position of the output of the previous component (not needed) */

  // Visualisation of the slit geometry
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    ReadSimData  (&TimeMeas, &LmbdWanted, &Freq);
    if (LmbdWanted > 0.0)
    {	nModuleNo=ReadInstrData(0, EndPos, &Length, &RotZ, &RotY, sInstrInfIn);
      time = (Length-0.01*Endpoint.D) / (10.0*V_FROM_LAMBDA(LmbdWanted)); /* velocity in m/s instead of cm/ms */
    }
    else
    { time = 0.0;
    }
    dir  = (short) (Frequency < 0.0 ? -1 : 1);

    stGeometry.pCircle  = (VtCircle*) calloc(NumberWindows + 1, sizeof(VtCircle));
    stGeometry.nCircles = NumberWindows + 1; 
    // stGeometry.pLine    = (VtLine*) calloc(2*NumberWindows, sizeof(VtLine));
    // stGeometry.nLines   = 2*NumberWindows; 

    stGeometry.pCircle[0].vCntr[0]   = -Endpoint.D/CmprFact;
    stGeometry.pCircle[0].vCntr[1]   = CenterY;
    stGeometry.pCircle[0].vCntr[2]   = CenterZ;
    stGeometry.pCircle[0].Radius     = Window[0].Bottom;
    stGeometry.pCircle[0].AngleBeg   =   0.01;
    stGeometry.pCircle[0].AngleEnd   = 359.99;
    stGeometry.pCircle[0].vNormal[0] =   1.0;
    stGeometry.pCircle[0].vNormal[1] =   0.0;
    stGeometry.pCircle[0].vNormal[2] =   0.0;

    for (k=0; k < NumberWindows; k++)
    { 
      phi_wnd = 180.0/M_PI * (Window[k].Pos + ChopperInitialOffset + Frequency * time) + 90.0; // 0° to left, not to top in vis. tool
      phi_red = RedAngle(phi_wnd, dir);

      stGeometry.pCircle[k+1].Radius     = Radius;
      stGeometry.pCircle[k+1].AngleBeg   = phi_red + 0.5*180.0/M_PI*Window[k].Opening;
      stGeometry.pCircle[k+1].AngleEnd   = phi_red - 0.5*180.0/M_PI*Window[k].Opening;
      stGeometry.pCircle[k+1].vCntr[0]   = -Endpoint.D/CmprFact;
      stGeometry.pCircle[k+1].vCntr[1]   = CenterY;
      stGeometry.pCircle[k+1].vCntr[2]   = CenterZ;
      stGeometry.pCircle[k+1].vNormal[0] = 1.0;
      stGeometry.pCircle[k+1].vNormal[1] = 0.0;
      stGeometry.pCircle[k+1].vNormal[2] = 0.0;

      /* stGeometry.pLine[2*k].vPosBeg[0] = -Endpoint.D;
      stGeometry.pLine[2*k].vPosBeg[1] = CenterY - Window[k].Bottom * cos(M_PI/180.0*stGeometry.pCircle[k+1].AngleBeg);
      stGeometry.pLine[2*k].vPosBeg[2] = CenterZ + Window[k].Bottom * sin(M_PI/180.0*stGeometry.pCircle[k+1].AngleBeg);
      stGeometry.pLine[2*k].vPosEnd[0] = -Endpoint.D;
      stGeometry.pLine[2*k].vPosEnd[1] = CenterY - Radius * cos(M_PI/180.0*stGeometry.pCircle[k+1].AngleBeg);
      stGeometry.pLine[2*k].vPosEnd[2] = CenterZ + Radius * sin(M_PI/180.0*stGeometry.pCircle[k+1].AngleBeg);  

      stGeometry.pLine[2*k+1].vPosBeg[0] = -Endpoint.D;
      stGeometry.pLine[2*k+1].vPosBeg[1] = CenterY - Window[k].Bottom * cos(M_PI/180.0*stGeometry.pCircle[k+1].AngleEnd);
      stGeometry.pLine[2*k+1].vPosBeg[2] = CenterZ + Window[k].Bottom * sin(M_PI/180.0*stGeometry.pCircle[k+1].AngleEnd);
      stGeometry.pLine[2*k+1].vPosEnd[0] = -Endpoint.D;
      stGeometry.pLine[2*k+1].vPosEnd[1] = CenterY - Radius * cos(M_PI/180.0*stGeometry.pCircle[k+1].AngleEnd);
      stGeometry.pLine[2*k+1].vPosEnd[2] = CenterZ + Radius * sin(M_PI/180.0*stGeometry.pCircle[k+1].AngleEnd); */
    }
  }
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  /* print error that might have occured many times */
  PrintMessage(CHOP_PASSED_OUTSIDE,  "", ON);
  PrintMessage(ALL_BEHIND_COMPONENT, "", ON);

  fprintf(LogFilePtr," \n");

  /* free allocated memory */
  free(Window);

  return;
}


/*************************************************************************************/
/* This subroutine accepts two structured variables containing information about the */
/* chopper in question and a single neutron incident on the plain of the chopper.    */
/* It calculates the offset of the chopper at the time index of the incident neutron */
/* and returns FALSE if the neutron is incident on a window.                         */
/*************************************************************************************/
unsigned short BlockedByChopper(Neutron* ThisNeutron)
{
  double ChopperOffset=0.0, ChopperOffsetRed=0.0, OriginNeutronDistance, Time;
  double Left, Right, WindowAngle=0.0, 
         NeutronAngle,   // angle from chopper axle to point of striking
         NeutronDist,    // distance from chopper axle to point of striking
         dY, dZ;         // point of striking
  short  k;
  int    RightTurns=0;
  int    LeftTurns=0;

  /***********************************************************************************/
  /* The main loop here cycles through each of the chopper's windows to find if any  */
  /* are open at the time the neutron strikes.                                       */
  /***********************************************************************************/


  if (bSetColour)
  ThisNeutron->Color = 0;

  for (k=0; k < NumberWindows;k++)
  {
    RightTurns=0; LeftTurns=0; /*modified*/

    /***********************************************************************************/
    /* The first check is to see whether the incident neutron is "above" the bottom of  */
    /* the chopper window,i.e. the distance to the center of the chopper is calculated  */
    /* and then compared to the distance bottom of chopper window <-> center of chopper */
    /***********************************************************************************/
    OriginNeutronDistance = sqrt((ThisNeutron->Position[0]-CenterX)
    *(ThisNeutron->Position[0]-CenterX)
    +(ThisNeutron->Position[1]-CenterY)
    *(ThisNeutron->Position[1]-CenterY)
    +(ThisNeutron->Position[2]-CenterZ)
    *(ThisNeutron->Position[2]-CenterZ));
    if(OriginNeutronDistance < Window[k].Bottom) continue;

    /* second check: if neutron does not hit chopper at all*/
    if(OriginNeutronDistance > Radius)
    goto passed_outside;

    /***********************************************************************************/
    /* The next statements calculate the angle between the chopper axle and the        */
    /* neutron and their distance.                                                     */
    /***********************************************************************************/
    dZ = ThisNeutron->Position[2] - CenterZ;
    dY = ThisNeutron->Position[1] - CenterY;
    NeutronAngle = atan2(dY,dZ);
    NeutronDist  = sqrt(dY*dY + dZ*dZ);

    /***********************************************************************************/
    /* The offset of this window at the time the neutron strikes is calculated.        */
    /***********************************************************************************/
    Time = ThisNeutron->Time/1000.0;  		/*msec. to sec. */
    ChopperOffset = Time * Frequency  + ChopperInitialOffset;

    WindowAngle = ChopperOffset + Window[k].Pos;
    Left        = ChopperOffset + Window[k].Pos - Window[k].Opening/2.0; 
    Right       = ChopperOffset + Window[k].Pos + Window[k].Opening/2.0;

    /* Correction of window width by deviation */
    if (Window[k].Left != 0.0)	
    Left  -= Window[k].Left  - asin(Window[k].Bottom/NeutronDist*sin(Window[k].Left));
    if (Window[k].Right != 0.0)	
    Right += Window[k].Right - asin(Window[k].Bottom/NeutronDist*sin(Window[k].Right));

    /***********************************************************************************/
    /* The angles calculated above are now renormalized to lie between +PI and -PI     */
    /***********************************************************************************/
    while(Left>=M_PI)  {Left-=2.0*M_PI; LeftTurns--;}
    while(Left<=-M_PI) {Left+=2.0*M_PI; LeftTurns++;}

    while(Right>=M_PI) {Right-=2.0*M_PI; RightTurns--;}
    while(Right<=-M_PI){Right+=2.0*M_PI; RightTurns++;}

    while(WindowAngle>=M_PI)    WindowAngle-=2.0*M_PI;
    while(WindowAngle<=-M_PI)   WindowAngle+=2.0*M_PI;

    while(ChopperOffset>=M_PI)  ChopperOffset-=2.0*M_PI;
    while(ChopperOffset<=-M_PI) ChopperOffset+=2.0*M_PI;

    /***********************************************************************************/
    /* Check this angle against the angle of the window sides - if it lie between the  */
    /* window sides, the neutron is NOT BlockedByChopper.                              */
    /***********************************************************************************/
    if(RightTurns==LeftTurns)
    {
      if((NeutronAngle>Left)&&(NeutronAngle<Right))
      {
        if (bSetColour)
        ThisNeutron->Color = (short) (k+1);
        goto passed;
      }
    }
    else
    {
      if (((NeutronAngle<Left)&&(NeutronAngle<Right)) ||   /* why not "< Left or > Right" */
          ((NeutronAngle>Left)&&(NeutronAngle>Right)))
      {
        if (bSetColour)
        ThisNeutron->Color = (short) (k+1);
        goto passed;
      }
    }
  }

  /***********************************************************************************/
  /* If the routine reaches this point, it has been found that none of the chopper   */
  /* is open at this time windows; the neutron IS BlockedByChopper.                  */
  /***********************************************************************************/

  return TRUE;

  passed_outside:
  /* depending on criterion: treatment of neutrons outside the chopper or not */
  if (bPassOutside==FALSE)
  {	return TRUE;    /* treated as blocked though it passed outside the chopper */
  }
  else
  {	CountMessageID(CHOP_PASSED_OUTSIDE, ThisNeutron->ID);
  }

  passed:
  /* set time (close to) zero, if demanded */
  if (bZeroTime)
  {
    ChopperOffsetRed  = ModPhase(ChopperOffset, NumEquWnds) ;
    ThisNeutron->Time = 1000.0 * ChopperOffsetRed / Frequency;
  }
  return FALSE;
}


/************************************************************/
/* RedAngle: reduces the angle to [0, 360] deg             */
/*                             or [-360,0] deg (dir = -1)  */
/************************************************************/
double RedAngle(double angle, short dir)
{
  double red_angle;

  red_angle = angle - floor(angle/360.0)*360.0;
  if (dir==1)
    red_angle -= 360.0;

  return(red_angle);
}


/****************************************************************************/
/* ModPhase: reduces the phase from a full circle to a section of a circle, */
/*           i.e. from [-pi,pi] to [-pi/n, pi/n]                            */
/*   phi_in: intial angle        [rad]                                      */
/*   nSect : number of sections   [-}                                       */
/****************************************************************************/
double ModPhase(double phase, int nSect)
{
  double ph_mod, al, k;

  al     = 2*M_PI/nSect;
  k      = floor(phase/al + 0.5);
  ph_mod = phase - k * al;

  return ph_mod;
}


/**************************************************************************************************/
/* calculates a proper value for the lower end of the time interval of the random time of arrival */
/**************************************************************************************************/
double CalcMinTrnd     (void)
{
  double Phase0=0.0, T0=0.0;

  if (NumberWindows > 1)
  {
    Phase0 = ChopperInitialOffset + 0.5*(Window[0].Pos + Window[1].Pos);   
  }
  else 
  {
    Phase0 = ChopperInitialOffset + Window[0].Pos + Radians(180.0);
  }

  T0 = Period * Phase0 / (2.0*M_PI);

  return T0;
}
