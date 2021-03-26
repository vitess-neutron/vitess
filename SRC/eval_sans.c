/*********************************************************************************************/
/*  VITESS module EVAL_ELAST_SANS                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Nov 2011  K. Lieutenant  initial version                                             */
/* 1.1  Nov 2013  K. Lieutenant  flight path correction                                      */
/* 1.2  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
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
FILE *pSpectrum=NULL,        // pointer to the file containing the SANS spectrum
     *pReference=NULL;       // pointer to the file containing the reference (=isotropic scattering) spectrum

int   bTOF       =FALSE,     /* TRUE : time of flight instrument */
      bDeadSpot  =FALSE,     /* TRUE : deadspot exists */
			bPathCor   =FALSE,     /* TRUE : correct TOF for real flight path from sample to detector */
      bLogBinning=FALSE;     /* TRUE : binning increases exponentially 
                                FALSE: linear binning                  */

long  nbins,                 /* number of bins */
      nColour=ANY_COLOR;     /* colour necessary for the trajectory to be regarded
                                colour ANY_COLOR (=-1) means: all trajectories are regarded  */

double referenceWavelength,  /* reference Wavelength for crystal monochromator (or mechanical velocity
                                  selector) instrument                                                 */
       deadspotangle=0,      /* excludes all neutrons with a scattering angle < deadspotangle [deg] */
       Flightpath0=0.0,      /* standard length of neutron flight path [cm] */
       DetDist=0.0,          /* detector distance             [cm] */
       TimeOffset=0,         /* global shift of the neutron time t= t-TimeOffset [ms] */
       Qmin,Qmax,            /* lower and upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
			 ProbScat,             /* scattering probability of the isotropic scatterer */
       dLogProz=0.0,         /* percentage of increase to next bin      */
       dDelLambda,           /* difference between wavelength calculated from TOF and true wavelength  */
       dEvalTimeMin=-1.0e10, /* minimal and maximal time for evaluation */
       dEvalTimeMax= 1.0e10;



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
  short  bWrite=FALSE,       /* criterion: write data point to output  */
         bRefFile=FALSE;     /* criterion: reference file available    */

  int    ibin,               /* index for the bins                     */
         nSpec=0;            /* number of calculated S-values          */

  long  i, 
        bcnt[BINS+1];       /* number of trajectories contributing to count rate */

  double bintc=0.0,
         binterval=1.0,
         bpost[BINS+1],      /* limits of the bins                     */
         bint [BINS+1],      /* count rate of a bin                    */
         rmid [BINS+1],      /* centers of the bins in the ref. file   */
         rint [BINS+1],      /* count rate of the reference file       */
         time,               /* TOF                                    */
         lambda,             /* wavelngth of the neutron               */
         TwoTheta,           /* scattering angle theta = 2 theta_bragg */
         Phi,                /* scattering direction phi               */
         Q,                  /* Q value of the scattering              */
         Svalue,             /* value S(Q)                             */
         prob=0,             /* weight of the trajectory               */
         Flightpath=0.0,     // real length of neutron flight path [cm] 
         DetPath=0.0;        // path length from sample to position of detection


  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_SANS;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.2");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  memset(bpost, 0, (BINS+1)*sizeof(double));
  memset(bint,  0, (BINS+1)*sizeof(double));
  memset(rmid,  0, (BINS+1)*sizeof(double));
  memset(rint,  0, (BINS+1)*sizeof(double));

  // Read reference spectrum
  bRefFile=ReadRefSpec(rmid, rint);
  if (bRefFile==FALSE)
    Warning("reference spectrum could not be read, constant value of 1 assumed");

  /* Construction of the Bins */
  /* logarithmic */
  if (bLogBinning)
  {	
    bpost[0] = Qmin;
    bint [0] = 0.0;
    bcnt [0] = 0;

    for(ibin = 1; bpost[ibin-1] < Qmax; ibin++)
    {
      bpost[ibin] = bpost[ibin-1] * (1.0 + dLogProz/100.);
      bint [ibin] = 0.0;
      bcnt [ibin] = 0;
    }
    nbins = ibin-1;
  }
  /* linear */
  else
  {	binterval = (Qmax - Qmin) / (double)nbins;
		
    for(ibin = 0; ibin<=nbins; ibin++)
    {
      bpost[ibin] = Qmin + binterval*ibin;
      bint [ibin] = 0.0;
      bcnt [ibin] = 0;
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
        lambda   = bTOF ? 395.60346/(Flightpath/time) : referenceWavelength;

        /* Writing out all neutrons, if 'exclusive counts = no' is set */
        // if (bExclCount==FALSE)		
        WriteNeutron(&InputNeutrons[i]);

        /* trajectories within deadspot */
        if (bDeadSpot && TwoTheta <= deadspotangle) continue;

        /* traj. out of time of evaluation */
        if (time < dEvalTimeMin || time > dEvalTimeMax) continue;

        /* exclude traj. with wrong colour: (nColour=-1 means: all colours accepted) */
        if (nColour!=ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;

        /* Writing out the neutrons that comply with the requirements, 
        if 'exclusive counts = yes' is set */
        /* if (bExclCount==TRUE)		
	        WriteNeutron(&InputNeutrons[i]); */

        Q = 4.0 * M_PI * sin(TwoTheta/2.0) / lambda;

        for(ibin = 0; ibin<nbins; ibin++)
        {	if (bpost[ibin] <= Q && Q < bpost[ibin+1])
          {
            bcnt[ibin]++;
            bint[ibin] = bint[ibin] + prob;
            bintc = bintc + prob;
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
  fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", bintc);

  /* Spectrum */
  if (pSpectrum != NULL)
  {
    double bmid;

    for(ibin = 0; ibin < nbins; ibin++)
    {	
      /* if (fabs(bint[ibin]) < 1E-40)
      bint[ibin] = 0.0; */
      if (bLogBinning)
        bmid = sqrt(bpost[ibin]*bpost[ibin+1]);
      else
        bmid = (bpost[ibin]+bpost[ibin+1])/2.0;

      if (!bRefFile)
      { rmid[ibin] = bmid;
        rint[ibin] = 1.0;
        bWrite     = TRUE;
      } 
      else
      {
        bWrite = (bint[ibin] > 0.0 && rint[ibin] > 0.0);
      }

      if (RoundP(bmid,6)==RoundP(rmid[ibin],6))
      { 
        nSpec++;
        if (bWrite)
        {
          Svalue = bint[ibin]/(rint[ibin]/ProbScat);
          fprintf(pSpectrum,"%12g %12g %7ld\n", bmid, Svalue, bcnt[ibin]);
        }
      }
    }
    fclose(pSpectrum);
    if (nSpec==0)
      Error ("no agreement in binning between reference spectrum and SANS spectrum");
  }
  
  /*Cleanup*/
  fprintf(LogFilePtr,"\n");
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

  if (pReference != NULL)
  {
    while (ReadLine(pReference, sLine, sizeof(sLine)-1))
    { StrgScanLF(sLine, TabVal, 6, 0);
      pRefBin[iBin] = TabVal[0];
      pRefVal[iBin] = TabVal[1];
      iBin++;
    }

    if (iBin > 0)
      rc=TRUE;

    fclose(pReference);
  }
  return rc;
}
      

/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  long   i;
  double winp;
  char * arg;

  for(i=1; i<argc; i++) 
  {
    arg = argv[i];
    if (*arg !='+') 
    {
      arg += 2;
      switch(arg[-1]) 
      {
        case 'S':
          pSpectrum = OpenOutputFile(arg, FALSE, "w");
          if (pSpectrum==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for spectra output\n",arg);
            exit(-1);
          }
          break;
        case 'I':
          pReference = OpenInputFile(arg, FALSE, "r");
          break;

        case 'n':
          nbins = atol(arg); /* number of bins */
          if (nbins > BINS)
          { fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
            exit(99);
          }
          break;

        case 'w':
          winp = atof(arg);
          if (winp == 1.0) bTOF = TRUE; /* time of flight instrument */
          break;

        case 'r':
          referenceWavelength = atof(arg); /* reference Wavelength for crystal monochromator */
          break;                           /* (or mechanical velocity selector) instrument   */

        case 'e':
          dEvalTimeMin = atof(arg);        /* minimal time for evaluation */
          break;
        case 'E':
          dEvalTimeMax = atof(arg);             /* maximal time for evaluation */
          break;

        case 'C':
          nColour = atol(arg);                  /*  excludes all neutrons with diff. Colour, if nColour >= 0 */
          break;

        case 'd':
          deadspotangle = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
          bDeadSpot = TRUE;                /* scattering angle < deadspotangle [deg] */
          break;

        case 'p':
          ProbScat = atof(arg);                 /* scattering probability of the isotropic scatterer */
          break;

        case 'm':
          Qmin = atof(arg);                     /* lower bound of Q range [1/A] */
          break;
        case 'M':
          Qmax = atof(arg);                     /* upper bound of Q range [1/A] */
          break;

        case 'R':
          dLogProz    = atof(arg);              /* percentage of increase to next bin */
          if (dLogProz!=0.0)
          bLogBinning = TRUE;
          break;

        case 'l':
          Flightpath0 = atof(arg);  /* length of neutron flight path [cm] */
          if (Flightpath0 <= 0.0)
          Error("you must define a flight path > 0.0");
          break;

        case 'L':
          DetDist = atof(arg);  /* length of neutron flight path [cm] */
          break;

        case 't':
          bPathCor = atol(arg);     /*  correct flight path length for location of detection */
          break;

        case 'T':
          TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
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
}

