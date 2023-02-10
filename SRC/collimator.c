/********************************************************************************************/
/*  VITESS module 'collimator'                                                              */
/*                                                                                          */
/* This module simulates a rectangular Soller collimator with absorbing channels            */
/*   (exit width and height can differ from entrance width and height)                      */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 0.9   Sep 2007  K. Lieutenant  unsused version (developed from guide.c)                  */
/* 1.0   May 2008  K. Lieutenant  initial version (dedicated code)                          */
/* 1.1   Aug 2019  K. Lieutenant  visualisation included                                    */
/********************************************************************************************/

#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit         (int argc, char *argv[]);                                            // Reads input parameters and sets global parameters
short PathThroughColl (double* tof, Neutron* ThisNeutron, Plane exit_wall, long keygrav);  // calculates the flight time to the exit wall and the point of impact
void  SetGeometry     (char* sColor);                                                      // fills the structure stGeometry for visualization

int   DetermineChannel(double pos, double coll_min, double chan_dist);                     // determines the channel as a function of horizontal position              
int   NumChan         (int nChanTot, int iHull);                                           // calculates the number of channels inside the hull
int   NumBlds         (int nChanTot, int iHull);                                           // calculates the number of blades inside the hull


/******************************/
/** Global variables         **/
/******************************/
// Input parameters
double CollEntrWidth =0.0,      // -w  [cm]   collimator width and height at entrance
       CollEntrHeight=0.0,      // -h  [cm]   
       CollExitWidth =0.0,      // -W  [cm]   collimator width and height at exit
       CollExitHeight=0.0,      // -H  [cm]   
       Length        =0.0,      // -l  [cm]   length of the collimator       
       BladeWidth    =0.0;      // -s  [cm]   thickness of the blades separating the channels
long   nChannels=1;             // -n   [-]   number of collimator channels  

// Variables determined from input parameters or trajectory data
double ChanWin=0.0, ChanWout=0.0;       // width of channel at entrance and exit


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long    i=0;                             // index of trajectories
  int     iChanIn=0, iChanOut=0;           // channel where neutron enters and leaves    
  short   bReach=FALSE;                    // boolean: hits exit or not          
  double  ChanDistIn=0.0, ChanDistOut=0.0, // distance between neighbouring channels at entrance and exit
          ChanMinIn=0.0,  ChanMinOut=0.0,  // minimal y-position for channel determination    
          ToF=0.0;                         // time-of-flight from entrance to exit of the collimator 
  
  Plane   CollExit;                        // plane determined by the exit area of the the collimator
  Neutron OutNeutron;                      // trajectory written to the output (to be read by the next module)

  InitNeutron(&OutNeutron);

	// reading of input data and initialisation
  // ----------------------------------------
  _eModule=MCN_COLLIMATOR;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.1");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = TRUE;

  InitNeutron(&OutNeutron);
  ChanDistIn  =  ChanWin  + BladeWidth;
  ChanDistOut =  ChanWout + BladeWidth;
  ChanMinIn   = -(CollEntrWidth+BladeWidth)/2.0;
  ChanMinOut  = -(CollExitWidth+BladeWidth)/2.0;

  /* exit plane */
  CollExit.A =  1.0;
  CollExit.B =  0.0;
  CollExit.C =  0.0;
  CollExit.D = -Length;

  DECLARE_ABORT;
  
	// loop over trajectories
  // ----------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
	  
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // Check to see if the neutron is initially in the entrance to the collimator
        // --------------------------------------------------------------------------
        if (fabs(InputNeutrons[i].Position[1]) > CollEntrWidth/2.0)  continue;
        if (fabs(InputNeutrons[i].Position[2]) > CollEntrHeight/2.0) continue;
	  
        OutNeutron = InputNeutrons[i];
	  
        // find out the entrance channel   (channel = 0 means 'blade position')
        iChanIn = DetermineChannel(OutNeutron.Position[1], ChanMinIn, ChanDistIn);
	  
        if (iChanIn > 0)
        {	
          // Pass a pointer to the neutron and the collimator structure to a subroutine to
          // calculate the propagation inside  collimator frame                           
          // Check if neutron leaves inside the exit area and determine the channel       
          // ------------------------------------------------------------------------------
          bReach = PathThroughColl(&ToF, &OutNeutron, CollExit, keygrav);
	      
          if (bReach)
          {
            if (fabs(OutNeutron.Position[1]) <= CollExitWidth/2.0  &&  
                fabs(OutNeutron.Position[2]) <= CollExitHeight/2.0)
            {	
              // find out the exit channel   (channel = 0 means 'blade position')
              iChanOut = DetermineChannel(OutNeutron.Position[1], ChanMinOut, ChanDistOut);
            }
            else
            {	
              iChanOut = -1;
            }
          }

          // Writeout new data set, if neutron enters and leaves through the same channel */
          // ------------------------------------------------------------------------------
          if (iChanIn==iChanOut)
          {
            OutNeutron.Position[0]=0.0;
            OutNeutron.Time += ToF;

            WriteNeutron(&OutNeutron);
          }
        } 
      }
    }
  }

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
 my_exit:

  // Writing to log file
  fprintf(LogFilePtr, "\nLinear collimator of %ld channels\n", nChannels);
  fprintf(LogFilePtr, "length         : %6.3f m\n",  Length/100.);
  fprintf(LogFilePtr, "width x height : %6.3f x %6.3f cm^2", CollEntrWidth, CollEntrHeight);
  if (CollExitWidth != CollEntrWidth || CollExitHeight != CollEntrHeight)
    fprintf(LogFilePtr, " -> %6.3f x %6.3f cm^2", CollExitWidth, CollExitHeight);
	fprintf(LogFilePtr, "Channel width  : %6.3f -> %5.3f cm \n", ChanWin, ChanWout);
  fprintf(LogFilePtr, "\n");
   
  SetGeometry("blue");

  Cleanup(Length,0.0,0.0, 0.0,0.0);

  return(0);
}


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit   (int argc, char *argv[])
{
  long  i=0,
        nBlades =0;        // number of the blades separating the channels
  char  *arg=NULL;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
	    arg=&argv[i][2];
	    switch(argv[i][1])
	    {
	      case 'h':
	        CollEntrHeight = atof(arg);
	        break;
	      case 'H':
	        CollExitHeight = atof(arg);
	        break;
	      case 'w':
	        CollEntrWidth  = atof(arg);
	        break;
	      case 'W':
	        CollExitWidth  = atof(arg);
	        break;

	      case 'l':
	        Length    = atof(arg);     // length of the collimator [cm]
	        break;
	      case 'n':
	        nChannels = atol(arg);     // number of the collmator channels
	        nBlades  = nChannels - 1;
	        break;
	      case 's':
	        BladeWidth =  atof(arg);   // thickness of the blades separating the channels
	        break;

	      default:
	        fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
	        exit(-1);
	        break;
	    }
    }
  }

  ChanWin  = (CollEntrWidth - nBlades*BladeWidth) / (double)nChannels;
  ChanWout = (CollExitWidth - nBlades*BladeWidth) / (double)nChannels;
}


/***********************************************************************************/
/* PathThroughColl:                                                                */
/*  calculates the flight time to exit wall and the point of impact                */
/***********************************************************************************/
short PathThroughColl(double* tof, Neutron* pThisNeutron, Plane exit_wall, long keygrav)
{
  Neutron TempNeutron;

  /***********************************************************************************/
  /* This routine calculates the trajectory a neutron follows through a simple       */
  /* collimator.                                                                     */
  /***********************************************************************************/

  TempNeutron = *pThisNeutron;

  if (keygrav == 1)
    *tof = NeutronPlaneIntersectionGrav(&TempNeutron, exit_wall);
  else
    *tof = NeutronPlaneIntersection1   (&TempNeutron, exit_wall);


  if(pThisNeutron->Vector[0] > 0.0)
    {
      pThisNeutron->Position[0] = TempNeutron.Position[0];
      pThisNeutron->Position[1] = TempNeutron.Position[1];
      pThisNeutron->Position[2] = TempNeutron.Position[2];

      pThisNeutron->Vector[2]   = TempNeutron.Vector[2]; // Vector[0] and Vector[1] cannot change

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}


/***********************************************************************************/
/* SetGeometry:                                                                    */
/*  fills the structure stGeometry for visualization                               */
/***********************************************************************************/
void SetGeometry(char* sColor)
{
 // Geometry data
  if (bVisInstr)
  {
    int i;   // index of hulls 

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.nHulls  = nChannels; 
    stGeometry.pHull   = (VtHull*) calloc(nChannels, sizeof(VtHull));

    for (i=1; i <= stGeometry.nHulls; i++)
    { 
      stGeometry.pHull[i-1].WidthIn   = BlowUp * (NumChan(i,nChannels)*ChanWin  + NumBlds(i,nChannels)*BladeWidth);
      stGeometry.pHull[i-1].WidthOut  = BlowUp * (NumChan(i,nChannels)*ChanWout + NumBlds(i,nChannels)*BladeWidth);
      stGeometry.pHull[i-1].HeightIn  = BlowUp * CollEntrHeight;
      stGeometry.pHull[i-1].HeightOut = BlowUp * CollExitHeight;
      stGeometry.pHull[i-1].Length    =     Length;
      stGeometry.pHull[i-1].vCntr[0]  = 0.5*Length;
      stGeometry.pHull[i-1].vCntr[1]  = 0.0;
      stGeometry.pHull[i-1].vCntr[2]  = 0.0;
      stGeometry.pHull[i-1].vNormal[0]= 1.0;
      stGeometry.pHull[i-1].vNormal[1]= 0.0;
      stGeometry.pHull[i-1].vNormal[2]= 0.0;
      stGeometry.pHull[i-1].rotAngle  = 0.0;
    }
  }
}


/***********************************************************************************/
/* DetermineChannel:                                                               */
/*  determines the channel as a function of horizontal position                    */
/***********************************************************************************/
int DetermineChannel(double pos, double coll_min, double chan_dist)
{
  int    iChan=1;
  double channel;

  if (nChannels > 1)
    {
      channel  = (pos - coll_min)  / chan_dist;

      if (fabs(channel - Round(channel)) >= 0.5*BladeWidth/chan_dist)
        iChan = (int) ceil(channel);
      else
        iChan = 0;
    }
  else
    {	iChan = 1;
    }

  return iChan;
} 


/***********************************************************************************/
/* NumChan, NumSpcr:                                                               */
/*  calculates the number of channels/blades inside the hull (for visualization)   */
/***********************************************************************************/
int NumChan(int nChanTot, int iHull)
{
  int nChanHull;   // number of channels included in the hull
 
  if (2*(nChanTot/2)==nChanTot)  
  { // for an even number of (the total number of) channels
    nChanHull = 2*(iHull/2);      
  }
  else
  { // for an odd number of channels
    nChanHull = 2*((iHull+1)/2) - 1;      
  }
  return(nChanHull);
}


int NumBlds(int nChanTot, int iHull)
{
  int nBldsHull;   // number of spacers included in the hull
 
  if (2*(nChanTot/2)==nChanTot)  
  { // for an even number of channels
    nBldsHull = 2*((iHull+1)/2) - 1;      
  }
  else
  { // for an odd number of channels
    nBldsHull = 2*(iHull/2);      
  }
  return(nBldsHull);
}

