#ifndef SOURCE_CSNS_H
#define SOURCE_CSNS_H

/***********************************************/
/* Functions to simulate the CSNS source       */
/***********************************************/


/***********************************************/
/* Prototypes                                  */
/***********************************************/

double CsnsTotalFU  (const double dTemp,   const short  eModType, const double dPower);
double CsnsModFU    (const double dLambda, const double dTime,    const double dPosY,    const double dPosZ);

#endif

