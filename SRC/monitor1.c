/********************************************************************************************/
/*  VITESS module 'monitor1.c'                                                              */
/*    monitoring of intensity as a function of 1 parameter                                  */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* 1.0 S. Schorr         1997                                                               */
/* 1.1 G. Zsigmond   JUL 2002 extended                                                      */
/* 1.2 G. Zsigmond   MAR 2003 error bars output and option for normalisation with binsize   */
/*                            included                                                      */
/* 1.3 K. Lieutenant NOV 2003 monitoring dependent on colour                                */
/* 1.4 K. Lieutenant JAN 2004 changes for 'instrument.dat'                                  */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "init.h"
#include "softabort.h"
#include "general.h"

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	dy;
  long	i, kind, exclusivecount, registered, normalise, BufferIndex, nbiny, binn[10001], 
         nColour=0 ;
  double bpost[10001],  bint[10001], SD[10001], m,p,M,probactiv, norm;
  double Divy, Divz, dEvalTimeMin=-1.0e10, dEvalTimeMax=1.0e10, time;




  BufferIndex = 0;
  kind = 1;
  probactiv=1.0;
  normalise = 0;
  p=0.0;
  exclusivecount=0;
  registered=0;


  /*input*/
  Init(argc, argv, VT_MONITOR_1);

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

	  case 'k':

	    kind = atol(&argv[i][2]); /* 1= monitorlambda; 2=monitortime; 3=monitordivy, 4=monitordivz, 5=monitory, 6=monitorz */
	    break;


	  case 'n':
	    nbiny = atol(&argv[i][2]); /* number of bins */

	    if(nbiny>10000)
	      {fprintf(LogFilePtr,"\n number of bins must be <= 10000"); exit(99);}
	    break;

	  case 'm':

	    m = atof(&argv[i][2]);   /* lower bound lambda, time or div. window [A], [ms], [deg]*/
	    break;

	  case 'M':

	    M = atof(&argv[i][2]);   /* upper bound lambda, time or div. window [A], [ms], [deg]*/
	    break;

	  case 'p':

	    probactiv = atof(&argv[i][2]);

	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;


	  case 'e':
	    if(argv[i][2]=='1') exclusivecount = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	    break;

	  case 'f':
	    if(argv[i][2]=='1') normalise = 1; /* if activated, each channel normalised with bin size */
	    break;

	  case 'C':
	    nColour = atol(&argv[i][2]);       /*  excludes all neutrons with diff. Colour, if nColour > 0 */
	    break;


	  case 't':
	    dEvalTimeMin = atof(&argv[i][2]);   /* minimal time for monitoring */
	    break;

	  case 'T':
	    dEvalTimeMax = atof(&argv[i][2]);   /* maximal time for monitoring */
	    break;


	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;
	  }
      }
    }



  if (MonitorFileName==NULL)
    {fprintf(LogFilePtr,"\n you must define a MonitorOutputFile"); exit(99);}


  print_module_name("monitor1 1.4");
  
  /*initialisation*/
  for (dy=0;dy<nbiny+1;dy++)
    {
      bpost[dy]=m+((M-m)*dy/(double)nbiny);
      bint[dy]=0.0;
      binn[dy]=0;
    }
         
	if(normalise==1) norm =(M-m)/(double)nbiny; else norm = 1; fprintf(LogFilePtr,"Norm:  %f\n", norm);

DECLARE_ABORT;
  while (ReadNeutrons()!= 0)
    {
CHECK;      for(i=0; i<NumNeutGot; i++)
	{
CHECK;	  registered=0;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;
	  
	  time = InputNeutrons[i].Time;

	  /* write out all neutrons, if 'exclusive counts = no' is set */
	  if (exclusivecount==0)		
	  {
	    WriteNeutron(&(InputNeutrons[i]));
	  }

	  /* exclude traj. with wrong colours: (nColour=0 means: all colours accepted) */
	  if (nColour!=0 && nColour!=InputNeutrons[i].Color) continue;

	  switch (kind) {
	  case 1:
	    dy=(int)floor((double)nbiny*(InputNeutrons[i].Wavelength - m)/(M-m));
	    
	    if(dy>=0 && dy<nbiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
	    {
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
	    break;
	    
	  case 2:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Time - m)/(M-m));	      
	    if((dy>=0)&&(dy<nbiny))
	    {
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
	    break;
	    
	  case 3:
	    Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
	    Divy*=180.0/M_PI;
	    if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	      {Divy=0.0;}
	    
	    dy = (int)floor(nbiny*(Divy - m)/(M-m));	      
	    if(dy>=0 && dy<nbiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
	    {
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
	    break;
	    
	  case 4:
	    Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
	    Divz*=180.0/M_PI;
	    if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	      {Divy=0.0;}
	    
	    dy = (int)floor(nbiny*(Divz - m)/(M-m));
	    
	    if(dy>=0 && dy<nbiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
	    { 
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
		 break;

	  case 5:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Position[1] - m)/(M-m));	      
	    if(dy>=0 && dy<nbiny  && time>=dEvalTimeMin && time<=dEvalTimeMax)
	    {
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
	    break;
	    
	  case 6:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Position[2] - m)/(M-m));	      
	    if(dy>=0 && dy<nbiny && time>=dEvalTimeMin && time<=dEvalTimeMax)
	    {
	       bint[dy] = bint[dy] + p;
	       binn[dy] = binn[dy] + 1;
	       registered=1;
	    }
	    break;
	  }
	    
	  
	  /* write out registered neutrons, if 'exclusive counts = yes' is set */
	  if((exclusivecount==1)&&(registered==1))
	    {
	      WriteNeutron(&(InputNeutrons[i]));
	    }
	}
    }
my_exit:
  for (dy = 0; dy<(nbiny); dy++)
    {
      if(binn[dy]==0) binn[dy]=1;
	  SD[dy] = bint[dy]*sqrt(1./(double)binn[dy]);
	  fprintf(fmonitor,"%  7.7E\t% 11.7E \t% 11.7E \n",(bpost[dy]+bpost[dy+1])/2.0,(bint[dy]/norm), SD[dy]/norm);
    }

  fclose(fmonitor);

  stPicture.eType = (short) kind;
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
