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
#define  N_SURF_M3   1801
#define  N_SURF_M3_S 1800


/****************************************/
/** Structures                         **/
/****************************************/

/* Structure to describe bender geometry */
typedef struct
{
  SurfaceSecond SurfLeft [N_SURF];
  SurfaceSecond SurfRight[N_SURF];
  SurfaceSecond SurfExit[N_SURF];
  SurfaceSecond SurfTopBottom[N_SURF];
}
Bender;

/* Structure to describe bender channel */
typedef struct
{
  SurfaceSecond Surf[5];
}
BenderChannel;


/**************************************/
/** Prototypes of external functions **/
/**************************************/

double PathThroughChannelGravOrder2(Neutron *ThisNeutron,    Bender  MyBender,         BenderChannel ThisBenderChnl,
                                    long     ChnlNumber,     double  disabut,
                                    double  *ReflUpL,        double *ReflUpR,          double *ReflUpTB,
                                    double  *ReflDownL,      double *ReflDownR,        double *ReflDownTB,
                                    double   surfacerough,   long    keypol,           long   qspin,
                                    double  *entrdiscenter,  double *exitdiscenter,    double spacer,
                                    VtWndMat ChnlMaterial,   double  *aLambdaChnl,     double  *aMuChnl,       long nMuValuesChnl);

double PathThroughBenderGravOrder2(Neutron *ThisNeutron,     Bender   MyBender,        BenderChannel ThisBenderChnl,
                                   long     ChnlNumber,      long     NumberSurfaces,  double   disabut,
                                   double  *reflectivitylup, double  *reflectivityrup, double  *reflectivitytbup,
                                   double  *reflectivityldo, double  *reflectivityrdo, double  *reflectivitytbdo,
                                   double   surfacerough,    long     keypol,          long     qspin,
                                   double  *entrdiscenter,   double  *exitdiscenter,   double   spacer,
                                   VtWndMat ChnlMaterial,    double  *aLambdaChnl,     double  *aMuChnl,       long nMuValuesChnl,
                                   VtWndMat AbsorbMaterialL, double  *aLambdaL,        double  *aMuL,          long nMuValuesL,
                                   VtWndMat AbsorbMaterialR, double  *aLambdaR,        double  *aMuR,          long nMuValuesR);
 
void GeometryTestBender(Bender, double *, double *, double *, double *,
                        double *, double *, double *, double *,
                        double, double , double , long);

#endif
