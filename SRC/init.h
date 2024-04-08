#ifndef INIT_H
#define INIT_H

#include <stdio.h>

#include "general.h"

// #define WriteIAP(a,b) {if(bVisTraj)WriteWWP(a,b);}

// maximum number of helper threads
#define MAXWORKER 32
#define MOD_NAME_LEN  20   // length of module name

#ifdef __cplusplus
extern "C" {
#endif

extern char*         sInstrInfIn;    /* instrument file that is read (default 'instrument.inf') */

extern McCompID      _eModule;       /* ID of the module                */
extern double        BlowUp;         /* Factor, by which the module length is compressed in the visualization, if bLengthCmpr=TRUE */  
extern long          BufferSize;     /* size of the neutron input and ouput buffer */
extern Neutron*      InputNeutrons;  /* input neutron Buffer */
extern Neutron*      OutputNeutrons; /* output neutron buffer */
extern long          OutNeutPtr;     /* points to the next free position in OutputNeutrons */
extern long          CompressedSize; /* if > 0, set for 2. module to indicate size of file gzipped by 1. module */
extern VtModGeom     stGeometry;    /* data needed to draw a picture of the component represented by the module */
extern unsigned long VRandomSeed;    /* random seed, default 0, set by --Z */
extern long          NumNeutGot;     /* number of neutrons read in the current batch */
extern double        NumNeutRead;    /* number of neutrons read in total */
extern double        NumNeutWritten; /* number of neutrons written in total */
extern long          NumEobRead;     /* number of 'EndOfBunch' data sets read in total */
extern long          NumEobWritten;  /* number of trajectories written in total */

extern FILE*         InputFilePtr;   /* stream from which the neutrons are read */
extern FILE*         OutputFilePtr;  /* stream to which the neutrons are written */
extern FILE*         LogFilePtr;     /* stream to which things are logged */
extern char*         InputFileName;  /* file to read neutrons */
extern char*         OutputFileName; /* file to write neutrons */
extern char*         LogFileName;    /* log filename  */
extern char*         ParDirectory;   /* parameter directory */
extern char          sModuleName[MOD_NAME_LEN+1],  /* name of the module        */
                     sVisDescrpt[MOD_NAME_LEN+9];  /* name in the visualization */

extern double        wei_min;        /* Minimal weight for tracing neutron */
extern long          keygrav;
extern long          idum;           /* random number specific */
extern short         bOldFrame,      /* criterion: new co-ordinate system set for current module */
                     bTest,          /* criterion: test run (without trajectories)   */
                     bVisInstalled,  /* criterion: visualization routines installed  */
                     bBlowUp,        /* criterion: width and height extended by factor 'BlowUp' in visualization  */
                     bVisTraj,       /* criterion: visualization of trajectories     */
                     bVisInstr;      /* criterion: instrument visualization          */

extern int           NThreads;       /* number of helper threads for execution, set by --T */
extern double        RotMatrixM[3][3];
extern double        RotMatrixMX[3][3];

FILE* OpenOutputFile  (const char *sName, short bErrMsg, const char* sMode);              // opens file in the output folder with or without error message
FILE* OpenInputFile   (const char *sName, short bErrMsg, const char* sMode);              // opens file in the input folder with or without error message
FILE* OpenInputFile2  (const char *sFilename, const char* sContent, const char* sMode);   // opens file in the input folder with extended error message   
FILE* OpenPackInpFile (const char *sFilename, const char* sPath, short bErrMsg);          // opens input file from the installation directory

void Init             (int argc, char **argv, const McCompID eModule);
void Cleanup          (double dShiftX, double dShiftY, double dShiftZ,
                       double dHorizAngle, double dVertAngle);
void print_module_name(const char *name);
void PrintModuleName  (const McCompID eModule, const char* sVsn);
void adjustProgress   (int spercent);
int  ReadNeutrons     ();
void WriteNeutron     (Neutron* OutNeutron);
void WriteEOB         ();
void ChangeNeutronID  (Neutron* n);

short PropagateX      (Neutron* pNeutron, double DistX);                                                // Propagates the neutron to a plane in a certain distance along the x-axis
void  WriteDIAP       (Neutron* pNeutron, VtReason eReason, double DistX);                              // Propagates the neutron by DelX before writing interaction ppoint for visualization
void  WriteScatIAP    (Neutron* pNeutrSF, VtReason eReason, double RotMatrixSF[3][3], VectorType PosS); // Transfers neutron from 'sample frame' (SF) back to 'incoming frame' (IF) before writing intersection point
void  WriteIAP        (Neutron* pNeutron, VtReason eReason);                                            // Writes interaction point if 'trajectory visualization' is chosen
void  WriteWWP        (Neutron* pNeutron, VtReason eReason);                                            // Writes interaction point

void  WriteInstrData  (VectorType EndPos);
long  ReadInstrData   (long    iModuleNo, VectorType EndPos, double* pLength, double* pRotZ, double* pRotY, const char* pInstrFile);
void  WriteSimData    (double  dTimeMeas, double dLmbdWant,  double  dFreq, double  nTraj, long  nBunches);
short ReadSimData     (double* pTimeMeas, double* pLmbdWant, double* pFreq, double* pTraj, long* pBunches);
void  WriteGeomData   (VectorType vBegPos, double Length);
long  ReadNumBnch     (void);
double ReadMeasTime   (void);

void DefineColors     (FILE* pGeomFile);
void DrawLine         (FILE* pGeomFile, const char* pDescr, VectorType RelPosB,  VectorType RelPosE);
void DrawRectangle    (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height, double rotAngle);
void DrawTriangle     (FILE* pGeomFile, const char* pDescr, VectorType vEdge1,   VectorType vEdge2, VectorType vEdge3);
void DrawOpenRect     (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height, 
                       double InnerWidth, double InnerHeight);
void DrawCircle       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Radius, double AngleBeg, double AngleEnd);
void DrawCuboid       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, double Height, double rotAngle); 
void DrawHull         (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, 
                       double WidthIn,  double WidthOut, double HeightIn, double HeightOut, double rotAngle); 
void DrawCylinder     (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len, const double Radius);
void DrawHolCyl       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len, const double Radius, const double InnerRadius);
void DrawSphere       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, double Radius);
void DrawEllipsoid    (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, double Height, double xLow, double xHigh);
void DrawCylSlice     (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Radius, double Width, double Height, double Phi, double openAngle);

void  CopyNeutron     (const Neutron* source, Neutron* dest);
void  InitNeutron     (Neutron* pNeut);

void  SetEOB          (Neutron* pNeut);
short IsEOB           (Neutron* pNeut);
short CheckEOB        (Neutron* pNeut);

double GetTotInt       (short iMon);
void  OutputBufferFlush(int final);
void  setDetachedWrite ();

#include <gsl/gsl_rng.h>
extern gsl_rng * vit_gsl_rng;

#define myExit(s) {fprintf (LogFilePtr,s); exit(-1);}
#define myExit1(s,a) {fprintf (LogFilePtr,s,a); exit(-1);}
#define myExit2(s,a,b) {fprintf (LogFilePtr,s,a,b); exit(-1);}

#ifdef _MSC_VER
#  if _MSC_VER >= 1700
#    define DODEBMACRO
#  endif
#else
#  define DODEBMACRO
#endif

#ifdef DODEBMACRO
# if DEBUG
#  define DEBUG_OUT(...) {fprintf(LogFilePtr, "%s, line %d :", __FILE__, __LINE__); fprintf(LogFilePtr, __VA_ARGS__); fprintf(LogFilePtr, "\n");}
# else 
#  define DEBUG_OUT(...) (void)0
# endif
# undef DODEBMACRO
#endif

#ifdef __cplusplus
}
#endif

#endif
