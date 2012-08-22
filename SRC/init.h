#ifndef INIT_H
#define INIT_H

#include <stdio.h>

#include "general.h"

#define WriteIAP(a,b) {if(bVisTraj)WriteWWP(a,b);}

// maximum number of helper threads
#define MAXWORKER 32

extern long     BufferSize;     /* size of the neutron input and ouput buffer */
extern Neutron* InputNeutrons;  /* input neutron Buffer */
extern Neutron* OutputNeutrons; /* output neutron buffer */
extern long     OutNeutPtr;     /* points to the next free position in OutputNeutrons */
extern long     CompressedSize; /* if > 0, set for 2. module to indicate size of file gzipped by 1. module */
extern ModProp  stPicture;      /* additional information for 'instrument.inf' */
extern VtModGeom stGeometry;    /* data needed to draw a picture of the component represented by the module */

extern long     NumNeutGot;     /* number of neutrons read in the current batch */
extern double   NumNeutRead;    /* number of neutrons read in total */
extern double   NumNeutWritten; /* number of neutrons written in total */

extern FILE*    InputFilePtr;   /* stream from which the neutrons are read */
extern FILE*    OutputFilePtr;  /* stream to which the neutrons are written */
extern FILE*    LogFilePtr;     /* stream to which things are logged */
extern char*    InputFileName;  /* file to read neutrons */
extern char*    OutputFileName; /* file to write neutrons */
extern char*    LogFileName;    /* log filename  */
extern char*    ParDirectory;   /* parameter directory */

extern double   wei_min;        /* Minimal weight for tracing neutron */
extern long     keygrav;
extern long     idum;           /* random number specific */
extern short    bOldFrame,      /* criterion: new co-ordinate system set for current module */
                bTest,          /* criterion: test run (without trajectories)   */
                bVisInstalled,  /* criterion: visualization routines installed  */
                bVisTraj,       /* criterion: instrument visualization          */
                bVisInstr;      /* criterion: visualization of trajectories     */

extern int      NThreads;       /* number of helper threads for execution, set by --T */

void Init             (int argc, char **argv, VtModID eModule);
void Cleanup          (double dShiftX, double dShiftY, double dShiftZ,
                       double dHorizAngle, double dVertAngle);
void print_module_name(const char *name);
int  ReadNeutrons     ();
void WriteNeutron     (Neutron* OutNeutron);

void WriteWWP(Neutron *pNeutron, VtReason eReason);

void WriteInstrData   (VectorType EndPos);
long ReadInstrData    (long    iModuleNo, VectorType EndPos, double* pLength, double* pRotZ, double* pRotY);
void WriteSimData     (double  dTimeMeas, double dLmbdWant,  double  dFreq);
void ReadSimData      (double* pTimeMeas, double* pLmbdWant, double* pFreq);
void WriteGeomData    (VectorType vBegPos, double Length);

void DrawLine         (FILE* pGeomFile, const char* pDescr, VectorType RelPosB,  VectorType RelPosE);
void DrawRectangle    (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height, double rotAngle);
void DrawTriangle     (FILE* pGeomFile, const char* pDescr, VectorType vEdge1, VectorType vEdge2,
		       VectorType vEdge3);
void DrawOpenRect     (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Width, double Height, 
                       double InnerWidth, double InnerHeight);
void DrawCircle       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Radius, double AngleBeg, double AngleEnd);
void DrawCuboid       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, double Height); 
void DrawHull         (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, 
                       double WidthIn,  double WidthOut, double HeightIn, double HeightOut); 
void DrawCylinder     (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len, const double Radius);
void DrawHolCyl       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, const double Len, 
                       const double Radius, const double InnerRadius);
void DrawSphere       (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, double Radius);
void DrawEllipsoid    (FILE* pGeomFile, const char* pDescr, VectorType vAbsCntr, VectorType vDir, double Length, double Width, 
		       double Height, double xLow, double xHigh);

void CopyNeutron      (Neutron* source, Neutron *dest);
long LinesInFile      (FILE* In);
long ColumnsInFile    (FILE* pFile);
char* FullParName     (const char* filename);
char* FullInstallName (const char* filename, const char* sRelPath);

void setDetachedWrite();

#include <gsl/gsl_rng.h>
extern gsl_rng * vit_gsl_rng;

#define myExit(s) {fprintf (LogFilePtr,s); exit(-1);}
#define myExit1(s,a) {fprintf (LogFilePtr,s,a); exit(-1);}
#define myExit2(s,a,b) {fprintf (LogFilePtr,s,a,b); exit(-1);}

#endif
