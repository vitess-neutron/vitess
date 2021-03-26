#ifndef MON2_HEADER_H
#define MON2_HEADER_H

#define BINSIZE 1001

#include "general.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

void WriteHeader1D (FILE* fMonitor, const char *sType, short bWeight, int nBinsX,         // Writes header for 1D monitor file
                    const char* sPar, const char* sUnit);   
void WriteHeader1DB(FILE* fMonitor, const char *sType,                                    // Writes header for 1D monitor file incl. number of proc. bundles
                    short iCol, long iBndl, long nBndl, int nBinsX, 
                    double IntMon, long nTrjMon, 
                    const char* sPar, const char* sUnit);   
void WriteHeader2D (FILE* fMonitor, VtFormat2D eFormat, const char *sType, short bWeight, // Writes header for 2D monitor file
                    int nBinsX, const char* sAxisTitleX, 
                    int nBinsY, const char* sAxisTitleY); 
void WriteHeader2DB(FILE* fMonitor, VtFormat2D eFormat, const char *sType, short bWeight, // Writes header for 2D monitor file incl. number of proc. bundles  
                    long iBndl, long nBndl, double IntMon, long nTrjMon, 
                    int nBinsX, const char* sAxisTitleX, 
                    int nBinsY, const char* sAxisTitleY); 
int  WriteOutput2D (FILE* fMonitor, int eFormat, short bWeight,                           // Writes 2D spectrum to monitor file
                   int nBinsX, double* BinPosY, int nArrayX,
                   int nBinsY, double* BinPosZ, double* IntYZ, double* IntYZError, long* nTrajYZ);  
int  WriteOutput2DB(FILE* fMonitor, int eFormat, short bWeight,                           // Writes 2D spectrum to monitor file
                   int nBinsX, double* BinPosY, int nArrayX,
                   int nBinsY, double* BinPosZ, double fNorm,
                   double* IntYZ, double* IntYZError, long* nTrajYZ);  

void printFloatItem(double v, FILE*f);                                                  // Writes one float value to the 2D monitor file
void OutFmt2Txt    (VtFormat2D eFormat);                                                // converts 2D output format to text

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
