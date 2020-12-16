/********************************************************************************************/
/*  VITESS module 'monitor2D.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Oct 2011  D. Nekrassov   initial version                                            */
/* 1.1  Feb 2020  K. Lieutenant  new central visualization parameters                       */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern "C" {
#include "init.h"
#include "softabort.h"
#include "general.h"
}

#include "mon2D.h"


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long	i;
  short bExclusive, bRegistered;
  
  bExclusive = FALSE;
  bRegistered= FALSE;

  // This is the class for a generic 2D monitor. It handles 11 parameter at the moment
  Mon2D templateMonitor;

  // initialisation
  // --------------
	Init(argc, argv, templateMonitor.eModule);
  PrintModuleName(templateMonitor.eModule, "1.1");
  templateMonitor.OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  /* bExclusive = 1: Only neutrons are considered further, 
     which end up in the monitor without being rejected */
  bExclusive = templateMonitor.exclCounts;

  /*************************************************************/
  DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
	  {
      CHECK;
      // bRegistered = 1 if neutron was considered in the monitor
	    bRegistered=templateMonitor.FillMonitor(&InputNeutrons[i]);
	  
	    if ((bExclusive==0) || (bRegistered==1)) 
	      WriteNeutron(&(InputNeutrons[i]));
    }
  }

my_exit:
  // Stores the information in the output file, closes the file and releases memory.
  templateMonitor.WriteOut();
  templateMonitor.FreeMemory();

  // releases memmory and writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
