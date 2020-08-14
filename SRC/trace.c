/*********************************************************************************************/
/*  VITESS functions 'trace'                                                                 */
/*    Functions to check if a trajectory is found in a file (containing IDs for ray-tracing) */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* May  2019  K. Lieutenant  separated from 'source' to enable usage in 'read_in'            */
/*********************************************************************************************/

#include <ctype.h>
#include <string.h>

#include "init.h"


/* global variables */
char*     __pTraceFileName=NULL; // name of the file containing the trajectories to be traced or started
TotalID*  __pTrace=NULL;         // table of trajectory IDs for tracing
long      __nLinesTr=0;          // Number of lines in the trace file  
short     __eTraceMode=0;        // mode 0: no tracing 
                                 // mode 1: write trace files for traj. of interest
                                 // mode 2: simulation only with traj. of interest 

void   LoadTraceFile();
char   GetTraceState(TotalID stID);
double IdNumber(TotalID stID);


/* load list of trajectories that shall be traced */
/* ---------------------------------------------- */
void LoadTraceFile()
{
  char  sBuffer[CHAR_BUF_LENGTH]; 
  FILE* pTraceFile=NULL;

  /* If there is a trace file go and load the file */
  if (__pTraceFileName!=NULL) 
   {
      /* opens distribution file */
      if ((pTraceFile = OpenInputFile(__pTraceFileName, FALSE, "rt"))!=NULL) 
      {
        long i;

        /* reads number of lines, allocates memory and then reads distribution file */
        __nLinesTr = LinesInFile(pTraceFile);
        __pTrace   = (TotalID*) calloc(__nLinesTr, sizeof(TotalID));

        for(i=0; i < __nLinesTr; i++)
        {  
          ReadLine(pTraceFile, sBuffer, CHAR_BUF_LENGTH-1);
          sscanf  (sBuffer, "%c%c%lu", &__pTrace[i].IDGrp[0], &__pTrace[i].IDGrp[1], &__pTrace[i].IDNo);
        }

        /* closes trace file */
        fclose(pTraceFile) ;
      } 
      else 
      { fprintf(LogFilePtr, "\nERROR: Can't open %s to read trace file\n", __pTraceFileName);
        exit (-1);
      }
   }
}


/* look if trajectory shall be traced */
/* ---------------------------------- */
char GetTraceState(TotalID stID)
{
  char cRet = 'N'; 
  long iS;                               // index of the searched ID in the table

  if (__nLinesTr > 5000)                // use smart search algorithm for a long list of trajectories
  {	
    long   iL = __nLinesTr-1;            // index of the last item in the table of wanted trajectories
                                         // numbers got by conversion from characters in the ID 
    double nL = IdNumber(__pTrace[iL]);  // - for the last item in the table of wanted trajectories
    double nC = IdNumber(stID);          // - for the current trajectory

    // estimation of the index of the searched ID
    iS = (long) (iL * nC / nL + 0.5);
    if (iS > iL) iS = iL;

    // search in the table for the index of the searched ID
    while (IdNumber(__pTrace[iS]) < nC  &&  iS < iL ) 
      iS++;
    while (IdNumber(__pTrace[iS]) > nC  &&  iS > 0  &&  iS <= iL) 
      iS--;

    // set 'tracing', if IDs are identical
    if (memcmp(stID.IDGrp, __pTrace[iS].IDGrp, 2)==0 && stID.IDNo==__pTrace[iS].IDNo)
      cRet='T'; 
  }
  else                                   // otherwise just go through  the list
  {
    for (iS=0; iS < __nLinesTr ; iS++)
    { // set 'tracing', if IDs are identical
      if (memcmp(stID.IDGrp, __pTrace[iS].IDGrp, 2)==0 && stID.IDNo==__pTrace[iS].IDNo)
      { cRet='T';
        goto exit_fct;
      }
    }
  }

 exit_fct:
  return cRet;
}


double IdNumber(TotalID stID)
{
  double g0 = 26.0 * (stID.IDGrp[0]-'A') * MAX_ULONG,  // * 26 * 2^32
         g1 =  1.0 * (stID.IDGrp[1]-'A') * MAX_ULONG,           //      * 2^32
         gN =  stID.IDNo;
 
  return g0+g1+gN;
}

