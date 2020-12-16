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
McCompID _eModule=MCN_MON2_DIV;

// Input parameters
char*  MonFileName = NULL;       // -O    [-]   Monitor output file containing intensity as a function of y- and z-position   
short  bProbactiv  = TRUE,       // -p    [-]   flag Display  : YES: Probability weight   NO: number of trajectories
       bExclusive  = FALSE;      // -e    [-]   flag Exclusion: YES: only neutrons meeting the monitor conditions are written   NO: all are written
long   nBinsY      = 1,          // -y    [-]   number of bins in horizontal direction
       nBinsZ      = 1;          // -z    [-]   number of bins in vertical direction
VtFormat2D eFormat = MATRIX;     // -F    [-]   file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double DivYmin     = 0.0,        // -w   [deg]  min. horizontal divergence to be monitored
       DivYmax     = 0.0,        // -W   [deg]  max. horizontal divergence to be monitored
       DivZmin     = 0.0,        // -h   [deg]  min. vertical divergence to be monitored
       DivZmax     = 0.0;        // -H   [deg]  max. vertical divergence to be monitored
double FiltLmbdMin =-1.0,        // -l   [Ang]  filter: lower bound value of the wavelength range
       FiltLmbdMax =-1.0,        // -L   [Ang]  filter: upper bound value of the wavelength range
       FiltYMin    =-1.0e10,     // -u   [cm]   filter: left edge position of the monitored area
       FiltYMax    = 1.0e10,     // -U   [cm]   filter: right edge position of the monitored area
       FiltZMin    =-1.0e10,     // -v   [cm]   filter: bottom position of the monitored area
       FiltZMax    = 1.0e10;     // -V   [cm]   filter: top position of the monitored area

// Variables determined from input parameters
FILE*  fMonitor     = NULL;

double BinPosY   [BINSIZE];           // edges of the bins of the first parameter 
double BinPosZ   [BINSIZE];           // edges of the bins of the second parameter 
double IntYZ     [BINSIZE][BINSIZE];  // intensity within a bin (in 2 dimensions) 
double IntYZError[BINSIZE][BINSIZE];  // standard deviation of this intensity 
long   nTrajYZ   [BINSIZE][BINSIZE];  // number of trajectories within a bin


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
  int	   iY=0, jZ=0;
  long	 i=0 ;
  double Divy  =0.0, 
         Divz  =0.0, 
         TotInt=0.0,  // Total intensitiy within monitor limits
         prob  =0.0;  // Intensitiy of a trajectory
   
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for (iY = 0; iY<nBinsY+1; iY++)
  {
    BinPosY[iY] = DivYmin + (DivYmax-DivYmin) * iY / (double)nBinsY;

    for(jZ = 0;jZ<(nBinsZ+1); jZ++)
	  {
	    BinPosZ       [jZ] = DivZmin + (DivZmax-DivZmin)  * jZ / (double) nBinsZ;
	    IntYZ     [iY][jZ] = 0.0;
	    IntYZError[iY][jZ] = 0.0;
	    nTrajYZ   [iY][jZ] = 0;
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

	    if (FiltLmbdMin >= 0. && InputNeutrons[i].Wavelength < FiltLmbdMin) continue;
	    if (FiltLmbdMax >= 0. && InputNeutrons[i].Wavelength > FiltLmbdMax) continue;
	    if (InputNeutrons[i].Position[1] < FiltYMin) continue;
	    if (InputNeutrons[i].Position[1] > FiltYMax) continue;
	    if (InputNeutrons[i].Position[2] < FiltZMin) continue;
	    if (InputNeutrons[i].Position[2] > FiltZMax) continue;

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

	    iY = (int)floor(nBinsY*(Divy-DivYmin)/(DivYmax-DivYmin));
	    jZ = (int)floor(nBinsZ*(Divz-DivZmin)/(DivZmax-DivZmin));
			
	    if (((iY>=0)&&(iY<nBinsY))&&((jZ>=0)&&(jZ<nBinsZ))) 
      {	
	      nTrajYZ[iY][jZ]++;
	      IntYZ  [iY][jZ]+= prob;
	      TotInt         += prob;
	      bRegistered=1;
	    }
	  
	    if ((bExclusive==1) && (bRegistered==1))
	      WriteNeutron(&(InputNeutrons[i]));
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteHeader2D(fMonitor, eFormat, "Intensity", bProbactiv,  nBinsY, "divergence Y [cm]", nBinsZ, "divergence Z [cm]");
  // WriteOutput2D(fMonitor, eFormat,              bProbactiv,  nBinsY, BinPosY,           nBinsZ, BinPosZ,  IntYZ, IntYZError, nTrajYZ);
  WriteOutput2D(fMonitor, eFormat,              bProbactiv,  nBinsY, BinPosY,  BINSIZE, nBinsZ, BinPosZ,  
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
	        nBinsY = atol(&argv[i][2]); /* number of bins horizontal axis */
	        if (nBinsY > BINSIZE)
	          {fprintf(LogFilePtr,"ERROR: number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nBinsZ = atol(&argv[i][2]); /* number of bins vertical axis */
	        if (nBinsZ > BINSIZE)
	          {fprintf(LogFilePtr,"ERROR: number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;

	      case 'w':
	        DivYmin = atof(&argv[i][2]);		/* left edge position window */
	        break;
	      case 'W':
	        DivYmax = atof(&argv[i][2]);		/* right edge position window */
	        break;
	      case 'h':
	        DivZmin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'H':
	        DivZmax =  atof(&argv[i][2]);   /* top position window */
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
          FiltLmbdMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'L':
          FiltLmbdMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'u':
          FiltYMin = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'U':
          FiltYMax = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'v':
          FiltZMin = atof(&argv[i][2]);   /* filter Z */
          break;
        case 'V':
          FiltZMax = atof(&argv[i][2]);   /* filter Z */
          break;

	      case 'F':
          eFormat = atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz */
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
