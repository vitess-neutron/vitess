#ifndef MESSAGE_H
#define MESSAGE_H

#include "general.h"


/********************************/
/** definitions and structures **/
/********************************/

#define MESSAGE_LEN 255

typedef enum
{	
	ALL_BEHIND_COMPONENT   = 001,
	ALL_BEHIND_BEG_COMP    = 002,
	SRC_L_RANGE_TOO_SMALL  = 100,
	SRC_T_RANGE_TOO_SMALL  = 101,
	SRC_LT_RANGE_TOO_SMALL = 102,
	GUID_OUT_OF_EXIT       = 200,
	CHOP_PASSED_OUTSIDE    = 300,
	SMPL_Q_RANGE_TOO_SMALL = 800,
	SMPL_TRAJ_INSIDE       = 801,
	DET_TRAJ_INSIDE        = 901
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
void PrintMessage  (VtMsgID eErrID, const char* pText, short bID);


#endif
