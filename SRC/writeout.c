/*********************************************************************************************/
/*  VITESS module  WRITEOUT                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  June 1999  ???             initial version                                           */
/* 1.1  Mar  2001  K. Lieutenant   headline                                                  */	
/* 1.2  Jan  2004  K. Lieutenant   changes for 'instrument.dat' and changed headline         */
/* 1.3  Feb  2004  K. Lieutenant   'FullParName' and 'ERROR' included                        */
/* 1.4  Mar  2004  K. Lieutenant   F-Format Option                                           */
/* 1.4e Jul  2005  M. Fromme       headline, simplification                                  */
/* 1.4f Jan  2010  A. Houben       WriteOut only if given color matches Neutron color        */
/* 1.4g Feb  2010  A. Houben       Added wavelength, Div and yz position filter              */
/* 1.4h Feb  2010  K. Lieutenant   colour = 0 means all                                      */
/* 1.4i Jul  2011  A. Houben       colour = -1 means all; colour = 0 means only untaged      */
/*                                 neutrons by previous modules                              */
/* 1.4j Aug  2011  A. Houben       extended divergence filters                               */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


FILE *AsciiFile;
short bF_format=FALSE;
short DetectColor = 0; // WriteOut only neutrons with a given color, -1 means any
double filtLambdaMin=-1.0,          /* filter      */
	   filtLambdaMax=-1.0,
 	   filtYMin=-1.0e10,
       filtYMax=1.0e10,
	   filtZMin=-1.0e10,
	   filtZMax=1.0e10,
	   filtYDivMin=-1.0,
	   filtZDivMin=-1.0,
     filtDivMin=-1.0,
     filtYDivMax=-1.0,
	   filtZDivMax=-1.0,
     filtDivMax=-1.0;
int  calcDivY = 0,
     calcDivZ = 0;

void OwnInit(int argc, char *argv[]);
void OwnCleanup();


int main(int argc, char **argv)
{
  int i;
  const char *form;
  double Divy, Divz, Div;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("writeout 1.4j");

  /* module specific initialization */
  OwnInit(argc, argv);
 
  /* Get the neutrons from the file */
  DECLARE_ABORT;
  
  if (bF_format) {
    fprintf(AsciiFile,"#___ID___  Trc color     TOF   lambda  count_rate     pos_x    pos_y    pos_z  "
                      "    dir_x     dir_y     dir_z   sp_x sp_y sp_z\n");
    form = "%c%c%09lu %c %5d  %7.3f %8.5f %11.3e  %8.4f %8.4f %8.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n";
  } else {
    fprintf(AsciiFile,"#___ID___  Trc color      TOF        lambda  count_rate          pos_x        pos_y        pos_z"
                      "   direction_x  direction_y  direction_z        spin_x       spin_y       spin_z\n");
    form = "%c%c%09lu %c %5d  %.5e %.5e %.5e  % .5e % .5e % .5e  % .5e % .5e % .5e  % .5e % .5e % .5e\n";
  }

  while((ReadNeutrons())!= 0)
  {
    CHECK;    
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

	  WriteNeutron(&(InputNeutrons[i]));

	  if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	  if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	  if (InputNeutrons[i].Position[1] < filtYMin) continue;
	  if (InputNeutrons[i].Position[1] > filtYMax) continue;
	  if (InputNeutrons[i].Position[2] < filtZMin) continue;
	  if (InputNeutrons[i].Position[2] > filtZMax) continue;
	  
    if (calcDivY) {
      Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
		  Divy*=180.0/M_PI;
		  if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
			{Divy=0.0;}
    }
    if (calcDivZ) {
      Divz = (double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
		  Divz*=180.0/M_PI;
		  if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
			{Divz=0.0;}
      if (calcDivY) Div = sqrt(sq(Divy) + sq(Divz));
    }

	  if (filtYDivMin >= 0.) {
		  if (fabs(Divy) < filtYDivMin) continue;
	  }    
    if (filtYDivMax >= 0.) {
		  if (fabs(Divy) > filtYDivMax) continue;
	  }
  
    if (filtZDivMin >= 0.) {
		  if (fabs(Divz) < filtZDivMin) continue;
	  }
	  if (filtZDivMax >= 0.) {
		  if (fabs(Divz) > filtZDivMax) continue;
	  }

    if (filtDivMin >= 0.) {
		  if (fabs(Div) < filtDivMin) continue;
	  }
	  if (filtDivMax >= 0.) {
		  if (fabs(Div) > filtDivMax) continue;
	  }

	  if (DetectColor < 0 || InputNeutrons[i].Color == DetectColor)
      fprintf(AsciiFile, form,
	      InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo,          
	      InputNeutrons[i].Debug,       InputNeutrons[i].Color,       
	      InputNeutrons[i].Time,        InputNeutrons[i].Wavelength,  InputNeutrons[i].Probability,
	      InputNeutrons[i].Position[0], InputNeutrons[i].Position[1], InputNeutrons[i].Position[2], 
	      InputNeutrons[i].Vector[0],   InputNeutrons[i].Vector[1],   InputNeutrons[i].Vector[2], 
	      InputNeutrons[i].Spin[0],     InputNeutrons[i].Spin[1],     InputNeutrons[i].Spin[2]);
    }
  }
  
  /* Do module specific cleanups */
 my_exit:
  OwnCleanup();
  
  /* Do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


void  OwnInit(int argc, char *argv[]) 
{
  char *AsciiFileName=NULL;
  int i;

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { case 'A':
          AsciiFileName = &argv[i][2];
          break;
        case 'F':
          bF_format = (short) atoi(&argv[i][2]);
          break;
	    case 'C':
          DetectColor = (short) atoi(&argv[i][2]);
          break;
		
		case 'l':
		  filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
          break;

	    case 'L':
		  filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
		  break;

        case 'y':
          filtYMin = atof(&argv[i][2]);   /* filter Y */
          break;

        case 'Y':
          filtYMax = atof(&argv[i][2]);   /* filter Y */
          break;

        case 'z':
          filtZMin = atof(&argv[i][2]);   /* filter Z */
          break;

        case 'Z':
          filtZMax = atof(&argv[i][2]);   /* filter Z */
          break;
	    
	      case 'd':
          filtYDivMax = atof(&argv[i][2]);   /* filter DivYMax, -1 means any */
          break;

        case 'D':
          filtZDivMax = atof(&argv[i][2]);   /* filter DivZMax, -1 means any */
          break;

        case 'e':
          filtYDivMin = atof(&argv[i][2]);   /* filter DivYMin, -1 means any */
          break;

        case 'E':
          filtZDivMin = atof(&argv[i][2]);   /* filter DivZMin, -1 means any */
          break;

        case 'g':
          filtDivMin = atof(&argv[i][2]);   /* filter DivMin, -1 means any */
          break;

        case 'G':
          filtDivMax = atof(&argv[i][2]);   /* filter DivMax, -1 means any */
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  calcDivY = (filtYDivMin >= 0. || filtYDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);
  calcDivZ = (filtZDivMin >= 0. || filtZDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);

  if (AsciiFileName != NULL)
  { if ((AsciiFile=fopen(FullParName(AsciiFileName),"wt"))==NULL) 
    { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", AsciiFileName);
      exit(-1);
    }
  } 
  else 
  { fprintf(LogFilePtr,"ERROR: The option -A to give the ascii file name is mandatory!\n");
    exit(-1);
  }
}


void OwnCleanup()
{
  fclose(AsciiFile);
}
