/*********************************************************************************************/
/*  VITESS module 'read-in'                                                                  */
/*                                                                                           */
/* This module reads neutron events (trajectories) from files in different formats           */
/*   (so it replaces a source module)                                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.9  Jan  2013  K. Lieutenant   initial version                                           */
/* 1.0  Aug  2013  K. Lieutenant   correction read format %09lu -> %lu                       */
/* 1.1  Sep  2013  K. Lieutenant   several input files                                       */
/* 1.1a Apr  2014  K. Lieutenant   repetition corrected                                      */
/* 1.2  Apr  2018  K. Lieutenant   MCPL and MCNPX format                                     */
/* 1.3  May  2019  K. Lieutenant   option to read only trajectories marked for tracing       */
/* 1.3a Jul  2019  K. Lieutenant   MCNPX format uses its own structure                       */
/* 1.3b Jul  2019  K. Lieutenant   smart trajectory search algorithm only for long lists     */
/* 1.4  Feb  2021  K. Lieutenant   options: binary and MCNP6 files                           */
/* 1.5  Feb  2022  K. Lieutenant   correction: binary MCNP6 files and surface file           */
/* 1.6  Aug  2022  P. Zakalek      read in of SSW files from MCNP implemented                */
/* 1.6a Aug  2022  P. Zakalek      removed automatic rotation of MCPL file format            */
/* 1.6b Aug  2022  P. Zakalek      added option to limit number of read neutrons             */
/* 1.6c Feb  2023  K. Lieutenant   'bBlowUp' instead of 'bLengthCmpr'                        */
/* 3.5  Jul  2024  J. Robledo      random sampling feature                                   */
/* 3.7  Apr  2025  N. Schmidt      KDsource functionality                                    */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "mcpl.h"
#include "trace.h"
#include "sswread.h"
#include "random_sampler.h"
#include "kdsource.h"

#define NF_MAX         3
#define MAX_HEADER  3000


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);                                             // Reads input parameters and sets global variables
void  OwnCleanup();                                                                // Does module specific cleanup

short ReadVitessTraj(Neutron* pNeutron, int* nTrj, FILE* pFile);      // Reads VITESS trajectory
short ReadMcStasTraj(Neutron* pNeutron, int* nTrj, FILE* pFile);      // Reads McStas trajectory
short ReadMcplTraj  (Neutron* pNeutron);                              // Reads MCPL trajectory 
short ReadMcnpxTraj (Neutron* pNeutron, int* nTrj, FILE* pFile);      // Reads MCNPX  trajectory
short ReadSSWTraj   (Neutron* pNeutron, int* nTrj, ssw_file_t pFile); // Reads MCNP SSW trajectory
short ReadMcnp6Traj (Neutron* pNeutron, int* nTrj, FILE* pFile);      // Reads MCNP6 trajectory
short ReadKDSTraj   (Neutron* pNeutron, KDSource *pFile, int perturb, double wcrit);             // Reads KDSource file

short ConvertMcStas2Vitess(Neutron* pVitNeut, const McNeutron*       pMcNeut);     // Converts McStas to VITESS trajectory
short ConvertMcpl2Vitess  (Neutron* pVitNeut, const mcpl_particle_t* pMcplPtcl);   // Converts MCPL to VITESS trajectory
short ConvertSSW2Vitess(Neutron* pVitNeutron, const ssw_particle_t * p);           // Converts ssw format of MCNP event files to VITESS trajectory

void  RotMc2Vit  (VectorType* pVitVector, const VectorType* pMcVector);            // Vector transfer from McStas to VITESS co-ordinate system
void  InitMcNeutr(Neutron* pNeutron);                                              // Initializes a trajectory
void  GetId      (TotalID* pID);                                                   // Creates next ID for a trajectory
void  ConvertDate(const char* sDateUS, char* sDateInt);                            // Converts American to international date format

extern char* FullParName(const char* filename);                                    // function in init.c, adds parameter directory to file name


/******************************/
/** Global Variables    **/
/******************************/
// Input parameters
VtPrgFormat  ePrgFormat=VT_VITESS_FMT;   // -f        data format of the program (VT_VITESS_FMT: Vitess   VT_MCSTAS_FMT: McStas   VT_MCPL_FMT: MCPL   VT_MCNPX_FMT and VT_MCNP6_FMT: MCNP)
VtDataFormat eDatFormat=VT_EXPONENTIAL;  // -F        format of the data to read (VT_EXPONENTIAL   VT_FLOAT   VT_BINARY)
char*        sInputFileName[NF_MAX];     // -A -B -D  names of the NF_MAX input files
double       Weight[NF_MAX];             // -a -b -d  Weights of the input files
double       FactInt=1.0;                // -I        Factor to normalize to the source intensity
int          iSurface=MISSING;           // -s        surface ID: if given, only neutrons with this ID are considered
short        iDetectColor=-1;            // -C        Only for VITESS format: Read only events with a given color.
int          nRep=1;                     // -R        Number of times the input is read
double       maxEv=-1;                   // -M        maximal numver of events read
int          sample=0;                   // -J        random sample or not
int          use_kde=1;                  // -K        Where to use KDE or just read the particles from the MCPL file refered in the xml file.

extern char* sInstrInfIn;                // --I       instrument file that is read (default 'instrument.inf')
extern char* _sTraceFileName;            // -T        name of the file containing the trajectories to be traced or started
extern VtTrace _eTraceMode;              // -t        NO_TRACING     : no tracing
                                         //           WRITE_TRC_FILES: write trace files for traj. of interest
                                         //           ONLY_TRC_TRAJ  : simulation only with traj. of interest

// Variables determined from input parameters or trajectory data
FILE*        pInFile[NF_MAX];            //           pointer to input file
mcpl_file_t  hInFile;                    //           handle to MCPL input file
ssw_file_t   hSSWFile;                   //           handle to SSW input file
KDSource*    hKDSFile;                   //           handle to KDSource input file

FILE*        LogFile;                    //           this module needs a separate log file to avoid mixing events with log output

// parameter used in different functions
char         sLine[256]="";              //           one line in an ASCII input file
int          nHeader=0;                  //           length of the header in a binary file
char         sHeader[MAX_HEADER]="";     //           string containing the header


/******************************************/
/**  Main Program                        **/
/******************************************/
int main(int argc, char **argv)
{
  short           m=0,              // index of input files
  iRep=0,                           // counter for number of repetitions
  rc=TRUE;                          // return code of the function reading the input file (TRUE/FALSE)
  int             nT=0;             // number of trajectories identified
  // char            sLine[256]="";    // one line in input file
  Neutron         InNeutron;
  double          w_crit=0.0;       // weight to use KDE while calling KDSource

  // Initialisation
  // --------------
  _eModule=MCN_READ_IN;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.7");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  InitNeutron(&InNeutron);
  NumNeutRead = 0.0;

  // loop over trajectories
  // ----------------------
  if (ePrgFormat==VT_MCPL_FMT)
  {
    if (hInFile.internal)
    {
      iRep=0;
      rc=TRUE;
      while (rc != VT_EOF)
      {
        if ((int)maxEv == -1 || NumNeutRead < maxEv) {
          rc = ReadMcplTraj(&InNeutron);
          if (rc == TRUE) {
            NumNeutRead += rc;
            WriteNeutron(&InNeutron);
          } else if (rc == VT_EOF) {
            iRep++;
            if (iRep < nRep) {
              mcpl_rewind(hInFile);
              rc = TRUE;
            }
          }
        } else {
          rc = VT_EOF;
        }
      }
    }
  } else if (ePrgFormat == VT_SSW_FMT) {
    rc = TRUE;
    while (rc != VT_EOF) {
      rc = ReadSSWTraj(&InNeutron, &nT, hSSWFile);
      if (rc == TRUE){
        int j;
        for (j=0; j<nT; j++) {
          if ((int)maxEv == -1 || NumNeutRead < maxEv) {
            if (j != 0) GetId(&InNeutron.ID);
            NumNeutRead += 1;
            WriteNeutron(&InNeutron);
          } else {
            rc = VT_EOF;
          }
        }
      }
    }
  } else if (ePrgFormat == VT_KDS_FMT) {
    rc = TRUE;
    int NumNeutrFile = hKDSFile->plist->npts;
    w_crit = use_kde ? KDS_w_mean(hKDSFile, 1000, NULL): -1;
    while (rc != VT_EOF && NumNeutRead < NumNeutrFile && NumNeutRead < maxEv) {
      rc =  ReadKDSTraj(&InNeutron, hKDSFile, use_kde, w_crit);
      if (rc == TRUE)
      {
        NumNeutRead += rc;
        WriteNeutron(&InNeutron);
        if (NumNeutRead <= 10)
        fprintf(LogFilePtr, "%i\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\n", InNeutron.ID.IDNo, InNeutron.Wavelength, InNeutron.Position[0], InNeutron.Position[1], InNeutron.Position[2],  InNeutron.Vector[0], InNeutron.Vector[1], InNeutron.Vector[2], InNeutron.Time, InNeutron.Probability);
      }
    }
  }
  else
  { // loop over input files
    if (sample == 0){
      for (m=0; m < NF_MAX; m++)
      {
        if (pInFile[m])
        {
          iRep=0;
          rc=TRUE;
          while (rc==TRUE)
          {
            switch (ePrgFormat)
            {
              case VT_VITESS_FMT: rc=ReadVitessTraj(&InNeutron, &nT, pInFile[m]); break;
              case VT_MCSTAS_FMT: rc=ReadMcStasTraj(&InNeutron, &nT, pInFile[m]); break;
                // case VT_MCNPX_FMT : rc=ReadMcnpxTraj (&InNeutron, &nT, pInFile[m]); break;
              case VT_MCNP6_FMT : rc=ReadMcnp6Traj (&InNeutron, &nT, pInFile[m]); break;
              default: Error("Data format is not (yet) implemented");

            }
            if (nT>=1)
            {
              if ((int)maxEv == -1 || NumNeutRead < maxEv) {
                InNeutron.Probability *= (Weight[m] / nRep);        // reduction of weight if data are read more than once or more than 1 file is read
                NumNeutRead += nT;
                WriteNeutron(&InNeutron);
              } else {
                rc = VT_EOF;
              }
            }

            // if reading was not possible anymore, try to start from beginning if applicable
            if (rc==FALSE)
            { iRep++;
              if (iRep < nRep)
              { rewind(pInFile[m]);
                rc=TRUE;
              }
            }
          }
        }
      }
    }
    else{
      // sample mode
      fprintf(LogFilePtr, "\nEntering read_in sampling mode\n"); 
      FILE* sampleFile = tmpfile(); // generate temporary file in which to store sample.
      randomSampleFile(sInputFileName[0], sampleFile, maxEv); // create and store sample.

      rc = TRUE;
      while (rc == TRUE){
        switch (ePrgFormat){
              case VT_VITESS_FMT: rc = ReadVitessTraj(&InNeutron, &nT, sampleFile); break;
              case VT_MCSTAS_FMT: rc = ReadMcStasTraj(&InNeutron, &nT, sampleFile); break;
              case VT_MCNP6_FMT: rc = ReadMcnp6Traj(&InNeutron, &nT, sampleFile); break;
              default: Error("Data format is not (yet) implemented");
        }
        if (rc == FALSE){
          iRep++;
          if (iRep < nRep){
              rewind(sampleFile);
              rc = TRUE;
          }
        }
        WriteNeutron(&InNeutron);
      }
      fclose(sampleFile);
    }
  }

  // Do module specific cleanups
  OwnCleanup();

  // Do the general cleanup
  Cleanup(0.0,0.0,0.0,0.0,0.0);
  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int i=0,m=0;

  for (m=0; m < NF_MAX; m++)
  {
    if (m==0) Weight [m]=1.0;
    else      Weight [m]=0.0;
    pInFile[m]=NULL;
    sInputFileName[m]=NULL;
  }

  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'A':
          sInputFileName[0] = &argv[i][2];
          break;
        case 'B':
          sInputFileName[1] = &argv[i][2];
          break;
        case 'D':
          sInputFileName[2] = &argv[i][2];
          break;

        case 'a':
          Weight[0] = (double)atof(&argv[i][2]);
          break;
        case 'b':
          Weight[1] = (double)atof(&argv[i][2]);
          break;
        case 'd':
          Weight[2] = (double)atof(&argv[i][2]);
          break;
        case 'I':
          FactInt =   (double)atof(&argv[i][2]);
          break;
        case 't':
          _eTraceMode = (VtTrace) atoi(&argv[i][2]);
          break;
        case 'T':
          _sTraceFileName = &argv[i][2];
          break;
        case 'M':
          maxEv = (double)atof(&argv[i][2]);
          break;
        case 'f':
          ePrgFormat = (VtPrgFormat) atoi(&argv[i][2]);
          break;
        case 'F':
          eDatFormat = (VtDataFormat) atoi(&argv[i][2]);
          break;
        case 's':
          iSurface = atoi(&argv[i][2]);
          break;
        case 'C':
          iDetectColor = (short) atoi(&argv[i][2]);
          break;
        case 'R':
          nRep = atoi(&argv[i][2]);
          break;
        case 'J':
          sample = (VtSampling) atoi(&argv[i][2]);
          break;
        case 'K':
          use_kde = (int)atoi(&argv[i][2]);
          break;
        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  // open input file(s)
  if (ePrgFormat== VT_MCPL_FMT)
  {
    if (sInputFileName[0] != NULL)
    {
      char *sFullInputName = FullParName(sInputFileName[0]);
      hInFile = mcpl_open_file(sFullInputName);
      free(sFullInputName);
      fprintf(LogFilePtr, mcpl_hdr_srcname(hInFile));       // Name of the generating application
    }
    else
    { Error("Input file 1 not given");
    }
    if (sInputFileName[1] != NULL || sInputFileName[2] != NULL)
      Warning("Input file 2 and 3 cannot be treated.");
  }
  else if (ePrgFormat==VT_SSW_FMT){
    hSSWFile = ssw_open_file(sInputFileName[0]);
  }
  else if (ePrgFormat==VT_KDS_FMT){
    hKDSFile = KDS_open(sInputFileName[0]);
  }
  else
  {
    for (m=0; m < NF_MAX; m++)
    { if (sInputFileName[m] != NULL)
      {
        if (eDatFormat==VT_BINARY)
          pInFile[m] = OpenInputFile(sInputFileName[m], TRUE, "rb");
        else
          pInFile[m] = OpenInputFile(sInputFileName[m], TRUE, "rt");

        if (pInFile[m]!=NULL)
          fprintf(LogFilePtr,"Input file %s used with weight %7.5f\n", sInputFileName[m], Weight[m]);
      }
    }
    if (pInFile[0]==NULL && pInFile[1]==NULL && pInFile[2]==NULL)
      Error("At least one ascii input file name is mandatory!");
  }

  // define header in binary files
  if (eDatFormat==VT_BINARY)
  {
    switch (ePrgFormat)
    {
      case VT_VITESS_FMT: nHeader=   0; break;  // value > 0 requires code change in 'ReadVitessTraj'
      case VT_MCSTAS_FMT: nHeader=   0; break;  // value > 0 requires code change in 'ReadMcStasTraj'
      case VT_MCPL_FMT  : nHeader=   0; break;  // no influence, it is handled inside mcpl.c
      case VT_MCNPX_FMT : nHeader=   0; Error  ("Binary input for MCNPX not yet properly implemented"); break;
      case VT_MCNP6_FMT : nHeader=   0; break;
      case VT_SSW_FMT: nHeader= 0; break;
      case VT_KDS_FMT: nHeader= 0; break;
      default: Error("Data format is not (yet) implemented");
    }
  }
  else
  { if (ePrgFormat==VT_MCPL_FMT)
      Note("Input and output of MCPL data is handled via module 'mcpl' which stores data in binary format.\nChoice of ASCII format ignored.");
  }

  if (_eTraceMode==ONLY_TRC_TRAJ && _sTraceFileName!=NULL)
    LoadTraceFile();

  if (_sTraceFileName!=NULL)
    fprintf(LogFilePtr, "trace file used              : %s\n", _sTraceFileName);
}


/******************************************/
/**  Does module specific cleanup        **/
/******************************************/
void OwnCleanup()
{
  int m;

  if (ePrgFormat== VT_MCPL_FMT)
  { // Deallocate memory and release file-handle
    mcpl_close_file(hInFile);
  } else if (ePrgFormat== VT_SSW_FMT){
    ssw_close_file(hSSWFile);
  } else if (ePrgFormat== VT_KDS_FMT){
    KDS_destroy(hKDSFile);
  }
  else
  { // Close all files
    for (m=0; m < NF_MAX; m++)
      if (pInFile[m])
        fclose(pInFile[m]);
  }

  if (_aTrace!=NULL) free(_aTrace);
}


/******************************************/
/**  Reads VITESS trajectory             **/
/******************************************/
short ReadVitessTraj(Neutron* pNeutron, int* nTrj, FILE* pFile)
{
  char* pForm="%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf";
  short rcs,
          rc=FALSE;   // tells if reading has worked

  // initialization
  *nTrj = 0;        // number of loaded and wanted trajectories (0 or 1)
  InitNeutron(pNeutron);

  if (eDatFormat==VT_BINARY)
  {
    *nTrj = fread(pNeutron, sizeof(Neutron), 1, pFile);
  }
  else
  {
    rc = ReadLine(pFile, sLine, sizeof(sLine));
    if (rc==TRUE)
    { rcs = sscanf(sLine, pForm, &pNeutron->ID.IDGrp[0], &pNeutron->ID.IDGrp[1], &pNeutron->ID.IDNo,
                   &pNeutron->Debug,       &pNeutron->Color,
                   &pNeutron->Time,        &pNeutron->Wavelength,  &pNeutron->Probability,
                   &pNeutron->Position[0], &pNeutron->Position[1], &pNeutron->Position[2],
                   &pNeutron->Vector[0],   &pNeutron->Vector[1],   &pNeutron->Vector[2],
                   &pNeutron->Spin[0],     &pNeutron->Spin[1],     &pNeutron->Spin[2]   );
      if (rcs > 10) *nTrj = 1;
    }
  }

  if (*nTrj > 0)
  {
    if (IsEOB(pNeutron))
    {
      NumEobRead++;
    }
    else
    {
      // sets trace state 'Y' or 'N' for 'write trace files'
      pNeutron->Debug = _eTraceMode==WRITE_TRC_FILES ? GetTraceState(pNeutron->ID) : 'N';

      // ignores trajectory if ID is not found in trace file if applicable  or  color is wrong
      if (_eTraceMode==ONLY_TRC_TRAJ && GetTraceState(pNeutron->ID)=='N' || iDetectColor > -1 && pNeutron->Color!=iDetectColor)
        *nTrj=0;
    }
    rc=TRUE;
  }

  return(rc);
}


/******************************************/
/**  Reads McStas trajectory             **/
/******************************************/
short ReadMcStasTraj(Neutron* pNeutron, int* nTrj, FILE* pFile)
{
  McNeutron McNeutr;
  short     rcs=0,
          rc=FALSE;  // tells if reading has worked

  // initialization
  *nTrj = 0;           // number of loaded and wanted trajectories (0 or 1)
  memset(&McNeutr, '\0', sizeof(McNeutron));

  if (eDatFormat==VT_BINARY)
  {
    *nTrj = fread(&McNeutr, sizeof(McNeutron), 1, pFile);
  }
  else
  {
    rc = ReadLine(pFile, sLine, sizeof(sLine));
    rcs = sscanf(sLine, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                 &McNeutr.Weight,
                 &McNeutr.Position[0], &McNeutr.Position[1], &McNeutr.Position[2],
                 &McNeutr.Speed[0],    &McNeutr.Speed[1],    &McNeutr.Speed[2],
                 &McNeutr.Time,
                 &McNeutr.Spin[0],     &McNeutr.Spin[1],     &McNeutr.Spin[2]    );
    if (rcs > 7) *nTrj = 1;
  }

  if (*nTrj > 0)
  {
    rcs= ConvertMcStas2Vitess(pNeutron, &McNeutr);
    if (rcs==FALSE)
      *nTrj=0;

    rc = TRUE;
  }

  return(rc);
}


/******************************************/
/**  Reads MCPL trajectory               **/
/******************************************
** rc: 1: neutron found
**     0: other particle
**    -1: EOF
*******************************************/
short ReadMcplTraj(Neutron* pNeutron)
{
  const mcpl_particle_t* pMcplPtcl;
  short rc=FALSE;

  pMcplPtcl = mcpl_read(hInFile);

  if (pMcplPtcl==NULL)
    rc = VT_EOF;
  else
    rc = ConvertMcpl2Vitess(pNeutron, pMcplPtcl);

  return(rc);
}

short ReadKDSTraj(Neutron* pNeutron, KDSource *pfile, int perturb, double wcrit)
{
    mcpl_particle_t part;
    short rc = FALSE;

    int result = KDS_sample2(pfile, &part, perturb, wcrit, NULL, 1);

    const mcpl_particle_t* pMcplPtcl = (const mcpl_particle_t*)malloc(sizeof(mcpl_particle_t));
    memcpy((void*)pMcplPtcl, (const void*)&part, sizeof(mcpl_particle_t));

    if (&part == NULL){
      rc = VT_EOF;
    }
      
    else{
      rc = ConvertMcpl2Vitess(pNeutron, pMcplPtcl);
    }
    free((void*)pMcplPtcl);

  return(rc);
}

/******************************************/
/**  Reads MCNP trajectory              **/
/******************************************/
short ReadMcnpxTraj(Neutron* pNeutron, int* nTrj, FILE* pFile)
{
  McnpxNeutron McnpNeutr;
  short        rcs=0,
          rc=FALSE;
  static int   i=0;

  // initialization
  *nTrj = 0;           // number of loaded and wanted trajectories (0 or 1)
  memset(&McnpNeutr, '\0', sizeof(McnpxNeutron));

  if (eDatFormat==VT_BINARY)
  {
    while (i < nHeader)
    { sHeader[i] = fgetc(pFile);
      i++;
    }
    *nTrj = fread(&McnpNeutr, sizeof(McnpxNeutron), 1, pFile);
  }
  else
  {
    rc = ReadLine(pFile, sLine, sizeof(sLine));
    rcs= sscanf(sLine, "%le %le %le %le %le %le %le %le %le",
                &McnpNeutr.Position[0], &McnpNeutr.Position[1], &McnpNeutr.Position[2],
                &McnpNeutr.Vector[0],   &McnpNeutr.Vector[1],   &McnpNeutr.Vector[2],
                &McnpNeutr.Energy,      &McnpNeutr.Counts,      &McnpNeutr.Shakes);
    if (rcs > 7) *nTrj = 1;
  }

  if (*nTrj > 0)
  {
    // initialization
    InitMcNeutr(pNeutron);

    CopyVector(McnpNeutr.Position, pNeutron->Position);
    CopyVector(McnpNeutr.Vector,   pNeutron->Vector);

    pNeutron->Wavelength  = LAMBDA_FROM_ENERGY(1.0e+12 * McnpNeutr.Energy); // unit MeV -> microeV,  lambda -> energy
    pNeutron->Probability = McnpNeutr.Counts * FactInt;                     // normalisation counts -> n/s
    pNeutron->Time        = McnpNeutr.Shakes * 1.0e-05;                     // unit  shakes (=1.0e-08 s) -> ms

    rc = TRUE;
  }

  return(rc);
}

short ReadSSWTraj(Neutron* pNeutron, int* nTrj, ssw_file_t  pFile) {

  const ssw_particle_t *p;
  short rc=FALSE;
  *nTrj = 0;
  if ((p = ssw_load_particle(pFile))) {
    if (!p->pdgcode) {
      return FALSE;
    }
    rc = ConvertSSW2Vitess(pNeutron, p);
    if (rc) *nTrj=nRep;
    return rc;
  }

  return VT_EOF;
}

short ReadMcnp6Traj(Neutron* pNeutron, int* nTrj, FILE* pFile)
{
  Mcnp6Neutron McnpNeutr;
  short        eSign=1,             // flight direction 1: forward  -1: backward
  rcs=0, rc=FALSE;
  int          i=0, nBytes=0, nSets=0,
          iNum=0,  iNum2=0,  iNum3=0, iNum4=0,
          iNum5=0, iNum6=0,  iNum7=0,  iNum8=0,
          iNum9=0, iNum10=0, iNum11=0, iNum12=0,
          iNum13=0, iNum14=0;
  char         sTitle[81]="", sName [ 9]="", sCode [ 9]="", sVsn[5]="",
          sDate [ 9]="", sTimeJ[ 9]="", sTimeM[ 9]="", sMuell[8]="", sNix[8]="", sHist[9]="",
          sDateP[11]="", sDateJ[11]="", sDateM[11]="",
          cBlank='\0',   c='\0';
  static short bHeader=FALSE;  // flag: Header treated already

  // initialization
  *nTrj = 0;           // number of loaded and wanted trajectories (0 or 1)
  memset(&McnpNeutr , '\0', sizeof(Mcnp6Neutron));

  if (eDatFormat==VT_BINARY)
  {
    if (bHeader==FALSE)
    { fread(&nBytes,    4, 1, pFile);
      fread(sName,      8, 1, pFile);
      fread(&iNum2,     4, 1, pFile);
      fread(&iNum3,     4, 1, pFile);
      fread(sCode,      8, 1, pFile);
      fread(sVsn,       4, 1, pFile); cBlank=fgetc(pFile);
      fread(sDate,      8, 1, pFile); cBlank=fgetc(pFile); ConvertDate(sDate, sDateP);
      fread(sDate,      8, 1, pFile); cBlank=fgetc(pFile); ConvertDate(sDate, sDateJ);
      fread(sTimeJ,     8, 1, pFile); cBlank=fgetc(pFile); cBlank=fgetc(pFile);
      fread(sDate,      8, 1, pFile); cBlank=fgetc(pFile); ConvertDate(sDate, sDateM);
      fread(sTimeM,     8, 1, pFile); cBlank=fgetc(pFile);
      fread(sTitle,    80, 1, pFile);
      fread(&iNum4,     4, 1, pFile);
      fread(&iNum5,     4, 1, pFile);
      fread(&iNum6,     4, 1, pFile);
      fread(&iNum7,     4, 1, pFile);
      fread(&iNum8,     4, 1, pFile);
      fread(&nSets,     4, 1, pFile);
      fread(&iNum9,     4, 1, pFile);
      fread(&iNum10,    4, 1, pFile);
      fread(&iNum11,    4, 1, pFile);
      fread(&iNum12,    4, 1, pFile);
      fread(&iNum13,    4, 1, pFile);
      fread(&iNum14,    4, 1, pFile);

      // search for beginning of data
      for (i=1; i < 20000; i++)
      {
        c = fgetc(pFile);
        if (c=='X')
        { fread(sMuell, 7, 1, pFile);
          if (memcmp(sMuell, sNix, 7)==0)
            break;
        }
      }

      // read first data set
      if (c=='X')
      {
        fread(sHist+4, 4, 1, pFile);
        memcpy(&McnpNeutr.History, sHist, 8);
        rc = (short) fread(&(McnpNeutr.ID),  sizeof(Mcnp6Neutron) - sizeof(double),  1, pFile);
        for (i=0; i < 4; i++) c = fgetc(pFile);

        bHeader=TRUE;
        fprintf(LogFilePtr, "%s%s simulation %s\nfrom %s %s containng %d data sets\n", sCode, sVsn, sTitle, sDateJ, sTimeJ, nSets);
      }
      else
      { Error("Searching for beginning of data failed");
      }

    }
    else
    {
      fread(&iNum, 4, 1, pFile);
      rc = (short) fread(&McnpNeutr , sizeof(Mcnp6Neutron), 1, pFile);
      for (i=0; i < 4; i++) c = fgetc(pFile);
    }
    if (McnpNeutr.ID!=0.0)
      eSign = McnpNeutr.ID/fabs(McnpNeutr.ID);

    if (rc > 0 && (iSurface==MISSING || iSurface==(int)McnpNeutr.Surface) && fabs(McnpNeutr.ID)==8.0)
      *nTrj = 1;
  }
  else
  {
    rc = ReadLine(pFile, sLine, sizeof(sLine));
    rcs= sscanf(sLine, "%le %le %le %le %le %le %le %le %le %le %le",
                &McnpNeutr.History,     &McnpNeutr.ID,
                &McnpNeutr.Counts,      &McnpNeutr.Energy,      &McnpNeutr.Shakes,
                &McnpNeutr.Position[0], &McnpNeutr.Position[1], &McnpNeutr.Position[2],
                &McnpNeutr.DirX,        &McnpNeutr.DirY,        &McnpNeutr.Surface);
    eSign = McnpNeutr.ID/fabs(McnpNeutr.ID);

    if (rcs > 7 && (iSurface==MISSING || iSurface==(int)McnpNeutr.Surface) && fabs(McnpNeutr.ID)==8.0)
      *nTrj = 1;
  }

  if (*nTrj > 0)
  {
    // initialization and ID for VITESS neutron structure

    InitMcNeutr(pNeutron);

    CopyVector(McnpNeutr.Position, pNeutron->Position);

    pNeutron->Vector[0] = McnpNeutr.DirX;
    pNeutron->Vector[1] = McnpNeutr.DirY;
    pNeutron->Vector[2] = sqrt(1 - sq(McnpNeutr.DirX)  - sq(McnpNeutr.DirY)) * eSign;

    pNeutron->Wavelength  = LAMBDA_FROM_ENERGY(1.0e+12 * McnpNeutr.Energy); // unit MeV -> microeV,  lambda -> energy
    pNeutron->Probability = McnpNeutr.Counts * FactInt;                     // normalisation counts -> n/s
    pNeutron->Time        = McnpNeutr.Shakes * 1.0e-05;                     // unit  shakes (=1.0e-08 s) -> ms

    rc = TRUE;
  }

  return(rc);
}


/**************************************************/
/** Converts McStas or MCPL to VITESS trajectory **/
/**************************************************/
short ConvertSSW2Vitess(Neutron* pVitNeutron, const ssw_particle_t * p)
{
  if ((iSurface==MISSING || iSurface==(int)p->isurf)) {
    if (p->pdgcode == NEUTRON_ID) {
      InitMcNeutr(pVitNeutron);

      pVitNeutron->Position[0] = p->x;
      pVitNeutron->Position[1] = p->y;
      pVitNeutron->Position[2] = p->z;
      pVitNeutron->Vector[0] = p->dirx;
      pVitNeutron->Vector[1] = p->diry;
      pVitNeutron->Vector[2] = p->dirz;
      pVitNeutron->Wavelength = LAMBDA_FROM_ENERGY(1.0e+12 * p->ekin);
      pVitNeutron->Probability = p->weight / nRep;
      pVitNeutron->Time = p->time * 1.0e-05;

      return TRUE;
    }
  }
  return FALSE;
}

short ConvertMcStas2Vitess(Neutron* pVitNeutron, const McNeutron* pMcNeutron)
{
  double  velocity;      // velocity of the neutron  [cm/ms]

  // initialization
  InitMcNeutr(pVitNeutron);

  velocity = 0.1 * sqrt(  sq(pMcNeutron->Speed[0])     // unit m/s -> cm/ms
                          + sq(pMcNeutron->Speed[1])
                          + sq(pMcNeutron->Speed[2]));
  pVitNeutron->Wavelength  = LAMBDA_FROM_V(velocity);
  pVitNeutron->Time        = pMcNeutron->Time*1000.0;  // unit s -> ms
  pVitNeutron->Probability = pMcNeutron->Weight;

  RotMc2Vit(&pVitNeutron->Position, &pMcNeutron->Position);
  RotMc2Vit(&pVitNeutron->Vector,   &pMcNeutron->Speed);
  RotMc2Vit(&pVitNeutron->Spin,     &pMcNeutron->Spin);

  MultiplyByScalar(pVitNeutron->Position, 100.0);      // unit   m -> cm
  NormVector      (pVitNeutron->Vector);               // velocity -> direction

  return(TRUE);
}

short ConvertMcpl2Vitess(Neutron* pVitNeutron, const mcpl_particle_t* pMcplParticle)
{
  if (pMcplParticle->pdgcode==NEUTRON_ID)
  {
    // initialization
    InitMcNeutr(pVitNeutron);

    pVitNeutron->Wavelength  = LAMBDA_FROM_ENERGY(pMcplParticle->ekin*1.0e12);    // MeV -> microeV
    pVitNeutron->Time        = pMcplParticle->time;
    pVitNeutron->Probability = pMcplParticle->weight;

    CopyVector(&pMcplParticle->position, &pVitNeutron->Position);
    CopyVector(&pMcplParticle->direction, &pVitNeutron->Vector);
    CopyVector(&pMcplParticle->polarisation, &pVitNeutron->Spin);

    return(TRUE);
  }
  else
  { return(FALSE);
  }
}


/****************************************************************/
/**  Vector transfer from McStas to VITESS co-ordinate system  **/
/****************************************************************/
void RotMc2Vit(VectorType* pVitVector, const VectorType* pMcVector)
{
  (*pVitVector)[0] = (*pMcVector)[2];
  (*pVitVector)[1] = (*pMcVector)[0];
  (*pVitVector)[2] = (*pMcVector)[1];
}


/******************************************/
/**  Initializes a trajectory            **/
/******************************************/
void InitMcNeutr(Neutron* pNeutron)
{
  InitNeutron(pNeutron);
  GetId(&pNeutron->ID);
  pNeutron->Debug ='N';                             // no debugging
  pNeutron->Color = 0;                              // no color
}


/******************************************/
/**  Creates next ID for a trajectory    **/
/******************************************/
void GetId(TotalID* pID)
{
  static unsigned long ig=0;
  static char          ig1='A', ig2='A';

  if (ig==4294967295U)
  {	ig=0;
    if (ig2=='Z')
    {	ig2='A'; ig1++;
    }
    else
    {	ig2++;
    }
  }
  else
  {	ig++;
  }
  pID->IDGrp[0] = ig1;
  pID->IDGrp[1] = ig2;
  pID->IDNo     = ig;
}


/******************************************************/
/**  Converts American to international date format  **/
/******************************************************/
void ConvertDate(const char* sDateUS, char* sDateInt)
{
  strcpy(sDateInt,"2022-01-31");
  sDateInt[2]=sDateUS[6];
  sDateInt[3]=sDateUS[7];
  sDateInt[5]=sDateUS[0];
  sDateInt[6]=sDateUS[1];
  sDateInt[8]=sDateUS[3];
  sDateInt[9]=sDateUS[4];
}


