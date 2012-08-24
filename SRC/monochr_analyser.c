/* The free non-commercial use of these routines is granted */
/* providing due credit is given to the authors.            */
/* Author: Géza Zsigmond, last change JUL 2002              */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "monochr_analyser.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"


int main(int argc, char **argv)
{
  /* Initialize the program according to the parameters given   */

  Init(argc, argv, VT_MONOC_ANALY);

  OwnInit(argc, argv);

  /* Get the neutrons from the file */
  DECLARE_ABORT;

  while (ReadNeutrons())  {
    int i, ii;
    double startTime;
    VectorType startPosition;
    VectorType startVector;

    CHECK;
    for (i=0; i<NumNeutGot; i++) {
      CHECK;

      InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

      /* selects CE on which the neutron is reflected and gives global variables
	 in	the frame of CE */

      startTime = InputNeutrons[i].Time;

      for (ii = 0; ii < 3; ii++) {
	startVector[ii] = InputNeutrons[i].Vector[ii];
	startPosition[ii] = InputNeutrons[i].Position[ii];
      }

      SelectCE(&Index, i) ;

      if (Index == 0./* no CE was found */) goto getlost ;

      /* moment of arriving at the crystal plane, new position */


      InputNeutrons[i].Time -= InputNeutrons[i].Position[0]  / fabs(InputNeutrons[i].Vector[0]) /
	V_FROM_LAMBDA(InputNeutrons[i].Wavelength);

      CopyVector(InputNeutrons[i].Vector, Path) ;

      MultiplyByScalar(Path, - InputNeutrons[i].Position[0]/ InputNeutrons[i].Vector[0] ) ;

      AddVector(InputNeutrons[i].Position, Path) ; /* Path = displacement vector */

      /* for flat option (Option =1) computes neutron direction in the "Bragg" frame keeping frame of CE for the position */

      RotBackVector(RotMatrixCE, InputNeutrons[i].Vector) ;

      RotVector(RotMatrixBragg, InputNeutrons[i].Vector) ;

      for(repet=0;repet<Repetition;repet++) {
        CHECK;
        TOF = InputNeutrons[i].Time ;

        WL = InputNeutrons[i].Wavelength ;

        Prob = InputNeutrons[i].Probability / Repetition ;

        CopyVector(InputNeutrons[i].Position, Pos) ;

        CopyVector(InputNeutrons[i].Vector, Dir) ;


        /* start here if no mosaic */
        if ((mosaic_fwhm[0]*mosaic_fwhm[1])==0.){
	
          double theta_Bragg, phi_Bragg, theta_refl, phi_refl, d_sp ;

          CartesianToSpherical(Dir, &theta_Bragg, &phi_Bragg);

          theta_refl = M_PI - theta_Bragg ;

          phi_refl = phi_Bragg ;

          SphericalToCartesian(Dir, &theta_refl, &phi_refl) ;

          d_sp = WL * OrderReflection / 2 / (double) cos(theta_Bragg) ;

          /* probability of reflection */

          Prob *= Reflectivity ;

          if(d_spr_option == 1) Prob *= dSpreadLorentzian(d_sp) ;
          if(d_spr_option == 2) Prob *= dSpreadGaussian(d_sp) ;

          if(Prob <= wei_min) goto getlost2 ;

          IntegralIntensity += Prob ;

          /** end here if no mosaic **/}

        /** start here if mosaic **/
        if ((mosaic_fwhm[0]*mosaic_fwhm[1])!=0.) 	{

          /* random d-spacing */

          d_ran_min = d_spacing - d_range/2. * d_fwhm ;

          d_ran_max = d_spacing + d_range/2. * d_fwhm ;

          d_ran = MonteCarlo(d_ran_min, d_ran_max) ;

          arg = WL * OrderReflection / 2. / d_ran ;

          if (arg > 1.) goto getlost2 ;/* wavelength to large */

          /* computes reflection angle corresponding to d_ran */

          thr = (double) acos(arg) ;

          /* computes mosaic orientation of maximum probability in "Bragg" frame which is the
             frame obtained when rotating the original frame until diffraction plane normal vector gets parallel
             to X axis (normal vector of Bragg planes showing in flight direction) */

          MosaicMaxProb(Dir, &thr, Mosaic) ;

          /* ... in frame of neutron which is the frame obtained when rotating the original frame
             until wave vector gets parallel to X axis*/

          RotMatrixX(Dir, Matrix) ;

          RotVector(Matrix, Mosaic) ;

          CartesianToSpherical(Mosaic, &thrmax, &phrmax) ;

          /*	computes random mosaic normal in the frame of neutron close to the maximum probability */

          deltaphr = mosaic_range * Min(mosaic_fwhm[0], mosaic_fwhm[1]) ;

          if(deltaphr > M_PI) deltaphr = M_PI ;

          phr = MonteCarlo(phrmax - deltaphr, phrmax + deltaphr) ;

          SphericalToCartesian(Mosaic, &thr, &phr) ;

          /* computes mosaic normal in the "Bragg" frame */

          RotMatrixX(Dir, Matrix) ;

          RotBackVector(Matrix, Mosaic) ;

          /* probability of reflection from mosaic */

          Prob *= Reflectivity ;

          Prob *= Mosaicity(Mosaic) ;

          if(d_spr_option == 1) Prob *= dSpreadLorentzian(d_ran) ;
          if(d_spr_option == 2) Prob *= dSpreadGaussian(d_ran) ;

          if(Prob <= wei_min && mode==1) goto getlost2 ;

          IntegralIntensity += Prob ;

          /*	 computes reflected direction in the frame of neutron */

          thr = M_PI - 2 * thr ;
          phr = M_PI + phr ;

          SphericalToCartesian(Dir, &thr, &phr) ;

          /* computes reflected direction in the "Bragg"(Option=1) or CE frame (Option !=1)*/

          RotBackVector(Matrix, Dir) ;
	
          /** end here if mosaic **/
        }

        if (mode == 2) {
          InputNeutrons[i].Probability -= Prob;
          InputNeutrons[i].Time = startTime;
          ii = 0;
          for (ii = 0; ii < 3; ii++) {
            InputNeutrons[i].Vector[ii] = startVector[ii];
            InputNeutrons[i].Position[ii] = startPosition[ii];
          }
          TransmitNeutron(&InputNeutrons[i]);
          continue;
        }

        /* computes neutron variables in the initial frame */

        RotBackVector(RotMatrixBragg, Dir);

        RotBackVector(RotMatrixCE, Pos) ;

        AddVector(Pos, PosCE) ;

        /* makes depth correction to get back to the old frame for Depth != 0 */

        RotBackVector(RotMatrixCE, Depth) ;

        AddVector(Pos, Depth) ;

        /* computes neutron variables in the output frame */

        SubVector(Pos, TranslFoc) ;

        RotVector(RotMatrixFoc, Pos) ;

        RotVector(RotMatrixFoc, Dir) ;

        /* transmit coordinates which were not changed, the rest overwrite below */

        Neutrons = InputNeutrons[i];

        Neutrons.Time = TOF ;

        Neutrons.Probability = Prob ;

        CopyVector(Pos, Neutrons.Position) ;

        CopyVector(Dir, Neutrons.Vector) ;

        /* writes output binary file */

        NumOut++ ;

        WriteNeutron(&Neutrons) ;

      getlost2: ;

      }/*repetition*/


      /* here continues if neutron gets lost */

    getlost: ;

    }
  }

 my_exit:

  /* Do the general cleanup */

  OwnCleanup();

  if (mode == 1) Cleanup(TranslFoc[0], TranslFoc[1], TranslFoc[2], AnglFocHoriz, AnglFocVert);
  else  Cleanup(totalXOffset, 0, 0, 0, 0);

  return 0;
}


/* selects CE on which the neutron is reflected and gives output in frame
   of CE */

void	SelectCE(double *index, int i)
{
  int		k, l ;
  double	pos[3], dir[3] ;


  for(k = 0;k<NumberCE[0];k++) {
    for(l = 0;l<NumberCE[1];l++) {

      /* intermediate variables */

      CopyVector(InputNeutrons[i].Position, pos) ;

      CopyVector(InputNeutrons[i].Vector, dir) ;


      CopyVectorsToVector(k, l, PosCE_F, PosCE) ;

      CopyVectorsToVector(k, l, DimCE_F, DimCE) ;

      CopyMatricesToMatrix(k, l, RotMatrixCE_F, RotMatrixCE) ;


      /* computes neutron variables in the frame of the CE */

      SubVector(pos, PosCE) ;

      RotVector(RotMatrixCE, pos) ;

      RotVector(RotMatrixCE, dir) ;


      /* here computes the depth where the neutron meets the reflecting
         plane, equivalent to a parallel shift of a t=0 CE in the frame of CE */

      {
        VectorType Pos1, Pos2 ;

        if(IntersectionWithRectangular(DimCE, pos, dir, Pos1, Pos2) == 0) { goto nextCE ;}

        Depth[0] = MonteCarlo(Pos1[0], Pos2[0]) ;
      }


      Depth[1] = Depth[2] = 0 ;

      SubVector(pos, Depth) ;


      /* here we have the CE and initialise the values */

      *index = 1. ;

      CopyVector(pos, InputNeutrons[i].Position) ;

      CopyVector(dir, InputNeutrons[i].Vector) ;

			
      /* NOTE: in focusing geometry the Bragg frame and CE frame are coincident*/

      if(Option != 1) { int p, q;
			
        for(p = 0;p<3;p++) for(q = 0;q<3;q++) RotMatrixBragg[p][q] = RotMatrixCE[p][q];
      }

      return ;

    nextCE: ;

      *index = 0. ;
    }
  }

  return ;

}/* End SelectCE */


/* computes mosaic orientation of maximum probability in frame of Bragg */

void	MosaicMaxProb(VectorType Dir, double *th, VectorType Mosaic)
{
  double thrmax, phrmax ;

  CartesianToSpherical(Dir, &thrmax, &phrmax) ;

  if(thrmax < *th)
    {
      thrmax = *th - thrmax ;
      phrmax = phrmax + M_PI;

    }else{

    thrmax = thrmax - *th ;
    phrmax = phrmax ;
  }

  SphericalToCartesian(Mosaic, &thrmax, &phrmax) ;
}



/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
  fprintf(LogFilePtr," \n") ;
  print_module_name("monochr_analyser 1.8") ;

  d_spr_option = 1;
  geom_option  = 1;
  DevH         = 0.0;
  DevV         = 0.0;
  GapH         = 0.0;
  GapV         = 0.0;
  mode = 1;
  absCoeff = 0;

  while(argc>1)
    {
      switch(argv[1][1])
	{
	case 'P':
	  if((Par_Crys = fopen(&argv[1][2],"r"))==NULL)
	    {
	      fprintf(LogFilePtr,"\nERROR: parameter file '%s' not found!",&argv[1][2]);
	      exit(0);
	    }
	  ParameterFileName=&argv[1][2];
	  break;

	case 'G':
	  GeomFileName=&argv[1][2];
	  break;

	case 'A':
	  sscanf(&argv[1][2], "%ld", &Repetition) ;
	  break;

	case 'O':
	  sscanf(&argv[1][2], "%lf", &Option) ;
	  break;

	case 'm':
	  sscanf(&argv[1][2], "%lf", &mosaic_fwhm[0]) ;
	  break;

	case 'M':
	  sscanf(&argv[1][2], "%lf", &mosaic_fwhm[1]) ;
	  break;

	case 't':
	  sscanf(&argv[1][2], "%lf", &DevH) ;
	  break;

	case 'T':
	  sscanf(&argv[1][2], "%lf", &DevV) ;
	  break;

	case 'X':
	  sscanf(&argv[1][2], "%d", &mode) ;
	  break;

	case 'C':
	  sscanf(&argv[1][2], "%lf", &absCoeff) ;
	  break;

	case 'd':
	  sscanf(&argv[1][2], "%d", &d_spr_option) ;
	  break;

	case 'D':
	  sscanf(&argv[1][2], "%lf", &d_fwhm) ;
	  break;

	case 'R':
	  sscanf(&argv[1][2], "%lf", &Reflectivity) ;
	  break;


	case 'g':
	  sscanf(&argv[1][2], "%d", &geom_option) ;
	  break;


	case 'h':
	  sscanf(&argv[1][2], "%lf", &GapH) ;
	  break;

	case 'v':
	  sscanf(&argv[1][2], "%lf", &GapV) ;
	  break;


	case 'H':
	  sscanf(&argv[1][2], "%d", &NumberCE[0]) ;
	  break;

	case 'V':
	  sscanf(&argv[1][2], "%d", &NumberCE[1]) ;
	  break;

	case 'r':
	  sscanf(&argv[1][2], "%lf", &ParGeom[0]) ;
	  break;

	case 'a':
	  sscanf(&argv[1][2], "%lf", &ParGeom[1]) ;
	  break;

	case 's':
	  sscanf(&argv[1][2], "%lf", &ParGeom[2]) ;
	  break;

	}
      argc--;
      argv++;
    }

  if((Option != 1.) && (Option != 2.) && (Option != 3.))
    {
      fprintf(LogFilePtr,"\nERROR: No valid option (1.,2. or 3.) found!!") ;
      exit(0) ;
    }

  NumOut=0 ;


  fprintf(LogFilePtr,"	mosaic spread horiz, vert	=%9.4f, %9.4f\n	d spread			=   %10.4e\n	reflectivity		=  %9.4f",
	  mosaic_fwhm[0], mosaic_fwhm[1], d_fwhm, Reflectivity) ;

  fprintf(LogFilePtr,"\n	repetition rate		=   %ld", Repetition) ;


  /* prints to log file */

  fprintf(LogFilePtr,"\ninitialised option: ") ;

  if(Option == 1.) fprintf(LogFilePtr,"	'crystal_flat'") ;

  if(Option == 2.)
    {
      fprintf(LogFilePtr,"	'crystal_focus'") ;

      fprintf(LogFilePtr,"\n	vertical  : number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg,  min. angle = %.3lf deg",
	      NumberCE[1], ParGeom[0], GapV, DevV, ParGeom[1]) ;

      fprintf(LogFilePtr,"\n	horizontal: number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg",
	      NumberCE[0], ParGeom[2], GapH, DevH) ;

      fprintf(LogFilePtr,"\n	focus file: '%s'", GeomFileName) ;
    }

  if(Option == 3.)
    {
      fprintf(LogFilePtr,"	'crystal_focus_dat'") ;

      fprintf(LogFilePtr,"\n	number of CE		=   %d, %d (h.,v.)", NumberCE[0], NumberCE[1]) ;

      fprintf(LogFilePtr,"\n	focus file: '%s'", GeomFileName) ;
    }


  /* reads parameter file */

  fprintf(LogFilePtr,"\ndata from parameter file: '%s'",ParameterFileName) ;

  ReadParameterFile() ;

  if(Par_Crys != NULL)fclose(Par_Crys) ;

  /* converts degs in radian etc. */

  mosaic_fwhm[0]	*= M_PI/180. ;

  mosaic_fwhm[1]	*= M_PI/180. ;

  RotHoriz		*= M_PI/180. ;

  RotVert			*= M_PI/180. ;

  BraggHoriz		*= M_PI/180. ;

  BraggVert		*= M_PI/180. ;

  AnglFocHoriz	*= M_PI/180. ;

  AnglFocVert		*= M_PI/180. ;

  d_fwhm			*= d_spacing ;

  IntegralIntensity = 0. ;

  /* here calculates the geometry file with options 1,2,3 */

  if(Option == 1.)
    {
      NumberCE[0] = NumberCE[1] = 1 ;

      CopyVectorToVectors(0, 0, PosCE, PosCE_F) ;
      CopyVectorToVectors(0, 0, DimCE, DimCE_F) ;

      RotHoriz_F[0][0]= RotHoriz ; RotVert_F[0][0]= RotVert ;

      FillRotMatrixZY(RotMatrixSurf, RotVert, RotHoriz);
      rotOffset = CalculateRotationOffset();
      totalXOffset = PosCE[0] + DimCE[0]/2. + rotOffset;
      goto cont ;
    }

  if(Option == 2.)
    {
      if(geom_option ==1) crys_geomLambda() ;
      if(geom_option ==2) crys_geomSphere() ;
      if(geom_option ==3) crys_geomVertCyl() ;
      if(geom_option ==4) crys_geomDoubleCyl() ;

      if((Foc_Crys = fopen(GeomFileName, "r"))==NULL)
	{
	  fprintf(LogFilePtr,"\nERROR: focus file '%s' not found!", GeomFileName);
	  exit(0);
	}

      ReadFocFile() ;

      if(Foc_Crys != NULL)fclose(Foc_Crys) ;

      goto cont ;
    }

  if(Option == 3.)
    {

      NumberCE[0]= NumberCE[1]=0;

      if((Foc_Crys = fopen(GeomFileName, "r"))==NULL)
	{
	  fprintf(LogFilePtr,"\nERROR: focus file '%s' not found!", GeomFileName);
	  exit(0);
	}

      ReadFocFile() ;

      if(Foc_Crys != NULL)fclose(Foc_Crys) ;

    }

 cont: ;

  /* computes rotation matrixes corresponding to CE (i,j) offset angles */

  FillRotMatrixFoc(RotMatrixCE_F, RotVert_F, RotHoriz_F) ;

  /* computes rotation matrixes corresponding to Bragg angles */

  FillRotMatrixZY(RotMatrixBragg, BraggVert, BraggHoriz) ;

  /* computes rotation matrix corresponding to the output frame
     (focus direction) */

  FillRotMatrixZY(RotMatrixFoc, AnglFocVert, AnglFocHoriz) ;

}/* End OwnInit */

/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{

}


/* ReadParameterFile() reads the parameters from "crys.par" */

void ReadParameterFile()
{

  /* reads from file by using ReadParF(Par_Crys) and ReadParComment(Par_Crys) */


  PosCE[0]=ReadParF(Par_Crys) ; PosCE[1]=ReadParF(Par_Crys) ; PosCE[2]=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

  RotHoriz=ReadParF(Par_Crys) ; RotVert=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

  BraggHoriz=ReadParF(Par_Crys) ; BraggVert=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

  DimCE[0]=ReadParF(Par_Crys) ; DimCE[1]=ReadParF(Par_Crys) ; DimCE[2]=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

  d_spacing=ReadParF(Par_Crys) ; OrderReflection=ReadParI(Par_Crys) ; ReadParComment(Par_Crys) ;

  mosaic_range=ReadParF(Par_Crys) ; d_range=ReadParF(Par_Crys) ;  ReadParComment(Par_Crys) ;

  User = ReadParI(Par_Crys) ; ReadParComment(Par_Crys) ;

  if(User == 1)
    {
      TranslFoc[0]=ReadParF(Par_Crys) ; TranslFoc[1]=ReadParF(Par_Crys) ; TranslFoc[2]=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

      AnglFocHoriz=ReadParF(Par_Crys) ; AnglFocVert=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;
    }

  /* check some values */
  {
    char	hv;
    int		k;
    double	m_cut;

    m_cut = 1.e-3;

    for(k=0; k<2; k++) {
      hv = k==0 ? 'h' : 'v';
  	
      if (mosaic_fwhm[k] < m_cut) {
	mosaic_fwhm[k]	= m_cut ;
	fprintf(LogFilePtr, "\nWARNING: minimum mosaicity %1.1e was set for mosaic spread %c. !", m_cut, hv) ;
      }
    }
  }

  /* sets default values if frame for output not user defined */

  if (User != 1.)  {
    /* computes default rotation matrix and angles corresponding to the output frame: */

    AnglesOutputFrame(BraggHoriz, BraggVert, &AnglFocHoriz, &AnglFocVert);

    /* shift output frame origin to center of focussing geometry */

    CopyVector(PosCE, TranslFoc) ;
  }


  /* print parameters to log file for verification */

  fprintf(LogFilePtr,"\n	d-spacing		=%9.4f\n	mosaic range factor	=   %10.4e\n	d range factor		=   %10.4e\n	order of reflection		=   %d",
	
	  d_spacing, mosaic_range, d_range, OrderReflection) ;

  fprintf(LogFilePtr,"\n	main position X, Y, Z	=  %9.4f, %9.4f, %9.4f\n	thickness, width, height	= %9.4f, %9.4f, %9.4f\n	horizontal offset		=  %9.4f\n	vertical offset		= %9.4f\n	horizontal Bragg		=  %9.4f\n	vertical Bragg		= %9.4f\n	cutoff probability		=    %8.1e",
	
	  PosCE[0], PosCE[1], PosCE[2], DimCE[0], DimCE[1], DimCE[2], RotHoriz, RotVert, BraggHoriz, BraggVert,wei_min) ;

  if (User == 1.) fprintf(LogFilePtr,"\nuser defined frame:") ;
  else  fprintf(LogFilePtr,"\nstandard frame generation:") ;

  fprintf(LogFilePtr,"\n	horizontal angle		=  %9.4f\n	vertical angle		= %9.4f\n	X',Y',Z'			=  %9.4f, %9.4f, %9.4f\n\n",
	  AnglFocHoriz, AnglFocVert, TranslFoc[0], TranslFoc[1], TranslFoc[2]) ;

}/* End ReadParameterFile */


/* ReadFocFile() reads the parameters from geometry file */

void ReadFocFile()
{
  int	i, j, k ;

  /* reads from file by using ReadParF() and ReadParComment() */

  NumberCE[0] = ReadParI(Foc_Crys) ; NumberCE[1] = ReadParI(Foc_Crys) ; ReadParComment(Foc_Crys) ;


  for(i = 0;i<NumberCE[0];i++)
    {
      for(j = 0;j<NumberCE[1];j++)
	{

	  PosCE_F[0][i][j]=ReadParF(Foc_Crys) ; PosCE_F[1][i][j]=ReadParF(Foc_Crys) ; PosCE_F[2][i][j]=ReadParF(Foc_Crys) ;

	  DimCE_F[0][i][j]=ReadParF(Foc_Crys) ; DimCE_F[1][i][j]=ReadParF(Foc_Crys) ; DimCE_F[2][i][j]=ReadParF(Foc_Crys) ;

	  RotHoriz_F[i][j]=ReadParF(Foc_Crys) ; RotVert_F[i][j]=ReadParF(Foc_Crys) ;


	  /* converts degs in radian etc. */

	  RotHoriz_F[i][j]		*= M_PI/180. ;

	  RotVert_F[i][j]		*= M_PI/180. ;


	  /* ads main parameters */

	  for(k=0;k<3;k++)
	    {
	      PosCE_F[k][i][j]		+= PosCE[k] ;

	      DimCE_F[k][i][j]		+= DimCE[k] ;
	    }

	  RotHoriz_F[i][j]		+= RotHoriz ;

	  RotVert_F[i][j]			+= RotVert ;

	}
    }

}/* End ReadFocFile */


/* computes frame angles of output */

void AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert)
{
  double n[3] ;

  FillRotMatrixZY(RotMatrixCE, M_PI/180.*RotVert, M_PI/180.*RotHoriz) ;

  n[0] = 1. ;		n[1] = 0. ;		n[2] = 0. ;

  RotVector(RotMatrixCE, n) ; /* components of a vector parallel to X in the frame of the CE */

  n[0] *= -1. ; /* reflection on the CE */

  RotBackVector(RotMatrixCE, n) ;  /* new components in the frame of input */

  CartesianToEulerZY(n, AnglFocVert, AnglFocHoriz) ;

  if(*AnglFocHoriz == - M_PI) *AnglFocHoriz = M_PI ;

  if(*AnglFocVert == - M_PI) *AnglFocVert = M_PI ;

  *AnglFocHoriz	*= 180./M_PI ;

  *AnglFocVert	*= 180./M_PI ;
}


/* fills the RotMatrixCE_F-s from RotVert_F, RotHoriz_F */

void	FillRotMatrixFoc(double RotMatrixCE_F[3][3][CRYS_SIZE][CRYS_SIZE], double RotVert_F[CRYS_SIZE][CRYS_SIZE], double RotHoriz_F[CRYS_SIZE][CRYS_SIZE])
{
  int i, j ;
  double RotHoriz, RotVert, RotMatrix[3][3] ;

  for(i = 0;i<NumberCE[0];i++)
    {
      for(j = 0;j<NumberCE[1];j++)
	{
	  RotHoriz = RotHoriz_F[i][j] ;

	  RotVert	= RotVert_F[i][j] ;

	  FillRotMatrixZY(RotMatrix, RotVert, RotHoriz) ;

	  CopyMatrixToMatrices(i, j, RotMatrix, RotMatrixCE_F) ;
	}
    }
}


void   CopyMatricesToMatrix(int i, int j, double Matrix[3][3][CRYS_SIZE][CRYS_SIZE], double Result[3][3])
{
  int k, l ;

  for(k = 0;k<3;k++)
    for(l = 0;l<3;l++)
      Result[k][l] = Matrix[k][l][i][j] ;
}

void	CopyMatrixToMatrices(int i, int j, double Result[3][3], double Matrix[3][3][CRYS_SIZE][CRYS_SIZE])
{
  int k, l ;

  for(k = 0;k<3;k++)
    for(l = 0;l<3;l++)
      Matrix[k][l][i][j] = Result[k][l] ;
	
}

void	CopyVectorsToVector(int i, int j, double Vector[3][CRYS_SIZE][CRYS_SIZE], double Result[3])
{
  int k ;

  for(k = 0;k<3;k++)
      Result[k] = Vector[k][i][j] ;
}

void	CopyVectorToVectors(int i, int j, double Vector[3], double Result[3][CRYS_SIZE][CRYS_SIZE])
{
  int k ;

  for(k = 0;k<3;k++)
    Result[k][i][j] = Vector[k] ;
}


void TransmitNeutron(Neutron* n)
{
  VectorType dir, pos, Pos1, Pos2;
  int i;
  double scalar, ToF;

  for (i = 0; i < 3; i++) {
    dir[i] = n->Vector[i];
    pos[i] = n->Position[i];
  }

  pos[0] -= PosCE[0];
  pos[1] -= PosCE[1];
  pos[2] -= PosCE[2];

  RotBackVector(RotMatrixSurf, pos);
  RotBackVector(RotMatrixSurf, dir);

  if(IntersectionWithRectangular(DimCE, pos, dir, Pos1, Pos2)) {
    double distInCrystal, weightFactor;
    SubVector(Pos2, Pos1);
    distInCrystal = LengthVector(Pos2);
    weightFactor = exp (-1.*distInCrystal*absCoeff);
    n->Probability *= weightFactor;
  }

  scalar = totalXOffset / n->Vector[0];
  for (i = 0; i < 3; i++)
    n->Position[i] += n->Vector[i]*scalar;

  ToF = totalXOffset/(V_FROM_LAMBDA(n->Wavelength)*n->Vector[0]);
  n->Time += ToF;

  WriteNeutron(n);
}

double CalculateRotationOffset()
{
  VectorType vec1, vec2;
  double xOffset1, xOffset2;

  vec1[0] = DimCE[0]/2.;
  vec1[1] = DimCE[1]/2;
  vec1[2] = DimCE[2]/2.;

  vec2[0] = DimCE[0]/2.;
  vec2[1] = DimCE[1]/2;
  vec2[2] = -DimCE[2]/2.;

  RotVector(RotMatrixSurf, vec1);
  RotVector(RotMatrixSurf, vec2);

  xOffset1 = fabs(vec1[0]);
  xOffset2 = fabs(vec2[0]);

  return Max(xOffset1, xOffset2);
}
