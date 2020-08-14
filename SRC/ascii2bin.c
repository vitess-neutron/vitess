/*********************************************************************************************/
/*  VITESS tool 'ascii2bin.c'                                                                */
/*    conversion of tractory files from ASCII format to binary format                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0      2000  G. Zsigmond    initial                                                     */
/* 1.1  Jul 2002  G. Zsigmond    correction                                                  */
/* 1.2  Jul 2002  K. Lieutenant  trace coordinates added                                     */
/* 1.3  Mar 2020  K. Lieutenant  tidy up, new central parameters and functions               */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "general.h"
#include "init.h"



/******************************/
/** Prototypes               **/
/******************************/
short OwnInit();                   // Reads input parameters and sets global parameters
void  OwnCleanup();                // Does module specific cleanup


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_TOOL_A2B;

FILE* pAsciiFile;
char  AsciiFileName [80]="";
char  BinaryFileName[80]="";

extern short bTrace;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int   i,j, rc;
  char  sLine[256];
	char  *pForm="%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf";
  short bFiles;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  bFiles = OwnInit();             // module specific initialization

  if (bFiles)
  { 
	  // loop over trajectories
    // ----------------------
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

  // Finish: writes and closes monitor files, writes to log and instrument file, frees memory
  // ----------------------------------------------------------------------------------------
  finish:
    printf("\n %6.0f trajectories written to binary file %s !\n", NumNeutRead, BinaryFileName);
  }

  /* do module specific cleanups */
  OwnCleanup();
  
  /* do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
short OwnInit(void) 
{
  NumNeutRead=0.0;
  bTrace = FALSE;

  // read ASCII input file
  printf("Give ASCII file name : ");
  scanf ("%s", AsciiFileName);
	
  pAsciiFile = OpenInputFile(AsciiFileName, FALSE, "r");
  if (pAsciiFile==NULL) 
  { 
    printf("Can't open file %s\n", AsciiFileName);
    return(FALSE);
  }

  // write binary file to the same folder
  printf("Give binary file name: ");
  scanf ("%s", BinaryFileName);

  OutputFilePtr = OpenInputFile(BinaryFileName, FALSE, "wb");
  if (OutputFilePtr==NULL)
  { printf("Can't open file %s\n", BinaryFileName);
    return(FALSE);
  }
  else
  { return(TRUE);
  }
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  printf("\n Hit [Enter] to terminate ! \n");
  getchar();
  getchar();
  getchar();
  
  if (pAsciiFile!=NULL)
    fclose(pAsciiFile);
}
