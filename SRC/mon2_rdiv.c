/********************************************************************************************/
/*  VITESS module 'mon2_rdiv.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  JAN 2010  A. Houben (idea by W. Schweika)                                           */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"

#define BINSIZE 201

  static double bphi[BINSIZE],bradius[BINSIZE];
  static double binyz[BINSIZE][BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy,dz;
  long	i, exclusivecount, registered, BufferIndex, nbiny, nbinz ;
  double radius, phi;
  VectorType xvec = {1, 0, 0}, kvec;
  double rmin, rmax, phimin, phimax,p, probactiv, bintc;
  double filtLambdaMin=-1.0,          /* filter      */
		 filtLambdaMax=-1.0,
		 filtYMin=-1.0e10,
         filtYMax=1.0e10,
		 filtZMin=-1.0e10,
		 filtZMax=1.0e10;


  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_rdiv 1.0");


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

	  case 'h':
	    phimin =  atof(&argv[i][2]);   /* bottom position window */
	    break;
	  case 'w':
	    rmin = atof(&argv[i][2]);		/* left edge position window */
	    break;

	  case 'H':
	    phimax =  atof(&argv[i][2]);   /* top position window */
	    break;
	  case 'W':
	    rmax = atof(&argv[i][2]);		/* right edge position window */
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

  bintc = 0;
  for(dy = 0; dy<nbiny+1; dy++)
    {
      bradius[dy] = rmin + (rmax-rmin) * dy / (double)nbiny;

      for(dz = 0;dz<(nbinz+1); dz++)
	{
	  bphi[dz] = phimin + (phimax-phimin)  * dz / (double) nbinz;
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
	  if (InputNeutrons[i].Position[1] < filtYMin) continue;
	  if (InputNeutrons[i].Position[1] > filtYMax) continue;
	  if (InputNeutrons[i].Position[2] < filtZMin) continue;
	  if (InputNeutrons[i].Position[2] > filtZMax) continue;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	  radius = sqrt(sq(InputNeutrons[i].Position[1])+sq(InputNeutrons[i].Position[2]));
	  CopyVector(InputNeutrons[i].Vector, kvec);
	  NormVector(kvec);
	  phi = acos(ScalarProduct(xvec, kvec))/M_PI*180.;

	  dy = (int)floor(nbiny*(radius-rmin)/(rmax-rmin));
	  dz = (int)floor(nbinz*(phi-phimin)/(phimax-phimin));
			
	  if(((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz))) {	
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
      fprintf(fmonitor,"%10.7f\t",(bradius[dy]+bradius[dy+1])/2.0);
    }
  for(dz = 0; dz<nbinz; dz++)
    {
      fprintf(fmonitor,"\n %5.3f\t",(bphi[dz]+bphi[dz+1])/2.0);
      for(dy = 0; dy<nbiny; dy++)
	{
	  fprintf(fmonitor,"%5.3E\t",binyz[dy][dz]);
	}
    }
  fclose(fmonitor);


  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
