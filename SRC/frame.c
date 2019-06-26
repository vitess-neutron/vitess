/********************************************************************************************/
/*  VITESS module 'frame.c'                                                                 */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* Author: Géza Zsigmond,                                                                   */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "softabort.h"

/* START HEADER STORY */

char		S1, S2, S3;
long		i, MirrX, MirrY, MirrZ;
double		AnglAroundZ, AnglAroundY, AnglAroundX, RotMatrixZY[3][3], RotMatrixAroundX[3][3];
VectorType	Translate, Translate1;
Neutron		Neutrons;

void		OwnInit(int argc, char *argv[]);
void		OwnCleanup();
void		Rotation();
void		Mirroring();

/* FINISH HEADER STORY */

int main(int argc, char **argv)
{

  /* Initialize the program according to the parameters given  */

  Init(argc, argv, VT_FRAME);

  OwnInit(argc, argv);/**/

  /* Get the neutrons from the file */
  DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
    {
      CHECK;     /* here is what happens to the neutron */

      for(i=0;i<NumNeutGot;i++)
	{
      CHECK;	

	  if((S1 == 'R') && (S2 == 'T') && (S3 == 'M'))
	    {
	      Rotation(); SubVector(InputNeutrons[i].Position, Translate); Mirroring(); goto output;
	    }

	  if((S1 == 'R') && (S2 == 'M') && (S3 == 'T'))
	    {
	      Rotation(); Mirroring(); SubVector(InputNeutrons[i].Position, Translate); goto output;
	    }

	  if((S1 == 'T') && (S2 == 'R') && (S3 == 'M'))
	    {
	      SubVector(InputNeutrons[i].Position, Translate); Rotation(); Mirroring(); goto output;
	    }

	  if((S1 == 'M') && (S2 == 'R') && (S3 == 'T'))
	    {
	      Mirroring(); Rotation(); SubVector(InputNeutrons[i].Position, Translate); goto output;
	    }

	  if((S1 == 'T') && (S2 == 'M') && (S3 == 'R'))
	    {
	      SubVector(InputNeutrons[i].Position, Translate); Mirroring(); Rotation(); goto output;
	    }

	  if((S1 == 'M') && (S2 == 'T') && (S3 == 'R'))
	    {
	      Mirroring(); SubVector(InputNeutrons[i].Position, Translate); Rotation();  goto output;
	    }

	  /* writes output binary file */

	output:;

      WriteNeutron(&(InputNeutrons[i]));
	}
    }
my_exit:;

  /* Do the general cleanup */
  Cleanup(Translate1[0],Translate1[1],Translate1[2], AnglAroundZ*M_PI/180., AnglAroundY*M_PI/180.);

  return 0;
}


/* rotation */

void		Rotation()
{
  RotVector(RotMatrixZY, InputNeutrons[i].Position);

  RotVector(RotMatrixAroundX, InputNeutrons[i].Position);


  RotVector(RotMatrixZY, InputNeutrons[i].Vector);

  RotVector(RotMatrixAroundX, InputNeutrons[i].Vector);


  RotVector(RotMatrixZY, InputNeutrons[i].Spin);

  RotVector(RotMatrixAroundX, InputNeutrons[i].Spin);
}


/* mirroring */

void		Mirroring()
{
  if(MirrX == 1) InputNeutrons[i].Position[0] *= - 1.;

  if(MirrY == 1) InputNeutrons[i].Position[1] *= - 1.;

  if(MirrZ == 1) InputNeutrons[i].Position[2] *= - 1.;


  if(MirrX == 1) InputNeutrons[i].Vector[0] *= - 1.;

  if(MirrY == 1) InputNeutrons[i].Vector[1] *= - 1.;

  if(MirrZ == 1) InputNeutrons[i].Vector[2] *= - 1.;


  if(MirrX == 1) InputNeutrons[i].Spin[0] *= - 1.;

  if(MirrY == 1) InputNeutrons[i].Spin[1] *= - 1.;

  if(MirrZ == 1) InputNeutrons[i].Spin[2] *= - 1.;
}


/* own initialization  */

void OwnInit(int argc, char *argv[])
{
  print_module_name("frame 1.2a");

  /*    INPUT  */
  S1 = 'x';
  while(argc>1)
    {
      switch(argv[1][1])
	{
	case 'S':
	    switch (argv[1][2]) {
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
	  sscanf(&argv[1][2], "%ld", &MirrX);
	  break;

	case 'j':
	  sscanf(&argv[1][2], "%ld", &MirrY);
	  break;

	case 'k':
	  sscanf(&argv[1][2], "%ld", &MirrZ);
	  break;
	}
      argc--;
      argv++;
    }

  if (S1 != 'x') {
    fprintf(LogFilePtr, "	sequence		%c %c %c", S1, S2, S3);
  } else {
    fprintf(LogFilePtr,"\nERROR: wrong option for sequence. Give R,T or M!");
    exit(0);
  }

  if((MirrX != 0) &&	(MirrX != 1))
    {
      fprintf(LogFilePtr,"\nERROR: wrong option for MirrX!");
      exit(0);
    }

  if((MirrY != 0) &&	(MirrY != 1))
    {
      fprintf(LogFilePtr,"\nERROR: wrong option for MirrY!");
      exit(0);
    }

  if((MirrZ != 0) &&	(MirrZ != 1))
    {
      fprintf(LogFilePtr,"\nERROR: wrong option for MirrZ!");
      exit(0);
    }

  fprintf(LogFilePtr,
	  "\n	angle around  Z, Y, X	%lf  %lf  %lf\n	translate        x, y, z	%lf  %lf  %lf",
	  AnglAroundZ, AnglAroundY, AnglAroundX, Translate[0], Translate[1], Translate[2]);

  fprintf(LogFilePtr,"\n	mirror	     YZ,ZX,XY	%ld %ld %ld\n",MirrX, MirrY, MirrZ);


  FillRotMatrixZY(RotMatrixZY, AnglAroundY * M_PI/180., AnglAroundZ * M_PI/180.);

  FillRotMatrixXZ(RotMatrixAroundX, 0., AnglAroundX * M_PI/180.);
  FillRotMatrixXZ(RotMatrixMX, 0., AnglAroundX * M_PI/180.);

  fprintf(LogFilePtr, "\nEuler frame rotations: Positive rotation of frame means rotation of one positive \n"
					  "axis towards a higher index positive axis : +X->+Y, +Y->+Z, +X->+Z \n\n");

  

/* here calculates Translate1 as input for Cleanup */

  	  if((S1 == 'R') && (S2 == 'T') && (S3 == 'M'))
	    {
		CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); }

	  if((S1 == 'R') && (S2 == 'M') && (S3 == 'T'))
	    {
		CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); }

	  if((S1 == 'T') && (S2 == 'R') && (S3 == 'M'))
	    {
		CopyVector(Translate, Translate1);	    }

	  if((S1 == 'M') && (S2 == 'R') && (S3 == 'T'))
	    {
		CopyVector(Translate, Translate1); RotBackVector(RotMatrixZY, Translate1); }

	  if((S1 == 'T') && (S2 == 'M') && (S3 == 'R'))
	    {
		CopyVector(Translate, Translate1);	    }

	  if((S1 == 'M') && (S2 == 'T') && (S3 == 'R'))
	    {
		CopyVector(Translate, Translate1);	    }
  

}/* End OwnInit */


void OwnCleanup()
{
  // do not free(&Neutrons); because Neutrons is a static variable
}

