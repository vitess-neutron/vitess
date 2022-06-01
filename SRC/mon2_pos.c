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
/* 1.3a Nov 2020  K. Lieutenant  new 'mon2_header'                                          */
/* 1.4  Mar 2021  K. Lieutenant  update after each bunch                                   */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "init.h"
#include "softabort.h"
#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*    MonFileName= NULL;     // -O    [-]   Monitor output file containing intensity as a function of y- and z-position   
short    bProbactiv = TRUE,     // -p    [-]   flag Display  : YES: Probability weight   NO: number of trajectories
         bExclusive = FALSE;    // -e    [-]   flag Exclusion: YES: only neutrons meeting the monitor conditions are written  NO: all are written
long     nBinsY     = 1,        // -y    [-]   number of bins in horizontal direction
         nBinsZ     = 1;        // -z    [-]   number of bins in vertical direction
VtFormat2D  eFormat = MATRIX;   // -F    [-]   file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double   WidthMin   = 0.0,      // -w   [cm]   left edge position of the monitored area
         WidthMax   = 0.0,      // -W   [cm]   right edge position of the monitored area
         HeightMin  = 0.0,      // -h   [cm]   bottom position of the monitored area
         HeightMax  = 0.0;      // -H   [cm]   top position of the monitored area
double   FiltLmbdMin=-1.0,      // -l   [Ang]  filter: lower bound value of the wavelength range
         FiltLmbdMax=-1.0;      // -L   [Ang]  filter: upper bound value of the wavelength range

// Variables determined from input parameters
long     nBunches  = 1;         //             number of bunches started
double*  BinPosY   = NULL;      //             edges of the bins of the first parameter
double*  BinPosZ   = NULL;      //             edges of the bins of the second parameter
double** IntYZ     = NULL;      //             intensity within a bin (in 2 dimensions) 
double** IntYZError= NULL;      //             standard deviation of this intensity 
long  ** nTrajYZ   = NULL;      //             number of trajectories within a bin
long     nTrajTot=0;            //             total number of traj. within monitor limits
double   TotInt  =0.0;          //             total intensitiy within monitor limits


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables
void UpdateMon(long iBnch);             // Updates monitor output file 


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=0;
  int	   iY=0, jZ=0;
  long 	 i=0,
         iBnch=0;       // current bunch
  double prob   = 0.0;  // Intensitiy of a trajectory

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON2_POS;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.4");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  nBunches = ReadNumBnch();

  // initializes arrays
  for (iY=0; iY <= nBinsY; iY++) BinPosY[iY] = WidthMin  +  (WidthMax-WidthMin)  * iY / (double)nBinsY;
  for (jZ=0; jZ <= nBinsZ; jZ++) BinPosZ[jZ] = HeightMin + (HeightMax-HeightMin) * jZ / (double)nBinsZ;

  for(iY=0; iY < nBinsY; iY++)
  { for(jZ=0; jZ < nBinsZ; jZ++)
	  {
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
	    bRegistered=0;

      // Update monitor output if EOB line is found
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      { 
        iBnch++;
        UpdateMon(iBnch);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
	      if (bExclusive==0) 
		      WriteNeutron(&(InputNeutrons[i]));

	      if (FiltLmbdMin >= 0.0 && InputNeutrons[i].Wavelength < FiltLmbdMin) continue;
	      if (FiltLmbdMax >= 0.0 && InputNeutrons[i].Wavelength > FiltLmbdMax) continue;

	      if (bProbactiv==TRUE) 
          prob = InputNeutrons[i].Probability;
	      else 
          prob = 1.0;

	      iY = (int)floor(nBinsY*(InputNeutrons[i].Position[1]-WidthMin) /(WidthMax -WidthMin));
	      jZ = (int)floor(nBinsZ*(InputNeutrons[i].Position[2]-HeightMin)/(HeightMax-HeightMin));
			
	      if (((iY>=0)&&(iY<nBinsY))&&((jZ>=0)&&(jZ<nBinsZ)))
	      {	
	        nTrajYZ[iY][jZ]++;
          nTrajTot++;
	        IntYZ  [iY][jZ]+= prob ;
	        TotInt         += prob;
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
  // writes final monitor output
  UpdateMon(nBunches);  

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
	        nBinsY = atol(&argv[i][2]); /* number of bins y-direction */
	        break;
	      case 'z':
	        nBinsZ = atol(&argv[i][2]); /* number of bins, z-direction */
	        break;

	      case 'w':
	        WidthMin = atof(&argv[i][2]);		/* left edge position window   [cm]*/
	        break;
	      case 'W':
	        WidthMax = atof(&argv[i][2]);		/* right edge position window    [cm]*/
	        break;
	      case 'h':
	        HeightMin =  atof(&argv[i][2]);   /* bottom position window   [cm]*/
	        break;
	      case 'H':
	        HeightMax =  atof(&argv[i][2]);   /* top position window   [cm]*/
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
            FiltLmbdMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
            break;
        case 'L':
            FiltLmbdMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
            break;

	      case 'F':
            eFormat = (VtFormat2D) atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz */
            break;

	      default:
	        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
	        exit(-1);
	    }
    }
  }

  // check for file name
  if (MonFileName==NULL)
    Error("You must define a monitor output file");

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


/*******************************************************/
/**  Updates main monitor output file                 **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  double f_norm  = 1.0;               // ratio of total to processed bunches after treating current bunch
  FILE*  fMonitor= NULL;              // pointer to output file

  // opens monitor file
  fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");

  if (fMonitor)
  {
    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    // writes header and data
    WriteHeader2DB(fMonitor, eFormat, "Intensity", bProbactiv, iBnch, nBunches, TotInt, nTrajTot,   
                   nBinsY, "y/cm",          nBinsZ, "z/cm");

    WriteOutput2DB(fMonitor, eFormat, bProbactiv,  
                   nBinsY, BinPosY,   nBinsZ, BinPosZ,  f_norm,
                   IntYZ, IntYZError, nTrajYZ);

    fclose(fMonitor);
  }
}
