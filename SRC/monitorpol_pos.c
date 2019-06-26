/********************************************************************************************/
/*  VITESS module 'monitorpol_pos.c'                                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
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

  static double bposz[BINSIZE],bposy[BINSIZE];
  static double binyz[BINSIZE][BINSIZE], binyzpol[BINSIZE][BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy,dz;
  long	i, exclusivecount, registered;
  long	BufferIndex;
  long	nbiny, nbinz;
  double RotMatrixAnalysis[3][3], bintc, bintcpol, analysis_dir[3];
  double widthmin, widthmax, heightmin, heightmax,p, probactiv;

  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;

  nbiny=nbinz=0;
  widthmin=widthmax=heightmin=heightmax=0;

  /*input*/
  Init(argc, argv, VT_MON_POL_POS);
  print_module_name("monitorpol_pos 1.1");


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
	    nbiny = atol(&argv[i][2]); /* number of bins y-direction */
	    if(nbiny>1000)
	      {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= 1000"); exit(99);}
	    break;

	  case 'z':
	    nbinz = atol(&argv[i][2]); /* number of bins, z-direction */
	    if(nbinz>1000)
	      {fprintf(LogFilePtr,"\nERROR:  number of bins must be <= 1000"); exit(99);}
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

	  }
      }
    }

  if (MonitorFileName==NULL)
    {
      fprintf(LogFilePtr,"\nERROR: you must define a MonitorOutputFile");
      exit(99);
    }

	{ double roty, rotz ;

	CartesianToEulerZY(analysis_dir, &roty, &rotz) ; 

	FillRotMatrixZY(RotMatrixAnalysis, roty, rotz) ; 	}



  /*initialisation */

  bintc = 0;
  bintcpol = 0;
  for(dy = 0; dy<nbiny+1; dy++)
    {
      bposy[dy] = widthmin + (widthmax-widthmin) * dy / (double)nbiny;

      for(dz = 0;dz<(nbinz+1); dz++)
	{
	  bposz[dz] = heightmin + (heightmax-heightmin)  * dz / (double) nbinz;
	  binyz[dy][dz] = 0.0;
	  binyzpol[dy][dz] = 0.0;
	}
    }

  /*************************************************************/
DECLARE_ABORT;
  while(ReadNeutrons()!= 0)
    {
CHECK;      for(i=0; i<NumNeutGot; i++)
	{
CHECK;	  registered=0;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	/* calculate spin vector in the direction of the analysis */

	RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

	  dy = (int)floor(nbiny*(InputNeutrons[i].Position[1]-widthmin)/(widthmax-widthmin));
	  dz = (int)floor(nbinz*(InputNeutrons[i].Position[2]-heightmin)/(heightmax-heightmin));
			
	  if(((dy>=0)&&(dy<nbiny))&&((dz>=0)&&(dz<nbinz)))
	    {	
	      binyzpol[dy][dz] = binyzpol[dy][dz] +  p * InputNeutrons[i].Spin[0];
	      bintcpol = bintcpol + p * InputNeutrons[i].Spin[0];
	      binyz[dy][dz] = binyz[dy][dz] +  p;
	      bintc = bintc + p;
	      registered=1;
	    }
	  
	  
	/* calculate spin vector in the original direction */

	RotBackVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

	  if((exclusivecount==0)||(registered==1))
	    {
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
	  if(binyz[dy][dz] == 0.)fprintf(fmonitor,"%5.3E\t", 0.);
	  else fprintf(fmonitor,"%5.3E\t",binyzpol[dy][dz]/binyz[dy][dz]);
	}
    }
  fclose(fmonitor);

  fprintf(LogFilePtr," \n");
  if(bintc != 0.) fprintf(LogFilePtr,"polarization: %3.5f \n",bintcpol/bintc);

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
