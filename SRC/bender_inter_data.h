#ifndef BENDER_INTER_DATA_H
#define BENDER_INTER_DATA_H

#include "defines.h"

/***********************************************************************/
/* bender_inter_data.h                                                 */
/* Prototypes of functions to calculate attenuation in materials       */
/***********************************************************************/

#define MAX_MU 500 // size of arrays for wavelength and corresponding attenuation values

// calculates attenuation of a material for a certain wavelength
double AttenuationWnd(const double Lambda, const VtWndMat  eMatID, double* aLambda, double* aMu, const long nVals, const char* sAttFile); // used in bender and window modules
double AttenuationAbs(const double Lambda, const VtAbsMat  eMatID, double* aLambda, double* aMu, const long nVals, const char* sAttFile); // used in chopper
double AttenuationMir(const double Lambda, const VtMirrMat eMatID, double* aLambda, double* aMu, const long nVals, const char* sAttFile); // used in SM ensemble or sample substrate

// reading of attenuation data 
long   LoadAttenFile(double* aLambda, double* aMu, const char* pTransFileName, long nArrayLen);
long   ReadAttenWnd (const VtWndMat  eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFile);
long   ReadAttenAbs (const VtAbsMat  eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFile);
long   ReadAttenMir (const VtMirrMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFile);

// interpolation
double Interpol(double lambda, double* aLambda, double* aMu, long nVal);

// writeout functions
void WriteMatInfo (VtWndMat eMaterial, char* pText);

// functions to assess mu(lambda) for different materials
long Gadolinium (double *pLambda, double *pMu, const long nArrayLen);
long Cadmium    (double *pLambda, double *pMu, const long nArrayLen);
long Bor10      (double *pLambda, double *pMu, const long nArrayLen);
long Eu         (double *pLambda, double *pMu, const long nArrayLen);
long Silicon    (double *pLambda, double *pMu, const long nArrayLen);
long Vacuum     (double* pLambda, double* pMu);
long IdealAbsorp(double* pLambda, double* pMu);


#endif
