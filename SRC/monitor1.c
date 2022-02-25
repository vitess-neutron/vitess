/***********************************************************************************************/
/*  VITESS module 'monitor1.c'                                                                 */
/*    monitoring of intensity as a function of 1 parameter                                     */
/*                                                                                             */
/* The free non-commercial use of these routines is granted providing due credit is given      */
/* to the authors:                                                                             */
/* 1.0      1997  S. Schorr                                                                    */
/* 1.1  JUL 2002  G. Zsigmond    extended                                                      */
/* 1.2  MAR 2003  G. Zsigmond    error bars output and option for normalisation with binsize   */
/*                               included                                                      */
/* 1.3  NOV 2003  K. Lieutenant  monitoring dependent on colour                                */
/* 1.4  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                                  */
/* 1.5  OKT 2004  K. Lieutenant  no abort, if output file cannot be opened; peak flux; count   */
/*                               rate within binning; protocol; no limit in number of binnings */
/* 1.5a DEC 2004  K. Lieutenant  total no of trajectories within ...                           */
/* 1.5b MAR 2005  K. Lieutenant  correction peak flux                                          */
/* 1.6  FEB 2005  K. Lieutenant  intensity as a function of energy                             */
/* 1.6a JAN 2010  A. Houben      filter for wavelength and yz position (only if applicable)    */
/* 1.7  MAY 2010  A. Houben      monitor for div on x-axis rotated by rot angle                */
/* 1.8  Aug 2012  K. Lieutenant  multiple file output                                          */
/* 1.9  Feb 2012  K. Lieutenant  any color = -1                                                */
/* 1.10 Feb 2020  K. Lieutenant  tidy up, new central visualization parameters                 */
/* 1.11 Nov 2020  K. Lieutenant  preparation for transfer to version 4                         */
/* 1.12 mar 2021  K. Lieutenant  update after each bundle                                      */
/***********************************************************************************************/

// includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "convert.h"
#include "init.h"
#include "softabort.h"
#include "mon2_header.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define MAX_KIND  8
#define MAX_COLS 25


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit      (int argc, char *argv[]);                                  // Reads input parameters and sets global parameters
void InitArrays   ();
void OpenFiles    ();                                                        // Opens all monitor files
void UpdateMon    (int iMon, long iBndl);                                    // Updates monitor output file 
void NumerateName (char* sFileLong, char* sFileShort, const short nNumber);  // Building a combined file name of 'sFileShort' and 'sNumber' without changing the extension
void ChangeName   (char* sFileNew,  char* sFileOld);                         // Putting "new_" in front of the original name                                              


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
char  *MonFileName    = NULL,      // -O    [-]   Monitor output file containing intensity as a function of the chosen parameter
      *RefFileName    = NULL;      // -R    [-]   reference file containing input data to normalize the monitor data

VtMon1Par ePar        = MON_DIV_YZ;// -k    [-]   ID for parameter, as a function of which the intensity is shown
VtMonNorm eNormalize  = NO_NORM;   // -f    [-]   enum: NO_NORM      : intensities of the neutron trajctories distributed unchanged into channels 
                                   //                   NORM_BIN_SIZE: intensities normalized to bin size
                                   //                   NORM_REF_FILE: intensities normalized to reference file
short  bAllFiles      = FALSE,     // -c    [-]   flag: generates additional files for colour=0, 1, ..., iColour
       bProbWeight    = TRUE,      // -p    [-]   flag: YES: Probability weight   NO: number of trajectories
       bSplitWeight   = TRUE,      // -P    [-]   flag for yz: YES split weight   NO: multiplication by number of detection angles
       bExclusive     = FALSE,     // -e    [-]   flag: YES: only neutrons meeting the monitor conditions are written  NO: all are written
       iColour        = ANY_COLOR; // -C    [-]   index: for bAllFiles=FALSE: excludes all neutrons with diff. Colour from monitoring , if iColour >= 0 
                                   //                    for bAllFiles=TRUE : max. colour to which additional monitor files are generated             
                                 
long   nBins          = 1;         // -n    [-]   number of monitor channels
                                 
double MinY           = 0.0,       // -m   [var]  lower bound value of the monitored range 
       MaxY           = 1.0,       // -M   [var]  upper bound value of the monitored range
       FiltLambdaMin  =-1.0,       // -l   [Ang]  minimal wavelength to be taken into account
       FiltLambdaMax  = 1.0e10,    // -L   [Ang]  maximal wavelength to be taken into account
       FiltYMin       =-1.0e10,    // -y    [cm]  minimal horizontal position to be taken into account
       FiltYMax       = 1.0e10,    // -Y    [cm]  maximal horizontal position to be taken into account
       FiltZMin       =-1.0e10,    // -z    [cm]  minimal vertical position to be taken into account
       FiltZMax       = 1.0e10,    // -Z    [cm]  maximal vertical position to be taken into account
       FiltTimeMin    =-1.0e10,    // -t    [ms]  minimal TOF to be taken into account
       FiltTimeMax    = 1.0e10,    // -T    [ms]  maximal TOF to be taken into account
       RotAngMin      = 0.0,       // -a   [deg]  lower bound rotation projection angle
       RotAngMax      = 0.0,       // -A   [deg]  lower bound rotation projection angle
       RotAngStep     = 0.0;       // -s   [deg]  rotation projection angle step

// Variables determined from input parameters or trajectory data
short  nAddMons       = 0;         //       [-]   number of additional output files (for separate colours) */                
long   nRot           = 1;         //       [-]   number of rot angles for yz
FILE	*pFileMon       = NULL,      //       [-]   monitor output file
      *pFileRef       = NULL;      //       [-]   reference file     
FILE  *pFileMonC[MAX_COLS]={NULL, NULL, NULL, NULL,NULL, NULL,NULL, NULL};  // Pointer to additional monitor files

// Variables needed in different parts of the program
double *PosT=NULL,          // limits of bin (minimal and maximal value)
       *Int=NULL,           // intensity (=count rate) per bin 
       *Norm=NULL,          // normalisation value for each bin
       *SD=NULL;            // standard deviation per bin      
long   *nBin=NULL;          // number of trajectories per bin  
long    nTrjTot=0;          // total number of traj. within binning and eval. time
double  IntTot=0.0;         // total count rate within binning and eval. time     
double  IntMax =-1.0e10,    // maximal count rate found in one bin
        BinSize= 0.0;       // size of each bin          
long    nBundle= 1;         // number of bundles started 
static
char   sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV", "deg"},                   // unit and parameter name
       sParN[MAX_KIND+1][22]={"", "wavelength", "time", "horizontal divergence", "vertical divergence",  // of the possible x-axis parameters
                               "horizontal position",   "vertical position", "energy", "divergence yz"};


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  char   sCompName  [21]="",
         sModVsnName[40]="";

  short  iCol=NO_COLOR;      /* colour of the trajectory */
  long   iBin=0,             /* bin number */
         iBndl=0,            // current bundle
         i=0, 
         bRegistered=FALSE; 
  double prob=0.0,           /* neutron weight */
         time; 
  double Divy=0.0, Divz=0.0, 
         Div=0.0, 
         rotang=0.0;

  double TimeMeas=0.0,       /* measuring time     (from simulation.inf, not needed) */
         LmbdWant=0.0,       /* desired wavelength (from simulation.inf, not needed) */
         Freq    =0.0,       /* source frequency   (from simulation.inf)   */
         nTraj   =0.0;       /* number of trajectories started per bundle  */


  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON1;

  Init   (argc, argv, _eModule);
  OwnInit(argc, argv);

  CompID2Name (sCompName, _eModule);
  sprintf(sModuleName, "%s_%s",      sCompName, sParN[ePar]);
  sprintf(sModVsnName, "%s_%s 1.12", sCompName, sParN[ePar]);
  print_module_name(sModVsnName);

  OpenFiles();

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  InitArrays();
  ReadSimData(&TimeMeas, &LmbdWant, &Freq, &nTraj, &nBundle);
  
  DECLARE_ABORT;
  
  // loop over trajectories
  // ----------------------
	while (ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;

      // Update monitor output if EOB line is found
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      { 
        iBndl++;
        UpdateMon(ANY_COLOR, iBndl);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {  bRegistered=0;

        if(bProbWeight==1) 
          prob = InputNeutrons[i].Probability;
        else
          prob = 1.0;

        time = InputNeutrons[i].Time;
        iCol = InputNeutrons[i].Color;

        /* write out all neutrons, if 'exclusive counts = no' is set */
        if (bExclusive==0)
          WriteNeutron(&(InputNeutrons[i]));

        /* exclude traj. with wrong colours: (iColour=-1 means: all colours accepted) */
        if (iColour != ANY_COLOR && iColour!=InputNeutrons[i].Color) continue;
        if (FiltLambdaMin >= 0. && InputNeutrons[i].Wavelength < FiltLambdaMin) continue;
        if (FiltLambdaMax >= 0. && InputNeutrons[i].Wavelength > FiltLambdaMax) continue;
        if (InputNeutrons[i].Position[1] < FiltYMin) continue;
        if (InputNeutrons[i].Position[1] > FiltYMax) continue;
        if (InputNeutrons[i].Position[2] < FiltZMin) continue;
        if (InputNeutrons[i].Position[2] > FiltZMax) continue;

        switch (ePar)
        {
          case MON_LAMBDA: // monitor lambda
            iBin = (int)floor((double)nBins*(InputNeutrons[i].Wavelength - MinY)/(MaxY-MinY));
            break;

          case MON_TIME: // monitor time
            iBin = (int)floor(nBins*(InputNeutrons[i].Time - MinY)/(MaxY-MinY));
            break;

          case MON_DIV_Y: // monitor div_y
            if (InputNeutrons[i].Vector[0] >= 0) 
              Divy = (double)atan2(InputNeutrons[i].Vector[1],  sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	          else 
              Divy = (double)atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
            Divy *= 180.0/M_PI;
            iBin = (int)floor(nBins*(Divy - MinY)/(MaxY-MinY));
            break;

          case MON_DIV_Z: // monitor div_z
            Divz=(double)atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));
            Divz*=180.0/M_PI;
            iBin = (int)floor(nBins*(Divz - MinY)/(MaxY-MinY));
            break;

          case MON_Y: // monitor y
            iBin = (int)floor(nBins*(InputNeutrons[i].Position[1] - MinY)/(MaxY-MinY));
            break;

          case MON_Z: // monitor z
            iBin = (int)floor(nBins*(InputNeutrons[i].Position[2] - MinY)/(MaxY-MinY));
            break;

          case MON_ENERGY: // monitor energy
            iBin=(int)floor((double)nBins*(0.001*ENERGY_FROM_LAMBDA(InputNeutrons[i].Wavelength) - MinY)/(MaxY-MinY));
            break;
		
          case MON_DIV_YZ: // monitor div_yz_angle
            Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
            Divy*=180.0/M_PI;
            if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divy=0.0;

            Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
            Divz*=180.0/M_PI;
            if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divz=0.0;
            //x' = x cos f - y sin f
		
            rotang = RotAngMin;
            do {
              Div = Divy * cos(-rotang) - Divz * sin(-rotang);

              iBin = (int)floor(nBins*(Div - MinY)/(MaxY-MinY));
              if(iBin>=0 && iBin<nBins && time>=FiltTimeMin && time<=FiltTimeMax)
                {
                  Int [iBin] += prob/nRot;
                  nBin[iBin] += 1;
                  IntTot      += prob/nRot;
                  nTrjTot     += 1;
                  bRegistered = 1;
                }
              rotang += RotAngStep;
            } while (RotAngStep > 0.0 && rotang <= RotAngMax && RotAngMax > RotAngMin);
            break;
        }

        if (ePar != MON_DIV_YZ)
        { if (iBin>=0 && iBin<nBins && time>=FiltTimeMin && time<=FiltTimeMax)
          {
            Int [iBin] += prob;
            nBin[iBin] += 1;
            IntTot     += prob;
            nTrjTot    += 1;
            bRegistered = 1;
            if (nAddMons > 0 && iCol >= 0 && iCol < nAddMons)
            { 
              Int [iBin + (iCol+1)*(nBins+1)] += prob;
              nBin[iBin + (iCol+1)*(nBins+1)] += 1;
            }
          }
	      }

        /* write out bRegistered neutrons, if 'exclusive counts = yes' is set */
        if ((bExclusive==1) && (bRegistered==1))
        {
          WriteNeutron(&(InputNeutrons[i]));
        }
      }
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// -----------------------------------------------------------------------------------------------
my_exit:
  UpdateMon(ANY_COLOR, nBundle);  // main monitor

  // additional monitors
  if (nAddMons > 0) 
  {
    int jMon;
    for (jMon=0; jMon<nAddMons; jMon++)
      UpdateMon(jMon, nBundle);
  }

  // TOF monitor 
  if (ePar==MON_TIME) 
  { 
    if (Freq > 0.0)
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s \n", 1000*IntMax/BinSize/Freq);
    else
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s (* 1/rep_rate for TOF instruments) \n", 1000*IntMax/BinSize);
  }

  // writes to log file
  if (bProbWeight)
    fprintf(LogFilePtr, "total neutron count rate within binning and eval. time: %11.4e n/s \n\n", IntTot);
  else
    fprintf(LogFilePtr, "total number of traject. within binning and eval. time: %ld\n\n", nTrjTot);

#ifdef REALLY_FREE_THINGS_THE_OS_KILLS_ELSE
  if (PosT!=NULL) free(PosT);
  if (Int !=NULL) free(Int);
  if (Norm!=NULL) free(Norm);
  if (SD  !=NULL) free(SD);
  if (nBin!=NULL) free(nBin);
#endif

  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int  i;
  double rotang=0.0;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'O':
          MonFileName=&argv[i][2];
          break;
        case 'R':
          RefFileName=&argv[i][2];
          break;

        case 'k':
          ePar = (VtMon1Par) atol(&argv[i][2]); /* 1=monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz,
                                       5=monitory,      6=monitorz     7=energy       8=divyz */
          break;

        case 'n':
          nBins = atol(&argv[i][2]); /* number of bins */
          break;

        case 'm':
          MinY = atof(&argv[i][2]);   /* lower bound lambda, time or div. window [A], [ms], [deg]*/
          break;
        case 'M':
          MaxY = atof(&argv[i][2]);   /* upper bound lambda, time or div. window [A], [ms], [deg]*/
          break;

        case 'a':
          RotAngMin = atof(&argv[i][2])*M_PI/180.;   /* lower bound rot projection angle [deg]*/
          break;
        case 'A':
          RotAngMax = atof(&argv[i][2])*M_PI/180.;   /* upper bound rot projection angle [deg]*/
          break;
        case 's':
          RotAngStep = atof(&argv[i][2])*M_PI/180.;   /* rot projection angle step [deg]*/
          break;

        case 'l':
          FiltLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'L':
          FiltLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'y':
          FiltYMin = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'Y':
          FiltYMax = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'z':
          FiltZMin = atof(&argv[i][2]);   /* filter Z */
          break;
        case 'Z':
          FiltZMax = atof(&argv[i][2]);   /* filter Z */
          break;

        case 'p':
          bProbWeight = (short) atol(&argv[i][2]); /* p=1 means probability weight activated, */
          break;                                   /* else neutron weight is set to 1.0       */
        case 'P':
          bSplitWeight = (short) atol(&argv[i][2]); /* p=1 means split weight activated for each angle, */
          break;                                   /* else neutron weight is multiplied by number of detection angles */

        case 'e':
          if(argv[i][2]=='1') bExclusive = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
          break;

        case 'f':
          eNormalize = (VtMonNorm) atoi(&argv[i][2]);      /* if 1, each channel normalised with bin size */
          break;                                           /* if 2, each channel normalised reference file */

        case 'C':
          iColour = atol(&argv[i][2]);         /*  excludes all neutrons with diff. Colour, if iColour >= 0   */
          break;  
                          
        case 'c':
          bAllFiles = (short)atoi(&argv[i][2]); /*  criterion: generates additional files for colour=0, 1, ..., iColour   */
          break;

        case 't':
          FiltTimeMin = atof(&argv[i][2]);   /* minimal time for monitoring */
          break;
        case 'T':
          FiltTimeMax = atof(&argv[i][2]);   /* maximal time for monitoring */
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
  
  // if all files wanted, treat all colours and regard iColour as max. colour to monitor in separate files
  if (bAllFiles == TRUE)                    
  { nAddMons = mini(MAX_COLS, iColour+1);
    iColour  = ANY_COLOR;
  }

  if (ePar == MON_DIV_YZ && bSplitWeight == 1) 
  {
    rotang = RotAngMin;
    do 
    { rotang += RotAngStep;
      nRot++;
    } 
    while (RotAngStep > 0.0 && rotang <= RotAngMax && RotAngMax > RotAngMin);
  } 
  else 
  {
    nRot = 1;
  }
  
  if (bProbWeight != 1) 
    bProbWeight = 0;

  return;
}

/*******************************************************/
/** Allocates memory and initializes monitor arrays   **/
/*******************************************************/
void InitArrays()
{
  char   sBuffer[512]="";
  short  jMon;               /* monitor number */
  long   iBin=0;             /* bin number     */
  double RefValue=0.0;       /* reference data */

  // allocates memory
  PosT = (double*) calloc(nBins+1, sizeof(double));
  Norm = (double*) calloc(nBins+1, sizeof(double));
  Int  = (double*) calloc((nBins+1)*(nAddMons+1), sizeof(double));
  SD   = (double*) calloc((nBins+1)*(nAddMons+1), sizeof(double));
  nBin = (long*)   calloc((nBins+1)*(nAddMons+1), sizeof(long));

  // initialisation of monitor arrays and setting of normalisation array
  BinSize = (MaxY-MinY)/(double)nBins;

  for (iBin=0; iBin<=nBins; iBin++)
  {
    PosT[iBin]=MinY+(BinSize*iBin);
    for (jMon=0; jMon<=nAddMons; jMon++)
    { 
      Int [iBin+jMon*(nBins+1)]=0.0;
      SD  [iBin+jMon*(nBins+1)]=0.0;
      nBin[iBin+jMon*(nBins+1)]=0;
    }

    if (eNormalize==NORM_BIN_SIZE)
    { Norm[iBin] =(MaxY-MinY)/(double)nBins;
    }
    else if (eNormalize==NORM_REF_FILE && ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1))
    { 
      StrgScanLF(sBuffer, &RefValue, 1, 1);
      Norm[iBin] = RefValue;  
    }
    else
    { Norm[iBin] = 1.0;
    }
  }
  
  switch (eNormalize)
  {
    case NORM_BIN_SIZE: fprintf(LogFilePtr, "Norm     : %f\n",  Norm[0]); break;
    case NORM_REF_FILE : fprintf(LogFilePtr, "normalized by %s", RefFileName); break;
  }
  fprintf(LogFilePtr, "Binning  : %ld bins from %10.5f to %10.5f %s\n", nBins, MinY, MaxY, sUnit[ePar]);
  fprintf(LogFilePtr, "File     : %s\n", MonFileName);

  // closes reference file
  if (pFileRef!=NULL)
    fclose(pFileRef);
}


/*******************************************************/
/**  Opens all files                                  **/
/*******************************************************/
void OpenFiles()
{
  // short jMon;                 /* monitor number */
  // char  sNewName[99]="";

  // opens reference file
  if (RefFileName!=NULL)
  { 
    pFileRef = OpenInputFile(RefFileName, FALSE, "rt");
    if (pFileRef!=NULL)
    { eNormalize=NORM_REF_FILE;
    }
    else
    {  fprintf(LogFilePtr,"\nERROR: Reference file %s could not be opened\n", RefFileName);
       exit(-1);
    }
  }

  // opens main monitor file
  // pFileMon = OpenOutputFile(MonFileName, TRUE, "at");

  /*  opens separate files from colour=0, 1, ..., iColour     
  if (nAddMons > 0)
  { 
	  for (jMon=0; jMon<nAddMons; jMon++)
	  { 
        NumerateName(sNewName, MonFileName, jMon);
        pFileMonC[jMon] = OpenOutputFile(sNewName, FALSE, "at");
        if (pFileMon==NULL)
          fprintf(LogFilePtr,"\nFile %s could not be opened for monitor output\n", sNewName);
	  } 
  } */

  return; 
}


/*******************************************************/
/**  Updates main monitor output file                 **/
/*******************************************************/
void UpdateMon(int jMon, long iBndl)
{
  char   sNewName[99]="";
  double f_norm;                 // ratio of total to processed bundles after treating current bundle
  long   iBin=0,                 // index of bins in x-axix and for main monitor
         kBin=0;                 // index in array for monitors of individual colors   
  double xBin=0.0;               // center of the current bin 
  FILE*  pFile=NULL;
  int    iColor=jMon;            // color of the trajectories

  if (jMon==ANY_COLOR)
  { StrgCopy(sNewName, MonFileName, strlen(MonFileName));
  }
  else
  { NumerateName(sNewName, MonFileName, jMon);
  }

  // writes current monitor file
  // ---------------------------
  pFile = OpenOutputFile(sNewName, TRUE, "wt");

  if (pFile != NULL)     
  { 
    WriteHeader1DB(pFile, "intensity", iColor, iBndl, nBundle, nBins, IntTot, nTrjTot, sParN[ePar], sUnit[ePar]);

    f_norm = (double) nBundle / (double) iBndl;
    IntMax = 0.0;

    for (iBin = 0; iBin < nBins; iBin++)
    {
      kBin = iBin + (jMon+1)*(nBins+1);
      xBin = (PosT[iBin]+PosT[iBin+1])/2.0;

      if (nBin[kBin]!=0) 
        SD [kBin] = Int[kBin]*sqrt(1./((double)nBin[kBin]/(double)nRot));

      if (Norm[iBin]!=0)
      { 
        if (ePar==MON_DIV_YZ) 
          fprintf(pFile, "%10.3f  %12.5e %12.5e  %10.2f\n", xBin, f_norm*Int[kBin]/Norm[iBin], f_norm*SD[kBin]/Norm[iBin], nBin[kBin]/(double)nRot);
        else
          fprintf(pFile, "%10.3f  %12.5e %12.5e  %7ld\n",   xBin, f_norm*Int[kBin]/Norm[iBin], f_norm*SD[kBin]/Norm[iBin], nBin[kBin]);
      }
      else
      { 
        if (ePar==MON_DIV_YZ) 
          fprintf(pFile, "%10.3f   0.0000000E+00  0.0000000E+00        0.00\n", xBin);
        else
          fprintf(pFile, "%10.3f   0.0000000E+00  0.0000000E+00        0\n",    xBin);
      }

      if (jMon==ANY_COLOR)
        IntMax = Max(IntMax, Int[kBin]);
    }

    IntMax *= f_norm;

    fclose(pFile);
  }
}


/************************************************************************************************************/
/* NumerateName: Building a combined file name of 'sFileShort' and 'sNumber' without changing the extension */
/************************************************************************************************************/
void NumerateName(char* sFileLong, char* sFileShort, const short nNumber)

{
   char sParExt [4],      // extension of file names (with simulation results)
        sParName[99];     // name (without extension) of those files

	strcpy  (sParExt,  sFileShort +strlen(sFileShort)-3);
	StrgCopy(sParName, sFileShort, strlen(sFileShort)-4);
	sprintf (sFileLong, "%s%hd.%s", sParName, nNumber, sParExt);
}


/************************************************************************************************************/
/* ChangeName  : Putting "new_" in front of the original name                                               */
/************************************************************************************************************/
void ChangeName(char* sFileNew, char* sFileOld)
{
  char* p1=NULL, *p2=NULL;

  p1= strrchr(sFileOld, '/');
  p2= strrchr(sFileOld, '\\');
  if (p1 > p2)
		sprintf(sFileNew, "new_%s", p1+1);
  if (p2 > p1)
		sprintf(sFileNew, "new_%s", p2+1);
  if (p1 != p2)
    fprintf(LogFilePtr,"file name changed to %s\n", sFileNew);
}

