#ifndef OPT_VARS_H
#define OPT_VARS_H

#include <stdio.h>

#include "opt_defs.h"

// global variables
extern FILE*  LogFilePtr;           // Pointer on the log file   (from init.c)
extern char   sLogFile[FN_LEN];     // name of the log file
//              sIniFile[FN_LEN];     // name of the file containing the control parameters for the optimization routine
extern double arP[MAX_SIM][NMAX+1], // parameter sets P, actual (P0) and variations P1 ... P_nSim-1
              arF[MAX_SIM][IMAX+1]; // functions F corresponding to parameter sets 0 ... nSim-1
extern double P00 [NMAX+1],         // initial value of vector P
              Pmin[NMAX+1],         // minimal values for components of vector P
              Pmax[NMAX+1],         // maximal values for components of vector P
              DelP[NMAX+1],         // DeltaP for numerical Differentiation
              X[IMAX+1],            // parameter X_1 ... X_anz (e.g. temperature)
              Y[IMAX+1],            // measured data Y_1 ... Y_anz
              W[IMAX+1];            // weight of measuring points
extern int    nPts,                 // number of measuring points
              iStep;                // actual optimization step
extern short  nPar,                 // number of fit parameters
              nSim,                 // number of simulation results found in sFCommFilename
              eOut;                 // parameter to control output
extern VtAppl eOption;              // application option  '1': 'sim_opt_pc'    MC sim. optimization
                                    //                     '2': 'sim_opt_grid'  MC sim. optimization on cluster
                                    //                     '3': 'fit'           fit
#endif
