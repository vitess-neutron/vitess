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
/* 1.3a Nov 2020  K. Lieutenant  new 'mon2_header'                                          */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "init.h"
#include "softabort.h"
#include "general.h"
#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName= NULL;      // -O    [-]    Monitor output file containing intensity as a function of y- and z-position  
short  bProbactiv = TRUE,      // -p    [-]    flag Display  : YES: Probability weight   NO: number of trajectories
       bExclusive = FALSE;     // -e    [-]    flag Exclusion: YES: only neutrons meeting the monitor conditions are written 
long   nbiny      = 1,         // -y    [-]    number of bins in horizontal direction (TOF)
       nbinz      = 1,         // -z    [-]    number of bins in vertical direction   (lambda)
       format     = MATRIX;    // -F    [-]    file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double TofMin     = 0.0,       // -w   [ms]    min. time of flight to be monitored
       TofMax     = 0.0,       // -W   [ms]    max. time of flight to be monitored
       LambdaMin  = 0.0,       // -m   [Ang]   min. wavelength to be monitored
       LambdaMax  = 0.0;       // -M   [Ang]   max. wavelength to be monitored

// Variables determined from input parameters
FILE*  fMonitor   = NULL;

double BinPosY   [BINSIZE];           // edges of the bins of the first parameter 
double BinPosZ   [BINSIZE];           // edges of the bins of the second parameter 
double IntYZ     [BINSIZE][BINSIZE];  // intensity within a bin (in 2 dimensions) 
double IntYZError[BINSIZE][BINSIZE];  // standard deviation of this intensity 
long   nTrajYZ   [BINSIZE][BINSIZE];  // number of trajectories within a bin


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);    // Reads input parameters and sets global variables


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
  _eModule=MCN_MON2_TOFWL;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for(dy=0; dy<nbiny+1; dy++)
  {
    BinPosY[dy] = TofMin + (TofMax-TofMin) * dy / (double)nbiny;

    for(dz=0; dz < (nbinz+1); dz++)
    {
	    BinPosZ       [dz] = LambdaMin + (LambdaMax-LambdaMin)  * dz / (double) nbinz;
	    IntYZ     [dy][dz] = 0.0;
	    IntYZError[dy][dz] = 0.0;
	    nTrajYZ   [dy][dz] = 0;
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

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        bRegistered=0;
	      if (bProbactiv==1.0) 
          prob = InputNeutrons[i].Probability;
	      else 
          prob=1.0;

	      dy = (int)floor(nbiny*(InputNeutrons[i].Time-TofMin)/(TofMax-TofMin));
	      dz = (int)floor(nbinz*(InputNeutrons[i].Wavelength-LambdaMin)/(LambdaMax-LambdaMin));
			
  	    if (((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	      {	
	        nTrajYZ[dy][dz]++;
	        IntYZ  [dy][dz]+= prob;
	        bintc          += prob;
	        bRegistered=1;
	      }
	  
  	    if ((bExclusive==0)||(bRegistered==1))
	        WriteNeutron(&(InputNeutrons[i]));
      }
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteHeader2D(fMonitor, format, "Intensity", bProbactiv,  nbiny, "tof [ms]", nbinz, "wavelength [A]");
  // WriteOutput2D(fMonitor, format,           bProbactiv,  nbiny, BinPosY,           nbinz, BinPosZ,  IntYZ, IntYZError, nTrajYZ);
  WriteOutput2D(fMonitor, format,              bProbactiv,  nbiny, BinPosY, BINSIZE,  nbinz, BinPosZ,  
                         (double*)IntYZ, (double*)IntYZError, (long*)nTrajYZ);
  fclose(fMonitor);

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
	          {fprintf(LogFilePtr,"ERROR: number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nbinz = atol(&argv[i][2]); /* number of bins vertical axis */
	        if(nbinz>BINSIZE)
	          {fprintf(LogFilePtr,"ERROR: number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;

	      case 'w':
	        TofMin = atof(&argv[i][2]);		/* minimal tof-value [ms]*/
	        break;
	      case 'W':
	        TofMax = atof(&argv[i][2]);		/* maximal tof-value [ms]*/
	        break;
	      case 'm':
	        LambdaMin =  atof(&argv[i][2]);   /* minimal wavelength [A]*/
	        break;
	      case 'M':
	        LambdaMax =  atof(&argv[i][2]);   /* maximal wavelength [A]*/
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
	        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
	        exit(-1);
      }
    }
  }

  // opens monitor file
  if (MonFileName==NULL)
  {
    Error("you must define a MonitorOutputFile");
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}
