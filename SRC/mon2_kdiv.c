/********************************************************************************************/
/*  VITESS module 'mon2_kdiv.c'                                                              */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/*                                                                                          */
/* 1.0  Feb 2006  K. Lieutenant                                                             */
/* 1.0a JAN 2010  A. Houben      xyz output                                                 */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "general.h"

#include "mon2_header.h"


static double bdivz[BINSIZE],bdivy[BINSIZE];
  

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy,dz;
  long	i, exclusivecount, registered, nbiny=20, nbinz=20;
  double Divy, Divz, DivKy, DivKz, 
         DivYmin=0.0, DivYmax=0.0, DivZmin=0.0, DivZmax=0.0,
         p, probactiv, bintc;
  long format = 0;

  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;

  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_kdiv 1.0a");


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
	    DivZmin =  atof(&argv[i][2]);   /* bottom position window */
	    break;
	  case 'w':
	    DivYmin = atof(&argv[i][2]);		/* left edge position window */
	    break;

	  case 'H':
	    DivZmax =  atof(&argv[i][2]);   /* top position window */
	    break;
	  case 'W':
	    DivYmax = atof(&argv[i][2]);		/* right edge position window */
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
  by = bdivy;
  bz = bdivz;

  for(dy = 0; dy<nbiny+1; dy++)
    {
      bdivy[dy] = DivYmin + (DivYmax-DivYmin) * dy / (double)nbiny;

      for(dz = 0;dz<(nbinz+1); dz++)
	{
	  bdivz[dz] = DivZmin + (DivZmax-DivZmin)  * dz / (double) nbinz;
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
CHECK;	  registered=0;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	  if (InputNeutrons[i].Vector[0] >=0) Divy = (double) atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	  else Divy = (double) atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	 

	   Divz = (double) atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));  
	   
          DivKy = Divy * 2.0 * M_PI / InputNeutrons[i].Wavelength;
	  DivKz = Divz * 2.0 * M_PI / InputNeutrons[i].Wavelength;

	  dy = (int)floor(nbiny*(DivKy-DivYmin)/(DivYmax-DivYmin));
	  dz = (int)floor(nbinz*(DivKz-DivZmin)/(DivZmax-DivZmin));
			
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
               "kY [1/Ang]", "kZ [1/Ang]");

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
