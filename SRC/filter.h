/********************************************************************************************/
/*  VITESS module 'filter.h'                                                                */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Apr 2013  D. Nekrassov  initial version                                              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "mathvector.h"

VtMonPar   filterParam [4];   // -I -J -K -L   [-]  ID for 1st, 2nd, 3rd, 4th filter parameter
double     filterVarMin[4];   // -u -v -w -x  [var] minimum value of 1st, 2nd, 3rd, 4th parameter
double     filterVarMax[4];   // -U -V -W -X  [var] maximum value of 1st, 2nd, 3rd, 4th parameter
VtFiltComb filterComb;        // -C            [-]  enum for combination of parameters: AND, OR AND_OR_AND

void   OwnInit(int argc, char *argv[]);            // Reads input parameters and sets global variables
int    CheckFilter(Neutron* n);                    // Checks if neutron complies with combination of filters
double DetermineParameter(int id, Neutron* n);     // Returns the parameter value identified by 'id'
