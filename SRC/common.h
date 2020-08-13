#ifndef COMMON_H
#define COMMON_H


typedef enum
{
  MCN_COMP_UNKNOWN =   0,
	MCN_SOURCE       = 100,
	MCN_SRC_SMPL     = 110,
	MCN_SRC_CWS      = 120,
	MCN_SRC_TOF      = 130,
	MCN_SRC_SP       = 140,
	MCN_SRC_LP       = 150,
	MCN_READ_IN      = 190,
	MCN_SPACE        = 200,
	MCN_SLIT         = 210,
	MCN_WINDOW       = 220,
	MCN_WND_MULT     = 222,
	MCN_GRID         = 224,
	MCN_LENSE        = 230,
	MCN_MIRROR       = 240,
	MCN_MIRROR_POL   = 242,
	MCN_MIRROR_ELLI  = 244,
	MCN_SM_ENSEMBLE  = 250,
	MCN_COLLIMATOR   = 260,
	MCN_COLL_SOLLER  = 262,
	MCN_COLL_RADIAL  = 264,
	MCN_COLL_VIRT    = 266,
	MCN_GUIDE        = 270,
	MCN_BENDER       = 280,
	MCN_CHOP_DISC    = 310,
	MCN_CHOP_FERMI   = 320,
	MCN_VEL_SELECT   = 330,
	MCN_MONO_ANA     = 350,
	MCN_MONOCHROM    = 352,
	MCN_POL_HE3      = 410,
	MCN_POL_SM       = 412,
	MCN_FLIP_COIL    = 420,
	MCN_FLIP_GRAD    = 422,
	MCN_RES_DRABKIN  = 430,
	MCN_PREC_FIELD   = 450,
	MCN_ROT_FIELD    = 460,
	MCN_SESANS_FIELD = 470,
	MCN_CAPTURE      = 510,
	MCN_BEAMSTOP     = 520,
	MCN_SMPL_ENVIRO  = 530,
	MCN_DETECTOR     = 540,
	MCN_WRITEOUT     = 590,
	MCN_SMPL_EL_ISO  = 610,
	MCN_SMPL_INELAS  = 620,
	MCN_SMPL_SNGL_X  = 630,
	MCN_SMPL_POWDER  = 640,
	MCN_SMPL_S_Q     = 650,
	MCN_SMPL_NXS     = 660,
	MCN_SMPL_SANS    = 670,
	MCN_SMPL_REFL    = 680,
	MCN_FRAME        = 710,
	MCN_FILTER       = 720,
	MCN_RESET        = 730,
	MCN_VISUAL       = 740,
	MCN_MONITOR1     = 800,
	MCN_MON1_BRL     = 810,
	MCN_MON1_POL     = 830,
	MCN_MONITOR2     = 850,
	MCN_MON2_POS     = 852,
	MCN_MON2_DIV     = 854,
	MCN_MON2_KDIV    = 856,
	MCN_MON2_POSDIV  = 860,
	MCN_MON2_HDIVH   = 861,
	MCN_MON2_VDIVV   = 862,
	MCN_MON2_TOFWL   = 865,
	MCN_MON2_WLDIV   = 876,
	MCN_MON2_POL_POS = 882,
	MCN_EVAL1_ELAST  = 900,
	MCN_EVAL1_INELAST= 920,
	MCN_EVAL2_ELAST  = 950,
	MCN_RUNTIME      = 990,
	MCN_TOOL         = 999
}
McCompID;

typedef enum
{
  MCN_MODERATOR = 100, // "mod"
  MCN_TARGET    = 200, // "wnd"
}
McDevID;

typedef enum
{
  MCN_SRC_NAME  = 100, // "name"
  MCN_SRC_POWER = 105, // "power"  unit "MW"
  MCN_MOD_SHAPE = 110, // "shape"  values: 1: "rect"   2: "circ"
  MCN_MOD_TEMP  = 115, // "T"      unit "K"
  MCN_WIDTH     = 120, // "W"      unit "m"
  MCN_HEIGHT    = 125, // "H"      unit "m"
  MCN_CNTR_HOR  = 130, // "CntrW"  unit "m"
  MCN_CNTR_VERT = 135, // "CntrH"  unit "m"
  MCN_CNTR_DIST = 140, // "CntrD"  unit "m"
}
McParID;

void     CompID2Name(char* sName, const McCompID eComp);
McCompID Name2CompID(const char* sName);

#endif

