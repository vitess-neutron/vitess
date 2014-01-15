#ifndef MONOCHRCLASS_CPP
#define MONOCHRCLASS_CPP

#include "mathfunctions.h"
#include "monochrclass.h"


Monochromator::Monochromator()
{
 
  // Initialise member variables
  Par_Crys = 0;   
  Foc_Crys = 0;

  ParameterFileName = ""; 
  GeomFileName = "";
  
  OrderReflection = 1;
  Option = 0;
  ParGeomN = -1;
  d_spr_option = 0; 
  geom_option = 0; 
  mode = -1;
  mosRndmDir = 0;
  firstElement = 1;

  for (int i = 0; i < 2; i++) NumberCE [i] = 0;
  
  NumOut = 0; 
  Repetition = 1; 

  peakWL = 0;
  braggAngleTot = 0;
  axisPhi = 0;
  maxDeviation = 1.;

  TOF = -1; 
  Prob = -1; 
  
  Index = -1; 
  User = 0; 
  IntegralIntensity = 0;

  for (int i = 0; i < PAR_GEOM; i++) ParGeom[i] = 0;
  
  d_spacing = 0; 
  d_fwhm = 0; 
  d_sigma = 0;
  absCoeff = 0;
  Reflectivity = 1; 

  for (int i = 0; i < 2; i++) mosaic_fwhm[i] = 0;

  RotHoriz = 0; 
  RotVert = 0; 
  BraggHoriz = 0; 
  BraggVert = 0;
  AnglFocHoriz = 0; 
  AnglFocVert = 0; 
  totalXOffset = 0; 
  rotOffset = 0;

  DevH = 0; 
  DevV = 0;
  GapH = 0; 
  GapV = 0;

  for (int i = 0; i < 3; i++) {

    Pos[i] = 0; 
    Dir[i] = 0; 
    Path[i] = 0;
    TranslFoc[i] = 0; 
    TranslFoc_def[i] = 0; 
    PosCE[i] = 0; 
    DimCE[i] = 0; 
    Depth[i] = 0;

    dSpacingSpreadParams[i] = 0; 
    horMosaicSpreadParams[i] = 0; 
    vertMosaicSpreadParams[i] = 0;;

    for (int j = 0; j < 3; j++) {

       RotMatrixCE[i][j] = 0; 
       RotMatrixBragg[i][j] = 0; 
       RotMatrixFoc[i][j] = 0; 
       RotMatrixSurf[i][j] = 0;

    }

  }

  currentNeutron = 0;

}


void Monochromator::Init(int argc, char* argv[])
{

  bVisInstalled = TRUE;

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
	  sscanf(&argv[1][2], "%d", &Option) ;
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
	  
	case 'o':
	  sscanf(&argv[1][2], "%d", &firstElement) ;
	  break;

	}
      argc--;
      argv++;
    }

  
  if((Option != 1) && (Option != 2) && (Option != 3))
    {
      fprintf(LogFilePtr,"\nERROR: No valid option (1.,2. or 3.) found!!") ;
      exit(0) ;
    }

  
  /* prints to log file */

  fprintf(LogFilePtr,"	mosaic spread horiz, vert	=%9.4f, %9.4f\n	d spread			=   %10.4e\n	reflectivity		=  %9.4f",
	  mosaic_fwhm[0], mosaic_fwhm[1], d_fwhm, Reflectivity) ;
  fprintf(LogFilePtr,"\n	repetition rate		=   %ld \n", Repetition) ;
  fprintf(LogFilePtr,"initialised option: ") ;

  if(Option == 1) fprintf(LogFilePtr,"	'crystal_flat'") ;

  else if(Option == 2)
    {
      fprintf(LogFilePtr,"	'crystal_focus'") ;
      fprintf(LogFilePtr,"\n	vertical  : number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg,  min. angle = %.3lf deg",
	      NumberCE[1], ParGeom[0], GapV, DevV, ParGeom[1]) ;
      fprintf(LogFilePtr,"\n	horizontal: number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg",
	      NumberCE[0], ParGeom[2], GapH, DevH) ;
      fprintf(LogFilePtr,"\n	focus file: '%s'", GeomFileName) ;
    }

  else if(Option == 3)
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
  RotVert		*= M_PI/180. ;
  BraggHoriz		*= M_PI/180. ;
  BraggVert		*= M_PI/180. ;
  AnglFocHoriz	        *= M_PI/180. ;
  AnglFocVert		*= M_PI/180. ;
  d_fwhm		*= d_spacing ;
  d_sigma                = d_fwhm/sqrt(8.*log(2.));

  // Filling the array of the d_spacing spread parameters 
  // for normalisation calculation and randomising
  dSpacingSpreadParams[0] = 1.;
  dSpacingSpreadParams[1] = d_spacing;
  if (d_spr_option == 1) dSpacingSpreadParams[2] = d_fwhm;
  else  dSpacingSpreadParams[2] = d_sigma;

  horMosaicSpreadParams[0] = 1.;
  horMosaicSpreadParams[1] = 0.;
  horMosaicSpreadParams[2] = mosaic_fwhm[0]/sqrt(8.*log(2.));

  vertMosaicSpreadParams[0] = 1.;
  vertMosaicSpreadParams[1] = 0.;
  vertMosaicSpreadParams[2] = mosaic_fwhm[1]/sqrt(8.*log(2.));


  FindPeakLambdaSortMosaicityDistr(BraggHoriz, BraggVert);
  //This function takes care of a proper normalisation of the fNorm mosaicity Gaussian 
  //to comply with the reflectivity value provided by the user
  NormFunction();

  fprintf(LogFilePtr,"\n Norm of the mosaic function:  %1.4lf, peak wavelength %1.4lf \n", fNorm[0], peakWL) ;

  /* here the geometry file is calculated according to options 1,2,3 */
  if(Option == 1)
    {
      NumberCE[0] = NumberCE[1] = 1 ;
      
      CopyVectorToVectors(0, 0, PosCE, PosCE_F) ;
      CopyVectorToVectors(0, 0, DimCE, DimCE_F) ;

      std::vector <double> RotHorizVector;
      RotHoriz_F.push_back(RotHorizVector);
      RotHoriz_F[0].push_back(RotHoriz);
      
      std::vector <double> RotVertVector;
      RotVert_F.push_back(RotVertVector);
      RotVert_F[0].push_back(RotVert);

      FillRotMatrixZY(RotMatrixSurf, RotVert, RotHoriz);
     
    }
  
  else if(Option == 2)
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
    }
  
  else if(Option == 3)
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
  
  //  rotOffset = CalculateRotationOffset();
  totalXOffset = TranslFoc[0] + DimCE[0]/2.*NumberCE[0]*cos(RotHoriz) + DimCE[1]/2.*NumberCE[1]*sin(RotVert);

  /* computes rotation matrixes corresponding to CE (i,j) offset angles */
  FillRotMatrixFoc() ;

  /* computes rotation matrixes corresponding to Bragg angles */
  FillRotMatrixZY(RotMatrixBragg, BraggVert, BraggHoriz) ;

  /* computes rotation matrix corresponding to the output frame
     (focus direction) */
  FillRotMatrixZY(RotMatrixFoc, AnglFocVert, AnglFocHoriz) ;

  if (bVisInstr)
    { 
      if(Option == 1) {
      // Visualisation of the monochromator geomentry
	stGeometry.pCuboid = (VtCuboid*) calloc(1, sizeof(VtCuboid));
	stGeometry.nCuboids = 1; 
	
	stGeometry.pCuboid[0].Length = DimCE[0]; 
	stGeometry.pCuboid[0].Width  = DimCE[1];
	stGeometry.pCuboid[0].Height = DimCE[2];
	stGeometry.pCuboid[0].vCntr[0]  = PosCE[0];
	stGeometry.pCuboid[0].vCntr[1]  = PosCE[1];
	stGeometry.pCuboid[0].vCntr[2]  = PosCE[2];
	stGeometry.pCuboid[0].vNormal[0]= 1.;
	stGeometry.pCuboid[0].vNormal[1]= tan(RotHoriz);
	stGeometry.pCuboid[0].vNormal[2]= tan(RotVert);
	
	stGeometry.pDescr  = "monochromator:yellow";
	stGeometry.eModule = VT_MONOC_ANALY;
      }

      else {

	stGeometry.nCuboids = NumberCE[0]*NumberCE[1];
	stGeometry.pCuboid = (VtCuboid*) calloc(stGeometry.nCuboids, sizeof(VtCuboid));
	stGeometry.pDescr  = "monochromator:yellow";
	stGeometry.eModule = VT_MONOC_ANALY;

	int k = 0;
	
	for(int i = 0;i<NumberCE[0];i++) {
	    for(int j = 0;j<NumberCE[1];j++) {
	      
	      VectorType DimCurrCE, PosCurrCE;
	      double RotMatrixCurrCE[3][3];

	      CopyVectorsToVector(i, j, PosCE_F, PosCurrCE) ;
	      CopyVectorsToVector(i, j, DimCE_F, DimCurrCE) ;
	      CopyMatricesToMatrix(i, j, RotMatrixCE_F, RotMatrixCurrCE) ;

	      VectorType normal={1, 0, 0};
	      RotBackVector(RotMatrixCurrCE , normal);

	      stGeometry.pCuboid[k].Length = DimCurrCE[0]; 
	      stGeometry.pCuboid[k].Width  = DimCurrCE[1];
	      stGeometry.pCuboid[k].Height = DimCurrCE[2];
	      stGeometry.pCuboid[k].vCntr[0]  = PosCurrCE[0];
	      stGeometry.pCuboid[k].vCntr[1]  = PosCurrCE[1];
	      stGeometry.pCuboid[k].vCntr[2]  = PosCurrCE[2];
	      stGeometry.pCuboid[k].vNormal[0]= normal[0];
	      stGeometry.pCuboid[k].vNormal[1]= normal[1];
	      stGeometry.pCuboid[k].vNormal[2]= normal[2];

	      k++;

	    }
	}
	
      }

  }
  
  return; 

}

void  Monochromator::FillRotMatrixFoc()
{
  int i, j ;
  double RotHoriz, RotVert, RotMatrix[3][3] ;

  for(i = 0;i<NumberCE[0];i++)
    {
      for(j = 0;j<NumberCE[1];j++)
	{
	  RotHoriz = RotHoriz_F[i][j] ;
	  RotVert  = RotVert_F[i][j] ;
	  FillRotMatrixZY(RotMatrix, RotVert, RotHoriz) ;
	  CopyMatrixToMatrices(i, j, RotMatrix, RotMatrixCE_F) ;
	}
    }

  return;
}


/* ReadParameterFile() reads the parameters from "crys.par" */

void Monochromator::ReadParameterFile()
{

  double mosaic_range, d_range;

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
  
  char	hv;
  int	k;
  double m_cut;
  
  m_cut = 1.e-3;
  
  for(k=0; k<2; k++) {
    hv = k==0 ? 'h' : 'v';
    
    if (mosaic_fwhm[k] < m_cut) {
      mosaic_fwhm[k]	= m_cut ;
      fprintf(LogFilePtr, "\nWARNING: minimum mosaicity %1.1e was set for mosaic spread %c. !", m_cut, hv) ;
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
  fprintf(LogFilePtr,"\nd-spacing =%9.4f\n order of reflection =   %d",	
	  d_spacing, OrderReflection) ;

  fprintf(LogFilePtr,"\n	main position X, Y, Z	=  %9.4f, %9.4f, %9.4f\n	thickness, width, height	= %9.4f, %9.4f, %9.4f\n	horizontal offset		=  %9.4f\n	vertical offset		= %9.4f\n	horizontal Bragg		=  %9.4f\n	vertical Bragg	       = %9.4f\n	cutoff probability		=    %8.1e",
	  PosCE[0], PosCE[1], PosCE[2], DimCE[0], DimCE[1], DimCE[2], RotHoriz, RotVert, BraggHoriz, BraggVert,wei_min) ;

  if (User == 1.) fprintf(LogFilePtr,"\nuser defined frame:") ;
  else  fprintf(LogFilePtr,"\nstandard frame generation:") ;

  fprintf(LogFilePtr,"\n	horizontal angle		=  %9.4f\n	vertical angle	 = %9.4f\n	X',Y',Z'			=  %9.4f, %9.4f, %9.4f\n\n",
	  AnglFocHoriz, AnglFocVert, TranslFoc[0], TranslFoc[1], TranslFoc[2]) ;

  return;

}/* End ReadParameterFile */


void Monochromator::AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert)
{


  FillRotMatrixZY(RotMatrixCE, M_PI/180.*RotVert, M_PI/180.*RotHoriz) ;

  VectorType n={1, 0, 0};
  RotVector(RotMatrixCE, n) ; /* components of a vector parallel to X in the frame of the CE */
  n[0] *= -1. ; /* reflection on the CE */
  RotBackVector(RotMatrixCE, n) ;  /* new components in the frame of input */

  CartesianToEulerZY(n, AnglFocVert, AnglFocHoriz) ;

  if(*AnglFocHoriz == - M_PI) *AnglFocHoriz = M_PI ;
  if(*AnglFocVert == - M_PI) *AnglFocVert = M_PI ;
  *AnglFocHoriz	*= 180./M_PI ;
  *AnglFocVert	*= 180./M_PI ;

  return;
}

/* ReadFocFile() reads the parameters from geometry file */
void Monochromator::ReadFocFile()
{
  int	i, j, k ;

  /* reads from file by using ReadParF() and ReadParComment() */
  NumberCE[0] = ReadParI(Foc_Crys) ; NumberCE[1] = ReadParI(Foc_Crys) ; ReadParComment(Foc_Crys) ;

  for(i = 0;i<NumberCE[0];i++)
    {

      std::vector <double> tempVector;
      PosCE_F[0].push_back(tempVector);
      PosCE_F[1].push_back(tempVector);
      PosCE_F[2].push_back(tempVector);
      DimCE_F[0].push_back(tempVector);
      DimCE_F[1].push_back(tempVector);
      DimCE_F[2].push_back(tempVector);
      RotHoriz_F.push_back(tempVector);
      RotVert_F.push_back(tempVector);

      for(j = 0;j<NumberCE[1];j++)
	{

	  PosCE_F[0][i].push_back(ReadParF(Foc_Crys)) ; PosCE_F[1][i].push_back(ReadParF(Foc_Crys)) ; PosCE_F[2][i].push_back(ReadParF(Foc_Crys)) ;
	  DimCE_F[0][i].push_back(ReadParF(Foc_Crys)) ; DimCE_F[1][i].push_back(ReadParF(Foc_Crys)) ; DimCE_F[2][i].push_back(ReadParF(Foc_Crys)) ;
	  RotHoriz_F[i].push_back(ReadParF(Foc_Crys)) ; RotVert_F[i].push_back(ReadParF(Foc_Crys)) ;

	  /* converts degs in radian etc. */
	  RotHoriz_F[i][j] *= M_PI/180. ;
	  RotVert_F[i][j] *= M_PI/180. ;

	  /* ads main parameters */
	  for(k=0;k<3;k++)
	    {
	      PosCE_F[k][i][j] += PosCE[k] ;
	      DimCE_F[k][i][j] += DimCE[k] ;
	    }

	  RotHoriz_F[i][j] += RotHoriz ;
	  RotVert_F[i][j] += RotVert ;

	}
    }

  return;

}/* End ReadFocFile */


// This function calculates the peak wavelength based on the horizontal and vertical
// Bragg angles and the given d spacing. In addition, the phi angle between the
// instrument axis (direction of incoming trajectories) and normal of the CE
// being (1,0,0) determines which mosaicity distribution (vertical or horizontal) 
// is used for randomisation and which as a look-up for the absolute norm.
// See the help file for further details
void Monochromator::FindPeakLambdaSortMosaicityDistr(double RotHoriz, double RotVert)
{

  double tempMatrix[3][3];

  FillRotMatrixZY(tempMatrix, RotVert, RotHoriz) ;

  VectorType n={1, 0, 0};
  VectorType n2={1, 0, 0};

  RotVector(tempMatrix, n) ; /* components of a vector parallel to X in the frame of the CE */

  braggAngleTot = AngleVectors(n, n2)/180.*M_PI;
  peakWL = cos(braggAngleTot) * 2. * d_spacing;

  if (braggAngleTot > 1e-4) {
    double thetaTemp = 0;
    CartesianToSpherical(n, &thetaTemp, &axisPhi);
    if (axisPhi > M_PI_2) axisPhi = fabs(M_PI - axisPhi);
  }
  else {
    braggAngleTot = 1e-4;
    axisPhi = 0;
  }

  double sinP = sin(axisPhi);
  double cosP = cos(axisPhi);

 if (sinP >= cosP) {
   CopyVector(horMosaicSpreadParams, fRndm);
   CopyVector(vertMosaicSpreadParams, fNorm);
   mosRndmDir = 1;
 }
 else {
   CopyVector(vertMosaicSpreadParams, fRndm);
   CopyVector(horMosaicSpreadParams, fNorm);
   mosRndmDir = 2;
 }

 return;

}

// This function is called in Init() and calculates the proper normalisation of the mosaicity
// distribution function that is used to look up the absolute norm after reflection of a trajectory
void Monochromator::NormFunction()
{

  const int nBins = 500;

  double y[nBins];
  for (int i = 0; i < nBins; i++) y[i] = 0;

  double sigma1 = dSpacingSpreadParams[2];
  double x1_incr = sigma1/20.;
  if (x1_incr == 0) x1_incr = 1.;

  double sigma2 = fRndm[2];
  double x2_incr = sigma2/100.;
  if (x2_incr == 0) x2_incr = 1.;

  double minSC = Min(sin(axisPhi), cos(axisPhi));
  maxDeviation = (1. + 2.*minSC - (minSC/sin(M_PI_2/2.)))*braggAngleTot;

  fprintf(LogFilePtr,"\n braggTot =%9.4f, sigma1 =   %f, sigma2 = %f, maxDev = %f", (M_PI_2 - braggAngleTot)*180./M_PI, sigma1, sigma2, maxDeviation*180./M_PI);

  for (double x1 = d_spacing - 2.975*sigma1; x1 <= d_spacing + 2.975*sigma1; x1 += x1_incr) {

    double prob_dspacing = 1.;
    if (sigma1 > 0) {
      if(d_spr_option == 1) prob_dspacing =  sq(sigma1) / ( 4.*sq(x1 - d_spacing) + sq(sigma1) ) ;
      else prob_dspacing = dSpacingSpreadParams[0]*exp(-sq(x1 - dSpacingSpreadParams[1])/(2.*sq(sigma1)));   
    }
    double braggAngleDev = acos(peakWL/(2.*x1)) - acos(peakWL/(2.*d_spacing));

    if ((braggAngleTot - braggAngleDev) < 1e-5) continue;

    for (double x2 = -4.995*sigma2; x2 <= 4.995*sigma2; x2 += x2_incr) {

      if (fabs(x2) > maxDeviation) continue;

      double ytemp = prob_dspacing * fRndm[0]*exp(-sq(x2 - fRndm[1])/(2.*sq(sigma2)));
      double x21 = 0;
      
      DetermineMosaicAngle(braggAngleDev, x2, x21);
         
      int index = ((int)(fNorm[0]*exp(-sq(x21 - fNorm[1])/(2.*sq(fNorm[2])))*nBins));
      y[index] += ytemp;      
      
    }
  }

  double sum = 0;
  double weight = 0;

  for (int i = 0; i < nBins; i++) {

    double x = 1./(2.*nBins) + (1./nBins)*((double) i);
    sum += x*y[i];
    weight += y[i];

  }

  double mean;
  if (weight > 0) mean = sum/weight;
  else mean = 1.;

  // Here the reflectivity is incorporated into the norm of the 
  // mosaicity Gaussian that is used to look up the absolute weight
  // of a reflected neutron.
  fNorm[0] = Reflectivity/mean;

  return;

} 

/* During the normalisation calculation, this function is used to determine the value of the mosaic angle
 perpendicular to the one that is scanned in the Norm() method. 
 Input parameters: angleDiff = angular deviation from the given bragg angle due to the d_spacing spread
                   mosaicAngle1 = First mosaic angle from the scan in the Norm() method
                   mosaicAngle2 = Is filled with the solution found by the method 
 See the help file for further details. */                   
void  Monochromator::DetermineMosaicAngle(double angleDiff, double mosaicAngle1, double &mosaicAngle2)
{

  double r1 = tan(braggAngleTot + angleDiff) - tan(angleDiff); 
  double r2 = tan(braggAngleTot + angleDiff);
  double x = fabs(sin(mosaicAngle1));
  double gamma = 0;
  if (mosaicAngle1 < 0) gamma = M_PI/2. + fabs(axisPhi);
  else gamma = M_PI/2. - fabs(axisPhi);
		   
  double c_sq = r1*r1 + x*x - 2*r1*x*cos(gamma);  
  double alpha = acos(-1.*(r1*r1 - c_sq - x*x)/(2.*sqrt(c_sq)*x));
  double beta = 0;
  if (alpha <= M_PI/2) beta = M_PI/2. - alpha;// - fabs(theta);
  else beta = M_PI*3./2. - alpha;// - fabs(theta);

  double y1, y2;

  if ((c_sq*cos(beta)*cos(beta) - c_sq + r2*r2) > 0) {
    y1 = sqrt(c_sq)*cos(beta) + sqrt(c_sq*cos(beta)*cos(beta) - c_sq + r2*r2);
    y2 = sqrt(c_sq)*cos(beta) - sqrt(c_sq*cos(beta)*cos(beta) - c_sq + r2*r2);
  }
  else {
    y1 =  sqrt(c_sq)*cos(beta);
    y2 = y1;
  }
   
  double phi1, phi2;

  if (y1 <= 1) phi1 = asin(y1*cos(mosaicAngle1));
  else phi1 = M_PI;

  if (fabs(y2) <= 1) phi2 = asin(y2*cos(mosaicAngle1));
  else phi2 = M_PI;

  mosaicAngle2 = Min(fabs(phi1), fabs(phi2));

  if (sqrt(c_sq) < r2) mosaicAngle2 = fabs(mosaicAngle2)*(-1.);
  else mosaicAngle2 = fabs(mosaicAngle2);

  return;
  
}

// In this function the trajectory is propagated to the monochromator,
// reflected off the corresponding CE and weighted with the reflection probability
void Monochromator::processNeutron(Neutron* neutron)
{

  Neutron neutronForTrajectories;
  Neutron resultNeutron;
  Neutron resultNeutron2;
  
  if (!firstElement && (mode == 1 || mode == 3)) {
    if (neutron->Color >= 1) {

      NumOut++;
      WriteNeutron(neutron);
      return;

    }
  }

  // Check the vector magnitude for unity
  neutron->Vector[0] = sqrt(1 - sq(neutron->Vector[1]) - sq(neutron->Vector[2])) ;

  /* selects CE on which the neutron is reflected and gives global variables
     in	the frame of CE */
  double startTime = neutron->Time;
  double WL = neutron->Wavelength;
  VectorType startPosition;
  VectorType startVector;

  for (int ii = 0; ii < 3; ii++) {
    startVector[ii] = neutron->Vector[ii];
    startPosition[ii] = neutron->Position[ii];
  }

  SelectCE(&Index, neutron) ;

  if (Index == 0./* no CE was found */) {
    // Return if neutron missed the monochromator in "Reflection" mode
    if (mode == 1) {
      return;
    }
    // No attenuation if neutron missed the monochromator in "Transmission" mode
    else if (mode == 2) {
      CopyVector(startPosition, neutron->Position);
      neutron->Time = startTime;
      TransmitNeutron(neutron);
      NumOut++ ;
      return;
    }
    else {
      CopyVector(startPosition, neutron->Position);
      neutron->Time = startTime;
      neutron->Color = 0;
      TransmitNeutron(neutron);
      WriteIAP(neutron, VT_PASSED);
      NumOut++ ;
      return;
    }
  }

  // If a CE was found,  InputNeutrons[i] contains now position and direction of the incoming neutron
  // in the frame of the reflecting crystal element.

  /* moment of arriving at the crystal plane, new position */   
  neutron->Time -= neutron->Position[0]  / fabs(neutron->Vector[0]) /V_FROM_LAMBDA(neutron->Wavelength);

  CopyVector(neutron->Vector, Path) ;
  MultiplyByScalar(Path, - neutron->Position[0]/ neutron->Vector[0] ) ;
  AddVector(neutron->Position, Path) ; /* Path = displacement vector */

  // InputNeutrons[i] contains now position and direction of the neutron at the point of reflection
  // in the frame of the reflecting crystal element.
  /* for flat option (Option =1) computes neutron direction in the "Bragg" frame keeping frame of CE for the position */

  RotBackVector(RotMatrixCE, neutron->Vector) ;  // Vector is now back to the original frame

  // write intersection point
  neutronForTrajectories = *neutron;
  //  AddVector(neutronForTrajectories.Position, PosCE);
  //  RotBackVector(RotMatrixCE, neutronForTrajectories.Position) ;
  //  AddVector(neutronForTrajectories.Position, TranslFoc);

  RotVector(RotMatrixBragg, neutron->Vector) ;   // Vector is now back in the frame of the Bragg refl. plane

  for(int repet=0; repet<Repetition; repet++) {
  
    double arg, pi2_bragg;

    double dran = d_spacing;

     /* random d-spacing */     
    if (d_fwhm > 0) {
      if(d_spr_option == 1) dran = RandomLorentzian(d_spacing, d_fwhm) ;
      if(d_spr_option == 2) dran = DistrGauss(d_spacing, d_sigma);  
    }
    
    arg = WL * OrderReflection / 2. / dran ;
    if (arg >= 1.) return;/* wavelength to large */

    TOF = neutron->Time ;
    Prob = neutron->Probability / Repetition ;
    CopyVector(neutron->Position, Pos) ;
    CopyVector(neutron->Vector, Dir) ;
             
    /* computes reflection angle corresponding to d_ran */
    pi2_bragg = (double) acos(arg) ;
    
    // Here the reflection probability is calculated
    // and the neutron trajectory changes direction after 
    // reflection off a mosaic element.

    Prob *= CalculateReflectionProbability(pi2_bragg, Dir);

   
    if(Prob <= wei_min && mode==1) return;
    if (Dir[0] != Dir[0] || Dir[1] != Dir[1] || Dir[2] != Dir[2]) return;
    IntegralIntensity += Prob ;
   	
    if (mode == 2) {
      neutron->Probability -= Prob;
      neutron->Time = startTime;
      CopyVector(startVector, neutron->Vector);
      CopyVector(startPosition, neutron->Position);
      TransmitNeutron(neutron);
      if (repet == 0) WriteIAP(&neutronForTrajectories, VT_PASSED);
      NumOut++ ;
      return;
    }

    /* computes neutron variables in the initial frame */
    RotBackVector(RotMatrixBragg, Dir);
    RotBackVector(RotMatrixCE, Pos) ;
    AddVector(Pos, PosCE) ;

    /* makes depth correction to get back to the old frame for Depth != 0 */
    RotBackVector(RotMatrixCE, Depth) ;
    AddVector(Pos, Depth) ;
   
    if (!firstElement || mode == 3) neutron->Color = 1;

    if (repet == 0 && Prob > wei_min) {
      CopyVector(Pos, neutronForTrajectories.Position) ;
      CopyVector(Dir, neutronForTrajectories.Vector) ;
      neutronForTrajectories.Color = neutron->Color;
      neutronForTrajectories.Probability = Prob / neutron->Probability * Repetition;
      WriteIAP(&neutronForTrajectories, VT_REFLECTED); 
    }

    // Rotate the Vectors to the output frame in the "Reflection" mode
    if (mode == 1 && firstElement) {     
      /* computes neutron variables in the output frame */
      SubVector(Pos, TranslFoc) ;
      RotVector(RotMatrixFoc, Pos) ;
      RotVector(RotMatrixFoc, Dir) ;
    }
    // Do not rotate vectors, create a new neutron for the "Reflection+Transition" mode
    else if (mode == 3) {
      resultNeutron2 = *neutron;
      resultNeutron2.Probability =  (neutron->Probability - Prob)*exp (-1.*maxDepth*absCoeff);;
      resultNeutron2.Color = 0;
      CopyVector(startPosition, resultNeutron2.Position) ;
      CopyVector(startVector, resultNeutron2.Vector) ;

#ifdef DEBUG
      double tempTh, tempPh;
      CartesianToSpherical(startVector, &tempTh, &tempPh);
      DEBUG_OUT("Direction in the initial frame transition: neutronTh = %f, neutronPh = %f  ProbR: %f  ProbT: %f \n", 
		 tempTh*180./M_PI, tempPh*180./M_PI, Prob, resultNeutron2.Probability);
#endif

      resultNeutron2.Time = startTime;
      ChangeNeutronID(&resultNeutron2);
      NumOut++ ;
      if (repet == 0) WriteIAP(&resultNeutron2, VT_PASSED); 
      WriteNeutron(&resultNeutron2);     
    }

    /* transmit coordinates which were not changed, the rest overwrite below */
    if (Prob <= wei_min) return;
    
    resultNeutron = *neutron;
    resultNeutron.Time = TOF ;
    resultNeutron.Probability = Prob ;
    CopyVector(Pos, resultNeutron.Position) ;
    CopyVector(Dir, resultNeutron.Vector) ;    

    /* writes output binary file */
    NumOut++ ;
    WriteNeutron(&resultNeutron) ;

  }/*repetition*/


  /* here continues if neutron gets lost */
  return;

}


void  Monochromator::SelectCE(double *index, Neutron* n)
{
  int		k, l ;
  double	pos[3], dir[3] ;

  maxDepth = 0;

  for(k = 0;k<NumberCE[0];k++) {
    for(l = 0;l<NumberCE[1];l++) {

      /* intermediate variables */
      CopyVector(n->Position, pos) ;
      CopyVector(n->Vector,   dir) ;

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
	maxDepth = Pos2[0];
        Depth[1] = Depth[2] = 0.0 ;
      }

      SubVector(pos, Depth) ;

      /* here we have the CE and initialise the values */
      *index = 1. ;

      CopyVector(pos, n->Position) ;
      CopyVector(dir, n->Vector) ;

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



// This function calculates the reflection probability for a neutron trajectory.
// In processNeutron(), the d_spacing and therefore the corresponding Bragg angle has been determined 
// by randomisation. In this function, the mosaicity in one dimension (fRndm) is randomised,
// while the mosaicity in the perpendicular direction is determined such that the Bragg condition
// between the direction of the mosaic element normal and the neutron trajectory is satisfied. This is done 
// by solving a system consisting of 3 equations:
// Eq. 1: sin(mosaicAngle1) = y/x (if the randomisation direction is horizontal) or sin(mosaicAngle1) = z (vertical)
// Here, mosaicAngle1 is the randomised mosaicity angle and x, y, z are vector components of the mosaic piece normal.
// Eq. 2: Mag(x, y, z) = 1 // The magnitude of the mosaic piece normal is 1
// Eq. 3: (x, y, z)*(x_n, y_n, z_n) = cos(pi/2 - braggAngle)
// where (x_n, y_n, z_n) is the neutron direction vector in the frame of the CE normal.
// Out of 2 possible solutions for the mosaic vector the one with the smaller mosaic angle is taken,
// since the other one gives an angle of the order of the bragg angle.
double Monochromator::CalculateReflectionProbability(double pi2_braggAngle, VectorType neutronDir)
{

  double tanM, tanMtilde, sinM, cosM, cBragg, alpha, beta, gamma;
  double mosaicAngle1, theta, phi;
  double norm;  

  double x1, x2;
  double angle11, angle12, angle21, angle22;

  double nTries = 1.;
  double maxNTries = (fRndm[2]/(maxDeviation/braggAngleTot*pi2_braggAngle))*5.;

  if (maxNTries < 1) maxNTries = 2;

  double x_n = neutronDir[0];
  double y_n = neutronDir[1];
  double z_n = neutronDir[2];

  VectorType mosaicVector={1, 0, 0};

  while (nTries <= maxNTries) {

    if (mosRndmDir == 1) {

      mosaicAngle1 = DistrGauss(fRndm[1], fRndm[2]);


      tanM = tan(mosaicAngle1);
      tanMtilde = 1 + tanM*tanM;
      cBragg = cos(pi2_braggAngle);
    
      alpha = 2.*tanM*x_n*y_n/(z_n*z_n) + x_n*x_n/(z_n*z_n) + tanM*tanM*y_n*y_n/(z_n*z_n) + tanMtilde;
      beta = -1.*(cBragg*x_n/(z_n*z_n) + cBragg*tanM*y_n/(z_n*z_n));
      gamma = cBragg*cBragg/(z_n*z_n) - 1;
    
      x1 = (-1.*beta + sqrt(beta*beta - alpha*gamma))/alpha;
      x2 = (-1.*beta - sqrt(beta*beta - alpha*gamma))/alpha;

      double y1 = x1*tanM;
      double y2 = x2*tanM;
      
      double z11, z12;
      if (x1*x1 + y1*y1 < 1) {
	z11 = sqrt(1. - x1*x1 - y1*y1);
	z12 = -sqrt(1. - x1*x1 - y1*y1);
      }
      else {
	z11 = 0;
	z12 = 0;
      }
           
      VectorType vMos11 = {x1, y1, z11};
      VectorType vMos12 = {x1, y1, z12};      
      angle11 = AngleVectors(neutronDir, vMos11)/180.*M_PI;
      angle12 = AngleVectors(neutronDir, vMos12)/180.*M_PI;
      
      if (fabs(angle11 - pi2_braggAngle) < 1e-5) {
	phi = atan2(z11,sqrt(x1*x1 + y1*y1));
	CopyVector(vMos11, mosaicVector);
      }
      else if (fabs(angle12 - pi2_braggAngle) < 1e-5) {
	phi = atan2(z12,sqrt(x1*x1 + y1*y1));
	CopyVector(vMos12, mosaicVector);
      }
      else {
	phi = 20.*fNorm[2];
      }

      // Use another solution if the resulting second mosaic angle
      // is too large, most probably there is a solution with a 
      // smaller mosaic angle that leads to a higher weight factor
      if (fabs(phi) > 10.*fNorm[2]) {
      
	double z21, z22;
	if (x2*x2 + y2*y2 < 1) {
	  z21 = sqrt(1. - x2*x2 - y2*y2);
	  z22 = -sqrt(1. - x2*x2 - y2*y2);
	}
	else {
	  z21 = 0;
	  z22 = 0;
	}
      
	VectorType vMos21 = {x2, y2, z21};
	VectorType vMos22 = {x2, y2, z22};      
	angle21 = AngleVectors(neutronDir, vMos21)/180.*M_PI;
	angle22 = AngleVectors(neutronDir, vMos22)/180.*M_PI;
      
	if (fabs(angle21 - pi2_braggAngle) < 1e-5) {
	  phi = atan2(z21,sqrt(x2*x2 + y2*y2));
	  CopyVector(vMos21, mosaicVector);
	}
	else if (fabs(angle22 - pi2_braggAngle) < 1e-5){
	  phi = atan2(z22,sqrt(x2*x2 + y2*y2));
	  CopyVector(vMos22, mosaicVector);
	}
	else {
	  nTries++;
	  continue;
	}
      
      }

      theta = mosaicAngle1;
      // Use the normalisation distribution defined in the NormFunction()
      // method to find a weight for the trajectory taking into account
      // the reflectivity value provided by the user
      norm = fNorm[0]*exp(-sq(phi - fNorm[1])/(2.*sq(fNorm[2])));
      break;
    }

    else {

      mosaicAngle1 = DistrGauss(fRndm[1], fRndm[2]);

      sinM = sin(mosaicAngle1);
      cosM = cos(mosaicAngle1);
      cBragg = cos(pi2_braggAngle);
    
      alpha = 1. + x_n*x_n/(y_n*y_n);
      beta =  sinM*x_n*z_n/(y_n*y_n) - cBragg*x_n/(y_n*y_n);
      gamma = cBragg*cBragg/(y_n*y_n) - cosM*cosM - 2.*cBragg*sinM*z_n/(y_n*y_n) + sinM*sinM*z_n*z_n/(y_n*y_n);
    
      x1 = (-1.*beta + sqrt(beta*beta - alpha*gamma))/alpha;
      x2 = (-1.*beta - sqrt(beta*beta - alpha*gamma))/alpha;

      double z1 = sinM;

      double y11, y12;
      if (z1*z1 + x1*x1 < 1) {
	y11 = sqrt(1 - z1*z1 - x1*x1);
	y12 = -sqrt(1 - z1*z1 - x1*x1);
      }
      else {
	y11 = 0;
	y12 = 0;
      }
    
      VectorType vMos11 = {x1, y11, z1};
      VectorType vMos12 = {x1, y12, z1};      
      angle11 = AngleVectors(neutronDir, vMos11)/180.*M_PI;
      angle12 = AngleVectors(neutronDir, vMos12)/180.*M_PI;

      if (fabs(angle11 - pi2_braggAngle) < 1e-5) {
	theta = atan2(y11,x1);
	CopyVector(vMos11, mosaicVector);
      }
      else if (fabs(angle12 - pi2_braggAngle) < 1e-5) {
	theta = atan2(y12,x1);
	CopyVector(vMos12, mosaicVector);
      }
      else {
	theta = 20.*fNorm[2];
      }


      if (fabs(theta) > 10.*fNorm[2]) {
      
	double y21, y22;
	if (z1*z1 + x2*x2 < 1) {
	  y21 = sqrt(1 - z1*z1 - x2*x2);
	  y22 = -sqrt(1 - z1*z1 - x2*x2);
	}
	else {
	  y21 = 0;
	  y22 = 0;
	}
      
	VectorType vMos21 = {x2, y21, z1};
	VectorType vMos22 = {x2, y22, z1};      
	angle21 = AngleVectors(neutronDir, vMos21)/180.*M_PI;
	angle22 = AngleVectors(neutronDir, vMos22)/180.*M_PI;

	if (fabs(angle21 - pi2_braggAngle) < 1e-5) {
	  theta = atan2(y21,x2);
	  CopyVector(vMos21, mosaicVector);
	}
	else if (fabs(angle22 - pi2_braggAngle) < 1e-5) {
	  theta = atan2(y22,x2);
	  CopyVector(vMos22, mosaicVector);
	}
	else {
	  nTries++;
	  continue;
	}
      
      }

      phi = mosaicAngle1;
      // Use the normalisation distribution defined in the NormFunction()
      // method to find a weight for the trajectory taking into account
      // the reflectivity value provided by the user
      norm = fNorm[0]*exp(-sq(theta - fNorm[1])/(2.*sq(fNorm[2])));
      break;

    }

  }

 
  // Here the new neutron direction is determined
  double mosaicMatrix[3][3];
  RotMatrixX(mosaicVector, mosaicMatrix);
  RotVector(mosaicMatrix, neutronDir);

  neutronDir[0] *= -1.;
  RotBackVector(mosaicMatrix, neutronDir);

#ifdef DEBUG
   double tempTh, tempPh;
   CartesianToSpherical(mosaicVector, &tempTh, &tempPh);
   DEBUG_OUT("Direction of the mosaic vector: tempTh = %f, tempPh = %f, neutronVec:    %f %f %f %f   %f \n", 
    	  tempTh*180./M_PI, tempPh*180./M_PI, angle11*180./M_PI, angle12*180./M_PI, angle21*180./M_PI, angle22*180./M_PI, nTries); 
#endif

  if (nTries < maxNTries) return norm/nTries;
  else return 0.;

}


// Transmit neutron through the monochromator
void Monochromator::TransmitNeutron(Neutron* n)
{
 
  double weightFactor, scalar, ToF;

  weightFactor = exp (-1.*maxDepth*absCoeff);
  n->Probability *= weightFactor;
  
  if (mode == 2)
    scalar = totalXOffset / n->Vector[0];
  else 
    scalar = TranslFoc[0] / n->Vector[0];

  for (int i = 0; i < 3; i++)
    n->Position[i] += n->Vector[i]*scalar;

  ToF = scalar/V_FROM_LAMBDA(n->Wavelength);
  n->Time += ToF;

  WriteNeutron(n);

  return;
}


double Monochromator::CalculateRotationOffset()
{
  double     xOffset1, xOffset2;
  VectorType vec1, vec2;

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


void Monochromator::CopyMatricesToMatrix(int i, int j, std::vector < std::vector<double> >  Matrix[3][3], double Result[3][3])
{

  for(int k = 0; k < 3; k++) {
    for(int l = 0; l < 3; l++) {
      if (Matrix[k][l].size() >= (i+1)) {
	if (Matrix[k][l][i].size() >= (j+1))
	  Result[k][l] = Matrix[k][l][i][j] ;
	else {
	  fprintf(LogFilePtr,"Trying to access non-existing matrix elements in CopyMatricesToMatrix! Abort! \n") ;
	  exit(-1);
	}
      }
      else {
	fprintf(LogFilePtr,"Trying to access non-existing matrix elements in CopyMatricesToMatrix! Abort! \n") ;
	exit(-1);
      }
    }
  }
  return;
}


void Monochromator::CopyMatrixToMatrices(int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3])
{

  for(int k = 0;k < 3; k++) {
    for(int l = 0;l < 3; l++) {
      while (Matrix[k][l].size() < (i+1)) {
	std::vector<double> tempVector;
	Matrix[k][l].push_back(tempVector);
      }
      while (Matrix[k][l][i].size() < (j+1)) Matrix[k][l][i].push_back(0.);
      Matrix[k][l][i][j] = Result[k][l] ;
    }
  }

  return;

}


void Monochromator::CopyVectorsToVector(int i, int j, std::vector < std::vector<double> >  Vector[3], double Result[3])
{
  
  for(int k = 0; k < 3; k++) {
    if (Vector[k].size() >= (i+1)) {
      if (Vector[k][i].size() >= (j+1))
	Result[k] = Vector[k][i][j] ;
      else {
	  fprintf(LogFilePtr,"Trying to access non-existing matrix elements in CopyVectorsToVector! Abort! \n") ;
	  exit(-1);
      }
    }
    else {
      fprintf(LogFilePtr,"Trying to access non-existing matrix elements in CopyVectorsToVector! Abort! \n") ;
      exit(-1);
    }
  }
  
  return;

}

void Monochromator::CopyVectorToVectors(int i, int j, double Vector[3], std::vector < std::vector<double> >  Result[3])
{

  for(int k = 0; k < 3; k++) {
    while (Result[k].size() < (i+1)) {
      std::vector<double> tempVector;
      Result[k].push_back(tempVector);
    }
    while (Result[k][i].size() < (j+1)) Result[k][i].push_back(0.);
    Result[k][i][j] = Vector[k] ;
  }

  return;

}


void Monochromator::declareVectors()
{

  std::vector <double> tempVector;
  PosCE_F[0].push_back(tempVector);
  PosCE_F[1].push_back(tempVector);
  PosCE_F[2].push_back(tempVector);
  DimCE_F[0].push_back(tempVector);
  DimCE_F[1].push_back(tempVector);
  DimCE_F[2].push_back(tempVector);
  RotHoriz_F.push_back(tempVector);
  RotVert_F.push_back(tempVector);
  
  return;

}

void Monochromator::crys_geomLambda()
{

  int		m, j, k ;
  double	b, c, R1, R11, R2, R20, RotView[3][3] ;
  double	Theta, Theta0, Phi, DeltaPhi;

  VectorType	r, Step_H, Step_V ;

  fprintf(LogFilePtr,"\ngeometry: vertical lambda focussing\n\n") ;

  ParGeomN = 2 ;

  FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ;
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if(ParGeom[0] == 0.)
    {
      fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;
      exit(0) ;
    }


  ParGeom[1] *= M_PI / 180. ;
  R20 = 0. ;

  /* computes CE parameters */
  b = - DimCE[2] * (double) cos(M_PI_2 + RotVert);
  c = - ( sq(ParGeom[0]) + ParGeom[0] * DimCE[2] * (double) cos(M_PI_2 + RotVert) );
  R2 = ( - b + sqrt(sq(b) - 4. * c) ) / 2.;
  R11 = (double) sqrt( sq(ParGeom[0]) + sq(DimCE[2] / 2.) + ParGeom[0] * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;

  Theta0 = (double) acos( (sq(ParGeom[0]) + sq(R11) - sq(DimCE[2] / 2.)) / 2. / ParGeom[0] / R11 )
    + (double) acos( (sq(R11) + sq(R2) - sq(DimCE[2] / 2.)) / 2. / R11 / R2 );

  for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
    {
      R2 = ParGeom[0] /* initial R1 */ ;
      Theta = M_PI_2 - ParGeom[1] + Theta0 ;
      DeltaPhi = 2. * (double) atan(DimCE[1] / 2. / R2) ;
      Phi = ((NumberCE[0] - 1) / 2. - m) * DeltaPhi ;

      declareVectors();

      for(j = 0;j<NumberCE[1];j++)	/* step vertical */
	{

	  R1 = R2 ;
	  b = - DimCE[2] * (double) cos(M_PI_2 + RotVert) ;
	  c = - ( sq(R1) + R1 * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;
	  R2 = ( - b + sqrt(sq(b) - 4. * c) ) / 2.  ;
	  R11 = (double) sqrt( sq(R1) + sq(DimCE[2] / 2.) + R1 * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;

	  Theta = Theta	- (double) acos( (sq(R1) + sq(R11) - sq(DimCE[2] / 2.)) / 2. / R1 / R11 )
	    - (double) acos( (sq(R11) + sq(R2) - sq(DimCE[2] / 2.)) / 2. / R11 / R2 ) ;

	  /* computes x translation at the end */
	  if(fabs(Theta - M_PI_2) <= (double) atan(DimCE[2] / 2. / R2)) R20 = R2 ;

	  /* output focussing geometry parameters */
	  PosCE_F[0][m].push_back( R2 * (double) sin(Theta) * (double) cos(Phi) );
	  PosCE_F[1][m].push_back( R2 * (double) sin(Theta) * (double) sin(Phi) );
	  PosCE_F[2][m].push_back( R2 * (double) cos(Theta) );


	  if( (m != 0) && (j != 0) )
	    {

	      for(k=0;k<3;k++)
		{
		  Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
		  Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
		}

	      DimCE_F[0][m].push_back( 0.) ;
	      DimCE_F[1][m].push_back(0.9 * (LengthVector(Step_H) - DimCE[1]));
	      DimCE_F[2][m].push_back(0.9 * (LengthVector(Step_V) - DimCE[2]));

	    } 

	  else {
	    for(k=0;k<3;k++) DimCE_F[k][m].push_back(0.) ;
	  }

	  RotHoriz_F[m].push_back( (Phi) * 180. / M_PI            + MonteCarlo(-0.5*DevH, 0.5*DevH));
	  RotVert_F [m].push_back((M_PI_2 - Theta) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));

	}
    }

  /* print to file */
  Foc_Crys = fopen(GeomFileName, "w") ;
  fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

  for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
    {
      for(j = 0;j<NumberCE[1];j++)	/* step vertical */
	{

	  PosCE_F[0][m][j] -= R20 ;/**/

	  /* rotate  */
	  CopyVectorsToVector(m, j, PosCE_F, r) ;
	  RotVector(RotMatrixCE, r) ;
	  CopyVectorToVectors(m, j, r, PosCE_F) ;

	  fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",
		  PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
	}
    }

  if(Foc_Crys != NULL)fclose(Foc_Crys) ;

  return ;
}


void Monochromator::OwnCleanup()
{

  if (mode == 1 && firstElement) Cleanup(TranslFoc[0], TranslFoc[1], TranslFoc[2], AnglFocHoriz, AnglFocVert);
  else if (mode == 2) Cleanup(totalXOffset, 0, 0, 0, 0);
  else Cleanup(0, 0, 0, 0, 0);

  return;
 
} 
/*******************************************************/
/* 'sphere-focussing' option                           */
/*******************************************************/

void	Monochromator::crys_geomSphere()
{
  int	    m, j, k, q ;
  double    R, Theta, Phi, DeltaPhi, RotView[3][3], DistRows;         // distance between 2 rows (= slab height + gap)  ;
  VectorType	r, Step_H, Step_V ;

  fprintf(LogFilePtr,"\ngeometry: focussing sphere\n\n") ;

  ParGeomN = 2 ;
  DistRows = DimCE[2] + GapV;

  FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ; 
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if(ParGeom[0] == 0.)
    {
      fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;
      exit(0) ;
    }

  ParGeom[1]		= ParGeom[1] * M_PI / 180. ;
  R = (double) sqrt(sq(ParGeom[0]) - sq(DistRows / 2.)) ; 

  /*computes CE parameters */
  for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
    {

      Theta = M_PI_2 - ParGeom[1] + 2 * (double) asin (DistRows / 2. / R);
      DeltaPhi = 2. * (double) atan(DimCE[1] / 2. / R) ;
      Phi = ((NumberCE[0]-1) / 2. - m) * DeltaPhi ;

      declareVectors();  

      for(j = 0;j<NumberCE[1];j++)	/* step vertical */
	{

	  Theta = Theta - 2 * (double) asin (DistRows / 2. / R) ;

	  /* output focussing geometry parameters */
	  r[0] = R * (double) sin(Theta) * (double) cos(Phi) - R ;
	  r[1] = R * (double) sin(Theta) * (double) sin(Phi) ;
	  r[2] = R * (double) cos(Theta) ;

	  for(q=0;q<3;q++) PosCE_F[q][m].push_back(r[q]) ;

	  if( (m != 0) && (j != 0) )
	    {

	      for(k=0;k<3;k++)
		{
		  Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
		  Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
		}

	      DimCE_F[0][m].push_back(0.) ;	
	      DimCE_F[1][m].push_back(0.9 * (LengthVector(Step_H) - DimCE[1]));	
	      DimCE_F[2][m].push_back(0.9 * (LengthVector(Step_V) - DimCE[2]));

	    } 
	  else {

	    for(k=0;k<3;k++) DimCE_F[k][m].push_back( 0.) ;	

	  }

	  RotHoriz_F[m].push_back((Phi) * 180. / M_PI            + MonteCarlo(-0.5*DevH, 0.5*DevH));
	  RotVert_F[m].push_back( (M_PI_2 - Theta) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));

	}
    }


  /* print to file */

  Foc_Crys = fopen(GeomFileName, "w") ;
  fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

  for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
    {
      for(j = 0;j<NumberCE[1];j++)	/* step vertical */
	{

	  /*PosCE_F[0][m][j] += R ;*/
	  /* rotate  */
	  CopyVectorsToVector(m, j, PosCE_F, r) ;
	  RotVector(RotMatrixCE, r) ;
	  CopyVectorToVectors(m, j, r, PosCE_F) ;

	  fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",
		  PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
	}
    }

  if(Foc_Crys != NULL)fclose(Foc_Crys) ;

  return ;

}/* End Crys_GeomSphere */


/*******************************************************/
/* 'vertical cylinder-focussing'  option               */
/*******************************************************/
void   Monochromator::crys_geomVertCyl()
{
  int		m, j, k, q ;
  double	R, Theta, RotView[3][3], DistRows;         // distance between 2 rows (= slab height + gap)  ;
  VectorType	r ;

  fprintf(LogFilePtr,"\ngeometry: vertically focussing cylinder\n\n") ;

  ParGeomN = 2 ;
  DistRows = DimCE[2] + GapV;

  FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ; 
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if(ParGeom[0] == 0.)
    {
      fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;
      exit(0) ;
    }

  ParGeom[1] = ParGeom[1] * M_PI / 180. ;
  R = (double) sqrt(sq(ParGeom[0]) - sq(DistRows / 2.)) ; 

  /*computes CE parameters */
  m = 0;		/* step horizontal */
  Theta = M_PI_2 - ParGeom[1] + 2 * (double) asin (DistRows / 2. / R);

  declareVectors();

  for(j = 0;j<NumberCE[1];j++)	/* step vertical */
    {

      Theta = Theta - 2 * (double) asin (DistRows / 2. / R) ;

      /* output focussing geometry parameters */
      r[0] = R * (double) sin(Theta) - R ;
      r[1] = 0. ; 
      r[2] = R * (double) cos(Theta) ;

      for(q=0;q<3;q++) PosCE_F[q][m].push_back (r[q]) ;

      for(k=0;k<3;k++) DimCE_F[k][m].push_back ( 0.) ;	

      RotHoriz_F[m].push_back(MonteCarlo(-0.5*DevH, 0.5*DevH));
      RotVert_F[m].push_back(MonteCarlo(-0.5*DevV, 0.5*DevV) + (M_PI_2 - Theta) * 180. / M_PI );

    }
  /* print to file */

  Foc_Crys = fopen(GeomFileName, "w") ;
  fprintf(Foc_Crys,"%d %d\n", 1, NumberCE[1]) ;/**/

  for(j = 0;j<NumberCE[1];j++)	/* step vertical */
    {

      /*PosCE_F[0][m][j] += R ;*/
      /* rotate */

      CopyVectorsToVector(m, j, PosCE_F, r) ;
      RotVector(RotMatrixCE, r) ;
      CopyVectorToVectors(m, j, r, PosCE_F) ;

      fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",
	      PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
    }

  if(Foc_Crys != NULL)fclose(Foc_Crys) ;

  return ;

}/* End Crys_GeomVertCyl */


/*******************************************************/
/* 'double focussing cylinder'  option               */
/*******************************************************/
void   Monochromator::crys_geomDoubleCyl()
{

	int	    m, j, k, q ;
	double	    SlabWidth,   /* width and height of one monochromatro element */
	            SlabHeight,
	            Zeta,        /* vertical angle to monochromator slab under consideration */
	            Phi,         /* horizontal angle to monochromator slab under consideration */
	            RadV,        /* radius of vertically focussing cylinder   */
	  RadH;        /* radius of horizontally focussing cylinder */
	  //        DelZeta=0.0, /* difference in vert. orientation between neighbouring rows                        */
	  //        DelPhi=0.0;  /* difference in hor. orientation between neighbouring columns                      */

	VectorType  r;

	fprintf(LogFilePtr,"\ngeometry: cylinder focussing vertically and horizontally\n\n") ;

	FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ; 

	ParGeomN   =  2;
	SlabWidth  =  DimCE[1];
	SlabHeight =  DimCE[2];
	RadV       =  ParGeom[0];
	RadH       =  ParGeom[2];

	// if (RadV > 0.0)
	// 	DelZeta =  2.0 * asin(0.5*(SlabHeight+GapV) / RadV);
	// if (RadH > 0.0)
	// 	DelPhi  =  2.0 * asin(0.5*(SlabWidth +GapH) / RadH); 

	if(RadV == 0.0 && RadH==0.0)
		Warning("both radii are zero");

	/*computes CE parameters */
	for(m = 0;m<NumberCE[0];m++)	   /* step horizontal, loop over columns */
	{
		r[0] =  0.0 ;
		r[1] = (NumberCE[0] - 1 - 2*m)/2.0 * (SlabWidth+GapH);
		// Phi = PhiMax - m * DelPhi;
		if (RadH > 0.0)	Phi = atan(r[1] / RadH);
		else Phi = 0.0;

		declareVectors();
		
		for(j = 0;j<NumberCE[1];j++)	/* step vertical,   loop over rows */
		{
			/* output focussing geometry parameters */
			r[2] = (NumberCE[1] - 1 - 2*j)/2.0 * (SlabHeight+GapV);
		
			if (RadV > 0.0)
				Zeta = atan(r[2] / RadV);
			else
				Zeta = 0.0;

			for(q=0;q<3;q++) PosCE_F[q][m].push_back( r[q]) ;
			for(k=0;k<3;k++) DimCE_F[k][m].push_back(0.0 );	

			RotHoriz_F[m].push_back( Phi * 180./M_PI + MonteCarlo(-0.5*DevH, 0.5*DevH));
			RotVert_F [m].push_back( Zeta * 180./M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));		}
	}

	/* print to file */

	Foc_Crys = fopen(GeomFileName, "w") ;
	fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

	for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
	{
		for(j = 0;j<NumberCE[1];j++)	/* step vertical */
		{

		/* rotate */
		CopyVectorsToVector(m, j, PosCE_F, r) ;
		RotVector(RotMatrixCE, r) ;
		CopyVectorToVectors(m, j, r, PosCE_F) ;

		fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",
				PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
		}
	}

	if(Foc_Crys != NULL)fclose(Foc_Crys) ;
	return ;

}/* End crys_geomDoubleCyl */


#endif
