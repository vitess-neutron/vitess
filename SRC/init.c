/********************************************************************************************/
/*  VITESS module 'init.c'                                                                  */
/*    Some general functions for all VITESS modules                                         */
/*                                                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,                                     */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin                                        */
/*                                                                                          */
/* Jan 2002  K. Lieutenant  reorganized routines                                            */
/* Mar 2003  K. Lieutenant  new function 'ColumnsInFile'                                    */
/* Jan 2004  K. Lieutenant  new function 'FullName' and changes for 'instrument.dat         */
/* Feb 2004  M. Fromme      functions FullParName and FullInstallName                       */
/* Feb 2004  K. Lieutenant  use of Full...Name, 'instrument.inf' and 'simulation.inf'       */
/* Mar 2008  M. Fromme      compressed neutron data                                         */
/********************************************************************************************/

#ifdef _MSC_VER
# include <fcntl.h>
# include <io.h>
# define cSlash '\\'
#else
# define cSlash '/'
#endif
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "init.h"

#define MAX_COL 4   /* max. number of count rates written separately for different colours
                       0 means no separate rates writable */
#define NUM_EOP 3   /* number of end-of-part lines that can be treated in 'instrument.inf' */

extern FILE* LogFilePtr;   /* pointer to the log file stream              */

/**************************************************************/
/* This file contains several global variables which are      */
/* essential to each VITESS program module                    */
/**************************************************************/

long     BufferSize;      /* size of the neutron input and output buffer */
Neutron* InputNeutrons;   /* input neutron Buffer */
Neutron* OutputNeutrons;  /* output neutron buffer */
long     OutNeutPtr;      /* points to the next free position in OutputNeutrons */
ModProp  stPicture;       /* data needed to draw a picture of the component represented by the module */

long     NumNeutGot;      /* number of trajectories read in the current batch */
double   NumNeutRead;     /* number of trajectories read in total */
double   NumNeutWritten;  /* number of trajectories written in total */

FILE*    InputFilePtr;    /* stream from which the neutrons are read */
FILE*    OutputFilePtr;   /* stream to which the neutrons are written */
char*    InputFileName;   /* file to read neutrons */
char*    OutputFileName;  /* file to write neutrons */
char*    LogFileName;     /* log filename  */
char*    ParDirectory;    /* parameter directory */
char*    InstallDirectory;


double   wei_min=0.0;     /* Minimal weight for tracing neutron */
long     keygrav=1;
short    bTrace=TRUE,     /* criterion: write trace files */
         bOldFrame=FALSE, /* criterion: co-ordinate system of prev. module used for current module */
         bSepRate=TRUE;   /* criterion: write separate count rates */


/**************************************************************/
/* static variables                                           */
/**************************************************************/

static long       TracePoints=FALSE;     /* creates dot for every written output buffer if TRUE */
static double     dProbTotal[MAX_COL+1], /* sum of the count rates of all trajectories [n/s]    */
                  dProbQuad;             /* sum of the squares of the count rates of all traj.  */

static const char VITESS_VERSION[] = "2.8";
static char       sModuleName[21];

static int ParDirectoryLength, InstallDirectoryLength;

#define COMPRESSBUFLEN 65536
static int compressModeR, compressModeW, compressedRestlen, spinVector;
static VectorType spinUp, spinDown, spinUpOut, spinDownOut;
static char *compressBuf;



/**************************************************************/
/* local functions                                            */
/**************************************************************/

static void   OutputBufferFlush();
static void   WriteTraceLine(Neutron* Neut);
static int    readCompressedNeutrons();
static void   writeCompressed();


/**************************************************************/
/* global functions that are also used in this module         */
/**************************************************************/

void  CopyNeutron    (Neutron* source,   Neutron* dest);
void  WriteInstrData (long   nModuleNo,  VectorType Pos, double  dLength, double  dRotZ, double  dRotY);
void  ReadInstrData  (long*  pModuleNo,  VectorType Pos, double* pLength, double* pRotZ, double* pRotY);
void  ReadSimData    (double* pTimeMeas, double* pLmbdWant, double* pFreq);
long  LinesInFile    (FILE* In);
char* FullParName    (char* filename);
char* FullInstallName(char* fileName, char* sRelPath);


static void setInstallDirectory (char *arg) {
  /* The executable path contains the installation path, which we assume
     to be that string part before MODULES */
  char *mp;
  mp = strstr(arg, "MODULES");
  if (! mp) mp = strstr(arg, "Debug");
  if (! mp) return;
  InstallDirectoryLength = (int)((long) mp - (long) arg);
  /* for pointers too big for long integers: */
  if (InstallDirectoryLength < 0) InstallDirectoryLength = -InstallDirectoryLength;
  InstallDirectory = (char *) malloc(InstallDirectoryLength + 1);
  memcpy(InstallDirectory, arg, InstallDirectoryLength);
  InstallDirectory[InstallDirectoryLength] = 0;
}

static void setParDirectory (char *a) {
  int len;
  if ((len = strlen(a))) {
    /* last character should be a slash */
    if (a[len-1] == cSlash) {
      memcpy ((ParDirectory = (char *) malloc(len+1)), a, len);
    } else {
      memcpy ((ParDirectory = (char *) malloc(len+2)), a, len);
      ParDirectory[len++] = cSlash;
      ParDirectory[len] = 0;
    }
    ParDirectoryLength = len;
  }
}

static char *conCat (char *b, char* c, int sel) {
  char *res, *a=NULL;
  int alen, blen, clen;
  if (b == 0)
    return 0;
  /* Do not change an absolute path. */
#ifdef _MSC_VER
  /* we consider a filename with : as absolute */
  if (strstr(b, ":")) sel = -1;
#else
  if (b[0] == '/') sel = -1;
#endif
  if (sel == -1) {
    alen = 0;
  } else if (sel == 0) {
    a = ParDirectory;
    alen = ParDirectoryLength;
  } else {
    a = InstallDirectory;
    alen = InstallDirectoryLength;
  }
  blen = strlen(b);
  clen = strlen(c);
  if ((res = (char *) malloc(alen+blen+clen+1)))
/*{ if (alen)
      memcpy(res, a, alen);
    strcpy(res+alen, c);
    strcpy(res+alen+clen, b);  */
  { if (alen)
      strcpy(res, a);
    else
      strcpy(res,"");
    strcat(res, c);
    strcat(res, b);
  }
  return res;
}

/* Adding path of parameter directory to file name */
char* FullParName(char* fileName)
{
  return conCat(fileName, "", 0);
}

/* Adding path of installation directory to file name */
char* FullInstallName(char* fileName, char* sRelPath)
{
  return conCat(fileName, sRelPath, 1);
}


/**************************************************************/
/* Init does a general program initialization, which is ok    */
/* for all modules of the VITESS program package.             */
/* a processed option is marked by setting the leading - to + */
/* Option processed here:                                     */
/*  --f  input file name                                      */
/*  --F  output file name                                     */
/*  --Z  random number generator initialization               */
/*  --B  number of buffer entries                             */
/*  --L  logfile                                              */
/*  --J  active trace points                                  */
/*  --G  gravitation                                          */
/*  --U  minimal neutron weight                               */
/*  --P  parameter directory                                  */
/**************************************************************/

void Init(int argc, char **argv, VtModID eModule)
{
  char *a, *arg;
  short l;
  static char * marg[3];
  const gsl_rng_type * T;

  /* Set some default values */
  InputFilePtr   = stdin;
  OutputFilePtr  = stdout;
  LogFilePtr     = stderr;
  InputFileName  = NULL;
  OutputFileName = NULL;
  LogFileName    = NULL;
  ParDirectory   = "";
  BufferSize     = BUFFER_SIZE;
  OutNeutPtr     = 0;
  TracePoints    = FALSE;
  for (l=0; l<=MAX_COL; l++)
    dProbTotal[l] = 0.0;
  stPicture.eModule= eModule;
  stPicture.dWPar  = 0.0;
  stPicture.dHPar  = 0.0;
  stPicture.dRPar  = 0.0;
  stPicture.nNumber= 1L;
  stPicture.eType  = 0;
  stPicture.pDescr = "";

  setInstallDirectory(*argv++);	/* extract installation path from program name */

  while ((a = *argv++)) {
    if ('-' != *a || '-' != a[1]) continue; /* first two chars must be - */
    arg = a + 3;
    switch (a[2]) {
    case 'f' :			/* input file if other than stdin */
      marg[0] = arg;
      break;

    case 'F':                   /* output file if other than stdout */
      marg[1] = arg;
      break;

    case 'L':                   /* output file if other than stderr */
      marg[2] = arg;
      break;

    case 'P':                   /* parameter (= default) directory */
      setParDirectory(arg);
      break;

    case 'Z':                   /* init number for the random number generator */
#if defined(PENV) || defined(_MSC_VER)
      {
	int i;
        if (sscanf(arg, "%i", &i)) {
	  char buf[24];	  
	  sprintf(buf, "GSL_RNG_SEED=%d", i);
#      ifdef _MSC_VER
  	  _putenv(buf);
#      else
  	  putenv(buf);
#      endif
	}
      }
#else
      setenv("GSL_RNG_SEED", arg, 1);
#endif
      break;

    case 'B':                   /* determine the buffer size */
      sscanf(arg,"%ld", &BufferSize);
      break;

    case 'G':
      keygrav = atol(arg);      /* key for gravity 1 -yes (default), 0 - no  */
      break;

    case 'U':
      wei_min = atof(arg);      /* minimal weight for tracing neutron */
      break;

    case 'J' :
      TracePoints=TRUE;
      break;

    default :
      continue;
    }
    *a = '+';                   /* remember that this argument has been processed */
  }

  /* Now we know how to handle filenames, which might correspond to the parameter directory */
  if ((arg = marg[0])) {
    if (strcmp(arg, "no_file")) {
      InputFilePtr = fileOpen((InputFileName = FullParName(arg)), "rb");
      if (InputFilePtr) {
	// Is it compressed ?
	char b[4];
	compressModeR = 0;
	if (4 == fread(b, 1, 4, InputFilePtr)) {
	  if (memcmp(b, "cmp2", 4) == 0)
	    compressModeR = 2;
	  else if (memcmp(b, "cmp1", 4) == 0)
	    compressModeR = 1;
	}
	if (compressModeR)
	  compressedRestlen = spinVector = 0;
	else
	  fseek(InputFilePtr, 0, SEEK_SET);
      }
    } else
      InputFilePtr = NULL;
  }

  if ((arg = marg[1])) {
    if (strcmp(arg,"no_file")) {
      OutputFilePtr = fileOpen((OutputFileName = FullParName(arg)), "wb");
      if (OutputFilePtr) {
	if (strstr(OutputFileName, ".float.")) {
	  compressModeW = 2;
	  fwrite("cmp2", 1, 4, OutputFilePtr);
	} else if (strstr(OutputFileName, ".nodebug.")) {
	  compressModeW = 1;
	  fwrite("cmp1", 1, 4, OutputFilePtr);
	} else 
	  compressModeW = 0;
      }
    } else
      OutputFilePtr = NULL;
  }

  if ((arg = marg[2]))
    LogFilePtr = fileOpen((LogFileName = FullParName(arg)), "wt");

#ifdef _MSC_VER
  if(InputFilePtr==stdin) {
    if( _setmode(_fileno( stdin ), _O_BINARY ) == -1) {
      fprintf(LogFilePtr,"Can't set stdin to binary mode\n");
      exit(-1);
    }
  }
  if(OutputFilePtr==stdout) {
    if(_setmode(_fileno( stdout ), _O_BINARY ) == -1) {
      fprintf(LogFilePtr,"Can't set stdout to binary mode\n");
      exit(-1);
    }
  }
#endif

  /* allocte memory for the neutron buffers */
  if((InputNeutrons=(Neutron *)calloc(BufferSize, sizeof(Neutron)))==NULL) {
    fprintf(LogFilePtr, "Couldn't allocate memory for input buffer\n");
    exit(-1);
  }
  if((OutputNeutrons=(Neutron *)calloc(BufferSize, sizeof(Neutron)))==NULL) {
    fprintf(LogFilePtr, "Couldn't allocate memory for output buffer\n");
    exit(-1);
  }

  /* Initialize the GNU random number generator */
  gsl_rng_env_setup();
  T = gsl_rng_default;
  vit_gsl_rng = gsl_rng_alloc (T);

}


/*****************************************************************/
/* Cleanup() does last things before the VITESS module is closed */
/* e.g. buffers are flushed and files are closed etc.            */
/* you should also write your OwnCleanup() for your module       */
/*****************************************************************/
void Cleanup(double dShiftX, double dShiftY, double dShiftZ,
             double dHorizAngle, double dVertAngle)
{
  double dTimeMeas, dLmbdWant, dFreq, nNoNeutrons,
         dRotMatrix[3][3], dRotY, dRotZ, dLength,
	      dCntRateErr;
  long   nModuleNo;
  int    k,l;
  VectorType Shift,  /* Shift of end position        [m] */
             EndPos; /* end position of prev. module [m] */

  /* update 'instrument.inf' */
  ReadInstrData(&nModuleNo, EndPos, &dLength, &dRotZ, &dRotY);
  ReadSimData  (&dTimeMeas, &dLmbdWant, &dFreq);
  nModuleNo++;
  Shift[0]= dShiftX/100.;
  Shift[1]= dShiftY/100.;
  Shift[2]= dShiftZ/100.;
  FillRMatrixZY(dRotMatrix, dRotY, dRotZ);
  RotBackVector(dRotMatrix, Shift);
  for (k=0; k<3; k++)
    EndPos[k] += Shift[k];
  dLength += LengthVector(Shift);
  dRotZ   += dHorizAngle;
  dRotY   += dVertAngle;
  WriteInstrData(nModuleNo, EndPos, dLength, dRotZ, dRotY);

  /* flush the output buffer and close the input and output file */
  OutputBufferFlush();
  if(InputFileName)
    fclose(InputFilePtr);
  if(OutputFilePtr && OutputFilePtr!=stdout)
    fclose(OutputFilePtr);

  /* release the buffer memory */
  free(InputNeutrons);
  free(OutputNeutrons);

  /* free GNU gsl rng state var */
  gsl_rng_free (vit_gsl_rng);

  /* error for the given count rate calculated through adding squared errors
     - of the number N of contributing traj.: sqrt(N) (Poisson distribution)
     - of the average count rate of each trajectory I_s = I_tot/N:
       sqrt((<I_s²> - <I_s>²)/(N-1))
     as independent contributions */
  if (NumNeutWritten > 1)
    dCntRateErr = sqrt( sq(dProbTotal[0])/NumNeutWritten
                      + (NumNeutWritten*dProbQuad-sq(dProbTotal[0])) / (NumNeutWritten-1) );
  else
    dCntRateErr = dProbTotal[0];

  fprintf(LogFilePtr, "%2ld number of trajectories read         : %11.0f\n", nModuleNo, NumNeutRead);
  fprintf(LogFilePtr, "   number of trajectories written      : %11.0f\n", NumNeutWritten);
  fprintf(LogFilePtr, "(time averaged) neutron count rate     : %11.4e +/- %10.3e n/s \n", dProbTotal[0], dCntRateErr);
  for (l=1; l<=MAX_COL; l++)
  { if (dProbTotal[l] > 0)
      fprintf(LogFilePtr, " count rate of colour %d                : %11.4e n/s \n", l, dProbTotal[l]);
  }

  if (dTimeMeas > 0.0)
  {
    nNoNeutrons = floor(dProbTotal[0]*dTimeMeas + 0.5);
    fprintf(LogFilePtr, "number of neutrons in %8.0f seconds : %11.4e  \n",
	    dTimeMeas, nNoNeutrons);
  }

  if (LogFileName) fclose(LogFilePtr);
}


/***********************************************************************/
/* 'print_module_name' writes the name (and version) of a module to    */
/*                     the LogFile                                     */
/***********************************************************************/

void print_module_name(char name[])
{
  char sNameHlp[41], *pBlank;

  fprintf(LogFilePtr,"\n\nVITESS version %s  module %s\n", VITESS_VERSION, name);

  /* Keeping name in mind (without "Space and" and without version number */
  if (strncmp(name, "Space and ", 10)==0)
    strcpy(sNameHlp, name+10);
  else
    strcpy(sNameHlp, name);

  pBlank=strrchr(sNameHlp, ' ');
  if (pBlank > sNameHlp)
    strncpy(sModuleName, sNameHlp, (int) Min(20, pBlank-sNameHlp));
  else
    strncpy(sModuleName, sNameHlp, 20);
}


/****************************************************************/
/* ReadNeutrons reads BufferSize neutrons form the input stream */
/* in the input neutron Buffer InputNeutrons. The number of     */
/* neutrons read is returned.                                   */
/****************************************************************/
int ReadNeutrons()
{
  long i;

  if (compressModeR == 0) {
    NumNeutGot = fread(InputNeutrons, sizeof(Neutron), BufferSize, InputFilePtr);
    for(i=0; i<NumNeutGot; i++) {
      WriteTraceLine(&InputNeutrons[i]);
      /* normalization of direction vector for modules representing hardware */
      if (stPicture.eModule < VT_MONITOR_1)
	NormVector(InputNeutrons[i].Vector);
    }
  } else {
    NumNeutGot = readCompressedNeutrons();
    // compressed neutrons are not meant for tracing
  }
  NumNeutRead += NumNeutGot;
  return NumNeutGot;
}


/*****************************************************************/
/* WriteNeutron writes a neutron to the neutron ouput buffer     */
/* OuputNeutrons and flushes the buffer to the ouput file if the */
/* buffer is full. Optput Neutrons contains BufferSize entries.  */
/*****************************************************************/

void WriteNeutron(Neutron *OutNeutron)
{
  dProbTotal[0] +=    OutNeutron->Probability;
  dProbQuad     += sq(OutNeutron->Probability);
  if (bSepRate && OutNeutron->Color >= 1 && OutNeutron->Color <= MAX_COL)
    dProbTotal[OutNeutron->Color] += OutNeutron->Probability;

  /* copy the neutron to the buffer */
  CopyNeutron(OutNeutron, &(OutputNeutrons[OutNeutPtr++]));

  if(OutNeutPtr >= BufferSize)
    /* write the neutrons to the stream if the buffer is full */
    OutputBufferFlush();

  WriteTraceLine(OutNeutron);
}



/***********************************************************************/
/* 'WriteSourceData()' writes Number of the current module  and        */
/*                            time of measurement                      */
/* 'ReadSourceData()'  reads these data                                */
/***********************************************************************/

void WriteInstrData(long nModuleNo, VectorType Pos, double dLength, double dRotZ, double dRotY)
{
  FILE*  pFile=NULL;
  char   *pBuffer, sBuffer[CHAR_BUF_SMALL+1];
  int    m, i;

  /* source module writes header lines */
  if (nModuleNo==0)
  { pFile = fopen( FullParName("instrument.inf"), "w");
    fprintf(pFile, "# No ID    module           len [m]  x [m]   y [m]   z [m]    hor. [deg] ver.      W-Par.       "
	    "H-Par.       R-Par       number  type Description\n");
    fprintf(pFile, "# ----------------------------------------------------------------------------------------------"
	    "--------------------------------------------------\n");
  }
  /* first module of 2nd, 3rd ... part re-writes file up to end of previous part */
  else if (InputFilePtr!=NULL && InputFilePtr!=stdin)
  { i=-1;
    pBuffer=malloc(CHAR_BUF_SMALL*(nModuleNo+3+NUM_EOP));
    pFile = fopen(FullParName("instrument.inf"), "r");
    if (pFile)
    { for (m=-2; m<nModuleNo; m++)
      { fgets (sBuffer, sizeof(sBuffer)-1, pFile);
        strcpy(&pBuffer[++i*CHAR_BUF_SMALL], sBuffer);
        if (memcmp(sBuffer, "EOP", 3)==0)
        { fgets (sBuffer, sizeof(sBuffer)-1, pFile);
          strcpy(&pBuffer[++i*CHAR_BUF_SMALL], sBuffer);
        }
      }
      fclose(pFile);
    }
    pFile = fopen( FullParName("instrument.inf"), "w");
    if (pFile)
    { for (m=0; m<=i; m++)
        fprintf(pFile, "%s", &pBuffer[CHAR_BUF_SMALL*m]);
      fprintf(pFile, "EOP\n");
    }
    free(pBuffer);
  }
  /* each other module appends a line */
  else
  { pFile = fopen(FullParName("instrument.inf"), "a");
  }

  if (pFile) {
    char cNF=' ';
    if (bOldFrame) cNF='F';
    fprintf(pFile, "%3ld %3d %-18.18s %7.3f %7.3f %7.3f %7.3f  %8.3f %8.3f  %12.4e %12.4e %12.4e %c %5ld %5d %s\n",
                   nModuleNo, stPicture.eModule, sModuleName, dLength, Pos[0], Pos[1], Pos[2],
                   180.0/M_PI*dRotZ, 180.0/M_PI*dRotY,
                   stPicture.dWPar,   stPicture.dHPar, stPicture.dRPar, cNF,
                   stPicture.nNumber, stPicture.eType, stPicture.pDescr);
    /* mark end of actual part */
    if (OutputFilePtr!=NULL && OutputFilePtr!=stdout && nModuleNo > 0)
      fprintf(pFile, "EOP\n");
    fclose(pFile);
  }
}

void ReadInstrData(long* pModuleNo, VectorType Pos, double* pLength, double* pRotZ, double* pRotY)
{
  FILE*  pFile=NULL;
  int    nModuleID;
  long   No=0, nDum;
  char   sBuffer[CHAR_BUF_LENGTH]="", sLine[CHAR_BUF_LENGTH]="", sLineH[CHAR_BUF_LENGTH]="";

  *pModuleNo = 0;
  Pos[0]   = Pos[1] = Pos[2] = 0.0;
  *pLength = 0.0;
  *pRotY   = 0.0;
  *pRotZ   = 0.0;

  pFile = fopen(FullParName("instrument.inf"), "r");
  if (pFile)
  {
    if (InputFilePtr==NULL || InputFilePtr==stdin)
    { /* Read last line and copy content, except:
		   lines containing F at pos 116-118, they have not a new frame) */
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
      { sscanf(sBuffer, "%ld", pModuleNo);
		  // ndig = short(floor(lg10(*pModuleNo));
        if (sBuffer[116]!='F' && sBuffer[117]!='F' && sBuffer[118]!='F') strcpy(sLine, sBuffer);
		}
    }
    else
    {  /* read until end of previous part, if input file is used */
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
      { if (memcmp(sBuffer, "EOP", 3)==0)
        {  *pModuleNo = No;
           strcpy(sLine, sLineH);
        }
        else
        { sscanf(sBuffer, "%ld", &No);
          if (sBuffer[116]!='F' && sBuffer[117]!='F' && sBuffer[118]!='F') strcpy(sLineH, sBuffer);
		  }
      }
      if (strlen(sLine)==0) {*pModuleNo = No; strcpy(sLine, sLineH);}
    }
    sscanf(sLine, "%ld %3d %18c %lf %lf %lf %lf %lf %lf",
                  &nDum, &nModuleID, sBuffer, pLength, &Pos[0], &Pos[1], &Pos[2], pRotZ, pRotY);
    *pRotZ *= M_PI/180.0;
    *pRotY *= M_PI/180.0;
    fclose(pFile);
  }
}


void WriteSimData(double dTimeMeas, double dLmbdWant, double dFreq)
{
  FILE*  pFile;

  pFile = fopen(FullParName("simulation.inf"), "w");
  if (pFile)
  { fprintf(pFile, "%15.5e   # measuring time     [s]\n", dTimeMeas);
    fprintf(pFile, "%10.5f        # desired wavelength [Ang]\n", dLmbdWant);
    fprintf(pFile, "%10.5f        # source frequency   [Hz]\n", dFreq);
    fclose(pFile);
  }
}

void ReadSimData(double* pTimeMeas, double* pLmbdWant, double* pFreq)
{
  FILE* pFile=NULL;
  char  sLine[CHAR_BUF_LENGTH];

  *pTimeMeas = 0.0;

  pFile = fopen(FullParName("simulation.inf"), "r");
  if (pFile)
  {
    /* First line - measuring time */
    ReadLine(pFile, sLine, sizeof(sLine)-1);
    sscanf(sLine, "%le", pTimeMeas);
    /* Second line - desired wavelength */
    ReadLine(pFile, sLine, sizeof(sLine)-1);
    sscanf(sLine, "%lf", pLmbdWant);
    /* Third line - frequency */
    ReadLine(pFile, sLine, sizeof(sLine)-1);
    sscanf(sLine, "%lf", pFreq);

    fclose(pFile);
  }
}



/***************************************************************/
/* Copies the contents of a structure 'Neutron' to another one */
/***************************************************************/

void CopyNeutron(Neutron *source, Neutron *dest)
{
  memcpy(dest, source, sizeof(Neutron));
}


/********************************************************************/
/* counts the number of lines in a text file and rewindes it        */
/********************************************************************/

long LinesInFile(FILE *pIn)
{
  char Buffer[CHAR_BUF_LARGE]="";
  long NumLines=0;

  rewind(pIn);
  if (pIn!=NULL)
  { while (ReadLine(pIn, Buffer, sizeof(Buffer)-1))
      NumLines++;
    rewind(pIn);
  }
  return NumLines;
}


/***********************************************************/
/* Function for counting the number of columns in a file   */
/*   pFile: pointer to file of interest                    */
/***********************************************************/
long ColumnsInFile(FILE* pFile)
{
#ifdef VERS26

  char  Buffer[CHAR_BUF_LARGE]="";
  char* pPos=NULL;
  int   iPos=0;
  long  nLns=0;

  if (pFile != NULL)
    {	ReadLine(pFile, Buffer, sizeof(Buffer)-1);
      
      pPos = strchr(Buffer, ' ');
      while (pPos != NULL)
	{
	  iPos = pPos - Buffer + 1;
	  if (iPos > 1)
	    nLns++;
	  StrgLShift(Buffer, iPos);
	  pPos = strchr(Buffer, ' ');
	}
      
      if (strlen(Buffer) > 1)
	nLns++;
      
      rewind(pFile);
    }

#else

  int i,v, nLns, isin;
  char buf[CHAR_BUF_LARGE];
  if (pFile == NULL)
    return 0;
  ReadLine(pFile, buf, CHAR_BUF_LARGE-1);
  rewind(pFile);
  for (nLns=isin=i=0; (v = buf[i]); i++)
    if (v != ' ')
      isin = 1;
    else if (isin) {
      nLns++;
      isin = 0;
    }
  if (isin)
    nLns++;

#endif

  return nLns;
}



/**************************************************************/
/* LOCAL FUNCTIONS                                            */
/**************************************************************/

#define HULK(a,b,s) memcpy((char*)(a), (char*)(b), s); wlen += s; a += s

static void writeCompressed() {

  Neutron *pn = OutputNeutrons;
  char *newspin,
    *op = (char *) OutputNeutrons;   // will be overwritten
  int dirbit, i, spincode, ineut;
  double dx,dy,dz, tarr[8];
  int wlen = 0;

  for (ineut = OutNeutPtr; ineut > 0; ineut--) {
    // copy data to prevent unwanted overwriting
    memcpy((char*) tarr, (char*) &pn->Time, 8*sizeof(double));

    // normalize direction
    dx = tarr[6]; dy = tarr[7]; dz = pn->Vector[2];
    double factor = 1.0 / sqrt(dx*dx + dy*dy + dz*dz);
    tarr[6] = dx * factor;
    tarr[7] = dy * factor;

    dirbit = dz > 0 ? 1 : 0;

    // code spin value
    newspin = 0;
    if (memcmp((void*) &pn->Spin, (void*) spinUpOut, sizeof(VectorType)) == 0)
      spincode = 1;
    else if (memcmp((void*) &pn->Spin, (void*) spinDownOut, sizeof(VectorType)) == 0)
      spincode = 2;
    else {
      if (spinVector++ % 2) {
	spincode = 3;
	newspin = (char*) spinUpOut;
      } else {
	spincode = 4;
	newspin = (char*) spinDownOut;
      }
      memcpy(newspin, (char*) &pn->Spin, sizeof(VectorType));
    }
    int *iop = (int *) op;
    *iop++ = pn->Color | ((dirbit | (spincode << 1)) << 16);  /* coded color + dirbit + spincode */
    op = (char*) iop;
    wlen += sizeof(int);

    if (newspin) {
      HULK(op, newspin, sizeof(VectorType));
    }

    // Write Time, Wavelength, Probability, Position vector and Vector x,y
    if (compressModeW == 1) {
      // lossless
      HULK(op, tarr, 8*sizeof(double));
    } else {
      // double -> float
      float  *fp = (float*) op;
      double *dp = tarr;
      for (i=0; i<6; i++)
	fp[i] = (float) (dp[i]);
      op   += 6*sizeof(float);
      wlen += 6*sizeof(float);
      // output direction dx, dy
      HULK(op, tarr + 6, 2*sizeof(double));
    }
    pn++;
  }
    
  fwrite(OutputNeutrons, 1, wlen, OutputFilePtr);
}
#undef HULK

#define GULP(s) p += s; rlen -= s;

static int readCompressedNeutrons (void) {

  Neutron *pn = InputNeutrons;
  char    *p, *readp;
  int     *pi, colword, dirbit,tocopy, i, rlen, ngot, toread, newread;
  static unsigned long idNo;

  if (! compressBuf)
    compressBuf = malloc(COMPRESSBUFLEN);
  p = readp = compressBuf;
  if (!p) return 0;

  ngot = 0;
  toread = COMPRESSBUFLEN;
  if ((rlen = compressedRestlen) > 0) {
    // do not overwrite saved rest, but add fresh input
    readp += rlen;
    toread -= rlen;
    compressedRestlen = 0;
  }
  newread = fread(readp, 1, toread, InputFilePtr);
  if (rlen <= 0 && newread <= 0) 
    return 0;
  if (newread > 0)
    rlen += newread;
  ngot = 0;
  do {
    double x,y,z;
    pn->ID.IDGrp[0] = 'A';
    pn->ID.IDGrp[1] = 'A';
    pn->ID.IDNo = ++idNo;
    pn->Debug = 'N';
    // get color, spin code, and direction bit from a 4 byte int
    pi = (int *)p;
    GULP(sizeof(int));
    colword = *pi;
    pn->Color = (short) colword & 0xffff;
    colword >>= 16;
    dirbit = colword & 1;

    switch (colword >> 1) {
    case 3: // new spin up
      memcpy((char*)spinUp, p, sizeof(VectorType));
      GULP(sizeof(VectorType));
    case 1: // spin up
      memcpy((void*) &pn->Spin, (void*)spinUp, sizeof(VectorType));
      break;
    case 4: // new spin down
      memcpy((char*)spinDown, p, sizeof(VectorType));
      GULP(sizeof(VectorType));
    case 2: // spin down
      memcpy((void*) &pn->Spin, (void*)spinDown, sizeof(VectorType));
      break;
    default: 
      return 0; 		/* bad data */
    }

    if (compressModeR == 1) {
      // lossless
      tocopy = 8*sizeof(double);
      memcpy((char *) &(pn->Time), p, tocopy);
      GULP(tocopy);
    } else {
      // copy 6 floats -> double
      float  *fp = (float*) p;
      double *dp = & pn->Time;
      for (i=0; i<8; i++)
	dp[i] = fp[i];
      GULP(6*sizeof(float));
      // copy direction x,y as double
      tocopy = 2*sizeof(double);
      memcpy((char *) &(pn->Vector), p, tocopy);
      GULP(tocopy);
    }
    x = pn->Vector[0]; y = pn->Vector[1];
    z = sqrt(1.0 - x*x - y*y);
    pn->Vector[2] = dirbit ? z : -z;

    ngot++;
    pn++;
  } while (ngot < BufferSize && (rlen >= sizeof(Neutron) || (rlen > 0 && newread == 0)));
 
  if (rlen > 0 && newread > 0) {
    // save the rest
    compressedRestlen = rlen;
    memcpy(compressBuf, p, rlen);
  }
  return ngot;
}
#undef GULP

static
void OutputBufferFlush()
{
  if (OutputFilePtr) {
    if (compressModeW == 0)
      fwrite(OutputNeutrons, sizeof(Neutron), OutNeutPtr, OutputFilePtr);
    else
      writeCompressed();
  }
  NumNeutWritten += OutNeutPtr;
  OutNeutPtr = 0;
  if(TracePoints) fprintf(LogFilePtr,".");
}

/* Writing one line into the trace file */
static
void   WriteTraceLine(Neutron* pNeutron)
{
  if (bTrace && pNeutron->Debug=='T')
  {
    char   sFileName[21]="";
    short  nModNr=1, nLns;
    FILE*  pFile;

    /* Generating file name from ID and add new line */
    sprintf(sFileName, "Trc%c%c%09lu.dat", pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo);
    if (strcmp(sModuleName, "Source and Window")==0)
    { pFile = fopen(FullParName(sFileName), "w");
      fprintf(pFile,"no     module           Trc color   TOF    lambda   count rate    "
			           "pos_x    pos_y    pos_z     dir_x     dir_y     dir_z     sp_x sp_y sp_z\n");
    }
    else
    { pFile = fopen(FullParName(sFileName), "r+");
      if (pFile != NULL)
      { nLns   = (short) LinesInFile(pFile);
        nModNr = (short) (nLns/2 + 1);
        fseek(pFile, 0, 2);     // setting pointer to end of file
      }
      else
      { pFile = fopen(FullParName(sFileName), "a");
      }
    }
    fprintf(pFile, "%2d %-20.20s:",  nModNr, sModuleName);
    fprintf(pFile," %c %5d  %7.3f %8.5f %11.3e  %8.4f %8.4f %8.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n",
            pNeutron->Debug,       pNeutron->Color,
            pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
            pNeutron->Position[0], pNeutron->Position[1], pNeutron->Position[2],
            pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
            pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]);
    /* fprintf(pFile, "%2d %-20.20s: t=% .5e y=% .5e z=% .5e col=%5d\n",  nModNr, sModuleName,
                   pNeutron->Time, pNeutron->Position[1], pNeutron->Position[2], pNeutron->Color);*/
    fclose (pFile);
  }
}
