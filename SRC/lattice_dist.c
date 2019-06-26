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

static short  PrintHeader(FILE* pFile, char* sSample, double Tdebye, double Tmeas, double A, double M, short bp);

static double GetDouble(const char* pText);
static long   GetLong  (const char* pText);
static void   GetString(char* pString, const char* pText);

char sBuffer[512];

int main(int argc, char* argv[])
{
	double dA,             // lattice constant
	       dB1, dB2,       // Scattering lengths of Atom 1 and 2 
	       dStrFac,        // Structure Factor without Debye-Waller-faktor
	       dStrFacDw,
	       dStrFacDwS,
	       dStrFacSum,     // Structure Factors incl. Debye-Waller-faktor
	       dDist,dDistOld, // Distance of planes
	       dFdw,           // Debye-Waller-faktor
	       dT,             // sample temperature
	       dTd,            // Debye temperature
	       Mamu,           // average mass of an atom [amu]
	       Mav,            // average mass of an atom [kg]
         rho,            // density          [kg/l]
         ucv,            // unit cell volume [Ang^3]
	       dQ;             // momentum transfer
	short  h=0, k=0, l=0, nHaeuf, rc;
	long   nSumMax;
	char   cGitter='A',
         sSample   [256],
	       sPdFileName[50], 
	       sSxFileName[50];
	FILE  *pPdFile=NULL,
        *pSxFile=NULL;

	Init(argc, argv, VT_TOOL);

	printf("-------------------------------------------------------------------------------\n");
	printf("Generation of structure factor files for 'sample_powder' and 'sample_singcryst'\n");
	printf("-------------------------------------------------------------------------------\n");
	printf("\nWarning: up to now, program can only treat FCC lattice!\n\n");

	// reading input data
	GetString(sSample,    "sample                    : ");
	rho      =  GetDouble("material density    [kg/l]: ");
	dB1      =  GetDouble("scattering length 1   [fm]: ");
	dB2      =  GetDouble("scattering length 2   [fm]: ");
	nSumMax  =  GetLong  ("max. h*h + k*k + l*l      : ");
	dTd      =  GetDouble("Debye temperature      [K]: ");
	dT       =  GetDouble("temperature            [K]: ");
	Mamu     =  GetDouble("average atomic mass   [mu]: ");
	GetString(sPdFileName,"name of the powder file   : ");
	GetString(sSxFileName,"name of the sngl Xtal file: ");

	dB1  /= 10;  // Umrechnung in 1E-14 m
	dB2  /= 10;
	Mav = MU*Mamu;
	dStrFacSum = 0.0;
	dDistOld   = 0.0;
  ucv = 4*Mav/rho*1.0e27;
  dA  = pow(ucv, 1.0/3.0);

	/* write to parameter directory */
	pPdFile = fopen(FullParName(sPdFileName), "w");
	pSxFile = fopen(FullParName(sSxFileName), "w");
	if (pPdFile!=NULL) 
          {	PrintHeader(pPdFile, sSample, dTd, dT, dA, Mamu, TRUE);
		fprintf (pPdFile, "# distance   Sigma  ( h  k  l) \n");
		fprintf (pPdFile, "#   [Ang]    [barn]            \n");
		fprintf (pPdFile, "# -----------------------------\n");
  }
	if (pSxFile!=NULL) 
          {	PrintHeader(pSxFile, sSample, dTd, dT, dA, Mamu, FALSE);
		fprintf (pSxFile, "# h  k  l  distance    |F|^2  DW-factor  Sigma  \n");
		fprintf (pSxFile, "#            [Ang]    [barn]            [barn] \n");
		fprintf (pSxFile, "#----------------------------------------------\n");
  }

	if (pPdFile==NULL && pSxFile==NULL) 
	{	printf("\nERROR: Output files %s and %s could both not be generated\n", sPdFileName, sSxFileName);
		goto exit;
	}

	while ((rc=NextHkl(&h, &k, &l, nSumMax)))
	{	nHaeuf = MultPlane(h,k,l);
		switch (cGitter)
		{
			case FCC:
				dDist   = dA/sqrt((double)SquareSum(h,k,l));
				dQ      = 2*PI*1.0E10/dDist;  
				dStrFac = StrFacFcc(h,k,l, dB1, dB2);
				break;
			default:
				break;
		}
		if (dStrFac > 0.0)
		{	dFdw     = 1.5 * dQ*dQ * H_Q*H_Q * dT / (Mav * KB * dTd*dTd); 
			dStrFacDw  = dStrFac * exp(-2*dFdw) * nHaeuf;
			dStrFacDwS = dStrFac * exp(-2*dFdw);
			printf ("(%2d %2d %2d)  %8.6lf  %2d *%8.4lf * %7.5lf\n", h,k,l, dDist, nHaeuf, dStrFac, exp(-2*dFdw));
			fprintf(pSxFile, " %2d %2d %2d  %8.6lf %8.4lf  %7.5lf %9.4lf\n", h, k, l, dDist, dStrFac, exp(-2*dFdw), dStrFacDwS);
			if (rc==NEW)
			{	if (dStrFacSum > 0.0) 
					fprintf (pPdFile, "  %8.6lf %8.3lf (%2d %2d %2d)\n", dDistOld, dStrFacSum, h,k,l);
				dDistOld    = dDist;
				dStrFacSum  = dStrFacDw;
			}
			else
			{	dStrFacSum += dStrFacDw;
			}
		}
	}
	if (dStrFacSum > 0.0) 
		fprintf (pPdFile, "%8.6lf %8.3lf\n", dDistOld, dStrFacSum);

	if(pPdFile) fclose(pPdFile);
	if(pSxFile) fclose(pSxFile);
	printf ("\nData written to %s and %s\n", sPdFileName, sSxFileName);

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


short PrintHeader(FILE* pFile, char* sSample, double Tdebye, double Tmeas, double A, double M, short bPowder)
{
  fprintf (pFile, "# sample: %s\n", sSample);
  fprintf (pFile, "#\n");
  fprintf (pFile, "# d-spacing and structure factors calculated by means of tool 'LatticeDistances'\n");
  if (bPowder)
    fprintf (pFile, "# Sigma(hkl) = multiplicity * |F(hkl)|^2 * F_dw\n");
  else
    fprintf (pFile, "# Sigma(hkl) = |F(hkl)|^2 * F_dw\n");
  fprintf (pFile, "# F_dw       = exp(-3 Q^2 (h/2pi)^2 T_meas / (k_b M_ave T_debye^2))\n");
  fprintf (pFile, "#\n");
  fprintf (pFile, "# Debye temperature: %8.3f K\n",     Tdebye);
  fprintf (pFile, "# temperature      : %8.3f K\n",     Tmeas);
  fprintf (pFile, "# lattice constant : %8.3f Ang\n",   A);
  fprintf (pFile, "# avrg atomic mass : %8.3f amu\n#\n", M);

  return(TRUE);
}		

static
double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

static
long GetLong(const char* pText)
{
	long nValue;
	
	printf("%s ", pText);
	scanf ("%ld", &nValue);

	return nValue;
}

static
void GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

