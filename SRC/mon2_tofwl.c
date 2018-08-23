/********************************************************************************************/
/*  VITESS module 'mon2_tofwl.c'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      xyz output                                                 */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#include "mon2_header.h"

static double bposz[BINSIZE],bposy[BINSIZE];


int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy,dz;
  long	i, exclusivecount, registered, nbiny=0, nbinz=0;
  double widthmin, widthmax, heightmin, heightmax,p, probactiv, bintc;
  long format = 0;

  widthmin = widthmax = heightmin = heightmax = 0;

  /* vertical: lambda, horizontal: tof   */

  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;

  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_tofwl 1.2a");


  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {
	  case 'O':
	    if((fmonitor = fopen(&argv[i][2],"w"))==NULL)
	      {
		fprintf(LogFilePtr,"\nFile %s could not be opened for monitoroutput\n",&argv[i][2]);
		exit(-1);
	      }
	    MonitorFileName=&argv[i][2];
	    break;

	  case 'y':
	    nbiny = atol(&argv[i][2]); /* number of bins horizontal axis */
	    if(nbiny>BINSIZE)
	      {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	    break;

	  case 'z':
	    nbinz = atol(&argv[i][2]); /* number of bins vertical axis */
	    if(nbinz>BINSIZE)
	      {fprintf(LogFilePtr,"\n number of bins must be <= %d", BINSIZE); exit(99);}
	    break;

	  case 'm':
	    heightmin =  atof(&argv[i][2]);   /* minimal wavelength [A]*/
	    break;
	  case 'w':
	    widthmin = atof(&argv[i][2]);		/* minimal tof-value [ms]*/
	    break;

	  case 'M':
	    heightmax =  atof(&argv[i][2]);   /* maximal wavelength [A]*/
	    break;
	  case 'W':
	    widthmax = atof(&argv[i][2]);		/* maximal tof-value [ms]*/
	    break;

	  case 'p':
	    probactiv = atof(&argv[i][2]);
	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;

	  case 'e':
	    if(argv[i][2]=='1')
	      exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
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

  if (MonitorFileName==NULL)
    {
      fprintf(LogFilePtr,"\n you must define a MonitorOutputFile");
      exit(99);
    }


  /*initialisation */

  if (probactiv != 1) probactiv = 0;

  bintc = 0;

   //New pointers allowing for global write out
  by = bposy;
  bz = bposz;

  for(dy = 0; dy<nbiny+1; dy++)
    {
      bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

      for(dz = 0;dz<(nbinz+1); dz++)
	{
	  bposz[dz] = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
	  binyz[dy][dz] = 0.0;
	  binyzerror[dy][dz]=0.;
	  binyzcounts[dy][dz]=0;
	}
    }

  /*************************************************************/
DECLARE_ABORT;
  while(ReadNeutrons()!= 0)
    {
CHECK;      for(i=0; i<NumNeutGot; i++)
	{
	  registered=0;
CHECK;
	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	  dy = (int)floor(nbiny*(InputNeutrons[i].Time-widthmin)/(widthmax-widthmin));
	  dz = (int)floor(nbinz*(InputNeutrons[i].Wavelength-heightmin)/(heightmax-heightmin));
			
	  if(((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	    {	
	      binyz[dy][dz] = binyz[dy][dz] +  p ;
	      bintc = bintc + p;
	      registered=1;
	      binyzcounts[dy][dz]++;
	    }
	  
	  if((exclusivecount==0)||(registered==1))
	    {
	      WriteNeutron(&(InputNeutrons[i]));
	    }
	}
    }

my_exit:
 
  WriteOutput (fmonitor, format, probactiv, nbiny, nbinz,
               "tof [ms]", "wavelength [A]");

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
