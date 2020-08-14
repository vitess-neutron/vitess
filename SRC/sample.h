#ifndef SAMPLE_H
#define SAMPLE_H

#include "intersection.h"
#include "general.h"

extern char* SampleFileName;

int colD, colF, colF2, colDW, colM, colh, colk, coll;
double scaleF2;

double* hVal;
double* kVal;
double* lVal;
double* F2Val;

void InitSample  (SampleType *Sample);
void ReadCube    (FILE *SampleFile, SampleType *Sample);
void ReadCylinder(FILE *SampleFile, SampleType *Sample);
void ReadBall    (FILE *SampleFile, SampleType *Sample);

void SetSampleGeometry(SampleType *Sample);

int  CompPair(const void* p1, const void* p2);
int  ReadTilComment(char* pBuffer, FILE* pSampleFile);

void ProcessNeutronToEnd(Neutron *Neut, VectorType SP, double l1,
                         double DetFac, double ScProb, double OutTheta,
                         double OutPhi, SampleType *Sample,
                         double RotMatrixNeut[3][3], double RotMatrixSmpl[3][3]);

long NeutronIntersectsSample(const Neutron *Nin, SampleType* pSample,
                             double SampleRotMatrix[3][3], VectorType ISP[2],
                             long* pNisp, VtDir eDir);

int ReadStructureFile(const char* sStructFile, int tag, DoublePair* structFactorLookup[]);


#endif
