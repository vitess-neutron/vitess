#ifndef BENDER_INTER_DATA_H
#define BENDER_INTER_DATA_H

#include "defines.h"

/***********************************************************************/
/* bender_inter_data.h                                                 */
/* Prototypes of functions to calculate attenuation in materials       */
/***********************************************************************/

#define MAX_MU   500                                                    // size of arrays for wavelength and corresponding attenuation values

double AttenuationAbs (const double Lambda, const VtAbsMat eID);        // chopper material
double AttenuationMirr(const double Lambda, const VtMirrMat eMatID);    // material used in SM ensemble or sample substrate

double Interpolation(double WavelenIn,    long    Material,             // material used in bender or windows
                     double WavF[MAX_MU], double MuF[MAX_MU], long nValFile);

void   Gadolinium(double wavelen[],    double mu[],  long* pVal);       // These functions assess mu(lambda) for different materials
void   Cadmium   (double wavelen[],    double mu[],  long* pVal);
void   Bor10     (double wavelen[],    double mu[],  long* pVal);
void   Eu        (double wavelen[],    double mu[],  long* pVal);
void   Silicon   (double wavelen[],    double mu[],  long* pVal);

#endif
