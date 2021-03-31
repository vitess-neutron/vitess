/********************************************************************************************/
/*  VITESS module 'mon2_kdiv.c'                                                              */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/*                                                                                          */
/* 1.0  Feb 2006  K. Lieutenant                                                             */
/* 1.0a JAN 2010  A. Houben      xyz output                                                 */
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
       bExclusive = FALSE;     // -e    [-]    flag Exclusion: YES: only neutrons meeting the monitor conditions are written   NO: all are written
long	 nbiny    =  1,          // -y    [-]    number of bins in horizontal direction
       nbinz    =  1,          // -z    [-]    number of bins in vertical direction
       format   =  MATRIX;     // -F    [-]    file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double DivKyMin =  0.0,        // -w  [1/Ang]  min. horizontal divergence to be monitored
       DivKyMax =  0.0,        // -W  [1/Ang]  max. horizontal divergence to be monitored
       DivKzMin =  0.0,        // -h  [1/Ang]  min. vertical divergence to be monitored
       DivKzMax =  0.0;        // -H  [1/Ang]  max. vertical divergence to be monitored
  
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
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  int	   iY=0, jZ=0;
  long	 i=0;
  double Divy =0.0, 
         Divz =0.0, 
         DivKy=0.0, 
         DivKz=0.0, 
         bintc=0.0,
         prob =0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON2_KDIV;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for(iY = 0; iY < nbiny+1; iY++)
  {
    BinPosY[iY] = DivKyMin + (DivKyMax-DivKyMin) * iY / (double)nbiny;

    for(jZ = 0;jZ<(nbinz+1); jZ++)
    {
	    BinPosZ       [jZ] = DivKzMin + (DivKzMax-DivKzMin)  * jZ / (double) nbinz;
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

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        bRegistered=0;
	      if(bProbactiv==1.0) 
          prob = InputNeutrons[i].Probability;
	      else 
          prob=1.0;

	      if (InputNeutrons[i].Vector[0] >=0) 
          Divy = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	      else 
          Divy = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	 

	      Divz = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));  
	   
        DivKy = Divy * 2.0 * M_PI / InputNeutrons[i].Wavelength;
	      DivKz = Divz * 2.0 * M_PI / InputNeutrons[i].Wavelength;

	      iY = (int)floor(nbiny*(DivKy-DivKyMin)/(DivKyMax-DivKyMin));
	      jZ = (int)floor(nbinz*(DivKz-DivKzMin)/(DivKzMax-DivKzMin));
			
	      if (((iY>=0)&&(iY<nbiny))&&((jZ>=0)&&(jZ<nbinz)))
	      {	
	        nTrajYZ[iY][jZ]++;
	        IntYZ  [iY][jZ]+= prob;
	        bintc          += prob;
	        bRegistered = 1;
	      }
	  
	      if ((bExclusive==0) || (bRegistered==1))
	        WriteNeutron(&(InputNeutrons[i]));
      }
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteHeader2D(fMonitor, format, "Intensity", bProbactiv,  nbiny, "k_y/(1/Ang)", nbinz, "k_z/(1/Ang)");
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

	      case 'h':
	        DivKzMin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'w':
	        DivKyMin = atof(&argv[i][2]);		/* left edge position window */
	        break;
	      case 'H':
	        DivKzMax =  atof(&argv[i][2]);   /* top position window */
	        break;
	      case 'W':
	        DivKyMax = atof(&argv[i][2]);		/* right edge position window */
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
    Error("You must define a MonitorOutputFile");
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }
 

  /*initialisation */
  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}

