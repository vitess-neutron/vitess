#ifndef SAMPLE_H
#define SAMPLE_H

#include "intersection.h"
#include "general.h"

/******************************/
/**   Global Variables       **/
/******************************/
extern int colD, colF, colF2, colDW, colM, colh, colk, coll;

extern double scaleF2;

extern double* hVal;
extern double* kVal;
extern double* lVal;
extern double* F2Val;


/******************************/
/** Prototypes               **/
/******************************/
void InitSample  (SampleType *Sample);
void FillSample(SampleType* pSample, const VtSmplGeom eGeom, 
                const double Xpos,   const double Ypos,  const double Zpos, 
                const double Xdir,   const double Ydir,  const double Zdir, 
                const double SizeD,  const double SizeH, const double SizeW, const double SizeT);

void SetSampleGeometry(SampleType *Sample, double CubeRotAngle);

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

void WriteScatIAP(Neutron* pNeutrSmpl, VtReason eReason, double RotMatrixSmpl[3][3], VectorType PosSmpl);

#endif
