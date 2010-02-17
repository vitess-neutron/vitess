/********************************************************************/
/* Tool DirectView:                                                 */
/*  Determination determine, if the curvature of a bender or guide  */ 
/*  is sufficient to prevent direct view                            */
/*  (whole program in this file)                                    */
/*                                                                  */
/********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define TRUE   1
#define FALSE  0
#define PI     3.14159265358

char sBuffer   [256];

double GetDouble(const char* pText);
void GetString(const char* pText, char* pString);


int main(int argc, char* argv[])
{
	double dR1, dD, dDelD, dAlpha, dAlphaD, dL1=0.0, dL2=0.0, dLen;
	char   cGo='y';

	while (cGo=='y' || cGo=='Y')
	{	
		dL1   = GetDouble("\n\nLength of the first straight part of the guide         [m] ");
		dLen  = GetDouble("Length of the bender/guide/mirror                      [m] ");
		dR1   = GetDouble("Inner radius of the bender/guide/mirror (0 = straight) [m] ");
		dD    = GetDouble("Width of the bender/guide                             [cm] ");
		dLen *= 100.0;
		dL1  *= 100.0;
		dR1  *= 100.0;

		if (dR1==0.0)
		{	double d_in, ta, dAlpha1, dAlpha2, dAlpha0;
			dAlpha1 = GetDouble("Inclination on both sides of the mirror              [deg] ") * PI/180.0;
			dAlpha2 = dAlpha1;
			dAlpha  = dAlpha1 + dAlpha2;
			d_in    = dD - dL1*sin(dAlpha1);
			ta      = d_in/(dLen + dL1/cos(dAlpha1));
			dAlphaD = atan(ta) + dAlpha1;
			if (d_in < 0.0)
			{	dAlpha0 = dAlpha1 - atan(dD/dL1);
				dDelD   = dLen*tan(dAlpha0);
			}
			else
			{	dDelD = 0.0;
			}
		}
		else
		{	
			dAlpha  = dLen/(dR1+dD/2.0);
			if (dL1 == 0.0)
			{	dAlphaD = acos(dR1 / (dR1+dD));
			}
			else
			{	double x, x1, x2, p, q, h, dR2;
				dR2= dR1+dD;
				h  = dL1*dL1 - dR1*dR1;
				p  = dR2*dL1 / h;
				q  = (dR2*dR2 - dR1*dR1) / h;
				x1 = p + sqrt(p*p - q);
				x2 = p - sqrt(p*p - q);
				printf("\nx                           : %8.3f, %8.3f\n",x1, x2);
				if (x2 > 0) x = x2;
				else        x = x1;
				dAlphaD = atan(x);
			}
			if (dAlpha > dAlphaD)
				dDelD   = dR1 / cos(dAlpha - dAlphaD) - dR1;
			else
				dDelD   = 0.0;
		}
		printf("\nmin. curvature for this set-up : %8.2f deg\n",dAlphaD*180/PI); 
		printf("act. curvature                 : %8.2f deg\n",dAlpha*180/PI); 

		if (dAlpha > dAlphaD)
		{	
			printf("\nDeltaD                          : %8.2f cm\n", dDelD); 

			if (dDelD < dD)
			{	dL2 = (dD - dDelD)/ tan(dAlpha - dAlphaD);
				printf("\nmin. length of the straight part: %8.2f cm\n",dL2); 
			}
			else
			{	dL2 = 0.0;
				printf("\nno straight part necessary"); 
			}
		}
		else
		{
			printf("\ncurvature too small ! \n"); 
		}
		
		GetString("\n\n next values ? [y]/[n]", &cGo);
	}

	return 0;
}


double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}


void GetString(const char* pText, char* pString)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}
