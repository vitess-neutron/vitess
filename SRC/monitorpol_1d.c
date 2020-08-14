/********************************************************************************************/
/*  VITESS module 'monitorpol_1D.c'                                                         */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
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


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_MON1_POL;

FILE*  fMonitor   = NULL;
char*  MonFileName= NULL;
short  bProbactiv = TRUE,
       bExclusive = FALSE,
       kind       = 1; 
long   nbiny      = 0;
double RotMatrixAnalysis[3][3], 
       analysis_dir[3],
       xMin = 0.0,
       xMax = 0.0;
       

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
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
  
  // initializes arrays
  for (dy=0;dy<nbiny+1;dy++)
  {
    bpost[dy]=xMin+((xMax-xMin)*dy/(double)nbiny);
    bint[dy]=0.0;
    bintch[dy]=0.0;
    binerror[dy]=0.;
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
      bRegistered=0;

	    if (bProbactiv==TRUE) 
        prob = InputNeutrons[i].Probability;
	    else 
        prob=1.0;
	  
	    /* calculate spin vector in the direction of the analysis */
	    RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

	    switch (kind) 
      {
	      case 1:
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
	    
	      case 2:
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
	    
	      case 3:
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
	    
	      case 4:
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

	      case 5:
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
	    
	      case 6:
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
	    }
	  
	    if((dy>=0)&&(dy<nbiny)) bincounts[dy]++;

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
  fprintf(fMonitor,"#Monitor %s\n", weightTag[bProbactiv]);
  for (dy = 0; dy<(nbiny); dy++)
  {
    if(bintch[dy]!=0.) 
    {
	    binerror[dy] = (bint[dy]/bintch[dy])*sqrt(1./bincounts[dy]);
	    fprintf(fMonitor, "%7.7f\t%5.3E\t%5.3E\t%d\n", (bpost[dy]+bpost[dy+1])/2.0,(bint[dy]/bintch[dy]), binerror[dy], bincounts[dy]);
    }
  }

  fclose(fMonitor);

  if(bintc != 0.) 
    fprintf(LogFilePtr,"\npolarization: %3.5f \n", binpol/bintc);

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
	        kind = atol(&argv[i][2]);       /* 1= monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz, 5=monitory, 6=monitorz */
	        break;


	      case 'n':
	        nbiny = atol(&argv[i][2]);      /* number of bins */
	        if (nbiny > 10000)
	          {fprintf(LogFilePtr,"\n number of bins must be <= 10000"); exit(99);}
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
	        break;
	    }
    }
  }

  if (MonFileName==NULL)
  { fprintf(LogFilePtr,"\n you must define a MonitorOutputFile"); 
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
