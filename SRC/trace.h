#ifndef TRACE_H
#define TRACE_H

#include <ctype.h>

#include "general.h"


/* global variables */
extern char*     __pTraceFileName; // name of the file containing the trajectories to be traced or started
extern TotalID*  __pTrace;         // table of trajectory IDs for tracing
extern long      __nLinesTr;       // Number of lines in the trace file  
extern short     __eTraceMode;     // mode 0: no tracing 
                                   // mode 1: write trace files for traj. of interest
                                   // mode 2: simulation only with traj. of interest 

/* functions */
void  LoadTraceFile();
char  GetTraceState(TotalID stID);

#endif