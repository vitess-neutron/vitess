/* START HEADER STORY */

#define	STRING_BUFFER 50
#define	FIELD_SIZE	100

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 


void		OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global parameters
void		OwnCleanup();                      // Does module specific cleanup
void		writemagneticmap();                // writes the magnetic map file
void		readmagneticmap();                 // reads the magnetic map file
void    SetGeometry(char* sColor);         // Fills the structure stGeometry for visualization 
// void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
// void		ReadParameterFile() ;
/* copy matrix/vector to 3D array of matrices/vectors or back */
void		CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3][3]) ;
void		CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) ;
void		CopyVectorsToVector3(long i, long j, long k, double Vector[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3]) ;
void		CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) ;
/* Intersection with rectangular object */
long		IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;

/* FINISH HEADER STORY */

