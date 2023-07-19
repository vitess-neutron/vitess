/********************************************************************************************/
/*  VITESS module 'monitor1D.c'                                                             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Oct 2011  D. Nekrassov   initial version                                            */
/* 1.1  Feb 2020  K. Lieutenant  new central visualization parameters                       */
/* 1.2  Mar 2021  K. Lieutenant  update after each bunch                                    */
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


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  long	i=0,
        iBnch=0;            // current bunch
  short bExclusive = FALSE, 
        bRegistered= FALSE;

  // This is the class for a generic 1D monitor. It handles 17 parameter at the moment
  Mon1D templateMonitor;

  // initialisation
  // --------------
  _eModule=templateMonitor.eModule;
	Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.2a");
  templateMonitor.OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  /* bExclusive = 1: Only neutrons are considered further, 
     which end up in the monitor without being rejected */
  bExclusive = templateMonitor.exclCounts;

  DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
    for (i=0; i<NumNeutGot; i++)
	  {
      CHECK;

      // Update monitor output if EOB line is found
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      { 
        iBnch++;
        templateMonitor.WriteOut(iBnch);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        // bRegistered = 1 if neutron was considered in the monitor
	      bRegistered=templateMonitor.FillMonitorArray(&InputNeutrons[i]);
	  
	      if ((bExclusive==0) || (bRegistered==1))
	        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

my_exit:
  // writes the information to the output file
  templateMonitor.WriteOut(templateMonitor.nBunches);

  // writes to instrument and log file and releases memmory
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  templateMonitor.FreeMemory();

  return(0);
}
