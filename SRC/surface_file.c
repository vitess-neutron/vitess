/*********************************************************************************************/
/* Tool SurfaceFile:                                                                         */
/*  Generates the surface file for the bender module for a bender consisting of thin layers. */
/*   Channels can exist that are separated at the entrance or the exit                       */
/*                                                                                           */
/* The free non-commercial use of these routines is granted provided due credit is given to  */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Apr 2003  K. Lieutenant  initial version                                             */
/* 1.1  May 2003  K. Lieutenant  Explanation of channels and wafers in the beginning         */
/* 1.2  Jul 2004  K. Lieutenant  feature 'space between channels at exit' reactivated;       */
/*                               correction for radius=0;                                    */
/* 1.3  Mar 2004  K. Lieutenant  files written to parameter directory or install_dir/FILES   */
/* 1.4  Jun 2013  K. Lieutenant  conical shape of channels allowed                           */
/* 1.5  Mar 2020  K. Lieutenant  tidy up, new central parameters and functions               */
/*********************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "init.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define TRUE   1
#define FALSE  0
#define PI     3.14159265358


/******************************/
/** Prototypes               **/
/******************************/
long   GetLong  (const char* pText);                 // Reads long value from stdin   
double GetDouble(const char* pText);                 // Reads double value from stdin    
void   GetString(char* pString, const char* pText);  // Reads string from stdin         

char*  FullInName(const char* filename);             // returns path\name.ext for input directory   located in init.c


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char* argv[])
{
	double dRadius,             // radius of the bender 
	       dWaferThkIn,         // thickness of each wafer at entrance 
	       dWaferThkOut,        // thickness of each wafer at exit 
         dWaferThkAvrg,       // average wafer thickness
	       dLength     = 0.0,   // length of the bender 
	       dDistEntr   = 0.0,   // distance between channels at the entrance
	       dDistExit   = 0.0,   // distance between channels at the exit 
	       dAngle      = 0.0;   // bender angle relativ to x-axis 
	long   nChannels = 0,       // Number of channels 
	       nWafers   = 0;       // Number wafers per channel		
	short  bConcentric = FALSE; // criterion: concentric circles
	char   sFileName[50], sConcentr[9];
	FILE*  pSurfaceFile;

  _eModule = MCN_TOOL_GEN_SURF;
	Init(argc, argv, _eModule);

	printf (">> Generation of the surface file for the bender module <<\n"
	        "----------------------------------------------------------\n"
	        "\nThe bender consists of N channels, each of which consists of M wafers.\n"
	        "The channels may have a spacing at the exit. In case of no spacing, there\n"
	        "is no difference between N channels of 1 wafer and 1 channel of N wafers.\n\n");

	nChannels = GetLong  ("\nNumber of channels                       ");
	nWafers   = GetLong  ("Number wafers per channel                ");
	dWaferThkIn = GetDouble("Thickness of wafer at entrance      [cm] ");
	dWaferThkOut= GetDouble("Thickness of wafer at exit          [cm] ");
	dDistExit   = GetDouble("Space between channels at exit      [cm] ");
	dRadius     = GetDouble("Radius of the bender (0 = straight) [cm] ");
	// dAngle   = GetDouble("Bender angle relativ to x-axis     [deg] ");
	GetString   (sConcentr, "Concentric circles           (y|n)       ");
	GetString   (sFileName, "Name of the surface file                 ");

	if (strcmp(sConcentr,"y")==0 || strcmp(sConcentr,"Y")==0 || strcmp(sConcentr,"yes")==0 || strcmp(sConcentr,"Yes")==0)
		bConcentric = TRUE;

	if (strlen(sFileName) > 0)
	{	if (nChannels > 0  &&  dRadius != 0.0  &&  dWaferThkIn > 0.0  &&  dWaferThkOut > 0.0  &&  strlen(sFileName) > 0) 
		{	
			double dYEntr, dYExit,      /* Border of wafer at entrance and exit */
			       dYE0=0.0,                 /* Exit height for angle 0°   */
			       dRadCenter=0.0;                /* Radius of centered circles */
			long   nCh, nWa;

			dRadCenter   = dRadius;
      dWaferThkAvrg= (dWaferThkIn+dWaferThkOut)/2.0;

			// GenerateSurfaceFile
			pSurfaceFile = OpenInputFile(sFileName, FALSE, "w");

			if (pSurfaceFile)
			{	/* dYE0  = dRadius - sqrt(dRadius*dRadius - length*length); */
				dYEntr = -0.5*(nChannels*nWafers*dWaferThkIn  + (nChannels-1)*dDistEntr);
				dYExit = -0.5*(nChannels*nWafers*dWaferThkOut + (nChannels-1)*dDistExit)
								     + dYE0 + dLength*tan(dAngle*PI/180.);
				if (dRadius != 0 && bConcentric)
					dRadCenter  = dRadius + 0.5*nChannels*nWafers*dWaferThkAvrg;

				for (nCh = 1; nCh <= nChannels; nCh++) 
				{
					/* First surface or surface between channels, if there is a spacing at the exit */
					if (nCh==1 || dDistExit > 0.0)
						fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.3f\n", dYEntr, dYExit, dRadCenter);

					for (nWa = 1; nWa <= nWafers; nWa++) 
					{
						dYEntr += dWaferThkIn;
						dYExit += dWaferThkOut;
						if (dRadius != 0 && bConcentric)
							dRadCenter -= dWaferThkAvrg;
						fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.3f\n", dYEntr, dYExit, dRadCenter);
					}
					dYEntr += dDistEntr;
					dYExit += dDistExit;
				}

				printf ("\nData written to %s\n", FullInName(sFileName));
				fclose(pSurfaceFile);
			}
			else
			{	printf("\nERROR: Output file could not be generated\n");
			}
		}
	}
	else
	{	printf("\nERROR: no surface file name given!\n File could not be generated");
	}

	printf("\n Hit any key to terminate ! \n");
	getchar();
	getchar();
	getchar();
	getchar();
  
	/* release the buffer memory */
	free(InputNeutrons);
	free(OutputNeutrons);

	return 0;
}


/*******************************************************/
/** Reads different types of parameters from stdin    **/
/**   GetLong  :   Reads long value from stdin       **/
/**   GetDouble:   Reads double value from stdin      **/
/**   GetString:   Reads string from stdin            **/
/*******************************************************/
long GetLong(const char* pText)
{
	long nValue;
	
	printf("%s ", pText);
	scanf ("%ld", &nValue);

	return nValue;
}

double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

void GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}
