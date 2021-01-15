#ifndef SRC_MODCHAR_H
#define SRC_MODCHAR_H

/***********************************************/
/* Functions for simulations of moderators     */
/***********************************************/


/***********************************************/
/* Definitions                                 */
/***********************************************/

// Name of source
#define ANYSOURCE 0
#define ESS       1
#define SNS       2
#define ISIS      3
#define CSNS      4
#define HBS       5
#define ILL      10
#define HMI      11

// Type of source
#define NO_TYPE   0
#define CWS       1
#define SPSS      2
#define LPSS      3

#define POISONED   1   /* moderator decoupled poisoned        */
#define DECOUPLED  2   /* moderator decoupled unpoisoned      */
#define COUPLED    3   /* moderator coupled                   */
#define MULT_SPEC  4   /* effective spectrum of a moderator consisting of a cold and thermal part      */

#define NUM_MOD    5   /* max. number of moderators in the moderator system */

#define FNL      101

typedef enum
{
  NO_SRC_KIND     =0,
  SRC_SIMPLE      =1,
  SRC_CWS         =2,
  SRC_PULSED      =3,
  SRC_ISIS        =4,
  SRC_ESS         =5
}
SrcKind;

typedef enum
{
  NO_VERSION      =0,
  MEZEI_2001      =1,
  ZANINI_2012     =2,
  SCHOENFELDT_2013=3,
  VARHEIGHT_2013  =4,
  BUTTERFLY2_2015 =5,
  BUTTERFLY1_2016 =6
}
EssModVsn;


/***********************************************/
/* Structures                                  */
/***********************************************/
typedef struct
{	
  SrcKind eSrcKind;       //             enum: source kind: uses 'SrcKind' (s.a.) to define the source type
  short   eSrcType;       //             enum: source type: CWS SPSS LPSS
  const char *pSrcName;   //             name of the source   
  short   nSource;        //             enum defining the sourc    
  double  PulseFreq;      //    [Hz]     repetition rate of the pulses in
  double  PulsePeriod;    //    [ms]     period of pulse cycle         in
  double  PulseLength;    //    [s]      LPSS pulse length             in
  double  Power;          //    [W]      average power of the source   in
}                         
Source;                   
                          
typedef struct            
{                         
  double ModTemp;         //     [K]     effective moderator temperature  
  short  eModType;        //     [-]     moderator type 
  short  eIsisTS;	        //             Target station 0: no ISIS moderator, 1: TS1, 2: TS2 
  short  nBackground;     //     [-]     index: order of moderators: higher number is in background
  short  nColour;         //     [-]     neutrons leaving this moderator get this colour
  char   bCircle;         //     [-]     flag: circular moderator
  double CntrX;           //     [cm]    x-component of the center fo the moderator
  double CntrY;           //     [cm]    y-component of the center fo the moderator           
  double CntrZ;           //     [cm]    y-component of the center fo the moderator
  double Diameter;        //     [cm]    diameter of the moderator
  double Width;           //     [cm]    width of the moderator 
  double Height;          //     [cm]    height of the moderator
  double Area;            //    [cm²]   area of the moderator*/
  double DistModWnd;      //     [cm]    distance moderator - propagation window 
  double WndFact;         //             factor to normalise divergence distribution defined 'by window'
  double PfmcFact;        //             performance factor considering losses by the technical realization
  char   sLFileName[FNL];
  char   sTFileName[FNL];
  char   sLTFileName[FNL];
  double TauAscMod;       //     [s]     ascent time constant of the moderated neutrons in the pulse
  double TauDecMod;       //     [s]     decay time constant of the moderated neutrons in the pulse 
                              
  double Chi;             //   [1/Ang]   factor for the wavelength dependence of under-moderated neutrons 
  double Kappa;           //     [1]     scaling factor for the flux of under-moderated neutrons
  double TauAscUM;        //     [s]     ascent time constant of the under-moderated neutrons in the pulse
  double TauDecUM;        //     [s]     decay time constant of the under-moderated neutrons in the pulse
                               
  double Current;         //    [n/s]    mean neutron current leaving the moderator
  double TotFluxMod,      //  [n/cm²/s]  total CW-flux of the moderated neutrons on the moderator surface    
         TotFluxUM;       //  [n/cm²/s]  total CW-flux of the under-moderated neutrons on the moderator surface
                          //             flux amplitude for moderated und undermoderated neutrons
  double FUAmpMod,        // [n/s/cm²/sr] - continuous sources: number of neutrons per area per solid angle per second 
         FUAmpUM;         //  [n/cm²/sr]  - pulsed sources:     number of neutrons per area per solid angle per pulse 
  double NormTrj,         // [cm²sr Ang] Phase space volume per trajectory 
         NormInt;         //  [n*Ang/s]   mean neutron current per traj. normalized by wavelength
                          //   [n*Ang]     and for pulsed sources by time interval   (= FUAmp*NormTrj)     
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
void   InitSource     (Source*    pSrc);
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

