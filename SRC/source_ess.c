/********************************************************************************************/
/*  VITESS module 'source_ess.c'                                                            */
/*    Functions to simulate the 2015 moderator description of the ESS                       */
/*    from T. Schoenfeldt                                                                   */
/*    see Schoenfeldt et al, "Phenomenological Study of Expected Cold and Thermal           */
/*    Brightness Phase-Space at ESS", in preparation                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/********************************************************************************************/
#include "init.h"
#include "general.h"
#include "src_modchar.h"
#include "message.h"
#include "source_ess.h"

#define LMBD_H_F  0.9045
#define LMBD_F_S  2.0224

#define PORT_SEP  6.0         // angular separation of the beamports

#define MOD_THML  1
#define MOD_COLD  2

#define NEUT_THML 1
#define NEUT_SLOW 2
#define NEUT_HOT  3

#define FLUX_F_THML  2.489E13
#define FLUX_S_THML  0.637E13
#define FLUX_F_COLD  0.817E13
#define FLUX_S_COLD  5.005E13


typedef struct
{
  long    nLines;
  double* pTabX;
  double* pTabFT;
  double* pTabFC;
  double  FluxIntF1, FluxIntS1,  // flux values at negative hor. positions, F: thermal, S:cold phys. moderator
          FluxIntF2, FluxIntS2;  // flux values at positive hor. positions, F: thermal, S:cold p phys. moderator
}
FctTableM;

FctTableM  stFluxHor;                         // data of horizontal flux distribution
char       sPortID[4];                        // ID of the beamport, e.g. "N2","S11", ...  
double     ModShift=0.0,                      // Position of the border between moderators (as shown in ESS documents)
           kappa1_thml=0.0, kappa2_thml=1.0,  // fraction of thermal and cold spectrum in moderator
           kappa1_cold=1.0, kappa2_cold=0.0;  // spectrum 1 (pos < ModShift) and spectrum 2 (pos > ModShift)

extern Source stSrc;
extern char*  pBeamline;
extern double Declination,
              PortAngle;

// prototypes
// ----------
static double TSC2015_ParaSpectra_BF3cm   (const double lambda, const double dev90);
static double TSC2015_ThermalSpectra_BF3cm(const double lambda, const double dev90);
static double TSC_TimeDist_Final_Thermal  (double time, double lambda, double height, double pulse_len);
static double TSC_TimeDist_Final_Cold     (double time, double lambda, double height, double pulse_len);

static double TSC2016_coldx0_BF3cm(const double x0, const double lambda);
static double TSC2015_coldx0_BF3cm(const double x0, const double dev90);
static double TSC2015_coldy0_BF3cm(const double y0);
static double TSC2015_coldy0_BF6cm(const double y0);

static double TSC2016_thmlx0_BF3cm(const double x0, const double lambda);
static double TSC2015_thmlx0_BF3cm(const double x0, const double dev90);
static double TSC2016_thmly0_BF3cm(const double y0);
static double TSC2015_thmly0_BF3cm(const double y0);
static double TSC2015_thmly0_BF6cm(const double y0);

static double HorFlux (const double y, short kMod);
static double NeutrYieldNorm(const double Power, const double Voltage);

extern short  iDataVsn;          /* version of the data base for the source characteristics */

/**************************************************************/
/*  static functions                                          */
/**************************************************************/
double EssTotFU2015(const double ModHeight, const double ModTemp, const double Power, const double Freq,
                    const double PulseLen)
/* ModHeight : [cm] moderator height
   ModTemp   : [K]  moderator temperature
   Power     : [W]  average source power
   Freq      : [Hz] pulse frequency
   PulseLen  : [s]  pulse length                             */
{
  double FUAmpl= 0.0,
         Epulse,           // [J] energy of 1 pulse
         U0    = 2.5e9,    // [V] accelerator voltage: 2.5 GV
         CurrMax;          // [A] max. current for this set-up
  char   sBuffer[256];

  Epulse = Power / Freq;

  /* maximal accelerator current */
  CurrMax   = Epulse / PulseLen / U0;
  if (CurrMax > 0.05001)
  {  sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of 50 mA", 1000.0*CurrMax);
    Warning(sBuffer);
  }

  if ( (ModHeight > 2.9 && ModHeight < 3.1) || (ModHeight > 5.9 && ModHeight < 6.1) ){
    // integral of TSC2015_[Para/Thermal]Spectra_BF3cm, mean of dev90 5-55 deg:
    if(fabs(ModTemp-50)<0.1)
      FUAmpl = 8.11e13/Freq ; // cold   16.0e14/25=6.4e13
    else
      FUAmpl = 5.85e13/Freq; // thermal  9.0e14/25=3.6e13
  }
  else if (ModHeight > 5.9 && ModHeight < 6.1){
    if(fabs(ModTemp-50)<0.1)
      FUAmpl = 8.11e13/Freq*0.631;
    else
      FUAmpl = 5.85e13/Freq*0.689;
  }
  else
    {
      Error("source_ess.c: ESS moderator data 2015 only implemented for 3 cm and 6 cm moderator height");
    }

  FUAmpl *= 0.75 * NeutrYieldNorm(Power, stSrc.Voltage);  //  engineering factor and scaling to an average power of 5 MW and 2.5 GeV proton energy

  return(FUAmpl);
}

double EssTotFU2016(const double ModHeight, const double ModTemp,  const double Power, const double Freq,
                    const double PulseLen,  const double PfmcThml, const double PfmcCold)
/* ModHeight : [cm] moderator height
   ModTemp   : [K]  moderator temperature
   Power     : [W]  average source power
   Freq      : [Hz] pulse frequency
   PulseLen  : [s]  pulse length                             */
{
  double FUAmpl= 0.0,
         // DutyCycle=0.04,
         // U0    = 2.5e9,    // [V] accelerator voltage: 2.5 GV
         Epulse =0.0,           // [J] energy of 1 pulse
         CurrMax=0.0;          // [A] max. current for this set-up
  char   sBuffer[256];


  /* maximal accelerator current */
  if (PulseLen > 0.0 && stSrc.Voltage > 0.0)
  {
    Epulse  = Power / Freq;
    CurrMax = Epulse / PulseLen / stSrc.Voltage;
    if (CurrMax > 0.05001)
    {  sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of 50 mA", 1000.0*CurrMax);
      Warning(sBuffer);
    }
  }

  if (ModHeight > 0.0)
  {
    // integral of TSC2015_[Para/Thermal]Spectra_BF3cm, mean of alpha 30 - 150 deg or 210 - 330 deg:
    if (ModTemp < 100.0)
      FUAmpl = 16.0e14 * stSrc.DutyCycle / Freq * 0.816 * PfmcCold * NeutrYieldNorm(Power, stSrc.Voltage); // cold
    else
      FUAmpl =  9.0e14 * stSrc.DutyCycle / Freq * 0.732 * PfmcThml * NeutrYieldNorm(Power, stSrc.Voltage); // thermal
  }
  else
  {
    Error("source_ess.c: moderator height must be positive");
  }

  return(FUAmpl);
}


double EssModFU_Butterfly2015(const double ModHeight, const double Power,    const double Freq, const double Declination, const Neutron* pNeutron,
                              const double PulseLen,  const double PfmcThml, const double PfmcCold)
/* ModHeight  : [cm]  moderator height
   Power      : [W]   average source power
   Declination: [deg] pulse frequency
   pNeutrom   : [s]   Pointer to data of the neutron trajectory   */
{
  double brightness=0.0,    // [n/(cm²s str Ang)]  brightness of neutron beam for given parameters
         lambda,            // [Ang] neutron wavelength
         dev90,             // [deg] deviation from moderator center = 90° relative proton beam
                            //       possible values 5°, 15°, 25°, 35°, 45° and 55°
         x0,                //  [m]  horizontal starting position on moderator (for cold and thermal moderator)
         y0,                //  [m]  vertical starting position on moderator
         time;              //  [s]  starting time at moderator

  // calculation of (McStas) parameters used in the analytical functions
  lambda = pNeutron->Wavelength;
  dev90  = CalcDev90(PortAngle); 
  x0     = pNeutron->Position[1]; // [cm]
  y0     = pNeutron->Position[2]; // [cm]
  time   = pNeutron->Time/1000.0;

  // mirror x0 to match weird coordinate system in functions
  if(dev90 > 0.0)
    x0 *= -1.0;
  // dev90=0: only place from which one can see both cold moderators
  if (fabs(dev90) < 0.01 && x0 > 0)
    x0 *= -1.0;

  if (fabs(dev90) > 55.0 && iDataVsn == BUTTERFLY2_2015)
    Error("source_ess.c: ESS moderator data 2015 only implemented for declination |dev90| <= 55 deg");

  if (ModHeight > 2.9 && ModHeight < 3.1)
  {
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(dev90))
                  *TSC2015_coldy0_BF3cm(y0)
                  *TSC2015_coldx0_BF3cm(x0, fabs(dev90))
                  *TSC_TimeDist_Final_Cold(time,lambda,ModHeight, PulseLen)
                  *PfmcCold                                               // loss due to engineering details not modeled
                  +
                  TSC2015_ThermalSpectra_BF3cm(lambda, fabs(dev90))
                  *TSC2015_thmly0_BF3cm(y0)
                  *TSC2015_thmlx0_BF3cm(x0, fabs(dev90))
                  *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight, PulseLen)
                  *PfmcThml );                                            // loss due to engineering details not modeled
  }
  else if (ModHeight > 5.9 && ModHeight < 6.1)
  {
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(dev90)) * 0.631 // apply factor BF6cm/BF3cm
       *TSC2015_coldy0_BF6cm(y0)                                          // parameters calc for 6cm
       *TSC2015_coldx0_BF3cm(x0, fabs(dev90))                             // same as 3cm
       *TSC_TimeDist_Final_Cold(time,lambda,ModHeight, PulseLen)          // same as 3cm
       *PfmcCold
           +
       TSC2015_ThermalSpectra_BF3cm(lambda, fabs(dev90)) * 0.689
       *TSC2015_thmly0_BF6cm(y0)
       *TSC2015_thmlx0_BF3cm(x0, fabs(dev90))
       *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight, PulseLen)
           *PfmcThml );
  }
  else
  {
    Error("source_ess.c: ESS moderator data 2015 only implemented for 3 cm and 6 cm moderator height");
  }

  brightness *= NeutrYieldNorm(Power, stSrc.Voltage) / Freq;  // scaling to an average power of 5 MW and 2.5 GeV proton energy

  return(brightness);
}

double EssModFU_Butterfly2016(const double ModTemp,   const double Power,    const double Freq, const double Declination, const Neutron* pNeutron,
                              const double PulseLen,  const double PfmcThml, const double PfmcCold)
/* ModTemp    : [cm]  moderator temperature
   Power      : [W]   average source power
   Freq       : [Hz]  pulse frequency
   Declination: [deg] deviation of beamline direction from moderator surface normal
   pNeutron   : [s]   Pointer to data of the neutron trajectory   */
{
  double brightness=0.0,        // [n/(cm²s str Ang)]  brightness of neutron beam for given parameters
         lambda,                // [Ang] neutron wavelength
         dev90,                 // [deg] deviation from moderator normal = 90° relative proton beam
                                //       possible values -60°, -54°, -48°, ... 60°
         x0,                    //  [m]  horizontal starting position on moderator
         y0,                    //  [m]  vertical starting position on moderator
         fxc=0.0, fxt=0.0,      //       factors defining horizontal intensity distribution
         fyc=0.0, fyt=0.0,      //       factors defining vertical intensity distribution
         kappa_thml, kappa_cold,//       fraction of thermal and cold spectrum in moderator
         time;                  //  [s]  starting time at moderator

  // calculation of (McStas) parameters used in the analytical functions
  lambda = pNeutron->Wavelength;
  dev90  = CalcDev90(PortAngle); 
  x0     = pNeutron->Position[1]; // [cm]
  y0     = pNeutron->Position[2]; // [cm]
  time   = pNeutron->Time/1000.0; // [s]

  if (fabs(dev90) > 60.0)
    Error("source_ess.c: ESS moderator data 2016 only implemented for declination |dev90| <= 60 deg");

  // set fractions of cold and thermal spectrum for the given moderator
  if (x0 < ModShift)
  { kappa_cold = kappa1_cold;
    kappa_thml = kappa1_thml;
  }
  else
  { kappa_cold = kappa2_cold;
    kappa_thml = kappa2_thml;
  }

 #ifdef _TEST
  if (ModTemp < 100.0) {fxc=1.0; fxt=0.0;}
  else                 {fxc=0.0; fxt=1.0;}
 #else
  if (kappa_cold > 0.0) fxc = TSC2016_coldx0_BF3cm(x0, lambda);  // factors considering the flux variation along the horizontal position
  if (kappa_thml > 0.0) fxt = TSC2016_thmlx0_BF3cm(x0, lambda);
 #endif
  if (kappa_cold > 0.0) fyc = TSC2015_coldy0_BF3cm(y0);          // factors condidering the drop in flux at the upper and lower end of the moderator
  if (kappa_thml > 0.0) fyt = TSC2016_thmly0_BF3cm(y0);
  brightness =  kappa_cold
              * TSC2015_ParaSpectra_BF3cm(lambda, fabs(dev90))
              * TSC_TimeDist_Final_Cold(time,lambda, 3.0, PulseLen)
              * fxc * fyc * 0.816 * PfmcCold                            // loss due to engineering details not modeled
             +  kappa_thml
              * TSC2015_ThermalSpectra_BF3cm(lambda, fabs(dev90))
              * TSC_TimeDist_Final_Thermal(time,lambda, 3.0, PulseLen)
              * fxt * fyt * 0.732 * PfmcThml ;                          // loss due to engineering details not modeled

  brightness *= NeutrYieldNorm(Power, stSrc.Voltage) / Freq;  // scaling to an average power of 5 MWand 2.5 GeV proton energy

  return(brightness);
}


/*************************************************************************************************/
/* GetModWidth_ESSbutterfly2016                                                                  */
/*  Input : beamline deviation from 90 deg, moderator temperature                                */
/*  Return: width of cold or thermal moderator                                                   */
/* calculated for |dev90| = 5, 15,... 55 deg, extrapolate in between                             */
/*************************************************************************************************/
double GetModWidth_ESSbutterfly2015(const double Dev90, double const ModTemp)
{
  double width = 0.0,
         Width[6][2]={ {7.1,14.2}, {8.0,14.1}, {8.0,14.2}, {7.9,14.3}, {6.9,15.6}, {7.0,16.0} };  // {cold, thml}
  int   i=1;
  short iMod = (fabs(ModTemp-50)<0.1) ? 0 : 1;

  if (Dev90 <= 5.0)
  {
    width = Width[0][iMod];
  }
  else if (Dev90 >= 55.0)
  {
    width = Width[5][iMod];
  }
  else
  {
    for(i=1; i < 6; i++)
    {
      if (Dev90 <= 5.0 + 10.0*i)
      {
        width = Width[i][iMod] - (Width[i][iMod]-Width[i-1][iMod]) / 10.0 * (5.0 + 10.0*i - Dev90);
        break;
      }
    }
  }
  return width;
}


/*************************************************************************************************/
/* GetModWidth_ESSbutterfly2016                                                                  */
/* GetShift_ESSbutterfly2016                                                                     */
/*   return width of the ESS Butterfly-1 moderators or their crossover position position         */
/*   (using the values taken from the ESS document 'BFpaper_LZ_ESS-0068256.pdf'                  */
/* Input : beamline deviation from 90 deg (and moderator temperature)                            */
/* Return: width of cold or thermal moderator or position where they adjoin resp.                */
/*************************************************************************************************/
double GetModWidth_ESSbutterfly2016(const double Dev90, const double ModTemp)
{
 //          Beamport      1    2     3    4    5    6    7     8     9    10    11
  double WidthThml[11] = {5.8, 6.80, 7.7, 7.7, 8.5, 9.1, 9.6, 10.0, 10.3, 10.5, 10.5},  // eff. widths thermal
         WidthCold[11] = {5.0, 7.25, 7.6, 8.5, 8.7, 8.8, 8.8,  8.7,  8.6,  8.3,  6.9},  // eff. widths cold
         delta = PORT_SEP,   // distance in deg between neighboring beamports
         DTheta,             // distance in deg to j-th standard beamline direction
         width=0.0,
         frac, kTheta, bin;
  char   cOrient=' ';          // character defining the guide hall ('S', 'E', 'W', 'N')    
  int    iBL1, iBL2;                  // number of the beamline (= index_of_the_tabulated_value + 1)

  // determine beamline and orientation
  // frac   = modf(bin, &kTheta);
  // DTheta = Dev90 - delta*kTheta;
  // iBL    = 11 - Round(kTheta);
  bin     = fabs(Dev90) / delta;
  DTheta  = Dev90 - delta*floor(bin);
  iBL2    = 11 - (int) ceil (bin);
  iBL1    = 11 - (int) floor(bin);

  if (ModTemp < 100.0)
    width = WidthCold[iBL1-1] + (WidthCold[iBL2-1] - WidthCold[iBL1-1])/delta * DTheta;
  else
    width = WidthThml[iBL1-1] + (WidthThml[iBL2-1] - WidthThml[iBL1-1])/delta * DTheta;

  return width;
}

double GetShift_ESSbutterfly2016(void)
{
 //          Beamport    1      2      3     4    5    6    7    8    9   10   11
  double aShift[11] = {-0.21, -0.48, -0.64, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         shift=0.0;                         // eff. shift of the cross over point between cold and thermal moderator
  int    iBL = 0;
  char   cOrient=' ';

  sscanf(pBeamline, "%c%hd", &cOrient, &iBL);
  shift = aShift[iBL-1];

  return shift;
}


short GetColour_ESSbutterfly2015(double x0, const double Dev90)
{
  // mirror x0 to match weird coordinate system in functions
  if (Dev90 > 0.0)
    x0 *= -1;
  // Dev90=0: only place from which one can see both cold moderators
  if (fabs(Dev90) < 0.01 && x0 > 0.0)
    x0 *= -1;

  if( TSC2015_coldx0_BF3cm(x0, fabs(Dev90)) > TSC2015_thmlx0_BF3cm(x0, fabs(Dev90)) )
    return 2;
  else
    return 1;
}


short GetColour_ESSbutterfly2016(const double lambda)
{
  short colour=0;

  if      (lambda > LMBD_F_S) colour=NEUT_SLOW;  // cold
  else if (lambda < LMBD_H_F) colour=NEUT_HOT;   // hot
  else                        colour=NEUT_THML;  // thermal

  return colour;
}


/*****************************************************************/
/* load horizontal intensity distribution from file, fills       */
/*  corresponding arrays, integrates and calculates average flux */
/* Input:  sID: ID of the ESS beamline, e.g. "S3"              */
/*****************************************************************/
void LoadHorDistrib(const char* sBeamID)
{
  long   i;
  double DelY=0.0,
         Y1min, Y1max, Y2min, Y2max,
         YR, YL, FTR, FTL, FCR, FCL,
         Dev90=0.0;
  char   sFileName[20],
         sBuffer[CHAR_BUF_LENGTH]="";
  FILE*  pDisFile=NULL;
  short  rc=FALSE,
         iBL=0;
  char   cOrient=' ';

  // check input
  if (sBeamID==NULL  || strlen(sBeamID) <= 0)
    Error("Beamline ID missing");
  
  sscanf(sBeamID, "%c%hd", &cOrient, &iBL);

  // loading file containing horizontal flux distribution
  if ((cOrient=='S' || cOrient=='N' || cOrient=='E' || cOrient=='W') && iBL > 0 && iBL < 12)
  {
    // Determine range of cold and thermal moderator
    Dev90=CalcDev90(CalcAlpha(sBeamID));
    double Width1, Width2,
           CosDecl= cos(Dev90*M_PI/180.0);
    ModShift  = GetShift_ESSbutterfly2016();
    if (Dev90 <= 0.00001)
    { Width1 = GetModWidth_ESSbutterfly2016(fabs(Dev90),  50.0);
      Width2 = GetModWidth_ESSbutterfly2016(fabs(Dev90), 325.0);
      kappa1_thml = 0.0;
      kappa1_cold = 1.0;
      kappa2_thml = 1.0;
      kappa2_cold = 0.0;
    }
    else
    { Width1 = GetModWidth_ESSbutterfly2016(fabs(Dev90), 325.0);
      Width2 = GetModWidth_ESSbutterfly2016(fabs(Dev90),  50.0);
      kappa1_thml = 1.0;
      kappa1_cold = 0.0;
      kappa2_thml = 0.0;
      kappa2_cold = 1.0;
    }

    Y1min = ModShift - Width1; Y1max = ModShift;
    Y2min = ModShift;          Y2max = ModShift + Width2;

    /* opening distribution file, either from the input directory or from the installation directory */
    sprintf(sFileName, "ESS2016_%s.dat", sBeamID);
    pDisFile = OpenParameterFile(sFileName, FALSE, "rt");
    if (pDisFile==NULL)
      pDisFile = OpenPackInpFile(sFileName, "FILES/moderators/ESS/", FALSE);
    if (pDisFile!=NULL)
    {
      /* reading number of lines, allocating memory and reading distribution files */
      stFluxHor.nLines = LinesInFile(pDisFile);
      stFluxHor.pTabX  = (double*) calloc(stFluxHor.nLines, sizeof(double));
      stFluxHor.pTabFT = (double*) calloc(stFluxHor.nLines, sizeof(double));
      stFluxHor.pTabFC = (double*) calloc(stFluxHor.nLines, sizeof(double));

      for(i=0; i < stFluxHor.nLines; i++)
      {
        ReadLine(pDisFile, sBuffer, sizeof(sBuffer)-1);
        sscanf  (sBuffer, "%lf %le %le", &stFluxHor.pTabX[i], &stFluxHor.pTabFT[i], &stFluxHor.pTabFC[i]);
      }

      /* integrating flux distribution */
      stFluxHor.FluxIntF1 = 0.0; stFluxHor.FluxIntS1 = 0.0;   // cf. comments in structure definition
      stFluxHor.FluxIntF2 = 0.0; stFluxHor.FluxIntS2 = 0.0;

      for (i=1; i < stFluxHor.nLines; i++)
      {
        if (stFluxHor.pTabX[i] > Y1min && stFluxHor.pTabX[i-1] < Y1max)
        {
          YR   =  Min(stFluxHor.pTabX [i],   Y1max);
          YL   =  Max(stFluxHor.pTabX [i-1], Y1min);
          DelY =  YR -YL;
          FTR  = HorFlux(YR, MOD_THML);
          FTL  = HorFlux(YL, MOD_THML);
          FCR  = HorFlux(YR, MOD_COLD);
          FCL  = HorFlux(YL, MOD_COLD);
          stFluxHor.FluxIntF1 += 0.5*(FTR+FTL)*DelY;
          stFluxHor.FluxIntS1 += 0.5*(FCR+FCL)*DelY;
        }
        if (stFluxHor.pTabX[i] > Y2min && stFluxHor.pTabX[i-1] < Y2max)
        {
          YR   =  Min(stFluxHor.pTabX [i],   Y2max);
          YL   =  Max(stFluxHor.pTabX [i-1], Y2min);
          DelY =  YR -YL;
          FTR  = HorFlux(YR, MOD_THML);
          FTL  = HorFlux(YL, MOD_THML);
          FCR  = HorFlux(YR, MOD_COLD);
          FCL  = HorFlux(YL, MOD_COLD);
          stFluxHor.FluxIntF2 += 0.5*(FTR+FTL)*DelY;
          stFluxHor.FluxIntS2 += 0.5*(FCR+FCL)*DelY;
        }
      }
      stFluxHor.FluxIntF1 /= Width1; stFluxHor.FluxIntS1 /= Width1;
      stFluxHor.FluxIntF2 /= Width2; stFluxHor.FluxIntS2 /= Width2;

      /* closes distribution file */
      fclose(pDisFile) ;

      /* if moderators show mixed spectrum
      kappa1_thml = (stFluxHor.FluxIntF1 * FLUX_S_COLD - stFluxHor.FluxIntS1 *FLUX_F_COLD) / (FLUX_F_THML * FLUX_S_COLD - FLUX_S_THML * FLUX_F_COLD);
      kappa1_cold = (stFluxHor.FluxIntF1 - kappa1_thml * FLUX_F_THML) / FLUX_F_COLD;
      kappa2_thml = (stFluxHor.FluxIntF2 * FLUX_S_COLD - stFluxHor.FluxIntS2 *FLUX_F_COLD) / (FLUX_F_THML * FLUX_S_COLD - FLUX_S_THML * FLUX_F_COLD);
      kappa2_cold = (stFluxHor.FluxIntF2 - kappa2_thml * FLUX_F_THML) / FLUX_F_COLD;
      */
    }
    else
    { Error2("Can't open file to read horizontal intensity distribution, neither in the input directory nor in 'InstallDir/FILES/moderators/ESS'\n",
                         sFileName);
    }
  }
  else
  { Error2("You have to give a valid beam port!\n", sBeamID);
  }
}


/*****************************************************************/
/* AnalyzeDecl                                                   */
/* Change old declination angle to real declination and beamport */
/* Input :  angle                                                */
/* Update: 'pBeamline', 'Declination'                            */
/*****************************************************************/
void AnalyzeDecl(double Angle)
{
  double alpha=0.0,  // stanard beamport orientation (according to ESS definition 0 ... 360 deg)
         dev90=0.0;  // deviation from 90° orientation

  // an angle larger than half the distance between neighboring beamports indicates that it is meant
  // as the deviation from 90° to define the beamport (and not the declination)
  if (fabs(Angle) > 0.5*PORT_SEP)
  {
    GenerPortFromDev90(Angle);

    // take this as the beamline or check for consistency
    if (pBeamline==NULL)
    { pBeamline = sPortID;
    }
    else
    { if (strcmp(pBeamline, sPortID)!=0)
        Error2("AnalyzeDecl: Given declination corresponds to a different beamport than the given one", pBeamline);
    }

    // subtract the nominal orientation to determine the declination 
    alpha = CalcAlpha(pBeamline);
    dev90 = CalcDev90(alpha);
    Declination -= dev90;
  }
}

/*****************************************************************/
/* CalcPortAngle                                                 */
/* Calculate real orientation of the ESS beamport (0 - 360 deg)  */
/*****************************************************************/
void CalcPortAngle()
{
  double alpha=0.0;  // standard beamport orientation (according to ESS definition 0 ... 360 deg)
         
  if (pBeamline==NULL)
  { Error("CalcPortAngle: Beamline ID missing");
  }
  else
  { alpha     = CalcAlpha(pBeamline);
    PortAngle = alpha + Declination;  // real beamport orientation
  }
}

/*****************************************************************/
/* Generation of the beamport ID                                 */
/* GenerPortFromAlpha: from the ESS beamport orientation 'alpha' */
/* GenerPortFromDev90: from the deviation from 90° (only E and S */
/*****************************************************************/
void GenerPortFromAlpha(const double Alpha)
{
  short  iPort=0;        // number of the beamport 1 .. 11
  char   sOrient =' ';
  double delta=PORT_SEP, // distance between neighboring beamports
         alpha=0.0,      // beamport orientation (according to ESS definition 0 ... 360°)
         dev90=0.0;      // deviation from 90° orientation

  if (Alpha > 360.0 || Alpha < 0.0)
    Error("Beamport orientation out of range 0 - 360 deg");

  dev90 = CalcDev90(Alpha);
  iPort = 11 - (int) Round(fabs(dev90) / delta); 

  if      (alpha >= 270.0) sOrient='W';
  else if (alpha >= 180.0) sOrient='N';
  else if (alpha >=  90.0) sOrient='E';
  else                     sOrient='S';

  sprintf(sPortID, "%c%d", sOrient, iPort);

  return;
}

void GenerPortFromDev90(const double Dev90)
{
  char   sOrient=' ';
  short  iPort  =0;         // number of the beamport 1 .. 11
  double delta  =PORT_SEP; // distance between neighboring beamports       

  if (Dev90 > 60.0 || Dev90 < -60.0)
    Error("GenerPortFromDev90: Beamport orientation out of range -60 - +60 deg");

  iPort = 11 - (int) Round(fabs(Dev90) / delta); 

  if (Dev90 > 0.0) sOrient='E';
  else             sOrient='S';

  sprintf(sPortID, "%c%d", sOrient, iPort);

  return;
}

/*****************************************************************/
/* CalcAlpha: beamport ID -> ESS beamport orientation 'alpha'    */
/* CalcDev90: beamport orientation 'alpha' -> deviation from 90° */
/*****************************************************************/
double CalcAlpha(const char* sID)
{
  double alpha=0.0,      // beamport orientation (according to ESS definition 0 ... 360°)
         delta=PORT_SEP, // distance between neighboring beamports
         Delta=0.0;      // distance from beamport to 90 deg orientation
  short  iBL=0;
  char   cOrient=' ';

  if (sID==NULL)
    Error("CalcAlpha: sID hat Null-Pointer");
  else if (strlen(sID) < 2 || strlen(sID) > 3)
    Error2("CalcAlpha: wrong length of the beamport ID (for the calculation of the beamline orientation)", sID);

  sscanf(sID, "%c%hd", &cOrient, &iBL);

  Delta = delta * (11 - iBL);

  switch (cOrient)
  { case 'W': alpha = 270.0 + Delta; break;
    case 'N': alpha = 270.0 - Delta; break;
    case 'E': alpha =  90.0 + Delta; break;
    case 'S': alpha =  90.0 - Delta; break;
    default : Error2("CalcAlpha: wrong character in the beamport ID (for the calculation of the beamline orientation", &cOrient);
  }
  return alpha;
}

double CalcDev90(const double Alpha)
{
  double dev90=0.0;   // deviation from 90° orientation

  if (Alpha > 180.0)
    dev90 = Alpha - 270.0;
  else
    dev90 = Alpha -  90.0;

  return dev90;
}


/**************************************************************/
/*  static functions                                          */
/**************************************************************/

double TSC2015_ParaSpectra_BF3cm(const double lambda, const double Dev90){

    double par0=8.44e13/25.;
    double par1=2.5;
    double par2=2.2;

    double par3=-13.-.5*(Dev90-5);
    double par4=2.53;
    double par5=-0.0478073-0.160*exp(-0.45186*(Dev90-5.)/10.);

    double par6=6.0e+015/25.;
    double par7=0.788956+0.00854184*(Dev90-5.)/10.;
    double par8=0.0461868-0.0016464*(Dev90-5.)/10.;
    double par9=0.325;

    double SD_part, para_part;

      //extrapolate linearly between fitted values
    if(Dev90<=5)
      par6=5.73745e+015/25.;
    else if(Dev90<=15)
      par6 = 5.88284e+015/25. - (5.88284e+015/25. - 5.73745e+015/25. )/10*(15-Dev90);
    else if(Dev90<=25)
      par6= 6.09573e+015/25. - (6.09573e+015/25.-5.88284e+015/25.)/10*(25-Dev90);
    else if(Dev90<=35)
      par6= 6.29116e+015/25. - (6.29116e+015/25.-6.09573e+015/25.)/10*(35-Dev90) ;
    else if(Dev90<=45)
      par6 = 6.03436e+015/25. - (6.03436e+015/25.-6.29116e+015/25.)/10*(45-Dev90);
    else if(Dev90<=55)
      par6 = 6.02045e+015/25. - (6.02045e+015/25.-6.03436e+015/25.)/10*(55-Dev90);

    SD_part=par0/((1+exp(par1*(lambda-par2)))*lambda);
    para_part=pow((1+exp(par3*(lambda-par4))),par5)*(par6*(exp(-par7*(lambda))+par8*exp(-par9*(lambda))));

    return para_part+SD_part;
}

double TSC2015_ThermalSpectra_BF3cm(const double lambda, const double Dev90){

    double i, par0, par2, par3, aOlsqr;

    if(lambda<=0)return 0;

    i=(Dev90-5.)/10.;
    par0=4.2906e+013-9.2758e+011*i+8.02603e+011*i*i-1.29523e+011*i*i*i;
    par2=6.24806e+012-8.84602e+010*i;
    par3=-0.31107+0.0221138*i;
    aOlsqr=949./(325*lambda*lambda);

    return par0*2.*aOlsqr*aOlsqr/lambda*pow(lambda,-par3)*exp(-aOlsqr)+par2/((1+exp(2.5*(lambda-0.88)))*lambda);
}


double TSC_TimeDist_Final_Thermal(double time,double lambda,double height, double pulse_len)
{
    double tau;

    if (time < 0.0) return 0;
    tau=3.00000e-004*(1.23048e-002*lambda*lambda+1.75628e-001*exp(-1.82452e-001*height)+9.27770e-001)*exp(-3.91090e+001*pow(Max(1e-13,lambda+0.987990),-7.65675));
    if (time < pulse_len) return 1/pulse_len*(1.0-exp(-time/tau));   // corrected exp() -> 1-exp()   (KL, 15.05.15)
    return 1/pulse_len*(1-exp(-pulse_len/tau))*exp(-(time-pulse_len)/tau);
}

double TSC_TimeDist_Final_Cold(double time,double lambda,double height, double pulse_len)
{
    double tau;

    if (time < 0.0) return 0;
    tau=3.00094e-004*(4.15681e-003*lambda*lambda+2.96212e-001*exp(-1.78408e-001*height)+7.77496e-001)*exp(-6.63537e+001*pow(Max(1e-13,lambda+0.9),-8.64455));
    if (time < pulse_len)return 1/pulse_len*(1.0-exp(-time/tau));   // corrected exp() -> 1-exp()   (KL, 15.05.15)
    return 1/pulse_len*(1-exp(-pulse_len/tau))*exp(-(time-pulse_len)/tau);
}


double TSC2016_coldx0_BF3cm(const double y, const double lambda)
{
  double F,f;
  short iCol=GetColour_ESSbutterfly2016(lambda);

  F = HorFlux(y, iCol);

  if (iCol==NEUT_SLOW)
    f = F/FLUX_S_COLD;
  else
    f = F/FLUX_F_COLD;

  return f;
}

double TSC2015_coldx0_BF3cm(const double x0, const double Dev90)
{
  double i, line, CutLeftCutRight;
  double par0, par1, par2, par3, par4, par5;

    i=(Dev90-5.)/10.;
    par0=0.0146115+0.00797729*i-0.00279541*i*i;
    par1=0.980886;
    //extrapolate linearly between fitted values
    if(i<=1)
      par1 = 0.974217;
    else if(i<=2)
      par1 = 0.981462 - (0.981462-0.974217)*(2-i);
    else if(i<=3)
      par1 = 1.01466 - (1.01466-0.981462)*(3-i);
    else if(i<=4)
      par1 = 1.11707 - (1.11707-1.01466)*(4-i);
    else if(i<=5)
      par1 = 1.16057 - (1.16057-1.11707)*(5-i);

    par2=-4-.75*i;
    if(i<=0)par2=-20;
    else if (i<1)
      par2 = -20 + (20-4-0.75*i)*(1-i);

    par3=-14.9402-0.178369*i+0.0367007*i*i;
    if(i<=0)par3=-14.27;

    par4=-15;
    if(i>=3)
      par4 = -1.9 - (3.5-1.9)/2*(5-i);
    else if (i>2)
      par4 = -3.5 - (15-3.5)*(3-i);

    par5=-7.07979+0.0835695*i-0.0546662*i*i;
    if(i==4)par5=-8.1;

    line=par0*(x0+12)+par1;
    CutLeftCutRight=1./((1+exp(par2*(x0-par3)))*(1+exp(-par4*(x0-par5))));

    return line*CutLeftCutRight;
}

double TSC2015_coldy0_BF3cm(const double y0){
    double par3=30;
    double par4=.35;
    long double cosh_ish=exp(-par4*y0)+exp(par4*y0);
    long double sinh_ish=pow(1+exp(par3*(y0-3./2.)),-1)*pow(1+exp(-par3*(y0+3./2.)),-1);
    return 1./2.*(double)((long double)cosh_ish*(long double)sinh_ish);
}
// bottom moderator: invert y0 since asymmetry comes from target on top instead of bottom,
// and adjust A->A/2 as well as alpha->alpha*2 according to 3cm re-fit difference (CZ, 29.05.2015)
double TSC2015_coldy0_BF6cm(const double y0){
  double par3=7.42571*2;
    double par4=0.295423;
    long double cosh_ish=exp(par4*y0)+0.822088*exp(-par4*y0);
    long double sinh_ish=pow(1+exp(-par3*(y0+6./2.)),-1)*pow(1+exp(par3*(y0-6./2.)),-1);
    return 1.02646/2*(double)((long double)cosh_ish*(long double)sinh_ish);
}


/**************************************************************/
/* considers the flux variation along the horizontal position */
/* Input:  x   : hor. position on the moderator               */
/* Return: factor (order of magnitude 1)                      */
/**************************************************************/
double TSC2016_thmlx0_BF3cm(const double x, const double lambda)
{
  double F,f;
  short iCol=GetColour_ESSbutterfly2016(lambda);

  F = HorFlux(x, iCol);

  if (iCol==NEUT_SLOW)
    f = F/FLUX_S_THML;
  else
    f = F/FLUX_F_THML;

  return f;
}

double TSC2015_thmlx0_BF3cm(const double x0, const double Dev90)
{
    double i, soften1, soften2, CutLeftCutRight, line1, line2, line3, add45degbumb;
    double par0, par1, par2, par3, par4, par5, par6, par7, par8, par9;

    i=(Dev90-5.)/10.;
    par0=-5.54775+0.492804*i;
    par1=-0.265929-0.711477*i;
    if(Dev90==55)par1=-2.55;

    par2=0.821885+0.00914832*i;
    par3=1.31108-0.00698647*i;
    if (Dev90==55) par3=1.23;
    par4=-.035;
    par5=-0.0817358+0.00807125*i;

    par6=-8;
    par7=-7.15;
    if(Dev90>35){
      if(Dev90<=45)
        par7 = - ( 8.2 -(8.2-7.15)/10*(45-Dev90) );
      else if(Dev90<=55)
        par7 = - ( 7.7 -(7.7-8.2)/10*(55-Dev90) );
    }

    par8=-8;
    par9=7.15;
    if(Dev90>35){
      if(Dev90<=45)
        par9 = 7.5 - (7.5-7.15)/10*(45-Dev90);
      else if(Dev90<=55)
        par9 = 8.2 - (8.2-7.5)/10*(55-Dev90);
    }
    soften1=1./(1+exp(8.*(x0-par0)));
    soften2=1./(1+exp(8.*(x0-par1)));
    CutLeftCutRight=1./((1+exp(par6*(x0-par7)))*(1+exp(-par8*(x0-par9))));
    line1=par4*(x0-par0)+par2;
    line2=(par2-par3)/(par0-par1)*(x0-par0)+par2;
    line3=par5*(x0-par1)+par3;
    add45degbumb=1.2*exp(-(x0+7.55)*(x0+7.55)/.35/.35);

    return CutLeftCutRight*((line1+add45degbumb)*soften1
                            +line2*soften2*(1-soften1)
                            +line3*(1-soften2) );
}


/**************************************************************/
/* condiders the flux drop at the vertical moderator ends     */
/* Input:  y0   : vert. position on the moderator             */
/* Return: factor (between 0 and 1.005)                       */
/**************************************************************/
double TSC2015_thmly0_BF3cm(const double y0){
    if(y0<-3./2.+0.105){
        return 1.005*exp(-pow((y0+3./2.-0.105)/0.372,2));
    } else if(y0>3./2.-0.105){
        return 1.005*exp(-pow((y0-3./2.+0.105)/0.372,2));
    }
    return 1.005;
}

double TSC2016_thmly0_BF3cm(const double y0)
{
  double HthmlHlf = 4.6/2.0 - 0.105;    // half of the height of the bright part of the thermal moderator
                                        // height increased from 3.0 to 4.6 cm for the thermal moderator
  if (y0 < -HthmlHlf)
  {
    return 1.005*exp(-pow(HthmlHlf/0.372, 2.0));
  }
  else if (y0 > HthmlHlf)
  {
    return 1.005*exp(-pow(HthmlHlf/0.372,2.0));
  }
  return 1.005;
}

double TSC2015_thmly0_BF6cm(const double y0){
  if(y0<-6./2.){
    return (-0.0037*y0+1)*exp(-pow((y0+6./2.)/0.835,2));
  } else if(y0>6./2.){
    return (-0.0037*y0+1)*exp(-pow((y0-6./2.)/0.835,2));
  }
  return 1.0;
}


/**************************************************************/
/* returns the interpolated flux value from the flux arrays   */
/* Input:  y   : hor. position on the moderator               */
/*         kMod: ID of the physical moderator                 */
/* Return: interpolated flux                                  */
/**************************************************************/
static double HorFlux(const double y, short kMod)
{
  double F=0.0,            // factor and flux
         Fn=0.0, Fn1=0.0;  // flux values: corr. to y, n-th and (n+1)the value in table
  short  n=0;

  //
  while (n+1 < stFluxHor.nLines  &&  stFluxHor.pTabX[n+1] < y)
  {  n++;
  }

  if (n+1 < stFluxHor.nLines)
  {
    /* linear  extrapolation */
    if (kMod==MOD_COLD)
    { Fn  = stFluxHor.pTabFC[n];
      Fn1 = stFluxHor.pTabFC[n+1];
    }
    else
    { Fn  = stFluxHor.pTabFT[n];
      Fn1 = stFluxHor.pTabFT[n+1];
    }

    F   = Fn  +  (Fn1-Fn ) / (stFluxHor.pTabX[n+1] - stFluxHor.pTabX[n])
                           * (y                    - stFluxHor.pTabX[n]);
  }
  else
  /* read error: wavelength larger than all values in the distribution file */
  {  CountMessage(SRC_Y_RANGE_TOO_SMALL);
  }

  return F;
}


/**************************************************************/
/* normalizes the moderator flux to proton current and energy */
/* Input:  Power  : timed averaged proton beam power in W     */
/*         Voltage: accelerator voltage in V                  */
/* Return: Normalization factor:                              */
/**************************************************************/
static double NeutrYieldNorm(const double PowerAvrg, const double Voltage)
{
  double NormFact=1.0;  // 1.0 for 5 MW, 2.5 GV, 4% duty cycle  

  if (stSrc.CurrAvrg > 0.0 && Voltage > 0.0)
  { NormFact  = stSrc.CurrAvrg * Voltage / (0.002 * 2.5e9); // standard: 2 mA, 2.5 GV  (corresp. to 5 MW)
    NormFact *= 1.0;                                        // proton energy dependent neutron yield not yet known
  }
  else
  {
    NormFact  = PowerAvrg / 5.0e6;                          // standard: 5 MW
  }
  return NormFact;
}
