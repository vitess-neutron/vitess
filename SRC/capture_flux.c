/*********************************************************************************************/
/* VITESS module caputure_flux                                                               */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00  Feb 2008  K. Lieutenant  initial version                                            */
/* 1.01  Oct 2009  A. Houben      User may change reference wavelength                       */
/* 1.10  Oct 2009  A. Houben      Limit capture area by circle or rectangle (like window)    */
/* 1.11  Nov 2009  A. Houben      Limit captured flux by wave-length range                   */
/* 1.12  Nov 2009  K. Lieutenant  gold foil area calculated                                  */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


void OwnInit(int argc, char *argv[]);
void OwnCleanup();

double CaptArea=1.0;
double ReferenceWavelength=1.798;
double heightmin  = 0.0, /* z-coordinate: bottom of rectangular window             [cm] */
       heightmax  = 0.0, /* z-coordinate: top of rectangular window                [cm] */
       widthmin   = 0.0, /* y-coordinate: lower frame value of rectangular window  [cm] */
       widthmax   = 0.0, /* y-coordinate: higher frame value of rectangular window [cm] */
       winradius  = 0.0, /* radius of circular window                              [cm] */
       ywincenter = 0.0, /* y coordinate: center of circular window                [cm] */
       zwincenter = 0.0; /* y coordinate: center of circular window                [cm] */
double lambdamin = 0.0,  /* Minimum lambda for flux count                          [A]  */
       lambdamax = 0.0;  /* Maximum lambda for flux count                          [A]  */
long   WindowType = 0; /* 0 = No restrictions; 1 = circular window; 2 = rectangular window */
double avColor = 0.0, avwColor = 0.0; /* Average color and weighted average color */


int main(int argc, char **argv)
{
  int i, Ntot=0;
  double CaptInt=0.0, CaptQuad=0.0, CaptErr=0.0;
  short  bOutOfWindow = FALSE, bOutOfLambda = FALSE;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_CAPTURE);
  print_module_name("capture_flux 1.12");

  /* module specific initialization */
  OwnInit(argc, argv);
 
  /* Get the neutrons from the file */
  DECLARE_ABORT;
  
  while((ReadNeutrons())!= 0)
  {
    CHECK;    
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

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


	  if (!bOutOfWindow && !bOutOfLambda) {
        if (ReferenceWavelength <= 0.) {
			CaptInt  +=    InputNeutrons[i].Probability;
			CaptQuad += sq(InputNeutrons[i].Probability);
		} else {
			CaptInt  +=    InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/ReferenceWavelength;
			CaptQuad += sq(InputNeutrons[i].Probability*InputNeutrons[i].Wavelength/ReferenceWavelength);
		}
		avColor += (double)InputNeutrons[i].Color;
		avwColor += (double)InputNeutrons[i].Color*InputNeutrons[i].Probability;
        Ntot++;
	  }

      WriteNeutron(&(InputNeutrons[i]));
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
	fprintf(LogFilePtr,"Circular window of %6.2f cm diameter \n", 2.0*winradius);
	break;
  case 2:
	fprintf(LogFilePtr,"Rectangular window of size %6.2f x %6.2f cm (W x H) \n", 
					 widthmax-widthmin, heightmax-heightmin);
	break;
  }

  if (lambdamin != 0. && lambdamax != 0.)
	fprintf(LogFilePtr,"Lambda window from %6.2f A to %6.2f A \n", lambdamin, lambdamax);

  fprintf(LogFilePtr, "Reference wavelength: %12.3f A\n", ReferenceWavelength);
  if (avColor != 0.0) {
	fprintf(LogFilePtr, "Average color       : %12.3f \n", avColor);
	fprintf(LogFilePtr, "Avr. weighted color : %12.3f \n", avwColor);
  }
  fprintf(LogFilePtr, "Capture area        : %12.3f cm^2\n", CaptArea);
  fprintf(LogFilePtr, "Capture flux        : %12.3e +/- %12.3e n/(s*cm^2) \n\n", CaptInt/CaptArea, CaptErr);
  
  /* Do module specific cleanups */
 my_exit:
  OwnCleanup();
  
  /* Do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


void  OwnInit(int argc, char *argv[]) 
{
  int i;

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
		/* area */
		/* case 'A':
			CaptArea = atof(&argv[i][2]);  
			break; */
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


void OwnCleanup()
{
  return;
}
