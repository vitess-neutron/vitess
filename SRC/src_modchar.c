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


/* global variables */
/* ---------------- */
extern
FctTable  stFluxT[NUM_MOD],  /* data of time distr.            */
          stFluxL[NUM_MOD],  /* data of wavelength distr.      */
          stFluxLT[NUM_MOD]; /* data of wavelength-time distr. */
extern
short     iDataVsn,          /* version of the data base for the source characteristics */
          nNumMod,           /* number of moderators in the moderator system            */
          imod;              /* index of moderators in the moderator system             */
extern
Moderator stMod  [NUM_MOD];  /* moderator data            */


/* static variables */
/* ---------------- */
static
ModInfo   stMInfo[NUM_MOD][2]; /* additional moderator data
                                  multispec. moder.: index 0 for cold part, 1 for thermal part
                                  otherwise        : only index 0 used */



static short  s_nSource=ANYSOURCE, /* s_nSource    : ANYSOURCE, ESS, SNS, CSNS                         */
              s_nModType=0;        /* s_nModType   : decoupled POISONED, DECOUPLED unpoisened, COUPLED */


// double NewMaxwell (const double _lambda, const double _temp);
double EmpCorrFact(double lmbd);


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
	   _dPulseLen  : [s]  pulse length                             */
  short  rc;
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
	memset(stMInfo[imod], '\0', 2*NUM_MOD*sizeof(ModInfo));

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
			if (dCurrMax > 0.05001)
			{	sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of %4.1f mA", 1000.0*dCurrMax, 1000.0*dCurrLimit);
				Warning(sBuffer);
			}

			if (s_nModType==MULT_SPEC)
			{	
        dFacM = 2.0;
        rc=GetEssModDat(&stMInfo[imod][0],   50.0, stMod[imod].dHeight, iDataVsn);  // Phi8, Schönfeldt, pancake
        if (rc)
        rc=GetEssModDat(&stMInfo[imod][1],  325.0, stMod[imod].dHeight, iDataVsn);  // Phi7, Schönfeldt
    	}
			else  // coupled
			{	
        rc=GetEssModDat(&stMInfo[imod][0], _dTemp, stMod[imod].dHeight, iDataVsn);  
        if (_dTemp==325.0)
          dFacM = 2.0;
			}
			break;

		case SNS:
			switch(s_nModType)
			{
				case POISONED:
					if      (_dTemp== 50.0)
					{ stMInfo[imod][0].dF001 = 2.7e10; stMInfo[imod][0].dF002 = 4.6e10;  /* Phi4 */
					}
					else if (_dTemp==325.0)
					{ 
            if (iDataVsn>=2)
            { stMInfo[imod][0].dF001 = 1.64e10; stMInfo[imod][0].dF002 = 3.0e10;  /* Phi1 */
            }
            else
            { stMInfo[imod][0].dF001 = 9.0e10;  stMInfo[imod][0].dF002 = 4.6e10;   /* Phi1 */
            }
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case DECOUPLED:
					if      (_dTemp== 50.0)
					{ stMInfo[imod][0].dF001 = 5.4e10; stMInfo[imod][0].dF002 = 9.2e10;  /* Phi5 */
					}
					else if (_dTemp==325.0)
					{ stMInfo[imod][0].dF001 = 1.8e11; stMInfo[imod][0].dF002 = 9.2e10;  /* Phi2 */
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case COUPLED:
					if      (_dTemp== 50.0)
					{ stMInfo[imod][0].dF001 = 2.3e11; stMInfo[imod][0].dF002 = 9.2e10;  /* Phi6*/
					}
					else if (_dTemp==325.0)
					{ 
						dFacM = 2.0;
						stMInfo[imod][0].dF001 = 4.5e11; stMInfo[imod][0].dF002 = 9.2e10;  /* Phi3*/
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
	stMInfo[imod][0].dF003 *= (dEpulse/dEp_std);
	
	if (s_nModType==MULT_SPEC)
	{	
		stMInfo[imod][1].dF001 *= (dEpulse/dEp_std);
		stMInfo[imod][1].dF002 *= (dEpulse/dEp_std);
		stMInfo[imod][1].dF003 *= (dEpulse/dEp_std);

		dFUAmpl = (        stMInfo[imod][0].dF001 + stMInfo[imod][0].dF003  + dFacN * stMInfo[imod][0].dF002         /* cold    */
		         + dFacM * stMInfo[imod][1].dF001                           + dFacN * stMInfo[imod][1].dF002) / 2.0; /* thermal */
	}
	else
	{	dFUAmpl =  dFacM *(stMInfo[imod][0].dF001 + stMInfo[imod][0].dF003) + dFacN * stMInfo[imod][0].dF002;
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
			{	dPSMC =  PulseIntEss(_dTime, 287e-6, 20, _dLength);
				dPSMT =  PulseIntEss(_dTime,  80e-6, 20, _dLength)
				       + PulseIntEss(_dTime, 400e-6, 20, _dLength);
			}
			else
			{	if      (dTemp== 50.0)
				{	/* Phi8 = integration of 3*Phi6 */
					dPSM =  PulseIntEss(_dTime,287e-6, 20, _dLength);
				}
				else if (dTemp==325.0)
				{	/* Phi7 = integration of 3*Phi3 */
					dPSM =  PulseIntEss(_dTime, 80e-6, 20, _dLength)
					      + PulseIntEss(_dTime,400e-6, 20, _dLength);
				}
				else
				{	Error("moderator temperature for ESS must be 50 or 325 K");
				}
				dN = NotMaxwell (_dLambda, stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD);
			}
			dPSN = PulseIntEss(_dTime, 12e-6*_dLambda, 5, _dLength);
			break;

		case SNS:
			switch(s_nModType)
			{
				case POISONED:
					if      (dTemp== 50.0)
					{	/* Phi4 */
						dPSM =  PulseShape(_dTime,  49.0e-6           , 5);
						dPSN =  PulseShape(_dTime,   7.0e-6 * _dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9, 2.2);
					}
					else if (dTemp==325.0)
					{	/* Phi1 */
            if (iDataVsn>=2)
						{ dPSM =  PulseShape(_dTime,  21.0e-6           , 5);
						  dPSN =  PulseShape(_dTime,   3.6e-6 * _dLambda, 5);
						  dN   =  NotMaxwell(_dLambda, 1.9, 2.2);
            }
            else
						{ dPSM =  PulseShape(_dTime,  22.0e-6           , 5);
						  dPSN =  PulseShape(_dTime,   7.0e-6 * _dLambda, 5);
						  dN   =  NotMaxwell(_dLambda, 2.5, 2.2);
            }
					}
					else
					{	Error("moderator temperature for SNS must be 50 or 325 K");
					}
					break;

				case DECOUPLED:
					if      (dTemp== 50.0)
					{	/* Phi5 */
						dPSM =  PulseShape(_dTime, 78e-6          , 5);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9, 2.2);
					}
					else if (dTemp==325.0)
					{	/* Phi2 */
						dPSM =  PulseShape(_dTime, 35e-6,           5);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 2.5, 2.2);
					}
					else
					{	Error("moderator temperature for SNS must be 50 or 325 K");
					}
					break;

				case COUPLED:
					if      (dTemp== 50.0)
					{	/* Phi6*/
						dPSM =  PulseShape(_dTime,287e-6          ,20);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 0.9, 2.2);
					}
					else if (dTemp==325.0)
					{	/* Phi3*/
						dPSM =  PulseShape(_dTime, 80e-6          ,20)
						       +PulseShape(_dTime,400e-6          ,20);
						dPSN =  PulseShape(_dTime, 12e-6*_dLambda, 5);
						dN   =  NotMaxwell(_dLambda, 2.5, 2.2);
					}
					else
					{	Error("ERROR: moderator temperature for SNS must be 50 or 325 K");
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
	{	double FUc, FUt;

		FUt  =  stMInfo[imod][1].dF001 * Maxwellian(_dLambda, stMInfo[imod][1].dTemp)    * dPSMT
		      + stMInfo[imod][1].dF002 * NotMaxwell(_dLambda, stMInfo[imod][1].alpha_SD, stMInfo[imod][1].kappa_SD) * dPSN ;
	
    if (iDataVsn >= 3)       // new cold moderator, analytical description
		{ FUc  =  stMInfo[imod][0].dF001 * LeakageFct(_dLambda, &stMInfo[imod][0]) * dPSMC
		        + stMInfo[imod][0].dF002 * NotMaxwell(_dLambda,  stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD) * dPSN ;
    }
    else
		{ FUc  =  stMInfo[imod][0].dF001 * Maxwellian(_dLambda, stMInfo[imod][0].dTemp) * dPSMC
		        + stMInfo[imod][0].dF002 * NotMaxwell(_dLambda, stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD)  * dPSN ;
      if (iDataVsn == 2)     // new cold moderator, empirical correction factor
        FUc *= EmpCorrFact(_dLambda);
    }
    dFU = f_cold (_dLambda) * FUc + f_therm(_dLambda) * FUt;
	}
	else
	{	
    if (iDataVsn >= 3 && dTemp < 100.0)     // new cold moderator, analytical description
      dM = LeakageFct(_dLambda, &stMInfo[imod][0]);
    else
		  dM = Maxwellian(_dLambda, dTemp);

		dFU =  stMInfo[imod][0].dF001 * dM * dPSM
		     + stMInfo[imod][0].dF002 * dN * dPSN;

    if (iDataVsn == 2 && dTemp < 100.0)     // new cold moderator, empirical correction factor
      dFU *= EmpCorrFact(_dLambda);
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


double LeakageFct(const double lambda, const ModInfo* pInfo)
{
	/* _lambda : Wavelength in Angstroem */

	double dM=0.0, xi;
  
  xi = pInfo->dF003 / pInfo->dF001;
	dM =  sqrt( 1.0 / (1.0 + exp(pInfo->alpha_L * (lambda - pInfo->lambda_L)) )) 
      * (exp(-pInfo->alpha_1 * lambda) + xi*exp(-pInfo->alpha_2 * lambda));

	return dM;
}

double EmpCorrFact(double lmbd)
{ 
	double factor = log(1.402 + 0.898 * lmbd);

	if (lmbd <= 2.5) 
    factor *= 2.0776 - 4.1093*lmbd + 4.8836*sq(lmbd) - 2.4715*pow(lmbd,3) + 0.4521*pow(lmbd,4);
	if (lmbd >  2.5 && lmbd <= 3.5) 
    factor *= 4.3369 - 1.8367*lmbd + 0.2524*sq(lmbd);

  return factor;              
}

/*
double NewMaxwell(const double _lambda, const double _temp)
{
	// _lambda : Wavelength in Angstroem 

	double dM=0.0, a, 
         lambdaT = 949.2;
  a  = lambdaT/_temp;
  dM = 4.36e14*a*a/pow(_lambda,5)*exp(-a/pow(_lambda,2));

	return dM;
}
*/

double NotMaxwell(const double lambda, const double alpha, const double kappa)
{
	// lambda: wavelength           [Ang]   
	// alpha : line shape parameter [1/Ang] 
	// kappa : line shape parameter [ ]     
  //     N = 1/lambda / (1+exp(alpha*lambda-kappa))

	double dN=0.0;
	
	if (lambda > 0.0)
	{
		dN = 1.0 / (1.0 + exp(alpha*lambda - kappa)) / lambda ;
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in NotMaxwell(): Lambda = %10.4e Ang\n",
		                   lambda);
		exit(99);
	}

	return dN;
}


short GetEssModDat(ModInfo* pModInfo, const double ModTemp, const double ModHeight, const short iVsn)
{
/* input : ModTemp  : moderator temperature 
           ModHeight: moderator height
   output: ModInfo  : various data describing moderator characteristics
*/
  FILE*      pFile=NULL;
  char       sLine[256];
  short      bFound=FALSE, rc;
  double     TempT, HeightT,   // moderator temperature and height in table
             HeightK;          // key value for moderator height to search in table
  EssModChar Info;

  // initialize
  memset (&Info, '\0', sizeof(EssModChar));

  // fill data structure (depending on version of the moderator characteristics and moderator temperature and height)
  if (iVsn < 3)
	{ 
    pModInfo->dTemp = ModTemp; 

    if (ModTemp==325.0)
    {
		  pModInfo->dF001 = 4.5e11;  pModInfo->alpha_SD = 2.5;  // Phi7, Mezei, thermal
      pModInfo->dF002 = 9.2e10;  pModInfo->kappa_SD = 2.2; 
      pModInfo->alpha_SD = 2.5;
      bFound = TRUE;
    }
		else if (ModTemp==50.0)
    { 
      pModInfo->dF001 = 2.3e11;  pModInfo->alpha_SD = 0.9;  // Phi8, Mezei, cold
      pModInfo->dF002 = 9.2e10;  pModInfo->kappa_SD = 2.2;
      bFound = TRUE;
    }
    else
		{  Error("moderator temperature for ESS must be 50 or 325 K");
		}
  }
  else
  {
    if (iVsn==3) 
      HeightK = 12.0;       // no dependence on moderator height assumed in vsn 3
    else
      HeightK = ModHeight;

    // open file containing ESS moderator characteristics
    pFile=fileOpen(FullInstallName("EssModChar.dat", "FILES/moderators/ESS/"), "rt");

    // search for a line with the given temperature and moderator height
    do
    {
      rc=ReadLine(pFile, sLine, sizeof(sLine));
      if (rc)
        sscanf(sLine, "%lf %lf  %lf %lf %lf  %lf %lf  %lf %lf  %lf %lf", &TempT, &HeightT, 
                      &Info.I_SD, &Info.alpha_SD, &Info.lambda_SD, &Info.alpha_L, &Info.lambda_L, &Info.I1, &Info.alpha_1, &Info.I2, &Info.alpha_2);

      if (TempT==ModTemp && HeightT==HeightK) bFound=TRUE;
    }
    while (bFound==FALSE && rc==TRUE);

    if (bFound) 
    { pModInfo->dTemp    = TempT;
      pModInfo->dF001    = Info.I1  /50.0/25.0;    // Phi7, Phi8, Schönfeldt
      pModInfo->dF002    = Info.I_SD/50.0/25.0;    // divided by SP source freq. and multiplied by duty cycle
      pModInfo->dF003    = Info.I2  /50.0/25.0;
      if (TempT > 200.0)                            
        pModInfo->dF001 *= 0.5;                    // factor 0.5 bc. of 2 fct. F(t) for Maxwellian part of thermal spectrum
      pModInfo->alpha_SD = Info.alpha_SD;
      pModInfo->kappa_SD = Info.alpha_SD * Info.lambda_SD;
      pModInfo->alpha_L  = Info.alpha_L;
      pModInfo->lambda_L = Info.lambda_L;
      pModInfo->alpha_1  = Info.alpha_1;
      pModInfo->alpha_2  = Info.alpha_2;
    }
  }
  return(bFound);
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
