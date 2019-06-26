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

#include "mathvector.h"


int filterParam[3];
double filterVarMin[3];
double filterVarMax[3];
int filterComb;

void OwnInit(int argc, char *argv[]);
int CheckFilter(Neutron* n);
double DetermineParameter(int id, Neutron* n);
