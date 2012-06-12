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
/* 1.4  May 2012  K. Lieutenant  parameter beta added                                      */
/*******************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "init.h"

#define THETA_NI 0.099138
#define PI       3.1415926535898 

// static char sBuffer[128];

double GetDouble(const char* pText);
void   GetString(char* pString, const char* pText);
double RoundD   (const double in, const int nDigits);

int main(int argc, char* argv[])
{
	double  M,           // m      : official factor of supermirror
          M2,          // m'     : real SM factor R(Q) profile 
	        Q,           // Q      : momentum transfer of the reflection
	        Qc,          // Q_c    : crit. momentum transfer  (see figure)
	        QcNi,        // Q_c(Ni): crit. momentum transfer of nickel
	        W,           // W      : width of cut-off  [1/Ang]
	        ThetaC,      // theta_c: crit. angle for lambda = 1 Angstroem
	        ThetaM,      // theta_max = m * theta_c(Ni)        (see figure)
	        ThetaW,      // 
	        Theta,       // theta  : reflection angle for lambda = 1 Angstroem
	        R0,          // R_0    : reflectivity for 0 <= Q <= Q_c 
	                     //                 (or 0 <= theta <= theta_c)
	        Rm,          // R_m    : reflectivity for Q = m * Q_c(Ni)
	        R,           // R      : reflectivity for Q or theta, 
	        alpha =0.0,  // slope Delta_R / Delta_theta
	        alphaQ=0.0,  // slope Delta_R / Delta_Q
          betaQ =0.0;  // quadratic term to describe R(q)
  short   bSN=FALSE;   // criterion: use SwissNeutronics parameter
	long    i, nLen;
	FILE*   pFile;
	char    sFileName[50]="", 
	        sSNPar[50]="", 
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
//  cut-off at m*theta_c, 
//      not at m*theta_c(Ni) !!

// read:
	GetString(sSNPar,"quadratic Swiss Neutronics description  ('yes'/'no') ");
	M    = GetDouble("m   = Qmax / Qmax(Ni)                  ");
  if (strcmp(sSNPar, "yes")==0 || strcmp(sSNPar, "Yes")==0 || strcmp(sSNPar, "Y")==0 || strcmp(sSNPar, "y")==0) bSN=TRUE;
  if (bSN)
  {
  	Qc = 0.0217;
    W  = 0.0022 - 0.0002*M;
  	R0 = 0.99;
    M2 = M*0.9853 + 0.1978;   
  }
  else
	{ Qc = GetDouble("Q_c = 4*pi*sin(theta_c)/lambda [1/Ang] \n     (0.0217   for Ni)                 ");
	  W  = GetDouble("width W of cut-off             [1/Ang] \n(typical 0.003; 0 for polygonal shape) ");
	  R0 = GetDouble("reflectivity(Q=0)                      ");
	  Rm = GetDouble("reflectivity(Q=m*Q_c(Ni))              ");
    M2 = M;   
  }
	GetString(sFileName, "Name of the mirror file                ");

	QcNi = RoundD(4*PI*sin(PI/180.0*THETA_NI)/1.0, 6);

	if (M*Qc < Qc)
	{	
		Qc *= M;
		printf("\nNOTE: m*Q_c < 1: therefore  Q_c = %7.5f  set \n\n", Qc); 
	}

	/* write to parameter directory or to FILES in install directory */
	pFullName = FullParName(sFileName);
	// if (strcmp(pFullName, sFileName)==0)
	// 	pFullName = FullInstallName(sFileName, "FILES/");
	pFile = fopen(pFullName, "w");
	
	if (pFile!=NULL) 
	{
		ThetaC = 180.0/PI*asin(Qc/(4*PI));
		ThetaW = 180.0/PI*asin(W/(4*PI));
		ThetaM = M * ThetaC;
		nLen   = (long) ((Max(ThetaM,ThetaC) + 6.0*ThetaW)*1000 + 4);

		/* slopes in theta and Q */
    if (bSN)
    { if (M > 3.0)
      {
        alphaQ =  5.0944 + 0.1204*M;
        betaQ  = 68.1137 - 7.6251*M;
      }
      else
      { alphaQ = M;
        betaQ  = 0.0;
      }
    }
    else
    {	if (ThetaM > ThetaC)
		  {	alpha  = -(Rm - R0) / (ThetaM - ThetaC);
			  alphaQ = -(Rm - R0) / (M*Qc   - Qc);
		  }
    }

		/* calculate reflectivity for 1 Ang in steps of 0.001 deg
		   and write 10 values into each line                     */
		i=0;
		for (Theta=0.0; i < nLen; Theta+=0.001)
		{
			i++;
			if (Theta < ThetaC)
			{	R = R0;
			}
			else	
			{	/* sharp cut-off at m*theta_c(Ni) */
				if (W==0.0)
				{	if (Theta > ThetaM)
						R = 0.0;
					else
						R = R0 - alpha*(Theta-ThetaC);
				}
				/* McStas function: smooth cut-off at m*theta_c */
				else
				{	Q = 4*PI*sin(PI/180.0*Theta)/1.0;
					R = R0 * 0.5*(1.0-tanh((Q-M2*Qc)/W)) * (1.0 - alphaQ*(Q-Qc) + betaQ*(Q-Qc)*(Q-Qc));
				}
			}
			if (10*(i/10) == i)
				fprintf(pFile, "%6.4f\n", R);
			else
				fprintf(pFile, "%6.4f ",  R);
		}

    if (W==0)
		  printf ("\nslope in Q: -%5.3f Ang\n", alphaQ);
    else
		  printf("\nR(Q) = %5.3f * 0.5*(1 - tanh((Q-%5.3f*Qc)/%7.5f)) * (1 + %5.3f*(Q-Qc) - %6.3f*(Q-Qc)^2, Qc=%7.5f 1/Ang\n", R0, M2, W, alphaQ, betaQ, Qc);
		printf("\nData written to %s\n", pFullName);
		fclose(pFile);
	}
	else
	{	printf("\nERROR: Output file could not be generated\n(%s)", pFullName);
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


double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

void GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

double RoundD(const double in, const int nDigits)
{	
	double out;

	out = floor(in * pow(10, nDigits) + 0.5);

	return out / pow(10, nDigits); 
}
