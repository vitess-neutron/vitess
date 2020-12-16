#ifndef TRACE_H
#define TRACE_H

#include <ctype.h>

#include "general.h"


/* global variables */
extern TotalID*  __pTrace;         // table of trajectory IDs for tracing
extern long      __nLinesTr;       // Number of lines in the trace file  
/*
extern char*     __pTraceFileName; // name of the file containing the trajectories to be traced or started
extern short     __eTraceMode;     // NO_TRACING     : no tracing 
                                   // WRITE_TRC_FILES: write trace files for traj. of interest
                                   // ONLY_TRC_TRAJ  : simulation only with traj. of interest  */

/* functions */
void  LoadTraceFile();
char  GetTraceState(TotalID stID);

#endif