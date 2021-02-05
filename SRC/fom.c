/*****************************************************************************************/
/*  Tool to evaluate the simulation results to optimize neutron scattering instruments   */
/*  for optimal flux or optimal signal to noise ratio                                    */
/*                                                                                       */
/* 1.0   May 2002  Klaus Lieutenant  1st version                                         */
/* 1.1   Apr 2013  Klaus Lieutenant  changes for new optimization concept                */
/*****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "init.h"
#include "opt_defs.h"

#define SPEC_LEN  999  /* maximal number of points in spectra      */


/*********************************************/
/* global and static variables               */
/*********************************************/
extern FILE*  LogFilePtr=NULL;    // Pointer on file for output of the progress of the fit

static
char  sFctFile[FN_LEN] = "Fcomm.dat",       // name of the file for the actual fitted values
      sIniFile[FN_LEN] = "fom.ini",         // name of the file containing the control parameters
      sLogFile[FN_LEN] = "Opt.log",         // name of the log file 
      sParDir [FN_LEN] = "";                // working directory
double Xs[SPEC_LEN], Ys[SPEC_LEN],          // x and y values of signal    (e.g. wavelength and intensity at sample) 
       Xr[SPEC_LEN], Yr[SPEC_LEN],          // x and y values of reference (e.g. wavelength and intensity at guide_exit) 
       Xn[SPEC_LEN], Yn[SPEC_LEN],	        // x and y values of noise
       Xw[SPEC_LEN], Yw[SPEC_LEN];	        // x and y values of weight



/*********************************************/
/* local functions                           */
/*********************************************/
short  ReadFoMInfo  (short* pAverage, double* pLmbdPow, double* pRefPow, double* pNoisePow, double* pFactor, 
                     char* sSignalFile, char* sRefFile, char* sNoiseFile, char* sWeightFile);
short  ReadSpectrum (const char* sStdFile, const short iSpec, double* pX, double* pY);
double CalcFoM      (short bAverage, double dLmbdPow, double dRefPow, double dNoisePow, short nMax);
double IntSpec      (short bAverage, double* pY, short nPts);
void   InitSpectra  (int nPtsW);
void   OwnInit      (int argc, char *argv[]);
void   OwnCleanup   ();
short  ReadParameter(char* cId, char* pFilename, FILE* pFile);
short  CheckFilename(char* sFilename);


/*********************************************/
/* main function                             */
/*********************************************/
int main(int argc, char* argv[])
{
	char   sParFileS[99]="",             // name of signal, 
	       sParFileR[99]="",             //         reference,
	       sParFileN[99]="",             //         noise,
	       sParFileW[99]="";             //     and weight  file
	int    i=0;                          // index over spectrum elements
	short  iSpec=0,                      // index counting spectra to evaluate
	       bAverage;                     // criterion sum (bAverage=0) or average (bAverage=1) in figure of merit
	int    nPtsS, nPtsR, nPtsN, nPtsW=-1;// number of points in the 4 spectra
	FILE*  pFctFile=NULL;
	double dLmbdPow, dRefPow, dNoisePow, // power of wavelength, reference intensity and noise in the figure of merit
	       FoM,                          // Figure of merit
	       Factor,                       // normalization factor for output value: Factor/FoM 
	       NoiseIntP=1.0;                // integral over noise spectrum to the power of 'dNoisePow'

	Init   (argc, argv, MCN_OPT_FOM);
	OwnInit(argc, argv);

	pFctFile = fileOpen(sFctFile, "w");

	/* read 'fom.ini'  */
	if (ReadFoMInfo(&bAverage, &dLmbdPow, &dRefPow, &dNoisePow, &Factor, sParFileS, sParFileR, sParFileN, sParFileW)==FALSE)
		Error("FoM_lambda: Not all necessary file names given");

	for (iSpec=0; iSpec < MAX_SIM; iSpec++)
	{	
    InitSpectra(nPtsW);
		nPtsS=-1; nPtsR=-1; nPtsN=-1;
		
		/* read given files */
		nPtsS=ReadSpectrum(sParFileS, iSpec, Xs, Ys);

		// process if signal file found
    if (nPtsS > 0)
		{ 
      // reference 
		  if (strlen(sParFileR) > 0 && strcmp(sParFileR, "no_file")!=0) 
		  {	nPtsR=ReadSpectrum(sParFileR, iSpec, Xr, Yr);
			  if (nPtsR!=nPtsS)
				  Warning("FoM_lambda: number of points in signal and reference spectrum do not agree");
		  }

		  // noise 
		  if (strlen(sParFileN) > 0 && strcmp(sParFileN, "no_file")!=0) 
		  {	nPtsN=ReadSpectrum(sParFileN, iSpec, Xn, Yn);
			  NoiseIntP = pow(IntSpec(bAverage, Yn, nPtsN), dNoisePow);
		  }

		  // weight
		  if (nPtsW==-1)
		  {	if (strlen(sParFileW) > 0 && strcmp(sParFileW, "no_file")!=0) 
			  {	nPtsW=ReadSpectrum(sParFileW, -1, Xw, Yw);
				  if (nPtsW!=nPtsS)
					  Warning("FoM_lambda: number of points in signal and weight spectrum do not agree");
			  }
			  else
			  {	for (i=1; i<=nPtsS; i++)
				  {	Xw[i] = Xs[i];
					  Yw[i] = 1.0;
				  }
				  nPtsW=nPtsS;
			  }
		  }

		  /* calculate figure of merit */
			FoM = CalcFoM(bAverage, dLmbdPow, dRefPow, NoiseIntP, nPtsS);
			fprintf(LogFilePtr, "FoM = %10.4e   Factor/FoM = %10.4e\n", FoM, Factor/FoM);
			if (FoM > 0.0)
				fprintf(pFctFile, "%le \n", Factor/FoM);
			else
				fprintf(pFctFile, "%le \n", 1.0e99); 
		}
	} 

	fprintf(LogFilePtr, "\n");

	if (pFctFile!=NULL)
		fclose  (pFctFile);

	OwnCleanup();

	return iSpec;
}

/****************************************/
/* Reading data from the parameter file */
/****************************************/
// output: *pAverage  : criterion sum over spectrum (0) or average value (1)    
//         *pLmbdPow  : power of wavelength in the figure of merit  
//         *pRefPow   : power of flux of reference intensity in the figure of merit  
//         *pLNoisePow: power of the integrated noise in the figure of merit  
//         sSignalFile: Name of the signal file (e.g. intensity at sample or guide exit)
//         sRefFile   : Name of the reference file (e.g. intensity at guide entrance)
//         sNoiseFile : Name of the noise file
//         sWeightFile: Name of the noise file
// return: TRUE/FALSE
short ReadFoMInfo(short* pAverage, double* pLmbdPow, double* pRefPow, double* pNoisePow, 
				  double* pFactor, char* sSignalFile, char* sRefFile, char* sNoiseFile, char* sWeightFile)
{
	short ret=TRUE,  rc, 
	      rcs=FALSE, rcr=FALSE, rcn=FALSE, rcw=FALSE;
	char  sParameter[99],
	      cId;                  // character identifying parameter 
	FILE* pFileI=NULL;

	// Initialisation
	*pAverage = 0;
	*pLmbdPow = *pRefPow = *pNoisePow = 0.0;
	*pFactor  = 1.0;

	// open file
	pFileI = fopen(sIniFile, "r");
	if (pFileI==NULL)
	{	printf("\n'%s' cannot be opened\n", sIniFile);
		return(-1);
	}

	// read file, set parameters and check input
	rc = ReadParameter(&cId, sParameter,  pFileI);
	while (rc)
	{
		switch (cId)
		{	case 'A': *pAverage  = atoi(sParameter); break;
			case 'l': *pLmbdPow  = atof(sParameter); break;
			case 'm': *pRefPow   = atof(sParameter); break;
			case 'n': *pNoisePow = atof(sParameter); break;
			case 'f': *pFactor   = atof(sParameter); break;
			case 'S': strcpy(sSignalFile,sParameter); rcs=CheckFilename(sSignalFile); break;
			case 'R': strcpy(sRefFile,   sParameter); rcr=CheckFilename(sRefFile);    break;
			case 'N': strcpy(sNoiseFile, sParameter); rcn=CheckFilename(sNoiseFile);  break;
			case 'W': strcpy(sWeightFile,sParameter); rcw=CheckFilename(sWeightFile); break;
			default: Warning("unknown parameter in 'fom.ini'");
		}
		rc = ReadParameter(&cId, sParameter,  pFileI);
	}
	if (rcs==FALSE || (*pRefPow != 0 && rcr==FALSE) || (*pNoisePow > 0 && rcn==FALSE)) 
		ret = FALSE;

	// close file
	if (pFileI!=NULL)
		fclose(pFileI);

	return ret;
}

/****************************************/
/* Reading spectrum                     */
/****************************************/
// input : sStdFile: Name of the file to be read without index of the simulation
//         iSpec   : index of the simulation (first is 0)
// output: *pX     : x values or the spectrum
//         *pY     : y values or the spectrum 
// return: number of points in spectrum
short ReadSpectrum(const char* sStdFile, const short iSpec, double* pX, double* pY)
{
	FILE*  pFile;
	char   sBuffer[BUF_LEN], sFilename[256],
	       sParName[99], sParExt[4];      // name and extension of the file 
	short  rc=TRUE;
	int    sl,       // length of the string 'sParDir' defining the working directory
	       i=0;      // counter of points in spectrum

	sl=strlen(sParDir);
	if (iSpec >= 0)
	{	
		strcpy  (sParExt,  sStdFile+strlen(sStdFile)-3);
		StrgCopy(sParName, sStdFile, strlen(sStdFile)-4);

		if (sl > 0)
		{	if (sParDir[sl-1]=='\\' || sParDir[sl-1]=='/')
				sprintf(sFilename,  "%s%s%hd.%s",  sParDir, sParName, iSpec, sParExt);
			else	
				sprintf(sFilename, "%s\\%s%hd.%s", sParDir, sParName, iSpec, sParExt);
		}
		else
		{	sprintf(sFilename, "%s%hd.%s", sParName, iSpec, sParExt);
		}
	}
	else
	{	if (sl > 0)
		{	if (sParDir[sl-1]=='\\' || sParDir[sl-1]=='/')
				sprintf(sFilename,  "%s%s",  sParDir, sStdFile);
			else	
				sprintf(sFilename, "%s\\%s", sParDir, sStdFile);
		}
		else
		{	strcpy(sFilename, sStdFile);
		}
	}

	// open file, read data and close file
	pFile = fopen(sFilename, "r");
	if (pFile!=NULL)
	{	rc=ReadLine(pFile,sBuffer, BUF_LEN-1);
		while (rc)
		{	if(strlen(sBuffer) > 1) 
			{	i++;
				sscanf(sBuffer, "%lf %le", &pX[i], &pY[i]);
			}
			rc=ReadLine(pFile,sBuffer, BUF_LEN-1);
		}

		fclose(pFile);
	}
	return i;
}


/****************************************/
/* Calculation of figure of merit       */
/****************************************/
// input : dLmbdPow  : power of wavelength in the figure of merit  
//         dRefPow   : power of flux of reference intensity in the figure of merit  
//         dLNoisePow: power of the integrated noise in the figure of merit 
//         nPts      : number of points in spectrum
// return: figure of merit
/****************************************/
double CalcFoM(short bAverage, double dLmbdPow, double dRefPow, double NoiseInt, short nPts)
{
	short  i,        // index for points		
	       n=0;      // number of points used to calculate figure of merit
	double FoM=0.0;  // figure of merit

	if (dRefPow==0.0)
	{
		for (i=1; i<=nPts; i++)
			FoM += Ys[i] * Yw[i] * pow(Xs[i], dLmbdPow);
		n=nPts;
	}
	else
	{	for (i=1; i<=nPts; i++)
		{	
			if (Xr[i]==Xs[i] && Yr[i] > 0.0)
			{	FoM += Ys[i] * Yw[i] / pow(Yr[i], dRefPow) * pow(Xs[i], dLmbdPow);
				n++;
			}
		}
		if (n==0)
			return -1.0;
	}
	if (bAverage)
		FoM /= n;

	if (NoiseInt != 0.0)
	{	FoM /= NoiseInt;
	}
	else
	{	Warning("FoM_lambda: Division by integral over noise spectrum skipped because of zero value");
	}

	return FoM;
}

/*****************************************************/
/* Integration of spectrum                           */
/*****************************************************/
// input : *pY  : pointer to spectrum to integrate 
//         nPts : number of points in spectrum
// return: integral
/*****************************************************/
double IntSpec(short bAverage, double* pY, short nPts)
{
	short  i;       // index for points		
	double Int=0.0; // integral

	for (i=1; i<=nPts; i++)
		Int += pY[i];

	if (bAverage)
		Int /= nPts;

	if (Int == 0)
		Warning("FoM_lambda: Integration over noise spectrum yielded zero");

	return Int;
}


void InitSpectra(int nPtsW)
{
	long   j;

	for (j=0; j < SPEC_LEN; j++)
	{	Xs[j]=0.0; Ys[j]=0.0;
		Xr[j]=0.0; Yr[j]=0.0;
		Xn[j]=0.0; Yn[j]=0.0;
		if (nPtsW==-1)
		{	Xw[j]=0.0; Yw[j]=0.0;}
	}
}

void OwnInit(int argc, char *argv[])
{
	long  i;
	char* arg;

	for(i=1; i<argc; i++) 
	{
		arg = argv[i];
		if (*arg !='+') 
		{
			arg += 2;
			switch(arg[-1]) 
			{	
				case 'F':
					strcpy(sFctFile, arg);
					break;
				case 'f':
					strcpy(sIniFile, arg);
					break;
				case 'L':
					strcpy(sLogFile, arg);
					break;
				case 'P':
					strcpy(sParDir, arg);
					break;
				default:
					fprintf(LogFilePtr,"FoM_lambda: unknown commandline option: %s\n", argv[i]);
					exit(-1);
			}
		}
	}
    LogFilePtr = fileOpen(sLogFile, "at");
}


void OwnCleanup()
{
	if (LogFilePtr!=NULL) 
		fclose(LogFilePtr);
}


/****************************************/
/* Reading data from the parameter file */
/****************************************/
// input : pFile      : pointer to parameter file 'fom.ini'  
// output: cId        : character identifying parameter 
//         sParameter : Name of the noise file
// return: TRUE/FALSE
/****************************************/
short ReadParameter(char* pId, char* sParameter, FILE* pFile)
{
	int   k, ks=0, ke;   // indices for string 
	short ret=TRUE;      // return code
	char  sBuffer[BUF_LEN];
	char* pPtr; 

	if (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
	{	
		*pId  = sBuffer[0];      // first character identifies parameter

		/* copy after '=' sign */
		pPtr = strchr(sBuffer, '=');
		strcpy(sParameter, pPtr+1);

		/* strip leading blanks */
		ke=strlen(sParameter);
		while (sParameter[ks]==' ') ks++;
		if (ks >0)
			for (k=0; k <= ke-ks; k++)
				sParameter[k] = sParameter[k+ks];
	}
	else
	{	ret=FALSE;
	}

	return ret;
}


/****************************************/
/* Check if file is chosen              */
/****************************************/
// input : sFilename : name read from the input file
// return: TRUE/FALSE
/****************************************/
short CheckFilename(char* sFilename)
{
	short cmp;
	
	cmp = strcmp(sFilename,"no_file");

	if (cmp==0 || strlen(sFilename)==0)
		return FALSE;
	else
		return TRUE;
}
