/********************************************************************************************/
/*  VITESS module 'mon2_posdiv.c'                                                           */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2a JAN 2010  A. Houben      Added wavelength and yz position filter                    */
/* 1.2b JAN 2010  A. Houben      xyz output                                                 */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "init.h"
#include "softabort.h"
#include "general.h"

#include "mon2_header.h"

static double bdiv_[BINSIZE],bpos_[BINSIZE];

int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int		index_yz , dpos,ddiv, probactiv;
  long	i, exclusivecount, registered, BufferIndex, nbin_pos=0, nbin_div=0 ;
  double pos_, div_, pos_min, pos_max, div_min, div_max,p,  bintc;
  double filtLambdaMin=-1.0,          /* filter      */
		 filtLambdaMax=-1.0,
		 filtYMin=-1.0e10,
         filtYMax=1.0e10,
		 filtZMin=-1.0e10,
		 filtZMax=1.0e10;
  long format = 0;
  pos_min = pos_max = div_min = div_max = 0;

  BufferIndex = 0;
  p=0.0;
  probactiv=1.0;
  exclusivecount=0;
  registered=0;
  index_yz=1;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("mon2_posdiv 1.2a");


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
      fprintf(LogFilePtr,"\n you must define a MonitorOutputFile");
      exit(99);
    }


  /*initialisation */
  if (probactiv != 1) probactiv = 0;

  bintc = 0;

   //New pointers allowing for global write out
  by = bpos_;
  bz = bdiv_;

  for(dpos = 0; dpos<nbin_pos+1; dpos++)
    {
      bpos_[dpos] = pos_min + (pos_max-pos_min) * dpos / (double)nbin_pos;

      for(ddiv = 0;ddiv<(nbin_div+1); ddiv++)
	{
	  bdiv_[ddiv] = div_min + (div_max-div_min)  * ddiv / (double) nbin_div;
	  binyz[dpos][ddiv] = 0.0;
	  binyzerror[dpos][ddiv]=0.;
	  binyzcounts[dpos][ddiv]=0;
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

	  pos_ = InputNeutrons[i].Position[index_yz];

	  if (index_yz == 1) {
	    if (InputNeutrons[i].Vector[0] >=0) div_ = (double) atan2(InputNeutrons[i].Vector[1], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));
	    else div_ = (double) atan2(InputNeutrons[i].Vector[1], -sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[2])));	  
	  div_*=180.0/M_PI;
	  }
	  else {
	    div_ = (double) atan2(InputNeutrons[i].Vector[2], sqrt(sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1])));	 
	    div_*=180.0/M_PI;
	  }
	  dpos = (int)floor(nbin_pos*(pos_-pos_min)/(pos_max-pos_min));
	  ddiv = (int)floor(nbin_div*(div_-div_min)/(div_max-div_min));
			
	  if(((dpos>=0)&&(dpos<nbin_pos))&&((ddiv>=0)&&(ddiv<nbin_div))) {	
	    binyz[dpos][ddiv] = binyz[dpos][ddiv] +  p ;
	    bintc = bintc + p;
	    registered=1;
	    binyzcounts[dpos][ddiv]++;
	  }
	  
	  if((exclusivecount==1) && (registered==1)) {
	    WriteNeutron(&(InputNeutrons[i]));
	  }
	}
  }
my_exit:
 
  WriteOutput (fmonitor, format, probactiv, nbin_pos, nbin_div);

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
