/*********************************************************************************************/
/* Tool SurfaceFile:                                                                         */
/*  Generation of a complete list of reflection from a list of only positive hkl reflections */
/*                                                                                           */
/* The free non-commercial use of these routines is granted provided due credit is given to  */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Apr 2023  K. Lieutenant  initial version                                             */
/*********************************************************************************************/

#include <ctype.h>
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
int    GetInt     (const char* pText);                 // Reads long value from stdin   
double GetDouble  (const char* pText);                 // Reads double value from stdin    
void   GetString  (char* pString, const char* pText);  // Reads string from stdin  

void   GenerFormat(char* sLine);
void   PrintLine  (int h, int k, int l);               // writes 1 h,k,l line

char*  FullOutName(const char* filename);             // returns path\name.ext for input directory   located in init.c



/******************************/
/** Global variables         **/
/******************************/
FILE  *pInFile =NULL,  // original hkl file
      *pOutFile=NULL;  // completed hkl file
char   sFormat[256]="";
int    iH=1;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char* argv[])
{
	int    h=0, k=0, l=0,      // h k l value
         iHKL=0,             // index of reflection
         nHKL=0;             // number of reflections 
  double Value[40];
	char   sInFileName [50]="",
         sOutFileName[50]="",
         sLine       [99]="";

  _eModule = MCN_TOOL_GEN_HKL;
	Init(argc, argv, _eModule);

	printf (">> Generation of a complete list of reflection from a list of only positive hkl reflections <<\n"
	        "----------------------------------------------------------------------------------------------\n\n");

	GetString  (sInFileName,  "Name of the input file (positive hkl values) ");
	GetString  (sOutFileName, "Name of the output file (all hkl values)     ");
  iH =               GetInt("column in which h is listed                  ");
	printf("\n");

	if (strlen(sInFileName) > 0 && strlen(sOutFileName) > 0)
	{	
		// Open files
		pInFile  = OpenInputFile (sInFileName,  FALSE, "r");
		pOutFile = OpenOutputFile(sOutFileName, FALSE, "w");

		if (pInFile!=NULL && pOutFile!=NULL )
		{	
      nHKL = LinesInFile(pInFile);

      for (iHKL=0; iHKL < nHKL; iHKL++)
      {
        ReadLine(pInFile, sLine, sizeof(sLine));
        GenerFormat(sLine);
        StrgScanLF(sLine, Value, 40, 0);
        h = (int)Value[iH-1]; k = (int)Value[iH]; l = (int)Value[iH+1];

        PrintLine(h, k, l);
        if (h > 0)
        { PrintLine(-h, k, l);
          if (k > 0)
          { PrintLine( h, -k, l);
            PrintLine(-h, -k, l);
            if (l > 0)
            { PrintLine( h,  k, -l);
              PrintLine(-h,  k, -l);
              PrintLine( h, -k, -l);
              PrintLine(-h, -k, -l);
            }
          }
          else
          { if (l > 0)
            { PrintLine( h, k, -l);
              PrintLine(-h, k, -l);
            }
          }
        }
        else
        {
          if (k > 0)
          { PrintLine(h, -k, l);
            if (l > 0)
            { PrintLine(h,  k, -l);
              PrintLine(h, -k, -l);
            }
          }
          else
          { if (l > 0)
            { PrintLine(h, k, -l);
            }
          }
        }
      }
    }
    fclose(pInFile);
    fclose(pOutFile);
	}
	else
	{	printf("ERROR: input and/or output file name not given!\n File could not be generated\n");
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
int GetInt(const char* pText)
{
	int nValue;
	
	printf("%s ", pText);
	scanf ("%d", &nValue);

	return nValue;
}

double GetDouble(const char* pText)
{
	double value;
	
	printf("%s ", pText);
	scanf ("%lf", &value);

	return value;
}

void GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

void PrintLine(int h, int k, int l)
{
  fprintf(pOutFile, sFormat, h, k, l);

  return;
}

void   GenerFormat(char* sLine)
{
  int n=0, k=0, k_beg=0, k_end=0;
  
  strcpy(sFormat, "");

  do
	{	/* search of beginning and end of 1st number of (remaining) string */
		k=0;
		/* step forward until first number or control character */
		while (isdigit(sLine[k])==0 && iscntrl(sLine[k])==0)
			k++;
    k_beg=k;
		/* step forward until space-like or control character */
		while (isspace(sLine[k])==0 && iscntrl(sLine[k])==0)
			k++;
    k_end=k;

		/* exchange h k l values by %3d, keep the rest */
    n++;
		if (n>= iH && n <=iH+2)
    { strcat(sFormat, " %3d");
    }
    else
		{
			strncat(sFormat, sLine, k_end);
		}
		sLine += k_end;
	}
	while (k > 0);

  strcat(sFormat, " \n");
}
