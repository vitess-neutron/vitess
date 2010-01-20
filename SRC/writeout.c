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
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


FILE *AsciiFile;
short bF_format=FALSE;
short DetectColor = -1; // WriteOut only neutrons with a given color, -1 means any

void OwnInit(int argc, char *argv[]);
void OwnCleanup();


int main(int argc, char **argv)
{
  int i;
  char *form;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("writeout 1.4e");

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
	  if (DetectColor < 0 || InputNeutrons[i].Color == DetectColor)
      fprintf(AsciiFile, form,
	      InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo,          
	      InputNeutrons[i].Debug,       InputNeutrons[i].Color,       
	      InputNeutrons[i].Time,        InputNeutrons[i].Wavelength,  InputNeutrons[i].Probability,
	      InputNeutrons[i].Position[0], InputNeutrons[i].Position[1], InputNeutrons[i].Position[2], 
	      InputNeutrons[i].Vector[0],   InputNeutrons[i].Vector[1],   InputNeutrons[i].Vector[2], 
	      InputNeutrons[i].Spin[0],     InputNeutrons[i].Spin[1],     InputNeutrons[i].Spin[2]);
             
      WriteNeutron(&(InputNeutrons[i]));
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
        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
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
