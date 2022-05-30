/********************************************************************************************/
/*  VITESS module 'mon2_rdiv.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  JAN 2010  A. Houben (idea by W. Schweika)                                           */
/* 1.0a JAN 2010  A. Houben      xyz output                                                 */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.3a Nov 2020  K. Lieutenant  new 'mon2_header'                                          */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "mon2_header.h"

/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName  = NULL;      // -O    [-]   Monitor output file containing intensity as a function of radius and radial divergence   
short  bProbactiv   = TRUE,      // -p    [-]   flag Display  : YES: Probability weight   NO: number of trajectories
       bExclusive   = FALSE;     // -e    [-]   flag Exclusion: YES: only neutrons meeting the monitor conditions are written   NO: all are written
long   nBinsY        = 1,         // -y    [-]   number of bins in radius
       nBinsZ        = 1,         // -z    [-]   number of bins in radial divergence
       format       = MATRIX;    // -F    [-]   file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double rmin         = 0.0,       // -w   [cm]   min. radius to be monitored
       rmax         = 0.0,       // -W   [cm]   max. radius to be monitored
       phimin       = 0.0,       // -h   [deg]  min. radial divergence to be monitored
       phimax       = 0.0,       // -H   [deg]  max. radial divergence to be monitored
       filtLambdaMin=-1.0,       // -l   [Ang]  filter: lower bound value of the wavelength range 
       filtLambdaMax=-1.0,       // -L   [Ang]  filter: upper bound value of the wavelength range
       filtYMin     =-1.0e10,    // -u   [cm]   filter: left edge position of the monitored area
       filtYMax     = 1.0e10,    // -U   [cm]   filter: right edge position of the monitored area
       filtZMin     =-1.0e10,    // -v   [cm]   filter: bottom position of the monitored area
       filtZMax     = 1.0e10;    // -V   [cm]   filter: top position of the monitored area


// Variables determined from input parameters
FILE*  fMonitor   = NULL;

double*  BinPosY   = NULL;       //             edges of the bins of the first parameter 
double*  BinPosZ   = NULL;       //             edges of the bins of the second parameter 
double** IntYZ     = NULL;       //             intensity within a bin (in 2 dimensions) 
double** IntYZError= NULL;       //             standard deviation of this intensity 
long  ** nTrajYZ   = NULL;       //             number of trajectories within a bin


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=0;
  int	   iR=0,
         jPhi=0;
  long	 i=0 ;
  double radius=0.0, 
         phi  =0.0,
         prob =0.0, 
         bintc=0.0;
  VectorType xvec = {1, 0, 0}, kvec;
  
  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON2_RDIV;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for (iR=0;   iR  <= nBinsY; iR++)   BinPosY[iR]   = rmin   + (rmax-rmin)     * iR   / (double)nBinsY;
  for (jPhi=0; jPhi<= nBinsZ; jPhi++) BinPosZ[jPhi] = phimin + (phimax-phimin) * jPhi / (double)nBinsZ;

  for (iR=0; iR < nBinsY; iR++)
  { for (jPhi=0; jPhi < nBinsZ; jPhi++)
	  {
	    IntYZ     [iR][jPhi] = 0.0;
	    IntYZError[iR][jPhi] = 0.0;
	    nTrajYZ   [iR][jPhi] = 0;
	  }
  }

  DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
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
	      bRegistered=0;
	      if (bExclusive==0)
		      WriteNeutron(&(InputNeutrons[i]));

	      if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	      if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	      if (InputNeutrons[i].Position[1] < filtYMin) continue;
	      if (InputNeutrons[i].Position[1] > filtYMax) continue;
	      if (InputNeutrons[i].Position[2] < filtZMin) continue;
	      if (InputNeutrons[i].Position[2] > filtZMax) continue;

	      if (bProbactiv==1.0) 
          prob = InputNeutrons[i].Probability;
	      else 
          prob=1.0;

	      radius = sqrt(sq(InputNeutrons[i].Position[1])+sq(InputNeutrons[i].Position[2]));
	      CopyVector(InputNeutrons[i].Vector, kvec);
	      NormVector(kvec);
	      phi = acos(ScalarProduct(xvec, kvec))/M_PI*180.;

	      iR   = (int)floor(nBinsY*(radius-rmin)/(rmax-rmin));
	      jPhi = (int)floor(nBinsZ*(phi-phimin)/(phimax-phimin));
			
	      if (((iR>=0)&&(iR<nBinsY))&&((jPhi>=0)&&(jPhi<nBinsZ)))
        {	
	        nTrajYZ[iR][jPhi]++;
	        IntYZ  [iR][jPhi]+= prob;
	        bintc            += prob;
	        bRegistered=1;
	      }
	  
	      if ((bExclusive==1) && (bRegistered==1))
          WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteHeader2D(fMonitor, format, "Intensity", bProbactiv,  nBinsY, "radius/cm", nBinsZ, "radial-divergence/deg");
  // WriteOutput2D(fMonitor, format,           bProbactiv,  nBinsY, BinPosY,           nBinsZ, BinPosZ,  IntYZ, IntYZError, nTrajYZ);
  WriteOutput2D(fMonitor, format,  bProbactiv,  
                nBinsY, BinPosY,   nBinsZ, BinPosZ,  
                IntYZ, IntYZError, nTrajYZ);
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
	        break;
	      case 'z':
	        nBinsZ = atol(&argv[i][2]); /* number of bins vertical axis */
	        break;

	      case 'w':
	        rmin = atof(&argv[i][2]);		/* left edge position window */
	        break;
	      case 'W':
	        rmax = atof(&argv[i][2]);		/* right edge position window */
	        break;
	      case 'h':
	        phimin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'H':
	        phimax =  atof(&argv[i][2]);   /* top position window */
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
	        fprintf(LogFilePtr,"Error: unknown commandline option: %s\n",argv[i]);
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
  
  if (bProbactiv != TRUE) 
    bProbactiv = FALSE;

  // Allocate memory for the monitor data
  BinPosY    = (double*)  malloc((nBinsY+1) * sizeof(double));
  BinPosZ    = (double*)  malloc((nBinsZ+1) * sizeof(double));

  IntYZ      = (double**) malloc(nBinsY * sizeof(double*));
  IntYZError = (double**) malloc(nBinsY * sizeof(double*));
  nTrajYZ    =   (long**) malloc(nBinsY * sizeof(long*));

  for (int iY=0; iY < nBinsY; iY++) 
  {
    IntYZ     [iY] = (double*) malloc(nBinsZ * sizeof(double));
    IntYZError[iY] = (double*) malloc(nBinsZ * sizeof(double));
    nTrajYZ   [iY] = (long*)   malloc(nBinsZ * sizeof(int));
  }

  return;
}