/*******************************************************************************************/
/* Tool MirrorCoating:                                                                     */
/*  Generating reflectivity files for mirror coating as used in the                        */
/*  modules 'guide' and 'bender' from parameters m, R_0, R_m, Q_c and W                    */
/*                                                                                         */
/* The free non-commercial use of these routines is granted provided due credit is given   */
/* to the authors.                                                                         */
/*                                                                                         */
/* 1.0  Sep 2003  K. Lieutenant  initial version                                           */
/* 1.1  Nov 2003  K. Lieutenant  more precise Q-value given                                */
/* 1.2  Mar 2004  K. Lieutenant  files written to parameter directory or install_dir/FILES;*/
/*                               heading                                                   */
/* 1.3  Nov 2005  K. Lieutenant  parameter W added                                         */
/* 1.4  May 2012  K. Lieutenant  parameter beta added                                      */
/* 1.5  Sep 2012  K. Lieutenant  new treatment of case m<1 and correction: output formula  */
/* 2.0  Sep 2019  K. Lieutenant  new standard reflect., strict use of 'general.h/c', header*/
/* 2.1  Mar 2020  K. Lieutenant  new central parameters and functions                      */
/*******************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "init.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define PI       3.14159265358979323846


/******************************/
/** Prototypes               **/
/******************************/
short  GetShort  (const char* pText);                // Reads short value from stdin   
double GetDouble (const char* pText);                // Reads double value from stdin  
void   GetString (char* pString, const char* pText); // Reads string from stdin        
void   Mode2Text (char* sReflMode, VtInMod iMode);   // Converts enum for reflectivity calculation to text

char*  FullInName(const char* filename);             // returns path\name.ext for input directory   located in init.c
void   setParDirectory (char *a);


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char* argv[])
{
	double  mO=1.0,           // m      : official m-value of supermirror
          mT=1.0,           // m'     : true m-value from the R(Q) profile
          m,                // m      : current m value in loop
	        Q,                // Q      : momentum transfer of the reflection
	        Qc=QC_NI,         // Q_c    : crit. momentum transfer  (see figure)
          thetaNi,          // theta_Ni reflection angle for 1 Ang and Q_c(Ni)
	        theta,            // theta  : reflection angle for lambda = 1 Ang
	        W=0.00157,        // W      : width of cut-off  [1/Ang]
	        R0=0.995,         // R_0    : reflectivity for 0 <= Q <= Q_c
	                          //                 (or 0 <= theta <= theta_c)
	        Rm=0.0,           // R_m    : reflectivity for Q = m * Q_c(Ni)
	        R =0.0;           // R      : reflectivity for Q or theta,
  double  aM[ROFQ_MAX],     // array of m values read from 2 column file
          aQ[ROFQ_MAX],     // array of Q values read from 2 column file
          aR[ROFQ_MAX];     // array of R values read from 2 column file
  int     nVals=0;          // number of Q and R values from 2 column file
  VtInMod eMode=VT_PAR_IN;  // mode of reflectivity calculation
	long    i, nLen=0;
	FILE   *pFileIn, 
         *pFileOut;
	char   *pFullName,
         *sDash="---------------------------------------------------------------------------------------------",
          sMode[CHAR_BUF_SMALL]="",
          sText[CHAR_BUF_SMALL]="",
          sFileIn [50]="",
          sFileOut[50]="";

  _eModule=MCN_TOOL_GEN_COAT;

	Init(argc, argv, _eModule);
  for (i=1; i < ROFQ_MAX; i++)
  { aQ[i]=0.0;
    aR[i]=0.0;
  }
  thetaNi = Degrees(asin(QC_NI/(4*PI)));

  printf("%s\nGeneration of a reflectivity file as used in 'Guide' and 'Bender'\n%s\n", sDash, sDash);
//	printf("                                              ");
//	printf("    |                                         ");
//	printf(" R_0|_____________                            ");
//	printf("    |             .\                          ");
//	printf("    |             .  \                        ");
//	printf("    |             .    \                      ");
//	printf("    |             .      \                    ");
//	printf(" R_m|......................\                  ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    |             .         |                 ");
//	printf("    +-------------+---------+-----> Q, theta  ");
//	printf("    0            Q_c     m*Q_c(Ni)            ");
//	printf("               theta_c   m*theta_c(Ni)        ");
//	printf("                                              ");

// WARNING:
//  cut-off at m*theta_c,
//      not at m*theta_c(Ni) !!

  // get parameters and determine array length
	eMode = GetShort ("Calculation mode:\n 1: standard reflectivities \n 2: from R(m) file         \n 3: from R(Q) file          \n 4: old quadr. SwissNeutr.  \n 5: from 5 parameters       ");

  if (eMode!=VT_M_R_COL && eMode!=VT_Q_R_COL)
    mO = GetDouble("m   = Qmax / Qmax(Ni)                  ");

  if (eMode==VT_PAR_IN)
	{ Qc = GetDouble("Q_c = 4*pi*sin(theta_c)/lambda [1/Ang] \n     (0.0217   for Ni)                 ");
	  W  = GetDouble("width W of cut-off             [1/Ang] \n(typical 0.0015; polygonal shape: 0.0) ");
	  R0 = GetDouble("reflectivity(Q=0)                      ");
    if (mO > 1.0)
	    Rm = GetDouble("reflectivity(Q=m*Q_c(Ni)) for W=0      ");
    else
      Rm = R0;
    mT = mO;
  }
  else if (eMode==VT_REFL_STD)
  {
    W  = 0.00157;             // only needed for memory allocation
    mT = mO + 0.14;           // only needed for memory allocation
  }
  else if (eMode==VT_SN_QUD)
  {
    W  = 0.0022 - 0.0002*mO;  // only needed for memory allocation
    mT = mO*0.9853 + 0.1978;  // only needed for memory allocation
  }
  else if (eMode==VT_M_R_COL)
  {
    GetString(sFileIn, "Name of the 2-column mirror file R(m)  ");
    mO      = GetDouble("m-value of the coating described there ");
	  pFileIn = OpenInputFile(sFileIn, FALSE, "r");
	  if (pFileIn!=NULL)
	  {
      nVals = ReadRofQ(pFileIn, aM, aR);

      fclose(pFileIn);
    }
    mT = aM[nVals-1];
  }
  else if (eMode==VT_Q_R_COL)
  {
    GetString(sFileIn, "Name of the 2-column mirror file R(Q)  ");
    mO      = GetDouble("m-value of the coating described there ");
	  pFileIn = OpenInputFile(sFileIn, FALSE, "r");
	  if (pFileIn!=NULL)
	  {
      nVals = ReadRofQ(pFileIn, aQ, aR);

      fclose(pFileIn);
    }
    mT=aM[nVals-1]/QC_NI;
  }
  else
  { mT = mO;
    Error("Unknown calculation mode");
  }

  // number of reflectivity values
  if (eMode==VT_Q_R_COL)
    nLen = NumDataPtsQ(aQ[nVals-1]);
  else if (eMode==VT_M_R_COL)
    nLen = NumDataPtsM(mT, Qc, 0.0);
  else
    nLen = NumDataPtsM(mT, Qc, W);
  
  if (nLen > 0)
  {
	  /* write to input directory */
	  GetString(sFileOut, "Name of the output mirror file         ");
    pFullName = FullInName(sFileOut);
		
    pFileOut = fopen(pFullName, "w"); 
	  if (pFileOut!=NULL)
	  {
      // Header
      GetActDate(sText, DATE_STD);
      Mode2Text (sMode, eMode);
      fprintf(pFileOut, "#%s\n# %s\n# %s for m=%4.2f\n", sDash, sText, sMode, mO);
	
		  switch (eMode)
      { case VT_M_R_COL : 
        case VT_Q_R_COL : sprintf(sText, "2 column file %s read and transferred to Vitess format", sFileIn);
                                                                                    break;   
        case VT_REFL_STD: R = ReflTypicalT(sText, 2.0*Qc, mO, TRUE);                break;
        case VT_SN_QUD  : R = ReflSNT     (sText, 2.0*Qc, mO, TRUE);                break; 
        case VT_PAR_IN  : R = ReflMirrT   (sText, 2.0*Qc, mO, R0, Rm, W, Qc, TRUE); break;
      }
      fprintf(pFileOut, "# %s\n#%s\n", sText, sDash);

      /* calculate reflectivity for 1 Ang in steps of 0.001 deg
		     and write 10 values into each line                     */
		  i=0;
		  for (theta=0.0; i < nLen; theta+=0.001)
		  {
			  i++;
			  Q = QbyRefl  (1.0, theta);
        m = theta/thetaNi;
			  switch (eMode)
        { case VT_M_R_COL : R = InterpolM       (m, aM, aR, nVals);             break;   
          case VT_Q_R_COL : R = InterpolQ       (Q, aQ, aR, nVals);             break;   
          case VT_REFL_STD: R = ReflTypical     (Q, mO);                        break;
          case VT_PAR_IN  : R = ReflMirrT(sText, Q, mO, R0, Rm, W, Qc, FALSE);  break;
          case VT_SN_QUD  : R = ReflSNT  (sText, Q, mO, FALSE);                 break; 
        }
			  if (10*(i/10) == i)
				  fprintf(pFileOut, "%6.4f  # theta=%5.3f  m=%5.3f \n", R, RoundP(theta,3), RoundP(m,3));
			  else
				  fprintf(pFileOut, "%6.4f ",  R);
		  }
		  fclose(pFileOut);

		  printf("\n%s\nData written to %s\n", sText, pFullName); 
	  }
	  else
	  {	printf("\nERROR: Output file %s could not be generated\n", pFullName);
	  }
  }
  else
  { Error("Calculation did not give a positive number of data points");
  }

	printf("\n Hit any key to terminate ! \n");
	getchar();
	getchar();
	getchar();

	/* release the buffer memory */
	free(InputNeutrons);
	free(OutputNeutrons);

	return 1;
}


/*******************************************************/
/** Reads different types of parameters from stdin    **/
/**   GetShort :   Reads short value from stdin       **/
/**   GetDouble:   Reads double value from stdin      **/
/**   GetString:   Reads string from stdin            **/
/*******************************************************/
short  GetShort (const char* pText)
{
	short nValue;
	
	printf("%s ", pText);
	scanf ("%hd", &nValue);

	return nValue;
}

double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

void   GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}



/********************************************************/
/** Converts enum for reflectivity calculation to text **/
/********************************************************/
void Mode2Text(char* sReflMode, VtInMod iMode)
{
  switch (iMode)
  { case VT_REFL_STD: strcpy(sReflMode, "standard reflectivity");       break;
    case VT_M_R_COL : strcpy(sReflMode, "reflectivity from R(m) file"); break;
    case VT_Q_R_COL : strcpy(sReflMode, "reflectivity from R(Q) file"); break;
    case VT_SN_QUD  : strcpy(sReflMode, "quadratic SN reflectivity");   break;
    case VT_PAR_IN  : strcpy(sReflMode, "reflectivity from 5 parameter McStas function"); break;
    default         : strcpy(sReflMode, "unknown reflectivity calculation mode");
  }
}
