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
/* Jan 2004  K. Lieutenant  new function 'FulName' and changes for 'instrument.dat          */
/* Feb 2004  M. Fromme      functions FullParName and FullInstallName                       */
/* Feb 2004  K. Lieutenant  use of Full...Name, 'instrument.inf' and 'simulation.inf'       */
/* Mar 2008  M. Fromme      compressed neutron data                                         */
/* Jan 2010  M. Fromme      support for parallel thread execution                           */
/* Oct 2010  M. Fromme      progress meter                                                  */
/* Mar 2011  M. Fromme      externally gzip comressed neutron data                          */
/* Jan 2012  K. Lieutenant  visualization                                                   */
/********************************************************************************************/

#ifdef _MSC_VER
# include <fcntl.h>
# include <io.h>
# define SET_BINARY_MODE(file) if (_setmode(_fileno(file),O_BINARY) == -1) myExit("Can't set binary mode\n");
#else
# define SET_BINARY_MODE(file)
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <time.h>
#include <sys/resource.h>
#endif

#include "convert.h"
#include "init.h"


#define MAX_COL            6   // max. number of count rates written separately for different colours, 0 means no separate rates writable
#define NUM_EOP           30   // number of end-of-part lines that can be treated in 'instrument.inf'
#define MAX_TRAJ        1200
#define COMPRESSBUFLEN 65536


/***************************************************************************************************/
/* GLOBAL VARIABLES                                                                                */
/* ----------------                                                                                */
/* This file contains several global variables which are essential to each VITESS program module   */
/***************************************************************************************************/
extern FILE*    LogFilePtr;  /* pointer to the log file stream  */


const
char *sInstrInfOut = "instrument.inf"; /* instrument file that is written ('instrument.inf')      */
char *sInstrInfIn  = "instrument.inf"; /* instrument file that is read (default 'instrument.inf') */

McCompID _eModule=MCN_COMP_UNKNOWN;    /* ID of the module                */
double   BlowUp;             /* Factor, by which width and height are extended for visualization */
long     BufferSize;         /* size of the neutron input and output buffer */
long     CompressedSize;     /* if > 0, set for 2. module to indicate size of file gzipped by 1. module */
int      CompressionMode;    /* if > 0, data compression mode 1 (nodebug) 2(float) */
Neutron* InputNeutrons;      /* input neutron Buffer */
Neutron* OutputNeutrons;     /* output neutron buffer */
Neutron *OutNeutronsCopy;    /* if this has been allocated in source.c, it may be used for double buffering */
long     OutNeutNum;         /* number of the next free position in OutputNeutrons */
// ModProp  stPicture;          /* additional information for 'instrument.inf' */
VtModGeom stGeometry;        /* data needed to draw a picture of the component represented by the module */

long     NumNeutGot=0;       /* number of trajectories read in the current batch */
double   NumNeutRead=0.0;    /* number of trajectories read in total */
double   NumNeutWritten=0.0; /* number of trajectories written in total */
long     NumEobRead=0;       /* number of 'EndOfBunch' data sets read in total */
long     NumEobWritten=0;    /* number of trajectories written in total */

FILE*    InputFilePtr;       /* stream from which the neutrons are read */
FILE*    OutputFilePtr;      /* stream to which the neutrons are written */
FILE*    TrajFilePtr=NULL;   /* pointer to file into which the interaction points of the trajectories are written */
char*    InputFileName;      /* file to read neutrons */
char*    OutputFileName;     /* file to write neutrons */
char*    LogFileName;        /* log file name  */
const char *pGeomFileName="geometry.inf"; /* name of instrument geometry file */
char*    pTrajFileName=NULL; /* trajectory file name  */
char*    ParDir;             /* parameter directory */
char*    InstallDir;
char     sModuleName[MOD_NAME_LEN+1]="";
char     sVisDescrpt[MOD_NAME_LEN+9]="";

char*    ProgressFile;       /* file to write progress in percent*/
int      SourcePercent;      /* quantisized progress so far */
long     SourceSize;         /* input file size */

double   wei_min=0.0;        /* Minimal weight for tracing neutron */
long     keygrav=1;
short    bInit =FALSE,       /* criterion: function Init() has been carried out already  */
         bTrace=TRUE,        /* criterion: write trace files             */
         bOldFrame=FALSE,    /* criterion: co-ordinate system of prev. module used for current module */
         bSepRate =TRUE,     /* criterion: write separate count rates    */
         bTest    =FALSE,    /* criterion: test run (without trajectories)   */
         bVisInstalled=FALSE,/* criterion: visualization routines installed */
         bBlowUp=FALSE,      /* criterion: blow up active in module, i.e. IAP is also blown up */
         bVisInstr=TRUE,     /* criterion: instrument visualization      */
         bVisTraj =FALSE;    /* criterion: visualization of trajectories */
double   BlnLen=0.0,         /* [cm] length of beamline from source to origin of this module */
         RotZ=0.0, RotY=0.0, /*      hor. and vert. rotation of the local co-ordinate system relative to the absolute one  */
         RotMatrixM[3][3],
         RotMatrixMX[3][3],
         RotMatrixS[3][3];   /*      matrix to rotate from abs. co-ordinate system to co-ordinate system of last section   */
long     nModuleNo=0,        /*      number of the actual module, set in Init() or Cleanup() */
         iModuleId=0,        //      ID of this module
         lastIDShift=0;      // Shift of the ID needed for visualisation in case modules create additional trajectories
int      powerIDShift=9;     // Needed to include the module number in the overall neutron ID avoiding dublication
VectorType vNull={0.0,0.0,0.0},
           vX   ={1.0,0.0,0.0},
           BegPosM={0.0,0.0,0.0}, /* [cm] end position of prev. module = origin of this module in absolute co-ordinate system   */
           BegPosS={0.0,0.0,0.0}; /* [cm] end position of prev. section = origin of this section in absolute co-ordinate system */

TotalID  tempID;
McCompID eFirstMod=MCN_COMP_UNKNOWN;

#ifndef WIN32
struct timespec tStart;
#ifdef CLOCK_MONOTONIC
#define VT_CLOCK CLOCK_MONOTONIC
#else
#define VT_CLOCK CLOCK_REALTIME
#endif
#endif


/**************************************************************/
/* STATIC VARIABLES                                           */
/**************************************************************/
static char       sFilePath[256]="";

static char*      InputDir =NULL;       //  [PATH_LEN]="";
static char*      OutputDir=NULL;       //  [PATH_LEN]="";

static long       TracePoints=FALSE;     /* creates dot for every written output buffer if TRUE */
static double     dProbTotal[MAX_COL+2], /* sum of the count rates of all trajectories [n/s]    */
                  dProbQuad;             /* sum of the squares of the count rates of all traj.  */

static int        ParDirLength,
                  InstallDirLength;

static int        compressModeR, compressModeW, compressBufLen, compressedRestlen, spinVector;
static long       byte_read_so_far;
static VectorType spinUp, spinDown, spinUpOut, spinDownOut;
static char      *compressBuf, *zcat_p;

int               NThreads=0;              // number of helper threads for execution, set by --T
unsigned long int VRandomSeed=0;           // random seed, default 0, set by --Z
int               bLogTimes=0;             // print out computation times to the log (UNIX only), set by --l


/**************************************************************/
/* PROTOTYPES OF LOCAL FUNCTIONS                              */
/**************************************************************/
static void  setInstallDirectory(char *arg);
static char* setDir (char *arg);
static char* conCat (const char *sFile, const char* sSubDir, VtDirType sel);
static McCompID GetModId(char* sBuffer);                            // returns ID of the module from a line in 'instrument.inf'
static void  Transform(VectorType AbsVec, const VectorType vRelVec, const VectorType vBegVec);
static void  writeCompressed();
static int   readCompressedNeutrons();
static void  WriteTraceLine(Neutron* Neut);
static short GetColMax();                                           // Returns maximum color with intensity > 0

// these function should only be used exceptionally outside init.c
char* FullInstallName (const char* filename, const char* sRelPath); // adds installation directory to file name
char* FullParName     (const char* filename);                       // adds parameter directory to file name
char* FullInName      (const char* filename);                       // adds input dir to file name
char* FullOutName     (const char* filename);                       // adds output dir to file name


/**************************************************************/
/* GLOBAL FUNCTIONS                                           */
/**************************************************************/

/* Adds path of the installation directory to a file name */
char* FullInstallName(const char* sFileName, const char* sRelPath)
{
  TotalPath(sFilePath, sFileName, sRelPath, INSTL_DIR);

  return sFilePath;
}

/* Adds the path of a directory - input, output or parameter - to a file name */
char* FullParName(const char* fileName)
{
  return conCat(fileName, "", PAR_DIR);
}

char* FullInName(const char* fileName)
{
  return conCat(fileName, "", IN_DIR);
}

char* FullOutName(const char* fileName)
{
  return conCat(fileName, "", OUT_DIR);
}


/* Adds the path of a directory - input, output or install_dir/sPath - to a file name
   opens the file using parameters 'sMode'
   and exits with error message if bErrMsg=TRUE     */
FILE* OpenInputFile(const char *sFilename, short bErrMsg, const char* sMode)
{
  char *sFullName = NULL;
  FILE *pFile = NULL;

  if (sFilename != NULL) {
    sFullName = FullInName(sFilename);
    if (sFullName) {
      if (bErrMsg)
        pFile = fileOpen(sFullName, sMode);
      else
        pFile = fopen(sFullName, sMode);
      free(sFullName);
    }
  }
  return (pFile);
}

FILE* OpenInputFile2(const char *sFilename, const char* sContent, const char* sMode)
{
  char *sFullName = NULL;
  FILE *pFile = NULL;

  if (sFilename != NULL) {
    sFullName = FullInName(sFilename);
    if (sFullName) {
      pFile = fileOpen2(sFullName, sMode, sContent);
      free(sFullName);
    }
  }

  return (pFile);
}

FILE* OpenOutputFile(const char *sFilename, short bErrMsg, const char* sMode)
{
  char *sFullName = NULL;
  FILE *pFile = NULL;

  if (sFilename != NULL) {
    sFullName = FullOutName(sFilename);
    if (sFullName) {
      if (bErrMsg)
        pFile = fileOpen(sFullName, sMode);
      else
        pFile = fopen(sFullName, sMode);
      free(sFullName);
    }
  }
  return (pFile);
}

FILE* OpenPackInpFile(const char *sFilename, const char* sPath, short bErrMsg)
{
  FILE *pFile = NULL;

  if (sFilename != NULL) {
    if (bErrMsg)
      pFile = fileOpen(FullInstallName(sFilename, sPath), "r");
    else
      pFile = fopen(FullInstallName(sFilename, sPath), "r");
  }
  return (pFile);
}

// tools for detached fwrite
static FILE *TWfile;
static size_t TWsize;
static int TWn;
static void *TWdata;

#ifdef WIN32

#ifndef WIN32KNOWN
# include <windows.h>
# include <process.h>
# define WIN32KNOWN 1
#endif

static HANDLE hWriteMutex;

void initParWrite(size_t s, int n) {
  if (TWdata) return;
  hWriteMutex = CreateMutex( NULL, FALSE, NULL );  // Cleared
  if (hWriteMutex == NULL)
    fprintf(LogFilePtr,"unable to CreateMutex, error: %d\n", GetLastError());
  else
    TWdata = malloc(s*n);
}

static void threadWriter (void *arg) {
  int nwr, rc;
  if (!TWdata) return;
  rc = WaitForSingleObject(hWriteMutex, INFINITE);
  if (rc)
    fprintf(LogFilePtr,"WaitForSingleObject problem, rc %d\n", rc);
  nwr = fwrite(TWdata, TWsize, TWn, TWfile);
  if (nwr != TWn)
    fprintf(LogFilePtr,"thread write problem, only %d of %d items written\n", nwr, TWn);
  rc = ReleaseMutex(hWriteMutex);
  if (rc == 0)
    fprintf(LogFilePtr,"unable to ReleaseMutex in threadWriter, error: %d\n", GetLastError());
}

static int fwritePar(void *d, size_t s, int n, FILE *f, int final) {
  uintptr_t trc;
  int nwr, rc;

  if (!TWdata)
  {
    nwr = fwrite(d, s, n, f);
    return n == nwr;
  }
  // Wait here, if a threadWriter is occupied by an older write operation
  // by allocating hWriteMutex.
  // The mutex becomes unlocked only after completion of threadWriter.
  rc = WaitForSingleObject( hWriteMutex, INFINITE);

  if (rc)
    fprintf(LogFilePtr,"WaitForSingleObject problem, rc %d\n", rc);
  if (final)
  {
    nwr = fwrite(d, s, n, f);
    return n == nwr;
  }
  memcpy(TWdata, d, s*n);
  TWsize = s;
  TWn = n;
  TWfile = f;
  rc = ReleaseMutex(hWriteMutex);
  if (rc == 0)
    fprintf(LogFilePtr,"unable to ReleaseMutex in fwritePar, error: %d\n", GetLastError());

  trc = _beginthread( threadWriter, 0, 0);

  return trc != 0 && trc != -1;
}

#else
// Linux

# include <sys/types.h>
# include <pthread.h>

void initParWrite(size_t s, int n) {
  if (!TWdata)
    TWdata = malloc(s*n);
}

static void *threadWriter (void *arg) {
  if (TWdata) {
    int nwr;
    nwr = fwrite(TWdata, TWsize, TWn, TWfile);
    if (nwr != TWn)
      fprintf(LogFilePtr,"thread write problem, only %d of %d items written\n", nwr, TWn);
  }
  return arg;
}

static pthread_t writerThread;

static int fwritePar(void *d, size_t s, int n, FILE *f, int final) {
  int nwr, rc;
  if (!TWdata) {
    nwr = fwrite(d, s, n, f);
    return nwr == n;
  }
  // Wait here, if an older threadWriter is still writing.
  if (writerThread) {
    rc = pthread_join(writerThread, 0);
    writerThread = 0;
  }

  if (final) {
    nwr = fwrite(d, s, n, f);
    return nwr == n;
  }
  memcpy(TWdata, d, s*n);
  TWsize = s;
  TWn = n;
  TWfile = f;
  rc = pthread_create(&writerThread, NULL, threadWriter, (void *) 0);
  return rc == 0;
}

#endif


static void setCompressBufLen()
{
  int full_len = sizeof(Neutron) * BufferSize;
  compressBufLen = COMPRESSBUFLEN > full_len ? full_len : COMPRESSBUFLEN;
}


/**************************************************************
 Init does a general program initialization, which is ok
 for all modules of the VITESS program package.
 A processed option is marked by setting the leading - to +
 Option processed here:
  --B  number of buffer entries
  --b  blow up
  --c  size        (if the first module is zcat, the second
                    gets the compressed file's size here)
  --C  mode        data compression mode 1 (nodebug) or float (2)
  --f  input file name
  --F  output file name
  --G  gravitation
  --i  input directory
  --I  instrument file of previous part of the simulation
  --J  active trace points
  --l  log computation times
  --L  logfile
  --N  module number 1,2,...
  --o  output directory
  --p  progress file
  --P  parameter directory
  --t  test mode
  --T  number of helper threads for execution
  --U  minimal neutron weight
  --v  visualization output: geometry file
  --V  visualization output: trajectory file
  --z  reserved for future random initialization
  --Z  random number generator initialization
*************************************************************/
void Init(int argc, char **argv, const McCompID eModule)
{
  short l, ii, jj;
  char  *a, *arg, text[99];
  const gsl_rng_type * T;

  /* Set some default values */
  InputFilePtr   = stdin;
  OutputFilePtr  = stdout;
  LogFilePtr     = stderr;
  InputFileName  = NULL;
  OutputFileName = NULL;
  LogFileName    = NULL;
  ParDir         = NULL;
  BlowUp         = 1.0;
  BufferSize     = BUFFER_SIZE;
  OutNeutNum     = 0;
  TracePoints    = FALSE;
  #ifndef WIN32
  clock_gettime(VT_CLOCK, &tStart);
  #endif
  for (l=0; l<=MAX_COL+1; l++)
    dProbTotal[l] = 0.0;

  /* memset(&stPicture, '\0', sizeof(ModProp));
  stPicture.eModule= eModule;
  stPicture.nNumber= 1L; */

  memset(&stGeometry, '\0', sizeof(VtModGeom));
  stGeometry.eModule= eModule;

  setInstallDirectory(*argv++);  // extract installation path from program name

  while ((a = *argv++)) {
    if ('-' != *a || '-' != a[1]) continue; // first two chars must be -
    arg = a + 3;
    switch (a[2]) {

    case 'B':                   // determine the buffer size
      BufferSize = atol(arg);
      break;
    case 'b':                   // determine the blow up factor
      BlowUp = atof(arg);
      break;

    case 'c' :
      CompressedSize = atol(arg);
      break;
    case 'C' :
      CompressionMode= atoi(arg);
      break;

    case 'f' :                   // input file if other than stdin
      InputFileName = arg;
      break;
    case 'F':                   // output file if other than stdout
      OutputFileName = arg;
      break;
    case 'l' :
      bLogTimes = atoi(arg);    // print out computation times to the log (UNIX only)
      break;
    case 'L':                   // output file if other than stderr
      LogFileName = arg;
      break;
    case 'I':                   // instrument file for reading data if other than instrument.inf
      sInstrInfIn = arg;
      break;

    case 'G':
      keygrav = atol(arg);      // key for gravity 1 -yes (default), 0 - no
      break;

    case 'J' :
      TracePoints=TRUE;
      break;

    case 'N':                   // module number
      iModuleId = atol(arg);
      break;

    case 'p':                   // progress file
      ProgressFile = arg;
      break;

    case 'P':                   // parameter (= default) directory
      ParDir=setDir(arg);
      break;
    case 'i':                   // input directory
      InputDir=setDir(arg);
      break;
    case 'o':                   // output directory
      OutputDir=setDir(arg);
      break;

    case 't' :
      bTest  = TRUE;
      bVisInstr = FALSE;
      break;

    case 'T' :
      NThreads = atol(arg);     // requested number of threads for execution
      break;

    case 'U':
      wei_min = atof(arg);      // minimal weight for tracing neutron
      break;

    case 'v' :
      pGeomFileName = arg;      // geometry file name
      bVisInstr=TRUE;
      break;

    case 'V' :
      pTrajFileName= arg;       // trajectory file name
      TrajFilePtr  = OpenOutputFile(pTrajFileName, TRUE, "w");
      bVisTraj     = TRUE;
      break;

    case 'Z':                   // init number for the random number generator
#if defined(PENV) || defined(_MSC_VER)
      if (sscanf(arg, "%li", &VRandomSeed)) {
        char buf[24];
        sprintf(buf, "GSL_RNG_SEED=%ld", VRandomSeed);
#      ifdef _MSC_VER
        _putenv(buf);
#      else
        putenv(buf);
#      endif
      }
#else
      if (sscanf(arg, "%li", &VRandomSeed))
        setenv("GSL_RNG_SEED", arg, 1);
#endif
      break;

    default :
      continue;
    }
    *a = '+';                   // remember that this argument has been processed
  }

  // The parameter directory is used, if no specific input and output directories are given
  if (InputDir ==NULL) InputDir  = ParDir;
  if (OutputDir==NULL) OutputDir = ParDir;

  // First of all try to open the log file, if it has been requested.
  if (LogFileName!=NULL)
  {
    // Do not use 'fileOpen' to avoid recursion, but open directly using 'fopen', i.e. set bErrMsg to FALSE.
    LogFilePtr = OpenOutputFile(LogFileName, FALSE, "w");
    if (LogFilePtr == NULL)
    {
      printf("ERROR: Can't open log file %s%c%s!\n", OutputDir, cSlash, LogFileName);
      exit (-1);
    }
  }

  // Then we care for input, decide if it is compressed.
  if (InputFileName!=NULL)
  {
    // we got some --f argument
    if (strcmp(InputFileName, "no_file") == 0)
      InputFilePtr = NULL;
    else
      InputFilePtr = OpenInputFile(InputFileName, TRUE, "rb");

    if (InputFilePtr!=NULL)
    {
      char b[4];
      // Get the file size to enable a progress bar
      if (0 == fseek(InputFilePtr, 0, SEEK_END))
      {
        SourceSize = ftell(InputFilePtr);
        rewind(InputFilePtr);
      }
      // Is it compressed ?
      compressModeR = 0;
      if (4 == fread(b, 1, 4, InputFilePtr))
      {
        if (memcmp(b, "cmp2", 4) == 0)
          compressModeR = 2;
        else if (memcmp(b, "cmp1", 4) == 0)
          compressModeR = 1;
      }
      if (compressModeR)
        compressedRestlen = spinVector = 0;
      else
        rewind(InputFilePtr);
    }
  }
  else
  {
    SET_BINARY_MODE(stdin);
    if (CompressedSize)
    {
      double factor;
      // we are second in the pipe, reading zcat output
      setCompressBufLen();
      zcat_p = compressBuf = (char*) malloc(compressBufLen);
      compressedRestlen = fread(compressBuf, 1, compressBufLen, stdin);
      if (compressedRestlen < 512)
        myExit("dubious data from first module\n");
      // Is it vitess-compressed also ?
      if (memcmp(compressBuf, "cmp2", 4) == 0)
        compressModeR = 2;
      else if (memcmp(compressBuf, "cmp1", 4) == 0)
        compressModeR = 1;
      else
        compressModeR = 0;

      if (compressModeR)
      {
        zcat_p += 4;
        compressedRestlen -= 4;
      }

      factor = compressModeR == 0 ? 55.0 : compressModeR == 2 ? 29.0 : 45.8;
      SourceSize = (long)(CompressedSize * 100.0 / factor);
    }
  }

  // Here we care for output, and if data compression is an option.
  if (OutputFileName!=NULL)
  {
    if (strcmp(OutputFileName, "no_file") == 0)
    { OutputFilePtr = NULL;
    }
    else
    {
      OutputFilePtr = OpenOutputFile(OutputFileName, TRUE, "wb");
      if (OutputFilePtr)
      {
        char *sFullName = FullOutName(OutputFileName);
        if (CompressionMode)
          compressModeW = CompressionMode; // it has been stated explicitly
        else if (strstr(sFullName, ".float."))
          compressModeW = 2;               // by filename convention
        else if (strstr(sFullName, ".nodebug."))
          compressModeW = 1;               // by filename convention

        if (compressModeW == 1)
          fwrite("cmp1", 1, 4, OutputFilePtr);
        else if (compressModeW == 2)
          fwrite("cmp2", 1, 4, OutputFilePtr);
        else
          compressModeW = 0;               // to catch an unknown CompressionMode
        free(sFullName);
      }
    }
  }
  else
  {
    SET_BINARY_MODE(stdout);
  }

  /* allocate memory for the neutron buffers */
  if ( (InputNeutrons  = (Neutron *)calloc(BufferSize, sizeof(Neutron))) == NULL ||
       (OutputNeutrons = (Neutron *)calloc(BufferSize, sizeof(Neutron))) == NULL)
    myExit("Couldn't allocate memory for I/O buffer\n");

  /* Initialize the GNU random number generator */
  gsl_rng_env_setup();
  T = gsl_rng_default;
  vit_gsl_rng = gsl_rng_alloc (T);
  if (VRandomSeed)
    gsl_rng_set(vit_gsl_rng, VRandomSeed);

  /* Read instrument data */
  if (bVisTraj)
  { if (eModule==MCN_SOURCE)
    { nModuleNo=1;
      BegPosM[0]=BegPosM[1]=BegPosM[2]=0.0;
      BegPosS[0]=BegPosS[1]=BegPosS[2]=0.0;
      BlnLen=0.0;
      RotY  = RotZ = 0.0;
    }
    else
    { nModuleNo=ReadInstrData(iModuleId, BegPosM, &BlnLen, &RotZ, &RotY, sInstrInfIn);
      if (nModuleNo==-1)
      { sprintf(text, "Module '%ld' could not be found in '%s'", iModuleId, sInstrInfIn);
        Error(text);
      }
      CopyVector(BegPosM, BegPosS);
    }
    FillRMatrixZY(RotMatrixM, RotY, RotZ);
    FillRMatrixZY(RotMatrixS, RotY, RotZ);

    /* Determine new ID scheme for neutrons cloned in the last module */
    if (nModuleNo > 99) powerIDShift -= 2;
    else if (nModuleNo > 9 && nModuleNo < 100) powerIDShift -= 1;
    lastIDShift = nModuleNo*pow(10.0, powerIDShift);
  }

  for (ii = 0; ii < 3; ii++) {
    for (jj = 0; jj < 3; jj++) {
      if (ii==jj) RotMatrixMX[ii][jj]=1.;
      else RotMatrixMX[ii][jj]=0.;
    }
  }

}


/*****************************************************************/
/* Cleanup() does last things before the VITESS module is closed */
/* e.g. buffers are flushed and files are closed etc.            */
/* you should also write your OwnCleanup() for your module       */
/*****************************************************************/
void Cleanup(double dShiftX, double dShiftY, double dShiftZ,
             double dHorizAngle, double dVertAngle)
{
  double TimeMeas=0.0, LmbdWant=0.0, Freq=0.0,
         nNumNeutr=0.0, nNumNeutrSrc=0.0,
         CntRateErr=0.0;
  long   nBnch=0;
  int    l=0;
  int    iCol=0, iColMin=1, iColMax=0;
  VectorType Shift,  /* Shift of end position        [m] */
             EndPos; /* end position of this module  [m] */

  /* update 'instrument.inf' */
  if (!bVisTraj)
  { if (_eModule == MCN_SOURCE)
    { nModuleNo = 1;
      BegPosM[0]= BegPosM[1] = BegPosM[2] = 0.0;
      BlnLen=0.0;
      RotY  = RotZ = 0.0;
    }
    else
    { if (bTest) Wait(0.75*iModuleId);
      nModuleNo = ReadInstrData(0, BegPosM, &BlnLen, &RotZ, &RotY, sInstrInfIn);
    }

    FillRMatrixZY(RotMatrixM, RotY, RotZ);

    ReadSimData  (&TimeMeas, &LmbdWant, &Freq, &nNumNeutrSrc, &nBnch);
    // nModuleNo++;
    Shift[0]= dShiftX;
    Shift[1]= dShiftY;
    Shift[2]= dShiftZ;
    RotBackVector(RotMatrixM, Shift);
    RotBackVector(RotMatrixMX, Shift);
    for (l=0; l<3; l++)
      EndPos[l] = BegPosM[l] + Shift[l];
    BlnLen += LengthVector(Shift);
    RotZ   += dHorizAngle;
    RotY   += dVertAngle;

    // fprintf(LogFilePtr, "writing instr data, module %ld\n", nModuleNo);
    WriteInstrData(EndPos);
  }

  if (bVisInstr)
  {
    if (bVisTraj)           // in the other case, the parameters were determined already
    { Shift[0]= dShiftX;
      Shift[1]= dShiftY;
      Shift[2]= dShiftZ;
      ReadInstrData(iModuleId, BegPosM, &BlnLen, &RotZ, &RotY, sInstrInfIn);
    }
    WriteGeomData(BegPosM, LengthVector(Shift));
  }


  /* flush the output buffer and close the input and output file */
  OutputBufferFlush(1);
  if(InputFilePtr)
    fclose(InputFilePtr);
  if(OutputFilePtr && OutputFilePtr != stdout)
    fclose(OutputFilePtr);

  /* release the buffer memory */
  if (InputNeutrons!=NULL) {
    free(InputNeutrons);
    InputNeutrons = NULL;
  }
  if (OutputNeutrons!=NULL) {
    free(OutputNeutrons);
    OutputNeutrons = NULL;
  }
  if (ParDir != NULL) {
    free(ParDir);
    ParDir = NULL;
  }
  if (InstallDir != NULL) {
    free(InstallDir);
    InstallDir = NULL;
  }

  /* free GNU gsl rng state var */
  gsl_rng_free (vit_gsl_rng);
  vit_gsl_rng = NULL;

  // subtract number of dummy data sets first
  NumNeutRead    -= NumEobRead;
  NumNeutWritten -= NumEobWritten;

  /* error for the given count rate calculated through adding squared errors
     - of the number N of contributing traj.: sqrt(N) (Poisson distribution)
     - of the average count rate of each trajectory I_s = I_tot/N: sqrt((<I_s²> - <I_s>²)/(N-1))
     as independent contributions */
  if (NumNeutWritten > 1)
    CntRateErr = sqrt( sq(dProbTotal[0])/NumNeutWritten
                      +  (NumNeutWritten*dProbQuad-sq(dProbTotal[0])) / (NumNeutWritten-1) );
  else
    CntRateErr = dProbTotal[0];

  fprintf(LogFilePtr, "%2ld number of trajectories read         : %11.0f\n", nModuleNo, NumNeutRead);
  fprintf(LogFilePtr, "%2ld number of trajectories written      : %11.0f\n", iModuleId, NumNeutWritten);
  fprintf(LogFilePtr, "(time averaged) neutron count rate     : %11.4e +/- %10.3e n/s \n", GetTotInt(ANY_COLOR), CntRateErr);
  iColMax= GetColMax();
  if (iColMax > 0)     // only write "intensity of color 0" if there exists a color > 0
    iColMin=0;
  for (iCol=iColMin; iCol<=iColMax; iCol++)
  { if (GetTotInt(iCol) > 0)
      fprintf(LogFilePtr, " count rate of colour %d                : %11.4e n/s \n", iCol, GetTotInt(iCol));
  }

  if (TimeMeas > 0.0)
  {
    nNumNeutr = floor(GetTotInt(ANY_COLOR)*TimeMeas + 0.5);
    fprintf(LogFilePtr, "number of neutrons in %8.0f seconds : %11.4e  \n", TimeMeas, nNumNeutr);
  }
#ifndef WIN32
  if (bLogTimes) {
    struct timespec tEnd;
    struct rusage ru;
    if (clock_gettime(VT_CLOCK, &tEnd) == 0 && getrusage(RUSAGE_SELF, &ru) == 0) {
      double wtime = (tEnd.tv_sec - tStart.tv_sec) + (tEnd.tv_nsec - tStart.tv_nsec) / 1e9;
      double utime = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6;
      double nneut = NumNeutRead > 0.0 ? NumNeutRead : NumNeutWritten;
      fprintf(LogFilePtr, "Module %ld, %s, wall time: %fs, cpu time: %fs, time per million neutrons: %fs\n",
          iModuleId, sModuleName, wtime, utime, utime * 1e6 / nneut);
    }
  }
#endif

  if (LogFileName) {
    fclose(LogFilePtr);
    LogFilePtr = NULL;
  }
  if (TrajFilePtr) {
    fclose(TrajFilePtr);
    TrajFilePtr = NULL;
  }
}


/***********************************************************************/
/* 'print_module_name' writes the name (and version) of a module to    */
/*                     the LogFile                                     */
/***********************************************************************/
void PrintModuleName(const McCompID eModule, const char* sModuleVsn)
{
  CompID2Name(sModuleName, eModule);

#ifdef WIN32
# if defined(VMAJOR) && defined(VMINOR)
    fprintf(LogFilePtr,"\n\nVITESS version %d.%d  %s  module %s %s\n",
            VMAJOR, VMINOR, __DATE__, sModuleName, sModuleVsn);
# else
    fprintf(LogFilePtr,"\n\nVITESS module %s %s  %s\n", sModuleName, sModuleVsn, __DATE__);
# endif
#else
# ifdef VVERS
    fprintf(LogFilePtr,"\n\nVITESS version %s  module %s %s\n", VVERS, sModuleName, sModuleVsn);
# else
    fprintf(LogFilePtr,"\n\nVITESS module %s %s  %s\n", sModuleName, sModuleVsn, __DATE__);
# endif
#endif
}

void print_module_name(const char *name)
{
  char sNameHlp[41], *pBlank;

#ifdef WIN32
# if defined(VMAJOR) && defined(VMINOR)
    fprintf(LogFilePtr,"\n\nVITESS version %d.%d  %s  module %s\n",
            VMAJOR, VMINOR, __DATE__, name);
# else
    fprintf(LogFilePtr,"\n\nVITESS module %s  %s\n", name, __DATE__);
# endif
#else
# ifdef VVERS
    fprintf(LogFilePtr,"\n\nVITESS version %s  module %s\n", VVERS, name);
# else
    fprintf(LogFilePtr,"\n\nVITESS module %s  %s\n", name, __DATE__);
# endif
#endif

  /* Keeping name in mind (without "Space and" and without version number */
  if (strncmp(name, "Space and ", 10)==0)
    strcpy(sNameHlp, name+10);
  else
    strcpy(sNameHlp, name);

  pBlank=strrchr(sNameHlp, ' ');
  if (pBlank > sNameHlp)
    strncpy(sModuleName, sNameHlp, (int) Min(MOD_NAME_LEN, pBlank-sNameHlp));
  else
    strncpy(sModuleName, sNameHlp, MOD_NAME_LEN);
}

void adjustProgress(int spercent) {
  if (spercent == SourcePercent) return;
  SourcePercent = spercent;
  if (ProgressFile) {
    FILE *fo;
    fo = fopen(ProgressFile, "w");
    fprintf(fo, "%d\n", spercent);
    fclose(fo);
  }
}

static void adjustFileProgress(int rlen) {
  long r;
  // do nothing if we do not read neutrons from file or zcat pipe
  if (SourceSize <= 0) return;
  if (stdin == InputFilePtr)
    r = byte_read_so_far += rlen; // we read from stdin and have to count bytes read so far by our selves
  else
    r = ftell(InputFilePtr);      // we read from file and use ftell to know which part we have done

  adjustProgress((int)(100.0 * r / SourceSize));
}

static int readFromPipe() {
  /* read neutrons from a gzip pipe.
     it seems zcat does not like very big chunks, so we read small chunks
     to an intermediate store, and deliver chunks of sizeof(Neutron) * BufferSize
  */
  char *p;
  int clen, rlen, full_len = sizeof(Neutron) * BufferSize;
  p = (char *)InputNeutrons;

  if ((clen = compressedRestlen) >= 0) {
    // First copy rest data to the buffer begin
    memcpy(InputNeutrons, compressBuf, compressedRestlen);
    p += compressedRestlen;
    compressedRestlen = 0;
  }

  while (clen < full_len) {
    if (clen + compressBufLen > full_len) {
      if ((rlen = fread(compressBuf, 1, compressBufLen, stdin)) <= 0)
        break;
      clen += rlen;
      if (clen > full_len) {
        compressedRestlen = clen - full_len;
        memcpy(p, compressBuf, rlen - compressedRestlen);
        memmove(compressBuf, compressBuf + (rlen - compressedRestlen), compressedRestlen);
        clen = full_len;
      } else {
        memcpy(p, compressBuf, rlen);
        p += rlen;
      }
    } else {
      if ((rlen = fread(p, 1, compressBufLen, stdin)) <= 0)
        break;
      clen += rlen;
      p += rlen;
    }
  }
  return clen;
}


/****************************************************************/
/* ReadNeutrons reads BufferSize neutrons from the input stream */
/* to the input neutron Buffer InputNeutrons. The number of     */
/* neutrons read is returned.                                   */
/****************************************************************/
int ReadNeutrons()
{
  long i;   // index of trajectories

  // no treatment of neutrons in preparation run
  if (bTest) return 0;

  if (compressModeR) {

    NumNeutGot = readCompressedNeutrons();

  } else if (compressBuf) {

    // we read data from a zcat pipe, no further vitess decompression
    int copy_len = readFromPipe();
    NumNeutGot = copy_len / sizeof(Neutron);
    adjustFileProgress(copy_len);

  } else {
    // uncompressed neutrons
    NumNeutGot = fread(InputNeutrons, sizeof(Neutron), BufferSize, InputFilePtr);
    adjustFileProgress(NumNeutGot*sizeof(Neutron));
  }

  for(i=0; i<NumNeutGot; i++)
  {
    WriteTraceLine(&InputNeutrons[i]);
    if (IsEOB(&InputNeutrons[i]))
    { NumEobRead++;
    }
    else
    { /* normalization of direction vector for modules representing hardware */
      if (_eModule < MCN_MONITOR1)
        NormVector(InputNeutrons[i].Vector);

      // Check if a neutron with such an ID has been seen before
      // If neutrons with same IDs arriving, shift the ID!
      if (tempID.IDNo != InputNeutrons[i].ID.IDNo || memcmp(tempID.IDGrp, InputNeutrons[i].ID.IDGrp, 2)!=0 )
      {
        WriteIAP(&InputNeutrons[i], VT_ENTERED);
        tempID = InputNeutrons[i].ID;
      }
      else
      {
        ChangeNeutronID(&InputNeutrons[i]);
        WriteIAP(&InputNeutrons[i], VT_ENTERED);
      }
    }
  }

  NumNeutRead += NumNeutGot;
  return NumNeutGot;
}

void ChangeNeutronID(Neutron* n)
{

  tempID = n->ID;
  lastIDShift++;
  // Increase the first letter to indicate the level of cloning of this trajectory
  n->ID.IDGrp[0]++;
  // Change the neutron ID to avoid dublicity with other trajectories
  n->ID.IDNo += lastIDShift;

}


/*******************************************************************/
/* WriteNeutron writes a neutron to the neutron ouput buffer       */
/* OuputNeutrons and flushes the buffer to the output file if the  */
/* buffer is full. Output Neutrons contains BufferSize entries.    */
/*******************************************************************/
void WriteNeutron(Neutron *OutNeutron)
{
  int    col_write =0;
  double tx = OutNeutron->Probability;
  // some modules may produce unreasonable probabilities
  if (ISNAN(tx) || tx < 0)
  {
    OutNeutron->Probability = 0.0;
  }
  else
  {
    dProbTotal[0] +=    tx;
    dProbQuad     += sq(tx);
    //use colorTB+colorLR in case they are counted separately (by guide)
    col_write=(OutNeutron->Color - OutNeutron->Color%100)  / 100 + (OutNeutron->Color %100);
    if (bSepRate && col_write >= 0 && col_write <= MAX_COL)
      dProbTotal[col_write+1] += tx;
  }

  if (OutputFilePtr)
    CopyNeutron(OutNeutron, OutputNeutrons + OutNeutNum);

  if (++OutNeutNum >= BufferSize)
    OutputBufferFlush(0);  // flush to stream, and give trace marks

  WriteTraceLine(OutNeutron);
  if (IsEOB(OutNeutron))
    NumEobWritten++;
}

void WriteEOB()
{
  Neutron OutEOB;

  InitNeutron(&OutEOB);
  SetEOB(&OutEOB);

  CopyNeutron(&OutEOB, OutputNeutrons + OutNeutNum);
  OutNeutNum++;
  NumEobWritten++;
  OutputBufferFlush(0);  // flush to stream, and give trace marks
}


/**********************************************************************************/
/*  PropagateX()     Propagates neutron to a plane in a distance along the x-axis */
/*  PropagateToF()   Propagates neutron for a given ToF along its flight direction*/
/*                                                                                */
/*  WriteDIAP()      writes interaction point to the traj. file after propagation */
/*  WriteScatIAP()   transfers neutron from 'sample frame' (SF)                   */
/*                    to 'incoming frame' (IF) before writing intersection point  */
/*  WriteIAP()       writes an interaction point to the traj. file if wanted      */
/*  WriteWWP()       writes an interaction point to the trajectory file           */
/**********************************************************************************/

short PropagatePath(Neutron* pNeutron, double* pToF, const double PathLen)
{
  VectorType vPath;
  short      rc=FALSE;

  if (pNeutron->Wavelength > 0.0)
  {
    *pToF = PathLen / V_FROM_LAMBDA(pNeutron->Wavelength);
    pNeutron->Time += *pToF;

    CopyVector(pNeutron->Vector, vPath) ;
    MultiplyByScalar(vPath, PathLen);
    AddVector (pNeutron->Position, vPath) ; /* vPath = displacement vector */
    rc = TRUE;
  }
  return rc;
}

short PropagateX(Neutron* pNeutron, double* pToF, const double DistX)
{
  VectorType vPath;
  double     PathLen=0.0;
  short      rc=FALSE;

  if (pNeutron->Vector[0] != 0.0 && pNeutron->Wavelength > 0.0)
  {
    PathLen = DistX / pNeutron->Vector[0];

    *pToF = PathLen / V_FROM_LAMBDA(pNeutron->Wavelength);
    pNeutron->Time += *pToF;

    CopyVector(pNeutron->Vector, vPath) ;
    MultiplyByScalar(vPath, PathLen);
    AddVector (pNeutron->Position, vPath) ; /* vPath = displacement vector */
    rc = TRUE;
  }
  return rc;
}

void PropagateToF(Neutron* pNeutron, const double ToF)
{
  VectorType vPath;
  double     PathLen=0.0;

  pNeutron->Time += ToF;
  PathLen = ToF * V_FROM_LAMBDA(pNeutron->Wavelength);

  CopyVector(pNeutron->Vector, vPath) ;
  MultiplyByScalar(vPath, PathLen);
  AddVector (pNeutron->Position, vPath) ; /* vPath = displacement vector */
}


void WriteDIAP(Neutron* pNeutron, VtReason eReason, double DistX)
{
  double ToF=0.0;

  if (bVisTraj==TRUE)
  {
    Neutron ScatNeutr;

    CopyNeutron(pNeutron, &ScatNeutr);
    PropagateX(&ScatNeutr, &ToF, DistX);

    WriteWWP(&ScatNeutr, eReason);
  }
}

void WriteScatIAP(Neutron* pNeutrSF, VtReason eReason, double RotMatrixSmpl[3][3], VectorType PosSmpl)
{
  if (bVisTraj==TRUE)
  {
    Neutron NeutrIF;

    CopyNeutron(pNeutrSF, &NeutrIF);

    /* computes neutron variables in the initial frame */
    RotBackVector(RotMatrixSmpl, NeutrIF.Position);
    RotBackVector(RotMatrixSmpl, NeutrIF.Vector);
    RotBackVector(RotMatrixSmpl, NeutrIF.Spin);
    AddVector(NeutrIF.Position, PosSmpl);

    WriteWWP(&NeutrIF, eReason);
  }
}

void WriteIAP(Neutron *pNeutron, VtReason eReason)
{
  if (bVisTraj)
    WriteWWP(pNeutron, eReason);
}

void WriteWWP(Neutron *pNeutron, VtReason eReason)
{
  VtTrajPoint Wwp;
  int         l;
  VectorType  RelPos;
  static int  nTraj=0,
              nCall=0;

  // write header
  if (nCall==0 /* && eReason==VT_CREATED */)
  { fprintf(TrajFilePtr, "# Trajectories\n#\n# units \n#  [m]  position\n# [Ang] lambda\n# [n/s] weight\n#\n");
    fprintf(TrajFilePtr, "#    ID      color  lambda    weight       pos_x      pos_y      pos_z  spin rsn\n");
  }
  nCall++;

  // only a limited number of trajectories will be written
  if (eReason==VT_OUTSIDE || eReason==VT_OUT_OF_WND || eReason==VT_ABSORBED || eReason==VT_DETECTED)
    nTraj++;
  if ((nTraj > MAX_TRAJ/(NThreads+1)) ||  (floor(pNeutron->Color/10000)==1 && eReason!=VT_DETECTED) ) return;


  // Calculate neutron position in the absolute co-ordinate system
  RelPos[0] = pNeutron->Position[0];
  RelPos[1] = pNeutron->Position[1]*BlowUp;
  RelPos[2] = pNeutron->Position[2]*BlowUp;
  // if (bLengthCmpr) RelPos[0] /= CmprFact;
  RotBackVector(RotMatrixS, RelPos);
  for (l=0; l<3; l++)
    Wwp.pos[l] = (BegPosS[l] + RelPos[l])/100.0;    // cm -> m

  Wwp.lambda = (float) pNeutron->Wavelength;
  if (eReason==VT_ABSORBED || eReason==VT_SCATTERED || eReason==VT_REFLECTED || eReason==VT_OUT_OF_WND || eReason==VT_DETECTED)
    Wwp.weight = (float) pNeutron->Probability;
  else
    Wwp.weight = 0.0;
  Wwp.id     = pNeutron->ID;
  Wwp.color  = pNeutron->Color;
  Wwp.reason = eReason;

  if (pNeutron->Spin[0]!=0.0)
  { if (pNeutron->Spin[0] > 0.0) Wwp.spin=SPIN_UP; else Wwp.spin=SPIN_DOWN;
  }
  else if (pNeutron->Spin[1]!=0.0)
  { if (pNeutron->Spin[1] > 0.0) Wwp.spin=SPIN_UP; else Wwp.spin=SPIN_DOWN;
  }
  else if (pNeutron->Spin[2]!=0.0)
  { if (pNeutron->Spin[2] > 0.0) Wwp.spin=SPIN_UP; else Wwp.spin=SPIN_DOWN;
  }
  else
  { Wwp.spin=SPIN_UNDEF;
  }

  fprintf(TrajFilePtr, "%c%c%010lu %5d %8.5f %11.3e %10.5f %10.5f %10.5f  %2d %2d \n",
                       Wwp.id.IDGrp[0], Wwp.id.IDGrp[1], Wwp.id.IDNo, Wwp.color, Wwp.lambda, Wwp.weight, Wwp.pos[0], Wwp.pos[1], Wwp.pos[2], Wwp.spin, Wwp.reason);
}


/**********************************************************************************/
/* 'WriteGeomData()   writes data to draw the instrument                          */
/* 'WriteInstrData()' writes position of each component in a global co-ord system */
/* 'ReadInstrData()'  reads these data        (from instrumne.inf)                */
/* 'WriteSimData()'   writes data that other modules may need                     */
/*                    (meas.time, wavelength, frequency, no. of bunches)          */
/* 'ReadSimData()'    reads these data        (from simulation.inf)               */
/* 'ReadNumBnch()'    reads number of bunches (from simulation.inf)               */
/* 'ReadSimData()'    reads meas.time         (from simulation.inf)               */
/**********************************************************************************/

void WriteGeomData(VectorType vBegPos, double Length)
{
  FILE*      pGeomFile=NULL;
  int        k;
  VectorType vRelPos,                            // position vector in the local co-ordinate system
             vDir,                               // direction vector in the absolute co-ordinate system
   vAbsCntr, vAbsPos1, vAbsPos2, vAbsPos3;       // position vector in the absolute co-ordinate system

  /* the source module opens the file */
  if (stGeometry.eModule == MCN_SOURCE)
  { pGeomFile = OpenOutputFile(pGeomFileName, TRUE, "w");
    if (pGeomFile) {
      DefineColors(pGeomFile);
      fprintf(pGeomFile, "#\n#units \n#  [m]  position, length, width, height, radius\n# [deg] angles\n#\n");
    }
    CopyVector(vNull, vBegPos);
  }
  /* each other module representing hardware appends a line */
  else if (stGeometry.eModule < MCN_MONITOR1)
  { pGeomFile = OpenOutputFile(pGeomFileName, TRUE, "a");
  }
  else
  {
    return;
  }

  if (pGeomFile)
  {
    if (bVisInstalled==TRUE)
    {
      /* Circles */
      for (k=0; k < stGeometry.nCircles; k++)
      {
        const char* sDescr;
        sDescr = "";
        if (k == 0 || k == (stGeometry.nCircles-1) || stGeometry.eModule == MCN_SOURCE ) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsCntr, stGeometry.pCircle[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pCircle[k].vNormal, vNull);

        DrawCircle(pGeomFile, sDescr, vAbsCntr, vDir,
                   stGeometry.pCircle[k].Radius, stGeometry.pCircle[k].AngleBeg, stGeometry.pCircle[k].AngleEnd);
      }

      /* Lines */
      for (k=0; k < stGeometry.nLines; k++)
      {
        const char* sDescr;
        sDescr = "";
        if (k == 0 || k == (stGeometry.nLines-1)) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsPos1, stGeometry.pLine[k].vPosBeg, vBegPos);
        Transform (vAbsPos2, stGeometry.pLine[k].vPosEnd, vBegPos);
        DrawLine(pGeomFile, sDescr, vAbsPos1, vAbsPos2);
      }

      /* Rectangles */
      for (k=0; k < stGeometry.nRectangles; k++)
      {
        const char* sDescr;
        sDescr = "";
        if (k == 0 || k == (stGeometry.nRectangles-1) || stGeometry.eModule == MCN_SOURCE ) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsCntr, stGeometry.pRectangle[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pRectangle[k].vNormal, vNull);

        DrawRectangle(pGeomFile, sDescr, vAbsCntr, vDir,
                      stGeometry.pRectangle[k].Width,
                      stGeometry.pRectangle[k].Height,
                      stGeometry.pRectangle[k].rotAngle);
      }

      /* Triangles */
      for (k=0; k < stGeometry.nTriangles; k++)
      {
        const char* sDescr;
        sDescr = "";
        if (k == 0 || k == (stGeometry.nTriangles-1)) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsPos1, stGeometry.pTriangle[k].vEdges[0], vBegPos);
        Transform (vAbsPos2, stGeometry.pTriangle[k].vEdges[1], vBegPos);
        Transform (vAbsPos3, stGeometry.pTriangle[k].vEdges[2], vBegPos);

        DrawTriangle(pGeomFile, sDescr, vAbsPos1, vAbsPos2, vAbsPos3);
      }


      /* OpenRectangles */
      for (k=0; k < stGeometry.nOpenRects; k++)
      {
        Transform (vAbsCntr, stGeometry.pOpenRect[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pOpenRect[k].vNormal, vNull);

        DrawOpenRect(pGeomFile, stGeometry.pDescr, vAbsCntr, vDir,
                     stGeometry.pOpenRect[k].Width,      stGeometry.pOpenRect[k].Height,
                     stGeometry.pOpenRect[k].InnerWidth, stGeometry.pOpenRect[k].InnerHeight);
      }

      /* Cuboids */
      for (k=0; k < stGeometry.nCuboids; k++)
      {
        const char* sDescr;
        sDescr = "";
        if (k == 0 || k == (stGeometry.nCuboids-1)) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsCntr, stGeometry.pCuboid[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pCuboid[k].vNormal,  vNull);

        DrawCuboid(pGeomFile, sDescr, vAbsCntr, vDir,
                   stGeometry.pCuboid[k].Length, stGeometry.pCuboid[k].Width,
                   stGeometry.pCuboid[k].Height, stGeometry.pCuboid[k].rotAngle);
      }

      /* Prisms */
      for (k = 0; k < stGeometry.nPrisms; k++)
      {
          const char* sDescr;
          sDescr = "";
          if (k == 0 || k == (stGeometry.nPrisms - 1)) sDescr = stGeometry.pDescr;
          else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

          // Transforming the center and orientation vectors
          Transform(vAbsCntr, stGeometry.pPrism[k].vCntr, vBegPos);
          Transform(vDir,     stGeometry.pPrism[k].vNormal, vNull);

          // Drawing the prism with the base vertices and height
          DrawPrism(pGeomFile, sDescr, vAbsCntr, vDir,
                    stGeometry.pPrism[k].vVertices,    // Array of vertices for the triangular faces
                    stGeometry.pPrism[k].PrismHeight);  // Height of the prism (distance between the triangular faces)
      }

      /* Hulls */
      for (k=0; k < stGeometry.nHulls; k++)
      {

        const char* sDescr;
        sDescr = (const char*) strchr(stGeometry.pDescr, ':');
        if (k == 0 || k == (stGeometry.nHulls-1)) sDescr = stGeometry.pDescr;
        else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsCntr, stGeometry.pHull[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pHull[k].vNormal,  vNull);

        DrawHull(pGeomFile, sDescr, vAbsCntr, vDir,
                 stGeometry.pHull[k].Length,
                 stGeometry.pHull[k].WidthIn,  stGeometry.pHull[k].WidthOut,
                 stGeometry.pHull[k].HeightIn, stGeometry.pHull[k].HeightOut, stGeometry.pHull[k].rotAngle);
      }

      /* Cylinders */
      for (k=0; k < stGeometry.nCylinders; k++)
      {
        Transform (vAbsCntr, stGeometry.pCylinder[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pCylinder[k].vSymAxis, vNull);

        DrawCylinder(pGeomFile, stGeometry.pDescr,  vAbsCntr, vDir,
                     stGeometry.pCylinder[k].Length, stGeometry.pCylinder[k].Radius);
      }

      /* Hollow Cylinders */
      for (k=0; k < stGeometry.nHolCyls; k++)
      {
        Transform (vAbsCntr, stGeometry.pHolCyl[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pHolCyl[k].vSymAxis, vNull);

        DrawHolCyl(pGeomFile, stGeometry.pDescr,  vAbsCntr, vDir,
                   stGeometry.pHolCyl[k].Length, stGeometry.pHolCyl[k].Radius, stGeometry.pHolCyl[k].InnerRadius);
      }

      /* Ellipsoids */
      for (k=0; k < stGeometry.nEllipsoids; k++)
      {
        Transform (vAbsCntr, stGeometry.pEllipsoid[k].vCntr,  vBegPos);
        Transform (vDir,     stGeometry.pEllipsoid[k].vSymAxis, vNull);

        DrawEllipsoid(pGeomFile, stGeometry.pDescr, vAbsCntr, vDir,
                                 stGeometry.pEllipsoid[k].Length, stGeometry.pEllipsoid[k].Width,
                                 stGeometry.pEllipsoid[k].Height, stGeometry.pEllipsoid[k].Xlow, stGeometry.pEllipsoid[k].Xhigh);
      }

      /* Spheres */
      for (k=0; k < stGeometry.nSpheres; k++)
      {
        Transform (vAbsCntr, stGeometry.pSphere[k].vCntr, vBegPos);

        DrawSphere(pGeomFile, stGeometry.pDescr,  vAbsCntr, stGeometry.pSphere[k].Radius);
      }

      /* CylSlices */
      for (k=0; k < stGeometry.nCylSlices; k++)
      {
        Transform (vAbsCntr, stGeometry.pCylSlice[k].vCntr,  vBegPos);
        Transform (vDir,     stGeometry.pCylSlice[k].vSymAxis, vNull);

        DrawCylSlice(pGeomFile, stGeometry.pDescr, vAbsCntr, vDir,
                                stGeometry.pCylSlice[k].Radius, stGeometry.pCylSlice[k].Width,
                                stGeometry.pCylSlice[k].Height, stGeometry.pCylSlice[k].Phi+Degrees(RotZ), stGeometry.pCylSlice[k].OpenAngle);
      }
    }
    else if (bVisInstalled==MISSING)
    {
      // if visualisation is not yet implemented draw square or cylinder
      char description[40];
      Transform (vDir, vX, vNull);

      if (Length > 0.0)
      {
        CopyVector      (vX, vRelPos);
        MultiplyByScalar(vRelPos, 0.5*Length);
        Transform (vAbsCntr, vRelPos, vBegPos);
        strcpy(description, sModuleName);
        strncat(description, ":white", 7);
        fprintf(LogFilePtr, "Description for module w/o visualisation: %s \n", description);
        DrawCylinder(pGeomFile, (const char*) description, vAbsCntr, vDir, Length, 5.0);
      } else {
        strcpy(description, sModuleName);
        strncat(description, ":grey", 6);
        fprintf(LogFilePtr, "Description for module w/o visualisation: %s \n", description);
        DrawRectangle(pGeomFile, (const char*) description, vBegPos, vDir, 15.0, 15.0, 0.);
      }
    }

    fclose(pGeomFile);
  }
}


void WriteInstrData(VectorType Pos)
{
  FILE*  pFile=NULL;
  char   *pBuffer;
  long   iModId=iModuleId;
  int    m=0, mFst=0;


  if (nModuleNo==0)  // if 'instrument.inf' does not exist
  {
    // 'source' module writes header with line number '0', 'read_in' with 'iModuleId'=1
    if (_eModule==MCN_SOURCE)
      iModId=0;
    else
      nModuleNo++;
    pFile = OpenOutputFile(sInstrInfOut, FALSE, "w");
    fprintf(pFile,
            "# No ID    module            len [m]    x [m]     y [m]     z [m]     hor. [deg] ver. \n"
            "# ------------------------------------------------------------------------------------\n");
  }
  else if ((InputFilePtr!=NULL && InputFilePtr!=stdin) || _eModule==MCN_READ_IN)
  {
    // first module of 2nd, 3rd ... part copy content from old to new instrument.inf file
    char *inp;
    pBuffer = inp = (char*) malloc(CHAR_BUF_XS*(nModuleNo+3+NUM_EOP));
    pFile = OpenInputFile(sInstrInfIn, FALSE, "r");
    if (pFile)
    {
      if  (eFirstMod==MCN_READ_IN) mFst=1;
      for (m=mFst; m<nModuleNo; m++)
      {
        /* Workaround until actual fix */
        /* read_in does not properly clear the instrument file */
        /* reading too many EOP leads to an overflow */
        if (inp > pBuffer + CHAR_BUF_XS*(nModuleNo+3+NUM_EOP-1)) {
          Warning("Overflow while reading instrument file");
          break;
        }
        if (fgets (inp, CHAR_BUF_XS-1, pFile))
        {
          if (memcmp(inp, "EOP", 3)==0 || memcmp(inp, "#", 1)==0)
            m--;
          inp += CHAR_BUF_XS;
        }
      }
      fclose(pFile);
    }
    pFile = OpenOutputFile(sInstrInfOut, FALSE, "w");
    if (pFile) {
      char *p = pBuffer;
      while (p != inp) {
        fputs(p, pFile);
        p += CHAR_BUF_XS;
      }
      fputs("EOP\n", pFile);
    }
    free(pBuffer);
    pBuffer=0;
  }
  else
  {
    // for each other module: open file to append a line
    pFile = OpenOutputFile(sInstrInfOut, FALSE, "a");
  }

  // each module appends a line
  if (pFile)
  {
    char cNF=' ';
    if (bOldFrame) cNF='F';
    fprintf(pFile, "%3ld %3d %-18.18s %9.5f %9.5f %9.5f %9.5f  %8.3f %8.3f %c\n",
                   iModId, _eModule, sModuleName, BlnLen/100., Pos[0]/100., Pos[1]/100., Pos[2]/100.,
                   180.0/M_PI*RotZ, 180.0/M_PI*RotY, cNF);
    /* mark end of actual part */
    if (OutputFilePtr!=NULL && OutputFilePtr!=stdout && nModuleNo > 0)
      fputs("EOP\n", pFile);
    fclose(pFile);
  }
}

long ReadInstrData(long iModId, VectorType Pos, double* pLength, double* pRotZ, double* pRotY, const char* pInstrFile)
{
  FILE*  pFile=NULL;
  int    nModuleID;
  long   nModNo=0, No=0, nDum;
  char   sBuffer[CHAR_BUF_LENGTH]="", sLine[CHAR_BUF_LENGTH]="";

  nModNo   = 0;
  Pos[0]   = Pos[1] = Pos[2] = 0.0;
  *pLength = 0.0;
  *pRotY   = 0.0;
  *pRotZ   = 0.0;

  if (_eModule==MCN_READ_IN)
    pFile = OpenInputFile (pInstrFile, FALSE, "r");
  else
    pFile = OpenOutputFile(pInstrFile, FALSE, "r");

  if (pFile)
  {
    if (iModId > 0)
    // module ID is given, search for it and return previous line
    { short found=FALSE;
      do
      { ReadLine(pFile, sBuffer, sizeof(sBuffer)-1);
        sscanf(sBuffer, "%ld", &No);
        if (iModId == No)
        {  found = TRUE;
        }
        else
        {  nModNo++;
          strcpy(sLine, sBuffer);
        }
      }
      while (!found && strlen(sBuffer) > 0);

      if (!found)
      { fclose(pFile);
        return -1;
      }

    }
/*    else if ((InputFilePtr!=NULL && InputFilePtr!=stdin) || _eModule==MCN_READ_IN)
    // read until end of previous part, if input file or read_in is used
    {
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
      {
        if (eFirstMod==MCN_COMP_UNKNOWN)
        { eFirstMod=GetModId(sBuffer);
          if (eFirstMod==MCN_READ_IN) nModNo=1;
        }
        if (memcmp(sBuffer, "EOP", 3)==0)
        {  strcpy(sLine, sLineH);
        }
        else
        { nModNo++;
          if (sBuffer[85]!='F' && sBuffer[86]!='F' && sBuffer[87]!='F') strcpy(sLineH, sBuffer);
        }
      }
      if (strlen(sLine)==0)
        strcpy(sLine, sLineH);
    } */
    else
    // otherwise: read last line
    {
      /* Read last line and copy content, except: lines containing F in 87. column, they have not a new frame) */
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
      {
        if (eFirstMod==MCN_COMP_UNKNOWN)
        { eFirstMod=GetModId(sBuffer);
          if (eFirstMod==MCN_READ_IN) nModNo=1;
        }
        if (memcmp(sBuffer, "EOP", 3)!=0)
          nModNo++;
        if (sBuffer[85]!='F' && sBuffer[86]!='F' && sBuffer[87]!='F') strcpy(sLine, sBuffer);
      }

    }

    // extract data from line and change to radians and cm
    sscanf(sLine, "%ld %3d %18c %lf %lf %lf %lf %lf %lf",
                  &nDum, &nModuleID, sBuffer, pLength, &Pos[0], &Pos[1], &Pos[2], pRotZ, pRotY);
    Pos[0]  *= 100.0;
    Pos[1]  *= 100.0;
    Pos[2]  *= 100.0;
    *pLength*= 100.0;
    *pRotZ  *= M_PI/180.0;
    *pRotY  *= M_PI/180.0;
    fclose(pFile);
  }
  return nModNo;
}


void WriteSimData(double dTimeMeas, double dLmbdWant, double dFreq, double nTraj, long  nBunches)
{
  FILE*  pFile;

  pFile = OpenOutputFile("simulation.inf", FALSE, "w");
  if (pFile)
  { fprintf(pFile, "%14.5e   # measuring time     [s]\n", dTimeMeas);
    fprintf(pFile, "%10.5f       # desired wavelength [Ang]\n", dLmbdWant);
    fprintf(pFile, "%10.5f       # source frequency   [Hz]\n", dFreq);
    fprintf(pFile, "%14.5e   # number of trajectories \n", nTraj);
    fprintf(pFile, "%4ld             # number of bunches \n", nBunches);
    fclose(pFile);
  }
}

short ReadSimData(double* pTimeMeas, double* pLmbdWant, double* pFreq, double* pTraj, long* pBunches)
{
  short rc=FALSE;
  FILE* pFile=NULL;
  char  sLine[CHAR_BUF_LENGTH];

  *pFreq = 1.0; *pTimeMeas = 0.0;
  *pTraj = 0.0; *pLmbdWant = 0.0; *pBunches = 1;

  pFile = OpenOutputFile("simulation.inf", FALSE, "r");
  if (pFile)
  {
    if (ReadLine(pFile, sLine, sizeof(sLine)-1)==TRUE) sscanf(sLine, "%le", pTimeMeas);  /* First line  - measuring time */
    if (ReadLine(pFile, sLine, sizeof(sLine)-1)==TRUE) sscanf(sLine, "%lf", pLmbdWant);  /* Second line - desired wavelength */
    if (ReadLine(pFile, sLine, sizeof(sLine)-1)==TRUE) sscanf(sLine, "%lf", pFreq);      /* Third line  - frequency */
    if (ReadLine(pFile, sLine, sizeof(sLine)-1)==TRUE) sscanf(sLine, "%le", pTraj);      /* fourth line - number of trajectories per bunch */
    if (ReadLine(pFile, sLine, sizeof(sLine)-1)==TRUE) sscanf(sLine, "%ld", pBunches);   /* fifth line  - number of bunches */

    fclose(pFile);
    rc=TRUE;
  }

  return rc;
}

long ReadNumBnch(void)
{
  double TimeMeas=0.0,       /* measuring time     (from simulation.inf, not needed) */
         LmbdWant=0.0,       /* desired wavelength (from simulation.inf, not needed) */
         Freq    =0.0,       /* source frequency   (from simulation.inf)   */
         nTraj   =0.0;       /* number of trajectories started per bunch  */
  long   nBnch  = 1;         // number of bunches started

  if (ReadSimData(&TimeMeas, &LmbdWant, &Freq, &nTraj, &nBnch)==FALSE)
    nBnch  = 1;

  return nBnch;
}

double ReadMeasTime(void)
{
  double TimeMeas=60.0,       // measuring time  [s]  */
         LmbdWant= 0.0,       // desired wavelength   */
         Freq    = 0.0,       // source frequency     */
         nTraj   = 0.0;       /* number of trajectories started per bunch  */
  long   nBnch  =  1;         // number of bunches started

  if (ReadSimData(&TimeMeas, &LmbdWant, &Freq, &nTraj, &nBnch)==FALSE || TimeMeas==0.0)
    TimeMeas = 60.0;

  return TimeMeas;
}


void DrawLine(FILE* pGeomFile, const char* pDescr, VectorType vAbsPosB, VectorType vAbsPosE)
{
  fprintf(pGeomFile, "Line           %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
                     vAbsPosB[0]/100.0, vAbsPosB[1]/100.0, vAbsPosB[2]/100.0,
                     vAbsPosE[0]/100.0, vAbsPosE[1]/100.0, vAbsPosE[2]/100.0,  pDescr);
}

void DrawRectangle(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                   double Width, double Height, double rotAngle)
{
  fprintf(pGeomFile, "Rectangle      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,
                     vDir[0], vDir[1], vDir[2],
                     Width/100.0, Height/100.0, rotAngle, pDescr);
}

void DrawTriangle(FILE* pGeomFile, const char* pDescr, VectorType vEdge1, VectorType vEdge2, VectorType vEdge3)
{
  fprintf(pGeomFile, "Triangle       %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
    vEdge1[0]/100.0, vEdge1[1]/100.0, vEdge1[2]/100.0,
    vEdge2[0]/100.0, vEdge2[1]/100.0, vEdge2[2]/100.0,
    vEdge3[0]/100.0, vEdge3[1]/100.0, vEdge3[2]/100.0, pDescr);
}

void DrawOpenRect(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height,
                  double InnerWidth, double InnerHeight)
{
  fprintf(pGeomFile, "OpenRectangle  %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f   %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,
                     vDir[0], vDir[1], vDir[2],
                     Width/100.0, Height/100.0,  InnerWidth/100.0, InnerHeight/100.0,   pDescr);
}

void DrawCircle(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                double Radius, double AngleBeg, double AngleEnd)
{
  fprintf(pGeomFile, "Circle         %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,
                     vDir[0], vDir[1], vDir[2],
                     Radius/100.0, AngleBeg, AngleEnd,  pDescr);
}

void DrawCuboid(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                double Length, double Width, double Height, double rotAngle)
{
  fprintf(pGeomFile, "Cuboid         %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Length/100.0, Width/100.0, Height/100.0, rotAngle, pDescr);
}

void DrawPrism(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
               VectorType vVertices[6], double PrismHeight)
{
  fprintf(pGeomFile, "Prism         %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,
                     vDir[0], vDir[1], vDir[2],
                     vVertices[0][0]/100.0, vVertices[0][1]/100.0, vVertices[0][2]/100.0,
                     vVertices[1][0]/100.0, vVertices[1][1]/100.0, vVertices[1][2]/100.0,
                     vVertices[2][0]/100.0, vVertices[2][1]/100.0, vVertices[2][2]/100.0,
                     vVertices[3][0]/100.0, vVertices[3][1]/100.0, vVertices[3][2]/100.0,
                     vVertices[4][0]/100.0, vVertices[4][1]/100.0, vVertices[4][2]/100.0,
                     vVertices[5][0]/100.0, vVertices[5][1]/100.0, vVertices[5][2]/100.0,
                     PrismHeight/100.0, pDescr);
}

void DrawHull(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
              double Length, double WidthIn, double WidthOut, double HeightIn, double HeightOut, double rotAngle)
{
  fprintf(pGeomFile, "Hull           %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Length/100.0, WidthIn/100.0, WidthOut/100.0,  HeightIn/100.0, HeightOut/100.0, rotAngle, pDescr);
}

void DrawCylinder(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                  const double Len, const double Radius)
{
  fprintf(pGeomFile, "Cylinder       %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Len/100.0, Radius/100.0,   pDescr);
}

void DrawHolCyl(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len,
                const double Radius, const double InnerRadius)
{
  fprintf(pGeomFile, "HollowCylinder %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Len/100.0, Radius/100.0, InnerRadius/Radius,   pDescr);
}

void DrawSphere(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, double Radius)
{
  fprintf(pGeomFile, "Sphere         %10.5f %10.5f %10.5f   %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,
                     Radius/100.0,   pDescr);
}

void DrawEllipsoid(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, double Height, double xLow, double xHigh)
{
  fprintf(pGeomFile, "Ellipsoid      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Length/100.0, Width/100.0, Height/100.0, xLow*2./Length, xHigh*2./Length, pDescr);
}

void DrawCylSlice(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Radius, double Width, double Height, double Phi, double openAngle)
{
  fprintf(pGeomFile, "CylSlice       %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f %10.5f   %s\n",
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Height/100.0,  Radius/100.0, Phi, openAngle, pDescr);
}

void DefineColors(FILE* pGeomFile)
{

  fputs("DEF red=<Material diffuseColor='.9 .01 .01' emissiveColor='.9 .01 .01' transparency='.4'/>\n"
        "DEF green=<Material diffuseColor='.01 .9 .01' emissiveColor='.01 .9 .01' transparency='.4'/>\n"
        "DEF blue=<Material diffuseColor='.01 .01 .9' emissiveColor='.01 .01 .9' transparency='.4'/>\n"
        "DEF yellow=<Material diffuseColor='.9 .6 .01' emissiveColor='.9 .6 .01' transparency='.3'/>\n"
        "DEF orange=<Material diffuseColor='.9 .4 .01' emissiveColor='.9 .4 .01' transparency='.4'/>\n"
        "DEF cyan=<Material diffuseColor='.0 .99 .99' emissiveColor='.0 .99 .99' transparency='.4'/>\n"
        "DEF light_blue=<Material diffuseColor='.6 .99 .99' emissiveColor='.0 .99 .99' transparency='.4'/>\n"
        "DEF magenta=<Material diffuseColor='.9 .01 .6' emissiveColor='.9 .01 .6' transparency='.4'/>\n"
        "DEF grey=<Material diffuseColor='.6 .6 .6' emissiveColor='.6 .6 .6' transparency='.4'/>\n"
        "DEF black=<Material diffuseColor='.01 .01 .01' emissiveColor='.01 .01 .01' transparency='.4'/>\n"
        "DEF white=<Material diffuseColor='.99 .99 .99' emissiveColor='.99 .99 .99' transparency='.4'/>\n",
        pGeomFile);
}


/****************************************************************/
/* Copy the contents of a structure 'Neutron' or initialze them */
/****************************************************************/
void CopyNeutron(const Neutron *source, Neutron *dest)
{
  memcpy(dest, source, sizeof(Neutron));
}

void InitNeutron(Neutron* pNeut)
{
  int k;

  pNeut->ID.IDGrp[0]='A';
  pNeut->ID.IDGrp[1]='A';
  pNeut->ID.IDNo=0;

  pNeut->Debug='N';
  pNeut->Color= 0;
  pNeut->Time       =0.0;
  pNeut->Wavelength =0.0;
  pNeut->Probability=0.0;

  for (k=0; k < 3; k++)
  { pNeut->Position[k]=0.0;
    pNeut->Vector[k]  =0.0;
    pNeut->Spin[k]    =0.0;
  }
}


/****************************************************************/
/* Handles the 'EndOfBunch' data set                            */
/****************************************************************/
void  SetEOB(Neutron* pNeut)
{
  pNeut->Debug='B';
}

short IsEOB(Neutron* pNeut)
{
  if (pNeut->Debug=='B')
    return TRUE;
  else
    return FALSE;
}

short CheckEOB(Neutron* pNeut)
{
  short rc=IsEOB(pNeut);

  if (rc)
  {
     NumEobRead++;
     WriteNeutron(pNeut);
  }
  return rc;
}


void  SetReset(ParChange* pChange)
{
  pChange->cChange='R';
}

short IsReset(const ParChange* pChange)
{
  if (pChange->cChange=='R')
    return TRUE;
  else
    return FALSE;
}


double GetTotInt(short iCol)
{
  return dProbTotal[iCol+1];
}


void OutputBufferFlush(int final)
{
  if (OutputFilePtr) {
    if (compressModeW == 0)
      fwritePar(OutputNeutrons, sizeof(Neutron), OutNeutNum, OutputFilePtr, final);
    else
      writeCompressed();
  }
  NumNeutWritten += OutNeutNum;
  OutNeutNum = 0;
  if (TracePoints) fprintf(LogFilePtr,".");
}


void setDetachedWrite()
{
  initParWrite(sizeof(Neutron), BufferSize);
}


/**************************************************************/
/* LOCAL FUNCTIONS                                            */
/**************************************************************/
/******************************************************************************************************/
/* Sets installation, parameter, input and output directory in correct form for the operating systme  */
/******************************************************************************************************/
static void setInstallDirectory (char *arg)
{
  // We need the InstallDir path for implicitly referenced data files.
  // For gridrun we take this from the VITESSROOT environment variable.
  // Normally we use the executable path of the module, which  contains the installation path;
  // We assume it to be that string part before MODULES .
  char *mp;
#ifndef WIN32
  // gridrun works with unix only
  char *s = getenv("VITESSROOT");
  if (s) {
    InstallDir = strdup(s);
    InstallDirLength = strlen(s);
    return;
  }
#endif
  mp = strstr(arg, "MODULES");
  if (! mp) mp = strstr(arg, "Debug");
  // if (! mp && strncmp(arg, "V:\\", 3)==0) mp=arg+3;    // Path abbreviation in Windows
  // if (! mp && strncmp(arg, "$V/",  3)==0) mp=arg+3;    // Path abbreviation in Linux
  if (! mp && arg[1]==':' && arg[2]=='\\') mp=arg+3;      // Path abbreviation in Windows
  if (! mp) return;
  InstallDirLength = (int)((long) mp - (long) arg);
  /* for pointers too big for long integers: */
  if (InstallDirLength < 0) InstallDirLength = -InstallDirLength;
  InstallDir = (char *) malloc(InstallDirLength + 1);
  memcpy(InstallDir, arg, InstallDirLength);
  InstallDir[InstallDirLength] = 0;
}

static char* setDir (char *arg)
{
  char* pDir=NULL;
  int len= strlen(arg);

  if (len > 0)
  {
    pDir = (char *) malloc(len+2);
    strcpy(pDir, arg);
    ChangeSlash(pDir);
    AddSlash(pDir);
  }
  return pDir;
}

/****************************************************************/
/* combines path to the directory with sub-directory and file   */
/*   (conCat shall be replaced by TotalPath in the future)      */
/****************************************************************/
static char* conCat (const char *sFile, const char* sSubDir, VtDirType sel)
{
  char *pResult=NULL;
  char *pDir=NULL;
  int  LenD=0, LenF=0, LenS=0;

  // no file, no full file name
  if (sFile == NULL)
    return NULL;


  /* Do not change an absolute path. */
#ifdef _MSC_VER
  /* we consider a filename with : as absolute */
  // if (strstr(sFile, ":")) sel = -1;
  if (sFile[1] == ':') sel = -1;
#else
  if (sFile[0] == '/') sel = -1;
#endif

  switch (sel)
  { case PAR_DIR  : pDir = ParDir;     LenD = ParDirLength;      break;
    case INSTL_DIR: pDir = InstallDir; LenD = InstallDirLength;  break;
    case IN_DIR   : pDir = InputDir;  if (InputDir !=NULL) LenD = strlen(InputDir);  break;
    case OUT_DIR  : pDir = OutputDir; if (OutputDir!=NULL) LenD = strlen(OutputDir); break;
    default: LenD = 0;
  }

  LenF = strlen(sFile);
  LenS = strlen(sSubDir);

  // allocate memory and copy all parts to the string
  if ((pResult = (char *) malloc(LenD+LenF+LenS+3)))
  {
    // set path
    if (LenD > 0)
    { memcpy(pResult, pDir, LenD);
      if (LenS > 0)
        strcpy(pResult+LenD, sSubDir);
      else
        pResult[LenD] = '\0';
    }
    else
    {  strcpy(pResult, sSubDir);
    }
    ChangeSlash(pResult);

    // add missing slash and file name
    if (LenS > 0 && sSubDir[LenS-1]!=cSlash)
      AddSlash(pResult);

    strcat(pResult, sFile);
  }

  return pResult;
}

void TotalPath(char* pPath, const char *sFile, const char* sSubDir, VtDirType sel)
{
  pPath[0] = '\0';

  switch (sel)
  {
    case PAR_DIR  : if (ParDir    != NULL) strcpy(pPath, ParDir);     break;
    case INSTL_DIR: if (InstallDir!= NULL) strcpy(pPath, InstallDir); break;
    case IN_DIR   : if (InputDir  != NULL) strcpy(pPath, InputDir);   break;
    case OUT_DIR  : if (OutputDir != NULL) strcpy(pPath, OutputDir);  break;
  }

  AddSlash(pPath);
  strcat(pPath, sSubDir);
  AddSlash(pPath);
  strcat(pPath, sFile);

  ChangeSlash(pPath);
}


/**********************************************************************/
/* returns module ID from a string (usually a line in instrument.inf  */
/**********************************************************************/
static McCompID GetModId(char* sBuffer)
{
  long nDum;
  McCompID eModule;

  sscanf(sBuffer, "%ld %3d", &nDum, &eModule);

  return eModule;
}


/***********************************************************************************************/
/* Transform Vector from local co-ordinate system of the module to absolute co-ordinate system */
/***********************************************************************************************/
static void Transform(VectorType vAbsVec, const VectorType vRelVec, const VectorType vBegVec)
{
  VectorType   Vec;

  CopyVector   (vRelVec, Vec);
  RotBackVector(RotMatrixM, Vec);
  AddVector    (Vec, vBegVec);
  CopyVector   (Vec, vAbsVec);
}

#define HULK(a,b,s) memcpy((char*)(a), (char*)(b), s); wlen += s; a += s
static void writeCompressed() {

  Neutron *pn = OutputNeutrons;
  char *newspin,
    *op = (char *) OutputNeutrons;   // will be overwritten
  int dirbit, i, spincode, ineut;
  double dx,dy,dz, tarr[8];
  int wlen = 0;

  for (ineut = OutNeutNum; ineut > 0; ineut--) {
    double factor;
    int *iop;
    // copy data to prevent unwanted overwriting
    memcpy((char*) tarr, (char*) &pn->Time, 8*sizeof(double));

    // normalize direction
    dx = tarr[6]; dy = tarr[7]; dz = pn->Vector[2];
    factor = 1.0 / sqrt(dx*dx + dy*dy + dz*dz);
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
    iop = (int *) op;
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

  newread = 0;

  rlen = compressedRestlen;
  if (zcat_p) {
    // we have read the first bytes from zcat already
    p = zcat_p;
    zcat_p = 0;
    adjustFileProgress(rlen);
  } else {
    if (! compressBuf) {
      setCompressBufLen();
      compressBuf = (char*) malloc(compressBufLen);
    }
    p = readp = compressBuf;
    toread = compressBufLen;
    if (rlen > 0) {
      // do not overwrite saved rest, but add fresh input
      readp += rlen;
      toread -= rlen;
    }
    if (toread > 0) {
      newread = fread(readp, 1, toread, InputFilePtr);
      if (rlen <= 0 && newread <= 0)
        return 0;
      if (newread > 0) {
        rlen += newread;
        adjustFileProgress(newread);
      }
    }
  }
  compressedRestlen = 0;
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
      return 0;     /* bad data */
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
      for (i=0; i<6; i++)
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
    memmove(compressBuf, p, rlen);
  }
  return ngot;
}
#undef GULP

/* Writing one line into the trace file */
static void   WriteTraceLine(Neutron* pNeutron)
{
  if (bTrace && pNeutron->Debug=='T')
  {
    char   sFileName[21]="";
    short  nModNr=1, nLns;
    FILE*  pFile;

    /* Generating file name from ID and add new line */
    sprintf(sFileName, "Trc%c%c%09lu.dat", pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo);
    if (strcmp(sModuleName, "Source and Window")==0)
    { pFile = OpenOutputFile(sFileName, FALSE, "w");
      fprintf(pFile,"no     module           Trc color   TOF    lambda   count rate    "
                    "pos_x    pos_y    pos_z     dir_x     dir_y     dir_z     sp_x sp_y sp_z\n");
    }
    else
    { pFile = OpenOutputFile(sFileName, FALSE, "r+");
      if (pFile != NULL)
      { nLns   = (short) LinesInFile(pFile);
        nModNr = (short) (nLns/2 + 1);
        fseek(pFile, 0, 2);     // setting pointer to end of file
      }
      else
      { pFile = OpenOutputFile(sFileName, FALSE, "a");
      }
    }
    if (pFile != NULL)
    {
      fprintf(pFile, "%2d %-20.20s:",  nModNr, sModuleName);
      fprintf(pFile," %c %5d  %7.3f %8.5f %11.3e  %8.4f %8.4f %8.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n",
              pNeutron->Debug,       pNeutron->Color,
              pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
              pNeutron->Position[0], pNeutron->Position[1], pNeutron->Position[2],
              pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
              pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]);
      fclose (pFile);
    }
  }
}


/* Returns maximum color with intensity > 0 */
static short GetColMax()
{
  short iMax=0, iCol=0;

  for (iCol=1; iCol <= MAX_COL; iCol++)
    if (GetTotInt(iCol) > 0.0)
      iMax=iCol;

  return iMax;
}
