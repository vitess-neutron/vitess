/********************************************************************************************/
/*  VITESS module 'mon2_tofwl.c'                                                            */
/*                                                                                          */
/* vertical: lambda, horizontal: tof                                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      xyz output                                                 */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_MON2_TOFWL;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE; 
long   nbiny      = 0, 
       nbinz      = 0,
       format     = 0;
double widthmin   = 0.0, 
       widthmax   = 0.0, 
       heightmin  = 0.0, 
       heightmax  = 0.0;
static 
double bposz[BINSIZE],
       bposy[BINSIZE];


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=0;
  int	   dy=0, dz=0;
  long	 i=0;
  double prob = 0.0, 
         bintc= 0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  //New pointers allowing for global write out
  by = bposy;
  bz = bposz;

  // initializes arrays
  for(dy = 0; dy<nbiny+1; dy++)
  {
    bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

    for(dz = 0;dz<(nbinz+1); dz++)
    {
	    bposz[dz] = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
	    binyz[dy][dz] = 0.0;
	    binyzerror[dy][dz]=0.;
	    binyzcounts[dy][dz]=0;
    }
  }

  DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
	  {
	    bRegistered=0;
      CHECK;
	    if (bProbactiv==1.0) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob=1.0;

	    dy = (int)floor(nbiny*(InputNeutrons[i].Time-widthmin)/(widthmax-widthmin));
	    dz = (int)floor(nbinz*(InputNeutrons[i].Wavelength-heightmin)/(heightmax-heightmin));
			
  	  if (((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	    {	
	      binyz[dy][dz] = binyz[dy][dz] + prob ;
	      bintc         = bintc + prob;
	      bRegistered=1;
	      binyzcounts[dy][dz]++;
	    }
	  
  	  if ((bExclusive==0)||(bRegistered==1))
	      WriteNeutron(&(InputNeutrons[i]));
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteOutput (fMonitor, format, bProbactiv, nbiny, nbinz, "tof [ms]", "wavelength [A]");

  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
	    switch(argv[i][1])
	    {
	      case 'O':
	        MonFileName=&argv[i][2];
	        break;

	      case 'y':
	        nbiny = atol(&argv[i][2]); /* number of bins horizontal axis */
	        if(nbiny>BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nbinz = atol(&argv[i][2]); /* number of bins vertical axis */
	        if(nbinz>BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

	      case 'm':
	        heightmin =  atof(&argv[i][2]);   /* minimal wavelength [A]*/
	        break;
	      case 'w':
	        widthmin = atof(&argv[i][2]);		/* minimal tof-value [ms]*/
	        break;
	      case 'M':
	        heightmax =  atof(&argv[i][2]);   /* maximal wavelength [A]*/
	        break;
	      case 'W':
	        widthmax = atof(&argv[i][2]);		/* maximal tof-value [ms]*/
	        break;

	      case 'p':
	        bProbactiv = atof(&argv[i][2]);
	        /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;

	      case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	        break;

	      case 'F':
            format = atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz */
            break;

	      default:
	        fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	        exit(-1);
	        break;
      }
    }
  }

  // opens monitor file
  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"\n you must define a MonitorOutputFile");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}
