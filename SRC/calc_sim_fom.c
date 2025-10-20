/********************************************************************************************/
/*  Tool to calculate the values of a fit function                                          */
/*                                                                                          */
/* 0.9   Apr 2011  Klaus Lieutenant  1st version                                            */
/********************************************************************************************/

#include "calc_sim_fom.h"
#include "init.h"

extern
double arP[MAX_SIM][NMAX+1], // function F to parameter set P
       arF[MAX_SIM][IMAX+1]; // for sets 0 ... nSim-1

extern char sFomPrg[FN_LEN];

static char sPipePrg[FN_LEN]="gener_pipe",
            sGridPrg[FN_LEN]="./gridrun -o";


/*********************************************************************/
/*  Prototypes                                                       */
/*********************************************************************/
static short WriteAllP(const char* sFilename, const short mMin, const short mMax, const short nPar);
static short ReadAllF (const char* sFilename, const short mMin, const short mMax);
static short AddPathAndExt(char* sCmdName);


/*********************************************************************/
/*  G L O B A L    F U N C T I O N S                                 */
/*********************************************************************/
/*********************************************************************/
/* Functions to write parameters sets                                */
/* input : F        : function corresponding to parameter set P      */
/*         X        : parameter X_1 ... X_anz (e.g. wavelength)      */
/*         P        : parameter set, for which the fct is calculated */
/*         mPts     : number of points in spectrum                   */
/*         mMin,mMax: indices of first & last function to determine  */
/*         mPar     : number of parameters per set (P1 ...P_npar)    */
/* return: TRUE/FALSE                                                */
/*********************************************************************/
short  FitFctPc(double F[IMAX+1], const double X[IMAX+1], const double P[NMAX+1], const int nPts, const short nPar)
{
  return FALSE;
}

short  OptFctPc(const double X[IMAX+1], const int nPts, const short mMin, const short mMax, const short nPar)
{
  static short bPath=FALSE;
  short rc=FALSE, rcp, rcf;

  if (!bPath)
  { AddPathAndExt(sPipePrg);
    AddPathAndExt(sFomPrg);
    bPath = TRUE;
  }

  WriteAllP   ("Pcomm.dat", mMin, mMax, nPar);
  rcp = system(sPipePrg);

  if (rcp > 0)
  {
#ifdef VT_WINDOWS
    system("Simulations.bat");
#else
    system("chmod u+x Simulations.sh");
    system("./Simulations.sh");
#endif
    rcf = system(sFomPrg);

    if (rcf)
      rc=ReadAllF("Fcomm.dat", mMin, mMax);
  }
  return rc;
}


short  OptFctGrid(const double X[IMAX+1], const int nPts, const short mMin, const short mMax, const short nPar, char* sGridOpt)
{
  static short bPath=FALSE;
  short rc=FALSE, rcf;

#ifdef VT_WINDOWS
  Error("Optimization on cluster only supported for Unix systems");
  return(FALSE);
#else
  char sGridCmd[120];

  if (!bPath)
  { AddPathAndExt(sFomPrg);
    bPath = TRUE;
  }

  WriteAllP("Pcomm.dat", mMin, mMax, nPar);

  sprintf(sGridCmd, "%s %s", sGridPrg, sGridOpt);
  system("chmod u+x gridrun");
  system(sGridCmd);

  rcf = system(sFomPrg);

  if (rcf)
    rc=ReadAllF("Fcomm.dat", mMin, mMax);

  return rc;
#endif
}


/********************************************************************/
/*  L O C A L    F U N C T I O N S                                  */
/********************************************************************/
/********************************************************************/
/* Function for writing parameters sets                             */
/* input : sFilename: Name of file to write to                      */
/*         mMin,mMax: indices of first & last function to determine */
/*         mPar     : number of parameters per set (P1 ...P_npar)   */
/* return: TRUE/FALSE                                               */
/********************************************************************/
static short WriteAllP(const char* sFilename, const short mMin, const short mMax, const short nPar)
{
  int   j,m;
  short rc=FALSE;
  FILE* pXFile;
  static
  short bPrint=TRUE;  // info for gener_pipe about fist call

  pXFile = fopen(sFilename, "w");
  if (pXFile)
  {
    fprintf(pXFile, "%d %2d\n", bPrint, mMin);
    for (m=mMin; m<=mMax; m++)
    {
      for (j=1; j<=nPar; j++)
      {  fprintf(pXFile, "%15.7e ", arP[m][j]);
      }
      fprintf(pXFile, "\n");
    }

    rc=TRUE;
    bPrint=FALSE;
    fclose (pXFile);
  }
  else
  { printf("Error in calc_sim_fom::WriteAllP: file '%s' could not be opened to write parameter values !\n", sFilename);
    exit(-1);
  }
  return rc;
}


/********************************************************************/
/* Function to read the calculated functions for all parameter sets */
/* Function for writing parameters sets                             */
/* input : sFilename: Name of file to read from                     */
/*         mMin,mMax: indices of first & last function to determine */
/* return: TRUE/FALSE                                               */
/********************************************************************/
static short ReadAllF(const char* sFilename, const short mMin, const short mMax)
{
  FILE* pFFile;
  short m, n,
        rc=FALSE, // return code
        nSim=0;   // no of lines = number of simulations performed in this step
  char  sLine[CHAR_BUF_LARGE+1];

  for (m=mMin; m <= mMax; m++)
    for (n=0; n<=IMAX; n++)
      arF[m][n]=0.0;

  pFFile = fopen(sFilename, "r");
  if (pFFile != NULL)
  {
    nSim = (short) LinesInFile(pFFile);

    if (nSim == mMax-mMin+1)
    { rc=TRUE;
      for (m=mMin; m <= mMax; m++)
      {  ReadLine  (pFFile, sLine, CHAR_BUF_LARGE);
        StrgScanLF(sLine, &arF[m][1], IMAX, 0);
      }
    }
    else
    { printf("Error in calc_sim_fom::ReadAllF: number of files created for figure of merit not correct !\n");
    }
    fclose (pFFile);
  }
  else
  { printf("Error in calc_sim_fom::ReadAllF: file '%s' could not be opened to read function values !\n", sFilename);
    exit(-1);
  }
  return rc;
}


short AddPathAndExt(char* sCmdName)
{
  FILE* pFile;
  char *pBlank, *pExt, *pSlash,
        sPath  [FN_LEN],
        sCmd   [31],
        sBuffer[CHAR_BUF_LENGTH];
  int   kBlank;
  short rc=FALSE;

  // open file
  pFile = fopen("std_instr.cmd", "r");
  if (pFile==NULL)
    Error("File 'std_instr.cmd' does not exist");

  // skip 2 header lines and read first command line
  ReadLine(pFile, sBuffer, sizeof(sBuffer)-1);
  ReadLine(pFile, sBuffer, sizeof(sBuffer)-1);
  ReadLine(pFile, sBuffer, sizeof(sBuffer)-1);
  fclose(pFile);

  strcpy(sCmd, sCmdName);

  // search for path and extension of executable
  pBlank = strchr (sBuffer, ' ');
  kBlank = pBlank-sBuffer;
  StrgCopy(sPath, sBuffer, kBlank);
  pSlash = strrchr(sPath, '/');

  pExt = strstr(sPath, "Linux");
  if (pExt==NULL)
    pExt = strstr(sPath, "Darwin");
  if (pExt==NULL)
    pExt = strstr(sPath, "exe");

  memset(pSlash+1, '\0', 1);

  // add path and extension to the command name, if the extension is found
  if (pExt!=NULL)
  { rc=TRUE;
    pExt--;
    sprintf(sCmdName, "%s%s%s", sPath, sCmd, pExt);
  }

  // set correct slash
  ChangeSlash(sCmdName);

  return(rc);
}
