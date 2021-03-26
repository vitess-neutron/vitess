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
/* 1.6  Feb 2004  K. Lieutenant  'FullParName' + ERROR included; check of 'kind' out of loop */
/* 1.7  Nov 2005  K. Lieutenant  transformation direction -> scattering angles added         */
/* 1.7a Jun 2009  A. Houben      increased NCENTER from 100 to 200                           */
/* 1.8  Apr 2013  K. Lieutenant  flight path correction                                      */
/* 1.9  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
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
#define BINS  10000
#define NCENTER 200


/*********************************/
/** Global Variables            **/
/*********************************/
FILE  *fSpectra=NULL, 
      *fTotCounts=NULL, 
      *fInfoFile=NULL;;

int    bProbactiv =TRUE,      /* bProbactiv=1 means probabilities activated, else neutron weight is set to 1.0         */
       bTOF       =FALSE,     /* TRUE : time of flight instrument */
       bDeadSpot  =FALSE,     /* TRUE : deadspot exists */
       bPathCor   =FALSE,     /* TRUE : correct TOF for real flight path from sample to detector */
       bExclCount =FALSE,     /* TRUE : only neutrons complying with the evaluate requirements are written to the output      */
       bCounted   =FALSE,     /* TRUE : neutron is counted, intensity added to channel and total intensity */
       bLogBinning=FALSE;     /* TRUE : binning increases exponentially    FALSE: linear binning                  */

int    scatterAxis = -1;      /* Direction of scattering for correct calculation of scattering parameters */

long   nbins=0,               /* number of bins */
       nColour=ANY_COLOR,     /* colour necessary for the trajectory to be regarded
                                 colour=-1(ANY_COLOR) means: all trajectories are regarded  */
       kind=0;                /* 1= d-spacing; 2=momentum transfer q; 3=scattering angle */

double referenceWavelength,   /* reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */
       deadspotangle=0.0,     /* excludes all neutrons with a scattering angle < deadspotangle [deg] */
       Flightpath0=0.0,       /* standard length of neutron flight path [cm] */
       DetDist=0.0,           /* detector distance             [cm] */
       TimeOffset=0.0,        /* global shift of the neutron time t= t-TimeOffset [ms] */
       m=0.1, M=10.0,         /* lower and upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
       dLogProz=0.0,          /* percentage of increase to next bin      */
       dDelLambda,            /* difference between wavelength calculated from TOF and true wavelength  */
       dEvalTimeMin=-1.0e10,  /* minimal and maximal time for evaluation */
       dEvalTimeMax= 1.0e10;


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  int    ibin=0;

  long   i=0, j=0, k=0, 
         bcnt [BINS+1],          /* number of trajectories contributing to count rate */
         leftedge=0, rightedge=0;

  double bintc    =0.0, 
         binterval=1.0,
         bpost [BINS+1],         /* limits of the bins                                */
         bint  [BINS+1],          /* count rate of a bin                               */
         center[NCENTER], totcenter[NCENTER], range[NCENTER],
         time=0.0, lambda=0.0, 
         TwoTheta=0.0, TwoThetaDeg=0.0, Phi=0.0, 
         qValue=0.0, dspacing=0.0, 
         prob  =0.0,
         Flightpath=0.0,        // real length of neutron flight path [cm] 
         DetPath=0.0;           // path length from sample to position of detection


  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_ELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.9");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  switch (kind) 
  {
    case 1: fprintf(LogFilePtr, "Option: d-spacing\n"); break;
    case 2: fprintf(LogFilePtr, "Option: momentum transfer Q\n"); break;
    case 3: fprintf(LogFilePtr, "Option: scattering angle\n");break;
    case 4: fprintf(LogFilePtr, "Option: wavelength difference\n");break;
    default:Error("Wrong value for evaluation parameter\n");
  }

  // initializes arrays
  memset(totcenter, 0,  NCENTER*sizeof(double));
  memset(center,    0,  NCENTER*sizeof(double));
  memset(range,     0,  NCENTER*sizeof(double));
  memset(bpost,     0, (BINS+1)*sizeof(double));
  memset(bint,      0, (BINS+1)*sizeof(double));
  memset(bcnt,      0, (BINS+1)*sizeof(long));

  /* Construction of the Bins */
  /* logarithmic */
  if (bLogBinning)
  {	
    bpost[0] = m;

    for(ibin = 1; bpost[ibin-1] < M; ibin++)
    {
      bpost[ibin] = bpost[ibin-1] * (1.0 + dLogProz/100.);
      bint [ibin] = 0.0;
      bcnt [ibin] = 0;
    }
  nbins = ibin-1;
  }
  /* linear */
  else
  {	binterval = (M - m) / (double)nbins;
		
    for(ibin = 0; ibin<=nbins; ibin++)
    {
      bpost[ibin] = m + binterval*ibin;
      bint [ibin] = 0.0;
      bcnt [ibin] = 0;
    }
  }

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
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        bCounted=FALSE;

        /* Writing out all neutrons, if 'exclusive counts = no' is set */
        if (bExclCount==FALSE)		
        WriteNeutron(&InputNeutrons[i]);

        /* exclusion of traj. with wrong colour: (nColour=-1 means: all colours accepted) */
        if (nColour!=ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;

        // determination of scattering angle
        if (scatterAxis == 1) 
        {
          /* Neutron temp = InputNeutrons[i]; */
          /* temp.Vector[0] = sqrt(sq(temp.Vector[0]) + sq(temp.Vector[2])); */
          /* CartesianToSpherical(temp.Vector, &TwoTheta, &Phi); */
          TwoTheta = (double) atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
          Phi	= (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[1]);
        }
        else if (scatterAxis == 2) 
        {
          /* Neutron temp = InputNeutrons[i]; */
          /* temp.Vector[0] = sqrt(sq(temp.Vector[0]) + sq(temp.Vector[1])); */
          /* CartesianToSpherical(temp.Vector, &TwoTheta, &Phi); */
          TwoTheta = (double) atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
          Phi	= (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[1]);
        }
        else 
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

        // determination of weight and wavelength
        prob     = bProbactiv ? InputNeutrons[i].Probability : 1.0;
        time     = InputNeutrons[i].Time - TimeOffset;
        lambda   = bTOF ? 395.60346/(Flightpath/time) : referenceWavelength;

        /* trajectories within deadspot */
        if (bDeadSpot && TwoTheta <= deadspotangle) continue;

        /* traj. out of time of evaluation */
        if (time < dEvalTimeMin || time > dEvalTimeMax) continue;

        switch (kind) 
        {
          case 1: /* dspacing */
            dspacing = lambda / (2.0 * sin(TwoTheta/2.0));
            for(ibin = 0; ibin<nbins; ibin++)
            {	if (bpost[ibin] <= dspacing && dspacing < bpost[ibin+1])
              {
                bcnt[ibin]++;
                bint[ibin] = bint[ibin] + prob;
                bintc = bintc + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case 2: /* q-range */
            qValue = (4.0*M_PI/lambda)*sin(TwoTheta/2.0);
            for(ibin = 0; ibin<nbins; ibin++)
            {	if (bpost[ibin] <= qValue && qValue < bpost[ibin+1])
              {
                bcnt[ibin]++;
                bint[ibin] = bint[ibin] + prob;
                bintc = bintc + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case 3:	/* scattering angle */
            TwoThetaDeg = TwoTheta*180.0/M_PI;
            for(ibin = 0; ibin<nbins; ibin++)
            {	if (bpost[ibin] <= TwoThetaDeg && TwoThetaDeg < bpost[ibin+1])
              {
                bcnt[ibin]++;
                bint[ibin] = bint[ibin] + prob;
                bintc = bintc + prob;
                bCounted=TRUE;
                break;
              }
            }
            break;

          case 4: /* lambda-diff */
            dDelLambda = lambda - InputNeutrons[i].Wavelength;
            for(ibin = 0; ibin<nbins; ibin++)
            {	if (bpost[ibin] <= dDelLambda && dDelLambda < bpost[ibin+1])
              {
                bcnt[ibin]++;
                bint[ibin] = bint[ibin] + prob;
                bintc = bintc + prob;
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
  /* Output of Results */
  fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", bintc);

  /* Spectrum */
  if (fSpectra != NULL)
  {
    double bmid;

    for(ibin = 0; ibin<(nbins); ibin++)
    {	
      /* if (fabs(bint[ibin]) < 1E-40)
      bint[ibin] = 0.0; */
      if (bLogBinning)
      bmid = sqrt(bpost[ibin]*bpost[ibin+1]);
      else
      bmid = (bpost[ibin]+bpost[ibin+1])/2.0;
      fprintf(fSpectra,"%12g %12g %7ld\n", bmid, bint[ibin], bcnt[ibin]);
    }
    fclose(fSpectra);
  }

  if (fTotCounts != NULL && binterval > 0)
  { 
    if (fInfoFile == NULL)
    {
      Error("you must define a spectra information file to obtain integrated counts");
    }

    fprintf(LogFilePtr,"\n binintervalls = integration range for each selected point:");

    i = 0;
    while(!feof(fInfoFile))
    if (2 == fscanf(fInfoFile, "%lf %lf", &center[i], &range[i]))
    i++;
    else
    break;
    fclose (fInfoFile);

    for (j=0; j<i; j++)
    {
      leftedge =  (long)floor( (center[j] - (range[j]/2.0) -m)/binterval );
      rightedge = (long)floor( (center[j] + (range[j]/2.0) -m)/binterval);
	  				  
      fprintf(LogFilePtr,"\n [%ld, %ld]",leftedge, rightedge);
				  
      for (k=leftedge; k<=rightedge; k++)
        totcenter[j] += bint[k];
    }

    /* writeout */
    for(j = 0; j<i; j++)
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
        case 'o':
          fSpectra = OpenOutputFile(arg, FALSE, "w");
          if (fSpectra ==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for spectra output\n",arg);
            exit(-1);
          }
          break;
					  
        case 'O':
          fTotCounts = OpenOutputFile(arg, FALSE, "w");
          if (fTotCounts==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for integrated output\n",arg);
            exit(-1);
          }
          break;

        case 'I':
          /* info file for generating integrated output */
          fInfoFile = OpenInputFile(arg, TRUE, "r");
          break;

        case 'n':
          nbins = atol(arg); /* number of bins */
          if (nbins > BINS)
          { fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
            exit(99);
          }
          break;

        case 'k':
          kind = atol(arg); /* 1= d-spacing; 2=momentum transfer q; 3=scattering angle */
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
          dEvalTimeMax = atof(arg);        /* maximal time for evaluation */
          break;


        case 'C':
          nColour = atol(arg);       /*  excludes all neutrons with diff. Colour, if nColour >= 0 */
          break;

        case 'd':
          deadspotangle = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
          bDeadSpot = TRUE;                /* scattering angle < deadspotangle [deg] */
          break;


        case 'm':
          m = atof(arg);   /* lower bound of d-spacing, q or theta range [A], [1/A], [deg]*/
          break;

        case 'M':
          M = atof(arg);   /* upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
          break;

        case 'R':
          dLogProz    = atof(arg);       /* percentage of increase to next bin */
          if (dLogProz!=0.0)
            bLogBinning = TRUE;
          break;


        case 'c':
          if(atol(arg)==1)        /* if activated, only neutrons complying with the  */
          bExclCount = TRUE;    /* evaluate requirements are considered further on */
          break;


        case 't':
          bPathCor = atol(arg);     /*  correct flight path length for location of detection */
          break;

        case 'l':
          Flightpath0 = atof(arg);  /* length of neutron flight path [cm] */
          if (Flightpath0 <= 0.0)
            Error("you must define a flight path > 0.0");
          break;

        case 'D':
          DetDist = atof(arg);  /* length of neutron flight path [cm] */
          break;

        case 'T':
          TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
          break;

        case 'p':
          bProbactiv = atoi(arg);
          /* bProbactiv=1 means probabilities activated, else neutron weight is set to 1.0 */
          break;

        case 'A':
          scatterAxis = atoi(arg);
          if (scatterAxis > 2) 
            Error("ERROR: invalid scattering axis!!!");
          break;
					
        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
          exit(-1);
          break;
      }
    }
  }

  /* checks */
  if (bLogBinning && m==0.0)
    Error("lower bound value must not be zero for logarithmic binning"); 
}

