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
/* 1.7  Jun 2023  K. Lieutenant  header + update                                                        */
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
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define	STRING_BUFFER 50
#define	BINS_BUFFER 5000

/*********************************/
/** Global Variables            **/
/*********************************/
// Input parameters
char	  	*FileNameTOF=NULL,            // -E   [-]   output file for the TOF spectrum
          *FileNameEnergy=NULL;         // -G   [-]   output file for showing the energy transfer spectrum
VtInstGeom eGeomOption=VT_NO_I_GEOM;    // -A   [-]   geometry option:   0: direct geometry   1: indirect geometry
short      bTofCorr= TRUE,              // -t   [-]   criterion: correction to true flight path length from sample to detector:  0: no   1: yes
           bBoseF  = FALSE,             // -D   [-]   criterion: divide by Bose factor  (not used in Vitess 3)  
           iColor=ANY_COLOR;            // -f   [-]   neutron 'color' to evaluate  -1: all neutrons
long       nBins=0;                     // -C   [-]   number of bins in TOF and energy spectrum
          
double     PrimaryFlightPath  =0.0,     // -a  [cm]   Distance from moderator to sample.
           SecondaryFlightPath=0.0,     // -b  [cm]   Distance from sample to detector
           LambdaRef =0.0,              // -c  [Ang]  reference wavelength 
           TimeOffset=0.0,              // -d  [ms]   average time of neutrons at start
           MinE  = 0.0,                 // -m  [meV]  minimal energy transfer to be monitored
           MaxE  = 0.0,                 // -M  [meV]  maximal energy transfer to be monitored
           MinTOF=-1.0,                 // -e  [ms]   minimal time of flight to be monitored
           MaxTOF=-1.0,                 // -g  [ms]   maximal time of flight to be monitored
           SlopeBins  = 0.0,            // -h   [-]   
           Temperature= 0.0,            // -i   [K]   temperature for Bose factor  (not used in Vitess 3)
           AngleCntr  = 0.0,            // -j  [deg]  horizontal angular range that is processed
           AngleRange=180.0;            // -k  [deg]  [AngleCntr - AngleRange, AngleCntr + AngleRange]

// Variables determined from input parameters or trajectory data
long       nTrjTotT   = 0,              //      [-]   total number of trajectories within TOF binning
           nTrjTotE   = 0,              //      [-]   total number of trajectories within energy binning
           nBunches   = 1,              //      [-]   number of bunches started
           nTrjT[BINS_BUFFER+1],        //      [-]   number of trajectories contributing to count rate in the TOF bins 
           nTrjE[BINS_BUFFER+1];        //      [-]   number of trajectories contributing to count rate in the energy bins 
double     EnergyRef  =0.0,             //     [meV]  energy and corresponding to reference wavelength
           VelocityRef=0.0,             //    [cm/ms] energy and velocity and corresponding to reference wavelength
           TofRef   =0.0,               //     [ms]   TOF for the reference part, i.e. primary flight path for direct geometry and secondary for indirect geometry
           TotIntTOF=0.0,               //     [n/s]  total intensity within TOF binning 
           TotIntE  =0.0,               //     [n/s]  total intensity within E binning
           t[BINS_BUFFER+1],            //     [ms]   limits of the time channels  (min. and max. value) 
           e[BINS_BUFFER+1],            //     [meV]  limits of the energy channels  (min. and max. value) 
           prob_t[BINS_BUFFER+1],       //     [n/s]  count rates in the time channels 
           prob_e[BINS_BUFFER+1];       //     [n/s]  count rates in the energy channels  
double     alpha=0.0, beta=0.0;
char       sPar [2][9]={"TOF", "energy"},//           name of the x-axis parameter
           sUnit[2][6]={"ms", "meV"};    //           unit of the x-axis parameter



/******************************/
/** Prototypes               **/
/******************************/
double TransformFactor(double DeltaE);    // Transform factor     
double BoseFactor(double T, double w);    // Bose factor
void   OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables
void   InitArrays();                      // Allocates memory and initializes evaluation arrays 
void   OwnCleanup();                      // Closes files   
void   UpdateMon(long iBnch);             // Updates evaluation output file 


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  long   i=0, k=0, 
         iBnch=0;                // current bunch 
  double PathToDetection=0.0,   // real pathlength fro sample to detector for individual trajectory
         TofToDet=0.0,          // TOF til detector - TOF offset
         rotz=0.0, roty=0.0,    // hor. and vertical Euler angles to describe flight direction    [rad]
         DelE     =0.0;         // energy gain (DelE > 0) or loss

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_EVAL1_INELAST;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.7");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  InitArrays();
  nBunches = ReadNumBnch();

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
        iBnch++;
        UpdateMon(iBnch);
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
            TotIntTOF += InputNeutrons[i].Probability; 
            nTrjT[k] += 1 ;
            nTrjTotT++;
          }
          if( (DelE > e[k]) && (DelE <= e[k+1]) )
          { prob_e[k] += InputNeutrons[i].Probability; // * TransformFactor(DelE) / BoseFactor(Temperature, DelE) ; 
            TotIntE   += InputNeutrons[i].Probability; 
            nTrjE[k] += 1 ;
            nTrjTotE++;
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
  // Output
  fprintf(LogFilePtr, "\ntotal intensity within TOF and E binning: %11.3e  %11.3e\n", TotIntTOF, TotIntE);

  // writes evaluation output
  UpdateMon(nBunches);  
	
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
  char   sInstGeom[21], sErrMsg[50];
  double TOF_total_e=0.0;   // total TOF without energy transfer
  int k;

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
        if (nBins >= BINS_BUFFER)
        { sprintf(sErrMsg, "Number of channels exceeds maximum value of %d", BINS_BUFFER);
          Error(sErrMsg);
        }
        break;
      case 'f':
        iColor = atoi(&argv[1][2]);
        break;

      case 'E':
        FileNameTOF = &argv[1][2];
        break;
      case 'G':
	      FileNameEnergy = &argv[1][2];
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
  if (FileNameEnergy==NULL || FileNameTOF==NULL)
    Error("No output file given");

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

  if(FileNameTOF    != NULL) fprintf(LogFilePtr," TOF spectrum file   : '%s'\n", FileNameTOF) ;
  if(FileNameEnergy != NULL) fprintf(LogFilePtr," energy spectrum file: '%s'\n", FileNameEnergy) ;

  fprintf(LogFilePtr, " number of bins       : %4ld     \n", nBins);
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

  for (k=0;k<nBins;k++) 
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


/********************************************************/
/** Allocates memory and initializes evaluation arrays **/
/********************************************************/
void InitArrays()
{
  long k=0;     // index of channel

  /* calculates TOF channel boundaries and init p_TOF*/
  t[0] = MinTOF ; 
  e[0] = MinE ;
  prob_t[0] = prob_e[0] = 0.0; 
  nTrjT[0]  = nTrjE[0] = 0;

  for(k=1; k<=nBins; k++)
  {
    t[k] = alpha *t[k-1] + beta ;
    e[k] = e[k-1] + (MaxE - MinE) / nBins ;

    prob_t[k] = prob_e[k]   = 0.0; 
    nTrjT[k]  = nTrjE[k] = 0;
  }
}


/*******************************************************/
/* closes files                                        */
/*******************************************************/
void OwnCleanup()
{
	return;
}/* End OwnCleanup */


/*******************************************************/
/**  Updates evaluation output file                   **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  long   iBin=0;                 // index of bins in x-axix and for main monitor
  double BinCtr=0.0,             // center of the current bin 
         sigma=0.0,              // standard deviation of the intensity in the bin
         f_norm =1.0;            // ratio of total to processed bunches after treating current bunch
  FILE	*pFileTOF=NULL,          // pointers to output files
        *pFileEnergy=NULL;

  // open files
  if (FileNameTOF!=NULL)
    pFileTOF = OpenOutputFile(FileNameTOF, FALSE, "wt");
  if (FileNameEnergy!=NULL)
    pFileEnergy = OpenOutputFile(FileNameEnergy, FALSE, "wt");

  // write spectra 
  if (pFileTOF != NULL)
  {
    WriteHeader1DB(pFileTOF, TRUE, "intensity", ANY_COLOR, iBnch, nBunches, nBins, TotIntTOF, nTrjTotT, sPar[0], sUnit[0]);

    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;
    else
      f_norm =1.0;

    for (iBin = 0; iBin < nBins; iBin++)
    {	
      BinCtr = (t[iBin]+t[iBin+1])/2.0;

      if (nTrjT[iBin]==0) 
        sigma = 0.0;
      else
        sigma = prob_t[iBin]/sqrt((double)nTrjT[iBin]); 

      fprintf(pFileTOF,"%10.4f  %12.5e %12.5e  %7ld\n", BinCtr,  f_norm*prob_t[iBin], f_norm*sigma, nTrjT[iBin]);
    }
    fclose(pFileTOF);
  }

  if (pFileEnergy != NULL)
  {
    WriteHeader1DB(pFileEnergy, TRUE, "intensity", ANY_COLOR, iBnch, nBunches, nBins, TotIntE, nTrjTotE, sPar[1], sUnit[1]);

    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;
    else
      f_norm =1.0;

    for (iBin = 0; iBin < nBins; iBin++)
    {	
      BinCtr = (e[iBin]+e[iBin+1])/2.0;

      if (nTrjE[iBin]==0) 
        sigma = 0.0;
      else
        sigma = prob_e[iBin]/sqrt((double)nTrjE[iBin]); 

      fprintf(pFileEnergy,"%10.4f  %12.5e %12.5e  %7ld\n", BinCtr,  f_norm*prob_e[iBin], f_norm*sigma, nTrjE[iBin]);
    }
    fclose(pFileEnergy);
  }
}