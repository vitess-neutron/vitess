/*******************************************************************************************/
/* Tool CrystalAnalyzerSpectrometer:                                                       */
/*	 Monte Carlo simulations following the theory in                                       */
/*  J. M. Carpenter, Time focusing of pulsed-source crystal analyzer spectrometers.        */
/*                   Part I: general analysis                                              */
/*                                                                                         */
/* The free non-commercial use of these routines is granted provided due credit is given   */
/* to the authors.                                                                         */
/*                                                                                         */
/* 1.0  Oct 2003  G. Zsigmond    initial version                                           */
/* 1.1  Mar 2020  K. Lieutenant  tidy up                                                   */
/*                                                                                         */
/*	All Versions 4.x work with uniform mosaicity, elastic scattering, uniform wavelength,  */
/*    delta function and time from moderator.                                              */
/*	Proposes in log file R_X and R_D after zeta rotation around W vector                   */
/*	X+ shows in beam direction, Y+ is horizontal left, Z+ vertical up                      */
/*	framework of moderator, sample, analyzer, detector means a coordinate system           */
/*    where the element is perpendicular to X+                                             */
/*	Version 4.0 includes the 1st order SD calculations                                     */
/*******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define	STRING_BUFFER 100


/*********************************/
/** Global and Static Variables **/
/*********************************/
double     d_spacing; // from input
VectorType R_M, R_S, R_X, R_D;   // from input

double     tof, deltatof, deltatof_av, deltatof_2av, deltatof_SD, tof_1st, deltatof_1st, deltatof_1st_av, deltatof_1st_2av, deltatof_1st_SD, deltatof_1st_eq19, tof_0;
double     lambda, lambda_random, widthlambda, deltalambda, deltalambda_av, deltalambda_2av, deltalambda_SD, lambda_0, L_1, L_2, L_3, l_1, l_2, l_3, l_total, l_1_av, l_2_av, l_3_av, l_total_av, l_1_2av, l_2_2av, l_3_2av, l_total_2av, l_1_SD, l_2_SD, l_3_SD, l_total_SD, deltal_1, deltal_2, deltal_3;
double     deltathetaB, deltaCos2thetaB, deltaCos2thetaB_1st, thetaB, Cos2thetaB, thetaB_0, Cos2thetaB_0, Phi, Phi_zeta, CosPhi;
double     Phi_M_foc, Phi_D_foc, Phi_X_foc, Phi_S_foc, psi, chi, alpha, U_abs, V_abs, W_abs, Z_abs, F_M_abs, F_S_abs, F_X_abs, F_D_abs;
double     P_M_abs, P_S_abs, P_X_abs, P_D_abs, S_M_abs, S_S_abs, S_X_abs, S_D_abs, zeta, xi_M, xi_S, xi_X, xi_D;
double     delta_M_term, delta_S_term, delta_X_term, delta_D_term, delta_M_term_SD, delta_S_term_SD, delta_X_term_SD, delta_D_term_SD, balance_eq43;
double     delta_M_term_2av, delta_S_term_2av, delta_X_term_2av, delta_D_term_2av, delta_M_term_av, delta_S_term_av, delta_X_term_av, delta_D_term_av, sigma2_M_th, sigma2_S_th, sigma2_X_th, sigma2_D_th, area_moment_M, area_moment_S, area_moment_X, area_moment_D;
VectorType L_1_vect, L_2_vect, L_3_vect;
VectorType L_1_vect_unit, L_2_vect_unit, L_3_vect_unit;
VectorType r_M, r_S, r_X, r_D;
VectorType l_1_vect, l_2_vect, l_3_vect;
VectorType l_1_vect_unit, l_2_vect_unit, l_3_vect_unit;
VectorType delta_M, delta_S, delta_X, delta_D;
VectorType F_M, F_S, F_X, F_D, F_M_unit, F_S_unit, F_X_unit, F_D_unit, n_M, n_S, n_X, n_D, P_M, P_S, P_X, P_D, S_M, S_S, S_X, S_D, P_M_unit, P_S_unit, P_X_unit, P_D_unit, S_M_unit, S_S_unit, S_X_unit, S_D_unit;
VectorType U_vect, V_vect, W_vect, Z_vect, Normal_M_foc, Normal_S_foc, Normal_X_foc, Normal_D_foc, q_B_0_vect;
VectorType R_X_zeta, R_D_zeta;   

/* variables defined by the distribution functions: heigth, width, Euler orientation of the rectangular surfaces. From input */
double     Height_M, Width_M, Height_S, Width_S, Height_X, Width_X, Height_D, Width_D;
double     Euler_M_aroundZ, Euler_M_aroundY, Euler_S_aroundZ, Euler_S_aroundY, Euler_X_aroundZ, Euler_X_aroundY, Euler_D_aroundZ, Euler_D_aroundY;
double     Euler_L1_aroundZ, Euler_L1_aroundY, Euler_L2_aroundZ, Euler_L2_aroundY, Euler_L3_aroundZ, Euler_L3_aroundY, Euler_q_B_0_aroundZ, Euler_q_B_0_aroundY;

/* computing variables */  
FILE       *Par_CAS, *Statistics_CAS, *TrajData_CAS, *TrajPoints_CAS, *DetPoints_CAS; 
char       *ParameterFileName, *StatisticsFileName, *TrajDataFileName, *TrajPointsFileName, *DetPointsFileName;
long       NoTrajectories, NoTrajectoriesEnd, i, Option, phi_repetition;
double     RotMatrix_M[3][3], RotMatrix_S[3][3], RotMatrix_X[3][3], RotMatrix_D[3][3], RotMatrix_W[3][3], RotMatrix_zeta[3][3];
double     Euler_W_aroundY, Euler_W_aroundZ, Euler_M_aroundY_foc, Euler_M_aroundZ_foc, Euler_S_aroundY_foc, Euler_S_aroundZ_foc, Euler_X_aroundY_foc, Euler_X_aroundZ_foc, Euler_D_aroundY_foc, Euler_D_aroundZ_foc;
VectorType n;


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);                                // Reads input parameters and sets global parameters
void  OwnCleanup();                                                   // Does module specific cleanup
void  VectorProduct(VectorType v1, VectorType v2, VectorType result); // calculates vector product of two vectors
void  CopyFileIntoFile(char *filename1, char *filename2);             // writes a string into a file not opened
void  EchoSomething(char *filename, char stringvar[STRING_BUFFER]);   // writes text into a file not opened
void  ShowTime(char *filename);                                       // writes time into a file not opened
void  ShowDate(char *filename);                                       // writes date into a file not opened

/* from init.c */
char* FullInName  (const char* filename);                             // adds output dir to file name 
char* FullOutName (const char* filename);                             // adds input dir to file name 


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv) 
{
  /* Initialize the program according to the parameters given   */
	Init(argc, argv, 1);
	OwnInit(argc, argv);

	/* start trajectories */
	for(i=1;i<(NoTrajectories+1);i++)
  {
  	/* computes points on the moderator: delta_M and r_M */
		delta_M[0] = 0.; delta_M[1] = MonteCarlo(- Width_M/2., Width_M/2.); delta_M[2] = MonteCarlo(- Height_M/2., Height_M/2.); // in frame of moderator
		RotBackVector(RotMatrix_M, delta_M); // in frame of lab
		CopyVector(delta_M, r_M); AddVector(r_M, R_M);

  	/* computes points on the sample: delta_S and r_S */
    delta_S[0] = 0.; delta_S[1] = MonteCarlo(- Width_S/2., Width_S/2.); delta_S[2] = MonteCarlo(- Height_S/2., Height_S/2.); // in frame of sample
		RotBackVector(RotMatrix_S, delta_S); // in frame of lab
		CopyVector(delta_S, r_S); AddVector(r_S, R_S);

  	/* computes points on the analyzer: delta_X and r_X */
		delta_X[0] = 0.; delta_X[1] = MonteCarlo(- Width_X/2., Width_X/2.); delta_X[2] = MonteCarlo(- Height_X/2., Height_X/2.); // in frame of analyzer
		RotBackVector(RotMatrix_X, delta_X); // in frame of lab
		CopyVector(delta_X, r_X); AddVector(r_X, R_X);

  	/* computes vector distances between vertices on moderator, sample, analyzer: l_1_vect, l_2_vect */ 
		CopyVector(r_S, l_1_vect); SubVector(l_1_vect, r_M); l_1 = LengthVector(l_1_vect);
		CopyVector(l_1_vect, l_1_vect_unit); MultiplyByScalar(l_1_vect_unit, 1./l_1);
		CopyVector(r_X, l_2_vect); SubVector(l_2_vect, r_S); l_2 = LengthVector(l_2_vect);
		CopyVector(l_2_vect, l_2_vect_unit); MultiplyByScalar(l_2_vect_unit, 1./l_2);

  	/* computes Bragg reflection and l_3_vect_unit */
		phi_repetition = 0;

  repet: ;
    {
      double	random=0., theta, phi, Matrix[3][3] ;

      random = MonteCarlo(-1.,1.);
      lambda_random = lambda + widthlambda/2.*random;

      thetaB = (double) asin(lambda_random/2./d_spacing); Cos2thetaB = cos(2.*thetaB);

      deltathetaB = thetaB - thetaB_0; 
      deltaCos2thetaB = Cos2thetaB - Cos2thetaB_0;
      deltalambda = lambda_random - lambda_0;

      if(lambda_random > 2*d_spacing) goto getlost;

      theta = 2*acos(lambda_random/2./d_spacing);
      phi = MonteCarlo(0., 1.); phi *=2*M_PI; phi_repetition +=1; // fprintf(LogFilePtr, "\n %ld", phi_repetition);
			  
      SphericalToCartesian(l_3_vect_unit, &theta, &phi);
      RotMatrixX(l_2_vect_unit, Matrix);
      RotBackVector(Matrix, l_3_vect_unit);

      MultiplyByScalar(l_3_vect_unit, -1.);
    }
		
    /* point on the detector: delta_D and l_3_vect */
    {
      VectorType r_X_detframe, l_3_vect_unit_detframe;

      CopyVector(r_X, r_X_detframe); 
      CopyVector(l_3_vect_unit, l_3_vect_unit_detframe); 
      SubVector(r_X_detframe, R_D); // shifts coordinate system origin to detector origin

      RotVector(RotMatrix_D, r_X_detframe); // rotates to frame of detector
      RotVector(RotMatrix_D, l_3_vect_unit_detframe); // rotates to frame of detector

      n[0] = 1.; n[1] = n[2] = 0.; if(PlaneLineIntersect2(r_X_detframe, l_3_vect_unit_detframe, n, 0., delta_D)!= TRUE) goto getlost; // delta_D in detector frame

      if((fabs(delta_D[1]) > Width_D/2)||(fabs(delta_D[2]) > Height_D/2)) {if(phi_repetition <100000) goto repet; else goto getlost;}	
		
      if(Option==3)fprintf(DetPoints_CAS, "%12.6f  %12.6f\n", delta_D[1], delta_D[2]);

      RotBackVector(RotMatrix_D, delta_D); // in frame of lab

      CopyVector(delta_D, r_D); AddVector(r_D, R_D); // in lab coordinate system
	  }
	
    /* computes vector distance between vertices on analyzer and detector: l_3_vect */ 
    CopyVector(r_D, l_3_vect); SubVector(l_3_vect, r_X); l_3 = LengthVector(l_3_vect);

    /* check first order eq(43) */
    deltaCos2thetaB_1st = - Cos2thetaB_0*(ScalarProduct(L_2_vect_unit, delta_X) - ScalarProduct(L_2_vect_unit, delta_S))/L_2
                          - Cos2thetaB_0*(ScalarProduct(L_3_vect_unit, delta_D) - ScalarProduct(L_3_vect_unit, delta_X))/L_3
                          + (ScalarProduct(L_2_vect_unit, delta_D) - ScalarProduct(L_2_vect_unit, delta_X))/L_3
                          + (ScalarProduct(L_3_vect_unit, delta_X) - ScalarProduct(L_3_vect_unit, delta_S))/L_2 ;

    balance_eq43 = deltaCos2thetaB -  deltaCos2thetaB_1st;

    /* calculate flight time */
    l_total  = l_1 + l_2 + l_3;
    deltal_1 = L_1 - l_1; deltal_2 = L_2 - l_2; deltal_3 = L_3 - l_3; 
    tof      = l_total/V_FROM_LAMBDA(lambda_random);
    deltatof = tof - tof_0;

    /* calculate first order approach of flight time */

    /* deltatof_1st_eq19 = - ScalarProduct(L_1_vect_unit, delta_M)/V_FROM_LAMBDA(lambda_0)
                          + (ScalarProduct(L_1_vect_unit, delta_S) - ScalarProduct(L_2_vect_unit, delta_S))/V_FROM_LAMBDA(lambda_0)
                          + (ScalarProduct(L_2_vect_unit, delta_X) - ScalarProduct(L_3_vect_unit, delta_X))/V_FROM_LAMBDA(lambda_0)
                          +  ScalarProduct(L_3_vect_unit, delta_D)/V_FROM_LAMBDA(lambda_0)
                          - (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))*deltaCos2thetaB_1st/V_FROM_LAMBDA(lambda_0);*/

    delta_M_term = ScalarProduct(F_M, delta_M);  
    delta_S_term = ScalarProduct(F_S, delta_S);  
    delta_X_term = ScalarProduct(F_X, delta_X);  
    delta_D_term = ScalarProduct(F_D, delta_D);  
    deltatof_1st = delta_M_term + delta_S_term + delta_X_term + delta_D_term ;

    /* statistics */
    l_total_2av += sq(l_total)/NoTrajectories;  l_total_av += l_total/NoTrajectories; 
    l_1_2av += sq(l_1)/NoTrajectories;  l_1_av += l_1/NoTrajectories; 
    l_2_2av += sq(l_2)/NoTrajectories;  l_2_av += l_2/NoTrajectories; 
    l_3_2av += sq(l_3)/NoTrajectories;  l_3_av += l_3/NoTrajectories; 

    deltatof_2av += sq(deltatof)/NoTrajectories;  deltatof_av += deltatof/NoTrajectories; 
    deltatof_1st_2av += sq(deltatof_1st)/NoTrajectories;  deltatof_1st_av += deltatof_1st/NoTrajectories; 

    deltalambda_2av += sq(deltalambda)/NoTrajectories;  deltalambda_av += deltalambda/NoTrajectories; 

    delta_M_term_2av += sq(delta_M_term)/NoTrajectories;  delta_M_term_av += delta_M_term/NoTrajectories; 
    delta_S_term_2av += sq(delta_S_term)/NoTrajectories;  delta_S_term_av += delta_S_term/NoTrajectories; 
    delta_X_term_2av += sq(delta_X_term)/NoTrajectories;  delta_X_term_av += delta_X_term/NoTrajectories; 
    delta_D_term_2av += sq(delta_D_term)/NoTrajectories;  delta_D_term_av += delta_D_term/NoTrajectories; 

    NoTrajectoriesEnd += 1;

    /* trajectory data output */
    if((Option==1)||(Option==2)||(Option==3))
    {
      fprintf(TrajData_CAS, "%12.6f   %12.6f   %12.6f   %12.6f   %le   %le   %le   %le   %le   %le   %le\n", l_1+l_2+l_3, l_1, l_2, l_3, deltatof, deltalambda, deltatof_1st, delta_M_term, delta_S_term, delta_X_term, delta_D_term);
    }

    if((Option==2)||(Option==3))
    {		
      if(i<501)
      {
        fprintf(TrajPoints_CAS, "%ld   %le   %le   %le\n", i, r_M[0], r_M[1], r_M[2]);
        fprintf(TrajPoints_CAS, "%ld   %le   %le   %le\n", i, r_S[0], r_S[1], r_S[2]);
        fprintf(TrajPoints_CAS, "%ld   %le   %le   %le\n", i, r_X[0], r_X[1], r_X[2]);
        fprintf(TrajPoints_CAS, "%ld   %le   %le   %le\n", i, r_D[0], r_D[1], r_D[2]);
      }
    }

getlost:;
	}
	OwnCleanup();

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
	print_module_name(" CAS simulation version 4.0") ;
	fprintf(LogFilePtr,"After J.M.Carpenter et al, NIMPR A483 (2002) 784-806.\n") ;
	fprintf(LogFilePtr,"Units: cm, ms, Angstroem, deg if not otherwise given.\n\n") ;

	Option = 1; NoTrajectories = 10000; //default

	/* initialises statistics values */
	NoTrajectoriesEnd = 0;

	delta_M_term_2av=0.; delta_S_term_2av=0.; delta_X_term_2av=0.; delta_D_term_2av=0.; 
	delta_M_term_av =0.; delta_S_term_av =0.; delta_X_term_av =0.; delta_D_term_av =0.; 

	deltatof_av = deltatof_2av = 0.; deltatof_1st_av = deltatof_1st_2av = 0.; deltalambda_av = deltalambda_2av = 0.; 

	l_1_av = l_1_2av = l_2_av = l_2_2av = l_3_av = l_3_2av = l_total_av = l_total_2av = 0.;

  /* read from option string */

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        if((Par_CAS = OpenInputFile(&argv[1][2], FALSE, "rt"))==NULL)
        {
          fprintf(LogFilePtr,"\nParameter file '%s' not found\n",&argv[1][2]);
          exit(0);
        }
        ParameterFileName=FullInName(&argv[1][2]);
        break;

      case 'S':
        StatisticsFileName=FullOutName(&argv[1][2]);
        break;

      case 'T':
        TrajDataFileName=FullOutName(&argv[1][2]);
        break;

      case 't':
        TrajPointsFileName=FullOutName(&argv[1][2]);
        break;

      case 'D':
        DetPointsFileName=FullOutName(&argv[1][2]);
        break;

      case 'O':
        sscanf(&argv[1][2], "%ld", &Option) ; // if 1 writes trajectories into log file
        break;

      case 'n':
        sscanf(&argv[1][2], "%ld", &NoTrajectories) ; // number of trajectories
        break;

      case 'l':
        sscanf(&argv[1][2], "%lf", &lambda) ; // medium wavelength considered
        break;

      case 'w':
        sscanf(&argv[1][2], "%lf", &widthlambda) ; // full wavelength range considered
        break;

      case 'k':
        sscanf(&argv[1][2], "%lf", &zeta) ; // zeta rotation angle around W
        break;
    }
    argc--;
    argv++;
  }
	
  if((Statistics_CAS = fopen(StatisticsFileName,"w"))==NULL)
  {
    fprintf(LogFilePtr,"\nTrajectories file '%s' could not be opened\n",&argv[1][2]);
    exit(0);
  }
  fprintf(Statistics_CAS, "average	SD\n");


  if((Option==1)||(Option==2)||(Option==3))
  {
    if((TrajData_CAS = fopen(TrajDataFileName,"w"))==NULL)
    {
      fprintf(LogFilePtr,"\nTrajectories file '%s' could not be opened\n",&argv[1][2]);
      exit(0);
    }
    fprintf(TrajData_CAS, "ltot		l1		l2		l3		dtof	dlambda		dtof1ord	dMterm	dSterm	dXterm	dDterm\n");
  }
  if((Option==2)||(Option==3))
  {
    if((TrajPoints_CAS = fopen(TrajPointsFileName,"w"))==NULL)
    {
      fprintf(LogFilePtr,"\nTrajectory points file '%s' could not be opened\n",&argv[1][2]);
      exit(0);
    }
    fprintf(TrajPoints_CAS, "No	 rMSXDx	 rMSXDy	 rMSXDz\n");
  }
  if(Option==3)
  {
    if((DetPoints_CAS = fopen(DetPointsFileName,"w"))==NULL)
    {
	    fprintf(LogFilePtr,"\nDetector points file '%s' could not be opened\n",&argv[1][2]);
	    exit(0);
    }
    fprintf(DetPoints_CAS, "deltaDy	 deltaDz\n");
  }

  /* read from parameter file */
  d_spacing=ReadParF(Par_CAS); ReadParComment(Par_CAS);

  R_M[0]=ReadParF(Par_CAS); R_M[1]=ReadParF(Par_CAS); R_M[2]=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  R_S[0]=ReadParF(Par_CAS); R_S[1]=ReadParF(Par_CAS); R_S[2]=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  R_X[0]=ReadParF(Par_CAS); R_X[1]=ReadParF(Par_CAS); R_X[2]=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  R_D[0]=ReadParF(Par_CAS); R_D[1]=ReadParF(Par_CAS); R_D[2]=ReadParF(Par_CAS); ReadParComment(Par_CAS);

  Height_M=ReadParF(Par_CAS); Width_M=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Height_S=ReadParF(Par_CAS); Width_S=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Height_X=ReadParF(Par_CAS); Width_X=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Height_D=ReadParF(Par_CAS); Width_D=ReadParF(Par_CAS); ReadParComment(Par_CAS);

  Euler_M_aroundZ=ReadParF(Par_CAS); Euler_M_aroundY=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Euler_S_aroundZ=ReadParF(Par_CAS); Euler_S_aroundY=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Euler_X_aroundZ=ReadParF(Par_CAS); Euler_X_aroundY=ReadParF(Par_CAS); ReadParComment(Par_CAS);
  Euler_D_aroundZ=ReadParF(Par_CAS); Euler_D_aroundY=ReadParF(Par_CAS); ReadParComment(Par_CAS);

  /* computes frame rotation matrices for moderator, sample, analyzer and detector orientation */
  Euler_M_aroundY *= M_PI/180.; Euler_M_aroundZ *= M_PI/180.; 
  Euler_S_aroundY *= M_PI/180.; Euler_S_aroundZ *= M_PI/180.; 
  Euler_X_aroundY *= M_PI/180.; Euler_X_aroundZ *= M_PI/180.; 
  Euler_D_aroundY *= M_PI/180.; Euler_D_aroundZ *= M_PI/180.; 

  FillRotMatrixZY(RotMatrix_M, Euler_M_aroundY, Euler_M_aroundZ);
  FillRotMatrixZY(RotMatrix_S, Euler_S_aroundY, Euler_S_aroundZ);
  FillRotMatrixZY(RotMatrix_X, Euler_X_aroundY, Euler_X_aroundZ);
  FillRotMatrixZY(RotMatrix_D, Euler_D_aroundY, Euler_D_aroundZ);

  /* compute other values */

  /* checks lambda */
  if((lambda - widthlambda/2.) > 2*d_spacing) {printf("ERROR: wavelength range incompatible with d_spacing\n\n"); exit(0);}
		
  fprintf(LogFilePtr, "\n*** INPUT: ***\n"
                      "\n* No. trajectories, main wavelength, wavelength band, zeta:\n\n"
                      "trajectories: %12ld\n"
                      "lambda:       %12.6f\n"
                      "widthlambda:  %12.6f\n"
                      "zeta:         %12.6f  (if |zeta|>0 : see output VI.)\n"
                      "\n* R_M, R_S, R_X, R_D, sizes, orientations:\n\n"
                      "parameter file: %s  (see below)\n"
                      "\n",
  NoTrajectories, lambda, widthlambda, zeta, ParameterFileName);

  /* computes and prints L_1_vect, L_2_vect, L_3_vect */
  CopyVector(R_S, L_1_vect); SubVector(L_1_vect, R_M); L_1 = LengthVector(L_1_vect);
  CopyVector(R_X, L_2_vect); SubVector(L_2_vect, R_S); L_2 = LengthVector(L_2_vect);/**/
  CopyVector(R_D, L_3_vect); SubVector(L_3_vect, R_X); L_3 = LengthVector(L_3_vect);/**/
  fprintf(LogFilePtr, "\n*** OUTPUT: ***\n"
                      "\n* I. CAS distance vectors:\n\n"
                      "L_1_vect:     %12.6f   %12.6f   %12.6f      L_1 = %12.6f \n" 
                      "L_2_vect:     %12.6f   %12.6f   %12.6f      L_2 = %12.6f \n"
                      "L_3_vect:     %12.6f   %12.6f   %12.6f      L_3 = %12.6f \n",			
  L_1_vect[0], L_1_vect[1], L_1_vect[2], L_1, L_2_vect[0], L_2_vect[1], L_2_vect[2], L_2, L_3_vect[0], L_3_vect[1], L_3_vect[2], L_3);

  /* computes and prints L_1_vect_unit, L_2_vect_unit, L_3_vect_unit */
  CopyVector(L_1_vect, L_1_vect_unit); MultiplyByScalar(L_1_vect_unit, 1./L_1);
  CartesianToEulerZY(L_1_vect_unit, &Euler_L1_aroundY, &Euler_L1_aroundZ);
  CopyVector(L_2_vect, L_2_vect_unit); MultiplyByScalar(L_2_vect_unit, 1./L_2);
  CartesianToEulerZY(L_2_vect_unit, &Euler_L2_aroundY, &Euler_L2_aroundZ);
  CopyVector(L_3_vect, L_3_vect_unit); MultiplyByScalar(L_3_vect_unit, 1./L_3);
  CartesianToEulerZY(L_3_vect_unit, &Euler_L3_aroundY, &Euler_L3_aroundZ);

  fprintf(LogFilePtr, "\nL_1_vect_unit:%12.6f   %12.6f   %12.6f \n" 
                      "L_2_vect_unit:%12.6f   %12.6f   %12.6f \n"
                      "L_3_vect_unit:%12.6f   %12.6f   %12.6f \n",			
  L_1_vect_unit[0], L_1_vect_unit[1], L_1_vect_unit[2], L_2_vect_unit[0], L_2_vect_unit[1], L_2_vect_unit[2], L_3_vect_unit[0], L_3_vect_unit[1], L_3_vect_unit[2]);

  fprintf(LogFilePtr, "\nEuler_L1_aroundZ, Euler_L1_aroundY:   %12.6f   %12.6f\n" 
                      "Euler_L2_aroundZ, Euler_L2_aroundY:   %12.6f   %12.6f\n"
                      "Euler_L3_aroundZ, Euler_L3_aroundY:   %12.6f   %12.6f\n",			
  180/M_PI*Euler_L1_aroundZ, 180/M_PI*Euler_L1_aroundY, 180/M_PI*Euler_L2_aroundZ, 180/M_PI*Euler_L2_aroundY, 180/M_PI*Euler_L3_aroundZ, 180/M_PI*Euler_L3_aroundY);
		
  /* computes and prints Phi, thetaB_0, thetaB, deltathetaB, deltaCos2thetaB */
  CosPhi = ScalarProduct(L_1_vect_unit, L_2_vect_unit); Phi = acos(CosPhi); 
  Cos2thetaB_0 = ScalarProduct(L_2_vect_unit, L_3_vect_unit); thetaB_0 = acos(Cos2thetaB_0)/2.;

	/* computes nominal wavelength and flight time (dominant term) */
	lambda_0 = 2.*d_spacing*(double) sin(thetaB_0);
	tof_0 = (L_1 + L_2 + L_3)/V_FROM_LAMBDA(lambda_0);

	/* computes F vectors according to the 1st order theory for standard deviations */
	{VectorType F_1, F_2, F_3;

	CopyVector(L_1_vect_unit, F_M);
	MultiplyByScalar(F_M, -1.);

	CopyVector(L_1_vect_unit, F_1); CopyVector(L_2_vect_unit, F_2); CopyVector(L_3_vect_unit, F_3);

	MultiplyByScalar(F_1, 1.);
	MultiplyByScalar(F_2, -1. - (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))*Cos2thetaB_0/L_2);
	MultiplyByScalar(F_3, (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))/L_2);

	AddVector(F_2, F_3); AddVector(F_1, F_2); CopyVector(F_1, F_S); 

	CopyVector(L_2_vect_unit, F_2); CopyVector(L_3_vect_unit, F_3);

  MultiplyByScalar(F_2, 1. + (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))*(Cos2thetaB_0/L_2 + 1./L_3));
	MultiplyByScalar(F_3, -1. - (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))*(Cos2thetaB_0/L_3 + 1./L_2));

	AddVector(F_2, F_3); CopyVector(F_2, F_X); 


	CopyVector(L_2_vect_unit, F_2); CopyVector(L_3_vect_unit, F_3);

	MultiplyByScalar(F_2, - (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))/L_3);
	MultiplyByScalar(F_3, 1. + (L_1 + L_2 + L_3)/4./sq((double) sin(thetaB_0))*Cos2thetaB_0/L_3);

	AddVector(F_2, F_3); CopyVector(F_2, F_D); 


	MultiplyByScalar(F_M, 1./V_FROM_LAMBDA(lambda_0)); F_M_abs = LengthVector(F_M); CopyVector(F_M, F_M_unit); if(F_M_abs !=0.) MultiplyByScalar(F_M_unit, 1./F_M_abs); 
	MultiplyByScalar(F_S, 1./V_FROM_LAMBDA(lambda_0)); F_S_abs = LengthVector(F_S); CopyVector(F_S, F_S_unit); if(F_S_abs !=0.) MultiplyByScalar(F_S_unit, 1./F_S_abs); 
	MultiplyByScalar(F_X, 1./V_FROM_LAMBDA(lambda_0)); F_X_abs = LengthVector(F_X); CopyVector(F_X, F_X_unit); if(F_X_abs !=0.) MultiplyByScalar(F_X_unit, 1./F_X_abs); 
	MultiplyByScalar(F_D, 1./V_FROM_LAMBDA(lambda_0)); F_D_abs = LengthVector(F_D); CopyVector(F_D, F_D_unit); if(F_D_abs !=0.) MultiplyByScalar(F_D_unit, 1./F_D_abs); 
	}

	EulerToCartesianZY(n_M, &Euler_M_aroundY, &Euler_M_aroundZ);
	EulerToCartesianZY(n_S, &Euler_S_aroundY, &Euler_S_aroundZ);
	EulerToCartesianZY(n_X, &Euler_X_aroundY, &Euler_X_aroundZ);
	EulerToCartesianZY(n_D, &Euler_D_aroundY, &Euler_D_aroundZ);


	xi_M = acos((ScalarProduct(n_M, F_M_unit))); //this rounds up the angles
	xi_S = acos((ScalarProduct(n_S, F_S_unit))); 
	xi_X = acos((ScalarProduct(n_X, F_X_unit))); 
	xi_D = acos((ScalarProduct(n_D, F_D_unit)));  


	VectorProduct(n_M, F_M_unit, P_M); P_M_abs = LengthVector(P_M); CopyVector(P_M, P_M_unit); if(P_M_abs !=0.) MultiplyByScalar(P_M_unit, 1./P_M_abs);
	VectorProduct(n_S, F_S_unit, P_S); P_S_abs = LengthVector(P_S); CopyVector(P_S, P_S_unit); if(P_S_abs !=0.) MultiplyByScalar(P_S_unit, 1./P_S_abs);
	VectorProduct(n_X, F_X_unit, P_X); P_X_abs = LengthVector(P_X); CopyVector(P_X, P_X_unit); if(P_X_abs !=0.) MultiplyByScalar(P_X_unit, 1./P_X_abs);
	VectorProduct(n_D, F_D_unit, P_D); P_D_abs = LengthVector(P_D); CopyVector(P_D, P_D_unit); if(P_D_abs !=0.) MultiplyByScalar(P_D_unit, 1./P_D_abs);

	VectorProduct(n_M, P_M, S_M); S_M_abs = LengthVector(S_M); CopyVector(S_M, S_M_unit); if(S_M_abs !=0.) MultiplyByScalar(S_M_unit, 1./S_M_abs);
	VectorProduct(n_S, P_S, S_S); S_S_abs = LengthVector(S_S); CopyVector(S_S, S_S_unit); if(S_S_abs !=0.) MultiplyByScalar(S_S_unit, 1./S_S_abs);
	VectorProduct(n_X, P_X, S_X); S_X_abs = LengthVector(S_X); CopyVector(S_X, S_X_unit); if(S_X_abs !=0.) MultiplyByScalar(S_X_unit, 1./S_X_abs);
	VectorProduct(n_D, P_D, S_D); S_D_abs = LengthVector(S_D); CopyVector(S_D, S_D_unit); if(S_D_abs !=0.) MultiplyByScalar(S_D_unit, 1./S_D_abs);

	/* calculates 1st order standard deviations */
	RotVector(RotMatrix_M, S_M); 
	area_moment_M = sq(S_M[1])*Height_M*pow(Width_M, 3.)/12. + sq(S_M[2])*Width_M*pow(Height_M, 3.)/12.;
	sigma2_M_th = sq(F_M_abs)*area_moment_M/Width_M/Height_M ;

	RotVector(RotMatrix_S, S_S); 
	area_moment_S = sq(S_S[1])*Height_S*pow(Width_S, 3.)/12. + sq(S_S[2])*Width_S*pow(Height_S, 3.)/12.;
	sigma2_S_th = sq(F_S_abs)*area_moment_S/Width_S/Height_S ;

	RotVector(RotMatrix_X, S_X); 
	area_moment_X = sq(S_X[1])*Height_X*pow(Width_X, 3.)/12. + sq(S_X[2])*Width_X*pow(Height_X, 3.)/12.;
	sigma2_X_th = sq(F_X_abs)*area_moment_X/Width_X/Height_X ;

	RotVector(RotMatrix_D, S_D); 
	area_moment_D = sq(S_D[1])*Height_D*pow(Width_D, 3.)/12. + sq(S_D[2])*Width_D*pow(Height_D, 3.)/12.;
	sigma2_D_th = sq(F_D_abs)*area_moment_D/Width_D/Height_D ;

	/* computes focusing conditions from first order theory */
	CopyVector(L_2_vect_unit, U_vect);
	MultiplyByScalar(U_vect, Cos2thetaB_0);
	SubVector(U_vect, L_3_vect_unit);
	U_abs = LengthVector(U_vect);

	CopyVector(L_2_vect_unit, V_vect);
	MultiplyByScalar(V_vect, 4.*L_2*sq((double) sin(thetaB_0))/(L_1 + L_2 + L_3));
	V_abs = LengthVector(V_vect);

	CopyVector(U_vect, W_vect);
	AddVector(W_vect, V_vect);
	W_abs = LengthVector(W_vect);

	psi = atan(U_abs/V_abs);
	chi = AngleVectors(L_1_vect_unit, W_vect);
	alpha = (double) sin(2.*thetaB_0)/tan(psi);

	CopyVector(L_1_vect_unit, Z_vect);
	MultiplyByScalar(Z_vect, alpha);
	SubVector(Z_vect, W_vect);

	Z_abs = LengthVector(Z_vect);

	Phi_M_foc = 0.;
	Phi_D_foc = atan((L_1 + L_2 + L_3)/2./L_3/tan(thetaB_0));
	Phi_X_foc = atan((L_2 - L_3)*(L_1 + L_2 + L_3)/tan(thetaB_0)/(4.*L_2*L_3 +(L_2 + L_3)*(L_1 + L_2 + L_3)/sq(tan(thetaB_0))));
	Phi_S_foc = atan(1./(cos(psi)/(double) sin(chi) - 1./tan(chi)));
		
	CopyVector(L_1_vect_unit, Normal_M_foc);
	CartesianToEulerZY(Normal_M_foc, &Euler_M_aroundY_foc, &Euler_M_aroundZ_foc);

	CopyVector(L_3_vect_unit, Normal_D_foc);
	MultiplyByScalar(Normal_D_foc, ((double) sin(2.*thetaB_0)/tan(Phi_D_foc) + Cos2thetaB_0));
	SubVector(Normal_D_foc, L_2_vect_unit);
	MultiplyByScalar(Normal_D_foc, (double) sin(Phi_D_foc)/(double) sin(2.*thetaB_0));
	CartesianToEulerZY(Normal_D_foc, &Euler_D_aroundY_foc, &Euler_D_aroundZ_foc);

	CopyVector(L_3_vect_unit, Normal_X_foc);
	MultiplyByScalar(Normal_X_foc, ((double) sin(2.*thetaB_0)/tan(Phi_X_foc + M_PI/2. - thetaB_0) + Cos2thetaB_0));
	SubVector(Normal_X_foc, L_2_vect_unit);
	MultiplyByScalar(Normal_X_foc, (double) sin(Phi_X_foc + M_PI/2. - thetaB_0)/(double) sin(2.*thetaB_0)); 
	MultiplyByScalar(Normal_X_foc, -1.); // multiply by -1 for VITESS definition!
	CartesianToEulerZY(Normal_X_foc, &Euler_X_aroundY_foc, &Euler_X_aroundZ_foc);

	CopyVector(Z_vect, Normal_S_foc);
	MultiplyByScalar(Normal_S_foc, 1./Z_abs);
	CartesianToEulerZY(Normal_S_foc, &Euler_S_aroundY_foc, &Euler_S_aroundZ_foc);

	CopyVector(L_3_vect_unit, q_B_0_vect);
	MultiplyByScalar(q_B_0_vect, ((double) sin(2.*thetaB_0)/tan(M_PI/2. - thetaB_0) + Cos2thetaB_0));
	SubVector(q_B_0_vect, L_2_vect_unit);
	MultiplyByScalar(q_B_0_vect, (double) sin(M_PI/2. - thetaB_0)/(double) sin(2.*thetaB_0)); 
	MultiplyByScalar(q_B_0_vect, -1.); // multiply by -1 for VITESS definition!
	CartesianToEulerZY(q_B_0_vect, &Euler_q_B_0_aroundY, &Euler_q_B_0_aroundZ);
}/* end initialisation */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  /* statistics */
  l_total_SD = sqrt(l_total_2av - sq(l_total_av));

  l_1_SD = sqrt(l_1_2av - sq(l_1_av));
  l_2_SD = sqrt(l_2_2av - sq(l_2_av));
  l_3_SD = sqrt(l_3_2av - sq(l_3_av));

  deltatof_SD     = sqrt(deltatof_2av - sq(deltatof_av));
  deltalambda_SD  = sqrt(deltalambda_2av - sq(deltalambda_av));
  deltatof_1st_SD = sqrt(deltatof_1st_2av - sq(deltatof_1st_av));
  delta_M_term_SD = sqrt(delta_M_term_2av - sq(delta_M_term_av));
  delta_S_term_SD = sqrt(delta_S_term_2av - sq(delta_S_term_av));
  delta_X_term_SD = sqrt(delta_X_term_2av - sq(delta_X_term_av));
  delta_D_term_SD = sqrt(delta_D_term_2av - sq(delta_D_term_av));

  printf(	"\nOutput: %s \n"
          "No.Trajectories:    %ld\n"
          "No.TrajectoriesEnd: %ld\n",  LogFileName, NoTrajectories, NoTrajectoriesEnd);

  /* prints values */
  fprintf(Statistics_CAS, "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"

                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"

                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"

                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n"
                          "%12.6e	%12.6e\n",
                          l_total_av, l_total_SD, l_1_av, l_1_SD, l_2_av, l_2_SD, l_3_av, l_3_SD, 
                          deltatof_av, deltatof_SD, deltalambda_av, deltalambda_SD, deltatof_1st_av, deltatof_1st_SD, 
                          delta_M_term_av, delta_M_term_SD, delta_S_term_av, delta_S_term_SD, delta_X_term_av, delta_X_term_SD, delta_D_term_av, delta_D_term_SD, delta_M_term_av + delta_S_term_av + delta_X_term_av + delta_D_term_av, sqrt(sq(delta_M_term_SD) + sq(delta_S_term_SD) + sq(delta_X_term_SD) + sq(delta_D_term_SD)), 
                          0., (double) sqrt(sigma2_M_th), 0., (double) sqrt(sigma2_S_th), 0., (double) sqrt(sigma2_X_th), 0., (double) sqrt(sigma2_D_th), 0., sqrt(sigma2_M_th + sigma2_S_th + sigma2_X_th + sigma2_D_th)) ;

  printf("\nSD_MC: %12.6e   SD_1st_MC: %12.6e   SD_1st_th: %12.6e \n\n", deltatof_SD, sqrt(sq(delta_M_term_SD) + sq(delta_S_term_SD) + sq(delta_X_term_SD) + sq(delta_D_term_SD)), sqrt(sigma2_M_th + sigma2_S_th + sigma2_X_th + sigma2_D_th)) ;


  fprintf(LogFilePtr, "\n\n* II. Main scattering angle, Bragg angle, wavelength, tof:\n\n"
                      "Phi:          %12.6f\n"
                      "thetaB_0:     %12.6f\n"
                      "lambda_0:     %12.6f\n"
                      "tof_0:        %12.6f\n\n",  180/M_PI*Phi, 180/M_PI*thetaB_0, lambda_0, tof_0);

  fprintf(LogFilePtr, "\n* III. Analytic first order delta-square terms:\n\n"
                      "n_M:               %12.6f   %12.6f   %12.6f\n" 
                      "n_S:               %12.6f   %12.6f   %12.6f\n"
                      "n_X:               %12.6f   %12.6f   %12.6f\n"			
                      "n_D:               %12.6f   %12.6f   %12.6f\n\n"
                      "xi_M:              %12.6f\n"
                      "xi_S:              %12.6f\n"
                      "xi_X:              %12.6f\n"
                      "xi_D:              %12.6f\n\n"
                      "area_moment_M:     %12.6e\n"
                      "area_moment_S:     %12.6e\n"
                      "area_moment_X:     %12.6e\n"
                      "area_moment_D:     %12.6e\n\n"
                      "F_M_unit:          %12.6f   %12.6f   %12.6f      F_M_abs = %12.6e \n" 
                      "F_S_unit:          %12.6f   %12.6f   %12.6f      F_S_abs = %12.6e \n"
                      "F_X_unit:          %12.6f   %12.6f   %12.6f      F_X_abs = %12.6e \n"			
                      "F_D_unit:          %12.6f   %12.6f   %12.6f      F_D_abs = %12.6e \n\n"
                      "P_M_unit:          %12.6f   %12.6f   %12.6f      P_M_abs = %12.6e \n" 
                      "P_S_unit:          %12.6f   %12.6f   %12.6f      P_S_abs = %12.6e \n"
                      "P_X_unit:          %12.6f   %12.6f   %12.6f      P_X_abs = %12.6e \n"			
                      "P_D_unit:          %12.6f   %12.6f   %12.6f      P_D_abs = %12.6e \n\n"
                      "S_M_unit:          %12.6f   %12.6f   %12.6f      S_M_abs = %12.6e \n" 
                      "S_S_unit:          %12.6f   %12.6f   %12.6f      S_S_abs = %12.6e \n"
                      "S_X_unit:          %12.6f   %12.6f   %12.6f      S_X_abs = %12.6e \n"			
                      "S_D_unit:          %12.6f   %12.6f   %12.6f      S_D_abs = %12.6e \n\n"
                      "sigma_M_th:        %12.6e\n"
                      "sigma_S_th:        %12.6e\n"
                      "sigma_X_th:        %12.6e\n"
                      "sigma_D_th:        %12.6e\n\n",
                      n_M[0], n_M[1], n_M[2], n_S[0], n_S[1], n_S[2], n_X[0], n_X[1], n_X[2], n_D[0], n_D[1], n_D[2], 
                      180./M_PI*xi_M, 180./M_PI*xi_S, 180./M_PI*xi_X, 180./M_PI*xi_D, 
                      area_moment_M, area_moment_S, area_moment_X, area_moment_D, 
                      F_M_unit[0], F_M_unit[1], F_M_unit[2], F_M_abs, F_S_unit[0], F_S_unit[1], F_S_unit[2], F_S_abs, F_X_unit[0], F_X_unit[1], F_X_unit[2], F_X_abs, F_D_unit[0], F_D_unit[1], F_D_unit[2], F_D_abs, 
                      P_M_unit[0], P_M_unit[1], P_M_unit[2], P_M_abs, P_S_unit[0], P_S_unit[1], P_S_unit[2], P_S_abs, P_X_unit[0], P_X_unit[1], P_X_unit[2], P_X_abs, P_D_unit[0], P_D_unit[1], P_D_unit[2], P_D_abs, 
                      S_M_unit[0], S_M_unit[1], S_M_unit[2], S_M_abs, S_S_unit[0], S_S_unit[1], S_S_unit[2], S_S_abs, S_X_unit[0], S_X_unit[1], S_X_unit[2], S_X_abs, S_D_unit[0], S_D_unit[1], S_D_unit[2], S_D_abs, 
                      (double) sqrt(sigma2_M_th), (double) sqrt(sigma2_S_th), (double) sqrt(sigma2_X_th), (double) sqrt(sigma2_D_th));

  fprintf(LogFilePtr, "\n* IV. Variables and the focusing conditions:\n\n"
                      "U_vect:       %12.6f   %12.6f   %12.6f      U_abs = %12.6f \n" 
                      "V_vect:       %12.6f   %12.6f   %12.6f      V_abs = %12.6f \n"
                      "W_vect:       %12.6f   %12.6f   %12.6f      W_abs = %12.6f \n"			
                      "Z_vect:       %12.6f   %12.6f   %12.6f      Z_abs = %12.6f \n",			
                      U_vect[0], U_vect[1], U_vect[2], U_abs, V_vect[0], V_vect[1], V_vect[2], V_abs, W_vect[0], W_vect[1], W_vect[2], W_abs, Z_vect[0], Z_vect[1], Z_vect[2], Z_abs);

  fprintf(LogFilePtr, "\npsi:          %12.6f rad\n"
                      "chi:          %12.6f\n"
                      "alpha:        %12.6f [-]\n\n"
                      "Phi_M_foc:    %12.6f\n"
                      "Phi_S_foc:    %12.6f\n"
                      "Phi_X_foc:    %12.6f\n"
                      "Phi_D_foc:    %12.6f\n",   psi, chi, alpha, 180/M_PI*Phi_M_foc, 180/M_PI*Phi_S_foc, 180/M_PI*Phi_X_foc, 180/M_PI*Phi_D_foc);

  fprintf(LogFilePtr, "\nNormal_M_foc: %12.6f   %12.6f   %12.6f\n" 
                      "Normal_S_foc: %12.6f   %12.6f   %12.6f\n"
                      "Normal_X_foc: %12.6f   %12.6f   %12.6f   (paper: *(-1))\n"			
                      "Normal_D_foc: %12.6f   %12.6f   %12.6f\n"			
                      "\nq_B_0_vect  : %12.6f   %12.6f   %12.6f   (paper: *(-1))\n",			
                      Normal_M_foc[0], Normal_M_foc[1], Normal_M_foc[2], Normal_S_foc[0], Normal_S_foc[1], Normal_S_foc[2], Normal_X_foc[0], Normal_X_foc[1], Normal_X_foc[2], Normal_D_foc[0], Normal_D_foc[1], Normal_D_foc[2], q_B_0_vect[0], q_B_0_vect[1], q_B_0_vect[2]);


  fprintf(LogFilePtr, "\nEuler_M_aroundZ_foc, Euler_M_aroundY_foc:   %12.6f   %12.6f\n" 
  "Euler_S_aroundZ_foc, Euler_S_aroundY_foc:   %12.6f   %12.6f\n"
  "Euler_X_aroundZ_foc, Euler_X_aroundY_foc:   %12.6f   %12.6f\n"			
  "Euler_D_aroundZ_foc, Euler_D_aroundY_foc:   %12.6f   %12.6f\n"			
  "\nEuler_q_B_0_aroundZ, Euler_q_B_0_aroundY:   %12.6f   %12.6f\n",			
  180/M_PI*Euler_M_aroundZ_foc, 180/M_PI*Euler_M_aroundY_foc, 180/M_PI*Euler_S_aroundZ_foc, 180/M_PI*Euler_S_aroundY_foc, 180/M_PI*Euler_X_aroundZ_foc, 180/M_PI*Euler_X_aroundY_foc, 180/M_PI*Euler_D_aroundZ_foc, 180/M_PI*Euler_D_aroundY_foc, 180/M_PI*Euler_q_B_0_aroundZ, 180/M_PI*Euler_q_B_0_aroundY);

  if(Option==1) fprintf(LogFilePtr, "\n\n* V. Output files :\n\n"
                                    "statistics data:    %s\n"
                                    "trajectory data:    %s\n\n",    StatisticsFileName, TrajDataFileName);

  if(Option==2) fprintf(LogFilePtr, "\n\n* V. Output files:\n\n"
                                    "statistics data:    %s\n"
                                    "trajectory data:    %s\n"
                                    "trajectory points:  %s\n\n",  StatisticsFileName, TrajDataFileName, TrajPointsFileName);

  if(Option==3) fprintf(LogFilePtr, "\n\n* V. Output files:\n\n"
                                    "statistics data:    %s\n"
                                    "trajectory data:    %s\n"
                                    "trajectory points:  %s\n"
                                    "detector points:    %s\n\n",  StatisticsFileName, TrajDataFileName, TrajPointsFileName, DetPointsFileName);

  if((Option!=1)&&(Option!=2)&&(Option!=3)) fprintf(LogFilePtr, "\n\n* V. Output file:\n\n"
                                                                "statistics data:    %s\n\n", StatisticsFileName);

  fprintf(LogFilePtr, "\nSD_MC: %12.6e   SD_1st_MC(sum): %12.6e   SD_1st_th(sum): %12.6e \n\n", deltatof_SD, sqrt(sq(delta_M_term_SD) + sq(delta_S_term_SD) + sq(delta_X_term_SD) + sq(delta_D_term_SD)), sqrt(sigma2_M_th + sigma2_S_th + sigma2_X_th + sigma2_D_th)) ;
		
  /* here proposed new R_X and R_D after rotation around W_vect by angle zeta  */
  if(zeta!=0)
  {
    CopyVector(R_X, R_X_zeta);
    CopyVector(R_D, R_D_zeta);

    SubVector(R_X_zeta, R_S);
    SubVector(R_D_zeta, R_S);

    CartesianToEulerZY(W_vect, &Euler_W_aroundY, &Euler_W_aroundZ);

    FillRotMatrixZY(RotMatrix_W, Euler_W_aroundY, Euler_W_aroundZ);
    FillRotMatrixX(RotMatrix_zeta, M_PI/180.*zeta);

    RotVector(RotMatrix_W, R_X_zeta);
    RotVector(RotMatrix_W, R_D_zeta);

    RotVector(RotMatrix_zeta, R_X_zeta);
    RotVector(RotMatrix_zeta, R_D_zeta);

    RotBackVector(RotMatrix_W, R_X_zeta); Phi_zeta = AngleVectors(L_1_vect_unit, R_X_zeta);
    RotBackVector(RotMatrix_W, R_D_zeta);

    AddVector(R_X_zeta, R_S);
    AddVector(R_D_zeta, R_S);

    fprintf(LogFilePtr, "\n* VI. Analyzer arm rotated by zeta around vector W_vect:\n\n"
    "R_X_zeta:    %12.6f   %12.6f   %12.6f\n" 
    "R_D_zeta:    %12.6f   %12.6f   %12.6f\n"
    "Phi_zeta:    %12.6f\n",
    R_X_zeta[0], R_X_zeta[1], R_X_zeta[2], R_D_zeta[0], R_D_zeta[1], R_D_zeta[2], Phi_zeta);
  }

  fclose(Statistics_CAS);
	
  if(Option==1)
  {
    fclose(TrajData_CAS);
  }
  if(Option==2)
  {
    fclose(TrajData_CAS);
    fclose(TrajPoints_CAS);
  }
  if(Option==3)
  {
    fclose(TrajData_CAS);
    fclose(TrajPoints_CAS);
    fclose(DetPoints_CAS);
  }

  fprintf(LogFilePtr, "\n\n*** Parameter file %s ***\n\n", ParameterFileName);

  fclose(LogFilePtr);

  CopyFileIntoFile(ParameterFileName, LogFileName);
		
  ShowDate(LogFileName);
  ShowTime(LogFileName);
}


/********************************************/
/* calculates vector product of two vectors */
/********************************************/
void VectorProduct(VectorType v1, VectorType v2, VectorType result)
{
  result[0] = v1[1]*v2[2] - v1[2]*v2[1] ;
  result[1] = v1[2]*v2[0] - v1[0]*v2[2] ;
  result[2] = v1[0]*v2[1] - v1[1]*v2[0] ;
  
  return;
}


/******************************************/
/* writes a string into a file not opened */
/******************************************/
void CopyFileIntoFile(char *filename1, char *filename2)
{
  char s[STRING_BUFFER] ;

  strcpy(s, "type ") ;
  strcat(s, filename1) ;
  strcat(s, " >> ") ;
  strcat(s, filename2) ;
  system(s) ;
}


/******************************************/
/* writes text into a file not opened */
/******************************************/
void EchoSomething(char *filename, char stringvar[STRING_BUFFER])
{
  char s[STRING_BUFFER] ;

  strcpy(s, "echo ") ;
  strcat(s, stringvar) ;
  strcat(s, " >> ") ;
  strcat(s, filename) ;
  system(s) ;
}


/**************************************/
/* writes time into a file not opened */
/**************************************/
void ShowTime(char *filename)
{
  char s[STRING_BUFFER] ;

  strcpy(s, "time/T >>") ;
  strcat(s, filename) ;
  system(s) ;
}


/**************************************/
/* writes date into a file not opened */
/**************************************/
void ShowDate(char *filename)
{
  char s[STRING_BUFFER] ;

  strcpy(s, "date/T >>") ;
  strcat(s, filename) ;
  system(s) ;
}
