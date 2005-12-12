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


/*********************************/
/** global and static variables **/
/*********************************/

VtMessage stMessage[12];              /* up to 11 messages can be treated (0 not used)  */
short     nMsgNo=0;                   /* counts the number of messages that are treated */
char      sMsgText[MESSAGE_LEN+1]=""; /* message text build of table text and data given by error */


/********************************************************************************************/
/***  global functions                                                                    ***/
/********************************************************************************************/

/********************************************************************************************/
/*  function 'MsgInit'                                                                      */
/*    initializes the message list                                                          */
/********************************************************************************************/

void MsgInit()
{
	memset(stMessage, '\0', 10*sizeof(VtMessage));
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
	{	stMessage[n].eID = eErrID;
	}
}
	
void CountMessageID(VtMsgID eErrID, TotalID eTrajID)
{
	short n=LfdNo(eErrID);

	stMessage[n].nNumber++;
	if (stMessage[n].nNumber==1)
	{	stMessage[n].eID = eErrID;
		stMessage[n].TrajID.IDNo = eTrajID.IDNo;
		strcpy(stMessage[n].TrajID.IDGrp, eTrajID.IDGrp);
	}
}


/********************************************************************************************/
/*  function 'PrintMessage'                                                                 */
/*    prints the message with the given ID including the number of appearances              */
/*                                                                                          */
/*  input:  eErrID: ID of the message  (see enum VtMsgID in message.h)                      */
/*          bID   : ON : ID of the first trajectory causing this message is added, if saved */
/*                  OFF: noting else is done                                                                        */
/********************************************************************************************/

void PrintMessage(VtMsgID eErrID, const char* pText, short bID)
{
	char  sText[MESSAGE_LEN+10], cMessType;
	short n=GetLfdNo(eErrID);
	
	if (n > 0)
	{	ReadMessageText(eErrID, sText, &cMessType);
		switch (cMessType)
		{	case 'E': sprintf(sMsgText, "\nError: %s.\n", sText); break;
		 	case 'W': sprintf(sMsgText, "\nWarning: %s.\n", sText); break;
			case 'N': sprintf(sMsgText, "\nNote: %s.\n", sText); break;
			case '-': sprintf(sMsgText, "\nError %d occured in %ld trajectories.\n", 
			                              eErrID, stMessage[n].nNumber); break;
			default : sprintf(sMsgText, "\n%s.\n", sText);
		}
#ifdef VERS26
		if (strlen(pText) > 0)
		{	if (stMessage[n].nNumber > 1)
				fprintf(LogFilePtr, sMsgText, stMessage[n].nNumber, "ies",pText);
			else
				fprintf(LogFilePtr, sMsgText, stMessage[n].nNumber, "y",  pText);
		}
		else
		{	if (stMessage[n].nNumber > 1)
				fprintf(LogFilePtr, sMsgText, stMessage[n].nNumber, "ies");
			else
				fprintf(LogFilePtr, sMsgText, stMessage[n].nNumber, "y");
		}
		if (bID==ON && stMessage[n].TrajID.IDNo > 0)
		{	if (stMessage[n].nNumber > 1)
				fprintf(LogFilePtr, "First trajectory has ID %c%c%09lu.\n", 
						  stMessage[n].TrajID.IDGrp[0], stMessage[n].TrajID.IDGrp[1], stMessage[n].TrajID.IDNo);
			else
				fprintf(LogFilePtr, "Trajectory has ID %c%c%09lu.\n", 
						  stMessage[n].TrajID.IDGrp[0], stMessage[n].TrajID.IDGrp[1], stMessage[n].TrajID.IDNo);
		}
#else
		{
		  int nid = stMessage[n].TrajID.IDNo, nno = stMessage[n].nNumber;
		  fprintf(LogFilePtr, sMsgText, nno, nno > 1 ? "ies" : "y", pText);
		  if (bID==ON && nid > 0)
		    fprintf(LogFilePtr, nno > 1 ? "First trajectory has ID %c%c%09lu.\n" : "Trajectory has ID %c%c%09lu.\n",
			    stMessage[n].TrajID.IDGrp[0], stMessage[n].TrajID.IDGrp[1], nid);
		}
#endif
	}
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
	FILE* pFile = NULL;
	char  sLine[MESSAGE_LEN+3]="", c;
	int   eTabID=0;
	short rc=FALSE, trc=FALSE;

	strcpy(pText,"");
	*pType='-'; 

	pFile = fopen(FullInstallName("ErrorTable.dat","FILES/"), "r");
	if (pFile != NULL)
	{
		do
		{	
			trc = (short) ReadLine(pFile, sLine, MESSAGE_LEN);
			if (trc) 
				sscanf(sLine, "%3d%c%c", &eTabID, &c, pType); 
		}
		while (eID != eTabID  && trc==TRUE);
		if (eID==eTabID)
		{	rc=TRUE;
#ifdef VERS26
			StrgLShift(sLine, 6);
			strcpy(pText, sLine);
#else
			strcpy(pText, sLine+6);
#endif
		}
		else
		{	*pType='-'; 
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
	{
		if (eID==stMessage[i].eID)
			return(i);
	}
	return(++nMsgNo);
}

static short GetLfdNo(VtMsgID eID)
{
	short i;

	for (i=1; i <= nMsgNo; i++)
	{
		if (eID==stMessage[i].eID)
			return(i);
	}
	return(0);
}
