/********************************************************************************************/
/*  VITESS module 'monitor1D.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Oct 2011  D. Nekrassov  initial version                                              */
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

#include "mon1D.h"

int main(int argc, char *argv[])
{
 
  long	i,exclusivecount, registered, BufferIndex;
  
  BufferIndex = 0;
  exclusivecount=0;
  registered=0;

  /*input*/
  Init(argc, argv, VT_MONITOR_2);
  print_module_name("monitor1D 1.0");

  /* This is the class for a generic 1D monitor. 
     It handles 11 parameter at the moment */
  Mon1D templateMonitor;
  templateMonitor.Init(argc, argv);

  /* exclusivecount = 1: Only neutrons are considered further, 
     which end up in the monitor without being rejected */
  exclusivecount = templateMonitor.exclCounts;

  /*************************************************************/
DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
  CHECK;
  for(i=0; i<NumNeutGot; i++)
	{
      CHECK;
      // registered = 1 if neutron was considered in the monitor
	  registered=templateMonitor.FillMonitor(&InputNeutrons[i]);
	  
	  if((exclusivecount==0) || (registered==1)) {
	      WriteNeutron(&(InputNeutrons[i]));
	  }
    }
  }
my_exit:

  // Stores the information in the output file.
  templateMonitor.WriteOut();

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}
