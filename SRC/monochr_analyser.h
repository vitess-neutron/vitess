#ifndef MONOCHR_ANALYSER_H
#define MONOCHR_ANALYSER_H

#include "general.h"

#define  CRYS_SIZE  500
#define  PAR_GEOM  10
#define  STRING_BUFFER 50

/********************************************************************/
/* general definitions for the module crys.c          */


extern FILE    *AsciiFile, *Par_Crys, *Foc_Crys;
extern char    *ParameterFileName, *GeomFileName ;
extern int      OrderReflection, NumberCE[2], ParGeomN, d_spr_option, geom_option, mode;
extern long    NumOut, BufferIndex, Repetition, repet;
extern double    TOF, WL, Prob, d_ran, d_ran_min, d_ran_max, arg, thr, phr, thrmax, phrmax, deltaphr ;
extern double    Matrix[3][3], Option, User, ParGeom[PAR_GEOM], IntegralIntensity ;
extern double    d_spacing, d_fwhm, d_range, mosaic_range, mosaic_fwhm[2], Reflectivity, absCoeff ;
extern double    RotHoriz, RotVert, BraggHoriz, BraggVert, PosCE[3], DimCE[3], Depth[3] ;
extern double    TranslFoc[3], TranslFoc_def[3], AnglFocHoriz, AnglFocVert, totalXOffset, rotOffset ;
extern double    RotMatrixCE[3][3], RotMatrixBragg[3][3], RotMatrixFoc[3][3], RotMatrixSurf[3][3] ;
extern double    RotHoriz_F[CRYS_SIZE][CRYS_SIZE], RotVert_F[CRYS_SIZE][CRYS_SIZE], PosCE_F[3][CRYS_SIZE][CRYS_SIZE], DimCE_F[3][CRYS_SIZE][CRYS_SIZE] ;
extern double    RotMatrixCE_F[3][3][CRYS_SIZE][CRYS_SIZE] ;
extern double      DevH, DevV;              // horizontal and vertical deviation from correct crystal orientation
extern double      GapH, GapV;              // horizontal and vertical distance between crystal elements
extern VectorType  Pos, Dir, Mosaic, Path ;
extern Neutron    Neutrons ;

void    crys_geomLambda() ;
void    crys_geomSphere() ;
void    crys_geomVertCyl() ;
void    crys_geomDoubleCyl() ;
void    OwnInit(int argc, char *argv[]) ;
void    OwnCleanup() ;
void    ReadParameterFile() ;
void    ReadFocFile() ;
void    SetGeometry(char* sColor);    // fills the structure stGeometry for visualization
void    FillRotMatrixFoc(double RotMatrixCE_F[3][3][CRYS_SIZE][CRYS_SIZE], double RotVert_F[CRYS_SIZE][CRYS_SIZE], double RotHoriz_F[CRYS_SIZE][CRYS_SIZE]) ;
void    OutputAscii(double TOF, double WL, double Prob, VectorType Pos, VectorType Dir) ;
void    SelectCE(double *index, int i) ;
void    MosaicMaxProb(VectorType Dir, double *thr, VectorType Mosaic) ;
void    AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert) ;
void    CopyMatricesToMatrix(int i, int j, double Matrix[3][3][CRYS_SIZE][CRYS_SIZE], double Result[3][3]) ;
void    CopyMatrixToMatrices(int i, int j, double Result[3][3], double Matrix[3][3][CRYS_SIZE][CRYS_SIZE]) ;
void    CopyVectorsToVector(int i, int j, double Vector[3][CRYS_SIZE][CRYS_SIZE], double Result[3]) ;
void    CopyVectorToVectors(int i, int j, double Vector[3], double Result[3][CRYS_SIZE][CRYS_SIZE]) ;
double    Mosaicity(VectorType Mosaic) ;
double    dSpreadLorentzian(double d_spacing) ;
double    dSpreadGaussian(double d_spacing) ;
//void            TransmitNeutron(Neutron* n);
//double          CalculateRotationOffset();

/********************************************************************/



#endif
