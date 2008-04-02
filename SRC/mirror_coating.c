/*******************************************************************************************/
/* Tool MirrorCoating:                                                                     */
/*  Generating reflectivity files for mirror coating as used in the                        */
/*  modules 'guide' and 'bender' from parameters m, R_0, R_m, Q_c and W                    */
/*                                                                                         */
/* 1.0  Sep 2003  K. Lieutenant  initial version                                           */
/* 1.1  Nov 2003  K. Lieutenant  more precise Q-value given                                */
/* 1.2  Mar 2004  K. Lieutenant  files written to parameter directory or install_dir/FILES;*/
/*                               heading                                                   */
/* 1.3  Nov 2005  K. Lieutenant  parameter W added                                         */
/*******************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "init.h"

#define THETA_NI 0.099138
#define PI       3.1415926535898 

// static char sBuffer[128];

double GetDouble(char* pText);
void   GetString(char* pString, char* pText);
double Round    (const double in, const int nDigits);

int main(int argc, char* argv[])
{
	double  dM,           // m      : factor of supermirror
	        dQ,           // Q      : momentum transfer of the reflection
	        dQc,          // Q_c    : crit. momentum transfer  (see figure)
	        dQcNi,        // Q_c(Ni): crit. momentum transfer of nickel
	        dW,           // W      : width of cut-off  [1/Ang]
	        dThetaC,      // theta_c: crit. angle for lambda = 1 Angstroem
	        dThetaM,      // theta_max = m * theta_c(Ni)        (see figure)
	        dThetaW,      // 
	        dTheta,       // theta  : reflection angle for lambda = 1 Angstroem
	        dR0,          // R_0    : reflectivity for 0 <= Q <= Q_c 
	                      //                 (or 0 <= theta <= theta_c)
	        dRm,          // R_m    : reflectivity for Q = m * Q_c(Ni)
	        dR,           // R      : reflectivity for Q or theta, 
	        dAlpha =0.0,  // slope Delta_R / Delta_theta
	        dAlphaQ=0.0;  // slope Delta_R / Delta_Q
	long    i, nLen;
	FILE*   pFile;
	char    sFileName[50], 
	       *pFullName;

	Init(argc, argv, VT_TOOL);

	printf("------------------------------------------------------------------\n");
	printf("Generation of a reflectivity file as used in 'Guide' and 'Bender' \n");
	printf("------------------------------------------------------------------\n\n");
//	printf("                                              ");
//	printf("    |                                         ");
//	printf(" R_0|_____________                            ");
//	printf("    |             .\                          ");
//	printf("    |             .  \                        ");
//	printf("    |             .    \                      ");
//	printf("    |             .      \                    ");
//	printf(" R_m|......................\                  ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    +-------------+---------+-----> Q, theta  ");
//	printf("    0            Q_c     m*Q_c(Ni)            ");
//	printf("               theta_c   m*theta_c(Ni)        ");
//	printf("                                              ");

// WARNING:
//  McStas function has its cut-off at m*theta_c, 
//                              not at m*theta_c(Ni) !!

read:
	dR0      = GetDouble("reflectivity(Q=0)                      ");
	dM       = GetDouble("m   = Qmax / Qmax(Ni)                  ");
	dQc      = GetDouble("Q_c = 4*pi*sin(theta_c)/lambda [1/Ang] \n     (0.021743 for Ni)                 ");
	dRm      = GetDouble("reflectivity(Q=m*Q_c(Ni))              ");
	dW       = GetDouble("width W of cut-off             [1/Ang] \n(typical 0.003; 0 for polygonal shape) ");
	GetString(sFileName, "Name of the mirror file                ");

	dQcNi    = Round(4*PI*sin(PI/180.0*THETA_NI)/1.0, 6);

	if (dM*dQc < dQc)
	{	printf("\nERROR: m*Q_c must not be less than Q_c \nm is meant to extent the Q range to values greater than Q_c \n"); 
		printf("Please repeat the input\n\n");
		goto read; 
	}

	/* write to parameter directory or to FILES in install directory */
	pFullName = FullParName(sFileName);
	if (strcmp(pFullName, sFileName)==0)
		pFullName = FullInstallName(sFileName, "FILES/");
	pFile = fopen(pFullName, "w");
	
	if (pFile!=NULL) 
	{
		dThetaC = 180.0/PI*asin(dQc/(4*PI));
		dThetaW = 180.0/PI*asin(dW/(4*PI));
		dThetaM = dM * THETA_NI;
		nLen    = (long) ((Max(dThetaM,dThetaC) + 6.0*dThetaW)*1000 + 4);

		/* slopes in theta and Q */
		if (dThetaM > dThetaC)
		{	dAlpha  = (dRm - dR0) / (dThetaM  - dThetaC);
			dAlphaQ = (dRm - dR0) / (dM*dQcNi - dQc);
		}

		/* calculate reflectivity for 1 Ang in steps of 0.001 deg
		   and write 10 values into each line                     */
		i=0;
		for (dTheta=0.0; i < nLen; dTheta+=0.001)
		{
			i++;
			if (dTheta < dThetaC)
			{	dR = dR0;
			}
			else	
			{	/* sharp cut-off at m*theta_c(Ni) */
				if (dW==0.0)
				{	if (dTheta > dThetaM)
						dR = 0.0;
					else
						dR = dR0 + dAlpha*(dTheta-dThetaC);
				}
				/* McStas function: smooth cut-off at m*theta_c */
				else
				{	dQ = 4*PI*sin(PI/180.0*dTheta)/1.0;
					dR = dR0 * 0.5*(1.0-tanh((dQ-dM*dQc)/dW)) * (1.0 + dAlphaQ*(dQ-dQc));
				}
			}
			if (10*(i/10) == i)
				fprintf(pFile, "%6.4f\n", dR);
			else
				fprintf(pFile, "%6.4f ",  dR);
		}

		printf ("\nslope in Q: %7.3f Ang\n", dAlphaQ);
		printf ("\nData written to %s\n", pFullName);
		fclose(pFile);
	}
	else
	{	printf("\nERROR: Output file could not be generated\n");
	}

	printf("\n Hit any key to terminate ! \n");
	getchar();
	getchar();
	getchar();
  
	/* release the buffer memory */
	free(InputNeutrons);
	free(OutputNeutrons);

	return 1;
}


double GetDouble(char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

void GetString(char* pString, char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

double Round(const double in, const int nDigits)
{	
	double out;

	out = floor(in * pow(10, nDigits) + 0.5);

	return out / pow(10, nDigits); 
}
