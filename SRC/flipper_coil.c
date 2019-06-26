/********************************************************************************************/
/*  VITESS module 'flipper_coil.c'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  JUL 2004  G. Zsigmond  corrections for tilted flipper option                        */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"
#include "softabort.h"

/* START HEADER STORY */

#define	STRING_BUFFER 50
#define	FIELD_SIZE	100

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 

	FILE		*Par_Field, *XFILE;
	char		Option[STRING_BUFFER], *ParameterFileName, XFileName[STRING_BUFFER];
	long		User, coildir, NumOut, Repetition, repet,  i, wall_1, wall_2, ind_x, ind_y, ind_z, ind_x_max, ind_y_max, ind_z_max ;
	double		field_guide[3], field_parameter, field_coil, TOF, TOF1, TOF2, TOF3, WL, Prob, phi, the, PhaseShift, NumberPrecessions1, NumberPrecessions2, NumberPrecessions3 ;
	double		width, height, depth, AnglMainHoriz, AnglMainVert, ProbCutoff, IntegralIntensity ;
	static double RotMatrixMain[3][3], domain_field_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], PosDomain_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], RotMatrixField[3][3], LarmorMatrix[3][3];
	VectorType	Pos, Dir, SpinVector, Path, Pos1, Pos2, domain_field, PosDomain, PosMain, DimDomain, TranslOut, FWHM ; ;
	Neutron		Neutrons ;

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

int main(int argc, char **argv)
{

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv, VT_FLIP_COIL); 

	OwnInit(argc, argv);

 /* Get the neutrons from the file */
DECLARE_ABORT;
  while((ReadNeutrons())!= 0)
  {
CHECK;	/* here is what happens to the neutron */


for(i=0;i<NumNeutGot ;i++)

{ 
CHECK;

	/*InputNeutrons[i].Position[0]	= 0. ;*/

	TOF = InputNeutrons[i].Time ;

	WL = InputNeutrons[i].Wavelength ;

	Prob = InputNeutrons[i].Probability ;

	CopyVector(InputNeutrons[i].Position, Pos) ;

	CopyVector(InputNeutrons[i].Vector, Dir) ;

	InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

	CopyVector(InputNeutrons[i].Spin, SpinVector) ; 


/* translates into frame of the main field and rotates coordinates  */
	
	SubVector(Pos, PosMain) ;

	RotVector(RotMatrixMain, Pos ) ; 

	RotVector(RotMatrixMain, Dir ) ; 

	/* enter position and TOF 	*/
	{

	VectorType Path  ; /* displacement vector*/

	TOF1 = (- depth/2. - Pos[0])/ fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

		/* precession calculated  */

		PhaseShift = TOF1 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)) ;  NumberPrecessions1 = PhaseShift/2./M_PI ;

		FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;

		RotVector(LarmorMatrix, SpinVector) ;

		RotVector(RotMatrixMain, SpinVector) ; 


	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, (- depth/2. - Pos[0])/ Dir[0] ) ;

	AddVector(Pos, Path) ;  TOF += TOF1 ;
	
	}			


/* looks for first domain */

	ind_y = (long) floor(Pos[1] / DimDomain[1]) + 1 + ind_y_max/2 ;

	if((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost ;

	ind_z = (long) floor(Pos[2] / DimDomain[2]) + 1 + ind_z_max/2 ;

	if((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost ;

	ind_x = 1 ; 

/*	
******************* starts to scan ******************************/

	NumberPrecessions2 = 0 ;

while (ind_x != (ind_x_max +1)) 
{
	CopyVectorsToVector3(ind_x, ind_y, ind_z, PosDomain_F, PosDomain) ;

	CopyVectorsToVector3(ind_x, ind_y, ind_z, domain_field_F, domain_field) ;

	/* calculate field matrix */

	FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 
		
	/**/



/* translates into frame of the field domain */
	
	SubVector(Pos, PosDomain) ;


/* calculate entrance end exit coordinates of domain*/


	{ 
		
		VectorType pos, dir;	CopyVector(Pos, pos) ;	CopyVector(Dir, dir) ;
	
		/* gives intersection positions with domain */

		if(IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) goto getlost ; 

	    if(wall_2 == 0) goto getlost ;

		/* ordering */

		if(Pos1[0] > Pos2[0]) 	
		
		{VectorType V ;	int wall; CopyVector(Pos1, V) ;	CopyVector(Pos2, Pos1) ; CopyVector(V, Pos2) ; 	
		
		wall = wall_1 ; wall_1 = wall_2 ; wall_2 = wall ;}

	}



	/* moment of arriving at the domain wall, new position */


	CopyVector(Pos1, Pos) ;

	/* time of precession in the domain field - precession calculated in the field frame */

	TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

	RotVector(RotMatrixField, SpinVector) ; 

	PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]) ;  NumberPrecessions2 += PhaseShift/2./M_PI ;

	FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;

	RotVector(LarmorMatrix, SpinVector) ;

	RotBackVector(RotMatrixField, SpinVector) ;


	/* moment of exiting at the domain wall, new position */

	TOF += TOF2 ;

	CopyVector(Pos2, Pos) ;

/* translates back into main frame */
	
	AddVector(Pos, PosDomain) ;

	/* searching new domain */ if(wall_2 == 1) goto getlost;

	if(wall_2 == 2) {ind_x += 1 ; }

	if(wall_2 == 3) {ind_y += -1 ; }

	if(wall_2 == 4) {ind_y += 1 ; }

	if(wall_2 == 5) {ind_z += -1 ; }

	if(wall_2 == 6) {ind_z += 1 ;}


/*if(ind_x > ind_x_max) goto exitfield ; */
if(ind_y == 0) goto exitfield ; 
if(ind_y > ind_y_max) goto exitfield ; 
if(ind_z == 0) goto exitfield; 
if(ind_z > ind_z_max) goto exitfield;

/*goto newdomain ;*/}

	exitfield: ;



/*******************************************************************************/


	/* Output matters */

	
	AddVector(Pos, PosMain) ;

	RotBackVector(RotMatrixMain, Pos ) ; 

	RotBackVector(RotMatrixMain, Dir ) ; 

	RotBackVector(RotMatrixMain, SpinVector) ; 

	IntegralIntensity += Prob ;

	NumOut++ ;


	/* computes neutron variables in the output frame */ 

	SubVector(Pos, TranslOut) ;


	/* translates neutron variables for output - X'=0. */

	{

	VectorType Path ;

	TOF3 = - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;


		/* precession calculated  */

		PhaseShift = TOF3 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)) ;  NumberPrecessions3 = PhaseShift/2./M_PI ;

		FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;

		RotVector(LarmorMatrix, SpinVector) ;


	
	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;

	AddVector(Pos, Path) ;  TOF += TOF3 ;
	
	}			/* Path = displacement vector */


	/* transmit coordinates which were not changed, the rest overwrite below */
	Neutrons = InputNeutrons[i]; 


	Neutrons.Time = TOF ;

	CopyVector(Pos, Neutrons.Position) ;

	CopyVector(SpinVector, Neutrons.Spin) ;


	/* writes output binary file */

	WriteNeutron(&Neutrons) ;

	getlost: ;

}

  }
   
 /* Do the general cleanup */
my_exit:
	OwnCleanup(); 

	Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], 0.0,0.0);	

	fprintf(LogFilePtr," \n") ;


  return 0;
}


/* own initialization of the Field module */

void OwnInit(int argc, char *argv[])
{

	fprintf(LogFilePtr," \n") ;
	print_module_name("flipper_coil 1.3") ;

/*    INPUT  */
	

	domain_field[0] = domain_field[1] = domain_field[2] = 0. ;

	field_guide[0] = field_guide[1] = field_guide[2] = 0. ;

		/* Flipper */
		ind_x_max = 98; ind_y_max = ind_z_max = 2 ;

	while(argc>1)
	{
		switch(argv[1][1])
		{
				
				
			case 'i':
			sscanf(&argv[1][2], "%lf", &AnglMainHoriz) ;
			break;

			case 'j':
			sscanf(&argv[1][2], "%lf", &AnglMainVert) ;
			break;

			case 'k':
			sscanf(&argv[1][2], "%lf", &PosMain[0]) ;
			break;

			case 'l':
			sscanf(&argv[1][2], "%lf", &PosMain[1]) ;
			break;

			case 'm':
			sscanf(&argv[1][2], "%lf", &PosMain[2]) ;
			break;

			case 'p':
			sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
			break;

			case 'r':
			sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
			break;

			case 's':
			sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
			break;

			case 'y':
			sscanf(&argv[1][2], "%ld", &coildir) ;
			break;

			case 'N':
			sscanf(&argv[1][2], "%ld", &ind_x_max) ;
			if((ind_x_max/2. - floor(ind_x_max/2.)) > 0.) {	ind_x_max += 1 ; fprintf(LogFilePtr,"\nWARNING: Number of domains must be even! Set %ld. \n", ind_x_max) ;}
			if(ind_x_max > 100) {	ind_x_max = 100 ; fprintf(LogFilePtr,"\nWARNING: Number of domains must be < 102 ! Set %ld. \n", ind_x_max) ;}
			break;

			case 'X':
			sscanf(&argv[1][2], "%lf", &depth) ;
			break;

			case 'Y':
			sscanf(&argv[1][2], "%lf", &width) ;
			break;

			case 'V':
			sscanf(&argv[1][2], "%lf", &height) ;
			break;

			case 't':
			sscanf(&argv[1][2], "%lf", &field_parameter) ;
			if(field_parameter < 0.001) field_parameter = 0.001 ;
			break;

			case 'G':
			sscanf(&argv[1][2], "%lf", &field_guide[0]) ;
			break;

			case 'H':
			sscanf(&argv[1][2], "%lf", &field_coil) ;
			break;


		}
		argc--;
		argv++;
	}

		if(PosMain[0] < depth/2.) {fprintf(LogFilePtr,"\nERROR: X position must be larger than depth/2 ! \n\n") ; exit (-1);}
		if(TranslOut[0] < (PosMain[0] + depth/2.)) {fprintf(LogFilePtr,"\nERROR: output position must be outside of flipper ! \n\n") ; exit (-1);}

	
	FillRotMatrixZY(RotMatrixMain, AnglMainVert, AnglMainHoriz) ;


		DimDomain[0] = depth/ind_x_max ; 

		DimDomain[1] = width/ind_y_max ; 

		DimDomain[2] = height/ind_z_max ; 




		/* inhomogeneous field */
		{ /*double the_field, phi_field ;*/ VectorType field_cart ;

		for(ind_x=1;ind_x<(ind_x_max+1);ind_x++) { for(ind_y=1;ind_y<(ind_y_max+1);ind_y++) { for(ind_z=1;ind_z<(ind_z_max+1);ind_z++) {

			
			PosDomain_F[0][ind_x][ind_y][ind_z] = ((ind_x -1)-(ind_x_max /2 - 0.5)) * DimDomain[0] ;
		
			PosDomain_F[1][ind_x][ind_y][ind_z] = ((ind_y -1)-(ind_y_max /2 - 0.5)) * DimDomain[1] ;
		
			PosDomain_F[2][ind_x][ind_y][ind_z] = ((ind_z -1)-(ind_z_max /2 - 0.5)) * DimDomain[2] ;


			/* Flipper  */

			
			if(PosDomain_F[0][ind_x][ind_y][ind_z] < (double) (- (ind_x_max/2 * DimDomain[0] - field_parameter)))
			{
				field_cart[0] = 0. ; 
				
				field_cart[1] = 0. ; 
				
				field_cart[2] = field_coil/field_parameter * (PosDomain_F[0][ind_x][ind_y][ind_z]  + ind_x_max/2 * DimDomain[0]) ;
			}

			else if(PosDomain_F[0][ind_x][ind_y][ind_z] > (double) (ind_x_max/2 * DimDomain[0] - field_parameter))
			{
				field_cart[0] = 0. ; 
				
				field_cart[1] = 0. ; 
				
				field_cart[2] =  field_coil * (1. - (PosDomain_F[0][ind_x][ind_y][ind_z]  - (ind_x_max/2 * DimDomain[0] - field_parameter))/field_parameter ) ;
			}

			else 
			{
				field_cart[0] = 0. ; field_cart[1] = 0 ; field_cart[2] = field_coil  ;
			}					

			if(coildir != 1) 
			{
			field_cart[1] = field_cart[2] ; field_cart[2] = 0.;
			}

			RotVector(RotMatrixMain, field_guide ) ; 

			AddVector(field_cart, field_guide) ;

			CartesianToEulerZY(field_cart, &domain_field_F[2][ind_x][ind_y][ind_z], &domain_field_F[1][ind_x][ind_y][ind_z]) ;	
			
			domain_field_F[0][ind_x][ind_y][ind_z] = LengthVector(field_cart) ;
		
		}}}

		}


/*	 init for ASCII output */

	NumOut=0 ;
	IntegralIntensity = 0. ;


}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  in guide field   : %lf", NumberPrecessions1) ;

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  in flipper coil  : %lf", NumberPrecessions2) ;

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  in guide field   : %lf\n", NumberPrecessions3) ;


}/* End OwnCleanup */





void   CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3][3])
{
long m, l ;
	
		for(m = 0;m<3;m++)
		{
			for(l = 0;l<3;l++)
			{
				Result[m][l] = Matrix[m][l][i][j][k] ;
			}
		}
}


void	CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE])
{
long m, l ;
	
		for(m = 0;m<3;m++)
		{
			for(l = 0;l<3;l++)
			{
				Matrix[m][l][i][j][k] = Result[m][l] ;
			}
		}
}

void	CopyVectorsToVector3(long i, long j, long k, double Vector[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3])
{
long l ;

		for(l = 0;l<3;l++)
		{
			Result[l] = Vector[l][i][j][k] ;
		}
}

void	CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) 
{
long l ;

		for(l = 0;l<3;l++)
		{
			Result[l][i][j][k] = Vector[l] ;
		}
}


/* Intersection with rectangular object */

long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
VectorType	n, pos0, pos1, pos2, pos3, pos4, pos5 ;
int			k ;

for(k=0;k<3;k++) Pos1[k] = Pos2[k] = 0. ;

n[0] = 1. ; n[1] = n[2] = 0. ; *wall_1 = *wall_2 = 0 ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
	{
		if(  (fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2) ) 
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos0, Pos1) ; *wall_1 = 1;}
			else {CopyVector(pos0, Pos2) ; *wall_2 = 1;}
	} 
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
	{
		if(  (fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos1, Pos1) ; *wall_1 = 2;}
			else {CopyVector(pos1, Pos2) ; *wall_2 = 2;}
		}
	}

n[1] = 1. ; n[2] = n[0] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
	{
		if( (fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos2, Pos1) ; *wall_1 = 3;}
			else {CopyVector(pos2, Pos2) ; *wall_2 = 3;}
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
	{
		if( (fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos3, Pos1) ; *wall_1 = 4;}
			else {CopyVector(pos3, Pos2) ; *wall_2 = 4;}
		}
	}

n[2] = 1. ; n[0] = n[1] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
	{
		if( (fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)  ) 
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos4, Pos1) ; *wall_1 = 5;}
			else {CopyVector(pos4, Pos2) ; *wall_2 = 5;}
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
	{
		if( (fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)  ) 
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos5, Pos1) ; *wall_1 = 6;}
			else {CopyVector(pos5, Pos2) ; *wall_2 = 6;}
		}
	}

	if((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) return 0 ;

	
	return 1 ;

}/* End IntersectionWithRectangularWallNumber() */
