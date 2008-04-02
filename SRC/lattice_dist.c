/****************************************************************************************/
/* Tool LatticeDistances:                                                               */
/*  Generation of a file containing lattice distances as used in 'SamplePowder'         */
/*                                                                                      */
/* 1.0  Mar 2004  K. Lieutenant    initial version                                      */
/****************************************************************************************/

#include <math.h>
#include <stdio.h>
#include "init.h"

#define FCC    'A'

#define FALSE   0
#define TRUE    1
#define NEW     2
#define MORE    3

#define PI   3.14159265358979323846 
#define MU   1.660542E-27
#define KB   1.380662E-23
#define H    6.626076E-34
#define H_Q  1.054573E-34

static double StrFacFcc(const short  h, const short k, const short l, 
                        const double dScLen1, const double dScLen2);
static long   SquareSum(const short h, const short k, const short l);
static short  NextHkl  (short* p_h, short* p_k, short* p_l, const long nSumMax);
static long   IncHkl   (short* p_h, short* p_k, short* p_l, const short max);
static short  MultPlane(const short h, const short k, const short l);
static int    GetChar  (const char *s);
static double GetDouble(char* pText);
static long   GetLong  (char* pText);
static void   GetString(char* pString, char* pText);

char sBuffer[512];

int main(int argc, char* argv[])
{
	double dA,             // lattice constant
	       dB1, dB2,       // Scattering lengths of Atom 1 and 2 
	       dStrFac,        // Structure Factor without Debye-Waller-faktor
	       dStrFacDw,
	       dStrFacSum,     // Structure Factors incl. Debye-Waller-faktor
	       dDist,dDistOld, // Distance of planes
	       dFdw,           // Debye-Waller-faktor
	       dT,             // sample temperature
	       dTd,            // Debye temperature
	       dMav,           // average mass of an atom
	       dQ;             // momentum transfer
	short  h=0, k=0, l=0, nHaeuf, rc;
	long   nSumMax;
	char   cGitter='A',
	       sFileName[50], 
	      *pFullName;
	FILE*  pFile=NULL;

	Init(argc, argv, VT_TOOL);

	printf("---------------------------------------------------------------------------\n");
	printf("Generation of a file containing lattice distances as used in 'SamplePowder'\n");
	printf("---------------------------------------------------------------------------\n");
	printf("\nWarning: program can only treat the FCC lattice up to now !\n\n");

	// reading of input data
	dA      =  GetDouble("Lattice constant   [Ang]: ");
	dB1     =  GetDouble("scattering length 1 [fm]: ");
	dB2     =  GetDouble("scattering length 2 [fm]: ");
	nSumMax =  GetLong  ("max. h*h + k*k + l*l    : ");
	dTd     =  GetDouble("Debye temperature    [K]: ");
	dT      =  GetDouble("temperature          [K]: ");
	dMav    =  GetDouble("average atomic mass [mu]: ");
	GetString (sFileName,"Name of file            : ");

	dB1  /= 10;  // Umrechnung in 1E-14 m
	dB2  /= 10;
	dMav *= MU;
	dStrFacSum = 0.0;
	dDistOld   = 0.0;

	/* write to parameter directory or to FILES in install directory */
	pFullName = FullParName(sFileName);
	if (strcmp(pFullName, sFileName)==0)
		pFullName = FullInstallName(sFileName, "FILES/");
	pFile = fopen(pFullName, "w");
	if (pFile==NULL) 
	{	printf("\nERROR: Output file %s could not be generated\n", pFullName);
		goto exit;
	}

	/* choice of lattice
	c=getchar();
	do
	{	cGitter = GetChar("Gitter  fcc [A]         : ");
	} 
	while (cGitter!='A'); */

	while ((rc=NextHkl(&h, &k, &l, nSumMax)))
	{	nHaeuf = MultPlane(h,k,l);
		switch (cGitter)
		{
			case FCC:
				dDist   = dA/sqrt(SquareSum(h,k,l));
				dQ      = 2*PI*1.0E10/dDist;  
				dStrFac = StrFacFcc(h,k,l, dB1, dB2);
				break;
			default:
				break;
		}
		if (dStrFac > 0.0)
		{	dFdw     = 1.5 * dQ*dQ * H_Q*H_Q * dT / (dMav * KB * dTd*dTd); 
			dStrFacDw = nHaeuf * dStrFac * exp(-2*dFdw);
			printf ("(%2d %2d %2d)  %8.6lf  %2d *%8.4lf * %7.5lf\n", h,k,l, dDist, nHaeuf, dStrFac, exp(-2*dFdw));
			if (rc==NEW)
			{	if (dStrFacSum > 0.0) 
					fprintf (pFile, "%8.6lf %8.3lf (%2d %2d %2d)\n", dDistOld, dStrFacSum, h,k,l);
				dDistOld    = dDist;
				dStrFacSum  = dStrFacDw;
			}
			else
			{	dStrFacSum += dStrFacDw;
			}
		}
	}
	if (dStrFacSum > 0.0) 
		fprintf (pFile, "%8.6lf %8.3lf\n", dDistOld, dStrFacSum);

	fclose(pFile);
	printf ("\nData written to %s\n", pFullName);

exit:
	printf("\n Hit any key to terminate ! \n");
	getchar();
	getchar();
	getchar();
  
	/* release the buffer memory */
	free(InputNeutrons);
	free(OutputNeutrons);

	return 0;
}

static 
double StrFacFcc(const short  h, const short k, const short l, 
                 const double dScLen1, 
                 const double dScLen2)
{
	double dStrucFac=0.0;
	// bool h_gerade=false, k_gerade=false, l_gerade=false;
	short h_gerade=FALSE, k_gerade=FALSE, l_gerade=FALSE;

	if (h/2*2 == h) h_gerade = TRUE;
	if (k/2*2 == k) k_gerade = TRUE;
	if (l/2*2 == l) l_gerade = TRUE;

	if ( h_gerade &&  k_gerade &&  l_gerade)
		dStrucFac = pow(4*(dScLen1 + dScLen2),2);
	if (!h_gerade && !k_gerade && !l_gerade)
		dStrucFac = pow(4*(dScLen1 - dScLen2),2);

	return dStrucFac;
}


static 
long SquareSum(const short h, const short k, const short l)
{
	return (h*h + k*k + l*l);
}


static 
short NextHkl(short* p_h, short* p_k, short* p_l, const long nSumMax)
{
	short Jmax;
	long  nSum, nSumAkt, nSumAlt;
	
	Jmax = (short) floor(sqrt((double) nSumMax));
	nSumAlt = nSum = SquareSum(*p_h, *p_k, *p_l);

	while (nSum <= nSumMax)
	{
		do
		{	nSumAkt = IncHkl(p_h, p_k, p_l, Jmax);
			if (nSumAkt==nSum && nSumAkt > 0)
			{	if (nSum==nSumAlt) 
					return MORE;
				else
					return NEW;
			}
		}
		while (nSumAkt != 0);

		nSum++;
	}
	return FALSE;
}

static 
long IncHkl(short* p_h, short* p_k, short* p_l, const short max)
{
	short h,k,l;

	h = *p_h; 
	k = *p_k; 
	l = *p_l;

	if      (l < k)   {l++;}
	else if (k < h)   {k++; l=0;}
	else if (h < max) {h++; l=0; k=0;}
	else              {     l=0; k=0; h=0;}

	*p_h = h; 
	*p_k = k; 
	*p_l = l;

	return (SquareSum(h,k,l));
}

static 
short MultPlane(const short h, const short k, const short l)
{
	short mult=48;

	if (h==0) mult /= 2;
	if (k==0) mult /= 2;
	if (l==0) mult /= 2;

	if (h==k && k==l) 
	{	mult /= 6;
	}
	else 
	{	if (h==k) mult /= 2;
		if (k==l) mult /= 2;
		if (h==l) mult /= 2;
	}
	return mult;
}


static
int GetChar (const char *s) 
{
	printf(s);
	fgets(sBuffer, 128, stdin);
	return sBuffer[0];
}

static
double GetDouble(char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

static
long GetLong(char* pText)
{
	long nValue;
	
	printf("%s ", pText);
	scanf ("%ld", &nValue);

	return nValue;
}

static
void GetString(char* pString, char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

