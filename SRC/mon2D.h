#ifndef MON2D_H
#define MON2D_H

/********************************************************************************************/
/*  VITESS module 'mon2D.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Oct 2011  D. Nekrassov  initial version                                              */
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


class Mon2D 
{
 public:
  McCompID eModule;   // defines type of module

  // input parameters
  string fMonitorFilename; // -O  name of the output file

  int      nBinsX;           // -x  number of x bins
  int      nBinsY;           // -y  number of y bins
  VtMonPar xParam;           // -X  parameter to be shown on the x axis
  VtMonPar yParam;           // -Y  parameter to be shown on the y axis

  double   xMin;             // -w  minimum x value
  double   xMax;             // -W  maximum x value
  double   yMin;             // -h  mininum y value
  double   yMax;             // -H  maximum y value

  int      bWeight;          // -p  use eigher actual probability of trajectories or 1 for all trajectories
  int      exclCounts;       // -e  do not forward neutrons to the pipe that do not contribute to the monitor data
  VtFormat2D format;         // -F  file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact

  // optional input parameters (filters and polarisation analysis)
  double     lambdaMin;        // -l  minimum wavelength, filter for the monitor
  double     lambdaMax;        // -L  maximum wavelength, filter for the monitor
  VtMonPar   filterParam1;     // -I  filter parameter 1
  VtMonPar   filterParam2;     // -J  filter parameter 2
  VtFiltComb filterComb;       // -C  filter parameters 1 and 2 combined with AND or OR
  double     filterVarMin1;    // -u  minimum value of parameter 1, additional filter for the monitor
  double     filterVarMin2;    // -U  maximum value of parameter 2, additional filter for the monitor
  double     filterVarMax1;    // -v  minimum value of parameter 1, additional filter for the monitor
  double     filterVarMax2;    // -V  maximum value of parameter 2, additional filter for the monitor
  
  int    analysePol;         // -P  switched on if polarisation analysis desired
  MathVector* 
    polAnalysisVector;       // -r -s -t  polarisation analysis vector

  // input parameters that are not (yet) implemented
  // int normalise;   // normaisation of the histogram by the size of x bins, input parameter
  // int colour;      // if switched on, display only neutrons of specific colour

  // Variables determined from input parameters or trajectory data
  MathMatrix* polAnalysisRotMatrix;  // rotation matrix for polarisation analysis 

  FILE*  fMonitor;             // pointer to output file
                           
  long   nBunches;              // number of bunches started 
  long   nTrajTot;             // total number of trajectories within monitor limits
  double IntTot;               // total intensity within monitor limits
                           
  double xBinSize;             // size of x bins 
  double yBinSize;             // size of y bons

  // arrays for data storage
  double*  BinPosX;             // edges of the bins of the first parameter
  double*  BinPosY;             // edges of the bins of the second parameter
  double** dataArray;           // here the monitor data is stored
  double** dataArrayPolWeights; // in case polarisation analysis is desired, here the spin weights are stored
  double** dataArrayError;
  double** dataArrayPol;        // this is the average polarisation in a bin
  long**   dataArrayCounts;

  // string weightTag[2];     // text: parameter
  // string formatTag[2];     // text: format
  // Neutron* currentNeutron;

  // constructor and destructor
  Mon2D();
  virtual ~Mon2D() {};

  // operations
  void   OwnInit(int argc, char* argv[]);              // Read in the monitor parameters from the command line
  double DetermineParameter(VtMonPar id, Neutron* n);  // Determine, which parameter has to be calculated
  int    FillMonitor(Neutron* n);                      // Fill monitor, if the neutron fulfills all constraints
  void   WriteOut(long iBnch);                         // Write output file
  void   ParId2Text(char* sName, const VtMonPar ePar); // Convert parameter ID to text
  void   FreeMemory();                                 // Free allocated memory
};


#endif
