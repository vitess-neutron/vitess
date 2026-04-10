/*********************************************************************************************/
/*  VITESS module EVAL_ELAST_SANS                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Nov 2011  K. Lieutenant  initial version                                             */
/* 1.1  Nov 2013  K. Lieutenant  flight path correction                                      */
/* 1.2  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/* 1.3  Dec 2021  K. Lieutenant  separation of S(Q) and I(Q)                                 */
/* 1.4  Jun 2023  K. Lieutenant  header + update                                                        */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "general.h"
#include "matrix.h"
#include "softabort.h"
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define BINS   5000


/*********************************/
/** Global Variables            **/
/*********************************/
// Input parameters
char  *pIntFileName  = NULL,   // -i   [-]   name of the file containing the intensity spectrum
      *pSofQFileName = NULL,   // -S   [-]   name of the file containing the S(Q) spectrum
      *pRefFileName  = NULL;   // -I   [-]   name of the file containing the reference (=isotropic scattering) spectrum

short  bTOF       = FALSE,     // -w   [-]   TRUE : wavelength is determined from time of flight (for TOF instruments)
       bPathCor   = FALSE;     // -t   [-]   TRUE : correct TOF for real flight path from sample to detector

long   nBins      = 0;         // -n   [-]   number of bins
double Qmin       = 0.0,       // -m [1/Ang] lower bound of rhe evaluated q range
       Qmax       = 0.0,       // -M [1/Ang] upper bound of the evaluated q range
       LogProz    = 0.0,       // -R   [%]   percentage of increase to next bin
       DeadSpot   = 0.0,       // -d  [deg]  excludes all neutrons with a scattering angle < DeadSpot
       LmbdRef    = 0.0,       // -r  [Ang]  reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */
       ProbScat   = 0.0,       // -p   [-]   scattering probability of the isotropic scatterer

       Flightpath0= 0.0,       // -l  [ms]   standard length of the total neutron flight path from sample to detector
       DetDist    = 0.0,       // -L  [cm]   detector distance
       TimeOffset = 0.0,       // -T  [ms]   global shift of the neutron time t= t-TimeOffset
       EvalTimeMin=-1.0e10,    // -e  [ms]   minimal and
       EvalTimeMax= 1.0e10;    // -E  [ms]     maximal time for evaluation
int    nColour    = ANY_COLOR; // -C   [-]   colour necessary for the trajectory to be regarded, colour ANY_COLOR (=-1) means: all trajectories are regarded

// Variables determined from input parameters or trajectory data
short  bLogBinning=FALSE,      //      [-]   flag: TRUE : binning increases exponentially    FALSE: linear binning
       bAllFiles  =FALSE,      //      [-]   flag: TRUE : S(Q) and reference file available
       bSofQ      =FALSE,      //      [-]   flag: TRUE : S(Q) can be written
       bDeadSpot  =FALSE;      //      [-]   flag: TRUE : deadspot exists
long   nBunches   = 1,         //      [-]   number of bunches started
       nTrjTot    = 0,         //      [-]   total number of trajectories within binning
       nTrj  [BINS+1];        //      [-]   number of trajectories contributing to count rate in a bin
double IntTot  =0.0,           //     [n/s]  total intensity within bin range
       BinSize =1.0,           //    [1/Ang] width of a channel
       BinLmts[BINS+1],        //    [1/Ang] limits of the bins
       IntBin[BINS+1],         //     [n/s]  count rate of a bin
       QRef   [BINS+1],        //    [1/Ang] centers of the Q bins in the ref. file
       IntRef [BINS+1];        //     [n/s]  count rate of the reference file

FILE  *pSofQFile  = NULL,      //      [-]   name of the file containing the S(Q) spectrum
      *pRefFile   = NULL;      //      [-]   name of the file containing the reference (=isotropic scattering) spectrum


/******************************/
/** Prototypes               **/
/******************************/
short ReadRefSpec(double* pRefBin, double* pRefVal);  // function to read reference spectrum
void  OwnInit    (int argc, char *argv[]);            // Reads input parameters and sets global variables
void  InitArrays ();                                  // Allocates memory and initializes evaluation arrays
void  UpdateMon  (long iBnch);                        // Updates evaluation output file


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bWrite  =FALSE;     /* criterion: write data point to output  */
  int    ibin =0,            /* index for the bins                     */
         nSpec=0;            /* number of calculated S-values          */
  long   i=0,                /* trajectory index */
         iBnch=0;            /* current bunch    */
  double time    =0.0,       /* TOF                                    */
         lambda  =0.0,       /* wavelngth of the neutron               */
         TwoTheta=0.0,       /* scattering angle theta = 2 theta_bragg */
         Phi     =0.0,       /* scattering direction phi               */
         Q       =0.0,       /* Q value of the scattering              */
         Svalue  =0.0,       /* value S(Q)                             */
         prob    =0.0,       /* weight of the trajectory               */
         Flightpath=0.0,     // real length of neutron flight path [cm]
         DetPath =0.0,       // path length from sample to position of detection
         sigma   =0.0,       // standard deviation of the intensity in the bin
         Qcntr;              // Q value calculated from the limits of the bin

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_SANS;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.4");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  nBunches = ReadNumBnch();

  memset(BinLmts, 0, (BINS+1)*sizeof(double));
  memset(IntBin,  0, (BINS+1)*sizeof(double));
  memset(QRef,    0, (BINS+1)*sizeof(double));
  memset(IntRef,  0, (BINS+1)*sizeof(double));

  // Read reference spectrum
  if (bAllFiles)
  { bSofQ=ReadRefSpec(QRef, IntRef);
    if (bSofQ==FALSE)
      Warning("reference spectrum could not be read, S(Q) file cannot be written");
  }

  /* Construction of the Bins */
  InitArrays();

  /* Processing of the Neutrons */
  DECLARE_ABORT

  // loop over trajectories
  // ----------------------
  while (ReadNeutrons()!=0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        iBnch++;
        UpdateMon(iBnch);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        CartesianToSpherical(InputNeutrons[i].Vector, &TwoTheta, &Phi);

        // flightpath correction if detector distance is given
        if (bPathCor)
        { // origin of co-ordinate system in sample center
          DetPath    = sqrt(sq(InputNeutrons[i].Position[0]) + sq(InputNeutrons[i].Position[1]) + sq(InputNeutrons[i].Position[2]));
          Flightpath = Flightpath0 + DetPath - DetDist;
        }
        else
        { Flightpath = Flightpath0;
        }

        prob     = InputNeutrons[i].Probability;
        time     = InputNeutrons[i].Time - TimeOffset;
        lambda   = bTOF ? 395.60346/(Flightpath/time) : LmbdRef;

        /* Writing out all neutrons, if 'exclusive counts = no' is set */
        // if (bExclCount==FALSE)
        WriteNeutron(&InputNeutrons[i]);

        /* trajectories within deadspot */
        if (bDeadSpot && TwoTheta <= DeadSpot) continue;

        /* traj. out of time of evaluation */
        if (time < EvalTimeMin || time > EvalTimeMax) continue;

        /* exclude traj. with wrong colour: (nColour=-1 means: all colours accepted) */
        if (nColour!=ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;

        /* Writing out the neutrons that comply with the requirements,
        if 'exclusive counts = yes' is set */
        /* if (bExclCount==TRUE)
          WriteNeutron(&InputNeutrons[i]); */

        Q = 4.0 * M_PI * sin(TwoTheta/2.0) / lambda;

        for(ibin = 0; ibin<nBins; ibin++)
        {  if (BinLmts[ibin] <= Q && Q < BinLmts[ibin+1])
          {
            nTrj  [ibin]++;
            nTrjTot++;
            IntBin[ibin] += prob;
            IntTot       += prob;
            break;
          }
        }
      }
    }
  }

// Finish: writes and closes evaluate file, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
  my_exit:
  /* Output of Results */
  fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n\n", IntTot);

  // writes evaluation output
  UpdateMon(nBunches);

  // writes S(Q) file if Q values agree and both values ar non-zero
  if (bSofQ)
  {
    WriteHeader1DB(pSofQFile, TRUE, "S(Q)", ANY_COLOR, nBunches, nBunches, nBins, IntTot, nTrjTot,
                              "Q", "[1/Ang]", Qmin, Qmax);

    for(ibin = 0; ibin < nBins; ibin++)
    {
      if (bLogBinning)
        Qcntr = sqrt(BinLmts[ibin]*BinLmts[ibin+1]);
      else
        Qcntr = 0.5*(BinLmts[ibin]+BinLmts[ibin+1]);

      bWrite = (IntBin[ibin] > 0.0 && IntRef[ibin] > 0.0);

      if (bWrite && RoundP(Qcntr,6)==RoundP(QRef[ibin],6))
      {
        nSpec++;
        Svalue = ProbScat * IntBin[ibin] / IntRef[ibin];
        if (nTrj[ibin] > 0)
          sigma = Svalue * sqrt(1.0/(double)nTrj[ibin]);
        fprintf(pSofQFile, "%10.4f  %12.5e %12.5e  %7ld\n", Qcntr, Svalue, sigma, nTrj[ibin]);
      }
    }
    if (nSpec==0)
      Warning("no agreement in binning between reference and current spectrum - no S(Q) file generated");
  }

  // Cleanup
  if (pSofQFile!=NULL) fclose(pSofQFile);
  if (pRefFile !=NULL) fclose(pRefFile );

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;
}


/**********************************************************/
/* ReadRefSpec()  function to read reference spectrum     */
/*                                                        */
/* output: *pRefBin: centers of the bins in the ref. file */
/*         *pRefVal: intensities in the reference file    */
/*                                                        */
/* return: rc      : TRUE/FALSE                           */
/**********************************************************/
short ReadRefSpec(double* pRefBin, double* pRefVal)
{
  int    iBin=0;
  short  rc=FALSE;
  char   sLine[1024];
  double TabVal[6];

  if (pRefFile != NULL)
  {
    while (ReadLine(pRefFile, sLine, sizeof(sLine)-1))
    { StrgScanLF(sLine, TabVal, 6, 0);
      pRefBin[iBin] = TabVal[0];
      pRefVal[iBin] = TabVal[1];
      iBin++;
    }

    if (iBin > 0)
      rc=TRUE;

    fclose(pRefFile);
    pRefFile = NULL;
  }
  return rc;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  long  i=0;
  char* arg=NULL;

  for(i=1; i<argc; i++)
  {
    arg = argv[i];
    if (*arg !='+')
    {
      arg += 2;
      switch(arg[-1])
      {
        case 'S':
          pSofQFileName = arg;
          break;
        case 'i':
          pIntFileName = arg;
          break;
        case 'I':
          pRefFileName = arg;
          break;

        case 'n':
          nBins = atol(arg); /* number of bins */
          if (nBins > BINS)
          { fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
            exit(99);
          }
          break;
        case 'm':
          Qmin = atof(arg);                     /* lower bound of Q range [1/A] */
          break;
        case 'M':
          Qmax = atof(arg);                     /* upper bound of Q range [1/A] */
          break;

        case 'R':
          LogProz  = atof(arg);              /* percentage of increase to next bin */
          if (LogProz!=0.0)
          bLogBinning = TRUE;
          break;
        case 'd':
          DeadSpot  = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
          bDeadSpot = TRUE;                 /* scattering angle < DeadSpot [deg] */
          break;
        case 'r':
          LmbdRef   = atof(arg);            /* reference Wavelength for crystal monochromator */
          break;                            /* (or mechanical velocity selector) instrument   */
        case 'p':
          ProbScat  = atof(arg);            /* scattering probability of the isotropic scatterer */
          break;

        case 'w':
          bTOF     = (short) atoi(arg); ;   /*  time of flight instrument -> wavelength from TOF */
          break;
        case 't':
          bPathCor = (short) atoi(arg);     /*  correct flight path length for location of detection */
          break;

        case 'l':
          Flightpath0 = atof(arg);  /* length of neutron flight path [cm] */
          if (Flightpath0 <= 0.0)
            Error("you must define a flight path > 0.0");
          break;
        case 'L':
          DetDist    = atof(arg);  /* length of neutron flight path [cm] */
          break;
        case 'T':
          TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
          break;

        case 'e':
          EvalTimeMin = atof(arg);        /* minimal time for evaluation */
          break;
        case 'E':
          EvalTimeMax = atof(arg);             /* maximal time for evaluation */
          break;
        case 'C':
          nColour = atoi(arg);                  /*  excludes all neutrons with diff. Colour, if nColour >= 0 */
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
          exit(-1);
          break;
      }
    }
  }

  /* checks */
  if (bLogBinning && Qmin==0.0)
    Error("lower bound value must not be zero for logarithmic binning");

  // open intensity file whenever possible, open also reference ans SofQ file if all files are given.
  /* if      (pIntFileName==NULL && pSofQFileName==NULL)
  { Error("No file given for the intensity spectrum");
  }
  else if (pIntFileName!=NULL && pSofQFileName==NULL)
  { pIntFile = OpenOutputFile(pIntFileName, TRUE, "w");
  }
  else if (pIntFileName==NULL && pSofQFileName!=NULL)
  { pIntFile = OpenOutputFile(pSofQFileName, TRUE, "w");
  }
  else if (pIntFileName!=NULL && pSofQFileName!=NULL)
  {
    pIntFile = OpenOutputFile(pIntFileName, TRUE, "w");
    if (pRefFileName!=NULL)
    { pSofQFile = OpenOutputFile(pSofQFileName, FALSE, "w");
      pRefFile  = OpenParameterFile (pRefFileName,  FALSE, "r");
    }
    if (pSofQFile==NULL || pRefFile==NULL)
      Warning("S(Q) or reference file could not be opened. S(Q) file cannot be generated");
    else
      bAllFiles=TRUE;
  }*/
  if (pIntFileName==NULL)
    Error("No file given for the intensity spectrum");

  if (pIntFileName!=NULL && pSofQFileName!=NULL &&  pRefFileName!=NULL)
    bAllFiles=TRUE;

  if (pRefFileName!=NULL)
    pRefFile = OpenParameterFile (pRefFileName,  FALSE, "r");
  if (pSofQFileName!=NULL)
    pSofQFile = OpenOutputFile(pSofQFileName, FALSE, "w");
}


/********************************************************/
/** Allocates memory and initializes evaluation arrays **/
/********************************************************/
void InitArrays()
{
  long iBin;     // index of channel

  /* Construction of the Bins */
  /* logarithmic */
  if (bLogBinning)
  {
    BinLmts[0] = Qmin;
    IntBin[0] = 0.0;
    nTrj  [0] = 0;

    for(iBin = 1; BinLmts[iBin-1] < Qmax; iBin++)
    {
      BinLmts[iBin] = BinLmts[iBin-1] * (1.0 + LogProz/100.);
      IntBin[iBin] = 0.0;
      nTrj  [iBin] = 0;
    }
    nBins = iBin-1;
  }
  /* linear */
  else
  {  BinSize = (Qmax - Qmin) / (double)nBins;

    for(iBin=0; iBin <= nBins; iBin++)
    {
      BinLmts[iBin] = Qmin + BinSize*iBin;
      IntBin [iBin] = 0.0;
      nTrj  [iBin] = 0;
    }
  }
}


/*******************************************************/
/**  Updates evaluation output file                   **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  long   iBin=0;                 // index of bins in x-axix and for main monitor
  double BinCtr=0.0,             // center of the current bin
         sigma=0.0,              // standard deviation of the intensity in the bin
         f_norm =1.0;            // ratio of total to processed bunches after treating current bunch
  FILE*  pIntFile=NULL;             // pointer to output file

  pIntFile = OpenOutputFile(pIntFileName, TRUE, "wt");

  /* Spectrum */
  if (pIntFile != NULL)
  {
    WriteHeader1DB(pIntFile, TRUE, "intensity", ANY_COLOR, iBnch, nBunches, nBins, IntTot, nTrjTot,
                             "Q", "[1/Ang]", Qmin, Qmax);

    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    for(iBin = 0; iBin < nBins; iBin++)
    {
      /* if (fabs(BinInt[iBin]) < 1E-40) BinInt[iBin] = 0.0; */
      if (bLogBinning)
        BinCtr = sqrt(BinLmts[iBin]*BinLmts[iBin+1]);
      else
        BinCtr = (BinLmts[iBin]+BinLmts[iBin+1])/2.0;

      if (nTrj[iBin] > 0)
        sigma = IntBin[iBin] * sqrt(1.0/(double)nTrj[iBin]);
      else
        sigma = 0.0;

      fprintf(pIntFile,"%10.6f  %12.5e %12.5e  %7ld\n", BinCtr,  f_norm*IntBin[iBin], f_norm*sigma, nTrj[iBin]);
    }
    fclose(pIntFile);
  }
}
