#ifndef INIT_H
#define INIT_H

#include <stdio.h>

#include "general.h"

extern long     BufferSize;     /* size of the neutron input and ouput buffer */
extern Neutron* InputNeutrons;  /* input neutron Buffer */
extern Neutron* OutputNeutrons; /* output neutron buffer */
extern long     OutNeutPtr;     /* points to the next free position in OutputNeutrons */
extern ModProp  stPicture;      /* data needed to draw a picture of the component represented by the module */

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
extern short    bOldFrame;      /* criterion: new co-ordinate system set for current module */


void Init             (int argc, char **argv, VtModID eModule);
void Cleanup          (double dShiftX, double dShiftY, double dShiftZ,
                       double dHorizAngle, double dVertAngle);
void print_module_name(char name[]);
int  ReadNeutrons     ();
void WriteNeutron     (Neutron* OutNeutron);
void WriteInstrData   (long    nModuleNo, VectorType EndPos, double  dLength, double  dRotZ, double  dRotY);
void ReadInstrData    (long*   pModuleNo, VectorType EndPos, double* pLength, double* pRotZ, double* pRotY);
void WriteSimData     (double  dTimeMeas, double dLmbdWant,  double  dFreq);
void ReadSimData      (double* pTimeMeas, double* pLmbdWant, double* pFreq);
void CopyNeutron      (Neutron* source, Neutron *dest);
long LinesInFile      (FILE* In);
long ColumnsInFile    (FILE* pFile);
char* FullParName     (char* filename);
char* FullInstallName (char* filename, char* sRelPath);

#include <gsl/gsl_rng.h>
extern gsl_rng * vit_gsl_rng;

#endif


