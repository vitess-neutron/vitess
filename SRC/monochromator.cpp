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
  
  // input
  Init(argc, argv, MCN_MONOCHROM);
  print_module_name("monochromator 2.1");

  // This is the class for a monochromator
  Monochromator monochrom;
  monochrom.Init(argc, argv);
  
  /*************************************************************/
  DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
  CHECK;

  for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      monochrom.processNeutron(&(InputNeutrons[i]));
    }
  }
 my_exit:
  
  // OwnCleanup includes the general Cleanup()
  monochrom.OwnCleanup();
  
  return(0);
}

