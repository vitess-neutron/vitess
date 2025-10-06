/*********************************************************************************************/
/* VITESS module capture_flux                                                                */
/*      determination of the flux value as obtained in gold foil measurements                                                                                     */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00  Feb 2008  K. Lieutenant  initial version                                            */
/* 1.01  Oct 2009  A. Houben      User may change reference wavelength                       */
/* 1.10  Oct 2009  A. Houben      Limit capture area by circle or rectangle (like window)    */
/* 1.11  Nov 2009  A. Houben      Limit captured flux by wave-length range                   */
/* 1.12  Nov 2009  K. Lieutenant  gold foil area calculated                                  */
/* 1.13  Mar 2020  K. Lieutenant  new central visualization parameters                       */
/* 1.13a Aug 2022  K. Lieutenant  output improved and standard deviation corrected           */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"

// To do for version 4
// - WindowType needs new structure definition and convert function
// - yaml file missing

/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);     // Reads input parameters and sets global parameters
void OwnCleanup();                        // Does module specific cleanup


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
double ReferenceWavelength=1.798;     // -R  [Ang]  reference wavelength
double heightmin  = 0.0,              // -h  [cm]   z-coordinate: bottom of rectangular window
       heightmax  = 0.0,              // -H  [cm]   z-coordinate: top of rectangular window
       widthmin   = 0.0,              // -w  [cm]   y-coordinate: lower frame value of rectangular window
       widthmax   = 0.0,              // -W  [cm]   y-coordinate: higher frame value of rectangular window
       winradius  = 0.0,              // -r  [cm]   radius of circular window
       ywincenter = 0.0,              // -y  [cm]   y coordinate: center of circular window
       zwincenter = 0.0;              // -z  [cm]   y coordinate: center of circular window
double lambdamin  = 0.0,              // -l  [Ang]  Minimum lambda for flux count
       lambdamax  = 0.0;              // -L  [Ang]  Maximum lambda for flux count
long   WindowType = 0;                // -t  [Ang]  0 = No restrictions; 1 = circular window; 2 = rectangular window

// Variables determined from input parameters or trajectory data
double CaptArea=1.0;                  //    [cm^2]  area for catching the neutrons
double avColor = 0.0, avwColor = 0.0; //     [Ang]  Average color and weighted average color */


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int    i=0, Ntot=0;
  double CaptInt=0.0, CaptQuad=0.0, CaptErr=0.0;
  short  bOutOfWindow = FALSE, bOutOfLambda = FALSE;

  /* Initialize the program according to the parameters given   */
  _eModule=MCN_CAPTURE;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.13a");
  OwnInit(argc, argv);    // module specific initialization

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
  /* Get the neutrons from the file */
  while((ReadNeutrons())!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        switch (WindowType)
        {
          case 1:
            bOutOfWindow = (winradius*winradius <
            ((InputNeutrons[i].Position[1] - ywincenter)*(InputNeutrons[i].Position[1] - ywincenter) +
            (InputNeutrons[i].Position[2] - zwincenter)*(InputNeutrons[i].Position[2] - zwincenter)));
            break;
          case 2:
            bOutOfWindow = ((widthmin  > InputNeutrons[i].Position[1]) || (widthmax < InputNeutrons[i].Position[1]) ||
            (heightmin > InputNeutrons[i].Position[2]) || (heightmax < InputNeutrons[i].Position[2]));
            break;
        }

        if (lambdamin != 0. && lambdamax != 0.)
          bOutOfLambda = (InputNeutrons[i].Wavelength < lambdamin || InputNeutrons[i].Wavelength > lambdamax);
        else
          bOutOfLambda = FALSE;


        if (!bOutOfWindow && !bOutOfLambda)
        {
          double col;

          if (ReferenceWavelength <= 0.)
          {
            CaptInt  +=    InputNeutrons[i].Probability;
            CaptQuad += sq(InputNeutrons[i].Probability);
          }
          else
          {
            CaptInt  +=    InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/ReferenceWavelength;
            CaptQuad += sq(InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/ReferenceWavelength);
          }

          //colour counting: sum (horizontal+vertical)
          col= (InputNeutrons[i].Color - InputNeutrons[i].Color%100)  / 100 + (InputNeutrons[i].Color %100);
          avColor += col;
          avwColor += col*InputNeutrons[i].Probability;

          Ntot++;
        }

        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

  /* error for the given count rate calculated through adding squared errors
  - of the number  of contributing traj.: sqrt(N) (Poisson distribution)
  - of the average count rate of each trajectory I_s = I_tot/N:
  sqrt((<I_s²> - <I_s>²)/(N-1))
  as independent contributions */
  if (Ntot > 1)
    CaptErr = sqrt(sq(CaptInt)/Ntot + (Ntot*CaptQuad-sq(CaptInt))/(Ntot-1));
  else
    CaptErr = CaptInt;

  avColor /= Ntot;
  avwColor /= CaptInt;

  switch (WindowType)
  {
    case 0:
      break;
    case 1:
      fprintf(LogFilePtr,"Circular window of %6.2f cm diameter,    area %7.3f cm^2\n", 2.0*winradius, CaptArea);
      break;
    case 2:
      fprintf(LogFilePtr,"Rectangular window of size %6.2f x %6.2f cm (W x H),    area %7.3f  cm^2 \n", widthmax-widthmin, heightmax-heightmin, CaptArea);
      break;
  }

  if (lambdamin != 0. && lambdamax != 0.)
    fprintf(LogFilePtr,"Lambda window from %7.3f A to %7.3f A \n", lambdamin, lambdamax);

  fprintf(LogFilePtr, "Reference wavelength: %12.3f A\n", ReferenceWavelength);
  if (avColor != 0.0 && Ntot!=0)
  {
    fprintf(LogFilePtr, "Average color       : %12.3f \n", avColor);
    fprintf(LogFilePtr, "Avr. weighted color : %12.3f \n", avwColor);
  }
  fprintf(LogFilePtr, "Captured intensity  : %12.3e +/- %12.3e n/s       by %10d trajectories\n", CaptInt, CaptErr,    Ntot);
  fprintf(LogFilePtr, "Capture flux        : %12.3e +/- %12.3e n/(s*cm^2) \n\n",          CaptInt/CaptArea, CaptErr/CaptArea);

  // Finish: writes and closes monitor files, writes to log and instrument file, frees memory
  // ----------------------------------------------------------------------------------------
 my_exit:
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i;

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'R':
          ReferenceWavelength = atof(&argv[i][2]);
          break;

        case 't':
          WindowType = atol(&argv[i][2]);
          break;

        case 'h':
          heightmin =  atof(&argv[i][2]);
          break;
        case 'w':
          widthmin = atof(&argv[i][2]);
          break;

        case 'H':
          heightmax =  atof(&argv[i][2]);
          break;
        case 'W':
          widthmax = atof(&argv[i][2]);
          break;

        case 'r':
          winradius = atof(&argv[i][2]);
          break;
        case 'y':
          ywincenter = atof(&argv[i][2]);
          break;
        case 'z':
          zwincenter = atof(&argv[i][2]);
          break;

        case 'l':
          lambdamin = atof(&argv[i][2]);
          break;
        case 'L':
          lambdamax = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  if (WindowType==1)        // circular
  {
    CaptArea = sq(winradius)*M_PI;
  }
  else if (WindowType==2)   // rectangular
  {
    CaptArea = (heightmax-heightmin)*(widthmax-widthmin);
  }

  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  return;
}
