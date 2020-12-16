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
/********************************************************************************************/

#include "general.h"
#include "init.h"
#include "softabort.h"

#include <stdlib.h>
#include <math.h>

/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and initializes global variables
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
McCompID _eModule=MCN_VEL_SELECT;

// Input parameters
double  Radius   = 0.0,    // -r  [cm]   radius of the velocity selector 
        Length   = 0.0,    // -l  [cm]   length of the velocity selector 
        Spacer   = 0.0,    // -d  [cm]   blade thickness   
        Freq     = 0.0,    // -s  [Hz]   number of velocity selector rotations per second
        Twist    = 0.0,    // -c  [deg]  twist of the velocity selector channels
        DistAxle = 0.0;    // -o  [cm]   distance origin - axle of the velocity selector
long    nChannels= 1;      // -w   [-]   number of selector channels
                                    
// Variables determined from input parameters or trajectory data
double* pAngIn;	            //            array: orientation of the blades
double  BladeAng=0.0,       //     [rad]  angular width of a blade at the origin
        WndWidth=0.0,       //     [cm]   inner width of a window at the origin
        WndAng=0.0,         //     [rad]  angular inner width of a window
        nRot =0.0,          //    [1/ms]  number of velocity selector rotations per millisecond
        Curve=0.0;          //     [rad]  twist of the velocity selector channels


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long    i, n;
  double  globalz, neutAng, Velocity, Rotang, 
          TrailingEdge, LeadingEdge, ToF, deltaRot;
  Neutron Output;
 
  // initialisation
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
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
      Velocity = V_FROM_LAMBDA(InputNeutrons[i].Wavelength);
      globalz = DistAxle + InputNeutrons[i].Position[2];  /* Distance axle velsel. neutron (z-direction)*/
      if (globalz < 0.0) Error("wrong geometry: neutron was found below axle");
  
      /* calculation  of  angle: z-axis; center velsel.; neutron position */
      neutAng = atan(InputNeutrons[i].Position[1]/globalz);
      if (fabs(neutAng) > M_PI) Error("internal geometry wrong");

      /* Rotation angle of velsel. corresponding to neutron time */
      Rotang = 2.0*M_PI*nRot*InputNeutrons[i].Time;
  
      /* all angles between -PI and PI; 0 corresponds to z-axis */
      while (Rotang >= M_PI) Rotang-=2.0*M_PI;
      while (Rotang < -M_PI) Rotang+=2.0*M_PI;
  
      /* Loop over all windows of the velsel. */
      for(n=0; n<nChannels;n++)
      {
        CHECK
  
        /* angle region of window n [TrailingEdge;LeadingEdge] */
        TrailingEdge = Rotang + pAngIn[n] + BladeAng;
        while (TrailingEdge >= M_PI) TrailingEdge-=2.0*M_PI;
        LeadingEdge = TrailingEdge + WndAng;
  
        if ((neutAng >TrailingEdge)&&(neutAng<LeadingEdge))
        {	/* caclculation if exit window is hit */
  
          /* time for passing the velsel. */
          ToF= Length/(Velocity*InputNeutrons[i].Vector[0]);
  
          globalz = DistAxle + InputNeutrons[i].Position[2];
  
          /* old coordinates and test if neutron hits the velselect front*/
          if ((globalz*globalz +InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1]) > Radius*Radius)
            goto Getnewneutron;
  
          /* new coordinates and new neutron angle and test if cylinder walls absorbed the neutron*/
          InputNeutrons[i].Position[0] += Velocity*ToF*InputNeutrons[i].Vector[0];
          InputNeutrons[i].Position[1] += Velocity*ToF*InputNeutrons[i].Vector[1];
          InputNeutrons[i].Position[2] += Velocity*ToF*InputNeutrons[i].Vector[2];
  
          InputNeutrons[i].Time+=ToF;
          globalz = DistAxle + InputNeutrons[i].Position[2];
  
          if ((globalz*globalz +InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1])> Radius*Radius)
            goto Getnewneutron;
  
          neutAng = atan(InputNeutrons[i].Position[1]/globalz);
          if (fabs(neutAng) > M_PI) Error("internal geometry wrong");
  
          /* update TrailingEdge and Leading Edge for the channel under consideration*/
          deltaRot = (ToF*2.0*M_PI*nRot) - Curve;
  
          if (fabs(deltaRot) >= M_PI) break;
  
          TrailingEdge+=deltaRot;
          while (TrailingEdge >= M_PI)  TrailingEdge-=2.0*M_PI;
          while (TrailingEdge <= -M_PI) TrailingEdge+=2.0*M_PI;
          LeadingEdge= WndAng+TrailingEdge;
  
          if ((neutAng >TrailingEdge)&&(neutAng<LeadingEdge)) 
            goto Transmission;
          else 
            goto Getnewneutron;
        }
        /* loop over windows continued */
      }

    Getnewneutron: 
      continue; /* case of neutron blocked by spacers or absorbed within a channel */
  
    Transmission:
      InputNeutrons[i].Position[0]=0.0;
  
      Output = InputNeutrons[i];
  
      WriteNeutron(&Output);
    }
  }
  
// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
  if (pAngIn!=NULL)
    free(pAngIn);
  
  SetGeometry("grey");                       // write geometry data for visualization
  Cleanup(Length,0.0,0.0, 0.0,0.0);          // print intensity, write instrument.inf, free memory
  
  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int i=0, k=0;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 's':
          Freq = atof(&argv[i][2]);   /* number of velsel. rotations per second */
          nRot = Freq/1000.0;         /* conversion to ms */
          break;

        case 'l':
          Length = atof(&argv[i][2]); /* length of velocity selector [cm] */
          break;

        case 'w':
          nChannels = atol(&argv[i][2]); /* number of windows */
          break;

        case 'd':
          Spacer = atof(&argv[i][2]); /* blade thickness [cm] */
          break;

        case 'c':
          Twist = atof(&argv[i][2]);  /* twist of the velocity selector channels [deg] */
          Curve = Radians(Twist);
          break;

        case 'r':
          Radius = atof(&argv[i][2]);    /* radius of the velocity selector [cm] */
          break;

        case 'o':
          DistAxle = fabs(atof(&argv[i][2])); /* distance origin - axle of velsel. rotations [cm] */
          break;                                /* origin = center of end of guide */

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
  
  /* definition of mean window width, corresponding angle and angle with respect to blade thickness */
  WndWidth = (2.0*M_PI*DistAxle/(double)nChannels) - 2.0*DistAxle*asin(Spacer/(2.0*DistAxle));
  WndAng   = WndWidth/DistAxle;
  BladeAng = 2.0*DistAxle*asin(Spacer/(2.0*DistAxle))/DistAxle;
  
  /* definition of window "coordinates" */
  pAngIn = calloc(nChannels, sizeof(double));
  
  for (k=0; k<nChannels; k++)
  {	
    pAngIn[k] = k*(WndAng + BladeAng);
  }

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

    stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
    stGeometry.nCylinders = 1;
       
    stGeometry.pCylinder[0].Radius = Radius;
    stGeometry.pCylinder[0].Length = Length;
    stGeometry.pCylinder[0].vCntr[0]  = stGeometry.pCylinder[0].Length/2.;
    stGeometry.pCylinder[0].vCntr[1]  = 0;
    stGeometry.pCylinder[0].vCntr[2]  = DistAxle;
    stGeometry.pCylinder[0].vSymAxis[0] = 1.;
    stGeometry.pCylinder[0].vSymAxis[1] = 0.;
    stGeometry.pCylinder[0].vSymAxis[2] = 0.;
  }    

  return;
}
