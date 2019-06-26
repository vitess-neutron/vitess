#ifndef SAMPLE_REFL_H
#define SAMPLE_REFL_H

#include "general.h"



/**************************************************/
/* global variables and constants                 */
/**************************************************/

FILE   *Par_Crys, *Foc_Crys ;
char   *ParameterFileName;
long   NumOut,  i ;

short  g_nNoAngle,       /* number of angles                               */
  useIncoherent;         /* Flag whether to use incoherent scattering: 0 for "not use", 1 "for use" */
long   g_nOption,        /* option: 
                            1: reflection of sample
                            2: reflection of reference                     */
       g_nLinesRefl;     /* number of lines in reflectivity file           */
double g_dRotAngle,      /* min. value of angle or reflection              */
       g_dRotHoriz,      /* max. value of angle or reflection              */
       g_dRotVert,       /* step size in angle or reflection               */
       g_dProbIn,        /* input probabilities for one angle              */
       g_dProbOut,       /* output probabilities for one angle             */
       g_dModCurrent,    /* mean beam current leaving the moderator        */
       g_dTimeMeas,      /* time of measurement in seconds                 */
       muInc,            /* Macroscopic incoherent cross section */    
       detWidth,         /* Width of the detector for solid angle calculation */
       detHeight,        /* Height of the detector for solid angle calculation */
       detDist,          /* Distance to the detector for solid angle calculation */
       minTheta,         /* Minimum and maximum theta angles for the incoherent scattering */
       maxTheta,
       maxProb,
      signalToBkgAreaFactor, /* Relates the area on the detector with signal counts to total detector area. */
      *g_pTabQ,          /* pointer on table of Q-values                   */    
      *g_pTabR;          /* pointer on table of the respective R-values    */ 
char   g_sRotAxis[4],    /* rotation axis of sample "Y" or"Z"              */
      *g_pReflFileName;  /* name of file for theoretical spectrum          */
FILE	*g_pReflFile;      /* pointer on file for theoretical spectrum       */

/* definitions of module parameters */
double ProbCutoff, RotHoriz, RotVert, PosCE[3], DimCE[3], AnglFocHoriz, AnglFocVert ;
double TranslFoc[3], Depth[3] ;
double RotMatrixCE[3][3], RotMatrixFoc[3][3], RotMatrixIncoherent[3][3] ;

/* focus geometry parameters */
int			NumberCE[2] ;
double		User ;



/**************************************************/
/** Prototypes                                   **/
/**************************************************/
short  Reflect    (Neutron* pNeutron, short int treatingReflection);
double ReadReflect(const double dQ);
void   OwnInit(int argc, char *argv[]) ;
void   OwnCleanup() ;
void   ReadParameterFile() ;
void   ReadReflectivityFile();
void   AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert) ;
void   CalculateThetaRange();
void   CalculatePhiRange(double theta, double *phiMin, double *phiMax, int *switchSign);

#endif
