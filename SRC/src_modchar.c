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


/********************/
/* global variables */
/********************/
extern FctTable  stFluxT[NUM_MOD],  /* data of time distr.            */
                 stFluxL[NUM_MOD],  /* data of wavelength distr.      */
                 stFluxLT[NUM_MOD]; /* data of wavelength-time distr. */
extern short     iDataVsn,          /* version of the data base for the source characteristics */
                 nNumMod,           /* number of moderators in the moderator system            */
                 imod;              /* index of moderators in the moderator system             */
extern Moderator stMod  [NUM_MOD];  /* moderator data            */


/********************/
/* static variables */
/********************/
static ModInfo stMInfo[NUM_MOD][2];  // additional moderator data  bispec. moder.: index 0 for cold part, 1 for thermal part
                                     //                            otherwise     : only index 0 used
static short   _eSource=ANYSOURCE,  /* _eSource    : ANYSOURCE, ESS, SNS, CSNS                         */
               _eModType=0;         /* _eModType   : decoupled POISONED, DECOUPLED unpoisoned, COUPLED */


/*********************************/
/* global functions              */
/*********************************/

/***************************************/
/* Initializes the Moderator structure */
/***************************************/
void   InitModerator(Moderator*   pMod)
{
  pMod->ModTemp    =0.0;
  pMod->nBackground=0;
  pMod->nColour    =NO_COLOR;
  pMod->bCircle    =TRUE; 
  pMod->CntrX      =0.0;
  pMod->CntrY      =0.0;
  pMod->CntrZ      =0.0;
  pMod->Width      =0.0;
  pMod->Height     =0.0;
  pMod->Diameter   =0.0;
  pMod->Area       =0.0;            
  pMod->DistModWnd =0.0;      
  pMod->WndFact    =0.0;         
  pMod->TotalFlux  =0.0;       
  pMod->Current    =0.0;         
  pMod->NormInt    =1.0;         
  pMod->PfmcFact   =1.0;
  strcpy(pMod->sLFileName ,"");
  strcpy(pMod->sTFileName ,"");
  strcpy(pMod->sLTFileName,"");
  pMod->eModType   =COUPLED;
  pMod->TauAscent  = 12.5;       
  pMod->TauDecay   =125.0;        
  pMod->TotFluxUM  =  0.0;       
  pMod->Chi        =  2.5;             
  pMod->Kappa      =  2.2;           
  pMod->TauAscUM   =  2.4;        
  pMod->TauDecUM   = 12.0;        
  pMod->FUAmpl     =  0.0;          
  pMod->FUAmpUM    =  0.0;          
  pMod->eIsisTS    =  1;	        
}

void   InitTrajRange(TrajParam* pTrj)
{ pTrj->LambdaMin  = 0.0;
  pTrj->LambdaMax  = 0.0;
  pTrj->MinDivY    = 0.0;
  pTrj->MaxDivY    = 0.0;
  pTrj->MinDivZ    = 0.0;
  pTrj->MaxDivZ    = 0.0;
  pTrj->TimeFrmMin = 0.0;
  pTrj->TimeFrmMax = 0.0;
}

void   CopyTrajRange(const TrajParam* pSrc, TrajParam* pDest)
{ pDest->LambdaMin  = pSrc->LambdaMin ;
  pDest->LambdaMax  = pSrc->LambdaMax ;
  pDest->MinDivY    = pSrc->MinDivY   ;
  pDest->MaxDivY    = pSrc->MaxDivY   ;
  pDest->MinDivZ    = pSrc->MinDivZ   ;
  pDest->MaxDivZ    = pSrc->MaxDivZ   ;
  pDest->TimeFrmMin = pSrc->TimeFrmMin;
  pDest->TimeFrmMax = pSrc->TimeFrmMax;
}



/****************************************************************************************/
/* IndLT  returns index in array 'pTabF' of 2-dim. table for flux(time, wavelength), */
/*	storage rowwise: 	i: index for time        (defines row)                            */
/*                    j: index for wavelength  (defines column)                         */
/****************************************************************************************/
long IndLT(const long i, const long j)
{
	return(i*(stFluxLT[imod].nColumns) + j);
}


/***********************************************************/
/* TotalFU  returns the flus amplitude for ESS and SNS     */
/***********************************************************/
double TotalFU(const double Temp,  const short  eSource, const short eModType, 
               const double Power, const double Period, const double PulseLen)
{
	/* Temp      : [K]  eff. moderator temperature 
	   eSource   :      ESS, SNS,
	   eModType  :      decoupled POISONED, DECOUPLED unpoisened, COUPLED
	   Power     : [W]  average source power                             
	   Period    : [ms] time between 2 pulses                             
	   PulseLen  : [s]  pulse length                             */
  short  rc;
	double FUAmpl= 0.0,
	       FacM  = 1.0,     //     integral of fct. M(lambda) = number of Maxwellian functions 
	       FacN  = 1.904,   //     integral of fct. N(lambda) in 0.1 Ang .... 20 Ang
	       U0    = 2.5e9,   // [V] accelerator voltage: 2.5 GV 
	       Ep_std= 1.0e5,   // [J] standard energy of 1 pulse: 5 MW * 20 ms = 100 kJ (as for ESS SPTS)
	       Epulse,          // [J] energy of 1 pulse          
	       period,          // [s] time between 2 pulses 
	       CurrLimit=0.050, // [A] max. possible accelerator current
	       CurrMax;         // [A] max. current for this set-up     
	char   sBuffer[256];

	/* initialize */
	memset(stMInfo[imod], '\0', 2*sizeof(ModInfo));

	_eSource     = eSource;
	_eModType    = eModType;
	if (_eModType!=MULT_SPEC)
		stMInfo[imod][0].Temp = Temp;

	/* integral of fct. N(lambda) in 0.1 Ang .... 20 Ang */
	if (Temp==50.0)
		FacN=2.808;
	else
		FacN=1.904;   

	/* energy of pulse for normalization:
	   standard values for energy of 1 pulse: E_pulse  */
	period = Period/1000.0;   // ms -> s
	Epulse = Power * period;

	switch(_eSource)
	{
		case ESS:
			/* maximal accelerator current */
			CurrMax   = Epulse / PulseLen / U0;
			if (CurrMax > 0.05001)
			{	sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of %4.1f mA", 1000.0*CurrMax, 1000.0*CurrLimit);
				Warning(sBuffer);
			}

			if (_eModType==MULT_SPEC)
			{	
        FacM = 2.0;
        rc=GetEssModDat(&stMInfo[imod][0],   50.0, stMod[imod].Height, iDataVsn);  // Phi8, Schönfeldt, pancake
        if (rc)
        rc=GetEssModDat(&stMInfo[imod][1],  325.0, stMod[imod].Height, iDataVsn);  // Phi7, Schönfeldt
    	}
			else  // coupled
			{	
        rc=GetEssModDat(&stMInfo[imod][0], Temp, stMod[imod].Height, iDataVsn);  
        if (Temp==325.0)
          FacM = 2.0;
			}
			break;

		case SNS:
			switch(_eModType)
			{
				case POISONED:
					if      (Temp== 50.0)
					{ stMInfo[imod][0].F001 = 2.7e10; stMInfo[imod][0].F002 = 4.6e10;  /* Phi4 */
					}
					else if (Temp==325.0)
					{ 
            if (iDataVsn>=2)
            { stMInfo[imod][0].F001 = 1.64e10; stMInfo[imod][0].F002 = 3.0e10;  /* Phi1 */
            }
            else
            { stMInfo[imod][0].F001 = 9.0e10;  stMInfo[imod][0].F002 = 4.6e10;   /* Phi1 */
            }
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case DECOUPLED:
					if      (Temp== 50.0)
					{ stMInfo[imod][0].F001 = 5.4e10; stMInfo[imod][0].F002 = 9.2e10;  /* Phi5 */
					}
					else if (Temp==325.0)
					{ stMInfo[imod][0].F001 = 1.8e11; stMInfo[imod][0].F002 = 9.2e10;  /* Phi2 */
					}
					else
					{  Error("moderator temperature for ESS/SNS must be 50 or 325 K");
					}
					break;

				case COUPLED:
					if      (Temp== 50.0)
					{ stMInfo[imod][0].F001 = 2.3e11; stMInfo[imod][0].F002 = 9.2e10;  /* Phi6*/
					}
					else if (Temp==325.0)
					{ 
						FacM = 2.0;
						stMInfo[imod][0].F001 = 4.5e11; stMInfo[imod][0].F002 = 9.2e10;  /* Phi3*/
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
			if (Temp==50.0)
			{	stMInfo[imod][0].F001 = stMInfo[imod][0].F001/1.3;
				stMInfo[imod][0].F002 = stMInfo[imod][0].F002/1.3;
			}
			else
			{	stMInfo[imod][0].F001 = stMInfo[imod][0].F001*1.3;
				stMInfo[imod][0].F002 = stMInfo[imod][0].F002*1.3;
			}
			break;

		default:
			Error("Internal error: source unknown in 'TotalFU'");
	}
			
	stMInfo[imod][0].F001 *= (Epulse/Ep_std);
	stMInfo[imod][0].F002 *= (Epulse/Ep_std);
	stMInfo[imod][0].F003 *= (Epulse/Ep_std);
	
	if (_eModType==MULT_SPEC)
	{	
		stMInfo[imod][1].F001 *= (Epulse/Ep_std);
		stMInfo[imod][1].F002 *= (Epulse/Ep_std);
		stMInfo[imod][1].F003 *= (Epulse/Ep_std);

		FUAmpl = (        stMInfo[imod][0].F001 + stMInfo[imod][0].F003  + FacN * stMInfo[imod][0].F002         /* cold    */
		         + FacM * stMInfo[imod][1].F001                          + FacN * stMInfo[imod][1].F002) / 2.0; /* thermal */
	}
	else
	{	FUAmpl =  FacM *(stMInfo[imod][0].F001 + stMInfo[imod][0].F003) + FacN * stMInfo[imod][0].F002;
	}
	return(FUAmpl);
}

/****************************************************************/
/* EssModFU  returns the flux (in FluxUnits) for ESS and SNS    */
/****************************************************************/
double EssModFU(const double Lambda, const double Time, const double PulsLen)
{
	/* Lambda    : wavelength                      in Angstroem
	   Time      : time (after beginning of pulse) in s
	   PulsLen    : pulse length of LPSS  */

	double dFU    =0.0,
	       dPSM=0.0, dPSMC=0.0, dPSMT=0.0, dPSN=0.0,
	       dM  =0.0, dN   =0.0,
	       dTemp=0;

	if (_eModType!=MULT_SPEC)
	{ if      (stMInfo[imod][0].Temp > 290.0) dTemp=325.0;
    else if (stMInfo[imod][0].Temp <  75.0) dTemp= 50.0;
		else     Error("moderator temperature for ESS/SNS must be around 50 or 325 K");
  }

	switch(_eSource)
	{
		case ESS:
			if (_eModType == MULT_SPEC)
			{	dPSMC =  LongPulseShape(Time, 287.0e-6, 20.0, PulsLen);
				dPSMT =  LongPulseShape(Time,  80.0e-6, 20.0, PulsLen)
				       + LongPulseShape(Time, 400.0e-6, 20.0, PulsLen);
			}
			else
			{	if (dTemp== 50.0)
					dPSM = LongPulseShape(Time, 287.0e-6, 20.0, PulsLen); /* Phi8 = integration of 3*Phi6 */
				else   // Temp=325 K
					dPSM = LongPulseShape(Time,  80.0e-6, 20.0, PulsLen)  /* Phi7 = integration of 3*Phi3 */
					     + LongPulseShape(Time, 400.0e-6, 20.0, PulsLen);
				dN = NotMaxwell (Lambda, stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD);
			}
			dPSN = LongPulseShape(Time, 12.0e-6*Lambda, 5.0, PulsLen);
			break;

		case SNS:
			switch(_eModType)
			{
				case POISONED:
					if (dTemp==50.0)
					{	dPSM =  ShortPulseShape(Time,  49.0e-6           , 5.0); /* Phi4 */
						dPSN =  ShortPulseShape(Time,   7.0e-6 * Lambda, 5.0);
						dN   =  NotMaxwell(Lambda, 0.9, 2.2);
					}
					else // Temp==325.0)
					{	/* Phi1 */
            if (iDataVsn>=2)
						{ dPSM =  ShortPulseShape(Time,  21.0e-6           , 5.0);
						  dPSN =  ShortPulseShape(Time,   3.6e-6 * Lambda, 5.0);
						  dN   =  NotMaxwell(Lambda, 1.9, 2.2);
            }
            else
						{ dPSM =  ShortPulseShape(Time,  22.0e-6           , 5.0);
						  dPSN =  ShortPulseShape(Time,   7.0e-6 * Lambda, 5.0);
						  dN   =  NotMaxwell(Lambda, 2.5, 2.2);
            }
					}
					break;

				case DECOUPLED:
					if      (dTemp== 50.0)
					{	dPSM =  ShortPulseShape(Time,  78.0e-6,          5.0); /* Phi5 */
						dPSN =  ShortPulseShape(Time,  12.0e-6*Lambda, 5.0);
						dN   =  NotMaxwell(Lambda, 0.9, 2.2);
					}
					else // Temp==325.0)
					{	dPSM =  ShortPulseShape(Time,  35.0e-6,          5.0);  /* Phi2 */
						dPSN =  ShortPulseShape(Time,  12.0e-6*Lambda, 5.0);
						dN   =  NotMaxwell(Lambda, 2.5, 2.2);
					}
					break;

				case COUPLED:
					if      (dTemp== 50.0)
					{	dPSM =  ShortPulseShape(Time, 287.0e-6         ,20.0);  /* Phi6*/
						dPSN =  ShortPulseShape(Time,  12.0e-6*Lambda, 5.0);
						dN   =  NotMaxwell(Lambda, 0.9, 2.2);
					}
					else // Temp==325.0)
					{	dPSM =  ShortPulseShape(Time,  80.0e-6         ,20.0)    
						       +ShortPulseShape(Time, 400.0e-6         ,20.0);  /* Phi3*/
						dPSN =  ShortPulseShape(Time,  12.0e-6*Lambda, 5.0);
						dN   =  NotMaxwell(Lambda, 2.5, 2.2);
					}
					break;

				default:
					Error("Internal ERROR: wrong moderator type in ModFU()");
			}
			break;

		default:
			Error("Internal ERROR: wrong source in ModFU()");
	}

	if (_eModType==MULT_SPEC)
	{	double FUc, FUt;

		FUt  =  stMInfo[imod][1].F001 * Maxwellian(Lambda, stMInfo[imod][1].Temp)    * dPSMT
		      + stMInfo[imod][1].F002 * NotMaxwell(Lambda, stMInfo[imod][1].alpha_SD, stMInfo[imod][1].kappa_SD) * dPSN ;
	
    if (iDataVsn >= 3)       // new cold moderator, analytical description
		{ FUc  =  stMInfo[imod][0].F001 * LeakageFct(Lambda, &stMInfo[imod][0]) * dPSMC
		        + stMInfo[imod][0].F002 * NotMaxwell(Lambda,  stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD) * dPSN ;
    }
    else
		{ FUc  =  stMInfo[imod][0].F001 * Maxwellian(Lambda, stMInfo[imod][0].Temp) * dPSMC
		        + stMInfo[imod][0].F002 * NotMaxwell(Lambda, stMInfo[imod][0].alpha_SD, stMInfo[imod][0].kappa_SD)  * dPSN ;
      if (iDataVsn == 2)     // new cold moderator, empirical correction factor
        FUc *= EmpCorrFact(Lambda);
    }
    dFU = f_cold (Lambda) * FUc + f_therm(Lambda) * FUt;
	}
	else
	{	
    if ((iDataVsn==3 || iDataVsn==4) && dTemp < 100.0)     // new cold moderator, analytical description
      dM = LeakageFct(Lambda, &stMInfo[imod][0]);
    else
		  dM = Maxwellian(Lambda, stMInfo[imod][0].Temp);

		dFU =  stMInfo[imod][0].F001 * dM * dPSM
		     + stMInfo[imod][0].F002 * dN * dPSN;

    if (iDataVsn == 2 && dTemp < 100.0)     // new cold moderator, empirical correction factor
      dFU *= EmpCorrFact(Lambda);
	}

	return(dFU);
}


/**********************************************************************************************/
/* Maxwellian is the Maxwellian spectrum normalized to 1 for integral lambda: 0 - > infinity  */
/**********************************************************************************************/
double Maxwellian(const double lambda, const double Temp)
{
	/* lambda : Wavelength                 in Angstroem */
	/* Temp   : eff. moderator temperature in Kelvin    */

	double M=0.0, b, a;

	if (Temp > 0.0  &&  lambda > 0.0)
	{
		b = pow(1.0e10*H_P, 2) / (2*K*MN);            /* b = h²/(2*k*m_n)  in (1E-10 m)²K */
		a = b / Temp;
		
		M = 2 * pow(a,2) * exp(-a / pow(lambda,2)) / pow(lambda,5) ;
	}
	else if (lambda < 0.0)
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in Maxwellian(): Lambda = %10.4e Ang, Temp = %9.3e K\n",
		                   lambda, Temp);
		exit(99);
	}

	return M;
}


/**********************************************************************************************/
/* LeakageFct : empirical flux corrections for ESS data base 3ff                              */
/* EmpCorrFact: empirical flux corrections for ESS data base 2                                */
/**********************************************************************************************/
double LeakageFct(const double lambda, const ModInfo* pInfo)
{
	/* _lambda : Wavelength in Angstroem */

	double dM=0.0, xi, sum_L;
  
  xi    = pInfo->F003 / pInfo->F001;
  sum_L = 1.0 + exp(pInfo->alpha_L * (lambda - pInfo->lambda_L));
	dM    =  pow(sum_L, pInfo->expo_L) 
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


/***********************************************************************************************************/
/* NotMaxwell represents a spectrum of under-moderated neutrons (integral depends on lower limit, about 2) */
/***********************************************************************************************************/
double NotMaxwell(const double lambda, const double chi, const double kappa)
{
	// lambda: wavelength           [Ang]   
	// chi   : line shape parameter [1/Ang] 
	// kappa : line shape parameter [ ]     
  //     N = 1/lambda / (1+exp(chi*lambda-kappa))

	double N=0.0;
	
	if (lambda > 0.0)
	{
		N = 1.0 / (1.0 + exp(chi*lambda - kappa)) / lambda;
	}
	else if (lambda < 0.0)
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in NotMaxwell(): Lambda = %10.4e Ang\n",
		                   lambda);
		exit(99);
	}

	return N;
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
  double     TempT=300.0,      // moderator temperature
             HeightT=3.0,      //  and height in table
             HeightK;          // key value for moderator height to search in table
  EssModChar Info;

  // initialize
  memset (&Info, '\0', sizeof(EssModChar));

  // fill data structure (depending on version of the moderator characteristics and moderator temperature and height)
  if (iVsn < 3)
	{ 
    pModInfo->Temp = ModTemp; 

    if (ModTemp==325.0)
    {
		  pModInfo->F001 = 4.5e11;  pModInfo->alpha_SD = 2.5;  // Phi7, Mezei, thermal
      pModInfo->F002 = 9.2e10;  pModInfo->kappa_SD = 2.2; 
      pModInfo->alpha_SD = 2.5;
      bFound = TRUE;
    }
		else if (ModTemp==50.0)
    { 
      pModInfo->F001 = 2.3e11;  pModInfo->alpha_SD = 0.9;  // Phi8, Mezei, cold
      pModInfo->F002 = 9.2e10;  pModInfo->kappa_SD = 2.2;
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
    pFile = OpenPackInpFile("EssModChar.dat", "FILES/moderators/ESS/", TRUE);

    // search for a line with the given temperature and moderator height
    do
    {
      rc=ReadLine(pFile, sLine, sizeof(sLine));
      if (rc)
      { sscanf(sLine, "%lf %lf  %lf %lf %lf  %lf %lf %lf  %lf %lf  %lf %lf  %lf", &TempT, &HeightT, 
                      &Info.I_SD, &Info.alpha_SD, &Info.lambda_SD, &Info.alpha_L, &Info.lambda_L, &Info.expo_L, &Info.I1, &Info.alpha_1, &Info.I2, &Info.alpha_2, &Info.T_real);
        if (TempT==ModTemp && HeightT==HeightK) bFound=TRUE;
      }
    }
    while (bFound==FALSE && rc==TRUE);

    if (bFound) 
    { pModInfo->Temp    = Info.T_real;
      pModInfo->F001    = Info.I1  /50.0/25.0;    // Phi7, Phi8, Schönfeldt
      pModInfo->F002    = Info.I_SD/50.0/25.0;    // divided by SP source freq. and multiplied by duty cycle
      pModInfo->F003    = Info.I2  /50.0/25.0;
      if (TempT > 200.0)                            
        pModInfo->F001 *= 0.5;                    // factor 0.5 bc. of 2 fct. F(t) for Maxwellian part of thermal spectrum
      pModInfo->alpha_SD = Info.alpha_SD;
      pModInfo->kappa_SD = Info.alpha_SD * Info.lambda_SD;
      pModInfo->alpha_L  = Info.alpha_L;
      pModInfo->lambda_L = Info.lambda_L;
      pModInfo->expo_L = Info.expo_L;
      pModInfo->alpha_1  = Info.alpha_1;
      pModInfo->alpha_2  = Info.alpha_2;
    }
  }
  return(bFound);
}


/*******************************************************************/
/* Functions to read flux distributions from file:                 */
/* UserLmbdTimeDis: F(lambda,t)                                    */
/* UserLmbdDis    : L(lambda)                                      */
/* UserTimeDis    : T(t)                                           */
/*******************************************************************/
double UserLmbdTimeDis(const double Lambda, const double Time)
{
	double Ud=0.0, LUd, time_ms, DelX, DelY,
	       LUij, LUi1j, LUij1, LUi1j1, LUi, LUi1;
	short  i=0, j=0;

	time_ms = Time*1000;   /*  time in milliseconds  */

	while (i+1 < stFluxLT[imod].nLines  &&  stFluxLT[imod].pTabX[i+1] < time_ms)
	{	i++;
	}
	while (j+1 < stFluxLT[imod].nColumns && stFluxLT[imod].pTabY[j+1] < Lambda)
	{	j++;
	}

	if (j+1 < stFluxLT[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		LUij   = stFluxLT[imod].pTabF[IndLT(i  ,j  )];
		LUij1  = stFluxLT[imod].pTabF[IndLT(i  ,j+1)];
		LUi1j  = stFluxLT[imod].pTabF[IndLT(i+1,j  )];
		LUi1j1 = stFluxLT[imod].pTabF[IndLT(i+1,j+1)];

		DelX   = stFluxLT[imod].pTabX[i+1] - stFluxLT[imod].pTabX[i];
		DelY   = stFluxLT[imod].pTabY[j+1] - stFluxLT[imod].pTabY[j];

		LUi    = LUij  + (LUij1 - LUij)  / DelY * (Lambda - stFluxLT[imod].pTabY[j]);
		LUi1   = LUi1j + (LUi1j1- LUi1j) / DelY * (Lambda - stFluxLT[imod].pTabY[j]);
		LUd    = LUi   + (LUi1  - LUi)   / DelX * (time_ms   - stFluxLT[imod].pTabX[i]);

    if (LUd==-100.0)
      Ud = 0.0;         // compare LoadWavelengthTimeDistribution() in source.c
    else
		  Ud = exp(LUd);
	}
	else
	/* read error: time or wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_LT_RANGE_TOO_SMALL);
	}

	return Ud;
}

double UserLambdaDis(const double Lambda, const double ModTemp)
{
	double Ud=0.0, LUd, LUdN, LUdN1;
	short  n=0;

	while (n+1 < stFluxL[imod].nLines  &&  stFluxL[imod].pTabX[n+1] < Lambda)
	{	n++;
	}

	if (n+1 < stFluxL[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		LUdN  = stFluxL[imod].pTabF[n];
		LUdN1 = stFluxL[imod].pTabF[n+1];
		LUd   = LUdN  +  (LUdN1-LUdN ) / (stFluxL[imod].pTabX[n+1] - stFluxL[imod].pTabX[n])
		                                  * (Lambda                - stFluxL[imod].pTabX[n]);
    if (LUd==-100.0)
      Ud = 0.0;         // compare LoadWavelengthDistribution() in source.c
    else
		  Ud = exp(LUd);
	}
	else
	/* read error: wavelength larger than all values in the distribution file */
	{	CountMessage(SRC_L_RANGE_TOO_SMALL);
	}

	return Ud;
}

double UserTimeDis(const double Time, const double Tau, const double TauRatio, const double PulsLen)
{
	double Ud=0.0, LUd, LUdN, LUdN1, time_ms;
	short  n=0;

	time_ms = Time*1000;   /*  time in milliseconds  */

	while (n+1 < stFluxT[imod].nLines  &&  stFluxT[imod].pTabX[n+1] < time_ms)
	{	n++;
	}

	if (n+1 < stFluxT[imod].nLines)
	{	/* linear  extrapolation in logarithmic scale */
		LUdN  = stFluxT[imod].pTabF[n];
		LUdN1 = stFluxT[imod].pTabF[n+1];
		LUd   = LUdN  +  (LUdN1 - LUdN ) / (stFluxT[imod].pTabX[n+1] - stFluxT[imod].pTabX[n])
		                                  * (time_ms                    - stFluxT[imod].pTabX[n]);
    if (LUd==-100.0)
      Ud = 0.0;         // compare LoadTimeDistribution() in source.c
    else
		  Ud = exp(LUd);
	}
	else
	/* read error: time larger than all values in the distribution file */
	{	CountMessage(SRC_T_RANGE_TOO_SMALL);
	}

	return Ud;
}


/*************************************************************************/
/* = ShortPulseShape, but with additional parameter as in LongPulseShape */
/*************************************************************************/
double PulseShapeP(const double Time, const double TauDecay, const double TauRatio, const double PulsLen)
{
   return ShortPulseShape(Time, TauDecay, TauRatio);
}


/***********************************************************************************************/
/* Normalized function describing the neutron pulse generated by a proton pulse of zero length */
/***********************************************************************************************/
double ShortPulseShape(const double Time, const double Tau, const double TauRatio)
{
	/* Time     : time (after beginning of Pulse) in sec                    */
	/* Tau      : decay Time constant             in sec                    */
	/* TauRatio : ratio 'decay time of pulse' to 'ascent of time of pulse'  */

	double F = 0.0,
	       tau_dec = Tau,             // Time constant for decay of pulse  in sec 
	       tau_asc = Tau/TauRatio;  // Time constant for ascent of pulse in sec 

	if (Time > 0.0)
  { if (Tau > 0.0  &&  TauRatio > 1.0)
	  {
		  F = (exp(-Time/tau_dec) - exp(-Time/tau_asc)) / (tau_dec - tau_asc);
	  }
	  else
	  {	fprintf(LogFilePtr,"ERROR: wrong parameter in ShortPulseShape(): Time = %10.4e s,  TauDecay = %10.4e s,  n = %6.2f\n",
		                     Time, Tau, TauRatio);
		  exit(99);
	  }
  }
	return F;
}


/****************************************************************************************************/
/* Normalized function describing the neutron pulse generated by a proton pulse of length 'PulsLen' */
/****************************************************************************************************/
double LongPulseShape(const double Time, const double Tau, const double TauRatio, const double PulsLen)
{
	/* Time    : time (after beginning of Pulse) in sec                   */
	/* Tau     : decay time constant             in sec                   */
	/* TauRatio: ratio 'decay time of pulse' to 'ascent of time of pulse' */
	/* PulsLen : proton pulse length             in sec                   */

	double FI = 0.0,
	       tau_dec = Tau,           // time constant for decay of pulse  in sec 
	       tau_asc = Tau/TauRatio;  // time constant for ascent of pulse in sec 

  if (Time > 0.0 && Tau > 0.0  &&  TauRatio > 1.0  &&  PulsLen > 0.0)
	{
		if (Time <= PulsLen)
		{
			FI = (  PulseShapeInt(Time,          tau_dec, tau_asc) + 1.0) / PulsLen;
		}
		else
		{
			FI = (  PulseShapeInt(Time,           tau_dec, tau_asc)
			      - PulseShapeInt(Time - PulsLen, tau_dec, tau_asc) ) / PulsLen;
		}
	}
	else if (Tau < 0.0  &&  TauRatio < 1.0  &&  PulsLen < 0.0)
	{  fprintf(LogFilePtr,"ERROR: wrong parameter in LongPulseShape(): Time = %10.4e s,  Tau_d = %10.4e s,  Pulse = %10.4e s   n = %6.2f\n",
		                    Time, Tau, PulsLen, TauRatio);
	    exit(99);
	}

	return FI;
}


/*************************************************************************************/
/* Integration of 'ShortPulseShape' facilitating the calculation of 'LongPulseShape' */
/*************************************************************************************/
double PulseShapeInt(const double Time, const double TauDecay, const double TauAscent)
{
	/* Time      : time (after beginning of pulse) in sec */
	/* TauDecay  : decay time constant             in sec */
	/* TauAscent : ascent time constant            in sec */

	double I = 0.0,
	       n;

	if (TauAscent > 0.0  &&  TauDecay > TauAscent)
	{
		n = TauDecay / TauAscent;
		I = (exp(-Time/TauAscent) - n*exp(-Time/TauDecay)) / (n-1.0);
	}
	else
	{	fprintf(LogFilePtr,"ERROR: wrong parameter in PulseShapeInt(): Time = %10.4e s,  Tau_d = %10.4e s,  Tau_a = %10.4e s \n",
		                   Time, TauDecay, TauAscent);
		exit(99);
	}

	return I;
}


/******************************************************************************************************/
/* functions to calculate the portion of flux that is fed into the guide by a special beam extraction
   system for the cold source (f_cold) and the thermal source (f_therm) of a multi-spectral moderator */
/******************************************************************************************************/
double f_cold(const double lambda)
{
	/* lambda : Wavelength           in Angstroem    */

	if (lambda > 1.4)
		return(0.95 - f_therm(lambda));
	else
		return 0.0;
}

double f_therm(const double lambda)
{
	/* lambda : Wavelength           in Angstroem    */

	double f=0.0, lmbd4;

	lmbd4 = pow(lambda,4);
	f = 0.86 * exp(-0.01677*lmbd4) + 0.14*(1 -pow(lmbd4/(lmbd4+40000),0.25));

	return f;
}


/*************************************************************************/
/* 'AveSolidAngleC',	'AveSolidAngleR'
	
   These functions calculate the average solid angle of the window
   seen from the moderator area:
   The solid angle is: Omega(y,z) =  [atan((w-y)/D) - atan((-w-y)/D)]
                                   * [atan((h-z)/D) - atan((-h-z)/D)]
   w: window width, h: window height, D distance moderator - window

   integration of  atan(x/D)  yields  x*atan(x/D) - D*ln(D²+x²)/2
   integration over rectangular moderator area yields
    I_ges = (I1 -I2) * (I3 - I4) with I2 = I1, I4=I3

   for the circular moderator only an approximation is calculated        */
/*************************************************************************/
double AveSolidAngleC(const double ModDiameter,
                      const double WndWidth, const double WndHeight, const double Dist)
{
	double OmAr  =0.0,
	       Omega =0.0,
	       ModY0, ModZ0, WndY0, WndZ0, I1, I3;

	ModY0 = 0.25*ModDiameter*sqrt(M_PI);
	ModZ0 = ModY0;
	WndY0 = 0.5* WndWidth;
	WndZ0 = 0.5* WndHeight;

	I1 = IntAtan(WndY0-ModY0, WndY0+ModY0, Dist);
	I3 = IntAtan(WndZ0-ModZ0, WndZ0+ModZ0, Dist);
	
	OmAr  = 4 * I1 * I3;
	Omega = OmAr / (M_PI * sq(ModDiameter/2.0) );

	return Omega;
}

double AveSolidAngleR(const double ModWidth, const double ModHeight,
                      const double WndWidth, const double WndHeight, const double Dist)
{
	double OmAr  =0.0,
	       OmegaQ=0.0,
	       ModY0, ModZ0, WndY0, WndZ0, I1, I3;

	ModY0 = 0.5* ModWidth;
	ModZ0 = 0.5* ModHeight;
	WndY0 = 0.5* WndWidth;
	WndZ0 = 0.5* WndHeight;

	I1 = IntAtan(WndY0-ModY0, WndY0+ModY0, Dist);
	I3 = IntAtan(WndZ0-ModZ0, WndZ0+ModZ0, Dist);
	/* whole integral (I1 -I2) * (I3 - I4) */
	OmAr   = 4 * I1 * I3;

	OmegaQ = OmAr / (ModWidth * ModHeight);

	return OmegaQ;
}


/*************************************************************************************************/
/* 'AveWeightC', 'AveWeightR'

   For 'DirectionByWindow' trajectories have to be normalized by f=cos²(phi)*cos²(theta).
   These functions 'AveWeightC' and 'AveWeightR' calculate the average normalization factors
   by integration over window area and over moderator area.
   The resulting factor F is included in the main program to give correct absolute flux values. */
/************************************************************************************************/
double AveWeightC(const double ModCntrY, const double ModCntrZ,  const double ModDiam,
                  const double WndWidth, const double WndHeight, const double Dist)
{	
	double y, z, Fact, Sum=0.0, Wy=0.0, Wz=0.0;
	long   num=0;

	/* numerical integration of 'weight' (see below) over moderator area */
	for (y=ModCntrY-0.495*ModDiam; y<=ModCntrY+0.5*ModDiam; y+= ModDiam/100.0)
	{	Wy = WeightDirByWnd(WndWidth, Dist, y);
		
		for (z=ModCntrZ-0.495*ModDiam; z<=ModCntrZ+0.5*ModDiam; z+= ModDiam/100.0)
		{	
			/* count if (y,z) within moderator */
			if (sq(y-ModCntrY) + sq(z-ModCntrZ) <= sq(0.5*ModDiam))
			{	Wz = WeightDirByWnd(WndHeight, Dist, z);
				Sum+= Wy*Wz;
				num ++;
			}
		}
	}
	Fact = Sum / num;

	return Fact;
}

double AveWeightR(const double ModCntrY, const double ModCntrZ,
                  const double ModWidth, const double ModHeight,
                  const double WndWidth, const double WndHeight, const double Dist)
{	
	double FactY, FactZ;

	/* integration of 'weight' (see below) over moderator width yields
	   F1 = dist² / (wnd_width*mod_width)
	              * ( IntAtan(tan(phi_min)...tan(phimax) for wnd_begin)
	                 -IntAtan(tan(phi_min)...tan(phimax) for wnd_end)       (same for height) */
	FactY =  pow(Dist,2) / (ModWidth*WndWidth)
	      * ( IntAtan((-0.5*WndWidth -(ModCntrY-0.5*ModWidth ))/Dist, (-0.5*WndWidth -(ModCntrY+0.5*ModWidth ))/Dist, 1)
	        - IntAtan(( 0.5*WndWidth -(ModCntrY-0.5*ModWidth ))/Dist, ( 0.5*WndWidth -(ModCntrY+0.5*ModWidth ))/Dist, 1));
	FactZ =  pow(Dist,2) / (ModHeight*WndHeight)
	       * ( IntAtan((-0.5*WndHeight-(ModCntrZ-0.5*ModHeight))/Dist, (-0.5*WndHeight-(ModCntrZ+0.5*ModHeight))/Dist, 1)
	         - IntAtan(( 0.5*WndHeight-(ModCntrZ-0.5*ModHeight))/Dist, ( 0.5*WndHeight-(ModCntrZ+0.5*ModHeight))/Dist, 1));

	return FactY*FactZ;
}


/**********************************************************************************************/
/* average of factor cos²(x) integrated over window width for a fixed moderator position
   is f1 = dist / wnd_width * (max_angle - min_angle)       (same for height)  */
/**********************************************************************************************/
double WeightDirByWnd(const double WndSize, const double Dist, const double ModPos)
{
	double Int, AngleMax=0.0, AngleMin=0.0;

	AngleMax = atan(( 0.5*WndSize - ModPos)/Dist);
	AngleMin = atan((-0.5*WndSize - ModPos)/Dist);
	Int =  Dist/WndSize * (AngleMax-AngleMin) ;

	return Int;
}


/**********************************************************************************************/
/* integration of atan(x/a) from x=beg to x=end  (a: param) */
/**********************************************************************************************/
double IntAtan(const double IntBeg, const double IntEnd, const double Param)
{
	double Int=0.0;

	Int =  (IntEnd * atan(IntEnd/Param) - 0.5*Param * log(sq(Param) + sq(IntEnd)))
	      - (IntBeg * atan(IntBeg/Param) - 0.5*Param * log(sq(Param) + sq(IntBeg)));

	return Int;
}
