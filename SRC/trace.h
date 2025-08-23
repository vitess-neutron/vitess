#ifndef TRACE_H
#define TRACE_H

#include <ctype.h>

#include "general.h"


/* global variables */
extern TotalID*  _aTrace;         // table of trajectory IDs for tracing
extern long      _nLinesTr;       // Number of lines in the trace file
extern char*     _sTraceFileName; // name of the file containing the trajectories to be traced or started
extern VtTrace   _eTraceMode;     // NO_TRACING     : no tracing
                                  // WRITE_TRC_FILES: write trace files for traj. of interest
                                  // ONLY_TRC_TRAJ  : simulation only with traj. of interest

/* functions */
void  LoadTraceFile();             // loads list of trajectories that shall be traced
char  GetTraceState(TotalID stID); // looks if trajectory shall be traced

#endif
