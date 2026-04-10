#ifndef LENSE_H
#define LENSE_H

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
  SurfaceSecond Surf[5];
}
LenseSecond;

/* 1 surface: first surface of lense 0
   2 surface: second surface of lense 1
   3 surface: cylindrical surface of lense 2
   4 surface: output plane 3       */


/******************************/
/** Prototypes               **/
/******************************/
/* Propagation of the neutron to the end of the lense          */
double  PathThroughLenseOrder2(Neutron *ThisNeutron, LenseSecond MyLense, double Radius1,
                               double Radius2, double RadiusMain, double Thickness, double Refract, double Atten, double AttScattering,
                               VectorType PosMain, VectorType TransOut, double wei_min, double surfacerough,
                               long keygrav, long NeutronLoss, long Attenkey, long CurrentLense,
                               long LenseForOut, long LenseForOutVis, long ServiceInfoK, FILE *COLLFILE, long LenseType);

/* Function for refraction procedure    */
double MakeRefract(Neutron *ThisNeutron, LenseSecond MyLense, double Refr, double surfacerough, long sn);

/* Procedure for moving the neutron WITH gravity    */
double NeutronMove  (Neutron *ThisNeutron, double Time);

/* Function for simulation of surface roughness        */
double SurfaceRough(double *AP, double *BP, double *CP, double surfacerough);

/* Calculation of the point of reflection on the lense without gravity */
double NeutronSurfaceSecIntersectionGrav  (Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond, long keygrav);

/* Calculation of the point of reflection on the lense with gravity    */
double NeutronSurfaceSecIntersectionGravLen(Neutron *ThisNeutron, SurfaceSecond ThisSurfaceSecond);

/* 3D random number generation */
void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);  // from gsl library

#endif
