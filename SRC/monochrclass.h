#ifndef MONOCHRCLASS_H
#define MONOCHRCLASS_H

/********************************************************************************************/
/*  VITESS module 'monochrclass.h'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Oct 2013  D. Nekrassov  initial version                                              */
/********************************************************************************************/

#define	PAR_GEOM	10
#define M_CUT     1.0e-3

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
  // ----------------

  // Monochromator variables from the main window
  char       *ParFileName,            // -P  Name of the parameter file (orientation, positione, size of a crystal element) 
             *GeomFileName;           // -G  Name of the geometry file  (arrangement of the crystal elements)        
  FILE*      pGeomFile;               //     Pointer to geometry file
  //  *Par_Crys,               //     Pointer to parameter file
       

  int        eGeomOption;             // -O  Geometry option:      1: single element   2: geometry calculated  3: geometry from file
  int        eFocGeom;                // -g  Focusing geometry:    1: constant lambda  2: spherical            3: vert. cylinder     4: double focussing
  int        bTransm;                 // -o  Treat tranmitted beam 1: yes              0: no
  int        eMonoMode;               // -X  Monochr. geometry:    1: reflection,      2: transmission,      
  int        d_spr_option;            // -d  d-spacing distribution function        1: Lorentzian           2: Gaussian
  int        nRepete;                 // -A  Number of times the neutrons is reflected at the monochromator

  double     d_fwhm,                  // -D  relative d-spread del_d/d (fwhm)
             d_sigma,                 //     absolute d-spread del_d   (sigma)
             mu_abs,                  // -C  Macrosc. absorption cross-section in the crystal for 1.798 Ang  [1/cm].
             mu_scat,                 // -C  Macrosc. total scattering cross-section in the crystal          [1/cm].
             Reflectivity;            // -R  Peak Reflectivity 

  int        NumberCE[2];             // -H -V  Number of horizontal and vertical CE segments
  double     mosaic_fwhm[2];          // -m -M  Horizontal and vertical mosaicity 
  double     DevH, DevV;              // -t -T  Horizontal and vertical deviation from correct crystal orientation
  double     GapH, GapV;              // -h -v  Horizontal and vertical distance between crystal elements
  double     RadH, RadV,              // -s -r  Horizontal and vertical radius
             Psi0;                    // -a     Angular offset of the bottom row of the crystal elements  

  // Monochromator variables from the parameter file
  double	   RotHoriz,    RotVert,    //     Horizontal and vertical surface orientation (relative backscattering direction)
             BraggHoriz,  BraggVert,  //     Horizontal and vertical crystal orientation (relative backscattering direction)
             AnglFocHoriz,AnglFocVert;//     Horizontal and vertical orientation of the output co-ordinate system
  VectorType Transl,                  //     Position [x,y,z] of the output co-ordinate system 
             PosCE0,                  //     Position [x,y,z] of the center of the monochromator
             DimCE0;                  //     thickness, width and height of a monochromator crystal element
  int        bUser;                   //     criterion: 'user defined frame' 1: yes   0: no
  double	   d_spacing;               //     Distance of the (h,k,l) crystal planes
  int		     OrderReflection;         //     Order of Bragg reflection (usually 1)

  // Monochromator variables from the geometry file or from calculation
  std::vector < std::vector<double> >   
             RotHoriz_F,              //     vector containing the horizontal orientations of all monochromator elements
             RotVert_F,               //     vector containing the vertical orientations of all monochromator elements
             PosCE_F[3],              //     3 vectors containing the x-pos., y-pos. and z-pos. of all monochromator elements
             DimCE_F[3],              //     3 vectors containing the thicknesses, widths and heights of all monochromator elements
             RotMatrixCE_F[3][3];

  // Calculated variables
  VectorType Depth,                    //    Vector from CE surface to the reflecting plane 
             PosCE,                    //    Position [x,y,z] of the center of the current crystal element (CE)
             DimCE;                    //    thickness, width and height of the current CE
  double     dSpacingSpreadParams  [3],//    data to treat d-spacing spread
             horMosaicSpreadParams [3],//    data to treat hor. mosaicity
             vertMosaicSpreadParams[3];//    data to treat hor. mosaicity
  double     RotMatrixCE   [3][3],     //    Matrix to rotate the trajectory into the system of the central crystal element
             RotMatrixBragg[3][3],     //    Matrix to rotate the trajectory into the system of the 
             RotMatrixFoc  [3][3],     //    Matrix to rotate the trajectory into  central crystal element
             RotMatrixSurf [3][3];     //    Matrix to rotate the trajectory into  central crystal element
  double     PathLenTrans,             //    Length of the trajectory through the crystal for the transmitted
             PathLenRefl,              //      and the reflected beam
             maxDeviation;             //    max. angle deviation considered for the reflectivity normalization
  double     peakWL,                   //    Peak wavelength
             braggAngleTot,            //    total bragg angle (in case braggHor > 0 and braggVer > 0), 
             axisPhi;                  //    spherical angle Phi of the instrument axis in the Bragg frame, needed for the normalisation procedure
  int        mosRndmDir;               //    defines direction for normal and random mosaicity: 1: vert. norm, hor. rnd  2: vice versa
  double     fNorm[3], fRndm[3];       //    contains parameters for Gaussian distribution of mosaicity
/*double     totalXOffset,             //    shift of the co-orddinate system (along x-direction), not used anymore
             rotOffset;                //    not used */

  // Variables of the trajectories 
  Neutron*	 currentNeutron;
  VectorType Pos, Dir;                 //    position and flight direction
  double	   TOF, Prob;                //    time of flight, weight
  long		   NumOut;                   //    number of written trajectories
  

  // Member functions
  // ----------------
  Monochromator();
  virtual ~Monochromator() {};

  void        Init(int argc, char *argv[]);
  void        OwnCleanup();
  void        FillRotMatrixFoc();
  void        ReadParameterFile();   // reads standard values for a crystal elements
  void        ReadFocFile();         // reads deviations in position, dimensions and orientation of all crystal elements
  void        addDev2Std();          // adds deviations in position, dimensions and orientation to standard values for all crystal elements

  void        AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert) ;
  void        FindPeakLambdaSortMosaicityDistr(double RotHoriz, double RotVert);
  void        NormFunction();
  void        DetermineMosaicAngle(double angleDiff, double mosaicAngle1, double &mosaicAngle2);

  void        processNeutron(Neutron* neutron);  

  bool        selectCE          (const Neutron* pNeutIn, Neutron* pNeutInCE);
  void        propNeutron2CE    (const Neutron* pNeutIn, Neutron* pNeutOut);
  void        transmitNeutron   (const Neutron* pNeutIn, Neutron* pNeutOut);
  double      calcReflProbAndDir(VectorType DirOut, const VectorType DirIn, const double pi2_braggAngle);
  double      CalculateRotationOffset();
  
  void        crys_geomLambda();
  void        crys_geomSphere();
  void        crys_geomVertCyl();
  void        crys_geomDoubleCyl();
  void        rotateCEs();
  void        writeFocData();

  static void	CopyMatricesToMatrix(int i, int j, std::vector < std::vector<double> >  Matrix[3][3], double Result[3][3]) ;
  static void	CopyMatrixToMatrices(int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3]) ;
  static void	CopyVectorsToVector (int i, int j, std::vector < std::vector<double> >  Vector[3], double Result[3]) ;
  static void	CopyVectorToVectors (int i, int j, double Vector[3], std::vector < std::vector<double> >  Result[3]) ;
  void        declareVectors();

};


#endif
