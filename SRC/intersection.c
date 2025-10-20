/********************************************************************************************/
/* Intersection.c (merger of YTSDefs.c and vectan.c)                                        */
/* Functions that calculate intersection points with various surfaces                       */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,                                     */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin                                        */
/*                                                                                          */
/* Change: K. Lieutenant  Jan 2002 and May 2023 reorganized routines                        */
/********************************************************************************************/

#ifdef VT_GRAPH
# include "cpgplot.h"
extern int do_visualise;
#endif

#include <string.h>

#include "intersection.h"
#include "init.h"


/***********************************************************************************/
/* global variables and constants                                                  */
/***********************************************************************************/

VectorType PVector[6] = {{1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
                         {0.0,-1.0, 0.0}, { 0.0, 0.0, 1.0}, {0.0, 0.0,-1.0}};


/********************/
/* Local functions  */
/********************/
int    OrderPositions(const VectorType Dir, VectorType Pos1, VectorType Pos2);

/***********************************************************************************/
/* global functions                                                                */
/***********************************************************************************/

/**********************************************************************************/
/*  FindWWP() determines the intersection point of an trajectory with a wall and
              writes it to the trajectory visualization file if a reason is given */
/**********************************************************************************/
short FindWWP(VectorType* pWWP, Neutron* pNeutronIn, const VectorType Center, const double Length, const double Width, const double Height, VtReason eReason)
{
  short   rc=FALSE, k=0, kMin=-1;
  Neutron NeutT[4];
  Plane   Wall [4]; //Left, Right, Top, Bottom;
  double  Time [4]={-1.0,-1.0,-1.0,-1.0},
          TimeMin=1.0e10;

  for (k=0; k < 4; k++)
    InitNeutron(&NeutT[k]);

  // defines the 4 walls
  Wall[0].A=0.0;  Wall[0].B= 1.0;  Wall[0].C= 0.0;  Wall[0].D=-0.5*Width -Center[1];  // left
  Wall[1].A=0.0;  Wall[1].B=-1.0;  Wall[1].C= 0.0;  Wall[1].D=-0.5*Width +Center[1];  // right
  Wall[2].A=0.0;  Wall[2].B= 0.0;  Wall[2].C= 1.0;  Wall[2].D=-0.5*Height-Center[2];  // top
  Wall[3].A=0.0;  Wall[3].B= 0.0;  Wall[3].C=-1.0;  Wall[3].D=-0.5*Height+Center[2];  // bottom

  // checks which one is hit first
  for (k=0; k < 4; k++)
  {
    CopyNeutron(pNeutronIn, &NeutT[k]);
    Time[k] = NeutronPlaneIntersection1(&NeutT[k], Wall[k]);
    if (Time[k] > 0.0 && Time[k] < TimeMin)
    { kMin = k;
      TimeMin = Time[k];
    }
  }

  // write event, if a reason is given and the first wall is hit before the end of the component
  if (kMin > -1 && NeutT[kMin].Position[0] < Center[0]+0.5*Length && NeutT[kMin].Position[0] > Center[0]-0.5*Length)
  {
    rc = TRUE;
    CopyVector(NeutT[kMin].Position, *pWWP);
    if (eReason!=VT_NO_REASON)
      WriteWWP(&NeutT[kMin], eReason);
  }

  return rc;
}


/**************************************************************************************************/
/* This Function is similar to NeutronLineIntersection, but it propagates the neutron
   to new position on the plane and returns its time of flight there
   Author: Manoshin Sergey,   20.02.2001                                                            */
/**************************************************************************************************/
double NeutronPlaneIntersection1(Neutron *ThisNeutron, const Plane ThisPlane)
{
  double  Time, BB, VelocityReal, v;

  // Velocity cm/ms, Time ms, Position cm
  // Calculating real velocity

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));

  BB = ThisPlane.A * ThisNeutron->Vector[0]
    +  ThisPlane.B * ThisNeutron->Vector[1]
    +  ThisPlane.C * ThisNeutron->Vector[2];

  BB *= VelocityReal;

  // Time BB*Time + CC = 0 */

  if (BB != 0.0) {
    double CC =
        ThisPlane.A * ThisNeutron->Position[0]
      + ThisPlane.B * ThisNeutron->Position[1]
      + ThisPlane.C * ThisNeutron->Position[2]
      + ThisPlane.D;
    Time = -CC/BB;
  } else
    Time = -1.0e4;

  if (fabs(Time) < 1e-12)
    Time = 0.0;

  // This part moves the neutron

  v = VelocityReal*Time;
  ThisNeutron->Position[0] += v * ThisNeutron->Vector[0];
  ThisNeutron->Position[1] += v * ThisNeutron->Vector[1];
  ThisNeutron->Position[2] += v * ThisNeutron->Vector[2];

  /* return Time in ms */

  return Time;
}

/***************************************************************************************************/
/* This Function similar NeutronPlaneIntersection, but he is include GRAVITY effect
   and calculate new position of neutron in the plane (update) and time of flight of neutron (return)
   Author: Manoshin Sergey,    12.02.01                                                            */
/***************************************************************************************************/
double NeutronPlaneIntersectionGrav(Neutron *ThisNeutron, const Plane ThisPlane)
{
  // Calculate the time of flight of a neutron INCLUDING gravity with the plane.

  double  Time, AA, BB, CC, v, VelocityReal;

  //	compute the coefficients of a quadratic equation
  //	G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)
  //	Velocity cm/ms, Time ms, Position cm

  // Calculate the real velocity

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));

  AA = -0.5*(G*1.0e-4)*ThisPlane.C;

  BB = ThisPlane.A * ThisNeutron->Vector[0]
     + ThisPlane.B * ThisNeutron->Vector[1]
     + ThisPlane.C * ThisNeutron->Vector[2];

  BB *= VelocityReal;

  CC = ThisPlane.A * ThisNeutron->Position[0]
     + ThisPlane.B * ThisNeutron->Position[1]
     + ThisPlane.C * ThisNeutron->Position[2]
     + ThisPlane.D;

  // solve the quadratic equation
  // AA*Time*Time + BB*Time + CC = 0

  Time = SolveQuadraticEq(AA,BB,CC);

  // Move the neutron in time including a gravity effect
  v = VelocityReal*Time;
  ThisNeutron->Position[0] += v * ThisNeutron->Vector[0];
  ThisNeutron->Position[1] += v * ThisNeutron->Vector[1];
  ThisNeutron->Position[2] += v * ThisNeutron->Vector[2];

  // Include gravity
  ThisNeutron->Position[2] -= 0.5*(G*1.0e-4)*Time*Time;
  ThisNeutron->Vector[2]   -= (G*1.0e-4)*Time/VelocityReal;

  // return Time in ms

  return Time;
}



/***********************************************************************************/
/* This subroutine calculates the angle between a directed line and a plane. The   */
/* formulae used can be found in any good mathematical handbook.                   */
/* (Modified by Manoshin Sergey for module of velocity not equal 1)                */
/***********************************************************************************/
double	NeutronPlaneAngle2(const Neutron *ThisNeutron, const double AP, const double BP, const double CP)
{
  /* Revised calling parameter */

  double SinGamma=0.0;
  double Velmod;
  double ABC;

  Velmod = sqrt(ThisNeutron->Vector[0]*ThisNeutron->Vector[0]+
		ThisNeutron->Vector[1]*ThisNeutron->Vector[1] +
		ThisNeutron->Vector[2]*ThisNeutron->Vector[2]);

  ABC = sqrt(AP*AP + BP*BP + CP*CP);

  if (ABC != 0.0)
    SinGamma = (AP*ThisNeutron->Vector[0] + BP*ThisNeutron->Vector[1] + CP*ThisNeutron->Vector[2])/ABC;


  if (Velmod != 0.0)
    SinGamma = SinGamma/Velmod;

  return asin(SinGamma);
}

/**************************************************************************************************/
/* This function move neutron from current position to the surface , furthemore he is include
   GRAVITY effect  and time of flight of neutron (return)
   Author: Manoshin Sergey,   25.03.01                                                            */
/**************************************************************************************************/
double NeutronSurfaceSecIntersectionGr(Neutron *ThisNeutron, const SurfaceSecond ThisSurfaceSecond, const long keygrav)
{
  /***********************************************************************************/
  /* This part calculates the time of flight of neutron INCLUDE gravity with   */
  /* plane.	This functions is core for transporting neutrons!									   */
  /***********************************************************************************/

  double  Time, AA, BB, CC, VelocityReal;
  double  VX, VY, VZ, X, Y, Z;

  /*	Make the koefficients of quadratic equation    */
  /*	G = 9.8 m/c**2, we need to cm/ms**2 (100/1000/1000)  */
  /*	Velocity cm/ms, Time ms, Position cm	*/

  /*	Calculating real velocity, cm/ms  */

  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength));

  /*	Components of velocity, projections, and position coordinats */
  /*	Local copy */

  X = ThisNeutron->Position[0];
  Y = ThisNeutron->Position[1];
  Z = ThisNeutron->Position[2];

  VX = VelocityReal*ThisNeutron->Vector[0];
  VY = VelocityReal*ThisNeutron->Vector[1];
  VZ = VelocityReal*ThisNeutron->Vector[2];

  /*	Find the coefficients of the quadratic equation */

  if (keygrav == 1)
    {

      /* Corrected: Marz 03 */

      AA = -0.5*(G*1.0e-4)*ThisSurfaceSecond.F;
    }
  else
    {
      AA = ThisSurfaceSecond.A*VX*VX +
	ThisSurfaceSecond.C*VY*VY +
	ThisSurfaceSecond.E*VZ*VZ +
	ThisSurfaceSecond.P*VX*VY +
	ThisSurfaceSecond.Q*VY*VZ +
	ThisSurfaceSecond.R*VX*VZ;
    }

  BB = 2.0*(ThisSurfaceSecond.A*VX*X + ThisSurfaceSecond.C*VY*Y + ThisSurfaceSecond.E*VZ*Z) +
    ThisSurfaceSecond.B*VX + ThisSurfaceSecond.D*VY + ThisSurfaceSecond.F*VZ +
    ThisSurfaceSecond.P*(X*VY+Y*VX) +
    ThisSurfaceSecond.Q*(Y*VZ+Z*VY) +
    ThisSurfaceSecond.R*(X*VZ+Z*VX);


  CC = ThisSurfaceSecond.A*X*X + ThisSurfaceSecond.B*X+
    ThisSurfaceSecond.C*Y*Y + ThisSurfaceSecond.D*Y+
    ThisSurfaceSecond.E*Z*Z + ThisSurfaceSecond.F*Z + ThisSurfaceSecond.W +
    ThisSurfaceSecond.P*X*Y + ThisSurfaceSecond.Q*Y*Z + ThisSurfaceSecond.R*X*Z;


  /***********************************************************************************/
  /* Now we must to decide quadratic equation for find */
  /* Time AA*Time*Time + BB*Time + CC = 0 */
  /***********************************************************************************/

  Time = SolveQuadraticEq(AA,BB,CC);

  /***********************************************************************************/
  /* This part make move neutron in the time include gravity effect  */
  /***********************************************************************************/

  ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*Time*(ThisNeutron->Vector[0]);
  ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*Time*(ThisNeutron->Vector[1]);
  ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*Time*(ThisNeutron->Vector[2]);

  /* Include gravity */
  if (keygrav == 1)
    {
      ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*Time*Time;
      ThisNeutron->Vector[2] = ThisNeutron->Vector[2] - ((G*1.0e-4)*Time/VelocityReal);
    }
  /* return Time; ms */

  return Time;
}



/******************************************************************************/
/* 'IntersectionWithRectangular'                                              */
/* 'LineIntersectsCube'                                                       */
/* 'LineIntersectsCylinder'                                                   */
/* 'LineIntersectsSphere'                                                     */
/* These function test whether a line defined by the vectors 'Offset'         */
/* and 'Direction' intersect a solid of certain geometry                      */
/* If the line intersects, the functions returns TRUE                         */
/* For 'LineIntersectsCube', 'LineIntersectsCylinder', 'LineIntersectsSphere' */
/* the parameters t[0] and t[1] are calculated                                */
/* (t[i] are proprotional to the time-of-flight until the intersection)       */
/******************************************************************************/

/*************************************************************************/
/* 'LineIntersectsCube'    intersection function for a cube              */
/* The function assumes that the corners of the cube are parallel        */
/* to the x-,y- and z-axis and the center of the cube resides at (0,0,0) */
/* (Author: F. Streffer)                                                                 */
/*************************************************************************/
long LineIntersectsCube(const VectorType Offset, const VectorType Direction, const CubeType *Cube,
			double t[2])
{
	long i,j,k;
	double     PDist[6];
	VectorType ISP[2], TestV;

	PDist[0] = 0.5*Cube->thickness;
	PDist[1] = 0.5*Cube->thickness;
	PDist[2] = 0.5*Cube->width;
	PDist[3] = 0.5*Cube->width;
	PDist[4] = 0.5*Cube->height,
	PDist[5] = 0.5*Cube->height;

	TestV[0] = 0.500000000001*Cube->thickness;   /* not equal 0.5*... to prevent rounding errors */
	TestV[1] = 0.500000000001*Cube->width;
	TestV[2] = 0.500000000001*Cube->height;

	k=0;
	i=MAXV(Direction);

	for(j=0;k<2 && j<6; j++)
	{
    int res;
		res = PlaneLineIntersect(Offset, Direction, PVector[j], PDist[j], ISP[k]);
		if(res && (fabs(ISP[k][0])<TestV[0]) &&
		   (fabs(ISP[k][1])<TestV[1]) &&
		   (fabs(ISP[k][2])<TestV[2]))
		{
			t[k]=(ISP[k][i]-Offset[i])/Direction[i];
			k++;
		}
	}
	/* Ordering: t1 > t0 */
	if(k>1)
	{	if (t[0] > t[1])
			Exchange(&t[0], &t[1]);
		return TRUE;
	}
	else
	{	return FALSE;
	}
}

/****************************************************************************/
/* 'LineIntersectsHollowCyl'  intersection function for a hollow cylinder   */
/* the cylinder axis points along the x-axis                                */
/* (Author: K. Lieutenant)                                                  */
/*************************************************************************/
long LineIntersectsHollowCyl (const VectorType Offset, const VectorType Direction, const HolCylType *HCyl,
			     double t[2], const VtDir eDir)
{
	CylinderType stCyl;
	double       t_cyl[2], t1, t2, t3, t4;
	short        rc=FALSE;

	t1=t2=t3=t4=0.0;

	// intersection points with the outer cylinder
	stCyl.height = HCyl->h_out;
	stCyl.r      = HCyl->r_out;
	if (LineIntersectsCylinder(Offset, Direction, &stCyl, t_cyl))
	{
		// t1 is reached first, t4 last
		if (t_cyl[0] < t_cyl[1])
		{	t1 = t_cyl[0];
			t4 = t_cyl[1];
		}
		else
		{	t1 = t_cyl[1];
			t4 = t_cyl[0];
		}
		// if t4 is close to zero, the neutron has already passed through the hollow cylinder
		if (t4 < 1.0E-08)
			rc = FALSE;
		else
			rc = TRUE;

		if (rc)
		{	// intersection points with the inner cylinder
			stCyl.height = HCyl->h_in;
			stCyl.r      = HCyl->r_in;
			if (LineIntersectsCylinder(Offset, Direction, &stCyl, t_cyl))
			{
				// t2 is reached before t3
				if (t_cyl[0] < t_cyl[1])
				{	t2 = t_cyl[0];
					t3 = t_cyl[1];
				}
				else
				{	t2 = t_cyl[1];
					t3 = t_cyl[0];
				}
				switch (eDir)
				{	// use first 2 points for incoming and last 2 points for outgoing neutrons
					case VT_IN :  t[0]=t1; t[1]=t2; break;
					case VT_OUT:  t[0]=t3; t[1]=t4; break;
					// look for t=0 crossing for neutrons already inside
					default: if  (t1*t2 < 0.0)
									 {t[0]=t1; t[1]=t2;}
								else{t[0]=t3; t[1]=t4;}
				}
			}
			else
			{	t[0]=t1;
				t[1]=t4;
			}
		}
	}
	return rc;
}

/*************************************************************************/
/* 'LineIntersectsCylinder'     intersection function for a cylinder     */
/* the cylinder axis points along the x-axis                             */
/* (Author: F. Streffer)                                                 */
/*************************************************************************/
long LineIntersectsCylinder(const VectorType Offset, const VectorType Direction, const CylinderType *Cyl,
			    double t[2])
{
	VectorType ISP[2];
	double p=0.0,q,
	       sp, spq, spr,
	       h;           /* half of the cylinder height*/
	long   result, i, iDir;

	/* determine the intersection of the line with an infinite cylinder */
	h   = Cyl->height/2.0;
	spq = Direction[1]*Direction[1] + Direction[2]*Direction[2];
	if(spq > 0.0)
	{
		p = (Offset[1]*Direction[1] + Offset[2]*Direction[2]) / spq;
		q = (Offset[1]*Offset[1] + Offset[2]*Offset[2] - Cyl->r*Cyl->r) / spq;
		sp = p*p-q;
	}
	else
	{
		sp = 0.0;
	}

	if(sp > 0.0)
	{
		/*Ok, the neutron intersects the infinite cylinder */
		spr=sqrt(sp);
		t[0]=-p-spr;
		t[1]=-p+spr;

		/* Give the t's an order so its easier later on */
		if(t[0] > t[1])
		  Exchange(&t[0], &t[1]);

		/* checks, where the line enters and leaves the cylinder */
		/* X[i] = ISP[i][0] (see intersection.h) */
		X(0) = Offset[0]+t[0]*Direction[0];
		X(1) = Offset[0]+t[1]*Direction[0];
		if (Direction[0] > 0.0) iDir = 1;
		else                    iDir = 0;
		i=MAXV(Direction);

		/* A) line enters and leaves through the cylinder wall */
		if     (fabs(X(0)) < h  &&  fabs(X(1)) < h)
		{
			result=TRUE;
		}
		/* B) line enters through the cylinder wall and leaves through one of the cylinder disks */
		else if(fabs(X(0)) < h  &&  fabs(X(1)) >= h)
		{
			PlaneLineIntersect(Offset, Direction, PVector[1-iDir], h, ISP[1]);
			t[1] = (ISP[1][i]-Offset[i])/Direction[i];
			result=TRUE;
		}
		/* C) lines enters through a cylinder disk and leaves through the cylinder wall */
		else if(fabs(X(0)) >= h  &&  fabs(X(1)) < h)
		{
			PlaneLineIntersect(Offset, Direction, PVector[iDir], h, ISP[0]);
			t[0] = (ISP[0][i]-Offset[i])/Direction[i];
			result=TRUE;
		}
		/* D) line enters through one of the cylinder disks and leaves through the other one */
		else if( (X(0) < -h  &&  X(1) > h) || (X(0) > h  &&  X(1) < -h) )
		{
			PlaneLineIntersect(Offset, Direction, PVector[1-iDir], h, ISP[1]);
			PlaneLineIntersect(Offset, Direction, PVector[iDir],   h, ISP[0]);
			t[0] = (ISP[0][i]-Offset[i])/Direction[i];
			t[1] = (ISP[1][i]-Offset[i])/Direction[i];
			result=TRUE;
		}
		else
		{
			result=FALSE;
		}
	}
	else
	{	/* Hmm, just missed, but wait one chance remains */
		result=FALSE;
		if(spq==0.0 && (Offset[1]*Offset[1] + Offset[2]*Offset[2] < Cyl->r*Cyl->r))
		{
			/* ok, really not very propable but ... */
			/* the neutron travels paralles to x-axis inside the cylinder   */
			t[0] = ( h-Offset[0])/Direction[0];
			t[1] = (-h-Offset[0])/Direction[0];
			result=TRUE;
		}
	}

	/* Ordering: t1 > t0 */
	if (result==TRUE  &&  t[0] > t[1])
		Exchange(&t[0], &t[1]);

	return result;
}

/*****************************************************************************/
/* 'LineIntersectsSphere'    intersection function for a sphere              */
/* (Author: F. Streffer)                                                                 */
/*************************************************************************/
long LineIntersectsSphere(const VectorType Offset, const VectorType Direction,
			  const BallType *Sphere, double t[2])
/* similar to Line_Intersects_Cube but for a sphere */
{
	/* lets solve x+y+z-r=0 with x=x_0+t*x_t */
	/* x_t+y_t+z_t=1 since Nin->Vector is normalized */
	/* p equals p/2 of the formual solving quadratic eqs. */
	double p,spq;

	p = ScalarProduct(Direction,Offset);
	spq= p*p-(ScalarProduct(Offset,Offset)-Sphere->r*Sphere->r);
	if(spq > 0.0)
	{
		spq=sqrt(spq);
		t[0]=-p-spq;
		t[1]=-p+spq;

		/* Ordering: t1 > t0 */
		if (t[0] > t[1])
			Exchange(&t[0], &t[1]);
		return TRUE;
	}
	else
	{	/* Also if spq is 0.0, i.e. the entrance and exit is the same point */
		/* the neutron is considered to have missed the sample */
		return FALSE;
	}
}



/******************************************************************************/
/* 'IntersectionWithRectangular'      intersection function for a rectangular */
/* (Author: G. Zsigmond)                                                                     */
/*************************************************************************/
long IntersectionWithRectangular(const VectorType DimSample, const VectorType Pos, const VectorType Dir,
				 VectorType Pos1, VectorType Pos2)
{
	VectorType	n, pos0, pos1, pos2, pos3, pos4, pos5 ;
	int			k ;

	for(k=0;k<3;k++) Pos1[k] = Pos2[k] = 0. ;

	n[0] = 1. ; n[1] = n[2] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimSample[0]/2, pos0) == 1)
	{
		if( /*(fabs(pos0[0]) <= DimSample[0]/2.) && */(fabs(pos0[1]) <= DimSample[1]/2.) && (fabs(pos0[2]) <= DimSample[2]/2.) ) CopyVector(pos0, Pos1) ;
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimSample[0]/2, pos1) == 1)
	{
		if( /*(fabs(pos1[0]) <= DimSample[0]/2) && */(fabs(pos1[1]) <= DimSample[1]/2) && (fabs(pos1[2]) <= DimSample[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos1, Pos1) ;
			else CopyVector(pos1, Pos2) ;
		}
	}

	n[1] = 1. ; n[2] = n[0] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimSample[1]/2, pos2) == 1)
	{
		if( (fabs(pos2[0]) <= DimSample[0]/2) && /*(fabs(pos2[1]) <= DimSample[1]/2) &&*/ (fabs(pos2[2]) <= DimSample[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos2, Pos1) ;
			else CopyVector(pos2, Pos2) ;
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimSample[1]/2, pos3) == 1)
	{
		if( (fabs(pos3[0]) <= DimSample[0]/2) && /*(fabs(pos3[1]) <= DimSample[1]/2) &&*/ (fabs(pos3[2]) <= DimSample[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos3, Pos1) ;
			else CopyVector(pos3, Pos2) ;
		}
	}

	n[2] = 1. ; n[0] = n[1] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimSample[2]/2, pos4) == 1)
	{
		if( (fabs(pos4[0]) <= DimSample[0]/2) && (fabs(pos4[1]) <= DimSample[1]/2) /*&& (fabs(pos4[2]) <= DimSample[2]/2)*/ )
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos4, Pos1) ;
			else CopyVector(pos4, Pos2) ;
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimSample[2]/2, pos5) == 1)
	{
		if( (fabs(pos5[0]) <= DimSample[0]/2) && (fabs(pos5[1]) <= DimSample[1]/2) /*&& (fabs(pos5[2]) <= DimSample[2]/2)*/ )
		{
			if(LengthVector(Pos1) == 0.) CopyVector(pos5, Pos1) ;
			else CopyVector(pos5, Pos2) ;
		}
	}
	/*pi(0);pv(pos0);pv(pos1);pv(pos2);pv(pos3);pv(pos4);pv(pos5);*/

	if((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) return 0 ;

	/*ordering intersection positions */

	OrderPositions(Dir, Pos1, Pos2);

	return 1 ;

}/* End IntersectionWithRectangular() */

/*************************************************************************/
/* 'IntersectionWithInfiniteCylinder'                                    */
/* gives coordinates of intersections with an infinite vertical cylinder */
/* (Author: G. Zsigmond)                                                 */
/*************************************************************************/
long	IntersectionWithInfiniteCylinder(const double DiameterCyl, const VectorType Pos, const VectorType Dir,
					 VectorType Pos1, VectorType Pos2)
{
double b, c, delta;

		b = (Pos[0] * Dir[0] + Pos[1] * Dir[1]) / (sq(Dir[0]) + sq(Dir[1])) ;
		c = (sq(Pos[0]) + sq(Pos[1]) - sq(DiameterCyl/2)) / (sq(Dir[0]) + sq(Dir[1])) ;

		if((delta = sq(b) - c) < 0.) return 0 ;

		CopyVector(Dir, Pos1) ;
		MultiplyByScalar(Pos1, - b + (double) sqrt(delta)) ;
		AddVector(Pos1, Pos) ;

		CopyVector(Dir, Pos2) ;
		MultiplyByScalar(Pos2, - b - (double) sqrt(delta)) ;
		AddVector(Pos2, Pos) ;

		return 1 ;
}

/*************************************************************************/
/* 'IntersectionWithCylinder'                                            */
/* gives coordinates of intersections with a vertical cylinder           */
/* DimSample: [diameter, unused, height]                                 */
/* (Author: G. Zsigmond)                                                 */
/*************************************************************************/
long IntersectionWithCylinder(const VectorType DimSample, const VectorType Pos, const VectorType Dir,
			      VectorType Pos1, VectorType Pos2)
{
  if(IntersectionWithInfiniteCylinder(DimSample[0], Pos, Dir, Pos1, Pos2) == 0) return 0 ;

  /* ordering intersection positions */

  OrderPositions(Dir, Pos1, Pos2);

  if((Pos1[2] > DimSample[2]/2.) && (Pos2[2] > DimSample[2]/2.)) return 0 ;
  if((Pos1[2] < - DimSample[2]/2.) && (Pos2[2] < - DimSample[2]/2.)) return 0 ;
  if((fabs(Pos1[2]) <= DimSample[2]/2.) && (fabs(Pos2[2]) <= DimSample[2]/2.)) return 1 ;/* both through cylinder walls */

  if( (Pos2[2] <= DimSample[2]/2.) && (Pos2[2] >= - DimSample[2]/2.) ) /* exit cylinder wall */
    {
      if(Pos1[2] >= DimSample[2]/2.)   /* entrance top */
      {
	      if(IntersectionWithHorizontalPlane(DimSample[2]/2., Pos2, Dir, Pos1) == 0) return 0 ;
	      return 1 ;
      }
      if(Pos1[2] <= - DimSample[2]/2.) /* entrance bottom */
      {
	      if(IntersectionWithHorizontalPlane(- DimSample[2]/2., Pos2, Dir, Pos1) == 0) return 0 ;
	      return 1;
      }
      else return 0 ;
    }

  if((Pos1[2] >= DimSample[2]/2.) && (Pos2[2] <= - DimSample[2]/2.)) /* entrance top exit bottom */
    {
      if(IntersectionWithHorizontalPlane(DimSample[2]/2., Pos, Dir, Pos1) == 0) return 0 ;
      if(IntersectionWithHorizontalPlane(- DimSample[2]/2., Pos, Dir, Pos2) == 0) return 0 ;
      return 1;
    }

  if((Pos1[2] <= - DimSample[2]/2.) && (Pos2[2] >= DimSample[2]/2.)) /* entrance bottom exit top */
    {
      if(IntersectionWithHorizontalPlane(- DimSample[2]/2., Pos, Dir, Pos1) == 0) return 0 ;
      if(IntersectionWithHorizontalPlane(DimSample[2]/2., Pos, Dir, Pos2) == 0) return 0 ;
      return 1;
    }

  if( (Pos1[2] >= - DimSample[2]/2.) && (Pos1[2] <= DimSample[2]/2.) ) /* entrance cylinder wall */
    {
      if(Pos2[2] >= DimSample[2]/2.) /* exit top */
      {
	      if(IntersectionWithHorizontalPlane(DimSample[2]/2., Pos1, Dir, Pos2) == 0) return 0 ;
	      return 1;
      }
      if(Pos2[2] <= - DimSample[2]/2.) /* exit bottom */
      {
	      if(IntersectionWithHorizontalPlane(- DimSample[2]/2., Pos1, Dir, Pos2) == 0) return 0 ;
	      return 1;
      }
      else return 0 ;
    }
  else return 0 ;
}

/*************************************************************************/
/* 'IntersectionWithHorCyl'                                              */
/* gives coordinates of intersections with a horizontal cylinder         */
/* using the function 'IntersectionWithCylinder'                         */
/* (Author: K. Lieutenant)                                               */
/*************************************************************************/
long IntersectionWithHorCyl(const double length, const double diameter, const VectorType Pos, const VectorType Dir,
			                      VectorType Pos1, VectorType Pos2)
{
  long       rc=FALSE;
  VectorType DimSmplV={0.0,0.0,0.0},
             PosV= {0.0,0.0,0.0},
             DirV= {0.0,0.0,0.0},
             Pos1V={0.0,0.0,0.0},
             Pos2V={0.0,0.0,0.0};

  // fill structure defining size
  DimSmplV[0] = diameter;
  DimSmplV[2] = length;

  // rotate position and direction 90° about y-axis
  PosV[0] = -Pos[2]; PosV[1] = Pos[1]; PosV[2] = Pos[0];
  DirV[0] = -Dir[2]; DirV[1] = Dir[1]; DirV[2] = Dir[0];

  // call function for vertical cylinder
  rc=IntersectionWithCylinder(DimSmplV, PosV, DirV, Pos1V, Pos2V);

  // rotate back intersection positions (-90° about y-axis)
  Pos1[0] = Pos1V[2]; Pos1[1] = Pos1V[1]; Pos1[2] = -Pos1V[0];
  Pos2[0] = Pos2V[2]; Pos2[1] = Pos2V[1]; Pos2[2] = -Pos2V[0];

  return rc;
}

/*****************************************************************************/
/* 'IntersectionWithSphere'  gives coordinates of intersections with a sphere */
/* (Author: G. Zsigmond)                                                                     */
/*************************************************************************/
long IntersectionWithSphere(const VectorType DimSample, const VectorType Pos, const VectorType Dir,
			    VectorType Pos1, VectorType Pos2)
{
double b, c, delta, khi1, khi2 ;

	b = ScalarProduct(Pos, Dir) ;
	c = ScalarProduct(Pos, Pos) - sq(DimSample[0]/2) ;
	delta = sq(b) - c ;

	khi1 = - b + (double) sqrt(delta) ;
	khi2 = - b - (double) sqrt(delta) ;

	if(delta < 0.) return 0 ;
	else
	{
		CopyVector(Dir, Pos1) ;
		MultiplyByScalar(Pos1, khi1) ;
		AddVector(Pos1, Pos) ;

		CopyVector(Dir, Pos2) ;
		MultiplyByScalar(Pos2, khi2) ;
		AddVector(Pos2, Pos) ;

		/*ordering intersection positions */

		OrderPositions(Dir, Pos1, Pos2);

		return 1 ;
	}
}



/****************************************************************/
/* 'PlaneLineIntersect'                                         */
/* 'PlaneLineIntersect2'                                        */
/* These functions compute the intersect of a line give by some */
/* offset and a Direction, and a plane in the Hessian form.     */
/****************************************************************/
/****************************************************************/
/*  PlaneLineIntersect                                          */
/* function returns TRUE if the line intersects and             */
/*                       'Result' will contain the point        */
/*                  FALSE if the line is parallel to plane      */
/* (Author: F. Streffer)                                        */
/***********************************************************************************/
int PlaneLineIntersect(const VectorType LineOffset, const VectorType LineDir,
                       const VectorType PlaneNormalVector, const double PlaneDistane,
                       VectorType Result)
{
  double     help,t, help1;
  int        i;

  help=ScalarProduct(LineDir,PlaneNormalVector);
  if(help==0.0)
      /* Ok, there is somehow a problem, the line given is parallel to   */
      /* the plane and will never intersect.                             */
    return FALSE;

  help1 = ScalarProduct(LineOffset,PlaneNormalVector);
  t = (PlaneDistane-help1)/help;
  for(i=0;i<3;i++)
    Result[i] = LineOffset[i] + t*LineDir[i];

  return TRUE;
}

/*********************************************************************/
/*  PlaneLineIntersect2                                              */
/*                                                                   */
/* function returns TRUE, 'Result' contains the point                */
/* if line is parallel to plane, 'Result' contains very high values  */
/* (Author: G. Zsigmond)                                             */
/***********************************************************************************/
int PlaneLineIntersect2(const VectorType LineOffset, const VectorType LineDir,
                       const VectorType PlaneNormalVector, const double PlaneDistane,
                       VectorType Result)
{
  double     help,t, help1;
  int        i;

  help = ScalarProduct(LineDir,PlaneNormalVector);
  if(help==0.0) {
    /* Ok, there is somehow a problem, the line given is parallel to   */
    /* the plane and will never intersect.                             */
    for(i=0;i<3;i++)
      Result[i]=1.e20;
    return 1;
  }

  help1 = ScalarProduct(LineOffset,PlaneNormalVector);
  t = (PlaneDistane-help1)/help;

  for(i=0;i<3;i++)
    Result[i] = LineOffset[i] + t*LineDir[i];

  return TRUE;
}


/***************************************/
/* IntersectionWithHorizontalPlane		*/
/* Z:		position of the plane		*/
/* PosVect: point on line				*/
/* Dir:		flight direction			*/
/* (Author: G. Zsigmond)                                                           */
/***********************************************************************************/
long IntersectionWithHorizontalPlane(const double Z , const VectorType PosVect, const VectorType Dir,
				     VectorType Result)
{
	if(Dir[2] ==0.) return 0 ;

	CopyVector(Dir, Result) ;
	MultiplyByScalar(Result, (Z - PosVect[2])/Dir[2]) ;
	AddVector(Result, PosVect) ;

	return 1;
}



/***********************************************************************************/
/* solving quadratic equation (for trajectory under gravity)                       */
/***********************************************************************************/
#ifdef VERS27
/*******************************************************************/
/* THIS FUNCTION SOLVES THE QUADRATIC EQUATION
   AA*X*X+BB*X+CC=0
   Version from 29.01.01, Author Manoshin Sergey                   */
/*******************************************************************/

double SolveQuadraticEq(double AA, double BB, double CC)
{
	double X1=0.0, X2=0.0;
	double DD, TEMP;
	double Time=0.0;

	/*	ignore a - very small or eq zero , to solve bx+c=0 */

	if ((fabs(AA)*1.0e12) <= fabs(BB)||(AA == 0.0))
	{
		if (BB != 0.0)
		{
			/*	AA=0 BB # 0	  */
			X1 = -CC/BB;
			X2 = X1;
			goto ok15;
		}
		else
		{
			if (CC == 0.0)
			{
				/*	AA=0 BB=0 CC=0	    */
				X1 = 0.0;
				X2 = 0.0;
				goto ok15;
			}
			/*	AA=0 BB=0 CC # 0	  */
			X1 = -1.0e4;
			X2 = X1;
			goto ok15;
		}
	}

	/*  later AA not eq zero */

	/*	square eq. Axx+Bx=0 AA # 0 C=0	*/
	if (CC == 0)
	{
		X1 = -BB/AA;
		X2 = 0.0;
		goto ok15;
	}

	/*	Axx+C=0	  */
	if (BB == 0.0)
	{
		TEMP = -CC/AA;
		if (TEMP < 0.0)
		{
			/*	SQRT(TEMP)   */
			X1 = -1.0e4;
			X2 = -1.0e4;
			goto ok15;
		}
		X1 = sqrt(TEMP);
		X2 = -X1;
		goto ok15;
	}


	/*	eq Axx+Bx+C=0	*/
	DD = BB*BB - 4.0*AA*CC;
	if (DD < 0.0)
	{
		/*	D<0   */
		X1 = -1.0e4;
		X2 = -1.0e4;
		goto ok15;
	}

	DD = sqrt(DD);
	if (DD == 0.0)
	{
		X1=-BB/(2.0*AA);
		X2=X1;
		goto ok15;
	}


	if (BB > 0.0)
	{
		X1 = (-BB-DD)/(2.0*AA);
		X2 = (2.0*CC)/(-BB-DD);
		goto ok15;
	}

	if (BB < 0.0)
	{
		X2 = (-BB+DD)/(2.0*AA);
		X1 = (2.0*CC)/(-BB+DD);
		goto ok15;
	}

	ok15:
	/*	printf("squares= %e  %e \n",X1,X2);	*/
	/* Now find smallest root and copy this in Time  */

	if (fabs(X1) < 4e-10)
		X1=0.0;
	if (fabs(X2) < 4e-10)
		X2=0.0;

	if ((X1 > 0.0) && (X2 > 0.0))
	{
		Time = Min(X1, X2);
	}
	else
	{
		Time = Max(X1, X2);
	}

	return Time;
}

#else

#define NZero(a) (fabs(a) < 4e-10 ? 0.0 : a)
#define makeNZero(a) if (fabs(a) < 4e-10) a = 0.0

double SolveQuadraticEq (double A, double B, double C) {

  // solve A*x^2 + B*x + C = 0

  double x1, x2, d;

  if (A == 0.0 || fabs(A) * 1.0e12 <= fabs(B)) {
    // A almost zero, solve Bx+C=0
    if (B != 0.0) {
      d = -C/B;
      return NZero(d);
    }

    return C == 0.0 ? 0.0 : -1.0e4;  // no solution if B==0 and C!=0
  }

  if (C == 0) {
    // Axx + Bx = 0 -> x*(x+B/A) = 0, two solutions x1=0, x2=-B/A
    x1 = 0.0;
    x2 = -B/A;

  } else {

    if (B == 0.0) {
      // Axx + C = 0
      d = -C/A;
      if (d < 0.0)
	return -1.0e4; // no solution
      if (d < 16e-20)
	return 0.0;
      return sqrt(d);
    }

    // full quadratic equation Axx+Bx+C=0
    d = B*B - 4*A*C;
    if (d < 0.0)
      return -1.0e4; // no real solution

    if (d == 0.0) {
      d = -B/(2*A);
      return NZero(d);
    }

    d = sqrt(d);

    x1 = (-B + d) / (2*A);
    x2 = (-B - d) / (2*A);
  }

  // return the smallest of 2 positive roots, or the bigger one

  makeNZero(x1);
  makeNZero(x2);
  return (x1 > 0.0 && x2 > 0.0) ? Min(x1, x2) : Max(x1, x2);
}

#endif



/***********************************************************************************/
/** local functions                                                               **/
/***********************************************************************************/

/***********************************************************************************/
/* ordering points on a trajectory so that Dir shows from Pos1 to Pos2 */
/***********************************************************************************/
int OrderPositions(const VectorType Dir, VectorType Pos1, VectorType Pos2)
{
  VectorType propag, V;

  CopyVector(Pos2, propag); SubVector(propag, Pos1);

  if(ScalarProduct(propag, Dir) == 0.) return 0;
  if(ScalarProduct(propag, Dir) < 0.)
    {
      CopyVector(Pos1, V) ;
      CopyVector(Pos2, Pos1) ;
      CopyVector(V, Pos2) ;
    }
  return 1;
}

