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
/* 1.4  Mar 2021  K. Lieutenant  update after each bunch                                    */
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

// Variables determined from input parameters or simulation file
long     nBunches  = 1;          //             number of bunches started
double*  BinPosY   = NULL;       //             edges of the bins of the first parameter
double*  BinPosZ   = NULL;       //             edges of the bins of the second parameter
double** IntYZ     = NULL;       //             intensity within a bin (in 2 dimensions)
double** IntYZError= NULL;       //             standard deviation of this intensity
long  ** nTrajYZ   = NULL;       //             number of trajectories within a bin
long     nTrajTot  = 0;          //             total number of traj. within monitor limits
double   TotInt    = 0.0;        //             total intensitiy within monitor limits


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);    // Reads input parameters and sets global parameters
void UpdateMon(long iBnch);              // Updates monitor output file


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  int     iY=0, jZ=0;
  long   iBnch=0,      // current bunch
         i=0 ;
  double Divy  =0.0,
         Divz  =0.0,
         prob  =0.0;   // Intensitiy of a trajectory

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule = MCN_MON2_DIV;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.4a");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  nBunches = ReadNumBnch();

  // initializes arrays
  for (iY=0; iY <= nBinsY; iY++) BinPosY[iY] = DivYmin + (DivYmax-DivYmin) * iY / (double)nBinsY;
  for (jZ=0; jZ <= nBinsZ; jZ++) BinPosZ[jZ] = DivZmin + (DivZmax-DivZmin) * jZ / (double)nBinsZ;

  for (iY=0; iY < nBinsY; iY++)
  { for (jZ=0; jZ < nBinsZ; jZ++)
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
      bRegistered = 0;

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
          nTrajTot++;
          IntYZ  [iY][jZ]+= prob;
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
  UpdateMon(nBunches);  // final monitor output

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
          DivYmin = atof(&argv[i][2]);    /* left edge position window */
          break;
        case 'W':
          DivYmax = atof(&argv[i][2]);    /* right edge position window */
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

  for (iY=0; iY < nBinsY; iY++)
  {
    IntYZ     [iY] = (double*) malloc(nBinsZ * sizeof(double));
    IntYZError[iY] = (double*) malloc(nBinsZ * sizeof(double));
    nTrajYZ   [iY] = (long*)   malloc(nBinsZ * sizeof(long));
  }

  return;
}


/*******************************************************/
/**  Updates main monitor output file                 **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  int    iBinY=0, jBinZ=0;            // matrix indices
  double f_norm  = 1.0;               // ratio of total to processed bunches after treating current bunch
  FILE*  fMonitor= NULL;              // pointer to output file

  // opens monitor file
  fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");

  if (fMonitor)
  {
    // normalize according number of bunches simulated
    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    // calculate standard deviation
    for (iBinY = 0; iBinY < nBinsY; iBinY++)
    { for (jBinZ = 0; jBinZ < nBinsZ; jBinZ++)
      {
        if (nTrajYZ[iBinY][jBinZ] > 0)
          IntYZError[iBinY][jBinZ] = IntYZ[iBinY][jBinZ] / sqrt(nTrajYZ[iBinY][jBinZ]);
        else
          IntYZError[iBinY][jBinZ] = 0.0;
      }
    }

    // writes header and data
    WriteHeader2DB(fMonitor, FALSE, eFormat, "Intensity", bProbactiv, iBnch, nBunches, TotInt, nTrajTot,
                   nBinsY, "div_y [deg]", DivYmin, DivYmax,
                   nBinsZ, "div_z [deg]", DivZmin, DivZmax);

    WriteOutput2DB(fMonitor, eFormat, bProbactiv,
                   nBinsY, BinPosY,   nBinsZ, BinPosZ,  f_norm,
                   IntYZ, IntYZError, nTrajYZ);

    fclose(fMonitor);
  }
}
