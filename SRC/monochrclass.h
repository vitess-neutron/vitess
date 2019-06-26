#ifndef MONOCHRCLASS_H
#define MONOCHRCLASS_H

/********************************************************************************************/
/*  VITESS module 'monochrclass.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Oct 2013  D. Nekrassov  initial version                                              */
/********************************************************************************************/

#define	PAR_GEOM	10


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>


extern "C" {
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"
}

//#include "mathvector.h"
//#include "mathmatrix.h"



class Monochromator{

  public:
  
  // Member variables
  FILE		*Par_Crys,   *Foc_Crys;  //Parameter files

  char		*ParameterFileName, *GeomFileName ; //Parameter file names

  int		OrderReflection, Option; // Number of reflection orders to process, typically 1
  int           NumberCE[2]; // Number of horizontal and vertical CE segments
  int           ParGeomN, d_spr_option, geom_option, mode;
  int           mosRndmDir;
  int           firstElement;

  long		NumOut, Repetition;

  double        peakWL, braggAngleTot, axisPhi;  //Peak wavelength and total bragg angle (in case braggHor > 0 and braggVer > 0), 
                                                 //spherical angle Phi of the instrument axis in the Bragg frame, needed for the normalisation procedure
  double        maxDeviation;
  double	TOF, Prob;
  double	Index, User, ParGeom[PAR_GEOM], IntegralIntensity ;
  double	d_spacing, d_fwhm, d_sigma, mosaic_fwhm[2], Reflectivity, absCoeff ; // Crystal parameters concerning d-spacing and mosaicity
  double	RotHoriz, RotVert, BraggHoriz, BraggVert;
  double        AnglFocHoriz, AnglFocVert, totalXOffset, rotOffset;
  double        DevH, DevV;              /* horizontal and vertical deviation from correct crystal orientation */
  double        GapH, GapV;              /* horizontal and vertical distance between crystal elements */
  double        maxDepth;
  double        dSpacingSpreadParams[3], horMosaicSpreadParams[3], vertMosaicSpreadParams[3];
  double        fNorm[3], fRndm[3];
  double	RotMatrixCE[3][3], RotMatrixBragg[3][3], RotMatrixFoc[3][3], RotMatrixSurf[3][3];

  std::vector < std::vector<double> >   RotHoriz_F, RotVert_F, PosCE_F[3], DimCE_F[3], RotMatrixCE_F[3][3];
 
  VectorType	Pos, Dir,  Path ;
  VectorType	TranslFoc, TranslFoc_def, PosCE, DimCE, Depth;

  Neutron*	currentNeutron;

  // Member functions

  Monochromator();
  virtual ~Monochromator() {};

  void		Init(int argc, char *argv[]) ;
  void          processNeutron(Neutron* neutron);  
  void		OwnCleanup() ;
  void		ReadParameterFile() ;
  void		ReadFocFile() ;  
  void		FillRotMatrixFoc() ;
  void		SelectCE(double *index, Neutron* n) ;
  void		AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert) ;
  void          FindPeakLambdaSortMosaicityDistr(double RotHoriz, double RotVert);
  void          NormFunction();
  void          DetermineMosaicAngle(double angleDiff, double mosaicAngle1, double &mosaicAngle2);
  void          TransmitNeutron(Neutron* n);
  double        CalculateReflectionProbability(double braggAngle, VectorType neutronDir);
  double        CalculateRotationOffset();
  
  void		crys_geomLambda() ;
  void		crys_geomSphere() ;
  void		crys_geomVertCyl() ;
  void		crys_geomDoubleCyl() ;

  void          declareVectors();
  static void		CopyMatricesToMatrix(int i, int j, std::vector < std::vector<double> >  Matrix[3][3], double Result[3][3]) ;
  static void		CopyMatrixToMatrices(int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3]) ;
  static void		CopyVectorsToVector(int i, int j, std::vector < std::vector<double> >  Vector[3], double Result[3]) ;
  static void		CopyVectorToVectors(int i, int j, double Vector[3], std::vector < std::vector<double> >  Result[3]) ;

};


#endif
