/*********************************************************************************************/
/*  VITESS module EVAL_ELAST                                                                 */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0                            initial version                                            */
/* 1.1  Jan 2001  K. Lieutenant  time of evaluation                                          */
/* 1.2  Nov 2001  K. Lieutenant  SOFTABORT                                                   */
/* 1.3  Jan 2002  K. Lieutenant  Cleanup(), log. binning, Writing out of the neutron data    */
/* 1.4  Nov 2003  K. Lieutenant  evaluation dependent on colour                              */
/* 1.5  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                */
/* 1.6  Feb 2004  K. Lieutenant  'FullParName' + ERROR included; check of 'eKind' out of loop*/
/* 1.7  Nov 2005  K. Lieutenant  transformation direction -> scattering angles added         */
/* 1.7a Jun 2009  A. Houben      increased NCENTER from 100 to 200                           */
/* 1.8  Apr 2013  K. Lieutenant  flight path correction                                      */
/* 1.9  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/* 1.9a Oct 2021  K. Lieutenant  tidy up completed                                           */
/* 1.10 Jun 2023  K. Lieutenant  header + update                                             */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "convert.h"
#include "init.h"
#include "general.h"
#include "matrix.h"
#include "softabort.h"
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define BINS  10000
#define NCENTER 200
#define MAX_KIND  4


/*********************************/
/** Global Variables            **/
/*********************************/
// Input parameters
VtEvalPar eKind=VT_NO_EVAL;      // -k  [-]  evaluation parameter: d-spacing, momentum transfer, scattering angle or wavelength difference

char  *EvalFileName= NULL;       // -o  [-]  output file containing the evaluated data
FILE  *fTotCounts  = NULL,       // -O  [-]  optional: file containing integrated intensities (see Help|evaluation)
      *fInfoFile   = NULL;       // -I  [-]  optional: file controling the output of integrated intensities

long   nBins      = 0;           // -n  [-]  number of bins

double MinX       = 0.0,         // -m [var] upper bound of d-spacing, q, theta or lambda range [Ang], [1/Ang], [deg]
       MaxX       = 0.0,         // -M [var] lower bound of d-spacing, q, theta or lambda range [Ang], [1/Ang], [deg]
       LogProz    = 0.0,         // -R  [%]  percentage of increase to next bin
       DeadSpot   = 0.0,         // -d [deg] excludes all neutrons with a scattering angle < DeadSpot
       LmbdRef    = 0.0;         // -r [Ang] reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */

short  bProbactiv = TRUE,        // -p  [-]  flag: TRUE: Probability weight   FALSE: number of trajectories neutron weight is set to 1.0
       bExclCount = FALSE,       // -c  [-]  flag: TRUE: only neutrons complying with the evaluate requirements are written to the output
       bTOF       = FALSE,       // -w  [-]  flag: TRUE: wavelength is determined from time of flight (for TOF instruments)
       bPathCor   = FALSE;       // -t  [-]  flag: TRUE: correct TOF for real flight path from sample to detector

VtAxis eScatAxis  = NO_AXIS;     // -A  [-]  direction of scattering for correct calculation of the scattering parameters

double TotLength  = 0.0,         // -l [cm]  standard length of total neutron flight path
       DetDist    = 0.0,         // -D [cm]  sample detector distance
       TimeOffset = 0.0,         // -T [ms]  global shift of the neutron time t' = t-TimeOffset
       EvalTimeMin=-1.0e10,      // -e [ms]  minimaltime for evaluation
       EvalTimeMax= 1.0e10;      // -E [ms]  maximal time for evaluation
int    nColour    = ANY_COLOR;   // -C  [-]  colour necessary for the trajectory to be regarded, colour=-1(ANY_COLOR) means: all trajectories are regarded

// Variables determined from input parameters or trajectory data
double BinSize=1.0,
       BinPos[BINS+1],           //          limits of the channels  (min. and max. value)
       IntBin[BINS+1],           //          count rates in the channels
       center[NCENTER],
       totcenter[NCENTER],
       range [NCENTER],
       IntTot     = 0.0;         //          total intensity within binning
short  bLogBinning=FALSE,        //          flag: TRUE : binning increases exponentially    FALSE: linear binning
       bDeadSpot  =FALSE;        //          flag: TRUE : deadspot exists
long   nTrj [BINS+1],            //          number of trajectories per channel contributing to count rate
       nTrjTot    = 0,           //          total number of trajectories within binning
       nBunches   = 1;           //          number of bunches started
char   sOption[25]="",           //          name of the x-axis parameter
       sUnit[MAX_KIND+1][6]={"", "Ang", "1/Ang", "deg", "Ang"};   // unit of the x-axis parameter


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables
void InitArrays();                      // Allocates memory and initializes evaluation arrays
void UpdateMon(long iBnch);             // Updates evaluation output file


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bCounted   =FALSE;      // TRUE : neutron is counted, intensity added to channel and total intensity
  long   i=0,
         leftedge=0, rightedge=0,
         iBin,
         iBnch=0;                // current bunch
  int    j=0, k=0, l=0;
  double time=0.0, lambda=0.0,
         TwoTheta=0.0, TwoThetaDeg=0.0, Phi=0.0,
         qValue  =0.0, dspacing=0.0,
         prob    =0.0,
         DelLmbd =0.0,           // difference between wavelength calculated from TOF and true wavelength
         DetPath =0.0,           // path length from sample to position of detection
         Flightpath=0.0;         // length of the total neutron flight path [cm]


  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_ELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.10");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  InitArrays();
  nBunches = ReadNumBnch();
  EvalPar_ID2Txt(sOption, eKind);

  DECLARE_ABORT

  // loop over trajectories
  // ----------------------
  while (ReadNeutrons())
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
        if (InputNeutrons[i].Probability <= wei_min)
          continue;

        bCounted=FALSE;

        /* Writing out all neutrons, if 'exclusive counts = no' is set */
        if (bExclCount==FALSE)
          WriteNeutron(&InputNeutrons[i]);

        /* exclusion of traj. with wrong colour: (nColour=-1 means: all colours accepted) */
        if (nColour!=ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;

        // determination of scattering angle
        if (eScatAxis == Y_AXIS)
        {
          /* Neutron temp = InputNeutrons[i]; */
          /* temp.Vector[0] = sqrt(sq(temp.Vector[0]) + sq(temp.Vector[2])); */
          /* CartesianToSpherical(temp.Vector, &TwoTheta, &Phi); */
          TwoTheta = (double) atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[0]);
          Phi      = (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[1]);
        }
        else if (eScatAxis == Z_AXIS)
        {
          /* Neutron temp = InputNeutrons[i]; */
          /* temp.Vector[0] = sqrt(sq(temp.Vector[0]) + sq(temp.Vector[1])); */
          /* CartesianToSpherical(temp.Vector, &TwoTheta, &Phi); */
          TwoTheta = (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[0]);
          Phi       = (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[1]);
        }
        else
          CartesianToSpherical(InputNeutrons[i].Vector, &TwoTheta, &Phi);

        // flightpath correction if detector distance is given
        if (bPathCor)
        { // origin of co-ordinate system in sample center
          DetPath    = sqrt(sq(InputNeutrons[i].Position[0]) + sq(InputNeutrons[i].Position[1]) + sq(InputNeutrons[i].Position[2]));
          Flightpath = TotLength + DetPath - DetDist;
        }
        else
        { Flightpath = TotLength;
        }

        // determination of weight and wavelength
        prob     = bProbactiv ? InputNeutrons[i].Probability : 1.0;
        time     = InputNeutrons[i].Time - TimeOffset;
        lambda   = bTOF ? 395.60346/(Flightpath/time) : LmbdRef;

        /* trajectories within deadspot */
        if (bDeadSpot && TwoTheta <= DeadSpot) continue;

        /* traj. out of time of evaluation */
        if (time < EvalTimeMin || time > EvalTimeMax) continue;

        switch (eKind)
        {
          case VT_EVAL_DSP: /* dspacing */
            dspacing = lambda / (2.0 * sin(TwoTheta/2.0));
            for(iBin = 0; iBin < nBins; iBin++)
            {  if (BinPos[iBin] <= dspacing && dspacing < BinPos[iBin+1])
              {
                nTrjTot++;
                nTrj[iBin]++;
                IntBin[iBin] = IntBin[iBin] + prob;
                IntTot = IntTot + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case VT_EVAL_Q: /* q-range */
            qValue = (4.0*M_PI/lambda)*sin(TwoTheta/2.0);
            for(iBin = 0; iBin < nBins; iBin++)
            {  if (BinPos[iBin] <= qValue && qValue < BinPos[iBin+1])
              {
                nTrjTot++;
                nTrj[iBin]++;
                IntBin[iBin] = IntBin[iBin] + prob;
                IntTot = IntTot + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case VT_EVAL_ANGLE:  /* scattering angle */
            TwoThetaDeg = TwoTheta*180.0/M_PI;
            for(iBin = 0; iBin < nBins; iBin++)
            {  if (BinPos[iBin] <= TwoThetaDeg && TwoThetaDeg < BinPos[iBin+1])
              {
                nTrjTot++;
                nTrj[iBin]++;
                IntBin[iBin] = IntBin[iBin] + prob;
                IntTot = IntTot + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case VT_EVAL_LMBD: /* lambda-diff */
            DelLmbd = lambda - InputNeutrons[i].Wavelength;
            for(iBin = 0; iBin < nBins; iBin++)
            {  if (BinPos[iBin] <= DelLmbd && DelLmbd < BinPos[iBin+1])
              {
                nTrjTot++;
                nTrj[iBin]++;
                IntBin[iBin] = IntBin[iBin] + prob;
                IntTot = IntTot + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;
        }

        /* Writing out the neutrons that comply with the requirements and are within evaluation range,
        if 'exclusive counts = yes' is set */
        if (bExclCount==TRUE && bCounted==TRUE)
          WriteNeutron(&InputNeutrons[i]);
      }
    }
  }

// Finish: writes and closes evaluate files, writes to log and instrument file, frees memory
// -----------------------------------------------------------------------------------------
 my_exit:
  /* Output */
  fprintf(LogFilePtr, "Option: %s\n", sOption);
  fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", IntTot);

  // writes evaluation output
  UpdateMon(nBunches);

  if (fTotCounts != NULL && BinSize > 0)
  {
    if (fInfoFile == NULL)
    {
      Error("you must define a spectra information file to obtain integrated counts");
    }

    fprintf(LogFilePtr,"\n bin intervalls = integration range for each selected point:");

    l = 0;
    while(!feof(fInfoFile))
      if (2 == fscanf(fInfoFile, "%lf %lf", &center[l], &range[l]))
        l++;
      else
        break;
    fclose (fInfoFile);

    for (j=0; j < l; j++)
    {
      leftedge =  (long)floor( (center[j] - (range[j]/2.0) -MinX)/BinSize );
      rightedge = (long)floor( (center[j] + (range[j]/2.0) -MinX)/BinSize);

      fprintf(LogFilePtr,"\n [%ld, %ld]",leftedge, rightedge);

      for (k=leftedge; k<=rightedge; k++)
        totcenter[j] += IntBin[k];
    }

    /* writeout */
    for(j=0; j < l; j++)
    {
      if (fabs(totcenter[j]) < 1E-40) totcenter[j] = 0.0;
      fprintf(fTotCounts,"%7.7f\t%11.7E\n", center[j], totcenter[j]);
    }
    fclose(fTotCounts);
  }  /* end totcounts */

  /*Cleanup*/
  fprintf(LogFilePtr,"\n");
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  long   i=0;
  char*  arg=NULL;

  for(i=1; i<argc; i++)
  {
    arg = argv[i];
    if (*arg !='+')
    {
      arg += 2;
      switch(arg[-1])
      {
        case 'k':
          eKind = (VtEvalPar) atol(arg);     /* evaluation parameter */
          break;

        case 'o':                            /* output file containing spectrum */
          EvalFileName = arg;
          break;
        case 'O':                            /* optional: file containing integrated intensities */
          fTotCounts = OpenOutputFile(arg, FALSE, "w");
          if (fTotCounts==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for integrated output\n",arg);
            exit(-1);
          }
          break;
        case 'I':                            /* optional: info file for generating integrated intensities */
          fInfoFile = OpenParameterFile(arg, TRUE, "r");
          break;

        case 'n':
          nBins = atol(arg);                 /* number of bins */
          if (nBins > BINS)
          { fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS); exit(99);}
          break;
        case 'm':
          MinX = atof(arg);                  /* lower bound of d-spacing, q or theta range [A], [1/A], [deg]*/
          break;
        case 'M':
          MaxX = atof(arg);                  /* upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
          break;

        case 'R':
          LogProz    = atof(arg);            /* percentage of increase to next bin */
          if (LogProz!=0.0)
            bLogBinning = TRUE;
          break;
        case 'd':
          DeadSpot = M_PI*atof(arg)/180.0;   /* excludes all neutrons with a           */
          if (DeadSpot!=0.0)
            bDeadSpot = TRUE;                /* scattering angle < DeadSpot [deg] */
          break;
        case 'r':
          LmbdRef     = atof(arg);           /* reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument */
          break;

        case 'p':
          bProbactiv = (short) atoi(arg);    /* bProbactiv=1 means probabilities activated, else neutron weight is set to 1.0 */
          break;
        case 'c':
          bExclCount = (short) atoi(arg);    /* if activated, only neutrons complying with the evaluate requirements are considered further on */
          break;
        case 'w':
          bTOF       = (short) atoi(arg);    /* time of flight instrument -> wavelength from TOF */
          break;
        case 't':
          bPathCor   = (short) atoi(arg);    /*  correct flight path length for location of detection */
          break;

        case 'A':
          eScatAxis = (VtAxis) atoi(arg);
          if (eScatAxis!=Y_AXIS && eScatAxis!=Z_AXIS && eScatAxis!=NO_AXIS)
            Error2("Invalid scattering axis", arg);
          break;

        case 'l':
          TotLength = atof(arg);           /* length of neutron flight path [cm] */
          if (TotLength <= 0.0)
            Error("you must define a flight path > 0.0");
          break;
        case 'D':
          DetDist     = atof(arg);           /* length of neutron flight path [cm] */
          break;
        case 'T':
          TimeOffset  = atof(arg);           /* global shift of the neutron time t= t-TimeOffset [ms] */
          break;
        case 'e':
          EvalTimeMin = atof(arg);           /* minimal time for evaluation */
          break;
        case 'E':
          EvalTimeMax = atof(arg);           /* maximal time for evaluation */
          break;
        case 'C':
          nColour = atoi(arg);               /*  excludes all neutrons with diff. Colour, if nColour >= 0 */
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
          exit(-1);
          break;
      }
    }
  }

  /* checks */
  if (bLogBinning && MinX==0.0)
    Error("lower bound value must not be zero for logarithmic binning");
}


/********************************************************/
/** Allocates memory and initializes evaluation arrays **/
/********************************************************/
void InitArrays()
{
  long iBin;     // index of channel

  // initializes arrays
  memset(totcenter, 0,  NCENTER*sizeof(double));
  memset(center,    0,  NCENTER*sizeof(double));
  memset(range,     0,  NCENTER*sizeof(double));
  memset(BinPos,    0, (BINS+1)*sizeof(double));
  memset(IntBin,    0, (BINS+1)*sizeof(double));
  memset(nTrj,      0, (BINS+1)*sizeof(long));

  /* Construction of the Bins */
  /* logarithmic */
  if (bLogBinning)
  {
    BinPos[0] = MinX;

    for(iBin = 1; BinPos[iBin-1] < MaxX; iBin++)
    {
      BinPos[iBin] = BinPos[iBin-1] * (1.0 + LogProz/100.);
      IntBin [iBin] = 0.0;
      nTrj [iBin] = 0;
    }
  nBins = iBin-1;
  }
  /* linear */
  else
  {  BinSize = (MaxX - MinX) / (double)nBins;

    for(iBin = 0; iBin <= nBins; iBin++)
    {
      BinPos[iBin] = MinX + BinSize*iBin;
      IntBin[iBin] = 0.0;
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
  FILE*  pFile=NULL;             // pointer to output file

  pFile = OpenOutputFile(EvalFileName, TRUE, "wt");

  /* Spectrum */
  if (pFile != NULL)
  {
    WriteHeader1DB(pFile, TRUE, "intensity", ANY_COLOR, iBnch, nBunches, nBins, IntTot, nTrjTot,
                          sOption, sUnit[eKind], MinX, MaxX);

    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    for(iBin = 0; iBin < nBins; iBin++)
    {
      /* if (fabs(IntBin[iBin]) < 1E-40) IntBin[iBin] = 0.0; */
      if (bLogBinning)
        BinCtr = sqrt(BinPos[iBin]*BinPos[iBin+1]);
      else
        BinCtr = (BinPos[iBin]+BinPos[iBin+1])/2.0;

      if (nTrj[iBin] > 0)
        sigma = IntBin[iBin] * sqrt(1.0/(double)nTrj[iBin]);
      else
        sigma = 0.0;

      fprintf(pFile,"%10.6f  %12.5e %12.5e  %7ld\n", BinCtr,  f_norm*IntBin[iBin], f_norm*sigma, nTrj[iBin]);
    }
    fclose(pFile);
  }
}
