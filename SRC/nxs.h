#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NXS_H
#define NXS_H

#include <stdio.h>

#if defined(__THINK__) || defined(__MWERKS__)
#include <console.h>
#define CONSOLE_LINES   36  /* number of lines to use for console */
#define CONSOLE_COLUMNS 90  /* number of columns to use for console */
#ifdef __MWERKS__
#include <sioux.h>
#endif
#endif

#define AppMalloc(ptr, n) (ptr) = malloc((n) * sizeof (*(ptr)))
#define AppFree(ptr, n) free(ptr)

#define SGCOREDEF__
#include "sginfo.h"

/**
  @author Mirko Boin, Helmholtz Centre Berlin for Materials and Energy <boin@helmholtz-berlin.de>
*/

/*! \file nxs.h
    \brief nxs header file.

    Details.
*/



/**
\struct <EquivHKL>

  \brief struct for symmetry equivalent Miller indices (hkl)

  This struct is used by struct HKL to hold symmetry equivalent hkl.
  \see HKL
*/
typedef struct {
  int h;                           /*!< Miller index */
  int k;                           /*!< Miller index */
  int l;                           /*!< Miller index */
} EquivHKL;


/**
\struct <HKL>

  \brief struct for Miller indices (hkl)

  According to a given reflection this struct stores the hkl indices, its multiplicity, its lattice spacing, its structure factor and the symmetry equivalent hkl indices.
*/
typedef struct {
  int h;                           /*!< Miller index */
  int k;                           /*!< Miller index */
  int l;                           /*!< Miller index */
  unsigned int multiplicity;       /*!< multiplicity of the hkl reflection */
  double dhkl;                     /*!< hkl lattice spacing in &Aring;*/
  double FSquare;                  /*!< \f$|F|^2\f$ (structure factor) */
  EquivHKL *equivHKL;              /*!< holds the symmetry equivalent reflections including the current indices */
} HKL;


/**
\struct <AtomInfo>

  \brief struct for atom descrpition

  This struct stores the position of an atom, its average cross sections, mass and Debye temperature.
*/
typedef struct {
  char wyckoffLetter;     /*!< Wyckoff letter */
  unsigned int nAtoms;    /*!< number of atoms = Wyckoff multiplicity */
  unsigned int elementNumber;  /*!< Element number from PSE */
  char *label;            /*!< Label for the atom */
  double x[192];          /*!< holds the x positions, the first entry is the Wyckoff position */
  double y[192];          /*!< holds the y positions, the first entry is the Wyckoff position */
  double z[192];          /*!< holds the z positions, the first entry is the Wyckoff position */
  double sigmaAbsorption; /*!< in [\f$barn = 10^{-24} cm^2  at \ 2200 \frac{m}{s} = 1.798 \AA\f$] */
  double sigmaCoherent;   /*!< in [\f$barn = 10^{-24} cm^2\f$] */
  double sigmaIncoherent; /*!< in [\f$barn = 10^{-24} cm^2\f$] */
  double b_coherent;
  double molarMass;       /*!< Molar mass in [\f$u = 1.66 \times 10^{-27} kg\f$] */
  double debyeTemp;       /*!< Debye temperature in [K] */
  double phi_1;           /*!< \f$\varphi_1\f$ */
  double phi_3;           /*!< \f$\varphi_3\f$ */
  double B_iso;           /*!< \f$B_{iso}\f$ the isotropic atomic displacement factor */
} AtomInfo;



/**
\struct <UnitCell>

  \brief struct for unit cell descrpition

  This struct stores space group symbol, the lattice parameters, the three sigma values, the atom mass the Debye temperature, some calculated values and the HKL and UnitCell struct as well as the SgInfo struct (see SgInfo documentation on http://cci.lbl.gov/sginfo/).
*/
typedef struct {
  int crystalSystem;         /*!< crysal system: XS_Tetragonal, XS_Hexagonal, XS_Cubic ... */
  const char *spaceGroup;    /*!< space group number or denotation */
  double a;                  /*!< lattice constant a */
  double b;                  /*!< lattice constant b */
  double c;                  /*!< lattice constant c */
  double alpha;              /*!< lattice constant \f$\alpha\f$ */
  double beta;               /*!< lattice constant \f$\beta\f$ */
  double gamma;              /*!< lattice constant \f$\gamma\f$ */
  double avgSigmaCoherent;   /*!< in [\f$barn = 10^{-24} cm^2\f$] */
  double avgSigmaIncoherent; /*!< in [\f$barn = 10^{-24} cm^2\f$] */
  unsigned int nAtoms;       /*!< total number of atoms inside the unit cell */
  unsigned int nAtomInfo;    /*!< number of unit cell atoms */
  AtomInfo *atomInfoList;    /*!< atom info \see AtomInfo */
  T_SgInfo sgInfo;           /*!< struct from SgInfo library needed for further calculations see SgInfo documentation on http://cci.lbl.gov/sginfo/ */
  double volume;             /*!< unit cell volume */
  unsigned int nHKL;         /*!< number of hkl reflections after initUnitCell() */
  unsigned int maxHKL_index; /*!< maximum hkl index */
  HKL *hklList;              /*!< \see HKL */
  double mass;               /*!< unit cell mass [\f$\frac{g}{mol}\f$]*/
  double density;            /*!< unit cell density [\f$\frac{g}{cm^3}\f$]*/
} UnitCell;



/**
\struct <Texture>

  \brief struct for texture descrpition using March-Dollase approach

*/
typedef struct{
  int a;
  int b;
  int c;
  double r;
  double f;
  double **sin_beta;
  double **cos_beta;
  double *P_alpha_H;
} Texture;



/**
\struct <MarchDollase>

  \brief struct for March-Dollase correction calculation

*/
typedef struct{
  unsigned int M;
  unsigned int N;
  unsigned int nOrientations;
  double *sin_phi;
  double *cos_phi;
  Texture *texture;
  UnitCell *unitcell;
} MarchDollase;


double calcDhkl( int h, int k, int l, UnitCell* uc );
double calcFSquare( HKL *hklReflex, UnitCell* uc );
int initUnitCell( UnitCell *uc );
int addAtomInfo( UnitCell *uc, AtomInfo ai );
int initHKL( UnitCell *uc );
double nxsAbsorption         ( double lambda, UnitCell* uc );
double nxsCoherentElastic    ( double lambda, UnitCell* uc );
double nxsCoherentInelastic  ( double lambda, UnitCell* uc );
double nxsTotalInelastic     ( double lambda, UnitCell* uc );
double nxsIncoherentElastic  ( double lambda, UnitCell* uc );
double nxsIncoherentInelastic( double lambda, UnitCell* uc );
double calcPhi_1( double theta );
double calcPhi_3( double theta );
MarchDollase initMarchDollase( UnitCell* uc );
void addTexture( MarchDollase* md, Texture texture );
double nxsCoherentElasticTexture( double lambda, MarchDollase* md );

#endif

#ifdef __cplusplus
}
#endif
