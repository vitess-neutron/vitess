/*********************************************************************************/
/* bender_inter_data.c                                                           */
/*                                                                               */
/* Functions to calculate attenuation in materials as a function of wavelength   */
/*                                                                               */
/* "key"	"description"                                                          */
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


static void   InitArrays();
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
    default          : AbsMat_ID2Txt(sMat, eMatID);
                       Error2("Attenuation for this material not yet implemented", sMat);
  }

  return mu_tot;
}


double Interpolation(double Wave,      long Material, 
                     double WAV1[MAX_MU], double MU1[MAX_MU], long nValFile)
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
        WAV[i-1] = WAV1[i];
        MU [i-1] = MU1 [i];
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
void Gadolinium(double WAV[44], double MU[44], long *nVal)
{
  *nVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  WAV[ 0] = 0.29; 	MU[ 0] = 1.06;
  WAV[ 1] = 0.30;		MU[ 1] = 1.21;	
  WAV[ 2] = 0.32;		MU[ 2] = 1.66;
  WAV[ 3] = 0.34;		MU[ 3] = 2.12;
  WAV[ 4] = 0.37;		MU[ 4] = 3.03;
  WAV[ 5] = 0.39;		MU[ 5] = 3.93;
  WAV[ 6] = 0.40;		MU[ 6] = 5.87;
  WAV[ 7] = 0.41;		MU[ 7] = 6.53;
  WAV[ 8] = 0.43;		MU[ 8] = 7.22;
  WAV[ 9] = 0.44;		MU[ 9] = 8.36;
  WAV[10] = 0.45;		MU[10] = 9.3;
  WAV[11] = 0.48;		MU[11] = 12.87;
  WAV[12] = 0.52;		MU[12] = 18.32;
  WAV[13] = 0.54;		MU[13] = 21.58;
  WAV[14] = 0.56;		MU[14] = 25.79;
  WAV[15] = 0.58;		MU[15] = 31.43;
  WAV[16] = 0.61;		MU[16] = 39.1;
  WAV[17] = 0.64;		MU[17] = 49.9;
  WAV[18] = 0.66;		MU[18] = 56.76;
  WAV[19] = 0.67;		MU[19] = 65.25;
  WAV[20] = 0.69;		MU[20] = 75.53;
  WAV[21] = 0.71;		MU[21] = 88.23;
  WAV[22] = 0.74;		MU[22] = 104.08;
  WAV[23] = 0.76;		MU[23] = 123.92;
  WAV[24] = 0.79;		MU[24] = 149.23;
  WAV[25] = 0.83;		MU[25] = 181.79;
  WAV[26] = 0.86;		MU[26] = 224.31;
  WAV[27] = 0.9;		MU[27] = 280.66;
  WAV[28] = 0.95;		MU[28] = 355.24;
  WAV[29] = 1.01;		MU[29] = 453.86;
  WAV[30] = 1.08;		MU[30] = 583.29;
  WAV[31] = 1.17;		MU[31] = 747.98;
  WAV[32] = 1.28;		MU[32] = 944.71;
  WAV[33] = 1.43;		MU[33] = 1161.75;
  WAV[34] = 1.65;		MU[34] = 1384.43;
  WAV[35] = 2.02;		MU[35] = 1630.73;
  WAV[36] = 2.86;		MU[36] = 2083.02;
  WAV[37] = 3.20;		MU[37] = 2268.93;
  WAV[38] = 3.69;		MU[38] = 2546.22;
  WAV[39] = 4.52;		MU[39] = 3028.2;
  WAV[40] = 6.39;		MU[40] = 4139.39;
  WAV[41] = 9.04;		MU[41] = 5749.4;
  WAV[42] = 12.78;	MU[42] = 8170.2;
  WAV[43] = 28.59;	MU[43] = 16945.6;
}


/* For cadmium */
void Cadmium(double WAV[44], double MU[44], long *nVal)
{
  *nVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  WAV[0] = 0.29; 		MU[0] = 1.02;
  WAV[1] = 0.30;		MU[1] = 1.27;	
  WAV[2] = 0.32;		MU[2] = 1.62;
  WAV[3] = 0.34;		MU[3] = 2.31;
  WAV[4] = 0.37;		MU[4] = 3.7;
  WAV[5] = 0.39;		MU[5] = 4.63;
  WAV[6] = 0.40;		MU[6] = 6.95;
  WAV[7] = 0.41;		MU[7] = 8.1;
  WAV[8] = 0.43;		MU[8] = 9.26;
  WAV[9] = 0.44;		MU[9] = 12.73;
  WAV[10] = 0.45;		MU[10] = 16.21;
  WAV[11] = 0.48;		MU[11] = 25.47;
  WAV[12] = 0.52;		MU[12] = 46.3;
  WAV[13] = 0.54;		MU[13] = 64.82;
  WAV[14] = 0.56;		MU[14] = 92.6;
  WAV[15] = 0.58;		MU[15] = 138.9;
  WAV[16] = 0.61;		MU[16] = 208.35;
  WAV[17] = 0.64;		MU[17] = 287.06;
  WAV[18] = 0.66;		MU[18] = 314.84;
  WAV[19] = 0.67;		MU[19] = 324.1;
  WAV[20] = 0.69;		MU[20] = 347.25;
  WAV[21] = 0.71;		MU[21] = 331.05;
  WAV[22] = 0.74;		MU[22] = 300.95;
  WAV[23] = 0.76;		MU[23] = 277.8;
  WAV[24] = 0.79;		MU[24] = 245.39;
  WAV[25] = 0.83;		MU[25] = 208.35;
  WAV[26] = 0.86;		MU[26] = 185.2;
  WAV[27] = 0.90;		MU[27] = 162.05;
  WAV[28] = 0.95;		MU[28] = 148.16;
  WAV[29] = 1.01;		MU[29] = 129.64;
  WAV[30] = 1.08;		MU[30] = 120.38;
  WAV[31] = 1.17;		MU[31] = 115.75;
  WAV[32] = 1.28;		MU[32] = 115.29;
  WAV[33] = 1.43;		MU[33] = 111.12;
  WAV[34] = 1.65;		MU[34] = 115.75;
  WAV[35] = 2.02;		MU[35] = 125.01;
  WAV[36] = 2.86;		MU[36] = 152.79;
  WAV[37] = 3.20;		MU[37] = 175.94;
  WAV[38] = 3.69;		MU[38] = 196.78;
  WAV[39] = 4.52;		MU[39] = 236.13;
  WAV[40] = 6.39;		MU[40] = 324.1;
  WAV[41] = 9.04;		MU[41] = 463.0;
  WAV[42] = 12.78;	MU[42] = 648.2;
  WAV[43] = 28.59;	MU[43] = 1389.0;
}


/* For Bor10 */
void Bor10(double WAV[44], double MU[44], long *nVal)
{
  *nVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  WAV[0] = 0.29; 		MU[0] = 79.07;
  WAV[1] = 0.30;		MU[1] = 83.35;	
  WAV[2] = 0.32;		MU[2] = 88.4;
  WAV[3] = 0.34;		MU[3] = 94.51;
  WAV[4] = 0.37;		MU[4] = 102.08;
  WAV[5] = 0.39;		MU[5] = 106.62;
  WAV[6] = 0.40;		MU[6] = 111.82;
  WAV[7] = 0.41;		MU[7] = 114.73;
  WAV[8] = 0.43;		MU[8] = 117.87;
  WAV[9] = 0.44;		MU[9] = 121.29;
  WAV[10] = 0.45;		MU[10] = 125.02;
  WAV[11] = 0.48;		MU[11] = 133.65;
  WAV[12] = 0.52;		MU[12] = 144.36;
  WAV[13] = 0.54;		MU[13] = 149.43;
  WAV[14] = 0.56;		MU[14] = 155.07;
  WAV[15] = 0.58;		MU[15] = 161.4;
  WAV[16] = 0.61;		MU[16] = 168.58;
  WAV[17] = 0.64;		MU[17] = 176.81;
  WAV[18] = 0.66;		MU[18] = 181.4;
  WAV[19] = 0.67;		MU[19] = 186.37;
  WAV[20] = 0.69;		MU[20] = 191.77;
  WAV[21] = 0.71;		MU[21] = 197.68;
  WAV[22] = 0.74;		MU[22] = 204.16;
  WAV[23] = 0.76;		MU[23] = 211.32;
  WAV[24] = 0.79;		MU[24] = 219.3;
  WAV[25] = 0.83;		MU[25] = 228.26;
  WAV[26] = 0.86;		MU[26] = 238.41;
  WAV[27] = 0.90;		MU[27] = 250.04;
  WAV[28] = 0.95;		MU[28] = 263.57;
  WAV[29] = 1.01;		MU[29] = 279.56;
  WAV[30] = 1.08;		MU[30] = 298.86;
  WAV[31] = 1.17;		MU[31] = 322.8;
  WAV[32] = 1.28;		MU[32] = 353.61;
  WAV[33] = 1.43;		MU[33] = 395.35;
  WAV[34] = 1.65;		MU[34] = 456.51;
  WAV[35] = 2.02;		MU[35] = 559.11;
  WAV[36] = 2.86;		MU[36] = 790.7;
  WAV[37] = 3.20;		MU[37] = 884.03;
  WAV[38] = 3.69;		MU[38] = 1020.79;
  WAV[39] = 4.52;		MU[39] = 1250.21;
  WAV[40] = 6.39;		MU[40] = 1768.06;
  WAV[41] = 9.04;		MU[41] = 2500.42;
  WAV[42] = 12.78;	MU[42] = 3536.13;
  WAV[43] = 28.59;	MU[43] = 7907.02;
}


/* For Eu */
void Eu(double WAV[44], double MU[44], long *nVal)
{
  *nVal = 44;

  /* Wavelength A;   Attenuation, cm^-1 */
  WAV[0] = 0.29; 		MU[0] = 14.54;
  WAV[1] = 0.30;		MU[1] = 6.23;	
  WAV[2] = 0.32;		MU[2] = 5.19;
  WAV[3] = 0.34;		MU[3] = 8.31;
  WAV[4] = 0.37;		MU[4] = 21.31;
  WAV[5] = 0.39;		MU[5] = 45.96;
  WAV[6] = 0.40;		MU[6] = 133.34;
  WAV[7] = 0.41;		MU[7] = 220.55;
  WAV[8] = 0.43;		MU[8] = 243.41;
  WAV[9] = 0.44;		MU[9] = 173.05;
  WAV[10] = 0.45;		MU[10] = 101.58;
  WAV[11] = 0.48;		MU[11] = 66.44;
  WAV[12] = 0.52;		MU[12] = 57.02;
  WAV[13] = 0.54;		MU[13] = 41.74;
  WAV[14] = 0.56;		MU[14] = 31.44;
  WAV[15] = 0.58;		MU[15] = 25.45;
  WAV[16] = 0.61;		MU[16] = 21.77;
  WAV[17] = 0.64;		MU[17] = 19.38;
  WAV[18] = 0.66;		MU[18] = 18.8;
  WAV[19] = 0.67;		MU[19] = 18.19;
  WAV[20] = 0.69;		MU[20] = 17.86;
  WAV[21] = 0.71;		MU[21] = 17.81;
  WAV[22] = 0.74;		MU[22] = 18.09;
  WAV[23] = 0.76;		MU[23] = 18.41;
  WAV[24] = 0.79;		MU[24] = 18.77;
  WAV[25] = 0.83;		MU[25] = 19.54;
  WAV[26] = 0.86;		MU[26] = 20.77;
  WAV[27] = 0.90;		MU[27] = 22.53;
  WAV[28] = 0.95;		MU[28] = 24.94;
  WAV[29] = 1.01;		MU[29] = 27.71;
  WAV[30] = 1.08;		MU[30] = 31.87;
  WAV[31] = 1.17;		MU[31] = 37.81;
  WAV[32] = 1.28;		MU[32] = 46.2;
  WAV[33] = 1.43;		MU[33] = 58.78;
  WAV[34] = 1.65;		MU[34] = 78.84;
  WAV[35] = 2.02;		MU[35] = 115.04;
  WAV[36] = 2.86;		MU[36] = 194.75;
  WAV[37] = 3.20;		MU[37] = 232.62;
  WAV[38] = 3.69;		MU[38] = 270.01;
  WAV[39] = 4.52;		MU[39] = 332.32;
  WAV[40] = 6.39;		MU[40] = 481.86;
  WAV[41] = 9.04;		MU[41] = 689.56;
  WAV[42] = 12.78;	MU[42] = 965.81;
  WAV[43] = 28.59;	MU[43] = 2284.7;
}


/* For silicon, 
Total cross section A.K. Freund, NIM 213 (1983) 495-501 */
void Silicon(double WAV[44], double MU[44], long *nVal)
{
  *nVal = 11;

  /* Wavelength A;   Attenuation, cm^-1 */
  WAV[ 0] =  0.4;  MU[ 0] = 0.108;
  WAV[ 1] =  0.5;	 MU[ 1] = 0.085;
  WAV[ 2] =  1.0;	 MU[ 2] = 0.0375;
  WAV[ 3] =  1.3;	 MU[ 3] = 0.03;
  WAV[ 4] =  1.5;	 MU[ 4] = 0.025;
  WAV[ 5] =  3.0;	 MU[ 5] = 0.025;
  WAV[ 6] =  4.5;	 MU[ 6] = 0.03;
  WAV[ 7] =  6.0;	 MU[ 7] = 0.0375;
  WAV[ 8] =  8.0;	 MU[ 8] = 0.05;
  WAV[ 9] = 10.0;	 MU[ 9] = 0.06;
  WAV[10] = 20.0;	 MU[10] = 0.11;
}

