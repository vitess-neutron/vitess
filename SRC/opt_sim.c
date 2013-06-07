/********************************************************************************************/
/*  Main program to control optimization                                                    */
/*                                                                                          */
/* 1.0  Nov 2010  K. Lieutenant  1st version - simulation and fitting routine on same level */
/* 2.0  Mar 2013  K. Lieutenant  2nd version - simulation routine is main program           */
/********************************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "opt_fct.h"
#include "init.h"

	
/*********************************************/
/* global variables                          */
/*********************************************/
extern 
FILE*  LogFilePtr;     // Pointer on file for output of the progress of the fit

double arP[MAX_SIM][NMAX+1], // parameter sets P, actual (P0) and variations P1 ... P_nSim-1
       arF[MAX_SIM][IMAX+1]; // functions F corresponding to parameter sets 0 ... nSim-1 
double P00 [NMAX+1],         // starting value of vector P               
       Pmin[NMAX+1],         // minimal values for components of vector P
       Pmax[NMAX+1],         // maximal values for components of vector P
       DelP[NMAX+1],         // DeltaP for numerical Differentiation     
       X[IMAX+1],            // parameter X_1 ... X_anz (e.g. temperature, wavelength, field strength) 
       Y[IMAX+1],            // measured data Y_1 ... Y_anz
       W[IMAX+1];            // weight of measuring points 
long   nPts = 0,             // number of measuring points 
       iStep= 0;             // actual fitting step
short  nPar = 0,             // number of fit parameters
       nSim = 0,             // number of simultaneously executed simulations
       eOut = 0,             // parameter to control output
       bParallel=FALSE;      // criterion: parallel computing
      
char  sIniFile[FN_LEN] = "",                // name of the file containing the control parameters
      sParFile[FN_LEN] = "opt_param.ini",   // name of the file containing intial values etc. of fit parameters
      sDatFile[FN_LEN] = "no_file",         // name of the file of the measured values
      sLogFile[FN_LEN] = "Opt.log";         // name of the log file 
char  sMethod[10][18]={"not defined", "opt_grad", "opt_grad_mc", "metropolis", "simplex", "swarm", "genetic"};
char  sParall[ 3][18]={"not defined", "sequential", "parallel"};



/*********************************************/
/* prototypes                                */
/*********************************************/
short OptGrad();
short OptGradMC();
short Metropolis();
short Patrol();

static void  OwnInit    (int argc, char *argv[]);
static void  OwnCleanup ();
static short ReadFitParam (VtFitMethod* pMethod, short* pParallel, 
                           double* pP, double* pPmin, double* pPmax, double* pDelP, const char* sParFilename);
static long  ReadData     (double* pX, double* pY, double* pW,  const char* pDatFilename);


int main(int argc, char* argv[])
{
	short       bCont=0, bIni=FALSE;
	VtFitMethod eMethod=VT_METHOD_NN;

	Init   (argc, argv, VT_TOOL);
	OwnInit(argc, argv);
	if (strcmp(sIniFile,"")!=0) bIni=TRUE;		

	nPts = ReadData    (X, Y, W,  sDatFile);
	nPar = ReadFitParam(&eMethod, &bParallel, P00, Pmin, Pmax, DelP, sParFile);

	switch (eMethod)
	{	case VT_OPT_GRAD: 
      if (!bIni) strcpy(sIniFile, "opt_grad.ini");    
      printf("Optimization has started\n");
      bCont = OptGrad();   
      printf("Optimization has ended\n");
      break;
		case VT_OPT_GRAD_MC: 
      if (!bIni) strcpy(sIniFile, "opt_grad_mc.ini"); 
      printf("Optimization has started\n");
      bCont = OptGradMC(); 
      printf("Optimization has ended\n");
      break;
		case VT_METROPOLIS: 
      if (!bIni) strcpy(sIniFile, "metro.ini");    
      printf("Optimization has started\n");
      bCont = Metropolis();   
      printf("Optimization has ended\n");
      break;
		default: 
      Error("opt_main: optimization algorithm could not be identified");
	}

	OwnCleanup();

	return bCont;
}


/***************************************************************************/
/* Function to read measured data Y_i, corr. parameter X_i and weight P_i  */
/*  input : sDatFilename:  name of the file                                */
/*  output: *pX         :  list of parameters                              */
/*          *pY         :  list of measured values                         */
/*          *pW         :  list of weights                                 */
/*  return: nMeas       :  number of measuring points                      */
/***************************************************************************/
long ReadData(double* pX, double* pY, double* pW, const char* sDatFilename)
{	
	short  i;
	long   nMeas,   // number of measuring points
	       nCol;    // number of columns in file
	FILE*  pDataFile=NULL;
	char   sBuffer[BUF_LEN];

	if (sDatFilename!=NULL)	
		pDataFile = fopen(sDatFilename, "r");

	if (pDataFile==NULL)
	{	nMeas=1;	
		pX[1] = 0.0;
		pY[1] = 0.0;
		pW[1] = 1.0;
	}
	else
	{	nMeas = LinesInFile  (pDataFile);
		nCol  = ColumnsInFile(pDataFile);
		if (nMeas > IMAX)
		{	Warning("opt_main::ReadData: Number of points higher than IMAX, rest ignored"); 
			nMeas = IMAX;
		}
		for (i=1; i<=nMeas; i++)
		{	pX[i] = 0.0;
			pW[i] = 1.0;
		}
		for (i=1; i<=nMeas; i++)
		{	fgets (sBuffer, BUF_LEN, pDataFile);
			if (nCol==1)
				sscanf(sBuffer, "%lf", &pY[i]);
			else
				sscanf(sBuffer, "%lf %lf %lf", &pX[i], &pY[i], &pW[i]);
		}
		fclose(pDataFile);
	}
	return nMeas;
}

/*******************************************************************/
/* Function to read initial, min, max P-values and DeltaX          */
/*  input : sParFile:  name of the input file                      */
/*  output: *pP     :  initial parameter set                       */
/*          *pPmin  :  minimal values for components of vector P   */
/*          *pPmax  :  minimal values for components of vector P   */
/*          *pDelP  :  step siye for numerical differiation        */
/*  return: nP      :  number of fit parameters                    */
/*******************************************************************/
short ReadFitParam(VtFitMethod* pMethod, short* pParallel, 
                   double* pP, double* pPmin, double* pPmax, double* pDelP, const char* sParFilename)
{	
	short j, ind,    // indices
	      nP=0;      // number of fit parameters
	FILE* pParFile;
	char  sDash[80]="---------------------------------------------------------------------------",
        sBuffer[BUF_LEN+1];

	for (j=0; j<=NMAX; j++)
	{	pP   [j]=0.0;
		pPmin[j]=0.0;
		pPmax[j]=0.0;
		pDelP[j]=0.0;
	}
	
	pParFile = fileOpen(sParFilename, "r");

	if (pParFile!=NULL)
	{	
		nP = (short) (LinesInFile(pParFile)-3);
		if (nP > NMAX)
			Error("fit_main: Number of parameters higher than NMAX"); 

	  // read name of the optimization
	  ReadLine(pParFile, sBuffer, BUF_LEN);
    fprintf(LogFilePtr, "%s\n START:  %s\n%s\n\n", sDash, sBuffer, sDash);

		// read method and parallelization option
    ReadLine(pParFile, sBuffer, BUF_LEN);
		for (j=1; j<=6; j++)
			if (strcmp(sBuffer, sMethod[j])==0) *pMethod=(VtFitMethod)j;

	  ReadLine(pParFile, sBuffer, BUF_LEN);
		if (strcmp(sBuffer, sParall[2])==0) *pParallel=TRUE;
		
    // read parameter list
		for (j=1; j<=nP; j++)
		{	ReadLine(pParFile, sBuffer, BUF_LEN);
			sscanf  (sBuffer, "%hd %lg %lg %lg %lg", &ind, &pP[j], &pPmin[j], &pPmax[j], &pDelP[j]);
		}

		fclose(pParFile);
	}
	else
	{	Error("fit_main: file containing fit parameters could not be opened");
	}

	return nP;
}

static void OwnInit(int argc, char *argv[])
{
	long   i;
	char * arg;

	for(i=1; i<argc; i++) 
	{
		arg = argv[i];
		if (*arg !='+') 
		{
			arg += 2;
			switch(arg[-1]) 
			{	
				case 'f':
					strcpy(sIniFile, arg);
					break;
				case 'p':
					strcpy(sParFile, arg);
					break;
				case 'D':
					strcpy(sDatFile, arg);
					break;
				case 'L':
					strcpy(sLogFile, arg);
					break;

				default:
					fprintf(LogFilePtr,"fit_main: unknown commandline option: %s\n", argv[i]);
					exit(-1);
			}
		}
	}
  LogFilePtr = fileOpen(sLogFile, "wt");
}


static void OwnCleanup()
{
  if (LogFilePtr!=NULL) 
		fclose(LogFilePtr);
}


