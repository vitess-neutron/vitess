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

extern "C" {
#include "general.h"
#include "init.h"
}

#include "mathvector.h"
#include "mathmatrix.h"



class Mon1D {

 public:

  double* dataArray; // here the monitor data is stored
  double* dataArrayPolWeights; // in case polarisation analysis is desired, here the spin weights are stored
  
  double xMin;  // minimum x value, input parameter
  double xMax;  // maximum x value, input parameter

  int nBinsX;  // number of x bins, input parameter

  double xBinSize; // size of x bins 

  int xParam;  // parameter to be shown on the x axis, input parameter

  FILE* fMonitor; // pointer to output file
  string fMonitorFilename;  // name of the output file, input parameter

  double lambdaMin;  // minimum wavelength, filter for the monitor, optional input parameter
  double lambdaMax;  // maximum wavelength, filter for the monitor, optional input parameter

  double filterVarMin1; // minimum value of parameter 1, additional filter for the monitor, optional input parameter
  double filterVarMin2; // maximum value of parameter 2, additional filter for the monitor, optional input parameter
  double filterVarMax1; // minimum value of parameter 1, additional filter for the monitor, optional input parameter
  double filterVarMax2; // maximum value of parameter 2, additional filter for the monitor, optional input parameter

  int filterParam1;  // filter parameter 1, optional input parameter
  int filterParam2;  // filter parameter 2, optional input parameter
  
  int analysePol;  // switched on if polarisation analysis desired, optional input parameter

  MathVector* polAnalysisVector;   // polarisation analysis vector
  MathMatrix* polAnalysisRotMatrix;  // rotation matrix for polarisation analysis 

  int normalise; // normaisation of the histogram by the size of x bins, input parameter


  int colour; // if switched on, display only neutrons of specific colour
  int pWeight; // use eigher actual probability of trajectories or 1 for all trajectories
  int exclCounts; // do not forward neutrons to the pipe that do not contribute to the monitor data

  Neutron* currentNeutron;

  Mon1D();
  virtual ~Mon1D() {};

  void Init(int argc, char* argv[]); // Read in the monitor parameters from the command line
  double DetermineParameter(int id, Neutron* n); // Determine, which parameter has to be calculated
  int FillMonitor(Neutron* n); // Fill monitor, if the neutron fulfills all constraints
  void WriteOut(); // Write output file

};


#endif
