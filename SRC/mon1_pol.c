/********************************************************************************************/
/*  VITESS module 'monitorpol_1D.c'                                                         */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.4  Nov 2020  K. Lieutenant  preparation for transfer to version 4                      */
/* 1.4a Nov 2020  K. Lieutenant  bug of too long module name fixed                          */
/* 1.5  Jun 2023  K. Lieutenant  update after each bunch                                    */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "convert.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define MAX_KIND  8


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
VtMon1Par ePar = NO_MON_PAR;   // -k    [-]   ID for parameter, as a function of which the intensity is shown
char*  MonFileName= NULL;      // -O    [-]   Monitor output file containing polarization as a function of the chosen parameter
short  bProbactiv = TRUE,      // -p    [-]   flag: YES: Probability weight   NO: number of trajectories
       bExclusive = FALSE,     // -e    [-]   flag: YES: only neutrons meeting the monitor conditions are written  NO: all are written
       iColour    = ANY_COLOR; // -C    [-]   index: for bAllFiles=FALSE: excludes all neutrons with diff. Colour from monitoring , if iColour >= 0
                               //                    for bAllFiles=TRUE : max. colour to which additional monitor files are generated
long   nBins  = 1;             // -n    [-]   number of monitor channels
double xMin   = 0.0,           // -m   [var]  lower bound value of the monitored range
       xMax   = 0.0,           // -M   [var]  upper bound value of the monitored range
       analysis_dir[3]         // -a -b -c    components of the quantization direction in x-, y- and z-direction
           ={0.0,0.0,1.0};

// Variables determined from input parameters
char   sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV", "deg"},
       sParN[MAX_KIND+1][11]={"", "lambda", "time", "div_y", "div_z", "pos_y", "pos_z", "energy", "div_rad"};
long   nBunches = 1,           //      [-]   number of bunches started
       nTrjTot  = 0,           //      [-]   total number of trajectories within binning
       nTrj  [10001];          //      [-]   number of trajectories per bin
double IntTot  =0.0,           //     [n/s]  total intensity within bin range
       PosT  [10001],          //            limits of the bins (minimal and maximal value)
       IntBin[10001],          //            total intensity (=count rate) per bin
       IntPol[10001],          //            summed up polarization per bin (Sum(count_rate * spin))
       RotMatrixAnalysis[3][3]={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables
void  UpdateMon  (long iBnch);          // Updates evaluation output file


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  char   sCompName  [21]="",
         sModVsnName[MOD_NAME_LEN+8]="";
  short  bRegistered=FALSE;
  int     iBin=0;
  long   iBnch=0,             // current bunch
         i=0;
  double Divy    =0.0,
         Divz    =0.0,
         prob    =0.0,
         binpol  =0.0;

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON1_POL;

  Init(argc, argv, _eModule);
  OwnInit(argc, argv);

  // the following 4 commands replace the call of 'PrintModuleName' to extend the module name
  CompID2Name (sCompName, _eModule);
  snprintf(sModuleName, MOD_NAME_LEN,   "%s_%s",     sCompName, sParN[ePar]);
  snprintf(sModVsnName, MOD_NAME_LEN+7, "%s_%s 1.5", sCompName, sParN[ePar]);
  print_module_name(sModVsnName);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  nBunches = ReadNumBnch();

  // initializes arrays
  for (iBin=0; iBin < nBins+1; iBin++)
  {
    PosT   [iBin] = xMin + ((xMax-xMin)*iBin/(double)nBins);
    IntPol [iBin]=0.0;
    IntBin [iBin]=0.0;
    nTrj   [iBin]=0;
  }

  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  while (ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        iBnch++;
        UpdateMon(iBnch);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        bRegistered=0;

        if (bProbactiv==TRUE)
          prob = InputNeutrons[i].Probability;
        else
          prob=1.0;

        /* write out all neutrons, if 'exclusive counts = no' is set */
        if (bExclusive==0)
          WriteNeutron(&(InputNeutrons[i]));

        /* exclude traj. with wrong colours: (iColour=-1 means: all colours accepted) */
        if (iColour != ANY_COLOR && iColour!=InputNeutrons[i].Color) continue;

        /* calculate spin vector in the direction of the analysis */
        RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

        switch (ePar)
        {
          case MON_LAMBDA:
            iBin = (int) floor((double)nBins*(InputNeutrons[i].Wavelength - xMin)/(xMax-xMin));
            break;

          case MON_TIME:
            iBin = (int) floor(nBins*(InputNeutrons[i].Time - xMin)/(xMax-xMin));
            break;

          case MON_DIV_Y:
            Divy  = atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
            Divy *= 180.0/M_PI;
            if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divy=0.0;
            iBin = (int)floor(nBins*(Divy - xMin)/(xMax-xMin));
            break;

          case MON_DIV_Z:
            Divz  = atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
            Divz *= 180.0/M_PI;
            if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divz=0.0;
            iBin = (int)floor(nBins*(Divz - xMin)/(xMax-xMin));
          break;

          case MON_Y:
            iBin = (int)floor(nBins*(InputNeutrons[i].Position[1] - xMin)/(xMax-xMin));
            break;

          case MON_Z:
            iBin = (int)floor(nBins*(InputNeutrons[i].Position[2] - xMin)/(xMax-xMin));
            break;

          default:
            fprintf(LogFilePtr,"Parameter not handled in %s", sCompName);
            exit(-1);
        }

        if (iBin >= 0 && iBin < nBins)
        { nTrj[iBin]++;
          nTrjTot++;
          IntPol[iBin] += prob * InputNeutrons[i].Spin[0];
          IntBin[iBin] += prob;
          binpol       += prob * InputNeutrons[i].Spin[0] ;
          IntTot       += prob;
          bRegistered = 1;
        }

        /* calculate spin vector in the original direction */
        RotBackVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

        /* write out registered neutrons, if 'exclusive counts = yes' is set */
        if ((bExclusive==1) && (bRegistered==1))
          WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes monitor output
  UpdateMon(nBunches);

  // writes to instrument and log file
  fprintf(LogFilePtr, "Binning  : %ld bins from %10.5f to %10.5f %s\n", nBins, xMin, xMax, sUnit[ePar]);
  fprintf(LogFilePtr, "File     : %s\n", MonFileName);
  if(IntTot != 0.)
    fprintf(LogFilePtr,"average polarization: %3.5f \n", binpol/IntTot);

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}



/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int    i;
  double roty, rotz;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'O':
          MonFileName=&argv[i][2];
          break;

        case 'a':
          sscanf(&argv[i][2], "%lf", &analysis_dir[0]) ;
          break;
        case 'b':
          sscanf(&argv[i][2], "%lf", &analysis_dir[1]) ;
          break;
        case 'c':
          sscanf(&argv[i][2], "%lf", &analysis_dir[2]) ;
          break;

        case 'k':
          ePar = (VtMon1Par) atol(&argv[i][2]);       /* 1= monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz, 5=monitory, 6=monitorz */
          break;

        case 'n':
          nBins = atol(&argv[i][2]);      /* number of bins */
          if (nBins > 10000)
            Error("number of bins must be <= 10000");
          break;
        case 'C':
          iColour = atol(&argv[i][2]);         /*  excludes all neutrons with diff. Colour, if iColour >= 0   */
          break;

        case 'm':
          xMin = atof(&argv[i][2]);       /* lower bound lambda, time or div. window [A], [ms], [deg]*/
          break;
        case 'M':
          xMax = atof(&argv[i][2]);       /* upper bound lambda, time or div. window [A], [ms], [deg]*/
          break;

        case 'p':
          bProbactiv = atof(&argv[i][2]);  /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
          break;
        case 'e':
          if(argv[i][2]=='1')
            bExclusive = 1;               /* if activated, only neutrons meeting the monitor conditions are considered further on */
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (MonFileName==NULL)
  { Error("you must define a MonitorOutputFile");
  }

  if (bProbactiv != 1)
    bProbactiv = 0;

  CartesianToEulerZY(analysis_dir, &roty, &rotz);
  FillRotMatrixZY(RotMatrixAnalysis, roty, rotz);

  return;
}


/*******************************************************/
/**  Updates main monitor output file                 **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  double f_norm=1.0;             // ratio of total to processed bunches after treating current bunch
  long   iBin=0;                 // index of bins in x-axix and for main monitor
  double xBin=0.0,               // center of the current bin
         Pol     =0.0,        // polarization in a bin
         PolError=0.0;        // standard variation of the polarization in a bin
  FILE*  pFile=NULL;

  // opens monitor file
  pFile = OpenOutputFile(MonFileName, TRUE, "wt");

  if (pFile != NULL)
  {
    WriteHeader1DB(pFile, FALSE, "polarization", ANY_COLOR, iBnch, nBunches, nBins, IntTot, nTrjTot,
                          sParN[ePar], sUnit[ePar], xMin, xMax);

    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    for (iBin = 0; iBin < nBins; iBin++)
    {
      xBin = (PosT[iBin]+PosT[iBin+1])/2.0;

      if (IntBin[iBin]!=0.0 && nTrj[iBin] > 0)
      {
        Pol      = IntPol[iBin]/IntBin[iBin];
        PolError = Pol * sqrt(1./nTrj[iBin]);
        fprintf(pFile, "%10.4f  %12.5e %12.5e  %7ld\n", xBin, Pol, PolError, nTrj[iBin]);
      }
      else
      {
        fprintf(pFile, "%10.4f   0.00000E+00  0.00000E+00        0\n",    xBin);
      }
    }

    fclose(pFile);
  }
}
