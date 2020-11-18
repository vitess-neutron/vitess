#ifndef SRC_MODCHAR_H
#define SRC_MODCHAR_H

/***********************************************/
/* Functions for simulations of moderators     */
/***********************************************/


/***********************************************/
/* Definitions                                 */
/***********************************************/

#define ANYSOURCE 0
#define ESS       1
#define SNS       2
#define ISIS      3
#define CSNS      4
#define HBS       5
#define ILL      10
#define HMI      11

#define CWS       1
#define SPSS      2
#define LPSS      3

#define POISONED   1   /* moderator decoupled poisoned        */
#define DECOUPLED  2   /* moderator decoupled unpoisoned      */
#define COUPLED    3   /* moderator coupled                   */
#define MULT_SPEC  4   /* effective spectrum of a moderator consisting of a cold and thermal part      */

#define NUM_MOD    5   /* max. number of moderators in the moderator system */

#define FNL      101


/***********************************************/
/* Structures                                  */
/***********************************************/
typedef struct
{	
  short  eSrcType;       /* source type: CWS SPSS LPSS */
  const char *pSrcName;
  short  nSource;
  double PulseFreq;     /* repetition rate of the pulses in Hz */
  double PulsePeriod;   /* period of pulse cycle         in ms */
  double PulseLength;   /* LPSS pulse length             in s  */
  double Power;         /* average power of the source   in W  */
}
Source;

typedef struct 
{
  double ModTemp;
  short  nBackground;
  short  nColour;
  char   bCircle;
  double CntrX;
  double CntrY;
  double CntrZ;
  double Width;
  double Height;
  double Diameter;
  double Area;            /* area of the moderator [cm²] */
  double DistModWnd;      /* distance moderator - propagation window  */
  double WndFact;         /* factor to normalise divergence distribution defined 'by window' */
  double TotalFlux;       /* total CW-flux on the moderator surface [n/cm²s]  */
  double Current;         /* mean neutron current leaving the moderator [n/s] */
  double NormInt;         /* mean neutron current per traj. normalized by wavelength [n*Ang/s]
                                (and for spallation sources) by time interval         [n*Ang]   */
  double NormTrj;         /* [Ang cm²sr] Phase space volume per trajectory = NormInt/FUAmpl */
  double PfmcFact;        /* performance factor considering losses by the technical realization */
  char   sLFileName[FNL];
  char   sTFileName[FNL];
  char   sLTFileName[FNL];
  short  eModType;
  double TauAscent;       //   [s]    ascent time constant of the moderated neutrons in the pulse
  double TauDecay;        //   [s]    decay time constant of the moderated neutrons in the pulse 

  double TotFluxUM;       // [n/cm²/s] total CW-flux of the under-moderated neutrons on the moderator surface
  double Chi;             // [1/Ang]   factor for the wavelength dependence of under-moderated neutrons 
  double Kappa;           //   [1]     scaling factor for the flux of under-moderated neutrons
  double TauAscUM;        //   [s]     ascent time constant of the under-moderated neutrons in the pulse
  double TauDecUM;        //   [s]     decay time constant of the under-moderated neutrons in the pulse

  double FUAmpl,          // [n/cm²/sr] pulse ampl. on the moderator surface (number of neutrons per area per solid angle per pulse ) 
         FUAmpUM;         //            same for undermoderated neutrons
  short  eIsisTS;	        /* Target station 0: no ISIS moderator, 1: TS1, 2: TS2 */
}
Moderator;


typedef struct
{
  double F001;
  double F002;
  double F003;
  double Temp;
  double alpha_SD;
  double kappa_SD;
  double alpha_L;
  double lambda_L;
  double expo_L;
  double alpha_1;
  double alpha_2;
}
ModInfo;


typedef struct 
{
  double I_SD;
  double alpha_SD;
  double lambda_SD;
  double alpha_L;
  double lambda_L;
  double expo_L;
  double I1;
  double alpha_1;
  double I2;
  double alpha_2;
  double T_real;
}
EssModChar;

typedef struct
{	
  double LambdaMin;
  double LambdaMax;
  double MinDivY;
  double MaxDivY;
  double MinDivZ;
  double MaxDivZ;
  double TimeFrmMin;
  double TimeFrmMax;
}
TrajParam;

typedef struct
{	
  double* pTabX;
  double* pTabY;
  double* pTabF;
  long    nLines;
  long    nColumns;
  double  Int;
  double (*pDisFct)();
}
FctTable;


/***********************************************/
/* Prototypes                                  */
/***********************************************/
void   InitModerator  (Moderator* pMod);
void   InitTrajRange  (TrajParam* pTrj);
void   CopyTrajRange  (const TrajParam* pSrc, TrajParam* pDest);

long   IndLT          (const long   i,       const long j);
double TotalFU        (const double Temp,    const short  eSource, const short  eModType,
					             const double Power,   const double Period,  const double PulseLen);
double EssModFU       (const double Lambda,  const double Time,    const double Length);
                      
double Maxwellian     (const double lambda,  const double ModTemp);
double LeakageFct     (const double lambda,  const ModInfo* pInfo);
double EmpCorrFact    (double lmbd);
double NotMaxwell     (const double lambda,  const double chi,     const double kappa);
short  GetEssModDat   (ModInfo*     ModInfo, const double ModTemp, const double ModHeight, const short iVsn);

double UserLambdaDis  (const double Lambda,  const double ModTemp);
double UserLmbdTimeDis(const double Lambda,  const double Time);
double UserTimeDis    (const double Time,    const double TauDecay, const double TauRatio, const double PulsLen);

double PulseShapeP    (const double Time,    const double TauDecay, const double TauRatio, const double PulsLen);
double ShortPulseShape(const double Time,    const double TauDecay, const double TauRatio);
double LongPulseShape (const double Time,    const double TauDecay, const double TauRatio, const double PulsLen);
double PulseShapeInt  (const double Time,    const double TauDecay, const double TauAscent);

double f_cold         (const double lambda);
double f_therm        (const double lambda);

double AveSolidAngleC(const double ModDiam,
                      const double WndWidth, const double WndHeight, const double Dist);
double AveSolidAngleR(const double ModWidth, const double ModHeight,
                      const double WndWidth, const double WndHeight, const double Dist);
double AveWeightC    (const double ModCntrY, const double ModCntrZ,  const double ModDiam,
                      const double WndWidth, const double WndHeight, const double Dist);
double AveWeightR    (const double ModCntrY, const double ModCntrZ,
                      const double ModWidth, const double ModHeight,
                      const double WndWidth, const double WndHeight, const double Dist);
double IntAtan       (const double IntAnf,   const double IntEnd,    const double Param);
double WeightDirByWnd(const double WndWidth, const double Dist,      const double ModPos);

#endif

