#ifndef INTERSECTION_H
#define INTERSECTION_H

/*********************************************************/
/* intersection.h                                        */
/* Functions that calculate intersection points with     */
/* various surfaces                                      */
/*********************************************************/

#include "general.h"

#define X(x) ISP[x][0]
#define Y(x) ISP[x][1]
#define Z(x) ISP[x][2]


/* for function 'PathThroughBenderGravOrder2' in module bender */
/* ----------------------------------------------------------- */
double NeutronPlaneAngle2             (const Neutron *, const double, const double, const double);
double NeutronSurfaceSecIntersectionGr(Neutron *, const SurfaceSecond, const long);

/* for several modules */
/* ------------------- */
double NeutronPlaneIntersectionGrav(Neutron *, const Plane);
double NeutronPlaneIntersection1   (Neutron *, const Plane);

double SolveQuadraticEq(double a, double b , double c);


/* for modules 'sample_sans', 'sample_powder', 'monochr_analyser' etc. */
/* ------------------------------------------------------------------- */
int  PlaneLineIntersect (const VectorType LineOffset, const VectorType LineDir, const VectorType PlaneNormalVector, const double PlaneDistane,
                         VectorType Result);
int  PlaneLineIntersect2(const VectorType LineOffset, const VectorType LineDir, const VectorType PlaneNormalVector, const double PlaneDistane,
                         VectorType Result);
long IntersectionWithHorizontalPlane(const double Z, const VectorType PosVect, const VectorType Dir, VectorType Result);
int  OrderPositions(const VectorType Dir, VectorType Pos1, VectorType Pos2);

long IntersectionWithRectangular(const VectorType DimSample, const VectorType Pos, const VectorType Dir,
                                                             VectorType Pos1, VectorType Pos2);

long LineIntersectsCube     (const VectorType Offset, const VectorType Direction, const CubeType *Cube,    double t[2]);
long LineIntersectsHollowCyl(const VectorType Offset, const VectorType Direction, const HolCylType *HCyl,  double t[2], const VtDir eDir);
long LineIntersectsCylinder (const VectorType Offset, const VectorType Direction, const CylinderType *Cyl, double t[2]);
long LineIntersectsSphere   (const VectorType Offset, const VectorType Direction, const BallType *Sphere,  double t[2]);

long IntersectionWithInfiniteCylinder(const double DiameterCyl, const VectorType Pos, const VectorType Dir,
                                                                VectorType Pos1, VectorType Pos2);

long IntersectionWithCylinder(const VectorType DimSample, const VectorType Pos, const VectorType Dir,
                                                          VectorType Pos1, VectorType Pos2);
long IntersectionWithSphere  (const VectorType DimSample, const VectorType Pos, const VectorType Dir,
                                                          VectorType Pos1, VectorType Pos2);

#endif
