#ifndef DEFINES_H
#define DEFINES_H

#define NN  0

#define NO  0
#define YES 1

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2


typedef enum
{
  NOT_EXIST = -1,  // module is completely ignored
  INACTIVE  =  0,  // module is replaced by space, i.e. instrument length is kept constant
  ACTIVE    =  1,  // module is treated normally
  FIRST_PAR =  2,  // first of an array of parallel modules: lost neutrons are written with their input values, neutrons that passed are written with x > 0
  MIDDLE_PAR=  3,  // module in between other parallel mod.: neutrons with x > 0 are ignored, lost neutrons are written with their input values, neutrons that passed are written with x > 0
  LAST_PAR  =  4   // last of an array of parallel modules : neutrons with x > 0 are ignored, otherwise neutrons are treated normally
}
VtModAct;


typedef enum
{
  LORENTZIAN = 1, 
  GAUSSIAN   = 2
}
VtDistr;


typedef enum
{
  SINGLE_CE     = 1,
  CE_ARRAY_CALC = 2,
  CE_ARRAY_FILE = 3
}
VtMonoGeom;


typedef enum
{
  REFL_MONO   = 1,
  TRANSM_MONO = 2
}
VtMonoType;


typedef enum
{
  CONST_LMBD = 1,
  SPHERICAL  = 2,
  VERT_CYL   = 3,
  DBL_FOC    = 4
}
VtMonoFocus;


typedef enum
{
  NO_MON_PAR = 0,
  MON_LAMBDA = 1,
  MON_TIME   = 2,
  MON_DIV_Y  = 3,
  MON_DIV_Z  = 4,
  MON_Y      = 5,
  MON_Z      = 6,
  MON_ENERGY = 7,
  MON_DIV_YZ = 8,
}
VtMonPar;


typedef enum
{
  NO_PAR   =  0,
  POS_X    = 17,   
  POS_Y    =  1,
  POS_Z    =  2,
  DIV_Y    =  3,
  DIV_Z    =  4,
  LAMBDA   =  5,
  ENERGY   =  6,
  TIME     =  7,
  K_Y      =  8,
  K_Z      =  9,
  POS_R    = 10,
  POS_PHI  = 11,
  DIR_PHI  = 15,
  DIR_THETA= 16,
  COL_VERT = 12,
  COL_HOR  = 13,
  COLOR    = 14,
}
VtPar;


typedef enum
{
  NO_NORM   = 0,   // no normalization
  NORM_SIZE = 1,   // normalization by bin size 
  NORM_REF  = 2    // relative to reference file
}
VtMonNorm;

typedef enum
{
  BRL_ABS    = 1,  // absolute brilliance
  BRL_TRANSF = 2,  // brilliance transfer, i.e. relative to reference file
  BRL_PCT    = 3   // brilliance within 1 percent DelLmbd/Lmbd
}
VtBrlNorm;


typedef enum
{
  NO_FORMAT =-1,
  MATRIX    = 0,
  XYZ       = 1,
  MATR_CMPT = 2,
  XYZ_CMPT  = 3
}
VtFormat2D;

#endif

