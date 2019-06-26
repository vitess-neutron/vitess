#ifndef SOURCE_ESS_H
#define SOURCE_ESS_H

/***********************************************/
/* Prototypes                                  */
/***********************************************/

double EssTotFU2015(const double ModHeight, const double ModTemp, const double Power, const double Freq, const double PulseLen);
double EssModFU2015(const double ModHeight, const double Power, const double Freq, const double Declination, const Neutron* pNeutron);
double GetModeratorWidth_ESSbutterfly2015(const double theta, const double ModTemp);
short GetColour_ESSbutterfly2015(const double x0, const double theta);

#endif

