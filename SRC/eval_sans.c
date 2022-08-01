/*********************************************************************************************/
/*  VITESS module EVAL_ELAST_SANS                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Nov 2011  K. Lieutenant  initial version                                             */
/* 1.1  Nov 2013  K. Lieutenant  flight path correction                                      */
/* 1.2  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/* 1.3  Dec 2021  K. Lieutenant  separation of S(Q) and I(Q)                                 */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "general.h"
#include "matrix.h"
#include "softabort.h"


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

short  bTOF       = FALSE,     // -w   [-]   TRUE : time of flight instrument 
			 bPathCor   = FALSE;     // -t   [-]   TRUE : correct TOF for real flight path from sample to detector 

long   nBins      = 0;         // -n   [-]   number of bins 
double Qmin       = 0.0,       // -m [1/Ang] lower bound of rhe evaluated q range 
       Qmax       = 0.0,       // -M [1/Ang] upper bound of the evaluated q range 
			 LogProz    = 0.0,       // -R   [%]   percentage of increase to next bin 
       DeadSpot   = 0.0,       // -d  [deg]  excludes all neutrons with a scattering angle < DeadSpot 
       LmbdRef    = 0.0,       // -r  [Ang]  reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */
       ProbScat   = 0.0,       // -p   [-]   scattering probability of the isotropic scatterer

       Flightpath0= 0.0,       // -l  [ms]   standard length of neutron flight path 
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
FILE  *pIntFile   = NULL,      //      [-]   name of the file containing the intensity spectrum
      *pSofQFile  = NULL,      //      [-]   name of the file containing the S(Q) spectrum
      *pRefFile   = NULL;      //      [-]   name of the file containing the reference (=isotropic scattering) spectrum


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit    (int argc, char *argv[]);            // Reads input parameters and sets global variables
short ReadRefSpec(double* pRefBin, double* pRefVal);  // function to read reference spectrum 


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bWrite  =FALSE;     /* criterion: write data point to output  */

  int    ibin =0,            /* index for the bins                     */
         nSpec=0;            /* number of calculated S-values          */

  long  i=0,                 /* trajectory index */
        nTraj[BINS+1];        /* number of trajectories contributing to count rate */

  double IntTot  =0.0,       /* total intensity with in bin range      */
         BinWidth=1.0,       /* bin width for q linear scale in Q      */
         BinLmts[BINS+1],    /* limits of the bins                     */
         IntSmpl[BINS+1],    /* count rate of a bin                    */
         QRef   [BINS+1],    /* centers of the Q bins in the ref. file */
         IntRef [BINS+1],    /* count rate of the reference file       */
         time    =0.0,       /* TOF                                    */
         lambda  =0.0,       /* wavelngth of the neutron               */
         TwoTheta=0.0,       /* scattering angle theta = 2 theta_bragg */
         Phi     =0.0,       /* scattering direction phi               */
         Q       =0.0,       /* Q value of the scattering              */
         Svalue  =0.0,       /* value S(Q)                             */
         prob    =0.0,       /* weight of the trajectory               */
         Flightpath=0.0,     // real length of neutron flight path [cm] 
         DetPath =0.0,       // path length from sample to position of detection
         Qcntr;              // Q value calculated from the limits of the bin


  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_SANS;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  memset(BinLmts, 0, (BINS+1)*sizeof(double));
  memset(IntSmpl, 0, (BINS+1)*sizeof(double));
  memset(QRef,    0, (BINS+1)*sizeof(double));
  memset(IntRef,  0, (BINS+1)*sizeof(double));

  // Read reference spectrum
  if (bAllFiles)
  { bSofQ=ReadRefSpec(QRef, IntRef);
    if (bSofQ==FALSE)
      Warning("reference spectrum could not be read, S(Q) file cannot be written");
  }

  /* Construction of the Bins */
  /* logarithmic */
  if (bLogBinning)
  {	
    BinLmts[0] = Qmin;
    IntSmpl[0] = 0.0;
    nTraj  [0] = 0;

    for(ibin = 1; BinLmts[ibin-1] < Qmax; ibin++)
    {
      BinLmts[ibin] = BinLmts[ibin-1] * (1.0 + LogProz/100.);
      IntSmpl[ibin] = 0.0;
      nTraj  [ibin] = 0;
    }
    nBins = ibin-1;
  }
  /* linear */
  else
  {	BinWidth = (Qmax - Qmin) / (double)nBins;
		
    for(ibin=0; ibin <= nBins; ibin++)
    {
      BinLmts[ibin] = Qmin + BinWidth*ibin;
      IntSmpl[ibin] = 0.0;
      nTraj  [ibin] = 0;
    }
  }

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
        {	if (BinLmts[ibin] <= Q && Q < BinLmts[ibin+1])
          {
            nTraj  [ibin]++;
            IntSmpl[ibin] = IntSmpl[ibin] + prob;
            IntTot = IntTot + prob;
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
  fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", IntTot);

  /* writes spectra */

  for(ibin = 0; ibin < nBins; ibin++)
  {	
    if (bLogBinning)
      Qcntr = sqrt(BinLmts[ibin]*BinLmts[ibin+1]);
    else
      Qcntr = 0.5*(BinLmts[ibin]+BinLmts[ibin+1]);

    // writes intensity file
    fprintf(pIntFile, "%12g %12g %7ld\n", Qcntr, IntSmpl[ibin], nTraj[ibin]);

    // writes S(Q) file if Q values agree and both values ar non-zero
    if (bSofQ)
    {
      bWrite = (IntSmpl[ibin] > 0.0 && IntRef[ibin] > 0.0);

      if (RoundP(Qcntr,6)==RoundP(QRef[ibin],6))
      { 
        nSpec++;
        if (bWrite)
        {
          Svalue = ProbScat * IntSmpl[ibin] / IntRef[ibin];
          fprintf(pSofQFile, "%12g %12g %7ld\n", Qcntr, Svalue, nTraj[ibin]);
        }
      }
    }
  }
  if (nSpec==0)
    Warning("no agreement in binning between reference and current spectrum - no S(Q) file generated");
  
  // Cleanup
  fprintf(LogFilePtr,"\n");

  if (pIntFile !=NULL) fclose(pIntFile );
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
          bDeadSpot = TRUE;                /* scattering angle < DeadSpot [deg] */
          break;
        case 'r':
          LmbdRef   = atof(arg); /* reference Wavelength for crystal monochromator */
          break;                           /* (or mechanical velocity selector) instrument   */
        case 'p':
          ProbScat  = atof(arg);                 /* scattering probability of the isotropic scatterer */
          break;

        case 'w':
          bTOF     = (short) atoi(arg); ; /* time of flight instrument */
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
  if      (pIntFileName==NULL && pSofQFileName==NULL)
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
      pRefFile  = OpenInputFile (pRefFileName,  FALSE, "r");
    }
    if (pSofQFile==NULL || pRefFile==NULL)
      Warning("S(Q) or reference file could not be opened. S(Q) file cannot be generated");
    else
      bAllFiles=TRUE;
  }
}

