/********************************************************************************************/
/*  VITESS module 'message.c'                                                               */
/*    Error functions for all VITESS modules                                                */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler, Michael Fromme, Klaus Lieutenant,   */
/* , Sergey Manoshin                                                                        */
/*                                                                                          */
/* Jan 2004  K. Lieutenant  initial version                                                 */
/********************************************************************************************/

#include "message.h"
#include "init.h"


/******************************/
/** Prototypes               **/
/******************************/

static short LfdNo          (VtMsgID eID);
static short GetLfdNo       (VtMsgID eID);
static short ReadMessageText(VtMsgID eID, char* sText, char* cType);

char* FullInstallName (const char* filename, const char* sRelPath); // adds installation directory to file name 


/*********************************/
/** global and static variables **/
/*********************************/

VtMessage stMessage[MAX_MSG];              /* up to 11 messages can be treated (0 not used)  */
short     nMsgNo=0;                   /* counts the number of messages that are treated */
char      sMsgText[MSG_LEN+1]=""; /* message text build of table text and data given by error */


/********************************************************************************************/
/***  global functions                                                                    ***/
/********************************************************************************************/

/********************************************************************************************/
/*  function 'MsgInit'                                                                      */
/*    initializes the message list                                                          */
/********************************************************************************************/

void MsgInit()
{
  memset(stMessage, 0, MAX_MSG*sizeof(VtMessage));
}


/********************************************************************************************/
/*  functions 'CountMessage'                                                                */
/*            'CountMessageID'                                                              */
/*    count number of messages with the given ID                                            */
/*                                                                                          */
/*  input:  eErrID:  ID of the message  (see enum VtMsgID in message.h)                     */
/*                                                                                          */
/*  'CountMessageID' additionly saves the ID of the first trajectory (causing this message) */
/********************************************************************************************/

void CountMessage(VtMsgID eErrID)
{
  short n=LfdNo(eErrID);

  stMessage[n].nNumber++;
  if (stMessage[n].nNumber==1)
    stMessage[n].eID = eErrID;
}
	
void CountMessageID(VtMsgID eErrID, TotalID eTrajID)
{
  short n=LfdNo(eErrID);

  stMessage[n].nNumber++;
  if (stMessage[n].nNumber==1) {
    stMessage[n].eID = eErrID;
    stMessage[n].TrajID.IDNo = eTrajID.IDNo;
    StrgCopy(stMessage[n].TrajID.IDGrp, eTrajID.IDGrp, 2);
  }
}

void CountMessageID_C(VtMsgID eErrID, TotalID eTrajID, int count)
{
  short n = LfdNo(eErrID);
  if (0 == stMessage[n].nNumber) {
    stMessage[n].eID = eErrID;
    stMessage[n].TrajID.IDNo = eTrajID.IDNo;
    StrgCopy(stMessage[n].TrajID.IDGrp, eTrajID.IDGrp, 2);
  }
  stMessage[n].nNumber += count;
}


/********************************************************************************************/
/*  function 'PrintMessage'                                                                 */
/*    prints the message with the given ID including the number of appearances              */
/*                                                                                          */
/*  input:  eErrID: ID of the message  (see enum VtMsgID in message.h)                      */
/*          bID   : ON : ID of the first trajectory causing this message is added, if saved */
/*                  OFF: noting else is done                                                */
/********************************************************************************************/

void PrintMessage(VtMsgID eErrID, const char* pText, short bID)
{
  char  sText[MSG_LEN+10], cMessType='-';
  short n=GetLfdNo(eErrID);
  int   nTr=0;
	
  if (n <= 0) return;

  ReadMessageText(eErrID, sText, &cMessType);
  switch (cMessType)  
  {
    case 'E': snprintf(sMsgText, MSG_LEN, "\nError: %s.\n", sText);   break;
    case 'W': snprintf(sMsgText, MSG_LEN, "\nWarning: %s.\n", sText); break;
    case 'N': snprintf(sMsgText, MSG_LEN, "\nNote: %s.\n", sText);    break;
    case '-': snprintf(sMsgText, MSG_LEN, "\nError %d occurred in %ld trajectories.\n",
                      eErrID, stMessage[n].nNumber);        break;
    default : snprintf(sMsgText, MSG_LEN, "\n%s.\n", sText);
  }

  nTr = stMessage[n].nNumber;
  fprintf(LogFilePtr, sMsgText, nTr, nTr > 1 ? "ies" : "y", pText);
  if (bID==ON && stMessage[n].TrajID.IDNo > 0)
    fprintf(LogFilePtr, nTr > 1 ? "First trajectory has ID %c%c%09lu.\n" : "Trajectory has ID %c%c%09lu.\n",
            stMessage[n].TrajID.IDGrp[0], stMessage[n].TrajID.IDGrp[1], stMessage[n].TrajID.IDNo);
}



/********************************************************************************************/
/***   local functions                                                                    ***/
/********************************************************************************************/

/********************************************************************************************/
/* function 'ReadMessageText'                                                               */
/*   reads text and type of message from table 'FILES/ErrorTable.dat'                       */
/*                                                                                          */
/* returns TRUE (text found) or FALSE                                                       */
/* input:  eID:  ID of the message  (see enum VtMsgID in message.h)                         */
/* output: pText: message text                                                              */
/*         pType: type of message  ('E':error 'W':warning 'N':note '-':table / ID not found)*/
/********************************************************************************************/

static short ReadMessageText(VtMsgID eID, char* pText, char* pType)
{
  FILE* pFile=NULL;
  char  sLine[MSG_LEN+3]="", c;
  short rc=FALSE;
  int   eTabID=0;

  strcpy(pText, "");
  *pType = '-';

  pFile = OpenPackInpFile("ErrorTable.dat","FILES/", FALSE);
  if (pFile != NULL) 
  {
    do 
    {	
      if (ReadLine(pFile, sLine, MSG_LEN))
        sscanf(sLine, "%3d%c%c", &eTabID, &c, pType);
      else
        break;
    }
    while (eID != eTabID);

    if (eID==eTabID) 
    { strcpy(pText, sLine+6);
      rc=TRUE;
    }

    fclose(pFile);
  }

  return rc;
}


/********************************************************************************************/
/*  functions 'LfdNo'                                                                       */
/*            'GetLfdNo'                                                                    */
/*                                                                                          */
/*  return number of current message,                                                       */
/*  input:  eID:  ID of the message  (see enum VtMsgID in message.h)                        */
/*                                                                                          */
/*  if ID is not found: GetLfdNo returns 0                                                  */
/*                      LfdNo    increases number of treated messages and returns this value*/
/********************************************************************************************/

static short LfdNo(VtMsgID eID)
{
  short i;

  for (i=1; i <= nMsgNo; i++)
    if (eID==stMessage[i].eID)
      return i;

  return ++nMsgNo;
}

static short GetLfdNo(VtMsgID eID)
{
  short i;

  for (i=1; i <= nMsgNo; i++)
    if (eID==stMessage[i].eID)
      return i;

  return 0;
}
