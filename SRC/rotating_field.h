/* START HEADER STORY */
/* Last modified Nov 03 */

/* Number of domain*/
#define	FIELD_SIZE	3000	/* must be given by  user */
#define FIELD_SIZE_2	6000	/* must be multipled by 2 */
#define	FIELD_SIZE_FL	9000	/* must be multipled by 3 */
/* After changing this definitions, you must to recompile the module */


#define	ENERGY_FROM_LAMBDA(x) ( 81805.048 / x / x ) /*[ueV]*/
#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 

	long		NumOut, k, i, j, wall_1, wall_2, ind_x, ind_y, ind_z, indp=1, ind_x_max=2, ind_y_max=2, ind_z_max=2 ; /* Number of domains in the X, Y and Z directions */
	
	long		count=1, ntfl=1;
	
	double		field_guide[3], field_parameter, field_precession, TOF, TOF1, TOF2, TOF3, WL, Prob, phi, the, PhaseShift, NumberPrecessions ;
	
	double		NumberPrecessionsave, NumberPrecessionssum, PhaseShift0;
	
	double		width, height, depth, AnglMainHoriz=0.0, IntegralIntensity;
	
	static double		RotMatrixMain[3][3], RotMatrixField[3][3], LarmorMatrix[3][3];
				
	static double	PolX[FIELD_SIZE_FL], PolY[FIELD_SIZE_FL], PolZ[FIELD_SIZE_FL], ProbM[FIELD_SIZE_FL],
			FielX[FIELD_SIZE_FL], FielY[FIELD_SIZE_FL], FielZ[FIELD_SIZE_FL], FielM[FIELD_SIZE_FL]; /* For output spin and field during flight */
			
	static double	PosD[FIELD_SIZE], FieldD[FIELD_SIZE], PFinput[FIELD_SIZE_2] ; /* for input of rot. field distribution */
	
	VectorType	 Pos, Dir, SpinVector, Path, Pos1, Pos2, domain_field, PosDomain, PosMain, DimDomain, TranslOut ;
	Neutron		Neutrons ;
	
	/* Variables for rotating(oscilating-os) and permanent magnetic field add Manoshin Sergey*/


	double FieldValue, FieldValueInit, Omega, omegainit, OmegaInit, phi0, phi0d; /* Rotating(os) magnetic field FieldValue*sin(Omega*t + phi0); Initial and current values */
	double FieldValueDevPer=0.0; /* Deviation of amplitude of rotating(os) magnetic field in %! */
	double OmegaDevPer=0.0; /* Deviation of frequency of rotating(os) magnetic field in %! */
	long DevLawAmpl=3; /* Law of distribution of amplitude of magnetic field */
	long DevLawFreq=2; /* Law of distribution of frequency of rotating  or oscilating */	
	double Number_NOP=0.0;
	long keyphase=1; /* 1 - use the neutron TOF from preceding modules for rotating(os) field phase; 0 - No;  */
	double TOFP; /* Neutron TOF from preceding modules for synhro rotations */
	long keyaxis=0; /* Value 0, 1, 2 - rotating(os) around axis 0x, 0y, 0z respectivly */
	long keysph=0; /* Key for activate output polarisation components in the file, default no */
	long keyrotos=0; /* Choose rotating (0 -value) or oscilating cos or sin magnetic field */
	long keycalculate=0; /* key for calculation some parameters, default 0 - no calculation */
	long keybootstrap=0; /* key for activating bootstrap configuration, default no(0), 1 - yes */
	double keywave=20.0; /* wavelength for calculation, A */
	double FieldValue0Init[3], FieldValue0[3]; /* Additional permanent field, initial and current */
	double FieldValue0Dev=0.0; /* Additional random magnetic field, amplitude, Oe */
	double FieldValue0Length, OmegaFV0 ; /* Internal variable */
	double FieldValueSat=1e99; /* Saturation magnetic field, amplitude */
	long fieldsize, code, keyexit; /* Internal */
	double FieldValue00[3]; /* Internal */
	double X1=0.0, X2=0.0, Y1=0.0, Y2=0.0 ; /* For interpolation */


	/* For randomisation of amplitude and frequency of rotating (os) field */	
	double FieldValueA=0.0, FieldValueB=0.0; /* Internal variables */
	double OmegaA=0.0, OmegaB=0.0; /* Internal variables */
	double SigmaField=0.0, SigmaOmega=0.0; /* Internal Variables */
	
/* For output polarization components and magnetic field */	
	FILE *fmonitp=NULL;
	char *Monitp=NULL;

	FILE *fmonitf=NULL;
	char *Monitf=NULL;	
	
/*	File for describing amplitude distribution of rotating(os) magnetic field */	

	FILE *fampld=NULL;
	char *Ampld=NULL;

/* Functions prototypes */

	long		IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;
	double 		DistrGauss(double, double);
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;
	long		CalculateField(long l_keyrotos, long l_keyaxis, double l_FieldValue, double l_FieldValue0[3],
			double l_Omega, double l_TimeR, double l_TOFP, double l_phi0, VectorType l_RR);		
	

/* FINISH HEADER STORY */


	




