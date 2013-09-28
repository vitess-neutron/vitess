/*********************************************************************************************/
/*  VITESS module 'mon_brilliance.c'                                                         */
/*    monitoring of brilliance as a function of 1 parameter                                  */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given    */
/* to the authors:                                                                           */
/* 1.0  K. Lieutenant May 2012 initial version (based on monitor1.c)                         */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"

#define MAX_KIND 8

void OwnInit(int argc, char *argv[]);

// Global variables
// ----------------
FILE	*pFileMon=NULL,        // pointer to monitor output file
      *pFileRef=NULL,              // pointer to reference file
      *pFileFlux=NULL;             // pointer to flux file
char  *MonitorFileName=NULL,   // name of monitor output file
      *RefFileName=NULL,           // name of reference file
      *FluxFileName=NULL;          // name of flux file
short  kind=0,                 // defines variable parameter in brilliance monitoring
       src_type=0,                  // defines source type: 0: constant source  1: pulsed source
       exclusivecount=0;            // criterion: only trajectories within limits are written
long   nBin=100,               // number of bins in monitor file
       nColour=ANY_COLOR;                   // color of trajectory that is monitored   (0=all)
double MinY = -10.0,  MaxY  = 10.0, // min. and max. width to be taken into account
       MinZ = -10.0,  MaxZ  = 10.0, // min. and max. height to be taken into account
       MinDivY=-5.0,  MaxDivY= 5.0, // min. and max. hor. div. to be taken into account
       MinDivZ=-5.0,  MaxDivZ= 5.0, // min. and max. vert. div. to be taken into account
       MinDivR= 0.0,  MaxDivR=10.0, // min. and max. radial div. to be taken into account
       MinLmbd= 0.0,  MaxLmbd=20.0, // min. and max. width to be taken into account
       MinTime=-1.0e9,MaxTime=1.0e9,// min. and max. TOF to be taken into account
       MinRange,      MaxRange;     // min. and max. value of the variable parameter
double DelLmbd,                // width of wavelength band used to calculate the brilliance (transfer)
       DelTime=0.0,                     // space of time used to calculate the brilliance (transfer); only used for pulsed sources
       DelY, DelZ,                  // spatial width and height used to calculate the brilliance (transfer)
       DelDivY, DelDivZ,            // hor. and vert. divergence range used to calculate the brilliance (transfer)
       DelDivR,                     // radial divergence range used to calculate the brilliance (transfer)
       Freq=0.0;                    // source frequency   (from simulation.inf)


int main(int argc, char *argv[])
{
  char	 sBuffer[512];
  char   sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "cm", "cm", "deg", "deg", "deg"},
         sParN[MAX_KIND+1][22]={"", "wavelength", "time", "horizontal position", "vertical position",
                                "horizontal divergence", "vertical divergence", "radial divergence"};

  char  weightTag[2][7] = {"", "weight"};
  short  registered=0;         // criterion: trajectory is within limits set
  long   iBin,                 // bin number
      i,                    // index of trajectories
      normalise,            // 1: absolute brilliance value   2: relative brilliance
      // crot = 0,          // number of rot angles for yz
      nTrjTot=0;            // total number of traj. within binning and eval. time
  double dIntTot=0.0,     // total count rate within binning and eval. time
      P,                    // weight,
      Lmbd,                 // wavelength,
      Time,                 // time of flight,
      Y, Z;                 // hor. and vert. position of the neutron under consideration
  double dBinSize,        // size of each bin
         MonData[3];           // data in one monitor row
  double DivY, DivZ=0, DivR,   // hor., vert. and radial divergence of the trajectory
      *pPosT=NULL,          /* limits of bin (minimal and maximal value)  */
      *pInt=NULL,           /* intensity (=count rate) per bin  */
      *pNorm=NULL,          /* normalisation value for each bin */
      *pSD=NULL,            /* standard deviation per bin       */
      ParCntr,              // center of a bin of the variable parameter
      PhaseSpaceVol,        // phase space volume of a bin
      Brilliance,           // brilliance within one bin
      Transmission,         // brilliance transfer within one bin
      BrillMax=0.0,         // maximal brilliance
      BrillAve=0,           // average brilliance
      TransMax=0.0,         // maximal brilliance transfer
      TransAve=0,           // average brilliance transfer
      BrillAveIn=0.0;       // average brilliance of reference spectrum
    long   *pBinN=NULL;     // number of trajectories per bin


    /* initialisation */
    /* -------------- */
    Init   (argc, argv, VT_MONITOR_1);
    print_module_name("mon_brilliance 1.0");
    OwnInit(argc, argv);

    if (pFileRef!=NULL)
      normalise = 2;
    else
      normalise = 1;

    pPosT = (double*) calloc(nBin+1,sizeof(double));
    pInt  = (double*) calloc(nBin+1,sizeof(double));
    pSD   = (double*) calloc(nBin+1,sizeof(double));
    pNorm = (double*) calloc(nBin+1,sizeof(double));
    pBinN = (long*)   calloc(nBin+1,sizeof(long));

    switch (kind)
      {
      case 1: MinRange = MinLmbd; MaxRange = MaxLmbd; break;  //monitor lambda dependent brilliance
      case 2: MinRange = MinTime; MaxRange = MaxTime; break;  //monitor time dependent brilliance
      case 3: MinRange = MinY;    MaxRange = MaxY;    break;  //monitor y dependent brilliance
      case 4: MinRange = MinZ;    MaxRange = MaxZ;    break;  //monitor z dependent brilliance
      case 5: MinRange = MinDivY; MaxRange = MaxDivY; break;  //monitor div_y dependent brilliance
      case 6: MinRange = MinDivZ; MaxRange = MaxDivZ; break;  //monitor div_z dependent brilliance
      case 7: MinRange = MinDivR; MaxRange = MaxDivR; break;  //monitor radial divergence dependent brilliance
      default:  Error("Kind of brilliance calculation unknown");
      }
    dBinSize = (MaxRange - MinRange)/(double)nBin;

    for (iBin=0; iBin<=nBin; iBin++)
      {
        pPosT[iBin]=MinRange+(dBinSize*iBin);
        pInt [iBin]=0.0;
        pSD  [iBin]=0.0;
        pBinN[iBin]=0;
		// for brilliance transfer read reference file and assign these values as normalization
        if (normalise==2)                       
          { ReadLine(pFileRef, sBuffer, sizeof(sBuffer)-1);
            StrgScanLF(sBuffer, MonData, 3, 0);
            pNorm[iBin] = MonData[1];           // brilliance is the second value in brilliance monitor
            BrillAveIn += MonData[1]/nBin;
          }
        else                                    // absolute brilliance
          { pNorm[iBin] = 1.0;
          }
      }

    /* output to log file */
    /* ------------------ */
    if (normalise==2)
      { fclose(pFileRef);
        fprintf(LogFilePtr, "brilliance transfer relative to %s ", RefFileName);
      }
    else
      { fprintf(LogFilePtr, "absolute brilliance ");
      }
    if (src_type==0)
      fprintf(LogFilePtr, "of a constant source \n");
    else
      fprintf(LogFilePtr, "of a pulsed source of frequency %4.1f Hz \n", Freq);
    fprintf(LogFilePtr, "Binning  : %ld bins in %s from %10.5f to %10.5f %s\n", nBin, sParN[kind], MinRange, MaxRange, sUnit[kind]);
    fprintf(LogFilePtr, "File     : %s\n", MonitorFileName);

    /* loop over trajectories */
    /* ---------------------- */
    DECLARE_ABORT;

    while (ReadNeutrons()!= 0) {
      CHECK;
      for(i=0; i<NumNeutGot; i++) {
        CHECK;

        /* write out all neutrons, if 'exclusive counts = no' is set */
        if (exclusivecount==0)
          WriteNeutron(&(InputNeutrons[i]));

        registered=0;
        P    = InputNeutrons[i].Probability;
        Lmbd = InputNeutrons[i].Wavelength;
        Time = InputNeutrons[i].Time;
        Y    = InputNeutrons[i].Position[1];
        Z    = InputNeutrons[i].Position[2];
        if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          DivY = 0.0;
        else
          DivY = 180.0/M_PI * atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[0]);
        if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
          DivY = 0.0;
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

		// determine channel for a given trajetory depending on the variable parameter
        switch (kind) {
        case 1: //monitor lambda dependent brilliance
          iBin = (int)floor(nBin * (Lmbd - MinLmbd)/(MaxLmbd - MinLmbd));
          break;

        case 2: //monitor time dependent brilliance
          iBin = (int)floor(nBin * (Time - MinTime)/(MaxTime - MinTime));
          break;

        case 3: //monitor y dependent brilliance
          iBin = (int)floor(nBin * (Y - MinY)/(MaxY - MinY));
          break;

        case 4: //monitor z dependent brilliance
          iBin = (int)floor(nBin * (Z - MinZ)/(MaxZ - MinZ));
          break;

        case 5: //monitor div_y dependent brilliance
          iBin = (int)floor(nBin * (DivY - MinDivY)/(MaxDivY - MinDivY));
          break;

        case 6: //monitor div_z dependent brilliance
          iBin = (int)floor(nBin * (DivZ - MinDivZ)/(MaxDivZ - MinDivZ));
          break;
		
        case 7: //monitor radial divergence dependent brilliance
          iBin = (int)floor(nBin * (DivR - MinDivR)/(MaxDivR - MinDivR));
          break;
        }

        if (iBin >= 0  &&  iBin < nBin) {
          pInt [iBin] += P;
          pBinN[iBin] += 1;
          dIntTot   += P;
          nTrjTot   += 1;
          registered=1;
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
    {
      fprintf(pFileMon, "#Monitor weight\n");
      for (iBin = 0; iBin < nBin; iBin++) 
      {
        switch(kind)
        { case 1: DelLmbd = (pPosT[iBin+1] - pPosT[iBin]);              break;
          case 2: DelTime = (pPosT[iBin+1] - pPosT[iBin]) / 1000.0;     break; // ms -> s
          case 3: DelY    = (pPosT[iBin+1] - pPosT[iBin]);              break;
          case 4: DelZ    = (pPosT[iBin+1] - pPosT[iBin]);              break;
          case 5: DelDivY = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
          case 6: DelDivZ = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
          case 7: DelDivR = (pPosT[iBin+1] - pPosT[iBin]) * M_PI/180.0; break; // deg -> rad
        }

        ParCntr = (pPosT[iBin]+pPosT[iBin+1])/2.0;

        if (kind==7) // radial divergence
          PhaseSpaceVol= DelLmbd * DelY * DelZ * 2*M_PI * ParCntr*M_PI/180.0 * DelDivR;
        else
          PhaseSpaceVol= DelLmbd * DelY * DelZ * DelDivY * DelDivZ;

        if (src_type==0)
          Brilliance = pInt[iBin] / PhaseSpaceVol;
        else
          Brilliance = pInt[iBin] / PhaseSpaceVol / Freq / DelTime;
        
		    if (pNorm[iBin] > 0.0)
		      Transmission = Brilliance / pNorm[iBin];
		    else
          Transmission = 1.0;

        if(pBinN[iBin]!=0)
          pSD[iBin] = Transmission / sqrt((double)pBinN[iBin]);
        else
          pSD[iBin] = 0.0;

        fprintf(pFileMon,"%11.7e  %11.7e  %11.7e  %7ld\n", ParCntr, Transmission, pSD[iBin], pBinN[iBin]);

        BrillMax = Max(BrillMax, Brilliance);
        TransMax = Max(TransMax, Transmission);
      }

      if (kind==7)
        PhaseSpaceVol = (MaxLmbd - MinLmbd) * (MaxY - MinY) * (MaxZ - MinZ) * M_PI * (sq(M_PI/180.0*MaxDivR) - sq(M_PI/180.0*MinDivR));
      else
        PhaseSpaceVol = (MaxLmbd - MinLmbd) * (MaxY - MinY) * (MaxZ - MinZ) * M_PI/180.0*(MaxDivY - MinDivY) * M_PI/180.0*(MaxDivZ - MinDivZ);

      if (src_type==0)
        BrillAve = dIntTot / PhaseSpaceVol;
      else
        BrillAve = dIntTot / (PhaseSpaceVol * Freq * (MaxTime - MinTime)/1000.0);

      TransAve = BrillAve/BrillAveIn;

      fclose(pFileMon);
    }

    if (pFileFlux != NULL) {
      fprintf(pFileFlux, "%10.3f %10.3f %10.3f %10.3f %10.3f %10.3f     %11.4e     %11.4e \n",
              0.5*(MaxLmbd+MinLmbd), 0.5*(MaxY+MinY), 0.5*(MaxZ+MinZ), 0.5*(MaxDivY+MinDivY), 0.5*(MaxDivZ+MinDivZ), 0.5*(MaxDivR+MinDivR),
              BrillAve, BrillMax);
      fclose(pFileFlux);
    }

    fprintf(LogFilePtr, "total neutron count rate within given ranges: %11.4e n/s \n", dIntTot);
    fprintf(LogFilePtr, "average and maximal brilliance         : %11.4e  %11.4e n/(cm^2 s Ang sterad)\n\n", BrillAve, BrillMax);
    if (normalise==2)
      fprintf(LogFilePtr, "average and maximal brilliance transfer: %7.3f  %7.3f \n\n", TransAve, TransMax);

    stPicture.eType  = (short) kind;
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


void OwnInit(int argc, char *argv[])
{
  int    i;
  double TimeMeas,   // measuring time
    LmbdWant;   // wanted wavelength
  char	 sNewName[99]="";

  for(i=1; i<argc; i++) {
    if(argv[i][0]!='+') {
      switch(argv[i][1]) {
      case 'O':
        MonitorFileName=&argv[i][2];
        pFileMon = fopen(FullParName(MonitorFileName),"wt");
        if (pFileMon==NULL) {
          char* p1=NULL, *p2=NULL;

          fprintf(LogFilePtr,"\nFile %s could not be opened for monitor output\n", MonitorFileName);

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

      case 'S':
        RefFileName=&argv[i][2];
        pFileRef = fopen(FullParName(RefFileName),"rt");
        if (pFileRef==NULL)
          {  fprintf(LogFilePtr,"\nReference file %s could not be opened\n", RefFileName);
          }
        break;

      case 'F':
        FluxFileName=&argv[i][2];
        pFileFlux = fopen(FullParName(FluxFileName),"at");
        if (pFileFlux==NULL)
          {  fprintf(LogFilePtr,"\nFlux file %s could not be opened\n", FluxFileName);
          }
        break;

      case 'k':
        kind = atol(&argv[i][2]);     // 1: lambda  2: time  3: y  4: z  5: div_y  6: div_z  7: div_rad
        break;
        /* case 'p':
           src_type = atol(&argv[i][2]); // 0: constant source  1: pulsed source
           break; */

      case 'n':
        nBin = atol(&argv[i][2]); /* number of bins */
        break;

      case 'l':
        MinLmbd = atof(&argv[i][2]);   // lower bound lambda [Å]
        break;
      case 'L':
        MaxLmbd = atof(&argv[i][2]);   // upper bound lambda [Å]
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
        if(argv[i][2]=='1') exclusivecount = 1;   // if activated, only neutrons meeting the monitor conditions are considered further on
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
        break;
      }
    }
  }

  // pulsed source assumed if time is variable parameter or frequency > 0
  if (kind==2  || Freq > 0.0) src_type=1;

  if (src_type==1 && Freq==0.0)
    ReadSimData(&TimeMeas, &LmbdWant, &Freq);

  // ranges of phase space
  if (src_type==1)
  {  if (MinTime==-1.0e9 || MaxTime==1.0e9)
      DelTime = 1.0/Freq;
    else
      DelTime = (MaxTime - MinTime)/1000.0;   // ms -> s;  only used for time dependent brilliance
  }
  DelLmbd =  MaxLmbd - MinLmbd;
  DelY    =  MaxY - MinY;
  DelZ    =  MaxZ - MinZ;
  DelDivY = (MaxDivY - MinDivY) * M_PI/180.0; // deg -> rad
  DelDivZ = (MaxDivZ - MinDivZ) * M_PI/180.0; // deg -> rad
  DelDivR = (MaxDivR - MinDivR) * M_PI/180.0; // deg -> rad

  if (MonitorFileName==NULL) {
    fprintf(LogFilePtr,"you must define a MonitorOutputFile\n");
    exit(99);
  }

}
