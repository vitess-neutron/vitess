/********************************************************************************************/
/*  VITESS module 'general.c'                                                               */
/*    Elementary functions for all VITESS modules                                           */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,                                     */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin                                        */
/*                                                                                          */
/* Change: K.L.  2002 JAN, reorganized routines                                             */
/* Change: G.Zs. 2002 JUL, new routines                                                     */
/* Change: K.L.  2003 JAN, new functions 'ReadLine', 'StrgLShift', and 'StrgCopy'           */
/* Change: K.L.  2003 FEB, definitions of 'idum' and 'LogFilePtr' from init to general      */
/* Change: K.L.  2003 MAR, new function 'StrgScanLF', additional parameter in 'ReadLine'    */
/* Change: M.F.  2005 DEC, random number generators from GNU GSL                            */
/* Change: A.H.  2009 OCT, new routine: RoundP for rounding after given decimal position    */
/********************************************************************************************/

#include <ctype.h>
#include <time.h>

#ifndef RND_SIMPLE
 #include "gsl/gsl_rng.h"
 gsl_rng * vit_gsl_rng;
#endif

#include "general.h"


double gsl_ran_gaussian (const gsl_rng * r, const double sigma);

FILE* LogFilePtr;        /* pointer to the log file stream              */


/****************************************************************************************/
/*  Conversion between physical properties                                              */
/****************************************************************************************/

double ENERGY_FROM_LAMBDA(const double x)
{
	return(81805.048 / x / x);   /* [Ang]   -> [ueV] */
}

double LAMBDA_FROM_ENERGY(const double e)
{
	return(sqrt(81805.048 / e)); /* [ueV]   -> [Ang] */
}

double ENERGY_FROM_V(const double v)
{
	return(0.5227033 * v * v);   /* [cm/ms] -> [ueV] */
}

double V_FROM_ENERGY(const double e)
{
	return(sqrt(e / 0.5227033)); /* [ueV] -> [cm/ms] */
}

double LAMBDA_FROM_V(const double x)
{
	return(395.60346 / x);       /* [cm/ms] -> [Ang] */
}

double V_FROM_LAMBDA(const double x)
{
	return(395.60346 / x);       /* [Ang]   -> [cm/ms] */
}


double Lambda2E(const double lmbd)
{
  double L2E = 0.5 * 1.0e23 * sq(H_P)/E_C / MN,   // Ang -> meV
         E   = L2E / sq(lmbd); 

  return(E);
}

double E2Lambda(const double E)
{
  double L2E  = 0.5 * 1.0e23 * sq(H_P)/E_C / MN,  // meV -> Ang 
         lmbd = sqrt(L2E / E); 

  return(lmbd);
}


double ReflAngle(const double lambda, const double Q)  // [Ang], [1/Ang] -> [deg]
{
  double thetaR = asin(lambda * Q/(4.0*M_PI));

  return(Degrees(thetaR));
}

double QbyRefl(const double lambda, const double thetaD)  // [Ang], [deg] -> [1/Ang] 
{
  double Q = 4.0*M_PI * sin(Radians(thetaD))/lambda;

  return(Q);
}

/****************************************************************************************/
/*  Random Functions                                                                    */
/****************************************************************************************/

/* uniformly distributed random numbers in [x,y] */
double MonteCarlo(const double x, const double y)
{
   return (y - x)*Vran() + x;
}

/* random numbers of Gaussian distribution with standard deviation 'Sigma' around 'Center' */
double DistrGauss(double Center, double Sigma)
{
  return Center + gsl_ran_gaussian(vit_gsl_rng, Sigma);
}


/****************************************************************************************/
/*  General Functions                                                                   */
/****************************************************************************************/

/* conversion between degree and rad */
double Radians(const double angleD)
{
  return (angleD * M_PI/180.0);
}

double Degrees(const double angleR)
{
  return (angleR * 180.0/M_PI);
}


/* computes square of a real value */
double sq(const double Value)
{
	return Value * Value ;
}


/* calculates atan2 in the range (0, 2*M_PI) */
double atan0(const double a, const double b)
{
	if (b > 0.)
	  return (double) atan(a / b) ;

	if (b == 0.)
	  return M_PI_2 ;

	return (double) atan(a / b) + M_PI ;
}

/* rounds a value mathematically  */
double Round(const double value)
{
	return floor(value + 0.5);
}

double RoundP(const double value, const int decimal)
{
	double f = pow(10.0, decimal);
	return Round(value * f) / f;
}


/* minimum and maximum of two double or long values */
long mini(const long value1, const long value2)
{
  return value1 < value2 ? value1 : value2;
}

long maxi(const long value1, const long value2)
{
  return value1 > value2 ? value1 : value2;
}

double Min(const double value1, const double value2)
{
  return value1 < value2 ? value1 : value2;
}

double Max(const double value1, const double value2)
{
  return value1 > value2 ? value1 : value2;
}

/* swap two values */
void Exchange(double* pValue1, double* pValue2)
{
  double dHelp;

  dHelp    = *pValue1;
  *pValue1 = *pValue2;
  *pValue2 = dHelp;
}


void CompID2Name (char* sCompName, const McCompID eComp)
{
  switch (eComp)
  { case MCN_SOURCE       : strcpy(sCompName, "Source");            break;           
	  case MCN_SRC_SMPL     : strcpy(sCompName, "SourceSimple");      break;     
	  case MCN_SRC_CWS      : strcpy(sCompName, "SourceConst");       break;      
	  case MCN_SRC_TOF      : strcpy(sCompName, "SourceTOF");         break;        
	  case MCN_SRC_SP       : strcpy(sCompName, "SourceSP");          break;         
	  case MCN_SRC_LP       : strcpy(sCompName, "SourceLP");          break;         
	  case MCN_READ_IN      : strcpy(sCompName, "EventsIn");          break;         
	  case MCN_SPACE        : strcpy(sCompName, "Space");             break;            
	  case MCN_SLIT         : strcpy(sCompName, "Slit");              break;             
	  case MCN_WINDOW       : strcpy(sCompName, "Window");            break;           
	  case MCN_WND_MULT     : strcpy(sCompName, "WindowMult");        break;       
	  case MCN_GRID         : strcpy(sCompName, "WindowGrid");        break;       
	  case MCN_LENSE        : strcpy(sCompName, "Lens");              break;             
	  case MCN_MIRROR       : strcpy(sCompName, "Mirror");            break;           
	  case MCN_MIRROR_POL   : strcpy(sCompName, "MirrorPolarizing");  break; 
	  case MCN_MIRROR_ELLI  : strcpy(sCompName, "MirrorElliptic");    break;   
	  case MCN_SM_ENSEMBLE  : strcpy(sCompName, "MirrorEmsemble");    break;   
	  case MCN_COLLIMATOR   : strcpy(sCompName, "Collimator");        break;       
	  case MCN_COLL_SOLLER  : strcpy(sCompName, "CollimatorSoller");  break; 
	  case MCN_COLL_RADIAL  : strcpy(sCompName, "CollimatorRadial");  break; 
	  case MCN_COLL_VIRT    : strcpy(sCompName, "CollimatorVirtual"); break;
	  case MCN_GUIDE        : strcpy(sCompName, "Guide");             break;            
	  case MCN_BENDER       : strcpy(sCompName, "Bender");            break;           
	  case MCN_CHOP_DISC    : strcpy(sCompName, "ChopperDisc");       break;      
	  case MCN_CHOP_FERMI   : strcpy(sCompName, "ChopperFermi");      break;     
	  case MCN_VEL_SELECT   : strcpy(sCompName, "VelocitySelector");  break; 
	  case MCN_MONO_ANA     : strcpy(sCompName, "MonochrAnalyzer");   break;  
	  case MCN_MONOCHROM    : strcpy(sCompName, "Monochromator");     break;    
	  case MCN_POL_HE3      : strcpy(sCompName, "PolarizerHe3");      break;     
	  case MCN_POL_SM       : strcpy(sCompName, "PolarizerSM");       break;      
	  case MCN_FLIP_COIL    : strcpy(sCompName, "FlipperCoil");       break;      
	  case MCN_FLIP_GRAD    : strcpy(sCompName, "FlipperGradient");   break;  
	  case MCN_RES_DRABKIN  : strcpy(sCompName, "ResonatorDrabkin");  break; 
	  case MCN_FIELD_PREC   : strcpy(sCompName, "MagnField");         break;        
	  case MCN_FIELD_ROT    : strcpy(sCompName, "MagnFieldRotating"); break;
	  case MCN_FIELD_SESANS : strcpy(sCompName, "MagnFieldSESANS");   break;  
	  case MCN_CAPTURE      : strcpy(sCompName, "Source");            break;           
	  case MCN_BEAMSTOP     : strcpy(sCompName, "BeamStop");          break;         
	  case MCN_SMPL_ENVIRO  : strcpy(sCompName, "SampleEnvironment"); break;
	  case MCN_DETECTOR     : strcpy(sCompName, "Detector");          break;         
	  case MCN_WRITEOUT     : strcpy(sCompName, "EventsOut");         break;        
	  case MCN_SMPL_EL_ISO  : strcpy(sCompName, "SampleElasticIsotr");break;
	  case MCN_SMPL_INELAST : strcpy(sCompName, "SampleInelastic");   break;  
	  case MCN_SMPL_SNGL_X  : strcpy(sCompName, "SampleSnglCrytal");  break; 
	  case MCN_SMPL_POWDER  : strcpy(sCompName, "SamplePowder");      break;     
	  case MCN_SMPL_S_Q     : strcpy(sCompName, "SampleSofQ");        break;       
	  case MCN_SMPL_NXS     : strcpy(sCompName, "SampleNXS");         break;        
	  case MCN_SMPL_SANS    : strcpy(sCompName, "SampleSANS");        break;       
	  case MCN_SMPL_REFL    : strcpy(sCompName, "SampleReflect");     break;    
	  case MCN_FRAME        : strcpy(sCompName, "Frame");             break;            
	  case MCN_FILTER       : strcpy(sCompName, "Filter");            break;           
	  case MCN_RESET        : strcpy(sCompName, "Reset");             break;            
	  case MCN_VISUAL       : strcpy(sCompName, "Visualization");     break;    
	  case MCN_MONITOR1     : strcpy(sCompName, "Monitor1D");         break;        
	  case MCN_MON1         : strcpy(sCompName, "Mon1D");             break;    
	  case MCN_MON1_BRL     : strcpy(sCompName, "Mon1D-Brill");       break;    
	  case MCN_MON1_POL     : strcpy(sCompName, "Mon1D-Pol");         break;    
	  case MCN_MONITOR2     : strcpy(sCompName, "Monitor2D");         break;        
    case MCN_MON2_POS     : strcpy(sCompName, "Mon2D_Pos");         break;    
	  case MCN_MON2_DIV     : strcpy(sCompName, "Mon2D_Div");         break;    
	  case MCN_MON2_KDIV    : strcpy(sCompName, "Mon2D_kDiv");        break;   
	  case MCN_MON2_RDIV    : strcpy(sCompName, "Mon2D_R-Div");       break;   
	  case MCN_MON2_POSDIV  : strcpy(sCompName, "Mon2D_Pos-Div");     break;
	  case MCN_MON2_WLDIV   : strcpy(sCompName, "Mon2D_Wl-Div");      break; 
	  case MCN_MON2_TOFWL   : strcpy(sCompName, "Mon2D_Tof-Wl");      break; 
	  case MCN_MON2_POL_POS : strcpy(sCompName, "Mon2D-Pol_Pos");     break;
	  case MCN_EVAL1_ELAST  : strcpy(sCompName, "Eval1D_Elastic");    break;   
	  case MCN_EVAL1_INELAST: strcpy(sCompName, "Eval1D_Inelastic");  break; 
	  case MCN_EVAL2_ELAST  : strcpy(sCompName, "Eval2D_Elastic");    break;   
	  case MCN_RUNTIME      : strcpy(sCompName, "RunTime");           break;          
	  case MCN_TOOL_A2B     : strcpy(sCompName, "Tool_Ascii2Bin");    break;             
	  case MCN_TOOL_CAS     : strcpy(sCompName, "Tool_CrysAnaSpec");  break;             
	  case MCN_TOOL_CHOP    : strcpy(sCompName, "Tool_ChopPhase");    break;             
	  case MCN_TOOL_DEF_DIR : strcpy(sCompName, "Tool_DefineDir");    break;             
	  case MCN_TOOL_DIR_VIEW: strcpy(sCompName, "Tool_DirectView");   break;             
	  case MCN_TOOL_GEN_BAT : strcpy(sCompName, "Tool_GenBatch");     break;             
	  case MCN_TOOL_GEN_COAT: strcpy(sCompName, "Tool_GenCoating");   break;             
	  case MCN_TOOL_GEN_EXTR: strcpy(sCompName, "Tool_GenExtrSys");   break;             
	  case MCN_TOOL_GEN_SURF: strcpy(sCompName, "Tool_GenSurface");   break;             
	  case MCN_TOOL_STD_DEV : strcpy(sCompName, "Tool_StdDeviat");    break;             
	  case MCN_TOOL_LAT_DST : strcpy(sCompName, "Tool_LatticeDist");  break;             
	  case MCN_TOOL_GUIDE   : strcpy(sCompName, "Tool_GuideShape");   break;             
	  case MCN_TOOL_DST_TIME: strcpy(sCompName, "Tool_DistTimePlot"); break;             
	  case MCN_TOOL_ANLZ_2D : strcpy(sCompName, "Tool_AnalyzeMon2D"); break;             
	  case MCN_OPT_MAIN     : strcpy(sCompName, "Opt_Main");          break;             
	  case MCN_OPT_PIPE     : strcpy(sCompName, "Opt_GenerPipe");     break;             
	  case MCN_OPT_FOM      : strcpy(sCompName, "Opt_FoM");           break;             
    default:                strcpy(sCompName, "unknown component"); 
  }
}


McCompID Name2CompID (const char* sCompName)
{
  McCompID eComp;

        if (strcmp(sCompName, "Source"))           eComp=MCN_SOURCE; 
  else if (strcmp(sCompName, "SourceSimple"))      eComp=MCN_SRC_SMPL     ;     
  else if (strcmp(sCompName, "SourceConst"))       eComp=MCN_SRC_CWS      ;      
  else if (strcmp(sCompName, "SourceTOF"))         eComp=MCN_SRC_TOF      ;        
  else if (strcmp(sCompName, "SourceSP"))          eComp=MCN_SRC_SP       ;         
  else if (strcmp(sCompName, "SourceLP"))          eComp=MCN_SRC_LP       ;         
  else if (strcmp(sCompName, "EventsIn"))          eComp=MCN_READ_IN      ;         
  else if (strcmp(sCompName, "Space"))             eComp=MCN_SPACE        ;            
  else if (strcmp(sCompName, "Slit"))              eComp=MCN_SLIT         ;             
  else if (strcmp(sCompName, "Window"))            eComp=MCN_WINDOW       ;           
  else if (strcmp(sCompName, "WindowMult"))        eComp=MCN_WND_MULT     ;       
  else if (strcmp(sCompName, "WindowGrid"))        eComp=MCN_GRID         ;       
  else if (strcmp(sCompName, "Lens"))              eComp=MCN_LENSE        ;             
  else if (strcmp(sCompName, "Mirror"))            eComp=MCN_MIRROR       ;           
  else if (strcmp(sCompName, "MirrorPolarizing"))  eComp=MCN_MIRROR_POL   ; 
  else if (strcmp(sCompName, "MirrorElliptic"))    eComp=MCN_MIRROR_ELLI  ;   
  else if (strcmp(sCompName, "MirrorEmsemble"))    eComp=MCN_SM_ENSEMBLE  ;   
  else if (strcmp(sCompName, "Collimator"))        eComp=MCN_COLLIMATOR   ;       
  else if (strcmp(sCompName, "CollimatorSoller"))  eComp=MCN_COLL_SOLLER  ; 
  else if (strcmp(sCompName, "CollimatorRadial"))  eComp=MCN_COLL_RADIAL  ; 
  else if (strcmp(sCompName, "CollimatorVirtual")) eComp=MCN_COLL_VIRT    ;
  else if (strcmp(sCompName, "Guide"))             eComp=MCN_GUIDE        ;            
  else if (strcmp(sCompName, "Bender"))            eComp=MCN_BENDER       ;           
  else if (strcmp(sCompName, "ChopperDisc"))       eComp=MCN_CHOP_DISC    ;      
  else if (strcmp(sCompName, "ChopperFermi"))      eComp=MCN_CHOP_FERMI   ;     
  else if (strcmp(sCompName, "VelocitySelector"))  eComp=MCN_VEL_SELECT   ; 
  else if (strcmp(sCompName, "MonochrAnalyzer"))   eComp=MCN_MONO_ANA     ;  
  else if (strcmp(sCompName, "Monochromator"))     eComp=MCN_MONOCHROM    ;    
  else if (strcmp(sCompName, "PolarizerHe3"))      eComp=MCN_POL_HE3      ;     
  else if (strcmp(sCompName, "PolarizerSM"))       eComp=MCN_POL_SM       ;      
  else if (strcmp(sCompName, "FlipperCoil"))       eComp=MCN_FLIP_COIL    ;      
  else if (strcmp(sCompName, "FlipperGradient"))   eComp=MCN_FLIP_GRAD    ;  
  else if (strcmp(sCompName, "ResonatorDrabkin"))  eComp=MCN_RES_DRABKIN  ;  
  else if (strcmp(sCompName, "MagnField"))         eComp=MCN_FIELD_PREC   ;        
  else if (strcmp(sCompName, "MagnFieldRotating")) eComp=MCN_FIELD_ROT    ;
  else if (strcmp(sCompName, "MagnFieldSESANS"))   eComp=MCN_FIELD_SESANS ;  
  else if (strcmp(sCompName, "Source"))            eComp=MCN_CAPTURE      ;           
  else if (strcmp(sCompName, "BeamStop"))          eComp=MCN_BEAMSTOP     ;         
  else if (strcmp(sCompName, "SampleEnvironment")) eComp=MCN_SMPL_ENVIRO  ;
  else if (strcmp(sCompName, "Detector"))          eComp=MCN_DETECTOR     ;         
  else if (strcmp(sCompName, "EventsOut"))         eComp=MCN_WRITEOUT     ;        
  else if (strcmp(sCompName, "SampleElasticIsotr"))eComp=MCN_SMPL_EL_ISO  ;
  else if (strcmp(sCompName, "SampleInelastic"))   eComp=MCN_SMPL_INELAST ; 
  else if (strcmp(sCompName, "SampleSnglCrytal"))  eComp=MCN_SMPL_SNGL_X  ; 
  else if (strcmp(sCompName, "SamplePowder"))      eComp=MCN_SMPL_POWDER  ; 
  else if (strcmp(sCompName, "SampleSofQ"))        eComp=MCN_SMPL_S_Q     ; 
  else if (strcmp(sCompName, "SampleNXS"))         eComp=MCN_SMPL_NXS     ; 
  else if (strcmp(sCompName, "SampleSANS"))        eComp=MCN_SMPL_SANS    ;
  else if (strcmp(sCompName, "SampleReflect"))     eComp=MCN_SMPL_REFL    ; 
  else if (strcmp(sCompName, "Frame"))             eComp=MCN_FRAME        ; 
  else if (strcmp(sCompName, "Filter"))            eComp=MCN_FILTER       ; 
  else if (strcmp(sCompName, "Reset"))             eComp=MCN_RESET        ; 
  else if (strcmp(sCompName, "Visualization"))     eComp=MCN_VISUAL       ;
  else if (strcmp(sCompName, "Monitor1D"))         eComp=MCN_MONITOR1     ;
  else if (strcmp(sCompName, "Mon1D"))             eComp=MCN_MON1         ;
  else if (strcmp(sCompName, "Mon1D-Brill"))       eComp=MCN_MON1_BRL     ;
  else if (strcmp(sCompName, "Mon1D-Pol"))         eComp=MCN_MON1_POL     ;
  else if (strcmp(sCompName, "Monitor2D"))         eComp=MCN_MONITOR2     ;
  else if (strcmp(sCompName, "Mon2D_Pos"))         eComp=MCN_MON2_POS     ;
  else if (strcmp(sCompName, "Mon2D_Div"))         eComp=MCN_MON2_DIV     ;
  else if (strcmp(sCompName, "Mon2D_kDiv"))        eComp=MCN_MON2_KDIV    ; 
  else if (strcmp(sCompName, "Mon2D_R-Div"))       eComp=MCN_MON2_RDIV   ; 
  else if (strcmp(sCompName, "Mon2D_Pos-Div"))     eComp=MCN_MON2_POSDIV  ;
  else if (strcmp(sCompName, "Mon2D_Wl-Div"))      eComp=MCN_MON2_WLDIV   ; 
  else if (strcmp(sCompName, "Mon2D_Tof-Wl"))      eComp=MCN_MON2_TOFWL   ; 
  else if (strcmp(sCompName, "Mon2D-Pol_Pos"))     eComp=MCN_MON2_POL_POS ;
  else if (strcmp(sCompName, "Eval1D_Elastic"))    eComp=MCN_EVAL1_ELAST  ; 
  else if (strcmp(sCompName, "Eval1D_Inelastic"))  eComp=MCN_EVAL1_INELAST; 
  else if (strcmp(sCompName, "Eval2D_Elastic"))    eComp=MCN_EVAL2_ELAST  ; 
  else if (strcmp(sCompName, "RunTime"))           eComp=MCN_RUNTIME      ; 
  else if (strcmp(sCompName, "Tool_Ascii2Bin"))    eComp=MCN_TOOL_A2B     ; 
  else if (strcmp(sCompName, "Tool_CrysAnaSpec"))  eComp=MCN_TOOL_CAS     ; 
  else if (strcmp(sCompName, "Tool_ChopPhase"))    eComp=MCN_TOOL_CHOP    ; 
  else if (strcmp(sCompName, "Tool_DefineDir"))    eComp=MCN_TOOL_DEF_DIR ; 
  else if (strcmp(sCompName, "Tool_DirectView"))   eComp=MCN_TOOL_DIR_VIEW; 
  else if (strcmp(sCompName, "Tool_GenBatch"))     eComp=MCN_TOOL_GEN_BAT; 
  else if (strcmp(sCompName, "Tool_GenCoating"))   eComp=MCN_TOOL_GEN_COAT; 
  else if (strcmp(sCompName, "Tool_GenExtrSys"))   eComp=MCN_TOOL_GEN_EXTR; 
  else if (strcmp(sCompName, "Tool_GenSurface"))   eComp=MCN_TOOL_GEN_SURF; 
  else if (strcmp(sCompName, "Tool_StdDeviat"))    eComp=MCN_TOOL_STD_DEV ; 
  else if (strcmp(sCompName, "Tool_LatticeDist"))  eComp=MCN_TOOL_LAT_DST ; 
  else if (strcmp(sCompName, "Tool_GuideShape"))   eComp=MCN_TOOL_GUIDE   ; 
  else if (strcmp(sCompName, "Tool_DistTimePlot")) eComp=MCN_TOOL_DST_TIME; 
  else if (strcmp(sCompName, "Tool_AnalyzeMon2D")) eComp=MCN_TOOL_ANLZ_2D ; 
  else if (strcmp(sCompName, "Opt_Main"))          eComp=MCN_OPT_MAIN     ; 
  else if (strcmp(sCompName, "Opt_GenerPipe"))     eComp=MCN_OPT_PIPE     ; 
  else if (strcmp(sCompName, "Opt_FoM"))           eComp=MCN_OPT_FOM      ; 
  else                                             eComp=MCN_COMP_UNKNOWN ; 

  return eComp; 
}


/* Approximation and exact calculation of the solid angle from horizontal and vertical opening angle */
/*   HorAngle : [rad] half of the opening in horizontal direction */
/*   VertAngle: [rad] half of the opening in vertical direction   */
double ApprSolidAngle(const double HorAngle, const double VertAngle)
{
  if (VertAngle < 0.55)
    /* solution for small angles: Omega = 2 phi * 2(tan(theta)-tan³(theta)/3) */
    return 4 * HorAngle  * (tan(VertAngle) - pow(tan(VertAngle),3)/3.0);
	
  if (HorAngle < 0.55)
    /* solution for small angles: Omega = 2(tan(phi)-tan³(phi)/3) * 2 theta */
    return 4 * VertAngle * (tan(HorAngle)  - pow(tan(HorAngle),3)/3.0);

  /* empirical approximation for large angles */
  return 4 * sqrt(HorAngle * sin(HorAngle) * VertAngle * sin(VertAngle));
}

double TrueSolidAngle(const double HorAngle, const double VertAngle)
{
  return (4 * asin(sin(HorAngle) * sin(VertAngle)));
}


// Calculation of reflectivity on supermirrors from wavelength and inclination angle
// either following quadratic SwissNeutronics description by Henrik Jacobsen (ReflSN)
// or using the new description   (ReflAllCpys)
// or using any reflectivity file (ReflInterpol)
//
// Lambda: wavelength        [Ang]
// Angle : inclination angle [deg]
// M     : nominal m value of the supermirror    
// Rdata : pointer to list of reflectivity values (ReflFile only)
//
double ReflSNT(char* sText, const double Q, const double m, const short bPrint)
{
  double S,T, 
    m2=m,          // m'     : 'real' m value
    Qc,            // Q_c    : crit. momentum transfer 
    QcNi  =QC_NI,  // Q_c,Ni : crit. momentum transfer of nickel
    R0    =0.99,   // R_0    : reflectivity for 0 <= Q <= Q_c
    alphaQ=0.0,    //          slope Delta_R / Delta_Q
    betaQ =0.0,    //          quadratic term to describe R(q)
    W     =0.0,    // W      : width of the cut-off  [1/Ang]
    R     =0.0;    // R      : reflectivity

  Qc = QcNi*Min(m, 1.0);
  //  Q  = 4*M_PI*sin(M_PI/180.0*Angle)/Lambda;

  if (Q <= Qc)
  { R = R0;
  }
  else
  { 
    if (m <= 1.0)
    { R=0.0;
    }
    else
    {
      W  = 0.0022 - 0.0002*m;
      m2 = m*0.9853 + 0.1978;

      if (m > 3.0)
      { alphaQ =  5.0944 + 0.1204*m;
        betaQ  = 68.1137 - 7.6251*m;
      }
      else
      { alphaQ = m;
        betaQ  = 0.0;
      }
      T = 0.5 * (1.0 - tanh((Q - m2*QcNi) / W));
      S = (1.0 - alphaQ * (Q-Qc) + betaQ * sq(Q-Qc));
	    R = R0 * S * T ;
    }
  }

  if (bPrint)
    sprintf(sText, "R(Q) = %5.3f * (1 - %5.3f*(Q-Qc) + %6.3f*(Q-Qc)^2) * 0.5*(1 - tanh((Q-%5.3f*Qc)/%7.5f)) ,   Qc=%7.5f 1/Ang", R0, alphaQ,betaQ, m2,W, Qc);

  return R;
}

// Description of a typical reflectivity curve (averaged over all companies) 
double ReflTypicalT(char* sText, const double Q, const double m, const short bPrint)
{
  double R    = 0.0,
         R0   = 0.995,
         W    = 0.00157,
         mReal= m + 0.14,
         Rcut = Min(R0, 1.096 - 0.0758*m);

  R = ReflMirrT(sText, Q, mReal, R0, Rcut, W, QC_NI, bPrint);

  return R;
}

double ReflTypical(const double Q, const double m)
{
  short  bPrint=FALSE;
  char*  sText = NULL;
  double mReal,
         R    = 0.0,
         R0   = 0.995,
         W    = 0.00157,
         Rcut = Min(R0, 1.096 - 0.0758*m);

  if (m > 1.25)           
    mReal = m + 0.14;     // supermirror coatings have a higher m value than the nomimal value
  else
    mReal = m;            // for Ni, Ni58 etc, the nominal value should be used

  R = ReflMirrT(sText, Q, mReal, R0, Rcut, W, QC_NI, FALSE);

  return R;
}

double ReflMirrT(char* sText, const double Q, const double m, const double R0, const double Rm, const double W, const double Qc, const short bPrint)
{
  char   sSlope[50]="", sDecay[50]="";
  double R=0.0, S=1.0, T=1.0,
         Qcm  = m * QC_NI,            // Q-value, where the reflectivity drops to zero
         Qc1  = Min(Qc, Qcm),         // Q-value, where R=R0 ends
         alpha= 0.0;

  if (m > 1.0)
  { 
    alpha = (R0 - Rm)/(m - 1.0)/QC_NI;
    S = Min(1.0, 1.0 - alpha*(Q - Qc1));
  }

  if (W > 0.0)
  {
    T = 0.5 * (1.0 - tanh((Q - Qcm)/W));
  }
  else
  { if (Q > Qcm) T = 0.0;
    else         T = 1.0;
  }

  R = R0 * S * T ;

  if (bPrint)
  { 
    if (m > 1.0) sprintf(sSlope, "* (1 - %5.3f*(Q-Qc))", alpha);
    if (W > 0.0) sprintf(sDecay, "* 0.5*(1 - tanh((Q-%5.3f*Qc)/%7.5f))", m, W);
    sprintf(sText,  "R(Q) = %5.3f %s %s   Rm=%5.3f  Qc=%7.5f 1/Ang", R0, sSlope, sDecay, Rm,Qc);
  }

  return R;
}


// Loads Reflectivity data from a file where R(Q) is given. Give pReflFile as input
// ----------------------------------------------------------
int ReadRofQ(FILE* pReflFile, double* aQ, double* aR) 
{
  char   sBuffer[CHAR_BUF_SMALL]="";
  int    k, nLines;           // index and number of lines

  nLines = LinesInFile(pReflFile);

  for (k=1; k<=nLines && k<ROFQ_MAX; k++)
  {
    ReadLine(pReflFile, sBuffer, sizeof(sBuffer)-1);
    sscanf(sBuffer, "%lf %lf", &aQ[k], &aR[k]);  
  }

  aQ[0]=0.0; 
  aR[0]=aR[1];

  return (nLines+1);
}


// Determines the number of points in a Vitess reflectivity array up to Qmax
// -------------------------------------------------------------------------
int NumDataPtsQ(const double Qmax)
{
  double ThetaMax = ReflAngle(1.0, Qmax);
  int    nPts     = (int)(ceil(1000*ThetaMax)) + 4;

  return(nPts);
}

int NumDataPtsM(const double m, const double Qc, const double W)
{
  double ThetaC, ThetaW, ThetaM;
  int    nPts=0;

  ThetaC = ReflAngle(1.0, Qc);
	ThetaW = ReflAngle(1.0, W);
	ThetaM = m * ThetaC;
	// nPts   = (int) ((Max(ThetaM,ThetaC) + 6.0*ThetaW)*1000) + 4;
	nPts   = (int) (ceil((ThetaM + 6.0*ThetaW)*1000)) + 4;

  return nPts;
}


// Fills Vitess reflectivity array from R(Q) data
// ----------------------------------------------
// pReflDat: pointer to array of reflectivity data for 1 Ang in steps of 0.001 deg
// aQ, aR  : pointers to arrays Q and reflecitvity R as read from 2 column file R(Q)
// nVals   : length of Q and R array
// --------------------------------------------------------------------------------------
void SetReflData(double* pReflDat, const double* aQ, const double* aR, const int nVals)
{
  int    j=0;   // index for Vitess reflectivity file  
  double theta,        // reflection angle
         Q;            // Q-value of the reflection angle for 1 Ang
  long   nArrayLen=NumDataPtsQ(aQ[nVals-1]);

  for (j=0; j < nArrayLen; j++)
  { 
    theta = j / 1000.0;
    Q     = QbyRefl(1.0, theta);
    pReflDat[j]= InterpolQ(Q, aQ, aR, nVals);
  }
}

double InterpolM(const double m, const double* aM, const double* aR, const int nVals)
{
  double R=0.0;

  for (int k=0; k < nVals-1; k++)
  {
    if (aM[k] <= m && aM[k+1] > m)
      R = aR[k] + (aR[k+1] - aR[k])/(aM[k+1] - aM[k]) * (m - aM[k]);   
  }

  return R;
}

double InterpolQ(const double Q, const double* aQ, const double* aR, const int nVals)
{
  double R=0.0;

  for (int k=0; k < nVals-1; k++)
  {
    if (aQ[k] <= Q && aQ[k+1] > Q)
      R = aR[k] + (aR[k+1] - aR[k])/(aQ[k+1] - aQ[k]) * (Q - aQ[k]);   
  }

  return R;
}

double ReflInterpol(const double Lambda, const double Angle, const double* Rdata, long MaxData)
{
  long   iw1;
  double w,         // angle/wavelength
         R=0.0;     // reflectivity

  w   = Angle*1000.0 / Lambda;

#ifdef FAST_SIM
  iw1 = (long) floor(w+0.5);
  R = Rdata[iw1];
#else
  iw1 = (long) floor(w);

  if ((iw1+1) < MaxData)
    R = Rdata[iw1] + (Rdata[iw1+1] - Rdata[iw1]) * (w - iw1);
#endif

  return(R);
}



/****************************************************************************************/
/*  Vector Functions                                                                    */
/****************************************************************************************/

/* sets vector to zero */
void InitVector(VectorType Vector)
{
  Vector[0]=0.0;
  Vector[1]=0.0;
  Vector[2]=0.0;
}

/* 'Copy' copies the contents of Vector 'Src' to vector 'Dest'  */
void CopyVector(const VectorType Src, VectorType Dest)
{
    Dest[0] = Src[0];
    Dest[1] = Src[1];
    Dest[2] = Src[2];
}


/* 'MAXV' returns the number of the largest component of 'Vector': 0, 1 or 2  */
long MAXV(const VectorType Vector)
{
  if( (fabs(Vector[0]) > fabs(Vector[1])) && (fabs(Vector[0]) > fabs(Vector[2])))
    return 0;
  if(fabs(Vector[1]) > fabs(Vector[2]))
    return 1;
  return 2;
}


/* 'LengthVector' returns the length of vector 'Vec'  */
double LengthVector(const VectorType Vec)
{
  //return sqrt(ScalarProduct(Vec,Vec));

  return sqrt(Vec[0]*Vec[0] + Vec[1]*Vec[1] + Vec[2]*Vec[2]);

}


/* 'NormVector' changes the vector length to 1  */
short NormVector(VectorType Vector)
{
  long   i;
  double dLen = LengthVector(Vector);

  if (dLen==0.0)
    return FALSE;

  for(i=0;i<3;i++)
    Vector[i] /= dLen;

  return TRUE;
}


/* 'DistVector' calculates the distance between the points described by Vec1 and Vec2  */
double DistVector(const VectorType Vec1, const VectorType Vec2)
{
  VectorType Vhlp;
  
  CopyVector(Vec1, Vhlp) ;
  SubVector (Vhlp, Vec2);
  return LengthVector(Vhlp);
}


/* 'AddVector' adds 'Add' to 'Value' and returns 'Value'  */
void AddVector(VectorType Value, const VectorType Add)
{
  int i ;
  for (i=0;i<3;i++)
    Value[i] += Add[i] ;
}


/* 'SubVector' Substracts 'Sub' from 'Value' and returns 'Value' */
void SubVector(VectorType Value, const VectorType Sub)
{
  int i ;
  for(i=0;i<3;i++)
    Value[i] -=  Sub[i];
}


/* 'MultiplyByScalar' multiplies a vector by a scalar */
void MultiplyByScalar(VectorType Vector, const double Scalar)
{
  int i;
  for (i=0;i<3;i++)
    Vector[i] *= Scalar;
}


/* 'ScalarProduct' calculates the scalar product of two vectors 'v1' and 'v2' */
double ScalarProduct(const VectorType v1, const VectorType v2)
{
  return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

/* angle between two vectors in degs */
double AngleVectors(const VectorType v1,  const VectorType v2)
{
  double theta;

  theta = ScalarProduct(v1, v2) / sqrt(ScalarProduct(v1, v1) * ScalarProduct(v2, v2)) ;
  return 180./M_PI * acos(theta) ;
}

/* area of triangle from two vectors, G.Zs */
double Area(const VectorType v1, const VectorType v2)
{
  double lv = LengthVector(v1) * LengthVector(v2);
  return lv * fabs(sin(acos( ScalarProduct(v1, v2) / lv)) /2.);

  //return LengthVector(v1) * LengthVector(v2) *
  //		fabs(sin(acos( ScalarProduct(v1, v2)/(LengthVector(v1) * LengthVector(v2)))) /2.);

}


/* sets plane to zero */
void InitPlane(Plane* pPlane)
{
  pPlane->A=0.0;
  pPlane->B=0.0;
  pPlane->C=0.0;
  pPlane->D=0.0;
}


/****************************************************************************************/
/*  Basic Matrix Functions                                                              */
/****************************************************************************************/

/* sets 3 x 3 matrix to zero */
void Init3x3Matrix(double Matrix[3][3])
{
  int i,j;

  for (i=0; i < 3; i++)
  { for (j=0; j < 3; j++)
      Matrix[i][j] = 0.0;  
  }
}

/* 'RotVector' does essentially a Vector times matrix multiplication   */
/* in order to rotate the Vector. The rotation Matrix may be supplied  */
/* by e.g. 'RotMatrixX'                                                */
/* Author: F. Streffer.                                                */
void RotVector(double RotMatrix[3][3], VectorType Vector)
{
  VectorType TempVec;
  int        i;

  for(i=0;i<3;i++)
    TempVec[i] = ScalarProduct(RotMatrix[i],Vector);
  CopyVector(TempVec, Vector);
}

/* 'RotBackVector' rotates a vector, by multiplication of the Vector  */
/* with the invers of RotMatrix. E.g if RotMatrix is the same as in   */
/* 'RotVector' and is applied to the result of RotVector the original */
/* vector is restored.   (Remember det(RotMatrix)=1)                  */
/* Author: F. Streffer.                                               */
void RotBackVector(double RotMatrix[3][3], VectorType Vector)
{
  VectorType TempVec;
  int        i;

  for(i=0;i<3;i++)
    TempVec[i]=RotMatrix[0][i]*Vector[0]+RotMatrix[1][i]*Vector[1]+RotMatrix[2][i]*Vector[2];
  CopyVector(TempVec, Vector);
}

/* 'FillRMatrixZY' calculates a rotation matrix, which rotates a frame   */
/* at first about the z-axis by 'rotz' and then about the y-axis by 'roty' */
/*  Author: F. Streffer.                                                   */
/*  Change: G. Zs. 16 JUL 2002  rotation convention                        */
void FillRMatrixZY(double RotMatrix[3][3], const double roty, const double rotz)
{
  double sz, cz, sy, cy;
  long   i,j;

  sy= (double) sin(roty);
  cy= (double) cos(roty);
  sz= (double) sin(rotz);
  cz= (double) cos(rotz);

  /* now, fill the matrix */
  RotMatrix[0][0] =  cy*cz;
  RotMatrix[0][1] =  cy*sz;
  RotMatrix[0][2] =  sy;
  RotMatrix[1][0] = -sz;
  RotMatrix[1][1] =  cz;
  RotMatrix[1][2] =  0.0;
  RotMatrix[2][0] = -sy*cz;
  RotMatrix[2][1] = -sy*sz;
  RotMatrix[2][2] =  cy;

  /* cut off very small matrix elements */
  for(i=0; i<3; i++)
    for(j=0; j<3; j++)
      if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}


/* 'CartesianToEulerZY' calculates Euler angles 'rotz' and 'roty'         */
/* to transfer the x-axis to 'Vector' by rotation around y- and z-axis ZY */
/* (cf. FillRotMatrixZY)                                                  */
/*  Author: G. Zsigmond                                                   */
void CartesianToEulerZY(VectorType Vector, double *roty, double *rotz)
{
  *rotz = (double) atan2( Vector[1] , Vector[0] ) ;

  *roty = (double) atan2( Vector[2] , ((double) cos(*rotz) * Vector[0] + (double) sin(*rotz) * Vector[1]) ) ;
}

/* Euler to cartesian - invers of previous                            */
/*  Author: G. Zsigmond                                               */
void EulerToCartesianZY(VectorType Vector, double *roty, double *rotz)
{
  double cos_roty = cos(*roty);
  Vector[0]= (double) cos_roty * (double) cos(*rotz) ;
  Vector[1]= (double) cos_roty * (double) sin(*rotz) ;
  Vector[2]= (double) sin(*roty) ;
}


/****************************************************************************************/
/*  General I/O Functions                                                               */
/****************************************************************************************/

/* fileOpen open file 'name' and gives pointer back
   in case of an opening error, a message is written to the LogFile */
FILE * fileOpen(const char *name, const char *mode)
{
  FILE *f;

  if (! (f = fopen(name, mode))) {
    fprintf(LogFilePtr, "ERROR: Can't open file %s!\n", name);
    exit(-1);
  }
  return f;
}

FILE * fileOpen2(const char* sName, const char* sMode, const char* sContent)
{
  FILE* fp;

  if (! (fp = fopen(sName, sMode))) 
  {
    fprintf(LogFilePtr, "ERROR: Can't open file %s containing %s!\n", sName, sContent);
    exit(-1);
  }
  return fp;
}


void Error(const char *text)
{
  fprintf(LogFilePtr, "ERROR: %s!\n", text);
  exit(-1);
}

void Warning(const char *text)
{
  fprintf(LogFilePtr, "WARNING: %s!\n", text);
}

void Note(const char *text)
{
  fprintf(LogFilePtr, "NOTE: %s!\n", text);
}


void Abort()
{
  exit(-1);
}


/* Wait(time)
   remains 'time' sec in this function
*/
void Wait(float WaitTime)
{
  int   c1, c2;
  float DelT;  // time in sec

  c1=clock();

  do
  { c2=clock();
    DelT = ((float)(c2-c1))/CLOCKS_PER_SEC;
  }
  while (DelT < WaitTime);

  return;
}


/****************************************************************************************/
/*  Functions for Reading of Input Data                                                 */
/****************************************************************************************/

/*  LinesInFile(FILE *pIn) counts the number of lines in a text file and rewinds it  */
long LinesInFile(FILE *pIn)
{
  char Buffer[CHAR_BUF_LARGE]="";
  long NumLines=0;

  rewind(pIn);
  if (pIn!=NULL)
  { while (ReadLine(pIn, Buffer, sizeof(Buffer)-1))
      NumLines++;
    rewind(pIn);
  }
  return NumLines;
}


/* ColumnsInFile(FILE *pIn) counts the number of columns in a text file by analyzing the line found using 'ReadLine()' and rewinds it  */
long ColumnsInFile(FILE* pFile)
{
  int i,v, nLns, isin;
  char buf[CHAR_BUF_LARGE];
  if (pFile == NULL)
    return 0;
  ReadLine(pFile, buf, CHAR_BUF_LARGE-1);
  rewind(pFile);
  for (nLns=isin=i=0; (v = buf[i]); i++)
    if (v != ' ')
      isin = 1;
    else if (isin) {
      nLns++;
      isin = 0;
    }
  if (isin)
    nLns++;

  return nLns;
}


/* ReadLine() reads the next line from the file 'pFile' into string 'pLine' that is not empty and not a comment line (beginning with #)
	  returning TRUE if a line is found and FALSE otherwise
   it strips comments at the end, leading and succeeding blanks, line feeds, tabs and cr
   the maximal number of characters in the string must be given in 'nStrLen'   */
int ReadLine(FILE* pFile, char* pLine, int nStrLen) 
{
  if (pFile)
    while (fgets (pLine, nStrLen, pFile)) {
      int v, k, kanf, kmax;

      /* substitute line feeds, tabs and carriage returns with blanks */
      for (k=0; (v = pLine[k]) && v != '#'; k++) {
	if (v=='\n' || v=='\t' || v=='\r')
	  pLine[k] = ' ';
      }
      if (k <= 0) continue;

      /* strip the comments and leading and succeeding blanks */
      for (kanf = 0; pLine[kanf] == ' '; kanf++) ;
      for (kmax = k-1; kmax >= kanf && pLine[kmax] == ' '; kmax--) ;
      if (kmax < kanf) continue;
      if (kanf == 0) {
	pLine[kmax+1] = 0;
      } else {
	for (k = 0; kanf <= kmax; k++, kanf++)
	  pLine[k] = pLine[kanf];
	pLine[k] = 0;
      }
      return TRUE;
    }

  *pLine = 0;
  return FALSE;
}


/*  ReadParString(FILE *fpt) reads one string value from parameter file */
void ReadParString(FILE *fpt, char *stringvar)
{
  fscanf(fpt,"%s", stringvar) ;
}


/*  ReadParF(FILE *fpt) reads one double value from parameter file */
double ReadParF(FILE *fpt)
{
  double value;
  return 1 == fscanf (fpt, "%lf", &value) ? value : 0.;
}


/*  ReadParI(FILE *fpt) reads one integer value from parameter file */
int ReadParI(FILE *fpt)
{
  int value;
  return fscanf(fpt,"%d", &value) == 1 ? value : 0;
}


/* ReadParComment(FILE *fpt) reads comment line */
void ReadParComment(FILE *fpt)
{
  char comment[100];
  fgets(comment, 100, fpt);
}


/**********************************************************/
/*  String Operations                                     */
/**********************************************************/

/* Copy 'nLen' bytes of 'sOrigin' into the new string 'sCopy' */
void StrgCopy(char* sCopy, const char* sOrigin, int nLen)
{
	strncpy(sCopy, sOrigin, nLen);
	sCopy[nLen]='\0';
}


/* Shift string 'sStr' 'kWidth' bytes to the left */
void StrgLShift(char* sStr, int kWidth)
{
	int k, ke;

	ke = strlen(sStr) - kWidth;

	for (k=0; k <= ke; k++)
		sStr[k] = sStr[k+kWidth];
}


/* Scan string 'sStr' and copy all values (but maximally 'nMax')
   to list 'pTab' of double values,  beginning with value number 'nStart'*/
long StrgScanLF(const char* sStr, double* pTab, const int nMax, const int nStart)
{
	int k, n=0;
	const char *pStr;
	char sNumber[31];

	pStr = sStr;
	n   -= nStart;
	do
	{	/* search of beginning and end of 1st number of (remaining) string */
		k=0;
		/* step forward until first number or control character */
		while (isdigit(pStr[k])==0 && iscntrl(pStr[k])==0)
			k++;
		/* step forward until space-like or control character */
		while (isspace(pStr[k])==0 && iscntrl(pStr[k])==0)
			k++;

		/* separating first number and adding it to the list */
		if (k > 0)
		{
			StrgCopy(sNumber, pStr, k);
			if (n >= 0)
				pTab[n] = atof(sNumber);
			n++;
			pStr += k;
		}
	}
	while (n < nMax && k > 0);

	return(n);
}


/**************************************************************/
/* Change of the Slashes to the right ones, e.g. '\' to '/'   */
/**************************************************************/
void ChangeSlash(char* pStr)
{
	int k, kLen;

	kLen = strlen(pStr);
	for (k=0; k < kLen; k++)
	{	if (pStr[k]=='/' || pStr[k]=='\\')
			pStr[k]=cSlash;
	}
}

void AddSlash(char* pStr)
{
  int kLen = strlen(pStr);

  if (pStr[kLen-1]!=cSlash)
  { pStr[kLen-1]=cSlash;
    pStr[kLen]  ='\0';
  }

}