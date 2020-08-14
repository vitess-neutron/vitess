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

#define MOD_THML  1
#define MOD_COLD  2

#define NEUT_FAST 1
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
  double  FluxIntF1, FluxIntS1,
          FluxIntF2, FluxIntS2;
}
FctTableM;

FctTableM  stFluxHor;                         // data of horizontal flux distribution
double     Shift=0.0,                         // Position of border between moderators
           kappa1_thml=0.0, kappa2_thml=1.0,  // fraction of thermal and cold spectrum in moderator 
           kappa1_cold=1.0, kappa2_cold=0.0;  // spectrum 1 (pos < Shift) and spectrum 2 (pos > Shift)

// prototypes
// ----------
static double TSC2015_ParaSpectra_BF3cm   (const double lambda, const double theta);
static double TSC2015_ThermalSpectra_BF3cm(const double lambda, const double theta);
static double TSC_TimeDist_Final_Thermal  (double time, double lambda, double height);
static double TSC_TimeDist_Final_Cold     (double time, double lambda, double height);

static double TSC2016_coldx0_BF3cm(const double x0, const double lambda);
static double TSC2015_coldx0_BF3cm(const double x0, const double theta);
static double TSC2015_coldy0_BF3cm(const double y0);
static double TSC2015_coldy0_BF6cm(const double y0);

static double TSC2016_thmlx0_BF3cm(const double x0, const double lambda);
static double TSC2015_thmlx0_BF3cm(const double x0, const double theta);
static double TSC2015_thmly0_BF3cm(const double y0);
static double TSC2015_thmly0_BF6cm(const double y0);

static double HorFlux (const double y, short kMod);

extern short  iDataVsn;          /* version of the data base for the source characteristics */
             
/**************************************************************/
/*  static functions                                          */
/**************************************************************/
double EssTotFU2015(const double ModHeight, const double ModTemp, const double Power, const double Freq, const double PulseLen)
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
	{	sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of 50 mA", 1000.0*CurrMax);
		Warning(sBuffer);
	}

	if ( (ModHeight > 2.9 && ModHeight < 3.1) || (ModHeight > 5.9 && ModHeight < 6.1) ){ 
	  // integral of TSC2015_[Para/Thermal]Spectra_BF3cm, mean of theta 5-55 deg:
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

	FUAmpl *= 0.75;          // engineering factor
	FUAmpl *= Power/5.0e06;  // scaling to an average power different from 5 MW

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
         DutyCycle=0.04,
	       Epulse,           // [J] energy of 1 pulse          
	       U0    = 2.5e9,    // [V] accelerator voltage: 2.5 GV 
	       CurrMax;          // [A] max. current for this set-up     
	char   sBuffer[256];

	Epulse = Power / Freq;

	/* maximal accelerator current */
	CurrMax   = Epulse / PulseLen / U0;
	if (CurrMax > 0.05001)
	{	sprintf(sBuffer,"Maximal accelerator current of %5.1f mA exceeds limit of 50 mA", 1000.0*CurrMax);
		Warning(sBuffer);
	}

	if ( (ModHeight > 2.9 && ModHeight < 5.0) )
  { 
	  // integral of TSC2015_[Para/Thermal]Spectra_BF3cm, mean of theta 30 -120 deg:
	  if (ModTemp < 100.0)
	    FUAmpl = 16.0e14*DutyCycle/Freq*0.816*PfmcCold; // cold 
	  else
	    FUAmpl =  9.0e14*DutyCycle/Freq*0.732*PfmcThml; // thermal
	}
	else
	{
	  Error("source_ess.c: ESS moderator data 2016 are only implemented for 3 cm moderator height");
	}

	return(FUAmpl);
}


double EssModFU_Butterfly2015(const double ModHeight, const double Power, const double Freq, const double Declination, const Neutron* pNeutron, const double PfmcThml, const double PfmcCold)                     
/* ModHeight  : [cm]  moderator height 
   Power      : [W]   average source power                             
   Declination: [deg] pulse frequency                             
   pNeutrom   : [s]   Pointer to data of the neutron trajectory   */
{
  double brightness=0.0,    // [n/(cm²s str Ang)]  brightness of neutron beam for given parameters
         lambda,            // [Ang] neutron wavelength
         theta,             // [deg] deviation from moderator center = 90° relative proton beam
                            //       possible values 5°, 15°, 25°, 35°, 45° and 55°
         x0,                //  [m]  horizontal starting position on moderator (for cold and thermal moderator)
         y0,                //  [m]  vertical starting position on moderator
         time;              //  [s]  starting time at moderator
 
  // calculation of (McStas) parameters used in the analytical functions
  lambda = pNeutron->Wavelength;
  theta  = Declination; //10.0*floor(abs(Declination)/10.0)+5.0;
  x0     = pNeutron->Position[1]; // [cm] 
  y0     = pNeutron->Position[2]; // [cm] 
  time   = pNeutron->Time/1000.0;
  
  // mirror x0 to match weird coordinate system in functions
  if(theta > 0.0)
    x0 *= -1.0;
  // theta=0: only place from which one can see both cold moderators
  if (fabs(theta) < 0.01 && x0 > 0)
    x0 *= -1.0;

  if (fabs(theta) > 55.0 && iDataVsn == 5)
    Error("source_ess.c: ESS moderator data 2015 only implemented for declination |theta| <= 55 deg");

  if (ModHeight > 2.9 && ModHeight < 3.1)
  { 
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(theta))
                  *TSC2015_coldy0_BF3cm(y0)
                  *TSC2015_coldx0_BF3cm(x0, fabs(theta))
                  *TSC_TimeDist_Final_Cold(time,lambda,ModHeight)
                  *PfmcCold                                               // loss due to engineering details not modeled
                  +
                  TSC2015_ThermalSpectra_BF3cm(lambda, fabs(theta))
                  *TSC2015_thmly0_BF3cm(y0)
                  *TSC2015_thmlx0_BF3cm(x0, fabs(theta))
                  *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight) 
                  *PfmcThml );                                            // loss due to engineering details not modeled
  }
  else if (ModHeight > 5.9 && ModHeight < 6.1)
  { 
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(theta)) * 0.631 // apply factor BF6cm/BF3cm
		   *TSC2015_coldy0_BF6cm(y0)                                      // parameters calc for 6cm
		   *TSC2015_coldx0_BF3cm(x0, fabs(theta))                         // same as 3cm
		   *TSC_TimeDist_Final_Cold(time,lambda,ModHeight)                // same as 3cm
		   *PfmcCold 
           +
		   TSC2015_ThermalSpectra_BF3cm(lambda, fabs(theta)) * 0.689
		   *TSC2015_thmly0_BF6cm(y0)
		   *TSC2015_thmlx0_BF3cm(x0, fabs(theta))
		   *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight)
           *PfmcThml ); 
  }
  else
  {
    Error("source_ess.c: ESS moderator data 2015 only implemented for 3 cm and 6 cm moderator height");
  }

  brightness /= Freq;   
  brightness *= Power/5.0e06;  // scaling to an average power different from 5 MW
    
  return(brightness);
}

double EssModFU_Butterfly2016(const double   ModTemp,  const double Power,    const double Freq, const double Declination, 
                              const Neutron* pNeutron, const double PfmcThml, const double PfmcCold)                     
/* ModTemp    : [cm]  moderator temperature
   Power      : [W]   average source power                             
   Freq       : [Hz]  pulse frequency
   Declination: [deg] deviation of beamline direction from moderator surface normal                             
   pNeutron   : [s]   Pointer to data of the neutron trajectory   */
{
  double brightness=0.0,        // [n/(cm²s str Ang)]  brightness of neutron beam for given parameters
         lambda,                // [Ang] neutron wavelength
         theta,                 // [deg] deviation from moderator normal = 90° relative proton beam
                                //       possible values -60°, -54°, -48°, ... 60°
         x0,                    //  [m]  horizontal starting position on moderator
         y0,                    //  [m]  vertical starting position on moderator
         fxc=0.0, fxt=0.0,      //       factors defining horizontal intensity distribution
         fyc=0.0, fyt=0.0,      //       factors defining vertical intensity distribution
         kappa_thml, kappa_cold,//       fraction of thermal and cold spectrum in moderator 
         time;                  //  [s]  starting time at moderator
 
  // calculation of (McStas) parameters used in the analytical functions
  lambda = pNeutron->Wavelength;
  theta  = Declination;         
  x0     = pNeutron->Position[1]; // [cm] 
  y0     = pNeutron->Position[2]; // [cm] 
  time   = pNeutron->Time/1000.0; // [s]
  
  if (fabs(theta) > 60.0)
    Error("source_ess.c: ESS moderator data 2016 only implemented for declination |theta| <= 60 deg");

  // set fractions of cold and thermal spectrum for the given moderator
  if (x0 < Shift)
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
  if (kappa_cold > 0.0) fxc = TSC2016_coldx0_BF3cm(x0, lambda);
  if (kappa_thml > 0.0) fxt = TSC2016_thmlx0_BF3cm(x0, lambda);
 #endif
  if (kappa_cold > 0.0) fyc = TSC2015_coldy0_BF3cm(y0);
  if (kappa_thml > 0.0) fyt = TSC2015_thmly0_BF3cm(y0);
  brightness =  kappa_cold
              * TSC2015_ParaSpectra_BF3cm(lambda, fabs(theta))
              * TSC_TimeDist_Final_Cold(time,lambda, 3.0)
              * fxc * fyc * 0.816 * PfmcCold                            // loss due to engineering details not modeled
             +  kappa_thml
              * TSC2015_ThermalSpectra_BF3cm(lambda, fabs(theta))
              * TSC_TimeDist_Final_Thermal(time,lambda, 3.0) 
              * fxt * fyt * 0.732 * PfmcThml ;                          // loss due to engineering details not modeled

  brightness /= Freq;   
  brightness *= Power/5.0e06;  // scaling to an average power different from 5 MW
    
  return(brightness);
}


// width of cold moderator defined as (thermal analogous):
//   TSC2015_coldx0_BF3cm > TSC2015_thmlx0_BF3cm (inner edge)
//    AND
//   TSC2015_coldx0_BF3cm > 0.5 (outer edge)
// calculated for theta=5,15,...55, extrapolate in between
double GetModWidth_ESSbutterfly2015(const double theta, double const ModTemp){
  double Width[6][2]={ {7.1,14.2}, {8.0,14.1}, {8.0,14.2}, {7.9,14.3}, {6.9,15.6}, {7.0,16.0} };  // {cold, thml}
  int i=1;
  short mod = (fabs(ModTemp-50)<0.1) ? 0 : 1;
 
   if(theta<=5){
     return Width[0][mod];
   }
   else{
     for(i=1; i<6; i++){
       if(theta<=5+10*i){
	 return Width[i][mod]-(Width[i][mod]-Width[i-1][mod])/10*(5+10*i-theta);
       }
     }
  }
   return 0;
}

double GetModWidth_ESSbutterfly2016(const double theta, double const ModTemp)
{ 
 //          Beamport      11    10     9     8    7    6    5    4    3    2     1      
  double WidthCold[11] = { 6.9,  8.3,  8.6,  8.7, 8.8, 8.8, 8.7, 8.5, 7.6, 7.25, 5.0},  // eff. widths cold
         WidthThml[11] = {10.5, 10.5, 10.3, 10.0, 9.6, 9.1, 8.5, 7.7, 7.7, 6.80, 5.8},  // eff. widths thermal
         DelTheta      = 6.0,  // distance in deg between neighboring beamports
         DTheta,               // distance in deg to j-th standard beamline direction 
         width=0.0,
         frac, kTheta;
  int    jTheta;

  frac = modf(theta/DelTheta, &kTheta);
  jTheta = Round(kTheta);
  DTheta = theta - DelTheta*kTheta;

  if (ModTemp < 100.0) 
    width = WidthCold[jTheta] + (WidthCold[jTheta+1] - WidthCold[jTheta])/DelTheta * DTheta;
  else                   
    width = WidthThml[jTheta] + (WidthThml[jTheta+1] - WidthThml[jTheta])/DelTheta * DTheta;

  width /= cos(theta*M_PI/180.0);
  return width;
}


double GetShift_ESSbutterfly2016(const double theta)
{ 
 //          Beamport  11   10    9    8    7    6    5    4     3      2      1      
  double aShift[11] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.64, -0.48, -0.21},
         shift=0.0;                         // eff. shift of the cross over point between cold and thermal moderator
  int    iTheta = Round(theta / 6.0); 

  shift = aShift[iTheta];
  shift /= cos(theta*M_PI/180.0);

  return shift;
}


short GetColour_ESSbutterfly2015(double x0, const double theta)
{
  // mirror x0 to match weird coordinate system in functions
  if(theta>0)
    x0 *= -1;
  // theta=0: only place from which one can see both cold moderators
  if (fabs(theta)<0.01 && x0>0)
    x0 *= -1;

  if( TSC2015_coldx0_BF3cm(x0, fabs(theta)) > TSC2015_thmlx0_BF3cm(x0, fabs(theta)) )
    return 2;
  else 
    return 1;
}


short GetColour_ESSbutterfly2016(const double lambda)
{
  short colour=0;

  if      (lambda > LMBD_F_S) colour=NEUT_SLOW;  // cold
  else if (lambda < LMBD_H_F) colour=NEUT_HOT;   // hot
  else                        colour=NEUT_FAST;  // thermal

  return colour;
}



// load horizontal intensity distribution from file
// ------------------------------------------------
void LoadHorDistrib(const char* sID)
{
  long   i;
  double DelY=0.0, 
         Y1min, Y1max, Y2min, Y2max,
         YR, YL, FTR, FTL, FCR, FCL,
         Decl;
  char   sFileName[20],
         sBuffer[CHAR_BUF_LENGTH]=""; 
  FILE*  pDisFile=NULL;
  short  iBL=0;
  char   cOrient=' ';

  // check input
  if (strlen(sID) > 0) 
    sscanf(sID, "%c%hd", &cOrient, &iBL);

  // loading file containing horizontal flux distribution 
  if ((cOrient=='S' || cOrient=='N' || cOrient=='O' || cOrient=='W') && iBL > 0 && iBL < 12) 
  {
    // Determine range of cold and thermal moderator
    Decl=CalcDecl(CalcTheta(sID));
    double Width1, Width2,
           CosDecl= cos(Decl*M_PI/180.0);
    Shift  = CosDecl * GetShift_ESSbutterfly2016(fabs(Decl));
    if (Decl < 0.0)
    { Width1 = CosDecl * GetModWidth_ESSbutterfly2016(fabs(Decl),  50.0);
      Width2 = CosDecl * GetModWidth_ESSbutterfly2016(fabs(Decl), 325.0);
      kappa1_thml = 0.0;
      kappa1_cold = 1.0;
      kappa2_thml = 1.0;
      kappa2_cold = 0.0;
    }
    else
    { Width1 = CosDecl * GetModWidth_ESSbutterfly2016(fabs(Decl), 325.0);
      Width2 = CosDecl * GetModWidth_ESSbutterfly2016(fabs(Decl),  50.0);
      kappa1_thml = 1.0;
      kappa1_cold = 0.0;
      kappa2_thml = 0.0;
      kappa2_cold = 1.0;
    }

    Y1min = Shift - Width1; Y1max = Shift; 
    Y2min = Shift;          Y2max = Shift + Width2;

    /* opening distribution file, either from the input directory or from the installation directory */
    sprintf(sFileName, "ESS2016_%s.dat", sID);
    pDisFile = OpenInputFile(sFileName, FALSE, "rt");
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
      stFluxHor.FluxIntF1 = 0.0; stFluxHor.FluxIntS1 = 0.0;
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
    { fprintf(LogFilePtr,"ERROR: Can't open file '%s' to read horizontal intensity distribution, neither in the input directory nor in 'InstallDir/FILES/moderators/ESS'\n", 
                         sFileName);
      exit (-1);
    }
  } 
  else 
  { Error("You have to give a valid beam port!\n");
  }
}


double CalcTheta(const char* sID)
{
  double theta=0.0, delTh;
  short  iBL;
  char   cOrient;

  sscanf(sID, "%c%hd", &cOrient, &iBL);

  delTh = 6.0 * (11 - iBL);

  switch (cOrient)
  { case 'W': theta = 270.0 + delTh; break;
    case 'N': theta = 270.0 - delTh; break;
    case 'E': theta =  90.0 + delTh; break;
    case 'S': theta =  90.0 - delTh; break;
    default : Error("wrong ID for the calculation of the beamline orientation");
  }
  return theta;  
}

double CalcDecl(const double theta)
{
  double decl=0.0;

  if (theta > 180.0)
    decl = theta - 270.0;
  else
    decl = theta -  90.0;

  return decl;    
}

char* GenerBeamport(const double Decl)
{
  static char sPort[5];
  char   sOrient =' ';
  short  iBP;
  double DelTheta=6.0;  // distance between neighboring beamports       

  iBP = 11 - (int) Round(fabs(Decl) / DelTheta); 

  if (Decl > 0) sOrient='E';
  else          sOrient='S';

  sprintf(sPort, "%c%d", sOrient, iBP);

  return sPort;
}

/**************************************************************/
/*  static functions                                          */
/**************************************************************/

double TSC2015_ParaSpectra_BF3cm(const double lambda, const double theta){

    double par0=8.44e13/25.;
    double par1=2.5;
    double par2=2.2;

    double par3=-13.-.5*(theta-5);
    double par4=2.53;
    double par5=-0.0478073-0.160*exp(-0.45186*(theta-5.)/10.);

    double par6=6.0e+015/25.;
    double par7=0.788956+0.00854184*(theta-5.)/10.;
    double par8=0.0461868-0.0016464*(theta-5.)/10.;
    double par9=0.325;

    double SD_part, para_part;

      //extrapolate linearly between fitted values
    if(theta<=5)
      par6=5.73745e+015/25.;
    else if(theta<=15)
      par6 = 5.88284e+015/25. - (5.88284e+015/25. - 5.73745e+015/25. )/10*(15-theta);
    else if(theta<=25)
      par6= 6.09573e+015/25. - (6.09573e+015/25.-5.88284e+015/25.)/10*(25-theta);
    else if(theta<=35)
      par6= 6.29116e+015/25. - (6.29116e+015/25.-6.09573e+015/25.)/10*(35-theta) ;
    else if(theta<=45)
      par6 = 6.03436e+015/25. - (6.03436e+015/25.-6.29116e+015/25.)/10*(45-theta);
    else if(theta<=55)
      par6 = 6.02045e+015/25. - (6.02045e+015/25.-6.03436e+015/25.)/10*(55-theta);

    SD_part=par0/((1+exp(par1*(lambda-par2)))*lambda);
    para_part=pow((1+exp(par3*(lambda-par4))),par5)*(par6*(exp(-par7*(lambda))+par8*exp(-par9*(lambda))));

    return para_part+SD_part;
}

double TSC2015_ThermalSpectra_BF3cm(const double lambda, const double theta){
   
    double i, par0, par2, par3, aOlsqr;    

    if(lambda<=0)return 0;

    i=(theta-5.)/10.;
    par0=4.2906e+013-9.2758e+011*i+8.02603e+011*i*i-1.29523e+011*i*i*i;
    par2=6.24806e+012-8.84602e+010*i;
    par3=-0.31107+0.0221138*i;
    aOlsqr=949./(325*lambda*lambda);

    return par0*2.*aOlsqr*aOlsqr/lambda*pow(lambda,-par3)*exp(-aOlsqr)+par2/((1+exp(2.5*(lambda-0.88)))*lambda);
}


double TSC_TimeDist_Final_Thermal(double time,double lambda,double height)
{
    double tau;
 
    if (time<0) return 0;
    tau=3.00000e-004*(1.23048e-002*lambda*lambda+1.75628e-001*exp(-1.82452e-001*height)+9.27770e-001)*exp(-3.91090e+001*pow(Max(1e-13,lambda+0.987990),-7.65675));
    if (time<0.0028) return 1/0.0028*(1.0-exp(-time/tau));   // corrected exp() -> 1-exp()   (KL, 15.05.15)
    return 1/0.0028*(1-exp(-0.0028/tau))*exp(-(time-0.0028)/tau);
}

double TSC_TimeDist_Final_Cold(double time,double lambda,double height)
{
    double tau;
 
    if (time<0) return 0;
    tau=3.00094e-004*(4.15681e-003*lambda*lambda+2.96212e-001*exp(-1.78408e-001*height)+7.77496e-001)*exp(-6.63537e+001*pow(Max(1e-13,lambda+0.9),-8.64455));
    if(time<0.0028)return 1/0.0028*(1.0-exp(-time/tau));   // corrected exp() -> 1-exp()   (KL, 15.05.15)
    return 1/0.0028*(1-exp(-0.0028/tau))*exp(-(time-0.0028)/tau);
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

double TSC2015_coldx0_BF3cm(const double x0, const double theta)
{
  double i, line, CutLeftCutRight;
  double par0, par1, par2, par3, par4, par5; 

    i=(theta-5.)/10.;
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


double TSC2016_thmlx0_BF3cm(const double y, const double lambda)
{ 
  double F,f;
  short iCol=GetColour_ESSbutterfly2016(lambda);

  F = HorFlux(y, iCol);

  if (iCol==NEUT_SLOW)
    f = F/FLUX_S_THML;
  else
    f = F/FLUX_F_THML;

  return f;
}

double TSC2015_thmlx0_BF3cm(const double x0, const double theta)
{
    double i, soften1, soften2, CutLeftCutRight, line1, line2, line3, add45degbumb;
    double par0, par1, par2, par3, par4, par5, par6, par7, par8, par9; 

    i=(theta-5.)/10.;
    par0=-5.54775+0.492804*i;
    par1=-0.265929-0.711477*i;
    if(theta==55)par1=-2.55;

    par2=0.821885+0.00914832*i;
    par3=1.31108-0.00698647*i;
    if (theta==55) par3=1.23;
    par4=-.035;
    par5=-0.0817358+0.00807125*i;
        
    par6=-8;
    par7=-7.15;
    if(theta>35){
      if(theta<=45)
	par7 = - ( 8.2 -(8.2-7.15)/10*(45-theta) );
      else if(theta<=55)
	par7 = - ( 7.7 -(7.7-8.2)/10*(55-theta) );
    }

    par8=-8;
    par9=7.15;
    if(theta>35){
      if(theta<=45)
	par9 = 7.5 - (7.5-7.15)/10*(45-theta);
      else if(theta<=55)
	par9 = 8.2 - (8.2-7.5)/10*(55-theta);
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

double TSC2015_thmly0_BF3cm(const double y0){
    if(y0<-3./2.+0.105){
        return 1.005*exp(-pow((y0+3./2.-0.105)/0.372,2));
    } else if(y0>3./2.-0.105){
        return 1.005*exp(-pow((y0-3./2.+0.105)/0.372,2));
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


static double HorFlux(const double y, short kMod)
{
  double F=0.0,            // factor and flux
         Fn=0.0, Fn1=0.0;  // flux values: corr. to y, n-th and (n+1)the value in table
	short  n=0;
  
  // 
	while (n+1 < stFluxHor.nLines  &&  stFluxHor.pTabX[n+1] < y)
	{	n++;
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
	{	CountMessage(SRC_Y_RANGE_TOO_SMALL);
	}

  return F;
}


