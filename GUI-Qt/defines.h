#ifndef DEFINES_H
#define DEFINES_H

constexpr auto NN = 0;

constexpr auto NO  = 0;
constexpr auto YES = 1;

typedef enum
{
  CWS  = 1,
  SPSS = 2,
  LPSS = 3,
}
VteType;
 
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


#endif

