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
/********************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "softabort.h"

/* START HEADER STORY */

#define	STRING_BUFFER 50
#define	BINS_BUFFER 5000

// global variables
FILE		*FilePtrTOF=NULL,  *FilePtrEnergy=NULL;
char		*FileNameTOF=NULL, *FileNameEnergy=NULL;
short    eGeomOption,      // geometry option:   0: direct geometry   1: indirect geometry
         bTofCorr= 1,      // criterion: correction to true flight path length from sample to detector:  0: no   1: yes
         bBoseF  = 0,      // criterion: divide by Bose factor   
         iColor=ANY_COLOR; // neutron 'color' to evaluate  -1: all neutrons
long     NoBins,           // number of bins in TOF and energy spectrum
         i, k , nperbint[BINS_BUFFER], nperbine[BINS_BUFFER];

double   PrimaryFlightPath, SecondaryFlightPath, 
         LambdaRef,                   // reference wavelength 
         EnergyRef, VelocityRef,      // and corresponding energy and velocity
         TofRef,                      // TOF for the reference part, i.e. primary flight path for direct geometry and secondary for indirect geometry
         DelE,                        // energy gain (DelE > 0) or loss
         MinTOF=-1.0, MaxTOF=-1.0,    // minimal and maximal time of flight to be monitored
         MinE  = 0.0, MaxE  = 0.0,    // minimal and maximal energy transfer to be monitored
         TimeOffset,                  // average time of neutrons at start
         SlopeBins, 
         Temperature,                 // temperature for Bose factor
         Angle=0.0, AngleRange=180.0; // horizontal angular range that is processed
double   t[BINS_BUFFER], e[BINS_BUFFER], 
         prob_t[BINS_BUFFER], prob_e[BINS_BUFFER]; 
double   alpha, beta;


// prototypes
double TransformFactor(double DelE);
double BoseFactor(double T, double w);
void   OwnInit(int argc, char *argv[]) ;
void   OwnCleanup() ;

/* FINISH HEADER STORY */



int main(int argc, char **argv)
{
  double PathToDetection,   // real pathlength fro sample to detector for individual trajectory
         TofToDet,          // TOF til detector - TOF offset
         rotz, roty,        // hor. and vertical Euler angles to describe flight direction    [rad]
         TotIntTof=0.0,     // total intensity within TOF and E binning resp.
         TotIntE=0.0;

  /* Initialize the program according to the parameters given  */
  Init   (argc, argv, VT_EVAL_INELAST);
  print_module_name("eval_inelast 1.5") ;
  OwnInit(argc, argv);

  /* calculates TOF channel boundaries and init p_TOF*/
  t[0] = MinTOF ; e[0] = MinE ;

  prob_t[0]   = prob_e[0]   = 0.0; 
  nperbint[0] = nperbine[0] = 0;

  for(k=1; k<=NoBins; k++)
  {
    t[k] = alpha *t[k-1] + beta ;
    e[k] = e[k-1] + (MaxE - MinE) / NoBins ;

    prob_t[k]   = prob_e[k]   = 0.0; 
    nperbint[k] = nperbine[k] = 0;
  }


  DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
  {
    CHECK;
    for(i=0; i < NumNeutGot; i++)
    {
      CHECK;
	
      // check color and flight direction
      if (iColor != ANY_COLOR  &&  iColor != InputNeutrons[i].Color) goto no_match;

      CartesianToEulerZY(InputNeutrons[i].Vector, &roty, &rotz);
      if (rotz < (Angle - AngleRange) || rotz > (Angle + AngleRange) ) goto no_match;

      /* determine estimated TOF from source to detector */
      TofToDet = InputNeutrons[i].Time - TimeOffset;

      /* energy transfer corresponding to total flight time to detector */
      if (eGeomOption == 0) 
      { 
        if (bTofCorr)       // Flight distance correction
          PathToDetection = sqrt(sq(InputNeutrons[i].Position[0]) + sq(InputNeutrons[i].Position[1]) + sq(InputNeutrons[i].Position[2]));
        else
          PathToDetection = SecondaryFlightPath;
        DelE = 0.001*ENERGY_FROM_V(PathToDetection / (TofToDet - TofRef)) - EnergyRef;          // direct geometry
      }
      else if (eGeomOption == 1) 
      { DelE = EnergyRef - 0.001*ENERGY_FROM_V(PrimaryFlightPath / (TofToDet - TofRef));
      }
      else
      { Error ("Wrong geometry option");
      }

      /* binning process  */
      for(k=0; k<NoBins; k++)
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

    } // end loop over trajectories
  }

  // write files
  for(k=0;k<NoBins;k++)
  {
    if (nperbint[k]==0) nperbint[k]=1; if(nperbine[k]==0) nperbine[k]=1; 
		
    if (FilePtrTOF    != NULL) fprintf(FilePtrTOF,    "%lf   %le   %le   %9ld\n", (t[k]+t[k+1])/2.0, prob_t[k], prob_t[k]/sqrt((double)nperbint[k]), nperbint[k]) ;
    if (FilePtrEnergy != NULL) fprintf(FilePtrEnergy, "%lf   %le   %le   %9ld\n", (e[k]+e[k+1])/2.0, prob_e[k], prob_e[k]/sqrt((double)nperbine[k]), nperbine[k]) ;
  }

  fprintf(LogFilePtr, "\ntotal intensity within TOF and E binning: %11.3e  %11.3e\n", TotIntTof, TotIntE);

  /* Do the general cleanup */
 my_exit:
	
  OwnCleanup();
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;

}/* End main() */


/* Transform factor */

double TransformFactor(double DelE)
{
	if(eGeomOption == 0) return SecondaryFlightPath * sqrt(EnergyRef) / sq(EnergyRef + DelE) ;
	if(eGeomOption == 1) return PrimaryFlightPath   / sqrt(EnergyRef) / (EnergyRef - DelE) ;

	else return 0 ;
}



/* Bose factor if w in ueV */

double	BoseFactor(double T, double w)
{
double betha;

	if(T == 0.) return 1. ;

	betha = 11.605 / T ; /* unit 1/meV ! */

	if(w > 0.) return 1 / ((double) exp(betha * w) - 1) +1 ;
	if(w < 0.) return 1 / ((double) exp(- betha * w) - 1) ;

	else return	0. ;
}


/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
  double TOF_total_e=0.0;   // total TOF without energy transfer

	while(argc>1)
	{
		switch(argv[1][1])
		{

			case 'A':
			sscanf(&argv[1][2], "%ld", &eGeomOption) ;
			break;
			case 't':
			sscanf(&argv[1][2], "%ld", &bTofCorr) ;
			break;
			case 'D':
			sscanf(&argv[1][2], "%ld", &bBoseF) ;
			break;

			case 'C':
			sscanf(&argv[1][2], "%ld", &NoBins) ;
			break;
			case 'f':
			sscanf(&argv[1][2], "%ld", &iColor) ;
			break;

			case 'E':
			if((FilePtrTOF = fopen(&argv[1][2],"w"))==NULL)
			{
				printf("\nTOF spectrum file not opened or created\n");
				exit(0);
			}
			if(FilePtrTOF != NULL) FileNameTOF = &argv[1][2];
			break;

			case 'G':
			if((FilePtrEnergy = fopen(&argv[1][2],"w"))==NULL)
			{
				printf("\nenergy spectrum file not opened or created\n");
				exit(0);
			}
			if(FilePtrEnergy != NULL) FileNameEnergy = &argv[1][2];
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
			sscanf(&argv[1][2], "%lf", &Angle) ;
			break;

			case 'k':
			sscanf(&argv[1][2], "%lf", &AngleRange) ;
			break;

		}
		argc--;
		argv++;
	}


  /* check */

  if((eGeomOption != 0) &&	(eGeomOption != 1))
  {
    fprintf(LogFilePtr,"ERROR: wrong geometry option!\n\n") ;
    exit(0) ;
  }

  if((bBoseF != 0) &&	(bBoseF != 1))
  {
	  fprintf(LogFilePtr,"ERROR: wrong option for Bose-factor!\n\n") ;
	  exit(0) ;
  }

  if(MinTOF > MaxTOF && MinE > MaxE)
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

    if(eGeomOption == 0)   // direct geometry
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
      if(eGeomOption == 0)  // direct geometry
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
      if(eGeomOption == 0)  // direct geometry
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


  /* options */
  if (eGeomOption == 0) fprintf(LogFilePtr,"\noption 'direct geometry'\n") ;
  if (eGeomOption == 1) fprintf(LogFilePtr,"\noption 'inverted geometry'\n") ;

  if(FilePtrTOF    != NULL) fprintf(LogFilePtr," TOF spectrum file   : '%s'\n", FileNameTOF) ;
  if(FilePtrEnergy != NULL) fprintf(LogFilePtr," energy spectrum file: '%s'\n", FileNameEnergy) ;

  fprintf(LogFilePtr, " number of bins       : %4ld      \n", NoBins);
  fprintf(LogFilePtr, " primary flight path  : %9.4f m  \n secondary flight path: %9.4f m \n",           PrimaryFlightPath/100.0, SecondaryFlightPath/100.0);
  fprintf(LogFilePtr, " reference wavelength : %9.4f Ang\n time offset          : %9.4f ms\n",           LambdaRef, TimeOffset, MinTOF, MaxTOF);
  fprintf(LogFilePtr, " gradient of timebins : %9.4f    \n angular range        : %9.4f +/-%9.4f deg\n", SlopeBins, Angle, AngleRange);

  if(Temperature == 0.) bBoseF = 0 ;
  if(bBoseF == 0) Temperature = 0. ;
  // if(bBoseF == 0) 	fprintf(LogFilePtr,"\nnot divided by Bose-factor") ;
  // if(bBoseF == 1) 	fprintf(LogFilePtr,"\ndivided by Bose-factor") ;

  /* calculates alpha, beta coeff. for TOF boundary calculation */
  alpha = 1 / (1 - SlopeBins) ;
  beta = 0. ;

  for(k=0;k<NoBins;k++) beta += pow(alpha, k) ;
  beta = (MaxTOF - pow(alpha, NoBins) * MinTOF) / beta ;

  fprintf(LogFilePtr,	"computed reference values:\n");
  fprintf(LogFilePtr, " reference energy     : %9.4f meV\n velocity             : %9.4f cm/ms\n", EnergyRef, VelocityRef);
	fprintf(LogFilePtr,	" effective TOF range  : %9.4f  - %9.4f ms\n"                             , MinTOF, MaxTOF);
	fprintf(LogFilePtr,	" TOF_total_elast      : %9.4f ms \n TOF_fixed_energy_side: %9.4f ms\n"   , TOF_total_e, TofRef);
  fprintf(LogFilePtr, " energy transfer      : %9.4f  - %9.4f meV\n"                            , MinE,   MaxE) ;

  Angle      *= M_PI / 180. ;
  AngleRange *= M_PI / 180. ;

}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{
	if(FilePtrTOF != NULL)fclose(FilePtrTOF) ;

	if(FilePtrEnergy != NULL)fclose(FilePtrEnergy) ;

}/* End OwnCleanup */




