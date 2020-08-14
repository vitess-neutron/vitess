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
#include "init.h"
#include "softabort.h"
#include "general.h"
#include "matrix.h"

#define BINSIZE 401

/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_MON2_POL_POS;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE; 
long   nbiny      = 0, 
       nbinz      = 0;
double widthmin   = 0.0, 
       widthmax   = 0.0,
       heightmin  = 0.0,
       heightmax  = 0.0;
static 
double RotMatrixAnalysis[3][3], 
       analysis_dir[3],
       bposz   [BINSIZE],
       bposy   [BINSIZE],
       binyz   [BINSIZE][BINSIZE], 
       binyzpol[BINSIZE][BINSIZE];


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  char   weightTag[2][7] = {"", "weight"};
  short  bRegistered=0;
  int	   dy=0, dz=0;
  long	 i=0;
  double bintc   =0.0, 
         bintcpol=0.0,
         prob    =0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for(dy = 0; dy<nbiny+1; dy++)
  {
    bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

    for(dz = 0;dz<(nbinz+1); dz++)
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

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  fprintf(fMonitor,"#Monitor matrix %s\n", weightTag[bProbactiv]);
  for (dy = 0; dy < nbiny; dy++)
  {
    fprintf(fMonitor, "%10.7f\t", (bposy[dy]+bposy[dy+1])/2.0);
  }
  for (dz = 0; dz<nbinz; dz++)
  {
    fprintf(fMonitor, "\n %5.3f\t", (bposz[dz]+bposz[dz+1])/2.0);
    for (dy = 0; dy < nbiny; dy++)
	  {
	    if (binyz[dy][dz] == 0.)
        fprintf(fMonitor,"%5.3E\t", 0.);
	    else 
        fprintf(fMonitor,"%5.3E\t", binyzpol[dy][dz]/binyz[dy][dz]);
	  }
  }

  fclose(fMonitor);

  fprintf(LogFilePtr," \n");
  if(bintc != 0.) 
    fprintf(LogFilePtr,"polarization: %3.5f \n",bintcpol/bintc);

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
	          {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= 1000"); exit(99);}
	        break;
	      case 'z':
	        nbinz = atol(&argv[i][2]);        /* number of bins, z-direction */
	        if (nbinz > 1000)
	          {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= 1000"); exit(99);}
	        break;

	      case 'h':
	        heightmin =  atof(&argv[i][2]);   /* bottom position window   [cm]*/
	        break;
	      case 'w':
	        widthmin = atof(&argv[i][2]);		  /* left edge position window   [cm]*/
	        break;
	      case 'H':
	        heightmax =  atof(&argv[i][2]);   /* top position window   [cm]*/
	        break;
	      case 'W':
	        widthmax = atof(&argv[i][2]);		  /* right edge position window    [cm]*/
	        break;

	      case 'p':
	        bProbactiv = atof(&argv[i][2]);   /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;

	      case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;                 /* if activated, only neutrons meeting the monitor conditions are considered further on */
	        break;
	    }
    }
  }

  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"\nERROR: you must define a MonitorOutputFile");
    exit(99);
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