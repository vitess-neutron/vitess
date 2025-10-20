/*********************************************************************************************/
/* VITESS module spin_reset                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.1  Apr 2008  K. Lieutenant  preliminary version                                         */
/* 1.0  Jan 2010  K. Lieutenant  first version, containing colour reset                      */
/* 1.1  Mar 2020  K. Lieutenant  new central visualization parameters                        */
/* 1.2  Jan 2024  K. Lieutenant  correction: color reset according to description            */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
short  nColors    = 0;      // -c   [cm]    number of colours to assign
double PolVecX    = 0.0,    // -X  [1/cm]   x-componont of the polarisation
       PolVecY    = 0.0,    // -Y  [1/cm]
       PolVecZ    = 0.0,    // -Z  [1/cm]
       PolDegree  = 0.0;    // -P   [%]     degree of polarization

// Variables determined from input parameters or trajectory data
short  bSetColor  = FALSE,  //      [-]     flag: color will be set
       bSetPol    = FALSE;  //      [-]     flag: polarisation is set
double PolNorm    = 1.0;    //      [-]     length of polarization vector given by user


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
  int    i=0;
  short  iColor=0;          // color set to a trajectory
  double FracPolDir = 0.0;  // fraction of neutrons in polarization direction

  /* Initialize the program according to the parameters given   */
  _eModule = MCN_RESET;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.2");
  OwnInit(argc, argv);    // module specific initialization

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

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

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        if (bSetPol)
        {  /* Polarization - spin vectors selected for each trajectory
             from one of the eigenvectors  in the polarisation direction */
          if (Vran() <= FracPolDir)
          {  /* spin eigenvector No 1 */
            InputNeutrons[i].Spin[0]= PolVecX;
            InputNeutrons[i].Spin[1]= PolVecY;
            InputNeutrons[i].Spin[2]= PolVecZ;
          }
          else
          {  /* spin eigenvector No 2 */
            InputNeutrons[i].Spin[0]= -PolVecX;
            InputNeutrons[i].Spin[1]= -PolVecY;
            InputNeutrons[i].Spin[2]= -PolVecZ;
          }
        }

        if (bSetColor)
        {  if (nColors < 2)
            iColor = nColors;
          else
            iColor = (short) ceil(MonteCarlo(0.0, (double) nColors));        // (i % nColors)+1;
          InputNeutrons[i].Color = iColor;
        }

        WriteNeutron(&(InputNeutrons[i]));
      }
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
  int i=0;

  for(i=1; i<argc; i++)
  {  if(argv[i][0]!='+')
    {  switch(argv[i][1])
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
          if (fabs(PolDegree) > 100.)
           Error("polarization degree must be <= 100 ");
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  if(nColors < 0)
    bSetColor=FALSE;
  else
    bSetColor=TRUE;

  PolNorm = sqrt(PolVecX*PolVecX + PolVecY*PolVecY + PolVecZ*PolVecZ);
  if(PolNorm==0.0)
  {  bSetPol=FALSE;
  }
  else
  {  bSetPol=TRUE;
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
