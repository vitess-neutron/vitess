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
  
  /*input*/
  Init(argc, argv, VT_MONOC_ANALY);
  print_module_name("monochromator 2.0");

  /* This is the class for a generic 2D monitor. 
     It handles 11 parameter at the moment */
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

