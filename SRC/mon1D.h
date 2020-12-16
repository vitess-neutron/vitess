#ifndef MON1D_H
#define MON1D_H

/********************************************************************************************/
/*  VITESS module 'mon1D.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Jan 2012  D. Nekrassov  initial version                                              */
/********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

extern "C" 
{
 #include "general.h"
 #include "init.h"
 #include "mon2_header.h"
}

#include "mathvector.h"
#include "mathmatrix.h"



class Mon1D 
{
 public:
  McCompID eModule;               // defines the type of module

  // input parameters
  string fMonitorFilename;        // -O        common part of the output file names 
  int    eParX [3];               // -X -Y -Z  parameter to be shown on the x axis 
  int    nBinsX[3];               // -x -y -z  number of x bins 
  double xMin[3];                 // -w -f -g  minimum x value 
  double xMax[3];                 // -W -F -G  maximum x value 
  int    bWeight;                 // -p        use either actual probability of trajectories or 1 for all trajectories
  int    exclCounts;              // -e        do not forward neutrons to the pipe that do not contribute to the monitor data

  // optional input parameters (filters and polarisation analysis)
  double lambdaMin;               // -l        minimum wavelength, filter for the monitor
  double lambdaMax;               // -L        maximum wavelength, filter for the monitor
  int    filterParam1;            // -I        filter parameter 1
  int    filterParam2;            // -J        filter parameter 2
  int    filterComb;              // -C        filter 1 and 2 combined with AND or OR
  double filterVarMin1;           // -u        minimum value of parameter 1, additional filter for the monitor
  double filterVarMax1;           // -U        minimum value of parameter 1, additional filter for the monitor
  double filterVarMin2;           // -v        maximum value of parameter 2, additional filter for the monitor
  double filterVarMax2;           // -V        maximum value of parameter 2, additional filter for the monitor

  int    analysePol;              // -P        switched on if polarisation analysis desired
  MathVector* polAnalysisVector;  // -r -s -t  polarisation analysis vector

  // input parameters that are not (yet) implemented
  // int normalise;                     // normalisation of the histogram by the size of x bins 

  // Variables determined from input parameters or trajectory data
  FILE*       fMonitor[3];           // pointer to output file
  double      xBinSize[3];           // size of x bins 
  int         monSwitchedOn[3];      // Switches are activated if parameter 1, 2 or 3 should be stored.
  string      sParameterNames[18];   // text: parameter
  // string      weightTag[2];          // text: probability weight
  MathMatrix* polAnalysisRotMatrix;  // rotation matrix for polarisation analysis 

  // arrays for data storage
  double* dataArray[3];              // here the monitor data is stored
  double* dataArrayPolWeights[3];    // in case polarisation analysis is desired, here the spin weights are stored
  double* dataArrayError[3];
  int*    dataArrayCounts[3];

  // constructor and destructor
  Mon1D();
  virtual ~Mon1D() {};

  // operations
  void   OwnInit(int argc, char* argv[]);        // Read in the monitor parameters from the command line
  double DetermineParameter(int id, Neutron* n); // Determine, which parameter has to be calculated
  int    FillMonitorArray(Neutron* n);           // Fill all monitors chosen
  int    FillMonitor(Neutron* n, int counter);   // Fill one monitor, if the neutron fulfills all constraints
  void   WriteOut();                             // Write output file
  void   FreeMemory();                           // Free allocated memory
};


#endif
