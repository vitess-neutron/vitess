/********************************************************************************************/
/*  VITESS module 'convert.c'                                                               */
/*    conversions from text to enum and back                                                */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/*                                                                                          */
/* 1.0   Apr 2021  K. Lieutenant  initial version                                           */
/********************************************************************************************/

#include "general.h"
#include "convert.h"

// #define CONV_ID2TXT(a)  {if (eID==a) strcpy(sText,"a");}
// #define CONV_TXT2ID(a)  {if (strcmp(sText,"a")==0) return a;}

// GENERAL
// ------

// Distribution function  (monochromator, sample_singlecryst)
void     RndGen_ID2Txt(char* sText, const VtRndGen eID)
{
  switch (eID)
  {
    case VT_RAN3   : strcpy(sText, "ran3"   ); break;
    case VT_TAUS   : strcpy(sText, "taus"   ); break;
    case VT_GFSR4  : strcpy(sText, "gfsr4"  ); break;
    case VT_MT19937: strcpy(sText, "mt19937"); break;
    case VT_RANLUX : strcpy(sText, "ranlux" ); break;
    default        : strcpy(sText, "");
  }
}
VtRndGen RndGen_Txt2ID(const char* sText)
{
  VtRndGen eID=VT_RAN3;

       if (strcmp(sText, "ran3"   )==0) eID=VT_RAN3   ;
  else if (strcmp(sText, "taus"   )==0) eID=VT_TAUS   ;
  else if (strcmp(sText, "gfsr4"  )==0) eID=VT_GFSR4  ;
  else if (strcmp(sText, "mt19937")==0) eID=VT_MT19937;
  else if (strcmp(sText, "ranlux" )==0) eID=VT_RANLUX ;
  
  return eID;
}

// Module ID      (all modules, internal)
void     CompID2Name (char* sCompName, const McCompID eComp)
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
	  case MCN_GRID         : strcpy(sCompName, "Grid");              break;       
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
	  case MCN_GUIDE_IDEAL  : strcpy(sCompName, "GuideIdeal");        break;            
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
	  case MCN_CAPTURE      : strcpy(sCompName, "CaptureFlux");       break;           
	  case MCN_BEAMSTOP     : strcpy(sCompName, "BeamStop");          break;         
	  case MCN_SMPL_ENVIRO  : strcpy(sCompName, "SampleEnvironment"); break;
	  case MCN_DETECTOR     : strcpy(sCompName, "Detector");          break;         
	  case MCN_WRITEOUT     : strcpy(sCompName, "EventsOut");         break;        
	  case MCN_SMPL_EL_ISO  : strcpy(sCompName, "SampleElasticIsotr");break;
	  case MCN_SMPL_INELAST : strcpy(sCompName, "SampleInelastic");   break;  
	  case MCN_SMPL_SNGL_X  : strcpy(sCompName, "SampleSnglCrystal"); break; 
	  case MCN_SMPL_POWDER  : strcpy(sCompName, "SamplePowder");      break;     
	  case MCN_SMPL_S_Q     : strcpy(sCompName, "SampleSofQ");        break;       
	  case MCN_SMPL_NXS     : strcpy(sCompName, "SampleNXS");         break;        
	  case MCN_SMPL_SANS    : strcpy(sCompName, "SampleSANS");        break;       
	  case MCN_SMPL_REFL    : strcpy(sCompName, "SampleReflect");     break;    
	  case MCN_FRAME        : strcpy(sCompName, "Frame");             break;            
	  case MCN_FILTER       : strcpy(sCompName, "Filter");            break;           
	  case MCN_FILTER2D     : strcpy(sCompName, "Filter2D");          break;           
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
    case MCN_EVAL1_SANS   : strcpy(sCompName, "Eval1D_SANS");       break; 
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
  else if (strcmp(sCompName, "Grid"))              eComp=MCN_GRID         ;       
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
  else if (strcmp(sCompName, "GuideIdeal"))        eComp=MCN_GUIDE_IDEAL  ;            
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
  else if (strcmp(sCompName, "CaptureFlux"))       eComp=MCN_CAPTURE      ;           
  else if (strcmp(sCompName, "BeamStop"))          eComp=MCN_BEAMSTOP     ;         
  else if (strcmp(sCompName, "SampleEnvironment")) eComp=MCN_SMPL_ENVIRO  ;
  else if (strcmp(sCompName, "Detector"))          eComp=MCN_DETECTOR     ;         
  else if (strcmp(sCompName, "EventsOut"))         eComp=MCN_WRITEOUT     ;        
  else if (strcmp(sCompName, "SampleElasticIsotr"))eComp=MCN_SMPL_EL_ISO  ;
  else if (strcmp(sCompName, "SampleInelastic"))   eComp=MCN_SMPL_INELAST ; 
  else if (strcmp(sCompName, "SampleSnglCrystal")) eComp=MCN_SMPL_SNGL_X  ; 
  else if (strcmp(sCompName, "SamplePowder"))      eComp=MCN_SMPL_POWDER  ; 
  else if (strcmp(sCompName, "SampleSofQ"))        eComp=MCN_SMPL_S_Q     ; 
  else if (strcmp(sCompName, "SampleNXS"))         eComp=MCN_SMPL_NXS     ; 
  else if (strcmp(sCompName, "SampleSANS"))        eComp=MCN_SMPL_SANS    ;
  else if (strcmp(sCompName, "SampleReflect"))     eComp=MCN_SMPL_REFL    ; 
  else if (strcmp(sCompName, "Frame"))             eComp=MCN_FRAME        ; 
  else if (strcmp(sCompName, "Filter"))            eComp=MCN_FILTER       ; 
  else if (strcmp(sCompName, "Filter2D"))          eComp=MCN_FILTER2D     ; 
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

// reason for writing interaction point (all modules, internal)
void     Reason_ID2Txt(char* sText, const VtReason eID)
{
  switch (eID)
  {
    case VT_CREATED   : strcpy(sText, "created"      ); break;
    case VT_OUTSIDE   : strcpy(sText, "outside"      ); break;
    case VT_OUT_OF_WND: strcpy(sText, "out of window"); break;
    case VT_PASSED    : strcpy(sText, "passed"       ); break;
    case VT_ENTERED   : strcpy(sText, "entered"      ); break;
    case VT_TRANSIT   : strcpy(sText, "transited"    ); break;
    case VT_REFLECTED : strcpy(sText, "reflected"    ); break;
    case VT_SCATTERED : strcpy(sText, "scattered"    ); break;
    case VT_ABSORBED  : strcpy(sText, "absorbed"     ); break;
    case VT_EXITED    : strcpy(sText, "exited"       ); break;
    case VT_DETECTED  : strcpy(sText, "detected"     ); break;
    default  : strcpy(sText, "");
  }
}
VtReason Reason_Txt2ID(const char* sText)
{
  VtReason eID=VT_NO_REASON;

       if (strcmp(sText, "created"      )==0) eID=VT_CREATED   ;
  else if (strcmp(sText, "outside"      )==0) eID=VT_OUTSIDE   ;
  else if (strcmp(sText, "out of window")==0) eID=VT_OUT_OF_WND;
  else if (strcmp(sText, "passed"       )==0) eID=VT_PASSED    ;
  else if (strcmp(sText, "entered"      )==0) eID=VT_ENTERED   ;
  else if (strcmp(sText, "transited"    )==0) eID=VT_TRANSIT   ;
  else if (strcmp(sText, "reflected"    )==0) eID=VT_REFLECTED ;
  else if (strcmp(sText, "scattered"    )==0) eID=VT_SCATTERED ;
  else if (strcmp(sText, "absorbed"     )==0) eID=VT_ABSORBED  ;
  else if (strcmp(sText, "exited"       )==0) eID=VT_EXITED    ;
  else if (strcmp(sText, "detected"     )==0) eID=VT_DETECTED  ;
  
  return eID;
}

// directory type (all modules, internal)
void      DirType_ID2Txt(char* sText, const VtDirType eID)
{
  switch (eID)
  {
    case PAR_DIR  : strcpy(sText, "parameter directory"   ); break;
    case INSTL_DIR: strcpy(sText, "installation directory"); break;
    case IN_DIR   : strcpy(sText, "input directory"       ); break;
    case OUT_DIR  : strcpy(sText, "output directory"      ); break;
    default  : strcpy(sText, "");
  }
}
VtDirType DirType_Txt2ID(const char* sText)
{
  VtDirType eID=NO_DIR;

       if (strcmp(sText, "parameter directory"   )==0) eID=PAR_DIR  ;
  else if (strcmp(sText, "installation directory")==0) eID=INSTL_DIR;
  else if (strcmp(sText, "input directory"       )==0) eID=IN_DIR   ;
  else if (strcmp(sText, "output directory"      )==0) eID=OUT_DIR  ;
  
  return eID;
}

// Axis  (monitors, evaluation, detector, sm_ensemble)
void     Axis_ID2Txt(char* sText, const VtAxis eID)
{
  switch (eID)
  {
    case X_AXIS: strcpy(sText, "x"); break;
    case Y_AXIS: strcpy(sText, "y"); break;
    case Z_AXIS: strcpy(sText, "z"); break;
    default     : strcpy(sText, "none");
  }
}
VtAxis   Axis_Txt2ID(const char* sText)
{
  VtAxis eID=NO_AXIS;

       if (strcmp(sText, "X")==0 || strcmp(sText, "x")==0) eID=X_AXIS;
  else if (strcmp(sText, "Y")==0 || strcmp(sText, "y")==0) eID=Y_AXIS;
  else if (strcmp(sText, "Z")==0 || strcmp(sText, "z")==0) eID=Z_AXIS;
  
  return eID;
}

// Rotation Axis             (sample_refl)
void      RotAxis_ID2Txt(char* sText, const VtRotAxis eID)
{
  switch (eID)
  {
    case VT_ROT_X: strcpy(sText, "x"); break;
    case VT_ROT_Y: strcpy(sText, "y"); break;
    case VT_ROT_Z: strcpy(sText, "z"); break;
    default    : strcpy(sText, "");
  }
}
VtRotAxis RotAxis_Txt2ID(const char* sText)
{
  VtRotAxis eID=NO_ROT_AX;

       if (strcmp(sText, "X")==0 || strcmp(sText, "x")==0) eID=VT_ROT_X;
  else if (strcmp(sText, "Y")==0 || strcmp(sText, "y")==0) eID=VT_ROT_Y;
  else if (strcmp(sText, "Z")==0 || strcmp(sText, "z")==0) eID=VT_ROT_Z;
  
  return eID;
}

// Orientation   (detector, mon2_wldiv, mon2_posdiv)
void     Orient_ID2Txt(char* sText, const VtOrient eID)
{
  switch (eID)
  {
    case HORIZONTAL: strcpy(sText, "horizontal"); break;
    case VERTICAL  : strcpy(sText, "vertical"  ); break;
    default        : strcpy(sText, "");
  }
}
VtOrient Orient_Txt2ID(const char* sText)
{
  VtOrient eID=NO_ORIENT;

       if (strcmp(sText, "horizontal")==0) eID=HORIZONTAL;
  else if (strcmp(sText, "vertical"  )==0) eID=VERTICAL  ;
  
  return eID;
}

// Direction in - out  (sample, sample environment)
void     DirInOut_ID2Txt(char* sText, const VtDir eID)
{
  switch (eID)
  {
    case VT_IN    : strcpy(sText, "in"    ); break;
    case VT_OUT   : strcpy(sText, "out"   ); break;
    case VT_INSIDE: strcpy(sText, "inside"); break;
    default       : strcpy(sText, "");
  }
}
VtDir DirInOut_Txt2ID(const char* sText)
{
  VtDir eID=VT_NO_DIR;

       if (strcmp(sText, "in"    )==0) eID=VT_IN    ;
  else if (strcmp(sText, "out"   )==0) eID=VT_OUT   ;
  else if (strcmp(sText, "inside")==0) eID=VT_INSIDE;
  
  return eID;
}

// Frame generation
void     FrameGen_ID2Txt(char* sText, const VtFrameGen eID)
{
  switch (eID)
  {
    case VT_FRAME_STD : strcpy(sText, "standard frame generation"); break;
    case VT_FRAME_USER: strcpy(sText, "user defined frame"       ); break;
    default        : strcpy(sText, "");
  }
}
VtFrameGen FrameGen_Txt2ID(const char* sText)
{
  VtFrameGen eID=VT_NO_FRAME;

       if (strcmp(sText, "standard frame generation")==0) eID=VT_FRAME_STD ;
  else if (strcmp(sText, "user defined frame"       )==0) eID=VT_FRAME_USER;
  
  return eID;
}

// Shape         (spacewindow)
void     Shape_ID2Txt(char* sText, const VtShape eID)
{
  switch (eID)
  {
    case VT_SQUARE: strcpy(sText, "rectangular"); break;
    case VT_CIRCLE: strcpy(sText, "circular"   ); break;
    default       : strcpy(sText, "");
  }
}
VtShape  Shape_Txt2ID(const char* sText)
{
  VtShape eID=VT_NO_SHAPE;

       if (strcmp(sText, "rectangular")==0) eID=VT_SQUARE;
  else if (strcmp(sText, "circular"   )==0) eID=VT_CIRCLE;
  
  return eID;
}

// Component Status  (foreseen for detector, monochromator, and all that can be de-activated like writeout in vsn. 3)
void      CompAct_ID2Txt(char* sText, const VtCompAct eID)
{
  switch (eID)
  {
    case INACTIVE   : strcpy(sText, "inactive"   ); break;
    case ACTIVE     : strcpy(sText, "active"     ); break;
    case FIRST_PART : strcpy(sText, "first part" ); break;
    case MIDDLE_PART: strcpy(sText, "middle part"); break;
    case LAST_PART  : strcpy(sText, "last part"  ); break;
    default         : strcpy(sText, ""           );
  }
}
VtCompAct CompAct_Txt2ID(const char* sText)
{
  VtCompAct eID=NOT_EXIST;

       if (strcmp(sText, "inactive"   )==0) eID=INACTIVE  ;
  else if (strcmp(sText, "active"     )==0) eID=ACTIVE    ;
  else if (strcmp(sText, "first part" )==0) eID=FIRST_PART ;
  else if (strcmp(sText, "middle part")==0) eID=MIDDLE_PART;
  else if (strcmp(sText, "last part"  )==0) eID=LAST_PART  ;
 
  return eID;
}

// Distribution function  (monochromator, sample_singlecryst)
void     Distr_ID2Txt(char* sText, const VtDistr eID)
{
  switch (eID)
  {
    case LORENTZIAN: strcpy(sText, "Lorentzian"); break;
    case GAUSSIAN  : strcpy(sText, "Gaussian"  ); break;
    default        : strcpy(sText, "");
  }
}
VtDistr  Distr_Txt2ID(const char* sText)
{
  VtDistr eID=LORENTZIAN;

       if (strcmp(sText, "Lorentzian")==0) eID=LORENTZIAN;
  else if (strcmp(sText, "Gaussian"  )==0) eID=GAUSSIAN  ;
  
  return eID;
}

// Absorbing material
void       AbsMat_ID2Txt(char* sText, const VtAbsMat eID)
{
  switch (eID)
  {
    case VT_NO_MAT   : strcpy(sText, "none"          ); break;
    case VT_ABS_IDEAL: strcpy(sText, "ideal absorber"); break;
    case VT_ABS_GD   : strcpy(sText, "Gadolinium"    ); break;
    case VT_ABS_B10  : strcpy(sText, "Bor-10"        ); break;
    case VT_ABS_FILE : strcpy(sText, "from file"     ); break;
    default          : strcpy(sText, "");
  }
}
VtAbsMat   AbsMat_Txt2ID(const char* sText)
{
  VtAbsMat eID=VT_NO_MAT;

       if (strcmp(sText, "ideal"         )==0) eID=VT_ABS_IDEAL;
  else if (strcmp(sText, "ideal absorber")==0) eID=VT_ABS_IDEAL;
  else if (strcmp(sText, "Gd"            )==0) eID=VT_ABS_GD   ;
  else if (strcmp(sText, "gadolinium"    )==0) eID=VT_ABS_GD   ;
  else if (strcmp(sText, "Bor"           )==0) eID=VT_ABS_B10  ;
  else if (strcmp(sText, "bor10"         )==0) eID=VT_ABS_B10  ;
  else if (strcmp(sText, "from file"     )==0) eID=VT_ABS_FILE ;
 
  return eID;
}

// Distribution function  (monochromator, sample_singlecryst)
void        InstGeom_ID2Txt(char* sText, const VtInstGeom eID)
{
  switch (eID)
  {
    case VT_DIRECT_GEOM: strcpy(sText, "direct geometry"  ); break;
    case VT_INVERT_GEOM: strcpy(sText, "inverted geometry"); break;
    default            : strcpy(sText, "no instrument geometry");
  }
}
VtInstGeom  InstGeom_Txt2ID(const char* sText)
{
  VtInstGeom eID=VT_NO_I_GEOM;

       if (strcmp(sText, "direct geometry"  )==0) eID=VT_DIRECT_GEOM;
  else if (strcmp(sText, "inverted geometry")==0) eID=VT_INVERT_GEOM;
  
  return eID;
}


// SOURCE + MODERATOR
// ------------------
// name defining the facility
void      SrcName_ID2Txt(char* sText, const VtSrcName eID)
{
  switch (eID)
  {
    case ESS : strcpy(sText, "ESS" ); break;
    case SNS : strcpy(sText, "SNS" ); break;
    case ISIS: strcpy(sText, "ISIS"); break;
    case CSNS: strcpy(sText, "CSNS"); break;
    case IPNS: strcpy(sText, "IPNS"); break;
    case HBS : strcpy(sText, "HBS" ); break;
    case ILL : strcpy(sText, "ILL" ); break;
    case HMI : strcpy(sText, "HMI" ); break;
    case FRM2: strcpy(sText, "FRM2"); break;
    default  : strcpy(sText, "-");
  }
}
VtSrcName SrcName_Txt2ID(const char* sText)
{
  VtSrcName eID=ANYSOURCE;

       if (strcmp(sText, "ESS" )==0) eID=ESS ;
  else if (strcmp(sText, "SNS" )==0) eID=SNS ;
  else if (strcmp(sText, "ISIS")==0) eID=ISIS;
  else if (strcmp(sText, "CSNS")==0) eID=CSNS;
  else if (strcmp(sText, "IPNS")==0) eID=IPNS;
  else if (strcmp(sText, "HBS" )==0) eID=HBS ;
  else if (strcmp(sText, "ILL" )==0) eID=ILL ;
  else if (strcmp(sText, "HMI" )==0) eID=HMI ;
  else if (strcmp(sText, "FRM2")==0) eID=FRM2;
  
  return eID;
}

// Kind of source: simple, ... pulsed ...   (Vitess 4)
void      SrcKind_ID2Txt(char* sText, const VtSrcKind eID)
{
  switch (eID)
  {
    case SRC_SIMPLE: strcpy(sText, "simple source"    ); break;
    case SRC_CWS   : strcpy(sText, "continuous source"); break;
    case SRC_PULSED: strcpy(sText, "pulsed source"    ); break;
    case SRC_ISIS  : strcpy(sText, "source ISIS"      ); break;
    case SRC_ESS   : strcpy(sText, "source ESS"       ); break;
    default       : strcpy(sText, "");
  }
}
VtSrcKind SrcKind_Txt2ID(const char* sText)
{
  VtSrcKind eID=NO_SRC_KIND;

       if (strcmp(sText, "simple source"    )==0) eID=SRC_SIMPLE;
  else if (strcmp(sText, "continuous source")==0) eID=SRC_CWS   ;
  else if (strcmp(sText, "pulsed source"    )==0) eID=SRC_PULSED;
  else if (strcmp(sText, "source ISIS"      )==0) eID=SRC_ISIS  ;
  else if (strcmp(sText, "source ESS"       )==0) eID=SRC_ESS   ;
  
  return eID;
}

// Type of source: CWS, SPSS, LPSS          (Vitess 3)
void      SrcType_ID2Txt(char* sText, const VtSrcType eID)
{
  switch (eID)
  {
    case CWS : strcpy(sText, "CWS" ); break;
    case SPSS: strcpy(sText, "SPSS"); break;
    case LPSS: strcpy(sText, "LPSS"); break;
    default  : strcpy(sText, "");
  }
}
VtSrcType SrcType_Txt2ID(const char* sText)
{
  VtSrcType eID=NO_TYPE;

       if (strcmp(sText, "CWS" )==0) eID=CWS ;
  else if (strcmp(sText, "SPSS")==0) eID=SPSS;
  else if (strcmp(sText, "LPSS")==0) eID=LPSS;
  
  return eID;
}

// Target station 1 or 2
void      TS_ID2Txt(char* sText, const VtTS eID)
{
  switch (eID)
  {
    case VT_TS_1: strcpy(sText, "TS 1"); break;
    case VT_TS_2: strcpy(sText, "TS 2"); break;
    default     : strcpy(sText, "");
  }
}
VtTS      TS_Txt2ID(const char* sText)
{
  VtTS eID=VT_NO_TS;

       if (strcmp(sText, "TS 1")==0) eID=VT_TS_1;
  else if (strcmp(sText, "TS 2")==0) eID=VT_TS_2;
  
  return eID;
}

// Type of moderator: coupled, ...
void      ModType_ID2Txt(char* sText, const VtModType eID)
{
  switch (eID)
  {
    case POISONED : strcpy(sText, "decoupled poisoned"  ); break;
    case DECOUPLED: strcpy(sText, "decoupled unpoisoned"); break;
    case COUPLED  : strcpy(sText, "coupled"             ); break;
    case MULT_SPEC: strcpy(sText, "multi-spectral"      ); break;
    default       : strcpy(sText, "");
  }
}
VtModType ModType_Txt2ID(const char* sText)
{
  VtModType eID=NO_MOD_TYPE;

       if (strcmp(sText, "decoupled poisoned"  )==0) eID=POISONED ;
  else if (strcmp(sText, "decoupled unpoisoned")==0) eID=DECOUPLED;
  else if (strcmp(sText, "coupled"             )==0) eID=COUPLED  ;
  else if (strcmp(sText, "multi-spectral"      )==0) eID=MULT_SPEC;
  
  return eID;
}

// moderator shape
void       ModShape_ID2Txt(char* sText, const VtModShape eID)
{
  switch (eID)
  {
    case VT_MOD_SQUARE: strcpy(sText, "rectangular"); break;
    case VT_MOD_CIRCLE: strcpy(sText, "circular"   ); break;
    default           : strcpy(sText, "");
  }
}
VtModShape ModShape_Txt2ID(const char* sText)
{
  VtModShape eID=VT_MOD_CIRCLE;

       if (strcmp(sText, "rectangular")==0) eID=VT_MOD_SQUARE;
  else if (strcmp(sText, "circular"   )==0) eID=VT_MOD_CIRCLE;
  
  return eID;
}

// definition of flight direction range 
void     Direct_ID2Txt(char* sText, const VtDirect eID)
{
  switch (eID)
  {
    case VT_DIVERGENCE: strcpy(sText, "by divergence"    ); break;
    case VT_REAL_WND  : strcpy(sText, "by real window"   ); break;
    case VT_VIRT_WND  : strcpy(sText, "by virtual window"); break;
    default        : strcpy(sText, "");
  }
}
VtDirect Direct_Txt2ID(const char* sText)
{
  VtDirect eID=VT_REAL_WND;

       if (strcmp(sText, "by divergence"    )==0) eID=VT_DIVERGENCE;
  else if (strcmp(sText, "by real window"   )==0) eID=VT_REAL_WND  ;
  else if (strcmp(sText, "by virtual window")==0) eID=VT_VIRT_WND  ;
  
  return eID;
}

// ESS version
void      ModVsn_ID2Txt(char* sText, const EssModVsn eID)
{
  switch (eID)
  {
    case MEZEI_2001      : strcpy(sText, "2001_Mezei"      ); break;
    case ZANINI_2012     : strcpy(sText, "2012_Zanini"     ); break;
    case SCHOENFELDT_2013: strcpy(sText, "2013_Schoenfeldt"); break;
    case VARHEIGHT_2013  : strcpy(sText, "2013_VarHeight"  ); break;
    case BUTTERFLY2_2015 : strcpy(sText, "2015_Butterfly2" ); break;
    case BUTTERFLY1_2016 : strcpy(sText, "2016_Butterfly1" ); break;
    default  : strcpy(sText, "");
  }
}
EssModVsn ModVsn_Txt2ID(const char* sText)
{
  EssModVsn eID=NO_VERSION;

       if (strcmp(sText, "2001_Mezei"      )==0) eID=MEZEI_2001      ;
  else if (strcmp(sText, "2012_Zanini"     )==0) eID=ZANINI_2012     ;
  else if (strcmp(sText, "2013_Schoenfeldt")==0) eID=SCHOENFELDT_2013;
  else if (strcmp(sText, "2013_VarHeight"  )==0) eID=VARHEIGHT_2013  ;
  else if (strcmp(sText, "2015_Butterfly2" )==0) eID=BUTTERFLY2_2015 ;
  else if (strcmp(sText, "2016_Butterfly1" )==0) eID=BUTTERFLY1_2016 ;
  
  return eID;
}


// READING and WRITING TRAJECTORIES
// --------------------------------
// tracing options  (source, read_in)
void    Trace_ID2Txt(char* sText, const VtTrace eID)
{
  switch (eID)
  {
    case WRITE_TRC_FILES: strcpy(sText, "write trace files"      ); break;
    case ONLY_TRC_TRAJ  : strcpy(sText, "only trace trajectories"); break;
    default             : strcpy(sText, "no tracing");
  }
}
VtTrace Trace_Txt2ID(const char* sText)
{
  VtTrace eID=NO_TRACING;

       if (strcmp(sText, "write trace files"      )==0) eID=WRITE_TRC_FILES;
  else if (strcmp(sText, "only trace trajectories")==0) eID=ONLY_TRC_TRAJ  ;
  
  return eID;
}

// data format of the program (read_in and writeout)
void        PrgFormat_ID2Txt(char* sText, const VtPrgFormat eID)
{
  switch (eID)
  {
    case VT_VITESS_FMT: strcpy(sText, "VITESS"); break;
    case VT_MCSTAS_FMT: strcpy(sText, "McStas"); break;
    case VT_MCPL_FMT  : strcpy(sText, "MCPL"  ); break;
    case VT_MCNPX_FMT : strcpy(sText, "MCNPX" ); break;
    case VT_MCNP6_FMT : strcpy(sText, "MCNP6" ); break;
    default  : strcpy(sText, "");
  }
}
VtPrgFormat PrgFormat_Txt2ID(const char* sText)
{
  VtPrgFormat eID=VT_VITESS_FMT;

       if (strcmp(sText, "VITESS")==0) eID=VT_VITESS_FMT;
  else if (strcmp(sText, "McStas")==0) eID=VT_MCSTAS_FMT;
  else if (strcmp(sText, "MCPL"  )==0) eID=VT_MCPL_FMT  ;
  else if (strcmp(sText, "MCNPX" )==0) eID=VT_MCNPX_FMT ;
  else if (strcmp(sText, "MCNP6" )==0) eID=VT_MCNP6_FMT ;
  
  return eID;
}

// data format used to store trajctories: float, exponential or binary   (read_in and writeout)
void         DataFormat_ID2Txt(char* sText, const VtDataFormat eID)
{
  switch (eID)
  {
    case VT_EXPONENTIAL: strcpy(sText, "exp"   ); break;
    case VT_FLOAT      : strcpy(sText, "float" ); break;
    case VT_BINARY     : strcpy(sText, "binary"); break;
    default  : strcpy(sText, "");
  }
}
VtDataFormat DataFormat_Txt2ID(const char* sText)
{
  VtDataFormat eID=VT_FLOAT;

       if (strcmp(sText, "exp"   )==0) eID=VT_EXPONENTIAL;
  else if (strcmp(sText, "float" )==0) eID=VT_FLOAT      ;
  else if (strcmp(sText, "binary")==0) eID=VT_BINARY     ;
  
  return eID;
}

// choice of separator in trajectory table (writeout)
void        Separator_ID2Txt(char* sText, const VtSeparator eID)
{
  switch (eID)
  {
    case VT_BLANK    : strcpy(sText, "space"    ); break;
    case VT_TABULATOR: strcpy(sText, "tabulator"); break;
    default  : strcpy(sText, "");
  }
}
VtSeparator Separator_Txt2ID(const char* sText)
{
  VtSeparator eID=VT_BLANK;

       if (strcmp(sText, "space"    )==0) eID=VT_BLANK    ;
  else if (strcmp(sText, "Space"    )==0) eID=VT_BLANK    ;
  else if (strcmp(sText, "tabulator")==0) eID=VT_TABULATOR;
  else if (strcmp(sText, "Tabulator")==0) eID=VT_TABULATOR;
  
  return eID;
}


// FRAME
// -----
// sequence of transformations
void       TfmnSeq_ID2Txt(char* sText, const VtTfmnSeq eID)
{
  switch (eID)
  {
    case VT_RTM: strcpy(sText, "RTM"); break;
    case VT_RMT: strcpy(sText, "RMT"); break;
    case VT_TRM: strcpy(sText, "TRM"); break;
    case VT_TMR: strcpy(sText, "TMR"); break;
    case VT_MTR: strcpy(sText, "MTR"); break;
    case VT_MRT: strcpy(sText, "MRT"); break;
    default            : strcpy(sText, "");
  }
}
VtTfmnSeq  TfmnSeq_Txt2ID(const char* sText)
{
  VtTfmnSeq eID=VT_NO_SEQ;

       if (strcmp(sText, "RTM")==0) eID=VT_RTM;
  else if (strcmp(sText, "RMT")==0) eID=VT_RMT;
  else if (strcmp(sText, "TRM")==0) eID=VT_TRM;
  else if (strcmp(sText, "TMR")==0) eID=VT_TMR;
  else if (strcmp(sText, "MTR")==0) eID=VT_MTR;
  else if (strcmp(sText, "MRT")==0) eID=VT_MRT;
  
  return eID;
}


// WINDOWS and COLLIMATORS
// -----------------------
// absorbing window material
void       WndAbs_ID2Txt(char* sText, const VtWndAbs eID)
{
  switch (eID)
  {
    case VT_WABS_FILE : strcpy(sText, "from file"     ); break;
    case VT_WABS_GD   : strcpy(sText, "gadolinium"    ); break;
    case VT_WABS_CD   : strcpy(sText, "cadmium"       ); break;
    case VT_WABS_B10  : strcpy(sText, "bor10"         ); break;
    case VT_WABS_EU   : strcpy(sText, "europium"      ); break;
    case VT_WABS_SI   : strcpy(sText, "silicon"       ); break;
    case VT_WABS_IDEAL: strcpy(sText, "ideal absorber"); break;
    default           : strcpy(sText, "");
  }
}
VtWndAbs   WndAbs_Txt2ID(const char* sText)
{
  VtWndAbs eID=VT_WABS_IDEAL;

       if (strcmp(sText, "from file"     )==0) eID=VT_WABS_FILE ;
  else if (strcmp(sText, "gadolinium"    )==0) eID=VT_WABS_GD   ;
  else if (strcmp(sText, "cadmium"       )==0) eID=VT_WABS_CD   ;
  else if (strcmp(sText, "bor10"         )==0) eID=VT_WABS_B10  ;
  else if (strcmp(sText, "Bor10"         )==0) eID=VT_WABS_B10  ;
  else if (strcmp(sText, "europium"      )==0) eID=VT_WABS_EU   ;
  else if (strcmp(sText, "Eu"            )==0) eID=VT_WABS_EU   ;
  else if (strcmp(sText, "silicon"       )==0) eID=VT_WABS_SI   ;
  else if (strcmp(sText, "Silicon"       )==0) eID=VT_WABS_SI   ;
  else if (strcmp(sText, "ideal absorber")==0) eID=VT_WABS_IDEAL;
  
  return eID;
}

// oscillation (collimator_radial)
void       Oscill_ID2Txt(char* sText, const VtOscill eID)
{
  switch (eID)
  {
    case VT_OSC_OFF  : strcpy(sText, "oscillation off"         ); break;
    case VT_RND_PHASE: strcpy(sText, "randow phase oscillation"); break;
    default          : strcpy(sText, "");
  }
}
VtOscill   Oscill_Txt2ID(const char* sText)
{
  VtOscill eID=VT_OSC_OFF;

       if (strcmp(sText, "oscillation off"         )==0) eID=VT_OSC_OFF  ;
  else if (strcmp(sText, "randow phase oscillation")==0) eID=VT_RND_PHASE;
  
  return eID;
}

// shape of multiple windows
void           MultWndShape_ID2Txt(char* sText, const VtMultWndShape eID)
{
  switch (eID)
  {
    case VT_MWND_AUTO  : strcpy(sText, "automatic"  ); break;
    case VT_MWND_CIRCLE: strcpy(sText, "spherical"  ); break;
    case VT_MWND_SQUARE: strcpy(sText, "rectangular"); break;
    default            : strcpy(sText, "");
  }
}
VtMultWndShape MultWndShape_Txt2ID(const char* sText)
{
  VtMultWndShape eID=VT_MWND_AUTO;

       if (strcmp(sText, "automatic"  )==0) eID=VT_MWND_AUTO  ;
  else if (strcmp(sText, "spherical"  )==0) eID=VT_MWND_CIRCLE;
  else if (strcmp(sText, "rectangular")==0) eID=VT_MWND_SQUARE;
  
  return eID;
}


// GUIDES and MIRRORS
// ------------------
// guide walls : top, bottom ... 
void        GdeWall_ID2Txt(char* sText, const VtGdeWall eID)
{
  switch (eID)
  {
    case GW_TOP   : strcpy(sText, "top"   ); break;
    case GW_BOTTOM: strcpy(sText, "bottom"); break;
    case GW_LEFT  : strcpy(sText, "lweft" ); break;
    case GW_RIGHT : strcpy(sText, "right" ); break;
    case GW_EXIT  : strcpy(sText, "exit"  ); break;
    case GW_INIT  : strcpy(sText, "init"  ); break;
    default       : strcpy(sText, "");
  }
}
VtGdeWall   GdeWall_Txt2ID(const char* sText)
{
  VtGdeWall eID=GW_INIT;

       if (strcmp(sText, "top"   )==0) eID=GW_TOP   ;
  else if (strcmp(sText, "bottom")==0) eID=GW_BOTTOM;
  else if (strcmp(sText, "lweft" )==0) eID=GW_LEFT  ;
  else if (strcmp(sText, "right" )==0) eID=GW_RIGHT ;
  else if (strcmp(sText, "exit"  )==0) eID=GW_EXIT  ;
  else if (strcmp(sText, "init"  )==0) eID=GW_INIT  ;
  
  return eID;
}

// guide shape
void        GdeShape_ID2Txt(char* sText, const VtGdeShape eID)
{
  switch (eID)
  {
    case VT_CONSTANT : strcpy(sText, "constant"     ); break;
    case VT_LINEAR   : strcpy(sText, "linear"       ); break;
    case VT_CURVED   : strcpy(sText, "curved"       ); break;
    case VT_PARABOLIC: strcpy(sText, "parabolic"    ); break;
    case VT_ELLIPTIC : strcpy(sText, "elliptic"     ); break;
    case VT_FROM_FILE: strcpy(sText, "from file"    ); break;
    case VT_LIN_CURV : strcpy(sText, "curved+linear"); break;
    default          : strcpy(sText, "");
  }
}
VtGdeShape  GdeShape_Txt2ID(const char* sText)
{
  VtGdeShape eID=VT_CONSTANT;

       if (strcmp(sText, "constant"     )==0) eID=VT_CONSTANT ;
  else if (strcmp(sText, "linear"       )==0) eID=VT_LINEAR   ;
  else if (strcmp(sText, "curved"       )==0) eID=VT_CURVED   ;
  else if (strcmp(sText, "parabolic"    )==0) eID=VT_PARABOLIC;
  else if (strcmp(sText, "elliptic"     )==0) eID=VT_ELLIPTIC ;
  else if (strcmp(sText, "from file"    )==0) eID=VT_FROM_FILE;
  else if (strcmp(sText, "curved+linear")==0) eID=VT_LIN_CURV ;
  
  return eID;
}

// waviness distribution
void        WaviDistr_ID2Txt(char* sText, const VtWaviDistr eID)
{
  switch (eID)
  {
    case VT_WAVI_RECT : strcpy(sText, "rectangular"); break;
    case VT_WAVI_GAUSS: strcpy(sText, "Gaussian"   ); break;
    default           : strcpy(sText, "");
  }
}
VtWaviDistr WaviDistr_Txt2ID(const char* sText)
{
  VtWaviDistr eID=VT_WAVI_RECT;

       if (strcmp(sText, "rectangular")==0) eID=VT_WAVI_RECT;
  else if (strcmp(sText, "Gaussian"   )==0) eID=VT_WAVI_GAUSS  ;
  
  return eID;
}

// mirror material
void        MirrMat_ID2Txt(char* sText, const VtMirrMat eID)
{
  switch (eID)
  {
    case VT_NO_MIRR_MAT: strcpy(sText, "none"    ); break;
    case VT_MIRR_OTHER : strcpy(sText, "other"   ); break;
    case VT_MIRR_SI    : strcpy(sText, "silicon" ); break;
    case VT_MIRR_SAPPH : strcpy(sText, "sapphire"); break;
    case VT_MIRR_GLASS : strcpy(sText, "glass"   ); break;
    case VT_MIRR_B4C   : strcpy(sText, "B4C"   ); break;
    default            : strcpy(sText, "");
  }
}
VtMirrMat   MirrMat_Txt2ID(const char* sText)
{
  VtMirrMat eID=VT_NO_MIRR_MAT;

       if (strcmp(sText, "Other"   )==0 || strcmp(sText, "other"   )==0) eID=VT_MIRR_OTHER;
  else if (strcmp(sText, "Silicon" )==0 || strcmp(sText, "silicon" )==0) eID=VT_MIRR_SI   ;
  else if (strcmp(sText, "Sapphire")==0 || strcmp(sText, "sapphire")==0) eID=VT_MIRR_SAPPH;
  else if (strcmp(sText, "Glass"   )==0 || strcmp(sText, "glass"   )==0) eID=VT_MIRR_GLASS;
  else if (strcmp(sText, "B4C"     )==0                                ) eID=VT_MIRR_B4C;
 
  return eID;
}

// reflection list parameter
void        ListPar_ID2Txt(char* sText, const VtListPar eID)
{
  switch (eID)
  {
    case VT_LIST_PASS   : strcpy(sText, "Trajectories passing the guide end"     ); break;
    case VT_LIST_PASS_LF: strcpy(sText, "Trajectories passing the guide end (LF)"); break;
    case VT_LIST_REFL   : strcpy(sText, "Only successful reflections"            ); break;
    case VT_LIST_REFL_LF: strcpy(sText, "Only successful reflections (LF)"       ); break;
    case VT_LIST_T1SR   : strcpy(sText, "Traj. with at least 1 reflection"       ); break;
    case VT_LIST_T1SR_LF: strcpy(sText, "Traj. with at least 1 reflection (LF)"  ); break;
    case VT_LIST_ALL    : strcpy(sText, "All trajectories"                       ); break;
    case VT_LIST_ALL_LF : strcpy(sText, "All trajectories (LF)"                  ); break;
    default             : strcpy(sText, "");
  }
}
VtListPar   ListPar_Txt2ID(const char* sText)
{
  VtListPar eID=VT_LIST_PASS_LF;

       if (strcmp(sText, "Trajectories passing the guide end"     )==0) eID=VT_LIST_PASS;
  else if (strcmp(sText, "Trajectories passing the guide end (LF)")==0 || strcmp(sText, "Trajectories passing the guide end (with linefeed)"                  )==0) eID=VT_LIST_PASS_LF;
  else if (strcmp(sText, "Only successful reflections"            )==0) eID=VT_LIST_REFL;
  else if (strcmp(sText, "Only successful reflections (LF)"       )==0 || strcmp(sText, "Only successful reflections (with linefeed)"                         )==0) eID=VT_LIST_REFL_LF;
  else if (strcmp(sText, "Traj. with at least 1 reflection"       )==0 || strcmp(sText, "Trajectories with at least one successful reflection"                )==0) eID=VT_LIST_T1SR;
  else if (strcmp(sText, "Traj. with at least 1 reflection (LF)"  )==0 || strcmp(sText, "Trajectories with at least one successful reflection (with linefeed)")==0) eID=VT_LIST_T1SR_LF;
  else if (strcmp(sText, "All trajectories"                       )==0) eID=VT_LIST_ALL;
  else if (strcmp(sText, "All trajectories (LF)"                  )==0 || strcmp(sText, "All trajectories (with linefeed)"                                    )==0) eID=VT_LIST_ALL_LF;
  
  return eID;
}

// additional output for reflection list
void        ListVbs_ID2Txt(char* sText, const VtListVbs eID)
{
  switch (eID)
  {
    case VT_LSTM_NO  : strcpy(sText, "no"          ); break;
    case VT_LSTM_YES : strcpy(sText, "yes"         ); break;
    case VT_LSTM_EDGE: strcpy(sText, "entry & exit"); break;
    default          : strcpy(sText, "");
  }
}
VtListVbs   ListVbs_Txt2ID(const char* sText)
{
  VtListVbs eID=VT_LSTM_EDGE;

       if (strcmp(sText, "no"          )==0) eID=VT_LSTM_NO  ;
  else if (strcmp(sText, "yes"         )==0) eID=VT_LSTM_YES ;
  else if (strcmp(sText, "entry & exit")==0) eID=VT_LSTM_EDGE;
  
  return eID;
}

// reflection plot parameter
void        PlotPar_ID2Txt(char* sText, const VtPlotPar eID)
{
  switch (eID)
  {
    case iKeyMode         : strcpy(sText, "Scattered (Mode)"); break;
    case iKeyMode0        : strcpy(sText, "Mode0"       ); break;
    case iKeyMode5        : strcpy(sText, "Mode5"       ); break;
    case iKeyMode10       : strcpy(sText, "Mode10"      ); break;
    case dKeyRefCount     : strcpy(sText, "RefCount"    ); break;
    case dKeyRefCountY    : strcpy(sText, "RefCountY"   ); break;
    case dKeyRefCountZ    : strcpy(sText, "RefCountZ"   ); break;
    case iKeyThisCollision: strcpy(sText, "Plane"       ); break;
    case dKeydegangular   : strcpy(sText, "Ref. Angle"  ); break;
    case dKeym            : strcpy(sText, "m"           ); break;
    case dKeyreflectivity : strcpy(sText, "Reflectivity"); break;
    case dKeyDivY         : strcpy(sText, "DivY"        ); break;
    case dKeyDivZ         : strcpy(sText, "DivZ"        ); break;
    case iKeyColor        : strcpy(sText, "color"       ); break;
    case dKeyTime         : strcpy(sText, "TOF"         ); break;
    case dKeyWavelength   : strcpy(sText, "Wavelength"  ); break;
    case dKeyProbability  : strcpy(sText, "Probability" ); break;
    case dKeyPositionX    : strcpy(sText, "Position X"  ); break;
    case dKeyPositionY    : strcpy(sText, "Position Y"  ); break;
    case dKeyPositionZ    : strcpy(sText, "Position Z"  ); break;
    case dKeyVectorX      : strcpy(sText, "Vector X"    ); break;
    case dKeyVectorY      : strcpy(sText, "Vector Y"    ); break;
    case dKeyVectorZ      : strcpy(sText, "Vector Z"    ); break;
    case dKeySpinX        : strcpy(sText, "Spin X"      ); break;
    case dKeySpinY        : strcpy(sText, "Spin Y"      ); break;
    case dKeySpinZ        : strcpy(sText, "Spin Z"      ); break;
    default               : strcpy(sText, "none");
  }
}
VtPlotPar   PlotPar_Txt2ID(const char* sText)
{
  VtPlotPar eID=KeyNone;

       if (strcmp(sText, "Scattered (Mode)")==0) eID=iKeyMode         ;
  else if (strcmp(sText, "Mode0"       )==0) eID=iKeyMode0        ;
  else if (strcmp(sText, "Mode5"       )==0) eID=iKeyMode5        ;
  else if (strcmp(sText, "Mode10"      )==0) eID=iKeyMode10       ;
  else if (strcmp(sText, "RefCount"    )==0) eID=dKeyRefCount     ;
  else if (strcmp(sText, "RefCountY"   )==0) eID=dKeyRefCountY    ;
  else if (strcmp(sText, "RefCountZ"   )==0) eID=dKeyRefCountZ    ;
  else if (strcmp(sText, "Plane"       )==0) eID=iKeyThisCollision;
  else if (strcmp(sText, "Ref. Angle"  )==0) eID=dKeydegangular   ;
  else if (strcmp(sText, "m"           )==0) eID=dKeym            ;
  else if (strcmp(sText, "Reflectivity")==0) eID=dKeyreflectivity ;
  else if (strcmp(sText, "DivY"        )==0) eID=dKeyDivY         ;
  else if (strcmp(sText, "DivZ"        )==0) eID=dKeyDivZ         ;
  else if (strcmp(sText, "Color"       )==0) eID=iKeyColor        ;
  else if (strcmp(sText, "TOF"         )==0) eID=dKeyTime         ;
  else if (strcmp(sText, "Wavelength"  )==0) eID=dKeyWavelength   ;
  else if (strcmp(sText, "Probability" )==0) eID=dKeyProbability  ;
  else if (strcmp(sText, "Position X"  )==0) eID=dKeyPositionX    ;
  else if (strcmp(sText, "Position Y"  )==0) eID=dKeyPositionY    ;
  else if (strcmp(sText, "Position Z"  )==0) eID=dKeyPositionZ    ;
  else if (strcmp(sText, "Vector X"    )==0) eID=dKeyVectorX      ;
  else if (strcmp(sText, "Vector Y"    )==0) eID=dKeyVectorY      ;
  else if (strcmp(sText, "Vector Z"    )==0) eID=dKeyVectorZ      ;
  else if (strcmp(sText, "Spin X"      )==0) eID=dKeySpinX        ;
  else if (strcmp(sText, "Spin Y"      )==0) eID=dKeySpinY        ;
  else if (strcmp(sText, "Spin Z"      )==0) eID=dKeySpinZ        ;
  
  return eID;
}

// reflection plot filter
void        PlotFilt_ID2Txt(char* sText, const VtPlotFilt eID)
{
  switch (eID)
  {
    case VT_PLOT_ALL : strcpy(sText, "all"           ); break;
    case VT_PLOT_SCAT: strcpy(sText, "only scattered"); break;
    case VT_PLOT_DIED: strcpy(sText, "only died"     ); break;
    default           : strcpy(sText, "");
  }
}
VtPlotFilt  PlotFilt_Txt2ID(const char* sText)
{
  VtPlotFilt eID=VT_PLOT_ALL;

       if (strcmp(sText, "all"           )==0 || strcmp(sText, "All"           )==0) eID=VT_PLOT_ALL ;
  else if (strcmp(sText, "only scattered")==0 || strcmp(sText, "Only scattered")==0) eID=VT_PLOT_SCAT;
  else if (strcmp(sText, "only died"     )==0 || strcmp(sText, "Only died"     )==0) eID=VT_PLOT_DIED;
  
  return eID;
}


// MONOCHROMATOR + CHOPPERS
// ------------------------
// arrangement of monochromator  
void          MonoArrange_ID2Txt(char* sText, const VtMonoArrange eID)
{
  switch (eID)
  {
    case SINGLE_CE    : strcpy(sText, "single CE"          ); break;
    case CE_ARRAY_CALC: strcpy(sText, "calculated CE array"); break;
    case CE_ARRAY_FILE: strcpy(sText, "CE array from file" ); break;
    default         : strcpy(sText, "");
  }
}
VtMonoArrange MonoArrange_Txt2ID(const char* sText)
{
  VtMonoArrange eID=SINGLE_CE;

       if (strcmp(sText, "single CE"          )==0) eID=SINGLE_CE    ;
  else if (strcmp(sText, "calculated CE array")==0) eID=CE_ARRAY_CALC;
  else if (strcmp(sText, "CE array from file" )==0) eID=CE_ARRAY_FILE;
  
  return eID;
}

// monochromator geometry
void        MonoType_ID2Txt(char* sText, const VtMonoType eID)
{
  switch (eID)
  {
    case REFL_MONO  : strcpy(sText, "reflection"  ); break;
    case TRANSM_MONO: strcpy(sText, "transmission"); break;
    default         : strcpy(sText, "");
  }
}
VtMonoType  MonoType_Txt2ID(const char* sText)
{
  VtMonoType eID=REFL_MONO;

       if (strcmp(sText, "reflection"  )==0 || strcmp(sText, "Reflection"  )==0) eID=REFL_MONO  ;
  else if (strcmp(sText, "transmission")==0 || strcmp(sText, "Transmission")==0) eID=TRANSM_MONO;
  
  return eID;
}

// focusing options
void        MonoFocus_ID2Txt(char* sText, const VtMonoFocus eID)
{
  switch (eID)
  {
    case CONST_LMBD: strcpy(sText, "constant lambda" ); break;
    case SPHERICAL : strcpy(sText, "spherical"       ); break;
    case VERT_CYL  : strcpy(sText, "vert. cylinder"  ); break;
    case DBL_FOC   : strcpy(sText, "double focussing"); break;
    default        : strcpy(sText, "");
  }
}
VtMonoFocus MonoFocus_Txt2ID(const char* sText)
{
  VtMonoFocus eID=NO_FOCUSING;

       if (strcmp(sText, "constant lambda" )==0) eID=CONST_LMBD;
  else if (strcmp(sText, "spherical"       )==0) eID=SPHERICAL ;
  else if (strcmp(sText, "vert. cylinder"  )==0) eID=VERT_CYL  ;
  else if (strcmp(sText, "double focussing")==0) eID=DBL_FOC   ;
  
  return eID;
}

// Channel Shape         (chopper_fermi)
void        ChnlShape_ID2Txt(char* sText, const VtChnlShape eID)
{
  switch (eID)
  {
    case VT_CHN_STR  : strcpy(sText, "straight"); break;
    case VT_CHN_IDEAL: strcpy(sText, "ideal"   ); break;
    case VT_CHN_CIRC : strcpy(sText, "circular"); break;
    default       : strcpy(sText, "");
  }
}
VtChnlShape ChnlShape_Txt2ID(const char* sText)
{
  VtChnlShape eID=VT_NO_CHN_SHAPE;

       if (strcmp(sText, "straight")==0) eID=VT_CHN_STR;
  else if (strcmp(sText, "ideal"   )==0) eID=VT_CHN_IDEAL;
  else if (strcmp(sText, "circular")==0) eID=VT_CHN_CIRC ;
  
  return eID;
}

// SAMPLES
// -------
// sample geometry
void        SmplGeom_ID2Txt(char* sText, const VtSmplGeom eID)
{
  switch (eID)
  {
    case VT_CUBE   : strcpy(sText, "cuboid"         ); break;
    case VT_CYL    : strcpy(sText, "cylinder"       ); break;
    case VT_SPHERE : strcpy(sText, "sphere"         ); break;
    case VT_HOL_CYL: strcpy(sText, "hollow-cylinder"); break;
    default         : strcpy(sText, "");
  }
}
VtSmplGeom  SmplGeom_Txt2ID(const char* sText)
{
  VtSmplGeom eID=VT_CYL;

       if (memcmp(sText, "cuboid"         , 3)==0) eID=VT_CUBE   ;
  else if (memcmp(sText, "rectangular"    , 3)==0) eID=VT_CUBE   ;
  else if (memcmp(sText, "cylinder"       , 3)==0) eID=VT_CYL    ;
  else if (memcmp(sText, "sphere"         , 3)==0) eID=VT_SPHERE ;
  else if (memcmp(sText, "ball"           , 3)==0) eID=VT_SPHERE ;
  else if (memcmp(sText, "hollow-cylinder", 3)==0) eID=VT_HOL_CYL;
  
  return eID;
}

// particle geometry
void        PtclGeom_ID2Txt(char* sText, const VtPtclGeom eID)
{
  switch (eID)
  {
    case VT_PTCL_SPHERE  : strcpy(sText, "spheres"               ); break;
    case VT_PTCL_POLY_SPH: strcpy(sText, "polydispersive spheres"); break;
    case VT_PTCL_ELLIPS  : strcpy(sText, "ellipsoids"            ); break;
    case VT_PTCL_CYL     : strcpy(sText, "cylinders"             ); break;
    case VT_PTCL_EPIPED  : strcpy(sText, "parallelepipeds"       ); break;
    case VT_ISOTROPIC    : strcpy(sText, "isotropic scattering"  ); break;
    default              : strcpy(sText, ""  );
  }
}
VtPtclGeom  PtclGeom_Char2ID(const char cID)
{
  VtPtclGeom eID=VT_NO_PTCL;

  switch (cID)
  {
    case 'S': eID=VT_PTCL_SPHERE  ; break;
    case 'D': eID=VT_PTCL_POLY_SPH; break;
    case 'E': eID=VT_PTCL_ELLIPS  ; break;
    case 'C': eID=VT_PTCL_CYL     ; break;
    case 'P': eID=VT_PTCL_EPIPED  ; break;
    case 'I': eID=VT_ISOTROPIC    ; break;
    default : eID=VT_NO_PTCL;
  }
  
  return eID;
}
VtPtclGeom  PtclGeom_Txt2ID(const char* sText)
{
  VtPtclGeom eID=VT_NO_PTCL;

       if (strcmp(sText, "spheres"               )==0) eID=VT_PTCL_SPHERE;
  else if (strcmp(sText, "polydispersive spheres")==0) eID=VT_PTCL_POLY_SPH;
  else if (strcmp(sText, "ellipsoids"            )==0) eID=VT_PTCL_ELLIPS;
  else if (strcmp(sText, "cylinders"             )==0) eID=VT_PTCL_CYL;
  else if (strcmp(sText, "parallelepipeds"       )==0) eID=VT_PTCL_EPIPED;
  else if (strcmp(sText, "isotropic scattering"  )==0) eID=VT_ISOTROPIC;
  
  return eID;
}

// source of S(Q) (sample_reflectom)
void        DataSrc_ID2Txt(char* sText, const VtDataSrc eID)
{
  switch (eID)
  {
    case VT_FR_FILE: strcpy(sText, "from file"  ); break;
    case VT_AS_FCT : strcpy(sText, "as function"); break;
    default        : strcpy(sText, "");
  }
}
VtDataSrc   DataSrc_Txt2ID(const char* sText)
{
  VtDataSrc eID=VT_NO_SRC;

       if (strcmp(sText, "from file"  )==0) eID=VT_FR_FILE;
  else if (strcmp(sText, "as function")==0) eID=VT_AS_FCT ;
  
  return eID;
}

// measuring mode (sample_reflectom)
void        MeasMode_ID2Txt(char* sText, const VtMeasMode eID)
{
  switch (eID)
  {
    case VT_SAMPLE   : strcpy(sText, "sample"   ); break;
    case VT_REFERENCE: strcpy(sText, "reference"); break;
    default        : strcpy(sText, "");
  }
}
VtMeasMode  MeasMode_Txt2ID(const char* sText)
{
  VtMeasMode eID=VT_SAMPLE;

       if (strcmp(sText, "sample"   )==0) eID=VT_SAMPLE   ;
  else if (strcmp(sText, "reference")==0) eID=VT_REFERENCE;
  
  return eID;
}


// DETECTOR
// --------
// detector geometry
void        DetGeom_ID2Txt(char* sText, const VtDetGeom eID)
{
  switch (eID)
  {
    case VT_DET_CYL : strcpy(sText, "cylindrical"); break;
    case VT_DET_FLAT: strcpy(sText, "flat"       ); break;
    default         : strcpy(sText, "");
  }
}
VtDetGeom   DetGeom_Txt2ID(const char* sText)
{
  VtDetGeom eID=VT_NO_DET_GEOM;

       if (strcmp(sText, "cylindrical")==0) eID=VT_DET_CYL ;
  else if (strcmp(sText, "flat"       )==0) eID=VT_DET_FLAT;
  
  return eID;
}

// detector type
void        DetType_ID2Txt(char* sText, const VtDetType eID)
{
  switch (eID)
  {
    case VT_DET_TUBE: strcpy(sText, "tubes"      ); break;
    case VT_DET_AREA: strcpy(sText, "area/volume"); break;
    default         : strcpy(sText, "");
  }
}
VtDetType   DetType_Txt2ID(const char* sText)
{
  VtDetType eID=VT_NO_DET_TYPE;

       if (strcmp(sText, "tubes"      )==0) eID=VT_DET_TUBE;
  else if (strcmp(sText, "area/volume")==0) eID=VT_DET_AREA;
  
  return eID;
}

// module usage
void       DetUse_ID2Txt(char* sText, const VtDetUse eID)
{
  switch (eID)
  {
    case VT_DET_REAL: strcpy(sText, "realistic   "); break;  // "normal" in vsn. 3
    case VT_MON_ONLY: strcpy(sText, "monitor only"); break;
    case VT_GRID_OFF: strcpy(sText, "grid off"    ); break;
    default         : strcpy(sText, "");
  }
}
VtDetUse   DetUse_Txt2ID(const char* sText)
{
  VtDetUse eID=VT_NO_DET_USE;

       if (strcmp(sText, "normal"      )==0 
        || strcmp(sText, "realistic"   )==0) eID=VT_DET_REAL;
  else if (strcmp(sText, "monitor only")==0) eID=VT_MON_ONLY;
  else if (strcmp(sText, "grid off"    )==0) eID=VT_GRID_OFF;
  
  return eID;
}

// tube shape
void        TubeShape_ID2Txt(char* sText, const VtTubeShape eID)
{
  switch (eID)
  {
    case VT_TUBE_CIRCLE: strcpy(sText, "circular"   ); break;
    case VT_TUBE_SQUARE: strcpy(sText, "rectangular"); break;
    default         : strcpy(sText, "");
  }
}
VtTubeShape TubeShape_Txt2ID(const char* sText)
{
  VtTubeShape eID=VT_NO_TUBE_SHAPE;

       if (strcmp(sText, "circular"   )==0) eID=VT_TUBE_CIRCLE;
  else if (strcmp(sText, "rectangular")==0) eID=VT_TUBE_SQUARE;
  
  return eID;
}

// absorbing detector material
void       DetAbs_ID2Txt(char* sText, const VtDetAbs eID)
{
  switch (eID)
  {
    case VT_GAS_BF3  : strcpy(sText, "BF3 gas"  ); break;
    case VT_GAS_HE3  : strcpy(sText, "3He gas"  ); break;
    case VT_SOLID_B10: strcpy(sText, "solid B10"); break;
    case VT_SOLID_LI6: strcpy(sText, "solid Li6"); break;
    case VT_ABS_OTHER: strcpy(sText, "other"    ); break;
    default          : strcpy(sText, "");
  }
}
VtDetAbs   DetAbs_Txt2ID(const char* sText)
{
  VtDetAbs eID=VT_NO_ABS_MAT;

       if (strcmp(sText, "BF3 gas"  )==0) eID=VT_GAS_BF3  ;
  else if (strcmp(sText, "3He gas"  )==0) eID=VT_GAS_HE3  ;
  else if (strcmp(sText, "solid B10")==0) eID=VT_SOLID_B10;
  else if (strcmp(sText, "solid Li6")==0) eID=VT_SOLID_LI6;
  else if (strcmp(sText, "other"    )==0) eID=VT_ABS_OTHER;
  
  return eID;
}


// MONITORS + FILTER + EVALUATION
// --------------------------------
// monitor parameter for mon1 and mon_pol1
void      Mon1Par_ID2Txt(char* sText, const VtMon1Par eID)
{
  switch (eID)
  {
    case MON_LAMBDA: strcpy(sText, "lambda" ); break;
    case MON_TIME  : strcpy(sText, "time"   ); break;
    case MON_DIV_Y : strcpy(sText, "div_y"  ); break;
    case MON_DIV_Z : strcpy(sText, "div_z"  ); break;
    case MON_Y     : strcpy(sText, "pos_y"  ); break;
    case MON_Z     : strcpy(sText, "pos_z"  ); break;
    case MON_ENERGY: strcpy(sText, "energy" ); break;
    case MON_DIV_YZ: strcpy(sText, "div_rad"); break;
    default        : strcpy(sText, "");
  }
}
VtMon1Par Mon1Par_Txt2ID(const char* sText)
{
  VtMon1Par eID=NO_MON_PAR;

       if (strcmp(sText, "lambda" )==0) eID=MON_LAMBDA;
  else if (strcmp(sText, "time"   )==0) eID=MON_TIME  ;
  else if (strcmp(sText, "div_y"  )==0) eID=MON_DIV_Y ;
  else if (strcmp(sText, "div_z"  )==0) eID=MON_DIV_Z ;
  else if (strcmp(sText, "pos_y"  )==0) eID=MON_Y     ;
  else if (strcmp(sText, "pos_z"  )==0) eID=MON_Z     ;
  else if (strcmp(sText, "energy" )==0) eID=MON_ENERGY;
  else if (strcmp(sText, "div_rad")==0) eID=MON_DIV_YZ;
  
  return eID;
}

// 2D monitor and filter parameter
void      Mon2Par_ID2Txt(char* sText, const VtMon2Par eID)
{
  switch (eID)
  {
    case MON2_POS : strcpy(sText, "position"  ); break;
    case MON2_DIV : strcpy(sText, "divergence"); break;
    default       : strcpy(sText, "");
  }
}
VtMon2Par Mon2Par_Txt2ID(const char* sText)
{
  VtMon2Par eID=NO_MON2_PAR;

       if (strcmp(sText, "position"  )==0) eID=MON2_POS;
  else if (strcmp(sText, "divergence")==0) eID=MON2_DIV;
  
  return eID;
}

// monitor parameter for monitor1D and monitor2D
void      MonPar_ID2Txt(char* sText, const VtMonPar eID)
{
  switch (eID)
  {
    case POS_X    : strcpy(sText, "pos_x"    ); break;
    case POS_Y    : strcpy(sText, "pos_y"    ); break;
    case POS_Z    : strcpy(sText, "pos_z"    ); break;
    case DIV_Y    : strcpy(sText, "div_y"    ); break;
    case DIV_Z    : strcpy(sText, "div_z"    ); break;
    case LAMBDA   : strcpy(sText, "lambda"   ); break;
    case ENERGY   : strcpy(sText, "energy"   ); break;
    case TIME     : strcpy(sText, "time"     ); break;
    case K_Y      : strcpy(sText, "k_y"      ); break;
    case K_Z      : strcpy(sText, "k_z"      ); break;
    case POS_R    : strcpy(sText, "pos_r"    ); break;
    case POS_PHI  : strcpy(sText, "pos_phi"  ); break;
    case DIR_PHI  : strcpy(sText, "dir_phi"  ); break;
    case DIR_THETA: strcpy(sText, "dir_theta"); break;
    case COL_VERT : strcpy(sText, "col_vert" ); break;
    case COL_HOR  : strcpy(sText, "col_hor"  ); break;
    case COLOR    : strcpy(sText, "color"    ); break;
    default       : strcpy(sText, "none");
  }
}
VtMonPar  MonPar_Txt2ID(const char* sText)
{
  VtMonPar eID=NO_PAR;

       if (strcmp(sText, "pos_x"    )==0) eID=POS_X    ;
  else if (strcmp(sText, "pos_y"    )==0) eID=POS_Y    ;
  else if (strcmp(sText, "pos_z"    )==0) eID=POS_Z    ;
  else if (strcmp(sText, "div_y"    )==0) eID=DIV_Y    ;
  else if (strcmp(sText, "div_z"    )==0) eID=DIV_Z    ;
  else if (strcmp(sText, "lambda"   )==0) eID=LAMBDA   ;
  else if (strcmp(sText, "energy"   )==0) eID=ENERGY   ;
  else if (strcmp(sText, "time"     )==0) eID=TIME     ;
  else if (strcmp(sText, "k_y"      )==0) eID=K_Y      ;
  else if (strcmp(sText, "k_z"      )==0) eID=K_Z      ;
  else if (strcmp(sText, "pos_r"    )==0 
        || strcmp(sText, "r")        ==0) eID=POS_R    ;
  else if (strcmp(sText, "pos_phi"  )==0) eID=POS_PHI  ;
  else if (strcmp(sText, "dir_phi"  )==0) eID=DIR_PHI  ;
  else if (strcmp(sText, "dir_theta")==0) eID=DIR_THETA;
  else if (strcmp(sText, "col_vert" )==0) eID=COL_VERT ;
  else if (strcmp(sText, "col_hor"  )==0) eID=COL_HOR  ;
  else if (strcmp(sText, "color"    )==0) eID=COLOR    ;
  
  return eID;
}

// normalization options
void      MonNorm_ID2Txt(char* sText, const VtMonNorm eID)
{
  switch (eID)
  {
    case NORM_BIN_SIZE: strcpy(sText, "bin size"      ); break;
    case NORM_REF_FILE: strcpy(sText, "reference file"); break;
    default          : strcpy(sText, "none");
  }
}
VtMonNorm MonNorm_Txt2ID(const char* sText)
{
  VtMonNorm eID = NO_NORM;

       if (strcmp(sText, "bin size"      )==0) eID=NORM_BIN_SIZE;
  else if (strcmp(sText, "yes"           )==0) eID=NORM_BIN_SIZE;
  else if (strcmp(sText, "reference file")==0) eID=NORM_REF_FILE;
  
  return eID;
}

// monitor parameter for brilliance monintor
void      BrlPar_ID2Txt(char* sText, const VtBrlPar eID)
{
  switch (eID)
  {
    case VT_TIME    : strcpy(sText, "time"   ); break;
    case VT_LAMBDA  : strcpy(sText, "lambda" ); break;
    case VT_ENERGY  : strcpy(sText, "energy" ); break;
    case VT_POS_Y   : strcpy(sText, "pos_y"  ); break;
    case VT_POS_Z   : strcpy(sText, "pos_z"  ); break;
    case VT_POS_R   : strcpy(sText, "pos_rad"); break;
    case VT_DIV_HOR : strcpy(sText, "div_y"  ); break;
    case VT_DIV_VERT: strcpy(sText, "div_z"  ); break;
    case VT_DIV_RAD : strcpy(sText, "div_rad"); break;
    default         : strcpy(sText, "");
  }
}
VtBrlPar  BrlPar_Txt2ID(const char* sText)
{
  VtBrlPar eID=VT_LAMBDA;

       if (strcmp(sText, "time"   )==0) eID=VT_TIME    ;
  else if (strcmp(sText, "lambda" )==0) eID=VT_LAMBDA  ;
  else if (strcmp(sText, "energy" )==0) eID=VT_ENERGY  ;
  else if (strcmp(sText, "pos_y"  )==0) eID=VT_POS_Y   ;
  else if (strcmp(sText, "pos_z"  )==0) eID=VT_POS_Z   ;
  else if (strcmp(sText, "pos_rad")==0) eID=VT_POS_R   ;
  else if (strcmp(sText, "div_y"  )==0) eID=VT_DIV_HOR ;
  else if (strcmp(sText, "div_z"  )==0) eID=VT_DIV_VERT;
  else if (strcmp(sText, "div_rad")==0) eID=VT_DIV_RAD ;
  
  return eID;
}

// normalization options for brilliance monintor
void      BrlNorm_ID2Txt(char* sText, const VtBrlNorm eID)
{
  switch (eID)
  {
    case BRL_ABS   : strcpy(sText, "absolute" ); break;
    case BRL_TRANSF: strcpy(sText, "transfer" ); break;
    case BRL_PCT   : strcpy(sText, "1% lambda"); break;
    default  : strcpy(sText, "");
  }
}
VtBrlNorm BrlNorm_Txt2ID(const char* sText)
{
  VtBrlNorm eID = BRL_ABS;

       if (strcmp(sText,  "absolute" )==0) eID=BRL_ABS   ;
  else if (strcmp(sText,  "transfer" )==0) eID=BRL_TRANSF;
  else if (strcmp(sText,  "1% lambda")==0) eID=BRL_PCT   ;
  
  return eID;
}

// data format used to store trajctories: float, exponential or binary
void       Format2D_ID2Txt(char* sText, const VtFormat2D eID)
{
  switch (eID)
  {
    case MATRIX   : strcpy(sText, "matrix"        ); break;
    case XYZ      : strcpy(sText, "xyz"           ); break;
    case MATR_CMPT: strcpy(sText, "matrix compact"); break;
    case XYZ_CMPT : strcpy(sText, "xyz compact   "); break;
    default  : strcpy(sText, "");
  }
}
VtFormat2D Format2D_Txt2ID(const char* sText)
{
  VtFormat2D eID = NO_2D_FORMAT;

       if (strcmp(sText, "matrix"        )==0) eID=MATRIX   ;
  else if (strcmp(sText, "xyz"           )==0) eID=XYZ      ;
  else if (strcmp(sText, "matrix compact")==0) eID=MATR_CMPT;
  else if (strcmp(sText, "xyz compact   ")==0) eID=XYZ_CMPT ;
  
  return eID;
}

// filter combination 
void       FiltComb_ID2Txt(char* sText, const VtFiltComb eID)
{
  switch (eID)
  {
    case OR_OR_OR   : strcpy(sText, "OR"        ); break;
    case AND_AND_AND: strcpy(sText, "AND"       ); break;
    case AND_OR_AND : strcpy(sText, "AND_OR_AND"); break;
    default         : strcpy(sText, "none");
  }
}
VtFiltComb FiltComb_Txt2ID(const char* sText)
{
  VtFiltComb eID=NO_FCOMB;

       if (strcmp(sText, "OR"        )==0) eID = OR_OR_OR   ;
  else if (strcmp(sText, "AND"       )==0) eID = AND_AND_AND;
  else if (strcmp(sText, "AND_OR_AND")==0) eID = AND_OR_AND ;
  
  return eID;
}

// evaluation parameter for all eval modules
void      EvalPar_ID2Txt(char* sText, const VtEvalPar eID)
{
  switch (eID)
  {
    case VT_EVAL_DSP  : strcpy(sText, "d-spacing"            ); break;
    case VT_EVAL_Q    : strcpy(sText, "momentum transfer Q"  ); break;
    case VT_EVAL_ANGLE: strcpy(sText, "scattering angle"     ); break;
    case VT_EVAL_LMBD : strcpy(sText, "wavelength difference"); break;
    default      : strcpy(sText, "");
  }
}
VtEvalPar EvalPar_Txt2ID(const char* sText)
{
  VtEvalPar eID=VT_NO_EVAL;

       if (strcmp(sText, "d-spacing"            )==0) eID = VT_EVAL_DSP  ;
  else if (strcmp(sText, "momentum transfer Q"  )==0) eID = VT_EVAL_Q    ;
  else if (strcmp(sText, "scattering angle"     )==0) eID = VT_EVAL_ANGLE;
  else if (strcmp(sText, "wavelength difference")==0) eID = VT_EVAL_LMBD ;
  
  return eID;
}

// evaluation combination for eval_elast2
void       EvalComb_ID2Txt(char* sText, const VtEvalComb eID)
{
  switch (eID)
  {
    case VT_SCA_LMBD: strcpy(sText, "Scattering angle [deg] and wavelength [Ang]"); break;
    case VT_SCA_TOF : strcpy(sText, "Scattering angle [deg] and TOF [ms]"        ); break;
    default      : strcpy(sText, "");
  }
}
VtEvalComb EvalComb_Txt2ID(const char* sText)
{
  VtEvalComb eID=VT_NO_ECOMB;

       if (strcmp(sText, "Scattering angle [deg] and wavelength [Ang]")==0) eID = VT_SCA_LMBD;
  else if (strcmp(sText, "Scattering angle [deg] and TOF [ms]"        )==0) eID = VT_SCA_TOF ;
  
  return eID;
}

// sort mode for eval_elast2
void       EvalSort_ID2Txt(char* sText, const VtEvalSort eID)
{
  switch (eID)
  {
    case VT_SORT_X    : strcpy(sText, "Scattering angle"          ); break;
    case VT_SORT_X_R  : strcpy(sText, "Scattering angle (reverse)"); break;
    case VT_SORT_Y    : strcpy(sText, "Wavelength/TOF"            ); break;
    case VT_SORT_Y_R  : strcpy(sText, "Wavelength/TOF (reverse)"  ); break;
    case VT_SORT_INT  : strcpy(sText, "Intensity"                 ); break;
    case VT_SORT_INT_R: strcpy(sText, "Intensity (reverse)"       ); break;
    case VT_SORT_CTS  : strcpy(sText, "Counts"                    ); break;
    case VT_SORT_CTS_R: strcpy(sText, "Counts (reverse)          "); break;
    default           : strcpy(sText, "");
  }
}
VtEvalSort EvalSort_Txt2ID(const char* sText)
{
  VtEvalSort eID=VT_NO_SORT;

       if (strcmp(sText, "Scattering angle"          )==0) eID = VT_SORT_X    ;
  else if (strcmp(sText, "Scattering angle (reverse)")==0) eID = VT_SORT_X_R  ;
  else if (strcmp(sText, "Wavelength/TOF"            )==0) eID = VT_SORT_Y    ;
  else if (strcmp(sText, "Wavelength/TOF (reverse)"  )==0) eID = VT_SORT_Y_R  ;
  else if (strcmp(sText, "Intensity"                 )==0) eID = VT_SORT_INT  ;
  else if (strcmp(sText, "Intensity (reverse)"       )==0) eID = VT_SORT_INT_R;
  else if (strcmp(sText, "Counts"                    )==0) eID = VT_SORT_CTS  ;
  else if (strcmp(sText, "Counts (reverse)          ")==0) eID = VT_SORT_CTS_R;
  
  return eID;
}

// angle selection mode for eval_elast2
void       AngleSel_ID2Txt(char* sText, const VtAngleSel eID)
{
  switch (eID)
  {
    case VT_SEL_DIR: strcpy(sText, "direction"); break;
    case VT_SEL_POS: strcpy(sText, "position" ); break;
    default      : strcpy(sText, "");
  }
}
VtAngleSel AngleSel_Txt2ID(const char* sText)
{
  VtAngleSel eID=VT_NO_SEL;

       if (strcmp(sText, "direction")==0) eID = VT_SEL_DIR;
  else if (strcmp(sText, "position" )==0) eID = VT_SEL_POS;
  
  return eID;
}


