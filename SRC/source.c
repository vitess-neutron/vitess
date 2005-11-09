/*********************************************************************************************/
/*  VITESS module source                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*       Jan  2001  K. Lieutenant  include of ESS parameters, absolute current values        */
/*       Feb  2001  S. Manoshin    include of gravity effect                                 */	
/* 1.01  June 2001  K. Lieutenant  avoiding p=0 for angular range, wavelength band,...=0,    */
/*                                 4 pi -> 2 pi for CWS, SOFTABORT                           */
/* 1.02  Nov  2001  K. Lieutenant  correction: SolidAngle(divergence), Time dist. fct.       */
/*                                 data SNS, ILL, HMI  and  ESS_C instead of ESS + Temp.     */
/*                                 all time values in source.c in ms                         */
/*                                 scaling factor -> total flux for SPSS, LPSS               */
/* 1.03  Jan  2002  K. Lieutenant  reorganisation                                            */
/* 1.04  Jun  2002  K. Lieutenant  Writing trace files; 'TotalID' and color introduced;      */
/*                                 Correction: Source ESS_C, ESS_T; dual-spectral moderator; */
/*                                 double for number of trajectories; better error messages  */
/* 1.05  Jul  2002  K. Lieutenant  dual-spectral moderator; declination of moderator         */
/*                                 length of ESS-LP from input, not fixed to 2 ms any more   */
/* 1.06  Apr  2003  K. Lieutenant  determination of flight direction by position at window   */
/*                                 source consisting of 1 to 3 moderators with separate data */
/*                                 F(lambda,t), new mode of tracing                          */
/* 1.07  Jun  2003  K. Lieutenant  some little corrections                                   */
/* 1.08  Nov  2003  K. Lieutenant  changes for 'instrument.dat'; macro pTabF -> fct. IndLT() */
/* 1.09  Feb  2004  K. Lieutenant  Full...Name(); output completed; time range & source data */
/*                                 back to main window; SourceData -> InstrData & SimData    */
/* 1.10  Mar  2004  K. Lieutenant  Correction: normalisation of flux distr. read from file   */
/* 1.11  Jun  2004  K. Lieutenant  Correction: transformation v <-> phi,theta;               */
/*                                 normalisation of traj. for 'direction by window'          */
/*                                 new way of integration in 'LoadWavelengthDistribution'    */
/* 1.11a Nov  2004  K. Lieutenant  'WriteSimData' extended, 'PolDegree'+'FracPolDir' introdu.*/
/* 1.11b Dec  2004  K. Lieutenant  solid angle calculation to general.c, (count rate errors) */
/*********************************************************************************************/

#include <string.h>

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "src_modchar.h"
#include "message.h"

typedef struct 
{
  int nEnergy;        ///< Number of energy bins
  int nTime;          ///< number of time bins

  double* TimeBin;    ///< Time bins
  double* EnergyBin;  ///< Energy bins

  double** Flux;       ///< Flux per bin (integrated)
  double* EInt;        ///< Integrated Energy point
  double Total;        ///< Integrated Total

} ISource;

/* global variables */
TotalID*  g_pTrace=NULL;       /* table of trajectory IDs for tracing             */
long      g_nLinesTr=0;        /* Number of lines in the trace file               */
char*     pTraceFileName=NULL;
short     eTraceMode=0,        /* mode 0: no tracing 
                                  mode 1: write trace files for traj. of interest
                                  mode 2: simulation only with traj. of interest  */
  bChoWndPoint=FALSE;  /* criterion 'choose only trajectories that pass window' */
char*     pModFileName=NULL;

short     nNumMod=0,           /* number of moderators in moderator system        */
  imod=0;              /* index of moderators in moderator system         */
double    NumberOfNeutrons=0,
  dTimeMeas   =  0.0,  /* time of measurement in seconds                  */
  dLmbdWant   =  0.0,  /* desired wavelength                              */

  PolVecX     =  0.0,  /* polarisation            */
  PolVecY     =  0.0, 
  PolVecZ     =  0.0, 
  PolDegree   =  0.0,  /* degree of polarization [%] */
  FracPolDir  =  0.0,  /* fraction of neutrons in polarization direction */

  Declination =  0.0,  /* declination between mod. surface normal and propagation window */
  dDistWnd    =  0.0,  /* distance moderator - window                                    */
  WindowHeight= 10.0, 
  WindowWidth = 10.0;
Plane	    Endpoint;

Source    stSrc;             /* source data               */
Moderator stMod   [NUM_MOD]; /* moderator data            */
TrajParam stTraj  [NUM_MOD]; /* trajectory data           */
FctTable  stFluxT [NUM_MOD], /* data of time distr.       */
  stFluxL [NUM_MOD], /* data of wavelength distr. */
  stFluxLT[NUM_MOD]; /* data of wavelength & time distr. */
// FctTable  stTrace;
ISource TS;

/* local functions */
void  OwnCleanup();
void  OwnInit();
void  LoadWavelengthDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL);
void  LoadTimeDistribution      (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxT);  
void  LoadWavelengthTimeDistrib (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL);
void  LoadTraceFile();
char  GetTraceState(TotalID stID);
short ReadModData(char* sFileName);
short PosBehindMod(short i, double Y, double Z);

// ISIS Functions

int cmdnumberD(char *,double*);
int cmdnumberI(char *,int*,const int);
double polInterp(double*,double*,int,double);
FILE* openFile(char*);
int LoadIsisDistrib(FILE*,const double,const double);
int timeStart(char*);
int timeEnd(char*);
int energyBin(char*,double,double,double*,double*);
int notComment(char*);
void ISISgetpoint(double*, double*);
double strArea(double,double,double,double,double);


/*********************************************************************************************/

int main(int argc, char *argv[])
{
  unsigned 
    long	  i=0;
  char    ig1='A', ig2='A',
    sText[50]="";
  long    TransmittedNeutrons=0,
    BufferIndex;
  double  TimeAtModerator,
    No,
    dSolAngle,         /* solid angle of the neutron beam at the moderator              */
    dDecCos,           /* cosinus and sinus of the declination of the moderator         */
    dDecSin,           /* to the instrument direction                                   */
    Phi, Theta,        /* angles of div. from x-dir. in x-y- and x-z-plane (MC choice)  */
    dWndY, dWndZ,      /* position where trajectory passes window 
			  (MC choice for option bChoWndPoint = TRUE)                    */
    Y0,                /* y-position of the starting point of the neutron in the frame 
			  of the moderator                                             */
    dFP,               /* flight path between moderator and window                      */
    TimeOF,            /* time of flight from moderator to window                       */
    helpvalue,         /* random number to define the spin state                        */ 
    CenterX, CenterY,  /* averaged values                                               */
    CenterZ, AveTimeOF,/*     at window                                                 */
    SumProb,           /* sum of probabilities (counts) used to calculate average values*/
    Ymin    = 1000.0, 
    Ymax    =-1000.0,  /* minimal and maximal y-position of moderator system            */
    dFact   =    1.0,  /* for 'direction by window' */
    PolNorm =    0.0;
  // local ISIS variables

  double yW,zW; /* pseudo window width and height with manual divergence */

  VectorType NullPos={0.0,0.0,0.0};
  Neutron Input;
  FILE* IFptr;

  /* Initialize */
  Init             (argc, argv, VT_SOURCE);
  print_module_name("Source and Window 1.11b");
  OwnInit          (argc, argv);
  CenterX   = 0.0; 
  CenterY   = 0.0;
  CenterZ   = 0.0; 
  SumProb   = 0.0;
  AveTimeOF = 0.0;        
  dDistWnd  = -Endpoint.D; 
  nNumMod   = ReadModData(pModFileName);

  /* load trace file */
  LoadTraceFile();

  /* normalise polarization direction */
  PolNorm= sqrt(PolVecX*PolVecX + PolVecY*PolVecY + PolVecZ*PolVecZ);
  if(PolNorm==0.0) Error("you have to give a polarization direction"); 
  PolVecX=PolVecX/PolNorm;
  PolVecY=PolVecY/PolNorm;
  PolVecZ=PolVecZ/PolNorm;


  /* simulation parameters and source characteristics */
  fprintf(LogFilePtr, "\n> Simulation of ");
  if (stSrc.eSrcType == CWS)
    {	fprintf(LogFilePtr, "constant wave source %s <\n\n", stSrc.pSrcName);
      stPicture.eType = CWS;
    }
  else	
    {	if (stSrc.eSrcType==SPSS)
	{	fprintf(LogFilePtr, "short pulsed spallation source %s <\n", stSrc.pSrcName);
	  stPicture.eType = SPSS;
	}
      else
	{	fprintf(LogFilePtr, "long pulsed spallation source %s <\n", stSrc.pSrcName);
	  fprintf(LogFilePtr, "pulse length                 : %7.3f ms \n", 1000*stSrc.dPulseLength);
	  stPicture.eType = LPSS;
	}
      fprintf(LogFilePtr, "pulse frequency              : %7.3f Hz \n\n", stSrc.dPulseFreq);
    }

  /* for all moderators in the system */
  for (imod=0; imod < nNumMod; imod++)
    {
      if (stMod[imod].nColour != 0 || stMod[imod].nBackground!=0)
	fprintf(LogFilePtr, "colour %d   spatial order %d\n", stMod[imod].nColour, stMod[imod].nBackground);
      /* load wavelength distribution and time distribution of pulse */
      if(strlen(stMod[imod].sLTFileName) > 0)
	{ 
	  if (memcmp(stMod[imod].sLTFileName, "Isis", 4)==0)
	    {
	      IFptr=openFile(stMod[imod].sLTFileName);
	      LoadIsisDistrib(IFptr,stTraj->dLambdaMin,stTraj->dLambdaMax);
	      fclose(IFptr);
	    }
	  else
	    {
	      LoadWavelengthTimeDistrib(&stMod[imod], &stTraj[imod], &stFluxLT[imod]);
	    }
	}
      else
	{
	  LoadWavelengthDistribution(&stMod[imod], &stTraj[imod], &stFluxL[imod]);  
	  if (stSrc.eSrcType != CWS)
	    LoadTimeDistribution(&stMod[imod], &stTraj[imod], &stFluxT[imod]);
	}

      /* solid angle [sterad], under which the neutrons leave the moderator */
      if (bChoWndPoint && stMod[imod].dDistModWnd > 0.0)
	{	if (stMod[imod].bCircle)
	    {	dSolAngle = AveSolidAngleC(stMod[imod].dDiameter, WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
	      stMod[imod].dWndFact = AveWeightC(stMod[imod].dCntrY, stMod[imod].dCntrZ, stMod[imod].dDiameter,
						WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
	    }
	  else
	    {
	      // calculate ISIS specific Solid Angle correction
	      if (memcmp(stMod[imod].sLTFileName, "Isis", 4)==0)
		{
		  dSolAngle=strArea(stMod[imod].dWidth/100.0,stMod[imod].dHeight/100.0,stMod[imod].dDistModWnd/100.0,WindowWidth/100.0,WindowHeight/100.0);
		  stMod[imod].dWndFact=1.0;
		}
	      else
		{
		  dSolAngle = AveSolidAngleR(stMod[imod].dWidth, stMod[imod].dHeight, WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
		  /* normalisation factor for 'direction by window' */
		  stMod[imod].dWndFact = AveWeightR(stMod[imod].dCntrY, stMod[imod].dCntrZ,
						    stMod[imod].dWidth, stMod[imod].dHeight,
						    WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
		}
	    }
	}
      else
	{
	      if (memcmp(stMod[imod].sLTFileName, "Isis", 4)==0)
		{
		  fprintf(LogFilePtr, "ERROR: ISIS moderator can only to be used with direction defined by propagation window.\n");
		  exit(-1);
		}
	      else
		{
		  dSolAngle = SolidAngle(stTraj[imod].dMaxDivY, stTraj[imod].dMaxDivZ);
		}
	}
      
      /* calculate flux and mean current of the neutron beam */
      if (stSrc.eSrcType == CWS)
	{  
	  if (stMod[imod].dTotalFlux==0.0)
	    stMod[imod].dTotalFlux = 2*M_PI * stFluxL[imod].dInt;
	}
      else
	{	
	  /* case: flux(lambda,t) was given in a file */
	  if(strlen(stMod[imod].sLTFileName) > 0)
	    {
	      if (memcmp(stMod[imod].sLTFileName, "Isis", 4)==0)
		{
		  // try and give flux in n/s/cm2
		  stMod[imod].dTotalFlux =TS.Total*3.744905847e14*1.1879451;
		  stMod[imod].dFUAmpl=stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq);
		}
	      else
		{
		  if (stMod[imod].dTotalFlux==0.0)
		    stMod[imod].dTotalFlux = 2*M_PI * stFluxLT[imod].dInt * stSrc.dPulseFreq;
		  if (stSrc.dPulseFreq != 0.0)
		    stMod[imod].dFUAmpl = stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq) ;
		}
	    }
	  /* case ESS, SNS */
	  else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
	    {	stMod[imod].dFUAmpl    = TotalFU(stMod[imod].dModTemp, stSrc.eSrcType, stSrc.nSource, stMod[imod].eModType);
	      stMod[imod].dTotalFlux = 2*M_PI * stMod[imod].dFUAmpl * stSrc.dPulseFreq ;
	    }
	  else
	    {	if (stMod[imod].dTotalFlux==0.0)
		stMod[imod].dTotalFlux = 2*M_PI * stFluxL[imod].dInt * stFluxT[imod].dInt * stSrc.dPulseFreq;
	      if (stSrc.dPulseFreq != 0.0)
		stMod[imod].dFUAmpl = stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq) ;
	    }
	  switch(stMod[imod].eModType)
	    {	case MULT_SPEC: fprintf(LogFilePtr, "multi-spectral moderator\n"); break;
	    case POISONED : fprintf(LogFilePtr, "decoupled poisoned moderator\n"); break;
	    case DECOUPLED: fprintf(LogFilePtr, "decoupled unpoisoned moderator\n"); break;
	    case COUPLED  : fprintf(LogFilePtr, "coupled moderator\n"); break;
	    }
	}
      /* current not given => current calculated from flux */
      if (stMod[imod].dCurrent==0.0)
	{	stMod[imod].dCurrent = stMod[imod].dTotalFlux * stMod[imod].dArea * dSolAngle / (2*M_PI);
	}
      /* current given => flux calculated from current, if possible */
      else if (stMod[imod].dArea * dSolAngle > 0.0)
	{	stMod[imod].dTotalFlux = stMod[imod].dCurrent * (2*M_PI) / (stMod[imod].dArea * dSolAngle);
	}
      else 
	{	stMod[imod].dTotalFlux = 0.0;
	}

      /* factor for normalisation (times in seconds) */
      if (stSrc.eSrcType == CWS)
	{  stMod[imod].dNorm = (stTraj[imod].dLambdaMax - stTraj[imod].dLambdaMin) 
	    * stMod[imod].dCurrent / (NumberOfNeutrons/nNumMod);
	  if (stMod[imod].dNorm==0.0)
	    stMod[imod].dNorm = 1.0 / (NumberOfNeutrons/nNumMod);
	}
      else
	{  stMod[imod].dNorm = (stTraj[imod].dLambdaMax - stTraj[imod].dLambdaMin) * (stTraj[imod].dTimeFrmMax - stTraj[imod].dTimeFrmMin)/1000. 
	    * stMod[imod].dCurrent / (NumberOfNeutrons/nNumMod);
	  if (stMod[imod].dNorm==0.0)
	    stMod[imod].dNorm = 1.0 / (NumberOfNeutrons/nNumMod);
	}

      /* write data to output file */
      if (strlen(stMod[imod].sLTFileName) > 0)
	{	fprintf(LogFilePtr, "wavelength-time distr. file  : %s \n",                stMod[imod].sLTFileName);
	}
      else	
	{	if (strlen(stMod[imod].sTFileName) > 0)
	    {	fprintf(LogFilePtr, "time distribution file       : %s \n",             stMod[imod].sTFileName);
	    }
	  else if (stSrc.eSrcType != CWS && stSrc.nSource==ANYSOURCE)
	    {	fprintf(LogFilePtr, "time constants of the pulse  : %7.3f us  , %7.3f us \n",stMod[imod].dTauAscent*1.0e6, 
			stMod[imod].dTauDecay*1.0e6);
	    }

	  if (strlen(stMod[imod].sLFileName) > 0)
	    {	fprintf(LogFilePtr, "wavelength distribution file : %s \n",             stMod[imod].sLFileName);
	    }
	  else
	    {	char text[10]="";
	      fprintf(LogFilePtr, "moderator temperature        : %7.3f K\n",      stMod[imod].dModTemp);
	      sprintf(text, "%5.1f K", stMod[imod].dModTemp);
	      if (imod > 0) 
		strcat (sText, " + ");
	      strcat(sText, text);
	    }
	}
      if (stMod[imod].dTotalFlux > 0)
	fprintf(LogFilePtr, "total neutron flux (in 2*pi) : %11.4e n/(cm²s) \n",   stMod[imod].dTotalFlux);
      fprintf(LogFilePtr, "moderator position           :(%7.3f  %7.3f  %7.3f) cm \n", stMod[imod].dCntrX, stMod[imod].dCntrY, stMod[imod].dCntrZ);
      if (stMod[imod].bCircle)
	{	fprintf(LogFilePtr, "moderator diameter           : %7.3f cm \n",          stMod[imod].dDiameter);
	  Ymin = Min(Ymin, stMod[imod].dCntrY - 0.5*stMod[imod].dDiameter);
	  Ymax = Max(Ymax, stMod[imod].dCntrY + 0.5*stMod[imod].dDiameter);
	}
      else
	{	fprintf(LogFilePtr, "moderator size (W x H)       : %7.3f cm  x %7.3f cm \n", stMod[imod].dWidth, stMod[imod].dHeight);
	  Ymin = Min(Ymin, stMod[imod].dCntrY - 0.5*stMod[imod].dWidth);
	  Ymax = Max(Ymax, stMod[imod].dCntrY + 0.5*stMod[imod].dWidth);
	}
      if (bChoWndPoint)
	fprintf(LogFilePtr, "divergence defined by propagation window \n");
      else
	fprintf(LogFilePtr, "angle of opening used        : %7.3f°    x %7.3f°   \n", 2*180*stTraj[imod].dMaxDivY/M_PI, 2*180*stTraj[imod].dMaxDivZ/M_PI);
      fprintf(LogFilePtr, "time averaged neutron current: %11.4e n/s in%9.6f str\n", stMod[imod].dCurrent, dSolAngle);
      fprintf(LogFilePtr, "wavelength band used         : %7.3f Ang - %7.3f Ang\n", stTraj[imod].dLambdaMin, stTraj[imod].dLambdaMax);
      if (stSrc.eSrcType != CWS)
	fprintf(LogFilePtr, "time interval used           : %7.3f ms  - %7.3f ms \n", stTraj[imod].dTimeFrmMin, stTraj[imod].dTimeFrmMax);
      if (stMod[imod].dCurrent*(stTraj[imod].dLambdaMax-stTraj[imod].dLambdaMin)==0.0)
	Warning("The calculated absolute neutron flux for the given parameter set is zero,\n"
		"probably because one parameter has a zero range (e.g. delta_lambda = 0, mod_area = 0, ...)\n"
		"the simulation is performed with a flux normalized to a max. value of 1 n/(cm²s) \n\n");
      fprintf(LogFilePtr, "\n");
    }

  stPicture.dWPar = Ymax - Ymin;
  stPicture.dRPar = Declination;
  stPicture.nNumber = nNumMod;
  stPicture.pDescr  = sText;
  WriteInstrData(0, NullPos,0.0, 0.0,0.0);
  WriteSimData  (dTimeMeas, dLmbdWant, stSrc.dPulseFreq);

  /* Propagation, Polarisation */
  fprintf(LogFilePtr, "window (W x H)               : %7.3f cm  x %7.3f cm \n", WindowWidth, WindowHeight);
  fprintf(LogFilePtr, "  in a distance of           : %7.3f m   \n",            dDistWnd/100.);
  fprintf(LogFilePtr, "  with a declination of      : %7.3f°    \n",            Declination);
  fprintf(LogFilePtr, "polarization                 : %7.3f %%  X: %5.3f Y: %5.3f Z: %5.3f\n", PolDegree, PolVecX, PolVecY, PolVecZ);
  if (pTraceFileName!=NULL)
    fprintf(LogFilePtr, "trace file used              : %s\n", pTraceFileName);

  /* redefinition in terms of eigenvectors e.g. 0 % means 50% Up and 50% Down */
  FracPolDir  = 0.5 + 0.5*PolDegree/100.0;

  dDecCos =  cos(Declination*M_PI/180.0);
  dDecSin = -sin(Declination*M_PI/180.0);

  /****************************************************************************************/
  /*   Generate neutron trajectories                                                      */
  /****************************************************************************************/

  DECLARE_ABORT

    for(No=0, BufferIndex=0, TransmittedNeutrons=0;No<NumberOfNeutrons;No++)
      {
	CHECK

	  /* ID of the trajectory */
	  if (i==4294967295U)
	    {	i=0; 
	      if (ig2=='Z')
		{	ig2='A'; ig1++;}
	      else
		{	ig2++;}
	    }
	  else
	    {	i++;
	    }
	Input.ID.IDGrp[0] = ig1;
	Input.ID.IDGrp[1] = ig2;
	Input.ID.IDNo     = i;
	if (eTraceMode==WRITE_TRC_FILES)
	  Input.Debug = GetTraceState(Input.ID);
	else
	  Input.Debug = 'N';

	/* choose moderator, if there are more than 1 */
	if (nNumMod > 1)
	  imod = (short) (i % nNumMod); // i - nNumMod*(i/nNumMod); 
	else
	  imod = 0;

	Input.Color = stMod[imod].nColour;

	/* MC choice of starting position */
	if (stMod[imod].bCircle)
	  {	do
	      {	Y0                = stMod[imod].dCntrY + stMod[imod].dDiameter/2.0 - stMod[imod].dDiameter*ran3(&idum);
		Input.Position[2] = stMod[imod].dCntrZ + stMod[imod].dDiameter/2.0 - stMod[imod].dDiameter*ran3(&idum);
	      }	/* repeat if starting point is out of circle */
	    while (sq(Y0-stMod[imod].dCntrY) + sq(Input.Position[2]-stMod[imod].dCntrZ) > sq(stMod[imod].dDiameter/2.0)) ; 
	  }
	else
	  {	Y0                = stMod[imod].dCntrY + stMod[imod].dWidth /2.0 - stMod[imod].dWidth *ran3(&idum);
	    Input.Position[2] = stMod[imod].dCntrZ + stMod[imod].dHeight/2.0 - stMod[imod].dHeight*ran3(&idum);
	  }

	/* check if another moderator is in front of the actual one */
	if (nNumMod > 1)
	  {	short im, bBehind=FALSE;
	    for (im=0; im<nNumMod; im++)
	      {	if (im!=imod && PosBehindMod(im, Y0, Input.Position[2])) bBehind=TRUE;
	      }
	    if (bBehind) continue; 
	  }

	/* Declination */
	Input.Position[1] = Y0 * dDecCos;
	Input.Position[0] = Y0 * dDecSin + stMod[imod].dCntrX;

	/* MC choice of wavelength and starting time */

	if (memcmp(stMod[imod].sLTFileName, "Isis", 4)==0)
	  {
	    //		Input.Wavelength 
	    //		Input.Time       
	    ISISgetpoint(&Input.Time,&Input.Wavelength);
	  }
	else
	  {
	    Input.Wavelength = (double)(stTraj[imod].dLambdaMin  + (stTraj[imod].dLambdaMax-stTraj[imod].dLambdaMin)  *ran3(&idum));
	    Input.Time       = (double)(stTraj[imod].dTimeFrmMin + (stTraj[imod].dTimeFrmMax-stTraj[imod].dTimeFrmMin)*ran3(&idum));
	  }
		
	/*Calculation of intensity expressed by a count rate for this trajectory referring to SPSS, LPSS or CWS */
	if (stSrc.eSrcType == CWS)
	  {  
	    Input.Probability =  stFluxL[imod].pDisFct(Input.Wavelength, stMod[imod].dModTemp)
	      / stFluxL[imod].dInt * stMod[imod].dNorm;
	  }
	else
	  {  
	    TimeAtModerator = Input.Time;
	    if  (stSrc.dPulsePeriod > 0.0)
	      {	while(TimeAtModerator < 0.0)                {TimeAtModerator+=stSrc.dPulsePeriod;}
		while(TimeAtModerator > stSrc.dPulsePeriod) {TimeAtModerator-=stSrc.dPulsePeriod;}
	      }
	    TimeAtModerator *= 0.001;   /*  time in seconds  */
	    
	    /* case: flux(lambda,t) was given in a file */
	    if(strlen(stMod[imod].sLTFileName) > 0)
	      {
		
		// change for 180 uAmp or 60 uAmp..... frequency of source....
		if (memcmp(stMod[imod].sLTFileName, "Isis1", 5)==0)
		  {
		    Input.Probability=TS.Total*3.744905847e14*1.1879451*dSolAngle*WindowWidth*WindowHeight*3.0*(stSrc.dPulseFreq/50)/NumberOfNeutrons;
		  }
		else if (memcmp(stMod[imod].sLTFileName, "Isis2", 5)==0)
		  {
		    Input.Probability=TS.Total*3.744905847e14*1.1879451*dSolAngle*WindowWidth*WindowHeight*(stSrc.dPulseFreq/10)/NumberOfNeutrons;
		  }
		else
		  {
		    Input.Probability =  stFluxLT[imod].pDisFct(Input.Wavelength, TimeAtModerator)
		      / stFluxLT[imod].dInt * stMod[imod].dNorm;
		  }
	      }
	    /* case ESS, SNS */
	    else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
	      {	Input.Probability =  EssModFU(Input.Wavelength, TimeAtModerator, stSrc.dPulseLength)
		  / stMod[imod].dFUAmpl * stMod[imod].dNorm ;
	      }
	    else
	      {	Input.Probability =  stFluxL[imod].pDisFct(Input.Wavelength, stMod[imod].dModTemp)
		  / stFluxL[imod].dInt  
		  * stFluxT[imod].pDisFct(TimeAtModerator, stMod[imod].dTauDecay, stMod[imod].dTauAscent, stSrc.dPulseLength) 
		  / stFluxT[imod].dInt * stMod[imod].dNorm;
	      }
	  }

	if(Input.Probability <= 0.0) continue; 

	/* direction of flight */
	/* defined by starting position on moderator and position on propagation window */
	if (bChoWndPoint && stMod[imod].dDistModWnd > 0.0)
	  {	
	    /* choosing point on propagtion window and calculating distance between points */
	    dWndY = MonteCarlo(-0.5*WindowWidth,  0.5*WindowWidth);
	    dWndZ = MonteCarlo(-0.5*WindowHeight, 0.5*WindowHeight);
	    dFP = sqrt(  sq(stMod[imod].dDistModWnd - Input.Position[0]) 
			 + sq(dWndY    - Input.Position[1])
			 + sq(dWndZ    - Input.Position[2]) );
	    /* correction for gravity effect */
	    if (keygrav==ON)
	      dWndZ += 0.5*G*sq(dFP/V_FROM_LAMBDA(Input.Wavelength))/10000.;

	    Input.Vector[1] = (dWndY - Input.Position[1])/dFP;
	    Input.Vector[2] = (dWndZ - Input.Position[2])/dFP;
	    Input.Vector[0] = sqrt(1.0 - sq(Input.Vector[1]) - sq(Input.Vector[2]));

	    /* correcting count rate for an equal distribution in solid angle 
	       factor: tan'(theta)*tan'(phi) = cos²(theta)*cos²(phi)          */
	    Phi   = atan(Input.Vector[1]/Input.Vector[0]);
	    Theta = atan(Input.Vector[2]/Input.Vector[0]);



	    dFact = sq(cos(Theta)*cos(Phi)) / stMod[imod].dWndFact;
	    Input.Probability *= dFact; 


	  }
	/* defined by divergence */
	else
	  {	Phi   = stTraj[imod].dMaxDivY*(1.0-2.0*ran3(&idum));
	    Theta = stTraj[imod].dMaxDivZ*(1.0-2.0*ran3(&idum));
	    Input.Vector[0] = 1.0 / sqrt(1.0 + sq(tan(Theta)) + sq(tan(Phi)));
	    Input.Vector[1] = Input.Vector[0] * tan(Phi);
	    Input.Vector[2] = Input.Vector[0] * tan(Theta);
	  }


	/* Polarization - spin vectors selected for each trajectory 
	   from one of the eigenvectors  in the polarisation direction */
	helpvalue=ran3(&idum);
	if (helpvalue <= FracPolDir) 
	  {	/* spin eigenvector No 1 */
	    Input.Spin[0]= PolVecX; 
	    Input.Spin[1]= PolVecY; 
	    Input.Spin[2]= PolVecZ; 
	  } 
	else
	  {	/* spin eigenvector No 2 */
	    Input.Spin[0]= -PolVecX; 
	    Input.Spin[1]= -PolVecY; 
	    Input.Spin[2]= -PolVecZ; 
	  } 
		

	/* propagation between Moderator and window */
	if (keygrav==ON)
	  TimeOF = NeutronPlaneIntersectionGrav(&Input,Endpoint);
	else
	  TimeOF = NeutronPlaneIntersection1(&Input,Endpoint);

	/* add time of flight (from mod. to window) and calculate average values at window */
	Input.Time += TimeOF;

	CenterX   += Input.Probability*Input.Position[0];
	CenterY   += Input.Probability*Input.Position[1];
	CenterZ   += Input.Probability*Input.Position[2]; 
	AveTimeOF += Input.Probability*Input.Time;
	SumProb   += Input.Probability;


	if (fabs(Input.Position[1]) > WindowWidth/2.0)  continue;
	if (fabs(Input.Position[2]) > WindowHeight/2.0) continue;

	Input.Position[0]=0.0;

	if (eTraceMode!=ONLY_TRC_TRAJ || GetTraceState(Input.ID)=='T')
	  WriteNeutron(&Input);
      }


 my_exit:

  if (SumProb != 0.0) 
    {
      CenterX   = CenterX/SumProb; 
      CenterY   = CenterY/SumProb;
      CenterZ   = CenterZ/SumProb;
      AveTimeOF = AveTimeOF/SumProb; 
      fprintf(LogFilePtr,"Center of beam at window     :(%7.3f  %7.3f  %7.3f) cm \n", CenterX, CenterY, CenterZ);
      fprintf(LogFilePtr,"Average TOF                  : %7.3f ms \n", AveTimeOF);
    }
  else
    {
      fprintf(LogFilePtr,"\nNo neutrons on the exit of this module \n");
    }	


  /* General simulation settings */
  if(keygrav == 1)
    fprintf(LogFilePtr,"\nGravity is enabled \n");
  else
    fprintf(LogFilePtr,"\nGravity is disabled \n");
  fprintf(LogFilePtr,"Cutoff probability per traj. : %10.3e \n", wei_min);
  // fprintf(LogFilePtr,"random seed                  : %ld \n",  idum);
  fprintf(LogFilePtr,"\nnumber of trajectories started         : %11.0f\n", NumberOfNeutrons);


  /* Do the general cleanup */
  OwnCleanup();
  Cleanup(dDistWnd,-0.5*(Ymax+Ymin),0.0, 0.0,0.0);

  return(0);
}


/* own init of the source module */
/* ----------------------------- */
void OwnInit(int argc, char **argv)
{
  short i;
  char  *arg=NULL;

  /* Initialize */
  Endpoint.D = 0.0;
  stSrc.dPulseLength = 0.002;                /* LPSS pulse length 2 ms            */ 
  stSrc.pSrcName="";

  /*  */
  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') 
	{
	  arg=&argv[i][2];   
	  switch(argv[i][1])
	    {
	      /* Simulation */
	    case 'n':
	      NumberOfNeutrons = atof(arg);
	      break;

	    case 'r':
	      pTraceFileName=arg;  
	      break;
	    case 'k':
	      eTraceMode = (short) atol(arg); 
	      break;

	    case 'd':
	      bChoWndPoint = (short) atol(arg); 
	      if (bChoWndPoint < 0 || bChoWndPoint > 1)
		Error("Wrong parameter for 'direction determination'");
	      break;

	    case 'A':
	      dTimeMeas = (double) atof(arg); /* [s] */
	      break;
	    case 'W':
	      dLmbdWant = (double) atof(arg); /* [s] */
	      break;


	      /* source and moderator */
	    case 'S':
	      stSrc.eSrcType = (short)atoi(arg); /* 1: CWS; 2: SPSS; 3: LPSS; 4: HiLPSS */
	      break;
	    case 'N':
	      stSrc.pSrcName = arg;
	      if (strcmp(arg,"ESS")==0)
		{	stSrc.nSource = ESS;
		}
	      else if (strcmp(arg,"SNS")==0)
		{	stSrc.nSource  = SNS;
		}
	      else 
		{	stSrc.nSource = ANYSOURCE;	  /* no specific source given */
		}
	      break;

	    case 'R':
	      stSrc.dPulseFreq = (double)atof(arg);           /* pulse repetition rate   [Hz] */
	      if (stSrc.dPulseFreq > 0.0)
		stSrc.dPulsePeriod = 1000./stSrc.dPulseFreq; /* time between two pulses [ms] */
	      else
		stSrc.dPulsePeriod = 0.0;
	      break;

	    case 'p':
	      stSrc.dPulseLength = (double)atof(arg); /* LPSS: proton pulselength [ms];  */
	      stSrc.dPulseLength*=0.001;
	      break;

	    case 'a':
	      pModFileName=arg;  
	      break;

	      /* neutron parameters */
	    case 'm':
	      stTraj[0].dLambdaMin = (double)atof(arg); /* [A] */
	      break;
	    case 'M':
	      stTraj[0].dLambdaMax = (double)atof(arg);  /* [A] */
	      break;

	    case 't':
	      stTraj[0].dTimeFrmMin = (double)atof(arg); /* TimeFrame [TimeFrameMin;TimeFrameMax] at Moderator [ms]*/
	      break;
	    case 'T':
	      stTraj[0].dTimeFrmMax = (double)atof(arg);
	      break;

	    case 'y':
	      stTraj[0].dMaxDivY = M_PI*(double)atof(arg)/180.0; /* [deg] */
	      break;
	    case 'z':
	      stTraj[0].dMaxDivZ = M_PI*(double)atof(arg)/180.0;  /* [deg] */
	      break;

	      /*polarization*/
	    case 'X':
	      PolVecX = atof(arg);  
	      break;
	    case 'Y':
	      PolVecY = atof(arg);  
	      break;
	    case 'V':
	      PolVecZ = atof(arg);  
	      break;
	    case 'P':
	      PolDegree = atof(arg); 
	      if(fabs(PolDegree) > 100.)
		Error("polarization degree must be <= 100 ");
	      break;

	      /* propagation */
	    case 'D':									/*  distance moderator propagation window [cm]*/
	      Endpoint.A = 1.0;
	      Endpoint.B = 0.0;
	      Endpoint.C = 0.0;
	      Endpoint.D = -atof(arg);
	      if (Endpoint.D > 0.0)
		Error("Distance moderator to window must have be greater equal zero");
	      break;
	    case 'i':
	      Declination = (double)atof(arg);  /* angle between moderator surface normal and beamline [deg]*/
	      break;
	    case 'w':
	      WindowWidth = (double)atof(arg); /*width of propagation window [cm]*/
	      break;
	    case 'h':
	      WindowHeight = (double)atof(arg);  /* height of propagation window [cm]*/
	      break;

	    default:
	      fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
	      exit(-1);
	      break;
	    }
	}
    }

  if (PolVecX==0.0 && PolVecY==0.0 && PolVecZ==0.0)
    PolVecX=1.0;

  if (Endpoint.D == 0.0 && bChoWndPoint==1)
    Error("Direction can only be defined by window, if distance moderator to window greater zero ");
}
/* End OwnInit */
 


/* own cleanup of the source module */
/* -------------------------------- */
void OwnCleanup()
{
  short m;

  /* print messages of loops (if existing) */
  PrintMessage(SRC_L_RANGE_TOO_SMALL, stMod[imod].sLFileName, OFF);
  PrintMessage(SRC_T_RANGE_TOO_SMALL, stMod[imod].sTFileName, OFF);
  PrintMessage(SRC_LT_RANGE_TOO_SMALL,stMod[imod].sLTFileName,OFF);

  /* free allocated memory */
  for (m=0; m < nNumMod; m++)
    {	if (stFluxL[m].pTabX!=NULL)  free(stFluxL[m].pTabX);
      if (stFluxL[m].pTabF!=NULL)  free(stFluxL[m].pTabF);
      if (stFluxT[m].pTabX!=NULL)  free(stFluxT[m].pTabX);
      if (stFluxT[m].pTabF!=NULL)  free(stFluxT[m].pTabF);
      if (stFluxLT[m].pTabX!=NULL) free(stFluxLT[m].pTabX);
      if (stFluxLT[m].pTabY!=NULL) free(stFluxLT[m].pTabY);
      if (stFluxLT[m].pTabF!=NULL) free(stFluxLT[m].pTabF);
    }
  if (g_pTrace!=NULL) free(g_pTrace);

  /* set description for instrument plot */
  stPicture.dWPar   = WindowWidth;
  stPicture.dHPar   = 0.0;
  stPicture.dRPar   = 0.0;
  stPicture.eType   = 0;
  stPicture.nNumber = 0L;
  stPicture.pDescr  = "";
}
/* End OwnCleanup */



/* load wavelength distribution from file or set 'Maxwellian' as distribution function */
/* ----------------------------------------------------------------------------------- */
void LoadWavelengthDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL)
{
  long   i;
  double dDelX=0.0, dF=0.0;
  char   sBuffer[CHAR_BUF_LENGTH]=""; 
  FILE*  pDisFile=NULL;

  /* loading wavelength distribution file, if its name is given and range is set properly  */
  if(strlen(pMod->sLFileName) > 0) 
    {
      if (pTraj->dLambdaMin >= 0.0  &&  pTraj->dLambdaMax > pTraj->dLambdaMin)
	{
	  /* opening distribution file */
	  pDisFile = fopen(FullParName(pMod->sLFileName),"rt");
	  if (pDisFile==NULL)
	    pDisFile = fopen(FullInstallName(pMod->sLFileName, "FILES/moderators/"),"rt");
	  if (pDisFile!=NULL) 
	    {
	      /* reading number of lines, allocating memory and reading distribution file */
	      pFluxL->nLines = LinesInFile(pDisFile);
	      pFluxL->pTabX  = calloc(pFluxL->nLines, sizeof(double));
	      pFluxL->pTabF  = calloc(pFluxL->nLines, sizeof(double));

	      for(i=0; i < pFluxL->nLines; i++)
		{  
		  ReadLine(pDisFile, sBuffer, sizeof(sBuffer)-1);
		  sscanf  (sBuffer, "%lf %le", &pFluxL->pTabX[i], &pFluxL->pTabF[i]);
		}

	      /* definition of wavelength dist. function after check 
		 if wavelength range of the simulation is covered by data in file */
	      if (pTraj->dLambdaMin >= pFluxL->pTabX[0] && 
		  pTraj->dLambdaMax <= pFluxL->pTabX[pFluxL->nLines-1])
		{
		  pFluxL->pDisFct = UserLambdaDis;
		} 
	      else 
		{  fprintf(LogFilePtr,"ERROR: The wavelength range given in %s is smaller than that in the simulation\n", pMod->sLFileName);
		  exit(-1);
		}

	      /* integration of function f(lambda) and storing of ln f */
	      pFluxL->dInt=0.0;
	      for(i=1; i < pFluxL->nLines; i++)
		{  
		  dDelX =  pFluxL->pTabX[i] - pFluxL->pTabX[i-1];
		  dF    = (pFluxL->pTabF[i] + pFluxL->pTabF[i-1])/2.0;
		  pFluxL->dInt += dF*dDelX;
		}
	      for(i=0; i < pFluxL->nLines; i++)
		{  
		  if (pFluxL->pTabF[i] <= 0.0)
		    pFluxL->pTabF[i] = -100.0;
		  else
		    pFluxL->pTabF[i] = log(pFluxL->pTabF[i]);
		}

	      /* closes distribution file */
	      fclose(pDisFile) ;
	    } 
	  else 
	    {  fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", 
		       pMod->sLFileName);
	      exit (-1);
	    }
	} 
      else 
	{  fprintf(LogFilePtr,"ERROR: You have to specify the wavelength range properly!\n");
	  exit(-1);
	}
    }
  else
    {
      /* otherwise use Maxwellian distribution */
      pFluxL->pDisFct = Maxwellian;
      pFluxL->dInt    = 1.0 ;
    }
}
/* End: LoadWavelengthDistribution() */



/* load time distribution for the pulse from file or set 'PulseShape' or 'PulseInt' as distribution function */
/* --------------------------------------------------------------------------------------------------------- */
void LoadTimeDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxT)
{
  long   i;
  double X1, X2, dDelX=0.0;
  char   sBuffer[CHAR_BUF_LENGTH]=""; 
  FILE*  pDisFile=NULL;

  /* loading time distribution file, if its name is given and range is set properly */
  if(strlen(pMod->sTFileName) > 0) 
    {
      if (pTraj->dTimeFrmMax > pTraj->dTimeFrmMin)
	{
	  /* opening distribution file */
	  pDisFile = fopen(FullParName(pMod->sTFileName),"rt");
	  if (pDisFile==NULL)
	    pDisFile = fopen(FullInstallName(pMod->sTFileName, "FILES/moderators/"),"rt");
	  if (pDisFile!=NULL) 
	    {
	      /* reading number of lines, allocating memory and reading distribution file */
	      pFluxT->nLines = LinesInFile(pDisFile);
	      pFluxT->pTabX = calloc(pFluxT->nLines, sizeof(double));
	      pFluxT->pTabF = calloc(pFluxT->nLines, sizeof(double));

	      for (i=0; i < pFluxT->nLines; i++)
		{  
		  ReadLine(pDisFile, sBuffer, sizeof(sBuffer)-1);
		  sscanf  (sBuffer, "%lf %le", &pFluxT->pTabX[i], &pFluxT->pTabF[i]);
		}

	      /* definition of time distribution function after check 
		 if time range of the simulation is covered by data in file */
	      if (pTraj->dTimeFrmMin >= pFluxT->pTabX[0] && 
		  pTraj->dTimeFrmMax <= pFluxT->pTabX[pFluxT->nLines-1])
		{
		  pFluxT->pDisFct =  UserTimeDis;
		} 
	      else 
		{	fprintf(LogFilePtr,"ERROR: The time range given in %s is smaller than that in the simulation\n", pMod->sTFileName);
		  exit(-1);
		}

	      /* integration  function f(time) and saves ln(f) */
	      pFluxT->dInt=0.0;
	      for (i=0; i < pFluxT->nLines; i++)
		{  /* binning of X_i */
		  if (i==0)
		    {	if (pFluxT->pTabX[i]==0.0)
			{	X1 = 0.0;
			  X2 = 0.5*pFluxT->pTabX[i+1];
			}
		      else
			{	X2 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i+1])) / 2.0);
			  X1 = sq(pFluxT->pTabX[i]) / X2;
			}
		    }
		  else if (i == pFluxT->nLines - 1)
		    {	X1 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i-1])) / 2.0);
		      X2 = sq(pFluxT->pTabX[i]) / X1;
		    }
		  else
		    {	if (pFluxT->pTabX[i-1]==0.0)
			X1 = 0.5*pFluxT->pTabX[i];
		      else
			X1 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i-1])) / 2.0);
		      X2 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i+1])) / 2.0);
		    }
		  dDelX = X2 - X1;

		  /* integration, factor 1000. because time values are given in ms instead of s  */
		  pFluxT->dInt += pFluxT->pTabF[i]*dDelX / 1000.0;

		  /* storing of logarithmic values ln(f(lambda,time)) */
		  if (pFluxT->pTabF[i] <= 0.0)
		    pFluxT->pTabF[i] = -100.0;
		  else
		    pFluxT->pTabF[i] = log(pFluxT->pTabF[i]);
		}

	      /* closing distribution file */
	      fclose(pDisFile) ;
	    } 
	  else 
	    {	fprintf(LogFilePtr,"ERROR: Can't open %s to read user given time distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", pMod->sTFileName);
	      exit (-1);
	    }
	}
      else 
	{	fprintf(LogFilePtr,"ERROR: You have to specify the -t and -T option properly!\n");
	  exit(-1);
	}
    }
  else
    {
      switch (stSrc.eSrcType)
	{	case SPSS: pFluxT->pDisFct = PulseShape; break;
	case LPSS: pFluxT->pDisFct = PulseInt;   break;
	default  : Error("Wrong value for variable 'source type'\n");
	  exit(-1);
	}
      pFluxT->dInt = 1.0 ;
    }
}
/* End: LoadTimeDistribution() */


void  LoadWavelengthTimeDistrib(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxLT)
{
  long   i, j;
  double X1, X2, dDelX=0.0, Y1, Y2, dDelY=0.0;
  char   sBuffer[CHAR_BUF_LARGE]=""; 
  FILE*  pDisFile=NULL;

  /* loading wavelength distribution file, if its range is set properly  */
  if (pTraj->dTimeFrmMax > pTraj->dTimeFrmMin &&
      pTraj->dLambdaMax  > pTraj->dLambdaMin  && pTraj->dLambdaMin >= 0.0)
    {
      /* openíng distribution file */
      pDisFile = fopen(FullParName(pMod->sLTFileName),"rt");
      if (pDisFile==NULL)
	pDisFile = fopen(FullInstallName(pMod->sLTFileName, "FILES/moderators/"),"rt");
      if (pDisFile!=NULL) 
	{
	  /* reading number of lines, allocating memory and reading distribution file */
	  pFluxLT->nLines   = LinesInFile  (pDisFile) - 1;
	  pFluxLT->nColumns = ColumnsInFile(pDisFile);
	  pFluxLT->pTabX = calloc(pFluxLT->nLines,   sizeof(double));
	  pFluxLT->pTabY = calloc(pFluxLT->nColumns, sizeof(double));
	  pFluxLT->pTabF = calloc(pFluxLT->nColumns*pFluxLT->nLines, sizeof(double));

	  ReadLine  (pDisFile, sBuffer, sizeof(sBuffer)-1);
	  StrgScanLF(sBuffer, pFluxLT->pTabY, pFluxLT->nColumns, 0);

	  for (i=0; i < pFluxLT->nLines; i++)
	    {  
	      ReadLine  (pDisFile, sBuffer, sizeof(sBuffer)-1);
	      sscanf    (sBuffer, "%lf", &pFluxLT->pTabX[i]);
	      StrgScanLF(sBuffer, &pFluxLT->pTabF[IndLT(i,0)], pFluxLT->nColumns, 1);
	    }

	  /* definition of wavelength-time distribution function after check 
	     if time and wavelength range of the simulation is covered by data in file */
	  if (   pTraj->dTimeFrmMin >= pFluxLT->pTabX[0]  
		 && pTraj->dTimeFrmMax <= pFluxLT->pTabX[pFluxLT->nLines-1] 
		 && pTraj->dLambdaMin  >= pFluxLT->pTabY[0] 
		 && pTraj->dLambdaMax  <= pFluxLT->pTabY[pFluxLT->nColumns-1])
	    {
	      pFluxLT->pDisFct = UserLmbdTimeDis;
	    } 
	  else 
	    {  fprintf(LogFilePtr,"ERROR: The wavelength or the time range given in %s is smaller than that in the simulation\n", pMod->sLTFileName);
	      exit(-1);
	    }

	  /* integrating function f(lambda, time) */
	  /* x range devided into logarithmicly increasing bins */
	  pFluxLT->dInt=0.0;
	  for (i=0; i < pFluxLT->nLines; i++)
	    {  /* binning of X_i */
	      if (i==0)
		{	if (pFluxLT->pTabX[i]==0.0)
		    {	X1 = 0.0;
		      X2 = 0.5*pFluxLT->pTabX[i+1];
		    }
		  else
		    {	X2 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i+1])) / 2.0);
		      X1 = sq(pFluxLT->pTabX[i]) / X2;
		    }
		}
	      else if (i == pFluxLT->nLines - 1)
		{	X1 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i-1])) / 2.0);
		  X2 = sq(pFluxLT->pTabX[i]) / X1;
		}
	      else
		{	if (pFluxLT->pTabX[i-1]==0.0)
		    X1 = 0.5*pFluxLT->pTabX[i];
		  else
		    X1 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i-1])) / 2.0);
		  X2 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i+1])) / 2.0);
		}
	      dDelX = X2 - X1;

	      for (j=0; j < pFluxLT->nColumns; j++)
		{	
		  /* binning of Y_j */
		  if (j==0)
		    {	if (pFluxLT->pTabY[j]==0.0)
			{	Y1 = 0.0;
			  Y2 = 0.5*pFluxLT->pTabY[j+1];
			}
		      else
			{	Y2 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j+1])) / 2.0);
			  Y1 = sq(pFluxLT->pTabY[j]) / Y2;
			}
		    }
		  else if (j == pFluxLT->nColumns - 1)
		    {	Y1 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j-1])) / 2.0);
		      Y2 = sq(pFluxLT->pTabY[j]) / Y1;
		    }
		  else
		    {	if (pFluxLT->pTabY[j-1]==0.0)
			Y1 = 0.5*pFluxLT->pTabY[j];
		      else
			Y1 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j-1])) / 2.0);
		      Y2 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j+1])) / 2.0);
		    }
		  dDelY = Y2 - Y1;

		  /* integration, factor 1000. because time values are given in ms instead of s  */
		  pFluxLT->dInt += pFluxLT->pTabF[IndLT(i,j)]*dDelX*dDelY / 1000.0;
		}				
	    }
			
	  /* ISIS normalisation: FU/proton -> FU */
	  if (pFluxLT->dInt < 1.0) 
	    {	
	      double fact=1.0;

	      if (memcmp(pMod->sLTFileName, "Isis1", 5)==0) 
		fact = 1.8e-04 / E_C;
	      else if (memcmp(pMod->sLTFileName, "Isis2", 5)==0) 
		fact = 0.6e-04 / E_C;

	      for (i=0; i < pFluxLT->nLines; i++)
		{  for (j=0; j < pFluxLT->nColumns; j++)
		    {	
		      pFluxLT->pTabF[IndLT(i,j)] *= fact;
		    }				
		}
	      pFluxLT->dInt *= fact;
	    }

	  /* storing of logarithmic values ln(f(lambda,time)) to save calculation time during run */
	  for (i=0; i < pFluxLT->nLines; i++)
	    {  for (j=0; j < pFluxLT->nColumns; j++)
		{	
		  if (pFluxLT->pTabF[IndLT(i,j)] <= 0.0)
		    pFluxLT->pTabF[IndLT(i,j)] = -100.0;
		  else
		    pFluxLT->pTabF[IndLT(i,j)] = log(pFluxLT->pTabF[IndLT(i,j)]);
		}				
	    }

	  /* closes distribution file */
	  fclose(pDisFile) ;
	} 
      else 
	{	fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength-time distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", pMod->sLTFileName);
	  exit (-1);
	}
    } 
  else 
    {	fprintf(LogFilePtr,"ERROR: You have to specify the parameters -m and -M as well as -t and -T properly!\n");
      exit(-1);
    }
}


/* load list of trajectories that shall be traced */
/* ---------------------------------------------- */
void LoadTraceFile()
{
  char  sBuffer[CHAR_BUF_LENGTH]; 
  FILE* pTraceFile=NULL;

  /* If there is a trace file go and load the file */
  if(pTraceFileName!=NULL) 
    {
      /* opens distribution file */
      if((pTraceFile=fopen(FullParName(pTraceFileName),"rt"))!=NULL) 
	{
	  long i;

	  /* reads number of lines, allocates memory and then reads distribution file */
	  g_nLinesTr = LinesInFile(pTraceFile);
	  g_pTrace   = calloc(g_nLinesTr, sizeof(TotalID));

	  for(i=0; i<g_nLinesTr; i++)
	    {  
	      ReadLine(pTraceFile, sBuffer, CHAR_BUF_LENGTH-1);
	      sscanf  (sBuffer, "%c%c%u", &g_pTrace[i].IDGrp[0], &g_pTrace[i].IDGrp[1], &g_pTrace[i].IDNo);
	    }

	  /* closes trace file */
	  fclose(pTraceFile) ;
	} 
      else 
	{	fprintf(LogFilePtr, "\nERROR: Can't open %s to read trace file\n", pTraceFileName);
	  exit (-1);
	}
    }
}


/* look if trajectory shall be traced */
/* ---------------------------------- */
char GetTraceState(TotalID stID)
{
  char cRet = 'N'; 

  if (g_nLinesTr > 0)
    {	
      long   i, il=g_nLinesTr-1;  /* lines in Table */
      double nS, nL;              /* numbers got by conversion from IDs */

      nS =  (stID.IDGrp[0]-'A')*1.117e11 
	+ (stID.IDGrp[1]-'A')*4.295e09 + stID.IDNo;
      nL =  (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + 
	+ (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[il].IDNo;
      i = (long) (il * nS / nL + 0.5);

      while ( (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + 
	      + (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[i].IDNo < nS  &&  i < il ) 
	i++;
      while ( (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + 
	      + (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[i].IDNo > nS  &&  i > 0 ) 
	i--;

      // set 'tracing', if IDs are identical
      if (memcmp(stID.IDGrp, g_pTrace[i].IDGrp, 2)==0 && stID.IDNo==g_pTrace[i].IDNo)
	cRet='T'; 
    }

  return cRet;
}


/* read moderator data from file      */
/* ---------------------------------- */
short ReadModData(char* sFileName)
{
  char  sShape   [2]="S",
    sBuffer [CHAR_BUF_LENGTH];
  FILE* pFileR;

  pFileR = fileOpen(FullParName(sFileName),"rt");


  /* Read a line for each moderator */
  while (ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1)==TRUE)
    {	
      sscanf(sBuffer, "%lf %hd %s  %lf %lf %lf  %lf %lf  %hd %lf %lf  %s %s %s  %hd %lf %lf", 
	     &stMod[imod].dModTemp,    &stMod[imod].nColour,     sShape,  
	     &stMod[imod].dCntrX,      &stMod[imod].dCntrY,     &stMod[imod].dCntrZ,  
	     &stMod[imod].dWidth,      &stMod[imod].dHeight, 
	     &stMod[imod].nBackground, &stMod[imod].dTotalFlux, &stMod[imod].dCurrent, 
	     stMod[imod].sLFileName,   stMod[imod].sTFileName,  stMod[imod].sLTFileName, 
	     &stMod[imod].eModType,    &stMod[imod].dTauAscent,  &stMod[imod].dTauDecay);
 
      stTraj[imod].dLambdaMin  = stTraj[0].dLambdaMin;
      stTraj[imod].dLambdaMax  = stTraj[0].dLambdaMax;
      stTraj[imod].dTimeFrmMin = stTraj[0].dTimeFrmMin;
      stTraj[imod].dTimeFrmMax = stTraj[0].dTimeFrmMax;
      stTraj[imod].dMaxDivY    = stTraj[0].dMaxDivY;
      stTraj[imod].dMaxDivZ    = stTraj[0].dMaxDivZ;

      stMod[imod].dTauAscent /= 1.0e06;
      stMod[imod].dTauDecay  /= 1.0e06;

      if (strcmp(stMod[imod].sLFileName, "none")==0 || strcmp(stMod[imod].sLFileName, "0")==0) 
	strcpy(stMod[imod].sLFileName, "");
      if (strcmp(stMod[imod].sTFileName, "none")==0 || strcmp(stMod[imod].sTFileName, "0")==0) 
	strcpy(stMod[imod].sTFileName, "");
      if (strcmp(stMod[imod].sLTFileName,"none")==0 || strcmp(stMod[imod].sLTFileName,"0")==0) 
	strcpy(stMod[imod].sLTFileName,"");

      if (strcmp(sShape, "C")==0)
	{	stMod[imod].dDiameter   = stMod[imod].dWidth;
	  stMod[imod].dWidth  = 0.0;
	  stMod[imod].dHeight = 0.0;
	  stMod[imod].dArea   = M_PI * sq(stMod[imod].dDiameter) / 4.0;
	  stMod[imod].bCircle    = TRUE;
	}
      else
	{	stMod[imod].dDiameter   = 0.0;
	  stMod[imod].dArea   = stMod[imod].dHeight * stMod[imod].dWidth;        
	  stMod[imod].bCircle    = FALSE;
	}
      stMod[imod].dDistModWnd = dDistWnd-stMod[imod].dCntrX;
      stMod[imod].dWndFact    = 0.0;
      imod++;
    }

  fclose(pFileR);

  /* initialize array pointers */
  stFluxL [imod].pTabX=NULL;
  stFluxL [imod].pTabF=NULL;
  stFluxT [imod].pTabX=NULL;
  stFluxT [imod].pTabF=NULL;
  stFluxLT[imod].pTabX=NULL;
  stFluxLT[imod].pTabY=NULL;
  stFluxLT[imod].pTabF=NULL;
	
  return imod;
}

/* Check whether position (X,Y) of actual moderator 'imod' is behind moderator i */
short
PosBehindMod(short i, double Y, double Z)
{
  if (   stMod[i].nBackground < stMod[imod].nBackground
	 && (  ( stMod[i].bCircle &&  sq(Y-stMod[i].dCntrY) + sq(Z-stMod[i].dCntrZ) 
		 <= sq(stMod[i].dDiameter/2.0) ) 
	       ||(!stMod[i].bCircle &&  fabs(Y-stMod[i].dCntrY) <= 0.5*stMod[i].dWidth 
		  &&  fabs(Z-stMod[i].dCntrZ) <= 0.5*stMod[i].dHeight ) )  )
    return TRUE;
  else
    return FALSE;
}



// ISIS SPECIFIC FUNCTIONS


double** 
matrix(const int m,const int n)
/*!
  Determine a double matrix
*/
{
  int i;
  double* pv;
  double** pd;

  if (m<1) return 0;
  if (n<1) return 0;
  pv = (double*) malloc(m*n*sizeof(double));
  pd = (double**) malloc(m*sizeof(double*));
  if (!pd) 
    {
      fprintf(stderr,"No room for matrix!\n");
      exit(1);
    }
  for (i=0;i<m;i++)
    pd[i]=pv + (i*n);
  return pd;
}


double 
polInterp(double* X,double* Y,int Psize,double Aim) 
/*! 
  returns the interpolated polynomial between Epnts
  and the integration 
  \param X :: X coordinates
  \param Y :: Y coordinates
  \param Psize :: number of valid point in array to use
  \param Aim :: Aim point to intepolate result (X coordinate)
  \returns Energy point
*/
{
  double out,errOut;         /* out put variables */
  double C[Psize],D[Psize];
  double testDiff,diff;
  
  double w,den,ho,hp;           /* intermediate variables */
  int i,m,ns;


  ns=0;
  diff=fabs(Aim-X[0]);
  C[0]=Y[0];
  D[0]=Y[0];
  for(i=1;i<Psize;i++)
    {
      testDiff=fabs(Aim-X[i]);
      if (diff>testDiff)
	{
	  ns=i;
	  diff=testDiff;
	}
      C[i]=Y[i];
      D[i]=Y[i];
    }
  
  out=Y[ns];
  ns--;              /* Now can be -1 !!!! */ 
  
  for(m=1;m<Psize;m++)
    {
      for(i=0;i<Psize-m;i++)
	{
	  ho=X[i]-Aim;
	  hp=X[i+m]-Aim;
	  w=C[i+1]-D[i];
	  /*	  den=ho-hp;  -- test !=0.0 */
	  den=w/(ho-hp);
	  D[i]=hp*den;
	  C[i]=ho*den;
	}
      
      errOut= (2*(ns+1)<(Psize-m)) ? C[ns+1] : D[ns--];
      out+=errOut;
    }
  return out;
}

int
binSearch(int Npts,double* AR,double V)
/*! 
  Object is to find the point in 
  array AR, closest to the value V 
  Checked for ordered array returns lower of backeting objects
*/
{
  int klo,khi,k;
  if (Npts<=0) 
    return 0;
  if (V>AR[Npts-1])
    return Npts;

  if(AR[0]>0.0)AR[0]=0.0;

  if (V<AR[0])
    {
      // if(AR[0]>0.0)AR[0]=0.0;
      fprintf(stderr,"here");
      return 0;
    }
  klo=0;
  khi= Npts-1;
  while (khi-klo >1)
    {
      k=(khi+klo) >> 1;    // quick division by 2
      if (AR[k]>V)
	khi=k;
      else
	klo=k;
    } 
  return khi;
}

int
cmdnumberD(char *mc,double* num)
/*! 
  \returns 1 on success 0 on failure
*/
{
  int i,j;
  char* ss;
  char **endptr;
  double nmb;  
  int len;
  
  len=strlen(mc);
  j=0;
  
  for(i=0;i<len && mc[i] && 
	(mc[i]=='\t' || mc[i]==' '  || mc[i]==',');i++);
  if(i==len || !mc[i]) return 0;
  ss=malloc(sizeof(char)*(len+1));
 
  for(;i<len && mc[i]!='\n' && mc[i] 
	&& mc[i]!='\t' && mc[i]!=' ' && mc[i]!=',';i++)
    {
      ss[j]=mc[i];
      j++;
    }
  if (!j)
    { 
      free(ss);
      return 0;         //This should be impossible
    }
  ss[j]=0;
  endptr=malloc(sizeof(char*));
  nmb = strtod(ss,endptr); 
  if (*endptr != ss+j)
    {
      free(endptr);
      free(ss);
      return 0;
    }
  *num = (double) nmb;
  for(j=0;j<i && mc[j];j++)
    mc[j]=' ';
  free(endptr);
  free(ss);
  return 1;
}

int 
notComment(char* Line)
/*!
  \returns 0 on a comment, 1 on a non-comment
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);

  if (!Line[i] || Line[i]=='c' || Line[i]=='C' ||
      Line[i]=='!' || Line[i]=='#')
    return 0;
  return 1;
}

int 
timeStart(char* Line)
/*!
  Search for a word time at the start of
  the line.
  \param Line :: Line to search
  \returns 1 on success 0 on failure
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<4) return 0;
  return (strncmp(Line+i,"time",4)) ? 0 : 1;
}

int 
timeEnd(char* Line)
/*!
  Search for a word time at the start of
  the line.
  \param Line :: Line to search
  \returns 1 on success 0 on failure
*/
{
  int len,i;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<5) return 0;
  return (strncmp(Line+i,"total",5)) ? 0 : 1;
}

int 
energyBin(char* Line,double Einit,double Eend,double* Ea,double* Eb)
/*!
  Search for a word "energy bin:" at the start of
  the line. Then separte off the energy bin values
  \param Line :: Line to search
  \param Ea :: first energy bin [meV]
  \param Eb :: second energy bin [meV]
  \returns 1 on success 0 on failure
*/
{
  int len,i;
  double A,B;

  len=strlen(Line);
  for(i=0;i<len && isspace(Line[i]);i++);
  if (len-i<11) return 0;


  if (strncmp(Line+i,"energy bin:",11))
    return 0;

  i+=11;
  if (!cmdnumberD(Line+i,&A))
    return 0;
  // remove 'to'
  for(;i<len-1 && Line[i]!='o';i++);
  i++;
  if (!cmdnumberD(Line+i,&B))
    return 0;
  A*=1e9;
  B*=1e9;
  *Ea=A;
  *Eb=B;
  if (*Eb>Einit && *Ea<Eend)
    return 1;
  return 0;
}

double
calcFraction(double EI,double EE,double Ea,double Eb)
/*!
  Calculate the fraction of the bin between Ea -> Eb
  that is encompassed by EI->EE
*/
{
  double frac;
  double dRange;

  if (EI>Eb)
    return 0.0;
  if (EE<Ea)
    return 0.0;

  dRange=Eb-Ea;
  frac=(EI>Ea) ? (Eb-EI)/dRange : 1.0;


  frac-=(EE<Eb) ? (Eb-EE)/dRange : 0.0;

  //  if(frac != 1.0)
  //  fprintf(stderr,"frac %g, Ea %g,Eb %g, EI %g, EE %g\n",frac,Ea,Eb,EI,EE);

  return frac;
}

int
LoadIsisDistrib(  FILE* TFile, double Einit, double Eend)
/*!
  Process a general h.o file to create an integrated
  table of results from Einit -> Eend
  \param Einit :: inital Energy
  \parma Eend  :: final energy
*/
{
  char ss[255];          /* BIG space for line */
  double Ea,Eb;
  double T,D,tmp;
  double Efrac;          // Fraction of an Energy Bin
  int Ftime;             // time Flag
  int eIndex;             // energy Index
  int tIndex;             // time Index
  double Tsum;           // Running integration 
  double Efraction;      // Amount to use for an energy/time bin

  extern ISource TS;
  
  int DebugCnt;
  int i;
  /*!
    Status Flag::
    Ftime=1 :: [time ] Reading Time : Data : Err [Exit on Total]
  

    /*
    Double Read File to determine how many bins and 
    memery size
  */


  //convert Einit and Eend to MeV and reorder

  Einit=81.793936/(Einit*Einit);
  Eend=81.793936/(Eend*Eend);

  if (Einit>Eend)
    {
      tmp=Eend;
      Eend=Einit;
      Einit=tmp;
    }
  /////////////////////////


  Ea=0.0;
  Eb=0.0;

  eIndex= -1;
  DebugCnt=0;
  Ftime=0;
  tIndex=0;
  TS.nTime=0;
  TS.nEnergy=0;
  // Read file and get time bins
  while(fgets(ss,255,TFile) && Eend>Ea)
    {
      if (notComment(ss))
	{
	  DebugCnt++;
          if (!Ftime)
	    {
	      if (energyBin(ss,Einit,Eend,&Ea,&Eb))
		{
		  if (eIndex==0)
		    TS.nTime=tIndex;
		  eIndex++;
		}
	      else if (timeStart(ss))
		{
		  Ftime=1;
		  tIndex=0;
		}
	    }
	  else  // In the time section
	    {
	      if (timeEnd(ss))     // Found "total"
		Ftime=0;
	      else
		{
		  // Need to read the line in the case of first run
		  if (TS.nTime==0)
		    {
		      if (cmdnumberD(ss,&T) &&
			  cmdnumberD(ss,&D))
			tIndex++;
		    }
		}
	    }
	}
    }
  // Plus 2 since we have a 0 counter and we have missed the last line.
  TS.nEnergy=eIndex+2;
  if (!TS.nTime && tIndex)
    TS.nTime=tIndex;
  // printf("tIndex %d %d %d %d \n",tIndex,eIndex,TS.nEnergy,TS.nTime);
  
  /* SECOND TIME THROUGH:: */
  rewind(TFile);

  TS.Flux=matrix(TS.nEnergy,TS.nTime);      
  TS.EInt=(double*) malloc(TS.nEnergy*sizeof(double));
  TS.TimeBin=(double*) malloc(TS.nTime*sizeof(double));
  TS.EnergyBin=(double*) malloc(TS.nEnergy*sizeof(double));

  Tsum=0.0;
  Ea=0.0;
  Eb=0.0;
  eIndex=-1;
  DebugCnt=0;
  Ftime=0;
  tIndex=0;
  TS.EInt[0]=0.0;
  // Read file and get time bins
  while(fgets(ss,255,TFile) && Eend>Ea)
    {
      if (notComment(ss))
	{
	  DebugCnt++;
          if (!Ftime)
	    {
	      if (energyBin(ss,Einit,Eend,&Ea,&Eb))
		{
		  eIndex++;
		  TS.EnergyBin[eIndex]=(Einit>Ea) ? Einit : Ea;
		  Efraction=calcFraction(Einit,Eend,Ea,Eb);
		  Ftime++;
		}
	    }
	  else if (Ftime==1)
	    {
	      if (timeStart(ss))
		{
		  Ftime=2;
		  tIndex=0;
		}
	    }

	  else           // In the time section
	    {
	      if (timeEnd(ss))     // Found "total"
		{
		  Ftime=0;
		  TS.EInt[eIndex+1]=Tsum;
		}
	      else
		{
		  // Need to read the line in the case of first run
		  if (cmdnumberD(ss,&T) &&
		      cmdnumberD(ss,&D))
		    {
		      TS.TimeBin[tIndex]=T/1e8;     // convert Time into second (from shakes)
		      Tsum+=D*Efraction;
		      TS.Flux[eIndex][tIndex]=Tsum;
		      tIndex++;
		    }
		}
	    }
	}
    }

  TS.EnergyBin[eIndex+1]=Eend;
  TS.Total=Tsum;

  //  printf("tIndex %d %d %d \n",tIndex,eIndex,TS.nTime);
  //printf("Tsum %g \n",Tsum);
  //fprintf(stderr,"ebin1 ebinN %g %g\n",TS.EnergyBin[0],TS.EnergyBin[TS.nEnergy-1]);
  
  return;
}

void
ISISgetpoint(double* TV,double* EV)
/*! 
  Calculate the Time and Energy 
  by sampling the file.
  Uses TS table to find the point
  \param TV :: 
  \param EV ::
  \param lim1 :: 
  \param lim2 ::  
*/
{
  int i;

  extern ISource TS;
  double R0,R1,R,Rend;
  int Epnt;       ///< Points to the next higher index of the neutron integral
  int Tpnt;
  int iStart,iEnd;
  double TRange,Tspread;
  double Espread,Estart;
  double *EX;

  // So that lowPoly+highPoly==maxPoly
  const int maxPoly=6; 
  const int highPoly=maxPoly/2;
  const int lowPoly=maxPoly-highPoly;

  // static int testVar=0;

  //  R0=rand01();
  R0=MonteCarlo(0,1);
  /* if (testVar==0)
     {
     R0=1.0e-8;
     testVar=1;
     }
  */
  Rend=R=TS.Total*R0;
  // This gives Eint[Epnt-1] > R > Eint[Epnt]
  Epnt=binSearch(TS.nEnergy-1,TS.EInt,R);
  
  //      if (Epnt < 0)
  //   Epnt=1;
  Tpnt=binSearch(TS.nTime-1,TS.Flux[Epnt-1],R);
  //  fprintf(stderr,"TBoundaryX == %12.6e %12.6e \n",TS.TimeBin[Tpnt-1],TS.TimeBin[Tpnt]);
  //  fprintf(stderr,"TFlux == %12.6e %12.6e %12.6e \n\n",TS.Flux[Epnt-1][Tpnt-1],R,TS.Flux[Epnt-1][Tpnt]);
  //  if (Epnt == -1)
  //{
  //    Epnt=0;
  // fprintf(stderr,"\n Rvals == %g %d %d %g\n",R,Epnt,Tpnt,TS.TimeBin[0]);
  //  fprintf(stderr,"EInt == %d %12.6e %12.6e %12.6e %12.6e \n",Epnt,TS.EInt[Epnt-1],R,TS.EInt[Epnt],TS.EInt[Epnt+1]);
  // printf("EBoundary == %12.6e %12.6e \n",TS.EnergyBin[Epnt],TS.EnergyBin[Epnt+1]);

  //  fprintf(stderr,"TFlux == %12.6e %12.6e %12.6e \n\n",TS.Flux[Epnt+1][Tpnt],R,TS.Flux[Epnt+1][Tpnt+1]);
  // }

  if(R < TS.Flux[Epnt-1][Tpnt-1] || R >TS.Flux[Epnt-1][Tpnt] )
    {
      fprintf(stderr,"outside bin limits Tpnt/Epnt problem  %12.6e %12.6e %12.6e \n",TS.Flux[Epnt-1][Tpnt-1],R,TS.Flux[Epnt-1][Tpnt]);
      exit(-1);
    }


  if(Epnt == 0)
    {
      Estart=0.0;
      Espread=TS.EInt[0];
      *EV=TS.EnergyBin[1];
    }
  else
    {
      Estart=TS.EInt[Epnt-1];
      Espread=TS.EInt[Epnt]-TS.EInt[Epnt-1];
      *EV=TS.EnergyBin[Epnt+1];
    }
  
  if (Tpnt==0 || Epnt==0)
    {
      fprintf(stderr,"BIG ERROR WITH Tpnt: %d and Epnt: %d\n",Tpnt,Epnt);
      exit(-1);
    }
  if (Tpnt==TS.nTime)
    {
      fprintf(stderr,"BIG ERROR WITH Tpnt and Epnt\n");
      exit(-1);
      *TV=0.0;
      Tspread=TS.Flux[Epnt-1][0]-TS.EInt[Epnt-1];
      TRange=TS.TimeBin[0];
      R-=TS.EInt[Epnt-1];
    }
  else
    {
      *TV=TS.TimeBin[Tpnt-1];
      TRange=TS.TimeBin[Tpnt]-TS.TimeBin[Tpnt-1];
      Tspread=TS.Flux[Epnt-1][Tpnt]-TS.Flux[Epnt-1][Tpnt-1];
      R-=TS.Flux[Epnt-1][Tpnt-1];
    }
  //  printf("R == %12.6e\n",R);
  R/=Tspread;
  //  printf("R == %12.6e\n",R);
  *TV+=TRange*R;
  
  
  R1=TS.EInt[Epnt-1]+Espread*MonteCarlo(0,1);
  iStart=Epnt>lowPoly ? Epnt-lowPoly : 0;                  // max(Epnt-halfPoly,0)
  iEnd=TS.nEnergy>Epnt+highPoly ? Epnt+highPoly : TS.nEnergy-1;  // min(nEnergy-1,Epnt+highPoly

  *EV=polInterp(TS.EInt+iStart,TS.EnergyBin+iStart,1+iEnd-iStart,R1);
  
  //  fprintf(stderr,"Energy == %d %d %12.6e %12.6e \n",iStart,iEnd,R1,*EV);
  //  fprintf(stderr,"bins == %12.6e %12.6e %12.6e %12.6e \n",TS.EnergyBin[iStart],TS.EnergyBin[iEnd],
  //	  TS.EInt[Epnt],TS.EInt[Epnt-1]);

  if(*TV < TS.TimeBin[Tpnt-1] || *TV > TS.TimeBin[Tpnt])
    {
      fprintf(stderr,"%d Tpnt %d Tval %g Epnt %d \n",TS.nTime,Tpnt,*TV,Epnt);
      fprintf(stderr,"TBoundary == %12.6e,%g , %12.6e \n\n",TS.TimeBin[Tpnt-1],*TV,TS.TimeBin[Tpnt]);
    }


  // need to return a value in wavelength

  *EV=9.044/sqrt(*EV);
  // and in milliseconds...
  *TV *=1000;
  return;
}

int
cmdnumberI(char *mc,int* num,const int len)
/*!
  \param mc == character string to use
  \param num :: Place to put output
  \param len == length of the character string to process 
  returns 1 on success and 0 on failure 
*/
{
  int i,j;
  char* ss;
  char **endptr;
  double nmb;
  
  if (len<1) 
    return 0;
  j=0;
      
  for(i=0;i<len && mc[i] && 
	(mc[i]=='\t' || mc[i]==' '  || mc[i]==',');i++);
  if(i==len || !mc[i]) return 0;
  ss=malloc(sizeof(char)*(len+1));
  /*  char *ss=new char[len+1]; */
  for(;i<len && mc[i]!='\n' && mc[i] 
	&& mc[i]!='\t' && mc[i]!=' ' && mc[i]!=',';i++)
    {
      ss[j]=mc[i];
      j++;
    }
  if (!j)
    { 
      free(ss);
      return 0;         //This should be impossible
    }
  ss[j]=0;
  endptr=malloc(sizeof(char*));
  nmb = strtod(ss,endptr); 
  if (*endptr != ss+j)
    {
      free(endptr);
      free(ss);
      return 0;
    }
  *num = (double) nmb;
  for(j=0;j<i && mc[j];j++)
    mc[j]=' ';
  free(endptr);
  free(ss);
  return 1;
}

FILE* openFile(char* FileName)
{ 
  FILE* efile=0;
  char ss[256];
  char mct[256];
  char mcdir[256];



  /* Is the file located in working dir? */
  efile=fopen(FileName,"r");
  if (!efile) {
    fprintf(LogFilePtr, "\nERROR: Can't open %s to read source file\n", FileName);
    exit (-1);
  }
  return efile;
}

double strArea(double rtmodX,double rtmodY,double dist,double xw,double yh)
{
  /* 
     Returns the mean Str view of the viewport 
     This integrates over each point on the window xw to yh 
     View port is symmetric so use only 1/4 of the view 
     for the calcuation.
     Control Values rtmodY rtmodX xw yh
  */
  
  double A;
  double Vx,Vy;        // view temp points
  double Mx,My;        // moderator x,y
  double D2;           // Distance ^2 
  int i,j,aa,bb;       // loop variables 
 
  D2=dist*dist;
  A=0.0;  

  for(i=0;i<50;i++)              // Mod X
    {
      Mx=i*rtmodX/100.0;
      for(j=0;j<50;j++)         // Mod Y
	{
	  My=j*rtmodY/100.0;
	  // Position on moderator == (Mx,My)
	  for(aa=-50;aa<51;aa++)  //view port
	    for(bb=-50;bb<51;bb++)
	      {
		Vx=aa*xw/101.0;
		Vy=bb*yh/101.0;
		A+=1.0/((Mx-Vx)*(Mx-Vx)+(My-Vy)*(My-Vy)+D2);
	      }
	}
    }
  //change to Mx*My
  A*=(rtmodY*rtmodX)/(10201.0*2500.0);
  // Correct for the area of the viewport. (tables are per cm^2)
  A*=xw*yh*10000;    

  //  fprintf(stderr,"Viewport == %g %g Moderator size == (%g * %g) m^2 \n",xw,yh,rtmodX,rtmodY);
  //  fprintf(stderr,"Dist == %g (metres) \n",dist);
  //  fprintf(stderr,"Viewport Solid angle == %g str\n",A/(xw*yh*10000));
  //      fprintf(stderr,"Solid angle used == %g str\n",A);
  //return A;
  return A/(xw*yh*10000);
}
