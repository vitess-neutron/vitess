/********************************************************************************************/
/*  VITESS module 'mon2_rdiv.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  JAN 2010  A. Houben (idea by W. Schweika)                                           */
/* 1.0a JAN 2010  A. Houben      xyz output                                                 */
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
McCompID _eModule=MCN_MON2_RDIV;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE; 
long   nbiny  = 0, 
       nbinz  = 0,
       format = 0;
double rmin  =0.0, 
       rmax  =0.0, 
       phimin=0.0, 
       phimax=0.0,
       filtLambdaMin=-1.0,          /* filter      */
       filtLambdaMax=-1.0,
       filtYMin     =-1.0e10,
       filtYMax     = 1.0e10,
       filtZMin     =-1.0e10,
       filtZMax     = 1.0e10;
static 
double bphi   [BINSIZE],
       bradius[BINSIZE];


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
  int	   dy=0,
         dz=0;
  long	 i=0 ;
  double radius=0.0, 
         phi  =0.0,
         prob =0.0, 
         bintc=0.0;
  VectorType xvec = {1, 0, 0}, kvec;
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  //New pointers allowing for global write out
  by = bradius;
  bz = bphi;

  // initializes arrays
  for(dy = 0; dy<nbiny+1; dy++)
  {
    bradius[dy] = rmin + (rmax-rmin) * dy / (double)nbiny;

    for(dz = 0;dz<(nbinz+1); dz++)
	  {
	    bphi[dz] = phimin + (phimax-phimin)  * dz / (double) nbinz;
	    binyz      [dy][dz] = 0.0;
	    binyzerror [dy][dz] = 0.;
	    binyzcounts[dy][dz] = 0;
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
	    bRegistered=0;

	    if(bExclusive==0)
		    WriteNeutron(&(InputNeutrons[i]));

	    if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	    if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	    if (InputNeutrons[i].Position[1] < filtYMin) continue;
	    if (InputNeutrons[i].Position[1] > filtYMax) continue;
	    if (InputNeutrons[i].Position[2] < filtZMin) continue;
	    if (InputNeutrons[i].Position[2] > filtZMax) continue;

	    if(bProbactiv==1.0) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob=1.0;

	    radius = sqrt(sq(InputNeutrons[i].Position[1])+sq(InputNeutrons[i].Position[2]));
	    CopyVector(InputNeutrons[i].Vector, kvec);
	    NormVector(kvec);
	    phi = acos(ScalarProduct(xvec, kvec))/M_PI*180.;

	    dy = (int)floor(nbiny*(radius-rmin)/(rmax-rmin));
	    dz = (int)floor(nbinz*(phi-phimin)/(phimax-phimin));
			
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
  WriteOutput (fMonitor, format, bProbactiv, nbiny, nbinz, "radius [cm]", "divergence radius [deg]");

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
	        if (nbiny > BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nbinz = atol(&argv[i][2]); /* number of bins vertical axis */
	        if (nbinz > BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

	      case 'h':
	        phimin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'w':
	        rmin = atof(&argv[i][2]);		/* left edge position window */
	        break;
	      case 'H':
	        phimax =  atof(&argv[i][2]);   /* top position window */
	        break;
	      case 'W':
	        rmax = atof(&argv[i][2]);		/* right edge position window */
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