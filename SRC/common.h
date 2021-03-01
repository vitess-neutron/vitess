#ifndef COMMON_H
#define COMMON_H


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

#endif

