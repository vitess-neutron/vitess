#ifndef GENER_H
#define GENER_H

#include <stdio.h>

// definitions
#define BUFLEN  999
#define MAX_FIL  10  // Code must be changed, if MAX_FIL > 10  (sscanf(sFile) gener_fct 187f)
#define MAX_PAR  25  // Code must be changed, if MAX_PAR > 25  (sscanf(nModNo)  gener_fct 195ff,
#define MAX_SIM 100  //                                         sscanf(sParId)  gener_fct 200ff,
#define MAX_MOD  30  //                                         sscanf(sParVal) gener_fct 254ff)

#define TRUE  1
#define FALSE 0

typedef enum
{  VT_SYS_NN = 0,
  VT_WIN_NT = 1,
  VT_WIN_98 = 2,
  VT_UNIX   = 3,
  VT_LINUX  = 4
}
VtSystem;

typedef enum
{  VT_SER_1F = 0,
  VT_SER_2F = 1,
  VT_FIT    = 2
}
VtModus;

void  InitArrays   ();
short ReadBatchFile();
short ReadInfoFile (short* pFileNum, char* sSeriesname);
short ChangeParam  (short iSim);
void  WriteCommand (FILE* pFile, short nModNo, short bEcho);

int   GetLine      (FILE* pFile, char* const pLine);
void  StripCmdLine (char* const pLine, char cShort);
void  SubstPar     (char* pLine, short kBeg, const char* pEnd, int nVar, const char* pVar);
void  EraseEndSlash(char* pStr, char* const pStrSl);
short ChangeParam  (short iSim);
short StrgChange   (char* sStr, const char* sOut, const char* sIn);
char* StrgFind     (char* sStr, const char* sSearch);
void  NumerateName (char* sFileLong, char* sFileShort, const short nNumber);

long  StrgScanHD   (const char* sStr, short* pTab, const int nMax);
long  StrgScanS    (const char* sStr, char* pTab, const int nMax, const int nTextLen);

#endif
