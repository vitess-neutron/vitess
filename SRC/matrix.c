/***************************************************************/
/*  VITESS module 'matrix.c'                                   */
/*    Functions for all VITESS modules:                        */
/*      - vector operations                                    */
/*      - matrix operations                                    */
/*      - transformation of co-ordinate systems                */
/*                                                             */
/* The free non-commercial use of these routines is granted    */
/* providing due credit is given to the authors:               */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,        */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin           */ 
/*                                                             */


/* Change: K. L. JAN 2002, reorganized routines                                     */
/* Change: G. Zs. 16 JUL 2002, "Higher index" convention, included routines         */


#include <math.h>
#include <string.h>
#include "general.h"
#include "matrix.h"

/* IMPORTANT NOTE         "Higher index" rotation convention:                    */
/*                                                                               */
/* Positive rotation of frame means rotation of one positive                     */
/* axis towards a higher index positive axis :                                   */
/*                      +X->+Y, +Y->+Z, +X->+Z                                   */




/*  Prototypes of local functions    */
/*  -----------------------------    */
void   MatrixIndexUp(double Matrix[3][3]);
void   MatrixIndexDown(double Matrix[3][3]);



/* 'OutputRotMatrix' computes the default matrix of the output angles */
/* after reflection                                                   */
/* Author: G. Zsigmond                                                */
void	OutputRotMatrix(double Matrix[3][3], double OutMatrix[3][3])
{
double n[3] ;

	n[0] = 1. ;
	n[1] = 0. ;
	n[2] = 0. ;

	RotVector(Matrix, n) ;     /* components of a vector parallel to X in the frame of the CE */

	n[0] *= -1. ;              /* reflection */

	RotBackVector(Matrix, n) ; /* new components in the frame of input */

	RotMatrixX(n, OutMatrix) ; /* calculates the rotation matrix corresponding to the new default direction */
}



/* 'RotMatrixX' computes a rotation matrix which rotates the frame  */
/* to get 'Vector' parallel to the new X axis                       */
/* Author: G. Zsigmond                                              */
void RotMatrixX(VectorType Vector, double Matrix[3][3])
{
	double roty, rotz;
  
	CartesianToEulerZY(Vector, &roty, &rotz) ;
	FillRotMatrixZY(Matrix, roty, rotz) ;
}



/* 'FillRotMatrixXZ' calculates a rotation matrix, which rotates a frame   */
/* at first about the x-axis by 'rotx' and then about the z-axis by 'rotz' */
/*  Author: F. Streffer.                                                   */
/*  Change: G. Zs. 16 JUL 2002  rotation convention                        */
void FillRotMatrixXZ(double RotMatrix[3][3], double rotz, double rotx)
{
  double sx, cx, sz, cz;
  long   i,j;

  sz= (double) sin(rotz);
  cz= (double) cos(rotz);
  sx= (double) sin(rotx);
  cx= (double) cos(rotx);

  /* now, fill the matrix */
  RotMatrix[0][0] =  cz;
  RotMatrix[0][1] =  sz*cx;
  RotMatrix[0][2] =  sz*sx;
  RotMatrix[1][0] =  -sz;
  RotMatrix[1][1] =  cz*cx;
  RotMatrix[1][2] =  cz*sx;
  RotMatrix[2][0] =  0.0;
  RotMatrix[2][1] =  -sx;
  RotMatrix[2][2] =  cx;

  /* cutoff very small matrix elements */
  for(i=0; i<3; i++)
    for(j=0; j<3; j++)
      if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}


/* 'FillRotMatrixYX' calculates a rotation matrix, which rotates a frame   */
/* at first about the y-axis by 'roty' and then about the x-axis by 'rotx' */
/*  Author: F. Streffer.                                                   */
/*  Change: G. Zs. 16 JUL 2002  rotation convention                        */
void FillRotMatrixYX(double RotMatrix[3][3], double rotx, double roty)
{
	double sx, cx, sy, cy;
	int   i,j;

	sx= (double) sin(rotx);
	cx= (double) cos(rotx);
	sy= (double) sin(roty);
	cy= (double) cos(roty);

	/* now, fill the matrix */
	RotMatrix[0][0] =  cy;
	RotMatrix[0][1] =  0.0;
	RotMatrix[0][2] =  sy;
	RotMatrix[1][0] =  -sy*sx;
	RotMatrix[1][1] =  cx;
	RotMatrix[1][2] =  cy*sx;
	RotMatrix[2][0] =  -sy*cx;
	RotMatrix[2][1] =  -sx;
	RotMatrix[2][2] =  cy*cx;

	/* cutoff very small matrix elements */
	for(i=0; i<3; i++)
	  for(j=0; j<3; j++)
	    if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}


/* 'FillRotMatrixZY' calculates a rotation matrix, which rotates a frame  */
/* at first about the z-axis by 'rotz' and the about the y-axis by 'roty' */
/*  Author: G. Zsigmond                                                                  */
void FillRotMatrixZY(double RotMatrix[3][3], double roty, double rotz)
{
	FillRotMatrixYX(RotMatrix, -roty, -rotz) ;

	MatrixIndexUp(RotMatrix) ;
}


/* These 3 functions calculate a rotation matrices, which rotate a frame   */
/* about the X-axis by rotx etc by using the "Higher index" convention   */
/*  Author: G. Zs.                                                       */

void FillRotMatrixX(double RotMatrix[3][3], double rotx)
{
	double sx, cx;
	int   i,j;

	sx=(double) sin(rotx);
	cx=(double) cos(rotx);

	/* now, fill the matrix */
	RotMatrix[0][0] =  1.;
	RotMatrix[0][1] =  0.;
	RotMatrix[0][2] =  0.;
	RotMatrix[1][0] =  0.;
	RotMatrix[1][1] =  cx;
	RotMatrix[1][2] =  sx;
	RotMatrix[2][0] =  0.;
	RotMatrix[2][1] =  - sx;
	RotMatrix[2][2] =  cx;

	/* cutoff very small matrix elements */
	for(i=0; i<3; i++)
	  for(j=0; j<3; j++)
	    if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}

void FillRotMatrixY(double RotMatrix[3][3], double roty)
{
	double sy, cy;
	int   i,j;

	sy=(double) sin(roty);
	cy=(double) cos(roty);

	/* now, fill the matrix */
	RotMatrix[0][0] =  cy;
	RotMatrix[0][1] =  0.;
	RotMatrix[0][2] =  sy;
	RotMatrix[1][0] =  0.;
	RotMatrix[1][1] =  1.;
	RotMatrix[1][2] =  0.;
	RotMatrix[2][0] =  - sy;
	RotMatrix[2][1] =  0.;
	RotMatrix[2][2] =  cy;

	/* cutoff very small matrix elements */
	for(i=0; i<3; i++)
	  for(j=0; j<3; j++)
	    if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}

void FillRotMatrixZ(double RotMatrix[3][3], double rotz)
{
	double sz, cz;
	int   i,j;

	sz=(double) sin(rotz);
	cz=(double) cos(rotz);

	/* now, fill the matrix */
	RotMatrix[0][0] =  cz;
	RotMatrix[0][1] =  sz;
	RotMatrix[0][2] =  0.;
	RotMatrix[1][0] =  - sz;
	RotMatrix[1][1] =  cz;
	RotMatrix[1][2] =  0.;
	RotMatrix[2][0] =  0.;
	RotMatrix[2][1] =  0.;
	RotMatrix[2][2] =  1.;

	/* cutoff very small matrix elements */
	for(i=0; i<3; i++)
	  for(j=0; j<3; j++)
	    if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}


/* 'RotMatrixToAnglesZY' calculates rotation angles from a rotation matrix */
/* if the first rotation is about the axis Z and the second about axis Y   */
/*  Author: G. Zsigmond                                                                   */
void RotMatrixToAnglesZY(double RotMatrix[3][3], double *roty, double *rotz)
{
	*rotz = (double) atan2(- RotMatrix[1][0], RotMatrix[1][1]) ;

	*roty = (double) atan2(RotMatrix[0][2], RotMatrix[2][2]) ;
}



/* 'CartesianToEulerZY' calculates Euler angles 'rotz' and 'roty'         */
/* to transfer the x-axis to 'Vector' by rotation around y- and z-axis ZY */
/* (cf. FillRotMatrixZY)                                                  */
/*  Author: G. Zsigmond                                                   */
void CartesianToEulerZY(VectorType Vector, double *roty, double *rotz)
{
	*rotz = (double) atan2( Vector[1] , Vector[0] ) ;

	*roty = (double) atan2( Vector[2] , ((double) cos(*rotz) * Vector[0] + (double) sin(*rotz) * Vector[1]) ) ;
}

/* Euler to cartesian - invers of previous                            */
/*  Author: G. Zsigmond                                               */

void EulerToCartesianZY(VectorType Vector, double *roty, double *rotz)
{

	Vector[0]= (double) cos(*roty) * (double) cos(*rotz) ;
	Vector[1]= (double) cos(*roty) * (double) sin(*rotz) ;
	Vector[2]= (double) sin(*roty) ;

}


/* 'CartesianToSpherical' calculates Theta and Phi of a unit vector  */
/* if Theta is the angle with the axis of lowest index               */
/*  Author: G. Zsigmond                                              */
void CartesianToSpherical(VectorType Vector, double *Theta, double *Phi)
{
	*Theta	= (double) acos(Vector[0]) ;

	*Phi	= (double) atan2(Vector[2], Vector[1]) ;
}


/* 'SphericalToCartesian' calculates the cartesian unit vector       */
/* if theta is the angle with the axis of lowest index               */
/*  Author: G. Zsigmond                                              */
void SphericalToCartesian(VectorType Vector, double *Theta, double *Phi)
{
	Vector[0]= (double) cos(*Theta) ;
	Vector[1]= (double) sin(*Theta) * (double) cos(*Phi) ;
	Vector[2]= (double) sin(*Theta) * (double) sin(*Phi) ;
}

/* compare two vectors and return 1 if they coincide */
int    CompareVectors(VectorType Vector1, VectorType Vector2)
{
if((fabs(Vector1[0]-Vector2[0])<1.e-10)&&(fabs(Vector1[1]-Vector2[1])<1.e-10)&&(fabs(Vector1[2]-Vector2[2])<1.e-10)) return 1; /* ~0.25e-14 was a critical limit*/
else return 0;
}



/******************************************************************/
/*     Local Functions                                            */
/******************************************************************/

/* 'MatrixIndexUp' drops matrix indexes by "1"                        */
/*  Author: G. Zsigmond                                               */
void	MatrixIndexUp(double Matrix[3][3])
{
	int i ;
	double Result[3][3] ;

	for(i=0; i<3; i++)
	{
	  Result[0][i]=Matrix[2][i] ;
	  Result[1][i]=Matrix[0][i] ;
	  Result[2][i]=Matrix[1][i] ;
	}
	for(i=0; i<3; i++)
	{
	  Matrix[i][0]=Result[i][2] ;
	  Matrix[i][1]=Result[i][0] ;
	  Matrix[i][2]=Result[i][1] ;
	}
}

/* 'MatrixIndexUp' rises matrix indexes by "1"                        */
/*  Author: G. Zsigmond                                               */
void	MatrixIndexDown(double Matrix[3][3])
{
	int i ;
	double Result[3][3] ;

	for(i=0; i<3; i++)
	{
	  Result[i][0]=Matrix[i][1] ;
	  Result[i][1]=Matrix[i][2] ;
	  Result[i][2]=Matrix[i][0] ;
	}
	for(i=0; i<3; i++)
	{
	  Matrix[0][i]=Result[1][i] ;
	  Matrix[1][i]=Result[2][i] ;
	  Matrix[2][i]=Result[0][i] ;
	}
}

