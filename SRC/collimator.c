/********************************************************************************************/
/*  VITESS module collimator                                                                */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 0.9    Sep 2007  K. Lieutenant  unsused version (developed from guide.c)                 */
/* 1.0    May 2008  K. Lieutenant  initial version (dedicated code)                         */
/********************************************************************************************/

#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit   (int argc, char *argv[]);
void  OwnCleanup();

int   DetermineChannel(double pos, double coll_min, double chan_dist);
short PathThroughCollGravOrder1(double* tof, Neutron* ThisNeutron, Plane exit_wall, long keygrav);


/******************************/
/** Global variables         **/
/******************************/

long   nChannels=1,        /* number of collimator channels                     */
  nSpacers =0;        /* number of partition walls separating the channels */

double CollEntrHeight=0.0,
  CollEntrWidth =0.0,
  CollExitHeight=0.0,
  CollExitWidth =0.0,
  ChanWin, ChanWout,  /* width of channel at entrance and exit             */
  Length   =0.0,      /* length of the guide collimator                    */
  Spacer   =0.0;


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  /********************************************************************************************/
  /* This module reads in a file of neutron structures, and defines a neutron guide as a set  */
  /* of five infinite planes with a global critical angle. It outputs the coordinates and time*/
  /* displacement of any neutrons that pass through the guide without being absorbed.			  */
  /*                                                                                          */
  /* Anything not directly commented is an InputNeutrons or an output routine.                */
  /********************************************************************************************/

  long   i;
  int    iChanIn, iChanOut;       /* channel where neutron enters and leaves                     */
  short  bReach;                  /* boolean: hits exit or not                                   */
  double ChanDistIn, ChanDistOut, /* distance between neighbouring channels at entrance and exit */
    ChanMinIn,  ChanMinOut,       /* minimal y-position for channel determination                */
    ToF;
  // double RotMatrix[3][3]={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  // char   sBuffer[512]="";

  Plane   CollExit;
  Neutron OutNeutron;


  /* Initialisation */
  Init(argc, argv, VT_COLLIMATOR);
  print_module_name("Collimator 1.0");
  OwnInit(argc, argv);

  ChanDistIn  = ChanWin  + Spacer;
  ChanDistOut = ChanWout + Spacer;
  ChanMinIn   = -(CollEntrWidth+Spacer)/2.0;
  ChanMinOut  = -(CollExitWidth+Spacer)/2.0;

  /* exit plane */
  CollExit.A =  1.0;
  CollExit.B =  0.0;
  CollExit.C =  0.0;
  CollExit.D = -Length;

  /* Writing to log file */
  fprintf(LogFilePtr, "\nLinear collimator of %ld channels\n", nChannels);
  fprintf(LogFilePtr, "length         : %6.3f m\n",  Length/100.);
  fprintf(LogFilePtr, "width x height : %6.3f x %6.3f cm²", CollEntrWidth, CollEntrHeight);
  if (CollExitWidth != CollEntrWidth || CollExitHeight != CollEntrHeight)
    fprintf(LogFilePtr, " -> %6.3f x %6.3f cm²", CollExitWidth, CollExitHeight);
  fprintf(LogFilePtr, "\n");

  DECLARE_ABORT;

  iChanOut = 0;
  
  while(ReadNeutrons()!= 0)
    {
      for(i=0; i<NumNeutGot; i++)
	{
	  CHECK;
	  
	  /*	InputNeutrons[i].Position.X = 0.0;   !!!!!!!! */
	  /****************************************************************************************/
	  /* Check to see if the neutron is initially in the entrance to the guide...             */
	  /****************************************************************************************/
	  if (fabs(InputNeutrons[i].Position[1]) > CollEntrWidth/2.0)  continue;
	  if (fabs(InputNeutrons[i].Position[2]) > CollEntrHeight/2.0) continue;
	  
	  OutNeutron = InputNeutrons[i];
	  
	  // find out the entrance channel   (channel = 0 means 'blade position')
	  iChanIn = DetermineChannel(OutNeutron.Position[1], ChanMinIn, ChanDistIn);
	  
	  if (iChanIn > 0)
	    {	
	      /****************************************************************************************/
	      /* Pass a pointer to the neutron and the collimator structure to a subroutine to        */ 
	      /* calculate the propagation inside  collimator frame                                   */
	      /* Check if neutron leaves inside the exit area and determine the channel               */
	      /****************************************************************************************/
	      bReach = PathThroughCollGravOrder1(&ToF, &OutNeutron, CollExit, keygrav);
	      
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

	      /********************************************************************************/
	      /* Writeout new data set, if neutron enters and leaves through the same channel */
	      /********************************************************************************/
	      if (iChanIn==iChanOut)
		{
		  OutNeutron.Position[0]=0.0;
		  OutNeutron.Time += ToF;

		  WriteNeutron(&OutNeutron);
		}
	    } 
	}
    }

 my_exit:
  OwnCleanup();
  Cleanup(Length,0.0,0.0, 0.0, 0.0);

  return(0);
}


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit   (int argc, char *argv[])
{
  long  i;
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
	      Length    = atof(arg);     /* length of the collimator in cm */
	      break;
	    case 'n':
	      nChannels = atol(arg);     /* bender: No. of channels        */
	      nSpacers  = nChannels - 1;
	      break;
	    case 's':
	      Spacer    =  atof(arg);    /* width of bender channel border in cm */
	      break;

	    default:
	      fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
	      exit(-1);
	      break;
	    }
	}
    }

  ChanWin  = (CollEntrWidth - nSpacers*Spacer) / (double)nChannels;
  ChanWout = (CollExitWidth - nSpacers*Spacer) / (double)nChannels;

}


/* own cleanup of the guide module */
/* --------------------------------*/
void OwnCleanup()
{

  fprintf(LogFilePtr," \n");

  /* set description for instrument plot */
  stPicture.dWPar   = CollEntrWidth;
  stPicture.dHPar   = CollExitWidth;
  stPicture.dRPar   = 0.0;
  stPicture.nNumber = nChannels;

}/* End OwnCleanup */



short
PathThroughCollGravOrder1(double* tof, Neutron* pThisNeutron, Plane exit_wall, long keygrav)
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


int DetermineChannel(double pos, double coll_min, double chan_dist)
{
  int    iChan=1;
  double channel;

  if (nChannels > 1)
    {
      channel  = (pos - coll_min)  / chan_dist;

      if (fabs(channel - Round(channel)) >= 0.5*Spacer/chan_dist)
	iChan = (int) ceil(channel);
      else
	iChan = 0;
    }
  else
    {	iChan = 1;
    }

  return iChan;
} 
