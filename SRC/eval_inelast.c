/********************************************************************************************/
/*  VITESS module 'eval_inelast.c'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  NOV 2005  K. Lieutenant  transformation scattering angles -> direction removed      */
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

#define	STRING_BUFFER 50
#define	BINS_BUFFER 5000

	FILE		*FilePtrTOF, *FilePtrEnergy ;
	char		*FileNameTOF, *FileNameEnergy, Option[STRING_BUFFER] ;
	long		GeomOption, NumOut, BoseF,  i, k , nperbint[BINS_BUFFER], nperbine[BINS_BUFFER], NoBins ;
	double		IntegralIntensity, PrimaryFlightPath, SecondaryFlightPath, ReferenceWavelength, MinTOF, MaxTOF, TimeOffset, SlopeBins, Temperature, Angle, AngleRange ;
	double		t[BINS_BUFFER], e[BINS_BUFFER], prob_t[BINS_BUFFER], prob_e[BINS_BUFFER], MinE, MaxE ;
	double		alpha, beta, E, energy, velocity, TOF, TOF_total_e ;

	double		TransformFactor(double E);
	double		BoseFactor(double T, double w);
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;

/* FINISH HEADER STORY */



int main(int argc, char **argv)
{
 /* Initialize the program according to the parameters given  */

	Init(argc, argv, VT_EVAL_INELAST);

	OwnInit(argc, argv);


	/* calculates TOF channel boundaries and init p_TOF*/

	t[0] = MinTOF ; e[0] = MinE ;

	prob_t[0] = prob_e[0] = 0. ; nperbint[0] = 0; nperbine[0] = 0;

	for(k=1;k<NoBins;k++)
	{
		t[k] = alpha *t[k-1] + beta ;

		e[k] = e[k-1] + (MaxE - MinE) / NoBins ;

		prob_t[k] = prob_e[k] = 0. ; nperbint[k] = 0; nperbine[k] = 0;
	}


DECLARE_ABORT;

while((ReadNeutrons())!= 0)
{
CHECK;
	for(i=0;i<NumNeutGot ;i++)

	{
CHECK;
	/* select angle and angle range */

	{
		double rotz, roty ; VectorType Vector ;

		// SphericalToCartesian(Vector, &InputNeutrons[i].Vector[0], &InputNeutrons[i].Vector[1]) ;

		CartesianToEulerZY(Vector, &roty, &rotz);

		if(	(rotz < Angle - AngleRange) || (rotz > Angle + AngleRange)	) goto getlost ;
	}


	/* shift pulse */

	InputNeutrons[i].Time -= TimeOffset ;


	/* energy transfer corresponding to total flight time */

	if(GeomOption == 0) E = energy - 0.001*ENERGY_FROM_V(SecondaryFlightPath / (InputNeutrons[i].Time - TOF)) ;

	if(GeomOption == 1) E = 0.001*ENERGY_FROM_V(PrimaryFlightPath / (InputNeutrons[i].Time - TOF)) - energy  ;


	/* binning process  */

	for(k=0;k<NoBins;k++)
	{

		if( (InputNeutrons[i].Time > t[k]) && (InputNeutrons[i].Time <= t[k+1]) )

		{prob_t[k] += InputNeutrons[i].Probability ; nperbint[k] += 1 ;}

		if( (E > e[k]) && (E <= e[k+1]) )

		{prob_e[k] += InputNeutrons[i].Probability * TransformFactor(E) / BoseFactor(Temperature, E) ; nperbine[k] += 1 ;}

	}


	IntegralIntensity += InputNeutrons[i].Probability ;

	NumOut++ ;

	  /* writes output binary file */

      WriteNeutron(&(InputNeutrons[i]));

/*	 here continues if neutron gets lost */

	getlost: ;

	}

}

	for(k=0;k<NoBins;k++)
	{
		if(nperbint[k]==0) nperbint[k]=1; if(nperbine[k]==0) nperbine[k]=1; 
		
		if(FilePtrTOF != NULL) fprintf(FilePtrTOF, "%lf   %le   %le\n", t[k]+TimeOffset, prob_t[k], prob_t[k]/sqrt(nperbint[k])) ;

		if(FilePtrEnergy != NULL) fprintf(FilePtrEnergy, "%lf   %le   %le\n", e[k], prob_e[k], prob_e[k]/sqrt(nperbine[k])) ;

	}

 /* Do the general cleanup */

my_exit:
	
	OwnCleanup();
	Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return 0;

}/* End main() */


/* Transform factor */

double TransformFactor(double E)
{
	if(GeomOption == 0) return SecondaryFlightPath * sqrt(energy) / sq(energy - E) ;

	if(GeomOption == 1) return PrimaryFlightPath / sqrt(energy) / (energy + E) ;

	else return 0 ;
}



/* Bose factor if w in ueV */

double	BoseFactor(double T, double w)
{
double betha;

	if(T == 0.) return 1. ;

	betha = 11.605 / T ; /* unit 1/meV ! */

	if(w > 0.) return 1 / ((double) exp(betha * w) - 1) +1 ;
	if(w < 0.) return 1 / ((double) exp(- betha * w) - 1) ;

	else return	0. ;
}


/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n") ;

	print_module_name("eval_inelast 1.3") ;


/*    INPUT  */

	while(argc>1)
	{
		switch(argv[1][1])
		{

			case 'A':
			sscanf(&argv[1][2], "%ld", &GeomOption) ;
			break;

			case 'C':
			sscanf(&argv[1][2], "%ld", &NoBins) ;
			break;

			case 'D':
			sscanf(&argv[1][2], "%ld", &BoseF) ;
			break;

			case 'E':
			if((FilePtrTOF = fopen(&argv[1][2],"w"))==NULL)
			{
				printf("\nTOF spectrum file not opened or created\n");
				exit(0);
			}
			if(FilePtrTOF != NULL) FileNameTOF = &argv[1][2];
			break;

			case 'G':
			if((FilePtrEnergy = fopen(&argv[1][2],"w"))==NULL)
			{
				printf("\nenergy spectrum file not opened or created\n");
				exit(0);
			}
			if(FilePtrEnergy != NULL) FileNameEnergy = &argv[1][2];
			break;

			case 'a':
			sscanf(&argv[1][2], "%lf", &PrimaryFlightPath) ;
			break;

			case 'b':
			sscanf(&argv[1][2], "%lf", &SecondaryFlightPath) ;
			break;

			case 'c':
			sscanf(&argv[1][2], "%lf", &ReferenceWavelength) ;
			break;

			case 'd':
			sscanf(&argv[1][2], "%lf", &TimeOffset) ;
			break;

			case 'e':
			sscanf(&argv[1][2], "%lf", &MinTOF) ;
			break;

			case 'g':
			sscanf(&argv[1][2], "%lf", &MaxTOF) ;
			break;

			case 'h':
			sscanf(&argv[1][2], "%lf", &SlopeBins) ;
			break;

			case 'i':
			sscanf(&argv[1][2], "%lf", &Temperature) ;
			break;

			case 'j':
			sscanf(&argv[1][2], "%lf", &Angle) ;
			break;

			case 'k':
			sscanf(&argv[1][2], "%lf", &AngleRange) ;
			break;


		}
		argc--;
		argv++;
	}


	/* check */

	if((GeomOption != 0) &&	(GeomOption != 1))
	{
		fprintf(LogFilePtr,"ERROR: wrong geometry option!\n\n") ;
		exit(0) ;
	}

	if((BoseF != 0) &&	(BoseF != 1))
	{
		fprintf(LogFilePtr,"ERROR: wrong option for Bose-factor!\n\n") ;
		exit(0) ;
	}

	if(MinTOF >= MaxTOF)
	{
		fprintf(LogFilePtr,"ERROR: minimal time must be < maximal time!\n\n") ;
		exit(0) ;
	}


	if(MinTOF <= 0)
	{
		MinTOF = 1.e-6 ;
		fprintf(LogFilePtr,"warning: negative TOF - minimal time was set to 0.000001 ms!") ;
	}

	if(fabs(SlopeBins) > 0.01)
	{
		fprintf(LogFilePtr,"ERROR: please set |SlopeBins| < 0.01!\n\n") ;
		exit(0) ;
	}


	/* options */


	if(FilePtrTOF != NULL) fprintf(LogFilePtr,"	TOF spectrum file: '%s'", FileNameTOF) ;


	if(FilePtrEnergy != NULL) fprintf(LogFilePtr,"\n	energy spectrum file: '%s'", FileNameEnergy) ;

	fprintf(LogFilePtr,"\n	number of bins		=  %ld\n	primary flight path		=  %9.4f\n	secondary flight path	=  %9.4f\n	reference wavelength	=  %9.4f\n	time offset		=  %9.4f\n	minimal time		=  %9.4f\n	maximal time		=  %9.4f\n	gradient of timebins	=  %9.4f\n	temperature		=  %9.4f\n	angle			=  %9.4f\n	angle range		=  %9.4f",
		NoBins, PrimaryFlightPath, SecondaryFlightPath, ReferenceWavelength, TimeOffset, MinTOF, MaxTOF, SlopeBins, Temperature, Angle, AngleRange) ;

	if(GeomOption == 0) 	fprintf(LogFilePtr,"\noption 'direct geometry'") ;

	if(GeomOption == 1) 	fprintf(LogFilePtr,"\noption 'inverted geometry'") ;

	if(Temperature == 0.) BoseF = 0 ;

	if(BoseF == 0) Temperature = 0. ;

	if(BoseF == 0) 	fprintf(LogFilePtr,"\nnot divided by Bose-factor") ;

	if(BoseF == 1) 	fprintf(LogFilePtr,"\ndivided by Bose-factor") ;



	/* computes global reference values */

	MinTOF -= TimeOffset ; 	MaxTOF -= TimeOffset ;

	energy = 0.001*ENERGY_FROM_LAMBDA(ReferenceWavelength) ;

	velocity = V_FROM_LAMBDA(ReferenceWavelength) ;

	TOF_total_e = (PrimaryFlightPath + SecondaryFlightPath) / velocity ;

	if(GeomOption == 0)
	{
		TOF = PrimaryFlightPath / velocity ;

		MinE = energy - 0.001*ENERGY_FROM_V(SecondaryFlightPath / (MinTOF - TOF)) ;

		MaxE = energy - 0.001*ENERGY_FROM_V(SecondaryFlightPath / (MaxTOF - TOF)) ;
	}

	if(GeomOption == 1)
	{
		TOF = SecondaryFlightPath / velocity ;

		MinE = 0.001*ENERGY_FROM_V(PrimaryFlightPath / (MaxTOF - TOF)) - energy  ;

		MaxE = 0.001*ENERGY_FROM_V(PrimaryFlightPath / (MinTOF - TOF)) - energy ;
	}


	/* calculates alpha, beta coeff. for TOF boundary calculation */

	alpha = 1 / (1 - SlopeBins) ;

	beta = 0. ;

	for(k=0;k<NoBins;k++) beta += pow(alpha, k) ;

	beta = (MaxTOF - pow(alpha, NoBins) * MinTOF) / beta ;


	fprintf(LogFilePtr,	"\ncomputed reference values:\n	energy			=  %9.4f meV\n	velocity			=  %9.4f cm/ms\n	MinTOF_effective		=  %9.4f ms\n	MaxTOF_effective		=  %9.4f ms\n	TOF_total_elast		=  %9.4f ms\n	TOF_fixed_energy_side	=  %9.4f ms\n	minimal energy transfer	=  %9.4f meV\n	maximal energy transfer	=  %9.4f meV\n\n",
							energy, velocity, MinTOF, MaxTOF, TOF_total_e, TOF, MinE, MaxE) ;



	Angle *= M_PI / 180. ;

	AngleRange *= M_PI / 180. ;


	NumOut=0 ;

	IntegralIntensity = 0. ;


}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{
	if(FilePtrTOF != NULL)fclose(FilePtrTOF) ;

	if(FilePtrEnergy != NULL)fclose(FilePtrEnergy) ;

}/* End OwnCleanup */




