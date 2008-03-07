/********************************************************************************************/
/*  VITESS module 'sample_inelast.c'                                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2  MAY 2004  G. Zsigmond  normalize with repetition                               */
/* 1.3  JUL 2004  G. Zsigmond  error warning for sample position                            */
/* 1.4  JUL 2004  G. Zsigmond  including hollow cylinder sample                             */
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

	FILE		*Par_Sample, *XFILE; 
	char		Option[STRING_BUFFER], *ParameterFileName, XFileName[STRING_BUFFER];
	long		User, NumOut, Repetition, BoseF, repet,  i ;
	double		TOF, WL, Prob ;
	double		MaxPathLength, MaxPathLengthHol=0., PathLength, PathLengthHol=0., scattered_dir[3], l_reference, h_reference, v_reference;
	double		P1, P2, P3, P4, Temperature, D1, D2, D3, AnglSampleHoriz, AnglSampleVert, AnglOutHoriz, AnglOutVert ;
	double		RotMatrixSample[3][3], RotMatrixScatter[3][3], RotMatrixOut[3][3], RotMatrixDelta[3][3];
	double		AbsorptionC, ScatteringC, ProbCutoff, IntegralIntensity ;
	VectorType	Pos1f, Pos2f, Pos3f, Pos4f, 
	            Pos1v, Pos2v, Pos3v, Pos4v, Pos, Dir,  
	            random_main, random_range, k_reference, PosSample, DimSample, DimSampleHol, TranslOut;
	Neutron		Neutrons ;

	long		S_q_w(double *wl, double *prob, VectorType Dir);
	double		FunctionS_q_w(VectorType q, double energy);
	double		Dispersion(VectorType q);
	double		BoseFactor(double T, double w);
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir);
	void		ReadParameterFile() ;
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;


/* FINISH HEADER STORY */


int main(int argc, char **argv)
{

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv, VT_SMPL_INELAST); 

	OwnInit(argc, argv);


 /* Get the neutrons from the file */

DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
  {
CHECK;	 /*here is what happens to the neutron */

	for(i=0;i<NumNeutGot ;i++)

{ 

CHECK;

	MaxPathLengthHol = PathLengthHol = 0.; 
      
	InputNeutrons[i].Vector[0]		= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;


	/* translates and rotates into frame of the sample  */

	SubVector(InputNeutrons[i].Position, PosSample) ;

	RotVector(RotMatrixSample, InputNeutrons[i].Position) ;

	RotVector(RotMatrixSample, InputNeutrons[i].Vector) ;


	/* gives intersection positions with sample */

	if(Option[1] == 'y')
	{
		if(IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
	}

	  if(Option[1] == 'o')
	  {
		if(IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos4f) == 0) goto getlost ; 
		else 
		{
			if(IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 0) CopyVector(Pos4f, Pos2f);
			if(IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 1)
			{	double r=MonteCarlo(-1.,1);

				if((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==1)) goto getlost;
				
				if((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==0))
				{
					if(r>0.)
					{ 
					SubVector(Pos2f, Pos1f);
						
					PathLengthHol = LengthVector(Pos2f); 

					CopyVector(Pos3f, Pos1f); CopyVector(Pos4f, Pos2f); 

					MaxPathLengthHol = PathLengthHol; 				
					}
					else 
					{
					SubVector(Pos4f, Pos3f);
						
					MaxPathLengthHol = LengthVector(Pos4f); 

					PathLengthHol = 0.;
					} 
				}
				if((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==1))
				{
					PathLengthHol = 0.; 

					MaxPathLengthHol = 0.;
				}
				if((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==0))
				{
					CopyVector(Pos3f, Pos1f); CopyVector(Pos4f, Pos2f); 

					PathLengthHol = 0.; 

					MaxPathLengthHol = 0.;
				}
			}
		}
	  }


	if(Option[1] == 'u')
	{
		if(IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
	}

	if(Option[1] == 'a')
	{
		if(IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
	}


	for(repet=0;repet<Repetition;repet++)
	{/**/

CHECK;
	CopyVector(Pos1f, Pos1v) ;
	CopyVector(Pos2f, Pos2v) ;
	CopyVector(Pos3f, Pos3v) ;
	CopyVector(Pos4f, Pos4v) ;
		
	TOF  = InputNeutrons[i].Time ;
	WL   = InputNeutrons[i].Wavelength ;
	Prob = InputNeutrons[i].Probability ;

	CopyVector(InputNeutrons[i].Position, Pos) ;
	CopyVector(InputNeutrons[i].Vector, Dir) ;


	/* scattering position and TOF untill scattering */	
	
	SubVector(Pos2v, Pos1v) ;					/*maximal path vector*/ 

    MaxPathLength = LengthVector(Pos2v) + MaxPathLengthHol ; 

	MultiplyByScalar(Pos2v, MonteCarlo(0.,1.)) ;	 /*random path vector in cylinder untill scattering */

    PathLength = LengthVector(Pos2v) + PathLengthHol ;

	AddVector(Pos1v, Pos2v) ;	
		
	{ VectorType propag;

	CopyVector(Pos1v, propag);

	SubVector(propag, Pos);
	
	TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;

	}

	CopyVector(Pos1v, Pos) ;						/*scattering position */


	/* attenuation untill scattering normalized to maximal path */

    Prob *= (double) exp( - PathLength * AbsorptionC * WL - PathLength * ScatteringC) ;
         
	Prob *= MaxPathLength * ScatteringC ; 


	/* S(q,w) scattering: new neutron variables*/ 

	Prob *= WL ;

	if(S_q_w(&WL, &Prob, Dir) == 0) goto getlost2 ;

	Prob *= 1/WL ;

CHECK;


	/* attenuation succeeding scattering */

	if(Option[1] == 'y')
	{
		if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
	}

	if(Option[1] == 'o')
	{
		
		if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos4v) == 0) goto getlost2 ; 
		else 
		{
			if(IntersectionWithCylinder(DimSampleHol, Pos, Dir, Pos2v, Pos3v) == 0) CopyVector(Pos4v, Pos2v);
			else
			{   VectorType Propag; 

				CopyVector(Pos2v, Propag); 

				SubVector(Propag, Pos);
				
				if(CompareVectors(Pos3v, Pos4v)==0)
				{
					if(ScalarProduct(Propag, Dir) > 0.)
					{ VectorType Propag1;

					CopyVector(Pos4v, Propag1);

					SubVector(Propag1, Pos3v);

					PathLengthHol = LengthVector(Propag1); 
					}
					else 
					{
						PathLengthHol = 0.; 
				
						CopyVector(Pos3v, Pos1v); CopyVector(Pos4v, Pos2v);
					}
				}
				else PathLengthHol = 0.; 
			}
		}
	}


	if(Option[1] == 'u')
	{
		if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
	}

	if(Option[1] == 'a')
	{
		if(IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
	}


	/* path in the sample after scattering */

	{
		VectorType Pos_final ;

		CopyVector(Pos2v, Pos_final) ;

		SubVector(Pos_final, Pos) ;

		PathLength = LengthVector(Pos_final) + PathLengthHol ;  
	}


	if(PathLengthHol != 0.) CopyVector(Pos4v, Pos2v);  /* for hollow cylinder option: set output position to where it crosses the outer cylinder if crossed  */
	
	/* Test: set output to scattering positions:  CopyVector(Pos, Pos2v); */
	
    Prob *= (double) exp( - PathLength * AbsorptionC * WL - PathLength * ScatteringC);


	/* Output matters */

	{ VectorType propag;

	CopyVector(Pos2v, propag);

	SubVector(propag, Pos);
	
	TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;
	}


	OutputTransformations(&TOF, &WL, &Prob, Pos2v, Dir) ;
	

	Prob *= random_range[1]/180. * sin(random_range[2]* M_PI/180.)/4.;  /* solid angle / 4pi */

	if(Prob <= ProbCutoff) goto getlost2 ;

	IntegralIntensity += Prob ;

	NumOut++ ;


	/* transmit coordinates which were not changed, the rest overwrite below */
	Neutrons = InputNeutrons[i]; 

	Neutrons.Time = TOF ;

	Neutrons.Wavelength = WL ;

	Neutrons.Probability = Prob/Repetition ;

	CopyVector(Pos2v, Neutrons.Position) ;

	CopyVector(Dir, Neutrons.Vector) ;


	/* writes output binary file */

	WriteNeutron(&Neutrons) ;


	getlost2: ;

	}/*repetition*/
	/* here continues if neutron gets lost */

	getlost: ;

}


  }
   
 /* Do the general cleanup */

my_exit:

  OwnCleanup(); 

	Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);	

	fprintf(LogFilePtr," \n") ;


  return 0;
}




/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/
/* S(q,w) scattering: new neutron variables */

long	S_q_w(double *wl, double *prob, VectorType Dir)
{
int		j ;

double	q[3], dir_fin[3], dir_inc[3], DeltaHoriz, DeltaVert, energy ;


	if(*wl == 0.) return 0 ; 

	/* new random direction */

	CopyVector(Dir, dir_inc) ;

	DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= random_range[1]/2. * M_PI/180. ;

	DeltaVert = MonteCarlo(-1. , 1.) ; DeltaVert *= random_range[2]/2. * M_PI/180. ;

	EulerToCartesianZY( dir_fin,  &DeltaVert,  &DeltaHoriz);

	RotBackVector(RotMatrixScatter, dir_fin) ; CopyVector(dir_fin, Dir) ; 

	RotVector(RotMatrixSample, Dir) ;

	/* new wavelength*/


	if(P2 <= 0.0002)

	{

		*wl = 1 / (double) sqrt (1 / sq(*wl) - P1 / L_2_E) ;

	}

	else

	{
	double fact; double wl_old = *wl;

	*wl = random_main[0] + MonteCarlo(- random_range[0]/2, random_range[0]/2) ;

	energy = ENERGY_FROM_LAMBDA(wl_old) - ENERGY_FROM_LAMBDA(*wl) ;

	MultiplyByScalar(dir_inc, 2 * M_PI / wl_old) ;

	MultiplyByScalar(dir_fin, 2 * M_PI / *wl) ;

	SubVector(dir_inc, dir_fin) ; for(j=0;j<3;j++) q[j]=dir_inc[j];

	fact = 1. / (*wl * sq(*wl)) /**/;
	
	*prob	*= fact * fabs(FunctionS_q_w(q, energy)) ;

	}
	return 1 ;
}

/*	FunctionS_q_w(q, energy) */

double	FunctionS_q_w(VectorType q, double energy)
{
double p ;

/* 2 DHO 

	p =		2 * ( P3 * P2 /(sq(energy - P1) + sq(P2)) 

				- P4 * P2 /(sq(energy + P1) + sq(P2))) ;

	p +=	2 * ( D3 * D2 /(sq(energy - D1) + sq(D2)) 

				- D3 * D2 /(sq(energy + D1) + sq(D2))) ; 

*/

/* normal vitess */

	p =		2 * P3 * ( sq(P2) /(sq(energy - P1 - Dispersion(q)) + sq(P2)) 

				- P4 * sq(P2) /(sq(energy + P1 + Dispersion(q)) + sq(P2))) ;


	if(BoseF == 1) p *= BoseFactor(Temperature, energy) ;

	return p ;
}

/* Energy dispersion */

double	Dispersion(VectorType q)
{
/* normal vitess */
	return	D1 * q[0] + D2 * q[1] + D3 * q[2] ;

/* helium dispersion */
	/*return	D1 + sq(LengthVector(q) - D2) / 2. /(0.9575E-03 * D3) ;  mHe4=0.9575 (2PIh)^2 [Angstrom^(-2)/meV] mass of free atom  */
}

/* Bose factor if w in ueV */

double	BoseFactor(double T, double w)
{
double betha;

	if(T == 0.) 
	{
		if(w > 0.) return 1. ;
		if(w < 0.) return 0. ;
	}

	betha = 11.605 / T / 1.e3 ; /* unit 1/ueV ! */

	if(w > 0.) return 1 / ((double) exp(betha * w) - 1) +1 ;
	if(w < 0.) return 1 / ((double) exp(- betha * w) - 1) ;
	
	else return	0. ;
}
/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/
	

/* Output matters */

void OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir)
{
	/* computes neutron variables in the initial frame */

	RotBackVector(RotMatrixSample, Pos) ;

	RotBackVector(RotMatrixSample, Dir) ;

	AddVector(Pos, PosSample) ;


	/* computes neutron variables in the output frame */

	SubVector(Pos, TranslOut) ;

	RotVector(RotMatrixOut, Pos) ;

	RotVector(RotMatrixOut, Dir) ;


	/* translates neutron variables for output - X'=0. 

	{

	VectorType Path ;

	*tof = *tof - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA_PT(wl) ;

	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;

	AddVector(Pos, Path) ;  
	
	}		     Path = displacement vector
	*/

} /* End OutputTransformations()*/


/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n") ;
	print_module_name("sample_inelast 1.4") ;
ProbCutoff=wei_min ;
/*    INPUT  */
	
	while(argc>1)
	{
		switch(argv[1][1])
		{
				
				
			case 'P':
			if((Par_Sample = fopen(&argv[1][2],"r"))==NULL)
			{
				fprintf(LogFilePtr,"\nParameter file '%s' not found\n",&argv[1][2]);
				exit(0);
			}
			ParameterFileName=&argv[1][2];
			break;
		
			case 'A':
			sscanf(&argv[1][2], "%ld", &Repetition) ;
			break;

			case 'D':
			sscanf(&argv[1][2], "%ld", &BoseF) ;
			break;

			case 'a':
			sscanf(&argv[1][2], "%lf", &P1) ;
			break;

			case 'b':
			sscanf(&argv[1][2], "%lf", &P2) ;
			break;

			case 'c':
			sscanf(&argv[1][2], "%lf", &P3) ;
			break;

			case 'd':
			sscanf(&argv[1][2], "%lf", &P4) ;
			break;

			case 'T':
			sscanf(&argv[1][2], "%lf", &Temperature) ;
			break;

			case 'x':
			sscanf(&argv[1][2], "%lf", &D1) ;
			break;

			case 'y':
			sscanf(&argv[1][2], "%lf", &D2) ;
			break;

			case 'z':
			sscanf(&argv[1][2], "%lf", &D3) ;
			break;


		}
		argc--;
		argv++;
	}
	

	if((BoseF != 0) &&	(BoseF != 1)) 
	{
		fprintf(LogFilePtr,"Wrong Option for Bose Factor!") ;  

		exit(0) ;
	}

	if(Repetition == 0)
	{
		fprintf(LogFilePtr,"Repetition rate must be > 0 !") ;  

		exit(0) ;
	}

	/* reads parameter file */

	ReadParameterFile() ; 

	if(Par_Sample != NULL)fclose(Par_Sample) ;

	
	/* computes global reference values */

		scattered_dir[0]= (double) cos(random_main[2]*M_PI/180.) * (double) cos(random_main[1]*M_PI/180.) ;
		scattered_dir[1]= (double) cos(random_main[2]*M_PI/180.) * (double) sin(random_main[1]*M_PI/180.) ;
		scattered_dir[2]= (double) sin(random_main[2]*M_PI/180.) ;

	{
		double	wl, wl_scattered, q_length, scattering_angle, energy_transfer ;

		VectorType	k_scattered ;


		wl = 2 * M_PI / LengthVector(k_reference) ;

		wl_scattered = random_main[0] ;

		scattering_angle = AngleVectors(k_reference, scattered_dir) ;

		CopyVector(scattered_dir, k_scattered) ;

		MultiplyByScalar(k_scattered, 2 * M_PI / wl_scattered) ;

		SubVector(k_scattered, k_reference) ;

		q_length = LengthVector(k_scattered) ;

		energy_transfer = ENERGY_FROM_LAMBDA(wl) - ENERGY_FROM_LAMBDA(wl_scattered) ;


		fprintf(LogFilePtr,	"\nscattering triangle corresponding to q-transfer and reference-k:\n	reference wavelength	= %9.4f A\n	scattered wavelength	= %9.4f A\n	scattering angle		= %9.4f deg\n	|q-transfer|		= %9.4f A-1\n	energy transfer		= %9.4f ueV",
							wl, wl_scattered, scattering_angle, q_length, energy_transfer) ;

	}


	FillRotMatrixZY(RotMatrixScatter, random_main[2]*M_PI/180., random_main[1]*M_PI/180.) ; 

	FillRotMatrixZY(RotMatrixSample, AnglSampleVert, AnglSampleHoriz) ;

	FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz) ;

	RotVector(RotMatrixSample, scattered_dir) ;


	/* init for ASCII output */

	NumOut=0 ;


	IntegralIntensity = 0. ;


}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{


}/* End OwnCleanup */


/* ReadParameterFile() reads the parameters from file */

void ReadParameterFile()
{

		fprintf(LogFilePtr,"	P1			=  %9.4f\n	P2			=  %9.4f\n	P3			=  %9.4f\n	P4			=  %9.4f\n	D1			=  %9.4f\n	D2			=  %9.4f\n	D3			=  %9.4f\n	temperature		=  %9.4f",
			
				P1, P2, P3, P4, D1, D2, D3, Temperature) ;

		fprintf(LogFilePtr,"\n	repetition  		=     %ld", Repetition) ;

  if(Repetition > 1)fprintf(LogFilePtr,"\nWarning: Excessive use of repetition rate > 1 can lead to wrong results. Be sure that you have very good statistics" 
	  "\nin wavelength, time, x,y,z and directions just before the sample") ;

		if(BoseF == 1) fprintf(LogFilePtr,"\nmultiplied by Bose-factor");

		if(BoseF != 1) fprintf(LogFilePtr,"\nnot multiplied by Bose-factor");

		if((BoseF == 1)&&(Temperature == 0.)) fprintf(LogFilePtr," (T = 0 means 1 for w > 0 and 0 for w < 0)");


	
		fprintf(LogFilePtr,"\ndata from parameter file: '%s':",ParameterFileName) ;
		if(P2 <= 0.0002)fprintf(LogFilePtr,"\nWARNING: P2 <= 0.0002 converted to P2 = 0.") ;

	
	

		/* reads from file by using ReadParF(Par_Sample) and ReadParComment(Par_Sample) */


		random_main[0]=ReadParF(Par_Sample) ; random_main[1]=ReadParF(Par_Sample) ; random_main[2]=ReadParF(Par_Sample) ;ReadParComment(Par_Sample) ;

		random_range[0]=ReadParF(Par_Sample) ; random_range[1]=ReadParF(Par_Sample) ; random_range[2]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		ScatteringC=ReadParF(Par_Sample) ; AbsorptionC=ReadParF(Par_Sample) ;  ReadParComment(Par_Sample) ;

		PosSample[0]=ReadParF(Par_Sample) ; PosSample[1]=ReadParF(Par_Sample) ; PosSample[2]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		AnglSampleHoriz=ReadParF(Par_Sample) ; AnglSampleVert=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		ReadParString(Par_Sample, Option) ; ReadParComment(Par_Sample) ;

		DimSample[0]=ReadParF(Par_Sample) ; DimSample[2]=ReadParF(Par_Sample) ; DimSample[1]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		User = ReadParI(Par_Sample) ; ReadParComment(Par_Sample) ;

		l_reference =ReadParF(Par_Sample) ; h_reference =ReadParF(Par_Sample) ; v_reference =ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		if(User == 1)
		{

		TranslOut[0]=ReadParF(Par_Sample) ; TranslOut[1]=ReadParF(Par_Sample) ; TranslOut[2]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		AnglOutHoriz=ReadParF(Par_Sample) ; AnglOutVert=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;

		}

		if((PosSample[0] < DimSample[0])||(PosSample[0] < DimSample[1])||(PosSample[0] < DimSample[2])) {fprintf(LogFilePtr,"\nERROR: Distance to sample must be larger than sample dimensions!\n") ; exit(0);}
		
/*	 checks some values */

	if((Option[1] != 'y') && (Option[1] != 'o') && (Option[1] != 'u') && (Option[1] != 'a'))
	{
			fprintf(LogFilePtr,"\nNo valid geometry option!\n") ;
			exit(0) ;
	}

	k_reference[0] = 2.* M_PI / l_reference * (double) cos(v_reference) * (double) cos(h_reference) ;

	k_reference[1] = 2.* M_PI / l_reference * (double) cos(v_reference) * (double) sin(h_reference) ;

	k_reference[2] = 2.* M_PI / l_reference * (double) sin(v_reference) ;



	if(LengthVector(k_reference) == 0.)
	{
		fprintf(LogFilePtr,"\nZero reference wavevector not allowed!\n");
		exit(0);
	}

		/* sets default values if frame for output not user defined */

		if(User != 1.)
		{

			if(LengthVector(k_reference) == 0.)
			{
				fprintf(LogFilePtr,"\nZero reference wavevector not allowed\n");
				exit(0);
			}


			/* computes angles corresponding to the output frame */
												
					
				AnglOutHoriz	= random_main[1] ;
				
				AnglOutVert		= random_main[2] ;
				

			/* shifts output frame origin to center of sample */

			CopyVector(PosSample, TranslOut) ;
		}
	

		/* prints parameters into log file for verification */

		fprintf(LogFilePtr,"\n	random main w, y, z		=  %9.4f, %9.4f, %9.4f\n	range x, y, z		=  %9.4f, %9.4f, %9.4f\n	absorption constant	=     %9.4e\n	cutoff probability		=     %8.1e",
			
				random_main[0], random_main[1], random_main[2], random_range[0], random_range[1], random_range[2], AbsorptionC, ProbCutoff) ;

		fprintf(LogFilePtr,"\n	position x, y, z		=  %9.4f, %9.4f, %9.4f\n	thickn./radius, height, width  =  %9.4f, %9.4f, %9.4f\n	offset angle horiz		=  %9.4f\n	offset angle vert		=  %9.4f",
			
				PosSample[0], PosSample[1], PosSample[2], DimSample[0], DimSample[2], DimSample[1], AnglSampleHoriz, AnglSampleVert) ;

		fprintf(LogFilePtr,"\n	reference-k x, y, z		=  %9.4f, %9.4f, %9.4f",
			
				k_reference[0], k_reference[1], k_reference[2]) ;


		if(Option[1] == 'y') fprintf(LogFilePtr,"\nsample geometry:	'cylinder'") ;

		if(Option[1] == 'o') fprintf(LogFilePtr,"\nsample geometry:	'hollow cylinder'") ;

		if(Option[1] == 'u') fprintf(LogFilePtr,"\nsample geometry:	'cuboid'") ;

		if(Option[1] == 'a') fprintf(LogFilePtr,"\nsample geometry:	'sphere'") ;/**/


		if(User != 1) fprintf(LogFilePtr,"\nstandard frame generation:") ;

		if(User == 1) fprintf(LogFilePtr,"\nuser defined frame:") ;

		fprintf(LogFilePtr,"\n	output horizontal angle	= %9.4f\n	output vertical angle	= %9.4f\n	X',Y',Z'			= %9.4f, %9.4f, %9.4f", 
			
				AnglOutHoriz, AnglOutVert, TranslOut[0], TranslOut[1], TranslOut[2]) ;


		/* converts degs in radian etc. */

		AnglSampleHoriz	*= M_PI/180. ;

		AnglSampleVert	*= M_PI/180. ;

		AnglOutHoriz	*= M_PI/180. ;

		AnglOutVert		*= M_PI/180. ;


	/* Hollow cylinder option */

  CopyVector(DimSample, DimSampleHol); DimSampleHol[0] = DimSample[1];

  
} /* End ReadParameterFile */


