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

#define UNUSED -1

#define OR_OR_OR    0
#define AND_AND_AND 1
#define AND_OR_AND  2

int    filterParam [4];
double filterVarMin[4];
double filterVarMax[4];
int    filterComb;

void OwnInit(int argc, char *argv[]);
int CheckFilter(Neutron* n);
double DetermineParameter(int id, Neutron* n);
