/********************************************************************************************/
/*  VITESS module 'frame.c'                                                                 */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* Author: Géza Zsigmond,                                                                   */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Mar 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "softabort.h"

/*********************************/
/** Global and Static Variables **/
/*********************************/
McCompID   _eModule=MCN_FRAME;

// input parameters
char		   S1='x', S2, S3;             // -S       [-]   sequence of operations RTM RMT TRM TMR MTR MRT
int 		   MirrX=FALSE,                // -i       [-]   flag: mirror trajectories at yz-plane            
           MirrY=FALSE,                // -j       [-]   flag: mirror trajectories at xz-plane 
           MirrZ=FALSE;                // -k       [-]   flag: mirror trajectories at xy-plane
double		 AnglAroundZ=0.0,            // -H      [deg]  rotation angle about z-axis
           AnglAroundY=0.0,            // -V      [deg]  rotation angle about y-axis
           AnglAroundX=0.0;            // -A      [deg]  rotation angle about x-axis
VectorType Translate={0.0,0.0,0.0};    // -x -y -z [cm]  translation vector

// Variables determined from input parameters or trajectory data
double     RotMatrixZY[3][3],          // [-]            rotation matrix to execute rotation about z- and y-axis
           RotMatrixAroundX[3][3];     // [-]            rotation matrix to execute rotation about x-axis
VectorType Translate1={0.0,0.0,0.0};   // [cm]           translation vector for 'instrument.inf'


/******************************/
/** Prototypes               **/
/******************************/
void    OwnInit(int argc, char *argv[]);     // Reads input parameters, sets global parameters and calculates rotation matrices etc.
void    OwnCleanup();                        // Does module specific cleanup
void		Rotation (Neutron* pNeutron);        // rotates position, direction and spin about z-, y- and x-axis
void		Mirroring(Neutron* pNeutron);        // mirrors position, direction and spin about z-, y- and x-axis


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  long i;
  Neutron OutNeutron;

  // Initialize the program according to the parameters given
  // --------------------------------------------------------
	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
	OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  DECLARE_ABORT;

	// loop over trajectories
  // ----------------------
  /* Get the neutrons from the file */
  while((ReadNeutrons())!= 0)
  {
    for(i=0;i<NumNeutGot;i++)
    {
      CHECK;	
      CopyNeutron(&InputNeutrons[i], &OutNeutron);

      if ((S1 == 'R') && (S2 == 'T') && (S3 == 'M'))
      {
        Rotation(&OutNeutron); SubVector(OutNeutron.Position, Translate); Mirroring(&OutNeutron); goto output;
      }

      if ((S1 == 'R') && (S2 == 'M') && (S3 == 'T'))
      {
        Rotation(&OutNeutron); Mirroring(&OutNeutron); SubVector(OutNeutron.Position, Translate); goto output;
      }

      if ((S1 == 'T') && (S2 == 'R') && (S3 == 'M'))
      {
        SubVector(OutNeutron.Position, Translate); Rotation(&OutNeutron); Mirroring(&OutNeutron); goto output;
      }

      if ((S1 == 'M') && (S2 == 'R') && (S3 == 'T'))
      {
        Mirroring(&OutNeutron); Rotation(&OutNeutron); SubVector(OutNeutron.Position, Translate); goto output;
      }

      if ((S1 == 'T') && (S2 == 'M') && (S3 == 'R'))
      {
        SubVector(OutNeutron.Position, Translate); Mirroring(&OutNeutron); Rotation(&OutNeutron); goto output;
      }

      if ((S1 == 'M') && (S2 == 'T') && (S3 == 'R'))
      {
        Mirroring(&OutNeutron); SubVector(OutNeutron.Position, Translate); Rotation(&OutNeutron);  goto output;
      }

      /* writes output binary file */
     output:
      WriteNeutron(&OutNeutron);
    }
  }

  // Finish: writes and closes monitor files, writes to log and instrument file, frees memory
  // ----------------------------------------------------------------------------------------
 my_exit:;
	/* Does module specific cleanup */
	OwnCleanup();

  /* Does the general cleanup */
  Cleanup(Translate1[0],Translate1[1],Translate1[2], AnglAroundZ*M_PI/180., AnglAroundY*M_PI/180.);

  return 0;
}


/*******************************************************/
/** rotation                                          **/
/*******************************************************/
void		Rotation(Neutron* pNeutron)
{
  RotVector(RotMatrixZY,      pNeutron->Position);
  RotVector(RotMatrixAroundX, pNeutron->Position);

  RotVector(RotMatrixZY,      pNeutron->Vector);
  RotVector(RotMatrixAroundX, pNeutron->Vector);

  RotVector(RotMatrixZY,      pNeutron->Spin);
  RotVector(RotMatrixAroundX, pNeutron->Spin);
}


/*******************************************************/
/** mirroring                                         **/
/*******************************************************/
void		Mirroring(Neutron* pNeutron)
{
  if(MirrX == 1) pNeutron->Position[0] *= - 1.;
  if(MirrY == 1) pNeutron->Position[1] *= - 1.;
  if(MirrZ == 1) pNeutron->Position[2] *= - 1.;

  if(MirrX == 1) pNeutron->Vector[0] *= - 1.;
  if(MirrY == 1) pNeutron->Vector[1] *= - 1.;
  if(MirrZ == 1) pNeutron->Vector[2] *= - 1.;

  if(MirrX == 1) pNeutron->Spin[0] *= - 1.;
  if(MirrY == 1) pNeutron->Spin[1] *= - 1.;
  if(MirrZ == 1) pNeutron->Spin[2] *= - 1.;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'S':
        switch (argv[1][2]) 
        {
          case '1' : S1 = 'R'; S2 = 'T'; S3='M'; break;
          case '2' : S1 = 'R'; S2 = 'M'; S3='T'; break;
          case '3' : S1 = 'T'; S2 = 'R'; S3='M'; break;
          case '4' : S1 = 'T'; S2 = 'M'; S3='R'; break;
          case '5' : S1 = 'M'; S2 = 'T'; S3='R'; break;
          case '6' : S1 = 'M'; S2 = 'R'; S3='T'; break;
        }
        break;

      case 'H':
        sscanf(&argv[1][2], "%lf", &AnglAroundZ);
        break;

      case 'V':
        sscanf(&argv[1][2], "%lf", &AnglAroundY);
        break;

      case 'A':
        sscanf(&argv[1][2], "%lf", &AnglAroundX);
        break;

      case 'x':
        sscanf(&argv[1][2], "%lf", &Translate[0]);
        break;

      case 'y':
        sscanf(&argv[1][2], "%lf", &Translate[1]);
        break;

      case 'z':
        sscanf(&argv[1][2], "%lf", &Translate[2]);
        break;

      case 'i':
        sscanf(&argv[1][2], "%d", &MirrX);
        break;

      case 'j':
        sscanf(&argv[1][2], "%d", &MirrY);
        break;

      case 'k':
        sscanf(&argv[1][2], "%d", &MirrZ);
        break;
    }
    argc--;
    argv++;
  }

  if (S1 != 'x') 
    fprintf(LogFilePtr, " sequence		%c %c %c\n", S1, S2, S3);
  else 
    Error("wrong option for sequence. Give R,T or M");

  if ((MirrX != 0) &&	(MirrX != 1))
    Error("wrong option for MirrX");

  if ((MirrY != 0) &&	(MirrY != 1))
    Error("wrong option for MirrY");

  if ((MirrZ != 0) &&	(MirrZ != 1))
    Error("wrong option for MirrZ");

  fprintf(LogFilePtr, " angle around  Z, Y, X: %lf  %lf  %lf\n translate     x, y, z: %lf  %lf  %lf\n",
                      AnglAroundZ, AnglAroundY, AnglAroundX, Translate[0], Translate[1], Translate[2]);

  fprintf(LogFilePtr," mirror YZ,ZX,XY %ld %ld %ld\n",MirrX, MirrY, MirrZ);

  FillRotMatrixZY(RotMatrixZY, AnglAroundY * M_PI/180., AnglAroundZ * M_PI/180.);

  FillRotMatrixXZ(RotMatrixAroundX, 0., AnglAroundX * M_PI/180.);
  FillRotMatrixXZ(RotMatrixMX,      0., AnglAroundX * M_PI/180.);

  fprintf(LogFilePtr, "Euler frame rotations: Positive rotation of frame means rotation of one positive \n"
                      "axis towards a higher index positive axis : +X->+Y, +Y->+Z, +X->+Z \n");

  /* here calculates Translate1 as input for Cleanup */
  if ((S1 == 'R') && (S2 == 'T') && (S3 == 'M'))
  {
    CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); 
  }

  if ((S1 == 'R') && (S2 == 'M') && (S3 == 'T'))
  {
    CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); 
  }

  if ((S1 == 'T') && (S2 == 'R') && (S3 == 'M'))
  {
    CopyVector(Translate, Translate1);
  }

  if ((S1 == 'M') && (S2 == 'R') && (S3 == 'T'))
  {
    CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); 
  }

  if ((S1 == 'T') && (S2 == 'M') && (S3 == 'R'))
  {
    CopyVector(Translate, Translate1);	    
  }

  if ((S1 == 'M') && (S2 == 'T') && (S3 == 'R'))
  {
    CopyVector(Translate, Translate1);	    
  }

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  // do not free(&Neutrons); because Neutrons is a static variable
}

