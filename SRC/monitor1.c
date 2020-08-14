/*********************************************************************************************/
/*  VITESS module 'monitor1.c'                                                               */
/*    monitoring of intensity as a function of 1 parameter                                   */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given    */
/* to the authors:                                                                           */
/* 1.0  S. Schorr         1997                                                               */
/* 1.1  G. Zsigmond   JUL 2002 extended                                                      */
/* 1.2  G. Zsigmond   MAR 2003 error bars output and option for normalisation with binsize   */
/*                             included                                                      */
/* 1.3  K. Lieutenant NOV 2003 monitoring dependent on colour                                */
/* 1.4  K. Lieutenant JAN 2004 changes for 'instrument.dat'                                  */
/* 1.5  K. Lieutenant OKT 2004 no abort, if output file cannot be opened; peak flux; count   */
/*                             rate within binning; protocol; no limit in number of binnings */
/* 1.5a K. Lieutenant DEC 2004 total no of trajectories within ...                           */
/* 1.5b K. Lieutenant MAR 2005 correction peak flux                                          */
/* 1.6  K. Lieutenant FEB 2005 intensity as a function of energy                             */
/* 1.6a A. Houben     JAN 2010 filter for wavelength and yz position (only if applicable)    */
/* 1.7  A. Houben     MAY 2010 monitor for div on x-axis rotated by rot angle                */
/* 1.8  K. Lieutenant Aug 2012 multiple file output                                          */
/* 1.9  K. Lieutenant Feb 2012 any color = -1                                                */
/* 1.10 K. Lieutenant Feb 2020 tidy up, new central visualization parameters                 */
/*********************************************************************************************/

// includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define MAX_KIND  8
#define MAX_COLS 25


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit     (int argc, char *argv[]);                                  // Reads input parameters and sets global parameters
void OpenFiles   ();                                                        // Opens all monitor files
void NumerateName(char* sFileLong, char* sFileShort, const short nNumber);
void ChangeName  (char* sFileNew,  char* sFileOld);


/******************************/
/** Global Variables         **/
/******************************/
McCompID _eModule=MCN_MON1;

FILE	*pFileMon=NULL,       /* monitor output file */
      *pFileRef=NULL;       /* reference file      */
FILE  *pFileMonC[MAX_COLS]={NULL, NULL, NULL, NULL,NULL, NULL,NULL, NULL};  // Pointer to additional monitor files

char *MonitorFileName= NULL,
     *RefFileName    = NULL;

short  bAllFiles     = FALSE,    /* criterion: files for all colors until the given one generated */ 
       bProbWeight   = 0,        /* Probability weight yes or no */
       bSplitWeight  = 1,        /* Split weight in case of yz: yes or no */
       nColour       = ANY_COLOR,
       normalise     = 0, 
       kind          = 1, 
       bExclusive    = FALSE, 
       nAddMons      = 0;        /* number of additional output files (for separate colours) */                

long   nBiny         =10;

double Miny          = 0.0,
       Maxy          = 1.0, 
       filtLambdaMin =-1.0,   /* filter      */
       filtLambdaMax =-1.0,
       filtYMin      =-1.0e10,
       filtYMax      = 1.0e10,
       filtZMin      =-1.0e10,
       filtZMax      = 1.0e10,
       dEvalTimeMin  =-1.0e10, /* min. and max. TOF to be taken into account */
       dEvalTimeMax  = 1.0e10,
       rotang_min    = 0.0, 
       rotang_max    = 0.0,
       rotang_step   = 0.0;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  char  sCompName  [21]="",
        sModVsnName[40]="",
        sBuffer[512];
  char  sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV", "deg"},
        sParN[MAX_KIND+1][22]={"", "wavelength", "time",
                               "horizontal divergence", "vertical divergence",
                               "horizontal position",   "vertical position", "energy", "divergence yz"};
	char  weightTag[2][7] = {"", "weight"};

  short  iCol,                 /* colour of the trajectory */
         jMon;                 /* monitor number */
  long   iBin,                 /* bin number */
         i, 
         bRegistered=0, 
         nTrjTot=0, crot = 0;  /* total number of traj. within binning and eval. time; number of rot angles for yz */
  double dIntTot=0.0,          /* total count rate within binning and eval. time      */
         prob=0.0,             /* neutron weight */
         time; 
  double Divy, Divz, Div, rotang,
         dIntMax=-1.0e10,      /* maximal count rate found in one bin        */
         dBinSize,             /* size of each bin                           */
         MonData;              /* reference data                             */

  double dTimeMeas,            /* measuring time     (from simulation.inf, not needed) */
         dLmbdWant,            /* desired wavelength (from simulation.inf, not needed) */
         dFreq;                /* source frequency   (from simulation.inf)   */
  double *pPosT=NULL,          /* limits of bin (minimal and maximal value)  */
         *pInt=NULL,           /* intensity (=count rate) per bin  */
         *pNorm=NULL,          /* normalisation value for each bin */
         *pSD=NULL;            /* standard deviation per bin       */
  long   *pBinN=NULL;          /* number of trajectories per bin   */


  // reading of input data and initilisation
  // ---------------------------------------
  Init   (argc, argv, _eModule);
  OwnInit(argc, argv);

  CompID2Name (sCompName, _eModule);
  sprintf(sModuleName, "%s_%s",      sCompName, sParN[kind]);
  sprintf(sModVsnName, "%s_%s 1.10", sCompName, sParN[kind]);
  print_module_name(sModVsnName);

  OpenFiles();

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // allocates memory
  pPosT = (double*) calloc(nBiny+1, sizeof(double));
  pNorm = (double*) calloc(nBiny+1, sizeof(double));
  pInt  = (double*) calloc((nBiny+1)*(nAddMons+1), sizeof(double));
  pSD   = (double*) calloc((nBiny+1)*(nAddMons+1), sizeof(double));
  pBinN = (long*)   calloc((nBiny+1)*(nAddMons+1), sizeof(long));

  // initialisation of monitor arrays and setting of normalisation array
  dBinSize = (Maxy-Miny)/(double)nBiny;

  for (iBin=0; iBin<=nBiny; iBin++)
  {
    pPosT[iBin]=Miny+(dBinSize*iBin);
    for (jMon=0; jMon<=nAddMons; jMon++)
    { 
      pInt [iBin+jMon*(nBiny+1)]=0.0;
      pSD  [iBin+jMon*(nBiny+1)]=0.0;
      pBinN[iBin+jMon*(nBiny+1)]=0;
    }
    if (normalise==1)
    { pNorm[iBin] =(Maxy-Miny)/(double)nBiny;
    }
    else if (normalise==2 && ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1))
    { 
      StrgScanLF(sBuffer, &MonData, 1, 1);
      pNorm[iBin] = MonData;  
    }
    else
    { pNorm[iBin] = 1.0;
    }
  }
  
  switch (normalise)
  {
    case 1: fprintf(LogFilePtr, "Norm     : %f\n",  pNorm[0]); break;
    case 2: fprintf(LogFilePtr, "normalized by %s", RefFileName); break;
  }
  fprintf(LogFilePtr, "Binning  : %ld bins from %10.5f to %10.5f %s\n", nBiny, Miny, Maxy, sUnit[kind]);
  fprintf(LogFilePtr, "File     : %s\n", MonitorFileName);

  // closes reference file
  if (pFileRef!=NULL)
    fclose(pFileRef);
  

  if (kind == 8 && bSplitWeight == 1) 
  {
    rotang = rotang_min;
    do 
    { rotang += rotang_step;
      crot++;
    } 
    while (rotang_step > 0.0 && rotang <= rotang_max && rotang_max > rotang_min);
  } 
  else 
  {
    crot = 1;
  }
  
  DECLARE_ABORT;
  
  // loop over trajectories
  // ----------------------
	while (ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      bRegistered=0;

      if(bProbWeight==1) 
        prob = InputNeutrons[i].Probability;
      else
        prob = 1.0;

      time = InputNeutrons[i].Time;
      iCol = InputNeutrons[i].Color;

      /* write out all neutrons, if 'exclusive counts = no' is set */
      if (bExclusive==0)
        WriteNeutron(&(InputNeutrons[i]));

      /* exclude traj. with wrong colours: (nColour=-1 means: all colours accepted) */
      if (nColour != ANY_COLOR && nColour!=InputNeutrons[i].Color) continue;
      if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
      if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
      if (InputNeutrons[i].Position[1] < filtYMin) continue;
      if (InputNeutrons[i].Position[1] > filtYMax) continue;
      if (InputNeutrons[i].Position[2] < filtZMin) continue;
      if (InputNeutrons[i].Position[2] > filtZMax) continue;

      switch (kind)
      {
      case 1: // monitor lambda
        iBin = (int)floor((double)nBiny*(InputNeutrons[i].Wavelength - Miny)/(Maxy-Miny));
        break;

      case 2: // monitor time
        iBin = (int)floor(nBiny*(InputNeutrons[i].Time - Miny)/(Maxy-Miny));
        break;

      case 3: // monitor div_y
        if (InputNeutrons[i].Vector[0] >= 0) 
          Divy = (double)atan2(InputNeutrons[i].Vector[1],  sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	      else 
          Divy = (double)atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
        Divy *= 180.0/M_PI;
        iBin = (int)floor(nBiny*(Divy - Miny)/(Maxy-Miny));
        break;

      case 4: // monitor div_z
        Divz=(double)atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));
        Divz*=180.0/M_PI;
        iBin = (int)floor(nBiny*(Divz - Miny)/(Maxy-Miny));
        break;

      case 5: // monitor y
        iBin = (int)floor(nBiny*(InputNeutrons[i].Position[1] - Miny)/(Maxy-Miny));
        break;

      case 6: // monitor z
        iBin = (int)floor(nBiny*(InputNeutrons[i].Position[2] - Miny)/(Maxy-Miny));
        break;

      case 7: // monitor energy
        iBin=(int)floor((double)nBiny*(0.001*ENERGY_FROM_LAMBDA(InputNeutrons[i].Wavelength) - Miny)/(Maxy-Miny));
        break;
		
      case 8: // monitor div_yz_angle
        Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
        Divy*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          Divy=0.0;

        Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
        Divz*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          Divz=0.0;
        //x' = x cos f - y sin f
		
        rotang = rotang_min;
        do {
          Div = Divy * cos(-rotang) - Divz * sin(-rotang);

          iBin = (int)floor(nBiny*(Div - Miny)/(Maxy-Miny));
          if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
            {
              pInt [iBin] += prob/crot;
              pBinN[iBin] += 1;
              dIntTot     += prob/crot;
              nTrjTot     += 1;
              bRegistered = 1;
            }
          rotang += rotang_step;
        } while (rotang_step > 0.0 && rotang <= rotang_max && rotang_max > rotang_min);
        break;
      }

      if (kind != 8)
      { if (iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
          pInt [iBin] += prob;
          pBinN[iBin] += 1;
          dIntTot     += prob;
          nTrjTot     += 1;
          bRegistered = 1;
          if (nAddMons > 0 && iCol >= 0 && iCol < nAddMons)
          { 
            pInt [iBin + (iCol+1)*(nBiny+1)] += prob;
            pBinN[iBin + (iCol+1)*(nBiny+1)] += 1;
          }
        }
	    }

      /* write out bRegistered neutrons, if 'exclusive counts = yes' is set */
      if((bExclusive==1) && (bRegistered==1))
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// -----------------------------------------------------------------------------------------------
my_exit:
  // main monitor
  if (pFileMon != NULL)     
  { 
    fprintf(pFileMon,"#Monitor %s %s\n", weightTag[bProbWeight], sParN[kind]);
    for (iBin = 0; iBin < nBiny; iBin++)
    { if(pBinN[iBin]!=0) 
	    pSD[iBin] = pInt[iBin]*sqrt(1./((double)pBinN[iBin]/(double)crot));
      if(pNorm[iBin]!=0)
	      fprintf(pFileMon,"%12.5e %14.7e   %14.7e %12.2f \n",
                         (pPosT[iBin]+pPosT[iBin+1])/2.0, (pInt[iBin]/pNorm[iBin]), pSD[iBin]/pNorm[iBin], pBinN[iBin]/(double)crot);
      else
        fprintf(pFileMon,"%12.5e   0.0000000   0.0000000  0.0000000\n",
                         (pPosT[iBin]+pPosT[iBin+1])/2.0);
      dIntMax = Max(dIntMax, pInt[iBin]);
    }
    fclose(pFileMon);
  }

  // additional monitors
  if (nAddMons > 0) 
  { for (jMon=0; jMon<nAddMons; jMon++)
    { if (pFileMonC[jMon] != NULL)
      { 
        fprintf(pFileMonC[jMon],"#Monitor %s %s\n", weightTag[bProbWeight], sParN[kind]);
        for (iBin = 0; iBin < nBiny; iBin++)
        {
          if(pBinN[iBin+(jMon+1)*(nBiny+1)]!=0) 
            pSD[iBin+(jMon+1)*(nBiny+1)] = pInt[iBin+(jMon+1)*(nBiny+1)]*sqrt(1./((double)pBinN[iBin+(jMon+1)*(nBiny+1)]));
          fprintf(pFileMonC[(jMon+1)-1],"%12.5e   %14.7e   %14.7e   %10ld\n",
            (pPosT[iBin]+pPosT[iBin+1])/2.0, (pInt[iBin+(jMon+1)*(nBiny+1)]/pNorm[iBin]), pSD[iBin+(jMon+1)*(nBiny+1)]/pNorm[iBin], pBinN[iBin+(jMon+1)*(nBiny+1)]);
        }
        fclose(pFileMonC[jMon]);
      }
    }
  }

  // TOF monitor 
  if (kind==2) 
  { 
    ReadSimData(&dTimeMeas, &dLmbdWant, &dFreq);
    if (dFreq > 0.0)
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s \n", 1000*dIntMax/dBinSize/dFreq);
    else
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s (* 1/rep_rate for TOF instruments) \n", 1000*dIntMax/dBinSize);
  }

  // writes to log file
  if (bProbWeight)
    fprintf(LogFilePtr, "total neutron count rate within binning and eval. time: %11.4e n/s \n\n", dIntTot);
  else
    fprintf(LogFilePtr, "total number of traject. within binning and eval. time: %ld\n\n", nTrjTot);

#ifdef REALLY_FREE_THINGS_THE_OS_KILLS_ELSE
  if (pPosT!=NULL) free(pPosT);
  if (pInt !=NULL) free(pInt);
  if (pNorm!=NULL) free(pNorm);
  if (pSD  !=NULL) free(pSD);
  if (pBinN!=NULL) free(pBinN);
#endif

  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int  i;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
      case 'O':
        MonitorFileName=&argv[i][2];
        break;
      case 'R':
        RefFileName=&argv[i][2];
        break;

      case 'k':
        kind = atol(&argv[i][2]); /* 1=monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz,
                                     5=monitory,      6=monitorz     7=energy       8=divyz */
        break;

      case 'n':
        nBiny = atol(&argv[i][2]); /* number of bins */
        break;

      case 'm':
        Miny = atof(&argv[i][2]);   /* lower bound lambda, time or div. window [A], [ms], [deg]*/
        break;
      case 'M':
        Maxy = atof(&argv[i][2]);   /* upper bound lambda, time or div. window [A], [ms], [deg]*/
        break;

      case 'a':
        rotang_min = atof(&argv[i][2])*M_PI/180.;   /* lower bound rot projection angle [deg]*/
        break;
      case 'A':
        rotang_max = atof(&argv[i][2])*M_PI/180.;   /* upper bound rot projection angle [deg]*/
        break;
      case 's':
        rotang_step = atof(&argv[i][2])*M_PI/180.;   /* rot projection angle step [deg]*/
        break;

      case 'l':
        filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;
      case 'L':
        filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;
      case 'y':
        filtYMin = atof(&argv[i][2]);   /* filter Y */
        break;
      case 'Y':
        filtYMax = atof(&argv[i][2]);   /* filter Y */
        break;
      case 'z':
        filtZMin = atof(&argv[i][2]);   /* filter Z */
        break;
      case 'Z':
        filtZMax = atof(&argv[i][2]);   /* filter Z */
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
        if(argv[i][2]=='1') normalise = 1;  /* if 1, each channel normalised with bin size */
        break;                              /* if 2, each channel normalised reference file */

      case 'C':
        nColour = atol(&argv[i][2]);        /*  excludes all neutrons with diff. Colour, if nColour >= 0   */
        break;  
                          
      case 'c':
        bAllFiles = (short)atoi(&argv[i][2]); /*  criterion: files containing all colors until the given one generated   */
        break;

      case 't':
        dEvalTimeMin = atof(&argv[i][2]);   /* minimal time for monitoring */
        break;
      case 'T':
        dEvalTimeMax = atof(&argv[i][2]);   /* maximal time for monitoring */
        break;

      default:
        fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
        exit(-1);
        break;
      }
    }
  }
  
  if (bProbWeight != 1) 
    bProbWeight = 0;

  return;
}


/*******************************************************/
/**  Opens all files                                  **/
/*******************************************************/
void OpenFiles()
{
  short jMon;                 /* monitor number */
  char  sNewName[99]="";

  // opens reference file
  if (RefFileName!=NULL)
  { 
    pFileRef = OpenInputFile(RefFileName, FALSE, "rt");
    if (pFileRef!=NULL)
    { normalise=2;
    }
    else
    {  fprintf(LogFilePtr,"\nReference file %s could not be opened\n", RefFileName);
       exit(-1);
    }
  }

  // opens main monitor file
  pFileMon = OpenOutputFile(MonitorFileName, TRUE, "wt");
  /* if (pFileMon==NULL)
  {
    fprintf(LogFilePtr,"\nFile %s could not be opened for monitor output\n", MonitorFileName);
    ChangeName(sNewName, MonitorFileName);
    pFileMon = OpenOutputFile(sNewName, FALSE, "wt");
  } */

  /*  opens separate files from colour=0, 1, ..., nColour      */
  if (bAllFiles == TRUE)                    
  { nAddMons = mini(MAX_COLS, nColour+1);
    nColour  = ANY_COLOR;
  }
  if (nAddMons > 0)
  { 
	  for (jMon=0; jMon<nAddMons; jMon++)
	  { 
        NumerateName(sNewName, MonitorFileName, jMon);
        pFileMonC[jMon] = OpenOutputFile(sNewName, FALSE, "wt");
        if (pFileMon==NULL)
          fprintf(LogFilePtr,"\nFile %s could not be opened for monitor output\n", sNewName);
	  } 
  }

  return;
}


/************************************************************************************************************/
/* NumerateName: Building a combined file name of 'sFileShort' and 'sNumber' without changing the extension */
/* ChangeName  : Putting "new_" in front of the original name                                               */
/************************************************************************************************************/
void
NumerateName(char* sFileLong, char* sFileShort, const short nNumber)

{
   char sParExt [4],      // extension of file names (with simulation results)
        sParName[99];     // name (without extension) of those files

	strcpy  (sParExt,  sFileShort +strlen(sFileShort)-3);
	StrgCopy(sParName, sFileShort, strlen(sFileShort)-4);
	sprintf (sFileLong, "%s%hd.%s", sParName, nNumber, sParExt);
}

void 
ChangeName(char* sFileNew, char* sFileOld)
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

