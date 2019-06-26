/*******************************************************************************************/
/* Tool SurfaceFile:                                                                       */
/*  Generates the surface file for the bender module for a bender                          */ 
/*  consisting of thin layers. Channels can exist that are separated                       */
/*  at the entrance or the exit                                                            */
/*                                                                                         */
/* 1.0  Apr 2003  K. Lieutenant  initial version                                           */
/* 1.1  May 2003  K. Lieutenant  Explanation of channels and wafers in the beginning       */
/* 1.2  Jul 2004  K. Lieutenant  feature 'space between channels at exit' reactivated;     */
/*                               correction for radius=0;                                  */
/* 1.3  Mar 2004  K. Lieutenant  files written to parameter directory or install_dir/FILES */
/*******************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "init.h"

#define TRUE   1
#define FALSE  0
#define PI     3.14159265358

char sBuffer   [256];

long   GetLong  (char* pText);
double GetDouble(char* pText);
void   GetString(char* pString, char* pText);


int main(int argc, char* argv[])
{
	double dRadius,            /* radius of the bender    */
	       dWaferThick,        /* thickness of each wafer */ 
	       dLength     = 0.0,  /* length of the bender    */
	       dDistEntr   = 0.0,  /* distance between channels at the entrance */
	       dDistExit   = 0.0,  /* distance between channels at the exit */
	       dAngle      = 0.0;  /* bender angle relativ to x-axis */
	long   nNoChannels = 0,    /* Number of channels        */
	       nNoWafers   = 0;    /* Number wafers per channel */			
	short  bConcentric = FALSE;/* criterion: concentric circles */
	char   sFileName[50], sConcentr[9],
	      *pFullName;
	FILE*  pSurfaceFile;

	Init(argc, argv, VT_TOOL);

	printf (">> Generation of the surface file for the bender module <<\n"
	        "----------------------------------------------------------\n"
	        "\nThe bender consists of N channels, each of which consists of M wafers.\n"
	        "The channels may have a spacing at the exit. In case of no spacing, there\n"
	        "is no difference between N channels of 1 wafer and 1 channel of N wafers.\n\n");
	nNoChannels = GetLong  ("\nNumber of channels                       ");
	nNoWafers   = GetLong  ("Number wafers per channel                ");
	dWaferThick = GetDouble("Thickness of wafer                  [cm] ");
	dDistExit   = GetDouble("Space between channels at exit      [cm] ");
	dRadius     = GetDouble("Radius of the bender (0 = straight) [cm] ");
	// dAngle   = GetDouble("Bender angle relativ to x-axis     [deg] ");
	GetString   (sConcentr, "Concentric circles           (y|n)       ");
	GetString   (sFileName, "Name of the surface file                 ");

	if (strcmp(sConcentr,"y")==0 || strcmp(sConcentr,"Y")==0 || strcmp(sConcentr,"yes")==0 || strcmp(sConcentr,"Yes")==0)
		bConcentric = TRUE;

	if (sFileName)
	{	if (nNoChannels > 0  &&  dRadius != 0.0  &&  dWaferThick > 0.0  &&  strlen(sFileName) > 0) 
		{	
			double dHeightEntr, dHeightExit,      /* Border of wafer at entrance and exit */
					 dHeightE0=0.0,                 /* Exit height for angle 0°   */
					 dRadCenter=0.0;                /* Radius of centered circles */
			long   nCh, nWa;

			dRadCenter = dRadius;

			// GenerateSurfaceFile
			pFullName = FullParName(sFileName);
			if (strcmp(pFullName, sFileName)==0)
				pFullName = FullInstallName(sFileName, "FILES/");
			pSurfaceFile = fopen(pFullName, "w");

			if (pSurfaceFile)
			{	/* dHeightE0  = dRadius - sqrt(dRadius*dRadius - length*length); */
				dHeightEntr = -0.5*(nNoChannels*nNoWafers*dWaferThick + (nNoChannels-1)*dDistEntr);
				dHeightExit = -0.5*(nNoChannels*nNoWafers*dWaferThick + (nNoChannels-1)*dDistExit)
								 + dHeightE0 + dLength*tan(dAngle*PI/180.);
				if (dRadius != 0 && bConcentric)
					dRadCenter  = dRadius + 0.5*nNoChannels*nNoWafers*dWaferThick;

				for (nCh = 1; nCh <= nNoChannels; nCh++) 
				{
					/* First surface or surface between channels, if there is a spacing at the exit */
					if (nCh==1 || dDistExit > 0.0)
						fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.2f\n", dHeightEntr, dHeightExit, dRadCenter);

					for (nWa = 1; nWa <= nNoWafers; nWa++) 
					{
						dHeightEntr += dWaferThick;
						dHeightExit += dWaferThick;
						if (dRadius != 0 && bConcentric)
							dRadCenter -= dWaferThick;
						fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.2f\n", dHeightEntr, dHeightExit, dRadCenter);
					}
					dHeightEntr += dDistEntr;
					dHeightExit += dDistExit;
				}

				printf ("\nData written to %s\n", pFullName);
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


long GetLong(char* pText)
{
	long nValue;
	
	printf("%s ", pText);
	scanf ("%ld", &nValue);

	return nValue;
}


double GetDouble(char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}


void GetString(char* pString, char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}
