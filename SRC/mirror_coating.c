/*******************************************************************************************/
/* Tool MirrorCoating:                                                                     */
/*  Generating reflectivity files for mirror coating as used in the                        */
/*  modules 'guide' and 'bender' from parameters m, R_0, R_m, Q_c                          */
/*                                                                                         */
/* 1.0  Sep 2003  K. Lieutenant  initial version                                           */
/* 1.1  Nov 2003  K. Lieutenant  more precise Q-value given                                */
/* 1.2  Mar 2004  K. Lieutenant  files written to parameter directory or install_dir/FILES;*/
/*                               heading                                                   */
/*******************************************************************************************/
//    ^
//    |
// R_0|_____________
//    |             .\
//    |             .  \
//    |             .    \
//    |             .      \
// R_m|......................\
//    |             .         |
//    |             .         |
//    |             .         |
//    |             .         |
//    +-------------+---------+-----> Q, theta
//    0            Q_c     m*Q_c(Ni)
//               theta_c   m*theta_c(Ni)

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "init.h"

#define THETA_NI 0.099138
#define PI       3.1415926535898 

double GetDouble(char* pText);
void   GetString(char* pString, char* pText);

int main(int argc, char* argv[])
{
	double  dM,           // m      : factor of supermirror
	        dQc,          // Q_c    : crit. momentum transfer  (see figure)
	        dThetaC,      // theta_c: crit. angle for lambda = 1 Angstroem
	        dThetaM,      // theta_max = m * theta_c(Ni)        (see figure)
	        dTheta,       // theta  : reflection angle for lambda = 1 Angstroem
	        dR0,          // R_0    : reflectivity for 0 <= Q <= Q_c 
	                      //                 (or 0 <= theta <= theta_c)
	        dRm,          // R_m    : reflectivity for Q = m * Q_c(Ni)
	        dR,           // R      : reflectivity for Q or theta, 
	        dAlpha;       // slope Delta_R / Delta_theta
	long    i, nLen;
	FILE*   pFile;
	char    sFileName[50], 
	       *pFullName;

	Init(argc, argv, VT_TOOL);

	printf("------------------------------------------------------------------\n");
	printf("Generation of a reflectivity file as used in 'Guide' and 'Bender' \n");
	printf("------------------------------------------------------------------\n\n");

	dR0      = GetDouble("reflectivity(Q=0)           ");
	dM       = GetDouble("m   = Qmax / Qmax(Ni)       ");
	dQc      = GetDouble("Q_c = 4*pi*sin(theta_c)/lambda\n     (0.021743 for Ni)       ");
	dRm      = GetDouble("reflectivity(Q=m*Q_c(Ni))   ");
	GetString(sFileName, "Name of the mirror file     ");

	/* write to parameter directory or to FILES in install directory */
	pFullName = FullParName(sFileName);
	if (strcmp(pFullName, sFileName)==0)
		pFullName = FullInstallName(sFileName, "FILES/");
	pFile = fopen(pFullName, "w");
	
	if (pFile!=NULL) 
	{
		dThetaC = 180.0/PI*asin(dQc/(4*PI));
		dThetaM = dM * THETA_NI;
		nLen    = (long) (dThetaM*1000+4);
		dAlpha  = (dRm - dR0) / (dThetaM - dThetaC);

		i=0;
		for (dTheta=0.0; i < nLen; dTheta+=0.001)
		{
			i++;
			if (dTheta < dThetaC)
				dR = dR0;
			else if (dTheta > dThetaM)
				dR = 0.0;
			else
				dR = dR0 + dAlpha*(dTheta-THETA_NI);
			if (10*(i/10) == i)
				fprintf(pFile, "%6.4f\n", dR);
			else
				fprintf(pFile, "%6.4f ",  dR);
		}

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

