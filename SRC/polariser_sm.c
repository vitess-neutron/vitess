/********************************************************************************************/
/*  VITESS module 'polariser_sm.c'                                                          */
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

	FILE		*Par_Field, *reflup_file, *refldown_file;
	char		Option[STRING_BUFFER], *ParameterFileName, *ReflUpFileName, *ReflDownFileName, XFileName[STRING_BUFFER];
	int			datanumber ;
	long		User, NumOut, Repetition, repet,  i, m, NoCh, ok ;
	double		TOF, TOFprec, WL, Prob, phi, the, phinew, thenew, WidthCh, WallTh, shift, CritUp, CritDown, ReflUp, ReflDown, AngleSM, aUU, aDD ;
	double		rupdata[1001], rdowndata[1001], NumberPrecessions, PhaseShift, LarmorMatrix[3][3];
	double		AngleSMHoriz, AngleSMVert, AnglOutHoriz, AnglOutVert, smearfUp, smearfDown ;
	double		RotMatrixSM[3][3], RotMatrixField[3][3], RotMatrixOut[3][3], RotMatrixAnalysis[3][3] ;
	double		ProbCutoff, IntegralIntensity ;
	VectorType	Pos, Dir, SpinVector, Path, n, postop, posbot, analysis_dir, guide_field, PosSM, DimSM, TranslOut ;
	Neutron		Neutrons ;

	double		reflectivity(double angle, double CritUp, double WL, double smearf, double ReflUp);
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);
	void		ReadParameterFile() ;
	void		OwnInit(int argc, char *argv[]) ;
	void		OwnCleanup() ;

/* FINISH HEADER STORY */


int main(int argc, char **argv)
{

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv, VT_POL_SM);
	OwnInit(argc, argv);/**/ 


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

	CopyVector(InputNeutrons[i].Spin, SpinVector) ; 

	InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;


/* translates into frame of the SM  */
	
	SubVector(Pos, PosSM) ;

	RotVector(RotMatrixSM, Pos) ;

	RotVector(RotMatrixSM, Dir) ;


/* calculate spin vector in the direction of the analysis */

	RotVector(RotMatrixAnalysis, SpinVector);

	CartesianToSpherical(SpinVector, &the, &phi);


/* select channel and shift vertically to its frame */n[1]=n[2]=0.;n[0]=1.;

if((PlaneLineIntersect(Pos, Dir, n, - DimSM[0]/2., posbot) == TRUE)&&(fabs(posbot[1]) < DimSM[1]/2.)&&(fabs(posbot[2]) < DimSM[2]/2.)&&(Dir[0] > 0.))
{
		double epsilonZ = (posbot[2] + DimSM[2]/2.) / DimSM[2] * (double) NoCh ;

		int No = (int) floor(epsilonZ)+1 ; 

		shift = (WidthCh + WallTh) *(- (NoCh -1)/2. + (No - 1)) ;
		Pos[2] += - shift ; 

		if(fabs(posbot[2]-shift) > WidthCh/2.) goto getlost;/**/

} 
else goto getlost;                      


TOFprec = 0. ;

/* reflecting in channels top/bottom */

for (m = 1; m < 1000; m++)		
{
	int r=0; 

CHECK;
	/* reflection on top/bottom */n[0]=n[1]=0.;n[2]=1.;
	
	if((PlaneLineIntersect(Pos, Dir, n, + WidthCh/2., postop) == TRUE)&&(postop[0] > (Pos[0]+0.1))&&(fabs(postop[0]) < DimSM[0]/2.)&&(Dir[2] > 0.))
	{
			
		{
		/* polarizing */
		
		double szog= fabs((double) asin(Dir[2])); 

		datanumber = (int) (szog *180./M_PI * 1000./WL) ; 
		
		if(datanumber > 1000) goto getlost ; 

		aUU = /*1.*/sqrt(rupdata[datanumber]) ;

		aDD = /*0.*/ sqrt(rdowndata[datanumber]) ;


		if((aUU == 0.)&&(aDD == 0.)) goto getlost ;
			
		thenew = 2. * (double) atan(aDD/ aUU *(double) tan(the/2.)) ;

		phinew = phi /* + Phipol */;

		Prob *= sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.)) ;

		}

		TOFprec += (postop[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

		CopyVector(postop, Pos);

		Dir[2] *= -1 ; r=1;				/*ps(i+1); ps(m); ps(+77);goto getlost;ps(the);*/


		the = thenew ; phi = phinew ;


		goto contin;  


	} 
	if((PlaneLineIntersect(Pos, Dir, n, - WidthCh/2., posbot) == TRUE)&&(posbot[0] > (Pos[0]+0.1))&&(fabs(posbot[0]) < DimSM[0]/2.)&&(Dir[2] < 0.))
	{
		{
		/* polarizing */
		
		double szog= fabs((double) asin(Dir[2]));

		datanumber = (int) (szog *180./M_PI * 1000./WL) ;

		if(datanumber > 1000) goto getlost ;  

		aUU = /*1.*/rupdata[datanumber] ;

		aDD = /*0.*/ rdowndata[datanumber] ;

		if((aUU == 0.)&&(aDD == 0.)) goto getlost ;
			
		thenew = 2. * (double) atan(aDD/ aUU *(double) tan(the/2.)) ;

		phinew = phi /* + Phipol */;

		Prob *= sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.)) ;

		}

		TOFprec += (postop[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

		CopyVector(posbot, Pos);

		Dir[2] *= -1 ; r=1;			


		the = thenew ; phi = phinew ;

		goto contin;  


	}
contin:;	
	   
	   
	/* absorbtion on the sides */n[0]=n[2]=0.;n[1]=1.;
	
	if((PlaneLineIntersect(Pos, Dir, n, + DimSM[1]/2, postop) == TRUE)&&(Dir[1] > 0.)&&(postop[0] > Pos[0])&&(fabs(postop[0]) < DimSM[0]/2.))
	{goto getlost;} 
	
	if((PlaneLineIntersect(Pos, Dir, n, - DimSM[1]/2, posbot) == TRUE)&&(Dir[1] < 0.)&&(posbot[0] > Pos[0])&&(fabs(posbot[0]) < DimSM[0]/2.))
	{goto getlost;} 
	
	if(r==0) goto leave; /* cannot be reflected anymore */
}

/* leave channel and shift vertically back to main SM frame */

leave:;
	
	Pos[2] += shift ;

	SphericalToCartesian(SpinVector, &the, &phi);


/*n[0]=1.;n[1]=0.;n[2]=0.;

if((PlaneLineIntersect(Pos, Dir, n, + DimSM[0]/2., posbot) == TRUE)) CopyVector(posbot, Pos);

 Output matters */

	if(Prob <= ProbCutoff) goto getlost ;

	IntegralIntensity += Prob ;

	NumOut++ ;


	/* translates into initial frame   */
	
	RotBackVector(RotMatrixSM, Pos) ;

	RotBackVector(RotMatrixSM, Dir) ;

	AddVector(Pos, PosSM) ;

	/* computes neutron variables in the output frame */

	SubVector(Pos, TranslOut) ;

	RotVector(RotMatrixOut, Pos) ;

	RotVector(RotMatrixOut, Dir) ;

	RotVector(RotMatrixOut, SpinVector) ;


	/* translates neutron variables for output - X'=0. */
	{

	VectorType Path ;

	TOFprec +=  - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

	CopyVector(Dir, Path) ;

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;

	AddVector(Pos, Path) ;  
	
	}			/* Path = displacement vector */


	/* precession in the guide field */

	RotVector(RotMatrixField, SpinVector) ; 

	PhaseShift = TOFprec * FREQUENCY_FROM_FIELD(guide_field[0]) ;  NumberPrecessions = PhaseShift/2./M_PI ;

	FillRotMatrixYX(LarmorMatrix, PhaseShift, 0) ;

	RotVector(LarmorMatrix, SpinVector) ;

	RotBackVector(RotMatrixField, SpinVector) ;


	/* transmit coordinates which were not changed, the rest overwrite below */
	Neutrons = InputNeutrons[i]; 

	Neutrons.Time = TOF + TOFprec;

	Neutrons.Probability = Prob ;

	CopyVector(Pos, Neutrons.Position) ;

	CopyVector(Dir, Neutrons.Vector) ;

	CopyVector(SpinVector, Neutrons.Spin) ;


	/* writes output binary file */

	WriteNeutron(&Neutrons) ;


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




/* reflectivity function */

double	reflectivity(double angle, double CritUp, double WL, double smearf, double ReflUp)
{

	return ReflUp/(exp((angle - CritUp*WL)/CritUp/WL/smearf)+1);
}




/* own initialization of the polarizersm module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n") ;
	print_module_name("polarizer_sm 1.2") ;

/*    INPUT  */
	
	guide_field[0] = guide_field[1] = guide_field[2] = 0. ;

	ProbCutoff=wei_min;
	
	while(argc>1)
	{
		switch(argv[1][1])
		{
				
			  case 'P':
				if((Par_Field = fopen(&argv[1][2],"r"))==NULL)
				{
					fprintf(LogFilePtr,"\nParameter file '%s' not found\n",&argv[1][2]);
					exit(0);
				}
				ParameterFileName=&argv[1][2];
				break;
		
			  case 'U':
				if( (reflup_file = fopen(&argv[1][2],"r"))==NULL)
				  {
				fprintf(LogFilePtr,"File %s could not be opened for InputNeutrons\n",&argv[1][2]);
				exit(-1);}

				ReflUpFileName=&argv[1][2];
				break;
						
			  case 'D':
				if( (refldown_file = fopen(&argv[1][2],"r"))==NULL)
				  {
				fprintf(LogFilePtr,"File %s could not be opened for InputNeutrons\n",&argv[1][2]);
				exit(-1);}

				ReflDownFileName=&argv[1][2];
				break;
				

			case 'a':
			sscanf(&argv[1][2], "%lf", &PosSM[0]) ;
			break;

			case 'b':
			sscanf(&argv[1][2], "%lf", &PosSM[1]) ;
			break;

			case 'c':
			sscanf(&argv[1][2], "%lf", &PosSM[2]) ;
			break;

			case 'H':
			sscanf(&argv[1][2], "%lf", &AngleSMHoriz) ;
			break;

			case 'V':
			sscanf(&argv[1][2], "%lf", &AngleSMVert) ;
			break;

			case 'R':
			sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
			break;

			case 'E':
			sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
			break;

			case 'G':
			sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
			break;

			case 'h':
			sscanf(&argv[1][2], "%lf", &AnglOutHoriz) ;
			break;

			case 'v':
			sscanf(&argv[1][2], "%lf", &AnglOutVert) ;
			break;



		}
		argc--;
		argv++;
	}
	
	/* reads reflectivity files */

    {long count;
		  for(count=0; count<1000; count++)
		{
		  if (fscanf(reflup_file,"%lf",&rupdata[count])==EOF)
			break;
		}
		  for(count=0; count<1000; count++)
		{
		  if (fscanf(refldown_file,"%lf",&rdowndata[count])==EOF)
			break;
		}

/*		  for(count=0; count<1000; count++)

		  {
			  fprintf(XFILE, " %f	%f	%f \n", (count*0.001), rupdata[count], rdowndata[count]);
		  }
*/
		fclose(reflup_file);

		fclose(refldown_file);
	}
 


	/* reads parameter file */

	ReadParameterFile() ; 

	if(Par_Field != NULL)fclose(Par_Field) ;

/*	 checks some values */

		/* prints parameters into log file for verification */

		if(((NoCh+1)/2. - floor((NoCh+1)/2.)) > 0.) {	NoCh += -1 ; fprintf(LogFilePtr,"\nWARNING: Number of channels must be odd! Set %ld. \n", NoCh) ;}

		
		fprintf(LogFilePtr,"\n	cutoff probability		=     %8.1e",
		
			 ProbCutoff) ;

		fprintf(LogFilePtr,"\n	position x, y, z		=  %9.4f, %9.4f, %9.4f\n	length, height, width  =  %9.4f, %9.4f, %9.4f\n	offset angle horiz		=  %9.4f\n	offset angle vert		=  %9.4f",
			
				PosSM[0], PosSM[1], PosSM[2], DimSM[0], DimSM[2], DimSM[1], AngleSMHoriz, AngleSMVert) ;




		fprintf(LogFilePtr,"\n	output horizontal angle	= %9.4f\n	output vertical angle	= %9.4f\n	X',Y',Z'			= %9.4f, %9.4f, %9.4f\n", 
			
				AnglOutHoriz, AnglOutVert, TranslOut[0], TranslOut[1], TranslOut[2]) ;


		/* converts degs in radian etc. */

		AngleSMHoriz	*= M_PI/180. ;

		AngleSMVert	*= M_PI/180. ;

		AnglOutHoriz	*= M_PI/180. ;

		AnglOutVert		*= M_PI/180. ;


	aUU = aDD = 1.;


	FillRotMatrixZY(RotMatrixSM, AngleSMVert, AngleSMHoriz) ;

	FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz) ;

	
	WidthCh = (DimSM[2] - (NoCh+1)*WallTh)/NoCh ;

	{ double roty, rotz ;

	CartesianToEulerZY(analysis_dir, &roty, &rotz) ; 

	FillRotMatrixZY(RotMatrixAnalysis, roty, rotz) ; 	}


	{ double roty, rotz ;

	CartesianToEulerZY(guide_field, &roty, &rotz) ; 

	FillRotMatrixZY(RotMatrixField, roty, rotz) ; 	}


/*	 init for ASCII output */


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

		fprintf(LogFilePtr,"\ndata from parameter file: '%s':\n",ParameterFileName) ;


		/* reads from file by using ReadParF(Par_SM) and ReadParComment(Par_Field) */


		DimSM[0]=ReadParF(Par_Field) ; DimSM[2]=ReadParF(Par_Field) ; DimSM[1]=ReadParF(Par_Field) ; ReadParComment(Par_Field) ;

		NoCh=ReadParI(Par_Field) ; WallTh=ReadParF(Par_Field) ; ReadParComment(Par_Field) ;

		guide_field[0]=ReadParF(Par_Field) ; guide_field[1]=ReadParF(Par_Field) ; guide_field[2]=ReadParF(Par_Field) ; ReadParComment(Par_Field) ;

		analysis_dir[0]=ReadParF(Par_Field) ; analysis_dir[1]=ReadParF(Par_Field) ; analysis_dir[2]=ReadParF(Par_Field) ; ReadParComment(Par_Field) ;

		fprintf(LogFilePtr,"	guide_fieldX			=  %9.4f\n	guide_fieldY			=  %9.4f\n	guide_fieldZ			=  %9.4f\n",
			
				guide_field[0], guide_field[1], guide_field[2]) ;

}




