/*********************************************************************************************/
/*  VITESS module EVAL_ELAST2                                                                */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Jun 2009  A. Houben      Copy of EVAL_ELAST 1.7a in order to do 3D analysis          */
/* 1.1  May 2013  A. Houben      Sample-Detector distance                                    */
/* 1.2  Aug 2014  A. Houben      Allow output of zero entries in output (helps with MatLab)  */
/* 1.3  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/*********************************************************************************************/
// --Z1 --U1.0e-25 --G1 --B10000 --PC:/Users/ahouben/Documents/POWTEX/Berechnung/mcPOWplot/090113-11_FS_Detector --LC:/Users/ahouben/Documents/POWTEX/Berechnung/mcPOWplot/090113-11_FS_Detector/vpipelog15 -k1 -oC:/Users/ahouben/Documents/POWTEX/Berechnung/mcPOWplot/090113-11_FS_Detector/elast_sca2.eva -OC:/Users/ahouben/Documents/POWTEX/Berechnung/mcPOWplot/090113-11_FS_Detector/elast_sca2.int -IC:/Users/ahouben/Documents/POWTEX/Berechnung/mcPOWplot/090113-11_FS_Detector/elast_sca.inf -n146 -m100 -x0 -X180 -y1.0 -Y4.8 -p1 -w1 -c0 -l4351.4 -T0 -e-1.e10 -E1.e10 -C0 --Fno_file --fC:\Users\ahouben\Documents\POWTEX\Berechnung\mcPOWplot\090113-11_FS_Detector\detector.out -s3 -L80 -D1 -f1

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
#define INDEX(x,y) (x*(nbinsY)+y)

typedef struct
{
	double X;
	double Y;
	double Int;
	long   Counts;
} BINDATA;


/*********************************/
/** Global Variables            **/
/*********************************/
FILE  *fSpectra=NULL;

int    bProbActive=TRUE,     /* bProbActive=1 means probabilities activated, else neutron weight is set to 1.0         */
       bTOF       =FALSE,    /* TRUE : time of flight instrument */
       bDeadSpot  =FALSE,    /* TRUE : deadspot exists */
       bTOFcorr = FALSE,     /* TRUE : correct time to shortest detector distance */
       bScatAng = FALSE,     /* TRUE : position information is used (needs more information)
                                FALSE: direction cosine is used */
       bExclCount =FALSE,    /* TRUE : only neutrons complying with the evaluate requirements are written to the output      */
       bLogBinningX=FALSE,   /* TRUE: binning increases exponentially 
                                FALSE: linear binning                  */
       bLogBinningY=FALSE,   /* TRUE: binning increases exponentially 
                                FALSE: linear binning                  */
       bFullMatrix=FALSE,    /* TRUE: Also lines with zero intensity/counts are written; needs more memory 
                                FALSE: Default: only write non-zero lines */
	     bSortMode = 0;			   /* 0=No sorting, 1=Sort by X, 2=Y, 3=Intensity, 4=Counts; <0 for reverse */

long   nbinsX,               /* number of bins in X */
       nbinsY,               /* number of bins in Y */
       nColour = -1,         /* colour necessary for the trajectory to be regarded  colour -1 means: all trajectories are regarded  */
       minColor = -1,        /* colour necessary for the trajectory to be regarded  colour -1 means: all trajectories are regarded  
                                use neutrons with color >= minColour */
       maxColor = -1,        /* colour necessary for the trajectory to be regarded  colour -1 means: all trajectories are regarded  
                                use neutrons with color <= maxColour */
       kind;                 /* 1=scattering angle and wavelength; 2=scattering angle and TOF */

double referenceWavelength,  /* reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */
       deadspotangle=0,      /* excludes all neutrons with a scattering angle < deadspotangle [deg] */
       Flightpath=0,         /* length of neutron flight path [cm] */
       sdpath=0,             /* shortest sample detector distance [cm] */
       TimeOffset=0,         /* global shift of the neutron time t= t-TimeOffset [ms] */
       x,X,                  /* lower and upper bound of d-spacing, q or theta range [A], [1/A], [deg] ==> X */
       y,Y,                  /* lower and upper bound of d-spacing, q or theta range [A], [1/A], [deg] ==> Y */
       dLogProzX=0.0,        /* percentage of increase to next bin      */
       dLogProzY=0.0,        /* percentage of increase to next bin      */
       dDelLambda,           /* difference between wavelength calculated from TOF and true wavelength  */
       dEvalTimeMin=-1.0e10, /* minimal and maximal time for evaluation */
       dEvalTimeMax= 1.0e10;

double *bpostX = {NULL};     /* limits of the bins                                           */
double *bpostY = {NULL};     /* limits of the bins                                           */
BINDATA **bin = {NULL};      /* Counts = count rate of a bin                                 */
                             /* number of trajectories contributing to count rate            */
BINDATA **bin_sorted={NULL}; /* pointers to BINDATA of bin; array size dynamically allocated */


/******************************/
/** Prototypes               **/
/******************************/
void CreateBin(BINDATA **bin, double *bpostX, double *bpostY);
int FindIndexXY(double *Xval, double *Yval, int *ibinX, int *ibinY);
void OwnInit   (int argc, char *argv[]);
int comparebinX(const void *a, const void *b);
int comparebinY(const void *a, const void *b);
int comparebinInt(const void *a, const void *b);
int comparebinCnt(const void *a, const void *b);
int (*comparebin)(const void *a, const void *b) = &comparebinInt;  /*bSortMode=3*/
int sign(int v);                           /* 1 for >= 0; else -1 */


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long i;
	
  double bintc=0.0, bintc_sorted=0., 
         bintervalX=1.0, bintervalY=1.0,
         time, lambda, dist,
         TwoTheta, TwoThetaDeg, Phi, 
         prob=0;

  int ibinX = 0, ibinY = 0, ibinXY = 0;

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL2_ELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.9");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
	
	switch (kind) 
	{
		case 1: fprintf(LogFilePtr, "Option: scattering angle and wavelength\n"); break;
		case 2: fprintf(LogFilePtr, "Option: scattering angle and TOF\n"); break;
		default:Error("Wrong value for evaluation parameter\n");
	}

	bpostX = malloc(sizeof(double)*nbinsX+1);
	memset(bpostX, 0, sizeof(double)*nbinsX+1);
	bpostY = malloc(sizeof(double)*nbinsY+1);
	memset(bpostY, 0, sizeof(double)*nbinsY+1);
	bin = malloc(sizeof(BINDATA*)*(INDEX(nbinsX, nbinsY)+1));
	memset(bin, 0, sizeof(BINDATA*)*(INDEX(nbinsX, nbinsY)+1));

	/* Construction of the Bins */
	/* logarithmic */
	if (bLogBinningX)
	{	
		//bpostX[0] = (double *)malloc(sizeof(double));
		bpostX[0] = x;

		for(ibinX = 1; bpostX[ibinX-1] < X; ibinX++)
		{
			//bpostX[ibinX] = (double *)malloc(sizeof(double));
			bpostX[ibinX] = (bpostX[ibinX-1]) * (1.0 + dLogProzX/100.);
		}
		nbinsX = ibinX-1;
	}
	/* linear */
	else
	{	bintervalX = (X - x) / (double)nbinsX;
		
		for(ibinX = 0; ibinX<=nbinsX; ibinX++)
		{
			//bpostX[ibinX] = (double *)malloc(sizeof(double));
			bpostX[ibinX] = x + bintervalX*ibinX;
		}
	}
	/* logarithmic */
	if (bLogBinningY)
	{	
		//bpostY[0] = (double *)malloc(sizeof(double));
		bpostY[0] = y;

		for(ibinY = 1; bpostY[ibinY-1] < Y; ibinY++)
		{
			//bpostY[ibinY] = (double *)malloc(sizeof(double));
			bpostY[ibinY] = (bpostY[ibinY-1]) * (1.0 + dLogProzY/100.);
		}
		nbinsY = ibinY-1;
	}
	/* linear */
	else
	{	bintervalY = (Y - y) / (double)nbinsY;
		
		for(ibinY = 0; ibinY<=nbinsY; ibinY++)
		{
			//bpostY[ibinY] = (double *)malloc(sizeof(double));
			bpostY[ibinY] = y + bintervalY*ibinY;
		}
	}

  if (bFullMatrix) {
    for(ibinX = 0; ibinX<(nbinsX); ibinX++)
		{	
			for(ibinY = 0; ibinY<(nbinsY); ibinY++)
			{
				ibinXY = INDEX(ibinX, ibinY);
				CreateBin(&bin[ibinXY], &bpostX[ibinX], &bpostY[ibinY]);
			}
		}
  }

	/* Processing of the Neutrons */
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
        dist     = sqrt(InputNeutrons[i].Position[0]*InputNeutrons[i].Position[0]+InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1]+InputNeutrons[i].Position[2]*InputNeutrons[i].Position[2]);

        if (bScatAng==0) { //use direction cosine
          CartesianToSpherical(InputNeutrons[i].Vector, &TwoTheta, &Phi);
        } else {
          /* select traj. according to colour: (nColour=0 means: all colours accepted) */
          TwoTheta = acos(InputNeutrons[i].Position[0]/dist); 
        }
      
			  prob     = bProbActive ? InputNeutrons[i].Probability : 1.0;
			  time     = InputNeutrons[i].Time - TimeOffset;
        //if (bTOFcorr==TRUE) time = time*sdpath/dist;
        //if (bTOFcorr==TRUE) time = time*Flightpath/(Flightpath-sdpath+dist);
			  //lambda   = bTOF ? 395.60346/(Flightpath/time) : InputNeutrons[i].Wavelength; //referenceWavelength;
        lambda   = bTOF ? 395.60346/((bTOFcorr ? Flightpath-sdpath+dist : Flightpath)/time) : InputNeutrons[i].Wavelength; //referenceWavelength;

			  /* Writing out all neutrons, if 'exclusive counts = no' is set */
			  if (bExclCount==FALSE)		
				  WriteNeutron(&InputNeutrons[i]);

			  /* trajectories within deadspot */
			  if (bDeadSpot && TwoTheta <= deadspotangle) continue;

			  /* traj. out of time of evaluation */
			  if (time < dEvalTimeMin || time > dEvalTimeMax) continue;

			  /* exclude traj. with wrong colour: (nColour=0 means: all colours accepted) */
			  if (nColour!=-1 && nColour!=InputNeutrons[i].Color) continue;
			  if (minColor >= 0 && InputNeutrons[i].Color < minColor) continue;
			  if (maxColor >= 0 && InputNeutrons[i].Color > maxColor) continue;

			  /* Writing out the neutrons that comply with the requirements, if 'exclusive counts = yes' is set */
			  if (bExclCount==TRUE)
				  WriteNeutron(&InputNeutrons[i]);
			
			  TwoThetaDeg = TwoTheta*180.0/M_PI;
			  //qValue = (4.0*M_PI/lambda)*sin(TwoTheta/2.0);
			  switch (kind) 
			  {
				  case 1:	// scattering angle + lambda
					  ibinXY = FindIndexXY(&TwoThetaDeg, &lambda, &ibinX, &ibinY);
					  break;
				  case 2:	// scattering angle + TOF
					  ibinXY = FindIndexXY(&TwoThetaDeg, &time, &ibinX, &ibinY);
					  break;
			  }
			  if (ibinXY >= 0)
			  {
				  if (bin[ibinXY] == NULL)
				  {
            CreateBin(&bin[ibinXY], &bpostX[ibinX], &bpostY[ibinY]);
				  }
				  bin[ibinXY]->Counts++;
				  bin[ibinXY]->Int += prob;

				  bintc += prob;
			  }
      }
		}
	}

// Finish: writes and closes evaluate files, writes to log and instrument file, frees memory
// -----------------------------------------------------------------------------------------
my_exit:

	// Output of Results
	fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", bintc);
	fflush(LogFilePtr);
	
	// Spectrum 
	if (fSpectra != NULL)
	{
		// method 1: faster, more memory
		//Copy data pointers to new 1D-array with no unallocated pointers
		bin_sorted = malloc(sizeof(BINDATA*)*(INDEX(nbinsX, nbinsY)+1));
		ibinX = 0;
		for (ibinXY = 0; ibinXY < INDEX(nbinsX, nbinsY); ibinXY++)
		{
			if (bin[ibinXY] != NULL)
			{
				bin_sorted[ibinX] = bin[ibinXY];
				//bintc_sorted += bin_sorted[ibinX]->Int;
				ibinX++;
			}
		}
		//fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", bintc_sorted);
		//bintc_sorted=0.;
		//Sort
		if (bSortMode != 0) qsort(bin_sorted, ibinX, sizeof(BINDATA*), comparebin);
		//Print spectrum
		for (ibinY = 0; ibinY < ibinX; ibinY++)
		{
			fprintf(fSpectra,"%12g %12g %12g %8ld\n", bin_sorted[ibinY]->X, bin_sorted[ibinY]->Y, bin_sorted[ibinY]->Int, bin_sorted[ibinY]->Counts);
			bintc_sorted += bin_sorted[ibinY]->Int;
			free(bin_sorted[ibinY]);
		}
		fprintf(LogFilePtr, "total neutron count rate within binning after sorting: %11.4e n/s \n", bintc_sorted);
		free(bin_sorted);
		
		/*// method 2: takes longer, less memory
		
		//Sort array with unallocated pointers
		if (bSortMode != 0) qsort(bin, INDEX(nbinsX, nbinsY)+1, sizeof(BINDATA*), comparebin);
		for(ibinX = 0; ibinX<(nbinsX); ibinX++)
		{	
			for(ibinY = 0; ibinY<(nbinsY); ibinY++)
			{
				ibinXY = INDEX(ibinX, ibinY);
				if (bin[ibinXY] != NULL)
					fprintf(fSpectra,"%12g %12g %12g %8d\n", bin[ibinXY]->X, bin[ibinXY]->Y, bin[ibinXY]->Int, bin[ibinXY]->Counts);
				free(bin[ibinXY]);
			}
		}*/

		free(bin);
		fclose(fSpectra);
	}

	//Cleanup
	fprintf(LogFilePtr,"\n");
	Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return 0;
}


void CreateBin(BINDATA **bin, double *bpostX, double *bpostY)
{
	*bin = (BINDATA *)malloc(sizeof(BINDATA));
	if (bLogBinningX)
		(*bin)->X = sqrt((*bpostX)*(*(bpostX+1)));
	else
		(*bin)->X = ((*bpostX)+(*(bpostX+1)))/2.0;
	if (bLogBinningY)
		(*bin)->Y = sqrt((*bpostY)*(*(bpostY+1)));
	else
		(*bin)->Y = ((*bpostY)+(*(bpostY+1)))/2.0;
	(*bin)->Counts = 0;
	(*bin)->Int = 0.;
}


int FindIndexXY(double *Xval, double *Yval, int *ibinX, int *ibinY)
{
	//int ibinX = -1, ibinY = -1;
	//int bin = (((X - x) / (double)nbinsX)*146+1e-4) / ((X - x) / (double)nbinsX);
	*ibinX = -1;
	*ibinY = -1;
	if (bLogBinningX){
		for(*ibinX = 0; *ibinX<nbinsX; (*ibinX)++){
			if (bpostX[*ibinX] <= *Xval && *Xval < bpostX[(*ibinX)+1])
				break;
		}
	} else
		*ibinX = (int)floor((*Xval - x) / ((X - x) / (double)nbinsX));
	
	if (bLogBinningY){
		for(*ibinY = 0; *ibinY<nbinsY; (*ibinY)++){	
			if (bpostY[*ibinY] <= *Yval && *Yval < bpostY[(*ibinY)+1])
				break;
		}
	} else
		*ibinY = (int)floor((*Yval - y) / ((Y - y) / (double)nbinsY));
	
	if ((*ibinX >= 0) && (*ibinY >= 0) && (*ibinX < nbinsX) && (*ibinY < nbinsY))
		return INDEX(*ibinX, *ibinY);
	else
		return -1;
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
					if (fSpectra==NULL) 
          { fprintf(LogFilePtr,"\nERROR: File %s could not be opened for spectra output\n",arg);
					  exit(-1);
					}
          else
          { fprintf(LogFilePtr,"\nOutput file: %s\n",arg);
          }
				  break;

				case 'n':
					nbinsX = atol(arg); /* number of bins */
					/*if (nbinsX > BINS){
						fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
						exit(99);
					}*/
					break;
				case 'm':
					nbinsY = atol(arg); /* number of bins */
					/*if (nbinsY > BINS){
						fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
						exit(99);
					}*/
					break;

				case 'k':
					kind = atol(arg); /* 1= d-spacing; 2=momentum transfer q; 3=scattering angle */
					break;

				case 'w':
					winp = atof(arg);
					if (winp == 1.0) bTOF = TRUE; /* time of flight instrument */
					break;

				//case 'r':
				//	referenceWavelength = atof(arg); /* reference Wavelength for crystal monochromator */
				//	break;                           /* (or mechanical velocity selector) instrument   */

				case 'e':
					dEvalTimeMin = atof(arg);        /* minimal time for evaluation */
					break;
				case 'E':
					dEvalTimeMax = atof(arg);        /* maximal time for evaluation */
					break;


				case 'C':
					nColour = atol(arg);       /*  excludes all neutrons with diff. Colour, if nColour > 0 */
					break;
				case 'a':
					minColor = atol(arg);       /*  use neutrons with color >= minColour */
					break;
				case 'A':
					maxColor = atol(arg);       /*  use neutrons with color <= maxColour */
					break;

				case 'd':
					deadspotangle = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
					bDeadSpot = TRUE;                /* scattering angle < deadspotangle [deg] */
					break;

				case 'x':
					x = atof(arg);   /* lower bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;
				case 'X':
					X = atof(arg);   /* upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;
				case 'y':
					y = atof(arg);   /* lower bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;
				case 'Y':
					Y = atof(arg);   /* upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;

				case 'R':
					dLogProzX    = atof(arg);       /* percentage of increase to next bin */
					if (dLogProzX!=0.0)
						bLogBinningX = TRUE;
					break;
				case 'S':
					dLogProzY    = atof(arg);       /* percentage of increase to next bin */
					if (dLogProzY!=0.0)
						bLogBinningY = TRUE;
					break;

				case 'c':
					if(atol(arg)==1)        /* if activated, only neutrons complying with the  */
						bExclCount = TRUE;   /* evaluate requirements are considered further on */
					break;


				case 'l':
					Flightpath = atof(arg);  /* length of neutron flight path [cm] */
					if (Flightpath <= 0.0)
						Error("you must define a flight path > 0.0");
					break;

        case 'L':
					sdpath = atof(arg);  /* length of sample detector distance [cm] */
					if (sdpath <= 0.0)
						Error("you must define a sample detector distance > 0.0");
					break;

        case 't':
					bTOFcorr = atof(arg); /* correct tof to constant sample-detector distance of '-L' (true/false) */
					break;

        case 'D':
					bScatAng = atof(arg); /* Select the way how the scattering angle is determined (0=direction/1=position) */
					break;

				case 'T':
					TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
					break;

				case 'p':
					bProbActive = atoi(arg);
					/* bProbActive=1 means probabilities activated, else neutron weight is set to 1.0 */
					break;
				
				case 's':
					bSortMode = atoi(arg);
					/* 0=No sorting, 1=Sort by X, 2=Y, 3=Intensity, 4=Counts; <0 for reverse */
					break;
					
				case 'f':
					if(atol(arg)==1)        /* if activated, also non-zero lines are written  */
						bFullMatrix = TRUE;   
					break;

				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
					exit(-1);
					break;
			}
		}
	}

	// checks
  // ------
	if ((bLogBinningX && x==0.0) || (bLogBinningY && y==0.0))
		Error("lower bound value must not be zero for logarithmic binning");
	if (fSpectra == NULL)
		Error("no spectra file given");
  
  fprintf(LogFilePtr,"Color: %ld\n",nColour);
  
  if (bScatAng==TRUE) 
  {
    if (sdpath<=0.)
      Error("You must provide a minimum source detector distance to evaluate the position.");
    //if ((nColour_c+nColour_v+nColour_r==0) && (minColor_c+minColor_v+minColor_r==-3) && (maxColor_c+maxColor_v+maxColor_r==-3))
      //Error("You should provide a colour choice for evaluating the position.");
  }

	switch (abs(bSortMode))
	{
	case 1:
		comparebin = &comparebinX;
		break;
	case 2:
		comparebin = &comparebinY;
		break;
	case 3:
		comparebin = &comparebinInt;
		break;
	case 4:
		comparebin = &comparebinCnt;
		break;
	default:
		comparebin = NULL;
		break;
	}
}



int comparebinX(const void *a, const void *b)
{
	const BINDATA *arg1 = *((BINDATA*const*)a);
	const BINDATA *arg2 = *((BINDATA*const*)b);
	int ret = 0;
	
	if (arg1 == NULL)
		ret = 1;
	else if (arg2 == NULL)
		ret = -1;
	else  if (arg1->X < arg2->X)
		ret = -1*sign(bSortMode);
	else if (arg1->X > arg2->X)
		ret = 1*sign(bSortMode);
	else if (bSortMode != 0)
	{
		int SortModeSave = bSortMode;
		bSortMode = 0;
		ret = comparebinY(a, b);
		bSortMode = SortModeSave;
		ret *= sign(bSortMode);
	}

	return ret;
}

int comparebinY(const void *a, const void *b)
{
	const BINDATA *arg1 = *((BINDATA*const*)a);
	const BINDATA *arg2 = *((BINDATA*const*)b);
	int ret = 0;
	
	if (arg1 == NULL)
		ret = 1;
	else if (arg2 == NULL)
		ret = -1*sign(bSortMode);
	else  if (arg1->Y < arg2->Y)
		ret = -1*sign(bSortMode);
	else if (arg1->Y > arg2->Y)
		ret = 1*sign(bSortMode);
	else if (bSortMode != 0)
	{
		int SortModeSave = bSortMode;
		bSortMode = 0;
		ret = comparebinX(a, b);
		bSortMode = SortModeSave;
		ret *= sign(bSortMode);
	}

	return ret;
}

int comparebinInt(const void *a, const void *b)
{
	const BINDATA *arg1 = *((BINDATA*const*)a);
	const BINDATA *arg2 = *((BINDATA*const*)b);
	int ret = 0;
	
	if (arg1 == NULL)
		ret = 1;
	else if (arg2 == NULL)
		ret = -1;
	else  if (arg1->Int < arg2->Int)
		ret = -1*sign(bSortMode);
	else if (arg1->Int > arg2->Int)
		ret = 1*sign(bSortMode);
	else if (bSortMode != 0)
	{
		int SortModeSave = bSortMode;
		bSortMode = 0;
		ret = comparebinCnt(a, b);
		if (ret == 0) ret = comparebinX(a, b);
		if (ret == 0) ret = comparebinY(a, b);
		bSortMode = SortModeSave;
		ret *= sign(bSortMode);
	}
	
	return ret;
}

int comparebinCnt(const void *a, const void *b)
{
	const BINDATA *arg1 = *((BINDATA*const*)a);
	const BINDATA *arg2 = *((BINDATA*const*)b);
	int ret = 0;
	
	if (arg1 == NULL)
		ret = 1;
	else if (arg2 == NULL)
		ret = -1;
	else  if (arg1->Counts < arg2->Counts)
		ret = -1*sign(bSortMode);
	else if (arg1->Counts > arg2->Counts)
		ret = 1*sign(bSortMode);
	else if (bSortMode != 0)
	{
		int SortModeSave = bSortMode;
		bSortMode = 0;
		ret = comparebinInt(a, b);
		if (ret == 0) ret = comparebinX(a, b);
		if (ret == 0) ret = comparebinY(a, b);
		bSortMode = SortModeSave;
		ret *= sign(bSortMode);
	}
	
	return ret;
}


int sign(int v)
{
	return v >= 0 ? 1 : -1;
}

