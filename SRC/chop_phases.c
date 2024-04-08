/*********************************************************************************************/
/* Tool ChopPhases:                                                                          */
/*  Calculation of chopper phases for single choppers or a set of choppers                   */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0      2001  K. Lieutenant  initial version                                             */
/* 1.1  Mar 2020  K. Lieutenant  tidy up                                                     */
/*********************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define TRUE  1
#define FALSE 0

#define PI         3.14159265
#define M_NEUTRON  1.674928e-27
#define H_PLANCK   6.626076e-34

typedef int bool;


/******************************/
/** Prototypes               **/
/******************************/
static void   OwnInit    (int argc, char *argv[]);                // Reads input parameters and sets global parameters
static double Time2Lambda(double distance, double t1, double t2); // calculates wavelength of neutrons for a certain TOF and distance
static double PhaseRed   (const double PhiTot, int* pNumRot);     // reduces angle to range [-360, 360] deg
static void   ErrorMsg   (const char *text);                      // Prints error message

static double FMax(double a, double b) { return a > b ? a : b; }  // return maximum of a and b
static double FMin(double a, double b) { return a < b ? a : b; }  // return minimum of a and b


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID  _eModule = MCN_TOOL_CHOP;
char   sBuffer[129];
double dRpm, dRotFreq,     // Rotat. frequency in rpm and Hz                     
       dRepRate,           // Repetition rate of the pulse in Hz                     
       dL, dL1, dL2,       // Distances: source - chopper (center, 1. and 2. of double chopper)
       dLwb,               // Distance and TOF: source - wavelength band chopper
       dAwb,  dA,          // Apertures of the wavelength band chopper and the frame overlap chopper
       dLambda,            // Wavelength (mimimal or average) in Angstroem
       dLDet=0.0,          // Distance: source - detector
       dBeam=0.0,          // Beam diameter at the wavelength band chopper in cm
       dBeamFo=0.0,        // Beam diameter at the frame overlap chopper in cm
       dTdelay,            // pulse lengths in ms:  end of pulse       (those neutrons shall pass the choppers)
       dTp,                // pulse lengths in ms:  end of pulse       (those neutrons shall pass the choppers)
       dTpMax,             //                       very end of pulse  (considered for the evaluation time)
       dRChop;             // Distance: center of beam - center of chopper  in cm  
char   cMode, cWBand, 
       cRotSense, cFoMode;
FILE*  pOutFile;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char* argv[])
{
	double dTprd,                        // Period of the chopper rotation
	       dTrep,                        // Period of the pulse repetition
	       dTwb,                         // TOF from the beg. of the pulse to pass the WB chopper at the center of the aperture 
	       dTAwb, dTAfo,                 // the time the aperture of the WB chopper and the frame overlap chopper is open 
	       dTA, dLdMinA, dLdMaxA,        // time, minimal and maximal Wavelength beginning of the pulse
	       dTC, dLdMinC, dLdMaxC,        // time, minimal and maximal Wavelength center of the pulse
	       dTE, dLdMinE, dLdMaxE,        // time, minimal and maximal Wavelength end of the pulse
	       dTM, dLdMinM, dLdMaxM,        // time, minimal and maximal Wavelength very end of the pulse
			                                 // Times of arrival at the detector: 
	       dTdet1,  dTdet2,              //   for the first and the last neutron of this pulse (WB chopper + FO chopper)
	       dTMin,   dTMax,               //   for the first and the last neutron of this pulse (only WB chopper)
	       dTevMin, dTevMax,             //   for the last neutrons of the preceding and the first of the subsequent pulse
	       dTevDW1, dTevDW2,             //   for the first and last neutrons of the desired wavelength range
	       dTevOp1, dTevOp2,             //   dito reduced by the 'half shadow' time
	       dLmbdOp1,dLmbdOp2,            //   respective wavelength range
	       dVwb=1.0, dVfo,               // Ratio of the distances: source - chopper / source - detector
	       dT1, dT2,                     // TOF to disc 1 and (in case of a double chopper) to disc 2 
	       dThsWb=0.0,                   // Time needed for a point on the (WB) chopper to pass the beam  
	       dThsFo=0.0,                   //    and frame overlap chopper (gives half shadow time at detector)
	       dThsFoM=0.0,                  // max. time of a FO chopper in the beam that the half shadow time of the WB chopper 
	                                     //    is not exceeded
	       dAddT,                        // Additional opening time of frame overlap chopper due to pulse length and half shadow
	       dPhi1, dPhi1r,                // Phases of discs 1 and 2
	       dPhi2, dPhi2r,                //   total and reduced (to -360° < phi <= 360°) 
	       dDistChop     = 0.052,        // Distance of the discs of a double chopper in m
	       dVelMean, dVelFast;           // Velocity of neutrons with the average and the shortest wavelength
	bool   bDouble=FALSE;
	int    nRot1, nRot2;                 // Number of completed rotations of discs 1 and 2

	double dLdMinFM, dLdMaxFA;

	pOutFile = stdout;

	OwnInit(argc, argv);		

	dRotFreq = dRpm /  60.0;          // Rotat. frequency of chopper in Hz
	dTprd    = 1.0  / dRotFreq;       // period of chopper in seconds

	if (cMode=='o') 
	{	dVwb     = (dLDet - dLwb) / dLwb;                              // ratio for WB chopper
		dThsWb   = 2 * asin(0.5*dBeam / dRChop) / (2*PI * dRotFreq);  // half shadow time at WB chopper
	}
	else 
	{	dTpMax = dTp;
		dVwb   = 1.0;  // not used
		dThsWb = 0.0;  // half shadow time not regarded
	}
	dTdelay *= 0.001;  // delay             in sec
	dTp    *= 0.001;  // pulse length      in sec
	dTpMax *= 0.001;  // max. pulse length in sec

	// Velocities, Times of Flight for the center of the aperture, opening time
	if (cWBand=='a')
	{  dVelMean = H_PLANCK / (M_NEUTRON * dLambda*1.0e-10) ;
		dTwb     = dLwb / dVelMean;
		dVelFast = dLwb / (dTwb - 0.5*dAwb/360 * dTprd) ;
	}
	else
	{	dVelFast = H_PLANCK / (M_NEUTRON * dLambda*1.0e-10) ;
		dTwb     = dLwb / dVelFast + 0.5*dAwb/360 * dTprd;
		dVelMean = dLwb / dTwb;
	}
	dTwb += dTdelay;                // average time of arrival at chopper
	dTAwb = dAwb / 360.0 * dTprd;  // opening time of chopper

	//  minimal and maximal wavelengths from source to WB chopper 
	//  A: for neutrons starting at the beginning of the pulse
	//  C: for neutrons starting at the center of the pulse
	//  E: for neutrons starting at the end of the pulse
	//  M: for neutrons starting at the very end of the pulse
	//  Phases of the center of the aperture of a chopper
	dTA = dTdelay-0.5*dTp;
	dTC = dTdelay;
	dTE = dTdelay+0.5*dTp;
	dTM = dTdelay+dTpMax-0.5*dTp;
	dLdMinA = Time2Lambda(dLwb, dTA, dTwb-0.5*dTAwb);
	dLdMaxA = Time2Lambda(dLwb, dTA, dTwb+0.5*dTAwb);
	dLdMinC = Time2Lambda(dLwb, dTC, dTwb-0.5*dTAwb);
	dLdMaxC = Time2Lambda(dLwb, dTC, dTwb+0.5*dTAwb);
	dLdMinE = Time2Lambda(dLwb, dTE, dTwb-0.5*dTAwb);
	dLdMaxE = Time2Lambda(dLwb, dTE, dTwb+0.5*dTAwb);
	dLdMinM = Time2Lambda(dLwb, dTM, dTwb-0.5*dTAwb);
	dLdMaxM = Time2Lambda(dLwb, dTM, dTwb+0.5*dTAwb);


	if (cMode=='o')
	{	
		dTrep   = 1.0 / dRepRate;

		/* Evaluation times calculated from TOFs to detector */
		/* -------------------------------------------------- */
		/* first and last time of arrival of neutrons at detector */
		dTMin   = dLDet * dLdMinM *1.0e-10 * M_NEUTRON / H_PLANCK + dTM;
		dTMax   = dLDet * dLdMaxA *1.0e-10 * M_NEUTRON / H_PLANCK + dTA;

		/* In case of overlap of pulses:
			  max. time of previous pulse determines min. eval. time  
			  min. time of following pulse determines max. eval. time */
		if ((dTMax - dTMin) > dTrep)
		{	dTevMin = dTMax - dTrep;
			dTevMax = dTMin + dTrep;
		}
		else
		{	dTevMin = dTMin;
			dTevMax = dTMax;
		} 
		/* time of arrival for desired wavelengths */
		dTevDW1 = dLDet * dLdMinA*1.0e-10 * M_NEUTRON / H_PLANCK + dTA;
		dTevDW2 = dLDet * dLdMaxE*1.0e-10 * M_NEUTRON / H_PLANCK + dTE;

		/* optimal evaluation time (without half-shadow) */
		dTevOp1 = dTevMin + dVwb * dThsWb;  // fmax(dTevDW1,dTevMin) + dVwb * dThsWb;
		dTevOp2 = dTevMax - dVwb * dThsWb;  //fmin(dTevDW2,dTevMax) - dVwb * dThsWb;

		// wavelength range that can be evaluated
		dLmbdOp1 = 3956.0346 / (dLDet/(dTevOp1-dTdelay));
		dLmbdOp2 = 3956.0346 / (dLDet/(dTevOp2-dTdelay));
		
		if (bDouble)
		{	dL1 = dL - 0.5*dDistChop;
			dL2 = dL + 0.5*dDistChop;
		}
		else
		{	dL1 = dL;
			dL2 = dL;
		}

		// Half shadow time and additional opening time of frame overlap chopper
		dVfo    = (dLDet - dL) / dL;
		dThsFo  = 2 * asin(0.5*dBeamFo / dRChop) / (2*PI * dRotFreq);
		dThsFoM = dThsWb * dVwb /dVfo;
		if (cFoMode=='h')
			dAddT   = dTp * (dLwb-dL)/dLwb + (dThsFo-dThsFoM); 
		else
			dAddT   = dTp * (dLwb-dL)/dLwb + dThsFo; 

		// Aperture angle, TOF for frame overlap chopper
		dA    = dAwb*dL/dLwb + 360.0*dAddT/dTprd;
		dT1   = dL1  / dVelMean + dTdelay;
		dT2   = dL2  / dVelMean + dTdelay;
		dTAfo = dA   / 360.0 * dTprd;

		//  minimal and maximal TOF to the WB chopper  and 
		//  minimal and maximal wavelengths            for frame overlap chopper
		dLdMinFM = Time2Lambda(dL2, dTM, dT2 - 0.5 * dA/360.0 * dTprd);
		dLdMaxFA = Time2Lambda(dL2, dTA, dT2 + 0.5 * dA/360.0 * dTprd);

		// min. and max. TOF to detector
		dTdet1  = dLDet * FMax(dLdMinFM, dLdMinM) *1.0e-10 * M_NEUTRON / H_PLANCK + dTM;
		dTdet2  = dLDet * FMin(dLdMaxFA, dLdMaxA) *1.0e-10 * M_NEUTRON / H_PLANCK + dTA;

		// Output
		fprintf(pOutFile, "%7.3f\n", dA);
		fprintf(pOutFile, "%7.3f\n%7.3f\n", dLdMinC, dLdMaxC);
		fprintf(pOutFile, "%7.3f\n%7.3f\n", dLdMinE, dLdMaxA);

		// Phases of the Center of the chopper aperture
		dPhi1  = 360.0 * dRotFreq * dT1;
		if (cRotSense=='r')
		{	dPhi1r = PhaseRed(-dPhi1, &nRot1);
		}
		else
		{	dPhi1r = PhaseRed( dPhi1, &nRot1);
		}
		fprintf(pOutFile, "%8.3f\n%+d\n", dPhi1r, nRot1);
		fprintf(pOutFile, "%8.3f\n%8.3f\n", (dT1-0.5*(dTAfo-dThsFo))*1000., (dT1+0.5*(dTAfo-dThsFo))*1000.);
		if (bDouble)
		{	dPhi2  = 360.0 * dRotFreq * dT2;
			if (cRotSense=='l')
			{	dPhi2r = PhaseRed(-dPhi2, &nRot2);
			}
			else
			{	dPhi2r = PhaseRed( dPhi2, &nRot2);
			}
			fprintf(pOutFile, "%8.3f\n%+d\n", dPhi2r, nRot2);
			fprintf(pOutFile, "%8.3f\n%8.3f\n", (dT2-0.5*(dTAfo-dThsFo))*1000., (dT2+0.5*(dTAfo-dThsFo))*1000.);
		}
		fprintf(pOutFile, "%8.3f\n%8.3f\n", dTdet1 *1000., dTdet2 *1000.);
		fprintf(pOutFile, "%8.3f\n%8.3f\n", dTevMin*1000., dTevMax*1000.);
		fprintf(pOutFile, "%8.3f\n%8.3f\n", dTevOp1*1000., dTevOp2*1000.);
		fprintf(pOutFile, "%8.3f\n%8.3f\n", dLmbdOp1,      dLmbdOp2);
	}
	else
	{	
		// Output
		fprintf(pOutFile, "%8.3f\n%7.3f\n", dLdMinC, dLdMaxC);
		fprintf(pOutFile, "%8.3f\n%7.3f\n", dLdMinE, dLdMaxA);

		// Phases of the Center of the chopper aperture
		dPhi1  = 360.0 * dRotFreq * dTwb;
		if (cRotSense=='r')
		{	dPhi1r = PhaseRed(-dPhi1, &nRot1);
		}
		else
		{	dPhi1r = PhaseRed( dPhi1, &nRot1);
		}
		fprintf(pOutFile, "%8.3f\n%+d\n", dPhi1r, nRot1);
		fprintf(pOutFile, "%8.3f\n%8.3f\n", (dTwb-0.5*(dTAwb-dThsWb))*1000., (dTwb+0.5*(dTAwb-dThsWb))*1000.);
	}

	return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
static void OwnInit(int argc, char *argv[])
{
	long   i;
	char  *arg=NULL;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') 
		{
			arg=&argv[i][2];   
			switch(argv[i][1])
			{
				case 'M':
					cMode = arg[0];
					break;
				case 'o':
					pOutFile = fopen(arg,"w");
					break;

				case 'R':
					/* repetition rate of pulses [1/s] */
					dRepRate = atof(arg);		
					break;
				case 'p':
					/* pulse length */
					dTp = atof(arg);		
					break;
				case 'd':
					/* delay */
					dTdelay = atof(arg);		
					break;
				case 'm':
					/* max. of pulse length */
					dTpMax = atof(arg);		
					break;

				case 's':
					/* rounds / min */
					dRpm = atof(arg);			
					if (dRpm > 0.0)
						cRotSense = 'r';
					else
						cRotSense = 'l';
					dRpm = fabs(dRpm);
					break;
				case 'D':
					/* distance source - detector [m] */
					dLDet = atof(arg);	
					break;

				case 'C':
					/* distance source - (WB) chopper [m] */
					dLwb = atof(arg);	
					break;
				case 'c':
					/* beam diameter at WB chopper */
					dBeam = atof(arg);
					break;
				case 'a':
					/* (WB) chopper aperture [deg] */
					dAwb = atof(arg);	
					break;
				case 'A':
					/* Distance: center of beam - center of chopper [cm] */
					dRChop = atof(arg);	
					break;

				case 'w':
					/* criterion: wavelength range determined by */
					cWBand = arg[0];
					if (cWBand!='m' && cWBand!='a')
						ErrorMsg("For wavelength range determination: Only 'm' or 'a' allowed");
					break;
				case 'W':
					dLambda = atof(arg);
					break;

				case 'F':
					/* distance source - FO chopper [m] */
					dL = atof(arg);	
					break;
				case 'f':
					/* beam diameter at FO chopper */
					dBeamFo = atof(arg);
					break;
				case 'k':
					/* criterion: FO choppers stops no  */
					cFoMode = arg[0];
					if (cFoMode!='n' && cFoMode!='h')
						ErrorMsg("For kind of FO chopper: only 'n' or 'h' allowed");
					break;
			}
		}
	}
}


/*******************************************************/
// calculates wavelength of neutrons 
// starting at 't1' and arriving at 't2' in 'distance' from starting point
/*******************************************************/
static double Time2Lambda(double distance, double t1, double t2)
{
	double lambda, DelT;

	DelT = t2 - t1;
	if (DelT <= 0) 
		DelT = 1.0e-6;
	lambda = H_PLANCK * DelT / (M_NEUTRON * distance * 1.0e-10) ;

	return lambda;
}


/*******************************************************/
/** reduces angle to range [-360, 360] deg            **/
/*******************************************************/
static double PhaseRed(const double dPhiTot, int* pNumRot)
{
	double dPhi=dPhiTot;	

	*pNumRot=0;

	while (dPhi > 360.0)
	{  dPhi -= 360.0;
		*pNumRot += 1;
	}
	while (dPhi <= -360.0)
	{  dPhi += 360.0;
		*pNumRot -= 1;
	}
	return dPhi;
}


/*******************************************************/
/** Prints error message                              **/
/*******************************************************/
static void   ErrorMsg(const char *text) 
{
	fprintf(pOutFile, "ERROR\n%s\n", text);
}
