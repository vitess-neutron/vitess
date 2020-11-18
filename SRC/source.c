/*********************************************************************************************/
/*  VITESS module source                                                                     */
/*                                                                                           */
/* This module generates trajectories using the flux distribution of a neutron source        */
/* (it normalises intensity according to number of traj., wavelength and angular range, ...) */
/*                                                                                           */
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
/* 1.25  Dec  2017  K. Lieutenant  new ESS Butterfly moderator, performance factor           */
/* 1.26  Jan  2020  K. Lieutenant  tidy up, correction reading ISIS moderator data from file */
/* 1.27  Sep  2020  K. Lieutenant  undermoderated neutrons for all moderators, par. renamed  */
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


/******************************/
/** Structures               **/
/******************************/
typedef enum
{	VT_DIVERGENCE = 0,
	VT_REAL_WND   = 1,
	VT_VIRT_WND   = 2,
}
VtDirect;


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char **argv);                                                  // Reads input parameters and sets global variables
void  OwnCleanup();                                                                    // Does module specific cleanup
short ReadModData(char* sFileName);                                                    // Reads moderator parameters from file
void  SetGeometry(char* sColor);                                                       // Fills the structure stGeometry for visualization 
void  LoadWavelengthDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL); // loads wavelength distribution from file or sets wavelength distribution function
void  LoadTimeDistribution      (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxT); // loads time distribution for the pulse from file or sets time distribution function 
void  LoadWavelengthTimeDistrib (Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL); // loads 2D wavelength-time distribution from file
void  LoadTraceFile();                                                                 // loads list of trajectories that shall be traced
char  GetTraceState(TotalID stID);                                                     // looks if trajectory shall be traced
int   PosBehindMod(const int i, const double Y, const double Z);                       // Checks if position of actual moderator is behind another moderator

// ISIS specific funciton 
int      binSearch(int, double*, double);
double** matrix(const int,const int);
double   calcFraction(double, double, double, double);


/******************************/
/** Global Variables         **/
/******************************/
McCompID _eModule=MCN_SOURCE;

// Input parameters
// -----------------
// simulation parameters
double    NumberOfNeutrons=0;   // -n    1.0e6  [-]   number of neutron trajectories (events) 
TrajParam stTraj  [NUM_MOD];    // -m -M       [Ang]  min. and max. of the wavelength range 
                                // -t -T       [ms]   min. and max. of the time frame to start neutrons
                                // -b -c       [deg]  min. horizontal and vertical divergence (absolute value) 
                                // -y -z       [deg]  max. horizontal and vertical divergence (half of angular spread, if min. value is zero) 
VtDirect  eDirDet=VT_VIRT_WND;  // -d   WINDOW  [-]   enum: mode to determine neutron directions: VT_DIVERGENCE  VT_VIRT_WND  VT_REAL_WND 
double    WindowDist  =  0.0,   // -D     0.0  [cm]   distance moderator - target window 
          WindowWidth = 10.0,   // -w    10.0  [cm]   width of the propagation window 
          WindowHeight= 10.0;   // -h    10.0  [cm]   height of the propagation window 
double    TofWndDist  =  0.0,   // -s     0.0  [cm]   distance moderator - position of time window (e.g. chopper)   
          TofMinWnd   =-1.0e10, // -f    -1e10 [cm]   min and 
          TofMaxWnd   = 1.0e10; // -F     1e10 [cm]   max TOF allowed in time window 

double    dTimeMeas   =  0.0,   // -A     0.0   [s]   time of measurement in second
          dLmbdWant   =  0.0;   // -W     0.0  [Ang]  desired wavelength           

// polarisation and trace parameters
double    PolVecX     =  0.0,   // -X     0.0  [cm]   x-component of the polarisation 
          PolVecY     =  0.0,   // -Y     0.0  [cm]   y-component of the polarisation
          PolVecZ     =  0.0,   // -V     0.0  [cm]   z-component of the polarisation
          PolDegree   =  0.0;   // -P     0.0  [cm]   degree of polarization [%] 

char*     sTraceFileName=NULL;  // -r           [-]   name of the file containing the IDs for tracing
short     eTraceMode=0;         /* -k           [-]   NO_TRACING     : no tracing 
                                                      WRITE_TRC_FILES: write trace files for traj. of interest
                                                      ONLY_TRC_TRAJ  : simulation only with traj. of interest  */
// source, moderator and beamline parameters
Source    stSrc;                // -N -S        [-]   name and type of the source 
                                // -R -L      [Hz/MW] pulse frequency and average source power
                                // -p          [ms]   proton pulse length  
short     iDataVsn=1;           // -v     1     [-]   version of the data base

char*     sModFileName=NULL;    // -a           [-]   name of the file containing moderator parameter
char*     pBeamline=NULL;       // -B           [-]   name of the beamline 
double    Declination =  0.0;   // -i     0.0  [deg]  declination between moderator surface normal and propagation window 

// Parameters read from file
// -------------------------
Moderator stMod   [NUM_MOD];    // file               moderator characteristics read from file

// Variables determined from input parameters or trajectory data
// -------------------------------------------------------------
double    dDecCos,              //                    cosinus and sinus of the declination of the moderator
          dDecSin;              //                    to the instrument direction 
double    FracPolDir  =  0.0;   //                    fraction of neutrons in polarization direction 
                                                      
Plane     Endpoint,             //                    structure describing (virtual) window position
          TofWnd;               //                    structure describing time window position
                                                      
FctTable  stFluxT [NUM_MOD],    //                    data of time distr.      
          stFluxL [NUM_MOD],    //                    data of wavelength distr.
          stFluxLT[NUM_MOD];    //                    data of wavelength & time distr.
                                                      
TotalID*  aTrace=NULL;          //                    array of trajectory IDs for tracing 
long      nLinesTr=0;           //                    number of lines in the trace file   
                                                      
short     nNumMod=0,            //                    number of moderators in moderator system
          imod=0,               //                    index of current moderators in moderator system 
          ColorByLmbd=FALSE;    //                    option: set color depending on wavelength  (only for ESS Butterfly-1)


/*******************************************************/
/** ISIS code (structure, prototypes, functions       **/
/*******************************************************/
#include "source_isis.c"


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
   unsigned long i=0;
   char    ig1='A', ig2='A',
           sText[50]="";
   long    TransmittedNeutrons=0,
           BufferIndex;
   double  test,
           TimeAtModerator,
           TimeAtWnd,         /* time of arrival a time window  */
           No,
           SolAngle=0.0,      /* solid angle of the neutron beam at the moderator              */
           Phi, Theta,        /* angles of div. from x-dir. in x-y- and x-z-plane (MC choice)  */
           WndY, WndZ,        /* position where trajectory passes window 
                                 (MC choice for option eDirDet = VT_REAL_WND or VT_VIRT_WND)   */
           Y0,                /* y-position of the starting point of the neutron in the frame of the moderator */
           Path,              /* flight path between moderator and window                      */
           TimeOF,            /* time of flight from moderator to window                       */
           CenterX, CenterY,  /* averaged values                                               */
           CenterZ, AveTimeOF,/*     at window                                                 */
           SumProb,           /* sum of probabilities (counts) used to calculate average values*/
           FactWnd =    1.0,  /* for 'direction by window' */
           IsisNorm=    1.0,
           PolNorm =    0.0;

   Moderator* pMod;
   VectorType NullPos={0.0,0.0,0.0};
   Neutron    Input, 
              TestNeutron;
     
   // ISIS specific parameter
   double ISISflux=0.0;

   // --------------
   // Initialisation
   // --------------
   Init(argc,argv, _eModule);
   PrintModuleName(_eModule, "1.26");
   OwnInit(argc, argv);

   bVisInstalled = TRUE;
   if (bVisInstr) 
     bLengthCmpr = TRUE;

   CenterX   = 0.0; 
   CenterY   = 0.0;
   CenterZ   = 0.0; 
   SumProb   = 0.0;
   AveTimeOF = 0.0;
   nNumMod   = ReadModData(sModFileName);

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
   {  
      fprintf(LogFilePtr, "constant wave source %s <\n\n", stSrc.pSrcName);
   }
   else	
   {  
      if (stSrc.eSrcType==SPSS)
      {  
         fprintf(LogFilePtr, "short pulse spallation source %s <\n", stSrc.pSrcName);
      }
      else
      {  
         fprintf(LogFilePtr, "long pulse spallation source %s <\n", stSrc.pSrcName);
         fprintf(LogFilePtr, "pulse length                 : %7.3f ms \n", 1000.*stSrc.PulseLength);
      }
      fprintf(LogFilePtr, "pulse frequency              : %7.3f Hz \n",   stSrc.PulseFreq);
      fprintf(LogFilePtr, "average power                : %7.3f MW \n",   stSrc.Power/1000000.);
      fprintf(LogFilePtr, "data base version            : %3d      \n\n", iDataVsn);
   }

   /* for all moderators in the system */
   for (imod=0; imod < nNumMod; imod++)
   {
      pMod = &(stMod[imod]);

      if (pMod->nColour != NO_COLOR || pMod->nBackground!=0)
         fprintf(LogFilePtr, "colour %d   spatial order %d\n", pMod->nColour, pMod->nBackground);

      /* load wavelength distribution and time distribution of pulse */
      if(strlen(pMod->sLTFileName) > 0)
      { 
        if (pMod->eIsisTS > 0) 
        {
          // set up ISIS specific parameters and values
          FILE* IFptr;
          IFptr = openFile(pMod->sLTFileName);
          ISISflux=LoadIsisDistrib(IFptr,stTraj->LambdaMin,stTraj->LambdaMax);
          fclose(IFptr);
          fprintf(LogFilePtr,"Isis moderator - target station %d \n", pMod->eIsisTS);

          // normalisation of ISIS data, which are for 60 µA, and division through frequency
          if (pMod->eIsisTS == 1)
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
      if ((eDirDet==VT_REAL_WND || eDirDet==VT_VIRT_WND) && pMod->DistModWnd > 0.0)
      {	 if (pMod->bCircle)
         {	SolAngle = AveSolidAngleC(pMod->Diameter, WindowWidth, WindowHeight, pMod->DistModWnd);
            pMod->WndFact = AveWeightC(pMod->CntrY, pMod->CntrZ, pMod->Diameter,
                                              WindowWidth, WindowHeight, pMod->DistModWnd);
         }
         else
         {
            // calculate ISIS specific Solid Angle correction
            if (pMod->eIsisTS > 0)
            {
               SolAngle=strArea(pMod->Width/100.0, pMod->Height/100.0,
                                pMod->DistModWnd/100.0, WindowWidth/100.0, WindowHeight/100.0);
               pMod->WndFact=1.0; 
               //fprintf(stderr,"ISIS solid angle %g \n",SolAngle);
            }
            else
            {
               SolAngle = AveSolidAngleR(pMod->Width, pMod->Height, WindowWidth, WindowHeight, pMod->DistModWnd);
               /* normalisation factor for 'direction by window' */
               pMod->WndFact = AveWeightR(pMod->CntrY, pMod->CntrZ,
                                                 pMod->Width, pMod->Height, WindowWidth, WindowHeight, pMod->DistModWnd);
            }
         }
      }
      else
      {
      /*   if (pMod->eIsisTS > 0) 
         {
            fprintf(LogFilePtr, "ERROR: ISIS moderator can only be used with direction defined by propagation window.\n");
            //	    exit(-1);
         } 
         else 
         { */
            SolAngle =  SolidAngle(stTraj[imod].MaxDivY, stTraj[imod].MaxDivZ) -
                      - SolidAngle(stTraj[imod].MinDivY, stTraj[imod].MinDivZ);
         //}
      }

      /* calculate flux and mean current of the neutron beam */
      if (stSrc.eSrcType == CWS)
      {  
         if (pMod->TotalFlux==0.0)
           pMod->TotalFlux = 2*M_PI * stFluxL[imod].Int;
      }
      else
      {	
         /* case: flux(lambda,t) was given in a file */
         if (strlen(pMod->sLTFileName) > 0)
         {
            if (pMod->eIsisTS > 0)
            {
               // try and give flux in n/s/cm2
               pMod->TotalFlux = ISISflux*3.744905847e14*1.1879451;
               pMod->FUAmpl    = pMod->TotalFlux / (2*M_PI * stSrc.PulseFreq);
            }
            else
            {
               if (pMod->TotalFlux==0.0)
                  pMod->TotalFlux = 2*M_PI * stFluxLT[imod].Int * stSrc.PulseFreq;
               if (stSrc.PulseFreq != 0.0)
                  pMod->FUAmpl = pMod->TotalFlux / (2*M_PI * stSrc.PulseFreq);
            }
         }
         /* case ESS, SNS */
         else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
         {  
            if      (stSrc.nSource==ESS && iDataVsn == 5)
	           pMod->FUAmpl = EssTotFU2015(pMod->Height, pMod->ModTemp, stSrc.Power, stSrc.PulseFreq, stSrc.PulseLength);
	        else if (stSrc.nSource==ESS && iDataVsn == 6)
	           pMod->FUAmpl = EssTotFU2016(pMod->Height, pMod->ModTemp, stSrc.Power, stSrc.PulseFreq, stSrc.PulseLength, stMod[0].PfmcFact, stMod[1].PfmcFact);
	        else
               pMod->FUAmpl = TotalFU(pMod->ModTemp, stSrc.nSource, pMod->eModType, stSrc.Power, stSrc.PulsePeriod, stSrc.PulseLength);

            pMod->TotalFlux = 2*M_PI * pMod->FUAmpl * stSrc.PulseFreq ;
         }
        /* case CSNS */
         else if (stSrc.nSource==CSNS)
         {  pMod->FUAmpl    = CsnsTotalFU(pMod->ModTemp, pMod->eModType, stSrc.Power);
            pMod->TotalFlux = 2*M_PI * pMod->FUAmpl * stSrc.PulseFreq ;
         }
         else
         {  if (pMod->TotalFlux==0.0)
               pMod->TotalFlux = 2*M_PI * stFluxL[imod].Int * stFluxT[imod].Int * stSrc.PulseFreq;
            if (stSrc.PulseFreq != 0.0)
               pMod->FUAmpl = pMod->TotalFlux / (2*M_PI * stSrc.PulseFreq) ;
         }

         switch(pMod->eModType)
         {  case MULT_SPEC: fprintf(LogFilePtr, "multi-spectral moderator\n"); break;
            case POISONED : fprintf(LogFilePtr, "decoupled poisoned moderator\n"); break;
            case DECOUPLED: fprintf(LogFilePtr, "decoupled unpoisoned moderator\n"); break;
            case COUPLED  : fprintf(LogFilePtr, "coupled moderator\n"); break;
         }
      }

      /* current not given => current calculated from flux */
      if (pMod->Current==0.0)
      {  pMod->Current = (pMod->TotalFlux + pMod->TotFluxUM) * pMod->Area * SolAngle / (2*M_PI);
      }
      /* current given => flux calculated from current, if possible */
      else if ((pMod->Area * SolAngle) > 0.0)
      {  if (pMod->TotFluxUM > 0.0)
         { pMod->TotalFlux = pMod->Current * (2*M_PI) / (pMod->Area * SolAngle) * pMod->TotalFlux /(pMod->TotalFlux + pMod->TotFluxUM);
           pMod->TotFluxUM = pMod->Current * (2*M_PI) / (pMod->Area * SolAngle) * pMod->TotFluxUM /(pMod->TotalFlux + pMod->TotFluxUM);
         }
         else
         { pMod->TotalFlux = pMod->Current * (2*M_PI) / (pMod->Area * SolAngle);
         }
      }
      else 
      {  pMod->TotalFlux = 0.0;
      }

      /* factor for normalisation (times in seconds) */
      if (stSrc.eSrcType == CWS)
      { pMod->NormTrj = pMod->Area * SolAngle * (stTraj[imod].LambdaMax - stTraj[imod].LambdaMin) / (NumberOfNeutrons/nNumMod);
        pMod->NormInt = pMod->Current *         (stTraj[imod].LambdaMax - stTraj[imod].LambdaMin) / (NumberOfNeutrons/nNumMod);
      }
      else
      { pMod->NormTrj = pMod->Area * SolAngle * stSrc.PulseFreq * (stTraj[imod].LambdaMax - stTraj[imod].LambdaMin) * (stTraj[imod].TimeFrmMax - stTraj[imod].TimeFrmMin)/1000.0 / (NumberOfNeutrons/nNumMod);
        pMod->NormInt = pMod->Current *                           (stTraj[imod].LambdaMax - stTraj[imod].LambdaMin) * (stTraj[imod].TimeFrmMax - stTraj[imod].TimeFrmMin)/1000.0 / (NumberOfNeutrons/nNumMod);
      }
      if (pMod->NormTrj==0.0)
        pMod->NormTrj = 1.0 / (pMod->FUAmpl * NumberOfNeutrons/nNumMod);
      test = pMod->FUAmpl * pMod->NormTrj; 

      /* write data to output file */
      if (strlen(pMod->sLTFileName) > 0)
      {  fprintf(LogFilePtr, "wavelength-time distr. file  : %s \n",                pMod->sLTFileName);
      }
      else	
      {	if (strlen(pMod->sTFileName) > 0)
         {  fprintf(LogFilePtr, "time distribution file       : %s \n",             pMod->sTFileName);
         }
         else if (stSrc.eSrcType != CWS && stSrc.nSource==ANYSOURCE)
         {  fprintf(LogFilePtr, "time constants of the pulse  : %7.3f us  , %7.3f us \n",pMod->TauAscent *1.0e6, 
            pMod->TauDecay*1.0e6);
         }

         if (strlen(pMod->sLFileName) > 0)
         {  fprintf(LogFilePtr, "wavelength distribution file : %s \n",             pMod->sLFileName);
         }
         else
         {  char text[10]="";
            fprintf(LogFilePtr, "moderator temperature        : %7.3f K\n",         pMod->ModTemp);
            sprintf(text, "%5.1f K", pMod->ModTemp);
            if (imod > 0) 
               strcat (sText, " + ");
            strcat(sText, text);
         }
      }
      fprintf(LogFilePtr,   "performance factor           : %7.3f \n",                   pMod->PfmcFact);
      if (pMod->TotalFlux > 0)
         fprintf(LogFilePtr,"total neutron flux (in 2*pi) :%14.4e n/(cm^2s) \n",         pMod->TotalFlux);
      fprintf(LogFilePtr,   "moderator position           :(%7.3f  %7.3f  %7.3f) cm \n", pMod->CntrX, pMod->CntrY, pMod->CntrZ);
      if (pMod->bCircle)
        fprintf(LogFilePtr, "moderator diameter           : %7.3f cm \n",                pMod->Diameter);
      else
        fprintf(LogFilePtr, "moderator size (W x H)       : %7.3f cm  x %7.3f cm \n",    pMod->Width, pMod->Height);

      /* if (eDirDet==VT_REAL_WND)
         fprintf(LogFilePtr, "divergence defined by propagation window \n");
      else if (eDirDet==VT_VIRT_WND)
         fprintf(LogFilePtr, "divergence defined by virtual propagation window \n");
      else
         fprintf(LogFilePtr, "angle of opening used        : %7.3f     x %7.3f deg \n", 2*180*stTraj[imod].MaxDivY/M_PI, 2*180*stTraj[imod].MaxDivZ/M_PI);*/
      fprintf(LogFilePtr,    "time averaged neutron current:%14.4e n/s in%9.6f str \n", pMod->Current, SolAngle);
      fprintf(LogFilePtr,    "wavelength band used         : %7.3f Ang - %7.3f Ang \n", stTraj[imod].LambdaMin, stTraj[imod].LambdaMax);
      if (stSrc.eSrcType != CWS)
         fprintf(LogFilePtr, "time interval used           : %7.3f ms  - %7.3f ms \n", stTraj[imod].TimeFrmMin, stTraj[imod].TimeFrmMax);
      if (pMod->Current*(stTraj[imod].LambdaMax-stTraj[imod].LambdaMin)==0.0)
         Warning("The calculated absolute neutron flux for the given parameter set is zero,\n"
                 "probably because one parameter has a zero range (e.g. delta_lambda = 0, mod_area = 0, ...)\n"
                 "the simulation is performed with a flux normalized to a max. value of 1 n/(cm^2s) \n\n");
      fprintf(LogFilePtr, "\n");
   }  // end loop over moderators

   if (!bVisTraj) 
     WriteInstrData(NullPos);
   WriteSimData(dTimeMeas, dLmbdWant, stSrc.PulseFreq, NumberOfNeutrons);

   /* Propagation, Polarisation */
   if (pBeamline!=NULL)
     fprintf(LogFilePtr, "Beamline %s\n", pBeamline);
   if (eDirDet==VT_DIVERGENCE)
   { fprintf(LogFilePtr, "direction by divergence      : %7.3f - %7.3f  x  %7.3f - %7.3f  deg\n  propagation distance       : %7.3f m\n",
                         2.0*180.0*stTraj[0].MinDivY/M_PI, 2.0*180.0*stTraj[0].MaxDivY/M_PI, 
                         2.0*180.0*stTraj[0].MinDivZ/M_PI, 2.0*180.0*stTraj[0].MaxDivZ/M_PI, WindowDist/100.0);
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
   if (sTraceFileName!=NULL)
      fprintf(LogFilePtr, "trace file used              : %s\n", sTraceFileName);

   /* redefinition in terms of eigenvectors e.g. 0 % means 50% Up and 50% Down */
   FracPolDir  = 0.5 + 0.5*PolDegree/100.0;

   /* General simulation settings */
   if(keygrav == 1)
      fprintf(LogFilePtr,"\nGravity is enabled \n");
   else
      fprintf(LogFilePtr,"\nGravity is disabled \n");
   fprintf(LogFilePtr,"Cutoff probability per traj. : %10.3e \n", wei_min);
   // fprintf(LogFilePtr,"random seed                  : %ld \n",  idum);

   dDecCos = cos(Declination*M_PI/180.0);
   dDecSin = sin(Declination*M_PI/180.0);

   if (NThreads > 0)
     setDetachedWrite();
 
   DECLARE_ABORT;

   // -------------------------------
   //   Generate neutron trajectories
   // -------------------------------
   for (No=BufferIndex=TransmittedNeutrons=0; No<NumberOfNeutrons; No++) 
   {
      double prob;

      CHECK;

      // provide data for progress meter
      if ((i & 0xff) == 0) 
      {
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
         {  ig2++;
         }
      } 
      else
      {  i++;
      }

      Input.ID.IDGrp[0] = ig1;
      Input.ID.IDGrp[1] = ig2;
      Input.ID.IDNo     = i;
      Input.Debug       = eTraceMode==WRITE_TRC_FILES ? GetTraceState(Input.ID) : 'N';

      /* choose moderator, if there are more than 1 */
      if (nNumMod > 1)
        imod = (short) (i % nNumMod); // i - nNumMod*(i/nNumMod); 
      else
        imod = 0;
      pMod = &(stMod[imod]);

      Input.Color = pMod->nColour;

      /* MC choice of starting position */
      if (pMod->bCircle) 
      {
         double diam, halfdiam;
         diam = pMod->Diameter;
         halfdiam = diam / 2.0;
         do 
         {
            Y0                = pMod->CntrY + halfdiam - diam*Vran();
            Input.Position[2] = pMod->CntrZ + halfdiam - diam*Vran();
         }	/* repeat if starting point is out of circle */
         while (sq(Y0 - pMod->CntrY) + sq(Input.Position[2] - pMod->CntrZ) > sq(halfdiam)) ; 
      } 
      else 
      {
         Y0                = pMod->CntrY + pMod->Width /2.0 - pMod->Width  * Vran();
         Input.Position[2] = pMod->CntrZ + pMod->Height/2.0 - pMod->Height * Vran();
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
      { Input.Position[1] = Y0 * dDecCos - pMod->CntrX * dDecSin;
	      Input.Position[0] = Y0 * dDecSin + pMod->CntrX * dDecCos;
      }
      else 
      { Input.Position[0] = 0.0;
        Input.Position[1] = Y0;
      }

      /* MC choice of wavelength and starting time */
      if (pMod->eIsisTS > 0)
      {
        ISISgetpoint(&Input.Time, &Input.Wavelength);
      }
      else 
      {
        Input.Wavelength = stTraj[imod].LambdaMin  + (stTraj[imod].LambdaMax  - stTraj[imod].LambdaMin)  * Vran();
        Input.Time       = stTraj[imod].TimeFrmMin + (stTraj[imod].TimeFrmMax - stTraj[imod].TimeFrmMin) * Vran();
      }
      if (Input.Wavelength==0.0) continue;
      	
      /* Calculation of intensity expressed by a count rate for this trajectory referring to SPSS, LPSS or CWS */
      if (stSrc.eSrcType == CWS)
      {   prob =                  pMod->NormInt * stFluxL[imod].pDisFct(Input.Wavelength, pMod->ModTemp) / stFluxL[imod].Int;
          test =  pMod->FUAmpl  * pMod->NormTrj * stFluxL[imod].pDisFct(Input.Wavelength, pMod->ModTemp) / stFluxL[imod].Int 
                + pMod->FUAmpUM * pMod->NormTrj * NotMaxwell(Input.Wavelength, pMod->Chi, pMod->Kappa);
      }
      else 
      {  
         TimeAtModerator = Input.Time;
         if (stSrc.PulsePeriod > 0.0) 
         {
            while(TimeAtModerator < 0.0)               {TimeAtModerator += stSrc.PulsePeriod;}
            while(TimeAtModerator > stSrc.PulsePeriod) {TimeAtModerator -= stSrc.PulsePeriod;}
         }
         TimeAtModerator *= 0.001;   /*  time in seconds  */

         if(strlen(pMod->sLTFileName) > 0) 
         {
            // case: flux(lambda,t) was given in a file
            if (pMod->eIsisTS > 0) 
               prob = IsisNorm * TS.Total * 3.744905847e14 * 1.1879451 * SolAngle * WindowWidth * WindowHeight * stSrc.PulseFreq / NumberOfNeutrons;
            else
               prob = stFluxLT[imod].pDisFct(Input.Wavelength, TimeAtModerator) / stFluxLT[imod].Int * pMod->NormInt;
         }
         else if (stSrc.nSource==ESS || stSrc.nSource==SNS)
         {  // case ESS, SNS
            if (stSrc.nSource==ESS && iDataVsn == 5)
            {  prob = EssModFU_Butterfly2015(pMod->Height, stSrc.Power, stSrc.PulseFreq, Declination, &Input, stMod[0].PfmcFact, stMod[1].PfmcFact);
               prob = prob / pMod->FUAmpl * pMod->NormInt;
               Input.Color=GetColour_ESSbutterfly2015(Input.Position[1], Declination);
            }
            else if (stSrc.nSource==ESS && iDataVsn == 6)
            {  prob = EssModFU_Butterfly2016(pMod->ModTemp, stSrc.Power, stSrc.PulseFreq, Declination, &Input, stMod[0].PfmcFact, stMod[1].PfmcFact);
               prob = prob / pMod->FUAmpl * pMod->NormInt;
               if (ColorByLmbd) 
                 Input.Color=GetColour_ESSbutterfly2016(Input.Wavelength);
            }
            else 
            {  prob = EssModFU(Input.Wavelength, TimeAtModerator, stSrc.PulseLength) / pMod->FUAmpl * pMod->NormInt;
            }
         }
         else if (stSrc.nSource==CSNS)
         {  // case CSNS
            prob = CsnsModFU(Input.Wavelength, TimeAtModerator, Input.Position[1], Input.Position[2]) / pMod->FUAmpl * pMod->NormInt;
         }
         else
         {  prob =                  pMod->NormInt * stFluxL[imod].pDisFct(Input.Wavelength, pMod->ModTemp) / stFluxL[imod].Int * stFluxT[imod].pDisFct(TimeAtModerator, pMod->TauDecay, pMod->TauDecay/pMod->TauAscent, stSrc.PulseLength) / stFluxT[imod].Int;
            test =  pMod->FUAmpl  * pMod->NormTrj * stFluxL[imod].pDisFct(Input.Wavelength, pMod->ModTemp) / stFluxL[imod].Int * stFluxT[imod].pDisFct(TimeAtModerator, pMod->TauDecay, pMod->TauDecay/pMod->TauAscent, stSrc.PulseLength) / stFluxT[imod].Int
                  + pMod->FUAmpUM * pMod->NormTrj * NotMaxwell(Input.Wavelength, pMod->Chi, pMod->Kappa                        * stFluxT[imod].pDisFct(TimeAtModerator, pMod->TauDecUM, pMod->TauDecUM/pMod->TauAscUM,  stSrc.PulseLength) / stFluxT[imod].Int);
         }
      }

      if(prob <= 0.0) continue; 

      /* Declination for ESS butterfly 2015 (others: has been done already)*/
      if(stSrc.nSource==ESS && iDataVsn == 5)
      { Input.Position[1] = Y0 * dDecCos - pMod->CntrX * dDecSin;
	      Input.Position[0] = Y0 * dDecSin + pMod->CntrX * dDecCos;
      }

      /* direction of flight */
      /* defined by starting position on moderator and position on propagation window */
      if (eDirDet!=VT_DIVERGENCE && pMod->DistModWnd > 0.0)  
      {	
         /* choosing point on propagtion window and calculating distance between points */
         WndY = MonteCarlo(-0.5*WindowWidth,  0.5*WindowWidth);
         WndZ = MonteCarlo(-0.5*WindowHeight, 0.5*WindowHeight);
         Path = sqrt( sq(pMod->DistModWnd - Input.Position[0]) 
                    + sq(WndY - Input.Position[1])
                    + sq(WndZ - Input.Position[2]) );
         /* correction for gravity effect */
         if (keygrav==ON)
            WndZ += 0.5*G*sq(Path/V_FROM_LAMBDA(Input.Wavelength))/10000.;

         Input.Vector[1] = (WndY - Input.Position[1])/Path;
         Input.Vector[2] = (WndZ - Input.Position[2])/Path;
         Input.Vector[0] = sqrt(1.0 - sq(Input.Vector[1]) - sq(Input.Vector[2]));

         /* correcting count rate for an equal distribution in solid angle 
            factor: tan'(theta)*tan'(phi) = cos²(theta)*cos²(phi)          */
         Phi   = atan(Input.Vector[1]/Input.Vector[0]);
         Theta = atan(Input.Vector[2]/Input.Vector[0]);

         FactWnd = sq(cos(Theta)*cos(Phi)) / pMod->WndFact;
         prob *= FactWnd; 
         prob /= sq(Path/pMod->DistModWnd); //  correction for solid angles in case of different distances from source to (virtual) window 
      } 
      else  
      {
         /* defined by divergence */
         double y=Vran(), z=Vran();
         if (y < 0.5) Phi   =                           -stTraj[imod].MaxDivY +  2 * y *(stTraj[imod].MaxDivY - stTraj[imod].MinDivY);
         else         Phi   = 2 * stTraj[imod].MinDivY - stTraj[imod].MaxDivY +  2 * y *(stTraj[imod].MaxDivY - stTraj[imod].MinDivY);
         if (z < 0.5) Theta =                           -stTraj[imod].MaxDivZ +  2 * z *(stTraj[imod].MaxDivZ - stTraj[imod].MinDivZ);
         else         Theta = 2 * stTraj[imod].MinDivZ - stTraj[imod].MaxDivZ +  2 * z *(stTraj[imod].MaxDivZ - stTraj[imod].MinDivZ);
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

   // ------------------------------------------------------------
   // Finish: write log, geometry and instrument file, free memory
   // ------------------------------------------------------------
  my_exit:
   if (SumProb != 0.0) 
   {
      CenterX /= SumProb;  CenterY   /= SumProb;
      CenterZ /= SumProb;  AveTimeOF /= SumProb; 
      fprintf(LogFilePtr,"Center of beam at window     :(%7.3f  %7.3f  %7.3f) cm \n", CenterX, CenterY, CenterZ);
      fprintf(LogFilePtr,"Average TOF                  : %7.3f ms \n", AveTimeOF);
   }
   else
   {
      fprintf(LogFilePtr,"\nNo neutrons on the exit of this module \n");
   }	

   fprintf(LogFilePtr,"\nnumber of trajectories started         : %11.0f\n", NumberOfNeutrons);

  /* write geometry file */
   SetGeometry("yellow");

   /* Do module specific cleanups */
   OwnCleanup();

   /* Do the general cleanup */
   Cleanup(-Endpoint.D,0.0,0.0, 0.0,0.0);

   return(0);
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char **argv)
{
  int  i,j;
   char  *arg=NULL;

   /* Initialize */
   stSrc.PulseLength = 0.002;      /* [s] LPSS pulse length 2 ms            */ 
   stSrc.pSrcName     = "";
   InitTrajRange(&stTraj[0]);

   for(i=1; i<argc; i++)
   {
      if(argv[i][0]!='+') 
      {
        arg=&argv[i][2];   //free:           C     e E     g G   H   I j J   K l           o O     q Q             u U         x         Z
        switch(argv[i][1]) //used: a A b B c   d D     f F     h   i       k     L m M n N     p P     r R s S t T     v V w W   X y Y z  
        {
            /* Simulation */
          case 'n':
            NumberOfNeutrons = atof(arg);
            break;

          case 'r':
            sTraceFileName=arg;  
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
            dLmbdWant = (double) atof(arg); /* [Ang] */
            break;


            /* source and moderator */
          case 'S':
            stSrc.eSrcType = (short)atoi(arg); /* 1: CWS; 2: SPSS; 3: LPSS */
            fprintf(LogFilePtr,"Source type                  : %d \n", stSrc.eSrcType);
            break;
          case 'N':
            stSrc.pSrcName = arg;
            if (strcmp(arg,"ESS")==0)
              stSrc.nSource = ESS;
            else if (strcmp(arg,"SNS")==0)
              stSrc.nSource  = SNS;
            else if (strcmp(arg,"ISIS")==0)
              stSrc.nSource  = ISIS;
            else if (strcmp(arg,"CSNS")==0)
              stSrc.nSource  = CSNS;
            else 
              stSrc.nSource = ANYSOURCE;	     /* no specific source given */
            break;
          case 'v':
            iDataVsn = (short)atoi(arg);       /* version of the data base */
            break;

          case 'R':
            stSrc.PulseFreq = (double)atof(arg);          /* pulse repetition rate   [Hz] */
            if (stSrc.PulseFreq > 0.0)
              stSrc.PulsePeriod = 1000./stSrc.PulseFreq; /* time between two pulses [ms] */
            else
              stSrc.PulsePeriod = 0.0;
            break;
          case 'L':
            stSrc.Power = (double)atof(arg);          /* average source power [MW];  */
            stSrc.Power*= 1.0e6;                      /* power                [W];   */
            break;
          case 'p':
            stSrc.PulseLength = (double)atof(arg);    /* LPSS: proton pulselength [ms]  */
            stSrc.PulseLength*=0.001;                 /* pulse length              [s]  */
            break;

          case 'a':
            sModFileName=arg;  
            break;
          case 'B':
            pBeamline=arg;  
            break;

            /* neutron parameters */
          case 'm':
            stTraj[0].LambdaMin = (double)atof(arg); /* [A] */
            break;
          case 'M':
            stTraj[0].LambdaMax = (double)atof(arg);  /* [A] */
            break;

          case 't':
            stTraj[0].TimeFrmMin = (double)atof(arg); /* TimeFrame [TimeFrameMin;TimeFrameMax] at Moderator [ms]*/
            break;
          case 'T':
            stTraj[0].TimeFrmMax = (double)atof(arg);
            break;

          case 'b':
            stTraj[0].MinDivY = M_PI*(double)atof(arg)/180.0; /* [deg] */
            break;
          case 'y':
            stTraj[0].MaxDivY = M_PI*(double)atof(arg)/180.0; /* [deg] */
            break;
          case 'c':
            stTraj[0].MinDivZ = M_PI*(double)atof(arg)/180.0;  /* [deg] */
            break;
          case 'z':
            stTraj[0].MaxDivZ = M_PI*(double)atof(arg)/180.0;  /* [deg] */
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
 

/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  short m;     /* index for moderators  */

  /* print messages of loops (if existing) */
  // PrintMessage(SRC_L_RANGE_TOO_SMALL, stMod[imod].sLFileName, OFF);
  // PrintMessage(SRC_T_RANGE_TOO_SMALL, stMod[imod].sTFileName, OFF);
  // PrintMessage(SRC_LT_RANGE_TOO_SMALL,stMod[imod].sLTFileName,OFF);


  /* free allocated memory */
  for (m=0; m < nNumMod; m++)
  { 
    if (stFluxL[m].pTabX!=NULL)  free(stFluxL[m].pTabX);
    if (stFluxL[m].pTabF!=NULL)  free(stFluxL[m].pTabF);
    if (stFluxT[m].pTabX!=NULL)  free(stFluxT[m].pTabX);
    if (stFluxT[m].pTabF!=NULL)  free(stFluxT[m].pTabF);
    if (stFluxLT[m].pTabX!=NULL) free(stFluxLT[m].pTabX);
    if (stFluxLT[m].pTabY!=NULL) free(stFluxLT[m].pTabY);
    if (stFluxLT[m].pTabF!=NULL) free(stFluxLT[m].pTabF);
  }
  if (aTrace!=NULL) free(aTrace);
}


/********************************************************/
/* reads moderator parameters from file                 */
/********************************************************/
short ReadModData(char* sFileName)
{
  char  sShape   [2]="S",
        sBuffer [CHAR_BUF_LENGTH];
  FILE* pFileR;

  pFileR = OpenInputFile(sFileName, TRUE,"rt");

  /* Read a line for each moderator */
  while (ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1)==TRUE)
  { 
    stMod[imod].eIsisTS   = 0;    // default: no ISIS moderator 
    stMod[imod].PfmcFact = 1.0;  //          performance factor = 1
    if (stSrc.nSource==ISIS)
    { sscanf(sBuffer, "%lf %hd %s  %lf %lf %lf  %lf %lf  %hd %lf %lf  %s %s %s  %hd %lf %lf  %hd  %lf %lf %lf  %lf %lf", 
                      &stMod[imod].ModTemp,     &stMod[imod].nColour,     sShape,  
                      &stMod[imod].CntrX,       &stMod[imod].CntrY,      &stMod[imod].CntrZ,  
                      &stMod[imod].Width,       &stMod[imod].Height,   
                      &stMod[imod].nBackground, &stMod[imod].TotalFlux,  &stMod[imod].Current, 
                       stMod[imod].sLFileName,   stMod[imod].sTFileName,  stMod[imod].sLTFileName, 
                      &stMod[imod].eModType,    &stMod[imod].TauAscent , &stMod[imod].TauDecay,
                      &stMod[imod].eIsisTS,
                      &stMod[imod].TotFluxUM,   &stMod[imod].Chi,        &stMod[imod].Kappa,
                      &stMod[imod].TauAscUM,    &stMod[imod].TauDecUM);
    }
    else
    { sscanf(sBuffer, "%lf %hd %s  %lf %lf %lf  %lf %lf  %hd %lf %lf  %s %s %s  %hd %lf %lf  %lf  %lf %lf %lf  %lf %lf", 
                      &stMod[imod].ModTemp,     &stMod[imod].nColour,      sShape,  
                      &stMod[imod].CntrX,       &stMod[imod].CntrY,       &stMod[imod].CntrZ,  
                      &stMod[imod].Width,       &stMod[imod].Height,   
                      &stMod[imod].nBackground, &stMod[imod].TotalFlux,   &stMod[imod].Current, 
                       stMod[imod].sLFileName,   stMod[imod].sTFileName,   stMod[imod].sLTFileName, 
                      &stMod[imod].eModType,    &stMod[imod].TauAscent ,  &stMod[imod].TauDecay,
                      &stMod[imod].PfmcFact,
                      &stMod[imod].TotFluxUM,   &stMod[imod].Chi,         &stMod[imod].Kappa,
                      &stMod[imod].TauAscUM,    &stMod[imod].TauDecUM);
    }

    CopyTrajRange(&stTraj[0], &stTraj[imod]);

    stMod[imod].TauAscent  /= 1.0e06;
    stMod[imod].TauDecay   /= 1.0e06;
    stMod[imod].TauAscUM   /= 1.0e06;
    stMod[imod].TauDecUM   /= 1.0e06;

    if (strcmp(stMod[imod].sLFileName, "none")==0 || strcmp(stMod[imod].sLFileName, "0")==0) 
      strcpy(stMod[imod].sLFileName, "");
    if (strcmp(stMod[imod].sTFileName, "none")==0 || strcmp(stMod[imod].sTFileName, "0")==0) 
      strcpy(stMod[imod].sTFileName, "");
    if (strcmp(stMod[imod].sLTFileName,"none")==0 || strcmp(stMod[imod].sLTFileName,"0")==0) 
      strcpy(stMod[imod].sLTFileName,"");

    if (strcmp(sShape, "C")==0)
    { stMod[imod].Diameter = stMod[imod].Width;
      stMod[imod].Width  = 0.0;
      stMod[imod].Height = 0.0;
      stMod[imod].Area   = M_PI * sq(stMod[imod].Diameter) / 4.0;
      stMod[imod].bCircle = TRUE;
    }
    else
    { stMod[imod].Diameter = 0.0;
      stMod[imod].Area     = stMod[imod].Height * stMod[imod].Width;        
      stMod[imod].bCircle  = FALSE;
    }
    stMod[imod].DistModWnd = WindowDist-stMod[imod].CntrX;
    stMod[imod].WndFact    = 0.0;
    stMod[imod].FUAmpUM    = stMod[imod].TotFluxUM / (2.0 * M_PI) / stSrc.PulseFreq;

    imod++;
  }

  fclose(pFileR);


  /* set only moderator option for ESS butterfly: */
  if (stSrc.nSource==ESS && iDataVsn >= 5)
  {
    if (iDataVsn==5)
    { 
      // set butterfly geometry:
      stMod[0].ModTemp = 325.0;
      stMod[0].CntrX   =   0.0; 
      stMod[0].CntrZ   =   0.0; 
      stMod[0].nColour   = 1;
      stMod[0].Diameter = 0.0;
      stMod[0].bCircle   = FALSE;
      stMod[0].DistModWnd = WindowDist-stMod[0].CntrX;

      stMod[1].ModTemp = 50.0;
      stMod[1].CntrX  = 0;
      stMod[1].CntrZ  = 0;
      stMod[1].nColour = 2;
      stMod[1].Height   = stMod[0].Height;
      stMod[1].Diameter = 0.0;
      stMod[1].bCircle   = FALSE;
      stMod[1].DistModWnd = WindowDist-stMod[1].CntrX;
      stMod[1].WndFact    = 0.0;
      stMod[1].eIsisTS = 0;

      stMod[0].PfmcFact = 0.75;
      stMod[1].PfmcFact = 0.75;
      stMod[0].Width = GetModWidth_ESSbutterfly2015(fabs(Declination),stMod[0].ModTemp);
      stMod[1].Width = GetModWidth_ESSbutterfly2015(fabs(Declination),stMod[1].ModTemp);
      stMod[0].CntrY =  0.0;
      stMod[1].CntrY = (Declination>0) ? (stMod[0].Width + stMod[1].Width) / 2.0       : -(stMod[0].Width + stMod[1].Width) / 2.0 ;
      stMod[0].Area  = stMod[0].Height * stMod[0].Width;        
      stMod[1].Area  = stMod[1].Height * stMod[1].Width;        
      imod=2;

      fprintf(LogFilePtr,"For ESS moderator description, both thermal and cold moderator are simulated and set to default values: \n"); 
      fprintf(LogFilePtr,"  thermal: at (0.0,%6.2f,0.0), %5.2f cm wide; colour: 1 \n", stMod[0].CntrY, stMod[0].Width);
      fprintf(LogFilePtr,"  cold   : at (0.0,%6.2f,0.0), %5.2f cm wide; colour: 2 \n", stMod[1].CntrY, stMod[1].Width);
      fprintf(LogFilePtr,"Only height is taken from moderator file (must be either 3 cm or 6 cm) \n");
      fprintf(LogFilePtr,"Choose beamport position via declination angle \n");
      fprintf(LogFilePtr,"Default orientation to the thermal centre (adjust with frame module) \n");
      fprintf(LogFilePtr,"! Note: default factors of %6.3f and %6.3f are applied to the brilliance to reflect the loss from engineering design details not included in the model ! \n", stMod[0].PfmcFact, stMod[1].PfmcFact);
    }
    else
    { double Shift  = GetShift_ESSbutterfly2016   (fabs(Declination)),
             Width1 = GetModWidth_ESSbutterfly2016(fabs(Declination), stMod[0].ModTemp),
             Width2 = GetModWidth_ESSbutterfly2016(fabs(Declination), stMod[1].ModTemp);
      stMod[0].CntrY = (Declination > 0.0) ? -Width1/2.0 - Shift :  Width1/2.0 + Shift ;  // thermal
      stMod[1].CntrY = (Declination > 0.0) ?  Width2/2.0 - Shift : -Width2/2.0 + Shift ;  // cold
      stMod[0].Width = Width1;
      stMod[1].Width = Width2;
      stMod[0].Area  = stMod[0].Height * stMod[0].Width * cos(Declination*M_PI/180.0);        
      stMod[1].Area  = stMod[1].Height * stMod[1].Width * cos(Declination*M_PI/180.0);        
      stMod[0].DistModWnd = WindowDist-stMod[0].CntrX;
      stMod[1].DistModWnd = WindowDist-stMod[1].CntrX;
      fprintf(LogFilePtr,"Widths and horizontal positions are calculated.\n");
    #ifndef _TEST
      if (pBeamline!=NULL)
        LoadHorDistrib(pBeamline);
      else 
        LoadHorDistrib(GenerBeamport(Declination));
    #endif
    }

    CopyTrajRange(&stTraj[0], &stTraj[1]);

    // add second cold for the theta=0 view:
    if (iDataVsn==5 && fabs(Declination) < 0.01)
    {
      imod=3;
      stMod[2].ModTemp = 50.0;
      stMod[2].Width =  stMod[1].Width;
      stMod[2].CntrY = -stMod[1].CntrY;
      stMod[2].CntrX = 0;
      stMod[2].CntrZ = 0;
      stMod[2].nColour = 2;
      stMod[2].Height = stMod[0].Height;
      stMod[2].Area   = stMod[2].Height * stMod[2].Width; 
        if (iDataVsn==6) stMod[2].Area *= cos(Declination*M_PI/180.0);
      stMod[2].bCircle   = FALSE;
      stMod[2].Diameter = 0.0;
      stMod[2].DistModWnd = WindowDist-stMod[2].CntrX;
      stMod[2].WndFact    = 0.0;
      stMod[2].PfmcFact   = stMod[2].PfmcFact;
      stMod[2].eIsisTS = 0;

      CopyTrajRange(&stTraj[0], &stTraj[2]);

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


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  short m,     /* index for moderators  */
        kc=0,  /* index for circular moderators */
        ks=0;  /* index for rectangular moderators */

  // Geometry data
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.nCircles=0;
    stGeometry.nRectangles=1;

    for (m=0; m < nNumMod; m++)
    { 
      if (stMod[m].bCircle)
        stGeometry.nCircles++;
      else
        stGeometry.nRectangles++;
    }
    if (stGeometry.nCircles > 0)
      stGeometry.pCircle =  (VtCircle*)    calloc(stGeometry.nCircles, sizeof(VtCircle));
    stGeometry.pRectangle = (VtRectangle*) calloc(stGeometry.nRectangles, sizeof(VtRectangle));

    // Moderators
    for (m=0; m < nNumMod; m++)
    { 
      if (stMod[m].bCircle)
      { stGeometry.pCircle[kc].vCntr[0]   = stMod[m].CntrX/CmprFact;
        stGeometry.pCircle[kc].vCntr[1]   = stMod[m].CntrY;
        stGeometry.pCircle[kc].vCntr[2]   = stMod[m].CntrZ;
        stGeometry.pCircle[kc].vNormal[0] = dDecCos;
        stGeometry.pCircle[kc].vNormal[1] = dDecSin;
        stGeometry.pCircle[kc].vNormal[2] = 0.0;
        stGeometry.pCircle[kc].Radius     = stMod[m].Diameter/2.0;
        stGeometry.pCircle[kc].AngleBeg   =   0.0;
        stGeometry.pCircle[kc].AngleEnd   = 359.99;
        kc++;
      }
      else
      { stGeometry.pRectangle[ks].vCntr[0]   = stMod[m].CntrX/CmprFact;
        stGeometry.pRectangle[ks].vCntr[1]   = stMod[m].CntrY;
        stGeometry.pRectangle[ks].vCntr[2]   = stMod[m].CntrZ;
        stGeometry.pRectangle[ks].vNormal[0] = dDecCos;
        stGeometry.pRectangle[ks].vNormal[1] = dDecSin;
        stGeometry.pRectangle[ks].vNormal[2] = 0.0;
        stGeometry.pRectangle[ks].Width      = stMod[m].Width;
        stGeometry.pRectangle[ks].Height     = stMod[m].Height;
        ks++;
      }
    }

    // Propagation window
    stGeometry.pRectangle[ks].vCntr[0]   = WindowDist/CmprFact;
    stGeometry.pRectangle[ks].vCntr[1]   = 0.0;
    stGeometry.pRectangle[ks].vCntr[2]   = 0.0;
    stGeometry.pRectangle[ks].vNormal[0] = 1.0;
    stGeometry.pRectangle[ks].vNormal[1] = 0.0;
    stGeometry.pRectangle[ks].vNormal[2] = 0.0;
    stGeometry.pRectangle[ks].Width      = WindowWidth;
    stGeometry.pRectangle[ks].Height     = WindowHeight;
  }
}


/****************************************************************************************/
/* loads wavelength distribution from file or set 'Maxwellian' as distribution function */
/****************************************************************************************/
void LoadWavelengthDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxL)
{
   long   i;
   double dDelX=0.0, dF=0.0;
   char   sBuffer[CHAR_BUF_LENGTH]=""; 
   FILE*  pDisFile=NULL;

   /* loading wavelength distribution file, if its name is given and range is set properly  */
   if(strlen(pMod->sLFileName) > 0) 
   {
      if (pTraj->LambdaMin >= 0.0  &&  pTraj->LambdaMax > pTraj->LambdaMin)
      {
        /* opening distribution file */
        pDisFile = OpenInputFile(pMod->sLFileName, FALSE, "rt");
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
            if (pTraj->LambdaMin >= pFluxL->pTabX[0] && pTraj->LambdaMax <= pFluxL->pTabX[pFluxL->nLines-1])
            {
               pFluxL->pDisFct = (double(*)()) UserLambdaDis;
            } 
            else 
            {  fprintf(LogFilePtr,"ERROR: The wavelength range given in %s is smaller than that in the simulation\n", pMod->sLFileName);
               exit(-1);
            }

            /* integration of function f(lambda) and storing of ln f */
            pFluxL->Int=0.0;
            for(i=1; i < pFluxL->nLines; i++)
            {  
              dDelX =  pFluxL->pTabX[i] - pFluxL->pTabX[i-1];
              dF    = (pFluxL->pTabF[i] + pFluxL->pTabF[i-1])/2.0;
              pFluxL->Int += dF*dDelX;
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
        { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength distribution\nPlease copy (from ...FILES/moderators/...) to input directory\n", 
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
    pFluxL->Int    = 1.0 ;
   }
}


/**************************************************************************************************************/
/* loads time distribution for the pulse from file or set 'PulseShape' or 'PulseInt' as distribution function */
/**************************************************************************************************************/
void LoadTimeDistribution(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxT)
{
  long   i;
  double X1, X2, dDelX=0.0;
  char   sBuffer[CHAR_BUF_LENGTH]=""; 
  FILE*  pDisFile=NULL;

  /* loading time distribution file, if its name is given and range is set properly */
  if(strlen(pMod->sTFileName) > 0) 
  {
    if (pTraj->TimeFrmMax > pTraj->TimeFrmMin)
    {
      /* opening distribution file */
      pDisFile = OpenInputFile(pMod->sTFileName, FALSE, "rt");
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
        if (pTraj->TimeFrmMin >= pFluxT->pTabX[0] &&  pTraj->TimeFrmMax <= pFluxT->pTabX[pFluxT->nLines-1])
        {
          pFluxT->pDisFct = (double(*)()) UserTimeDis;
        } 
        else 
        { fprintf(LogFilePtr,"ERROR: The time range given in %s is smaller than that in the simulation\n", pMod->sTFileName);
          exit(-1);
        }

        /* integration  function f(time) and saves ln(f) */
        pFluxT->Int=0.0;
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
          pFluxT->Int += pFluxT->pTabF[i]*dDelX / 1000.0;

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
      { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given time distribution\nPlease copy (from ...FILES/moderators/...) to input directory\n", pMod->sTFileName);
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
      case SPSS: pFluxT->pDisFct = (double(*)()) PulseShapeP;    break;
      case LPSS: pFluxT->pDisFct = (double(*)()) LongPulseShape; break;
      default  : fprintf(LogFilePtr,"ERROR: Wrong value %d for variable 'source type'\n", stSrc.eSrcType);
                 exit(-1);
    }
    pFluxT->Int = 1.0 ;
  }
}


/********************************************************/
/* loads 2D wavelength-time distribution from file      */
/********************************************************/
void  LoadWavelengthTimeDistrib(Moderator* pMod, TrajParam* pTraj, FctTable* pFluxLT)
{
  long   i, j;
  double X1, X2, dDelX=0.0, Y1, Y2, dDelY=0.0;
  char   sBuffer[CHAR_BUF_LARGE]=""; 
  FILE*  pDisFile=NULL;

  /* loading distribution file, if its range is set properly  */
  if (pTraj->TimeFrmMax > pTraj->TimeFrmMin &&
      pTraj->LambdaMax  > pTraj->LambdaMin  && pTraj->LambdaMin >= 0.0)
  {
    /* openíng distribution file */
    pDisFile = OpenInputFile(pMod->sLTFileName, FALSE, "rt");
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
      if (   pTraj->TimeFrmMin >= pFluxLT->pTabX[0]  
          && pTraj->TimeFrmMax <= pFluxLT->pTabX[pFluxLT->nLines-1] 
          && pTraj->LambdaMin  >= pFluxLT->pTabY[0] 
          && pTraj->LambdaMax  <= pFluxLT->pTabY[pFluxLT->nColumns-1])
      {
        pFluxLT->pDisFct = (double(*)()) UserLmbdTimeDis;
      } 
      else 
      { fprintf(LogFilePtr,"ERROR: The wavelength or the time range given in %s is smaller than that in the simulation\n", pMod->sLTFileName);
        exit(-1);
      }
 
      /* integrating function f(lambda, time) */
      /* x range devided into logarithmicly increasing bins */
      pFluxLT->Int=0.0;
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
          pFluxLT->Int += pFluxLT->pTabF[IndLT(i,j)]*dDelX*dDelY / 1000.0;
        } // end for loop over columns (index j)				
      }   // end for loop over lines (index i)	
  			
      /* ISIS normalisation: FU/proton -> FU */
      if (pFluxLT->Int < 1.0 && pMod->eIsisTS > 0) 
      {	
        double fact = pMod->eIsisTS == 1 ? 1.8e-04 / E_C : 0.6e-04 / E_C;
  	      
        for (i=0; i < pFluxLT->nLines; i++)
          for (j=0; j < pFluxLT->nColumns; j++)
            pFluxLT->pTabF[IndLT(i,j)] *= fact;
  
        pFluxLT->Int *= fact;
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
    { fprintf(LogFilePtr,"ERROR: Can't open %s to read user given wavelength-time distribution\nPlease copy (from ...FILES/moderators/...) to input directory\n", pMod->sLTFileName);
      exit (-1);
    }
  } 
  else // ranges not properly set 
  { fprintf(LogFilePtr,"ERROR: You have to specify the parameters -m and -M as well as -t and -T properly!\n");
    exit(-1);
  }
}


/********************************************************/
/* loads list of trajectories that shall be traced      */
/********************************************************/
void LoadTraceFile()
{
  char  sBuffer[CHAR_BUF_LENGTH]; 
  FILE* pTraceFile=NULL;

  /* If there is a trace file go and load the file */
  if(sTraceFileName!=NULL) 
   {
      /* opens distribution file */
      if ((pTraceFile = OpenInputFile(sTraceFileName, FALSE,"rt")) != NULL) 
      {
        long i;

        /* reads number of lines, allocates memory and then reads distribution file */
        nLinesTr = LinesInFile(pTraceFile);
        aTrace   = (TotalID*) calloc(nLinesTr, sizeof(TotalID));

        for(i=0; i<nLinesTr; i++)
        {  
          ReadLine(pTraceFile, sBuffer, CHAR_BUF_LENGTH-1);
          sscanf  (sBuffer, "%c%c%lu", &aTrace[i].IDGrp[0], &aTrace[i].IDGrp[1], &aTrace[i].IDNo);
        }

        /* closes trace file */
        fclose(pTraceFile) ;
      } 
      else 
      { fprintf(LogFilePtr, "\nERROR: Can't open %s to read trace file\n", sTraceFileName);
        exit (-1);
      }
   }
}


/********************************************************/
/* looks if trajectory shall be traced                  */
/********************************************************/
char GetTraceState(TotalID stID)
{
  char cRet = 'N'; 

  if (nLinesTr > 0) {	
    long   i, il=nLinesTr-1;  /* lines in Table */
    double nS, nL;              /* numbers got by conversion from IDs */
    double gS, gL;

    gS = (aTrace[il].IDGrp[0]-'A')*1.117e11;
    gL = (aTrace[il].IDGrp[1]-'A')*4.295e09;
    nS = (stID.IDGrp[0]-'A')*1.117e11 + (stID.IDGrp[1]-'A')*4.295e09 + stID.IDNo;
    //nL = (aTrace[il].IDGrp[0]-'A')*1.117e11 + (aTrace[il].IDGrp[1]-'A')*4.295e09 + aTrace[il].IDNo;
    nL = gS                                   + gL                                   + aTrace[il].IDNo;
    i = (long) (il * nS / nL + 0.5);
    if (i > il) i = il;

    /*while ( (aTrace[il].IDGrp[0]-'A')*1.117e11 + (aTrace[il].IDGrp[1]-'A')*4.295e09 + aTrace[i].IDNo < nS  &&  i < il ) 
      i++;
      while ( (aTrace[il].IDGrp[0]-'A')*1.117e11 + (aTrace[il].IDGrp[1]-'A')*4.295e09 + aTrace[i].IDNo > nS  &&  i > 0 ) 
      i--;
    */

    while ( (i < il) && (gS + gL + aTrace[i].IDNo < nS) ) 
      i++;
    while ( (i > 0) && (i <= il) && (gS + gL + aTrace[i].IDNo > nS) ) 
      i--;

    // set 'tracing', if IDs are identical
    if (memcmp(stID.IDGrp, aTrace[i].IDGrp, 2)==0 && stID.IDNo==aTrace[i].IDNo)
      cRet='T'; 
  }
  
  return cRet;
}


/****************************************************************************************/
/* Checks whether position (X,Y) of actual moderator 'imod' is behind moderator i       */
/****************************************************************************************/
int  PosBehindMod(const int i, const double Y, const double Z)
{
  return stMod[i].nBackground < stMod[imod].nBackground &&
    (  ( stMod[i].bCircle &&   sq(Y-stMod[i].CntrY) + sq(Z-stMod[i].CntrZ) <= sq(stMod[i].Diameter/2.0) ) ||
       (!stMod[i].bCircle && fabs(Y-stMod[i].CntrY) <= 0.5*stMod[i].Width 
	                        && fabs(Z-stMod[i].CntrZ) <= 0.5*stMod[i].Height) );
}



/*******************************************************/
/* ISIS SPECIFIC FUNCTIONS                             */
/*******************************************************/
double** matrix(const int m,const int n)
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


int    binSearch(int Npts,double* AR,double V)
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


double calcFraction(double EI,double EE,double Ea,double Eb)
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
