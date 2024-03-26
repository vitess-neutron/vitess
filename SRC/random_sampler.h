#ifndef RANDOM_SAMPLER_H
#define RANDOM_SAMPLER_H

/********************************************************************************************/
/*  VITESS module 'random_sampler.h'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 03 Oct 2024  J. Robledo initial version                                              */
/********************************************************************************************/

#include <stdio.h>

/********************************************************************/
/* general definitions for the module random_sampler.c			    */

void randomSampleFile(char* inFile, FILE* outFilePtr, int sampleSize);

/********************************************************************/

#endif