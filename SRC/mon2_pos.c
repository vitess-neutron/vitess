/********************************************************************************************/
/*  VITESS module monitor2                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Sep 1999  D. Wechsler                                                               */
/* 1.1  JUL 2002  G. Zsigmond    reorganized                                                */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added wavelength filter                                    */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#define BINSIZE 201

  static double bposz[BINSIZE],bposy[BINSIZE];
  static double binyz[BINSIZE][BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy,dz;
  long	i, exclusivecount, registered, BufferIndex, nbiny, nbinz ;
  double widthmin, widthmax, heightmin, heightmax,p, probactiv, bintc;
  double filtLambdaMin=-1.0,          /* filter      */
		 filtLambdaMax=-1.0;

  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_pos 1.2a");


  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {
	  case 'O':
	    if((fmonitor = fopen(&argv[i][2],"w"))==NULL)
	      {
		fprintf(LogFilePtr,"\nERROR: File %s could not be opened for monitoroutput\n",&argv[i][2]);
		exit(-1);
	      }
	    MonitorFileName=&argv[i][2];
	    break;

	  case 'y':
	    nbiny = atol(&argv[i][2]); /* number of bins y-direction */
	    if(nbiny>BINSIZE)
	      {fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINSIZE); exit(99);}
	    break;

	  case 'z':
	    nbinz = atol(&argv[i][2]); /* number of bins, z-direction */
	    if(nbinz>BINSIZE)
	      {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= %d", BINSIZE); exit(99);}
	    break;

	  case 'h':
	    heightmin =  atof(&argv[i][2]);   /* bottom position window   [cm]*/
	    break;
	  case 'w':
	    widthmin = atof(&argv[i][2]);		/* left edge position window   [cm]*/
	    break;

	  case 'H':
	    heightmax =  atof(&argv[i][2]);   /* top position window   [cm]*/
	    break;
	  case 'W':
	    widthmax = atof(&argv[i][2]);		/* right edge position window    [cm]*/
	    break;

	  case 'p':
	    probactiv = atof(&argv[i][2]);
	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;

	  case 'e':
	    if(argv[i][2]=='1')
	      exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	    break;
	  
	  case 'l':
        filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;

      case 'L':
        filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;
	  }
      }
    }

  if (MonitorFileName==NULL)
    {
      fprintf(LogFilePtr,"\nERROR:  you must define a MonitorOutputFile");
      exit(99);
    }


  /*initialisation */

  bintc = 0;
  for(dy = 0; dy<nbiny+1; dy++)
    {
      bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

      for(dz = 0;dz<(nbinz+1); dz++)
	{
	  bposz[dz] = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
	  binyz[dy][dz] = 0.0;
	}
    }

  /*************************************************************/
DECLARE_ABORT;
  while(ReadNeutrons()!= 0)
  {
	CHECK;      
	for(i=0; i<NumNeutGot; i++)
	{
	  CHECK;
	  registered=0;

	  if(exclusivecount==0) {
		  WriteNeutron(&(InputNeutrons[i]));
	  }

	  if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	  if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	  dy = (int)floor(nbiny*(InputNeutrons[i].Position[1]-widthmin)/(widthmax-widthmin));
	  dz = (int)floor(nbinz*(InputNeutrons[i].Position[2]-heightmin)/(heightmax-heightmin));
			
	  if(((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	  {	
	      binyz[dy][dz] = binyz[dy][dz] +  p ;
	      bintc = bintc + p;
	      registered=1;
	  }
	  
	  if((exclusivecount==1) && (registered==1)) {
	      WriteNeutron(&(InputNeutrons[i]));
	  }
	}
  }

my_exit:
  for(dy = 0; dy<nbiny; dy++)
    {
      fprintf(fmonitor,"%10.7f\t",(bposy[dy]+bposy[dy+1])/2.0);
    }
  for(dz = 0; dz<nbinz; dz++)
    {
      fprintf(fmonitor,"\n %5.3f\t",(bposz[dz]+bposz[dz+1])/2.0);
      for(dy = 0; dy<nbiny; dy++)
	{
	  fprintf(fmonitor,"%5.3E\t",binyz[dy][dz]);
	}
    }
  fclose(fmonitor);


  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
