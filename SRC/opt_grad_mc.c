/********************************************************************************************/
/*  Optimization using gradient method adapted to MC simulations                            */
/*                                                                                          */
/* 1.0  Jan 2011  K. Lieutenant  1st version - simulation and fitting routine on same level */
/* 2.0  Mar 2013  K. Lieutenant  2nd version - simulation routine is main program           */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "general.h"
#include "opt_fct.h"
#include "opt_vars.h"


/*********************************************/
/* global variables                          */
/*********************************************/
static double F0[IMAX+1],     // calculated values F_1 ... F_anz 
              FT[IMAX+1],     //  as a function of parameter set P0 and PT
              F [IMAX+1],     //  any other function
              Q [MAX_SIM];    // error square sum for the different simulations               
static short  eDir[NMAX+1];   // enum for result of variation = number that give improvement (0, 1 or 2)

/*********************************************/
/* prototypes                                */
/*********************************************/
static short ReadIniFile(short*  pOut, short*  pNZmax, short*  pNDmax, 
                         double* pTD0, double* pDamp,  double* pTfac,  double* pQverm, double* pQmin, double* pRDelP,  
                         const char* sIniFile);

/**********************************************************************************/
/* Least Square Fitting routine                                                   */
/**********************************************************************************/
short OptGradMC(char* sIniFile)
{
                             /* sets of the parameter set P to be fitted                                    */
  double P0[NMAX+1],         /* P vector of the last step (or starting value)                               */
         PM[NMAX+1],         /* DeltaX to the new minimum in the linearised function                        */
         PT[NMAX+1],         /* new P vector to be checked PT = P0 + t * PM                                 */
         DelPR[NMAX+1],      /* DeltaP for numerical Differentiation                                        */     
         Q0=0.0, QT=0.0,     /* error square sum of P0 and PT                                               */
         D0=0.0,             /* theor. diff. of error square sum in linear. function                        */ 
         DP[NMAX+1],         /* standard deviation of vector P                                              */
         TD=1.0, TD0=0.5,    /* (initial) factor t in optimization step size between lin. and real function */
         RDelP=0.8;          /* factor by which DelP is reduced in case of deterioration in both directions */
  double DT=0.0,             /* difference of error square sum DT = QT-Q0                                   */
         SG=0.0,             /* standard deviaton of the measured values with respect to the fitted curve   */
         R[NMAX+1],          /* vector -0.5 grad Q                                                          */
         Qmin  = 1.0E-24,    /* minimal error square sum                                                    */
         Damp  = 0.5,        /* damping factor (reduction in optimization step size t_n+1 = Damp * t_n)     */
         Tfac  = 0.333,      /* factor in comparison Q-reduction in linearized and original function        */
         Qverm = 0.995,      /* minimal ratio: error square sum of step n / error square sum of step n+1    */
         NM[NMAX+1][NMAX+1], /* matrix of second derivations (normal matrix)                                */
         NI[NMAX+1][NMAX+1]; /* inverted normal matrix                                                      */
  
  short  mSteps = 25,        /* max. number of fitting steps                             */
         mDamps = 12,        /* max. number of dampings                                  */
         bDamp  = TRUE,      /* criterion: damping necessary                             */
         bDir1  = FALSE,     /* criterion: does direction with eDir==1 exist             */
         nImpr  =  0,        /* number of improvements by varying parameters             */
         eDirV  =  1,        /* enum: which eDir rules variation of parameter            */
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
  
  // read data and starting values etc. for fitting
  if (eOut==0)
    ReadIniFile(&eOut, &mSteps, &mDamps, &TD0, &Damp, &Tfac, &Qverm, &Qmin, &RDelP, sIniFile);	
  
  // initialization and writing of initial fit values
  for (j=0; j<=NMAX; j++)
  { P0[j] = P00[j];
    PT[j] = 0.0;
    PM[j] = 0.0;
    DP[j] = 0.0;
    R [j] = 0.0;
    DelPR[j] = DelP[j];
    for (m=0; m<=NMAX; m++)
    { NM[m][j]=0.0;
      NI[m][j]=0.0;
    }
  }
  
  fprintf(LogFilePtr, "\nInitial values:\n---------------\n");
  PrintP  (P0, ON);
  if (Calc1Fct(F0, P0, 0)==FALSE) goto ErrorExit;
  Q0 = SquareSum(F0, ON);
  
  iStep=1;
  
  while (TRUE)   // optimization loop
  {	
    bDamp=TRUE;
    
    // calculate functions for all derivatives, i.e. P+DelPR and P-DelPR
    fprintf(LogFilePtr, "\n%2d. Step:\n--------\n", iStep);
    fprintf(LogFilePtr, "Derivatives:\n------------\n");
    if (CalcAllFctsG(P0, DelPR, OFF)==FALSE) goto ErrorExit;

    // compare error square sums
    for (j=0; j<=2*nPar; j++)
    {	FctF(F,j);
      Q[j] = SquareSum(F, OFF);
    }	
    nImpr=0;
    bDir1=FALSE;
    for (j=1; j <= nPar; j++)
    {	if (Q[2*j-1] < Q[0] && Q[0] < Q[2*j] ||      // improvement in one, deterioration in the other direction
      	  Q[2*j-1] > Q[0] && Q[0] > Q[2*j]) 
      { eDir[j] = 1;
        bDir1   = TRUE;
      }
      else if (Q[2*j-1] < Q[0] && Q[2*j] <  Q[0])  // improvement in both directions
      { eDir[j] = 2;
      }
      else                                         // deterioration in both directions
      { eDir[j] = 0;
      }
      nImpr+=eDir[j];
    }
    // Stop optimization if no variation gave an improvement    
    if (nImpr==0) 
    {	for (j=1; j<=nPar; j++)   // go back to previous result
        PT[j]=P0[j];
      goto End;  
    }
    if (bDir1) eDirV=1;  // if there is at least 1 direction with improvement in one and deterioration in the other
    else       eDirV=2;  // direction use these cases, otherwise use directions with improvements in both directions
    
    // a vector to the new minimum has to be calculated
    for (j=1; j<=nPar; j++)
    {	PM[j] = 0.0;
      for (m=1; m<=nPar; m++)
        NI[j][m]=0.0;
    }
    
    // build and invert differential matrix 
    Differentiate(NM, R, DelPR);
    Invert       (NI, NM);
    
    for (j=1; j<=nPar; j++)
    { DP[j]=sqrt(NI[j][j]);
    }
    // calculate vector pointing to the new minimum of the linearized function
    for (j=1; j<=nPar; j++)
    { for (m=1; m<=nPar; m++)
      { PM[j] = PM[j] + NI[j][m]*R[m];
      }
    }
    D0 = 0.0;
    for (j=1; j<=nPar; j++)
    { D0 = D0 - R[j]*PM[j];
    }
    
    // calculate new parameter set considering improvements, maximal and minimal values
    TD = TD0;
    for (j=1; j<=nPar; j++)
    {	
      // change parameter with improvement in one direction or in both directions, if the former does not exist
      if (eDir[j]==eDirV)
        PT[j] = P0[j] + TD*PM[j];
      else
        PT[j] = P0[j];
    
      // reduce DelPR, if deterioration in both directions occurs
      if (eDir[j]==0)
        DelPR[j] *= RDelP;
    
      // keep in range between minimum and maximum
      if (PT[j] < Pmin[j]) PT[j]=Pmin[j];
      if (PT[j] > Pmax[j]) PT[j]=Pmax[j];
    }
    
    PrintP  (PT, ON);
    if (Calc1Fct(FT, PT, 0)==FALSE) goto ErrorExit;
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
          goto End;  // fit finished
      
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
      {	/*    Damping    */
        TD *= Damp;
      
        if (TD < pow(Damp, mDamps))   
        {	
          // maximal number of dampings reached, go back to previous result and stop optimization
          fprintf(LogFilePtr, " break after %d dampings\n", mDamps);
      	  QT = Q0;
      	  for (j=1; j<=nPar; j++)     
      	    PT[j] = P0[j];
      	  for (i=1; i<=nPts; i++)
      	    FT[i] = F0[i];
      	  goto End;
        }
        else 
        {
      	  // change parameter with improvement in one direction or in both directions, if the former does not exist
      	  for (j=1; j<=nPar; j++)
      	  { if (eDir[j]==eDirV)
      	      PT[j] = P0[j] + TD*PM[j];
      	    else
      	      PT[j] = P0[j];
      
      	    // keep in range between minimum and maximum
      	    if (PT[j] < Pmin[j]) PT[j]=Pmin[j];
      	    if (PT[j] > Pmax[j]) PT[j]=Pmax[j];
      	  }
      	  if (eOut >= 2) 
      	    fprintf(LogFilePtr, "\nt = %7.5f:\n", TD);
      	  PrintP  (PT, ON);
      	  if (Calc1Fct(FT, PT, 0)==FALSE) goto ErrorExit;   
      	  QT = SquareSum(FT, ON);
        } // end if TD <= ... (max. number of damps)
      }  // end if DT <= ... (parameter set accepted)
    }   // end damping loop
  }    // end optimization loop
  
  
  /*     END OF FIT                            */
  /*********************************************/
 End:  
  fprintf(LogFilePtr, "\nOptimization was finished after %d steps\n", iStep);
  goto Results;
 
 ErrorExit: 
  fprintf(LogFilePtr, "\nOptimization was stopped because of error after %d steps\n", iStep);

 Results:
  fprintf(LogFilePtr, "\nFinal values :\n");
  // print optimized spectrum
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
    { DP[j]*=SG;
      fprintf(LogFilePtr, " P(%2d) = %13.5e +/-%13.5e\n", j,PT[j],DP[j]);
    }
    fprintf(LogFilePtr, "\nsum of squared errors :%12.4e\nstandard deviation    :%12.4e\n", QT, SG);
  }
  else
  {	for (j=1; j<=nPar; j++)
    { fprintf(LogFilePtr, " P(%2d) = %13.5e\n", j,PT[j]);
    }
    fprintf(LogFilePtr, "\nsum of squared errors :%12.4e\n", QT);
  }
  
  return READY;
}
 
/***********************************************************/
/* Function to read fit control parameters from file 
    Input : sIniFile: Name of the file               
    Output: *pOut  :  control parameter for output
            *pNZmax:  max. number of fitting steps
            *pNDmax:  max. number of dampings     
            *pTD0  :  initial factor t in optimization step size between lin. and real function
            *pDamp :  damping factor              
            *pTfac :  fraction Q-reduction in original to linearized function
            *pQverm:  ratio of Q-reduction within 1 step to stop optimization
            *pQmin :  Q-value to stop fitting    
            *pRDelP:  factor by which DelP is reduced in case of deterioration in both directions
    Return: TRUE/FALSE                                     */
/***********************************************************/
short ReadIniFile(short*  pOut, short*  pNZmax, short*  pNDmax, 
                  double* pTD0, double* pDamp,  double* pTfac,  double* pQverm, double* pQmin, double* pRDelP,  
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
      { case 'a': *pOut    = (short) atoi(sParameter); break;
        case 's': *pNZmax  = (short) atoi(sParameter); break;
        case 'n': *pNDmax  = (short) atoi(sParameter); break;
        case 'D': *pTD0    = atof(sParameter); break;
        case 'd': *pDamp   = atof(sParameter); break;
        case 't': *pTfac   = atof(sParameter); break;
        case 'r': *pQverm  = atof(sParameter); break;
        case 'm': *pQmin   = atof(sParameter); break;
        case 'R': *pRDelP  = atof(sParameter); break;
        default : sprintf(sMessage, "unknown parameter in '%s'", sIniFile);
                  Warning(sMessage);
      }
      rp = ReadParameter(&cId, sParameter,  pIniFile);
    }
    fclose(pIniFile);
  }
  else
  { Warning("opt_grad_mc: file containing control parameters could not be opened, default values are used");
    rc=FALSE;
  }
  
  return rc;
}

