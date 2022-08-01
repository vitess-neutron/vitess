/********************************************************************************************/
/*  Functions 'gener_fct.c'                                                                 */
/*    Some functions for 'gener_batch' and 'gener_fit'                                      */
/*                                                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,                                     */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin                                        */
/*                                                                                          */
/* 1.0: Jan 2002 K. Lieutenant   routines collected here                                    */
/********************************************************************************************/

#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "gener_fct.h"
#include "general.h"

/*********************************************/
/* global and static variables               */
/*********************************************/
static char  sOsName      [15]="",
             sBuffer  [BUFLEN]="";


extern char  cQuot,
             cNL,
             cShort,
             sCommName[50],
             sLogFile [18],
             sPathSl  [CHAR_BUF_SMALL],
             sPath    [CHAR_BUF_SMALL],
             sDirSl   [CHAR_BUF_SMALL],
             sDir     [CHAR_BUF_SMALL],
             sPDir    [10],
             sCall    [11],
             sType    [11],
             sDel     [11],
             sCopy    [11],
             sLine    [MAX_MOD][BUFLEN],/* MAX_MOD strings, each contains 1 exe command (corr. to 1 module)     */
             sFile    [MAX_FIL][50],    /* MAX_FIL strings, each contains 1 name of a file to be copied */
             sSimName [MAX_SIM][50],    /* MAX_SIM strings, each contains 1 name of a single simulations */
             sParList [MAX_SIM][200],   /* MAX_SIM strings, each contains all parameter values for 1 simulatíon */
             sParId   [MAX_PAR][4],     /* MAX_PAR strings, each contains the ID of 1 parameter, e.g. "-n" */
             sParVal  [MAX_PAR][51];    /* MAX_PAR strings, each contains the value of the param., e.g. "20" */
extern short    nModNo[MAX_PAR];        /* MAX_PAR integers, each contains the module no, where the parameter can be found */
extern VtModus  eModus;
extern VtSystem eSystem;


/*********************************************/
/* source code                               */
/*********************************************/

/* Initialisation */
/******************/
void InitArrays()
{
	short j=0;

	memset(sLine,   '\0',MAX_MOD*BUFLEN);
	memset(sFile,   '\0',MAX_FIL*    50);
	memset(sSimName,'\0',MAX_SIM*    50);
	memset(sParList,'\0',MAX_SIM*   200);
	memset(sParId,  '\0',MAX_PAR*     4);
	memset(sParVal, '\0',MAX_PAR*    51);
	for (j=0; j < MAX_PAR; j++)
		nModNo[j]=0;
}


/* Reading the commmand file xxx.bat */
/*************************************/
short ReadBatchFile()
{
	short m=0;  /* counts modules           */
	FILE* pFileR=NULL;
	char  sFileName    [60]="";

	sprintf(sFileName, "%s.bat", sCommName);
	pFileR = fopen(sFileName, "r");
	if (pFileR==NULL)
	{	printf("\nFile '%s' does not exist\n", sFileName);
		return(-1);
	}
	GetLine(pFileR, sBuffer);
	GetLine(pFileR, sBuffer);

	while(GetLine(pFileR, sBuffer) && m < MAX_MOD)
	{	StripCmdLine(sBuffer, cShort);
		strcpy      (sLine[m], sBuffer);
		m++;
	}

	fclose(pFileR);

	return m;
}


/* Reading data from the parameter file */
/****************************************/
short ReadInfoFile(short* pFileNo, char* sSeriesname)
{
  short i=0;        /* counts simulations        */
  int	nNoFiles=0; /* number of batch files to be generated (1 or 2) */
  char  sFileName[60]="";
  FILE* pFileR=NULL;

  sprintf(sFileName, "%s.inf", sCommName);
  pFileR = fopen(sFileName, "r");
  if (pFileR==NULL) {
    printf("\nFile '%s' does not exist\n", sFileName);
    return(-1);
  }

  // Operating System
  GetLine(pFileR, sBuffer);
  sscanf(sBuffer, "%s %d", sOsName, &nNoFiles);
  if (strcmp("Unix", sOsName)==0 || strcmp("UNIX", sOsName)==0) {
    eSystem = VT_UNIX;
  } else if (strcmp("Linux", sOsName)==0) {
    eSystem = VT_LINUX;
  } else if (strcmp(sOsName, "NT")==0 || strcmp(sOsName, "WinNT")==0) {
    eSystem = VT_WIN_NT;
  } else if (strcmp(sOsName, "Win98")==0) {
    eSystem = VT_WIN_98;
  }
  if (eSystem == VT_UNIX || eSystem == VT_LINUX) {
    strcpy(sType ,"cat");
    strcpy(sCopy ,"cp");
    strcpy(sDel  ,"rm");
    strcpy(sCall ,"time");
    cNL    = '\n';
    cQuot  = '\"';
  } else {
    strcpy(sType ,"type");
    strcpy(sCopy ,"copy");
    strcpy(sDel  ,"erase");
    strcpy(sCall ,"call");
    cNL    = '\n';
    cQuot  = ' ';
    if (nNoFiles==2 && eModus == VT_SER_1F)
      eModus = VT_SER_2F;
  }

  GetLine(pFileR, sBuffer);
  sscanf(sBuffer, "%c", &cShort);
  if (eSystem == VT_UNIX || eSystem == VT_LINUX) {
    if (cShort=='V')
      strcpy(sLogFile, "/tmp/L");
    else if (cShort=='S')
      strcpy(sLogFile, "/tmp/log");
    else
      strcpy(sLogFile, "/tmp/vpipelog");
  } else {
    if (cShort=='V')
      strcpy(sLogFile, "C:\\L");
    else if (cShort=='S')
      strcpy(sLogFile, "C:\\temp\\log");
    else
      strcpy(sLogFile, "C:\\temp\\vpipelog");
  }
  GetLine(pFileR, sDirSl);
  ChangeSlash  (sDirSl);
  EraseEndSlash(sDir, sDirSl);
  GetLine(pFileR, sSeriesname);

  // Files to be copied
  GetLine(pFileR, sBuffer);
  *pFileNo = (short) StrgScanS(sBuffer, &sFile[0][0], MAX_FIL, 50);

  // variable parameters
  GetLine(pFileR, sBuffer);
  StrgScanHD(sBuffer, nModNo, MAX_PAR);
  GetLine(pFileR, sBuffer);
  StrgScanS (sBuffer, &sParId[0][0], MAX_PAR, 4);

  // Extract parameter list (without leading blanks) for each simulation
  while(GetLine(pFileR, sBuffer) && i < MAX_SIM)
    {
      int n;
      sscanf(sBuffer, "%s%n", sSimName[i], &n);
      while (sBuffer[n] == ' ')
	n++;
      strcpy(sParList[i], sBuffer+n);
      i++;
    }
  fclose(pFileR);

  return i;
}


/* Writing the command pipe */
/****************************/
void WriteCommand(FILE* pFile, short nMod, short bEcho)
{
	short m;
	
	if (bEcho)
	{	fprintf(pFile, "%c", cNL);
		for (m=0; m < nMod; m++)
		{	fprintf(pFile, "echo %s >> %sHistory.txt%c", sLine[m], sPDir, cNL);
		}
	}

	fprintf(pFile, "%s", sLine[0]);
	for (m=1; m < nMod; m++)
	{	
		fprintf(pFile, " | %s", sLine[m]);
	}
	fprintf(pFile, "%c", cNL);
}
	

/* Changing the command file xxx.bat to the lower level batch file xxxV.bat (2 files option) or
   to the command with the parameters of the 'iSim'th simulation (1 file option)                 */
/*************************************************************************************************/
short ChangeParam(short iSim)
{
	short j,   /* counts number of varied parameters */
	      k, kBeg, kMax, m;
	char *pEnd,
	      sComp  [4];

	/* for the 1-file-series and the fit option the parameters must be scanned first */
	if (iSim>=0 && iSim < MAX_SIM)	
		StrgScanS(sParList[iSim], &sParVal[0][0], MAX_PAR, 51);
	j=0;
	while (sParId[j]!=NULL && strlen(sParId[j]) > 0)
	{	// determine the line number from the module number
		m = (short) (nModNo[j]-1);
		pEnd = NULL;
		kBeg = 0;
		kMax = (short) (strlen(sLine[m])-2);
		// searching for the parameter in the line
		for (k=0; k < kMax; k++)
		{	StrgCopy(sComp, sLine[m]+k, strlen(sParId[j]));
			// looking for the beginning and the end of the parameter
			if (strcmp(sComp, sParId[j])==0)
			{	kBeg = (short) (k + strlen(sParId[j]));
				pEnd = strchr(sLine[m]+k, ' ');
				k=kMax;
			}	
		}
		if (kBeg > 0)
			SubstPar(sLine[m], kBeg, pEnd, j+1, sParVal[j]);
		else
			printf("\nERROR: parameter %s in module %d not found\n", sParId[j], nModNo[j]);
		j++;
	}

	return j;
}


/* Read next line from info file or batch file */
/***********************************************/
int
GetLine(FILE* pFile, char* const pLine)
{
  int rc;

  if ((rc = ReadLine(pFile, sBuffer, BUFLEN)))
    strcpy(pLine, sBuffer);
  return rc;
}


/* Change and shorten commandline */
/**********************************/
void
StripCmdLine(char* const pLine, char cSh)
{
	char  *pBlank, *pSlash;
	int   kBlank;
	short rc;

	// change '/' to the right slash
	ChangeSlash(pLine);

	// delete leading line feeds and " | "
	{
	  int k,v;
	  for (k=0; (v = pLine[k]) && (v==' ' || v=='|'); k++) ;
	  if (k)
	    strcpy(pLine, pLine+k);
	}

	// extract the PATH directory from the command
	if (strlen(sPath)==0)
	{	pBlank = strchr(pLine, ' ');
		kBlank = pBlank-pLine;
		StrgCopy(sPathSl, pLine, kBlank);

		pSlash = strrchr(sPathSl, cSlash)+1;
		*pSlash= '\0';
		EraseEndSlash(sPath, sPathSl);
	}

	// substitute PATH and default directory by abbrevations
	if (eSystem == VT_UNIX || eSystem == VT_LINUX)
	{	StrgChange(pLine, sPath, "$VIMG");
		do
		{ rc=StrgChange(pLine, sDir,  "$PDIR");
		}
		while (rc==TRUE);
	}
	else
	{	StrgChange(pLine, sPath, "V:");
		do
		{ rc=StrgChange(pLine, sDir, "P:");;
		}
		while (rc==TRUE);
	}

	// Shorten the command
	if (cSh=='V')
		StrgChange(pLine, " --B10000", "");	

	if (eSystem == VT_UNIX || eSystem == VT_LINUX)
	{	char *p1, *p2, sLog[20]="";
		p1 = StrgFind  (pLine, "--L/tmp");
		p2 = StrgFind  (pLine, "vpipelog");
		StrgCopy  (sLog, p1+3, p2-p1+5);
		StrgChange(pLine, sLog, sLogFile);	
	}
	else
	{	StrgChange(pLine, "C:\\temp\\vpipelog", sLogFile);	
	}
	
}


/* substuting the parameter at pos. 'kBeg' by "%"'nVar' (2 files option) or 'pVar' (1 file option) */
/***************************************************************************************************/
void
SubstPar(char* pLine, short kBeg, const char* pEnd, int nVar, const char* pVar)
{
	char sLeftPart[BUFLEN], sRightPart[BUFLEN]="";

	if (pEnd != NULL)
		strcpy(sRightPart, pEnd);
	StrgCopy  (sLeftPart,  pLine, kBeg);
	if (eModus==VT_SER_2F)
		sprintf(pLine, "%s%%%d%s", sLeftPart, nVar, sRightPart);
	else
		sprintf(pLine, "%s%s%s", sLeftPart, pVar, sRightPart);
}


/* Changing the Slashes to the right ones, e.g. '\' to '/' */
/***********************************************************/
void  EraseEndSlash(char* pStr, char* const pStrSl)
{
	int klen;

	strcpy(pStr, pStrSl);
	klen = strlen(pStr);

	if (pStr[klen-1]==cSlash)
		pStr[klen-1]='\0';
}


/* Change (the first occurence of) string 'sOut' in String 'sStr' to 'sIn' */
/***************************************************************************/
short
StrgChange(char* sStr, const char* sOut, const char* sIn)
{
	short rc=FALSE;
	int   k, kout, kmax;
	char  sRest[BUFLEN];

	kout = strlen(sOut);
	kmax = strlen(sStr)-kout;

	for (k=0; k < kmax; k++)
	{
		if (memcmp(&sStr[k], sOut, kout)==0)
		{
			strcpy(sRest, &sStr[k+kout]);
			sprintf(&sStr[k], "%s%s", sIn, sRest);
			k =kmax;
			rc=TRUE;
		}
	}
	return rc;
}


/* Search for (the first occurence of) string 'sSearch' in String 'sStr' */
/*************************************************************************/
char*
StrgFind(char* sStr, const char* sSearch)
{
	int   k, ksrch, kmax;
	char* pFind=NULL;

	ksrch = strlen(sSearch);
	kmax  = strlen(sStr)-ksrch;

	for (k=0; k < kmax; k++)
	{
		if (memcmp(&sStr[k], sSearch, ksrch)==0)
		{
			pFind = &sStr[k];
			k=kmax;
		}
	}
	return pFind;
}


/* Building a combined file name of 'sFilename' and 'sAddition' without changing the extension */
/***********************************************************************************************/
void
NumerateName (char* sFileLong, char* sFileShort, const short nNumber)

{
   char sParExt [4],      // extension of file names (with simulation results)
        sParName[99];     // name (without extension) of those files

	strcpy  (sParExt,  sFileShort +strlen(sFileShort)-3);
	StrgCopy(sParName, sFileShort, strlen(sFileShort)-4);
	sprintf (sFileLong, "%s%hd.%s", sParName, nNumber, sParExt);
}



/* Scan string 'sStr' and copy all values (but maximally 'nMax')
   to list 'pTab' of short values,  beginning with value number 'nStart'  */
/**************************************************************************/
long
StrgScanHD(const char* sStr, short* pTab, const int nMax)
{
	long   k, n=0;
	const char *pStr;
	char sNumber[31];

	pStr = sStr;
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
				pTab[n] = (short) atol(sNumber);
			n++;	
			pStr += k;
		}
	}
	while (n < nMax && k > 0);

	return(n);
}


/* Scan string 'sStr' and copy all values (but maximally 'nMax')
   to list 'pTab' of string values,  beginning with value number 'nStart'*/
/**************************************************************************/
long
StrgScanS(const char* sStr, char* pTab, const int nMax, const int nTextLen)
{
  const char   *pStr;
  char sNumber[31];
  long   i, k, n=0;

	pStr = (const char*) sStr;
	do
	{	/* search of beginning and end of 1st number of (remaining) string */
		i=0;k=0;
		/* step forward until first alphanumerical character or point-like character */
		while (isalnum(pStr[i+k])==0 && ispunct(pStr[i+k])==0 && iscntrl(pStr[i+k])==0)
			i++;
		/* step forward until space-like or control character */
		while (isspace(pStr[i+k])==0 && iscntrl(pStr[i+k])==0)
			k++;

		/* separating first text and adding it to the list */
		if (k > 0)
		{	
			StrgCopy(sNumber, pStr+i, k);
			if (n >= 0)
				strcpy(&pTab[n*nTextLen], sNumber);
			n++;	
			pStr += (i+k);
		}
	}
	while (n < nMax && k > 0);

	return(n);
}
