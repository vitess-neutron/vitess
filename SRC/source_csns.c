/********************************************************************************************/
/*  VITESS module 'source_csns.c'                                                           */
/*    Functions to simulate the moderators of the Chinese Spallation Neutron Source (CSNS)  */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/********************************************************************************************/

#include "init.h"
#include "src_modchar.h"
#include "message.h"


/* global variables */
/* ---------------- */
extern
FctTable  stFluxT[NUM_MOD],  /* data of time distr.            */
          stFluxL[NUM_MOD],  /* data of wavelength distr.      */
          stFluxLT[NUM_MOD]; /* data of wavelength-time distr. */
extern
short     nNumMod,         /* number of moderators in moderator system */
          imod;            /* index of moderators in moderator system  */
extern
Moderator stMod  [NUM_MOD]; /* moderator data            */

/* global variables */
/* ---------------- */
static short  s_nSource=ANYSOURCE, /* s_nSource    : ANYSOURCE, ESS, SNS, CSNS                         */
              s_nModType=0;        /* s_nModType   : decoupled POISONED, DECOUPLED unpoisened, COUPLED */


/* dTemp      : [K]  eff. moderator temperature 
   nModType   :      decoupled POISONED, DECOUPLED unpoisoned, COUPLED
   dPower     : [W]  average source power                    */          
double CsnsTotalFU(const double dTemp,   const short  eModType, const double dPower)
{
  double dFUAmpl=0.0;;

  s_nSource = CSNS; 

  if (dTemp < 100.0)
  { 
    if (eModType==POISONED)
      dFUAmpl = dPower * 2.7e10/5.0e06/(25.0/50.0);
	else
      dFUAmpl = dPower * 2.3e11/5.0e06/(25.0/50.0);
  }
  else
  { 
    dFUAmpl = dPower * 1.8e11/5.0e06/(25.0/50.0);
  }
  return dFUAmpl;
}


/* dLambda: [Ang]  wavelength                          [Ang]
   dTime  :  [s]   time (after beginning of pulse)     [s]          
   dPosY  :  [cm]  horinzontal position on the source  [cm]
   dPosY  :  [cm]  vertical position on the source     [cm]   */

double CsnsModFU(const double dLambda, const double dTime, const double dPosY, const double dPosZ)
{
  double dFuA,         // amplitude of the flux    [n/(cm²  sterad Ang]
         dFu=0.0;      // actualflux value         [n/(cm²s sterad Ang]

  dFuA = stMod[imod].dFUAmpl * Maxwellian(dLambda, stMod[imod].dModTemp);

  if      (stMod[imod].eModType==COUPLED   && stMod[imod].dModTemp < 100.0)
    dFu = dFuA * PulseShape(dTime, 2.9e-04, 20.0);
  else if (stMod[imod].eModType==DECOUPLED && stMod[imod].dModTemp > 100.0)
    dFu = dFuA * PulseShape(dTime, 3.5e-05,  5.0);
  else if (stMod[imod].eModType==POISONED  && stMod[imod].dModTemp < 100.0)
    dFu = dFuA * PulseShape(dTime, 4.9e-05,  5.0);
  else
    Error("data for chosen CSNS moderator not available");

  return dFu;
}
  
