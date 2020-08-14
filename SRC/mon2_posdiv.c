/********************************************************************************************/
/*  VITESS module 'mon2_posdiv.c'                                                           */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added wavelength and yz position filter                    */
/* 1.2b JAN 2010  A. Houben      xyz output                                                 */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_MON2_POSDIV;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE; 
int    index_yz= 1;
long   nbin_pos= 0, 
       nbin_div= 0,
       format  = 0;
double pos_min = 0.0,
       pos_max = 0.0,
       div_min = 0.0,
       div_max = 0.0;
double filtLambdaMin=-1.0,          /* filter      */
       filtLambdaMax=-1.0,
       filtYMin     =-1.0e10,
       filtYMax     = 1.0e10,
       filtZMin     =-1.0e10,
       filtZMax     = 1.0e10;
static 
double bdiv_[BINSIZE],
       bpos_[BINSIZE];


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  int		 dpos=0, ddiv=0;
  long	 i=0;
  double pos_=0.0, 
         div_=0.0, 
         bintc=0.0,
         prob =0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

   //New pointers allowing for global write out
  by = bpos_;
  bz = bdiv_;

  for(dpos = 0; dpos<nbin_pos+1; dpos++)
  {
    bpos_[dpos] = pos_min + (pos_max-pos_min) * dpos / (double)nbin_pos;

    for(ddiv = 0;ddiv<(nbin_div+1); ddiv++)
	  {
	    bdiv_[ddiv]       = div_min + (div_max-div_min)  * ddiv / (double) nbin_div;
	    binyz[dpos][ddiv] = 0.0;
	    binyzerror[dpos][ddiv] =0.;
	    binyzcounts[dpos][ddiv]=0;
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
	    bRegistered = FALSE;
       
	    if(bExclusive==0) 
	      WriteNeutron(&(InputNeutrons[i]));

	    if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	    if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	    if (InputNeutrons[i].Position[1] < filtYMin) continue;
	    if (InputNeutrons[i].Position[1] > filtYMax) continue;
	    if (InputNeutrons[i].Position[2] < filtZMin) continue;
	    if (InputNeutrons[i].Position[2] > filtZMax) continue;

	    if (bProbactiv==TRUE) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob=1.0;

	    pos_ = InputNeutrons[i].Position[index_yz];

	    if (index_yz == 1) 
      {
	      if (InputNeutrons[i].Vector[0] >=0) 
          div_ = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	      else 
          div_ = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	  
	      div_ *= 180.0/M_PI;
	    }
	    else 
      {
	      div_  = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));	 
	      div_ *= 180.0/M_PI;
	    }
	    dpos = (int)floor(nbin_pos*(pos_-pos_min)/(pos_max-pos_min));
	    ddiv = (int)floor(nbin_div*(div_-div_min)/(div_max-div_min));
			
	    if (((dpos>=0)&&(dpos<nbin_pos))&&((ddiv>=0)&&(ddiv<nbin_div)))
      {	
	      binyz[dpos][ddiv] = binyz[dpos][ddiv] +  prob ;
	      bintc = bintc + prob;
	      bRegistered=1;
	      binyzcounts[dpos][ddiv]++;
	    }
	  
	    if((bExclusive==1) && (bRegistered==1))
	      WriteNeutron(&(InputNeutrons[i]));
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  if (index_yz==1)
    WriteOutput (fMonitor, format, bProbactiv, nbin_pos, nbin_div, " Y [cm]", "divergence Y [deg]");
  else
    WriteOutput (fMonitor, format, bProbactiv, nbin_pos, nbin_div, " Z [cm]", "divergence Z [deg]");

  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}

/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'q':
	        index_yz = atol(&argv[i][2]); /*  y or z direction */
	        break;

        case 'O':
	        MonFileName=&argv[i][2];
	        break;

        case 'y':
	        nbin_pos = atol(&argv[i][2]); /* number of bins horizontal axis*/
	        if(nbin_pos>BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;
        case 'z':
	        nbin_div = atol(&argv[i][2]); /* number of bins vertical axis*/
	        if(nbin_div>BINSIZE)
	          {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

        case 'h':
	        div_min =  atof(&argv[i][2]);   /* bottom position window */
	        break;
        case 'w':
	        pos_min = atof(&argv[i][2]);		/* left edge position window */
	        break;
        case 'H':
	        div_max =  atof(&argv[i][2]);   /* top position window  */
	        break;
        case 'W':
	        pos_max = atof(&argv[i][2]);		/* right edge position window  */
	        break;

        case 'p':
	        bProbactiv = atof(&argv[i][2]);
	        /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;
        case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	        break;

        case 'l':
          filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'L':
          filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;
        case 'u':
          filtYMin = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'U':
          filtYMax = atof(&argv[i][2]);   /* filter Y */
          break;
        case 'v':
          filtZMin = atof(&argv[i][2]);   /* filter Z */
          break;
        case 'V':
          filtZMax = atof(&argv[i][2]);   /* filter Z */
          break;

        case 'F':
          format = atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz */
          break;

        default:
	        fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	        exit(-1);
	        break;
      }
    }
  }

  // opens monitor file
  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"\n you must define a MonitorOutputFile");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  /*initialisation */
  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}
