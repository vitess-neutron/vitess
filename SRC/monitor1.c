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
/*********************************************************************************************/

// includes and definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"

#define MAX_KIND  8
#define MAX_COLS 25


// Prototypes
void NumerateName(char* sFileLong, char* sFileShort, const short nNumber);


int main(int argc, char *argv[])
{
  FILE	*pFileMon=NULL,       /* monitor output file */
        *pFileRef=NULL;       /* reference file      */
  FILE*  pFileMonC[MAX_COLS]={NULL, NULL, NULL, NULL,NULL, NULL,NULL, NULL};  // Pointer to additional monitor files
  char	*MonitorFileName=NULL,
        *RefFileName=NULL,
        sNewName[99]="", sModuleName[41],
        sBuffer[512];
  char  sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV", "deg"},
        sParN[MAX_KIND+1][22]={"", "wavelength", "time",
                               "horizontal divergence", "vertical divergence",
                               "horizontal position",   "vertical position", "energy", "divergence yz"};
	char  weightTag[2][7] = {"", "weight"};

  short  bProbWeight=0,        /* Probability weight yes or no */
         bAllFiles=FALSE,      /* criterion: files for all colors until the given one generated */ 
         iCol,                 /* colour of the trajectory */
         // jCol,                  colour to be used in a monitor
         jMon,                 /* monitor number */
         nColour =ANY_COLOR,
         nAddMons=0,           /* number of additional output files (for separate colours) */                
         bSplitWeight=1;       /* Split weight in case of yz: yes or no */
  long   iBin,                 /* bin number */
         i, 
         kind=1, 
         exclusivecount=0, 
         registered=0, 
         normalise =0, 
         nBiny   =10, 
         nTrjTot=0, crot = 0;  /* total number of traj. within binning and eval. time; number of rot angles for yz */
  double dIntTot=0.0,          /* total count rate within binning and eval. time      */
         Miny=0.0,Maxy=1.0, 
         prob=0.0,             /* neutron weight */
		 time, 
         rotang_min=0.0, 
         rotang_max=0.0,
         rotang_step=0.0;
  double filtLambdaMin=-1.0,   /* filter      */
         filtLambdaMax=-1.0,
         filtYMin=-1.0e10,
         filtYMax= 1.0e10,
         filtZMin=-1.0e10,
         filtZMax= 1.0e10;
  double Divy, Divz, Div, rotang,
         dEvalTimeMin=-1.0e10, /* min. and max. TOF to be taken into account */
         dEvalTimeMax=1.0e10,
         dIntMax=-1.0e10,      /* maximal count rate found in one bin        */
         dBinSize;             /* size of each bin                           */
  double dTimeMeas,            /* measuring time     (from simulation.inf, not needed) */
         dLmbdWant,            /* desired wavelength (from simulation.inf, not needed) */
         dFreq;                /* source frequency   (from simulation.inf)   */
  double *pPosT=NULL,          /* limits of bin (minimal and maximal value)  */
         *pInt=NULL,           /* intensity (=count rate) per bin  */
         *pNorm=NULL,          /* normalisation value for each bin */
         *pSD=NULL;            /* standard deviation per bin       */
  long   *pBinN=NULL;          /* number of trajectories per bin   */


  /* init */
  Init(argc, argv, VT_MONITOR_1);

  /* own init */
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
      case 'O':
        MonitorFileName=&argv[i][2];
        pFileMon = fileOpen(FullParName(MonitorFileName),"wt");
        if (pFileMon==NULL)
        {
          char* p1=NULL, *p2=NULL;

          fprintf(LogFilePtr,"\nFile %s could not be opened for monitor output\n",MonitorFileName);

          p1= strrchr(MonitorFileName, '/');
          p2= strrchr(MonitorFileName, '\\');
          if (p1 > p2)
				sprintf(sNewName, "new_%s", p1+1);
          if (p2 > p1)
				sprintf(sNewName, "new_%s", p2+1);
          if (p1 != p2)
           fprintf(LogFilePtr,"file name changed to %s\n", sNewName);
        }
        break;

      case 'R':
        RefFileName=&argv[i][2];
        pFileRef = fopen(FullParName(RefFileName),"rt");
        if (pFileRef==NULL)
        {  fprintf(LogFilePtr,"\nReference file %s could not be opened\n", RefFileName);
        }
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
        if(argv[i][2]=='1') exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
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

  sprintf(sModuleName, "monitor1_%s 1.9", sParN[kind] );
  print_module_name(sModuleName);
  memset(pFileMonC, '\0', sizeof (FILE*));

  if (bAllFiles == TRUE)                    /*  writes separate files from colour=0, 1, ..., nColour      */
  { nAddMons = mini(MAX_COLS, nColour+1);
    nColour  = ANY_COLOR;
  }
  if (nAddMons > 0)
  { 
	  for (jMon=0; jMon<nAddMons; jMon++)
	  { 
        NumerateName(sNewName, MonitorFileName, jMon);
        pFileMonC[jMon] = fopen(FullParName(sNewName),"wt");
	  } 
  }

  pPosT = (double*) calloc(nBiny+1, sizeof(double));
  pNorm = (double*) calloc(nBiny+1, sizeof(double));
  pInt  = (double*) calloc((nBiny+1)*(nAddMons+1), sizeof(double));
  pSD   = (double*) calloc((nBiny+1)*(nAddMons+1), sizeof(double));
  pBinN = (long*)   calloc((nBiny+1)*(nAddMons+1), sizeof(long));

  if (pFileRef!=NULL)
    normalise=2;

  /*initialisation*/
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
    if(normalise==1)
    { pNorm[iBin] =(Maxy-Miny)/(double)nBiny;
    }
    else if (normalise==2 && ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1))
      { sscanf(sBuffer, "%lf", &(pNorm[iBin]));
    }
    else
    { pNorm[iBin] = 1.0;
    }
  }

  switch (normalise)
  {
    case 1: fprintf(LogFilePtr, "Norm     : %f\n", pNorm[0]); break;
    case 2: fprintf(LogFilePtr, "normalized by %s", RefFileName); break;
  }
  fprintf(LogFilePtr, "Binning  : %ld bins from %10.5f to %10.5f %s\n", nBiny, Miny, Maxy, sUnit[kind]);
  fprintf(LogFilePtr, "File     : %s\n", MonitorFileName);


  if (kind == 8 && bSplitWeight == 1) {
    rotang = rotang_min;
    do {
      rotang += rotang_step;
      crot++;
    } while (rotang_step > 0.0 && rotang <= rotang_max && rotang_max > rotang_min);
  } else {
    crot = 1;
  }

  if (bProbWeight != 1) bProbWeight = 0;

  DECLARE_ABORT;
  while (ReadNeutrons()!= 0)
  {
    CHECK;
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      registered=0;

      if(bProbWeight==1) 
        prob = InputNeutrons[i].Probability;
      else
        prob = 1.0;

      time = InputNeutrons[i].Time;
      iCol = InputNeutrons[i].Color;

      /* write out all neutrons, if 'exclusive counts = no' is set */
      if (exclusivecount==0)
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
        Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
        Divy*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          Divy=0.0;
        iBin = (int)floor(nBiny*(Divy - Miny)/(Maxy-Miny));
        break;

      case 4: // monitor div_z
        Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
        Divz*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          {Divy=0.0;}
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
          Divy=0.0;
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
              registered = 1;
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
          registered = 1;
          if (nAddMons > 0 && iCol >= 0 && iCol < nAddMons)
          { 
            pInt [iBin + (iCol+1)*(nBiny+1)] += prob;
            pBinN[iBin + (iCol+1)*(nBiny+1)] += 1;
          }
        }
	  }

      /* write out registered neutrons, if 'exclusive counts = yes' is set */
      if((exclusivecount==1) && (registered==1))
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

my_exit:
  // main monitor
  if (pFileMon != NULL)     
  { 
    fprintf(pFileMon,"#Monitor %s %s\n", weightTag[bProbWeight], sParN[kind]);
    for (iBin = 0; iBin < nBiny; iBin++)
    { if(pBinN[iBin]!=0) 
	    pSD[iBin] = pInt[iBin]*sqrt(1./((double)pBinN[iBin]/(double)crot));
      fprintf(pFileMon,"%12.4e   %14.7e   %14.7e %12.2f\n",
                       (pPosT[iBin]+pPosT[iBin+1])/2.0, (pInt[iBin]/pNorm[iBin]), pSD[iBin]/pNorm[iBin], pBinN[iBin]/(double)crot);
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
          fprintf(pFileMonC[(jMon+1)-1],"%12.4e   %14.7e   %14.7e   %10ld\n",
                  (pPosT[iBin]+pPosT[iBin+1])/2.0, (pInt[iBin+(jMon+1)*(nBiny+1)]/pNorm[iBin]), pSD[iBin+(jMon+1)*(nBiny+1)]/pNorm[iBin], 
                  pBinN[iBin+(jMon+1)*(nBiny+1)]);
        }
        fclose(pFileMonC[jMon]);
      }
    }
  }

  if (kind==2) /* monitor TOF */
  { 
    ReadSimData(&dTimeMeas, &dLmbdWant, &dFreq);
    if (dFreq > 0.0)
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s \n", 1000*dIntMax/dBinSize/dFreq);
    else
      fprintf(LogFilePtr, "Peak flux: %11.4e n/s (* 1/rep_rate for TOF instruments) \n", 1000*dIntMax/dBinSize);
  }

  if (bProbWeight)
    fprintf(LogFilePtr, "total neutron count rate within binning and eval. time: %11.4e n/s \n\n", dIntTot);
  else
    fprintf(LogFilePtr, "total number of traject. within binning and eval. time: %ld\n\n", nTrjTot);

  stPicture.eType = (short) kind;
#ifdef REALLY_FREE_THINGS_THE_OS_KILLS_ELSE
  if (pPosT!=NULL) free(pPosT);
  if (pInt !=NULL) free(pInt);
  if (pNorm!=NULL) free(pNorm);
  if (pSD  !=NULL) free(pSD);
  if (pBinN!=NULL) free(pBinN);
#endif

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}

/* Building a combined file name of 'sFilename' and 'sAddition' without changing the extension */
/***********************************************************************************************/
void
NumerateName(char* sFileLong, char* sFileShort, const short nNumber)

{
   char sParExt [4],      // extension of file names (with simulation results)
        sParName[99];     // name (without extension) of those files

	strcpy  (sParExt,  sFileShort +strlen(sFileShort)-3);
	StrgCopy(sParName, sFileShort, strlen(sFileShort)-4);
	sprintf (sFileLong, "%s%hd.%s", sParName, nNumber, sParExt);
}

