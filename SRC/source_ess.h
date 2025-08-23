#ifndef SOURCE_ESS_H
#define SOURCE_ESS_H

#include "defines.h"

/***********************************************/
/* Prototypes                                  */
/***********************************************/

double EssTotFU2015      (const double ModHeight, const double ModTemp,  const double Power, const double Freq, const double PulseLen);
double EssTotFU2016      (const double ModHeight, const double ModTemp,  const double Power, const double Freq, 
                          const double PulseLen,  const double PfmcThml, const double PfmcCold);
double EssModFU_Butterfly2015(const double ModHeight, const double Power, const double Freq, const double Declination, const Neutron* pNeutron,
                              const double PulseLen, const double PfmcThml, const double PfmcCold);
double EssModFU_Butterfly2016(const double ModTemp,  const double Power, const double Freq, const double Declination, const Neutron* pNeutron,
                              const double PulseLen, const double PfmcThml, const double PfmcCold);
double GetModWidth_ESSbutterfly2015(const double theta, const double ModTemp);
double GetModWidth_ESSbutterfly2016(const double theta, const double ModTemp);
double GetShift_ESSbutterfly2016   (const double theta);
short  GetColour_ESSbutterfly2015  (double x0, const double theta);
short  GetColour_ESSbutterfly2016  (const double lambda);
void   LoadHorDistrib(const char* sID);

double CalcTheta(const char* sID);
double CalcDecl (const double theta);
char*  GenerBeamport(const double Decl);

#endif

