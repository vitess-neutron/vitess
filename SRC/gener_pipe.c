/********************************************************************************************/
/*  Tool to generate the command pipe to run the instrument simulation using VITESS         */
/*                                                                                          */
/* 1.0   Apr 2003  Klaus Lieutenant  1st version                                            */
/* 1.1   Apr 2013  Klaus Lieutenant  changes for new optimization concept                   */
/********************************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "init.h"
#include "opt_defs.h"
#include "pipe_fct.h"


/*********************************************/
/* local functions                           */
/*********************************************/
void   DetSimParamStd(double P[MAX_SIM][NMAX], short iSim, short nSimParNo, short nFitParNo);
void   DetSimParamMtr(double P[MAX_SIM][NMAX], short iSim, short nSimParNo, short nFitParNo, short iFirstMPar);
void   OwnInit       (int argc, char *argv[]);
void   OwnCleanup    ();


/*********************************************/
/* global and static variables               */
/*********************************************/
extern FILE*  LogFilePtr=NULL;  // Pointer on file for output of the progress of the fit

static
char     sBuffer[BUF_LEN+1]="";

FILE    *pExeVFile=NULL;
char     cSlash       = '/',
         cQuot        = ' ',         
         cNL          = '\n',         
         cShort       = 'N',
         sInstr   [FN_LEN+1]= "std_instr.cmd",      // file of the initial instrument
         sFitInfo [FN_LEN+1]= "",            // file containing information about operating system, ... files to copied
         sSimPar  [FN_LEN+1]= "sim_param.ini",      // file containing a list of parameters to be varied 
         sPcoFile [FN_LEN+1]= "Pcomm.dat",          // file of the actual parameter set
         sExeVFile[FN_LEN+1]= "Simulations.bat",    // output file containing all pipe commands
         sLogFile [FN_LEN+1]= "Opt.log",
         sPathSl  [FN_LEN+1]= "",
         sExeDirC [FN_LEN+1]= "",
         sExeDirN [FN_LEN+1]= "",
         sParDirC [FN_LEN+1]= "",
         sParDirN [FN_LEN+1]= "",
         sPDir    [10]= "",
         sCall    [11]= "",
         sType    [11]= "",
         sDel     [11]= "",
         sCopy    [11]= "",
         sLine    [MAX_MOD][BUF_LEN],/* MAX_MOD strings, each contains 1 exe command (corr. to 1 module)     */
         sFile    [MAX_FIL][50],     /* MAX_FIL strings, each contains 1 name of a file to be copied */
         sSimName [MAX_SIM][50],     /* MAX_SIM strings, each contains 1 name of a single simulations */
         sParList [MAX_SIM][200],    /* MAX_SIM strings, each contains all parameter values for 1 simulatíon */
         sParId   [MAX_PAR][4],      /* MAX_PAR strings, each contains the ID of 1 parameter, e.g. "-n" */
         sParVal  [MAX_PAR][51];     /* MAX_PAR strings, each contains the value of the param., e.g. "20" */
short    nModNo   [MAX_PAR];         /* MAX_PAR integers, each contains the module no, where the parameter can be found */



/*********************************************/
/* program                                   */
/*********************************************/

int main(int argc, char* argv[])
{
	short               nModuleNo=0,  // number of executables in the pipe (from file xxx.cmd)
	       l=0,         nFileNo  =0,  // number of files to be copied 
	                    nFitParNo=0,  // number of fit parameters
	                    nSimParNo=0,  // number of simulation parameters
	       m=0, mMin=0, 
              mMax=0, nSimulNo =0,  // number of simulations to perform
         bPrtCmd;                   // criterion: write standard instrument 
	double P[MAX_SIM][NMAX];
	char   sParFct    [KW_LEN+1]="",
	       sFilename  [FN_LEN+1]="",
         sNumber    [ 6]="";

	Init   (argc, argv, VT_TOOL);
	OwnInit(argc, argv);
	InitArrays();

  // read input files
	nSimParNo = ReadSimPar(sParFct, &nFileNo);
	if (nSimParNo < 1)
		Error("gener_pipe: reading of parameter file ('sim_param.ini') did not yield any variable simulation parameter");

	nSimulNo = ReadPData(&nFitParNo, &mMin, &mMax, &bPrtCmd, P, sPcoFile);
	if (nSimulNo==0)
	{	fprintf(LogFilePtr,"gener_pipe: parameters cannot be read from file %s\n", sPcoFile);
		return(-1);
	}

	nModuleNo = ReadCmdFile(bPrtCmd);
	if (nModuleNo < 1)
		Error("gener_pipe: reading of command file ('std_instr.cmd') did not yield any line of executable command (in the pipe)");

  if (strlen(sExeDirN)==0) strcpy(sExeDirN, sExeDirC);
  if (strlen(sParDirN)==0) strcpy(sParDirN, sParDirC);
	
	// general settings
#ifdef VT_WINDOWS
  strcpy (sPDir, "P:\\");
  fprintf(pExeVFile, "subst V: /d %c", cNL);
  fprintf(pExeVFile, "subst V: %s %c", sExeDirN, cNL);
  fprintf(pExeVFile, "subst P: /d %c", cNL);
  fprintf(pExeVFile, "subst P: %s %c", sParDirN, cNL);
#else
		strcpy (sPDir, "$P/");
    fprintf(pExeVFile, "V=%s %c", sExeDirN, cNL);
    fprintf(pExeVFile, "P=%s %c", sParDirN, cNL);
#endif

		
	// delete old files
	for (l=0; l < nFileNo; l++) 
	{	ExtendFilename(sFilename, sFile[l], "*");
		fprintf(pExeVFile, "%s %s%c", sDel, sFilename, cNL);
	}

  // write command and additional commands for each parameter set
	for (m=mMin; m <= mMax; m++)
	{	
		// create pipe commands and write to batch file
		if (strcmp(sParFct, "STD")==0 || strcmp(sParFct, "Std")==0 || strcmp(sParFct, "std")==0)
		{	DetSimParamStd(P, m, nSimParNo, nFitParNo);
		}
		else if (strcmp(sParFct, "MTR02")==0)
		{	DetSimParamMtr(P, m, nSimParNo, nFitParNo,  2);
		}
		else
		{	fprintf(LogFilePtr, "ERROR: gener_pipe: string %s to define function for parameter calculation unknown\n", sParFct);
		}
		ChangeParam(-1);
		WriteCommand(pExeVFile, nModuleNo, FALSE);

		// add commands to copy all files that will be needed for figure of merit
		for (l=0; l < nFileNo; l++) 
		{	itoa(m, sNumber, 10);
			ExtendFilename(sFilename, sFile[l], sNumber);
			fprintf(pExeVFile, "%s %s%s %s%c", sCopy, sPDir, sFile[l], sFilename, cNL);
		}

    // collect and remove pipelogs
#ifdef VT_WINDOWS
    fprintf(pExeVFile, "%s ", sCopy);
		for (l=0; l < nModuleNo; l++) 
		{	
			if (l > 0) fprintf(pExeVFile, " + ");
			fprintf(pExeVFile, "%svpipelog%d", sPDir, l+1);
		}
		fprintf(pExeVFile, " %sSim%d.log%c", sPDir, m, cNL);
#else
    fprintf(pExeVFile, "%s %svpipelog* > %sSim%d.log%c", sType, sPDir, sPDir, m, cNL);
#endif
    fprintf(pExeVFile, "%s %svpipelog*%c", sDel, sPDir, cNL);
	}

	OwnCleanup();

	return nSimulNo;
}


/****************************************************************/
/* DetermineSimParam                                            */
/*  P        : sets of optimization parameters                  */
/*  iSim     : index of simulations to perform                  */
/*  nSimParNo: number of simulation parameters                  */
/*  nFitParNo: number of optimization parameters                */
/*                                                              */
void DetSimParamStd(double P[MAX_SIM][NMAX], short iSim, short nSimParNo, short nFitParNo)
{
	short  j=0; 
	double dParVal[MAX_PAR];

	for (j=0; j < nSimParNo; j++)
		dParVal[j] = P[iSim][j];

	// set unused simulation parameters to zero
	for (j=nSimParNo; j < MAX_PAR; j++)
		dParVal[j] = 0.0;
	
	// transform parameters to strings
	for (j=0; j < nSimParNo; j++)
		sprintf(sParVal[j], "%le", dParVal[j]);
}

void DetSimParamMtr(double P[MAX_SIM][NMAX], short iSim, short nSimParNo, short nFitParNo, short iFirstMPar)
{
	short  j=0; 
	double dParVal[MAX_PAR];

	for (j=0; j < iFirstMPar; j++)
		dParVal[j] = P[iSim][j];
	for (j=iFirstMPar; j < nSimParNo; j++)
		dParVal[j] = P[iSim][j]*100.0;

	// set unused simulation parameters to zero
	for (j=nSimParNo; j < MAX_PAR; j++)
		dParVal[j] = 0.0;
	
	// transform parameters to strings
	for (j=0; j < nSimParNo; j++)
		sprintf(sParVal[j], "%le", dParVal[j]);
}

void OwnInit(int argc, char *argv[])
{
	long   i;
	char * arg;

	for(i=1; i<argc; i++) 
	{
		arg = argv[i];
		if (*arg !='+') 
		{
			arg += 2;
			switch(arg[-1]) 
			{	
				case 'f':
					strcpy(sFitInfo, arg);
					break;
				case 's':
					strcpy(sSimPar, arg);
					break;
				case 'P':
					strcpy(sPcoFile, arg);
					break;
				case 'I':
					strcpy(sInstr, arg);
					break;
				case 'L':
					strcpy(sLogFile, arg);
					break;
				case 'S':
					strcpy(sExeVFile, arg);
					break;

				default:
					fprintf(LogFilePtr, "gener_pipe: unknown commandline option: %s\n", argv[i]);
					exit(-1);
			}
		}
	}
  LogFilePtr = fileOpen(sLogFile, "at");
	pExeVFile  = fileOpen(sExeVFile, "w");
}


void OwnCleanup()
{
	if (pExeVFile!=NULL)
		fclose(pExeVFile);
	if (LogFilePtr!=NULL) 
		fclose(LogFilePtr);
}

