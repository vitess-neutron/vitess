/********************************************************************************************************/
/*  VITESS module 'eval_inelast.c'                                                                      */
/*                                                                                                      */
/* The free non-commercial use of these routines is granted                                             */
/* providing due credit is given to the authors.                                                        */
/* 1.0            Géza Zsigmond                                                                         */
/* 1.1  Jul 2002  Géza Zsigmond  change                                                                 */
/* 1.2  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                           */
/* 1.3  Nov 2005  K. Lieutenant  transformation scattering angles -> direction removed                  */
/* 1.4  Aug 2012  K. Lieutenant  calculation of energy transfer and restriction of ang. range corrected */
/* 1.5  Oct 2013  K. Lieutenant  Bose and transform. factor removed, color and ToF correction included  */
/* 1.6  Mar 2020  K. Lieutenant  new central visualization parameters                                   */
/* 1.6a Oct 2021  K. Lieutenant  tidying up finished                                                    */
/********************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "convert.h"
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "softabort.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define	STRING_BUFFER 50
#define	BINS_BUFFER 5000

/*********************************/
/** Global Variables            **/
/*********************************/
// Input parameters
char	  	*FileNameTOF=NULL,            // -E  [-]  output file for the TOF spectrum
          *FileNameEnergy=NULL;         // -G  [-]  output file for showing the energy transfer spectrum
VtInstGeom eGeomOption=VT_NO_I_GEOM;    // -A  [-]  geometry option:   0: direct geometry   1: indirect geometry
short      bTofCorr= TRUE,              // -t  [-]  criterion: correction to true flight path length from sample to detector:  0: no   1: yes
           bBoseF  = FALSE,             // -D  [-]  criterion: divide by Bose factor  (not used in Vitess 3)  
           iColor=ANY_COLOR;            // -f  [-]  neutron 'color' to evaluate  -1: all neutrons
long       nBins=0;                     // -C  [-]  number of bins in TOF and energy spectrum
          
double     PrimaryFlightPath  =0.0,     // -a  [-]   Distance from moderator to sample.
           SecondaryFlightPath=0.0,     // -b  [-]   Distance from sample to detector
           LambdaRef =0.0,              // -c  [-]   reference wavelength 
           TimeOffset=0.0,              // -d  [-]   average time of neutrons at start
           MinE  = 0.0,                 // -m  [-]   minimal energy transfer to be monitored
           MaxE  = 0.0,                 // -M  [-]   maximal energy transfer to be monitored
           MinTOF=-1.0,                 // -e  [-]   minimal time of flight to be monitored
           MaxTOF=-1.0,                 // -g  [-]   maximal time of flight to be monitored
           SlopeBins  = 0.0,            // -h  [-]   
           Temperature= 0.0,            // -i  [-]   temperature for Bose factor  (not used in Vitess 3)
           AngleCntr  = 0.0,            // -j  [-}   horizontal angular range that is processed
           AngleRange=180.0;            // -k  [-]     [AngleCntr - AngleRange, AngleCntr + AngleRange]

// Variables determined from input parameters or trajectory data
FILE  	  *FilePtrTOF=NULL,  
          *FilePtrEnergy=NULL;
double     EnergyRef, VelocityRef,      //     [-]   energy and velocity and corresponding to reference wavelength
           TofRef,                      //     [-]   TOF for the reference part, i.e. primary flight path for direct geometry and secondary for indirect geometry
           prob_t[BINS_BUFFER], prob_e[BINS_BUFFER]; 
double     alpha, beta;


/******************************/
/** Prototypes               **/
/******************************/
double TransformFactor(double DeltaE);    // Transform factor     
double BoseFactor(double T, double w);    // Bose factor
void   OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables
void   OwnCleanup();                      // Closes files   


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  long   i=0, k=0, nperbint[BINS_BUFFER], nperbine[BINS_BUFFER];
  double t[BINS_BUFFER], e[BINS_BUFFER],
         PathToDetection=0.0,   // real pathlength fro sample to detector for individual trajectory
         TofToDet=0.0,          // TOF til detector - TOF offset
         rotz=0.0, roty=0.0,    // hor. and vertical Euler angles to describe flight direction    [rad]
         TotIntTof=0.0,         // total intensity within TOF and E binning resp.
         DelE     =0.0,         // energy gain (DelE > 0) or loss
         TotIntE  =0.0;

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_INELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.6a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  /* calculates TOF channel boundaries and init p_TOF*/
  t[0] = MinTOF ; 
  e[0] = MinE ;
  prob_t[0]   = prob_e[0]   = 0.0; 
  nperbint[0] = nperbine[0] = 0;

  for(k=1; k<=nBins; k++)
  {
    t[k] = alpha *t[k-1] + beta ;
    e[k] = e[k-1] + (MaxE - MinE) / nBins ;

    prob_t[k]   = prob_e[k]   = 0.0; 
    nperbint[k] = nperbine[k] = 0;
  }


  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  while (ReadNeutrons()!=0)
  {
    for(i=0; i < NumNeutGot; i++)
    {
      CHECK;
	
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // check color and flight direction
        if (iColor != ANY_COLOR  &&  iColor != InputNeutrons[i].Color) goto no_match;

        CartesianToEulerZY(InputNeutrons[i].Vector, &roty, &rotz);
        if (rotz < (AngleCntr - AngleRange) || rotz > (AngleCntr + AngleRange) ) goto no_match;

        /* determine estimated TOF from source to detector */
        TofToDet = InputNeutrons[i].Time - TimeOffset;

        /* energy transfer corresponding to total flight time to detector */
        if (eGeomOption == VT_DIRECT_GEOM) 
        { 
          if (bTofCorr)       // Flight distance correction
            PathToDetection = sqrt(sq(InputNeutrons[i].Position[0]) + sq(InputNeutrons[i].Position[1]) + sq(InputNeutrons[i].Position[2]));
          else
            PathToDetection = SecondaryFlightPath;
          DelE = 0.001*ENERGY_FROM_V(PathToDetection / (TofToDet - TofRef)) - EnergyRef;          // direct geometry
        }
        else if (eGeomOption == VT_INVERT_GEOM) 
        { DelE = EnergyRef - 0.001*ENERGY_FROM_V(PrimaryFlightPath / (TofToDet - TofRef));
        }
        else
        { Error ("Wrong geometry option");
        }

        /* binning process  */
        for(k=0; k<nBins; k++)
        {
          if( (TofToDet > t[k]) && (TofToDet <= t[k+1]) )
          { prob_t[k] += InputNeutrons[i].Probability; 
            TotIntTof += InputNeutrons[i].Probability; 
            nperbint[k] += 1 ;
          }
          if( (DelE > e[k]) && (DelE <= e[k+1]) )
          { prob_e[k] += InputNeutrons[i].Probability; // * TransformFactor(DelE) / BoseFactor(Temperature, DelE) ; 
            TotIntE   += InputNeutrons[i].Probability; 
            nperbine[k] += 1 ;
          }
        }

        /* continues here if neutron is not considered */
	     no_match: ;

        /* writes output binary file */
        WriteNeutron(&(InputNeutrons[i]));
      }
    } // end loop over trajectories
  }

// Finish: writes and closes evaluate file, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
 my_exit:
  // write files
  for(k=0; k<nBins; k++)
  {
    if (nperbint[k]==0) nperbint[k]=1; 
    if (nperbine[k]==0) nperbine[k]=1; 
		
    if (FilePtrTOF    != NULL) fprintf(FilePtrTOF,    "%lf   %le   %le   %9ld\n", (t[k]+t[k+1])/2.0, prob_t[k], prob_t[k]/sqrt((double)nperbint[k]), nperbint[k]) ;
    if (FilePtrEnergy != NULL) fprintf(FilePtrEnergy, "%lf   %le   %le   %9ld\n", (e[k]+e[k+1])/2.0, prob_e[k], prob_e[k]/sqrt((double)nperbine[k]), nperbine[k]) ;
  }

  fprintf(LogFilePtr, "\ntotal intensity within TOF and E binning: %11.3e  %11.3e\n", TotIntTof, TotIntE);
	
  OwnCleanup();
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;

}/* End main() */


/*******************************************************/
/* Transform factor                                    */
/*******************************************************/
double TransformFactor(double DeltaE)
{
	if(eGeomOption == VT_DIRECT_GEOM) return SecondaryFlightPath * sqrt(EnergyRef) / sq(EnergyRef + DeltaE) ;
	if(eGeomOption == VT_INVERT_GEOM) return PrimaryFlightPath   / sqrt(EnergyRef) / (EnergyRef - DeltaE) ;

	else return 0 ;
}


/*******************************************************/
/* Bose factor if w in ueV                             */
/*******************************************************/
double	BoseFactor(double T, double w)
{
double betha;

	if(T == 0.) return 1. ;

	betha = 11.605 / T ; /* unit 1/meV ! */

	if(w > 0.) return 1 / ((double) exp(betha * w) - 1) +1 ;
	if(w < 0.) return 1 / ((double) exp(- betha * w) - 1) ;

	else return	0. ;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  char   sInstGeom[21];
  double TOF_total_e=0.0;   // total TOF without energy transfer

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'A':
        eGeomOption = (VtInstGeom) atoi(&argv[1][2]);
        break;
      case 't':
        bTofCorr = atoi(&argv[1][2]) ;
        break;
      case 'D':
        bBoseF = atoi(&argv[1][2]) ;
        break;

      case 'C':
        sscanf(&argv[1][2], "%ld", &nBins) ;
        break;
      case 'f':
        iColor = atoi(&argv[1][2]);
        break;

      case 'E':
        FileNameTOF = &argv[1][2];
        FilePtrTOF  = OpenOutputFile(&argv[1][2], FALSE, "w");
	      if (FilePtrTOF==NULL)
	      {
		      printf("\nTOF spectrum file could not be not opened\n");
		      exit(0);
	      }
      break;

      case 'G':
	      FileNameEnergy = &argv[1][2];
        FilePtrEnergy  = OpenOutputFile(&argv[1][2], FALSE, "w");
	      if(FilePtrEnergy==NULL)
	      {
		      printf("\nenergy spectrum file not opened or created\n");
		      exit(0);
	      }
      break;

      case 'a':
        sscanf(&argv[1][2], "%lf", &PrimaryFlightPath) ;
        break;
      case 'b':
        sscanf(&argv[1][2], "%lf", &SecondaryFlightPath) ;
        break;

      case 'c':
        sscanf(&argv[1][2], "%lf", &LambdaRef) ;
        break;

      case 'd':
        sscanf(&argv[1][2], "%lf", &TimeOffset) ;
        break;

      case 'e':
        sscanf(&argv[1][2], "%lf", &MinTOF) ;
        break;
      case 'g':
        sscanf(&argv[1][2], "%lf", &MaxTOF) ;
        break;

      case 'm':
        sscanf(&argv[1][2], "%lf", &MinE) ;
        break;
      case 'M':
        sscanf(&argv[1][2], "%lf", &MaxE) ;
        break;

      case 'h':
        sscanf(&argv[1][2], "%lf", &SlopeBins) ;
        break;

      case 'i':
        sscanf(&argv[1][2], "%lf", &Temperature) ;
        break;

      case 'j':
        sscanf(&argv[1][2], "%lf", &AngleCntr) ;
        break;

      case 'k':
        sscanf(&argv[1][2], "%lf", &AngleRange) ;
        break;
    }
    argc--;
    argv++;
  }


  // checks
  // ------
  if (eGeomOption!=VT_DIRECT_GEOM &&	eGeomOption!=VT_INVERT_GEOM)
  {
    fprintf(LogFilePtr,"ERROR: wrong geometry option!\n\n") ;
    exit(0) ;
  }

  if (bBoseF!=FALSE &&	bBoseF!=TRUE)
  {
    fprintf(LogFilePtr,"ERROR: wrong option for Bose factor!\n\n") ;
    exit(0) ;
  }

  if (MinTOF > MaxTOF && MinE > MaxE)
  {
    fprintf(LogFilePtr,"ERROR: minimal time/energy must be < maximal time/energy !\n\n") ;
    exit(0) ;
  }

  /* computes global reference values */
  if (LambdaRef > 0.0 && PrimaryFlightPath > 0.0 && SecondaryFlightPath > 0.0)
  { 
    EnergyRef   = 0.001*ENERGY_FROM_LAMBDA(LambdaRef) ;
    VelocityRef = V_FROM_LAMBDA(LambdaRef) ;
    TOF_total_e = (PrimaryFlightPath + SecondaryFlightPath) / VelocityRef ;

    if(eGeomOption == VT_DIRECT_GEOM)   // direct geometry
      TofRef = PrimaryFlightPath / VelocityRef ;
    else                  // indirect geometry
      TofRef = SecondaryFlightPath / VelocityRef ;
  }
  else
  { Error("Reference wavelength or flight path missing");
  }

  if (MinTOF >= 0.0 && MaxTOF >=0.0)  // TOF range given,
  { 
    if(MinE == 0.0 || MaxE == 0.0)    // but not energy range
    { 
      if(eGeomOption == VT_DIRECT_GEOM)  // direct geometry
      {
        MinE = 0.001*ENERGY_FROM_V(SecondaryFlightPath / (MaxTOF - TofRef)) - EnergyRef;
        MaxE = 0.001*ENERGY_FROM_V(SecondaryFlightPath / (MinTOF - TofRef)) - EnergyRef ;
      }
      else                  // indirect geometry
      {
        MinE = EnergyRef - 0.001*ENERGY_FROM_V(PrimaryFlightPath / (MinTOF - TofRef)) ;
        MaxE = EnergyRef - 0.001*ENERGY_FROM_V(PrimaryFlightPath / (MaxTOF - TofRef)) ;
      }
    }
  }
  else if(MinE != 0.0 && MaxE != 0.0)    // energy range given
  {
    if (MinTOF == -1.0 || MaxE == -1.0)  // but not TOF range
    { 
      if(eGeomOption == VT_DIRECT_GEOM)  // direct geometry
      {
        MinTOF = TofRef + SecondaryFlightPath / V_FROM_ENERGY(1000*(EnergyRef + MaxE));
        MaxTOF = TofRef + SecondaryFlightPath / V_FROM_ENERGY(1000*(Max(0.1,EnergyRef + MinE)));
      }
      else                 // indirect geometry
      {
        MinTOF = PrimaryFlightPath / V_FROM_ENERGY(1000*(EnergyRef - MinE)) + TofRef;
        MaxTOF = PrimaryFlightPath / V_FROM_ENERGY(1000*(EnergyRef - MaxE)) + TofRef;
      }
    }
  }
  else                     // no range given
  {
    Error("TOF or energy range has to be given");
  }

  if(fabs(SlopeBins) > 0.01)
  {
    fprintf(LogFilePtr,"ERROR: please set |SlopeBins| < 0.01!\n\n") ;
    exit(0) ;
  }


  // options and printing to log file
  // --------------------------------
  InstGeom_ID2Txt(sInstGeom, eGeomOption);
  fprintf(LogFilePtr,"\noption %s\n", sInstGeom);

  if(FilePtrTOF    != NULL) fprintf(LogFilePtr," TOF spectrum file   : '%s'\n", FileNameTOF) ;
  if(FilePtrEnergy != NULL) fprintf(LogFilePtr," energy spectrum file: '%s'\n", FileNameEnergy) ;

  fprintf(LogFilePtr, " number of bins       : %4ld      \n", nBins);
  fprintf(LogFilePtr, " primary flight path  : %9.4f m  \n secondary flight path: %9.4f m \n",           PrimaryFlightPath/100.0, SecondaryFlightPath/100.0);
  fprintf(LogFilePtr, " reference wavelength : %9.4f Ang\n time offset          : %9.4f ms\n",           LambdaRef, TimeOffset);
  fprintf(LogFilePtr, " gradient of timebins : %9.4f    \n angular range        : %9.4f +/-%9.4f deg\n", SlopeBins, AngleCntr, AngleRange);

  if(Temperature == 0.0) bBoseF = FALSE ;
  if(bBoseF == FALSE) Temperature = 0.0 ;
  // if(bBoseF == FALSE) 	fprintf(LogFilePtr,"\nnot divided by Bose-factor") ;
  // if(bBoseF == TRUE) 	fprintf(LogFilePtr,"\ndivided by Bose-factor") ;

  /* calculates alpha, beta coeff. for TOF boundary calculation */
  alpha = 1 / (1 - SlopeBins) ;
  beta = 0. ;

  for (int k=0;k<nBins;k++) 
    beta += pow(alpha, k) ;
  beta = (MaxTOF - pow(alpha, nBins) * MinTOF) / beta ;

  fprintf(LogFilePtr,	"computed reference values:\n");
  fprintf(LogFilePtr, " reference energy     : %9.4f meV\n velocity             : %9.4f cm/ms\n", EnergyRef, VelocityRef);
	fprintf(LogFilePtr,	" effective TOF range  : %9.4f  - %9.4f ms\n"                             , MinTOF, MaxTOF);
	fprintf(LogFilePtr,	" TOF_total_elast      : %9.4f ms \n TOF_fixed_energy_side: %9.4f ms\n"   , TOF_total_e, TofRef);
  fprintf(LogFilePtr, " energy transfer      : %9.4f  - %9.4f meV\n"                            , MinE,   MaxE) ;

  AngleCntr  *= M_PI / 180. ;
  AngleRange *= M_PI / 180. ;

}/* End OwnInit */


/*******************************************************/
/* closes files                                        */
/*******************************************************/
void OwnCleanup()
{
	if(FilePtrTOF != NULL)fclose(FilePtrTOF) ;

	if(FilePtrEnergy != NULL)fclose(FilePtrEnergy) ;

}/* End OwnCleanup */

