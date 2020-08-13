#ifndef BENDER_INTER_DATA_H
#define BENDER_INTER_DATA_H

/***********************************************************************/
/* bender_inter_data.h                                                 */
/* Prototypes of functions to calculate attenuation in materials       */
/***********************************************************************/

#define MAX_MU   500    // size of arrays for wavelength and corresponding attenuation values

void   Gadolinium   (double wavelen[],    double mu[],  long* pVal);
void   Cadmium      (double wavelen[],    double mu[],  long* pVal);
void   Bor10        (double wavelen[],    double mu[],  long* pVal);
void   Eu           (double wavelen[],    double mu[],  long* pVal);
void   Silicon      (double wavelen[],    double mu[],  long* pVal); 

double Interpolation(double WavelenIn,    long    Material, 
                     double WavF[MAX_MU], double MuF[MAX_MU], long nValFile);

#endif
