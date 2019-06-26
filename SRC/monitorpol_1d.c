/********************************************************************************************/
/*  VITESS module 'monitorpol_1D.c'                                                         */
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
#include "matrix.h"


int main(int argc, char *argv[])
{
  FILE	*fmonitor=NULL;
  char	*MonitorFileName=NULL;
  int	 dy;
  long	 i, kind, exclusivecount, registered, BufferIndex, nbiny ;
  double RotMatrixAnalysis[3][3], bpost[10001], bintc, binpol, analysis_dir[3];
  double Divy, Divz, bint[10001],bintch[10001],m,p,M,probactiv;

  BufferIndex = 0;
  kind = 1;
  probactiv=1.0;
  p=0.0;
  exclusivecount=0;
  registered=0;


  /*input*/
  Init(argc, argv, VT_MON_POL_1);

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
    {fprintf(LogFilePtr,"\n you must define a MonitorOutputFile"); exit(99);}

	{ double roty, rotz ;

	CartesianToEulerZY(analysis_dir, &roty, &rotz) ; 

	FillRotMatrixZY(RotMatrixAnalysis, roty, rotz) ; 	}


  print_module_name("monitorpol1 1.2");
  
  /*initialisation*/
  
  binpol=0.0;
  bintc=0.0;
  for (dy=0;dy<nbiny+1;dy++)
    {
      bpost[dy]=m+((M-m)*dy/(double)nbiny);
      bint[dy]=0.0;
      bintch[dy]=0.0;
    }
         
DECLARE_ABORT;
  while (ReadNeutrons()!= 0)
    {
CHECK;      for(i=0; i<NumNeutGot; i++)
	{
CHECK;	  registered=0;

	  if(probactiv==1.0) {p = InputNeutrons[i].Probability;}
	  else p=1.0;
	  
	/* calculate spin vector in the direction of the analysis */

	RotVector(RotMatrixAnalysis, InputNeutrons[i].Spin);

	  switch (kind) {
	  case 1:
	    dy=(int)floor((double)nbiny*(InputNeutrons[i].Wavelength - m)/(M-m));
	    
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0];
		bintc = bintc + p;
		registered=1;
	      }
	    break;
	    
	  case 2:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Time - m)/(M-m));	      
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0] ;
		bintc = bintc + p;
		registered=1;
	      }
	    break;
	    
	  case 3:
	    Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
	    Divy*=180.0/M_PI;
	    if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	      {Divy=0.0;}
	    
	    dy = (int)floor(nbiny*(Divy - m)/(M-m));	      
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0] ;
		bintc = bintc + p;
		registered=1;
	      }
	    break;
	    
	  case 4:
	    Divz=(double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
	    Divz*=180.0/M_PI;
	    if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
	      {Divy=0.0;}
	    
	    dy = (int)floor(nbiny*(Divz - m)/(M-m));
	    
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0] ;
		bintc = bintc + p;
		registered=1;
	      }
		break;

	  case 5:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Position[1] - m)/(M-m));	      
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0] ;
		bintc = bintc + p;
		registered=1;
	      }
	    break;
	    
	  case 6:
	    dy = (int)floor(nbiny*(InputNeutrons[i].Position[2] - m)/(M-m));	      
	    if((dy>=0)&&(dy<nbiny))
	      {
		bint[dy] = bint[dy] + p * InputNeutrons[i].Spin[0];
		bintch[dy] = bintch[dy] + p;
		binpol = binpol + p * InputNeutrons[i].Spin[0] ;
		bintc = bintc + p;
		registered=1;
	      }
	    break;
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
  for (dy = 0; dy<(nbiny); dy++)
    {
      if(bintch[dy]!=0.)fprintf(fmonitor,"%  7.7f\t% 11.7E \n",(bpost[dy]+bpost[dy+1])/2.0,(bint[dy]/bintch[dy]));
    }

  fclose(fmonitor);

  if(bintc != 0.) fprintf(LogFilePtr,"\npolarization: %3.5f \n", binpol/bintc);

  stPicture.eType = (short) kind;
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
