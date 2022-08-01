/********************************************************************************************/
/*  VITESS module 'mon2_posdiv.c'                                                           */
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
#include "init.h"

#include "softabort.h"
#include "defines.h"
#include "general.h"
#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName  = NULL;        // -O    [-]   Monitor output file containing intensity as a function of y- and z-position   
short  bProbactiv   = TRUE,        // -p    [-]   flag Display:   YES: Probability weight   NO: number of trajectories
       bExclusive   = FALSE;       // -e    [-]   flag Exclusion: YES: only neutrons meeting the monitor conditions are written  NO: all are written
VtAxis index_yz     = NO_AXIS;     // -q    [-]   enum direction:  Y_AXIS  Z_AXIS   
long   nBinsPos     = 1,           // -y    [-]   number of position bins
       nBinsDiv     = 1,           // -z    [-]   number of divergence bins
       format       = MATRIX;      // -F    [-]   file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double pos_min      = 0.0,         // -w   [cm]   min. position to be monitored
       pos_max      = 0.0,         // -W   [cm]   max. position to be monitored
       div_min      = 0.0,         // -h   [deg]  min. divergence to be monitored
       div_max      = 0.0;         // -H   [deg]  max. divergence to be monitored
double filtLambdaMin=-1.0,         // -l   [Ang]  filter: lower bound value of the wavelength range
       filtLambdaMax=-1.0,         // -L   [Ang]  filter: upper bound value of the wavelength range
       filtYMin     =-1.0e10,      // -u   [cm]   filter: left edge position of the monitored area
       filtYMax     = 1.0e10,      // -U   [cm]   filter: right edge position of the monitored area
       filtZMin     =-1.0e10,      // -v   [cm]   filter: bottom position of the monitored area
       filtZMax     = 1.0e10;      // -V   [cm]   filter: top position of the monitored area
                                 
// Variables determined from input parameters
FILE*  fMonitor     = NULL;      
                                 
double*  BinPosY    = NULL;        //             edges of the bins of the first parameter 
double*  BinPosZ    = NULL;        //             edges of the bins of the second parameter 
double** IntYZ      = NULL;        //             intensity within a bin (in 2 dimensions) 
double** IntYZError = NULL;        //             standard deviation of this intensity 
long  ** nTrajYZ    = NULL;        //             number of trajectories within a bin


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  int		 iPos=0, jDiv=0;
  long	 i=0;
  double pos_=0.0, 
         div_=0.0, 
         bintc=0.0,  // Total intensitiy within monitor limits
         prob =0.0;  // Intensitiy of a trajectory
  
  // reading of input data and initilisation
  // ---------------------------------------
 _eModule=MCN_MON2_POSDIV;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for (iPos=0; iPos <= nBinsPos; iPos++) BinPosY[iPos] = pos_min + (pos_max-pos_min) * iPos / (double)nBinsPos;
  for (jDiv=0; jDiv <= nBinsDiv; jDiv++) BinPosZ[jDiv] = div_min + (div_max-div_min) * jDiv / (double)nBinsDiv;

  for(iPos=0; iPos < nBinsPos; iPos++)
  { for(jDiv=0; jDiv < nBinsDiv; jDiv++)
	  {
	    IntYZ     [iPos][jDiv] = 0.0;
	    IntYZError[iPos][jDiv] = 0.0;
	    nTrajYZ   [iPos][jDiv] = 0;
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
        bRegistered = FALSE;
	      if(bExclusive==0) 
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
          prob=1.0;

	      pos_ = InputNeutrons[i].Position[index_yz];

	      if (index_yz == Y_AXIS)
        {
	        if (InputNeutrons[i].Vector[0] >=0) 
            div_ = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	        else 
            div_ = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	  
	        div_ *= 180.0/M_PI;
	      }
	      else if (index_yz == Z_AXIS) 
        {
	        div_  = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));	 
	        div_ *= 180.0/M_PI;
	      }
        else
        {
          Error("Analysis direction does not have a proper value");
        }

        iPos = (int)floor(nBinsPos*(pos_-pos_min)/(pos_max-pos_min));
	      jDiv = (int)floor(nBinsDiv*(div_-div_min)/(div_max-div_min));
			
	      if (((iPos>=0)&&(iPos<nBinsPos))&&((jDiv>=0)&&(jDiv<nBinsDiv)))
        {	
	        nTrajYZ[iPos][jDiv]++;
	        IntYZ [iPos][jDiv] += prob ;
	        bintc              += prob;
	        bRegistered=1;
	      }
	  
	      if((bExclusive==1) && (bRegistered==1))
	        WriteNeutron(&(InputNeutrons[i]));
      }
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  if (index_yz==Y_AXIS)
    WriteHeader2D (fMonitor, format, "Intensity", bProbactiv, nBinsPos, "y/cm", nBinsDiv, "y-divergence/deg");
  else if (index_yz == Z_AXIS) 
    WriteHeader2D (fMonitor, format, "Intensity", bProbactiv, nBinsPos, "z/cm", nBinsDiv, "z-divergence/deg");
  else
    Error("Analysis direction does not have a proper value");

 // WriteOutput2D(fMonitor, format, bProbactiv,  nBinsPos, BinPosY,        nBinsDiv, BinPosZ,  IntYZ, IntYZError, nTrajYZ);
  WriteOutput2D(fMonitor, format,  bProbactiv,  
                nBinsPos, BinPosY, nBinsDiv, BinPosZ,  
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
  int i, iY;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'q':
	        index_yz = (VtAxis) atoi(&argv[i][2]); /*  y or z direction */
	             if (index_yz==Y_AXIS) fprintf(LogFilePtr,"horizontal direction \n");
		      else if (index_yz==Z_AXIS) fprintf(LogFilePtr,"vertical direction \n");
	        else    Error2("No or wrong direction parameter", &argv[i][2]);
	        break;

        case 'O':
	        MonFileName=&argv[i][2];
	        break;

        case 'y':
	        nBinsPos = atol(&argv[i][2]); /* number of bins horizontal axis*/
	        break;
        case 'z':
	        nBinsDiv = atol(&argv[i][2]); /* number of bins vertical axis*/
	        break;

        case 'w':
	        pos_min = atof(&argv[i][2]);		/* min. position to be monitored */
	        break;
        case 'W':
	        pos_max = atof(&argv[i][2]);		/* max. position to be monitored  */
	        break;
        case 'h':
	        div_min =  atof(&argv[i][2]);   /* min. divergence to be monitored */
	        break;
        case 'H':
	        div_max =  atof(&argv[i][2]);   /* max. divergence to be monitored  */
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
	        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
	        exit(-1);
      }
    }
  }

  // opens monitor file
  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"ERROR: you must define a MonitorOutputFile\n");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  /*initialisation */
  if (bProbactiv != TRUE) 
    bProbactiv = FALSE;

  // Allocate memory for the monitor data
  BinPosY    = (double*)  malloc((nBinsPos+1) * sizeof(double));
  BinPosZ    = (double*)  malloc((nBinsDiv+1) * sizeof(double));

  IntYZ      = (double**) malloc(nBinsPos * sizeof(double*));
  IntYZError = (double**) malloc(nBinsPos * sizeof(double*));
  nTrajYZ    =   (long**) malloc(nBinsPos * sizeof(long*));

  for (iY=0; iY < nBinsPos; iY++) 
  {
    IntYZ     [iY] = (double*) malloc(nBinsDiv * sizeof(double));
    IntYZError[iY] = (double*) malloc(nBinsDiv * sizeof(double));
    nTrajYZ   [iY] = (long*)   malloc(nBinsDiv * sizeof(long));
  }

  return;
}
