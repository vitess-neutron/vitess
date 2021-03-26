/********************************************************************************************/
/*  VITESS module 'mon2_header.c'                                                           */
/*    Some functions for the 2D monitors                                                    */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors.                                                                          */
/*                                                                                          */
/********************************************************************************************/

#include "mon2_header.h"
#include "init.h"


/******************************/
/**   Defines                **/
/******************************/
#define PrintFloat(a)  {double v = a; if (v==0) fputs("0 ",fMonitor); else printFloatItem(v,fMonitor); }
#define PrintItem(f,a) {double v = a; if (v==0) fputs("0 ",fMonitor); else fprintf(fMonitor,f,v); }
#define PrintInt(f,a)  {int    v = a; if (v==0) fputs("0 ",fMonitor); else fprintf(fMonitor,f,v); }
#define Newline fputc('\n',fMonitor)


/******************************/
/**   Global Variables       **/
/******************************/
static char sFormat   [16]="";             // text describing the 2D output format


/*********************************************************/
/* 'WriteHeader1D/2D': Writes 1D/2D monitor file header  */
/*********************************************************/
void WriteHeader1D(FILE* fMonitor, const char *sType, short bWeight, 
                   int nBinsX, const char* sPar, const char* sUnit) 
{
  fprintf(fMonitor,"#1D %s monitor %s:  %d bins: %s [%s]\n", sType,
          bWeight==FALSE ? "(events)" : "(weight)",
          nBinsX, sPar, sUnit);
  fputs("#     x         F(x)      DeltaF(x)  events\n", fMonitor);  // assumes format "%10.3f  %12.5e %12.5e  %7ld\n"

  return;
}

void WriteHeader1DB(FILE* fMonitor, const char *sType, short iCol, long iBndl, long nBndl, int nBinsX, 
                    double IntMon, long nTrjMon, const char* sPar, const char* sUnit) 
{
  char sDate[11], sTime[9];

  OutputBufferFlush(0);
  GetActDate(sDate);
  GetActTime(sTime);

  fprintf(fMonitor, "# 1D %s monitor:  %d bins: %s [%s]\n", sType, nBinsX, sPar, sUnit);
  fprintf(fMonitor, "# Date: %s  Time: %s\n", sDate, sTime);
  fprintf(fMonitor, "# Total intensity: %10.3e n/s   Trajectories:%11.0f\n", GetTotInt(iCol), NumNeutWritten - (double)NumEobWritten);
  fprintf(fMonitor, "# Within binning : %10.3e n/s   Trajectories:%11ld \n", IntMon, nTrjMon);
  fprintf(fMonitor, "  %ld %ld  #  bundles written and total\n\n", iBndl, nBndl);
  fprintf(fMonitor, "#     x         F(x)       DeltaF(x)    events\n");  // assumes format "%10.3f  %12.5e %12.5e  %7ld\n"

  return;
}

void WriteHeader2D(FILE* fMonitor, VtFormat2D eFormat, const char *sType, short bWeight, 
                   int nBinsX, const char* sAxisTitleX, int nBinsY, const char* sAxisTitleY) 
{
  OutFmt2Txt(eFormat);  // fills static string 'sFormat'

  fprintf(fMonitor,"#2D %s monitor, Format: %s  %s:   %d bins: %s   %d bins: %s\n", sType,
          sFormat,
          bWeight==FALSE ? "(events)" : "(weight)",
          nBinsX, sAxisTitleX, nBinsY, sAxisTitleY);

  switch (eFormat)
  {
    case MATRIX:
      fputs("#      y        F(x,y) \n              ", fMonitor);
      break;

    case XYZ:
      fputs("#     x           y        F(x,y)     DeltaF(x,y)   events\n", fMonitor);
      break;

    case XYZ_CMPT: 
      fputs("# x  y  z\n", fMonitor);
      break;
  }

  return;
}


void WriteHeader2DB(FILE* fMonitor, VtFormat2D eFormat, const char *sType, short bWeight, long iBndl, long nBndl, double IntMon, long nTrjMon, 
                   int nBinsX, const char* sAxisTitleX, int nBinsY, const char* sAxisTitleY) 
{
  char sEvents[11]="", sDate[11], sTime[9];

  OutputBufferFlush(0);
  GetActDate(sDate);
  GetActTime(sTime);
  OutFmt2Txt(eFormat);  // fills static string 'sFormat'
  if (bWeight==FALSE)
    strcpy(sEvents," (events)");

  fprintf(fMonitor, "# 2D %s monitor%s, Format: %s \n#x-axis:%3d bins: %s  \n#y-axis:%3d bins: %s\n", sType, sEvents, sFormat, nBinsX, sAxisTitleX, nBinsY, sAxisTitleY);
  fprintf(fMonitor, "# Date: %s  Time: %s\n", sDate, sTime);
  fprintf(fMonitor, "# Total Intensity: %10.3e n/s   Trajectories:%11.0f\n", GetTotInt(0), NumNeutWritten - (double)NumEobWritten);
  fprintf(fMonitor, "# Within binning : %10.3e n/s   Trajectories:%11ld \n", IntMon, nTrjMon);
  fprintf(fMonitor, "  %ld %ld  #  bundles written and total\n\n", iBndl, nBndl);

  switch (eFormat)
  {
    case MATRIX:
      fputs("#      y        F(x,y) \n              ", fMonitor);
      break;

    case XYZ:
      fputs("#     x           y        F(x,y)     DeltaF(x,y)   events\n", fMonitor);
      break;

    case XYZ_CMPT: 
      fputs("# x  y  z\n", fMonitor);
      break;
  }

  return;
}

/*********************************************/
/* 'WriteOutput2D': Writes 2D monitor file   */
/*********************************************/
int WriteOutput2D(FILE* fMonitor, int eFormat, short bWeight, 
                   int nBinsX, double* BinPosY, int nArrayX, 
                   int nBinsY, double* BinPosZ, 
                   double* IntYZ, double* IntYZError, long* nTrajYZ) 
{
  return WriteOutput2DB(fMonitor, eFormat, bWeight, 
                        nBinsX, BinPosY, nArrayX, 
                        nBinsY, BinPosZ, 1.0, IntYZ, IntYZError, nTrajYZ);
}
  
int WriteOutput2DB(FILE* fMonitor, int eFormat, short bWeight, 
                   int nBinsX, double* BinPosY, int nArrayX, 
                   int nBinsY, double* BinPosZ, double fNorm,
                   double* IntYZ, double* IntYZError, long* nTrajYZ) 
{
  int    i=0, j=0, k=0, c=0;
  double x=0.0, y=0.0;

  switch (eFormat)
  {
    case MATRIX:
      for (i = 0; i < nBinsX; i++)
        fprintf(fMonitor, "%10.3f    ", (BinPosY[i] + BinPosY[i+1]) / 2.0);
      Newline;

      for (j=0; j < nBinsY; j++) 
      {
        fprintf(fMonitor, "%10.3f  ", (BinPosZ[j]+BinPosZ[j+1]) / 2.0);
        for (i=0; i < nBinsX; i++)
        {
          k = nArrayX * i + j; 
          if (bWeight==TRUE)
            fprintf(fMonitor, "%12.5e ", fNorm*IntYZ[k]);
          else
            fprintf(fMonitor, "%7ld ", (long)(fNorm*nTrajYZ[k]));
        }
        Newline;
      }
      break;

    case XYZ:
      for (j=0; j < nBinsY; j++) 
      {
        y = (BinPosZ[j]+BinPosZ[j+1]) / 2.0;
        for (i=0; i < nBinsX; i++) 
        {
          x = (BinPosY[i]+BinPosY[i+1]) / 2.0;
          k = nArrayX * i + j; 
          fprintf(fMonitor, "%10.3f %10.3f  %12.5e %12.5e  %7ld\n", x,y, fNorm*IntYZ[k], fNorm*IntYZError[k], (long)(fNorm*nTrajYZ[k]));
        }
        Newline;
      }
      break;

    case MATR_CMPT: 
      for (i = 0; i < nBinsX; i++)
        PrintFloat((BinPosY[i] + BinPosY[i+1]) / 2.0);
      Newline;

      for (j = 0; j < nBinsY; j++) 
      {
        PrintItem("%5.3f ", (BinPosZ[j] + BinPosZ[j+1]) / 2.0);
        for (i = 0; i < nBinsX; i++)
        {
          k = nArrayX * i + j; 
          if (bWeight==TRUE)
            PrintItem("%5.3E ", fNorm*IntYZ[k])
          else
            PrintInt("%ld ", (long)(fNorm*nTrajYZ[k]))
        }
        Newline;
      }
      break;

    case XYZ_CMPT: 
      for (j = 0; j < nBinsY; j++) 
      {
        double error, binc;
        y = (BinPosZ[j]+BinPosZ[j+1]) / 2.0;
        for (i = 0; i < nBinsX; i++) 
        {
          k = nArrayX * i + j; 
          c = fNorm*nTrajYZ[k];
          PrintFloat((BinPosY[i]+BinPosY[i+1]) / 2.0);
          PrintFloat(y);
          if (c <= 0)
          { fputs("0 0 0\n", fMonitor);
          }
          else 
          {
            binc  = fNorm*IntYZ[k];
            error = binc <= 0 ? 0 : binc * sqrt(1./c);
            PrintItem("%5.3E ", binc);
            PrintItem("%5.3E ", error);
            fprintf(fMonitor, "%ld\n", c);
          }
        }
        Newline;
      }
      break;

    default:
      Error("value for variable 'eFormat' unknown");
  }

  return 1;
}


/******************************************************************/
/* 'printFloatItem': Write one float value to the 2D monitor file */
/******************************************************************/
void printFloatItem(double v, FILE*f) 
{
  static char buf[16];
  int k;
  sprintf(buf, "%10.7f", v);
  // cut off trailing zeroes
  for (k=9; k>=0; k--)
    if (buf[k] != '0')
      break;
  buf[k+1] = 0;
  fputs(buf,f);
  fputc(' ',f);
}


/****************************************************/
/* 'OutFmt2Txt': converts 2D output format to text  */
/****************************************************/
void OutFmt2Txt(VtFormat2D eFormat)
{
  switch (eFormat)
  {
    case MATRIX   : strcpy(sFormat, "matrix");         break;
    case XYZ      : strcpy(sFormat, "x y z");          break;
    case MATR_CMPT: strcpy(sFormat, "matrix compact"); break;
    case XYZ_CMPT : strcpy(sFormat, "x y z compact");  break;
    default       : Error("unknown value for 2D output format"); 
  }
  return;
}


#undef PrintItem
#undef Newline
