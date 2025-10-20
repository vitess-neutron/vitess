/* START HEADER STORY */

#include "defines.h"

#define  STRING_BUFFER 50
#define  FIELD_SIZE_X  1600
#define FIELD_SIZE_Y    25
#define FIELD_SIZE_Z    25

void    OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global parameters
void    OwnCleanup();                      // Does module specific cleanup
void    writemagneticmap();                // writes the magnetic map file
void    readmagneticmap();                 // reads the magnetic map file
void    SetGeometry(char* sColor);         // Fills the structure stGeometry for visualization
// void    OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
// void    ReadParameterFile() ;
/* copy matrix/vector to 3D array of matrices/vectors or back */
void    CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FIELD_SIZE_X][FIELD_SIZE_Y][FIELD_SIZE_Z], double Result[3][3]) ;
void    CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FIELD_SIZE_X][FIELD_SIZE_Y][FIELD_SIZE_Z]) ;
void    CopyVectorsToVector3(long i, long j, long k, double Vector[3][FIELD_SIZE_X][FIELD_SIZE_Y][FIELD_SIZE_Z], double Result[3]) ;
void    CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FIELD_SIZE_X][FIELD_SIZE_Y][FIELD_SIZE_Z]) ;
/* Intersection with rectangular object */
long    IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;

/* FINISH HEADER STORY */
