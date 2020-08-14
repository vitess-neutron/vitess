/********************************************************************************************/
/*  VITESS module monitor2                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Sep 1999  D. Wechsler                                                               */
/* 1.1  JUL 2002  G. Zsigmond    reorganized                                                */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added wavelength filter                                    */
/* 1.2b JAN 2010  A. Houben      xyz output                                                 */
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
McCompID _eModule=MCN_MON2_POS;

FILE*    fMonitor   = NULL;
char*    MonFileName= NULL;
short    bProbactiv = TRUE,
         bExclusive = FALSE; 
long     nbiny      = 0, 
         nbinz      = 0,
         format     = 0;
double   widthmin   = 0.0,
         widthmax   = 0.0,
         heightmin  = 0.0,
         heightmax  = 0.0;
double   filtLambdaMin=-1.0,          /* filter      */
         filtLambdaMax=-1.0;
static 
double   bposz[BINSIZE],
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
  int	   dy=0,dz=0;
  long 	 i=0;
  double prob  = 0.0,
         bintc = 0.0;

  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // New pointers allowing for global write out
  by = bposy;
  bz = bposz;

  // initializes arrays
  for(dy = 0; dy<nbiny+1; dy++)
  {
    bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

    for(dz = 0;dz<(nbinz+1); dz++)
	  {
	    bposz[dz]         = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
	    binyz[dy][dz]     = 0.0;
	    binyzerror[dy][dz]= 0.0;
	    binyzcounts[dy][dz]=0;
	  }
  }

  DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
  while(ReadNeutrons()!= 0)
  {
	  CHECK;      
	  for(i=0; i<NumNeutGot; i++)
	  {
	    CHECK;
	    bRegistered=0;

	    if (bExclusive==0) 
		    WriteNeutron(&(InputNeutrons[i]));

	    if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	    if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;

	    if (bProbactiv==TRUE) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob = 1.0;

	    dy = (int)floor(nbiny*(InputNeutrons[i].Position[1]-widthmin)/(widthmax-widthmin));
	    dz = (int)floor(nbinz*(InputNeutrons[i].Position[2]-heightmin)/(heightmax-heightmin));
			
	    if (((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	    {	
	        binyz[dy][dz] = binyz[dy][dz] +  prob ;
	        bintc = bintc + prob;
	        bRegistered=1;
	        binyzcounts[dy][dz]++;
	    }
	  
	    if ((bExclusive==1) && (bRegistered==1)) 
	      WriteNeutron(&(InputNeutrons[i]));
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteOutput (fMonitor, format, bProbactiv, nbiny, nbinz, "Y [cm]", "Z [cm]");

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
	        nbiny = atol(&argv[i][2]); /* number of bins y-direction */
	        if(nbiny>BINSIZE)
	          {fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

	      case 'z':
	        nbinz = atol(&argv[i][2]); /* number of bins, z-direction */
	        if(nbinz>BINSIZE)
	          {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

	      case 'h':
	        heightmin =  atof(&argv[i][2]);   /* bottom position window   [cm]*/
	        break;
	      case 'w':
	        widthmin = atof(&argv[i][2]);		/* left edge position window   [cm]*/
	        break;

	      case 'H':
	        heightmax =  atof(&argv[i][2]);   /* top position window   [cm]*/
	        break;
	      case 'W':
	        widthmax = atof(&argv[i][2]);		/* right edge position window    [cm]*/
	        break;

	      case 'p':
	        bProbactiv = atoi(&argv[i][2]);
	        /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;

	      case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	        break;
	  
	      case 'l':
            filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
            break;

          case 'L':
            filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
            break;

	      case 'F':
            format = atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz */
            break;
	    }
    }
  }

  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"\nERROR:  you must define a MonitorOutputFile");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1) 
    bProbactiv = 0;
}