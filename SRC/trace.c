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
char*     _sTraceFileName=NULL;   // name of the file containing the trajectories to be traced or started
TotalID*  _aTrace=NULL;           // table of trajectory IDs for tracing
long      _nLinesTr=0;            // Number of lines in the trace file  
int       _eTraceMode=NO_TRACING; // NO_TRACING     : no tracing 
                                   // WRITE_TRC_FILES: write trace files for traj. of interest
                                   // ONLY_TRC_TRAJ  : simulation only with traj. of interest 

static double IdNumber(TotalID stID);


/* load list of trajectories that shall be traced */
/* ---------------------------------------------- */
void LoadTraceFile()
{
  char  sBuffer[CHAR_BUF_LENGTH]; 
  FILE* pTraceFile=NULL;

  /* If there is a trace file go and load the file */
  if (_sTraceFileName!=NULL) 
   {
      /* opens distribution file */
      if ((pTraceFile = OpenInputFile(_sTraceFileName, FALSE, "rt"))!=NULL) 
      {
        long i;

        /* reads number of lines, allocates memory and then reads distribution file */
        _nLinesTr = LinesInFile(pTraceFile);
        _aTrace   = (TotalID*) calloc(_nLinesTr, sizeof(TotalID));

        for(i=0; i < _nLinesTr; i++)
        {  
          ReadLine(pTraceFile, sBuffer, CHAR_BUF_LENGTH-1);
          sscanf  (sBuffer, "%c%c%lu", &_aTrace[i].IDGrp[0], &_aTrace[i].IDGrp[1], &_aTrace[i].IDNo);
        }

        /* closes trace file */
        fclose(pTraceFile) ;
      } 
      else 
      { fprintf(LogFilePtr, "\nERROR: Can't open %s to read trace file\n", _sTraceFileName);
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

  if (_nLinesTr > 5000)                // use smart search algorithm for a long list of trajectories
  {	
    long   iL = _nLinesTr-1;            // index of the last item in the table of wanted trajectories
                                         // numbers got by conversion from characters in the ID 
    double nL = IdNumber(_aTrace[iL]);  // - for the last item in the table of wanted trajectories
    double nC = IdNumber(stID);          // - for the current trajectory

    // estimation of the index of the searched ID
    iS = (long) (iL * nC / nL + 0.5);
    if (iS > iL) iS = iL;

    // search in the table for the index of the searched ID
    while (IdNumber(_aTrace[iS]) < nC  &&  iS < iL ) 
      iS++;
    while (IdNumber(_aTrace[iS]) > nC  &&  iS > 0  &&  iS <= iL) 
      iS--;

    // set 'tracing', if IDs are identical
    if (memcmp(stID.IDGrp, _aTrace[iS].IDGrp, 2)==0 && stID.IDNo==_aTrace[iS].IDNo)
      cRet='T'; 
  }
  else                                   // otherwise just go through  the list
  {
    for (iS=0; iS < _nLinesTr ; iS++)
    { // set 'tracing', if IDs are identical
      if (memcmp(stID.IDGrp, _aTrace[iS].IDGrp, 2)==0 && stID.IDNo==_aTrace[iS].IDNo)
      { cRet='T';
        goto exit_fct;
      }
    }
  }

 exit_fct:
  return cRet;
}


static double IdNumber(TotalID stID)
{
  double g0 = 26.0 * (stID.IDGrp[0]-'A') * MAX_ULONG,  // * 26 * 2^32
         g1 =  1.0 * (stID.IDGrp[1]-'A') * MAX_ULONG,           //      * 2^32
         gN =  stID.IDNo;
 
  return g0+g1+gN;
}

