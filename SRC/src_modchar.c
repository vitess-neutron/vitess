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
       
       

static short  s_nSourceType=CWS,   /* s_nSourceType: CWS, SPSS, LPSS                                   */
              s_nSource=ANYSOURCE, /* s_nSource    : ANYSOURCE, ESS, SNS                               */
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
double TotalFU(const double p_dTemp,  
               const short  p_nSourceType, const short  p_nSource, const short  p_nModType)
{
	/* p_dTemp      : eff. moderator temperature      in Kelvin    
	   p_nSourceType: CWS, SPSS, LPSS                              
	   p_nSource    : ESS, SNS,                                          
	   p_nModType   : decoupled POISONED, DECOUPLED unpoisened, COUPLED */

	double dFUAmpl=0.0, 
	       dFacM=1.0,      /* number of time dist. functions */
			 dFacN=1.904;    /* integral of fct. N(lambda) in 0.1 Ang .... 20 Ang */

	/* initialize */
	memset(stMInfo[imod], '\0', sizeof(ModInfo));

	s_nSourceType = p_nSourceType;
	s_nSource     = p_nSource;
	s_nModType    = p_nModType;
	if (s_nModType!=MULT_SPEC)
		stMInfo[imod][0].dTemp = p_dTemp;   

	if (p_dTemp==50.0)
		dFacN=2.808; 

	switch(s_nSource)
	{
		case ESS: 
		case SNS: 
			switch(s_nSourceType)
			{
				case LPSS:
					if (s_nModType==MULT_SPEC)
					{	/* one side cold; Phi8 = integration of 3*Phi6 */
						stMInfo[imod][0].dF001 = 3.0*2.3e11;
						stMInfo[imod][0].dF002 = 3.0*9.2e10;
						stMInfo[imod][0].dTemp =  50.0;
						/* one side thermal; Phi7 = integration of 3*Phi3 */
						dFacM = 2.0;
						stMInfo[imod][1].dF001 = 3.0*4.5e11;
						stMInfo[imod][1].dF002 = 3.0*9.2e10;
						stMInfo[imod][1].dTemp = 325.0;
					}
					else
					{	if      (p_dTemp== 50.0)
						{  /* Phi8 = integration of 3*Phi6 */
							stMInfo[imod][0].dF001 = 3.0*2.3e11;
							stMInfo[imod][0].dF002 = 3.0*9.2e10;
						}
						else if (p_dTemp==325.0)
						{  /* Phi7 = integration of 3*Phi3 */
							dFacM = 2.0;
							stMInfo[imod][0].dF001 = 3.0*4.5e11;
							stMInfo[imod][0].dF002 = 3.0*9.2e10;
						}
						else
						{  Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
						}   
					}
					break;

				case SPSS: 
					switch(s_nModType)
					{
						case POISONED: 
							if      (p_dTemp== 50.0)
							{  /* Phi4 */
								stMInfo[imod][0].dF001 = 2.7e10;
								stMInfo[imod][0].dF002 = 4.6e10;
							}
							else if (p_dTemp==325.0)
							{  /* Phi1 */
								stMInfo[imod][0].dF001 = 9.0e10;
								stMInfo[imod][0].dF002 = 4.6e10;
							}
							else
							{  Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case DECOUPLED: 
							if      (p_dTemp== 50.0)
							{  /* Phi5 */
								stMInfo[imod][0].dF001 = 5.4e10;
								stMInfo[imod][0].dF002 = 9.2e10;
							}
							else if (p_dTemp==325.0)
							{  /* Phi2 */
								stMInfo[imod][0].dF001 = 1.8e11;
								stMInfo[imod][0].dF002 = 9.2e10;
							}
							else
							{  Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case COUPLED: 
							if      (p_dTemp== 50.0)
							{  /* Phi6*/
								stMInfo[imod][0].dF001 = 2.3e11;
								stMInfo[imod][0].dF002 = 9.2e10;
							}
							else if (p_dTemp==325.0)
							{  /* Phi3*/
								dFacM = 2.0;
								stMInfo[imod][0].dF001 = 4.5e11;
								stMInfo[imod][0].dF002 = 9.2e10;
							}
							else
							{	Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case MULT_SPEC: 
							/* left side cold; Phi4 */
							stMInfo[imod][0].dF001 = 2.3e11;
							stMInfo[imod][0].dF002 = 9.2e10;
							stMInfo[imod][0].dTemp =  50.0;
							/* right side warm; Phi1 */
							dFacM = 2.0;
							stMInfo[imod][1].dF001 = 4.5e11;
							stMInfo[imod][1].dF002 = 9.2e10;
							stMInfo[imod][1].dTemp = 325.0;
							break;

						default:
							Error("Internal error: wrong moderator type in 'TotalFU'"); 
					}   
					break;
			}  
 
			if (s_nSource==SNS)
			{	/* SNS power is a factor 2.5 less than that of ESS, giving a factor of 3 per pulse.
				   The cold moderator of the ESS is situated in the favourite upstream position, 
				   the thermal moderator in downstream position; at SNS this is vice versa     */
				if (s_nModType==MULT_SPEC)
				{	stMInfo[imod][0].dF001 = stMInfo[imod][0].dF001/3.0/1.3; /* cold part */
					stMInfo[imod][0].dF002 = stMInfo[imod][0].dF002/3.0/1.3; 
					stMInfo[imod][1].dF001 = stMInfo[imod][1].dF001/3.0*1.3; /* thermal part */
					stMInfo[imod][1].dF002 = stMInfo[imod][1].dF002/3.0*1.3;
				}
				else
				{	if (p_dTemp==50.0)
					{	stMInfo[imod][0].dF001 = stMInfo[imod][0].dF001/3.0/1.3;
						stMInfo[imod][0].dF002 = stMInfo[imod][0].dF002/3.0/1.3;
					}
					else
					{	stMInfo[imod][0].dF001 = stMInfo[imod][0].dF001/3.0*1.3;
						stMInfo[imod][0].dF002 = stMInfo[imod][0].dF002/3.0*1.3;
					}
				}
			}

			if (s_nModType==MULT_SPEC)
				dFUAmpl = (      stMInfo[imod][0].dF001 + dFacN*stMInfo[imod][0].dF002	      /* cold    */
				         + dFacM*stMInfo[imod][1].dF001 + dFacN*stMInfo[imod][1].dF002) / 2.0; /* thermal */
			else
				dFUAmpl =  dFacM*stMInfo[imod][0].dF001 + dFacN*stMInfo[imod][0].dF002;

			break;

		default:
			Error("Internal error: source unknown in 'TotalFU'"); 
	}

	return(dFUAmpl);
}


double EssModFU(const double p_dLambda, const double p_dTime, const double p_dLength)
{
	/* p_dLambda    : wavelength                      in Angstroem 
	   p_dTime      : time (after beginning of pulse) in s       
	   p_dLength    : pulse length of LPSS            in s    
	   p_eSide      : side  (left or right) */

	double dFU    =0.0,
	       dPSM=0.0, dPSMC=0.0, dPSMT=0.0, dPSN=0.0,
	       dM  =0.0, dN   =0.0, 
	       dTemp;

	if (s_nModType!=MULT_SPEC)
		dTemp = stMInfo[imod][0].dTemp;

	switch(s_nSourceType)
	{
		case LPSS:
			switch(s_nSource)
			{
				case ESS:
				case SNS: 
					if (s_nModType == MULT_SPEC) 
					{	dPSMC =  PulseIntEss(p_dTime,287e-6          ,20, p_dLength);
						dPSMT =  PulseIntEss(p_dTime, 80e-6          ,20, p_dLength)
						        +PulseIntEss(p_dTime,400e-6          ,20, p_dLength); 
						dPSN  =  PulseIntEss(p_dTime, 12e-6*p_dLambda, 5, p_dLength);
					}
					else
					{	if      (dTemp== 50.0)
						{	/* Phi8 = integration of 3*Phi6 */
							dPSM =  PulseIntEss(p_dTime,287e-6          ,20, p_dLength);
							dPSN =  PulseIntEss(p_dTime, 12e-6*p_dLambda, 5, p_dLength);
							dN   =  NotMaxwell(p_dLambda, 0.9);
						}
						else if (dTemp==325.0)
						{	/* Phi7 = integration of 3*Phi3 */
							dPSM =  PulseIntEss(p_dTime, 80e-6          ,20, p_dLength)
									 +PulseIntEss(p_dTime,400e-6          ,20, p_dLength); 
							dPSN =  PulseIntEss(p_dTime, 12e-6*p_dLambda, 5, p_dLength);
							dN   =  NotMaxwell(p_dLambda, 2.5); 
						}
						else
						{	Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
						}   
					}
					break;

				default:
					Error("Internal ERROR: wrong source in ModFU()"); 
			}   
			break;

		case SPSS: 
			switch(s_nSource)
			{
				case ESS: 
				case SNS: 
					switch(s_nModType)
					{
						case POISONED: 
							if      (dTemp== 50.0)
							{	/* Phi4 */
								dPSM =  PulseShapeEss(p_dTime, 49e-6          , 5);
								dPSN =  PulseShapeEss(p_dTime,  7e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 0.9);
							}
							else if (dTemp==325.0)
							{	/* Phi1 */
								dPSM =  PulseShapeEss(p_dTime, 22e-6          , 5);
								dPSN =  PulseShapeEss(p_dTime,  7e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 2.5);
							}
							else
							{	Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case DECOUPLED: 
							if      (dTemp== 50.0)
							{	/* Phi5 */
								dPSM =  PulseShapeEss(p_dTime, 78e-6          , 5);
								dPSN =  PulseShapeEss(p_dTime, 12e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 0.9);
							}
							else if (dTemp==325.0)
							{	/* Phi2 */
								dPSM =  PulseShapeEss(p_dTime, 35e-6,           5);
								dPSN =  PulseShapeEss(p_dTime, 12e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 2.5);
							}
							else
							{	Error("moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case COUPLED: 
							if      (dTemp== 50.0)
							{	/* Phi6*/
								dPSM =  PulseShapeEss(p_dTime,287e-6          ,20);
								dPSN =  PulseShapeEss(p_dTime, 12e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 0.9);
							}
							else if (dTemp==325.0)
							{	/* Phi3*/
								dPSM =  PulseShapeEss(p_dTime, 80e-6          ,20)
								       +PulseShapeEss(p_dTime,400e-6          ,20); 
								dPSN =  PulseShapeEss(p_dTime, 12e-6*p_dLambda, 5);
								dN   =  NotMaxwell(p_dLambda, 2.5); 
							}
							else
							{	Error("ERROR: moderator temperature for ESS/SNS must be 50 or 325 K"); 
							}   
							break;

						case MULT_SPEC: 
							dPSMC =  PulseShapeEss(p_dTime,287e-6          ,20);
							dPSMT =  PulseShapeEss(p_dTime, 80e-6          ,20)
								    + PulseShapeEss(p_dTime,400e-6          ,20); 
							dPSN  =  PulseShapeEss(p_dTime, 12e-6*p_dLambda, 5);
							break;

						default:
							Error("Internal ERROR: wrong moderator type in ModFU()"); 
					}   
					break;
      
				default:
					Error("Internal ERROR: wrong source in ModFU()"); 
			}   
			break;

		default:
			Error("Internal ERROR: wrong source type in ModFU()"); 
	}

	if (s_nModType==MULT_SPEC)
	{	double fc, ft;
	
		fc  = f_cold (p_dLambda);
		ft  = f_therm(p_dLambda);
		dFU =  fc * ( stMInfo[imod][0].dF001 * Maxwellian(p_dLambda, stMInfo[imod][0].dTemp) * dPSMC
		            + stMInfo[imod][0].dF002 * NotMaxwell(p_dLambda, 0.9)      * dPSN )
		     + ft * ( stMInfo[imod][1].dF001 * Maxwellian(p_dLambda, stMInfo[imod][1].dTemp) * dPSMT
		            + stMInfo[imod][1].dF002 * NotMaxwell(p_dLambda, 2.5)      * dPSN );
	}
	else
	{	
		dM  =  Maxwellian(p_dLambda, dTemp);
		dFU =  stMInfo[imod][0].dF001 * dM * dPSM
			  + stMInfo[imod][0].dF002 * dN * dPSN;
	}

	return(dFU);
}



double UserLambdaDis(const double p_dLambda, const double p_dModTemp)
{
	double dUd=0.0, dLUd, dLUdN, dLUdN1;
	short  n=0;

	while (n+1 < stFluxL[imod].nLines  &&  stFluxL[imod].pTabX[n+1] < p_dLambda) 
	{	n++;
	}

	if (n+1 < stFluxL[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		dLUdN  = stFluxL[imod].pTabF[n];
		dLUdN1 = stFluxL[imod].pTabF[n+1];
		dLUd   = dLUdN  +  (dLUdN1-dLUdN ) / (stFluxL[imod].pTabX[n+1] - stFluxL[imod].pTabX[n]) 
		                                   * (p_dLambda                - stFluxL[imod].pTabX[n]);
		dUd    = exp(dLUd);
	}
	else
	/* read error: wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_L_RANGE_TOO_SMALL);
	}

	return dUd;
}


double Maxwellian(const double p_dLambda, const double p_dModTemp)
{
	/* p_dLambda : Wavelength                 in Angstroem */
	/* p_dModTemp: eff. moderator temperature in Kelvin    */

	double dM=0.0, dFakt, dA;

	if (p_dModTemp > 0.0  &&  p_dLambda > 0.0)
	{
		dFakt = pow(1e10*H, 2) / (2*K*MN);            /* Fakt = h²/(2*k*m_n)  in (1E-10 m)²/K */
		dA    = dFakt / p_dModTemp;
		   
		dM      = 2 * pow(dA,2) * exp(-dA / pow(p_dLambda,2)) / pow(p_dLambda,5) ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in Maxwellian(): Lambda = %10.4e Ang, Temp = %9.3e K\n",
		                   p_dLambda, p_dModTemp); 
		exit(99);
	}   

	return dM;
}


double NotMaxwell(const double p_dLambda, const double p_dParam)
{
	/* p_dLambda : Wavelength           in Angstroem    */
	/* p_dParam  : line shape parameter in 1/Angstroem  */

	double dN=0.0;
	   
	if (p_dLambda > 0.0)
	{
		dN = 1.0 / (1.0 + exp(p_dParam*p_dLambda-2.2)) / p_dLambda ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in NotMaxwell(): Lambda = %10.4e Ang\n",
		                   p_dLambda); 
		exit(99);
	}   

	return dN;
}


double UserLmbdTimeDis(const double p_dLambda, const double p_dTime)
{
	double dUd=0.0, dLUd, dTime, dDelX, dDelY,
	       dLUij, dLUi1j, dLUij1, dLUi1j1, dLUi, dLUi1;
	short  i=0, j=0;

	dTime = p_dTime*1000;   /*  time in milliseconds  */

	while (i+1 < stFluxLT[imod].nLines  &&  stFluxLT[imod].pTabX[i+1] < dTime) 
	{	i++;
	}
	while (j+1 < stFluxLT[imod].nColumns && stFluxLT[imod].pTabY[j+1] < p_dLambda) 
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

		dLUi    = dLUij  + (dLUij1 - dLUij)  / dDelY * (p_dLambda - stFluxLT[imod].pTabY[j]);
		dLUi1   = dLUi1j + (dLUi1j1- dLUi1j) / dDelY * (p_dLambda - stFluxLT[imod].pTabY[j]);
		dLUd    = dLUi   + (dLUi1  - dLUi)   / dDelX * (dTime     - stFluxLT[imod].pTabX[i]);
		dUd     = exp(dLUd);
	}
	else
	/* read error: time or wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_LT_RANGE_TOO_SMALL);
	}

	return dUd;
}


double UserTimeDis(const double p_dTime, const double p_dTauDecay, const double p_dTauAscent,
                   const double p_dPLength)
{
	double dUd=0.0, dLUd, dLUdN, dLUdN1, dTime;
	short  n=0;

	dTime = p_dTime*1000;   /*  time in milliseconds  */

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


double PulseShape(const double p_dTime, const double p_dTauDecay, const double p_dTauAscent,
                  const double dPLength)
{
	/* p_dTime      : time (after beginning of Pulse)   in sec */
	/* p_dTauDecay  : time constant for decay of pulse  in sec */
	/* p_dTauAscent : time constant for ascent of pulse in sec */

	double dF = 0.0; 

	if (p_dTauAscent > 0.0  &&  p_dTauDecay > p_dTauAscent  &&  p_dTime >= 0.0)
	{
		dF = (exp(-p_dTime/p_dTauDecay) - exp(-p_dTime/p_dTauAscent)) / (p_dTauDecay - p_dTauAscent);
	}
	else
	{  
		fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShape(): Time = %10.4e s,  TauDecay = %10.4e s,  TauAscent = %10.4e s\n",
		                   p_dTime, p_dTauDecay, p_dTauAscent); 
		exit(99);
	}   

	return dF;
}


double PulseInt(const double p_dTime, const double p_dTauDecay, const double p_dTauAscent,
                const double p_dLength)
{
	/* p_dTime      : time (after beginning of Pulse) in sec */
	/* p_dTauDecay  : decay time constant             in sec */
	/* p_dTauAscent : ascent time constant            in sec */
	/* p_dPLength   : pulse length                    in sec */

	double dInt = 0.0; 

	if (p_dTauAscent > 0.0  &&  p_dTauDecay > p_dTauAscent  &&  p_dTime >= 0.0  &&  p_dLength >= 0.0)
	{
		if (p_dTime <= p_dLength)
		{  
			dInt = (  PulseShapeInt(p_dTime,           p_dTauDecay, p_dTauAscent) + 1.0) / p_dLength;
		}
		else
		{  
			dInt = (  PulseShapeInt(p_dTime,           p_dTauDecay, p_dTauAscent)
			        - PulseShapeInt(p_dTime-p_dLength, p_dTauDecay, p_dTauAscent) ) / p_dLength;
		}
	}
	else
	{  fprintf(LogFilePtr,"ERROR: wrong parameter in PulseInt(): Time = %10.4e s,  Tau_d = %10.4e s,  Tau_a = %10.4e s,  Pulse = %10.4e s\n",
		                   p_dTime, p_dTauDecay, p_dTauAscent, p_dLength); 
	   exit(99);
	}   

	return dInt;
}


double PulseShapeInt(const double p_dTime, const double p_dTauDecay, const double p_dTauAscent)
{
	/* p_dTimeS     : time (after beginning of pulse) in sec */
	/* p_dTauDecayS : decay time constant             in sec */
	/* p_dTauAscent : ascent time constant            in sec */

	double dF = 0.0, 
	       dN; 

	if (p_dTauAscent > 0.0  &&  p_dTauDecay > p_dTauAscent  &&  p_dTime >= 0.0)
	{
		dN = p_dTauDecay / p_dTauAscent;

		dF = (exp(-p_dTime/p_dTauAscent) - dN*exp(-p_dTime/p_dTauDecay)) / (dN-1.0);
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShapeInt(): Time = %10.4e s,  Tau_d = %10.4e s,  Tau_a = %10.4e s \n",
		                   p_dTime, p_dTauDecay, p_dTauAscent); 
		exit(99);
	}   

	return dF;
}



double PulseShapeEss(const double p_dTime, const double p_dTau, const short p_nTauRatio)
{
	/* p_dTime     : time (after beginning of Pulse) in sec */
	/* p_dTau      : decay time constant             in sec */
	/* p_dTauRatio : ratio decay of pulse : ascent of pulse   */

	double dF = 0.0, 
	       dN; 

	if (p_nTauRatio>=2)
	{
		dN = (double) p_nTauRatio;

		dF = PulseShape(p_dTime, p_dTau, p_dTau/dN, 0.0) ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShapeEss(): n = %d\n", p_nTauRatio); 
		exit(99);
	}

	return dF;
}


double PulseIntEss(const double p_dTime, const double p_dTauD, const short p_nTauRatio,
                   const double p_dLength)
{
	/* p_dTime     : time (after beginning of Pulse) in sec */
	/* p_dTauD     : decay time constant             in sec */
	/* p_dTauRatio : ratio decay of pulse : ascent of pulse */
	/* p_dPLength  : pulse length                    in sec */

	double dInt = 0.0, 
	       dN; 

	if (p_nTauRatio>=2)
	{
		dN   = (double) p_nTauRatio;

		dInt = PulseInt(p_dTime, p_dTauD, p_dTauD/dN,  p_dLength) ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShapeEss(): n = %d\n", p_nTauRatio); 
		exit(99);
	}

	return dInt;
}



/* functions to calculate the portion of flux that is fed into the guide by a special beam extraction
   system for the cold source (f_cold) and the thermal source (f_therm) of a multi-spectral moderator */

double f_cold(const double dLambda)
{
	/* p_dLambda : Wavelength           in Angstroem    */

	if (dLambda > 1.4)
		return(0.95 - f_therm(dLambda));
	else
		return 0.0;
}

double f_therm(const double dLambda)
{
	/* p_dLambda : Wavelength           in Angstroem    */

	double f=0.0, dLmbd4;

	dLmbd4 = pow(dLambda,4);
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
