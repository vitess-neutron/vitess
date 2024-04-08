/*********************************************************************************************/
/*  VITESS module monochromator                                                              */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0      2000  G. Zsigmond     initial version                                            */
/* ...                                                                                       */
/* 1.8  Jul 2002  G. Zsigmond                                                                */
/* 1.9  Aug 2012  K. Lieutenant   visualization included                                     */  
/* 2.0  Oct 2013  D. Nekrassov    written as a C++ class, included algorithms to determine   */  
/*                                normalisation without an extra normalisation run           */
/*                                visualisation improved                                     */
/*                                Allow for several monochromators after each other          */
/* 2.1  Jan 2020  K. Lieutenant   tidy up, transmission geom. corrected, attenuation improved*/  
/* 2.2  Jan 2020  K. Lieutenant   option: rotating monochromator                             */  
/* 2.3  Apr 2020  K. Lieutenant   adaption to VITESS 4, e.g. file parameters to input param. */  
/* 2.4  Feb 2024  K. Lieutenant   PST option added                                           */  
/* 2.5  Feb 2024  K. Lieutenant   Doppler drive and random TOF options added, windows for PST*/
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "init.h"
#include "softabort.h"
#include "general.h"
}

#include "monochrclass.h"

int main(int argc, char *argv[])
{
  long	i = 0;

  // This is the class for a monochromator
  Monochromator monochrom;

  // initialisation
  // --------------
  _eModule=monochrom.eModule;
	Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "2.5");

  monochrom.OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = FALSE;

  /* Reads monochromator parameters and combines them with input parameters */
   monochrom.setMonochrPar();

  /* Determines the dependent parameters and writes out important parameters */
   monochrom.calcAndWritePar();
  
  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      { 
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        monochrom.processNeutron(&(InputNeutrons[i]));
      }
    }
  }
 my_exit:
  
  // Geometry and OwnCleanup, which includes the general Cleanup()
  monochrom.setGeometry("yellow");
  monochrom.OwnCleanup();
  
  return(0);
}

