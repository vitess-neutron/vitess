/********************************************************************************************/
/*  VITESS module 'sample_singcryst.c'                                                      */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Apr 2003  Géza Zsigmond  initial version                                            */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2  Nov 2013  D. Nekrassov   Visualisation, flexible input file formats introduced      */
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
#include "sample.h"

/* START HEADER STORY */

#define	STRING_BUFFER 200


	FILE		*Par_Sample, *StructureFactorFile; 
	char		Option[STRING_BUFFER], *ParameterFileName, *StructureFactorFileName, *SampleFileName;
	long		d_spr_option, datanumbermax = 10000, Repetition, repet, i,j;
	double		TOF, WL, Prob ;
        double		A_reciproc[3], B_reciproc[3], C_reciproc[3];
	double          *Fhkl2, *hh, *kk, *ll;
        int             *no;
	double		AnglPhi, AnglOmega, AnglChi, AnglOutHoriz, AnglOutVert ;
	double		RotMatrixOmega[3][3], RotMatrixChi[3][3], RotMatrixPhi[3][3], RotMatrixOut[3][3], RotMatrixDelta[3][3];
	double		AbsorptionC, Normalisation, Ddperd;
	VectorType	Pos, Dir, PosSample, DimSample, GG;
	Neutron		Neutrons ;



	double		dSpreadLorentzian(double rdelta);
	double		dSpreadGaussian(double rdelta);
	void		OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir);
	void		ReadParameterFile();
	void		OwnInit(int argc, char *argv[]);
	void		OwnCleanup();



/* FINISH HEADER STORY */



int main(int argc, char **argv)
{
  VectorType	Pos1f, Pos2f,
    Pos1v, Pos2v;

  /* Initialize the program according to the parameters given  */ 

  Init(argc, argv, VT_SMPL_EL_ISO); 

  OwnInit(argc, argv);


  /* Get the neutrons from the file */ 

  DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
    {
      CHECK;

      /* here is what happens to the neutron */

      {

	double	MaxPathLength, PathLength;

	for(i=0;i<NumNeutGot ;i++)

	  { 
	    CHECK;

	    InputNeutrons[i].Vector[0]		= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

	    if(InputNeutrons[i].Probability <= wei_min) goto getlost ;

	    /* translates and rotates into frame of the sample, where phi-khi-omega are consecutive rotations of the sample(!) in the initial frame */

	    SubVector(InputNeutrons[i].Position, PosSample);

	    RotBackVector(RotMatrixOmega, InputNeutrons[i].Position);
	    RotBackVector(RotMatrixChi, InputNeutrons[i].Position);
	    RotBackVector(RotMatrixPhi, InputNeutrons[i].Position);
	
	    RotBackVector(RotMatrixOmega, InputNeutrons[i].Vector);
	    RotBackVector(RotMatrixChi, InputNeutrons[i].Vector);
	    RotBackVector(RotMatrixPhi, InputNeutrons[i].Vector);

	
	    /* gives intersection positions with sample */

	    if(Option[1] == 'y')
	      {
		if(IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
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
	      {
		CHECK;
		CopyVector(Pos1f, Pos1v) ;
		CopyVector(Pos2f, Pos2v) ;
									
		GG[0] = hh[repet] * A_reciproc[0] + kk[repet] * B_reciproc[0] + ll[repet] * C_reciproc[0];
		GG[1] = hh[repet] * A_reciproc[1] + kk[repet] * B_reciproc[1] + ll[repet] * C_reciproc[1];
		GG[2] = hh[repet] * A_reciproc[2] + kk[repet] * B_reciproc[2] + ll[repet] * C_reciproc[2];
			
		TOF  = InputNeutrons[i].Time ;
		WL   = InputNeutrons[i].Wavelength ;
		Prob = InputNeutrons[i].Probability ;

		CopyVector(InputNeutrons[i].Position, Pos);
		CopyVector(InputNeutrons[i].Vector, Dir);


		/* scattering position and TOF untill scattering */	
				
		SubVector(Pos2v, Pos1v);					/*maximal path vector*/ 

		MaxPathLength = LengthVector(Pos2v); 

		MultiplyByScalar(Pos2v, MonteCarlo(0.,1.));	 /*random path vector untill scattering */

		PathLength = LengthVector(Pos2v);

		AddVector(Pos1v, Pos2v);	
					
		TOF += (Pos1v[0] - Pos[0])/ fabs(Dir[0]) / V_FROM_LAMBDA(WL);

		CopyVector(Pos1v, Pos);						/*scattering position */


		if((WL*LengthVector(GG)/4./M_PI)>1) Prob = 0.; 

		/* attenuation untill scattering normalized to maximal path and probability */

		Prob *= (double) exp( - PathLength * AbsorptionC * WL );
				
		Prob *= MaxPathLength * Normalisation * Fhkl2[repet] * 4. * M_PI * sq(WL/LengthVector(GG)) ; 


		/* scattering: new neutron variables*/ 
					
		{
		  double	k_inc[3], GGact[3]; 

		  CopyVector(Dir, k_inc);
		  MultiplyByScalar(k_inc, 2 * M_PI / WL);
		  CopyVector(GG, GGact); 
		  MultiplyByScalar(GGact, (-2.* ScalarProduct(GG, k_inc)/ScalarProduct(GG,GG)));

		  if(d_spr_option == 1) Prob *= dSpreadLorentzian((1. - LengthVector(GGact)/LengthVector(GG)));
		  if(d_spr_option == 2) Prob *=   dSpreadGaussian((1. - LengthVector(GGact)/LengthVector(GG)));

		  /* defines now outgoing k direction */
		  AddVector(k_inc, GGact); 
		  MultiplyByScalar(k_inc, 1./LengthVector(k_inc));
		  CopyVector(k_inc, Dir);
		}


		/* attenuation succeeding scattering */


		if(Option[1] == 'y')
		  {
		    if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.; 
		  }

		if(Option[1] == 'u')
		  {
		    if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.;  
		  }

		if(Option[1] == 'a')
		  {
		    if(IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.; 
		  }


		/* path in the sample after scattering */

		{
		  VectorType Pos_final ;

					
		  CopyVector(Pos2v, Pos_final);

					
		  SubVector(Pos_final, Pos);

					
		  PathLength = LengthVector(Pos_final);  
		}


		Prob *= (double) exp( - PathLength * AbsorptionC * WL );


		/* Output matters */

		TOF +=  PathLength / V_FROM_LAMBDA(WL);


		OutputTransformations(&TOF, &WL, &Prob, Pos2v, Dir);
					

		/* transmit coordinates which were not changed, the rest overwrite below */
		Neutrons = InputNeutrons[i]; 


		Neutrons.Time = TOF ;

		Neutrons.Probability = Prob ;

		CopyVector(Pos2v, Neutrons.Position);

		CopyVector(Dir, Neutrons.Vector);

		Neutrons.Color = (short) no[repet]; 


		/*	 writes output binary file */

		if(Prob > wei_min) WriteNeutron(&Neutrons);

					
	      }/*repetition*/
	    /* here continues if neutron gets lost */

	  getlost:;
	  }
      }  
    }
   
  /* Do the general cleanup */


 my_exit:

  fprintf(LogFilePtr," \n");

  if (no!=NULL) {
    free(no); 
    free(hh);
    free(kk);
    free(ll);
    free(Fhkl2);
  }
  Cleanup(PosSample[0], PosSample[1], PosSample[2], AnglOutHoriz, AnglOutVert);

  return 0;
}

/* here is what happens to the neutron */

	
/* Output matters */

void OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir)
{
	/* computes neutron variables in the initial frame */

	RotVector(RotMatrixPhi, Dir);
	RotVector(RotMatrixChi, Dir);
	RotVector(RotMatrixOmega, Dir);
	
	RotVector(RotMatrixPhi, Pos);
	RotVector(RotMatrixChi, Pos);
	RotVector(RotMatrixOmega, Pos);
	
	AddVector(Pos, PosSample);


	/* computes neutron variables in the output frame */

	SubVector(Pos, PosSample);

	RotVector(RotMatrixOut, Pos);

	RotVector(RotMatrixOut, Dir);


	/* translates neutron variables for output - X'=0. 

	{

	VectorType Path ;

	*tof = *tof - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA_PT(wl);

	CopyVector(Dir, Path);

	MultiplyByScalar(Path, - Pos[0]/ Dir[0] );

	AddVector(Pos, Path);  
	
	}			Path = displacement vector 
	*/

} /* End OutputTransformations()*/


/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
	fprintf(LogFilePtr," \n");
	print_module_name("sample_singcryst 1.1");

	Ddperd=0.01;
	d_spr_option = 1 ;

	colh = -1; colk = -1; coll = -1; colD = -1;
	colF = -1; colF2 = -1; colM = -1; colDW = -1;
	scaleF2 = 1.;
	

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
		
			case 'S':
			if((StructureFactorFile = fopen(&argv[1][2],"r"))==NULL)
			{
				fprintf(LogFilePtr,"\nStructure factor file '%s' not found\n",&argv[1][2]);
				exit(0);
			}
			StructureFactorFileName=&argv[1][2];
			break;
		
		case 'd':
			sscanf(&argv[1][2], "%lf", &Ddperd); /* d-spacing spread, this gives the relative 'thickness' of the Ewald sphere */
			break;

		case 'o':
			sscanf(&argv[1][2], "%ld", &d_spr_option);
			break;

		}
		argc--;
		argv++;
	}
	
	/* reads parameter file */

	ReadParameterFile(); 

	if(Par_Sample != NULL)fclose(Par_Sample);

	
	FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz);


	FillRotMatrixZY(RotMatrixPhi, 0., AnglPhi); 

	FillRotMatrixXZ(RotMatrixChi, 0., AnglChi); 

	FillRotMatrixZY(RotMatrixOmega, 0., AnglOmega); 


	if (strstr(StructureFactorFileName, ".dat") == &StructureFactorFileName[strlen(StructureFactorFileName)-4])
    {
      long count;
      datanumbermax = LinesInFile(StructureFactorFile);
      rewind(StructureFactorFile);
     
      no  = (int*) calloc(datanumbermax, sizeof(int));
      hh = (double*) calloc(datanumbermax, sizeof(double));
      kk = (double*) calloc(datanumbermax, sizeof(double));
      ll = (double*) calloc(datanumbermax, sizeof(double));
      Fhkl2 = (double*) calloc(datanumbermax, sizeof(double));
      
		  for(count=0; count<datanumbermax; count++)
		{
 			no[count] = ReadParF(StructureFactorFile);
			 
			hh[count] = ReadParF(StructureFactorFile);
			 
			kk[count] = ReadParF(StructureFactorFile);
			 
			ll[count] = ReadParF(StructureFactorFile);
			 
			Fhkl2[count] = ReadParF(StructureFactorFile) ; if(Fhkl2[count] == 0.) break ;

			ReadParComment(StructureFactorFile);

			Repetition = count +1 ; 
		}
			fprintf(LogFilePtr, "\nNumber of reflections: %ld\n", Repetition);

			//		  for(count=0; count<Repetition; count++) fprintf(LogFilePtr, "%ld  %lf  %lf  %lf  %lf\n", count, hh[count], kk[count], ll[count],  Fhkl2[count]);
    }
	else {
	  int jj;
	  Repetition = ReadStructureFile(StructureFactorFileName, 2, 0);
	  no  = (int*) calloc(Repetition, sizeof(int));
	  for (jj = 0; jj < Repetition; jj++) no[jj] = jj+1;
	  hh = hVal;
	  kk = kVal;
	  ll = lVal;
	  Fhkl2 = F2Val;
	}

	fprintf(LogFilePtr, "\nStructure file read!\n");
      


}/* End OwnInit */



/* ReadParameterFile() reads the parameters from file */

void ReadParameterFile()
{

  SampleType sample;
  
		/* reads from file by using ReadParF(Par_Sample) and ReadParComment(Par_Sample) */


		A_reciproc[0]=ReadParF(Par_Sample); A_reciproc[1]=ReadParF(Par_Sample); A_reciproc[2]=ReadParF(Par_Sample);/* */ReadParComment(Par_Sample);

		B_reciproc[0]=ReadParF(Par_Sample); B_reciproc[1]=ReadParF(Par_Sample); B_reciproc[2]=ReadParF(Par_Sample);/* */ReadParComment(Par_Sample);

		C_reciproc[0]=ReadParF(Par_Sample); C_reciproc[1]=ReadParF(Par_Sample); C_reciproc[2]=ReadParF(Par_Sample);/* */ReadParComment(Par_Sample);

		Normalisation=ReadParF(Par_Sample); AbsorptionC=ReadParF(Par_Sample); ReadParComment(Par_Sample);

		PosSample[0]=ReadParF(Par_Sample); PosSample[1]=ReadParF(Par_Sample); PosSample[2]=ReadParF(Par_Sample); ReadParComment(Par_Sample);

		AnglPhi=ReadParF(Par_Sample); AnglChi=ReadParF(Par_Sample); AnglOmega=ReadParF(Par_Sample); ReadParComment(Par_Sample);

		ReadParString(Par_Sample, Option) ; ReadParComment(Par_Sample) ;

		DimSample[0]=ReadParF(Par_Sample); DimSample[2]=ReadParF(Par_Sample); DimSample[1]=ReadParF(Par_Sample); ReadParComment(Par_Sample);

		AnglOutHoriz=ReadParF(Par_Sample); AnglOutVert=ReadParF(Par_Sample); ReadParComment(Par_Sample);

		colh = ReadParF(Par_Sample); colk = ReadParF(Par_Sample); coll = ReadParF(Par_Sample);
		colF = ReadParF(Par_Sample); colF2 = ReadParF(Par_Sample); colDW= ReadParF(Par_Sample);
		scaleF2 = ReadParF(Par_Sample);
		
/*	 checks some values */

	if((Option[1] != 'y') && (Option[1] != 'u') && (Option[1] != 'a'))
	{
			fprintf(LogFilePtr,"\nNo valid geometry option!\n") ;
			exit(0) ;
	}


	sample.Position[0] = PosSample[0];
	sample.Position[1] = PosSample[1];
	sample.Position[2] = PosSample[2];

 if(Option[1] == 'y') {

    sample.SG.Cyl.r = DimSample[0];
    sample.SG.Cyl.height = DimSample[1];
    sample.Type = VT_CYL;

    fprintf(LogFilePtr,"\n             sample geometry:	'cylinder'") ;

  }
  if(Option[1] == 'u') {
   
    sample.SG.Cube.thickness = DimSample[0];
    sample.SG.Cube.width = DimSample[1];
    sample.SG.Cube.height = DimSample[2];
    sample.Type = VT_CUBE;

    fprintf(LogFilePtr,"\n             sample geometry:	'cuboid'") ;

  }
  if(Option[1] == 'a') {

    sample.SG.Ball.r = DimSample[0];
    sample.Type = VT_SPHERE;

    fprintf(LogFilePtr,"\n             sample geometry:	'sphere'") ;

  }

  SetSampleGeometry(&sample);	


		
		/* converts degs in radian etc. */

		AnglOmega *= M_PI/180.;

		AnglChi	*= M_PI/180.;

		AnglPhi	*= M_PI/180.;

		AnglOutHoriz	*= M_PI/180.;

		AnglOutVert		*= M_PI/180.;



}/* End ReadParameterFile */


/* probability of finding a d-spacing in Lorentzian approximation.
Maximum probability amplitude = 1 */

double	dSpreadLorentzian(double rdelta)
{
	return	sq(Ddperd) / ( 4*sq(rdelta) + sq(Ddperd) );

}/* End dSpreadLorentzian */


/* probability of finding a d-spacing in Gaussian approximation.
Maximum probability amplitude = 1 */

double	dSpreadGaussian(double rdelta)
{
double argd ;

	argd = - sq(rdelta / Ddperd) * 4. *log(2);

	if(argd < - 100.) argd = -100.;

	return (double) exp(argd);

}/* End dSpreadGaussian */



