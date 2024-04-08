/********************************************************************/
/* Tool 'gener_batch':                                              */
/*  Generation of a batch file for a series of simulations          */ 
/*                                                                  */
/********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gener_fct.h"
#include "general.h"

// static functions
void  PrintDate   (FILE* pFile, const char* pDir);
void  OwnInit(int argc, char *argv[]);

// global and static variables
char     cQuot         = ' ',         
         cNL           = '\n',         
         cShort        = 'N',
        *pFileNameS,
         sCommName [50]= "",
         sLogFile  [18]= "",
         sPathSl   [99]= "",
         sPath     [99]= "",
         sDirSl    [99]= "",
         sDir      [99]= "",
         sPDir     [10]= "",
         sCall     [11]= "",
         sType     [11]= "",
         sDel      [11]= "",
         sCopy     [11]= "",
         sLine     [MAX_MOD][BUFLEN],/* MAX_MOD strings, each contains 1 exe command (corr. to 1 module)     */
         sFile     [MAX_FIL][50],    /* MAX_FIL strings, each contains 1 name of a file to be copied */
         sSimName  [MAX_SIM][50],    /* MAX_SIM strings, each contains 1 name of a single simulations */
         sParList  [MAX_SIM][200],   /* MAX_SIM strings, each contains all parameter values for 1 simulatíon */
         sParId    [MAX_PAR][4],     /* MAX_PAR strings, each contains the ID of 1 parameter, e.g. "-n" */
         sParVal   [MAX_PAR][51];    /* MAX_PAR strings, each contains the value of the param., e.g. "20" */
short    nModNo    [MAX_PAR],        /* MAX_PAR integers, each contains the module no, where the parameter can be found */
         bAppend = FALSE;
VtModus  eModus  = VT_SER_1F;
VtSystem eSystem = VT_SYS_NN;

// main program
int main(int argc, char* argv[])
{
	FILE *pFileB=NULL,  /* Pointer to batch file xxxS(.bat) or xxxB.bat  */
	     *pFileV=NULL,  /* Pointer to batch file xxxV.bat (2 files case) */
	     *pFilePar;     /* Pointer to temporary file to write parameter for 'Collect.dat' */
	char  sParValue    [51]="",
	      sExt          [4]="",
	      sFileName    [60]="",
	      sSeriesName  [50]="";
	
	short i=0, nSimulNo=0,
	           nParamNo=0,
	      l=0, nFileNo=0,
	      m=0, nModuleNo=0;

begin:
	OwnInit(argc, argv);

	if (strlen(sCommName) == 0)
	{	printf("\n(Drive +) Directory + Name of command and info file (without extension)\n");
		scanf ("%s", sCommName);
	}

	InitArrays();


	/* Reading data from the parameter file xxx.inf and the commmand file xxx.bat */
	/******************************************************************************/
	nSimulNo  = ReadInfoFile(&nFileNo, sSeriesName);
	if (nSimulNo==-1) goto begin;

	nModuleNo = ReadBatchFile();
	if (nModuleNo==-1) goto begin;

	/* case series with 2 files: substituting values of variable parameter by '%i' */
	/*******************************************************************************/
	if (eModus==VT_SER_2F) 
		nParamNo = ChangeParam(-1);


	/* Generation of the batch file xxxB.bat or xxxS.bat */
	/*****************************************************/
	if (bAppend)
	{	pFileB = fopen(pFileNameS, "a");
	}
	else
	{	if (eSystem == VT_UNIX || eSystem == VT_LINUX)
		{	sprintf(sFileName, "%sS", sCommName);
		}
		else
		{	if (eModus==VT_SER_2F)
				sprintf(sFileName, "%sB.bat", sCommName);
			else
				sprintf(sFileName, "%sS.bat", sCommName);
		}
		pFileB = fopen(sFileName, "w");
		
		if (eSystem == VT_UNIX || eSystem == VT_LINUX)
		{	fprintf(pFileB,"#!\\bin\\sh%cVIMG=%s%cPDIR=%s%c", cNL, sPath, cNL, sDir, cNL);
		}
		else 
		{	fprintf(pFileB, "subst V: /d%csubst V: %s%c", cNL, sPath, cNL);
			fprintf(pFileB, "subst P: /d%csubst P: %s%c", cNL, sDir,  cNL);
		}
	}

	// general settings
	if (eSystem == VT_UNIX || eSystem == VT_LINUX)
	{	strcpy (sPDir, "$PDIR/");
	}
	else 
	{	strcpy (sPDir, "P:\\");
	}

	// writing the commands for each simulation
	for (i=0; i < nSimulNo; i++)
	{	
		/* title and date */
		fprintf(pFileB, "%cecho --------------------------------------------- >> %sHistory.txt%c", 
		                cNL, sPDir, cNL);
		fprintf(pFileB, "echo %c** START ** %s %s ***%c >> %sHistory.txt%c", 
		                cQuot, sSeriesName, sSimName[i], cQuot, sPDir, cNL);
		PrintDate(pFileB, sPDir);

		/* pipe command */
		if (eModus==VT_SER_2F) 
		{	/* 2 files: listing xxxV.bat and varied parameters; calling xxxV.bat with varied parameters */
			fprintf(pFileB, "%c%s %sV.bat >> %sHistory.txt%c", cNL, sType, sCommName, sPDir, cNL);
			fprintf(pFileB, "echo Parameter list: %s >> %sHistory.txt%c", sParList[i], sPDir, cNL);
			fprintf(pFileB, "%s %sV.bat %s%c%c", sCall, sCommName, sParList[i], cNL, cNL);
		}
		else
		{	/* 1 file : listing single commands and writing pipe command (both with varied parameters) */
			ChangeParam (i);
			WriteCommand(pFileB, nModuleNo, TRUE);
			fprintf(pFileB, "%c", cNL);
		}

		/* copying and deleting log files */
		for (m=1; m <= nModuleNo; m++)
			fprintf(pFileB, "%s %s%d >> %sHistory.txt%c", sType, sLogFile, m, sPDir, cNL);
		fprintf(pFileB, "%s %s*%c%c", sDel, sLogFile, cNL, cNL);

		/* copying files */
		for (l=0; l < nFileNo; l++) 
		{	
			fprintf(pFileB, "%s %s%s %s%s_%s%c", sCopy, sPDir, sFile[l], sPDir, sSimName[i], sFile[l], cNL);

			/* files with extension 'col' are copied together with Parameter 1 to Collect.dat */
			strcpy(sExt, sFile[l]+strlen(sFile[l])-3);
			if (strcmp(sExt, "col")==0)
			{	char sParFile[101];
				sscanf(sParList[i], "%s", sParValue);
				sprintf(sParFile, "%s%c%s_par.col", sDir, cSlash, sSimName[i]);
				pFilePar=fopen(sParFile, "w");
				if (pFilePar==NULL)
				{	printf("ERROR: File %s could not be opened", sParFile);
					goto error;
				}
				else
				{	fprintf(pFilePar, "%s ", sParValue);
					fclose (pFilePar);
				}
				fprintf(pFileB, "%s %s >> %sCollect.dat%c", sType, sParFile, sPDir, cNL);
				fprintf(pFileB, "%s %s%s_%s >> %sCollect.dat%c", sType, sPDir, sSimName[i], sFile[l], sPDir, cNL);
			}
		}

		/* date and title */
		PrintDate(pFileB, sPDir);
		fprintf(pFileB, "%cecho %c*** END *** %s %s ***%c >> %sHistory.txt%c", 
		                cNL, cQuot, sSeriesName, sSimName[i], cQuot, sPDir, cNL);
		fprintf(pFileB, "echo --------------------------------------------- >> %sHistory.txt%c%c", 
		                sPDir, cNL, cNL);
	}
	fclose(pFileB);

	/* Generation of the batch file xxxV.bat for the 2 files option */
	/****************************************************************/
	if (eModus==VT_SER_2F) 
	{	sprintf(sFileName, "%sV.bat", sCommName);
		pFileV = fopen(sFileName, "w");
		WriteCommand(pFileB, nModuleNo, FALSE);
		fclose (pFileV);
	}
	printf("\n Batch generated !\n");
	goto end;

error:
	printf("\n Batch could not be generated !\n ");

end:
	printf(" Hit any key to terminate ! \n");
	getchar();
	getchar();
	return 0;
}


/* Print the actual date and time into the batch file */
/******************************************************/
void
PrintDate(FILE* pFileW, const char* sOutDir)
{
	if (eSystem==VT_WIN_NT || eSystem==VT_WIN_98)
	{	fprintf(pFileW, "%cdate/T >> %sHistory.txt%c", cNL, sOutDir, cNL);
		fprintf(pFileW, "time/T >> %sHistory.txt%c", sOutDir, cNL);
	}
	else if (eSystem==VT_UNIX || eSystem==VT_LINUX)
	{	fprintf(pFileW, "%cdate >> %sHistory.txt%c", cNL, sOutDir, cNL);
	}
}


void  OwnInit(int argc, char *argv[]) 
{
	int i;

	for(i=1; i<argc; i++) 
	{	if(argv[i][0]!='+') 
		{	switch(argv[i][1])
			{	case 'F':
					strcpy(sCommName, &argv[i][2]);
					break;
				case 'A':
					pFileNameS = &argv[i][2];
					break;
			}
		}
	}
	if (pFileNameS != NULL)
		bAppend = TRUE;
}

