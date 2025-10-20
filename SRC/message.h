#ifndef MESSAGE_H
#define MESSAGE_H

#include "general.h"


/********************************/
/** definitions and structures **/
/********************************/

#define MSG_LEN 255
#define MAX_MSG  10

typedef enum
{
  ALL_BEHIND_COMPONENT   = 001,
  ALL_BEHIND_BEG_COMP    = 002,
  ALL_NEGATIVE_INT       = 003,
  ALL_L_RANGE_TOO_SMALL  = 010,
  SRC_L_RANGE_TOO_SMALL  = 100,
  SRC_T_RANGE_TOO_SMALL  = 101,
  SRC_LT_RANGE_TOO_SMALL = 102,
  SRC_Y_RANGE_TOO_SMALL  = 110,
  GUID_OUT_OF_EXIT       = 200,
  GUID_NO_PLANE          = 201,
  CHOP_PASSED_OUTSIDE    = 300,
  SELECT_OUTSIDE         = 310,
  SELECT_NO_BLADES       = 311,
  WNDO_L_RANGE_TOO_SMALL = 402,
  WNDI_L_RANGE_TOO_SMALL = 403,
  WND_CROSS_TALK         = 410,
  WND_PASSED_OUTSIDE     = 420,
  MON_ZERO_BIN_SIZE      = 500,
  SMPL_Q_RANGE_TOO_SMALL = 800,
  SMPL_TRAJ_INSIDE       = 801,
  ENV_TRAJ_INSIDE        = 802,
  ENV_TRAJ_OUTSIDE       = 803,
  DET_TRAJ_INSIDE        = 901,
  DET_L_RANGE_TOO_SMALL  = 902
}
VtMsgID;

typedef struct
{
  VtMsgID  eID;
  long     nNumber;
  TotalID  TrajID;
}
VtMessage;


/******************************/
/** Prototypes               **/
/******************************/

void MsgInit       ();
void CountMessage  (VtMsgID eErrID);
void CountMessageID(VtMsgID eErrID, TotalID eTrajID);
void CountMessageID_C(VtMsgID eErrID, TotalID eTrajID, int count);
void PrintMessage  (VtMsgID eErrID, const char* pText, short bID);


#endif
