/*********************************************************************************************/
/*  VITESS module 'mon_brilliance.c'                                                         */
/*    monitoring of brilliance as a function of 1 parameter                                  */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given    */
/* to the authors:                                                                           */
/* 1.0  May 2012  K. Lieutenant  initial version (based on monitor1.c)                       */
/* 1.1  Oct 2019  K. Lieutenant  brilliance as a function of energy, logarithmic binning     */
/* 1.2  Feb 2020  K. Lieutenant  new central visualization parameters                        */
/* 1.3  Mar 2021  K. Lieutenant  update after each bundle                                    */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "init.h"
#include "softabort.h"
#include "general.h"
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define MAX_PAR 9


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit  (int argc, char *argv[]);                // Reads input parameters and sets global parameters
void   OpenFiles();                                      // Opens all monitor files
void   UpdateMon(long iBndl);                            // Updates monitor output file 
void   WriteResult();                                    // Writes result to flux file and log file
double E2DelLmbd(const double Emax, const double Emin);  // converts DelE = Emax - Emin (meV) into DelLambda (Ang)
double RangeAvrg(const double Xmin, const double Xmax);  // calculates average value according to binning type


/******************************/
/** Global variables         **/
/******************************/
// Input parameters
char  *MonitorFileName=NULL,              // -O     name of a file to monitor the brilliance
      *RefFileName =NULL,                 // -S     name of a reference file (it is used to calculate the brilliance transfer)
      *FluxFileName=NULL;                 // -F     name of a flux file      (it is used to monitor the average and max. brilliance as a function of any parameter in running a series of simulations)
VtBrlPar  eBrlPar=VT_NOT_DEF;             // -k     defines the variable parameter in brilliance monitoring
VtBrlNorm eBrlNorm=BRL_ABS;               // -N     1: absolute brilliance [n/(cm²s sr Ang)]   2: relative brilliance (= brilliance transfer)   3: brilliance within 1% DelLamdba/Lambda [n/(cm²s sr)]
short  bExclusive=FALSE,                  // -e     criterion: only trajectories within limits are written
       bLogBin   =FALSE;                  // -B     TRUE : bin size increases exponentially  FALSE: linear binning
long   nBins  =1,                         // -n     number of bins in the monitor file
       nColour=ANY_COLOR;                 // -C     color of trajectory that is monitored   (-1=all)
double MinY   =  -1.0e9, MaxY   =  1.0e9, // -y -Y  min. and max. width to be taken into account
       MinZ   =  -1.0e9, MaxZ   =  1.0e9, // -z -Z  min. and max. height to be taken into account
       MinDivY=-180.0,   MaxDivY=180.0,   // -h -H  min. and max. hor. div. to be taken into account
       MinDivZ=-180.0,   MaxDivZ=180.0,   // -v -V  min. and max. vert. div. to be taken into account
       MinDivR=   0.0,   MaxDivR=180.0,   // -r -R  min. and max. radial div. to be taken into account
       MinLmbd=   0.0,   MaxLmbd=1.0e9,   // -l -L  min. and max. wavelength to be taken into account
       MinE   =   0.0,   MaxE   =1.0e9,   // -m -M  min. and max. energy to be taken into account
       MinTime=-1.0e9,   MaxTime=1.0e9,   // -t -T  min. and max. TOF to be taken into account
       Freq=0.0;                          // -f     source frequency   (from simulation.inf)

// Variables determined from input parameters, simulation file or trajectory data or needed in different parts of the module
FILE	*pFileMon =NULL,               // pointer to the monitor output file
      *pFileRef =NULL,               // pointer to the reference file
      *pFileFlux=NULL;               // pointer to the flux file 
short  bPulsedSrc=FALSE;             // defines source type: FALSE: constant source  TRUE: pulsed source
long   nBundle   = 1,                // number of bundles started
       nTrjTot   = 0;                // total number of traj. within binning and eval. time
double IntTot    = 0.0,              // total count rate within binning and eval. time
       BrillAveIn= 0.0;              // average brilliance of reference spectrum
double *pPosT=NULL,                  // limits of bin (minimal and maximal value)
       *pInt =NULL,                  // intensity (=count rate) per bin
       *pNorm=NULL,                  // normalisation value for each bin
       *pNormSD=NULL,                // standard deviation of normalisation value for each bin
       *pSD=NULL;                    // standard deviation per bin 
long   *pBinN=NULL;                  // number of trajectories per bin
double DelLmbd =0.0,                 // width of wavelength band used to calculate the brilliance (transfer)
       DelTime =0.0,                 // space of time used to calculate the brilliance (transfer); only used for pulsed sources
       DelY    =0.0, DelZ   =0.0,    // spatial width and height used to calculate the brilliance (transfer)
       DelDivY =0.0, DelDivZ=0.0,    // hor. and vert. divergence range used to calculate the brilliance (transfer)
       DelDivR =0.0,                 // radial divergence range used to calculate the brilliance (transfer)
       BrillMax=0.0,                 // maximal brilliance
       TransMax=0.0,                 // maximal brilliance transfer
       MinRange=0.0, MaxRange=0.0;   // min. and max. value of the variable parameter
char   sUnit[MAX_PAR+1][ 4]={"", "Ang", "ms", "cm", "cm", "deg", "deg", "deg", "eV"},
       sParN[MAX_PAR+1][22]={"", "wavelength", "time", "horizontal position", "vertical position",
                                  "horizontal divergence", "vertical divergence", "radial divergence", "energy"};
 

/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  char	 sBuffer[512]="";
  short  bRegistered=FALSE;    // criterion: trajectory is within limits set
  long   iBin=0,               // bin index
         iBndl=0,              // current bundle
         i=0;                  // index of trajectories
  double Time=0.0,             // time of flight
         Lmbd=0.0,             // wavelength
         E=0.0,                // energy
         P=0.0,                // weight
         Y=0.0, Z=0.0;         // hor. and vert. position of the neutron under consideration
  double BinSize=0.0,          // size of each bin
         FactLog=1.0,          // ratio of neighbouring bin values for logarithmic binning
         Bins   =0.0,          // number of bins as double value
         MonData[3]={0.0,0.0,0.0};  // data in one monitor row
  double DivY=0.0, DivZ=0.0,   // hor., vert. and radial divergence of the trajectory
         DivR=0.0;            

  /* initialisation */
  /* -------------- */
  _eModule=MCN_MON1_BRL;

  Init   (argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
  OpenFiles();
    
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  pPosT   = (double*) calloc(nBins+1,sizeof(double));
  pInt    = (double*) calloc(nBins+1,sizeof(double));
  pSD     = (double*) calloc(nBins+1,sizeof(double));
  pNorm   = (double*) calloc(nBins+1,sizeof(double));
  pNormSD = (double*) calloc(nBins+1,sizeof(double));
  pBinN   = (long*)   calloc(nBins+1,sizeof(long));

  Bins = (double)nBins;
  if (bLogBin)
    FactLog = pow(MaxRange/MinRange, 1.0/Bins);
  else
    BinSize = (MaxRange - MinRange)/Bins;

  for (iBin=0; iBin<=nBins; iBin++)
  { // logarithmic
	  if (bLogBin)
    { if (iBin==0)
        pPosT[iBin] = MinRange;
      else
        pPosT[iBin] = pPosT[iBin-1] * FactLog;
    }
	  else
    { pPosT[iBin] = MinRange+(BinSize*iBin);  // linear
    }
    pInt [iBin] = 0.0;
    pSD  [iBin] = 0.0;
    pBinN[iBin] = 0;
  
    // for brilliance transfer read reference file and assign these values as normalization
    if (eBrlNorm==BRL_TRANSF)                       
    { ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1);
      StrgScanLF(sBuffer, MonData, 3, 0);
      pNorm  [iBin] = MonData[1];           // brilliance is the second value in brilliance monitor
	    pNormSD[iBin] = MonData[2];         // uncertainty is the third value in brilliance monitor
      BrillAveIn += MonData[1]/nBins;
    }
    else                                    // absolute brilliance
    { pNorm[iBin]   = 1.0;
	    pNormSD[iBin] = 0.0;
    }
  }

  /* output to log file */
  /* ------------------ */
  if (eBrlNorm==BRL_TRANSF)
  { fclose(pFileRef);
    fprintf(LogFilePtr, "brilliance transfer relative to %s ", RefFileName);
  }
  else if (eBrlNorm==BRL_PCT)
  { fprintf(LogFilePtr, "brilliance in 1%% DelLambda/Lambda  [n/(cm^2 s sr)]");
  }
  else
  { fprintf(LogFilePtr, "absolute brilliance  [n/(cm^2 s sr Ang)]");
  }
  if (bPulsedSrc==FALSE)
    fprintf(LogFilePtr, "of a constant source \n");
  else
    fprintf(LogFilePtr, "of a pulsed source of frequency %4.1f Hz \n", Freq);
  fprintf(LogFilePtr, "Binning  : %ld bins in %s from %10.5f to %10.5f %s\n", nBins, sParN[eBrlPar], MinRange, MaxRange, sUnit[eBrlPar]);
  fprintf(LogFilePtr, "File     : %s\n", MonitorFileName);

  /* loop over trajectories */
  /* ---------------------- */
  DECLARE_ABORT;

  while (ReadNeutrons()!= 0) 
  {
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

      // Update monitor output if EOB line is found
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      { 
        iBndl++;
        UpdateMon(iBndl);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        /* write out all neutrons, if 'exclusive counts = no' is set */
        if (bExclusive==FALSE)
          WriteNeutron(&(InputNeutrons[i]));

        bRegistered=FALSE;
        P    = InputNeutrons[i].Probability;
        Lmbd = InputNeutrons[i].Wavelength;
        Time = InputNeutrons[i].Time;
        Y    = InputNeutrons[i].Position[1];
        Z    = InputNeutrons[i].Position[2];
        E    = Lambda2E(Lmbd);                // Ang -> meV
        if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          DivY = 0.0;
        else
          DivY = 180.0/M_PI * atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[0]);
        if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          DivZ = 0.0;
        else
          DivZ = 180.0/M_PI * atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
        DivR = sqrt(sq(DivY) + sq(DivZ));

        /* exclude traj. with wrong colours: (nColour=-1 means: all colours accepted) */
        if (nColour!=ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;

		    /* exclude traj. outside the given ranges */
        if (Lmbd < MinLmbd || Lmbd > MaxLmbd) continue;
        if (Time < MinTime || Time > MaxTime) continue;
        if (Y    < MinY    || Y    > MaxY   ) continue;
        if (Z    < MinZ    || Z    > MaxZ   ) continue;
        if (DivY < MinDivY || DivY > MaxDivY) continue;
        if (DivZ < MinDivZ || DivZ > MaxDivZ) continue;
        if (DivR < MinDivR || DivR > MaxDivR) continue;
        if (E    < MinE    || E    > MaxE   ) continue;

	      // determine the channel for a given trajectory depending on the variable parameter
        if (bLogBin==TRUE)
        { 
          switch (eBrlPar) 
          {
            case VT_LAMBDA  : iBin = (int)floor(nBins * log(Lmbd/MinLmbd)/log(MaxLmbd/MinLmbd)); break;
            case VT_TIME    : iBin = (int)floor(nBins * log(Time/MinTime)/log(MaxTime/MinTime)); break;
            case VT_POS_Y   : iBin = (int)floor(nBins * log(Y   /MinY   )/log(MaxY   /MinY));    break;
            case VT_POS_Z   : iBin = (int)floor(nBins * log(Z   /MinZ   )/log(MaxZ   /MinZ));    break;
            case VT_DIV_HOR : iBin = (int)floor(nBins * log(DivY/MinDivY)/log(MaxDivY/MinDivY)); break;
            case VT_DIV_VERT: iBin = (int)floor(nBins * log(DivZ/MinDivZ)/log(MaxDivZ/MinDivZ)); break;
            case VT_DIV_RAD : iBin = (int)floor(nBins * log(DivR/MinDivR)/log(MaxDivR/MinDivR)); break;
            case VT_ENERGY  : iBin = (int)floor(nBins * log(E   /MinE   )/log(MaxE   /MinE));    break;
          }
        }
        else 
        { 
          switch (eBrlPar) 
          {
            case VT_LAMBDA  : iBin = (int)floor(nBins * (Lmbd - MinLmbd)/(MaxLmbd - MinLmbd)); break;
            case VT_TIME    : iBin = (int)floor(nBins * (Time - MinTime)/(MaxTime - MinTime)); break;
            case VT_POS_Y   : iBin = (int)floor(nBins * (Y    - MinY   )/(MaxY    - MinY));    break;
            case VT_POS_Z   : iBin = (int)floor(nBins * (Z    - MinZ   )/(MaxZ    - MinZ));    break;
            case VT_DIV_HOR : iBin = (int)floor(nBins * (DivY - MinDivY)/(MaxDivY - MinDivY)); break;
            case VT_DIV_VERT: iBin = (int)floor(nBins * (DivZ - MinDivZ)/(MaxDivZ - MinDivZ)); break;
            case VT_DIV_RAD : iBin = (int)floor(nBins * (DivR - MinDivR)/(MaxDivR - MinDivR)); break;
            case VT_ENERGY  : iBin = (int)floor(nBins * (E    - MinE   )/(MaxE    - MinE));    break;
          }
        }

        if (iBin >= 0  &&  iBin < nBins) 
        {
          pInt [iBin] += P;
          pBinN[iBin] += 1;
          IntTot      += P;
          nTrjTot     += 1;
          bRegistered = TRUE;
        }

        /* write out registered neutrons, if 'exclusive counts = yes' is set */
        if((bExclusive==TRUE) && (bRegistered==TRUE))
        {
          WriteNeutron(&(InputNeutrons[i]));
        }
      }
    }
  }

my_exit:
  // Evaluate binned data, write to monitor files and close them
  // -----------------------------------------------------------
  UpdateMon(nBundle);  // final monitor output

  WriteResult();

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------

#ifdef REALLY_FREE_THINGS_THE_OS_KILLS_ELSE
  if (pPosT!=NULL)   free(pPosT);
  if (pInt !=NULL)   free(pInt);
  if (pNorm!=NULL)   free(pNorm);
  if (pNormSD!=NULL) free(pNormSD);
  if (pSD  !=NULL)   free(pSD);
  if (pBinN!=NULL)   free(pBinN);
#endif

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit(int argc, char *argv[])
{
  int    i;
  double time,     // measuring time
         lambda,   // wanted wavelength
         freq,     // source frequency 
         nTraj;    // number of the trajectories started per bundle

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1]) 
      {
        case 'O':
          MonitorFileName=&argv[i][2];
          break;
        case 'S':
          RefFileName=&argv[i][2];
          break;
        case 'F':
          FluxFileName=&argv[i][2];
          break;

        case 'k':
          eBrlPar  = (short) atoi(&argv[i][2]);     // 1: lambda  2: time  3: y  4: z  5: div_y  6: div_z  7: div_rad  8: energy
          break;
        case 'N':
          eBrlNorm = (short) atoi(&argv[i][2]);     // 1: absolute brilliance [n/(cm²s sr Ang)]   2: brilliance transfer   3: brilliance within 1% DelLamdba/Lambda [n/(cm²s sr)]
          break;
				case 'B':
					bLogBin  = (short) atoi(&argv[i][2]);     // TRUE : bin size increases exponentially   FALSE: linear binning
					break;

        case 'n':
          nBins = atol(&argv[i][2]);     // number of bins
          break;

        case 'l':
          MinLmbd = atof(&argv[i][2]);   // lower bound wavelength [Å]
          break;
        case 'L':
          MaxLmbd = atof(&argv[i][2]);   // upper bound wavelength [Å]
          break;

        case 'm':
          MinE = atof(&argv[i][2]);      // lower bound energy [meV]
          break;
        case 'M':
          MaxE = atof(&argv[i][2]);      // upper bound energy [meV]
          break;

        case 'y':
          MinY = atof(&argv[i][2]);      // lower bound width [cm]
          break;
        case 'Y':
          MaxY = atof(&argv[i][2]);      // upper bound width [cm]
          break;

        case 'z':
          MinZ = atof(&argv[i][2]);      // lower bound height [cm]
          break;
        case 'Z':
          MaxZ = atof(&argv[i][2]);      // upper bound height [cm]
          break;

        case 'h':
          MinDivY = atof(&argv[i][2]);      // lower bound hor. divergence [deg]
          break;
        case 'H':
          MaxDivY = atof(&argv[i][2]);      // upper bound hor. divergence [deg]
          break;

        case 'v':
          MinDivZ = atof(&argv[i][2]);      // lower bound vert. divergence [deg]
          break;
        case 'V':
          MaxDivZ = atof(&argv[i][2]);      // upper bound vert. divergence [deg]
          break;

        case 'r':
          MinDivR = atof(&argv[i][2]);      // lower bound radial divergence [deg]
          break;
        case 'R':
          MaxDivR = atof(&argv[i][2]);      // upper bound radial divergence [deg]
          break;

        case 'e':
          bExclusive = (short) atoi(&argv[i][2]);  // if activated, only neutrons meeting the monitor conditions are considered further on
          break;
        case 'C':
          nColour = atol(&argv[i][2]);       //  excludes all neutrons with diff. Colour, if nColour >= 0
          break;

        case 't':
          MinTime = atof(&argv[i][2]);   // minimal time for monitoring [s]
          break;
        case 'T':
          MaxTime = atof(&argv[i][2]);   // maximal time for monitoring [s]
          break;
        case 'f':
          Freq = atof(&argv[i][2]);      // frequency of the pulsed source [Hz]
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  // parameter must be chosen
  if (eBrlPar==VT_NOT_DEF) 
    Error("variable parameter missing");

  // pulsed source assumed if time is the variable parameter or frequency > 0
  if (eBrlPar==VT_TIME  || Freq > 0.0) 
    bPulsedSrc=TRUE;

  // Read information from 'simulation.inf' if not given here
  ReadSimData(&time, &lambda, &freq, &nTraj, &nBundle);
  if (bPulsedSrc==TRUE && Freq==0.0)
    Freq=freq;
  if (Freq!=freq)
    Warning("Frequency given here differs from frequency used in the source");

  // ranges of phase space
  if (bPulsedSrc==TRUE)
  {  if (MinTime==-1.0e9 || MaxTime==1.0e9)
      DelTime = 1.0/Freq;
    else
      DelTime = (MaxTime - MinTime)/1000.0;   // ms -> s;  only used for time dependent brilliance
  }

  // calculate wavelength band from energy for an energy dependent monitor or if the wavelength range is not given
  if (eBrlPar==VT_ENERGY || (MaxLmbd > 100.0 && MaxE < 1.0e6) )
    DelLmbd =  E2DelLmbd(MaxE, MinE);
  else
    DelLmbd =  MaxLmbd - MinLmbd;

  DelY    =  MaxY - MinY;
  DelZ    =  MaxZ - MinZ;
  DelDivY = (MaxDivY - MinDivY) * M_PI/180.0; // deg -> rad
  DelDivZ = (MaxDivZ - MinDivZ) * M_PI/180.0; // deg -> rad
  DelDivR = (MaxDivR - MinDivR) * M_PI/180.0; // deg -> rad

  // check if file name is given
  if (MonitorFileName==NULL) 
  {
    fprintf(LogFilePtr,"you must define a MonitorOutputFile\n");
    exit(99);
  }

  // set range for monitor file
  switch (eBrlPar)
  {
    case VT_LAMBDA  : MinRange = MinLmbd; MaxRange = MaxLmbd; break;  // monitor lambda dependent brilliance
    case VT_TIME    : MinRange = MinTime; MaxRange = MaxTime; break;  // monitor time dependent brilliance
    case VT_POS_Y   : MinRange = MinY;    MaxRange = MaxY;    break;  // monitor y dependent brilliance
    case VT_POS_Z   : MinRange = MinZ;    MaxRange = MaxZ;    break;  // monitor z dependent brilliance
    case VT_DIV_HOR : MinRange = MinDivY; MaxRange = MaxDivY; break;  // monitor div_y dependent brilliance
    case VT_DIV_VERT: MinRange = MinDivZ; MaxRange = MaxDivZ; break;  // monitor div_z dependent brilliance
    case VT_DIV_RAD : MinRange = MinDivR; MaxRange = MaxDivR; break;  // monitor radial divergence dependent brilliance
    case VT_ENERGY  : MinRange = MinE;    MaxRange = MaxE;    break;  // monitor energy dependent brilliance
    default:  Error("Kind of brilliance calculation unknown");
  }

  // check if the range could be determined from the values for the chosen paramter
  if (MinRange==-1.0e9 || MinRange==1.0e9)
    Error("range was not given for the chosen paramter");

  // check if the lower bound value is positive for logarithmic binning
  if (bLogBin && MinRange<=0.0)
    Error("Lower bound value of the range must be positive for logarithmic binning");

  return;
}


/*******************************************************/
/**  Opens all files                                  **/
/*******************************************************/
void OpenFiles()
{
  // opens reference file if needed
  if (eBrlNorm==BRL_TRANSF)
  { if (RefFileName!=NULL)
    { pFileRef = OpenInputFile(RefFileName, FALSE, "rt");
      if (pFileRef==NULL)
      { fprintf(LogFilePtr,"\nERROR: Reference file %s could not be opened\n", RefFileName);
        exit(-1);
      }
    }
    else
    { Error("Reference file missing. This is needed for monitoring the brilliance transfer");
    }
  }

  // opens flux file
  if (FluxFileName!=NULL)
  { pFileFlux = OpenOutputFile(FluxFileName, FALSE, "at");
    if (pFileFlux==NULL)
      fprintf(LogFilePtr,"\nWARNING: Flux file %s could not be opened\n", FluxFileName);
  }
 
  return;
}


/*******************************************************/
/**  Updates monitor output file                      **/
/*******************************************************/
void UpdateMon(long iBndl)
{
  double ParCntr=0.0,          // center of a bin of the variable parameter
         PhaseSpaceVol=0.0,    // phase space volume of a bin
         LmbdPrz      =1.0,    // percentage DelLambda/Lambda
         LmbdAve      =0.0,    // average wavelength in a bin
         Brilliance   =0.0,    // brilliance within one bin
         Transmission =1.0;    // brilliance transfer within one bin
  long   iBin=0;               // bin index

  // opens monitor output file
  pFileMon = OpenOutputFile(MonitorFileName, TRUE, "wt");

  if (pFileMon!=NULL) 
    WriteHeader1DB(pFileMon, "brilliance", ANY_COLOR, iBndl, nBundle, nBins, IntTot, nTrjTot, sParN[eBrlPar], sUnit[eBrlPar]);

  for (iBin=0; iBin < nBins; iBin++) 
  {
    switch(eBrlPar)
    { case VT_LAMBDA  : DelLmbd = (pPosT[iBin+1] - pPosT[iBin]);              break;
      case VT_TIME    : DelTime = (pPosT[iBin+1] - pPosT[iBin]) / 1000.0;     break; // ms -> s
      case VT_POS_Y   : DelY    = (pPosT[iBin+1] - pPosT[iBin]);              break;
      case VT_POS_Z   : DelZ    = (pPosT[iBin+1] - pPosT[iBin]);              break;
      case VT_DIV_HOR : DelDivY = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
      case VT_DIV_VERT: DelDivZ = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
      case VT_DIV_RAD : DelDivR = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
      case VT_ENERGY  : DelLmbd = E2DelLmbd(pPosT[iBin+1], pPosT[iBin]);      break; 
    }

    // center of the bin
    ParCntr = RangeAvrg(pPosT[iBin], pPosT[iBin+1]);

    // calculate brilliance
    if (eBrlPar==VT_DIV_RAD) // radial divergence
      PhaseSpaceVol = DelLmbd * DelY * DelZ * 2*M_PI * ParCntr*M_PI/180.0 * DelDivR;
    else
      PhaseSpaceVol = DelLmbd * DelY * DelZ * DelDivY * DelDivZ;

    if (bPulsedSrc==FALSE)
      Brilliance = pInt[iBin] / PhaseSpaceVol;
    else
      Brilliance = pInt[iBin] / PhaseSpaceVol / Freq / DelTime;

    // normalisation to 1% in DelLambda/Lambda
    if (eBrlNorm==BRL_PCT)
    { 
      if (eBrlPar==VT_LAMBDA)
        LmbdAve = ParCntr;
      else if (eBrlPar==VT_ENERGY)
        LmbdAve = RangeAvrg(E2Lambda(pPosT[iBin]), E2Lambda(pPosT[iBin+1]));  // meV -> Ang
      else 
        LmbdAve = RangeAvrg(MinLmbd, MaxLmbd);

      LmbdPrz = 100.0 * 1.0 / LmbdAve;       // Brilliance calculated per Ang; this gives the calculated percentage in DelLmbd/Lmbd 
      Brilliance /= LmbdPrz;
    }
 
    BrillMax = Max(BrillMax, Brilliance);
        
    // calculate transmission for brilliance transfer 
    // write transmission or brilliance to monitor file
    if (eBrlNorm==BRL_TRANSF)
    { if (pNorm[iBin] > 0.0)
		    Transmission = Brilliance / pNorm[iBin];
		  else
        Transmission = 1.0;
      TransMax = Max(TransMax, Transmission);

      if(pBinN[iBin]!=0)
        pSD[iBin] = Transmission * sqrt( 1/((double)pBinN[iBin]) + sq(pNormSD[iBin]/pNorm[iBin]) );
      else
        pSD[iBin] = 0.0;

      if (pFileMon != NULL) 
        fprintf(pFileMon, "%10.3f  %12.5e %12.5e  %7ld\n", ParCntr, Transmission, pSD[iBin], pBinN[iBin]);
    }
    else
    {
      if(pBinN[iBin]!=0)
        pSD[iBin] = Brilliance * sqrt( 1/((double)pBinN[iBin]) + sq(pNormSD[iBin]/pNorm[iBin]) );
      else
        pSD[iBin] = 0.0;

      if (pFileMon != NULL) 
        fprintf(pFileMon, "%10.3f  %12.5e %12.5e  %7ld\n", ParCntr, Brilliance, pSD[iBin], pBinN[iBin]);
    }
  }

  if (pFileMon!=NULL) 
    fclose(pFileMon);
}


/*******************************************************/
/**  Writes result to flux file and log file          **/
/*******************************************************/
void   WriteResult()
{
  double BrillAve    =0.0,     // average brilliance
         BrillTiAv   =0.0,     // brilliance averaged over the whole period (assuming that the complete pulse is within the given time range)
         TransAve    =0.0,     // average brilliance transfer
         PhaseSpaceVolTot=0.0; // phase space volume of the whole range

  // calculate average brilliance
  if (eBrlPar==VT_DIV_RAD)
    PhaseSpaceVolTot = (MaxLmbd - MinLmbd) * (MaxY - MinY) * (MaxZ - MinZ) * M_PI * (sq(M_PI/180.0*MaxDivR) - sq(M_PI/180.0*MinDivR));
  else
    PhaseSpaceVolTot = (MaxLmbd - MinLmbd) * (MaxY - MinY) * (MaxZ - MinZ) * M_PI/180.0*(MaxDivY - MinDivY) * M_PI/180.0*(MaxDivZ - MinDivZ);

  if (bPulsedSrc==FALSE)
  { BrillAve  = IntTot / PhaseSpaceVolTot;
    BrillTiAv = BrillAve; 
  }
  else
  { BrillAve  = IntTot / (PhaseSpaceVolTot * Freq * (MaxTime - MinTime)/1000.0);    // ms -> s
    BrillTiAv = IntTot /  PhaseSpaceVolTot; 
  }

  /* normalisation to 1% in DelLambda/Lambda
  if (eBrlNorm==BRL_PCT)
  { 
    LmbdAve = RangeAvrg(MinLmbd, MaxLmbd);
    LmbdPrz = 100.0 * 1.0 / LmbdAve;       // Brilliance calculated per Ang; this gives the calculated percentage in DelLmbd/Lmbd 
    Brilliance /= LmbdPrz;
  }*/

  // write out one line into the flux file
  if (pFileFlux!=NULL) 
  {
    fprintf(pFileFlux, "%10.3f %10.3f %10.3f %10.3f %10.3f %10.3f     %12.5e     %12.5e     %12.5e \n",
            0.5*(MaxLmbd+MinLmbd), 0.5*(MaxY+MinY), 0.5*(MaxZ+MinZ), 0.5*(MaxDivY+MinDivY), 0.5*(MaxDivZ+MinDivZ), 0.5*(MaxDivR+MinDivR),
            BrillAve, BrillMax, BrillTiAv);
    fclose(pFileFlux);
  }

  // write total and average values to log file
  fprintf(LogFilePtr, "total neutron count rate within given ranges: %12.5e n/s \n", IntTot);
  fprintf(LogFilePtr, "average and maximal brilliance         : %12.5e  %12.5e n/(cm^2 s Ang sterad)\n", BrillAve, BrillMax);
  if (eBrlNorm==BRL_TRANSF)
  { TransAve = BrillAve/BrillAveIn;
    fprintf(LogFilePtr, "average and maximal brilliance transfer: %7.3f  %7.3f \n", TransAve, TransMax);
  }
  if (bPulsedSrc==TRUE)
    fprintf(LogFilePtr, "time averaged brilliance               : %12.5e              n/(cm^2 s Ang sterad)\n", BrillTiAv);
  fprintf(LogFilePtr, "\n");
}


/***********************************************************************************/
/* E2DelLmbd:                                                                      */
/* convert DelE = Emax - Emin (meV) into DelLambda (Ang)                           */
/***********************************************************************************/
double E2DelLmbd(const double Emax, const double Emin)
{
  double LmbdMin=E2Lambda(Emax),   // meV -> Ang
         LmbdMax=E2Lambda(Emin);

  return(LmbdMax-LmbdMin);
}


/***********************************************************************************/
/* RangeAvrg:                                                                      */
/* calculate average value according to binning type                               */
/***********************************************************************************/
double RangeAvrg(const double Xmin, const double Xmax)
{
  double Xavrg;

  if (bLogBin)
    Xavrg = sqrt(Xmin * Xmax);
  else
    Xavrg = (Xmin + Xmax)/2.0;

  return(Xavrg);
}
