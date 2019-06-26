/* START HEADER STORY */

#define	FIELD_SIZE	3000
#define	FIELD_SIZE_FL	9000

#define	ENERGY_FROM_LAMBDA(x) ( 81805.048 / x / x ) /*[ueV]*/
#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 

	long		NumOut,  i, wall_1, wall_2, ind_x, ind_y, ind_z, indp=1, ind_x_max=2, ind_y_max=2, ind_z_max=2 ; /* Number of domains in the X, Y and Z directions */
	
	double		field_guide[3], field_parameter, field_precession, TOF, TOF1, TOF2, TOF3, WL, Prob, phi, the, PhaseShift, NumberPrecessions ;
	
	double		NumberPrecessionsave, NumberPrecessionssum, PhaseShift0;
	
	double		width, height, depth, IntegralIntensity;
	
	static double	RotMatrixField[3][3], LarmorMatrix[3][3];
				
	static double	PolX[FIELD_SIZE_FL], PolY[FIELD_SIZE_FL], PolZ[FIELD_SIZE_FL], ProbM[FIELD_SIZE_FL],
			FielX[FIELD_SIZE_FL], FielY[FIELD_SIZE_FL], FielZ[FIELD_SIZE_FL], FielM[FIELD_SIZE_FL];
	
	VectorType	 Pos, Dir, SpinVector, Path, Pos1, Pos2, domain_field, PosDomain, PosMain, DimDomain, TranslOut ;
	Neutron		Neutrons ;
	


	double FieldValue, FieldValueInit;
	double FieldValueDevPer=0.0; /* Deviation of amplitude of periodical magnetic field in %! */
	long DevLawAmpl=0; /* Law of distribution of amplitude of periodical field: 0 - Normal (Default), 1 -Uniform */
	double Number_NOP=0.0;
	double TOFP; /* Neutron TOF from preceding modules for synhro rotations */
	long keyaxis=0; /* Value 0, 1, 2 - periodical magnetic field parallel to axis 0x, 0y, 0z respectivly */
	long keysph=0; /* Key for activate output polarisation components in the file, default no */
	double FieldValue0Init[3], FieldValue0[3]; /* Additional permanent field , initial and current */
	double FieldValue0Dev=0.0; /* Additional random magnetic field, amplitude, Oe */
	double FieldValue0Length, OmegaFV0, OmegaFV0in; /* Internal variable */
	double spacecurr=0.0, spacemin=0.0, spacemax=0.0; /* Min and max values spacing between domains */
	long keyperiod=0; /* for periodical algorithm */
	long keyampldistr=0; /* Key for choosing the distrinution of amplitude of the periodical magnetic field */
	double SigmaNorm=1; /* SKO in the gauss distribution for amplitude of periodical magnetic field */ 


	/* For random amplitude and frequency of magnetic field */	
	double FieldValueA=0.0, FieldValueB=0.0; /* Internal variables */
	double SigmaField=0.0;  /* Internal Variable */
	
	
/* For output polarization components and magnetic field */	
	FILE *fmonitp=NULL;
	char *Monitp=NULL;

	FILE *fmonitf=NULL;
	char *Monitf=NULL;	

/* Functions prototypes */

	long		IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;
	double 		RectangularF(double, double, double);
	double 		DistrGauss(double, double);
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;

/* FINISH HEADER STORY */

