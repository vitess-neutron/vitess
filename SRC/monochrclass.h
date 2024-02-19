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

#include "mathvector.h"
#include "mathmatrix.h"


class Monochromator{

  public:
  
  // Member variables
  // ----------------
  McCompID   eModule;

  // Monochromator variables from the main window
  char       *ParFileName,             // -P        [-]   Name of the parameter file (orientation, positione, size of a crystal element) 
             *GeomFileName;            // -G        [-]   Name of the geometry file  (arrangement of the crystal elements)        
                                                   
  VtMonoType    eMonoMode;             // -X        [-]   Monochr. geometry:     1: reflection,      2: transmission,      
  VtMonoArrange eGeomOption;           // -O        [-]   Geometry option:       1: single element   2: geometry calculated  3: geometry from file
  VtMonoMove    eMonoMove;             // -b        [-]   Monochr. movement:     0: none             1: vert. rotation       2: hor. rotation (PST)   3: hor. oscillation (Doppler)
  short      bTransm,                  // -B        [-]   Treat transmitted beam 1: yes              0: no
             bRndTof;                  // -K        [-]   flag: time of arrival is set by a random choice within the period of the monochromator movement
  double     Freq,                     // -f       [1/s]  Frequency of the monochromator rotation or translation
             Zeta0,                    // -p       [deg]  zero time orientation of the moving monochromator
             AmplDop,                  // -Q       [cm]   For Doppler drive only: Amplitude of the Doppler drive along x axis (= max. distance from zero position)
             RadiusPST,                // -w       [cm]   For PST only: Distance from the chopper axle to the center of the monochromator
             AreaWidthPST;             // -q       [deg]  For PST only: angular range for each area, where the monochromator is mounted on the chopper
  int        nAreasPST,                // -n        [-]   For PST only: number of identical areas, where the monochromator is mounted on the chopper
             nRepete;                  // -A        [-]   Number of times the neutrons is reflected at the monochromator
  double     mosaic_fwhm[2];           // -m -M    [deg]  Horizontal and vertical mosaicity 
  VtDistr    d_spr_option;             // -d        [-]   d-spacing distribution function        1: Lorentzian           2: Gaussian
  double     d_fwhm,                   // -D        [-]   relative d-spread del_d/d (fwhm)   (multiplied by d to get del_d after read in)
             mu_abs,                   // -C      [1/cm]  Macrosc. absorption cross-section in the crystal for 1.798 Ang  .
             mu_scat,                  // -c      [1/cm]  Macrosc. total scattering cross-section in the crystal          .
             Reflectivity;             // -R        [-]   Peak Reflectivity 
  VtMonoFocus eFocGeom;                // -g        [-]   Focusing geometry:     1: constant lambda  2: spherical            3: vert. cylinder     4: double focussing
  int        NumberCE[2];              // -H -V     [-]   Number of horizontal and vertical CE segments
  double     DevH, DevV;               // -t -T    [deg]  Horizontal and vertical deviation from correct crystal orientation
  double     GapH, GapV;               // -h -v    [cm]   Horizontal and vertical distance between crystal elements
  double     RadH, RadV,               // -s -r    [cm]   Horizontal and vertical radius
             Psi0;                     // -a       [deg]  Angular offset of the bottom row of the crystal elements   
                                       
  // Monochromator variables from main  window or parameter file
  double	   BraggHor,  BraggVert,     // -l -L    [deg]  Horizontal and vertical crystal plane orientation (relative backscattering direction)
             SrfcHor,   SrfcVert,      // -e -E    [deg]  Horizontal and vertical surface orientation (relative backscattering direction)
             OutHor,    OutVert;       // -u -U    [deg]  Horizontal and vertical orientation of the output co-ordinate system
  VectorType Transl,                   // -W -Y -Z [cm]   Position [x,y,z] of the output co-ordinate system 
             PosCE0,                   // -x -y -z [cm]   Position [x,y,z] of the center of the monochromator
             DimCE0;                   // -i -j -k [cm]   thickness, width and height of a monochromator crystal element
  VtFrameGen eFrame;                   // -F       [-]    flag: 'user defined frame' 1: yes   0: no
  double     d_spacing;                // -S       [cm]   Distance of the (h,k,l) crystal planes
  int        nOrderRefl;               // -N       [-]    Order of Bragg reflection (usually 1), -1 means all

  // used: a b c d e f g h i j k l m n   p q r s t u v w x y z
  //       A B C D E F G H     K L M N O P Q R S T U V W X Y Z
  // free:                             o
  //                       I J
                                              
  // Monochromator variables from the  geometry file or from calculation
  std::vector < std::vector<double> >         
             RotCEhor_F,               //          [rad]  vector containing the horizontal orientations of all monochromator elements           
             RotCEvert_F,              //          [rad]  vector containing the vertical orientations of all monochromator elements             
             PosCE_F[3],               //          [cm]   3 vectors containing the x-pos., y-pos. and z-pos. of all monochromator elements      
             DimCE_F[3],               //          [cm]   3 vectors containing the thicknesses, widths and heights of all monochromator elements
             RotMatrixCE_F[3][3];

// Variables determined from input parameters or trajectory data
  FILE*      pGeomFile;                //           [-]   Pointer to geometry file
  double     d_sigma;                  //           [cm]  absolute d-spread del_d   (sigma)
  VectorType Depth,                    //                 Vector from CE surface to the reflecting plane 
             PosCE,                    //                 Position [x,y,z] of the center of the current crystal element (CE)
             DimCE;                    //                 thickness, width and height of the current CE
  double     dSpacingSpreadParams  [3],//                 data to treat d-spacing spread
             horMosaicSpreadParams [3],//                 data to treat hor. mosaicity
             vertMosaicSpreadParams[3];//                 data to treat hor. mosaicity
  double     RotMatrixCE   [3][3],     //                 Matrix to rotate the trajectory into the system of the current crystal element
             RotMatrixCE0  [3][3],     //                 Matrix to rotate the trajectory into the system of the central crystal element
             RotMatrixBragg[3][3],     //                 Matrix to rotate the trajectory into the system of the reflecting planes
             RotMatrixOut  [3][3];     //                 Matrix to rotate the trajectory into output frame
  double     PathLenTrans,             //                 Length of the trajectory through the crystal for the transmitted
             PathLenRefl,              //                   and the reflected beam
             maxDeviation;             //                 max. angle deviation considered for the reflectivity normalization
  double     peakWL,                   //                 Peak wavelength
             braggAngleTot,            //                 total bragg angle (in case braggHor > 0 and braggVer > 0), 
             axisPhi;                  //                 spherical angle Phi of the instrument axis in the Bragg frame, needed for the normalisation procedure
  int        mosRndmDir;               //                 defines direction for normal and random mosaicity: 1: vert. norm, hor. rnd  2: vice versa
  double     fNorm[3], fRndm[3],       //                 contains parameters for Gaussian distribution of mosaicity
             Period,                   //           [ms]  period of the monochromator movement
             TrndMin, TrndMax;         //                 minimum and maximum value of the randomized arrival time
                                                          
  // Variables of the trajectories                        
  Neutron*   currentNeutron;                              
  VectorType Pos, Dir;                 //                 position and flight direction
  double     TOF, Prob;                //                 time of flight, weight
  long       NumOut;                   //                 number of written trajectories
                                                          
  // Parameters for the monochromator rotation or oscillation
  double     DelZetaMax,               //  [deg]          max. difference in rotational angle between last and last but one approximation step
             DelXmax;                  //  [cm]           max. difference in Doppler drive position between last and last but one approximation step
  //         vMax;                     // [cm/ms]         max. speed of the Doppler drive or speed of the crystal on PST chopper
  VectorType rDop,                     //  [cm]           position of the Doppler drive at time of impact (in the frame of module)
             vDop;                     // [cm/ms]         speed of the Doppler drive at time of impact (in the frame of module)

  

  // Member functions
  // ----------------
  Monochromator();
  virtual ~Monochromator() {};

  void        OwnInit(int argc, char *argv[]);
  void        setMonochrPar();                  // reads monochromator parameters and combines them with input parameters
  void        calcAndWritePar();                // determines the dependent parameters and write out important parameters 
  void        readFocFile();                    // reads deviations in position, dimensions and orientation of all crystal elements
  void        setGeometry(char* sColor);        // fills the structure stGeometry for visualization
  void        OwnCleanup();

  void        fillRotMatrices(double MonoHor);
  void        AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert) ;
  void        FindPeakLambdaSortMosaicityDistr(double RotHoriz, double RotVert);
  void        NormFunction();
  void        DetermineMosaicAngle(double angleDiff, double mosaicAngle1, double &mosaicAngle2);

  void        processNeutron(Neutron* neutron);  

  bool        rotateMonochr     (int& kLast, int& lLast, const Neutron* pNeutIn);
  bool        translateMonoX    (int& kLast, int& lLast, Neutron* pNeutInM, const Neutron* pNeutIn);
  void        TranslBackMonoX   (Neutron* pNeutIn, Neutron* pNeutOut);
  void        rotateMonoYZ      (Neutron* pNeutIn, Neutron* pNeutRot, double Freq);
  bool        checkCE           (double& Time, const MathVector vPosCE, const VectorType SizeCE, const MathMatrix Mrot, const Neutron* pNeut);
  bool        isNeutInCE        (MathVector vPosN, const VectorType SizeCE);
  bool        selectCE          (const Neutron* pNeutIn, Neutron* pNeutInCE, int kStart=0, int lStart=0);
  double      calcReflProbAndDir(VectorType DirOut, const VectorType DirIn, const double pi2_braggAngle);
  void        propNeutron2CE    (const Neutron* pNeutIn, Neutron* pNeutOut);
  void        transmitNeutron   (const Neutron* pNeutIn, Neutron* pNeutOut);
  
  void        crys_geomLambda();
  void        crys_geomSphere();
  void        crys_geomVertCyl();
  void        crys_geomDoubleCyl();
  void        rotateCEs();
  void        addDev2Std();          // adds deviations in position, dimensions and orientation to standard values for all crystal elements
  void        writeFocData();

  static void	CopyMatricesToMatrix(int i, int j, std::vector < std::vector<double> >  Matrix[3][3], double Result[3][3]) ;
  static void	CopyMatrixToMatrices(int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3]) ;
  static void	CopyVectorsToVector (int i, int j, std::vector < std::vector<double> >  Vector[3], double Result[3]) ;
  static void	CopyVectorToVectors (int i, int j, double Vector[3], std::vector < std::vector<double> >  Result[3]) ;
  void        declareVectors();

  bool        checkPlaneIntersect (const MathVector vLineOffset,  const MathVector vLineDir,
                                   const MathVector vPlaneNormal, const double PlaneDistance, double& t);
};


#endif
