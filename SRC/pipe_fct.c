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

#include "opt_defs.h"
#include "init.h"
#include "pipe_fct.h"


/*********************************************/
/* global and static variables               */
/*********************************************/
extern char  cQuot,         
             cNL,  
             cShort,
             sInstr   [FN_LEN], // file of the initial instrument
             sFitInfo [FN_LEN], // file containing information about operating system, ... files to copied
             sSimPar  [FN_LEN], // file containing a list of parameters to be varied 
             sExeVFile[FN_LEN], // output file containing all pipe commands
             sLogFile [FN_LEN],
             sPathSl  [FN_LEN],
             sExeDirC [FN_LEN+1],
             sExeDirN [FN_LEN+1],
             sParDirC [FN_LEN+1],
             sParDirN [FN_LEN+1],
             sLogName [FN_LEN+1],
             sPDir    [10],
             sCall    [11],
             sType    [11],
             sDel     [11],
             sCopy    [11],
             sLine    [MAX_MOD][BUF_LEN],/* MAX_MOD strings, each contains 1 exe command (corr. to 1 module)     */
             sFile    [MAX_FIL][50],    /* MAX_FIL strings, each contains 1 name of a file to be copied */
             sSimName [MAX_SIM][50],    /* MAX_SIM strings, each contains 1 name of a single simulations */
             sParList [MAX_SIM][200],   /* MAX_SIM strings, each contains all parameter values for 1 simulatíon */
             sParId   [MAX_PAR][4],     /* MAX_PAR strings, each contains the ID of 1 parameter, e.g. "-n" */
             sParVal  [MAX_PAR][51];    /* MAX_PAR strings, each contains the value of the param., e.g. "20" */
extern short    nModNo[MAX_PAR];        /* MAX_PAR integers, each contains the module no, where the parameter can be found */
//extern VtSystem eSystem;

static char  sOsName      [15]="",
             sBuffer  [BUF_LEN+1]="";


/*********************************************/
/* source code                               */
/*********************************************/

/******************/
/* Initialisation */
/******************/
void InitArrays()
{
	short j=0;

	memset(sLine,   '\0',MAX_MOD*BUF_LEN);
	memset(sFile,   '\0',MAX_FIL*    50);
	memset(sSimName,'\0',MAX_SIM*    50);
	memset(sParList,'\0',MAX_SIM*   200);
	memset(sParId,  '\0',MAX_PAR*     4);
	memset(sParVal, '\0',MAX_PAR*    51);
	for (j=0; j < MAX_PAR; j++)
		nModNo[j]=0;
}

	
/****************************************************************/
/* 'ReadSimPar': Reading the parameter file 'sim_param.ini'     */
/*  output: *pParFct: code for function to transfer 
                      opt. parameters to simulation parameters  */
/*          *pFileNo: number of files to be copied              */
/*  return: number of simulation parameters                     */
/****************************************************************/
short ReadSimPar(char* pParFct, short* pFileNo)
{
	short iPar=0;        /* counts simulation parameters        */
	short ret=TRUE;      
	FILE* pFileR=NULL;

  // open file
	pFileR = fopen(sSimPar, "r");
	if (pFileR==NULL)
	{
		fprintf(LogFilePtr, "\n'%s does not exist\n", sSimPar);
		return(-1);
	}

  // set commands
 #ifdef VT_WINDOWS
    strcpy(sType ,"type");
    strcpy(sCopy ,"copy");
    strcpy(sDel  ,"erase");
    strcpy(sCall ,"call");
    cNL    = '\n';
    cQuot  = ' ';
 #else
    strcpy(sType ,"cat");
    strcpy(sCopy ,"cp");
    strcpy(sDel  ,"rm");
    strcpy(sCall ,"time");
    cNL    = '\n';
		cQuot  = '\"';
 #endif


	// read directories
  ReadLine(pFileR, sBuffer, BUF_LEN);
  if (memcmp(sBuffer,"STD", 3)!=0 && memcmp(sBuffer,"Std", 3)!=0 && memcmp(sBuffer,"std", 3)!=0)
  { StrgCopy(sExeDirN, sBuffer, FN_LEN-1);
    ChangeSlash(sExeDirN);
  }
  ReadLine(pFileR, sBuffer, BUF_LEN);
  if (memcmp(sBuffer,"STD", 3)!=0 && memcmp(sBuffer,"Std", 3)!=0 && memcmp(sBuffer,"std", 3)!=0)
  { StrgCopy(sParDirN, sBuffer, FN_LEN-1);
    ChangeSlash(sParDirN);
  }

	// read files to be copied
	if (ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1))
	{	*pFileNo = (short) StrgScanS(sBuffer, &sFile[0][0], MAX_FIL, 50);
	}
	else
	{	ret=FALSE;
	}

  // read parameter list
  if (ret)
  { // Extract parameter list for each simulation
    ReadLine(pFileR, sBuffer, BUF_LEN);
    StrgCopy(pParFct, sBuffer, KW_LEN);

    // Extract parameter list for each simulation
    while(ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1) && iPar < MAX_SIM)
    {
	    sscanf(sBuffer, "%hd %s", &nModNo[iPar], &sParId[iPar][0]);
	    iPar++;
    }
  }

	fclose(pFileR);

	return iPar;
}


/****************************************************************/
/* 'ReadPData': Reading the parameter sets from 'Pcomm.dat'     */
/*  input :  sFilename: name of the file                        */
/*  output: *pNx : number of optimization parameters (=columns) */
/*          *pMin: first parameter set                          */
/*          *pMax: last parameter set                           */
/*           P   : sets of optimizationt parameters             */
/*  return: number of parameter sets read (=number of simulat.) */
/****************************************************************/
short ReadPData(short* pNx, short* pMin, short* pMax, short* pPrtCmd, double P[MAX_SIM][NMAX], const char* sFilename)
{
	FILE*  pFile;
	short  n,            /* parameters needed : 0    ... Nx */
	       m, nSim=0;    /* simulations needed: mMin ... mMax */

	/* Init */
	for (m=0; m < MAX_SIM; m++)
		for (n=0; n < NMAX; n++)
			P[m][n]=0.0;	
			
	pFile = fopen(sFilename, "r");

	if (pFile!=NULL)
	{	
		nSim = (short) LinesInFile(pFile) - 1;
  	ReadLine(pFile, sBuffer, BUF_LEN);       // skip first line containing control parameters
		*pNx = (short) ColumnsInFile(pFile);

  	ReadLine(pFile, sBuffer, BUF_LEN);
    sscanf  (sBuffer, "%hd %hd", pPrtCmd, pMin);
    *pMax = nSim + *pMin - 1; 

		for (m=*pMin; m <= *pMax; m++)
		{	
			ReadLine  (pFile, sBuffer, BUF_LEN);
			StrgScanLF(sBuffer, P[m], NMAX, 0);
		}
		fclose(pFile);
	}

	return nSim;
}


/****************************************************************/
/* 'ReadCmdFile': Reading the commmand file 'std_instr.cmd'     */
/*  input: bPrtCmd: criterion: print standard instrument        */
/*  return: number of modules (=lines in pipe cmommand)         */
/****************************************************************/
short ReadCmdFile(short bPrtCmd)
{
	short m=0,  /* counts modules           */
        mMax;
	FILE* pFileR=NULL;

  // open file
	pFileR = fopen(sInstr, "r");
	if (pFileR==NULL)
	{	fprintf(LogFilePtr, "\nFile '%s' does not exist\n", sInstr);
		return(-1);
	} 

  // skip 2 header lines
	ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1);
	ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1);
  memset(sBuffer, '\0', sizeof(sBuffer));

  // read pipe command of standard instrument line by line, shorten it and store it in array 'sLine'
	while(ReadLine(pFileR, sBuffer, sizeof(sBuffer)-1) && m < MAX_MOD)
	{	StripCmdLine(sBuffer, cShort);
		strcpy      (sLine[m], sBuffer);
		m++;
    memset(sBuffer, '\0', sizeof(sBuffer));
	}
  mMax=m;

  // print standard instrument, if demanded
  if (bPrtCmd)
  { 
    fprintf(LogFilePtr, "\nstandard instrument\n-------------------\n");
	  for (m=0; m < mMax; m++)
      fprintf(LogFilePtr, "%s\n", sLine[m]);
    fprintf(LogFilePtr, "\n");
  }

	fclose(pFileR);

	return mMax;
}


/************************************************/
/* Writing the command pipe to 'Simlations.bat' */
/************************************************/
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
	

/*************************************************************************************************/
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
		    if( strcmp(sComp, "--")==0){
		      k+=2;
		      continue;
		    }
			// looking for the beginning and the end of the parameter
			if (strcmp(sComp, sParId[j])==0)
			{	kBeg = (short) (k + strlen(sParId[j]));
				pEnd = strchr(sLine[m]+k, ' ');
				k=kMax;
			}	
		}
		if (kBeg > 0)
			SubstPar(sLine[m], kBeg, pEnd, sParVal[j]);
		else
			fprintf(LogFilePtr, "\nERROR: parameter %s in module %d not found\n", sParId[j], nModNo[j]);
		j++;
	}

	return j;
}


/*********************************************************/
/* Change and shortening of one part of the pipe command */
/*********************************************************/
void
StripCmdLine(char* const pLine, char cSh)
{
	char  *pBlank, *pSlash, sLocBuf[BUF_LEN]="";
	int   kBlank;
	short rc;

	// change '/' to the right slash
	ChangeSlash(pLine);

	// delete leading line feeds and '|'
#ifdef SLOW_COMPUTER
	{
	  int  k;
    char v;
	  for (k=0; (v = pLine[k]) && (v==' ' || v=='|'); k++) ;
	  if (k > 0)
	    StrgLShift(pLine, k);
	}
#else
	while (pLine[0]==' ' || pLine[0]=='|')
	{	StrgLShift(pLine, 1);
	}
#endif

	// extract the PATH directory from the command
	if (strlen(sExeDirC)==0)
	{	pBlank = strchr(pLine, ' ');
		kBlank = pBlank-pLine;
		StrgCopy(sPathSl, pLine, kBlank);

		pSlash = strrchr(sPathSl, cSlash)+1;
		*pSlash= '\0';
		EraseEndSlash(sExeDirC, sPathSl);
	}

	// extract the DEFAULT directory from the command
	if (strlen(sParDirC)==0)
	{	
		pBlank = strchr(pLine, ' ');
		while (memcmp(pBlank, " --P", 4)!=0)
		{	strcpy(sLocBuf, pBlank+3);
			pBlank = strchr(sLocBuf, ' ');
		}
		strcpy(sLocBuf, pBlank+4);
		pBlank = strchr(sLocBuf, ' ');
		kBlank = pBlank-sLocBuf;
		StrgCopy(sPathSl, sLocBuf, kBlank);
		EraseEndSlash(sParDirC, sPathSl);
	}

	// extract the LOG directory from the command
	if (strlen(sLogName)==0)
	{	
		pBlank = strchr(pLine, ' ');
		while (memcmp(pBlank, " --L", 4)!=0)
		{	strcpy(sLocBuf, pBlank+3);
			pBlank = strchr(sLocBuf, ' ');
		}
		strcpy(sLocBuf, pBlank+4);
		pBlank = strchr(sLocBuf, ' ');
		kBlank = pBlank-sLocBuf;
		StrgCopy(sPathSl, sLocBuf, kBlank-1);
		EraseEndSlash(sLogName, sPathSl);
	}

	// substitute PATH and DEFAULT directory by abbrevations
#ifdef VT_WINDOWS
  StrgChange(pLine, sExeDirC, "V:");
	do
	{ rc=StrgChange(pLine, sParDirC, "P:");
	} 
	while (rc==TRUE);
#else
	StrgChange(pLine, sExeDirC, "$V");
	do
	{ rc=StrgChange(pLine, sParDirC,  "$P");
	} 
	while (rc==TRUE);
#endif

	// Shorten the command
	if (cSh=='V')
		StrgChange(pLine, " --B10000", "");	
}


/********************************************************************************************************/
/* substitution of the parameter at pos. 'kBeg' by "%"'nVar' (2 files option) or 'pVar' (1 file option) */
/********************************************************************************************************/
void
SubstPar(char* pLine, short kBeg, const char* pEnd, const char* pVar)
{
	char sLeftPart[BUF_LEN], sRightPart[BUF_LEN]="";

	if (pEnd != NULL)
		strcpy(sRightPart, pEnd);
	StrgCopy  (sLeftPart,  pLine, kBeg);
	sprintf(pLine, "%s%s%s", sLeftPart, pVar, sRightPart);
}


/***********************************************************/
/* Elimination of the end slash                            */
/***********************************************************/
void  EraseEndSlash(char* pStr, char* const pStrSl)
{
	int klen;

	strcpy(pStr, pStrSl);
	klen = strlen(pStr);

	if (pStr[klen-1]==cSlash)
		pStr[klen-1]='\0';
}


/******************************************************************************/
/* Change of (the first occurence of) string 'sOut' in String 'sStr' to 'sIn' */
/******************************************************************************/
short
StrgChange(char* sStr, const char* sOut, const char* sIn)
{
	short rc=FALSE;
	int   k, kout, kmax;
	char  sRest[BUF_LEN];

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


/*************************************************************************/
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


/***********************************************************************************************/
/* Building a combined file name of 'sFileShort' and 'sAddition' without changing the extension */
/***********************************************************************************************/
void
ExtendFilename(char* sFileLong, char* sFileShort, char* sAddition)

{
   char sParExt [4],      // extension of file names (with simulation results)
        sParName[FN_LEN];     // name (without extension) of those files

	strcpy  (sParExt,  sFileShort +strlen(sFileShort)-3);
	StrgCopy(sParName, sFileShort, maxi(0,strlen(sFileShort)-4));
	sprintf (sFileLong, "%s%s.%s", sParName, sAddition, sParExt);
}


/********************************************************************************/
/* Scanning of the string 'sStr' and copying of all values (but maximally 'nMax') 
   to list 'pTab' of short values,  beginning with value number 'nStart'        */
/********************************************************************************/
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


/********************************************************************************/
/* Scanning of the string 'sStr' and copying of all values (but maximally 'nMax') 
   to list 'pTab' of string values,  beginning with value number 'nStart'       */
/********************************************************************************/
long
StrgScanS(const char* sStr, char* pTab, const int nMax, const int nTextLen)
{
  const char   *pStr;
  char sNumber[50];
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
