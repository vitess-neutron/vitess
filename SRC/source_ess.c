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

// prototypes
// ----------
//static double TSC2015_z0_BF3cm(const double x0);
static double TSC2015_ParaSpectra_BF3cm(const double lambda, const double theta);
static double TSC2015_ThermalSpectra_BF3cm(const double lambda, const double theta);
static double TSC2015_coldx0_BF3cm(const double x0, const double theta);
static double TSC2015_coldy0_BF3cm(const double y0);
static double TSC2015_thermaly0_BF3cm(const double y0);
static double TSC2015_thermalx0_BF3cm(const double x0, const double theta);
static double TSC_TimeDist_Final_Thermal(double time,double lambda,double height);
static double TSC_TimeDist_Final_Cold(double time,double lambda,double height);
static double TSC2015_coldy0_BF6cm(const double y0);
static double TSC2015_thermaly0_BF6cm(const double y0);
            
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
	    FUAmpl = 8.11e13/Freq ; // cold 
	  else
	    FUAmpl = 5.85e13/Freq; // thermal
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


double EssModFU2015(const double ModHeight, const double Power, const double Freq, const double Declination, const Neutron* pNeutron)                     
/* ModHeight  : [cm]  moderator height 
   Power      : [W]   average source power                             
   Declination: [deg] pulse frequency                             
   pNeutrom   : [s]   Pointer to data of the neutron trajectory   */
{
  double brightness=0.0,    // [n/(cm²s str Ang)]  brightness of neutron beam for given parameters
         lambda,            // [Ang] neutron wavelength
         theta,             // [deg] deviation from moderator center = 90° relative proton beam
                            //       possible values 5°, 15°, 25°, 35°, 45° and 55°
         x0,                //  [m]  horizontal starting position on moderator
         y0,                //  [m]  vertical starting position on moderator
         time;              //  [s]  starting time at moderator
 
  // calculation of (McStas) parameters used in the analytical functions
  lambda = pNeutron->Wavelength;
  theta  = Declination; //10.0*floor(abs(Declination)/10.0)+5.0;
  x0     = pNeutron->Position[1]; // [cm] 
  y0     = pNeutron->Position[2]; // [cm] 
  time   = pNeutron->Time/1000.0;
  
  // mirror x0 to match weird coordinate system in functions
  if(theta>0)
    x0 *= -1;
  // theta=0: only place from which one can see both cold moderators
  if (fabs(theta)<0.01 && x0>0)
    x0 *= -1;

  if(fabs(theta)>55)
    Error("source_ess.c: ESS moderator data 2015 only implemented for declination |theta| <= 55 deg");

  if (ModHeight > 2.9 && ModHeight < 3.1)
  { 
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(theta))
		   *TSC2015_coldy0_BF3cm(y0)
		   *TSC2015_coldx0_BF3cm(x0, fabs(theta))
		   *TSC_TimeDist_Final_Cold(time,lambda,ModHeight)
		   +
		   TSC2015_ThermalSpectra_BF3cm(lambda, fabs(theta))
		   *TSC2015_thermaly0_BF3cm(y0)
		   *TSC2015_thermalx0_BF3cm(x0, fabs(theta))
		   *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight) )*0.75; // -25% due to engineering details not modeled
  }
  else if (ModHeight > 5.9 && ModHeight < 6.1)
  { 
    brightness = ( TSC2015_ParaSpectra_BF3cm(lambda, fabs(theta)) * 0.631 // apply factor BF6cm/BF3cm
		   *TSC2015_coldy0_BF6cm(y0)                              // parameters calc for 6cm
		   *TSC2015_coldx0_BF3cm(x0, fabs(theta))                 // same as 3cm
		   *TSC_TimeDist_Final_Cold(time,lambda,ModHeight)        // same as 3cm
		   +
		   TSC2015_ThermalSpectra_BF3cm(lambda, fabs(theta)) * 0.689
		   *TSC2015_thermaly0_BF6cm(y0)
		   *TSC2015_thermalx0_BF3cm(x0, fabs(theta))
		   *TSC_TimeDist_Final_Thermal(time,lambda,ModHeight) )*0.75; 
  }
  else
  {
    Error("source_ess.c: ESS moderator data 2015 only implemented for 3 cm and 6 cm moderator height");
  }

  brightness /= Freq;   
  brightness *= Power/5.0e06;  // scaling to an average power different from 5 MW
    
  return(brightness);
}


/**************************************************************/
/* // not used 
double TSC2015_z0_BF3cm(const double x0){
    if(x0<-7.16)
        return (8.27-5.1)/(-7.16+14.2)*(x0+14.2)+5.1;
    return 8.27;
} */

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

double TSC2015_thermaly0_BF3cm(const double y0){
    if(y0<-3./2.+0.105){
        return 1.005*exp(-pow((y0+3./2.-0.105)/0.372,2));
    } else if(y0>3./2.-0.105){
        return 1.005*exp(-pow((y0-3./2.+0.105)/0.372,2));
    }
    return 1.005;
}

double TSC2015_thermaly0_BF6cm(const double y0){
  if(y0<-6./2.){
    return (-0.0037*y0+1)*exp(-pow((y0+6./2.)/0.835,2));
  } else if(y0>6./2.){
    return (-0.0037*y0+1)*exp(-pow((y0-6./2.)/0.835,2));
  }
  return 1.0;
}

double TSC2015_thermalx0_BF3cm(const double x0, const double theta)
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

// --------------------------------


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


// width of cold moderator defined as (thermal analogous):
//   TSC2015_coldx0_BF3cm > TSC2015_thermalx0_BF3cm (inner edge)
//    AND
//   TSC2015_coldx0_BF3cm > 0.5 (outer edge)
// calculated for theta=5,15,...55, extrapolate in between
double GetModeratorWidth_ESSbutterfly2015(double theta, double ModTemp){
  double Width[6][2]={ {7.1,14.2}, {8.0,14.1}, {8.0,14.2}, {7.9,14.3}, {6.9,15.6}, {7.0,16.0} };  // {cold, thermal}
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



short GetColour_ESSbutterfly2015(double x0, double theta){
  // mirror x0 to match weird coordinate system in functions
  if(theta>0)
    x0 *= -1;
  // theta=0: only place from which one can see both cold moderators
  if (fabs(theta)<0.01 && x0>0)
    x0 *= -1;

  if( TSC2015_coldx0_BF3cm(x0, fabs(theta)) > TSC2015_thermalx0_BF3cm(x0, fabs(theta)) )
    return 2;
  else 
    return 1;
}


