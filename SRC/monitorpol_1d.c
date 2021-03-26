/********************************************************************************************/
/*  VITESS module 'monitorpol_1D.c'                                                         */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.4  Nov 2020  K. Lieutenant  preparation for tranfer to version 4                       */
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


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define MAX_KIND  8


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
char*  MonFileName= NULL;   // -O    [-]   Monitor output file containing polarization as a function of the chosen parameter
short  bProbactiv = TRUE,   // -p    [-]   flag: YES: Probability weight   NO: number of trajectories
       bExclusive = FALSE;  // -e    [-]   flag: YES: only neutrons meeting the monitor conditions are written  NO: all are written
VtMonPar ePar = NO_PAR;     // -k    [-]   ID for parameter, as a function of which the intensity is shown
long   nbiny  = 1;          // -n    [-]   number of monitor channels
double xMin   = 0.0,        // -m   [var]  lower bound value of the monitored range 
       xMax   = 0.0,        // -M   [var]  upper bound value of the monitored range
       analysis_dir[3]      // -a -b -c    components of the quantization direction in x-, y- and z-direction
           ={0.0,0.0,1.0};

// Variables determined from input parameters
FILE*  fMonitor   = NULL;
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
  char   sCompName  [21]="",
         sModVsnName[40]="";
  char   sUnit[MAX_KIND+1][ 4]={"", "Ang", "ms", "deg", "deg","cm", "cm", "meV", "deg"},
         sParN[MAX_KIND+1][22]={"", "wavelength", "time",
                               "horizontal divergence", "vertical divergence",
                               "horizontal position",   "vertical position", "energy", "divergence yz"};
  short  bRegistered=FALSE;
  int	   dy=0, 
         bincounts[10001];
  long	 i=0;
  double bpost   [10001],
         bint    [10001],
         bintch  [10001], 
         binerror[10001],
         Divy  =0.0, 
         Divz  =0.0,  
         prob  =0.0,
         bintc =0.0,
         binpol=0.0;
  
  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_MON1_POL;

  Init(argc, argv, _eModule);
  OwnInit(argc, argv);

  CompID2Name (sCompName, _eModule);
  sprintf(sModuleName, "%s_%s",     sCompName, sParN[ePar]);
  sprintf(sModVsnName, "%s_%s 1.4", sCompName, sParN[ePar]);
  print_module_name(sModVsnName);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
  
  // initializes arrays
  for (dy=0; dy < nbiny+1; dy++)
  {
    bpost[dy] = xMin + ((xMax-xMin)*dy/(double)nbiny);
    bint     [dy]=0.0;
    bintch   [dy]=0.0;
    binerror [dy]=0.0;
    bincounts[dy]=0;
  }

  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  while (ReadNeutrons()!= 0)
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

	      if (bProbactiv==TRUE) 
          prob = InputNeutrons[i].Probability;
	      else 
          prob=1.0;
	  
	      /* calculate spin vector in the direction of the analysis */
	      RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

	      switch (ePar) 
        {
	        case MON_LAMBDA:
	          dy = (int) floor((double)nbiny*(InputNeutrons[i].Wavelength - xMin)/(xMax-xMin));
	    
	          if ((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0];
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
	          break;
	    
	        case MON_TIME:
	          dy = (int) floor(nbiny*(InputNeutrons[i].Time - xMin)/(xMax-xMin));	      
	          if ((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0] ;
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
	          break;
	    
	        case MON_DIV_Y:
	          Divy  = atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
	          Divy *= 180.0/M_PI;
	          if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	            Divy=0.0;
	    
	          dy = (int)floor(nbiny*(Divy - xMin)/(xMax-xMin));	      
	          if((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0] ;
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
	          break;
	    
	        case MON_DIV_Z:
	          Divz  = atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
	          Divz *= 180.0/M_PI;
	          if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	            Divz=0.0;
	    
	          dy = (int)floor(nbiny*(Divz - xMin)/(xMax-xMin));
	          if ((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0] ;
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
		      break;

	        case MON_Y:
	          dy = (int)floor(nbiny*(InputNeutrons[i].Position[1] - xMin)/(xMax-xMin));	      
	          if ((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0] ;
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
	          break;
	    
	        case MON_Z:
	          dy = (int)floor(nbiny*(InputNeutrons[i].Position[2] - xMin)/(xMax-xMin));	      
	          if ((dy>=0)&&(dy<nbiny))
	          {
		          bint[dy]   = bint[dy] + prob * InputNeutrons[i].Spin[0];
		          bintch[dy] = bintch[dy] + prob;
		          binpol     = binpol + prob * InputNeutrons[i].Spin[0] ;
		          bintc      = bintc + prob;
		          bRegistered=1;
	          }
	          break;

          default:
            fprintf(LogFilePtr,"Parameter not handled in %s", sCompName);
            exit(-1);
	      }
	  
	      if((dy>=0)&&(dy<nbiny)) bincounts[dy]++;

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
  WriteHeader1D(fMonitor, "polarization", bProbactiv, nbiny, sParN[ePar], sUnit[ePar]);
  for (dy = 0; dy<(nbiny); dy++)
  {
    if (bintch[dy]!=0.0 && bincounts[dy] > 0) 
    {
	    binerror[dy] = (bint[dy]/bintch[dy])*sqrt(1./bincounts[dy]);
    }
    fprintf(fMonitor, "%10.3f  %12.5e %12.5e  %7d\n", (bpost[dy]+bpost[dy+1])/2.0,(bint[dy]/bintch[dy]), binerror[dy], bincounts[dy]);
  }

  fclose(fMonitor);

  fprintf(LogFilePtr, "Binning  : %ld bins from %10.5f to %10.5f %s\n", nbiny, xMin, xMax, sUnit[ePar]);
  fprintf(LogFilePtr, "File     : %s\n", MonFileName);
  if(bintc != 0.) 
    fprintf(LogFilePtr,"average polarization: %3.5f \n", binpol/bintc);

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

	      case 'k':
	        ePar = atol(&argv[i][2]);       /* 1= monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz, 5=monitory, 6=monitorz */
	        break;

	      case 'n':
	        nbiny = atol(&argv[i][2]);      /* number of bins */
	        if (nbiny > 10000)
	          Error("number of bins must be <= 10000");
          break;

	      case 'm':
	        xMin = atof(&argv[i][2]);       /* lower bound lambda, time or div. window [A], [ms], [deg]*/
	        break;
	      case 'M':
	        xMax = atof(&argv[i][2]);       /* upper bound lambda, time or div. window [A], [ms], [deg]*/
	        break;

	      case 'p':
	        bProbactiv = atof(&argv[i][2]);	/* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;
	      case 'e':
	        if(argv[i][2]=='1')
	          bExclusive = 1;               /* if activated, only neutrons meeting the monitor conditions are considered further on */
	        break;

    	  default:
	        fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	        exit(-1);
	    }
    }
  }

  if (MonFileName==NULL)
  { Error("you must define a MonitorOutputFile"); 
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
