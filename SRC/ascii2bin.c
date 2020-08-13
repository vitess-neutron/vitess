/* The free non-commercial use of these routines is granted          */
/* providing due credit is given to the authors.                     */
/* Author: Géza Zsigmond, last change JUL 2002                       */
/* Change: Klaus Lieutenant, JUL 2002, trace coordinates added       */

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"


extern short bTrace;

FILE* pAsciiFile;
char  AsciiFileName [80]="";
char  BinaryFileName[80]="";


short OwnInit(void) 
{
  short bDirGiven=TRUE;

  bTrace = FALSE;

  printf("Give ASCII file name : ");
  scanf("%s", AsciiFileName);
	
  pAsciiFile=fopen(AsciiFileName,"r");
  if (pAsciiFile==NULL) 
  { 
    bDirGiven=FALSE;
    pAsciiFile=fopen(FullParName(AsciiFileName),"r");
    if (pAsciiFile==NULL)
    { printf("Can't open file %s\n", AsciiFileName);
      return(FALSE);
    }
  }

  // write output file
  printf("Give binary file name: ");
  scanf("%s", BinaryFileName);
  if(bDirGiven)
    OutputFilePtr=fopen(BinaryFileName,"wb");
  else
    OutputFilePtr=fopen(FullParName(BinaryFileName),"wb");

  if (OutputFilePtr==NULL)
  { printf("Can't open file %s\n", BinaryFileName);
    return(FALSE);
  }
  else
  { return(TRUE);
  }
}

void OwnCleanup()
{
  printf("\n Hit [Enter] to terminate ! \n");
  getchar();
  getchar();
  getchar();
  
  if (pAsciiFile!=NULL)
    fclose(pAsciiFile);
}

int main(int argc, char **argv)
{
  int   i,j, rc;
  char  sLine[256];
	char  *pForm="%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf";
  short bFiles;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, MCN_TOOL);
  print_module_name("ascii2bin");

  /* module specific initialization */
  NumNeutRead=0.0;
  bFiles = OwnInit();
  if (bFiles)
  { 
    for(j=0; j<1e10; j++)
    {
      for(i=0; i<BufferSize; i++) 
      {
        ReadLine(pAsciiFile, sLine, sizeof(sLine)-1);

        rc=sscanf(sLine, pForm, &InputNeutrons[i].ID.IDGrp[0], &InputNeutrons[i].ID.IDGrp[1], &InputNeutrons[i].ID.IDNo, 
                                &InputNeutrons[i].Debug,       &InputNeutrons[i].Color,                        
                                &InputNeutrons[i].Time,        &InputNeutrons[i].Wavelength,  &InputNeutrons[i].Probability, 
                                &InputNeutrons[i].Position[0], &InputNeutrons[i].Position[1], &InputNeutrons[i].Position[2], 
                                &InputNeutrons[i].Vector[0],   &InputNeutrons[i].Vector[1],   &InputNeutrons[i].Vector[2], 
                                &InputNeutrons[i].Spin[0],     &InputNeutrons[i].Spin[1],     &InputNeutrons[i].Spin[2]   ); 
        if (rc < 1)
          goto finish;
                       
        WriteNeutron(&(InputNeutrons[i]));
        NumNeutRead += 1;
      }
    }
  finish:
    printf("\n %6.0f trajectories written to binary file %s !\n", NumNeutRead, BinaryFileName);
  }

  /* do module specific cleanups */
  OwnCleanup();
  
  /* do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}
