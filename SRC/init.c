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
# define cSlash '\\'
# define SET_BINARY_MODE(file) if (_setmode(_fileno(file),O_BINARY) == -1) myExit("Can't set binary mode\n");
#else
# define cSlash '/'
# define SET_BINARY_MODE(file)
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "init.h"

#define MAX_COL 6   /* max. number of count rates written separately for different colours
                       0 means no separate rates writable */
#define NUM_EOP 3   /* number of end-of-part lines that can be treated in 'instrument.inf' */
#define MAX_TRAJ 1200

extern FILE* LogFilePtr;   /* pointer to the log file stream              */

const char *sInstrumentInf = "instrument.inf";

/**************************************************************/
/* This file contains several global variables which are      */
/* essential to each VITESS program module                    */
/**************************************************************/

long     BufferSize;         /* size of the neutron input and output buffer */
long     CompressedSize;     /* if > 0, set for 2. module to indicate size of file gzipped by 1. module */
int      CompressionMode;    /* if > 0, compression mode 1 (nodebug) 2(float) */
Neutron* InputNeutrons;      /* input neutron Buffer */
Neutron* OutputNeutrons;     /* output neutron buffer */
Neutron *OutNeutronsCopy;    /* if this has been allocated in source.c, it may be used for double buffering */
long     OutNeutNum;         /* number of the next free position in OutputNeutrons */
ModProp  stPicture;          /* additional information for 'instrument.inf' */
VtModGeom stGeometry;        /* data needed to draw a picture of the component represented by the module */

long     NumNeutGot;         /* number of trajectories read in the current batch */
double   NumNeutRead;        /* number of trajectories read in total */
double   NumNeutWritten;     /* number of trajectories written in total */

FILE*    InputFilePtr;       /* stream from which the neutrons are read */
FILE*    OutputFilePtr;      /* stream to which the neutrons are written */
FILE*    TrajFilePtr=NULL;   /* pointer to file into which the interaction points of the trajectories are written */
char*    InputFileName;      /* file to read neutrons */
char*    OutputFileName;     /* file to write neutrons */
char*    LogFileName;        /* log file name  */
const char *pGeomFileName="geometry.inf"; /* name of instrument geometry file */
char*    pTrajFileName=NULL; /* trajectory file name  */
char*    ParDirectory;       /* parameter directory */
char*    InstallDirectory;

char*    ProgressFile;       /* file to write progress in percent*/
int      SourcePercent;      /* quantisized progress so far */
long     SourceSize;         /* input file size */
                          
double   wei_min=0.0;        /* Minimal weight for tracing neutron */
long     keygrav=1;       
short    bTrace=TRUE,        /* criterion: write trace files             */
         bOldFrame=FALSE,    /* criterion: co-ordinate system of prev. module used for current module */
         bSepRate =TRUE,     /* criterion: write separate count rates    */
         bTest    =FALSE,    /* criterion: test run (without trajectories)   */
         bVisInstalled=FALSE,/* criterion: visualization routines installed */
         bVisInstr=TRUE,     /* criterion: instrument visualization      */
         bVisTraj =FALSE;    /* criterion: visualization of trajectories */
double   BlnLen=0.0,         /* [cm] length of beamline from source to origin of this module */
         RotZ=0.0, RotY=0.0, /*      hor. and vert. rotation of the local co-ordinate system relative to the absolute one  */
         RotMatrixM[3][3],            
         RotMatrixS[3][3];   /*      matrix to rotate from abs. co-ordinate system to co-ordinate system of last section   */
long     nModuleNo=0,        /*      number of the previous module, increased in Cleanup() */
         iModuleNo=0,        //      number of this module determined from parameter for log file
         lastIDShift=0;      // Shift of the ID needed for visualisation in case modules create additional trajectories
int      powerIDShift=9;     // Needed to include the module number in the overall neutron ID avoiding dublication 
VectorType vNull={0.0,0.0,0.0},
           vX   ={1.0,0.0,0.0},
           BegPosM={0.0,0.0,0.0}, /* [cm] end position of prev. module = origin of this module in absolute co-ordinate system   */
           BegPosS={0.0,0.0,0.0}; /* [cm] end position of prev. section = origin of this section in absolute co-ordinate system */

TotalID  tempID;
/**************************************************************/
/* static variables                                           */
/**************************************************************/

static long       TracePoints=FALSE;     /* creates dot for every written output buffer if TRUE */
static double     dProbTotal[MAX_COL+1], /* sum of the count rates of all trajectories [n/s]    */
                  dProbQuad;             /* sum of the squares of the count rates of all traj.  */

static char       sModuleName[21];

static int ParDirectoryLength, InstallDirectoryLength;

#define COMPRESSBUFLEN 65536
static int compressModeR, compressModeW, compressBufLen, compressedRestlen, spinVector;
static long byte_read_so_far;
static VectorType spinUp, spinDown, spinUpOut, spinDownOut;
static char *compressBuf, *zcat_p;

int      NThreads;      // number of helper threads for execution, set by --T
unsigned long int VRandomSeed;  // random seed, default 0, set by --Z

/**************************************************************/
/* local functions                                            */
/**************************************************************/

static void  OutputBufferFlush(int final);
static void  WriteTraceLine(Neutron* Neut);
static int   readCompressedNeutrons();
static void  writeCompressed();
static void  Transform(VectorType AbsVec, const VectorType vRelVec, const VectorType vBegVec);
static long  DetModNo(const char* pArg);


/**************************************************************/
/* global functions that are also used in this module         */
/**************************************************************/

void  CopyNeutron    (Neutron* source,   Neutron* dest);
long  LinesInFile    (FILE* In);
char* FullParName    (const char* filename);
char* FullInstallName(const char* fileName, const char* sRelPath);


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

static char *conCat (const char *b, const char* c, int sel) {
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
  if ((res = (char *) malloc(alen+blen+clen+1))) {
    if (alen) {
      memcpy(res, a, alen);
      if (clen)
        strcpy(res+alen, c);
      else
        res[alen] = 0;
    } else
      strcpy(res, c);
    strcat(res, b);
  }
  return res;
}

/* Adding path of parameter directory to file name */
char* FullParName(const char* fileName)
{
  return conCat(fileName, "", 0);
}

/* Adding path of installation directory to file name */
char* FullInstallName(const char* fileName, const char* sRelPath)
{
  return conCat(fileName, sRelPath, 1);
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

  if (!TWdata) {
    nwr = fwrite(d, s, n, f);
    return n == nwr;
  }
  // Wait here, if a threadWriter is occupied by an older write operation
  // by allocating hWriteMutex.
  // The mutex becomes unlocked only after completion of threadWriter.
  rc = WaitForSingleObject( hWriteMutex, INFINITE);

  if (rc)
    fprintf(LogFilePtr,"WaitForSingleObject problem, rc %d\n", rc);
  if (final) {
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


static void setCompressBufLen() {
  int full_len = sizeof(Neutron) * BufferSize;
  compressBufLen = COMPRESSBUFLEN > full_len ? full_len : COMPRESSBUFLEN;
}


/*
*************************************************************

 Init does a general program initialization, which is ok
 for all modules of the VITESS program package.
 A processed option is marked by setting the leading - to +
 Option processed here:
  --B  number of buffer entries
  --c  size        (if the first module is zcat, the second
                    gets the compressed file's size here)
  --C  mode        compression mode 1 (nodebug) or float (2)
  --f  input file name
  --F  output file name
  --G  gravitation
  --J  active trace points
  --L  logfile
  --p  progress file
  --P  parameter directory
  --t  test mode
  --T  number of helper threads for execution
  --U  minimal neutron weight
  --v  visualization output: geometry file
  --V  visualization output: trajectory file
  --Z  random number generator initialization

************************************************************
*/

void Init(int argc, char **argv, VtModID eModule)
{
  char *a, *arg, text[99];
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
  ParDirectory   = NULL;
  BufferSize     = BUFFER_SIZE;
  OutNeutNum     = 0;
  TracePoints    = FALSE;
  for (l=0; l<=MAX_COL; l++)
    dProbTotal[l] = 0.0;

  memset(&stPicture, '\0', sizeof(ModProp));
  stPicture.eModule= eModule;
  stPicture.nNumber= 1L;

  memset(&stGeometry, '\0', sizeof(VtModGeom));
  stGeometry.eModule= eModule;

  setInstallDirectory(*argv++);	// extract installation path from program name

  while ((a = *argv++)) {
    if ('-' != *a || '-' != a[1]) continue; // first two chars must be -
    arg = a + 3;
    switch (a[2]) {

    case 'B':                   // determine the buffer size
      sscanf(arg,"%ld", &BufferSize);
      break;

    case 'c' :
      sscanf(arg,"%ld", &CompressedSize);
      break;
    case 'C' :
      sscanf(arg,"%d", &CompressionMode);
      break;

    case 'f' :			// input file if other than stdin
      marg[0] = arg;
      break;

    case 'F':                   // output file if other than stdout
      marg[1] = arg;
      break;

    case 'G':
      keygrav = atol(arg);      // key for gravity 1 -yes (default), 0 - no
      break;

    case 'J' :
      TracePoints=TRUE;
      break;

    case 'L':                   // output file if other than stderr
      marg[2] = arg;
      iModuleNo = DetModNo(arg);
      break;

    case 'p':                   // progress file
      ProgressFile = arg;
      break;

    case 'P':                   // parameter (= default) directory
      setParDirectory(arg);
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
      TrajFilePtr  = fopen(FullParName(pTrajFileName), "w");
      bVisTraj     = TRUE;
      break;

    case 'Z':                   // init number for the random number generator
#if defined(PENV) || defined(_MSC_VER)
      if (sscanf(arg, "%i", &VRandomSeed)) {
        char buf[24];	
        sprintf(buf, "GSL_RNG_SEED=%d", VRandomSeed);
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

  /* Now we know how to handle filenames, which might correspond to the parameter directory */

  // First of all try to open the logfile, if it has been requested.
  if ((arg = marg[2])) {
    // Do no use FileOpen to avoid recursion, but fopen directly.
    LogFilePtr = fopen((LogFileName = FullParName(arg)), "w");
    if (LogFilePtr == NULL) {
      printf("ERROR: Can't open log file %s (%s)!\n", LogFileName, arg);
      exit (-1);
    }
  }

  // Then we care for input, decide if it is compressed.
  if ((arg = marg[0])) {
    // we got some --f argument
    if (strcmp(arg, "no_file") == 0)
      InputFilePtr = NULL;
    else
      InputFilePtr = fileOpen((InputFileName = FullParName(arg)), "rb");
    if (InputFilePtr) {
      char b[4];
      // Get the file size to enable a progress bar
      if (0 == fseek(InputFilePtr, 0, SEEK_END)) {
	SourceSize = ftell(InputFilePtr);
	rewind(InputFilePtr);
      }
      // Is it compressed ?
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
	rewind(InputFilePtr);
    }
  } else {
    SET_BINARY_MODE(stdin);
    if (CompressedSize) {
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
      
      if (compressModeR) {
	zcat_p += 4;
	compressedRestlen -= 4;
      }
      
      factor = compressModeR == 0 ? 55.0 : compressModeR == 2 ? 29.0 : 45.8;
      SourceSize = (long)(CompressedSize * 100.0 / factor);
    }
  }

  // Here we care for ouput, and if compression is an option.
  if ((arg = marg[1])) {
    if (strcmp(arg,"no_file") == 0)
      OutputFilePtr = NULL;
    else {
      OutputFilePtr = fileOpen((OutputFileName = FullParName(arg)), "wb");
      if (OutputFilePtr) {
	if (CompressionMode)
	  compressModeW = CompressionMode; // it has been stated explicitly
	else if (strstr(OutputFileName, ".float."))
	  compressModeW = 2;               // by filename convention
	else if (strstr(OutputFileName, ".nodebug."))
	  compressModeW = 1;               // by filename convention    

	if (compressModeW == 1)
	  fwrite("cmp1", 1, 4, OutputFilePtr);
	else if (compressModeW == 2)
	  fwrite("cmp2", 1, 4, OutputFilePtr);
	else
	  compressModeW = 0;               // to catch an unknown CompressionMode
      }
    }
  } else {
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
  { if (eModule!=VT_SOURCE)
    { nModuleNo=ReadInstrData(iModuleNo, BegPosM, &BlnLen, &RotZ, &RotY);
      if (nModuleNo!=(iModuleNo-1))
      { sprintf(text, "Module %ld could not be found in 'instrument.inf'", iModuleNo);
        Error(text);
      }
	  else
	  {  nModuleNo = iModuleNo;
	  }
      CopyVector(BegPosM, BegPosS);
    }
    else
    { nModuleNo=1;
      BegPosM[0]=BegPosM[1]=BegPosM[2]=0.0;
      BegPosS[0]=BegPosS[1]=BegPosS[2]=0.0;
      BlnLen=0.0;
      RotY  = RotZ = 0.0;
    }
    FillRMatrixZY(RotMatrixM, RotY, RotZ);
    FillRMatrixZY(RotMatrixS, RotY, RotZ);

    /* Determine new ID scheme for neutrons cloned in the last module */
    if (nModuleNo > 99) powerIDShift -= 2;
    else if (nModuleNo > 9 && nModuleNo < 100) powerIDShift -= 1;
    lastIDShift = nModuleNo*pow(10, powerIDShift);
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
  double dTimeMeas=0.0, dLmbdWant=0.0, dFreq=0.0, nNoNeutrons,
	       dCntRateErr;
  int    l;
  VectorType Shift,  /* Shift of end position        [m] */
             EndPos; /* end position of this module  [m] */

  /* update 'instrument.inf' */
  if (!bVisTraj)
  { if (stPicture.eModule == VT_SOURCE) 
    { nModuleNo=0;
      BegPosM[0]=BegPosM[1]=BegPosM[2]=0.0;
      BlnLen=0.0;
      RotY  = RotZ = 0.0;
    } 
    else 
    { if (bTest) Wait(0.75*iModuleNo);
      nModuleNo = ReadInstrData(0, BegPosM, &BlnLen, &RotZ, &RotY);
    }

    FillRMatrixZY(RotMatrixM, RotY, RotZ);

    ReadSimData  (&dTimeMeas, &dLmbdWant, &dFreq);
    nModuleNo++;
    Shift[0]= dShiftX;
    Shift[1]= dShiftY;
    Shift[2]= dShiftZ;
    RotBackVector(RotMatrixM, Shift);
    for (l=0; l<3; l++)
      EndPos[l] = BegPosM[l] + Shift[l];
    BlnLen += LengthVector(Shift);
    RotZ   += dHorizAngle;
    RotY   += dVertAngle;

    fprintf(LogFilePtr, "writing instr data, module %ld\n", nModuleNo);
    WriteInstrData(EndPos);
  }
  if (bVisInstr) {
    Shift[0]= dShiftX;
    Shift[1]= dShiftY;
    Shift[2]= dShiftZ;
    WriteGeomData(BegPosM, LengthVector(Shift));
  }
    

  /* flush the output buffer and close the input and output file */
  OutputBufferFlush(1);
  if(InputFileName)
    fclose(InputFilePtr);
  if(OutputFilePtr && OutputFilePtr != stdout)
    fclose(OutputFilePtr);

#ifdef REALLY_FREE_THINGS_THE_OS_KILLS_ELSE
  /* release the buffer memory */
  free(InputNeutrons);
  free(OutputNeutrons);

  /* free GNU gsl rng state var */
  gsl_rng_free (vit_gsl_rng);
#endif

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
  if (TrajFilePtr) fclose(TrajFilePtr);
}


/***********************************************************************/
/* 'print_module_name' writes the name (and version) of a module to    */
/*                     the LogFile                                     */
/***********************************************************************/

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
    strncpy(sModuleName, sNameHlp, (int) Min(20, pBlank-sNameHlp));
  else
    strncpy(sModuleName, sNameHlp, 20);
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
    
    if (stPicture.eModule < VT_MONITOR_1)
      NormVector(InputNeutrons[i].Vector);
    else continue;
    // Check if a neutron with such an ID has been here before
    // If neutrons with same IDs arriving, shift the ID!
    if (tempID.IDNo != InputNeutrons[i].ID.IDNo) {
      WriteIAP(&InputNeutrons[i], VT_ENTERED);
      tempID = InputNeutrons[i].ID;
    }
    else {     
      tempID = InputNeutrons[i].ID;
      lastIDShift++;
      // Increase the first letter to indicate the level of cloning of this trajectory
      InputNeutrons[i].ID.IDGrp[0]++;
      // Change the neutron ID to avoid dublicity with other trajectories
      InputNeutrons[i].ID.IDNo += lastIDShift;
      WriteIAP(&InputNeutrons[i], VT_ENTERED);
    }
    /* normalization of direction vector for modules representing hardware */
    
  }

  NumNeutRead += NumNeutGot;
  return NumNeutGot;
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
  if (ISNAN(tx) || tx < 0) {
    OutNeutron->Probability = 0;
  } else {
    dProbTotal[0] +=    tx;
    dProbQuad     += sq(tx);
    //use colorTB+colorLR in case they are counted separately (by guide)
    col_write=(OutNeutron->Color - OutNeutron->Color%100)  / 100 + (OutNeutron->Color %100);
    if (bSepRate && col_write >= 1 && col_write <= MAX_COL)
      dProbTotal[col_write] += tx;
  }

  if (OutputFilePtr)
    CopyNeutron(OutNeutron, OutputNeutrons + OutNeutNum);

  if (++OutNeutNum >= BufferSize)
    OutputBufferFlush(0);  // flush to stream, and give trace marks

  WriteTraceLine(OutNeutron);
}

/**********************************************************************************/
/* 'WriteInstrData()' writes position of each component in a global co-ord system */
/* 'WriteGeomData()   writes data to draw the instrument                          */
/*  WriteWWP()        writes an intersection point to the trajectory file         */
/* 'WriteInstrData()' writes data that other modules may need                     */
/*                    (meas.time, wavelength, frequency)                          */
/* 'ReadInstrData()'  reads these data                                            */
/**********************************************************************************/

void WriteInstrData(VectorType Pos)
{
  FILE*  pFile=NULL;
  char   *pBuffer;

  if (nModuleNo==0) {

    // source module writes header

    pFile = fopen( FullParName(sInstrumentInf), "w");
    fprintf(pFile,
            "# No ID    module            len [m]    x [m]     y [m]     z [m]     hor. [deg] ver. \n"
            "# ------------------------------------------------------------------------------------\n");

  } else if (InputFilePtr!=NULL && InputFilePtr!=stdin) {

    // first module of 2nd, 3rd ... part re-writes file up to end of previous part
  
    char *inp;
    pBuffer = inp = (char*) malloc(CHAR_BUF_SMALL*(nModuleNo+3+NUM_EOP));
    pFile = fopen(FullParName(sInstrumentInf), "r");
    if (pFile) {
      int m;
      for (m=-2; m<nModuleNo; m++) {
        if (fgets (inp, CHAR_BUF_SMALL-1, pFile)) {
          if (memcmp(inp, "EOP", 3)==0)
            fgets (inp, CHAR_BUF_SMALL-1, pFile);
          inp += CHAR_BUF_SMALL;
        }
      }
      fclose(pFile);
    }
    pFile = fopen(FullParName(sInstrumentInf), "w");
    if (pFile) {
      char *p = pBuffer;
      while (p != inp) {
        fputs(p, pFile);
        p += CHAR_BUF_SMALL;
      }
      fputs("EOP\n", pFile);
    }
    free(pBuffer);
    pBuffer=0;

  } else {

    // each other module appends a line
    pFile = fopen(FullParName(sInstrumentInf), "a");

  }

  if (pFile) {
    char cNF=' ';
    if (bOldFrame) cNF='F';
    fprintf(pFile, "%3ld %3d %-18.18s %9.5f %9.5f %9.5f %9.5f  %8.3f %8.3f %c\n",
                   nModuleNo, stPicture.eModule, sModuleName, BlnLen/100., Pos[0]/100., Pos[1]/100., Pos[2]/100.,
                   180.0/M_PI*RotZ, 180.0/M_PI*RotY, cNF);
    /* mark end of actual part */
    if (OutputFilePtr!=NULL && OutputFilePtr!=stdout && nModuleNo > 0)
      fputs("EOP\n", pFile);
    fclose(pFile);
  }
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
  if (nTraj > MAX_TRAJ/(NThreads+1)) return;

  // Calculate neutron position in the absolute co-ordinate system
  for (l=0; l<3; l++)
    RelPos[l] = pNeutron->Position[l];
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


void WriteGeomData(VectorType vBegPos, double Length)
{
  FILE*      pGeomFile=NULL;
  int        k;
  VectorType vRelPos,                            // position vector in the local co-ordinate system
             vDir,                               // direction vector in the absolute co-ordinate system
   vAbsCntr, vAbsPos1, vAbsPos2, vAbsPos3;       // position vector in the absolute co-ordinate system

  /* the source module opens the file */
  if (stGeometry.eModule == VT_SOURCE)
  { pGeomFile = fopen( FullParName(pGeomFileName), "w");
    if (pGeomFile)
      DefineColors(pGeomFile);
      fprintf(pGeomFile, "#\n#units \n#  [m]  position, length, width, height, radius\n# [deg] angels\n#\n"); 
    CopyVector(vNull, vBegPos);
  }
  /* each other module appends a line */
  else
  { pGeomFile = fopen(FullParName(pGeomFileName), "a");
  }

  if (pGeomFile)
  { 
    if (bVisInstalled)
    { 
      /* Circles */
      for (k=0; k < stGeometry.nCircles; k++)
      { 

	const char* sDescr;
	sDescr = "";
	if (k == 0 || k == (stGeometry.nCircles-1) || stGeometry.eModule == VT_SOURCE ) sDescr = stGeometry.pDescr;
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
	if (k == 0 || k == (stGeometry.nRectangles-1) || stGeometry.eModule == VT_SOURCE ) sDescr = stGeometry.pDescr;
	else if (strchr(stGeometry.pDescr, ':')) sDescr = strchr(stGeometry.pDescr, ':');

        Transform (vAbsCntr, stGeometry.pRectangle[k].vCntr, vBegPos);
        Transform (vDir,     stGeometry.pRectangle[k].vNormal, vNull);

        DrawRectangle(pGeomFile, sDescr, vAbsCntr, vDir, 
                      stGeometry.pRectangle[k].Width, stGeometry.pRectangle[k].Height, 
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
		   stGeometry.pCuboid[k].Height,  stGeometry.pCuboid[k].rotAngle); 
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

        DrawHull(pGeomFile, sDescr, vAbsCntr, vDir, stGeometry.pHull[k].Length, 
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
		     stGeometry.pCylSlice[k].Height, stGeometry.pCylSlice[k].Phi, stGeometry.pCylSlice[k].OpenAngle);

      }

    } else {
      // if visualisation is not yet implemented draw square or cylinder
      char description[40];        
      Transform (vDir, vX, vNull);

      if (Length > 0.0) {
        CopyVector      (vX, vRelPos);
        MultiplyByScalar(vRelPos, 0.5*Length);
        Transform (vAbsCntr, vRelPos, vBegPos);
	strcpy(description, sModuleName);
	strncat(description, ":white", 6);
	fprintf(LogFilePtr, "Description for module w/o visualisation: %s \n", description); 
        DrawCylinder(pGeomFile, (const char*) description, vAbsCntr, vDir, Length, 5.0);
      } else {
	strcpy(description, sModuleName);
	strncat(description, ":grey", 5);
	fprintf(LogFilePtr, "Description for module w/o visualisation: %s \n", description); 
	DrawRectangle(pGeomFile, (const char*) description, vBegPos, vDir, 15.0, 15.0, 0.);
      }
    }

    fclose(pGeomFile);
  }
}

long ReadInstrData(long iModuleNo, VectorType Pos, double* pLength, double* pRotZ, double* pRotY)
{
  FILE*  pFile=NULL;
  int    nModuleID;
  long   nModNo=0, No=0, nDum;
  char   sBuffer[CHAR_BUF_LENGTH]="", sLine[CHAR_BUF_LENGTH]="", sLineH[CHAR_BUF_LENGTH]="";

  nModNo   = 0;
  Pos[0]   = Pos[1] = Pos[2] = 0.0;
  *pLength = 0.0;
  *pRotY   = 0.0;
  *pRotZ   = 0.0;

  pFile = fopen(FullParName(sInstrumentInf), "r");

  if (pFile)  
  {
    if (iModuleNo > 0) {
      // module no is given, read its description row
      int found=0;
      while (ReadLine(pFile, sLine, sizeof(sLine)-1))
        if (sscanf(sLine, "%ld", &nModNo)==1 && nModNo==(iModuleNo-1)) 
          {
            found = 1;
            break;
          }
      if (!found) {
        fclose(pFile);
        return 0;
      }
      
    } 
    else if (InputFilePtr==NULL || InputFilePtr==stdin) {

    // otherwise read last line

      /* Read last line and copy content, except: lines containing F in 87. column, they have not a new frame) */
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1)) {
        sscanf(sBuffer, "%ld", &nModNo);
        if (sBuffer[85]!='F' && sBuffer[86]!='F' && sBuffer[87]!='F') strcpy(sLine, sBuffer);
      }

    } else {

      // read until end of previous part, if input file is used
      while (ReadLine(pFile, sBuffer, sizeof(sBuffer)-1))
      { if (memcmp(sBuffer, "EOP", 3)==0)
        {  nModNo = No;
           strcpy(sLine, sLineH);
        }
        else
        { sscanf(sBuffer, "%ld", &No);
          if (sBuffer[85]!='F' && sBuffer[86]!='F' && sBuffer[87]!='F') strcpy(sLineH, sBuffer);
        }
      }
      if (strlen(sLine)==0) {nModNo = No; strcpy(sLine, sLineH);}
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

/*********************************************************************************/
/* Transform Vector from local co-ordinate system of the module                  */
/* to absolute co-ordinate system                                                */
/*********************************************************************************/
static void Transform(VectorType vAbsVec, const VectorType vRelVec, const VectorType vBegVec)
{
  VectorType   Vec;

  CopyVector   (vRelVec, Vec);
  RotBackVector(RotMatrixM, Vec);
  AddVector    (Vec, vBegVec);
  CopyVector   (Vec, vAbsVec);
}



void DrawLine(FILE* pGeomFile, const char* pDescr, VectorType vAbsPosB, VectorType vAbsPosE)
{
  fprintf(pGeomFile, "Line           %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f  %s\n", 
                     vAbsPosB[0]/100.0, vAbsPosB[1]/100.0, vAbsPosB[2]/100.0, 
                     vAbsPosE[0]/100.0, vAbsPosE[1]/100.0, vAbsPosE[2]/100.0,  pDescr);
}   

void DrawRectangle(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                   double Width, double Height, double rotAngle)
{
  fprintf(pGeomFile, "Rectangle      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f  %10.5f %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  
                     vDir[0], vDir[1], vDir[2],
	             Width/100.0, Height/100.0, rotAngle, pDescr);        
}

void DrawTriangle(FILE* pGeomFile, const char* pDescr, VectorType vEdge1, VectorType vEdge2,
                   VectorType vEdge3)
{
  fprintf(pGeomFile, "Triangle      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f %s\n", 
	  vEdge1[0]/100.0, vEdge1[1]/100.0, vEdge1[2]/100.0, 
	  vEdge2[0]/100.0, vEdge2[1]/100.0, vEdge2[2]/100.0,
	  vEdge3[0]/100.0, vEdge3[1]/100.0, vEdge3[2]/100.0, pDescr);        
}

void DrawOpenRect(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height, 
                  double InnerWidth, double InnerHeight)
{
  fprintf(pGeomFile, "OpenRectangle  %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f   %10.5f %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  
                     vDir[0], vDir[1], vDir[2],
                     Width/100.0, Height/100.0,  InnerWidth/100.0, InnerHeight/100.0,   pDescr);        
}
   
void DrawCircle(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                double Radius, double AngleBeg, double AngleEnd)
{
  fprintf(pGeomFile, "Circle         %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  
                     vDir[0], vDir[1], vDir[2],
                     Radius/100.0, AngleBeg, AngleEnd,  pDescr); 
}   

void DrawCuboid(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, 
                double Length, double Width, double Height, double rotAngle)
{
  fprintf(pGeomFile, "Cuboid         %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f %10.5f  %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
	  Length/100.0, Width/100.0, Height/100.0, rotAngle, pDescr);
}

void DrawHull(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, 
              double Length, double WidthIn, double WidthOut, double HeightIn, double HeightOut, double rotAngle)
{
  fprintf(pGeomFile, "Hull           %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
	  Length/100.0, WidthIn/100.0, WidthOut/100.0,  HeightIn/100.0, HeightOut/100.0, rotAngle, pDescr);
}

void DrawCylinder(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir,
                  const double Len, const double Radius)
{
  fprintf(pGeomFile, "Cylinder       %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Len/100.0, Radius/100.0,   pDescr);
}   

void DrawHolCyl(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len, 
                const double Radius, const double InnerRadius)
{
  fprintf(pGeomFile, "HollowCylinder %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
                     Len/100.0, Radius/100.0, InnerRadius/100.0,   pDescr);
}   

void DrawSphere(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, 
                double Radius)
{
  fprintf(pGeomFile, "Sphere         %10.5f %10.5f %10.5f   %10.5f   %s\n", 
                     vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  
                     Radius/100.0,   pDescr);
}   

void DrawEllipsoid(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, double Height, double xLow, double xHigh)
{
  fprintf(pGeomFile, "Ellipsoid      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f %10.5f %10.5f   %s\n", 
	  vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
	  Length/100.0, Width/100.0, Height/100.0, xLow*2./Length, xHigh*2./Length, pDescr);
}

void DrawCylSlice(FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Radius, double Width, double Height, double Phi, double openAngle)
{
  fprintf(pGeomFile, "CylSlice      %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f    %10.5f %10.5f %10.5f %10.5f   %s\n", 
	  vAbsCntr[0]/100.0, vAbsCntr[1]/100.0, vAbsCntr[2]/100.0,  vDir[0], vDir[1], vDir[2],
	  Height/100.0,  Radius/100.0, Phi, openAngle, pDescr);
}

void DefineColors(FILE* pGeomFile)
{

  fprintf(pGeomFile, "DEF red=<Material diffuseColor='.9 .01 .01' emissiveColor='.9 .01 .01' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF green=<Material diffuseColor='.01 .9 .01' emissiveColor='.01 .9 .01' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF blue=<Material diffuseColor='.01 .01 .9' emissiveColor='.01 .01 .9' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF yellow=<Material diffuseColor='.9 .6 .01' emissiveColor='.9 .6 .01' transparency='.3'/> \n");
  fprintf(pGeomFile, "DEF orange=<Material diffuseColor='.9 .4 .01' emissiveColor='.9 .4 .01' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF cyan=<Material diffuseColor='.0 .99 .99' emissiveColor='.0 .99 .99' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF magenta=<Material diffuseColor='.9 .01 .6' emissiveColor='.9 .01 .6' transparency='.4'/> \n");
  fprintf(pGeomFile, "DEF grey=<Material diffuseColor='.6 .6 .6' emissiveColor='.6 .6 .6' transparency='.4'/> \n"); 
  fprintf(pGeomFile, "DEF black=<Material diffuseColor='.01 .01 .01' emissiveColor='.01 .01 .01' transparency='.4'/> \n"); 
  fprintf(pGeomFile, "DEF white=<Material diffuseColor='.99 .99 .99' emissiveColor='.99 .99 .99' transparency='.4'/> \n");

  return;

}


/*************************************************************/
/* Copy the contents of a structure 'Neutron' to another one */
/*************************************************************/

void CopyNeutron(Neutron *source, Neutron *dest)
{
  memcpy(dest, source, sizeof(Neutron));
}


/********************************************************************/
/* counts the number of lines in a text file and rewinds it         */
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

void setDetachedWrite() {
  initParWrite(sizeof(Neutron), BufferSize);
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


static
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
    if (pFile != NULL) {
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

static long DetModNo(const char* pArg)
{
  char* pos;
  long iMod;

  pos = (char*) strrchr(pArg, 'g');
  pos++;
  iMod= atol(pos);

  return iMod;
}
