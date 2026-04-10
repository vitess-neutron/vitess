#ifndef SOURCE_ESS_H
#define SOURCE_ESS_H

#include "defines.h"

/***********************************************/
/* Prototypes                                  */
/***********************************************/

double EssTotFU2015      (const double ModHeight, const double ModTemp,   const double Power, const double Freq,
                          const double PulseLen);
double EssTotFU2016      (const double ModHeight, const double ModTemp,   const double Power, const double Freq,
                          const double PulseLen,  const double PfmcThml,  const double PfmcCold);
double EssModFU_Butterfly2015(const double ModHeight, const double Power, const double Freq,   const double Declination, const Neutron* pNeutron,
                              const double PulseLen, const double PfmcThml, const double PfmcCold);
double EssModFU_Butterfly2016(const double ModTemp,  const double Power,    const double Freq, const double Declination, const Neutron* pNeutron,
                              const double PulseLen, const double PfmcThml, const double PfmcCold);
double GetModWidth_ESSbutterfly2015(const double Dev90, const double ModTemp);
double GetModWidth_ESSbutterfly2016(const double Dev90, const double ModTemp);
double GetShift_ESSbutterfly2016   (void);
short  GetColour_ESSbutterfly2015  (double x0, const double Dev90);
short  GetColour_ESSbutterfly2016  (const double lambda);
void   LoadHorDistrib              (const char* sBeamlineID);

void   AnalyzeDecl       (const double Angle);  // if applicable: change old declination angle 'dev90' to real declination and generate beamport if missing
void   GenerPortFromAlpha(const double Alpha);  // generate beamport ID from orientation 'alpha' as defined by ESS      (range:  30 to 330 deg)
void   GenerPortFromDev90(const double Dev90);  // generate beamport ID from deviation 'dev90' from 90 deg orientation  (range: -60 to  60 deg, only S amd E beamports)
void   CalcPortAngle     (void);                // calculate real ESS beamport orientation 'alpha' from beamport ID     (range:  27 to 333 deg)
double CalcAlpha         (const char*  sID);    // determine standard ESS beamport orientation 'alpha' from beamport ID (range:  30 to 330 deg)
double CalcDev90         (const double Alpha);  // calculate deviation 'dev90' from 90 deg beamport (from 'alpha')      (range: -63 to  63 deg,  

#endif
