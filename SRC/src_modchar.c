/********************************************************************************************/
/*  VITESS module 'src_modchar.c'                                                           */
/*    Analytic Functions for Simulations of Moderators                                      */
/*                                                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/********************************************************************************************/

#include "init.h"
#include "src_modchar.h"
#include "message.h"


typedef struct
{
	double dF001;
	double dF002;
	double dTemp;
}
ModInfo;


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


/* static variables */
/* ---------------- */
static
ModInfo   stMInfo[NUM_MOD][2]; /* additional moderator data
                                  multispec. moder.: index 0 for cold part, 1 for thermal part
                                  otherwise        : only index 0 used */



static short  s_nSource=ANYSOURCE, /* s_nSource    : ANYSOURCE, ESS, SNS                               */
              s_nModType=0;        /* s_nModType   : decoupled POISONED, DECOUPLED unpoisened, COUPLED */



/* ----------------------------- */
/* global functions              */
/* ----------------------------- */

/* FluxLT() : returns index in array 'pTabF' of 2-dim. table for flux(time, wavelength),
	storage rowwise
	i: index for time        (defines row)
   j: index for wavelength  (defines column)
 */
long IndLT(const long i, const long j)
{
	return(i*(stFluxLT[imod].nColumns) + j);
}


/* TotalFU() : returns amplitude of a pulsed source in FluxUnits
 */
double TotalFU(const double _dTemp,  const short  _nSource, const short  _nModType, 
               const double _dPower, const double _dPeriod, const double _dPulseLen)
{
	/* _dTemp      : [K]  eff. moderator temperature 
	   _nSource    :      ESS, SNS,
	   _nModType   :      decoupled POISONED, DECOUPLED unpoisened, COUPLED
	   _dPower     : [W]  average source power                             
	   _dPeriod    : [ms] time between 2 pulses                             
	   _dPulseLen  : [s]  average source power                             */

	double dFUAmpl= 0.0,
	       dFacM  = 1.0,     //     integral of fct. M(lambda) = number of Maxwellian functions 
	       dFacN  = 1.904,   //     integral of fct. N(lambda) in 0.1 Ang .... 20 Ang
	       U0     = 2.5e9,   // [V] accelerator voltage: 2.5 GV 
	       dEp_std= 1.0e5,   // [J] standard energy of 1 pulse: 5 MW * 20 ms = 100 kJ (as for ESS SPTS)
	       dEpulse,          // [J] energy of 1 pulse          
	       dPeriod,          // [s] time between 2 pulses 
	       dCurrLimit=0.050, // [A] max. possible accelerator current
	       dCurrMax;         // [A] max. current for this set-up     
	char   sBuffer[256];

	/* initialize */
	memset(stMInfo[imod], '\0', sizeof(ModInfo));

	s_nSource     = _nSource;
	s_nModType    = _nModType;
	if (s_nModType!=MULT_SPEC)
		stMInfo[imod][0].dTemp = _dTemp;

	/* integral of fct. N(lambda) in 0.1 Ang .... 20 Ang */
	if (_dTemp==50.0)
		dFacN=2.808;
	else
		dFacN=1.904;   

	/* energy of pulse for normalization:
	   standard values for energy of 1 pulse: E_pulse  */
	dPeriod = _dPeriod/1000.0;   // ms -> s
	dEpulse = _dPower * dPeriod;

	switch(s_nSource)
	{
		case ESS:
			/* maximal accelerator current */
			dCurrMax   = dEpulse / _dPulseLen / U0;
			if (dCurrMax > 0.050)
			{	sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of %4.1f mA", 1000.0*dCurrMax, 1000.0*dCurrLimit);
				Warning(sBuffer);
			}

			if (s_nModType==MULT_SPEC)
			{	/* one side cold; Phi8 = integration of 3*Phi6 */
				stMInfo[imod][0].dF001 = 2.3e11;
				stMInfo[imod][0].dF002 = 9.2e10;
				stMInfo[imod][0].dTemp =  50.0;
				/* one side thermal; Phi7 = integration of 3*Phi3 */
				dFacM = 2.0;
				stMInfo[imod][1].dF001 = 4.5e11;
				stMInfo[imod][1].dF002 = 9.2e10;
				stMInfo[imod][1].dTemp = 325.0;
			}
			else  // coupled
			{	if      (_dTemp== 50.0)
				{  /* Phi8 = integration of 3*Phi6 */
					stMInfo[imod][0].dF001 = 2.3e11;
					stMInfo[imod][0].dF002 = 9.2e10;
				}
				else if (_dTemp==325.0)
				{  /* Phi7 = integration of 3*Phi3 */
					dFacM = 2.0;
					stMInfo[imod][0].dF001 = 4.5e11;
					stMInfo[imod][0].dF002 = 9.2e10;
				}
				else
				{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
				}
			}
			break;

		case SNS:
			switch(s_nModType)
			{
				case POISONED:
					if      (_dTemp== 50.0)
					{  /* Phi4 */
						stMInfo[imod][0].dF001 = 2.7e10;
						stMInfo[imod][0].dF002 = 4.6e10;
					}
					else if (_dTemp==325.0)
					{  /* Phi1 */
						stMInfo[imod][0].dF001 = 9.0e10;
						stMInfo[imod][0].dF002 = 4.6e10;
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case DECOUPLED:
					if      (_dTemp== 50.0)
					{  /* Phi5 */
						stMInfo[imod][0].dF001 = 5.4e10;
						stMInfo[imod][0].dF002 = 9.2e10;
					}
					else if (_dTemp==325.0)
					{  /* Phi2 */
						stMInfo[imod][0].dF001 = 1.8e11;
						stMInfo[imod][0].dF002 = 9.2e10;
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case COUPLED:
					if      (_dTemp== 50.0)
					{  /* Phi6*/
						stMInfo[imod][0].dF001 = 2.3e11;
						stMInfo[imod][0].dF002 = 9.2e10;
					}
					else if (_dTemp==325.0)
					{  /* Phi3*/
						dFacM = 2.0;
						stMInfo[imod][0].dF001 = 4.5e11;
						stMInfo[imod][0].dF002 = 9.2e10;
					}
					else
					{	Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				default:
					Error("Internal error: wrong moderator type in 'TotalFU'");
			}

			/* The cold moderator of the ESS is situated in the favourite upstream position,
			   the thermal moderator in downstream position; at SNS this is vice versa     */
			if (_dTemp==50.0)
			{	stMInfo[imod][0].dF001 = stMInfo[imod][0].dF001/1.3;
				stMInfo[imod][0].dF002 = stMInfo[imod][0].dF002/1.3;
			}
			else
			{	stMInfo[imod][0].dF001 = stMInfo[imod][0].dF001*1.3;
				stMInfo[imod][0].dF002 = stMInfo[imod][0].dF002*1.3;
			}
			break;

		default:
			Error("Internal error: source unknown in 'TotalFU'");
	}
			
	stMInfo[imod][0].dF001 *= (dEpulse/dEp_std);
	stMInfo[imod][0].dF002 *= (dEpulse/dEp_std);
	
	if (s_nModType==MULT_SPEC)
	{	
		stMInfo[imod][1].dF001 *= (dEpulse/dEp_std);
		stMInfo[imod][1].dF002 *= (dEpulse/dEp_std);

		dFUAmpl = (      stMInfo[imod][0].dF001 + dFacN*stMInfo[imod][0].dF002         /* cold    */
		         + dFacM*stMInfo[imod][1].dF001 + dFacN*stMInfo[imod][1].dF002) / 2.0; /* thermal */
	}
	else
	{	dFUAmpl =  dFacM*stMInfo[imod][0].dF001 + dFacN*stMInfo[imod][0].dF002;
	}
	return(dFUAmpl);
}


double EssModFU(const double _dLambda, const double _dTime, const double _dLength)
{
	/* _dLambda    : wavelength                      in Angstroem
	   _dTime      : time (after beginning of pulse) in s
	   _dLength    : pulse length of LPSS  */

	double dFU    =0.0,
	       dPSM=0.0, dPSMC=0.0, dPSMT=0.0, dPSN=0.0,
	       dM  =0.0, dN   =0.0,
	       dTemp=0;

	if (s_nModType!=MULT_SPEC)
		dTemp = stMInfo[imod][0].dTemp;

	switch(s_nSource)
	{
		case ESS:
			if (s_nModType == MULT_SPEC)
			{	dPSMC =  PulseIntEss(_dTime,287e-6          ,20, _dLength);
				dPSMT =  PulseIntEss(_dTime, 80e-6          ,20, _dLength)
				        +PulseIntEss(_dTime,400e-6          ,20, _dLength);
				dPSN  =  PulseIntEss(_dTime, 12e-6*_dLambda, 5, _dLength);
			}
			else
			{	if      (dTemp== 50.0)
				{	/* Phi8 = integration of 3*Phi6 */
					dPSM =  PulseIntEss(_dTime,287e-6          ,20, _dLength);
					dPSN =  PulseIntEss(_dTime, 12e-6*_dLambda, 5, _dLength);
					dN   =  NotMaxwell(_dLambda, 0.9);
				}
				else if (dTemp==325.0)
				{	/* Phi7 = integration of 3*Phi3 */
					dPSM =  PulseIntEss(_dTime, 80e-6          ,20, _dLength)
							 +PulseIntEss(_dTime,400e-6          ,20, _dLength);
					dPSN =  PulseIntEss(_dTime, 12e-6*_dLambda, 5, _dLength);
					dN   =  NotMaxwell(_dLambda, 2.5);
				}
				else
				{	Error("moderator temperature for ESS/SNS must be 50 or 325 K");
				}
			}
			break;

		case SNS:
			switch(s_nModType)
			{
				case POISONED:
					if      (dTemp== 50.0)
					{	/* Phi4 */
						dPSM =  PulseShape(_dTime, 49e-6          , 5);
						dPSN =  PulseShape(_dTime,  7e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9);
					}
					else if (dTemp==325.0)
					{	/* Phi1 */
						dPSM =  PulseShape(_dTime, 22e-6          , 5);
						dPSN =  PulseShape(_dTime,  7e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 2.5);
					}
					else
					{	Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case DECOUPLED:
					if      (dTemp== 50.0)
					{	/* Phi5 */
						dPSM =  PulseShape(_dTime, 78e-6          , 5);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9);
					}
					else if (dTemp==325.0)
					{	/* Phi2 */
						dPSM =  PulseShape(_dTime, 35e-6,           5);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 2.5);
					}
					else
					{	Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case COUPLED:
					if      (dTemp== 50.0)
					{	/* Phi6*/
						dPSM =  PulseShape(_dTime,287e-6          ,20);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9);
					}
					else if (dTemp==325.0)
					{	/* Phi3*/
						dPSM =  PulseShape(_dTime, 80e-6          ,20)
						       +PulseShape(_dTime,400e-6          ,20);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 2.5);
					}
					else
					{	Error("ERROR: moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				default:
					Error("Internal ERROR: wrong moderator type in ModFU()");
			}
			break;

		default:
			Error("Internal ERROR: wrong source in ModFU()");
	}

	if (s_nModType==MULT_SPEC)
	{	double fc, ft;
	
		fc  = f_cold (_dLambda);
		ft  = f_therm(_dLambda);
		dFU =  fc * ( stMInfo[imod][0].dF001 * Maxwellian(_dLambda, stMInfo[imod][0].dTemp) * dPSMC
		            + stMInfo[imod][0].dF002 * NotMaxwell(_dLambda, 0.9)      * dPSN )
		     + ft * ( stMInfo[imod][1].dF001 * Maxwellian(_dLambda, stMInfo[imod][1].dTemp) * dPSMT
		            + stMInfo[imod][1].dF002 * NotMaxwell(_dLambda, 2.5)      * dPSN );
	}
	else
	{	
		dM  =  Maxwellian(_dLambda, dTemp);
		dFU =  stMInfo[imod][0].dF001 * dM * dPSM
		     + stMInfo[imod][0].dF002 * dN * dPSN;
	}

	return(dFU);
}




double Maxwellian(const double _dLambda, const double _dModTemp)
{
	/* _dLambda : Wavelength                 in Angstroem */
	/* _dModTemp: eff. moderator temperature in Kelvin    */

	double dM=0.0, dFakt, dA;

	if (_dModTemp > 0.0  &&  _dLambda > 0.0)
	{
		dFakt = pow(1e10*H, 2) / (2*K*MN);            /* Fakt = h²/(2*k*m_n)  in (1E-10 m)²K */
		dA    = dFakt / _dModTemp;
		
		dM      = 2 * pow(dA,2) * exp(-dA / pow(_dLambda,2)) / pow(_dLambda,5) ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in Maxwellian(): Lambda = %10.4e Ang, Temp = %9.3e K\n",
		                   _dLambda, _dModTemp);
		exit(99);
	}

	return dM;
}


double NotMaxwell(const double _dLambda, const double _dParam)
{
	/* _dLambda : Wavelength           in Angstroem    */
	/* _dParam  : line shape parameter in 1/Angstroem  */

	double dN=0.0;
	
	if (_dLambda > 0.0)
	{
		dN = 1.0 / (1.0 + exp(_dParam*_dLambda-2.2)) / _dLambda ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in NotMaxwell(): Lambda = %10.4e Ang\n",
		                   _dLambda);
		exit(99);
	}

	return dN;
}


double UserLmbdTimeDis(const double _dLambda, const double _dTime)
{
	double dUd=0.0, dLUd, dTime, dDelX, dDelY,
	       dLUij, dLUi1j, dLUij1, dLUi1j1, dLUi, dLUi1;
	short  i=0, j=0;

	dTime = _dTime*1000;   /*  time in milliseconds  */

	while (i+1 < stFluxLT[imod].nLines  &&  stFluxLT[imod].pTabX[i+1] < dTime)
	{	i++;
	}
	while (j+1 < stFluxLT[imod].nColumns && stFluxLT[imod].pTabY[j+1] < _dLambda)
	{	j++;
	}

	if (j+1 < stFluxLT[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		dLUij   = stFluxLT[imod].pTabF[IndLT(i  ,j  )];
		dLUij1  = stFluxLT[imod].pTabF[IndLT(i  ,j+1)];
		dLUi1j  = stFluxLT[imod].pTabF[IndLT(i+1,j  )];
		dLUi1j1 = stFluxLT[imod].pTabF[IndLT(i+1,j+1)];

		dDelX   = stFluxLT[imod].pTabX[i+1] - stFluxLT[imod].pTabX[i];
		dDelY   = stFluxLT[imod].pTabY[j+1] - stFluxLT[imod].pTabY[j];

		dLUi    = dLUij  + (dLUij1 - dLUij)  / dDelY * (_dLambda - stFluxLT[imod].pTabY[j]);
		dLUi1   = dLUi1j + (dLUi1j1- dLUi1j) / dDelY * (_dLambda - stFluxLT[imod].pTabY[j]);
		dLUd    = dLUi   + (dLUi1  - dLUi)   / dDelX * (dTime     - stFluxLT[imod].pTabX[i]);
		dUd     = exp(dLUd);
	}
	else
	/* read error: time or wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_LT_RANGE_TOO_SMALL);
	}

	return dUd;
}


double UserLambdaDis(const double _dLambda, const double _dModTemp)
{
	double dUd=0.0, dLUd, dLUdN, dLUdN1;
	short  n=0;

	while (n+1 < stFluxL[imod].nLines  &&  stFluxL[imod].pTabX[n+1] < _dLambda)
	{	n++;
	}

	if (n+1 < stFluxL[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		dLUdN  = stFluxL[imod].pTabF[n];
		dLUdN1 = stFluxL[imod].pTabF[n+1];
		dLUd   = dLUdN  +  (dLUdN1-dLUdN ) / (stFluxL[imod].pTabX[n+1] - stFluxL[imod].pTabX[n])
		                                   * (_dLambda                - stFluxL[imod].pTabX[n]);
		dUd    = exp(dLUd);
	}
	else
	/* read error: wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_L_RANGE_TOO_SMALL);
	}

	return dUd;
}

double UserTimeDis(const double _dTime, const double _dTau, const double _dTauRatio, const double _dPLength)
{
	double dUd=0.0, dLUd, dLUdN, dLUdN1, dTime;
	short  n=0;

	dTime = _dTime*1000;   /*  time in milliseconds  */

	while (n+1 < stFluxT[imod].nLines  &&  stFluxT[imod].pTabX[n+1] < dTime)
	{	n++;
	}

	if (n+1 < stFluxT[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		dLUdN  = stFluxT[imod].pTabF[n];
		dLUdN1 = stFluxT[imod].pTabF[n+1];
		dLUd   = dLUdN  +  (dLUdN1 - dLUdN ) / (stFluxT[imod].pTabX[n+1] - stFluxT[imod].pTabX[n])
		                                     * (dTime                    - stFluxT[imod].pTabX[n]);
		dUd    = exp(dLUd);
	}
	else
	/* read error: time larger than all values in the distribution file */
	{	CountMessage(SRC_T_RANGE_TOO_SMALL);
	}

	return dUd;
}


double PulseShapeP(const double _dTime, const double _dTauDecay, const double _dTauRatio, const double _dPLength)
{
   return PulseShape(_dTime, _dTauDecay, _dTauRatio);
}


double PulseShape(const double _dTime, const double _dTau, const double _dTauRatio)
{
	/* _dTime     : time (after beginning of Pulse) in sec */
	/* _dTau      : decay time constant             in sec */
	/* _dTauRatio : ratio decay of pulse : ascent of pulse   */

	double dF = 0.0,
	       dTauDecay  = _dTau,             // time constant for decay of pulse  in sec 
	       dTauAscent = _dTau/_dTauRatio;  // time constant for ascent of pulse in sec 

	if (_dTime >= 0.0  && _dTau > 0.0  &&  _dTauRatio > 1.0)
	{
		dF = (exp(-_dTime/dTauDecay) - exp(-_dTime/dTauAscent)) / (dTauDecay - dTauAscent);
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShape(): Time = %10.4e s,  TauDecay = %10.4e s,  n = %6.2f\n",
		                   _dTime, _dTau, _dTauRatio);
		exit(99);
	}

	return dF;
}


double PulseIntEss(const double _dTime, const double _dTauD, const double _dTauRatio, const double _dLength)
{
	/* _dTime     : time (after beginning of Pulse) in sec */
	/* _dTauD     : decay time constant             in sec */
	/* _dTauRatio : ratio decay of pulse : ascent of pulse */
	/* _dPLength  : pulse length                    in sec */

	double dInt = 0.0,
	       dTauDecay  = _dTauD,             // time constant for decay of pulse  in sec 
	       dTauAscent = _dTauD/_dTauRatio;  // time constant for ascent of pulse in sec 

	if (_dTime >= 0.0  &&  _dTauD > 0.0  &&  _dTauRatio > 1.0  &&  _dLength >= 0.0)
	{
		if (_dTime <= _dLength)
		{
			dInt = (  PulseShapeInt(_dTime,          dTauDecay, dTauAscent) + 1.0) / _dLength;
		}
		else
		{
			dInt = (  PulseShapeInt(_dTime,          dTauDecay, dTauAscent)
			        - PulseShapeInt(_dTime-_dLength, dTauDecay, dTauAscent) ) / _dLength;
		}
	}
	else
	{  fprintf(LogFilePtr,"ERROR: wrong parameter in PulseIntEss(): Time = %10.4e s,  Tau_d = %10.4e s,  Pulse = %10.4e s   n = %6.2f\n",
		                   _dTime, _dTauD, _dLength, _dTauRatio);
	   exit(99);
	}

	return dInt;
}


double PulseShapeInt(const double _dTime, const double _dTauDecay, const double _dTauAscent)
{
	/* _dTimeS     : time (after beginning of pulse) in sec */
	/* _dTauDecayS : decay time constant             in sec */
	/* _dTauAscent : ascent time constant            in sec */

	double dF = 0.0,
	       dN;

	if (_dTauAscent > 0.0  &&  _dTauDecay > _dTauAscent  &&  _dTime >= 0.0)
	{
		dN = _dTauDecay / _dTauAscent;
		dF = (exp(-_dTime/_dTauAscent) - dN*exp(-_dTime/_dTauDecay)) / (dN-1.0);
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShapeInt(): Time = %10.4e s,  Tau_d = %10.4e s,  Tau_a = %10.4e s \n",
		                   _dTime, _dTauDecay, _dTauAscent);
		exit(99);
	}

	return dF;
}



/* functions to calculate the portion of flux that is fed into the guide by a special beam extraction
   system for the cold source (f_cold) and the thermal source (f_therm) of a multi-spectral moderator */

double f_cold(const double _dLambda)
{
	/* _dLambda : Wavelength           in Angstroem    */

	if (_dLambda > 1.4)
		return(0.95 - f_therm(_dLambda));
	else
		return 0.0;
}

double f_therm(const double _dLambda)
{
	/* _dLambda : Wavelength           in Angstroem    */

	double f=0.0, dLmbd4;

	dLmbd4 = pow(_dLambda,4);
	f = 0.86 * exp(-0.01677*dLmbd4) + 0.14*(1 -pow(dLmbd4/(dLmbd4+40000),0.25));

	return f;
}



/* 'AveSolidAngleC',	'AveSolidAngleR'
	
   These functions calculate the average solid angle of the window
   seen from the moderator area:
   The solid angle is: Omega(y,z) =  [atan((w-y)/D) - atan((-w-y)/D)]
                                   * [atan((h-z)/D) - atan((-h-z)/D)]
   w: window width, h: window height, D distance moderator - window

   integration of  atan(x/D)  yields  x*atan(x/D) - D*ln(D²+x²)/2
   integration over rectangular moderator area yields
    I_ges = (I1 -I2) * (I3 - I4) with I2 = I1, I4=I3

   for the circular moderator only an approximation is calculated  */

double AveSolidAngleC(const double dModDiameter,
                      const double dWndWidth, const double dWndHeight, const double dDist)
{
	double dOmAr  =0.0,
	       dOmega =0.0,
	       dModY0, dModZ0, dWndY0, dWndZ0, I1, I3;

	dModY0 = 0.25*dModDiameter*sqrt(M_PI);
	dModZ0 = dModY0;
	dWndY0 = 0.5* dWndWidth;
	dWndZ0 = 0.5* dWndHeight;

	I1 = IntAtan( dWndY0-dModY0,  dWndY0+dModY0, dDist);
	I3 = IntAtan( dWndZ0-dModZ0,  dWndZ0+dModZ0, dDist);
	
	dOmAr  = 4 * I1 * I3;
	dOmega = dOmAr / (M_PI * sq(dModDiameter/2.0) );

	return dOmega;
}

double AveSolidAngleR(const double dModWidth, const double dModHeight,
                      const double dWndWidth, const double dWndHeight, const double dDist)
{
	double dOmAr  =0.0,
	       dOmegaQ=0.0,
	       dModY0, dModZ0, dWndY0, dWndZ0, I1, I3;

	dModY0 = 0.5* dModWidth;
	dModZ0 = 0.5* dModHeight;
	dWndY0 = 0.5* dWndWidth;
	dWndZ0 = 0.5* dWndHeight;

	I1 = IntAtan( dWndY0-dModY0,  dWndY0+dModY0, dDist);
	I3 = IntAtan( dWndZ0-dModZ0,  dWndZ0+dModZ0, dDist);
	/* whole integral (I1 -I2) * (I3 - I4) */
	dOmAr   = 4 * I1 * I3;

	dOmegaQ = dOmAr / (dModWidth * dModHeight);

	return dOmegaQ;
}

/* 'AveWeightC', 'AveWeightR'

   For 'DirectionByWindow' trajectories have to be normalized by f=cos²(phi)*cos²(theta).
   These functions 'AveWeightC' and 'AveWeightR' calculate the average normalization factors
   by integration over window area and over moderator area.
   The resulting factor F is included in the main program to give correct absolute flux values.
*/
double AveWeightC(const double dModCntrY, const double dModCntrZ,  const double dModDiam,
                  const double dWndWidth, const double dWndHeight, const double dDist)
{	
	double y, z, dFact, dSum=0.0, dWy=0.0, dWz=0.0;
	long   num=0;

	/* numerical integration of 'weight' (see below) over moderator area */
	for (y=dModCntrY-0.495*dModDiam; y<=dModCntrY+0.5*dModDiam; y+= dModDiam/100.0)
	{	dWy = WeightDirByWnd(dWndWidth, dDist, y);
		
		for (z=dModCntrZ-0.495*dModDiam; z<=dModCntrZ+0.5*dModDiam; z+= dModDiam/100.0)
		{	
			/* count if (y,z) within moderator */
			if (sq(y-dModCntrY) + sq(z-dModCntrZ) <= sq(0.5*dModDiam))
			{	dWz = WeightDirByWnd(dWndHeight, dDist, z);
				dSum+= dWy*dWz;
				num ++;
			}
		}
	}
	dFact = dSum / num;

	return dFact;
}

double AveWeightR(const double dModCntrY, const double dModCntrZ,
                  const double dModWidth, const double dModHeight,
                  const double dWndWidth, const double dWndHeight, const double dDist)
{	
	double dFactY, dFactZ;

	/* integration of 'weight' (see below) over moderator width yields
	   F1 = dist² / (wnd_width*mod_width)
	              * ( IntAtan(tan(phi_min)...tan(phimax) for wnd_begin)
	                 -IntAtan(tan(phi_min)...tan(phimax) for wnd_end)       (same for height) */
	dFactY =  pow(dDist,2) / (dModWidth*dWndWidth)
	       * ( IntAtan((-0.5*dWndWidth -(dModCntrY-0.5*dModWidth ))/dDist, (-0.5*dWndWidth -(dModCntrY+0.5*dModWidth ))/dDist, 1)
	         - IntAtan(( 0.5*dWndWidth -(dModCntrY-0.5*dModWidth ))/dDist, ( 0.5*dWndWidth -(dModCntrY+0.5*dModWidth ))/dDist, 1));
	dFactZ =  pow(dDist,2) / (dModHeight*dWndHeight)
	       * ( IntAtan((-0.5*dWndHeight-(dModCntrZ-0.5*dModHeight))/dDist, (-0.5*dWndHeight-(dModCntrZ+0.5*dModHeight))/dDist, 1)
	         - IntAtan(( 0.5*dWndHeight-(dModCntrZ-0.5*dModHeight))/dDist, ( 0.5*dWndHeight-(dModCntrZ+0.5*dModHeight))/dDist, 1));

	return dFactY*dFactZ;
}


/* average of factor cos²(x) integrated over window width for a fixed moderator position
   is f1 = dist / wnd_width * (max_angle - min_angle)       (same for height)  */
double WeightDirByWnd(const double dWndSize, const double dDist, const double dModPos)
{
	double dInt, dAngleMax=0.0, dAngleMin=0.0;

	dAngleMax = atan(( 0.5*dWndSize - dModPos)/dDist);
	dAngleMin = atan((-0.5*dWndSize - dModPos)/dDist);
	dInt =  dDist/dWndSize * (dAngleMax-dAngleMin) ;

	return dInt;
}


/* integration of atan(x/a) from x=beg to x=end  (a: param) */
double IntAtan(const double dIntBeg, const double dIntEnd, const double dParam)
{
	double dInt=0.0;

	dInt =  (dIntEnd * atan(dIntEnd/dParam) - 0.5*dParam * log(sq(dParam) + sq(dIntEnd)))
	      - (dIntBeg * atan(dIntBeg/dParam) - 0.5*dParam * log(sq(dParam) + sq(dIntBeg)));

	return dInt;
}
