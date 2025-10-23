/* The free non-commercial use of these routines is granted */
/* providing due credit is given to the authors.            */
/* Author: Géza Zsigmond, last change SEP 2003              */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "init.h"
#include "monochr_analyser.h"
#include "matrix.h"


/*******************************************************/
/* This routine calculates the input geometry for the  */
/* monochromator/analyser module  and can be         */
/* exchanged                                     .     */
/* The output file 'anything' gives the deviation of   */
/* position, dimension, angular offset of the CE     */
/* relative to the main values read from the parameter */
/* file.                         */
/* Additional information: VITESS Help.           */
/*******************************************************/


/*******************************************************/
/* 'Lambda-focussing' i.e. the constant lambda option  */
/*******************************************************/

void  crys_geomLambda()
{
  int      m, j, k ;
  double    b, c, R1, R11, R2, R20, RotView[3][3] ;
  double    Theta, Theta0, Phi, DeltaPhi;

VectorType  r, Step_H, Step_V ;


  fprintf(LogFilePtr,"\ngeometry: vertical lambda focussing\n\n") ;


  ParGeomN = 2 ;


  FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ;

  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;


  if(ParGeom[0] == 0.)
  {
    fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;

    exit(0) ;
  }


  ParGeom[1]    = ParGeom[1] * M_PI / 180. ;

  R20 = 0. ;


  /* computes CE parameters */


  b = - DimCE[2] * (double) cos(M_PI_2 + RotVert) ;

  c = - ( sq(ParGeom[0]) + ParGeom[0] * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;

  R2 = ( - b + sqrt(sq(b) - 4. * c) ) / 2.  ;

  R11 = (double) sqrt( sq(ParGeom[0]) + sq(DimCE[2] / 2.) + ParGeom[0] * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;


  Theta0 = (double) acos( (sq(ParGeom[0]) + sq(R11) - sq(DimCE[2] / 2.)) / 2. / ParGeom[0] / R11 )

      + (double) acos( (sq(R11) + sq(R2) - sq(DimCE[2] / 2.)) / 2. / R11 / R2 );



  for(m = 0;m<NumberCE[0];m++)    /* step horizontal */
  {

  R2 = ParGeom[0] /* initial R1 */ ;

  Theta = M_PI_2 - ParGeom[1] + Theta0 ;


  DeltaPhi = 2. * (double) atan(DimCE[1] / 2. / R2) ;

  Phi = ((NumberCE[0] - 1) / 2. - m) * DeltaPhi ;


    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
    {

    R1 = R2 ;


    b = - DimCE[2] * (double) cos(M_PI_2 + RotVert) ;

    c = - ( sq(R1) + R1 * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;

    R2 = ( - b + sqrt(sq(b) - 4. * c) ) / 2.  ;

    R11 = (double) sqrt( sq(R1) + sq(DimCE[2] / 2.) + R1 * DimCE[2] * (double) cos(M_PI_2 + RotVert) ) ;


    Theta = Theta  - (double) acos( (sq(R1) + sq(R11) - sq(DimCE[2] / 2.)) / 2. / R1 / R11 )

            - (double) acos( (sq(R11) + sq(R2) - sq(DimCE[2] / 2.)) / 2. / R11 / R2 ) ;


    /* computes x translation at the end */

    if(fabs(Theta - M_PI_2) <= (double) atan(DimCE[2] / 2. / R2)) R20 = R2 ;

    /* output focussing geometry parameters */

    PosCE_F[0][m][j] = R2 * (double) sin(Theta) * (double) cos(Phi) ;

    PosCE_F[1][m][j] = R2 * (double) sin(Theta) * (double) sin(Phi) ;

    PosCE_F[2][m][j] = R2 * (double) cos(Theta) ;


    if( (m != 0) && (j != 0) )
    {

      for(k=0;k<3;k++)
      {
        Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
        Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
      }

    DimCE_F[0][m][j] = 0. ;
    DimCE_F[1][m][j] = 0.9 * (LengthVector(Step_H) - DimCE[1]);
    DimCE_F[2][m][j] = 0.9 * (LengthVector(Step_V) - DimCE[2]);

    } else {

      for(k=0;k<3;k++) DimCE_F[k][m][j] = 0. ;

    }


    RotHoriz_F[m][j] = (Phi) * 180. / M_PI            + MonteCarlo(-0.5*DevH, 0.5*DevH);
    RotVert_F [m][j] = (M_PI_2 - Theta) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV);


    }
  }


  /* print to file */

  Foc_Crys = OpenOutputFile(GeomFileName, TRUE, "w") ;

  fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

  for(m = 0;m<NumberCE[0];m++)    /* step horizontal */
  {
    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
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

  if (Foc_Crys != NULL) {
    fclose(Foc_Crys);
    Foc_Crys = NULL;
  }
}/* End Crys_GeomLambda */


/*******************************************************/
/* 'sphere-focussing' option                           */
/*******************************************************/


void  crys_geomSphere()
{
int      m, j, k, q ;
double    R, Theta, Phi, DeltaPhi, RotView[3][3],
            DistRows;         // distance between 2 rows (= slab height + gap)  ;
VectorType  r, Step_H, Step_V ;

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

  ParGeom[1]    = ParGeom[1] * M_PI / 180. ;


  R = (double) sqrt(sq(ParGeom[0]) - sq(DistRows / 2.)) ;

   /*computes CE parameters */


  for(m = 0;m<NumberCE[0];m++)    /* step horizontal */
  {

  Theta = M_PI_2 - ParGeom[1] + 2 * (double) asin (DistRows / 2. / R);

  DeltaPhi = 2. * (double) atan(DimCE[1] / 2. / R) ;

  Phi = ((NumberCE[0]-1) / 2. - m) * DeltaPhi ;


    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
    {

    Theta = Theta - 2 * (double) asin (DistRows / 2. / R) ;

    /* output focussing geometry parameters */

    r[0] = R * (double) sin(Theta) * (double) cos(Phi) - R ;
    r[1] = R * (double) sin(Theta) * (double) sin(Phi) ;
    r[2] = R * (double) cos(Theta) ;


    for(q=0;q<3;q++) PosCE_F[q][m][j] = r[q] ;

    if( (m != 0) && (j != 0) )
    {

      for(k=0;k<3;k++)
      {
        Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
        Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
      }

    DimCE_F[0][m][j] = 0. ;
    DimCE_F[1][m][j] = 0.9 * (LengthVector(Step_H) - DimCE[1]);
    DimCE_F[2][m][j] = 0.9 * (LengthVector(Step_V) - DimCE[2]);

    } else {

      for(k=0;k<3;k++) DimCE_F[k][m][j] = 0. ;

    }


    RotHoriz_F[m][j] = (Phi) * 180. / M_PI            + MonteCarlo(-0.5*DevH, 0.5*DevH);
    RotVert_F [m][j] = (M_PI_2 - Theta) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV);




    }
  }


  /* print to file */

  Foc_Crys = OpenOutputFile(GeomFileName, TRUE, "w") ;

  fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

  for(m = 0;m<NumberCE[0];m++)    /* step horizontal */
  {
    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
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

  if (Foc_Crys != NULL) {
    fclose(Foc_Crys);
    Foc_Crys = NULL;
  }
}/* End Crys_GeomSphere */


/*******************************************************/
/* 'vertical cylinder-focussing'  option               */
/*******************************************************/


void  crys_geomVertCyl()
{
int      m, j, k, q ;
double    R, Theta, RotView[3][3],
            DistRows;         // distance between 2 rows (= slab height + gap)  ;
VectorType  r ;


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

  ParGeom[1]    = ParGeom[1] * M_PI / 180. ;


  R = (double) sqrt(sq(ParGeom[0]) - sq(DistRows / 2.)) ;


   /*computes CE parameters */


  m = 0;    /* step horizontal */

  Theta = M_PI_2 - ParGeom[1] + 2 * (double) asin (DistRows / 2. / R);


    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
    {

    Theta = Theta - 2 * (double) asin (DistRows / 2. / R) ;

    /* output focussing geometry parameters */

    r[0] = R * (double) sin(Theta) - R ;
    r[1] = 0. ;
    r[2] = R * (double) cos(Theta) ;


    for(q=0;q<3;q++) PosCE_F[q][m][j] = r[q] ;

    for(k=0;k<3;k++) DimCE_F[k][m][j] = 0. ;

    RotHoriz_F[m][j] = MonteCarlo(-0.5*DevH, 0.5*DevH) ;

    RotVert_F [m][j] = MonteCarlo(-0.5*DevV, 0.5*DevV) + (M_PI_2 - Theta) * 180. / M_PI ;

  }


  /* print to file */

  Foc_Crys = OpenOutputFile(GeomFileName, TRUE, "w") ;

  fprintf(Foc_Crys,"%d %d\n", 1, NumberCE[1]) ;/**/

    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
    {

    /*PosCE_F[0][m][j] += R ;*/

    /* rotate */

    CopyVectorsToVector(m, j, PosCE_F, r) ;

    RotVector(RotMatrixCE, r) ;

    CopyVectorToVectors(m, j, r, PosCE_F) ;

    fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",

        PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
    }

  if (Foc_Crys != NULL) {
    fclose(Foc_Crys);
    Foc_Crys = NULL;
  }
}/* End Crys_GeomVertCyl */


/*******************************************************/
/* 'double focussing cylinder'  option               */
/*******************************************************/
void  crys_geomDoubleCyl()
{
  int      m, j, k, q ;
  double    SlabWidth,   /* width and height of one monochromatro element */
              SlabHeight,
              Zeta,        /* vertical angle to monochromator slab under consideration */
              Phi,         /* horizontal angle to monochromator slab under consideration */
              RadV,        /* radius of vertically focussing cylinder   */
          RadH,        /* radius of horizontally focussing cylinder */
          // ZetaMax,     /* angle to bottom slab of vertically focussing cylinder, usually > 0.0       [rad] */
          // PhiMax,      /* angle to rightmost slab of horinzontally focussing cylinder, usually > 0.0 [rad] */
              DelZeta=0.0, /* difference in vert. orientation between neighbouring rows                        */
              DelPhi=0.0;  /* difference in hor. orientation between neighbouring columns                      */
  VectorType  r ;

  fprintf(LogFilePtr,"\ngeometry: cylinder focussing vertically and horizontally\n\n") ;

  FillRotMatrixZY(RotMatrixCE, - RotVert, - RotHoriz) ;

  ParGeomN   =  2;
  SlabWidth  =  DimCE[1];
  SlabHeight =  DimCE[2];
  RadV       =  ParGeom[0];
  RadH       =  ParGeom[2];
  // ZetaMax    = -ParGeom[1] * M_PI / 180.;
  // PhiMax     = -ParGeom[3] * M_PI / 180.;
  if (RadV > 0.0)
    DelZeta =  2.0 * asin(0.5*(SlabHeight+GapV) / RadV);
  if (RadH > 0.0)
    DelPhi  =  2.0 * asin(0.5*(SlabWidth +GapH) / RadH);

  if(RadV == 0.0 && RadH==0.0)
    Warning("both radii are zero");


  /*computes CE parameters */
  for(m = 0;m<NumberCE[0];m++)     /* step horizontal, loop over columns */
  {
    r[0] =  0.0 ;
    r[1] = (NumberCE[0] - 1 - 2*m)/2.0 * (SlabWidth+GapH);
    // Phi = PhiMax - m * DelPhi;
    if (RadH > 0.0)
      Phi = atan(r[1] / RadH);
    else
      Phi = 0.0;

    for(j = 0;j<NumberCE[1];j++)  /* step vertical,   loop over rows */
    {
      /* output focussing geometry parameters */
      r[2] = (NumberCE[1] - 1 - 2*j)/2.0 * (SlabHeight+GapV);
      // Zeta = ZetaMax - j * DelZeta ;
      if (RadV > 0.0)
        Zeta = atan(r[2] / RadV);
      else
        Zeta = 0.0;

      for(q=0;q<3;q++) PosCE_F[q][m][j] = r[q] ;
      for(k=0;k<3;k++) DimCE_F[k][m][j] = 0.0 ;

      RotHoriz_F[m][j] =  Phi * 180./M_PI + MonteCarlo(-0.5*DevH, 0.5*DevH);
      RotVert_F [m][j] = Zeta * 180./M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV);
    }
  }


  /* print to file */

  Foc_Crys = OpenOutputFile(GeomFileName, TRUE, "w") ;

  fprintf(Foc_Crys,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

  for(m = 0;m<NumberCE[0];m++)    /* step horizontal */
  {

    for(j = 0;j<NumberCE[1];j++)  /* step vertical */
    {

    /*PosCE_F[0][m][j] += R ;*/

    /* rotate */

    CopyVectorsToVector(m, j, PosCE_F, r) ;

    RotVector(RotMatrixCE, r) ;

    CopyVectorToVectors(m, j, r, PosCE_F) ;

    fprintf(Foc_Crys,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",

        PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotHoriz_F[m][j], RotVert_F[m][j] ) ;
    }
  }

  if (Foc_Crys != NULL) {
    fclose(Foc_Crys);
    Foc_Crys = NULL;
  }
}/* End crys_geomDoubleCyl */
