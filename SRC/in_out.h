#ifndef IN_OUT_H
#define IN_OUT_H

#include <stdio.h>
#include <stdlib.h>

#include "general.h"

/***********************/
/** Definitions       **/
/***********************/


/***********************/
/** Structures        **/
/***********************/
typedef struct
{
	double         Weight;
	VectorType     Position;
	VectorType     Speed;
	double         Time;
	VectorType     Spin;
}
McNeutron;

typedef struct
{
	VectorType     Position;
	VectorType     Vector;
  double         Energy;
	double         Counts;
	double         Shakes;
}
McnpNeutron;


/***********************/
/** Enums             **/
/***********************/

#define VT_EOF -1

typedef enum
{ VT_VITESS_FMT = 1,
  VT_MCSTAS_FMT = 2,
  VT_MCPL_FMT   = 3,
  VT_MCNPX_FMT  = 4,
  VT_VITESS_BIN = 5
}
VtPrgFormat;

typedef enum
{ VT_EXPONENTIAL = 0,
  VT_FLOAT       = 1,
  VT_BINARY      = 2
}
VtDataFormat;

typedef enum
{ VT_BLANK     = 0,
  VT_TABULATOR = 1
}
VtSeparator;



#endif