/* The free non-commercial use of these routines is granted          */
/* providing due credit is given to the authors.                     */
/* Author: Géza Zsigmond, last change JUL 2002                       */
/* Change: Klaus Lieutenant, JUL 2002, trace coordinates added       */

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"

/* Here, any include file may follow you like */

extern short bTrace;

FILE *AsciiFile;

char *c, comment[100];


short OwnInit() 
{
  char  AsciiFileName [80];
  char  BinaryFileName[80];
  short bDirGiven=TRUE;

  bTrace = FALSE;

  printf("Give ASCII file name : ");
  scanf("%s", AsciiFileName);
	
  if((AsciiFile=fopen(AsciiFileName,"r"))==NULL) 
  { 
    bDirGiven=FALSE;
    AsciiFile=fopen(FullParName(AsciiFileName),"r");
    if (AsciiFile==NULL)
    { printf("Can't open file %s\n", AsciiFileName);
      return(FALSE);
    }
  }

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
  
  return TRUE;
}

void OwnCleanup()
{
  printf("\n Hit [Enter] to terminate ! \n");
  getchar();
  getchar();
  getchar();
  
  if (AsciiFile!=NULL)
    fclose(AsciiFile);
}

int main(int argc, char **argv)
{
  int i,j;
  char cBlank;
  short bFiles;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_TOOL);
  print_module_name("ASCII2BIN");

  /* module specific initialization */
  bFiles=OwnInit(argc, argv);

  if (bFiles)
  { 
    /* header line */
    {
      char comment[1000], *c ;
      c=fgets(comment, 1000, AsciiFile) ;
    }

    for(j=0; j<1e10; j++)
    {
      for(i=0; i<BufferSize; i++) 
      {
        if (fscanf(AsciiFile,"%2s", (char *) &InputNeutrons[i].ID.IDGrp )==EOF) goto finish;
        fscanf(AsciiFile,"%lu", &InputNeutrons[i].ID.IDNo ) ;
        fscanf(AsciiFile,"%c%c", &cBlank,  &InputNeutrons[i].Debug ) ;
        fscanf(AsciiFile,"%hd", &InputNeutrons[i].Color ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Time ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Wavelength ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Probability ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Position[0] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Position[1] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Position[2] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Vector[0] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Vector[1] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Vector[2] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Spin[0] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Spin[1] ) ;
        fscanf(AsciiFile,"%lf", &InputNeutrons[i].Spin[2] ) ;
        c=fgets(comment, 100, AsciiFile) ;

        NumNeutRead += 1;
               
        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  finish:
    printf("\n binary file written !\n");
  }

  /* do module specific cleanups */
  OwnCleanup();
  
  /* do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}
