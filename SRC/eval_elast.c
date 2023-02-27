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

/************************************/
/** Definitions, structures, enums **/
/************************************/
#define BINS  10000
#define NCENTER 200


/*********************************/
/** Global Variables            **/
/*********************************/
// Input parameters
VtEvalPar eKind=VT_NO_EVAL;      // -k  [-]  evaluation parameter: d-spacing, momentum transfer, scattering angle or wavelength difference 

FILE  *fSpectra   = NULL,        // -o  [-]  output file containing the evaluated data
      *fTotCounts = NULL,        // -O  [-]  optional: file containing integrated intensities (see Help|evaluation) 
      *fInfoFile  = NULL;        // -I  [-]  optional: file controling the output of integrated intensities

long   nbins      = 0;           // -n  [-]  number of bins 

double MinX       = 0.0,         // -m [var] upper bound of d-spacing, q, theta or lambda range [Ang], [1/Ang], [deg]
       MaxX       = 0.0,         // -M [var] lower bound of d-spacing, q, theta or lambda range [Ang], [1/Ang], [deg]
       LogProz    = 0.0,         // -R  [%]  percentage of increase to next bin 
       DeadSpot   = 0.0,         // -d [deg] excludes all neutrons with a scattering angle < DeadSpot 
       LmbdRef    = 0.0;         // -r [Ang] reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */

short  bProbactiv = TRUE,        // -p  [-]  flag: TRUE: Probability weight   FALSE: number of trajectories neutron weight is set to 1.0 
       bExclCount = FALSE,       // -c  [-]  flag: TRUE: only neutrons complying with the evaluate requirements are written to the output 
       bTOF       = FALSE,       // -w  [-]  flag: TRUE: time of flight instrument 
       bPathCor   = FALSE;       // -t  [-]  flag: TRUE: correct TOF for real flight path from sample to detector 

VtAxis eScatAxis  = NO_AXIS;     // -A  [-]  direction of scattering for correct calculation of the scattering parameters 
                                 
double TotLength  = 0.0,         // -l [cm]  standard length of total neutron flight path
       DetDist    = 0.0,         // -D [cm]  sample detector distance                     
       TimeOffset = 0.0,         // -T [ms]  global shift of the neutron time t' = t-TimeOffset
       EvalTimeMin=-1.0e10,      // -e [ms]  minimaltime for evaluation
       EvalTimeMax= 1.0e10;      // -E [ms]  maximal time for evaluation
int    nColour    = ANY_COLOR;   // -C  [-]  colour necessary for the trajectory to be regarded, colour=-1(ANY_COLOR) means: all trajectories are regarded  

// Variables determined from input parameters or trajectory data
short  bLogBinning=FALSE,        //          flag: TRUE : binning increases exponentially    FALSE: linear binning  
       bDeadSpot  =FALSE;        //          flag: TRUE : deadspot exists 


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  char   sOption[25]="";
  short  bCounted   =FALSE;      // TRUE : neutron is counted, intensity added to channel and total intensity 
  int    ibin=0;

  long   i=0, j=0, k=0, 
         bcnt [BINS+1],          /* number of trajectories contributing to count rate */
         leftedge=0, rightedge=0;

  double bintc    =0.0, 
         binterval=1.0,
         bpost [BINS+1],         /* limits of the bins                                */
         bint  [BINS+1],         /* count rate of a bin                               */
         center[NCENTER], totcenter[NCENTER], range[NCENTER],
         time=0.0, lambda=0.0, 
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
  PrintModuleName(_eModule, "1.9a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  EvalPar_ID2Txt(sOption, eKind); 
  fprintf(LogFilePtr, "Option: %s\n", sOption); 

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
    bpost[0] = MinX;

    for(ibin = 1; bpost[ibin-1] < MaxX; ibin++)
    {
      bpost[ibin] = bpost[ibin-1] * (1.0 + LogProz/100.);
      bint [ibin] = 0.0;
      bcnt [ibin] = 0;
    }
  nbins = ibin-1;
  }
  /* linear */
  else
  {	binterval = (MaxX - MinX) / (double)nbins;
		
    for(ibin = 0; ibin<=nbins; ibin++)
    {
      bpost[ibin] = MinX + binterval*ibin;
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
          Phi	     = (double) atan2(InputNeutrons[i].Vector[2], InputNeutrons[i].Vector[1]);
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

          case VT_EVAL_Q: /* q-range */
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

          case VT_EVAL_ANGLE:	/* scattering angle */
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

          case VT_EVAL_LMBD: /* lambda-diff */
            DelLmbd = lambda - InputNeutrons[i].Wavelength;
            for(ibin = 0; ibin<nbins; ibin++)
            {	if (bpost[ibin] <= DelLmbd && DelLmbd < bpost[ibin+1])
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
      leftedge =  (long)floor( (center[j] - (range[j]/2.0) -MinX)/binterval );
      rightedge = (long)floor( (center[j] + (range[j]/2.0) -MinX)/binterval);
	  				  
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
          fSpectra = OpenOutputFile(arg, FALSE, "w");
          if (fSpectra ==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for spectra output\n",arg);
            exit(-1);
          }
          break;
        case 'O':                            /* optional: file containing integrated intensities */
          fTotCounts = OpenOutputFile(arg, FALSE, "w");
          if (fTotCounts==NULL)
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for integrated output\n",arg);
            exit(-1);
          }
          break;
        case 'I':                            /* optional: info file for generating integrated intensities */
          fInfoFile = OpenInputFile(arg, TRUE, "r");
          break;

        case 'n':
          nbins = atol(arg);                 /* number of bins */
          if (nbins > BINS)
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
          bTOF       = (short) atoi(arg);    /* time of flight instrument */
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

