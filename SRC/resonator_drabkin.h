#ifndef RES_DRABKIN_H
#define RES_DRABKIN_H

#include "init.h"

/************************************/
/** Definitions, structures, enums **/
/************************************/
/* Number of domains */
#define  FIELD_SIZE    3000
#define  FIELD_SIZE_FL  9000

#define  ENERGY_FROM_LAMBDA(x)    ( 81805.048 / x / x ) /*[ueV]*/


/**************************/
/** Functions prototypes **/
/**************************/
void   OwnInit(int argc, char *argv[]);                             // Reads input parameters and sets global parameters
void   OwnCleanup();                                                // Does module specific cleanup
void   SetGeometry(char* sColor);                                   // Fills the structure stGeometry for visualization
void   WriteFiles (long nDom);                                      // Writes files containing polarisation and magnetic field
/* Intersection with rectangular object */
long   IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);

#endif
