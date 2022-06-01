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

#include "convert.h"
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
// Input parameters
FILE  *fSpectra=NULL;

VtEvalComb eComb    =VT_NO_ECOMB;// -k  [-]  1=scattering angle and wavelength; 2=scattering angle and TOF 
VtEvalSort eSortMode=VT_NO_SORT; // -s  [-]  0=No sorting, 1=Sort by X, 2=Y, 3=Intensity, 4=Counts; <0 for reverse 
VtAngleSel eScatAng =VT_NO_SEL;  // -D  [-]  TRUE : position information is used (needs more information)  FALSE: direction cosine is used 
	     
short  bFullMatrix=FALSE,    // -f  [-]  TRUE: Also lines with zero intensity/counts are written; needs more memory  FALSE: Default: only write non-zero lines 
       bProbActive=TRUE,     // -p  [-]  bProbActive=1 means probabilities activated, else neutron weight is set to 1.0 
       bExclCount =FALSE,    // -c  [-]  TRUE : only neutrons complying with the evaluate requirements are written to the output 
       bTOF       =FALSE,    // -w  [-]  TRUE : time of flight instrument 
       bTOFcorr   =FALSE;    // -t  [-]  TRUE : correct time to shortest detector distance 
       
long   nbinsX=0,             // -n  [-]  number of bins in X 
       nbinsY=0;             // -m  [-]  number of bins in Y 
int    nColour = ANY_COLOR,  // -C  [-]  colour necessary for the trajectory to be regarded  colour -1 means: all trajectories are regarded 
       minColor= ANY_COLOR,  // -a  [-]  neutrons with color >= minColour used  colour -1 means: all trajectories are regarded 
       maxColor= ANY_COLOR;  // -A  [-]  neutrons with color <= maxColour used,  colour -1 means: all trajectories are regarded

double // referenceWavelength// reference Wavelength for crystal monochromator (or mechanical velocity selector) instrument                                                 */
       MinX=0.0,             // -x [deg] lower bound of theta range ==> X 
       MaxX=0.0,             // -X [deg] upper bound of theta range ==> X 
       MinY=0.0,             // -y [var] lower bound of wavelength or TOF [Ang],[ms] ==> Y 
       MaxY=0.0,             // -Y [var] upper bound of wavelength or TOF [Ang],[ms] ==> Y 
       dLogProzX=0.0,        // -R  [%]  percentage of increase to next bin ==> X 
       dLogProzY=0.0,        // -S  [%]  percentage of increase to next bin ==> Y
       Flightpath=0.0,       // -l  [cm] length of neutron flight path 
       sdpath    =0.0,       // -L  [cm]  shortest sample detector distance
       TimeOffset=0.0,       // -T [deg] global shift of the neutron time t= t-TimeOffset
       deadspotangle=0.0,    // -d [deg] excludes all neutrons with a scattering angle < deadspotangle 
       EvalTimeMin=-1.0e10,  // -e [ms] minimal and maximal time for evaluation 
       EvalTimeMax= 1.0e10;  // -E [ms] minimal and maximal time for evaluation 

// Variables determined from input parameters or trajectory data
double *bpostX = {NULL};     //          limits of the bins                                           */
double *bpostY = {NULL};     //          limits of the bins                                           */
BINDATA **bin = {NULL};      //          Counts = count rate of a bin                                 */
                             //          number of trajectories contributing to count rate            */
BINDATA **bin_sorted={NULL}; //          pointers to BINDATA of bin; array size dynamically allocated */

short  bLogBinningX=FALSE,   //          flag: TRUE : binning on x-axis increases exponentially    FALSE: linear binning  
       bLogBinningY=FALSE,   //          flag: TRUE : binning on y-axis increases exponentially    FALSE: linear binning  
       bDeadSpot   =FALSE;   //          flag: TRUE : deadspot exists 


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
int (*comparebin)(const void *a, const void *b) = &comparebinInt;  /*eSortMode=3*/
int sign(int v);                           /* 1 for >= 0; else -1 */


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long i;
	
  double bintc=0.0, bintc_sorted=0.0, 
         bintervalX=1.0, bintervalY=1.0,
         time=0.0, lambda=0.0, dist=0.0,
         TwoTheta=0.0, TwoThetaDeg=0.0, Phi=0.0, 
         prob=0.0;

  int ibinX = 0, ibinY = 0, ibinXY = 0;

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL2_ELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.9a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

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
		bpostX[0] = MinX;

		for(ibinX = 1; bpostX[ibinX-1] < MaxX; ibinX++)
		{
			//bpostX[ibinX] = (double *)malloc(sizeof(double));
			bpostX[ibinX] = (bpostX[ibinX-1]) * (1.0 + dLogProzX/100.);
		}
		nbinsX = ibinX-1;
	}
	/* linear */
	else
	{	bintervalX = (MaxX - MinX) / (double)nbinsX;
		
		for(ibinX = 0; ibinX<=nbinsX; ibinX++)
		{
			//bpostX[ibinX] = (double *)malloc(sizeof(double));
			bpostX[ibinX] = MinX + bintervalX*ibinX;
		}
	}
	/* logarithmic */
	if (bLogBinningY)
	{	
		//bpostY[0] = (double *)malloc(sizeof(double));
		bpostY[0] = MinY;

		for(ibinY = 1; bpostY[ibinY-1] < MaxY; ibinY++)
		{
			//bpostY[ibinY] = (double *)malloc(sizeof(double));
			bpostY[ibinY] = (bpostY[ibinY-1]) * (1.0 + dLogProzY/100.);
		}
		nbinsY = ibinY-1;
	}
	/* linear */
	else
	{	bintervalY = (MaxY - MinY) / (double)nbinsY;
		
		for(ibinY = 0; ibinY<=nbinsY; ibinY++)
		{
			//bpostY[ibinY] = (double *)malloc(sizeof(double));
			bpostY[ibinY] = MinY + bintervalY*ibinY;
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

        if (eScatAng==VT_SEL_DIR) { //use direction cosine
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
			  if (time < EvalTimeMin || time > EvalTimeMax) continue;

			  /* exclude traj. with wrong colour: (nColour=0 means: all colours accepted) */
			  if (nColour!=-1 && nColour!=InputNeutrons[i].Color) continue;
			  if (minColor >= 0 && InputNeutrons[i].Color < minColor) continue;
			  if (maxColor >= 0 && InputNeutrons[i].Color > maxColor) continue;

			  /* Writing out the neutrons that comply with the requirements, if 'exclusive counts = yes' is set */
			  if (bExclCount==TRUE)
				  WriteNeutron(&InputNeutrons[i]);
			
			  TwoThetaDeg = TwoTheta*180.0/M_PI;
			  //qValue = (4.0*M_PI/lambda)*sin(TwoTheta/2.0);
			  switch (eComb) 
			  {
				  case VT_SCA_LMBD:	// scattering angle + lambda
					  ibinXY = FindIndexXY(&TwoThetaDeg, &lambda, &ibinX, &ibinY);
					  break;
				  case VT_SCA_TOF:	// scattering angle + TOF
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
		if (eSortMode != VT_NO_SORT) qsort(bin_sorted, ibinX, sizeof(BINDATA*), comparebin);
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
		if (eSortMode != VT_NO_SORT) qsort(bin, INDEX(nbinsX, nbinsY)+1, sizeof(BINDATA*), comparebin);
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
	//int bin = (((MaxX - MinX) / (double)nbinsX)*146+1e-4) / ((MaxX - MinX) / (double)nbinsX);
	*ibinX = -1;
	*ibinY = -1;
	if (bLogBinningX){
		for(*ibinX = 0; *ibinX<nbinsX; (*ibinX)++){
			if (bpostX[*ibinX] <= *Xval && *Xval < bpostX[(*ibinX)+1])
				break;
		}
	} else
		*ibinX = (int)floor((*Xval - MinX) / ((MaxX - MinX) / (double)nbinsX));
	
	if (bLogBinningY){
		for(*ibinY = 0; *ibinY<nbinsY; (*ibinY)++){	
			if (bpostY[*ibinY] <= *Yval && *Yval < bpostY[(*ibinY)+1])
				break;
		}
	} else
		*ibinY = (int)floor((*Yval - MinY) / ((MaxY - MinY) / (double)nbinsY));
	
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
  char  sEvalComb[51];
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

				case 'k':
					eComb = (VtEvalComb) atoi(arg); /* 1= scattering angle + lambda;  2: scattering angle + TOF */
					break;
				case 's':
					eSortMode = (VtEvalSort) atoi(arg);
					/* 0=No sorting, 1=Sort by X, 2=Y, 3=Intensity, 4=Counts; <0 for reverse */
					break;
        case 'D':
					eScatAng = (VtAngleSel) atoi(arg); /* Select the way how the scattering angle is determined (0=direction/1=position) */
					break;

				case 'n':
					nbinsX = atol(arg); /* number of bins in x direction */
					break;
				case 'm':
					nbinsY = atol(arg); /* number of bins in y direction */
					break;
				case 'x':
					MinX = atof(arg);   /* lower bound of theta range in x direction  [deg]*/
					break;
				case 'X':
					MaxX = atof(arg);   /* upper bound of theta range in x direction  [deg] */
					break;
				case 'y':
					MinY = atof(arg);   /* lower bound of lambda or TOF [Ang], [ms]  in y direction */
					break;
				case 'Y':
					MaxY = atof(arg);   /* upper bound of lambda or TOF [Ang], [ms]  in y direction */
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

				case 'f':
					bFullMatrix = (short) atoi(arg);  /* if activated, also non-zero lines are written  */
					break;
				case 'p':
					bProbActive = (short) atoi(arg);  /* bProbActive=1 means probabilities activated, else neutron weight is set to 1.0 */
					break;
				case 'c':
					bExclCount  = (short) atoi(arg);  /* if activated, only neutrons complying with the evaluate requirements are considered further on */
					break;
				case 'w':
					bTOF        = (short) atoi(arg);  /* time of flight instrument */
					break;
        case 't':
					bTOFcorr    = (short) atoi(arg);  /* correct tof to constant sample-detector distance of '-L' (true/false) */
					break;

				//case 'r':
				//	referenceWavelength = atof(arg); /* reference Wavelength for crystal monochromator */
				//	break;                           /* (or mechanical velocity selector) instrument   */

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
				case 'T':
					TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
					break;
				case 'd':
					deadspotangle = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
					bDeadSpot = TRUE;                /* scattering angle < deadspotangle [deg] */
					break;
					
				case 'e':
					EvalTimeMin = atof(arg);        /* minimal time for evaluation */
					break;
				case 'E':
					EvalTimeMax = atof(arg);        /* maximal time for evaluation */
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

				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
					exit(-1);
					break;
			}
		}
	}

	// checks
  // ------
	if ((bLogBinningX && MinX==0.0) || (bLogBinningY && MinY==0.0))
		Error("lower bound value must not be zero for logarithmic binning");
	if (fSpectra == NULL)
		Error("no spectra file given");
  
  EvalComb_ID2Txt(sEvalComb, eComb);
  fprintf(LogFilePtr,"\noption %s\n", sEvalComb);

  if (nColour!=NO_COLOR && nColour!=ANY_COLOR)
    fprintf(LogFilePtr, "Color: %ld\n", nColour);
  
  if (eScatAng==VT_SEL_POS) 
  {
    if (sdpath<=0.)
      Error("You must provide a minimum source detector distance to evaluate the position.");
    //if ((nColour_c+nColour_v+nColour_r==0) && (minColor_c+minColor_v+minColor_r==-3) && (maxColor_c+maxColor_v+maxColor_r==-3))
      //Error("You should provide a colour choice for evaluating the position.");
  }

	switch (abs(eSortMode))
	{
	case VT_SORT_X:
		comparebin = &comparebinX;
		break;
	case VT_SORT_Y:
		comparebin = &comparebinY;
		break;
	case VT_SORT_INT:
		comparebin = &comparebinInt;
		break;
	case VT_SORT_CTS:
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
		ret = -1*sign(eSortMode);
	else if (arg1->X > arg2->X)
		ret = 1*sign(eSortMode);
	else if (eSortMode != VT_NO_SORT)
	{
		int SortModeSave = eSortMode;
		eSortMode = VT_NO_SORT;
		ret = comparebinY(a, b);
		eSortMode = SortModeSave;
		ret *= sign(eSortMode);
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
		ret = -1*sign(eSortMode);
	else  if (arg1->Y < arg2->Y)
		ret = -1*sign(eSortMode);
	else if (arg1->Y > arg2->Y)
		ret = 1*sign(eSortMode);
	else if (eSortMode != VT_NO_SORT)
	{
		int SortModeSave = eSortMode;
		eSortMode = VT_NO_SORT;
		ret = comparebinX(a, b);
		eSortMode = SortModeSave;
		ret *= sign(eSortMode);
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
		ret = -1*sign(eSortMode);
	else if (arg1->Int > arg2->Int)
		ret = 1*sign(eSortMode);
	else if (eSortMode != VT_NO_SORT)
	{
		int SortModeSave = eSortMode;
		eSortMode = VT_NO_SORT;
		ret = comparebinCnt(a, b);
		if (ret == 0) ret = comparebinX(a, b);
		if (ret == 0) ret = comparebinY(a, b);
		eSortMode = SortModeSave;
		ret *= sign(eSortMode);
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
		ret = -1*sign(eSortMode);
	else if (arg1->Counts > arg2->Counts)
		ret = 1*sign(eSortMode);
	else if (eSortMode != VT_NO_SORT)
	{
		int SortModeSave = eSortMode;
		eSortMode = VT_NO_SORT;
		ret = comparebinInt(a, b);
		if (ret == 0) ret = comparebinX(a, b);
		if (ret == 0) ret = comparebinY(a, b);
		eSortMode = SortModeSave;
		ret *= sign(eSortMode);
	}
	
	return ret;
}


int sign(int v)
{
	return v >= 0 ? 1 : -1;
}

