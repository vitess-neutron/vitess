#ifndef SOURCEAI_H
#define SOURCEAI_H

/********************************************************************************************/
/*  VITESS module 'source_ai.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 31 Jul 2024 J. Robledo initial version                                                   */
/********************************************************************************************/

extern "C"
{
  #include "convert.h"
  #include "init.h"
  #include "general.h"
  #include "trace.h"
}

#include <torch/script.h>

/******************************/
/** Global Variables    **/
/******************************/

// Input parameters
char       *ModelFileName;             // -M       [-]   Name of the Model file.
int         nNeut,                     // -n       [-]   Number of neutrons.
            nBunches;                  // -B       [-]   Number of bunches.

// For tracing
/*
extern char* _sTraceFileName;            // -T        name of the file containing the trajectories to be traced or started
extern VtTrace _eTraceMode;              // -t        NO_TRACING     : no tracing
                                         //           WRITE_TRC_FILES: write trace files for traj. of interest
                                         //           ONLY_TRC_TRAJ  : simulation only with traj. of interest
*/
torch::jit::script::Module model;
torch::Device device(torch::kCPU);

/******************************/
/** Prototypes               **/
/******************************/
void    OwnInit(int argc, char *argv[]);     // Reads input parameters, sets global parameters and calculates rotation matrices etc.
void    OwnCleanup();                        // Does module specific cleanup

#endif
