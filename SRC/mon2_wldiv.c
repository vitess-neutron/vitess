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
/* 1.3a Nov 2020  K. Lieutenant  new 'mon2_header'                                          */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "init.h"
#include "softabort.h"
#include "general.h"
#include "mon2_header.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName= NULL;      // -O    [-]    Monitor output file containing intensity as a function of y- and z-position  
short  bProbactiv = TRUE,      // -p    [-]    flag Display  : YES: Probability weight   NO: number of trajectories
       bExclusive = FALSE;     // -e    [-]    flag Exclusion: YES: only neutrons meeting the monitor conditions are written
int    index_yz   = Y_AXIS;    // -q    [-]   enum direction:  Y_AXIS  Z_AXIS   
long   nbin_wl    = 1,         // -y    [-]    number of bins in horizontal direction (TOF)
       nbin_div   = 1,         // -z    [-]    number of bins in vertical direction   (lambda)
       format     = MATRIX;    // -F    [-]    file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double wl_min       = 0.0,     // -w   [Ang]   min. wavelength to be monitored 
       wl_max       = 0.0,     // -W   [Ang]   max. wavelength to be monitored 
       div_min      = 0.0,     // -h   [deg]    min. divergence to be monitored 
       div_max      = 0.0,     // -H   [cm]    max. divergence to be monitored       
       constrain_min= 0.0,     // -c   [cm]    constraint: min. divergence in perpendicular direction  
       constrain_max= 0.0,     // -C   [cm]    constraint: max. divergence in perpendicular direction      
       filtYMin     =-1.0e10,  // -u   [cm]   filter: left edge position of the monitored area
       filtYMax     = 1.0e10,  // -U   [cm]   filter: right edge position of the monitored area
       filtZMin     =-1.0e10,  // -v   [cm]   filter: bottom position of the monitored area
       filtZMax     = 1.0e10;  // -V   [cm]   filter: top position of the monitored area

// Variables determined from input parameters
FILE*  fMonitor   = NULL;

double BinPosY   [BINSIZE];           // edges of the bins of the first parameter 
double BinPosZ   [BINSIZE];           // edges of the bins of the second parameter 
double IntYZ     [BINSIZE][BINSIZE];  // intensity within a bin (in 2 dimensions) 
double IntYZError[BINSIZE][BINSIZE];  // standard deviation of this intensity 
long   nTrajYZ   [BINSIZE][BINSIZE];  // number of trajectories within a bin


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global variables


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
  short  bRegistered=0;
  int    index_c =0, 
         iwl =0,
         idiv=0;
  long   i=0;
  double wl  =0.0,   // wavelength
         div =0.0,   // divergence and
         prob=0.0,   // weight of a trajectory
         bintc=0.0;  // total intensity within binning
  double div_other_direction=0.0;
  
  
  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON2_WLDIV;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3a");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  // initializes arrays
  for (iwl=0; iwl < nbin_wl+1; iwl++)
  {
    BinPosY[iwl] = wl_min + (wl_max-wl_min) * iwl / (double)nbin_wl;

    for (idiv=0; idiv < (nbin_div+1); idiv++)
    {
	    BinPosZ        [idiv] = div_min + (div_max-div_min)  * idiv / (double) nbin_div;
	    IntYZ     [iwl][idiv] = 0.0;
	    IntYZError[iwl][idiv] = 0.0;
	    nTrajYZ   [iwl][idiv] = 0;
    }
  }

	if(index_yz == Y_AXIS) index_c = Z_AXIS;	
	if(index_yz == Z_AXIS) index_c = Y_AXIS;
  
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
      
	      wl = InputNeutrons[i].Wavelength;

	      if (index_yz == Y_AXIS)
        {
	        if (InputNeutrons[i].Vector[0] >=0) 
            div = atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	        else 
            div = atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	  
	        div*=180.0/M_PI;
	      }
	      else if (index_yz == Z_AXIS) 
        {
	        div = atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));	 
	        div*=180.0/M_PI;
	      }
        else
        {
          Error("Analysis direction does not have a proper value");
        }

	      iwl  = (int)floor(nbin_wl *(wl -wl_min) /(wl_max -wl_min));
	      idiv = (int)floor(nbin_div*(div-div_min)/(div_max-div_min));
			
	      if (((iwl>=0)&&(iwl<nbin_wl))&&((idiv>=0)&&(idiv<nbin_div))) 
        {	
	        nTrajYZ[iwl][idiv]++;
	        IntYZ  [iwl][idiv]+= prob;
	        bintc             += prob;
	        bRegistered = 1;
	      }

	      if ((bExclusive==1) && (bRegistered==1))
	        WriteNeutron(&(InputNeutrons[i]));
      }
	  }
  }

// Finish: writes and closes monitor files, writes to log and instrument file, frees memory
// ----------------------------------------------------------------------------------------
my_exit:
  // writes and closes monitor file 
  if (index_yz==Y_AXIS)
    WriteHeader2D (fMonitor, format, "Intensity", bProbactiv, nbin_wl, "wavelength [A]", nbin_div, "divergence Y [deg]");
  else if (index_yz == Z_AXIS) 
    WriteHeader2D (fMonitor, format, "Intensity", bProbactiv, nbin_wl, "wavelength [A]", nbin_div, "divergence Z [deg]");
  else
    Error("Analysis direction does not have a proper value");

  // WriteOutput2D(fMonitor, format, bProbactiv,  nbin_wl, BinPosY,        nbin_div, BinPosZ,  IntYZ, IntYZError, nTrajYZ);
  WriteOutput2D(fMonitor, format, bProbactiv,  nbin_wl, BinPosY, BINSIZE,  nbin_div, BinPosZ,  
                         (double*)IntYZ, (double*)IntYZError, (long*)nTrajYZ);
  fclose(fMonitor);

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
	        if (index_yz==Y_AXIS) fprintf(LogFilePtr,"horizontal direction \n");
		      if (index_yz==Z_AXIS) fprintf(LogFilePtr,"vertical direction \n");
	        if (index_yz!=Y_AXIS && index_yz!=Z_AXIS)
            {index_yz = Y_AXIS ; Warning("horizontal direction chosen");}
		    break;

        case 'O':
	        MonFileName=&argv[i][2];
	        break;

	      case 'y':
	        nbin_wl = atol(&argv[i][2]); /* number of bins y-direction */
	        if(nbin_wl>BINSIZE)
	          {fprintf(LogFilePtr,"ERROR:  number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;
	      case 'z':
	        nbin_div = atol(&argv[i][2]); /* number of bins, z-direction */
	        if(nbin_div>BINSIZE)
	          {fprintf(LogFilePtr,"ERROR:  number of bins must be <= %d \n", BINSIZE); exit(99);}
	        break;

	      case 'c':
	        constrain_min =  atof(&argv[i][2]);   
	        break;
	      case 'C':
	        constrain_max =  atof(&argv[i][2]);   
	        break;

	      case 'w':
	        wl_min = atof(&argv[i][2]);		
	        break;
	      case 'W':
	        wl_max = atof(&argv[i][2]);	
	        break;
	      case 'h':
	        div_min =  atof(&argv[i][2]);   
	        break;
	      case 'H':
	        div_max =  atof(&argv[i][2]);   
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
	        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
	        exit(-1);
	    }
    }
  }

  // opens monitor file
  if (MonFileName==NULL)
  {
    fprintf(LogFilePtr,"ERROR: you must define a MonitorOutputFile \n");
    exit(99);
  }
  else
  { fMonitor = OpenOutputFile(MonFileName, TRUE, "wt");
  }

  if (bProbactiv != 1) 
    bProbactiv = 0;

  return;
}