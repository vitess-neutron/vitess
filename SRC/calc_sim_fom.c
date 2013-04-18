/********************************************************************************************/
/*  Tool to calculate the values of a fit function                                          */
/*                                                                                          */
/* 0.9   Apr 2011  Klaus Lieutenant  1st version                                            */
/********************************************************************************************/

#include "calc_sim_fom.h"
#include "init.h"

extern
double arP[MAX_SIM][NMAX+1], // function F to parameter set P 
       arF[MAX_SIM][IMAX+1]; // for sets 0 ... nSim-1 


/*********************************************************************/
/*  Prototypes                                                       */
/*********************************************************************/
static short WriteAllP(const char* sFilename, const short mMin, const short mMax, const short nPar);
static short ReadAllF (const char* sFilename, const short mMin, const short mMax);


/*********************************************************************/
/*  G L O B A L    F U N C T I O N S                                 */
/*********************************************************************/
/*********************************************************************/
/* Functions to write parameters sets                                */
/* input : F        : function corresponding to parameter set P      */
/*         X        : parameter X_1 ... X_anz (e.g. wavelength)      */
/*         P        : parameter set, for which the fct is calculated */                 
/*         mPts     : number of points in spectrum                   */
/*         mMin,mMax: indices of first & last function to determine  */
/*         mPar     : number of parameters per set (P1 ...P_npar)    */
/* return: TRUE/FALSE                                                */
/*********************************************************************/
short  ExtFunction (double F[IMAX+1], const double X[IMAX+1], const double P[NMAX+1], const int nPts, const short nPar)
{
  return FALSE;
}

short  ExtFunctions(const double X[IMAX+1], const int nPts, const short mMin, const short mMax, const short nPar)
{
  short rc=FALSE, rcp;

  WriteAllP   ("Pcomm.dat", mMin, mMax, nPar);
  rcp = system("gener_pipe.exe");

  if (rcp > 0)
  { system("Simulations.bat");
    system("FigureOfMerit.bat");
    rc=ReadAllF("Fcomm.dat", mMin, mMax);
  }
  return rc;
}


/********************************************************************/
/*  L O C A L    F U N C T I O N S                                  */
/********************************************************************/
/********************************************************************/
/* Function for writing parameters sets                             */
/* input : sFilename: Name of file to write to                      */
/*         mMin,mMax: indices of first & last function to determine */
/*         mPar     : number of parameters per set (P1 ...P_npar)   */
/* return: TRUE/FALSE                                               */
/********************************************************************/
static short WriteAllP(const char* sFilename, const short mMin, const short mMax, const short nPar)
{
	int   j,m;
	short rc=FALSE;
	FILE* pXFile;
  static 
  short bPrint=TRUE;  // info for gener_pipe about fist call

	pXFile = fopen(sFilename, "w");
	if (pXFile)
	{	
    fprintf(pXFile, "%d %2d\n", bPrint, mMin);
    for (m=mMin; m<=mMax; m++)
    { 
      for (j=1; j<=nPar; j++)
		  {	fprintf(pXFile, "%15.7e ", arP[m][j]);
		  }
      fprintf(pXFile, "\n");
    }
		
		rc=TRUE;
    bPrint=FALSE;
		fclose (pXFile);
	}
  else
  { fprintf(LogFilePtr, "Error: file '%s' could not be opened to write parameter values !\n", sFilename);
    exit(-1);
  }
	return rc;
}


/********************************************************************/
/* Function to read the calculated functions for all parameter sets */
/* Function for writing parameters sets                             */
/* input : sFilename: Name of file to read from                     */
/*         mMin,mMax: indices of first & last function to determine */
/* return: TRUE/FALSE                                               */
/********************************************************************/
static short ReadAllF(const char* sFilename, const short mMin, const short mMax)
{
	FILE* pFFile;
	short m, n,
        rc=FALSE, // return code
	      nSim=0;   // no of lines = number of simulations performed in this step
	char  sLine[CHAR_BUF_LARGE+1];

	for (m=mMin; m < mMax; m++)
		for (n=0; n<=IMAX; n++)
			arF[m][n]=0.0;	

	pFFile = fopen(sFilename, "r");
	if (pFFile != NULL)
	{	
		nSim = (short) LinesInFile(pFFile);

    if (nSim == mMax-mMin+1)
    { rc=TRUE;
	    for (m=mMin; m <= mMax; m++)
		  {	ReadLine  (pFFile, sLine, CHAR_BUF_LARGE);
			  StrgScanLF(sLine, &arF[m][1], IMAX, 0);
		  }
		  fclose (pFFile); 
	  }
  }
  else
  { fprintf(LogFilePtr, "Error: file '%s' could not be opened to read function values !\n", sFilename);
    exit(-1);
  }
	return rc;
}
