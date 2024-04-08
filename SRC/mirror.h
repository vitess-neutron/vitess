#ifndef MIRROR_H
#define MIRROR_H

#include <string.h>

#include "intersection.h"
#include "init.h"

#ifdef VT_GRAPH
  # include "cpgplot.h"
  extern int do_visualise; /* default : no visualisation */
#endif


/******************************/
/** Structures and Enums     **/
/******************************/
typedef struct
{
  SurfaceSecond Surf[2];
}
MirrorSecond;


/******************************/
/** Prototypes               **/
/******************************/
/* Propagation of the neutron to the end of the mirror          */
double PathThroughMirrorGravOrder2(Neutron *ThisNeutron, MirrorSecond MyMirror, double X_MIN, double X_MAX, double Y_MIN, double Y_MAX, double Z_MIN, double Z_MAX, double halfaxis[3], VectorType PosMain,
                                   double wei_min, double reflectivitylup[1000], double reflectivityldo[1000], double surfacerough, long keygrav, long keypol, long qspin, int vistype, long visall, long keyreflect, int *reflperf, double RotMatrixMirror[3][3], long keyfluxair, double mu1, double mu2);

/* Check if neutron position is within the limits of the mirror */
double Checklimits  (Neutron *ThisNeutron, int ikey, double Time, double X_MIN, double X_MAX, double Y_MIN, double Y_MAX, double Z_MIN, double Z_MAX);

/* Procedure for moving the neutron WITH gravity       */
double NeutronMove  (Neutron *ThisNeutron, double Time);

/* Function for simulation of surface roughness        */
double SurfaceRough(double *AP, double *BP, double *CP, double surfacerough);

/* Calculation of the point of reflection on the mirror without gravity */
double NeutronSurfaceSecIntersectionGrav  (Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond, long keygrav);

/* Calculation of the point of reflection on the mirror with gravity    */
double NeutronSurfaceSecIntersectionGravLen(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond);

/* 3D random number generation */
void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);  // from gsl library

#endif
