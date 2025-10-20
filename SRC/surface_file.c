/*********************************************************************************************/
/* Tool SurfaceFile:                                                                         */
/*  Generates the surface file for the bender module of a bender consisting of amin channels */
/*  and sub-channels. The main channels can be separated at the entrance or exit             */
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
/* 1.6  Mar 2022  K. Lieutenant  correction: input->output file, check: r > 0 removed        */
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

char*  FullOutName(const char* filename);             // returns path\name.ext for input directory   located in init.c


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char* argv[])
{
  double Radius      = 0.0,   // radius of the bender
         WaferThkIn  = 0.0,   // thickness of each sub-channel at entrance
         WaferThkOut = 0.0,   // thickness of each sub-channel at exit
         WaferThkAvrg= 0.0,   // average sub-channel thickness
         dLength     = 0.0,   // length of the bender
         DistEntr    = 0.0,   // distance between channels at the entrance
         DistExit    = 0.0,   // distance between channels at the exit
         Angle       = 0.0;   // bender angle relativ to x-axis
  int    nChannels   = 0,     // Number of channels
         nWafers     = 0;     // Number sub-channels per channel
  short  bConcentric = FALSE; // criterion: concentric circles
  char   sFileName[50]="", sConcentr[9]="no";
  FILE*  pSurfaceFile =NULL;

  _eModule = MCN_TOOL_GEN_SURF;
  Init(argc, argv, _eModule);

  printf (">> Generation of the surface file for the bender module, vsn 1.6 <<\n"
          "-------------------------------------------------------------------\n"
          "\nThe bender consists of N main channels, each of which consists of M sub-channels.\n"
          "The main channels can have a spacing at the entrance or the exit or both. \n"
          "Without any spacing, there is no difference between N main channels of 1 sub-channel\nand 1 main channel of N sub-channels.\n\n");

  nChannels   = GetLong("\nNumber of main channels                      ");
  nWafers     = GetLong  ("Number sub-channels per channel              ");
  WaferThkIn = GetDouble("Thickness of sub-channels at entrance [cm]   ");
  WaferThkOut= GetDouble("Thickness of sub-channels at exit     [cm]   ");
  DistEntr   = GetDouble("Space between main channels at entrance [cm] ");
  DistExit   = GetDouble("Space between main channels at exit   [cm]   ");
  Radius     = GetDouble("Radius of the bender (0 = straight)   [cm]   ");
  if (Radius > 0.0)
    GetString (sConcentr, "Concentric circles           (y|n)           ");
  GetString   (sFileName, "Name of the surface file                     ");
  printf("\n");

  if (WaferThkIn  <= 0.0) Warning("Channel thickness is zero or less at entrance, bender will not be able to transmit neutrons");
  if (WaferThkOut <= 0.0) Warning("Channel thickness is zero or less at exit, bender will not be able to transmit neutrons");
  // if (DistEntr > 0.0 && DistExit > 0.0) Warning("Having exit and entrance difference greater creates additional channels that are presumably not foreseen and need to be blocked");

  if (strcmp(sConcentr,"y")==0 || strcmp(sConcentr,"Y")==0 || strcmp(sConcentr,"yes")==0 || strcmp(sConcentr,"Yes")==0)
    bConcentric = TRUE;

  if (strlen(sFileName) > 0)
  {
    if (nChannels > 0  &&  nWafers > 0)
    {
      double Yentr, Yexit,      /* Border of sub-channel at entrance and exit */
             YE0=0.0,            /* Exit height for angle 0°   */
             RadCenter=0.0;      /* Radius of centered circles */
      long   nCh, nWa;

      RadCenter    = Radius;
      WaferThkAvrg= (WaferThkIn+WaferThkOut)/2.0;

      // GenerateSurfaceFile
      pSurfaceFile = OpenOutputFile(sFileName, FALSE, "w");

      if (pSurfaceFile)
      {  /* YE0  = Radius - sqrt(Radius*Radius - length*length); */
        Yentr = -0.5*(nChannels*nWafers*WaferThkIn  + (nChannels-1)*DistEntr);
        Yexit = -0.5*(nChannels*nWafers*WaferThkOut + (nChannels-1)*DistExit)
                     + YE0 + dLength*tan(Angle*PI/180.);
        if (Radius != 0 && bConcentric)
          RadCenter  = Radius + 0.5*nChannels*nWafers*WaferThkAvrg;

        for (nCh = 1; nCh <= nChannels; nCh++)
        {
          /* First surface or surface between channels, if there is a spacing at the exit */
          if (nCh==1 || DistEntr > 0.0 || DistExit > 0.0)
            fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.3f\n", Yentr, Yexit, RadCenter);

          for (nWa = 1; nWa <= nWafers; nWa++)
          {
            Yentr += WaferThkIn;
            Yexit += WaferThkOut;
            if (Radius != 0 && bConcentric)
              RadCenter -= WaferThkAvrg;
            fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.3f\n", Yentr, Yexit, RadCenter);
          }
          Yentr += DistEntr;
          if (DistEntr > 0.0 && DistExit > 0.0 && nCh < nChannels)
            fprintf(pSurfaceFile, "%8.4f\t%8.4f\t%9.3f\n", Yentr, Yexit, RadCenter);
          Yexit += DistExit;
        }

        printf ("\nData written to %s\n", FullOutName(sFileName));
        fclose(pSurfaceFile);
      }
      else
      {  printf("ERROR: Output file '%s' could not be generated\n", FullOutName(sFileName));
      }
    }
    else
    {  printf("ERROR: Number of channels and subchannels per channel must both be greater 0 -  no file generated\n");
    }
  }
  else
  {  printf("ERROR: no surface file name given!\n File could not be generated\n");
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
