/********************************************************************************************/
/*  VITESS module 'monitorpol_pos.c'                                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "init.h"
#include "softabort.h"
#include "general.h"
#include "matrix.h"
#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName= NULL;   // -O    [-]   Monitor output file containing polarization as a function of y- and z-position
short  bProbactiv = TRUE,   // -p    [-]   flag Display  : YES: Probability weight   NO: number of trajectories
       bExclusive = FALSE;  // -e    [-]   flag Exclusion: YES: only neutrons meeting the monitor conditions are written  NO: all are written
long   nbiny      = 1,      // -y    [-]   number of bins in horizontal direction
       nbinz      = 1;      // -z    [-]   number of bins in vertical direction
double analysis_dir[3]      // -a -b -c    components of the quantization direction in x-, y- and z-direction
           ={0.0,0.0,1.0},
       widthmin   = 0.0,    // -w   [cm]   left edge position of the monitored area
       widthmax   = 0.0,    // -W   [cm]   right edge position of the monitored area
       heightmin  = 0.0,    // -h   [cm]   bottom position of the monitored area
       heightmax  = 0.0;    // -H   [cm]   top position of the monitored area

// Variables determined from input parameters
FILE*  fMonitor     = NULL;
double RotMatrixAnalysis[3][3]={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  // char   weightTag[2][7] = {"", "weight"};
  short  bRegistered=0;
  int     dy=0, dz=0;
  long   i=0;
  double bintc   =0.0,
         bintcpol=0.0,
         prob    =0.0;
  double bposz   [BINSIZE],
         bposy   [BINSIZE],
         binyz   [BINSIZE][BINSIZE],
         binyzpol[BINSIZE][BINSIZE];
  VtFormat2D  eFormat = MATRIX;   //  file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON2_POL_POS;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  // initializes arrays
  for(dy=0; dy < nbiny+1; dy++)
  {
    bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

    for(dz=0; dz < (nbinz+1); dz++)
    {
      bposz   [dz]     = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
      binyz   [dy][dz] = 0.0;
      binyzpol[dy][dz] = 0.0;
    }
  }

  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        bRegistered=0;

        if(bProbactiv==1.0)
          prob = InputNeutrons[i].Probability;
        else
          prob=1.0;

        /* calculate spin vector in the direction of the analysis */
        RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

        dy = (int)floor(nbiny*(InputNeutrons[i].Position[1]-widthmin)/(widthmax-widthmin));
        dz = (int)floor(nbinz*(InputNeutrons[i].Position[2]-heightmin)/(heightmax-heightmin));

        if (((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
        {
          binyzpol[dy][dz] = binyzpol[dy][dz] +  prob * InputNeutrons[i].Spin[0];
          bintcpol = bintcpol + prob * InputNeutrons[i].Spin[0];
          binyz[dy][dz] = binyz[dy][dz] +  prob;
          bintc = bintc + prob;
          bRegistered=1;
        }

        /* calculate spin vector in the original direction */
        RotBackVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

        if ((bExclusive==0)||(bRegistered==1))
          WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file
  WriteHeader2D(fMonitor, eFormat, "polarization", bProbactiv,
                          nbiny, "pos_y [cm]", widthmin,  widthmax,
                          nbinz, "pos_z [cm]", heightmin, heightmax);

  for (dy = 0; dy < nbiny; dy++)
  {
    fprintf(fMonitor, "%10.4f   ", (bposy[dy]+bposy[dy+1])/2.0);
  }
  for (dz = 0; dz<nbinz; dz++)
  {
    fprintf(fMonitor, "\n%10.4f  ", (bposz[dz]+bposz[dz+1])/2.0);
    for (dy = 0; dy < nbiny; dy++)
    {
      if (binyz[dy][dz] == 0.)
        fprintf(fMonitor,"%12.5e ", 0.0);
      else
        fprintf(fMonitor,"%12.5e ", binyzpol[dy][dz]/binyz[dy][dz]);
    }
  }
  fprintf(fMonitor, "\n");
  fclose(fMonitor);

  if(bintc != 0.0)
    fprintf(LogFilePtr, "polarization: %8.5f \n", bintcpol/bintc);

  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int    i;
  double roty, rotz;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'O':
          MonFileName=&argv[i][2];
          break;

        case 'a':
          sscanf(&argv[i][2], "%lf", &analysis_dir[0]) ;
          break;
        case 'b':
          sscanf(&argv[i][2], "%lf", &analysis_dir[1]) ;
          break;
        case 'c':
          sscanf(&argv[i][2], "%lf", &analysis_dir[2]) ;
          break;

        case 'y':
          nbiny = atol(&argv[i][2]);        /* number of bins y-direction */
          if (nbiny > 1000)
            {fprintf(LogFilePtr,"ERROR:  number of bins must be <= 1000 \n"); exit(99);}
          break;
        case 'z':
          nbinz = atol(&argv[i][2]);        /* number of bins, z-direction */
          if (nbinz > 1000)
            {fprintf(LogFilePtr,"ERROR:  number of bins must be <= 1000 \n"); exit(99);}
          break;

        case 'w':
          widthmin = atof(&argv[i][2]);      /* left edge position window   [cm]*/
          break;
        case 'W':
          widthmax = atof(&argv[i][2]);      /* right edge position window    [cm]*/
          break;
        case 'h':
          heightmin =  atof(&argv[i][2]);   /* bottom position window   [cm]*/
          break;
        case 'H':
          heightmax =  atof(&argv[i][2]);   /* top position window   [cm]*/
          break;

        case 'p':
          bProbactiv = atof(&argv[i][2]);   /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
          break;

        case 'e':
          if(argv[i][2]=='1')
            bExclusive = 1;                 /* if activated, only neutrons meeting the monitor conditions are considered further on */
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (MonFileName==NULL)
  {
    Error("you must define a MonitorOutputFile");
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1)
    bProbactiv = 0;

  CartesianToEulerZY(analysis_dir, &roty, &rotz);
  FillRotMatrixZY(RotMatrixAnalysis, roty, rotz);

  return;
}
