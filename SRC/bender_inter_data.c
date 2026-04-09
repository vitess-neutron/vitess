/*********************************************************************************/
/* bender_inter_data.c                                                           */
/*                                                                               */
/* Functions to calculate attenuation in materials as a function of wavelength   */
/*                                                                               */
/* The data was given by Thomas Krist, HAHN-MEITNER-INSTITUT, BERLIN             */
/*                                                                               */
/* 1.0  Oct 2003  S. Manoshin     initial version                                */
/* 1.1  Aug 2019  K. Lieutenant   tidy up                                        */
/* 1.2  May 2022  K. Lieutenant   functions AttenuationXyz() added               */
/*********************************************************************************/

#include <math.h>

#include "init.h"
#include "convert.h"
#include "message.h"
#include "bender_inter_data.h"


/******************************/
/** Local  Functions         **/
/******************************/
static void InitArrays (double* pLambda, double* pMu, const long  nArrayLen);


/******************************/
/** Local Variables          **/
/******************************/
static 
double aLambdaL[MAX_MU], 
       aMuL    [MAX_MU]; // data arrays (wavelength, attenuation)

 
/****************************************************************/
/* Attenuation as a function of wavelength for some materials   */
/****************************************************************/
double AttenuationWnd(const double Lambda, const VtWndMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFile)
{
  char   sMat[21]="";
  long   nValues=0;        // number of data points
  double mu_tot=0.0,       // total attenuation
         *pLmbd, *pMu;     // pointer to data arrays (wavelength, attenuation)

  // gets data if not delivered
  if (aLambdaI==NULL || aMuI==NULL || nArrayLen < 2)
  { 
    InitArrays(aLambdaL, aMuL, MAX_MU);

    switch (eMatID)
    {
      case VT_WND_FILE  : nValues = LoadAttenFile(aLambdaL, aMuL, sAttenFile, MAX_MU); break;
      case VT_WND_GD    : nValues = Gadolinium   (aLambdaL, aMuL, MAX_MU);          break;
      case VT_WND_CD    : nValues = Cadmium      (aLambdaL, aMuL, MAX_MU);          break;
      case VT_WND_B10   : nValues = Bor10        (aLambdaL, aMuL, MAX_MU);          break;
      case VT_WND_EU    : nValues = Eu           (aLambdaL, aMuL, MAX_MU);          break;
      case VT_WND_SI    : nValues = Silicon      (aLambdaL, aMuL, MAX_MU);          break;
      case VT_WND_VAC   : nValues = Vacuum       (aLambdaL, aMuL);                     break;
      case VT_WND_IDEAL : nValues = IdealAbsorp  (aLambdaL, aMuL);                     break;
      case VT_WND_ARRAY : Error ("bender_inter_data:AttenuationWnd(): Array has NULL pointer");             break;
      case VT_NO_WND_MAT: Error ("bender_inter_data:AttenuationWnd(): Material for attenuation not given"); break;
      default           : WndMat_ID2Txt(sMat, eMatID);
                          Error2("bender_inter_data:AttenuationWnd(): Attenuation for this material not yet implemented", sMat);
    }
    pLmbd = aLambdaL;
    pMu   = aMuL;
  }
  else
  { 
    nValues = nArrayLen;
    pLmbd   = aLambdaI;
    pMu     = aMuI;
  }

  mu_tot=Interpol(Lambda, pLmbd, pMu, nValues);

  return mu_tot;
}


double AttenuationAbs(const double Lambda, const VtAbsMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFile)
{
  char   sMat[21]="";      
  long   nValues=0;        // number of data points
  double mu_tot=0.0,       // total attenuation
         *pLmbd, *pMu;     // pointer to data arrays (wavelength, attenuation)

  // gets data if not delivered
  if (aLambdaI==NULL || aMuI==NULL || nArrayLen < 2)
  {
    InitArrays(aLambdaL, aMuL, MAX_MU);

    switch (eMatID)
    {
      case VT_ABS_FILE  : nValues = LoadAttenFile(aLambdaL, aMuL, sAttenFile, MAX_MU); break;
      case VT_ABS_GD    : nValues = Gadolinium   (aLambdaL, aMuL, MAX_MU);          break;
      case VT_ABS_CD    : nValues = Cadmium      (aLambdaL, aMuL, MAX_MU);          break;
      case VT_ABS_B10   : nValues = Bor10        (aLambdaL, aMuL, MAX_MU);          break;
      case VT_ABS_EU    : nValues = Eu           (aLambdaL, aMuL, MAX_MU);          break;
      case VT_ABS_IDEAL : nValues = IdealAbsorp  (aLambdaL, aMuL);                     break;
      case VT_ABS_ARRAY : Error ("bender_inter_data:AttenuationAbs(): Array has NULL pointer");             break;
      case VT_NO_ABS_MAT: Error ("bender_inter_data:AttenuationAbs(): Aaterial for attenuation not given"); break;
      default           : AbsMat_ID2Txt(sMat, eMatID);
                          Error2("bender_inter_data:AttenuationAbs(): Attenuation for this material not yet implemented", sMat);
    }
    pLmbd = aLambdaL;
    pMu   = aMuL;
  }
  else
  { 
    nValues = nArrayLen;
    pLmbd   = aLambdaI;
    pMu     = aMuI;
  }

  mu_tot=Interpol(Lambda, pLmbd, pMu, nValues);

  return mu_tot;
}


double AttenuationMir(const double Lambda, const VtMirrMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttFile)
{
  char   sMat[21]="";
  long   nValues=0;        // number of data points
  double mu_tot=0.0,       // total attenuation
         *pLmbd, *pMu;     // pointer to data arrays (wavelength, attenuation)

  // gets data if not delivered
  if (aLambdaI==NULL || aMuI==NULL || nArrayLen < 2)
  {
    InitArrays(aLambdaL, aMuL, MAX_MU);

    switch (eMatID)
    {
      case VT_MIRR_FILE  : nValues = LoadAttenFile(aLambdaL, aMuL, sAttFile, MAX_MU); break;
      case VT_MIRR_VACUUM: nValues = Vacuum       (aLambdaL, aMuL);                   break;
      case VT_MIRR_SI    : nValues = Silicon      (aLambdaL, aMuL, MAX_MU);        break;
      case VT_MIRR_B4C   : nValues = Bor10        (aLambdaL, aMuL, MAX_MU);        break;
      case VT_MIRR_ARRAY : Error ("bender_inter_data:AttenuationMir(): Array has NULL pointer");             break;
      case VT_NO_MIRR_MAT: Error ("bender_inter_data:AttenuationMir(): Material for attenuation not given"); break;
      case VT_MIRR_OTHER : Error ("bender_inter_data:AttenuationMir(): 'other material' cannot be handled, give file name or data arrays");   break;
      default            : MirrMat_ID2Txt(sMat, eMatID);
                           Error2("bender_inter_data:AttenuationMir(): Attenuation for this material not yet implemented", sMat);
    }
    pLmbd = aLambdaL;
    pMu   = aMuL;
  }
  else
  { 
    nValues = nArrayLen;
    pLmbd   = aLambdaI;
    pMu     = aMuI;
  }

  mu_tot = Interpol(Lambda, pLmbd, pMu, nValues);
  if (eMatID==VT_MIRR_B4C)
    mu_tot *= 0.8 * 2.52/2.34 * 0.199;  // ratio of atom numbers, densities and isotope ratio; C and B-11 contribution ignored

  return mu_tot;
}


/*******************************************************/
/** Read attenuation data                             **/
/*******************************************************/
long LoadAttenFile(double* Lambda, double* Mu, const char* sAttenFileName, long nArrayLen)
{
  long  nValues=-1, nLines=0, iLine=0;
  char  sBuffer[100]="";
  FILE* pTransFile=NULL;

  if (sAttenFileName != NULL)
  {
    pTransFile = OpenParameterFile(sAttenFileName, TRUE, "r");;

    if (pTransFile != NULL)
    {
      nLines = LinesInFile(pTransFile);
      nValues= mini(nLines, nArrayLen);
      for (iLine=0; iLine < nValues; iLine++)
      {
        ReadLine(pTransFile, sBuffer, sizeof(sBuffer)-1);
        sscanf(sBuffer,"%lf %lf", &Lambda[iLine], &Mu[iLine]);

        if (Mu[iLine] < 0.0 || Lambda[iLine] <= 0.0)
          Error("bender_inter_data:LoadAttenFile(): Negative value or zero-wavelength found in attenuation file");
      }

      fclose(pTransFile);
    }

    /* check the input data */
    for (iLine=0; iLine < nValues-1; iLine++)
    {
      if (Lambda[iLine+1] <= Lambda[iLine])
        Error("bender_inter_data:LoadAttenFile(): : wavelength values must be in ascending order in attenuation file");
    }

    fprintf(LogFilePtr,"Wavelength range read: %f7.3 - %f7.3 \n", Lambda[0], Lambda[nLines-1]);
  }
  else
  {
    Error("bender_inter_data:LoadAttenFile(): No name given for the file describing the transmission of the bender channel");
  }

  return nValues;
}


long ReadAttenWnd(const VtWndMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFileName)
{
  char   sMat[21]="";
  long   nValues=-1;                  // number of data points

  if (aLambdaI!=NULL && aMuI!=NULL)
  { 
    InitArrays(aLambdaI, aMuI, nArrayLen);

    switch (eMatID)
    {
      case VT_WND_FILE  : nValues = LoadAttenFile(aLambdaI, aMuI, sAttenFileName, nArrayLen); break;
      case VT_WND_GD    : nValues = Gadolinium   (aLambdaI, aMuI, nArrayLen); break;
      case VT_WND_CD    : nValues = Cadmium      (aLambdaI, aMuI, nArrayLen); break;
      case VT_WND_B10   : nValues = Bor10        (aLambdaI, aMuI, nArrayLen); break;
      case VT_WND_EU    : nValues = Eu           (aLambdaI, aMuI, nArrayLen); break;
      case VT_WND_SI    : nValues = Silicon      (aLambdaI, aMuI, nArrayLen); break;
      case VT_WND_VAC   : nValues = Vacuum       (aLambdaI, aMuI);            break;
      case VT_WND_IDEAL : nValues = IdealAbsorp  (aLambdaI, aMuI);            break;
      case VT_WND_ARRAY :                                            
      case VT_NO_WND_MAT: Error ("bender_inter_data:ReadAttenWnd(): Material for attenuation not given");                             break;
      default           : WndMat_ID2Txt(sMat, eMatID);
                          Error2("bender_inter_data:ReadAttenWnd(): Attenuation for this material not yet implemented", sMat);
    }
  }

  return nValues;
}


long ReadAttenAbs(const VtAbsMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFileName)
{
  char   sMat[21]="";
  long   nValues=-1;                  // number of data points

  if (aLambdaI!=NULL && aMuI!=NULL)
  { 
    InitArrays(aLambdaI, aMuI, nArrayLen);

    switch (eMatID)
    {
      case VT_ABS_FILE  : nValues = LoadAttenFile(aLambdaI, aMuI, sAttenFileName, nArrayLen); break;
      case VT_ABS_GD    : nValues = Gadolinium   (aLambdaI, aMuI, nArrayLen); break;
      case VT_ABS_CD    : nValues = Cadmium      (aLambdaI, aMuI, nArrayLen); break;
      case VT_ABS_B10   : nValues = Bor10        (aLambdaI, aMuI, nArrayLen); break;
      case VT_ABS_EU    : nValues = Eu           (aLambdaI, aMuI, nArrayLen); break;
      case VT_ABS_IDEAL : nValues = IdealAbsorp  (aLambdaI, aMuI);            break;
      case VT_ABS_ARRAY :                                            
      case VT_NO_ABS_MAT: Error ("bender_inter_data:ReadAttenAbs(): Material for attenuation not given");                      break;
      default           : AbsMat_ID2Txt(sMat, eMatID);
                          Error2("bender_inter_data:ReadAttenAbs(): Attenuation for this material not yet implemented", sMat);
    }
  }

  return nValues;
}


long ReadAttenMir(const VtMirrMat eMatID, double* aLambdaI, double* aMuI, const long nArrayLen, const char* sAttenFileName)
{
  char   sMat[21]="";
  long   i=0, nValues=-1;                  // number of data points

  if (aLambdaI!=NULL && aMuI!=NULL)
  { 
    InitArrays(aLambdaI, aMuI, nArrayLen);

    switch (eMatID)
    {
      case VT_MIRR_FILE  : nValues = LoadAttenFile(aLambdaI, aMuI, sAttenFileName, nArrayLen); break;
      case VT_MIRR_SI    : nValues = Silicon      (aLambdaI, aMuI, nArrayLen); break;
      case VT_MIRR_B4C   : nValues = Bor10        (aLambdaI, aMuI, nArrayLen); break;
      case VT_MIRR_VACUUM: nValues = Vacuum       (aLambdaI, aMuI);            break;
      case VT_MIRR_ARRAY  :                                            
      case VT_MIRR_OTHER : Error ("bender_inter_data:ReadAttenMir(): 'other' cannot be handled here");                               break;
      case VT_NO_MIRR_MAT: Error ("bender_inter_data:ReadAttenMir(): Material for attenuation not given");                           break;
      default            : MirrMat_ID2Txt(sMat, eMatID);
                           Error2("bender_inter_data:ReadAttenMir(): Attenuation for this material not yet implemented", sMat);
    }

    if (eMatID==VT_MIRR_B4C)
      for (i=0; i < nValues; i++)  
        aMuI[i] *= 0.8 * 2.52/2.34 * 0.199;  // ratio of atom numbers, densities and isotope ratio; C and B-11 contribution ignored
  }

  return nValues;
}


double Interpol(double lambda, double* aLambda, double* aMu, long nValues)
{
  double X1=0.0, X2=0.0,           // wavelength
         Y1=0.0, Y2=0.0,           // and attenuation values of the interpolation interval
         Mu=-1.0;                  // interpolated attenuation value [1/cm]
  long   i=0,                      // index ....
         iIpn=0;                   // index of left side of interval for interpolation

  // Check input and interval
  // ------------------------
  if (aLambda==NULL || aMu==NULL)
    Error("bender_inter_data:Interpol(): Data arrays missing");

  if (nValues < 2)
    Error("bender_inter_data:Interpol(): Insufficient number of data points for interpolation");
  
  if (ISNAN(aLambda[0]) || ISNAN(aLambda[nValues-1]) || ISNAN(aMu[0]) || ISNAN(aMu[nValues-1]))
    Error("bender_inter_data:Interpol(): Data arrays not properly filled");

  if (lambda < aLambda[0] || lambda > aLambda[nValues-1])
  {
    CountMessage(ALL_L_RANGE_TOO_SMALL);
    return(-10000.0);
  }

  // Search interval
  // --------------
  for (i=0; i < (nValues-1); i++)
  {
    if (lambda >= aLambda[i] && lambda <= aLambda[i+1])
    {
      X1 = aLambda[i];
      X2 = aLambda[i+1];
      Y1 = aMu[i];
      Y2 = aMu[i+1];
      iIpn = i;
      break;
    }
  }

  // Linear interpolation
  // --------------------
  if (X1 != X2)
  {
    Mu = Y1 + ((Y2-Y1)*(lambda-X1)/(X2-X1));
  }
  else
  {
    if (Y1==Y2)
      Mu = Y1;
    else
      Error("bender_inter_data:Interpol(): Two different mu-values for the wavelength found");
  }

  return(Mu);
}


void WriteMatInfo (VtWndMat eMaterial, char* pLocation)
{
  char sMaterial[21]="";

  WndMat_ID2Txt(sMaterial, eMaterial);
  fprintf(LogFilePtr,"%s is %s\n", pLocation, sMaterial);
}


/****************************************************************/
/* Local functions: mu(lambda) values for different materials   */
/****************************************************************/
/* For gadolinium */
long Gadolinium(double *pLmbd, double *pMu, const long nArrayLen)
{
  long i=0, nValues=0; 

  /* Wavelength Ang;   Attenuation, cm^-1 */
  aLambdaL[ 0] = 0.29;    aMuL[ 0] = 1.06;
  aLambdaL[ 1] = 0.30;    aMuL[ 1] = 1.21;
  aLambdaL[ 2] = 0.32;    aMuL[ 2] = 1.66;
  aLambdaL[ 3] = 0.34;    aMuL[ 3] = 2.12;
  aLambdaL[ 4] = 0.37;    aMuL[ 4] = 3.03;
  aLambdaL[ 5] = 0.39;    aMuL[ 5] = 3.93;
  aLambdaL[ 6] = 0.40;    aMuL[ 6] = 5.87;
  aLambdaL[ 7] = 0.41;    aMuL[ 7] = 6.53;
  aLambdaL[ 8] = 0.43;    aMuL[ 8] = 7.22;
  aLambdaL[ 9] = 0.44;    aMuL[ 9] = 8.36;
  aLambdaL[10] = 0.45;    aMuL[10] = 9.3;
  aLambdaL[11] = 0.48;    aMuL[11] = 12.87;
  aLambdaL[12] = 0.52;    aMuL[12] = 18.32;
  aLambdaL[13] = 0.54;    aMuL[13] = 21.58;
  aLambdaL[14] = 0.56;    aMuL[14] = 25.79;
  aLambdaL[15] = 0.58;    aMuL[15] = 31.43;
  aLambdaL[16] = 0.61;    aMuL[16] = 39.1;
  aLambdaL[17] = 0.64;    aMuL[17] = 49.9;
  aLambdaL[18] = 0.66;    aMuL[18] = 56.76;
  aLambdaL[19] = 0.67;    aMuL[19] = 65.25;
  aLambdaL[20] = 0.69;    aMuL[20] = 75.53;
  aLambdaL[21] = 0.71;    aMuL[21] = 88.23;
  aLambdaL[22] = 0.74;    aMuL[22] = 104.08;
  aLambdaL[23] = 0.76;    aMuL[23] = 123.92;
  aLambdaL[24] = 0.79;    aMuL[24] = 149.23;
  aLambdaL[25] = 0.83;    aMuL[25] = 181.79;
  aLambdaL[26] = 0.86;    aMuL[26] = 224.31;
  aLambdaL[27] = 0.90;    aMuL[27] = 280.66;
  aLambdaL[28] = 0.95;    aMuL[28] = 355.24;
  aLambdaL[29] = 1.01;    aMuL[29] = 453.86;
  aLambdaL[30] = 1.08;    aMuL[30] = 583.29;
  aLambdaL[31] = 1.17;    aMuL[31] = 747.98;
  aLambdaL[32] = 1.28;    aMuL[32] = 944.71;
  aLambdaL[33] = 1.43;    aMuL[33] = 1161.75;
  aLambdaL[34] = 1.65;    aMuL[34] = 1384.43;
  aLambdaL[35] = 2.02;    aMuL[35] = 1630.73;
  aLambdaL[36] = 2.86;    aMuL[36] = 2083.02;
  aLambdaL[37] = 3.20;    aMuL[37] = 2268.93;
  aLambdaL[38] = 3.69;    aMuL[38] = 2546.22;
  aLambdaL[39] = 4.52;    aMuL[39] = 3028.2;
  aLambdaL[40] = 6.39;    aMuL[40] = 4139.39;
  aLambdaL[41] = 9.04;    aMuL[41] = 5749.4;
  aLambdaL[42] = 12.78;  aMuL[42] = 8170.2;
  aLambdaL[43] = 28.59;  aMuL[43] = 16945.6;

  nValues = mini(44, nArrayLen);
  for (i=0; i < nValues; i++)
  { pLmbd[i] = aLambdaL[i];
    pMu  [i] = aMuL    [i];
  }

  return(nValues);
}


/* For cadmium */
long Cadmium(double *pLmbd, double *pMu, const long nArrayLen)
{
  long i=0, nValues=0; 

  /* Wavelength Ang;   Attenuation, cm^-1 */
  aLambdaL[0] = 0.29;     aMuL[0] = 1.02;
  aLambdaL[1] = 0.30;    aMuL[1] = 1.27;
  aLambdaL[2] = 0.32;    aMuL[2] = 1.62;
  aLambdaL[3] = 0.34;    aMuL[3] = 2.31;
  aLambdaL[4] = 0.37;    aMuL[4] = 3.7;
  aLambdaL[5] = 0.39;    aMuL[5] = 4.63;
  aLambdaL[6] = 0.40;    aMuL[6] = 6.95;
  aLambdaL[7] = 0.41;    aMuL[7] = 8.1;
  aLambdaL[8] = 0.43;    aMuL[8] = 9.26;
  aLambdaL[9] = 0.44;    aMuL[9] = 12.73;
  aLambdaL[10] = 0.45;    aMuL[10] = 16.21;
  aLambdaL[11] = 0.48;    aMuL[11] = 25.47;
  aLambdaL[12] = 0.52;    aMuL[12] = 46.3;
  aLambdaL[13] = 0.54;    aMuL[13] = 64.82;
  aLambdaL[14] = 0.56;    aMuL[14] = 92.6;
  aLambdaL[15] = 0.58;    aMuL[15] = 138.9;
  aLambdaL[16] = 0.61;    aMuL[16] = 208.35;
  aLambdaL[17] = 0.64;    aMuL[17] = 287.06;
  aLambdaL[18] = 0.66;    aMuL[18] = 314.84;
  aLambdaL[19] = 0.67;    aMuL[19] = 324.1;
  aLambdaL[20] = 0.69;    aMuL[20] = 347.25;
  aLambdaL[21] = 0.71;    aMuL[21] = 331.05;
  aLambdaL[22] = 0.74;    aMuL[22] = 300.95;
  aLambdaL[23] = 0.76;    aMuL[23] = 277.8;
  aLambdaL[24] = 0.79;    aMuL[24] = 245.39;
  aLambdaL[25] = 0.83;    aMuL[25] = 208.35;
  aLambdaL[26] = 0.86;    aMuL[26] = 185.2;
  aLambdaL[27] = 0.90;    aMuL[27] = 162.05;
  aLambdaL[28] = 0.95;    aMuL[28] = 148.16;
  aLambdaL[29] = 1.01;    aMuL[29] = 129.64;
  aLambdaL[30] = 1.08;    aMuL[30] = 120.38;
  aLambdaL[31] = 1.17;    aMuL[31] = 115.75;
  aLambdaL[32] = 1.28;    aMuL[32] = 115.29;
  aLambdaL[33] = 1.43;    aMuL[33] = 111.12;
  aLambdaL[34] = 1.65;    aMuL[34] = 115.75;
  aLambdaL[35] = 2.02;    aMuL[35] = 125.01;
  aLambdaL[36] = 2.86;    aMuL[36] = 152.79;
  aLambdaL[37] = 3.20;    aMuL[37] = 175.94;
  aLambdaL[38] = 3.69;    aMuL[38] = 196.78;
  aLambdaL[39] = 4.52;    aMuL[39] = 236.13;
  aLambdaL[40] = 6.39;    aMuL[40] = 324.1;
  aLambdaL[41] = 9.04;    aMuL[41] = 463.0;
  aLambdaL[42] = 12.78;  aMuL[42] = 648.2;
  aLambdaL[43] = 28.59;  aMuL[43] = 1389.0;

  nValues = mini(44, nArrayLen);
  for (i=0; i < nValues; i++)
  { pLmbd[i] = aLambdaL[i];
    pMu  [i] = aMuL    [i];
  }

  return(nValues);
}


/* For Bor10 */
long Bor10(double *pLmbd, double *pMu, const long nArrayLen)
{
  long i=0, nValues=0; 

  /* Wavelength Ang;   Attenuation, cm^-1 */
  aLambdaL[0] = 0.29;     aMuL[0] = 79.07;
  aLambdaL[1] = 0.30;    aMuL[1] = 83.35;
  aLambdaL[2] = 0.32;    aMuL[2] = 88.4;
  aLambdaL[3] = 0.34;    aMuL[3] = 94.51;
  aLambdaL[4] = 0.37;    aMuL[4] = 102.08;
  aLambdaL[5] = 0.39;    aMuL[5] = 106.62;
  aLambdaL[6] = 0.40;    aMuL[6] = 111.82;
  aLambdaL[7] = 0.41;    aMuL[7] = 114.73;
  aLambdaL[8] = 0.43;    aMuL[8] = 117.87;
  aLambdaL[9] = 0.44;    aMuL[9] = 121.29;
  aLambdaL[10] = 0.45;    aMuL[10] = 125.02;
  aLambdaL[11] = 0.48;    aMuL[11] = 133.65;
  aLambdaL[12] = 0.52;    aMuL[12] = 144.36;
  aLambdaL[13] = 0.54;    aMuL[13] = 149.43;
  aLambdaL[14] = 0.56;    aMuL[14] = 155.07;
  aLambdaL[15] = 0.58;    aMuL[15] = 161.4;
  aLambdaL[16] = 0.61;    aMuL[16] = 168.58;
  aLambdaL[17] = 0.64;    aMuL[17] = 176.81;
  aLambdaL[18] = 0.66;    aMuL[18] = 181.4;
  aLambdaL[19] = 0.67;    aMuL[19] = 186.37;
  aLambdaL[20] = 0.69;    aMuL[20] = 191.77;
  aLambdaL[21] = 0.71;    aMuL[21] = 197.68;
  aLambdaL[22] = 0.74;    aMuL[22] = 204.16;
  aLambdaL[23] = 0.76;    aMuL[23] = 211.32;
  aLambdaL[24] = 0.79;    aMuL[24] = 219.3;
  aLambdaL[25] = 0.83;    aMuL[25] = 228.26;
  aLambdaL[26] = 0.86;    aMuL[26] = 238.41;
  aLambdaL[27] = 0.90;    aMuL[27] = 250.04;
  aLambdaL[28] = 0.95;    aMuL[28] = 263.57;
  aLambdaL[29] = 1.01;    aMuL[29] = 279.56;
  aLambdaL[30] = 1.08;    aMuL[30] = 298.86;
  aLambdaL[31] = 1.17;    aMuL[31] = 322.8;
  aLambdaL[32] = 1.28;    aMuL[32] = 353.61;
  aLambdaL[33] = 1.43;    aMuL[33] = 395.35;
  aLambdaL[34] = 1.65;    aMuL[34] = 456.51;
  aLambdaL[35] = 2.02;    aMuL[35] = 559.11;
  aLambdaL[36] = 2.86;    aMuL[36] = 790.7;
  aLambdaL[37] = 3.20;    aMuL[37] = 884.03;
  aLambdaL[38] = 3.69;    aMuL[38] = 1020.79;
  aLambdaL[39] = 4.52;    aMuL[39] = 1250.21;
  aLambdaL[40] = 6.39;    aMuL[40] = 1768.06;
  aLambdaL[41] = 9.04;    aMuL[41] = 2500.42;
  aLambdaL[42] = 12.78;  aMuL[42] = 3536.13;
  aLambdaL[43] = 28.59;  aMuL[43] = 7907.02;

  nValues = mini(44, nArrayLen);
  for (i=0; i < nValues; i++)
  { pLmbd[i] = aLambdaL[i];
    pMu  [i] = aMuL    [i];
  }

  return(nValues);
}


/* For Eu */
long Eu(double *pLmbd, double *pMu, const long nArrayLen)
{
  long i=0, nValues=0; 

  /* Wavelength Ang;   Attenuation, cm^-1 */
  aLambdaL[0] = 0.29;     aMuL[0] = 14.54;
  aLambdaL[1] = 0.30;    aMuL[1] = 6.23;
  aLambdaL[2] = 0.32;    aMuL[2] = 5.19;
  aLambdaL[3] = 0.34;    aMuL[3] = 8.31;
  aLambdaL[4] = 0.37;    aMuL[4] = 21.31;
  aLambdaL[5] = 0.39;    aMuL[5] = 45.96;
  aLambdaL[6] = 0.40;    aMuL[6] = 133.34;
  aLambdaL[7] = 0.41;    aMuL[7] = 220.55;
  aLambdaL[8] = 0.43;    aMuL[8] = 243.41;
  aLambdaL[9] = 0.44;    aMuL[9] = 173.05;
  aLambdaL[10] = 0.45;    aMuL[10] = 101.58;
  aLambdaL[11] = 0.48;    aMuL[11] = 66.44;
  aLambdaL[12] = 0.52;    aMuL[12] = 57.02;
  aLambdaL[13] = 0.54;    aMuL[13] = 41.74;
  aLambdaL[14] = 0.56;    aMuL[14] = 31.44;
  aLambdaL[15] = 0.58;    aMuL[15] = 25.45;
  aLambdaL[16] = 0.61;    aMuL[16] = 21.77;
  aLambdaL[17] = 0.64;    aMuL[17] = 19.38;
  aLambdaL[18] = 0.66;    aMuL[18] = 18.8;
  aLambdaL[19] = 0.67;    aMuL[19] = 18.19;
  aLambdaL[20] = 0.69;    aMuL[20] = 17.86;
  aLambdaL[21] = 0.71;    aMuL[21] = 17.81;
  aLambdaL[22] = 0.74;    aMuL[22] = 18.09;
  aLambdaL[23] = 0.76;    aMuL[23] = 18.41;
  aLambdaL[24] = 0.79;    aMuL[24] = 18.77;
  aLambdaL[25] = 0.83;    aMuL[25] = 19.54;
  aLambdaL[26] = 0.86;    aMuL[26] = 20.77;
  aLambdaL[27] = 0.90;    aMuL[27] = 22.53;
  aLambdaL[28] = 0.95;    aMuL[28] = 24.94;
  aLambdaL[29] = 1.01;    aMuL[29] = 27.71;
  aLambdaL[30] = 1.08;    aMuL[30] = 31.87;
  aLambdaL[31] = 1.17;    aMuL[31] = 37.81;
  aLambdaL[32] = 1.28;    aMuL[32] = 46.2;
  aLambdaL[33] = 1.43;    aMuL[33] = 58.78;
  aLambdaL[34] = 1.65;    aMuL[34] = 78.84;
  aLambdaL[35] = 2.02;    aMuL[35] = 115.04;
  aLambdaL[36] = 2.86;    aMuL[36] = 194.75;
  aLambdaL[37] = 3.20;    aMuL[37] = 232.62;
  aLambdaL[38] = 3.69;    aMuL[38] = 270.01;
  aLambdaL[39] = 4.52;    aMuL[39] = 332.32;
  aLambdaL[40] = 6.39;    aMuL[40] = 481.86;
  aLambdaL[41] = 9.04;    aMuL[41] = 689.56;
  aLambdaL[42] = 12.78;  aMuL[42] = 965.81;
  aLambdaL[43] = 28.59;  aMuL[43] = 2284.7;

  nValues = mini(44, nArrayLen);
  for (i=0; i < nValues; i++)
  { pLmbd[i] = aLambdaL[i];
    pMu  [i] = aMuL    [i];
  }

  return(nValues);
}


/* For silicon,
Total cross section A.K. Freund, NIM 213 (1983) 495-501 */
long Silicon(double *pLmbd, double *pMu, const long nArrayLen)
{
  long i=0, nValues=0; 

  /* Wavelength Ang;   Attenuation, cm^-1 */
  aLambdaL[ 0] =  0.4;  aMuL[ 0] = 0.108;
  aLambdaL[ 1] =  0.5;  aMuL[ 1] = 0.085;
  aLambdaL[ 2] =  1.0;  aMuL[ 2] = 0.0375;
  aLambdaL[ 3] =  1.3;  aMuL[ 3] = 0.03;
  aLambdaL[ 4] =  1.5;  aMuL[ 4] = 0.025;
  aLambdaL[ 5] =  3.0;  aMuL[ 5] = 0.025;
  aLambdaL[ 6] =  4.5;  aMuL[ 6] = 0.03;
  aLambdaL[ 7] =  6.0;  aMuL[ 7] = 0.0375;
  aLambdaL[ 8] =  8.0;  aMuL[ 8] = 0.05;
  aLambdaL[ 9] = 10.0;  aMuL[ 9] = 0.06;
  aLambdaL[10] = 20.0;  aMuL[10] = 0.11;

  nValues = mini(11, nArrayLen);
  for (i=0; i < nValues; i++)
  { pLmbd[i] = aLambdaL[i];
    pMu  [i] = aMuL    [i];
  }

  return(nValues);
}


long Vacuum(double *aLambdaL, double *aMuL)
{
  /* Wavelength A;   Attenuation, cm^-1 */
  aLambdaL[0] =    0.0;  aMuL[0] = 0.0;
  aLambdaL[1] =  999.0;  aMuL[1] = 0.0;

  return(2);
}


long IdealAbsorp(double *aLambdaL, double *aMuL)
{
  /* Wavelength A;   Attenuation, cm^-1 */
  aLambdaL[0] =    0.0;  aMuL[0] = 10000.0;
  aLambdaL[1] =  999.0;  aMuL[1] = 10000.0;

  return(2);
}

/****************************************************************/
/* Local function: initialization                               */
/****************************************************************/
static void InitArrays(double* Lambda, double* Mu, const long nArrayLen)
{
  int i;

  for(i=0; i < nArrayLen; i++)
  {
    Lambda[i] = 0.0;
    Mu    [i] = 0.0;
  }
}

