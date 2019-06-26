/******************************************************************************************/
/*  Optimization using Metropolis algorithm                                               */
/*                                                                                        */
/* 1.00   Mar 2004  Klaus Lieutenant  1st version                                         */
/******************************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "general.h"
#include "opt_fct.h"
#include "opt_vars.h"

typedef enum
{	
	VT_INIT = 0,
	VT_ADD  = 1,
	VT_EXIT = 2
}
VtCheckMode;

typedef enum
{	
	VT_NO_CSTR   = 0,
	VT_CSTR_STD  = 1,
}
VtConstr;


/*********************************************/
/* global variables                          */
/*********************************************/
static double F0[IMAX+1],     // calculated values F_1 ... F_anz 
              FM[IMAX+1];     //  
static long   NZloc;          // number of steps within local minimum 


/*********************************************/
/* prototypes                                */
/*********************************************/
short ReadIniFile(short*  pOut,   short*  pTstPar, VtConstr* pConstr, long*   pSteps, short*  pStpOut, short* pStpMin, 
                  double* pSigma, double* pQverm,  double*   pQmin,  double* pQlimit, 
                  const char* sIniFile);

static short Constraints(const double P[NMAX+1], const short jPar);
static void  ChangePar  (const short m,     const double* pDelP, const VtConstr eConstr);
static void  LocalMin   (const VtCheckMode, const double QT,     const double Q0, 
                         const double* pP,  const double Qlimit);


/**********************************************************************************/
/* Metropolis optimization routine                                                */
/**********************************************************************************/
short Metropolis()
{
                         /* sets of the parameter set P to be optimized   */
  double P0[NMAX+1],     /* P vector of the last step (or starting value) */
         PN[NMAX+1],     /* new P vector to be checked                    */
         PM[NMAX+1],     /* best P vector found so far                    */
         ChiQ0=0.0,      // chi square sum of P0,                 
         ChiQM=9.9E99,   //   PM,
         ChiQT=0.0,      //   the new vectors under test
         ChiQN=0.0,      //   and the best of them 
         Sigma=1.0,      /* standard deviation of a measurement value                                 */
         SG=0.0,         /* standard deviaton of the measured values with respect to the fitted curve */
         Qverm =0.995,   /* maximal ratio of chi square new : chi square old for best set             */
         Qmin  =1.0E-24, /* minimal error square sum - fit terminated                                 */
         Qlimit=0.0;     /* error square sum that determines border of local minimum to be noted      */
  
  short  nStpOut=  10,   /* each 'nStpOut' step is written to the output      */
         nStpMin= 100,   /* each 'nStpMin' step the actual minimum is written */
         nTstPar=   1,   /* number of parameter sets tested in one step       */
         j,              /* counter of dim of P  (1 ... nPar)                 */
         m,              /* counter for calculations                          */
         mOpt;           /* calculation giving the best figure of merit       */
  long   i,              /* counter number of measuring values (1 ... nPts)   */
         mSteps =2500,   /* max. number of fit steps                          */
         iStpTry=   0,   /* number of calculated function (= tried steps)     */
         iStpOpt=   0;   /* step in which best set was found                  */
  char   sConstrText[41];/* text which constraints are used                   */
  VtConstr eConstr=VT_CSTR_STD; /* defines constraint function */
  FILE*  pFile;
  
  /* initialize */
  /**************/
  // read control parameter for the optimization
  ReadIniFile(&eOut, &nTstPar, &eConstr, &mSteps, &nStpOut, &nStpMin, &Sigma, &Qverm, &Qmin, &Qlimit, sIniFile);	
  
  for (j=0; j<=NMAX; j++)
  { P0[j]    = P00[j];
    PN[j]    = P00[j];
    PM[j]    = P00[j];
  }
  iStep=0;
  if (Qlimit > 0.0)
    LocalMin(VT_INIT, 0.0, 0.0, P0, 0.0);
  
  /* start printing into log file */
  switch (eConstr)
  {	case VT_NO_CSTR  : strcpy (sConstrText, "no constraints");       break;
    case VT_CSTR_STD : strcpy (sConstrText, "standard constraints"); break;
    default   : Error("Wrong parameter for constraints");
  }
  fprintf(LogFilePtr, "\n%s\nmax. steps :%6ld\nwriteout   :%6hd %5hd\nSigma      =%10.3f\nChi ratio  =%10.3f\nQ_min      =%15.3e\nQ_limit    =%15.3e\n\n",
  		                sConstrText, mSteps, nStpOut, nStpMin, Sigma, Qverm, Qmin, Qlimit);
  
  for (j=1; j<=nPar; j++)
    fprintf(LogFilePtr, "Delta P[%2d] = %10.3e\n", j, DelP[j]);
  fprintf(LogFilePtr, "\nInitial values:\n-------------\n");

  PrintP  (P0, ON);
  Calc1Fct(F0, P0, 0);
  ChiQ0 = ChiSquared(F0, Sigma, ON);
  ChiQM = ChiQ0;

  
  /* optimization loop */
  /*********************/
  for (iStep=1; iStep<=mSteps; iStep++)  
  {	
    double q=0.5,  /* random number to decide about acceptance             */
           p=0.0;  /* probability of acceptance in case of impaired change */
    short  bAccept=FALSE;

    for (j=0; j<=NMAX; j++)
      arP[0][j]= P0[j];
   
    /* change a parameter until it is accepted */
    do
    { 
      /* vary parameter set in 'nTstPar' different ways and calculate all spectra */
      for (m=1; m<=nTstPar; m++)
        ChangePar(m, DelP, eConstr);
      iStpTry += nTstPar;
    
      /* calculate chi square of each parameter set and take the set with the lowest chi */
      CalcAllFcts(1, nTstPar);
      ChiQN = 1.0e99;
      mOpt  = 9999;
      for (m=1; m<=nTstPar; m++)
      { 
        ChiQT = ChiSquared(arF[m], Sigma, OFF);
        if (ChiQT < ChiQN)
        { mOpt = m;
          ChiQN= ChiQT;
        }
      }  
    
      /* calculate probability of acceptance in case of impaired change */
      if ((ChiQN-ChiQ0) >= 0.0 && (ChiQN-ChiQ0) <= 600.0)
      {	q = MonteCarlo(0.0, 1.0);
    	  p = exp(-(ChiQN-ChiQ0)/2.0);
      }
    
      /* accept in case of improvement or successful MC choice - otherwise: set change back */
      if (ChiQN < ChiQ0 || q < p) 
      { bAccept=TRUE;
        for (j=1; j<=nPar; j++)
    	    PN[j] = arP[mOpt][j];
      }
    }
    while (bAccept==FALSE);
  	
    /* treat local minimum */
    if (Qlimit > 0.0)
    	LocalMin(VT_ADD, QFromChi2(ChiQN, Sigma), QFromChi2(ChiQ0, Sigma), PN, Qlimit);
    
    /* copy parameters and function */
    for (j=1; j<=nPar; j++)
    	P0[j] = arP[mOpt][j];
		for (i=1; i<=nPts; i++)
			F0[i] = arF[mOpt][i];
    ChiQ0=ChiQN;
    
    /* save best parameter set */
    if (ChiQ0 < Qverm*ChiQM)
    {	for (j=1; j<=nPar; j++)
    	{	PM[j] = P0[j];
    	}
    	for (i=1; i<=nPts; i++)
    	{	FM[i] = F0[i];
    	}
    	ChiQM   = ChiQ0;
    	iStpOpt = iStep;
    	if (ChiQM < Chi2FromQ(Qmin, Sigma)) goto End;
    }

    /* print to log file */    
    if (LogFilePtr!=NULL && iStep/nStpOut*nStpOut == iStep) 
    {	fprintf(LogFilePtr, "\n%ld. STEP:\n----------\n", iStep);
    	PrintP(P0, ON);
	  	fprintf(LogFilePtr, " X²= %13.5e\n Q = %13.5e\n", ChiQ0, QFromChi2(ChiQ0, Sigma));
    }
    if (LogFilePtr!=NULL && iStep/nStpMin*nStpMin == iStep) 
    {	fprintf(LogFilePtr, "\nActual best values found in step %ld:\n", iStpOpt);
    	for (j=1; j<=nPar; j++)
    	{	fprintf(LogFilePtr, " P(%2d) = %13.5e\n", j,PM[j]);
    	}
    	fprintf(LogFilePtr, "\nchi squared           :%12.4e\nsum of squared errors :%12.4e\n", 
    							        ChiQM, QFromChi2(ChiQM, Sigma));
    }
  }     // end optimization loop
  iStep--;   

  /* end of optimization */
  /***********************/
  End:  
  if (Qlimit > 0.0 && NZloc > 0)
    LocalMin(VT_EXIT, 0.0, 0.0, PM, 0.0);

  fprintf(LogFilePtr, "\nFit was finished after %ld accepted and %ld tried steps\n\nFinal values found in step %ld:\n",
                      iStep, iStpTry, iStpOpt);
  
  if (nPts > nPar)
  {	SG = sqrt(QFromChi2(ChiQM, Sigma)/(nPts-nPar));
    for (j=1; j<=nPar; j++)
    {	fprintf(LogFilePtr, " P(%2d) = %13.5e\n", j,PM[j]);
    }
    fprintf(LogFilePtr, "\nchi squared           :%12.4e\nsum of squared errors :%12.4e\nstandard deviation    :%12.4e\n", 
                        ChiQM, QFromChi2(ChiQM, Sigma), SG);
  }
  else
  {	for (j=1; j<=nPar; j++)
  	{ fprintf(LogFilePtr, " P(%2d) = %13.5e\n", j,PM[j]);
  	}
  	fprintf(LogFilePtr, "\nchi squared           :%12.4e\nsum of squared errors :%12.4e\n", 
  	                    ChiQM, QFromChi2(ChiQM, Sigma));
  }

  if (eOut>=2)
  { 
    pFile=fileOpen("CalcSpec.dat", "wt");
    if (pFile)
    { for (i=1; i<=nPts; i++)
      fprintf(pFile, "%10.5f  %12.5e\n", X[i],FM[i]);
    }
    fclose(pFile);
  }
  
  return READY;
}


/*****************************************************************************/
/* Function to change one of the parameters
   Input : m      :  index of calculation 
           DelP   :  possible range for the change
           eConstr:  control parameter for constraints                       */
/*****************************************************************************/
static void  
ChangePar(const short m,  const double DelP[NMAX+1],  const VtConstr eConstr)
{
  short  n,               /* parameter to be changed    */
         j,               /* index for parameter        */
         bChangeOk=FALSE; /* result of constraint check */
  double DeltaP;          /* size of change             */
  
  for (j=1; j<=nPar; j++)
    arP[m][j] = arP[0][j];

  while (bChangeOk==FALSE)
  { /* choice of the parameter to be changed */
    n = (short) floor(MonteCarlo(1.0, nPar+1.0));
    
    /* choice of the size of change */
    DeltaP = DelP[n] * MonteCarlo(-1.0, 1.0);
    
    /* check change on constraints */
    arP[m][n] += DeltaP;
    
    switch (eConstr)
    { case VT_NO_CSTR  : break;
      case VT_CSTR_STD : bChangeOk = Constraints(arP[m],n); break;
      default   : Error("Wrong parameter for constraints");
    }
    if (!bChangeOk)
      arP[m][n] -= DeltaP;
  }
}


/*****************************************************************************/
/* Function to check if parameter value is within limits
   Input : P   :  actual parameter set 
           jPar:  index of parameter to check                                */
/*****************************************************************************/
static short
Constraints(const double P[NMAX+1], const short jPar)
{
  short bTest=TRUE;
  
  if (P[jPar] < Pmin[jPar] || P[jPar] < Pmin[jPar]) bTest=FALSE;
  
  return bTest;
}


/*****************************************************************************/
/* Function to determine position and width of local Minima
   Input : eMode :  parameter defining first, last or any other parameter set
           QT    :  error square sum of the actual parameter set
           Q0    :  error square sum of the previous parameter set
           P     :  actual parameter set 
           Qlimit:  error square sum that determines border of local minimum */
/*****************************************************************************/
static void  
LocalMin(const VtCheckMode eMode, const double  QT, const double Q0, const double* pP, const double Qlimit)
{
  short  j;
  double fac  =1.0,       /* weight factor in building the average               */
         P_ave=0.0,       /* average P values inside local minimum               */ 
         P_var=0.0,       /* varianz of P values inside local minimum            */ 
         P_sig=0.0;       /* standard deviation of P values inside local minimum */ 
  static
  double PSum,            /* sum of weights within local minimum     */
         SP1[NMAX+1],     /* sum of x values within local minimum    */
         SP2[NMAX+1];     /* sum of x*x values within local minimum  */
  
  /* initialize (if entering to local minimum) */
  if (eMode==VT_ADD && Q0 > Qlimit && QT < Qlimit  ||  eMode==VT_INIT) 
  {	
    PSum = 0.0;
    NZloc= 0;
    for (j=1; j<=nPar; j++)
    { SP1[j] = 0.0;
      SP2[j] = 0.0;
    }
  }
  
  /* adding current value to the calculation of the local minimum */
  if (eMode==VT_ADD && QT < Qlimit)
  {	
    fac   = (Qlimit - QT);
    PSum += fac;
    NZloc++; 
    for (j=1; j<=nPar; j++)
    { SP1[j] += fac*pP[j];
      SP2[j] += fac*pP[j]*pP[j];
    }
  }
  
  /* calculate average (if leaving local minimum) */
  if (NZloc > 4 && (eMode==VT_ADD && Q0 < Qlimit && QT > Qlimit  ||  eMode==VT_EXIT))
  {	
    fprintf(LogFilePtr, "\n%ld steps within local minimum:\n", NZloc);
    for (j=1; j<=nPar; j++)
    { P_ave = SP1[j]/PSum;
      P_var = SP2[j]/PSum - sq(P_ave);
      if (P_var > 0.0) 
        P_sig = sqrt(P_var);
      else
        P_sig = 0.0;
      fprintf(LogFilePtr, " P(%2d) = %13.5e +/-%13.5e\n", j, P_ave, P_sig);
    }
  } 
  
  return;
}

/*******************************************************************************************/
/* Function to read optimization control parameters from file 
   Input : sIniFile:  Name of the file 
   Output: *pOut   :  control parameter for output 
           *pTstPar:  number of test parameter sets done at the same time
           *pConstr:  control parameter for constraints
           *pSteps :  max. number of optimization steps 
           *pStpOut:  each 'nStpOut' step is written to the output 
           *pStpMin:  each 'nStpMin' step the actual minimum is written
           *pSigma :  standard deviation of a measurement value    
           *pQverm :  ratio of Q-reduction within 1 step to stop optimization
           *pQmin  :  Q-value to stop optimization
           *pQlimit:  error square sum that determines border of local minimum to be noted */
/*******************************************************************************************/
short ReadIniFile(short*  pOut,   short*  pTstPar, VtConstr* pConstr, long*   pSteps, short*  pStpOut, short* pStpMin, 
                  double* pSigma, double* pQverm,  double*   pQmin,  double* pQlimit, 
                  const char* sIniFile)
{	
	short rc=TRUE,
        rp=TRUE;               // return code from 'ReadParameter'
	FILE* pIniFile;
	char  sParameter[BUF_LEN+1], // content of the parameter
	      cId,                   // character defining the parameter
        sMessage[50];
	
	pIniFile = fileOpen(sIniFile, "r");

	if (pIniFile!=NULL)
	{	
		// read file, set parameters and check input
		rp = ReadParameter(&cId, sParameter,  pIniFile);
		while (rp)
		{
			switch (cId)
			{	case 'a': *pOut    = (short) atoi(sParameter); break;
				case 'n': *pTstPar = (short) atoi(sParameter); break;
        case 'c': *pConstr = (VtConstr) atoi(sParameter); break;
				case 's': *pSteps  = atoi(sParameter); break;
				case 'o': *pStpOut = (short) atoi(sParameter); break;
				case 'd': *pStpMin = (short) atoi(sParameter); break;
				case 'g': *pSigma  = atof(sParameter); break;
				case 'r': *pQverm  = atof(sParameter); break;
				case 'm': *pQmin   = atof(sParameter); break;
				case 'l': *pQlimit = atof(sParameter); break;
				default : sprintf(sMessage, "unknown parameter in '%s'", sIniFile);
                  Warning(sMessage);
			}
			rp = ReadParameter(&cId, sParameter,  pIniFile);
		}
		fclose(pIniFile);
	}
	else
	{	Warning("metropolis: file containing control parameters could not be opened, default values are used");
    rc=FALSE;
	}

	return rc;
}

