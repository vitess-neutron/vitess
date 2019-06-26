/* START HEADER STORY */

#define	STRING_BUFFER 50
#define	FIELD_SIZE	100

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 

	FILE		*FieldMapFile, *XFILE;
	char		*FieldFileName, XFileName[STRING_BUFFER];
	long		Option, User, coildir, NumOut, Repetition, repet,  i, wall_1, wall_2, ind_x, ind_y, ind_z, ind_x_max, ind_y_max, ind_z_max ;
	double		field_guide[3], field_parameter, field_precession, TOF, TOF1, TOF2, TOF3, WL, Prob, phi, the, PhaseShift, NumberPrecessions ;
	double		width, height, depth, field_hom[3], AnglMainHoriz, AnglMainVert, ProbCutoff, IntegralIntensity ;
	static double	domain_field_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], 
			PosDomain_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], DimDomain_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], RotMatrixField[3][3], LarmorMatrix[3][3];
	VectorType	 Pos, Dir, SpinVector, Path, Pos1, Pos2, domain_field, PosDomain, PosMain, DimDomain, TranslOut, FWHM ;
	Neutron		Neutrons ;

	void		writemagneticmap();
	void		readmagneticmap();
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
	void		ReadParameterFile() ;
	void		CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3][3]) ;
	void		CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) ;
	void		CopyVectorsToVector3(long i, long j, long k, double Vector[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3]) ;
	void		CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) ;
	long		IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;

/* FINISH HEADER STORY */

