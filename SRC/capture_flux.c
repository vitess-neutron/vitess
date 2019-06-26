/*********************************************************************************************/
/* VITESS module caputure_flux                                                               */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Feb 2008            initial version                                                  */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


void OwnInit(int argc, char *argv[]);
void OwnCleanup();

double CaptArea=1.0;

int main(int argc, char **argv)
{
  int i, Ntot=0;
  double CaptInt=0.0, CaptQuad=0.0, CaptErr=0.0;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_MONITOR_1);
  print_module_name("capture_flux 1.0");

  /* module specific initialization */
  OwnInit(argc, argv);
 
  /* Get the neutrons from the file */
  DECLARE_ABORT;
  
  while((ReadNeutrons())!= 0)
  {
    CHECK;    
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

		CaptInt  +=    InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/1.798;
		CaptQuad += sq(InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/1.798);
      Ntot++;             

      WriteNeutron(&(InputNeutrons[i]));
    }
  }

  /* error for the given count rate calculated through adding squared errors
     - of the number  of contributing traj.: sqrt(N) (Poisson distribution)
     - of the average count rate of each trajectory I_s = I_tot/N:
       sqrt((<I_s²> - <I_s>²)/(N-1))
     as independent contributions */
  if (Ntot > 1)
    CaptErr = sqrt(sq(CaptInt)/Ntot + (Ntot*CaptQuad-sq(CaptInt))/(Ntot-1));
  else
    CaptErr = CaptInt;
 
  fprintf(LogFilePtr, "Capture flux: %12.3e +/- %12.3e n/(s*cm^2) \n", CaptInt/CaptArea, CaptErr);
  
  /* Do module specific cleanups */
 my_exit:
  OwnCleanup();
  
  /* Do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


void  OwnInit(int argc, char *argv[]) 
{
  int i;

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
	      /* area */
	    case 'A':
	      CaptArea= atof(&argv[i][2]);  
	      break;

       default:
         fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
         exit(-1);
         break;
      }
    }
  }
  return;
}


void OwnCleanup()
{
  return;
}
