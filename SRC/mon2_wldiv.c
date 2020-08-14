/********************************************************************************************/
/*  VITESS module 'mon2_wldiv.c'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added yz position filter                                   */
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
McCompID _eModule=MCN_MON2_WLDIV;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE;
int    index_yz   = 1;
long   nbin_wl    = 0, 
       nbin_div   = 0,
       format     = 0;
double wl_min       = 0.0, 
       wl_max       = 0.0, 
       constrain_min= 0.0, 
       constrain_max= 0.0, 
       div_min      = 0.0, 
       div_max      = 0.0,       
       filtYMin     =-1.0e10,
       filtYMax     = 1.0e10,
       filtZMin     =-1.0e10,
       filtZMax     = 1.0e10;

static 
double bdiv_[BINSIZE],
       bwl_ [BINSIZE];


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  short  bRegistered=0;
  int    index_c =0, 
         dwl =0,
         ddiv=0;
  long   i=0;
  double wl_=0.0, 
         div_=0.0, 
         prob=0.0, 
         bintc=0.0;
  double div_other_direction=0.0;
  
  
  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  //New pointers allowing for global write out
  by = bwl_;
  bz = bdiv_;

  // initializes arrays
  for(dwl = 0; dwl<nbin_wl+1; dwl++)
  {
    bwl_[dwl] = wl_min + (wl_max-wl_min) * dwl / (double)nbin_wl;

    for(ddiv = 0;ddiv<(nbin_div+1); ddiv++)
    {
	    bdiv_[ddiv] = div_min + (div_max-div_min)  * ddiv / (double) nbin_div;
	    binyz[dwl][ddiv] = 0.0;
	    binyzerror[dwl][ddiv]=0.;
	    binyzcounts[dwl][ddiv]=0;
    }
  }

	if(index_yz == 1) index_c = 2 ;	
	if(index_yz == 2) index_c = 1 ;
  
  DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
  while(ReadNeutrons()!= 0)
  {
    
    for(i=0; i<NumNeutGot; i++)
	  {
      CHECK;
	    bRegistered=0;

	    if(bExclusive==0) 
		  WriteNeutron(&(InputNeutrons[i]));

	    if (InputNeutrons[i].Position[1] < filtYMin) continue;
	    if (InputNeutrons[i].Position[1] > filtYMax) continue;
	    if (InputNeutrons[i].Position[2] < filtZMin) continue;
	    if (InputNeutrons[i].Position[2] > filtZMax) continue;

	    if (bProbactiv==1.0) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob=1.0;

	    /* selects only trajectories in the given interval: constrain_min, constrain_max for the other direction */
	    div_other_direction = 180.0/M_PI * atan2(InputNeutrons[i].Vector[index_c],InputNeutrons[i].Vector[0]);
	    if ((div_other_direction <= constrain_min)||(div_other_direction >= constrain_max)) continue;
      
	    wl_ = InputNeutrons[i].Wavelength;

	    if (index_yz == 1)
      {
	      if (InputNeutrons[i].Vector[0] >=0) 
          div_ = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	      else 
          div_ = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	  
	      div_*=180.0/M_PI;
	    }
	    else 
      {
	      div_ = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));	 
	      div_*=180.0/M_PI;
	    }

	    dwl  = (int)floor(nbin_wl*(wl_-wl_min)/(wl_max-wl_min));
	    ddiv = (int)floor(nbin_div*(div_-div_min)/(div_max-div_min));
			
	    if (((dwl>=0)&&(dwl<nbin_wl))&&((ddiv>=0)&&(ddiv<nbin_div))) 
      {	
	        binyz[dwl][ddiv] = binyz[dwl][ddiv] +  prob;
	        bintc      = bintc + prob;
	        bRegistered=1;
	        binyzcounts[dwl][ddiv]++;
	    }

	    if ((bExclusive==1) && (bRegistered==1))
	      WriteNeutron(&(InputNeutrons[i]));
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  WriteOutput (fMonitor, format, bProbactiv, nbin_wl, nbin_div, "wavelength [A]", "divergence [deg]");

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
	        if (index_yz == 1)fprintf(LogFilePtr,"\nhorizontal direction \n");
		      if (index_yz == 2)fprintf(LogFilePtr,"\nvertical direction \n");
	        if ((index_yz != 1)&&(index_yz != 2)){index_yz = 1 ; fprintf(LogFilePtr,"\nwarning: horizontal direction chosen!\n");}
		    break;

        case 'O':
	        MonFileName=&argv[i][2];
	        break;

	      case 'y':
	        nbin_wl = atol(&argv[i][2]); /* number of bins y-direction */
	        if(nbin_wl>BINSIZE)
	          {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= %d", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nbin_div = atol(&argv[i][2]); /* number of bins, z-direction */
	        if(nbin_div>BINSIZE)
	          {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= %d", BINSIZE); exit(99);}
	        break;

	      case 'c':
	        constrain_min =  atof(&argv[i][2]);   
	        break;
	      case 'C':
	        constrain_max =  atof(&argv[i][2]);   
	        break;

	      case 'h':
	        div_min =  atof(&argv[i][2]);   
	        break;
	      case 'w':
	        wl_min = atof(&argv[i][2]);		
	        break;
	      case 'H':
	        div_max =  atof(&argv[i][2]);   
	        break;
	      case 'W':
	        wl_max = atof(&argv[i][2]);	
	        break;

	      case 'p':
	        bProbactiv = atof(&argv[i][2]);
	        /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;

	      case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
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
    fprintf(LogFilePtr,"\nERROR: you must define a MonitorOutputFile");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}