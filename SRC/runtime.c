/*********************************************************************************************/
/* VITESS module runtime                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Oct 2009  A. Houben      initial version                                             */
/* 1.1  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "general.h"
#include "init.h"
#include "softabort.h"


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);  // Reads input parameters and sets global variables
void OwnCleanup();                     // Does module specific cleanup


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int i=0;
  time_t start = time(NULL);
  time_t end = 0;
  double seconds = 0.;

  // reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_RUNTIME;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.1");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  /* Get the neutrons from the file */
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
  int i=0;

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
