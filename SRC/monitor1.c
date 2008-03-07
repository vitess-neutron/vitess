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
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"

#define MAX_KIND 7


int main(int argc, char *argv[])
{
  FILE	*pFileMon=NULL,       /* monitor output file */
         *pFileRef=NULL;       /* reference file      */
  char	*MonitorFileName=NULL,
         *RefFileName=NULL,
         sNewName[99]="", sModuleName[41],
         sBuffer[512];
  char   sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV"},
         sParN[MAX_KIND+1][22]={"", "wavelength", "time",
                                "horizontal divergence", "vertical divergence",
                                "horizontal position",   "vertical position", "energy"};

  short  bProbWeight=0;        /* Probability weight yes or no */
  long   iBin,                 /* bin number */
         i, kind, exclusivecount, registered, normalise, BufferIndex, nBiny=10, nColour=0,
         nTrjTot=0;            /* total number of traj. within binning and eval. time */
  double dIntTot=0.0,          /* total count rate within binning and eval. time      */
         Miny=0.0,p,Maxy=1.0, time;
  double Divy, Divz,
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


  /*initialisation*/
  BufferIndex = 0;
  kind = 1;
  normalise = 0;
  p=0.0;
  exclusivecount=0;
  registered=0;


  /* input */
  Init(argc, argv, VT_MONITOR_1);

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
      case 'O':
        MonitorFileName=&argv[i][2];
        pFileMon = fopen(FullParName(MonitorFileName),"wt");
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
                                     5=monitory,      6=monitorz     7=energy */
        break;

      case 'n':
        nBiny = atol(&argv[i][2]); /* number of bins */

        pPosT = (double*) calloc(nBiny+1,sizeof(double));
        pInt  = (double*) calloc(nBiny+1,sizeof(double));
        pSD   = (double*) calloc(nBiny+1,sizeof(double));
        pNorm = (double*) calloc(nBiny+1,sizeof(double));
        pBinN = (long*)   calloc(nBiny+1,sizeof(long));

        //if (nbiny>10000) {fprintf(LogFilePtr,"\n number of bins must be <= 10000"); exit(99);}
        break;

      case 'm':
        Miny = atof(&argv[i][2]);   /* lower bound lambda, time or div. window [A], [ms], [deg]*/
        break;

      case 'M':
        Maxy = atof(&argv[i][2]);   /* upper bound lambda, time or div. window [A], [ms], [deg]*/
        break;

      case 'p':
        bProbWeight = (short) atol(&argv[i][2]); /* p=1 means probability weight activated, */
        break;                                   /* else neutron weight is set to 1.0       */

      case 'e':
        if(argv[i][2]=='1') exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
        break;

      case 'f':
        if(argv[i][2]=='1') normalise = 1; /* if 1, each channel normalised with bin size */
        break;                             /* if 2, each channel normalised reference file */

      case 'C':
        nColour = atol(&argv[i][2]);       /*  excludes all neutrons with diff. Colour, if nColour > 0 */
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


  if (MonitorFileName==NULL)
    {fprintf(LogFilePtr,"\n you must define a MonitorOutputFile"); exit(99);}

  sprintf(sModuleName, "monitor1_%s 1.6", sParN[kind] );
  print_module_name(sModuleName);

  if (pFileRef!=NULL)
    normalise=2;

  /*initialisation*/
  dBinSize = (Maxy-Miny)/(double)nBiny;
  for (iBin=0; iBin<=nBiny; iBin++)
  {
    pPosT[iBin]=Miny+(dBinSize*iBin);
    pInt [iBin]=0.0;
    pSD  [iBin]=0.0;
    pBinN[iBin]=0;

    if(normalise==1)
    { pNorm[iBin] =(Maxy-Miny)/(double)nBiny;
    }
    else if (normalise==2 && ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1))
    { sscanf(sBuffer, "%lf", pNorm[iBin]);
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
  fprintf(LogFilePtr, "Binning  : %d bins from %10.5f to %10.5f %s\n", nBiny, Miny, Maxy, sUnit[kind]);
  fprintf(LogFilePtr, "File     : %s\n", MonitorFileName);


  DECLARE_ABORT;
  while (ReadNeutrons()!= 0)
  {
    CHECK;
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      registered=0;

      if(bProbWeight==1.0) {p = InputNeutrons[i].Probability;}
      else p=1.0;

      time = InputNeutrons[i].Time;

      /* write out all neutrons, if 'exclusive counts = no' is set */
      if (exclusivecount==0)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }

      /* exclude traj. with wrong colours: (nColour=0 means: all colours accepted) */
      if (nColour!=0 && nColour!=InputNeutrons[i].Color) continue;

      switch (kind)
      {
      case 1:
        iBin=(int)floor((double)nBiny*(InputNeutrons[i].Wavelength - Miny)/(Maxy-Miny));

        if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 2:
        iBin = (int)floor(nBiny*(InputNeutrons[i].Time - Miny)/(Maxy-Miny));
        if((iBin>=0)&&(iBin<nBiny))
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 3:
        Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
        Divy*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          {Divy=0.0;}

        iBin = (int)floor(nBiny*(Divy - Miny)/(Maxy-Miny));
        if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 4:
        Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
        Divz*=180.0/M_PI;
        if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          {Divy=0.0;}

        iBin = (int)floor(nBiny*(Divz - Miny)/(Maxy-Miny));

        if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 5:
        iBin = (int)floor(nBiny*(InputNeutrons[i].Position[1] - Miny)/(Maxy-Miny));
        if(iBin>=0 && iBin<nBiny  && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 6:
        iBin = (int)floor(nBiny*(InputNeutrons[i].Position[2] - Miny)/(Maxy-Miny));
        if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;

      case 7:
        iBin=(int)floor((double)nBiny*(0.001*ENERGY_FROM_LAMBDA(InputNeutrons[i].Wavelength) - Miny)/(Maxy-Miny));

        if(iBin>=0 && iBin<nBiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
        {
           pInt [iBin] += p;
           pBinN[iBin] += 1;
           dIntTot   += p;
           nTrjTot   += 1;
           registered=1;
        }
        break;
      }

      /* write out registered neutrons, if 'exclusive counts = yes' is set */
      if((exclusivecount==1) && (registered==1))
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

my_exit:
  if (pFileMon != NULL)
  { for (iBin = 0; iBin < nBiny; iBin++)
    {
      if(pBinN[iBin]==0) pBinN[iBin]=1;
      pSD[iBin] = pInt[iBin]*sqrt(1./(double)pBinN[iBin]);
      fprintf(pFileMon,"% 7.7E\t% 11.7E \t% 11.7E \n",
                       (pPosT[iBin]+pPosT[iBin+1])/2.0,(pInt[iBin]/pNorm[iBin]), pSD[iBin]/pNorm[iBin]);
      dIntMax = Max(dIntMax, pInt[iBin]);
    }

    fclose(pFileMon);
  }

  if (kind==2) /* monitor TOF */
  { ReadSimData(&dTimeMeas, &dLmbdWant, &dFreq);
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
  if (pPosT!=NULL) free(pPosT);
  if (pInt !=NULL) free(pInt);
  if (pNorm!=NULL) free(pNorm);
  if (pSD  !=NULL) free(pSD);
  if (pBinN!=NULL) free(pBinN);

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
