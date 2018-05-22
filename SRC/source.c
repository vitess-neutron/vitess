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
/* 1.12  Aug  2005  D. Champion    special code to describe ISIS source                      */
/* 1.13  Jul  2006  K. Lieutenant  virtual window                                            */
/* 1.14  Apr  2007  D. Champion    fix parameter directory bug in isis moderator file reading*/
/* 1.14a Feb  2010  A. Houben      Bug fix in GetTraceState: index out of array dimension    */
/* 1.15  Mar  2011  K. Lieutenant  ESS and SNS moderator character. as a function of power   */
/* 1.16  Jan  2012  K. Lieutenant  visualization                                             */
/* 1.17  Aug  2012  K. Lieutenant  new characteristics for the ESS cold moderator            */
/* 1.18  Sep  2012  K. Lieutenant  CSNS source                                               */
/* 1.19  Mar  2013  K. Lieutenant  correction ISIS source brlliance                          */
/* 1.20  May  2013  K. Lieutenant  data base versions for moderator characteristics          */
/* 1.21  Sep  2013  K. Lieutenant  time focusing                                             */
/* 1.22  Nov  2013  K. Lieutenant  pancake moderator                                         */
/* 1.23  Mar  2015  K. Lieutenant  correction of solid angle for large declination angles    */
/* 1.24  May  2015  Lieutenant/Zendler  new ESS moderator data (Butterfly)                   */
/* 1.25  Dec  2017  Lieutenant     new ESS Butterfly moderator, performance factor           */
/*********************************************************************************************/

#include <ctype.h>
#include <string.h>

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "src_modchar.h"
#include "source_csns.h"
#include "source_ess.h"
#include "message.h"


typedef enum
{	VT_DIVERGENCE = 0,
	VT_REAL_WND   = 1,
	VT_VIRT_WND   = 2,
}
VtDirect;

/* global variables */
TotalID*  g_pTrace=NULL;       /* table of trajectory IDs for tracing             */
long      g_nLinesTr=0;        /* Number of lines in the trace file               */
char*     pTraceFileName=NULL;
short     eTraceMode=0;        /* mode 0: no tracing 
                                  mode 1: write trace files for traj. of interest
                                  mode 2: simulation only with traj. of interest  */
char*     pModFileName=NULL;
char*     pBeamline=NULL;      // name of the beamline 

short     iDataVsn=1,          /* version of the data base for the source characteristics */
          nNumMod=0,           /* number of moderators in moderator system        */
          imod=0,              /* index of moderators in moderator system         */
          ColorByLmbd=FALSE;   // option: set color depending on wavelength

double    NumberOfNeutrons=0,
          dTimeMeas   =  0.0,  /* time of measurement in seconds                  */
          dLmbdWant   =  0.0,  /* desired wavelength                              */

          PolVecX     =  0.0,  /* polarisation            */
          PolVecY     =  0.0, 
          PolVecZ     =  0.0, 
          PolDegree   =  0.0,  /* degree of polarization [%] */
          FracPolDir  =  0.0,  /* fraction of neutrons in polarization direction */

          Declination =  0.0,  /* declination between mod. surface normal and propagation window */
          dDecCos,             /* cosinus and sinus of the declination of the moderator         */
          dDecSin,             /* to the instrument direction                                   */
          WindowDist  =  0.0,  /* distance moderator - (virtual) window                          */
          WindowHeight= 10.0,  // width of the propagation window
          WindowWidth = 10.0,  // height of the propagation window
          TofWndDist  =  0.0,  // distance moderator - position of time window   
          TofMinWnd  =-1.0e10, // min and 
          TofMaxWnd  = 1.0e10; // max TOF allowed in time window 

Plane     Endpoint,            // structure describing (virtual) window position
          TofWnd;              // structure describing time window position
VtDirect  eDirDet=VT_REAL_WND; /* enum 'modus to determine neutron flight direction' */

Source    stSrc;             /* source data               */
Moderator stMod   [NUM_MOD]; /* moderator data            */
TrajParam stTraj  [NUM_MOD]; /* trajectory data           */
FctTable  stFluxT [NUM_MOD], /* data of time distr.       */
          stFluxL [NUM_MOD], /* data of wavelength distr. */
          stFluxLT[NUM_MOD]; /* data of wavelength & time distr. */


/* local functions */
void  OwnCleanup();
void  OwnInit();
void  LoadWavelengthDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL);
void  LoadTimeDistribution      (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxT);  
void  LoadWavelengthTimeDistrib (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL);
void  LoadTraceFile();
char  GetTraceState(TotalID stID);
short ReadModData(char* sFileName);
int PosBehindMod(const int i, const double Y, const double Z);
int binSearch(int, double*, double);
double**matrix(const int,const int);
double calcFraction(double, double, double, double);

void OwnInit(int argc, char *argv[]);
void adjustProgress(int spercent);

// ISIS Functions
#include "source_isis.c"

/*********************************************************************************************/

int main(int argc, char *argv[])
{
   unsigned long i=0;
   char    ig1='A', ig2='A',
           sText[50]="";
   long    TransmittedNeutrons=0,
           BufferIndex;
   double  TimeAtModerator,
           TimeAtWnd,         /* time of arrival a time window  */
           No,
           dSolAngle=0,       /* solid angle of the neutron beam at the moderator              */
           Phi, Theta,        /* angles of div. from x-dir. in x-y- and x-z-plane (MC choice)  */
           dWndY, dWndZ,      /* position where trajectory passes window 
                                 (MC choice for option eDirDet = VT_REAL_WND or VT_VIRT_WND)   */
           Y0,                /* y-position of the starting point of the neutron in the frame of the moderator */
           dFP,               /* flight path between moderator and window                      */
           TimeOF,            /* time of flight from moderator to window                       */
           CenterX, CenterY,  /* averaged values                                               */
           CenterZ, AveTimeOF,/*     at window                                                 */
           SumProb,           /* sum of probabilities (counts) used to calculate average values*/
           dFact   =    1.0,  /* for 'direction by window' */
           IsisNorm=    1.0,
           PolNorm =    0.0;

   VectorType NullPos={0.0,0.0,0.0};
   Neutron    Input, 
              TestNeutron;
     
   // ISIS specific parameter
   double ISISflux=0.0;

   /* Initialize */
   bVisInstalled = TRUE;
   Init             (argc, argv, VT_SOURCE);
 #ifdef _TEST
   print_module_name("Source and Window 1.25t");
 #else
   print_module_name("Source and Window 1.25e");
 #endif
   OwnInit(argc, argv);
   CenterX   = 0.0; 
   CenterY   = 0.0;
   CenterZ   = 0.0; 
   SumProb   = 0.0;
   AveTimeOF = 0.0;
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
   {  fprintf(LogFilePtr, "constant wave source %s <\n\n", stSrc.pSrcName);
   }
   else	
   {  if (stSrc.eSrcType==SPSS)
      {  fprintf(LogFilePtr, "short pulse spallation source %s <\n", stSrc.pSrcName);
      }
      else
      {  fprintf(LogFilePtr, "long pulse spallation source %s <\n", stSrc.pSrcName);
         fprintf(LogFilePtr, "pulse length                 : %7.3f ms \n", 1000.*stSrc.dPulseLength);
      }
      fprintf(LogFilePtr, "pulse frequency              : %7.3f Hz \n",   stSrc.dPulseFreq);
      fprintf(LogFilePtr, "average power                : %7.3f MW \n",   stSrc.dPower/1000000.);
      fprintf(LogFilePtr, "data base version            : %3d      \n\n", iDataVsn);
   }

   /* for all moderators in the system */
   for (imod=0; imod < nNumMod; imod++)
   {
      if (stMod[imod].nColour != NO_COLOR || stMod[imod].nBackground!=0)
         fprintf(LogFilePtr, "colour %d   spatial order %d\n", stMod[imod].nColour, stMod[imod].nBackground);

      /* load wavelength distribution and time distribution of pulse */
      if(strlen(stMod[imod].sLTFileName) > 0)
      { 
        if (stMod[imod].eIsisTS > 0) 
        {
          // set up ISIS specific parameters and values
          FILE* IFptr;
          IFptr = openFile(FullParName(stMod[imod].sLTFileName));
          ISISflux=LoadIsisDistrib(IFptr,stTraj->dLambdaMin,stTraj->dLambdaMax);
          fclose(IFptr);
          fprintf(LogFilePtr,"Isis moderator - target station %d \n", stMod[imod].eIsisTS);

          // normalisation of ISIS data, which are for 60 µA, and division through frequency
          if (stMod[imod].eIsisTS == 1)
            IsisNorm = 160.0/60.0/40.0;  // 60 µA -> 160 µA;   40 Hz
          else
            IsisNorm =  40.0/60.0/10.0;  // 60 µA ->  40 µA;   10 Hz
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
      if ((eDirDet==VT_REAL_WND || eDirDet==VT_VIRT_WND) && stMod[imod].dDistModWnd > 0.0)
      {	 if (stMod[imod].bCircle)
         {	dSolAngle = AveSolidAngleC(stMod[imod].dDiameter, WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
            stMod[imod].dWndFact = AveWeightC(stMod[imod].dCntrY, stMod[imod].dCntrZ, stMod[imod].dDiameter,
                                              WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
         }
         else
         {
            // calculate ISIS specific Solid Angle correction
            if (stMod[imod].eIsisTS > 0)
            {
               dSolAngle=strArea(stMod[imod].dWidth/100.0, stMod[imod].dHeight/100.0,
                                 stMod[imod].dDistModWnd/100.0, WindowWidth/100.0, WindowHeight/100.0);
               stMod[imod].dWndFact=1.0; 
               //fprintf(stderr,"ISIS solid angle %g \n",dSolAngle);
            }
            else
            {
               dSolAngle = AveSolidAngleR(stMod[imod].dWidth, stMod[imod].dHeight, WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
               /* normalisation factor for 'direction by window' */
               stMod[imod].dWndFact = AveWeightR(stMod[imod].dCntrY, stMod[imod].dCntrZ,
                                                 stMod[imod].dWidth, stMod[imod].dHeight, WindowWidth, WindowHeight, stMod[imod].dDistModWnd);
            }
         }
      }
      else
      {
      /*   if (stMod[imod].eIsisTS > 0) 
         {
            fprintf(LogFilePtr, "ERROR: ISIS moderator can only be used with direction defined by propagation window.\n");
            //	    exit(-1);
         } 
         else 
         { */
            dSolAngle = SolidAngle(stTraj[imod].dMaxDivY, stTraj[imod].dMaxDivZ);
         //}
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
            if (stMod[imod].eIsisTS > 0)
            {
               // try and give flux in n/s/cm2
               stMod[imod].dTotalFlux = ISISflux*3.744905847e14*1.1879451;
               stMod[imod].dFUAmpl    = stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq);
            }
            else
            {
               if (stMod[imod].dTotalFlux==0.0)
                  stMod[imod].dTotalFlux = 2*M_PI * stFluxLT[imod].dInt * stSrc.dPulseFreq;
               if (stSrc.dPulseFreq != 0.0)
                  stMod[imod].dFUAmpl = stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq);
            }
         }
         /* case ESS, SNS */
         else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
         {  
            if      (stSrc.nSource==ESS && iDataVsn == 5)
	           stMod[imod].dFUAmpl = EssTotFU2015(stMod[imod].dHeight, stMod[imod].dModTemp, stSrc.dPower, stSrc.dPulseFreq, stSrc.dPulseLength);
	        else if (stSrc.nSource==ESS && iDataVsn == 6)
	           stMod[imod].dFUAmpl = EssTotFU2016(stMod[imod].dHeight, stMod[imod].dModTemp, stSrc.dPower, stSrc.dPulseFreq, stSrc.dPulseLength, stMod[0].dPfmcFact, stMod[1].dPfmcFact);
	        else
               stMod[imod].dFUAmpl = TotalFU(stMod[imod].dModTemp, stSrc.nSource, stMod[imod].eModType, stSrc.dPower, stSrc.dPulsePeriod, stSrc.dPulseLength);

            stMod[imod].dTotalFlux = 2*M_PI * stMod[imod].dFUAmpl * stSrc.dPulseFreq ;
         }
        /* case CSNS */
         else if (stSrc.nSource==CSNS)
         {  stMod[imod].dFUAmpl    = CsnsTotalFU(stMod[imod].dModTemp, stMod[imod].eModType, stSrc.dPower);
            stMod[imod].dTotalFlux = 2*M_PI * stMod[imod].dFUAmpl * stSrc.dPulseFreq ;
         }
         else
         {  if (stMod[imod].dTotalFlux==0.0)
               stMod[imod].dTotalFlux = 2*M_PI * stFluxL[imod].dInt * stFluxT[imod].dInt * stSrc.dPulseFreq;
            if (stSrc.dPulseFreq != 0.0)
               stMod[imod].dFUAmpl = stMod[imod].dTotalFlux / (2*M_PI * stSrc.dPulseFreq) ;
         }

         switch(stMod[imod].eModType)
         {  case MULT_SPEC: fprintf(LogFilePtr, "multi-spectral moderator\n"); break;
            case POISONED : fprintf(LogFilePtr, "decoupled poisoned moderator\n"); break;
            case DECOUPLED: fprintf(LogFilePtr, "decoupled unpoisoned moderator\n"); break;
            case COUPLED  : fprintf(LogFilePtr, "coupled moderator\n"); break;
         }
      }

      /* current not given => current calculated from flux */
      if (stMod[imod].dCurrent==0.0)
      {  stMod[imod].dCurrent = stMod[imod].dTotalFlux * stMod[imod].dArea * dSolAngle / (2*M_PI);
      }
      /* current given => flux calculated from current, if possible */
      else if (stMod[imod].dArea * dSolAngle > 0.0)
      {  stMod[imod].dTotalFlux = stMod[imod].dCurrent * (2*M_PI) / (stMod[imod].dArea * dSolAngle);
      }
      else 
      {  stMod[imod].dTotalFlux = 0.0;
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
      {  fprintf(LogFilePtr, "wavelength-time distr. file  : %s \n",                stMod[imod].sLTFileName);
      }
      else	
      {	if (strlen(stMod[imod].sTFileName) > 0)
         {  fprintf(LogFilePtr, "time distribution file       : %s \n",             stMod[imod].sTFileName);
         }
         else if (stSrc.eSrcType != CWS && stSrc.nSource==ANYSOURCE)
         {  fprintf(LogFilePtr, "time constants of the pulse  : %7.3f us  , %7.3f us \n",stMod[imod].dTauAscent*1.0e6, 
            stMod[imod].dTauDecay*1.0e6);
         }

         if (strlen(stMod[imod].sLFileName) > 0)
         {  fprintf(LogFilePtr, "wavelength distribution file : %s \n",             stMod[imod].sLFileName);
         }
         else
         {  char text[10]="";
            fprintf(LogFilePtr, "moderator temperature        : %7.3f K\n",         stMod[imod].dModTemp);
            sprintf(text, "%5.1f K", stMod[imod].dModTemp);
            if (imod > 0) 
               strcat (sText, " + ");
            strcat(sText, text);
         }
      }
      fprintf(LogFilePtr,   "performance factor           : %7.3f \n",                   stMod[imod].dPfmcFact);
      if (stMod[imod].dTotalFlux > 0)
         fprintf(LogFilePtr,"total neutron flux (in 2*pi) :%14.4e n/(cm^2s) \n",          stMod[imod].dTotalFlux);
      fprintf(LogFilePtr,   "moderator position           :(%7.3f  %7.3f  %7.3f) cm \n", stMod[imod].dCntrX, stMod[imod].dCntrY, stMod[imod].dCntrZ);
      if (stMod[imod].bCircle)
        fprintf(LogFilePtr, "moderator diameter           : %7.3f cm \n",                stMod[imod].dDiameter);
      else
        fprintf(LogFilePtr, "moderator size (W x H)       : %7.3f cm  x %7.3f cm \n",    stMod[imod].dWidth, stMod[imod].dHeight);

      /* if (eDirDet==VT_REAL_WND)
         fprintf(LogFilePtr, "divergence defined by propagation window \n");
      else if (eDirDet==VT_VIRT_WND)
         fprintf(LogFilePtr, "divergence defined by virtual propagation window \n");
      else
         fprintf(LogFilePtr, "angle of opening used        : %7.3f     x %7.3f deg \n", 2*180*stTraj[imod].dMaxDivY/M_PI, 2*180*stTraj[imod].dMaxDivZ/M_PI);*/
      fprintf(LogFilePtr,    "time averaged neutron current:%14.4e n/s in%9.6f str \n", stMod[imod].dCurrent, dSolAngle);
      fprintf(LogFilePtr,    "wavelength band used         : %7.3f Ang - %7.3f Ang \n", stTraj[imod].dLambdaMin, stTraj[imod].dLambdaMax);
      if (stSrc.eSrcType != CWS)
         fprintf(LogFilePtr, "time interval used           : %7.3f ms  - %7.3f ms \n", stTraj[imod].dTimeFrmMin, stTraj[imod].dTimeFrmMax);
      if (stMod[imod].dCurrent*(stTraj[imod].dLambdaMax-stTraj[imod].dLambdaMin)==0.0)
         Warning("The calculated absolute neutron flux for the given parameter set is zero,\n"
                 "probably because one parameter has a zero range (e.g. delta_lambda = 0, mod_area = 0, ...)\n"
                 "the simulation is performed with a flux normalized to a max. value of 1 n/(cm^2s) \n\n");
      fprintf(LogFilePtr, "\n");
   }  // end loop over moderators

   if (!bVisTraj) 
     WriteInstrData(NullPos);
   WriteSimData  (dTimeMeas, dLmbdWant, stSrc.dPulseFreq);

   /* Propagation, Polarisation */
   if (pBeamline!=NULL)
     fprintf(LogFilePtr, "Beamline %s\n", pBeamline);
   if (eDirDet==VT_DIVERGENCE)
   { fprintf(LogFilePtr, "direction by max. divergence : %7.3f     x %7.3f deg\n  propagation distance       : %7.3f m\n",
                         2*180*stTraj[0].dMaxDivY/M_PI, 2*180*stTraj[0].dMaxDivZ/M_PI, WindowDist/100.0);
   }
   else
   { fprintf(LogFilePtr, "%s (W x H)       : %7.3f cm  x %7.3f cm\n  in a distance of           : %7.3f m\n",
                         (eDirDet==VT_VIRT_WND ? "virtual window" : "real window   "), WindowWidth, WindowHeight, WindowDist/100.);
   }
   fprintf(LogFilePtr, "  Declination                : %7.3f deg\n", Declination);

   if (TofMinWnd > -1.0e10 || TofMaxWnd < 1.0e10)
    fprintf(LogFilePtr, "  time window                : %7.3f ms  - %7.3f ms\n", TofMinWnd, TofMaxWnd);

   fprintf(LogFilePtr,  "polarization                 : %7.3f %%  X: %5.3f Y: %5.3f Z: %5.3f\n",
                      PolDegree, PolVecX, PolVecY, PolVecZ);
   if (pTraceFileName!=NULL)
      fprintf(LogFilePtr, "trace file used              : %s\n", pTraceFileName);

   /* redefinition in terms of eigenvectors e.g. 0 % means 50% Up and 50% Down */
   FracPolDir  = 0.5 + 0.5*PolDegree/100.0;

   dDecCos = cos(Declination*M_PI/180.0);
   dDecSin = sin(Declination*M_PI/180.0);

   if (NThreads > 0)
     setDetachedWrite();

   /****************************************************************************************/
   /*   Generate neutron trajectories                                                      */
   /****************************************************************************************/

   DECLARE_ABORT;

   for (No=BufferIndex=TransmittedNeutrons=0; No<NumberOfNeutrons; No++) 
   {
      double prob;
      Moderator *sM;

      CHECK;

      // provide data for progress meter
      if ((i & 0xff) == 0) {
        adjustProgress((int)(100.0 * No / NumberOfNeutrons));
      }

      /* ID of the trajectory */
      if (i==4294967295U) // = 2^32-1 = largest unsigned integer number
      {
         i=0; 
         if (ig2=='Z') 
         {	ig2='A';
            ig1++;
         } 
         else
            ig2++;
      } 
      else
         i++;

      Input.ID.IDGrp[0] = ig1;
      Input.ID.IDGrp[1] = ig2;
      Input.ID.IDNo     = i;
      Input.Debug       = eTraceMode==WRITE_TRC_FILES ? GetTraceState(Input.ID) : 'N';

      /* choose moderator, if there are more than 1 */
      if (nNumMod > 1)
        imod = (short) (i % nNumMod); // i - nNumMod*(i/nNumMod); 
      else
        imod = 0;
      sM = &(stMod[imod]);

      Input.Color = sM->nColour;

      /* MC choice of starting position */
      if (sM->bCircle) 
      {
         double diam, halfdiam;
         diam = sM->dDiameter;
         halfdiam = diam / 2.0;
         do {
            Y0                = sM->dCntrY + halfdiam - diam*Vran();
            Input.Position[2] = sM->dCntrZ + halfdiam - diam*Vran();
         }	/* repeat if starting point is out of circle */
         while (sq(Y0 - sM->dCntrY) + sq(Input.Position[2] - sM->dCntrZ) > sq(halfdiam)) ; 

      } 
      else 
      {
         Y0                = sM->dCntrY + sM->dWidth /2.0 - sM->dWidth  * Vran();
         Input.Position[2] = sM->dCntrZ + sM->dHeight/2.0 - sM->dHeight * Vran();
      }

      /* check if another moderator is in front of the actual one */
      if (nNumMod > 1) 
      {
         int im;
         for (im=0; im<nNumMod; im++)
         if (im!=imod && PosBehindMod(im, Y0, Input.Position[2])) 
         {
           im = -1;
           break;
         }
         if (im < 0) continue; 
      }


      /* Declination */
	    /* for ESS butterfly 2015: do this after prob is calculated*/
	    if(stSrc.nSource!=ESS || iDataVsn != 5)
      { Input.Position[1] = Y0 * dDecCos - sM->dCntrX * dDecSin;
	      Input.Position[0] = Y0 * dDecSin + sM->dCntrX * dDecCos;
      }
      else 
      { Input.Position[0] = 0.0;
        Input.Position[1] = Y0;
      }

      /* MC choice of wavelength and starting time */
      if (sM->eIsisTS > 0)
        ISISgetpoint(&Input.Time, &Input.Wavelength);
      else 
      {
        Input.Wavelength = stTraj[imod].dLambdaMin  + (stTraj[imod].dLambdaMax  - stTraj[imod].dLambdaMin)  * Vran();
        Input.Time       = stTraj[imod].dTimeFrmMin + (stTraj[imod].dTimeFrmMax - stTraj[imod].dTimeFrmMin) * Vran();
      }
      if (Input.Wavelength==0.0) continue;
      	
      /*Calculation of intensity expressed by a count rate for this trajectory referring to SPSS, LPSS or CWS */
      if (stSrc.eSrcType == CWS)
      {   prob = stFluxL[imod].pDisFct(Input.Wavelength, sM->dModTemp)
              / stFluxL[imod].dInt * sM->dNorm;
      }
      else 
      {  
         TimeAtModerator = Input.Time;
         if (stSrc.dPulsePeriod > 0.0) 
         {
            while(TimeAtModerator < 0.0)                {TimeAtModerator += stSrc.dPulsePeriod;}
            while(TimeAtModerator > stSrc.dPulsePeriod) {TimeAtModerator -= stSrc.dPulsePeriod;}
         }
         TimeAtModerator *= 0.001;   /*  time in seconds  */

         if(strlen(sM->sLTFileName) > 0) 
         {
            // case: flux(lambda,t) was given in a file
            if (sM->eIsisTS > 0) 
               prob = IsisNorm * TS.Total * 3.744905847e14 * 1.1879451 * dSolAngle * WindowWidth * WindowHeight * stSrc.dPulseFreq / NumberOfNeutrons;
            else
               prob = stFluxLT[imod].pDisFct(Input.Wavelength, TimeAtModerator) / stFluxLT[imod].dInt * sM->dNorm;
         }
         else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
         {  // case ESS, SNS
            if (stSrc.nSource==ESS && iDataVsn == 5)
            {  prob = EssModFU_Butterfly2015(stMod[imod].dHeight, stSrc.dPower, stSrc.dPulseFreq, Declination, &Input, stMod[0].dPfmcFact, stMod[1].dPfmcFact);
               prob = prob / sM->dFUAmpl * sM->dNorm;
               Input.Color=GetColour_ESSbutterfly2015(Input.Position[1], Declination);
            }
            else if (stSrc.nSource==ESS && iDataVsn == 6)
            {  prob = EssModFU_Butterfly2016(stMod[imod].dModTemp, stSrc.dPower, stSrc.dPulseFreq, Declination, &Input, stMod[0].dPfmcFact, stMod[1].dPfmcFact);
               prob = prob / sM->dFUAmpl * sM->dNorm;
               if (ColorByLmbd) 
                 Input.Color=GetColour_ESSbutterfly2016(Input.Wavelength);
            }
            else 
            {  prob = EssModFU(Input.Wavelength, TimeAtModerator, stSrc.dPulseLength) / sM->dFUAmpl * sM->dNorm;
            }
         }
         else if (stSrc.nSource==CSNS)
         {  // case CSNS
            prob = CsnsModFU(Input.Wavelength, TimeAtModerator, Input.Position[1], Input.Position[2]) / sM->dFUAmpl * sM->dNorm;
         }
         else
         {  prob = stFluxL[imod].pDisFct(Input.Wavelength, sM->dModTemp) / stFluxL[imod].dInt  
                  * stFluxT[imod].pDisFct(TimeAtModerator, sM->dTauDecay, sM->dTauDecay/sM->dTauAscent, stSrc.dPulseLength) 
                  / stFluxT[imod].dInt * sM->dNorm;
         }
      }

      if(prob <= 0.0) continue; 

      /* Declination for ESS butterfly 2015 (others: has been done already)*/
      if(stSrc.nSource==ESS && iDataVsn == 5)
      { Input.Position[1] = Y0 * dDecCos - sM->dCntrX * dDecSin;
	      Input.Position[0] = Y0 * dDecSin + sM->dCntrX * dDecCos;
      }

      /* direction of flight */
      /* defined by starting position on moderator and position on propagation window */
      if (eDirDet!=VT_DIVERGENCE && sM->dDistModWnd > 0.0)  
      {	
         /* choosing point on propagtion window and calculating distance between points */
         dWndY = MonteCarlo(-0.5*WindowWidth,  0.5*WindowWidth);
         dWndZ = MonteCarlo(-0.5*WindowHeight, 0.5*WindowHeight);
         dFP = sqrt(  sq(sM->dDistModWnd - Input.Position[0]) 
                    + sq(dWndY - Input.Position[1])
                    + sq(dWndZ - Input.Position[2]) );
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

         dFact = sq(cos(Theta)*cos(Phi)) / sM->dWndFact;
         prob *= dFact; 
         prob /= sq(dFP/sM->dDistModWnd); //  correction for solid angles in case of different distances from source to (virtual) window 
      } 
      else  
      {
         /* defined by divergence */
         Phi   = stTraj[imod].dMaxDivY*(1.0-2.0*Vran());
         Theta = stTraj[imod].dMaxDivZ*(1.0-2.0*Vran());
         Input.Vector[0] = 1.0 / sqrt(1.0 + sq(tan(Theta)) + sq(tan(Phi)));
         Input.Vector[1] = Input.Vector[0] * tan(Phi);
         Input.Vector[2] = Input.Vector[0] * tan(Theta);
      }

      /* Time fosusing */
      if (TofWndDist > 0.0)
      {  CopyNeutron(&Input, &TestNeutron);
         if (keygrav==ON)
            TimeAtWnd = TestNeutron.Time + NeutronPlaneIntersectionGrav(&TestNeutron,TofWnd);
         else
            TimeAtWnd = TestNeutron.Time + NeutronPlaneIntersection1   (&TestNeutron,TofWnd);
         if (TimeAtWnd < TofMinWnd || TimeAtWnd > TofMaxWnd) continue;
      }
            
      /* Polarization - spin vectors selected for each trajectory 
         from one of the eigenvectors  in the polarisation direction */
      if (Vran() <= FracPolDir) 
      {  // spin eigenvector No 1
         Input.Spin[0]= PolVecX; 
         Input.Spin[1]= PolVecY; 
         Input.Spin[2]= PolVecZ; 
      } 
      else  
      {  // spin eigenvector No 2
         Input.Spin[0]= -PolVecX; 
         Input.Spin[1]= -PolVecY; 
         Input.Spin[2]= -PolVecZ; 
      } 

      // Write interaction point
		  WriteIAP(&Input, VT_CREATED);

      /* propagation between Moderator and window */
      if (keygrav==ON)
         TimeOF = NeutronPlaneIntersectionGrav(&Input,Endpoint);
      else
         TimeOF = NeutronPlaneIntersection1(&Input,Endpoint);

      /* add time of flight (from mod. to window) and calculate average values at window */
      Input.Time += TimeOF;

      CenterX   += prob*Input.Position[0];
      CenterY   += prob*Input.Position[1];
      CenterZ   += prob*Input.Position[2]; 
      AveTimeOF += prob*Input.Time;
      SumProb   += prob;
      Input.Probability = prob;

      // Check passing through slit and write interaction point

      if (eDirDet!=VT_VIRT_WND)
      {  if (eDirDet==VT_REAL_WND && (fabs(Input.Position[1]) > WindowWidth/2.0 || fabs(Input.Position[2]) > WindowHeight/2.0) )
	     {  WriteIAP(&Input, VT_OUT_OF_WND);
	        continue;
	     }
	     else
	     {  WriteIAP(&Input, VT_PASSED);
	     }
	  }
      Input.Position[0]=0.0;

      if (!bTest && (eTraceMode!=ONLY_TRC_TRAJ || GetTraceState(Input.ID)=='T'))
         WriteNeutron(&Input);
   }  // end loop over trajectories

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
  stGeometry.pDescr = "source:yellow";   // or: Z.121: sText="Source";  here: stGeometry.pDescr = sText;
  OwnCleanup();
  Cleanup(-Endpoint.D,0.0,0.0, 0.0,0.0);

  return(0);
}


/* own init of the source module */
/* ----------------------------- */
void OwnInit(int argc, char **argv)
{
  int  i,j;
   char  *arg=NULL;

   /* Initialize */
   stSrc.dPulseLength = 0.002;      /* [s] LPSS pulse length 2 ms            */ 
   stSrc.pSrcName     = "";

   for(i=1; i<argc; i++)
   {
      if(argv[i][0]!='+') 
      {
        arg=&argv[i][2];   //free a   b B c C     e E     g G   H   I j J   K l           o O     q Q             u U v V     x         Z
        switch(argv[i][1]) //used   A         d D     f F     h   i       k     L m M n N     p P     r R s S t T         w W   X y Y z  
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
            j = atol(arg); 
            if (j < 0 || j > 2)
              Error("Wrong parameter for 'direction determination'");
            else
              eDirDet = (VtDirect) j;
            break;

          case 'A':
            dTimeMeas = (double) atof(arg); /* [s] */
            break;
          case 'W':
            dLmbdWant = (double) atof(arg); /* [s] */
            break;


            /* source and moderator */
          case 'S':
            stSrc.eSrcType = (short)atoi(arg); /* 1: CWS; 2: SPSS; 3: LPSS */
            break;
          case 'N':
            stSrc.pSrcName = arg;
            if (strcmp(arg,"ESS")==0)
              stSrc.nSource = ESS;
            else if (strcmp(arg,"SNS")==0)
              stSrc.nSource  = SNS;
            else if (strcmp(arg,"CSNS")==0)
              stSrc.nSource  = CSNS;
            else 
              stSrc.nSource = ANYSOURCE;	     /* no specific source given */
            break;
          case 'v':
            iDataVsn = (short)atoi(arg);       /* version of the data base */
            break;

          case 'R':
            stSrc.dPulseFreq = (double)atof(arg);          /* pulse repetition rate   [Hz] */
            if (stSrc.dPulseFreq > 0.0)
              stSrc.dPulsePeriod = 1000./stSrc.dPulseFreq; /* time between two pulses [ms] */
            else
              stSrc.dPulsePeriod = 0.0;
            break;
          case 'L':
            stSrc.dPower = (double)atof(arg);          /* average source power [MW];  */
            stSrc.dPower*= 1.0e6;                      /* power                [W];   */
            break;
          case 'p':
            stSrc.dPulseLength = (double)atof(arg);    /* LPSS: proton pulselength [ms]  */
            stSrc.dPulseLength*=0.001;                 /* pulse length              [s]  */
            break;

          case 'a':
            pModFileName=arg;  
            break;
          case 'B':
            pBeamline=arg;  
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
          case 'D':									           //  distance moderator propagation window [cm]
            WindowDist = (double) atof(arg);
            if (WindowDist < 0.0)
              Error("Distance from moderator to window must have be greater equal zero");
            break;
          case 'i':
            Declination = (double)atof(arg);   // angle between moderator surface normal and beamline [deg]
            break;
          case 'w':
            WindowWidth = (double)atof(arg);   // width of propagation window [cm]
            break;
          case 'h':
            WindowHeight = (double)atof(arg);  // height of propagation window [cm]
            break;

          /* time focusing */
          case 's':									           //  distance from moderator to position of time window [cm]
            TofWndDist = (double) atof(arg);
            if (TofWndDist < 0.0)
              Error("Distance from moderator to position of time window must have be greater equal zero");
            break;
          case 'f':
            TofMinWnd = (double)atof(arg);     // min. TOF to position to time window [cm]
            break;
          case 'F':
            TofMaxWnd = (double)atof(arg);     // max. TOF to position to time window [cm]
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

   if (WindowDist == 0.0 && eDirDet!=VT_DIVERGENCE)
      Error("Direction can only be defined by window, if distance from moderator to window is greater zero ");
   Endpoint.A = 1.0;
   Endpoint.B = 0.0;
   Endpoint.C = 0.0;
   if (eDirDet == VT_VIRT_WND)
      Endpoint.D = 0.0;
   else
      Endpoint.D = -WindowDist;

    memcpy(&TofWnd, &Endpoint, sizeof(Endpoint));
    TofWnd.D = -TofWndDist;

   // For ESS declination can be calculated from the beamport
   if (pBeamline!=NULL)
   { double theta;
     if (Declination==0.0 && stSrc.nSource==ESS)
     { theta       = CalcTheta(pBeamline);
       Declination = CalcDecl (theta);
     }
   }
}
/* End OwnInit */
 


/* own cleanup of the source module */
/* -------------------------------- */
void OwnCleanup()
{
  short m,     /* index for moderators  */
        kc=0,  /* index for circular moderators */
        ks=0;  /* index for rectangular moderators */

  /* print messages of loops (if existing) */
  // PrintMessage(SRC_L_RANGE_TOO_SMALL, stMod[imod].sLFileName, OFF);
  // PrintMessage(SRC_T_RANGE_TOO_SMALL, stMod[imod].sTFileName, OFF);
  // PrintMessage(SRC_LT_RANGE_TOO_SMALL,stMod[imod].sLTFileName,OFF);


  // Geometry data
  if (bVisInstr)
  { stGeometry.nCircles=0;
    stGeometry.nRectangles=1;

    for (m=0; m < nNumMod; m++)
    { 
      if (stMod[m].bCircle)
        stGeometry.nCircles++;
      else
        stGeometry.nRectangles++;
    }
    if (stGeometry.nCircles > 0)
      stGeometry.pCircle = (VtCircle*) calloc(stGeometry.nCircles, sizeof(VtCircle));
    stGeometry.pRectangle = (VtRectangle*) calloc(stGeometry.nRectangles, sizeof(VtRectangle));

    // Moderators
    for (m=0; m < nNumMod; m++)
    { 
      if (stMod[m].bCircle)
      { stGeometry.pCircle[kc].vCntr[0]   = stMod[m].dCntrX;
        stGeometry.pCircle[kc].vCntr[1]   = stMod[m].dCntrY;
        stGeometry.pCircle[kc].vCntr[2]   = stMod[m].dCntrZ;
        stGeometry.pCircle[kc].vNormal[0] = dDecCos;
        stGeometry.pCircle[kc].vNormal[1] = dDecSin;
        stGeometry.pCircle[kc].vNormal[2] = 0.0;
        stGeometry.pCircle[kc].Radius     = stMod[m].dDiameter/2.0;
        stGeometry.pCircle[kc].AngleBeg   =   0.0;
        stGeometry.pCircle[kc].AngleEnd   = 359.99;
        kc++;
      }
      else
      { stGeometry.pRectangle[ks].vCntr[0]   = stMod[m].dCntrX;
        stGeometry.pRectangle[ks].vCntr[1]   = stMod[m].dCntrY;
        stGeometry.pRectangle[ks].vCntr[2]   = stMod[m].dCntrZ;
        stGeometry.pRectangle[ks].vNormal[0] = dDecCos;
        stGeometry.pRectangle[ks].vNormal[1] = dDecSin;
        stGeometry.pRectangle[ks].vNormal[2] = 0.0;
        stGeometry.pRectangle[ks].Width      = stMod[m].dWidth;
        stGeometry.pRectangle[ks].Height     = stMod[m].dHeight;
        ks++;
      }
    }

    // Propagation window
    stGeometry.pRectangle[ks].vCntr[0]   = WindowDist;
    stGeometry.pRectangle[ks].vCntr[1]   = 0.0;
    stGeometry.pRectangle[ks].vCntr[2]   = 0.0;
    stGeometry.pRectangle[ks].vNormal[0] = 1.0;
    stGeometry.pRectangle[ks].vNormal[1] = 0.0;
    stGeometry.pRectangle[ks].vNormal[2] = 0.0;
    stGeometry.pRectangle[ks].Width      = WindowWidth;
    stGeometry.pRectangle[ks].Height     = WindowHeight;

    stGeometry.eModule = VT_SOURCE;
  }

  /* free allocated memory */
  for (m=0; m < nNumMod; m++)
  {   if (stFluxL[m].pTabX!=NULL)  free(stFluxL[m].pTabX);
      if (stFluxL[m].pTabF!=NULL)  free(stFluxL[m].pTabF);
      if (stFluxT[m].pTabX!=NULL)  free(stFluxT[m].pTabX);
      if (stFluxT[m].pTabF!=NULL)  free(stFluxT[m].pTabF);
      if (stFluxLT[m].pTabX!=NULL) free(stFluxLT[m].pTabX);
      if (stFluxLT[m].pTabY!=NULL) free(stFluxLT[m].pTabY);
      if (stFluxLT[m].pTabF!=NULL) free(stFluxLT[m].pTabF);
  }
  if (g_pTrace!=NULL) free(g_pTrace);
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
        if (pDisFile!=NULL) 
        {
            /* reading number of lines, allocating memory and reading distribution file */
            pFluxL->nLines = LinesInFile(pDisFile);
            pFluxL->pTabX  = (double*) calloc(pFluxL->nLines, sizeof(double));
            pFluxL->pTabF  = (double*) calloc(pFluxL->nLines, sizeof(double));

            for(i=0; i < pFluxL->nLines; i++)
            {  
              ReadLine(pDisFile, sBuffer, sizeof(sBuffer)-1);
              sscanf  (sBuffer, "%lf %le", &pFluxL->pTabX[i], &pFluxL->pTabF[i]);
            }

            /* definition of wavelength dist. function after check 
               if wavelength range of the simulation is covered by data in file */
            if (pTraj->dLambdaMin >= pFluxL->pTabX[0] && pTraj->dLambdaMax <= pFluxL->pTabX[pFluxL->nLines-1])
            {
               pFluxL->pDisFct = (double(*)()) UserLambdaDis;
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
        { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", 
	                         pMod->sLFileName);
          exit (-1);
        }
      } 
      else 
      { fprintf(LogFilePtr,"ERROR: You have to specify the wavelength range properly!\n");
        exit(-1);
      }
   }
   else
   {
    /* otherwise use Maxwellian distribution */
    pFluxL->pDisFct = (double(*)()) Maxwellian;
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
      if (pDisFile!=NULL) 
      {
        /* reading number of lines, allocating memory and reading distribution file */
        pFluxT->nLines = LinesInFile(pDisFile);
        pFluxT->pTabX = (double*) calloc(pFluxT->nLines, sizeof(double));
        pFluxT->pTabF = (double*) calloc(pFluxT->nLines, sizeof(double));

        for (i=0; i < pFluxT->nLines; i++)
        {  
          ReadLine(pDisFile, sBuffer, sizeof(sBuffer)-1);
          sscanf  (sBuffer, "%lf %le", &pFluxT->pTabX[i], &pFluxT->pTabF[i]);
        }

        /* definition of time distribution function after check 
           if time range of the simulation is covered by data in file */
        if (pTraj->dTimeFrmMin >= pFluxT->pTabX[0] &&  pTraj->dTimeFrmMax <= pFluxT->pTabX[pFluxT->nLines-1])
        {
          pFluxT->pDisFct = (double(*)()) UserTimeDis;
        } 
        else 
        { fprintf(LogFilePtr,"ERROR: The time range given in %s is smaller than that in the simulation\n", pMod->sTFileName);
          exit(-1);
        }

        /* integration  function f(time) and saves ln(f) */
        pFluxT->dInt=0.0;
        for (i=0; i < pFluxT->nLines; i++)
        { /* binning of X_i */
          if (i==0)
          { if (pFluxT->pTabX[i]==0.0)
            { X1 = 0.0;
              X2 = 0.5*pFluxT->pTabX[i+1];
            }
            else
            { X2 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i+1])) / 2.0);
              X1 = sq(pFluxT->pTabX[i]) / X2;
            }
          }
          else if (i == pFluxT->nLines - 1)
          { X1 = exp((log(pFluxT->pTabX[i]) + log(pFluxT->pTabX[i-1])) / 2.0);
            X2 = sq(pFluxT->pTabX[i]) / X1;
          }
          else
          { if (pFluxT->pTabX[i-1]==0.0)
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
      { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given time distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", pMod->sTFileName);
        exit (-1);
      }
    }
    else 
    { fprintf(LogFilePtr,"ERROR: You have to specify the -t and -T option properly!\n");
      exit(-1);
    }
  }
  else
  {
    switch (stSrc.eSrcType) 
    {
      case SPSS: pFluxT->pDisFct = (double(*)()) PulseShapeP; break;
      case LPSS: pFluxT->pDisFct = (double(*)()) PulseIntEss; break;
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

  /* loading distribution file, if its range is set properly  */
  if (pTraj->dTimeFrmMax > pTraj->dTimeFrmMin &&
      pTraj->dLambdaMax  > pTraj->dLambdaMin  && pTraj->dLambdaMin >= 0.0)
  {
    /* openíng distribution file */
    pDisFile = fopen(FullParName(pMod->sLTFileName),"rt");
    if (pDisFile!=NULL) 
    {
      /* reading number of lines, allocating memory and reading distribution file */
      pFluxLT->nLines   = LinesInFile  (pDisFile) - 1;
      pFluxLT->nColumns = ColumnsInFile(pDisFile);
      pFluxLT->pTabX = (double*) calloc(pFluxLT->nLines,   sizeof(double));
      pFluxLT->pTabY = (double*) calloc(pFluxLT->nColumns, sizeof(double));
      pFluxLT->pTabF = (double*) calloc(pFluxLT->nColumns*pFluxLT->nLines, sizeof(double));
 
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
        pFluxLT->pDisFct = (double(*)()) UserLmbdTimeDis;
      } 
      else 
      { fprintf(LogFilePtr,"ERROR: The wavelength or the time range given in %s is smaller than that in the simulation\n", pMod->sLTFileName);
        exit(-1);
      }
 
      /* integrating function f(lambda, time) */
      /* x range devided into logarithmicly increasing bins */
      pFluxLT->dInt=0.0;
      for (i=0; i < pFluxLT->nLines; i++)
      { /* binning of X_i */
        if (i==0)
        { if (pFluxLT->pTabX[i]==0.0)
          { X1 = 0.0;
            X2 = 0.5*pFluxLT->pTabX[i+1];
          }
          else
          { X2 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i+1])) / 2.0);
            X1 = sq(pFluxLT->pTabX[i]) / X2;
          }
        }
        else if (i == pFluxLT->nLines - 1)
        { X1 = exp((log(pFluxLT->pTabX[i]) + log(pFluxLT->pTabX[i-1])) / 2.0);
          X2 = sq(pFluxLT->pTabX[i]) / X1;
        }
        else
        { if (pFluxLT->pTabX[i-1]==0.0)
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
          { if (pFluxLT->pTabY[j]==0.0)
            { Y1 = 0.0;
              Y2 = 0.5*pFluxLT->pTabY[j+1];
            }
          else
            { Y2 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j+1])) / 2.0);
              Y1 = sq(pFluxLT->pTabY[j]) / Y2;
            }
          }
          else if (j == pFluxLT->nColumns - 1)
          { Y1 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j-1])) / 2.0);
            Y2 = sq(pFluxLT->pTabY[j]) / Y1;
          }
          else
          { if (pFluxLT->pTabY[j-1]==0.0)
              Y1 = 0.5*pFluxLT->pTabY[j];
            else
              Y1 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j-1])) / 2.0);
          Y2 = exp((log(pFluxLT->pTabY[j]) + log(pFluxLT->pTabY[j+1])) / 2.0);
          }
          dDelY = Y2 - Y1;
  
          /* integration, factor 1000. because time values are given in ms instead of s  */
          pFluxLT->dInt += pFluxLT->pTabF[IndLT(i,j)]*dDelX*dDelY / 1000.0;
        } // end for loop over columns (index j)				
      }   // end for loop over lines (index i)	
  			
      /* ISIS normalisation: FU/proton -> FU */
      if (pFluxLT->dInt < 1.0 && pMod->eIsisTS > 0) 
      {	
        double fact = pMod->eIsisTS == 1 ? 1.8e-04 / E_C : 0.6e-04 / E_C;
  	      
        for (i=0; i < pFluxLT->nLines; i++)
          for (j=0; j < pFluxLT->nColumns; j++)
            pFluxLT->pTabF[IndLT(i,j)] *= fact;
  
        pFluxLT->dInt *= fact;
      }
  
      /* storing of logarithmic values ln(f(lambda,time)) to save calculation time during run */
      for (i=0; i < pFluxLT->nLines; i++)
      { for (j=0; j < pFluxLT->nColumns; j++)
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
    else  // distribution file not existing 
    { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength-time distribution\nPlease copy (from ...FILES/moderators) to parameter directory\n", pMod->sLTFileName);
      exit (-1);
    }
  } 
  else // ranges not properly set 
  { fprintf(LogFilePtr,"ERROR: You have to specify the parameters -m and -M as well as -t and -T properly!\n");
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
        g_pTrace   = (TotalID*) calloc(g_nLinesTr, sizeof(TotalID));

        for(i=0; i<g_nLinesTr; i++)
        {  
          ReadLine(pTraceFile, sBuffer, CHAR_BUF_LENGTH-1);
          sscanf  (sBuffer, "%c%c%lu", &g_pTrace[i].IDGrp[0], &g_pTrace[i].IDGrp[1], &g_pTrace[i].IDNo);
        }

        /* closes trace file */
        fclose(pTraceFile) ;
      } 
      else 
      { fprintf(LogFilePtr, "\nERROR: Can't open %s to read trace file\n", pTraceFileName);
        exit (-1);
      }
   }
}


/* look if trajectory shall be traced */
/* ---------------------------------- */
char GetTraceState(TotalID stID)
{
  char cRet = 'N'; 

  if (g_nLinesTr > 0) {	
    long   i, il=g_nLinesTr-1;  /* lines in Table */
    double nS, nL;              /* numbers got by conversion from IDs */
    double gS, gL;

    gS = (g_pTrace[il].IDGrp[0]-'A')*1.117e11;
    gL = (g_pTrace[il].IDGrp[1]-'A')*4.295e09;
    nS = (stID.IDGrp[0]-'A')*1.117e11 + (stID.IDGrp[1]-'A')*4.295e09 + stID.IDNo;
    //nL = (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[il].IDNo;
    nL = gS                                   + gL                                   + g_pTrace[il].IDNo;
    i = (long) (il * nS / nL + 0.5);
    if (i > il) i = il;

    /*while ( (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[i].IDNo < nS  &&  i < il ) 
      i++;
      while ( (g_pTrace[il].IDGrp[0]-'A')*1.117e11 + (g_pTrace[il].IDGrp[1]-'A')*4.295e09 + g_pTrace[i].IDNo > nS  &&  i > 0 ) 
      i--;
    */

    while ( (i < il) && (gS + gL + g_pTrace[i].IDNo < nS) ) 
      i++;
    while ( (i > 0) && (i <= il) && (gS + gL + g_pTrace[i].IDNo > nS) ) 
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
    stMod[imod].eIsisTS   = 0;    // default: no ISIS moderator 
    stMod[imod].dPfmcFact = 1.0;  //          performance factor = 1
    if (stSrc.nSource==ISIS)
    { sscanf(sBuffer, "%lf %hd %s  %lf %lf %lf  %lf %lf  %hd %lf %lf  %s %s %s  %hd %lf %lf  %hd", 
                      &stMod[imod].dModTemp,    &stMod[imod].nColour,      sShape,  
                      &stMod[imod].dCntrX,      &stMod[imod].dCntrY,      &stMod[imod].dCntrZ,  
                      &stMod[imod].dWidth,      &stMod[imod].dHeight,   
                      &stMod[imod].nBackground, &stMod[imod].dTotalFlux,  &stMod[imod].dCurrent, 
                       stMod[imod].sLFileName,   stMod[imod].sTFileName,   stMod[imod].sLTFileName, 
                      &stMod[imod].eModType,    &stMod[imod].dTauAscent,  &stMod[imod].dTauDecay,
                      &stMod[imod].eIsisTS);
    }
    else
    { sscanf(sBuffer, "%lf %hd %s  %lf %lf %lf  %lf %lf  %hd %lf %lf  %s %s %s  %hd %lf %lf  %lf", 
                      &stMod[imod].dModTemp,    &stMod[imod].nColour,      sShape,  
                      &stMod[imod].dCntrX,      &stMod[imod].dCntrY,      &stMod[imod].dCntrZ,  
                      &stMod[imod].dWidth,      &stMod[imod].dHeight,   
                      &stMod[imod].nBackground, &stMod[imod].dTotalFlux,  &stMod[imod].dCurrent, 
                       stMod[imod].sLFileName,   stMod[imod].sTFileName,   stMod[imod].sLTFileName, 
                      &stMod[imod].eModType,    &stMod[imod].dTauAscent,  &stMod[imod].dTauDecay,
                      &stMod[imod].dPfmcFact);
    }
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
    { stMod[imod].dDiameter   = stMod[imod].dWidth;
      stMod[imod].dWidth  = 0.0;
      stMod[imod].dHeight = 0.0;
      stMod[imod].dArea   = M_PI * sq(stMod[imod].dDiameter) / 4.0;
      stMod[imod].bCircle = TRUE;
    }
    else
    { stMod[imod].dDiameter = 0.0;
      stMod[imod].dArea     = stMod[imod].dHeight * stMod[imod].dWidth;        
      stMod[imod].bCircle   = FALSE;
    }
    stMod[imod].dDistModWnd = WindowDist-stMod[imod].dCntrX;
    stMod[imod].dWndFact    = 0.0;
    imod++;
  }

  fclose(pFileR);


  /* set only moderator option for ESS butterfly: */
  if (stSrc.nSource==ESS && iDataVsn >= 5)
  {
    if (iDataVsn==5)
    { 
      // set butterfly geometry:
      stMod[0].dModTemp = 325.0;
      stMod[0].dCntrX   =   0.0; 
      stMod[0].dCntrZ   =   0.0; 
      stMod[0].nColour   = 1;
      stMod[0].dDiameter = 0.0;
      stMod[0].bCircle   = FALSE;
      stMod[0].dDistModWnd = WindowDist-stMod[0].dCntrX;

      stMod[1].dModTemp = 50.0;
      stMod[1].dCntrX  = 0;
      stMod[1].dCntrZ  = 0;
      stMod[1].nColour = 2;
      stMod[1].dHeight   = stMod[0].dHeight;
      stMod[1].dDiameter = 0.0;
      stMod[1].bCircle   = FALSE;
      stMod[1].dDistModWnd = WindowDist-stMod[1].dCntrX;
      stMod[1].dWndFact    = 0.0;
      stMod[1].eIsisTS = 0;

      stMod[0].dPfmcFact = 0.75;
      stMod[1].dPfmcFact = 0.75;
      stMod[0].dWidth = GetModWidth_ESSbutterfly2015(fabs(Declination),stMod[0].dModTemp);
      stMod[1].dWidth = GetModWidth_ESSbutterfly2015(fabs(Declination),stMod[1].dModTemp);
      stMod[0].dCntrY =  0.0;
      stMod[1].dCntrY = (Declination>0) ? (stMod[0].dWidth + stMod[1].dWidth) / 2.0       : -(stMod[0].dWidth + stMod[1].dWidth) / 2.0 ;
      stMod[0].dArea  = stMod[0].dHeight * stMod[0].dWidth;        
      stMod[1].dArea  = stMod[1].dHeight * stMod[1].dWidth;        
      imod=2;

      fprintf(LogFilePtr,"For ESS moderator description, both thermal and cold moderator are simulated and set to default values: \n"); 
      fprintf(LogFilePtr,"  thermal: at (0.0,%6.2f,0.0), %5.2f cm wide; colour: 1 \n", stMod[0].dCntrY, stMod[0].dWidth);
      fprintf(LogFilePtr,"  cold   : at (0.0,%6.2f,0.0), %5.2f cm wide; colour: 2 \n", stMod[1].dCntrY, stMod[1].dWidth);
      fprintf(LogFilePtr,"Only height is taken from moderator file (must be either 3 cm or 6 cm) \n");
      fprintf(LogFilePtr,"Choose beamport position via declination angle \n");
      fprintf(LogFilePtr,"Default orientation to the thermal centre (adjust with frame module) \n");
      fprintf(LogFilePtr,"! Note: default factors of %6.3f and %6.3f are applied to the brilliance to reflect the loss from engineering design details not included in the model ! \n", stMod[0].dPfmcFact, stMod[1].dPfmcFact);
    }
    else
    { double Shift  = GetShift_ESSbutterfly2016   (fabs(Declination)),
             Width1 = GetModWidth_ESSbutterfly2016(fabs(Declination), stMod[0].dModTemp),
             Width2 = GetModWidth_ESSbutterfly2016(fabs(Declination), stMod[1].dModTemp);
      stMod[0].dCntrY = (Declination > 0.0) ? -Width1/2.0 - Shift :  Width1/2.0 + Shift ;  // thermal
      stMod[1].dCntrY = (Declination > 0.0) ?  Width2/2.0 - Shift : -Width2/2.0 + Shift ;  // cold
      stMod[0].dWidth = Width1;
      stMod[1].dWidth = Width2;
      stMod[0].dArea  = stMod[0].dHeight * stMod[0].dWidth * cos(Declination*M_PI/180.0);        
      stMod[1].dArea  = stMod[1].dHeight * stMod[1].dWidth * cos(Declination*M_PI/180.0);        
      stMod[0].dDistModWnd = WindowDist-stMod[0].dCntrX;
      stMod[1].dDistModWnd = WindowDist-stMod[1].dCntrX;
      fprintf(LogFilePtr,"Widths and horizontal positions are calculated.\n");
    #ifndef _TEST
      if (pBeamline!=NULL)
        LoadHorDistrib(pBeamline);
      else 
        LoadHorDistrib(GenerBeamport(Declination));
    #endif
    }
    stTraj[1].dLambdaMin  = stTraj[0].dLambdaMin;
    stTraj[1].dLambdaMax  = stTraj[0].dLambdaMax;
    stTraj[1].dTimeFrmMin = stTraj[0].dTimeFrmMin;
    stTraj[1].dTimeFrmMax = stTraj[0].dTimeFrmMax;
    stTraj[1].dMaxDivY    = stTraj[0].dMaxDivY;
    stTraj[1].dMaxDivZ    = stTraj[0].dMaxDivZ;

    // add second cold for the theta=0 view:
    if (iDataVsn==5 && fabs(Declination) < 0.01)
    {
      imod=3;
      stMod[2].dModTemp = 50.0;
      stMod[2].dWidth =  stMod[1].dWidth;
      stMod[2].dCntrY = -stMod[1].dCntrY;
      stMod[2].dCntrX = 0;
      stMod[2].dCntrZ = 0;
      stMod[2].nColour = 2;
      stMod[2].dHeight = stMod[0].dHeight;
      stMod[2].dArea   = stMod[2].dHeight * stMod[2].dWidth; 
        if (iDataVsn==6) stMod[2].dArea *= cos(Declination*M_PI/180.0);
      stMod[2].bCircle   = FALSE;
      stMod[2].dDiameter = 0.0;
      stMod[2].dDistModWnd = WindowDist-stMod[2].dCntrX;
      stMod[2].dWndFact    = 0.0;
      stMod[2].dPfmcFact   = stMod[2].dPfmcFact;
      stMod[2].eIsisTS = 0;
      stTraj[2].dLambdaMin  = stTraj[0].dLambdaMin;
      stTraj[2].dLambdaMax  = stTraj[0].dLambdaMax;
      stTraj[2].dTimeFrmMin = stTraj[0].dTimeFrmMin;
      stTraj[2].dTimeFrmMax = stTraj[0].dTimeFrmMax;
      stTraj[2].dMaxDivY    = stTraj[0].dMaxDivY;
      stTraj[2].dMaxDivZ    = stTraj[0].dMaxDivZ;
      fprintf(LogFilePtr,"3rd moderator was added because of 90° position.\n");
    }
  }

  /* initialize array pointers */
  stFluxL [imod].pTabX=NULL;
  stFluxL [imod].pTabF=NULL;
  stFluxT [imod].pTabX=NULL;
  stFluxT [imod].pTabF=NULL;
  stFluxLT[imod].pTabX=NULL;
  stFluxLT[imod].pTabY=NULL;
  stFluxLT[imod].pTabF=NULL;
	
  // set automatic color option
  if (  imod==1 && stMod[0].nColour==0
     || imod==2 && stMod[0].nColour==0 && stMod[1].nColour==0
     || imod==3 && stMod[0].nColour==0 && stMod[1].nColour==0 && stMod[2].nColour==0) 
    ColorByLmbd=TRUE;

  return imod;
}

/* Check whether position (X,Y) of actual moderator 'imod' is behind moderator i */
int
PosBehindMod(const int i, const double Y, const double Z)
{
  return stMod[i].nBackground < stMod[imod].nBackground &&
    (  ( stMod[i].bCircle &&  sq(Y-stMod[i].dCntrY) + sq(Z-stMod[i].dCntrZ) <= sq(stMod[i].dDiameter/2.0) ) ||
       (!stMod[i].bCircle &&  fabs(Y-stMod[i].dCntrY) <= 0.5*stMod[i].dWidth 
	  &&  fabs(Z-stMod[i].dCntrZ) <= 0.5*stMod[i].dHeight ) );
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
