/*********************************************************************************************/
/* VITESS module runtime                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00  Oct 2009  A. Houben      initial version                                            */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "general.h"
#include "init.h"
#include "softabort.h"

void OwnInit(int argc, char *argv[]);
void OwnCleanup();


int main(int argc, char **argv)
{
  int i;
  time_t start = time(NULL);
  time_t end = 0;
  double seconds = 0.;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_RUNTIME);
  print_module_name("runtime 1.00");

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

      WriteNeutron(&(InputNeutrons[i]));
    }
  }
  
  /* Do module specific cleanups */
 my_exit:
  OwnCleanup();

  end = time(NULL);
  seconds = ((double)end - (double)start);
  fprintf(LogFilePtr, 
         "Seconds   : %12.1f\n"
		 "Minutes   : %12.3f\n"
		 "Hours     : %12.3f\n"
		 "Days      : %12.3f\n\n", 
		 seconds, seconds/60., seconds/3600., seconds/86400.);
  
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
		/*case 'A':
			CaptArea = atof(&argv[i][2]);  
			break;*/
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

