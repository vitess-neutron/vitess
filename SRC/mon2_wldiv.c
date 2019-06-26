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
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#define BINSIZE 201

  static double bdiv_[BINSIZE],bwl_[BINSIZE];
  static double bin_wldiv[BINSIZE][BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	index_yz, index_c, dwl,ddiv;
  long	i, exclusivecount, registered, BufferIndex, nbin_wl, nbin_div;
  double wl_, div_, wl_min, wl_max, constrain_min, constrain_max, div_min, div_max,p, probactiv, bintc;
  double filtYMin=-1.0e10,
         filtYMax=1.0e10,
		 filtZMin=-1.0e10,
		 filtZMax=1.0e10;
  double div_other_direction;
  long format = 0;


  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;
  index_yz=1;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_wldiv 1.2b");


  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {

	  case 'q':
	    index_yz = atol(&argv[i][2]); /*  y or z direction */
	    if(index_yz == 1)fprintf(LogFilePtr,"\nhorizontal direction chosen\n");
		if(index_yz == 2)fprintf(LogFilePtr,"\nvertical direction chosen\n");
	    if((index_yz != 1)&&(index_yz != 2)){index_yz = 1 ; fprintf(LogFilePtr,"\nwarning: horizontal direction chosen!\n");}
		break;

	case 'O':
	    if((fmonitor = fopen(&argv[i][2],"w"))==NULL)
	      {
		fprintf(LogFilePtr,"\nERROR: File %s could not be opened for monitoroutput\n",&argv[i][2]);
		exit(-1);
	      }
	    MonitorFileName=&argv[i][2];
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
	    probactiv = atof(&argv[i][2]);
	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;

	  case 'e':
	    if(argv[i][2]=='1')
	      exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
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

/*	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;*/
	  }
      }
    }

  if (MonitorFileName==NULL)
    {
      fprintf(LogFilePtr,"\nERROR: you must define a MonitorOutputFile");
      exit(99);
    }


  /*initialisation */

  bintc = 0;
  for(dwl = 0; dwl<nbin_wl+1; dwl++)
    {
      bwl_[dwl] = wl_min + (wl_max-wl_min) * dwl / (double)nbin_wl;

      for(ddiv = 0;ddiv<(nbin_div+1); ddiv++)
	{
	  bdiv_[ddiv] = div_min + (div_max-div_min)  * ddiv / (double) nbin_div;
	  bin_wldiv[dwl][ddiv] = 0.0;
	}
    }

	if(index_yz == 1) index_c = 2 ;	
	if(index_yz == 2) index_c = 1 ;


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

	  if (InputNeutrons[i].Position[1] < filtYMin) continue;
	  if (InputNeutrons[i].Position[1] > filtYMax) continue;
	  if (InputNeutrons[i].Position[2] < filtZMin) continue;
	  if (InputNeutrons[i].Position[2] > filtZMax) continue;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;

	  /* selects only trajectories in the given interval: constrain_min, constrain_max for the other direction */
	  div_other_direction = 180.0/M_PI * (double)atan2(InputNeutrons[i].Vector[index_c],InputNeutrons[i].Vector[0]);
	  if((div_other_direction <= constrain_min)||(div_other_direction >= constrain_max)) continue;
      
      wl_ = InputNeutrons[i].Wavelength;

	  div_ = (double)atan2(InputNeutrons[i].Vector[index_yz],InputNeutrons[i].Vector[0]);
	  div_ *=180.0/M_PI;
	  if ((InputNeutrons[i].Vector[index_yz]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	    {div_=0.0;}

	  dwl = (int)floor(nbin_wl*(wl_-wl_min)/(wl_max-wl_min));
	  ddiv = (int)floor(nbin_div*(div_-div_min)/(div_max-div_min));
			
	  if(((dwl>=0)&&(dwl<nbin_wl))&&((ddiv>=0)&&(ddiv<nbin_div))) {	
	      bin_wldiv[dwl][ddiv] = bin_wldiv[dwl][ddiv] +  p;
	      bintc = bintc + p;
	      registered=1;
	  }

	  if((exclusivecount==1) && (registered==1)) {
	      WriteNeutron(&(InputNeutrons[i]));
	  }
	}
  }
my_exit:
  switch (format) {
	case 0:
	  for(dwl = 0; dwl<nbin_wl; dwl++)
		{
		  fprintf(fmonitor,"%10.7f\t",(bwl_[dwl]+bwl_[dwl+1])/2.0);
		}
	  for(ddiv = 0; ddiv<nbin_div; ddiv++)
		{
		  fprintf(fmonitor,"\n %5.3f\t",(bdiv_[ddiv]+bdiv_[ddiv+1])/2.0);
		  for(dwl = 0; dwl<nbin_wl; dwl++)
		{
		  fprintf(fmonitor,"%5.3E\t",bin_wldiv[dwl][ddiv]);
		}
		}
	  break;
    case 1:
	  fprintf(fmonitor, "#x  y  z\n");
	  for(ddiv = 0; ddiv<nbin_div; ddiv++) {
		   for(dwl = 0; dwl<nbin_wl; dwl++) {
			fprintf(fmonitor,"%10.7f  %10.7f  %5.3E\n", (bwl_[dwl]+bwl_[dwl+1])/2.0, (bdiv_[ddiv]+bdiv_[ddiv+1])/2.0, bin_wldiv[dwl][ddiv]);
		  }
		  fprintf(fmonitor, "\n");
	  }
	break;
  }
  fclose(fmonitor);


  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
