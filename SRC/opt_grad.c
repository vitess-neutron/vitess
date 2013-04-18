/*******************************************************************************************/
/*  Optimization using gradient method                                                     */
/*                                                                                         */
/* 0.1  May 2002  K. Lieutenant  1st version - simulation and fitting routine on same level*/
/* 0.2  Aug 2003  K. Lieutenant  new order in log file                                     */
/* 1.0  Nov 2003  K. Lieutenant  'FitF' and 'Read/WriteStatic' from fit_fct to fit_a.c     */
/* 1.1  Nov 2010  K. Lieutenant  controlling data from parameter file                      */
/* 2.0  Feb 2013  K. Lieutenant  2nd version - simulation routine is main program          */
/*******************************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "general.h"
#include "opt_fct.h"
#include "opt_vars.h"


/*********************************************/
/* static variables                          */
/*********************************************/
static double F0[IMAX+1],     // calculated values F_1 ... F_anz 
              FT[IMAX+1];     //  as a function of parameter set P0 and PT



/*********************************************/
/* prototypes                                */
/*********************************************/
static short ReadIniFile(short*  pOut,  short*  pNZmax, short*  pNDmax, 
                         double* pDamp, double* pTfac,  double* pQverm,  double* pQmin,  
                         const char* sIniFile);


/**********************************************************************************/
/* Least Square Fitting routine                                                   */
/**********************************************************************************/
short OptGrad()
{
	                           /* sets of the parameter set P to be fitted                                  */
	double P0[NMAX+1],         /* P vector of the last step (or starting value)                             */
	       PM[NMAX+1],         /* DeltaX to the new minimum in the linearised function                      */
	       PT[NMAX+1],         /* new P vector to be checked PT = P0 + t * PM                               */
	       Q0=0.0, QT=0.0,     /* error square sum of P0 and PT                                             */
	       D0=0.0,             /* theor. diff. of error square sum in linear. function                      */ 
	       DP[NMAX+1],         /* standard deviation of vector P                                            */
	       TD=1.0, TD0=1.0;    /* (initial) parameter t of the fitting step size                            */
	double DT=0.0,             /* difference of error square sum DT = QT-Q0                                 */
	       SG=0.0,             /* standard deviaton of the measured values with respect to the fitted curve */
	       R[NMAX+1],          /* vector -0.5 grad Q                                                        */
	       Qmin  = 1.0E-24,    /* minimal error square sum                                                  */
	       Damp  = 0.500,      /* damping factor                                                            */
	       Tfac  = 0.500,      /* factor in comparison Q-reduction in linearized and original function      */
	       Qverm = 0.999,      /* minimal ratio: error square sum of step n / error square sum of step n+1  */
	       NM[NMAX+1][NMAX+1], /* matrix of second derivations (normal matrix)                              */
	       NI[NMAX+1][NMAX+1]; /* inverted normal matrix                                                    */
	
	short  mSteps = 25,        /* max. number of fitting steps                             */
	       mDamps = 12,        /* max. number of dampings                                  */
         bDamp  = TRUE,      /* criterion: damping necessary                             */
	       i,                  /* counters of meas. values of Y,F  (1 ... nPts)            */
	       j, m;               /* counters of dim of P  (1 ... nPar)                       */

  FILE*  pFile;


	/*	FITPROGRAMM       
		Version vom 12.04.89
		geschrieben von Klaus Lieutenant
		nach der Standardmethode des Least-Square-Fits
		unter Verwendung der Abschaetzung von D. Braess

		Programmiert werden muessen auf jeden Fall :
		die Funktion F(T;P[1],...,P[nPar]), die an die (Mess-)
		werte Y(T) angepasst werden soll. Dieses geschieht im Unter-
		programm Function();

		Eingelesen bzw. uebergeben werden muessen :
		nPts               die Anzahl der Messpunkte
		nPar                die Anzahl der Fitparameter
		eOut              Parameter, der die Ausgabe steuert
		                  (1): nur Fitergebnisse
		                  (2): (1) + Ergebnisse nach jedem Fitschritt
		                  (3): (2) + einige Funktionswerte nach Fit
		P0[1] .... P0[nPar]   die Startwerte der Fitparameter
		DELX[1] .. DELX[nPar] die Werte von DeltaX für die numerische Differentiation
		Y[1] ..... Y[nPts]    die (Mess-)Werte
		und im allgemeinen
		X[1] ..... X[nPts]    ein Parameter, von dem die Funktionswerte
								  F abbhaengen 

		Das Programm liefert dann die Parameter P[1] ... P[nPar], fuer die
		die Fehlerquadratsumme Q moeglichst klein wird.
  
		Dazu werden die Ableitungen A[I,J] des Funktionswertes F[I]
		nach dem Parameter P[J] benoetigt.
		Diese koennen entweder numerisch als Differenzenquotient berech-
		net werden (ABL='NUM', Voreinstellung),
		oder sie werden analytisch berechnet (ABL='ANA'); beides ge- 
		schieht im Unterprogramm Differentiate()
	*/
	
	// read control parameter for the optimization
	ReadIniFile(&eOut, &mSteps, &mDamps, &Damp, &Tfac, &Qverm, &Qmin, sIniFile);	

	// initialization and writing of initial fit values
	for (j=0; j<=NMAX; j++)
	{	P0[j] = P00[j];
		PT[j] = 0.0;
		PM[j] = 0.0;
		DP[j] = 0.0;
		R [j] = 0.0;
		for (m=0; m<=NMAX; m++)
		{	NM[m][j]=0.0;
			NI[m][j]=0.0;
		}
	}

	fprintf(LogFilePtr, "\nInitial values:\n---------------\n");
	PrintP  (P0, ON);
	Calc1Fct(F0, P0, 0);
	Q0 = SquareSum(F0, ON);

	iStep=1;
	
	while (TRUE)  // optimization loop
	{	
		bDamp=TRUE;

		// a vector to the new minimum has to be calculated and written
		//  minimum of the linearized function   
		for (j=1; j<=nPar; j++)
		{	PM[j] = 0.0;
			for (m=1; m<=nPar; m++)
				NI[j][m]=0.0;
		}
  	fprintf(LogFilePtr, "\n%2d. Step:\n--------\n", iStep);
    fprintf(LogFilePtr, "Derivatives:\n------------\n");

    // calculate functions for all derivatives, build and invert differential matrix 
    if (CalcAllFctsG(P0, DelP, OFF)==FALSE) goto End;
		Differentiate(NM, R, DelP);
		Invert       (NI, NM);

		for (j=1; j<=nPar; j++)
		{	DP[j]=sqrt(NI[j][j]);
		}
		// calculate vector pointing to the new minimum of the linearized function
		for (j=1; j<=nPar; j++)
		{	for (m=1; m<=nPar; m++)
			{	PM[j] = PM[j] + NI[j][m]*R[m];
			}
		}
		D0 = 0.0;
		for (j=1; j<=nPar; j++)
		{	D0 = D0 - R[j]*PM[j];
		}

		TD = TD0;
		for (j=1; j<=nPar; j++)
		{	PT[j] = P0[j] + TD*PM[j];
		}

		PrintP  (PT, ON);
		Calc1Fct(FT, PT, 0);
		QT = SquareSum(FT, ON);

		while (bDamp==TRUE)    // damping loop
		{
			// error square sum of the new parameter set has to be checked
			// change is reduced, if expected improvement is too little
			DT = QT - Q0;

			if (DT <= Tfac*TD*D0)    // new values accepted
			{	
				// (another) damping step is not necessary
				bDamp=FALSE;
				if ((QT/Q0 > Qverm) || (iStep==mSteps) || (QT < Qmin)) 
					goto End;   // fit finished

				// next step
				for (j=1; j<=nPar; j++)
				{	P0[j] = PT[j];
				}
				for (i=1; i<=nPts; i++)
				{	F0[i] = FT[i];
				}
				Q0 = QT;
				iStep++;
			}
			else
			{	//  damping
				TD *= Damp;
				if (TD < pow(Damp, mDamps))
				{	
          // maximal number of dampings reached, go back to previous result and stop optimization
          fprintf(LogFilePtr, " break after %d dampings\n", mDamps);
				  QT = Q0;
				  for (j=1; j<=nPar; j++)
				  {	PT[j] = P0[j];
				  }
     		  for (i=1; i<=nPts; i++)
	    	  {	FT[i] = F0[i];
		      }
				  goto End;
				}
				else
				{	// new parameter set calculated (using the new damping factor) and used to calculate the spectrum
          for (j=1; j<=nPar; j++)
				  	PT[j] = P0[j] + TD*PM[j];
				  if (eOut >= 2) 
					  fprintf(LogFilePtr, "\nt = %7.5f:\n", TD);
				  PrintP  (PT, ON);
				  Calc1Fct(FT, PT, 0);   
				  QT = SquareSum(FT, ON);
				}
			}
		}
	}


	/*     END OF FIT                            */
	/*********************************************/
  End:  
	fprintf(LogFilePtr, "\nOptimization was finished after %d steps\n\nFinal values :\n", iStep);

  if (eOut>=2)
  { 
    pFile=fileOpen("CalcSpec.dat", "wt");
    if (pFile)
    { for (i=1; i<=nPts; i++)
      fprintf(pFile, "%10.5f  %12.5e\n", X[i],FT[i]);
    }
    fclose(pFile);
  }

	if (nPts > nPar)
	{	SG = sqrt(QT/(nPts-nPar));
		for (j=1; j<=nPar; j++)
		{	DP[j]*=SG;
			fprintf(LogFilePtr, " P(%2d) = %13.5e +/-%13.5e\n", j,PT[j],DP[j]);
		}
		fprintf(LogFilePtr, "\nsum of squared errors :%12.4e\nstandard deviation    :%12.4e\n", 
								QT, SG);
	}
	else
	{	for (j=1; j<=nPar; j++)
		{	fprintf(LogFilePtr, " P(%2d) = %13.5e\n", j,PT[j]);
		}
		fprintf(LogFilePtr, "\nsum of squared errors :%12.4e\n", QT);
	}

	return READY;
}
 
/***********************************************************/
/* Function to read optimization control parameters from file 
/*  Input : sIniFile: Name of the file 
/*  Output: *pOut  :  control parameter for output
            *pNZmax:  max. number of fitting steps
            *pNDmax:  max. number of dampings     
            *pDamp :  damping factor              
            *pTfac :  fraction Q-reduction in original to linearized function
            *pQverm:  ratio of Q-reduction within 1 step to stop optimization
            *pQmin :  Q-value to stop fitting   
/***********************************************************/
short ReadIniFile(short*  pOut,  short*  pNZmax, short*  pNDmax, 
                  double* pDamp, double* pTfac,  double* pQverm,  double* pQmin,  
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
			{	case 'a': *pOut   = (short) atoi(sParameter); break;
				case 's': *pNZmax = (short) atoi(sParameter); break;
				case 'n': *pNDmax = (short) atoi(sParameter); break;
				case 'd': *pDamp  = atof(sParameter); break;
				case 't': *pTfac  = atof(sParameter); break;
				case 'r': *pQverm = atof(sParameter); break;
				case 'm': *pQmin  = atof(sParameter); break;
				default : sprintf(sMessage, "unknown parameter in '%s'", sIniFile);
                  Warning(sMessage);
			}
			rp = ReadParameter(&cId, sParameter,  pIniFile);
		}
		fclose(pIniFile);
	}
	else
	{	Warning("opt_grad: file containing control parameters could not be opened, default values are used");
    rc=FALSE;
	}

	return (rc);
}

