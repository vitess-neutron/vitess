/*********************************************************************************************/
/* VITESS module spin_reset                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.1  Apr 2008  K. Lieutenant  preliminary version                                         */
/* 1.0  Jan 2010  K. Lieutenant  first version, containing colour reset                      */
/* 1.1  Mar 2020  K. Lieutenant  new central visualization parameters                        */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID _eModule=MCN_RESET;

short  nColors     =  0,
       iColor,  
       bSetColor   =  FALSE,
       bSetPol     =  FALSE;
double PolVecX     =  0.0,  /* polarisation            */
       PolVecY     =  0.0, 
       PolVecZ     =  0.0,
       PolNorm,             /* length of polarization vector given by user    */
       PolDegree   =  0.0,  /* degree of polarization [%]                     */
       FracPolDir  =  0.0;  /* fraction of neutrons in polarization direction */


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]);     // Reads input parameters and sets global parameters
void OwnCleanup();                        // Does module specific cleanup


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
	int i;

	/* Initialize the program according to the parameters given   */
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.1");
  OwnInit(argc, argv);    // module specific initialization
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

	/* redefinition in terms of eigenvectors e.g. 0 % means 50% Up and 50% Down */
	FracPolDir  = 0.5 + 0.5*PolDegree/100.0;

	DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
	/* Get the neutrons from the file */
	while((ReadNeutrons())!= 0)
	{
		for(i=0; i<NumNeutGot; i++) 
		{
  		CHECK;    
			if (bSetPol) 
			{	/* Polarization - spin vectors selected for each trajectory 
				   from one of the eigenvectors  in the polarisation direction */
				if (Vran() <= FracPolDir) 
				{	/* spin eigenvector No 1 */
					InputNeutrons[i].Spin[0]= PolVecX; 
					InputNeutrons[i].Spin[1]= PolVecY; 
					InputNeutrons[i].Spin[2]= PolVecZ; 
				}
				else
				{	/* spin eigenvector No 2 */
					InputNeutrons[i].Spin[0]= -PolVecX; 
					InputNeutrons[i].Spin[1]= -PolVecY; 
					InputNeutrons[i].Spin[2]= -PolVecZ; 
				} 
			}

			if (bSetColor)
			{	iColor = (short) (i % nColors)+1;
				InputNeutrons[i].Color = iColor;
			}

			WriteNeutron(&(InputNeutrons[i]));
		}
	}
  
  // Finish: writes and closes monitor files, writes to log and instrument file, frees memory
  // ----------------------------------------------------------------------------------------
 my_exit:
	/* Do module specific cleanup */
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
	{	if(argv[i][0]!='+') 
		{	switch(argv[i][1])
			{ 
				/* colour */
				case 'c':
				  nColors = atoi(&argv[i][2]);    // number of colours to assign  
				  break;

				/* polarization */
				case 'X':
				  PolVecX = atof(&argv[i][2]);  
				  break;
				case 'Y':
				  PolVecY = atof(&argv[i][2]);  
				  break;
				case 'Z':
				  PolVecZ = atof(&argv[i][2]);  
				  break;
				case 'P':
				  PolDegree = atof(&argv[i][2]); 
				  if(fabs(PolDegree) > 100.)
				   Error("polarization degree must be <= 100 ");
				  break;

				default:
				  fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
				  exit(-1);
				  break;
			}
		}
	}

	if(nColors==0)
		bSetColor=FALSE;
	else
		bSetColor=TRUE;

	PolNorm = sqrt(PolVecX*PolVecX + PolVecY*PolVecY + PolVecZ*PolVecZ);
	if(PolNorm==0.0) 
	{	bSetPol=FALSE;
	}
	else
	{	bSetPol=TRUE;
		PolVecX=PolVecX/PolNorm;
		PolVecY=PolVecY/PolNorm;
		PolVecZ=PolVecZ/PolNorm;
	}
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
	return;
}
