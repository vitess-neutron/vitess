/********************************************************************************************/
/* Functions for optimizations routines                                                     */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors.                                                                          */
/*                                                                                          */
/* 1.0  Nov 2003  K. Lieutenant  1st version - simulation and fitting routine on same level */
/* 2.0  Feb 2013  K. Lieutenant  2nd version - simulation routine is main program           */
/********************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "init.h"
#include "opt_fct.h"
#include "opt_vars.h"

extern char  sGridOpt[99];


/***********************************************************/
/* Function to deliver calculated function or derivative   */
/*  fct: pointer on function or derivative                 */
/*  m  : derivative to be returned (m=0: function itself)  */
/***********************************************************/
void FctF(double F[IMAX+1], const int m)
{	
	int i;

	for (i=0; i<=IMAX; i++)
	{
		if (i>=1 && i<=nPts)
		{	F[i] = arF[m][i];
		}
		else
		{	F[i] = 0.0;
		}
	}
}


/*********************************************************************/
/* Calculation of the theoretical spectrum as a function of vector P */     
/* input : P     : parameter set (P1, ... Pnpar)                     */
/*       : m     : index of simulation                               */
/* output: F     : fit function                                      */
/* return: TRUE /FALSE                                               */
/*********************************************************************/
short Calc1Fct(double F[IMAX+1], const double P[NMAX+1], const short m)
{
  short rc=FALSE;
  int   i,j;
  FILE* pFile;	

  // set parameter values  (not needed for application 'fit') 
  for (j=1; j<=nPar; j++)
    arP[m][j] = P[j];

  // calculate function 
  switch (eOption)
  { 
    case VT_OPT_PC:
      fclose(LogFilePtr);
      rc=OptFctPc(X, nPts, m, m, nPar);    /* uses arP and arF */
      LogFilePtr = fileOpen(sLogFile, "at");
      for (i=1; i<=nPts; i++)
        F[i] = arF[m][i];
      break;
    case VT_OPT_GRID:
      fclose(LogFilePtr);
      rc=OptFctGrid(X, nPts, m, m, nPar, sGridOpt);    /* uses arP and arF */
      LogFilePtr = fileOpen(sLogFile, "at");
      for (i=1; i<=nPts; i++)
        F[i] = arF[m][i];
      break;
    case VT_FIT_PC:
       rc=FitFctPc(F, X, P, nPts, nPar);
       for (i=1; i<=nPts; i++)
        arF[m][i] = F[i];
      break;
    default: Error("optimization option not (yet) implemented");
  }

  if (eOut==3)
  { 
    fprintf(LogFilePtr, "P:");
    for (j=1; j<=nPar; j++)
      fprintf(LogFilePtr, " %12.5e", P[j]);
    fprintf(LogFilePtr, "\n");

    pFile=fileOpen("CalcSpec.dat", "wt");
    if (pFile) {
      for (i=1; i<=nPts; i++)
        fprintf(pFile, "%10.5f  %12.5e\n", X[i],F[i]);
      fclose(pFile);
    }
  }
  return rc;
}


/**********************************************************************/
/* Calculation of all functions needed for the next optimization step */     
/* input : mMin  : index of first function to be calculated           */
/*         mMax  : index of last function to be calculated            */
/* return: TRUE / FALSE                                               */
/**********************************************************************/
short CalcAllFcts(const short mMin, const short mMax)
{	
  short  i,       // index counting measured values
         m,       // index counting calculations/simulations
         rc=TRUE, // return codes
         rcf;  

  // Initialize
  for (m=mMin; m<=mMax; m++)
    for (i=0; i<=IMAX; i++)
      arF[m][i]=0.0;	

  // Calculate function for all sets P from mMin to mMax
  switch (eOption)
  { 
    case VT_OPT_PC:
      fclose(LogFilePtr);
      rc=OptFctPc(X, nPts, mMin, mMax, nPar);  
      LogFilePtr = fileOpen(sLogFile, "at");
      break;
    case VT_OPT_GRID:
      fclose(LogFilePtr);
      rc=OptFctGrid(X, nPts, mMin, mMax, nPar, sGridOpt);  
      LogFilePtr = fileOpen(sLogFile, "at");
      break;
    case VT_FIT_PC:
      for (m=mMin; m<=mMax; m++)
      { rcf=FitFctPc(arF[m], X, arP[m], nPts, nPar);    /* fct(P) */
        if (rcf==FALSE) rc=FALSE;
      }
      break;
    default: Error("optimization option not (yet) implemented");
  }

  return rc;
}


/*******************************************************************/
/* Calculation of the derivatives of the theoretical spectrum      */     
/* input : P     : parameter set (P1, ... Pnpar)                   */
/*         delP  : variations of the parameters                    */
/*         bF0   : criterion: function itself too (ON/OFF)         */
/* return: TRUE/FALSE                                              */
/*******************************************************************/
short CalcAllFctsG(const double P[NMAX+1], const double delP[NMAX+1], const short bF0)
{	
  short  j,        // index counting parameters
         m,        // index counting simulations
         mMin,     // index of first and last function to calculate
         mMax,     // mMax-Mmin+1 = number of simulations performed in this step
         rc;       // return code

  if (bF0==ON) mMin=0; 
  else         mMin=1;

  // Initialize
  for (m=mMin; m < MAX_SIM; m++)
    for (j=0; j<=NMAX; j++)
      arP[m][j]=0.0;	

  for (m=1; m<=nPar; m++)
  {	for (j=1; j<=nPar; j++)
    {	
      if (m==j)
      { arP[2*m-1][j] = P[j] - delP[j]; 
        arP[2*m  ][j] = P[j] + delP[j]; 
      }
      else
      { arP[2*m-1][j] = P[j]; 
        arP[2*m  ][j] = P[j];
      }
    }
  }
  mMax = 2 * nPar; 

  rc=CalcAllFcts(mMin, mMax);

  return rc;
}


/*******************************************************************/
/* Calculation of the sum of the squared errors                    */     
/* input : F    : function to be used                              */
/*       : nStep: every 'nStep'th step is written to the log file  */
/* return: error square sum                                        */
/*******************************************************************/
double SquareSum(const double F[IMAX+1], const short bPrint)
{
	double Q = 0.0;
	long   i;

	for (i=1; i<=nPts; i++)
	{	Q += pow(F[i]-Y[i], 2) * W[i];
	}
	if (eOut>=2 && LogFilePtr!=NULL && bPrint==ON) 
	{	fprintf(LogFilePtr, " Q = %13.5e\n", Q);
	}
	return Q;
}

/*******************************************************************/
/* Calculation of the normalized error X² = Q/(N_pts*sigma^2)      */     
/* input : F    : function to be used                              */
/*       : nStep: every 'nStep'th step is written to the log file  */
/* return: chi squared                                             */
/*******************************************************************/
double ChiSquared(const double F[IMAX+1], const double sigma, const short bPrint)
{
	double chi2=0.0, Q=0.0;

  Q    = SquareSum(F, OFF);
  chi2 = Chi2FromQ(Q, sigma);

	if (eOut>=2 && LogFilePtr!=NULL && bPrint==ON) 
	{	fprintf(LogFilePtr, " X^2= %13.5e\n", chi2);
	}
	return chi2;
}

double Chi2FromQ(const double Q, const double sigma)
{
  return(Q/(nPts*sq(sigma)));
}

double QFromChi2(const double Chi2, const double sigma)
{
   return(nPts*sq(sigma)*Chi2);
}


/***********************************************************/
/* Printing of P vector                                    */     
/***********************************************************/
void PrintP(const double P[NMAX+1], const short bNL)
{
	long jj;

	if (eOut >= 2 && LogFilePtr != NULL) 
	{	fprintf(LogFilePtr, " P :");
		for (jj=1; jj<=nPar; jj++)
		{	fprintf(LogFilePtr, " %13.5e", P[jj]);
		}
		if (bNL==ON)
			fprintf(LogFilePtr, "\n");
	}
}


/******************************************/
/* Reading one line from a parameter file */
/******************************************/
// input : pFile      : pointer to the parameter file 
// output: cId        : character identifying parameter 
//         sParameter : string containing the parameter value
// return: TRUE/FALSE
/****************************************/
short ReadParameter(char* pId, char* sParameter, FILE* pFile)
{
	int   k, ks=0, ke;   // indices for string 
	short ret=TRUE;      // return code
	char  sBuffer[BUF_LEN];
	char* pPtr; 

	if (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
	{	
		*pId  = sBuffer[0];      // first character identifies parameter

		/* copy after '=' sign */
		pPtr = strchr(sBuffer, '=');
		strcpy(sParameter, pPtr+1);

		/* strip leading blanks */
		ke=strlen(sParameter);
		while (sParameter[ks]==' ') ks++;
		if (ks >0)
			for (k=0; k <= ke-ks; k++)
				sParameter[k] = sParameter[k+ks];
	}
	else
	{	ret=FALSE;
	}

	return ret;
}


/***********************************************************/
/* Calculation of the numerical differentiations,          */
/* and generating matrix of second derivation and vector R */
/*  NM         : matrix of second derivation               */
/*  R          : vector -0.5 grad Q                        */
/*  delP       : diff. in param. set P for num. different. */
/***********************************************************/
void Differentiate(double NM[NMAX+1][NMAX+1], double R[NMAX+1], const double delP[NMAX+1])
{
  short  i, j, m, js=0;
  double A[IMAX+1][NMAX+1], FF1[IMAX+1], FF2[IMAX+1], dNMs=1e99;

	for (j=1; j<=nPar; j++)
	{ R [j]=0.0;
	  for (m=1; m<=nPar; m++)
	  {	NM[j][m]=0.0;
	  }
	}
	
	for (j=1; j<=nPar; j++)
	{	
	  if (eOut==3)
		{	fprintf(LogFilePtr, "Delta P(%2d) =%12.4e\n", j, delP[j]);
		}
		FctF(FF1, 2*j-1);  /* fct(x - del_x) */
		FctF(FF2, 2*j  );  /* fct(x + del_x) */
		
		for (i=1; i<=nPts; i++) 
		{	A[i][j] = (FF2[i]-FF1[i])/(2*delP[j]);
			R[j]   += A[i][j] * (Y[i]-arF[0][i]) * W[i];
			for (m=1; m<=j; m++)
			{	NM[j][m] += A[i][j]*A[i][m]*W[i];
				NM[m][j] = NM[j][m];
			}
		}
		if (NM[j][j] < dNMs) 
		{	dNMs=NM[j][j];
			js=j;
		}
	}
	if (nPts<=10)
	{	for (j=1; j<=nPar; j++)
		{	/* NM[j][j] *= MonteCarlo(0.9999,1.0001);         */
			/* NM[j][j] += MonteCarlo(-0.01*dNMs, 0.01*dNMs); */
			NM[j][j] += 0.01*dNMs;
		}
	}

  if (eOut == 3) 
  { fprintf(LogFilePtr, "\n\nDifferentiations : \n");
    for (i=1; i<=nPts; i+= (short) ((nPts/22)+1))
    {	for (j=1; j<=nPar; j++)
        fprintf(LogFilePtr, "%12.4f ", A[i][j]);
      fprintf(LogFilePtr, "\n");
    }
  } 
  return;
}


/***********************************************************/
/* Inversion of a matrix (NM to NI)                        */
/*  NI: pointer to inverted matrix                         */
/*  NM: pointer to matrix                                  */
/***********************************************************/
void Invert(double NI[NMAX+1][NMAX+1], double NM[NMAX+1][NMAX+1])
{
	short  i,j,m;
	double DiaM, D;

	for (i=1; i<=nPar; i++)
	{	NM[i][i] += 1.0;
	}
	for (m=nPar; m>=1; m--)
	{	DiaM=NM[m][m]-1;
      if (DiaM == 0.0) 
	  {  Error("fit_fct: Matrix diagonal element is 0\n");
         exit(99);
      }
		for (j=1; j<=nPar; j++)
		{	NM[m][j]=NM[m][j]/DiaM;
		}
		for (i=1; i<=nPar; i++)
		{	if (i != m) 
			{	D = NM[i][m];
				for (j=1; j<=nPar; j++)
				{	NM[i][j] = NM[i][j] - D*NM[m][j];
				}
         }
		}
	}

	for (m=1; m<=nPar; m++)
	{	for (j=1; j<=nPar; j++)
		{	NI[m][j]=NM[m][j];
		}
      NI[m][m] -= 1.0;
      if (NI[m][m] == 0.0) 
		{	Warning("fit_fct: Normal matrix singular\n");
         return;
      }
	}

	/* Parameter output of this step*/
   if (eOut == 3) 
	{	fprintf(LogFilePtr, "\n---------------------------\n");
		for (i=1; i<=nPar; i++)
		{	for (j=1; j<=nPar; j++)
			{	fprintf(LogFilePtr, "%12.4f", NI[i][j]);
			}
			fprintf(LogFilePtr, "\n");
		}
		fprintf(LogFilePtr, "\n---------------------------\n");
	}
}
