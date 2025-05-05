#ifndef MATRIX_H
#define MATRIX_H

#include "general.h"

#ifdef __cplusplus
extern "C" {
#endif

void   OutputRotMatrix    (double Matrix[3][3], double OutMatrix[3][3]);
void   RotMatrixX         (VectorType Vector,   double Matrix[3][3]) ;
void   FillRotMatrixXZ    (double RotMatrix[3][3], double rotz, double rotx);
void   FillRotMatrixYX    (double RotMatrix[3][3], double rotx, double roty);
void   FillRotMatrixZY    (double RotMatrix[3][3], double roty, double rotz);
void   RotMatrixToAnglesZY(double RotMatrix[3][3], double *roty, double *rotz);
void   FillRotMatrixX(double RotMatrix[3][3], double rotx);
void   FillRotMatrixY(double RotMatrix[3][3], double roty);
void   FillRotMatrixZ(double RotMatrix[3][3], double rotz);

void   CartesianToSpherical(VectorType Vector, double *Theta, double *Phi);
void   SphericalToCartesian(VectorType Vector, double *Theta, double *Phi);

int    CompareVectors(VectorType Vector1, VectorType Vector2);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
