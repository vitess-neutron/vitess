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


typedef enum
{	
	VT_IN     = 1,
	VT_OUT    = 2,
	VT_INSIDE = 3
}
VtDir;

/* for function 'PathThroughBenderGravOrder2' in module bender */
/* ----------------------------------------------------------- */
double NeutronPlaneAngle2             (Neutron *, double, double, double);
double NeutronSurfaceSecIntersectionGr(Neutron *, SurfaceSecond, long);

/* for several modules */
/* ------------------- */
double NeutronPlaneIntersectionGrav(Neutron *, Plane);
double NeutronPlaneIntersection1   (Neutron *, Plane);

/* for modules 'sample_sans', 'sample_powder', 'monochr_analyser' etc. */
/* ------------------------------------------------------------------- */
int  PlaneLineIntersect (VectorType LineOffset, VectorType LineDir, VectorType PlaneNormalVector, double PlaneDistane, VectorType Result);
int  PlaneLineIntersect2(VectorType LineOffset, VectorType LineDir, VectorType PlaneNormalVector, double PlaneDistane, VectorType Result);
long IntersectionWithHorizontalPlane(double Z , VectorType PosVect , VectorType Dir, VectorType Result);
int  OrderPositions(VectorType Dir, VectorType Pos1, VectorType Pos2);

long IntersectionWithRectangular(VectorType DimSample, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2);

long LineIntersectsCube     (VectorType Offset, VectorType Direction, CubeType    *Cube, double t[2]);
long LineIntersectsHollowCyl(VectorType Offset, VectorType Direction, HolCylType  *HCyl, double t[2], VtDir eDir);
long LineIntersectsCylinder (VectorType Offset, VectorType Direction, CylinderType *Cyl, double t[2]);
long LineIntersectsSphere   (VectorType Offset, VectorType Direction, BallType  *Sphere, double t[2]);

long IntersectionWithInfiniteCylinder(double DiameterCyl, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2);

long IntersectionWithCylinder(VectorType DimSample, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2);
long IntersectionWithSphere  (VectorType DimSample, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2);

#endif
