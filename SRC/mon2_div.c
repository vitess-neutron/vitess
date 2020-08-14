/********************************************************************************************/
/*  VITESS module 'mon2_div.c'                                                              */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added wavelength and yz position filter                    */
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
McCompID _eModule=MCN_MON2_DIV;

FILE*  fMonitor     = NULL;
char*  MonFileName  = NULL;
short  bProbactiv   = TRUE,
       bExclusive   = FALSE; 
long   nbiny        = 0, 
       nbinz        = 0,
       format       = 0;
double DivYmin      = 0.0, 
       DivYmax      = 0.0, 
       DivZmin      = 0.0, 
       DivZmax      = 0.0,
       filtLambdaMin=-1.0,          /* filter      */
       filtLambdaMax=-1.0,
       filtYMin     =-1.0e10,
       filtYMax     = 1.0e10,
       filtZMin     =-1.0e10,
       filtZMax     = 1.0e10;
static 
double bdivz[BINSIZE],
       bdivy[BINSIZE];


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);    // Reads input parameters and sets global parameters


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  int	   dy=0, dz=0;
  long	 i=0 ;
  double Divy =0.0, 
         Divz =0.0, 
         bintc=0.0,
         prob =0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // New pointers allowing for global write out
  by = bdivy;
  bz = bdivz;

  // initializes arrays
  for (dy = 0; dy<nbiny+1; dy++)
  {
    bdivy[dy] = DivYmin + (DivYmax-DivYmin) * dy / (double)nbiny;

    for(dz = 0;dz<(nbinz+1); dz++)
	  {
	    bdivz[dz] = DivZmin + (DivZmax-DivZmin)  * dz / (double) nbinz;
	    binyz      [dy][dz]=0.0;
	    binyzerror [dy][dz]=0.0;
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
      CHECK;
	    bRegistered = 0;

	    if (bExclusive==0) 
		    WriteNeutron(&(InputNeutrons[i]));

	    if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	    if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	    if (InputNeutrons[i].Position[1] < filtYMin) continue;
	    if (InputNeutrons[i].Position[1] > filtYMax) continue;
	    if (InputNeutrons[i].Position[2] < filtZMin) continue;
	    if (InputNeutrons[i].Position[2] > filtZMax) continue;

	    if (bProbactiv==TRUE) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob = 1.0;

	    if (InputNeutrons[i].Vector[0] >= 0) 
        Divy = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	    else 
        Divy = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	    Divy*=180.0/M_PI;

	    Divz = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));
	    Divz*= 180.0/M_PI;

	    dy = (int)floor(nbiny*(Divy-DivYmin)/(DivYmax-DivYmin));
	    dz = (int)floor(nbinz*(Divz-DivZmin)/(DivZmax-DivZmin));
			
	    if (((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz))) 
      {	
	        binyz[dy][dz] = binyz[dy][dz] + prob ;
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
  WriteOutput (fMonitor, format, bProbactiv, nbiny, nbinz, "divergence Y [deg]", "divergence Z [deg]");

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

	      case 'h':
	        DivZmin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'w':
	        DivYmin = atof(&argv[i][2]);		/* left edge position window */
	        break;
	      case 'H':
	        DivZmax =  atof(&argv[i][2]);   /* top position window */
	        break;
	      case 'W':
	        DivYmax = atof(&argv[i][2]);		/* right edge position window */
	        break;

	      case 'p':
	        bProbactiv = atof(&argv[i][2]);
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
        case 'u':
          filtYMin = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'U':
          filtYMax = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'v':
          filtZMin = atof(&argv[i][2]);   /* filter Z */
          break;
        case 'V':
          filtZMax = atof(&argv[i][2]);   /* filter Z */
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
