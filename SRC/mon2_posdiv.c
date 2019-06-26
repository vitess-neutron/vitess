/********************************************************************************************/
/*  VITESS module 'mon2_posdiv.c'                                                           */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#define BINSIZE 201

  static double bdiv_[BINSIZE],bpos_[BINSIZE];
  static double bin_posdiv[BINSIZE][BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int		index_yz , dpos,ddiv;
  long	i, exclusivecount, registered, BufferIndex, nbin_pos, nbin_div ;
  double pos_, div_, pos_min, pos_max, div_min, div_max,p, probactiv, bintc;


  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;
  index_yz=1;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_posdiv 1.2");


  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {

	  case 'q':
	    index_yz = atol(&argv[i][2]); /*  y or z direction */
	    break;

	case 'O':
	    if((fmonitor = fopen(&argv[i][2],"w"))==NULL)
	      {
		fprintf(LogFilePtr,"\nFile %s could not be opened for monitoroutput\n",&argv[i][2]);
		exit(-1);
	      }
	    MonitorFileName=&argv[i][2];
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
	    probactiv = atof(&argv[i][2]);
	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;

	  case 'e':
	    if(argv[i][2]=='1')
	      exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	    break;

/*	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;*/
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
  for(dpos = 0; dpos<nbin_pos+1; dpos++)
    {
      bpos_[dpos] = pos_min + (pos_max-pos_min) * dpos / (double)nbin_pos;

      for(ddiv = 0;ddiv<(nbin_div+1); ddiv++)
	{
	  bdiv_[ddiv] = div_min + (div_max-div_min)  * ddiv / (double) nbin_div;
	  bin_posdiv[dpos][ddiv] = 0.0;
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

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	    pos_ = InputNeutrons[i].Position[index_yz];

	    div_ = (double)atan2(InputNeutrons[i].Vector[index_yz],InputNeutrons[i].Vector[0]);
	    div_*=180.0/M_PI;
	    if ((InputNeutrons[i].Vector[index_yz]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	      {div_=0.0;}


	  dpos = (int)floor(nbin_pos*(pos_-pos_min)/(pos_max-pos_min));
	  ddiv = (int)floor(nbin_div*(div_-div_min)/(div_max-div_min));
			
	  if(((dpos>=0)&&(dpos<nbin_pos))&&((ddiv>=0)&&(ddiv<nbin_div)))
	    {	
	      bin_posdiv[dpos][ddiv] = bin_posdiv[dpos][ddiv] +  p ;
	      bintc = bintc + p;
	      registered=1;
	    }
	  
	  if((exclusivecount==0)||(registered==1))
	    {
	      WriteNeutron(&(InputNeutrons[i]));
	    }
	}
    }
my_exit:

  for(dpos = 0; dpos<nbin_pos; dpos++)
    {
      fprintf(fmonitor,"%10.7f\t",(bpos_[dpos]+bpos_[dpos+1])/2.0);
    }
  for(ddiv = 0; ddiv<nbin_div; ddiv++)
    {
      fprintf(fmonitor,"\n %5.3f\t",(bdiv_[ddiv]+bdiv_[ddiv+1])/2.0);
      for(dpos = 0; dpos<nbin_pos; dpos++)
	{
	  fprintf(fmonitor,"%5.3E\t",bin_posdiv[dpos][ddiv]);
	}
    }
  fclose(fmonitor);


  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
