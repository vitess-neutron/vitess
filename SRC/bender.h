#ifndef BENDER_H
#define BENDER_H

/***********************************************************************/
/* bender.h                                                            */
/* Definitions, structures and prototypes for the bender module        */
/***********************************************************************/
#include "general.h"

/******************************/
/** Definitions              **/
/******************************/
#define  N_SURF       601
#define  N_SURF_S     600
#define  N_SURF_M3	 1801
#define  N_SURF_M3_S 1800


/****************************************/
/** Structures (changed in March 2002) **/
/****************************************/

/* Structure to describe bender geometry */

typedef struct
{
/* double CriticalAngles;
  double CutoffAngles; */
  SurfaceSecond SurfLeft [N_SURF];
  SurfaceSecond SurfRight[N_SURF];
  SurfaceSecond SurfExit[N_SURF];
  SurfaceSecond SurfTopBottom[N_SURF];
}
Bender;


/* Structure to describe bender channel */

typedef struct
{
/* double CriticalAngles;
  double CutoffAngles; */
  SurfaceSecond Surf[5];
}
BenderChannel;


/**************************************/
/** Prototypes of external functions **/
/**************************************/

double PathThroughChannelGravOrder2(Neutron *, Bender, BenderChannel, long, long, double, double, double *, double *,
                                   double *, double *, double *, double *, double,
                                   long, long, long, double *, double *, double);

double PathThroughBenderGravOrder2(Neutron *, Bender, BenderChannel, long, long, double, double, double *, double *,
                                   double *, double *, double *, double *, double,
                                   long, long, long, double *, double *, double,
                                   long, long, long,
                                   double *, double *, long,
                                   double *, double *, long,
                                   double *, double *, long);

void GeometryTestBender(Bender, double *, double *, double *, double *,
                        double *, double *, double *, double *,
                        double, double , double , long);


#endif
