/********************************************************************************************/
/*  VITESS module 'polariser_he3.c'                                                         */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"

/* START HEADER STORY */

#define	STRING_BUFFER 50

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 

	FILE		*Par_Field, *XFILE, *polarization_file, *transmission_file;
	char		*PolarizationFileName, *TransmissionFileName, XFileName[STRING_BUFFER];
	long		Option, User, NumOut, Repetition, repet,  i , datanumber, datanumbermax;
	double		TOF, TOF1, TOF2, TOF3, WL, Prob, phi, the, PhaseShift, NumberPrecessions1, NumberPrecessions2, NumberPrecessions3 ;
	double		AngleMainHoriz, AngleMainVert, AnglOutHoriz, AnglOutVert ;
	double		RotMatrixMain[3][3], RotMatrixOut[3][3], RotMatrixField[3][3], RotMatrixG_Field[3][3], RotMatrixGM_Field[3][3], LarmorMatrix[3][3];
	double		polardata[5001], transdata[5001], ProbCutoff, IntegralIntensity, polHe3, polXsection, density ;
	VectorType	Pos, Dir, SpinVector, Path, Pos1, Pos2;
	VectorType	field_guide, scan_field, field_pol, guide_field_pol, PosMain, DimMain, TranslOut, FWHM ;
	Neutron		Neutrons ;

	void		polarizerHe3();
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
	void		ReadParameterFile() ;
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;


/* FINISH HEADER STORY */



int main(int argc, char **argv)
{

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv, VT_POL_HE3);/*strcpy(XFileName, "xfile.dat");  XFILE=fopen(XFileName, "w");*/ 

	OwnInit(argc, argv);


 /* Get the neutrons from the file */
DECLARE_ABORT;
  while((ReadNeutrons())!= 0)
  {
CHECK;	/* here is what happens to the neutron */

for(i=0;i<NumNeutGot ;i++)

{ 


	/*InputNeutrons[i].Position[0]	= 0. ;*/

	TOF = InputNeutrons[i].Time ;

	WL = InputNeutrons[i].Wavelength ;

	Prob = InputNeutrons[i].Probability ;

	CopyVector(InputNeutrons[i].Position, Pos) ;

	CopyVector(InputNeutrons[i].Vector, Dir) ;

	CopyVector(InputNeutrons[i].Spin, SpinVector) ; 


	InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

	/* compute polarization and transmission location corresponding to the wavelength */
	
	datanumber = (int) (WL * 100.) ; 
	
	if(datanumber > datanumbermax) goto getlost ;   



/* translates into frame of the field domain */
	
	SubVector(Pos, PosMain) ; 


/* calculate entrance end exit coordinates of domain*/

	/* rotates into the frame of the field domain   */

	{ 
		
	VectorType pos, dir;	CopyVector(Pos, pos) ;	CopyVector(Dir, dir) ;
	
	RotVector(RotMatrixMain, pos) ; 	RotVector(RotMatrixMain, dir) ; 

		/* gives intersection positions with domain */	

		if(IntersectionWithCylinder(DimMain, pos, dir, Pos1, Pos2) == 0) goto getlost ; 

	}

	/* rotates coordinates to previous frame */

	RotBackVector(RotMatrixMain, Pos1 ) ;

	RotBackVector(RotMatrixMain, Pos2) ;


	/* ordering */

	if(Pos1[0] > Pos2[0]) 	{	VectorType V ;	CopyVector(Pos1, V) ;	CopyVector(Pos2, Pos1) ;	CopyVector(V, Pos2) ;}


	/* time of precession in the guide field - precession calculated in the field frame */

	TOF1 = fabs(Pos1[0] - Pos[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

	PhaseShift = TOF1 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)) ; NumberPrecessions1 = PhaseShift/2./M_PI ;

	FillRotMatrixZY(LarmorMatrix, PhaseShift, 0) ; 

	RotVector(RotMatrixG_Field, SpinVector) ; 

	RotVector(LarmorMatrix, SpinVector) ;

	RotBackVector(RotMatrixG_Field, SpinVector) ; 



	/* moment of arriving at the domain wall, new position */

	TOF += TOF1 ;

	CopyVector(Pos1, Pos) ;

	/* time of precession in the domain field - precession calculated in the field frame */

	RotVector(RotMatrixGM_Field, SpinVector) ; 

	TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

	PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(LengthVector(guide_field_pol)) ;  NumberPrecessions2 = PhaseShift/2./M_PI ;

	FillRotMatrixZY(LarmorMatrix, PhaseShift, 0) ;

	RotVector(LarmorMatrix, SpinVector) ;

	RotBackVector(RotMatrixGM_Field, SpinVector) ;

	
	/* moment of exiting at the domain wall, new position */

	TOF += TOF2 ;

	CopyVector(Pos2, Pos) ;


/* translates into original frame */
	
	AddVector(Pos, PosMain) ; 

	/* computes neutron variables in the output frame */

	SubVector(Pos, TranslOut) ;

	RotVector(RotMatrixOut, Pos) ;

	RotVector(RotMatrixOut, Dir) ;

	RotVector(RotMatrixOut, SpinVector) ;


	/* translates neutron variables for output - X'=0. */

	{

	VectorType Path ;

	TOF3 =  - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;

	AddVector(Pos, Path) ;  
	
	}			/* Path = displacement vector */


	/* time of precession in the guide field again - precession calculated in the field frame */

	RotVector(RotMatrixG_Field, SpinVector) ;

	PhaseShift = TOF3 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)) ;  NumberPrecessions3 = PhaseShift/2./M_PI ;

	FillRotMatrixZY(LarmorMatrix, PhaseShift, 0) ;

	RotVector(LarmorMatrix, SpinVector) ;

	RotBackVector(RotMatrixG_Field, SpinVector) ;

	
	/* flipping process at some time */ 

	{ double pDown; 
		
		CartesianToSpherical(SpinVector, &the, &phi);

		pDown = sqrt(1. - polardata[datanumber]);

		the = 2. * (double) asin(pDown *(double) sin(the/2.)) ;

		SphericalToCartesian(SpinVector, &the, &phi);
	}

	/* Output matters */

	Prob *= transdata[datanumber] ; 

	if(Prob <= ProbCutoff) goto getlost ;


	IntegralIntensity += Prob ;

	NumOut++ ;							/*goto jumpwrite ;	jumpwrite :;*/


	/* transmit coordinates which were not changed, the rest overwrite below */
	Neutrons = InputNeutrons[i]; 

	Neutrons.Time = TOF+TOF3 ;

	Neutrons.Probability = Prob ;

	CopyVector(Pos, Neutrons.Position) ;

	CopyVector(SpinVector, Neutrons.Spin) ;


	/* writes output binary file */ 

	WriteNeutron(&Neutrons) ;


	getlost: ;

}
  }
   
 /* Do the general cleanup */
my_exit:
	OwnCleanup(); /*fclose(XFILE);*/

	Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], 0.0,0.0);	

	fprintf(LogFilePtr," \n") ;


  return 0;
}

/* own initialization of the polarizer He3 module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n") ;
	print_module_name("polarizer_He3 1.2") ;

/*    INPUT  */
	
	field_guide[0] = field_guide[1] = field_guide[2] = 0. ;

	field_pol[0] = field_pol[1] = field_pol[2] = 0. ;

ProbCutoff= wei_min;/**/

	while(argc>1)
	{
		switch(argv[1][1])
		{
				
			case 'a':
			sscanf(&argv[1][2], "%ld", &Option) ;
			break;

			case 'b':
			sscanf(&argv[1][2], "%lf", &polHe3) ;
			break;

			case 'c':
			sscanf(&argv[1][2], "%lf", &polXsection) ;
			break;

			case 'd':
			sscanf(&argv[1][2], "%lf", &density) ;  
			break;

			case 'P':
			PolarizationFileName=&argv[1][2];
			break;
					
			case 'T':
			TransmissionFileName=&argv[1][2];
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

			case 'X':
			sscanf(&argv[1][2], "%lf", &DimMain[2]) ;
			break;

			case 'Y':
			sscanf(&argv[1][2], "%lf", &DimMain[0]) ;
			break;

/*			case 'e':
			sscanf(&argv[1][2], "%lf", &ProbCutoff) ;
			break;
*/
			case 'G':
			sscanf(&argv[1][2], "%lf", &field_guide[0]) ;
			break;

			case 'H':
			sscanf(&argv[1][2], "%lf", &field_guide[1]) ;
			break;

			case 'K':
			sscanf(&argv[1][2], "%lf", &field_guide[2]) ;
			break;

			case 'M':
			sscanf(&argv[1][2], "%lf", &field_pol[0]) ;
			break;

			case 'N':
			sscanf(&argv[1][2], "%lf", &field_pol[1]) ;
			break;

			case 'O':
			sscanf(&argv[1][2], "%lf", &field_pol[2]) ;
			break;

			case 'p':
			sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
			if(TranslOut[0] < (PosMain[0] + DimMain[0]/2.)) {fprintf(LogFilePtr,"\nERROR: output position must be outside of flipper ! \n\n") ; exit (-1);}
			break;

			case 'r':
			sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
			break;

			case 's':
			sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
			break;


		}
		argc--;
		argv++;
	}
	
		AngleMainHoriz	= 0. ;

		AngleMainVert	= - M_PI/2. *(1+0.1e-10) ;

		DimMain[1] = 0.;


	FillRotMatrixZY(RotMatrixMain, AngleMainVert, AngleMainHoriz) ;

	FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz) ;

	/* calculate field matrixes */

		{	double roty_g, rotz_g ; double roty_gm, rotz_gm ; 

			
		if(LengthVector(field_guide) !=0) {
		
			CartesianToEulerZY(field_guide, &roty_g, &rotz_g) ; }

		else  { roty_g = 0. ; rotz_g = 0. ; }

		FillRotMatrixZY(RotMatrixG_Field, roty_g, rotz_g) ; 
		

		CopyVector(field_guide, guide_field_pol); 

		AddVector(guide_field_pol, field_pol) ;


		if(LengthVector(guide_field_pol) !=0) {
		
			CartesianToEulerZY(guide_field_pol, &roty_gm, &rotz_gm) ; }

		else  { roty_gm = 0 ; rotz_gm = 0 ; }

		FillRotMatrixZY(RotMatrixGM_Field, roty_gm, rotz_gm) ; 
		
		} 


		/* generate polarization and transmission files */

		datanumbermax = 5000 ;

		if(Option == 1)
		{long l;
		 double Polarization, Transmission, wavel, Mue, Nue ;

			if( (polarization_file = fopen(PolarizationFileName,"w"))==NULL)
			  {
			fprintf(LogFilePtr,"ERROR: File %s could not be opened!\n",PolarizationFileName);
			exit(-1);}

			if( (transmission_file = fopen(TransmissionFileName,"w"))==NULL)
			  {
			fprintf(LogFilePtr,"ERROR: File %s could not be opened!\n",TransmissionFileName);
			exit(-1);}


			for(l=0;l<datanumbermax;l++)
			{
				wavel = 0.01 * (l+1) ; /*energy = ENERGY_FROM_LAMBDA(wavel)/1.E6 ;*/

				Mue = (polXsection * wavel /*5327 * sqrt(0.025/energy)  polarization cross section*/)* 1.E-24  * 
					(density /*2.7E20  atomic density cm-3 at 10 atm pressure*/) ;

				Nue = (polHe3/100./*0.50 Polarization of He3*/) * Mue ;
					
				Polarization = (double) tanh(Nue * DimMain[0]) ;

				Transmission = (double) exp(- Mue * DimMain[0]) *
					
					(double) cosh(Nue * DimMain[0]) ;

				fprintf(polarization_file, "  %le \n", Polarization);
				fprintf(transmission_file, "  %le \n", Transmission);
			}
			
			fclose(polarization_file);

			fclose(transmission_file);

		

		}
		
		fprintf(LogFilePtr,"OK!\n");

		/* reads polarization and transmission files */

		if( (polarization_file = fopen(PolarizationFileName,"r"))==NULL)
		  {
		fprintf(LogFilePtr,"ERROR: File %s could not be opened!\n",PolarizationFileName);
		exit(-1);}

		if( (transmission_file = fopen(TransmissionFileName,"r"))==NULL)
		  {
		fprintf(LogFilePtr,"ERROR: File %s could not be opened!\n",TransmissionFileName);
		exit(-1);}


    {long count;
		  for(count=0; count<datanumbermax; count++)
		{
		  if (fscanf(polarization_file,"%le",&polardata[count])==EOF)
			break;
		}
		  for(count=0; count<datanumbermax; count++)
		{
		  if (fscanf(transmission_file,"%le",&transdata[count])==EOF)
			break;
		}

/*		  for(count=0; count<datanumbermax; count++)

		  {
			  fprintf(XFILE, " %le	%le	%le \n", 0.01*(count+1), polardata[count], transdata[count]);
		  }
*/
		fclose(polarization_file);

		fclose(transmission_file);
	}
 


/*	 init for ASCII output */

	NumOut=0 ;


	IntegralIntensity = 0. ;


}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  1   : %lf", NumberPrecessions1) ;

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  2   : %lf", NumberPrecessions2) ;

	if(NumOut != 0) fprintf(LogFilePtr,"\nNumber of precessions  3   : %lf\n", NumberPrecessions3) ;

}/* End OwnCleanup */



