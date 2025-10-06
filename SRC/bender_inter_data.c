/*********************************************************************************/
/* bender_inter_data.c                                                           */
/*                                                                               */
/* Functions to calculate attenuation in materials as a function of wavelength   */
/*                                                                               */
/* "key"  "description"                                                          */
/*   0   FROM FILE, which the user has to give as input                          */
/*   1   GADOLINIUM                                                              */
/*   2   CADMIUM                                                                 */
/*   3   BOR10                                                                   */
/*   4   EU                                                                      */
/*   5   SILICON                                                                 */
/*   6   VACUUM                                                                  */
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


static void InitArrays(double Lambda[MAX_MU], double Mu[MAX_MU]);
static double Interpol(double Wavelen, double WAV1[MAX_MU], double MU1[MAX_MU], long nValFile);


/****************************************************************/
/* Attenuation as a function of wavelength for some materials   */
/****************************************************************/
double AttenuationAbs(const double Lambda, const VtAbsMat eMatID)
{
  char   sMat[21]="";
  long   nValues=0;                  // number of data points
  double mu_tot=0.0,                 // total attenuation
         aLmbd[MAX_MU], aMu[MAX_MU]; // arrays for data (wavelength, attenuation)

  InitArrays(aLmbd, aMu);

  switch (eMatID)
  {
    case VT_ABS_IDEAL:                                   mu_tot=1.0e99;           break;
    case VT_ABS_GD   : Gadolinium(aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_ABS_B10  : Bor10     (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_ABS_CD   : Cadmium   (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_ABS_EU   : Eu        (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_NO_MAT   : Error ("material for attenuation not given");    break;
    case VT_ABS_FILE : Error ("Wrong function: Use function 'Interpolation' for attenuation from file");   break;
    default          : AbsMat_ID2Txt(sMat, eMatID);
                       Error2("Attenuation for this material not yet implemented", sMat);
  }

  return mu_tot;
}


double AttenuationMirr(const double Lambda, const VtMirrMat eMatID)
{
  char   sMat[21]="";
  long   nValues=0;                  // number of data points
  double mu_tot=0.0,                 // total attenuation
         aLmbd[MAX_MU], aMu[MAX_MU]; // arrays for data (wavelength, attenuation)

  InitArrays(aLmbd, aMu);

  switch (eMatID)
  {
    case VT_MIRR_VACUUM:                                 mu_tot=0.0;                                   break;
    case VT_MIRR_SI    : Silicon (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_MIRR_B4C   : Bor10   (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues) * 0.8 * 2.52/2.34; break; // ratio of atom numbers and densities, C contribution ignored
    // case VT_MIRR_SAPPH : Sapphire(aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    // case VT_MIRR_GLASS : Glass   (aLmbd, aMu, &nValues); mu_tot=Interpol(Lambda, aLmbd, aMu, nValues); break;
    case VT_NO_MIRR_MAT: Error ("material for attenuation not given");    break;
    case VT_MIRR_OTHER : Error ("Wrong function: Use absorption and scattering cross-section to determine the attenuation or the function 'Interpolation' for attenuation from file");   break;
    default          : MirrMat_ID2Txt(sMat, eMatID);
                       Error2("Attenuation for this material not yet implemented", sMat);
  }

  return mu_tot;
}

double Interpolation(double Wave, long Material, double *WavF, double *MuF, long nValFile)
{
  double WAV[MAX_MU];              // array of wavelengths         [Ang]
  double MU [MAX_MU];              // array of attenuation values  [1/cm]
  double X1=0.0, X2=0.0,           // wavelength
         Y1=0.0, Y2=0.0,           // and attenuation values of the interpolation interval
         Mu;                       // interpolated attenuation value [1/cm]
  long   nVal=0,                   // number of given wavelength and attenuation values
         i,                        // index ....
         iIpn=1;                   // index of left side of interval for interpolation
  short  bVacuum=FALSE,            // flag: no window (pane) material
         bIdeal =FALSE;            // flag: perfectly absorbing material

  // Initialize
  // ----------
  InitArrays(WAV, MU);

  // Fill the array with data
  // ------------------------
  switch (Material)
  {
    case 0:       // data from file
      for(i = 1; i <= nValFile; i++)
      {
        WAV[i-1] = WavF[i];
        MU [i-1] = MuF [i];
      }
      nVal = nValFile;
      break;

    case 1:
      Gadolinium(WAV, MU, &nVal);
      break;

    case 2:
      Cadmium(WAV, MU, &nVal);
      break;

    case 3:
      Bor10(WAV, MU, &nVal);
      break;

    case 4:
      Eu(WAV, MU, &nVal);
      break;

    case 5:
      Silicon(WAV, MU, &nVal);
      break;

    case 6:      // vacuum
      bVacuum = TRUE;
      break;

    case 99:       // ideal absorber
      bIdeal = TRUE;
      break;

    default:
      Error("No such material for interpolation of attenuation data");
      break;
  }

  if (bVacuum == TRUE)
  {
    Mu = 0.0;
  }
  else if (bIdeal == TRUE)
  {
    Mu = 1.0e99;
    return(Mu);
  }
  else
  {
    for(i = 0; i < (nVal-1); i++)
    {
      if ((Wave >= WAV[i]) && (Wave <= WAV[i+1]))
      {
        X1 = WAV[i];
        X2 = WAV[i+1];
        Y1 = MU[i];
        Y2 = MU[i+1];
        iIpn = i;
        break;
      }
    }

    // Check interval
    // --------------
    if (!((Wave >= WAV[iIpn])&&(Wave <= WAV[iIpn+1])))
    {
      // fprintf(LogFilePtr,"ERROR: Wavelength of neutron is out of the data interval. Exit!\n");
      return(-10000.0);
    }

    // Linear interpolation
    // --------------------
    if (X1 != X2)
    {
      Mu = Y1 + ((Y2-Y1)*(Wave-X1)/(X2-X1));
    }
    else
    {
      // fprintf(LogFilePtr,"ERROR: Interpolation: Incorrect values for wavelength \n");
      Mu = -10000.0;
    }
  }

  return(Mu);
}


/****************************************************************/
/* Local functions: interpolation and initialization            */
/****************************************************************/
void InitArrays(double Lambda[MAX_MU], double Mu[MAX_MU])
{
  int i;
  for(i=0; i < MAX_MU; i++)
  {
    Lambda[i] = 0.0;
    Mu    [i] = 0.0;
  }
}


double Interpol(double Wavelen, double WAV[MAX_MU], double MU[MAX_MU], long nVal)
{
  double X1=0.0, X2=0.0,           // wavelength
         Y1=0.0, Y2=0.0,           // and attenuation values of the interpolation interval
         Mu;                       // interpolated attenuation value [1/cm]
  long   i,                        // index ....
         iIpn=1;                   // index of left side of interval for interpolation

  for(i = 0; i < (nVal-1); i++)
  {
    if ((Wavelen >= WAV[i]) && (Wavelen <= WAV[i+1]))
    {
      X1 = WAV[i];
      X2 = WAV[i+1];
      Y1 = MU[i];
      Y2 = MU[i+1];
      iIpn = i;
      break;
    }
  }

  // Check interval
  // --------------
  if (!((Wavelen >= WAV[iIpn])&&(Wavelen <= WAV[iIpn+1])))
  {
    CountMessage(ALL_L_RANGE_TOO_SMALL);
    return(-10000.0);
  }

  // Linear interpolation
  // --------------------
  if (X1 != X2)
  {
    Mu = Y1 + ((Y2-Y1)*(Wavelen-X1)/(X2-X1));
  }
  else
  {
    CountMessage(MON_ZERO_BIN_SIZE);
    Mu = -10000.0;
  }

  return(Mu);
}


/****************************************************************/
/* Local functions: mu(lambda) values for different materials   */
/****************************************************************/
/* For gadolinium */
void Gadolinium(double *wavelen, double *mu, long *pVal)
{
  *pVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  wavelen[ 0] = 0.29;   mu[ 0] = 1.06;
  wavelen[ 1] = 0.30;    mu[ 1] = 1.21;
  wavelen[ 2] = 0.32;    mu[ 2] = 1.66;
  wavelen[ 3] = 0.34;    mu[ 3] = 2.12;
  wavelen[ 4] = 0.37;    mu[ 4] = 3.03;
  wavelen[ 5] = 0.39;    mu[ 5] = 3.93;
  wavelen[ 6] = 0.40;    mu[ 6] = 5.87;
  wavelen[ 7] = 0.41;    mu[ 7] = 6.53;
  wavelen[ 8] = 0.43;    mu[ 8] = 7.22;
  wavelen[ 9] = 0.44;    mu[ 9] = 8.36;
  wavelen[10] = 0.45;    mu[10] = 9.3;
  wavelen[11] = 0.48;    mu[11] = 12.87;
  wavelen[12] = 0.52;    mu[12] = 18.32;
  wavelen[13] = 0.54;    mu[13] = 21.58;
  wavelen[14] = 0.56;    mu[14] = 25.79;
  wavelen[15] = 0.58;    mu[15] = 31.43;
  wavelen[16] = 0.61;    mu[16] = 39.1;
  wavelen[17] = 0.64;    mu[17] = 49.9;
  wavelen[18] = 0.66;    mu[18] = 56.76;
  wavelen[19] = 0.67;    mu[19] = 65.25;
  wavelen[20] = 0.69;    mu[20] = 75.53;
  wavelen[21] = 0.71;    mu[21] = 88.23;
  wavelen[22] = 0.74;    mu[22] = 104.08;
  wavelen[23] = 0.76;    mu[23] = 123.92;
  wavelen[24] = 0.79;    mu[24] = 149.23;
  wavelen[25] = 0.83;    mu[25] = 181.79;
  wavelen[26] = 0.86;    mu[26] = 224.31;
  wavelen[27] = 0.9;    mu[27] = 280.66;
  wavelen[28] = 0.95;    mu[28] = 355.24;
  wavelen[29] = 1.01;    mu[29] = 453.86;
  wavelen[30] = 1.08;    mu[30] = 583.29;
  wavelen[31] = 1.17;    mu[31] = 747.98;
  wavelen[32] = 1.28;    mu[32] = 944.71;
  wavelen[33] = 1.43;    mu[33] = 1161.75;
  wavelen[34] = 1.65;    mu[34] = 1384.43;
  wavelen[35] = 2.02;    mu[35] = 1630.73;
  wavelen[36] = 2.86;    mu[36] = 2083.02;
  wavelen[37] = 3.20;    mu[37] = 2268.93;
  wavelen[38] = 3.69;    mu[38] = 2546.22;
  wavelen[39] = 4.52;    mu[39] = 3028.2;
  wavelen[40] = 6.39;    mu[40] = 4139.39;
  wavelen[41] = 9.04;    mu[41] = 5749.4;
  wavelen[42] = 12.78;  mu[42] = 8170.2;
  wavelen[43] = 28.59;  mu[43] = 16945.6;
}


/* For cadmium */
void Cadmium(double *wavelen, double *mu, long *pVal)
{
  *pVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  wavelen[0] = 0.29;     mu[0] = 1.02;
  wavelen[1] = 0.30;    mu[1] = 1.27;
  wavelen[2] = 0.32;    mu[2] = 1.62;
  wavelen[3] = 0.34;    mu[3] = 2.31;
  wavelen[4] = 0.37;    mu[4] = 3.7;
  wavelen[5] = 0.39;    mu[5] = 4.63;
  wavelen[6] = 0.40;    mu[6] = 6.95;
  wavelen[7] = 0.41;    mu[7] = 8.1;
  wavelen[8] = 0.43;    mu[8] = 9.26;
  wavelen[9] = 0.44;    mu[9] = 12.73;
  wavelen[10] = 0.45;    mu[10] = 16.21;
  wavelen[11] = 0.48;    mu[11] = 25.47;
  wavelen[12] = 0.52;    mu[12] = 46.3;
  wavelen[13] = 0.54;    mu[13] = 64.82;
  wavelen[14] = 0.56;    mu[14] = 92.6;
  wavelen[15] = 0.58;    mu[15] = 138.9;
  wavelen[16] = 0.61;    mu[16] = 208.35;
  wavelen[17] = 0.64;    mu[17] = 287.06;
  wavelen[18] = 0.66;    mu[18] = 314.84;
  wavelen[19] = 0.67;    mu[19] = 324.1;
  wavelen[20] = 0.69;    mu[20] = 347.25;
  wavelen[21] = 0.71;    mu[21] = 331.05;
  wavelen[22] = 0.74;    mu[22] = 300.95;
  wavelen[23] = 0.76;    mu[23] = 277.8;
  wavelen[24] = 0.79;    mu[24] = 245.39;
  wavelen[25] = 0.83;    mu[25] = 208.35;
  wavelen[26] = 0.86;    mu[26] = 185.2;
  wavelen[27] = 0.90;    mu[27] = 162.05;
  wavelen[28] = 0.95;    mu[28] = 148.16;
  wavelen[29] = 1.01;    mu[29] = 129.64;
  wavelen[30] = 1.08;    mu[30] = 120.38;
  wavelen[31] = 1.17;    mu[31] = 115.75;
  wavelen[32] = 1.28;    mu[32] = 115.29;
  wavelen[33] = 1.43;    mu[33] = 111.12;
  wavelen[34] = 1.65;    mu[34] = 115.75;
  wavelen[35] = 2.02;    mu[35] = 125.01;
  wavelen[36] = 2.86;    mu[36] = 152.79;
  wavelen[37] = 3.20;    mu[37] = 175.94;
  wavelen[38] = 3.69;    mu[38] = 196.78;
  wavelen[39] = 4.52;    mu[39] = 236.13;
  wavelen[40] = 6.39;    mu[40] = 324.1;
  wavelen[41] = 9.04;    mu[41] = 463.0;
  wavelen[42] = 12.78;  mu[42] = 648.2;
  wavelen[43] = 28.59;  mu[43] = 1389.0;
}


/* For Bor10 */
void Bor10(double *wavelen, double *mu, long *pVal)
{
  *pVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  wavelen[0] = 0.29;     mu[0] = 79.07;
  wavelen[1] = 0.30;    mu[1] = 83.35;
  wavelen[2] = 0.32;    mu[2] = 88.4;
  wavelen[3] = 0.34;    mu[3] = 94.51;
  wavelen[4] = 0.37;    mu[4] = 102.08;
  wavelen[5] = 0.39;    mu[5] = 106.62;
  wavelen[6] = 0.40;    mu[6] = 111.82;
  wavelen[7] = 0.41;    mu[7] = 114.73;
  wavelen[8] = 0.43;    mu[8] = 117.87;
  wavelen[9] = 0.44;    mu[9] = 121.29;
  wavelen[10] = 0.45;    mu[10] = 125.02;
  wavelen[11] = 0.48;    mu[11] = 133.65;
  wavelen[12] = 0.52;    mu[12] = 144.36;
  wavelen[13] = 0.54;    mu[13] = 149.43;
  wavelen[14] = 0.56;    mu[14] = 155.07;
  wavelen[15] = 0.58;    mu[15] = 161.4;
  wavelen[16] = 0.61;    mu[16] = 168.58;
  wavelen[17] = 0.64;    mu[17] = 176.81;
  wavelen[18] = 0.66;    mu[18] = 181.4;
  wavelen[19] = 0.67;    mu[19] = 186.37;
  wavelen[20] = 0.69;    mu[20] = 191.77;
  wavelen[21] = 0.71;    mu[21] = 197.68;
  wavelen[22] = 0.74;    mu[22] = 204.16;
  wavelen[23] = 0.76;    mu[23] = 211.32;
  wavelen[24] = 0.79;    mu[24] = 219.3;
  wavelen[25] = 0.83;    mu[25] = 228.26;
  wavelen[26] = 0.86;    mu[26] = 238.41;
  wavelen[27] = 0.90;    mu[27] = 250.04;
  wavelen[28] = 0.95;    mu[28] = 263.57;
  wavelen[29] = 1.01;    mu[29] = 279.56;
  wavelen[30] = 1.08;    mu[30] = 298.86;
  wavelen[31] = 1.17;    mu[31] = 322.8;
  wavelen[32] = 1.28;    mu[32] = 353.61;
  wavelen[33] = 1.43;    mu[33] = 395.35;
  wavelen[34] = 1.65;    mu[34] = 456.51;
  wavelen[35] = 2.02;    mu[35] = 559.11;
  wavelen[36] = 2.86;    mu[36] = 790.7;
  wavelen[37] = 3.20;    mu[37] = 884.03;
  wavelen[38] = 3.69;    mu[38] = 1020.79;
  wavelen[39] = 4.52;    mu[39] = 1250.21;
  wavelen[40] = 6.39;    mu[40] = 1768.06;
  wavelen[41] = 9.04;    mu[41] = 2500.42;
  wavelen[42] = 12.78;  mu[42] = 3536.13;
  wavelen[43] = 28.59;  mu[43] = 7907.02;
}


/* For Eu */
void Eu(double *wavelen, double *mu, long *pVal)
{
  *pVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  wavelen[0] = 0.29;     mu[0] = 14.54;
  wavelen[1] = 0.30;    mu[1] = 6.23;
  wavelen[2] = 0.32;    mu[2] = 5.19;
  wavelen[3] = 0.34;    mu[3] = 8.31;
  wavelen[4] = 0.37;    mu[4] = 21.31;
  wavelen[5] = 0.39;    mu[5] = 45.96;
  wavelen[6] = 0.40;    mu[6] = 133.34;
  wavelen[7] = 0.41;    mu[7] = 220.55;
  wavelen[8] = 0.43;    mu[8] = 243.41;
  wavelen[9] = 0.44;    mu[9] = 173.05;
  wavelen[10] = 0.45;    mu[10] = 101.58;
  wavelen[11] = 0.48;    mu[11] = 66.44;
  wavelen[12] = 0.52;    mu[12] = 57.02;
  wavelen[13] = 0.54;    mu[13] = 41.74;
  wavelen[14] = 0.56;    mu[14] = 31.44;
  wavelen[15] = 0.58;    mu[15] = 25.45;
  wavelen[16] = 0.61;    mu[16] = 21.77;
  wavelen[17] = 0.64;    mu[17] = 19.38;
  wavelen[18] = 0.66;    mu[18] = 18.8;
  wavelen[19] = 0.67;    mu[19] = 18.19;
  wavelen[20] = 0.69;    mu[20] = 17.86;
  wavelen[21] = 0.71;    mu[21] = 17.81;
  wavelen[22] = 0.74;    mu[22] = 18.09;
  wavelen[23] = 0.76;    mu[23] = 18.41;
  wavelen[24] = 0.79;    mu[24] = 18.77;
  wavelen[25] = 0.83;    mu[25] = 19.54;
  wavelen[26] = 0.86;    mu[26] = 20.77;
  wavelen[27] = 0.90;    mu[27] = 22.53;
  wavelen[28] = 0.95;    mu[28] = 24.94;
  wavelen[29] = 1.01;    mu[29] = 27.71;
  wavelen[30] = 1.08;    mu[30] = 31.87;
  wavelen[31] = 1.17;    mu[31] = 37.81;
  wavelen[32] = 1.28;    mu[32] = 46.2;
  wavelen[33] = 1.43;    mu[33] = 58.78;
  wavelen[34] = 1.65;    mu[34] = 78.84;
  wavelen[35] = 2.02;    mu[35] = 115.04;
  wavelen[36] = 2.86;    mu[36] = 194.75;
  wavelen[37] = 3.20;    mu[37] = 232.62;
  wavelen[38] = 3.69;    mu[38] = 270.01;
  wavelen[39] = 4.52;    mu[39] = 332.32;
  wavelen[40] = 6.39;    mu[40] = 481.86;
  wavelen[41] = 9.04;    mu[41] = 689.56;
  wavelen[42] = 12.78;  mu[42] = 965.81;
  wavelen[43] = 28.59;  mu[43] = 2284.7;
}


/* For silicon,
Total cross section A.K. Freund, NIM 213 (1983) 495-501 */
void Silicon(double *wavelen, double *mu, long *pVal)
{
  *pVal = 11;

  /* Wavelength A;   Attenuation, cm^-1 */
  wavelen[ 0] =  0.4;  mu[ 0] = 0.108;
  wavelen[ 1] =  0.5;   mu[ 1] = 0.085;
  wavelen[ 2] =  1.0;   mu[ 2] = 0.0375;
  wavelen[ 3] =  1.3;   mu[ 3] = 0.03;
  wavelen[ 4] =  1.5;   mu[ 4] = 0.025;
  wavelen[ 5] =  3.0;   mu[ 5] = 0.025;
  wavelen[ 6] =  4.5;   mu[ 6] = 0.03;
  wavelen[ 7] =  6.0;   mu[ 7] = 0.0375;
  wavelen[ 8] =  8.0;   mu[ 8] = 0.05;
  wavelen[ 9] = 10.0;   mu[ 9] = 0.06;
  wavelen[10] = 20.0;   mu[10] = 0.11;
}
