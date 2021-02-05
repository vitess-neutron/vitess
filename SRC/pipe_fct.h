#ifndef PIPE_FCT_H
#define PIPE_FCT_H

#include <stdio.h>

#ifdef _MSC_VER
 #define VT_WINDOWS
#endif 

void  InitArrays   ();
short ReadSimPar   (char*  pParFct, short* pFileNo);
short ReadPData    (short* pNx, short* pMin, short* pMax, short* pPrtCmd, double P[MAX_SIM][NMAX], const char* sFilename);
short ReadCmdFile  (short bPrtCmd);

short ChangeParam  (short  iSim);
void  WriteCommand (FILE*  pFile, short nModNo, short bEcho);

void  StripCmdLine  (char* const pLine, char cShort);
void  SubstPar      (char* pLine, short kBeg, const char* pEnd, const char* pVar);
void  EraseEndSlash (char* pStr, char* const pStrSl);
short StrgChange    (char* sStr, const char* sOut, const char* sIn);
char* StrgFind      (char* sStr, const char* sSearch);
void  ExtendFilename(char* sFileLong, char* sFileShort, char* sAddition);

long  StrgScanHD(const char* sStr, short* pTab, const int nMax);
long  StrgScanS (const char* sStr, char* pTab, const int nMax, const int nTextLen);

#endif
