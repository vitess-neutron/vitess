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
{	short  eSrcType;       /* source type: CWS SPSS LPSS */
	const char *pSrcName;
	short  nSource;
	double dPulseFreq;     /* repetition rate of the pulses in Hz */
	double dPulsePeriod;   /* period of pulse cycle         in ms */
	double dPulseLength;   /* LPSS pulse length             in s  */
	double dPower;         /* average power of the source   in W  */
}
Source;

typedef struct {
  double dModTemp;
  short  nBackground;
  short  nColour;
  char   bCircle;
  double dCntrX;
  double dCntrY;
  double dCntrZ;
  double dWidth;
  double dHeight;
  double dDiameter;
  double dArea;         /* area of the moderator [cm²] */
  double dDistModWnd;   /* distance moderator - propagation window  */
  double dWndFact;      /* factor to normalise divergence distribution defined 'by window' */
  double dTotalFlux;    /* total CW-flux on the moderator surface [n/cm²s]  */
  double dCurrent;      /* mean neutron current leaving the moderator [n/s] */
  double dNorm;         /* mean neutron current per traj. normalized by wavelength [n*Ang/s]
			   (and for spallation sources) by time interval         [n*Ang]   */
  double dPfmcFact;     /* performance factor considering losses by the technical realization */
  char   sLFileName[FNL];
  char   sTFileName[FNL];
  char   sLTFileName[FNL];
  short  eModType;
  double dTauDecay;        /* decay time constant of pulse in s  */
  double dTauAscent;       /* ascent time constant of pulse in s */
  double dFUAmpl;          /* pulse ampl. on the moderator surface   [n/(cm²*sterad)]
			      (number of neutrons per area per solid angle per pulse )     */
  short eIsisTS;	   /* Target station 0: no ISIS moderator, 1: TS1, 2: TS2 */
}
Moderator;

typedef struct
{
	double dF001;
	double dF002;
	double dF003;
	double dTemp;
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
{	double dLambdaMin;
	double dLambdaMax;
	double dMaxDivY;
	double dMaxDivZ;
	double dTimeFrmMin;
	double dTimeFrmMax;
}
TrajParam;

typedef struct
{	double* pTabX;
	double* pTabY;
	double* pTabF;
	long    nLines;
	long    nColumns;
	double  dInt;
	double (*pDisFct)();
}
FctTable;


/***********************************************/
/* Prototypes                                  */
/***********************************************/

long   IndLT        (const long i, const long j);
double TotalFU      (const double Temp,    const short  eSource,  const short  eModType,
					           const double dPower,  const double dPeriod,  const double dPulseLen);
double EssModFU     (const double Lambda,  const double dTime,    const double dLength);

double Maxwellian   (const double Lambda,  const double dModTemp);
double LeakageFct   (const double Lambda,  const ModInfo* pInfo);
double NotMaxwell   (const double Lambda,  const double alpha,   const double kappa);
short  GetEssModDat (ModInfo*     ModInfo, const double ModTemp, const double ModHeight, const short iVsn);

double UserLambdaDis(const double dLambda, const double dModTemp);
double UserLmbdTimeDis(const double dLambda, const double dTime);
double UserTimeDis  (const double dTime, const double dTauDecay, const double dTauRatio, const double dPLength);

double PulseShapeP  (const double dTime, const double dTauDecay, const double dTauRatio, const double dPLength);
double PulseIntEss  (const double dTime, const double dTauDecay, const double dTauRatio, const double dPLength);
double PulseShape   (const double dTime, const double dTauDecay, const double dTauRatio);
double PulseShapeInt(const double dTime, const double dTauDecay, const double dTauAscent);

double f_cold       (const double dLambda);
double f_therm      (const double dLambda);

double AveSolidAngleC(const double dModDiam,
                      const double dWndWidth, const double dWndHeight, const double dDist);
double AveSolidAngleR(const double dModWidth, const double dModHeight,
                      const double dWndWidth, const double dWndHeight, const double dDist);
double AveWeightC    (const double dModCntrY, const double dModCntrZ,  const double dModDiam,
                      const double dWndWidth, const double dWndHeight, const double dDist);
double AveWeightR    (const double dModCntrY, const double dModCntrZ,
                      const double dModWidth, const double dModHeight,
                      const double dWndWidth, const double dWndHeight, const double dDist);
double IntAtan       (const double dIntAnf,   const double dIntEnd,    const double dParam);
double WeightDirByWnd(const double dWndWidth, const double dDist, const double dModPos);

#endif

