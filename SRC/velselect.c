/********************************************************************************************/
/*  VITESS module 'velselect'                                                               */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Jun 1999  D. Wechsler							                                                  */
/* 1.1  Jun 2001  K. Lieutenant  correction: memory allocation for pAngIn + SOFTABORT       */
/* 1.2  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Feb 2020  K. Lieutenant  new central visualization parameters                       */
/* 1.4  Jul 2021  K. Lieutenant  entrance and exit window, any position, inner radius       */
/* 2.0  Aug 2021  K. Lieutenant  new algorithm for correct treatment of selector abobe guide*/
/********************************************************************************************/

#include <stdlib.h>
#include <math.h>

#include "message.h"
#include "general.h"
#include "init.h"
#include "softabort.h"

#define NO_CHANNEL -10000


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit(int argc, char *argv[]);                                   // reads input parameters and initializes global variables
void   OwnCleanup();                                                      // does module specific cleanup
void   SetGeometry(char* sColor);                                         // fills the structure stGeometry for visualization
double SelectorAngle(const double y, const double z);                     // calculates the orientation of the selector for a neutron position (x,y,z)
short  DetChannel(int* pChan, const double NeutAng, const double RotAng); // Determines in which channel the neutron is


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
double  WndWidth = 1.0e10,  // -W  [cm]   width of entrance and exit window
        WndHeight= 1.0e10,  // -H  [cm]   height of entrance and exit window
        RadiusO  = 0.0,     // -r  [cm]   outer radius of the velocity selector 
        RadiusI  = 0.0,     // -i  [cm]   inner radius of the velocity selector 
        Length   = 0.0,     // -l  [cm]   length of the velocity selector 
        Spacer   = 0.0,     // -d  [cm]   blade thickness   
        Freq     = 0.0,     // -s  [Hz]   number of velocity selector rotations per second
        TwistD   = 0.0,     // -c  [deg]  twist of the velocity selector channels
        DistAxle = 0.0,     // -o  [cm]   distance: center of beamline - axle of the velocity selector
        AxlePosY = 0.0,     // -Y  [cm]   horizontal position of the axle (in the co-ordinate system of the beamline)
        AxlePosZ = 0.0;     // -Z  [cm]   vertical position of the axle
long    nChannels= 1;       // -w   [-]   number of selector channels
short   bPassOutside=FALSE; // -p   [-]   flag: neutrons can pass outside the selector 
                                    
// Variables determined from input parameters or trajectory data
double  BeamDir =0.0,      //     [deg]  direction of the beam seen from the axle of the selector  
        BladeAng=0.0,      //     [rad]  angular width of a blade at the origin
        ChnWidth=0.0,      //     [cm]   inner width of a window at the origin
        ChnAngle=0.0,      //     [rad]  angular inner width of a window
        nRot    =0.0,      //    [1/ms]  number of velocity selector rotations per millisecond
        TwistR  =0.0;      //     [rad]  twist of the velocity selector channels


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short   bChanIn=FALSE, bChanOut=FALSE;    // flag: neutron is within a channel at entrance and exit
  int     kChanIn=0,     kChanOut=0;        // channel, in which the neutron is at entrance and exit
  long    i=0;
  double  Velocity=0.0,                     // neutron velocity  [cm/ms]
          Dist=0.0,                         // distance of the neutron position to the axle
          NeutAngIn=0.0, NeutAngOut=0.0,    // selector orientation of the neutron position at entrance and exit
          RotAngIn =0.0, RotAngOut =0.0,    // selector orientation of the first blade  at entrance and exit
          ToF=0.0;                          // TOF of the neutron through  the selector
  Neutron Output;

  InitNeutron(&Output);
 
  // initialisation
  // --------------
  _eModule = MCN_VEL_SELECT;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "2.0");
  OwnInit(argc, argv);
  MsgInit();
 
  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  DECLARE_ABORT
 
  // loop over all trajectories
  // --------------------------
	while (ReadNeutrons()!= 0)
  {
    for (i=0; i<NumNeutGot; i++)
    {
  		CHECK;    

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /* checks entrance position    */
        if (fabs(InputNeutrons[i].Position[1]) > 0.5*WndWidth || fabs(InputNeutrons[i].Position[2]) > 0.5*WndHeight )
          goto GetNewNeutron;

        Velocity = V_FROM_LAMBDA(InputNeutrons[i].Wavelength);
  
        /* checks if neutron hits the area covered by the selector blades in the front */
        Dist = sqrt(sq(InputNeutrons[i].Position[1] - AxlePosY) + sq(InputNeutrons[i].Position[2] - AxlePosZ));
        if (Dist < RadiusI)
          goto GetNewNeutron; // CountMessageID(SELECT_NO_BLADES, InputNeutrons[i].ID);
         
        if (Dist > RadiusO)
        { if (bPassOutside==TRUE)
            CountMessageID(SELECT_OUTSIDE, InputNeutrons[i].ID);
          else
            goto GetNewNeutron;
        }

        /* calculation of the angle: z-axis; center velsel.; neutron position */
        NeutAngIn = SelectorAngle(InputNeutrons[i].Position[1], InputNeutrons[i].Position[2]);

        /* Rotation angle of velsel. corresponding to neutron time */
        RotAngIn = 2.0*M_PI*nRot*InputNeutrons[i].Time;
  
        /* Determine entrance channel */
        bChanIn = DetChannel(&kChanIn, NeutAngIn, RotAngIn);
        if (bChanIn==FALSE)
          goto GetNewNeutron;
  
        /* time for passing the velsel. */
        ToF = Length/(Velocity*InputNeutrons[i].Vector[0]);
  
        /* new coordinates and new neutron angle and test if cylinder walls absorbed the neutron*/
        Output = InputNeutrons[i];
        Output.Position[0] += Velocity*ToF*Output.Vector[0];
        Output.Position[1] += Velocity*ToF*Output.Vector[1];
        Output.Position[2] += Velocity*ToF*Output.Vector[2];
  
        Output.Time+=ToF;
  
        /* checks exit position    */
        if (fabs(Output.Position[1]) > 0.5*WndWidth || fabs(Output.Position[2]) > 0.5*WndHeight )
          goto GetNewNeutron;

        /* checks if neutron hits the area covered by the selector blades on the backside */
        Dist = sqrt(sq(Output.Position[1] - AxlePosY) + sq(Output.Position[2] - AxlePosZ));
        if (Dist < RadiusI)
          goto GetNewNeutron; // CountMessageID(SELECT_NO_BLADES, Output.ID);
            
        if (Dist > RadiusO)
        { if (bPassOutside==TRUE)
            CountMessageID(SELECT_OUTSIDE, Output.ID);
          else
            goto GetNewNeutron;
        }

        NeutAngOut = SelectorAngle(Output.Position[1], Output.Position[2]);

        /* update TrailingEdge and Leading Edge for the channel under consideration*/
        RotAngOut = RotAngIn + (ToF*2.0*M_PI*nRot) - TwistR;
  
        /* Determine exit channel */
        bChanOut = DetChannel(&kChanOut, NeutAngOut, RotAngOut);
        if (bChanOut==FALSE)
          goto GetNewNeutron;
  
        if (kChanIn == kChanOut) 
          goto Transmission;

      GetNewNeutron: 
        continue; /* case of neutron blocked by spacers or absorbed within a channel */
  
      Transmission:
        Output.Position[0]=0.0;
    
        WriteNeutron(&Output);
      }
    }
  }
  
// Finish: print parameters, write geometry and instrument file, free memory
// -------------------------------------------------------------------------
my_exit:
  /* writes to log file */
  fprintf(LogFilePtr, "%ld channels of  %6.3f deg each\n", nChannels, ChnAngle*180.0/M_PI); 
  fprintf(LogFilePtr, "Axle position: (%6.3f,%6.3f) cm\n", AxlePosY, AxlePosZ); 
  if (bPassOutside==TRUE)
    fprintf(LogFilePtr, "Neutrons passing outside the rotor are kept\n");
  
  /* print messages that might have occured many times */
  PrintMessage(SELECT_OUTSIDE,   "", ON);
  PrintMessage(SELECT_NO_BLADES, "", ON);
  
  /* write geometry data for visualization */
  SetGeometry("grey");                       

  /* print intensity, write instrument.inf, free memory */
  Cleanup(Length,0.0,0.0, 0.0,0.0);
  
  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int    i=0;
  double dist=0.0;    // distance: origin - axle from y and z position of the axle 

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'W':
          WndWidth  = atof(&argv[i][2]); /* width of entrance and exit window [cm] */
          break;
        case 'H':
          WndHeight = atof(&argv[i][2]); /* height of entrance and exit window [cm] */
          break;

        case 'l':
          Length = atof(&argv[i][2]); /* length of velocity selector [cm] */
          break;
        case 'r':
          RadiusO = atof(&argv[i][2]);    /* outer radius of the velocity selector [cm] */
          break;
        case 'i':
          RadiusI = atof(&argv[i][2]);    /* inner radius of the velocity selector [cm] */
          break;

        case 's':
          Freq = atof(&argv[i][2]);   /* number of velsel. rotations per second */
          nRot = Freq/1000.0;         /* conversion to rotation per ms */
          break;
        case 'w':
          nChannels = atol(&argv[i][2]); /* number of windows */
          break;
        case 'c':
          TwistD = atof(&argv[i][2]);  /* twist of the velocity selector channels [deg] */
          TwistR = Radians(TwistD);
          break;

        case 'd':
          Spacer = atof(&argv[i][2]); /* blade thickness [cm] */
          break;

        case 'o':
          DistAxle = fabs(atof(&argv[i][2])); /* distance: center of beamline - axle of selector [cm] */
          break;                                
        case 'Y':
          AxlePosY = atof(&argv[i][2]); /* horizontal position of the axle [cm] */
          break;
        case 'Z':
          AxlePosZ = atof(&argv[i][2]); /* horizontal position of the axle [cm] */
          break;

        case 'p':
          bPassOutside = (short) atoi(&argv[i][2]);   /* TRUE: neutrons outside the rotor are kept */
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
  
  /* determine axle distance from axle position */
  if (AxlePosY != 0.0 || AxlePosZ != 0.0)
  {
    dist = sqrt(sq(AxlePosY) + sq(AxlePosZ));
    if (DistAxle != 0.0 && DistAxle != dist)
      Warning("Distance between axle and beamline not compatible with axle position, changed to value calculated from position");
    DistAxle = dist;
  }
  else
  { AxlePosZ = -DistAxle;
  }
  BeamDir = SelectorAngle(0.0, 0.0);

  if (WndWidth==0.0)
    Error("Width of the entrance window is zero");
  if (WndHeight==0.0)
    Error("Height of the entrance window is zero");

  if (DistAxle==0.0)
    Error("Distance: beamline - axle could not be determined");
  if (RadiusO==0.0)
    Error("Radius of the selector is zero");
  if (Length==0.0)
    Error("Length of the selector channels is zero");

  if (Freq==0.0)
    Warning("Frequency of the selector is zero");
  if (TwistD==0.0)
    Warning("Twist of the selector channels is zero");

  /* definition of mean window width, corresponding angle and angle with respect to blade thickness */
  ChnWidth = (2.0*M_PI*DistAxle/(double)nChannels) - 2.0*DistAxle*asin(Spacer/(2.0*DistAxle));
  ChnAngle = ChnWidth/DistAxle;
  BladeAng = 2.0*DistAxle*asin(Spacer/(2.0*DistAxle))/DistAxle;

  return;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the velocity selector geometry
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.nHolCyls = 1;
    stGeometry.pHolCyl  = calloc(stGeometry.nHolCyls, sizeof(VtHolCyl));
    stGeometry.pHolCyl[0].Radius      = RadiusO;
    stGeometry.pHolCyl[0].InnerRadius = RadiusI;
    stGeometry.pHolCyl[0].Length      = Length;
    stGeometry.pHolCyl[0].vCntr[0]    = Length/2.;
    stGeometry.pHolCyl[0].vCntr[1]    = AxlePosY;
    stGeometry.pHolCyl[0].vCntr[2]    = AxlePosZ;
    stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
    stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
    stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;

    if (WndWidth < 100.0 && WndHeight < 100.0)
    { stGeometry.nRectangles = 2;
      stGeometry.pRectangle  = calloc(stGeometry.nRectangles, sizeof(VtRectangle));
      stGeometry.pRectangle[0].Width    = WndWidth;
      stGeometry.pRectangle[0].Height   = WndHeight;
      stGeometry.pRectangle[0].vCntr[0] = 0.0;
      stGeometry.pRectangle[0].vCntr[1] = 0.0;
      stGeometry.pRectangle[0].vCntr[2] = 0.0;
      stGeometry.pRectangle[0].vNormal[0] = 1.0;
      stGeometry.pRectangle[0].vNormal[1] = 0.0;
      stGeometry.pRectangle[0].vNormal[2] = 0.0;
      stGeometry.pRectangle[1].Width    = WndWidth;
      stGeometry.pRectangle[1].Height   = WndHeight;
      stGeometry.pRectangle[1].vCntr[0] = Length;
      stGeometry.pRectangle[1].vCntr[1] = 0.0;
      stGeometry.pRectangle[1].vCntr[2] = 0.0;
      stGeometry.pRectangle[1].vNormal[0] = 1.0;
      stGeometry.pRectangle[1].vNormal[1] = 0.0;
      stGeometry.pRectangle[1].vNormal[2] = 0.0;
    }
  }    

  return;
}


/*******************************************************************************/
/** Calculates the orientation of the selector for a neutron position (x,y,z) **/
/*******************************************************************************/
double SelectorAngle(const double y, const double z)
{
  double Angle=0.0, Ysel=0.0, Zsel=0.0;

  Ysel = y - AxlePosY;
  Zsel = z - AxlePosZ;

  Angle = atan2(Ysel, Zsel);

  return Angle;
 }


/*******************************************************************************/
/** Determines in which channel the neutron is                                **/
/*******************************************************************************/
short DetChannel(int* pChan, const double NeutAng, const double RotAng)
{
  short  bInside=FALSE;                       // flag: neutron is within a channel
  int    k=0;                                 // channel, in which the neutron is
  double TrailingEdge=0.0, LeadingEdge=0.0,   // selector orientations of the blades confining the channel 
         NeutAngP=NeutAng;                      

  // shift Neutron angle to range [0, 2pi[
  if (NeutAngP < BeamDir - M_PI)
    NeutAngP += 2.0*M_PI;

  // determine nearest channel
  k = floor(nChannels * (NeutAngP - RotAng)/(2.0*M_PI));

  // check if neutron is inside the channel, i.e. within the range [TrailingEdge, LeadingEdge]
  TrailingEdge = RotAng + k*(ChnAngle + BladeAng) + BladeAng;
  LeadingEdge = TrailingEdge + ChnAngle;
  
  if (NeutAngP > TrailingEdge && NeutAngP < LeadingEdge)
  { *pChan  = k;
    bInside = TRUE;
  }
  else
  { *pChan = NO_CHANNEL;
  }

  return bInside;
}
