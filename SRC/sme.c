/*********************************************************************************************/
/*  VITESS module 'sm_ensemble'                                                              */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.1  Jul 2002  G. Zsigmond	initial version                                              */
/* 1.0  Sep 2002  G. Zsigmond	active on both sides                                         */
/* 1.1  Apr 2003  S. Manoshin	visualisation included                                       */
/*              + G. Zsigmond	and position filter                                          */
/* 1.2  May 2003  G. Zsigmond	collision file name as input parameter,                      */
/*                              coll. output changed, quantisation direction incl.           */
/* 1.3  Jun 2003  G. Zsigmond	included if when closing coll file; softabort included       */
/* 1.4  Jan 2004  K. Lieutenant changes for 'instrument.dat'                                 */
/* 1.5	Feb 2004  S. Manoshin   Visualise only first 10000 neutrons			     */
/*				Choose the output device : screen, file or both	             */
/*				New external variable gselec 			             */
/* 1.6  Apr 2004  G. Zsigmond	Visualise only first 1000 neutrons, write sm_ensemble.ps     */
/* 1.7  Mar 2008  M. Fromme     deflate, constant nWallsMax = max. number of planes          */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"
#include "softabort.h"

#include "cpgplot.h"

/* MF: visualisation for Windows and generation of file for the picture */
#ifdef DO_WIN32
# define GDEV "sm_ensemble.ps"
#else
# define GDEV "sm_ensemble.png"
#endif

extern int      gselec; /* choose the output 1 - display only, 2 - file only, 3 - both, defined in cpgplot.c */

#define	STRING_BUFFER 1000

#define nWallsMax 16

// global variables
static FILE	*Par_Field, *COLLFILE;
static char	*ParameterFileName, *COLLFILEName="collision.dat";
static int	p, vistype, quant_dir=2,
                nWalls, Wallonoff__[nWallsMax];
static long	NumWrong, m, nocol, nocolM = 10000,
		number_vis_tr; /* current number of visualised trajectories, add SM */
static double	TOF, WL, Prob,
                OutputAngleHoriz, OutputAngleVert, RotMatrixOut[3][3],
                IntegralIntensity, Path, Path0,
                Windw=-10.0, WindW=200.0, Windh=-10.0, WindH=10.0, wei_min1=0.0; /* Variables for visualisation */

static Neutron	neutron;
static VectorType Pos, Dir, SpinVector, TranslOutput,
        	WallOffset__[nWallsMax], WallNormal__[nWallsMax], r1__[nWallsMax], r2__[nWallsMax], r3__[nWallsMax], r4__[nWallsMax];
static double	RotMatrixWall__[nWallsMax][3][3], WallVert__[nWallsMax], WallHoriz__[nWallsMax], Path__[nWallsMax],
                thetaC__[nWallsMax][2], thetaCSM__[nWallsMax][2], RthetaCSM__[nWallsMax][2],
                mued__[nWallsMax][4], mrangh__[nWallsMax], mrangv__[nWallsMax], prob__[nWallsMax];

// external functions
double   	reflectivity(double angle, double CritUp, double WL, double smearf, double ReflUp);
void    	OutputTransformations(double *tof, double *wl, double *prob, VectorType Pos, VectorType Dir, VectorType SpinVector);

// forward declarations for local functions
static int	hittriangle(VectorType r1, VectorType r2, VectorType rt);
static int	hitwall(VectorType r1, VectorType r2, VectorType r3, VectorType r4, VectorType rt);
static double	CollideWall(double *prob, VectorType pos,VectorType dir,VectorType spin,VectorType WallOffset,VectorType WallNormal,
			    double  RotMatrixWall[3][3],VectorType r1,VectorType r2,VectorType r3,VectorType r4,
			    double thetaC[2], double thetaCSM[2], double RthetaCSM[2], double mued[4], double mrangh, double mrangv);
static void	ReadParameterFile();
static void	OwnInit(int argc, char *argv[]);
static void	OwnCleanup();

// static void     CartesianToSpherical2(VectorType Vector, double *Theta, double *Phi);


static void dumpN (int i, int m) {

  fprintf(COLLFILE, "     %c%c%07ld %c %5d    %10d  %2d  %d     %12.5f  %12.5f  %12.5f     %12.5f  %12.5f\n",
	  InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo,
	  InputNeutrons[i].Debug,       InputNeutrons[i].Color, i, ((int) SpinVector[quant_dir]),
	  m, Pos[0], Pos[1], Pos[2],
	  180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));
}

static void drawIt(int p) {

  if (vistype == 0) {
    switch (p) {
    case 2: cpgdraw((float) Pos[0], (float) Pos[1]); break;
    case 3: cpgdraw((float) Pos[0], (float) Pos[2]); break;
    case 4: cpgdraw((float) Pos[1], (float) Pos[2]); break;
    }
    return;
  }
  if (vistype != 1) return;
  switch (p) {
  case 2: cpgpt1((float) Pos[0], (float) Pos[1], -2); break;
  case 3: cpgpt1((float) Pos[0], (float) Pos[2], -2); break;
  case 4: cpgpt1((float) Pos[1], (float) Pos[2], -2); break;
  }
}

static void dumpAndDraw(int p, int i, int m, double prob) {
  if (p==1)
    dumpN(i,m);
  else if (prob > wei_min1)
    drawIt(p);
}


int main(int argc, char **argv)
{

  VectorType pos__[nWallsMax], dir__[nWallsMax], spin__[nWallsMax];
  int i,j;

  /* Initialize the program according to the parameters given  */
  Init(argc, argv, VT_SM_ENSEMBLE);
  OwnInit(argc, argv);

  if (p==1) {
    COLLFILE = fopen(COLLFILEName, "w");
    fprintf(COLLFILE, "     ID      debug color          no  sp wall         x/cm          y/cm          z/cm           dir y/°       dir z/° \n\n");
  }

  DECLARE_ABORT;

  while (ReadNeutrons())

    for(i=0; i<NumNeutGot; i++) {

      CHECK;

      if (number_vis_tr == 1000)
	p = 100;

      InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

      TOF = InputNeutrons[i].Time;
      WL = InputNeutrons[i].Wavelength;

      Prob = InputNeutrons[i].Probability; /*Prob = 1.;*/

      CopyVector(InputNeutrons[i].Position, Pos);
      CopyVector(InputNeutrons[i].Vector, Dir);
      CopyVector(InputNeutrons[i].Spin, SpinVector);

      /*  compute hit positions on the walls */

      Path0 = 0.; m=0; nocol =0;
  
      if (p==1)
	dumpN(i,0);
      else if (p >= 2 && p <= 4)
	cpgsci((int)(InputNeutrons[i].Color));

      if(Prob > wei_min1)
	drawIt(p);

      // test for collisions in the ensemble, up to a maximum of 1000 collisions.
 
      for (j=0; j<1000; j++) {

	int nw, anyhit;

	// test if the neutron hits any wall
	anyhit = 0;

	for (nw=0; nw<nWalls; nw++) {
	  double pa;

	  CopyVector(Pos, pos__[nw]);
	  CopyVector(Dir, dir__[nw]);
	  CopyVector(SpinVector, spin__[nw]);
	  prob__[nw] = Prob;

	  if (m != nw+1 && Wallonoff__[nw])
	    pa = CollideWall(&prob__[nw], pos__[nw], dir__[nw], spin__[nw], WallOffset__[nw], WallNormal__[nw],
			     RotMatrixWall__[nw], r1__[nw], r2__[nw], r3__[nw], r4__[nw], thetaC__[nw], thetaCSM__[nw], RthetaCSM__[nw],
			     mued__[nw], mrangh__[nw] , mrangv__[nw]);
	  else
	    pa = 99999;

	  if (pa != 99999)
	    anyhit = 1;
	  Path__[nw] = pa;
	}

	if (! anyhit) break;  /* leave for j loop */

	for (nw=0; nw<nWalls; nw++)
	  if (m != nw+1) {
	    int nn;
	    double tpath = Path__[nw];
	    for (nn=1; nn<=nWalls; nn++)
	      if (tpath > Path__[(nn+nw) % nWalls])
		break;
	    if (nn <= nWalls) continue;
	    CopyVector(pos__[nw], Pos);
	    CopyVector(dir__[nw], Dir);
	    Path = Path__[nw];
	    Prob = prob__[nw];
	    m = nw+1;
	    nocol++;
	    dumpAndDraw(p,i,m,Prob);
	  }

	if (nocol == nocolM) break;  /* leave for j loop */

	if (Prob < wei_min) goto getlost;

	Path0 += Path;

      } // end of for j loop

      if (Prob < wei_min) goto getlost;

      /* transform into output frame */
      SubVector(Pos, TranslOutput);
      RotVector(RotMatrixOut, Pos);
      RotVector(RotMatrixOut, Dir);

      /* translate neutron variables for output - X'=0. */
      {
	VectorType Path ;

	if (Pos[0] > 0.0) goto getlost; /*	Filter if collision after output YZ plane */

	TOF += - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

	CopyVector(Dir, Path) ;
	MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;
	AddVector(Pos, Path) ;
      }			/* Path = displacement vector */

      /* transform into input frame to give here the exit positions */
      if (p >= 1 && p <= 4) {
	VectorType posex, direx ;
	CopyVector(Pos, posex) ;
	CopyVector(Dir, direx) ;
	RotBackVector(RotMatrixOut, direx);
	RotBackVector(RotMatrixOut, posex);
	AddVector(posex, TranslOutput);
	if (p == 1)
	  dumpN(i,0);
	else if (Prob > wei_min1) {
	  int ia = p == 4 ? 1 : 0;
	  int ib = p <= 2 ? 1 : 2;

	  if      (vistype == 0) cpgdraw((float) posex[ia], (float) posex[ib]);
	  else if (vistype == 1) cpgpt1 ((float) posex[ia], (float) posex[ib], -2);
	}
      }

      /* transmit coordinates which were not changed, overwrite the rest below */
      neutron = InputNeutrons[i];

      neutron.Time        = TOF + Path0 / V_FROM_LAMBDA(WL);
      neutron.Probability = Prob;

      CopyVector(Pos, neutron.Position);
      CopyVector(Dir, neutron.Vector);

      neutron.Vector[0] = (double) sqrt(1 - sq(neutron.Vector[1]) - sq(neutron.Vector[2]));

      CopyVector(SpinVector, neutron.Spin);

      number_vis_tr = number_vis_tr + 1;

      WriteNeutron(&neutron);

    getlost:
      if (p==1) fprintf(COLLFILE, " \n");
    }


 my_exit:

  if (p==1) fclose(COLLFILE);

  OwnCleanup();

  Cleanup(TranslOutput[0], TranslOutput[1], TranslOutput[2], OutputAngleHoriz,OutputAngleVert);

  fprintf(LogFilePtr," \n");

  return 0;
}



/* own initialization of the ensemble module */

static void OwnInit(int argc, char *argv[])
{
  int j;
  fprintf(LogFilePtr," \n");
  print_module_name("supermirror_ensemble 1.6");

  for (j=0;j<3;j++) TranslOutput[j] = 0.;
  OutputAngleHoriz = OutputAngleVert = 0.;

  gselec = 1 ; /* Activate visualisation device -screen */

  while (argc>1)  {
    char *arg = &argv[1][2];
    switch (argv[1][1]) {
    case 'P':
      if((Par_Field = fopen(arg,"r"))==NULL) {
	fprintf(LogFilePtr,"\nERROR: Parameter file '%s' not found.\n",arg);
	exit(0);
      }
      ParameterFileName = arg;
      break;
    case 'C':
      COLLFILEName = arg;
      break;
    case 'M':
      sscanf(arg, "%ld", &nocolM);
      fprintf(LogFilePtr,"Stops at %ld collisions.\n", nocolM);
      break;
    case 'T':
      sscanf(arg, "%d", &p);
      break;
    case 'Q':
      sscanf(arg, "%d", &quant_dir);
      if (quant_dir != 0 && quant_dir != 1 && quant_dir != 2) {
	fprintf(LogFilePtr,"\nERROR:  wrong quantization direction definition.\n\n");
	exit(0);
      }
      break;
    case 'r':
      sscanf(arg, "%lf", &TranslOutput[0]);
      break;
    case 's':
      sscanf(arg, "%lf", &TranslOutput[1]);
      break;
    case 't':
      sscanf(arg, "%lf", &TranslOutput[2]);
      break;
    case 'h':
      sscanf(arg, "%lf", &OutputAngleHoriz);
      break;
    case 'v':
      sscanf(arg, "%lf", &OutputAngleVert);
      break;
    case 'w':
      sscanf(arg, "%lf", &Windw);
      break;
    case 'W':
      sscanf(arg, "%lf", &WindW);
      break;
    case 'a':
      sscanf(arg, "%lf", &Windh);
      break;
    case 'A':
      sscanf(arg, "%lf", &WindH);
      break;
    case 'b':
      sscanf(arg, "%lf", &wei_min1);
      break;      
    case 'c':
      sscanf(arg, "%d", &vistype);
      break;
    case 'o':
      gselec = atol(arg);
      break;	  
    }
    argc--;
    argv++;
  }


  if (p==1)
    fprintf(LogFilePtr,"Prints the coordinates of collisions to '%s'. \n", COLLFILEName);

  else if ((p == 2)||(p == 3)||(p == 4)) {

    /* initialize pgplot with device GraphDev */

    switch (gselec) {
    case 1:  fprintf(LogFilePtr,"Open visual output device - display\n"); break;
    case 2:  fprintf(LogFilePtr,"Open visual output device - file\n"); break;
    case 3:  fprintf(LogFilePtr,"Open visual output device - display+file\n"); break;
    default: fprintf(LogFilePtr,"ERROR: Incorrect output device, correct option -o, value 1,2 or 3\n");
      exit(-1);
    }


    fprintf(LogFilePtr,"Visual activation for the first  %ld  trajectories.\n", BufferSize);
    /*	    initialize pgplot with device GraphDev  */
    if (cpgopen(GDEV) < 1) {
      fprintf(LogFilePtr,"ERROR: I cannot open graphical output device.\n");
      exit(-1);
    }

    fprintf(LogFilePtr,"Minimum neutron weigth for visualisation: %f.\n", wei_min1);
  }

  if (p>=2 && p<=4) {
    cpgenv((float) Windw, (float) WindW, (float) Windh, (float) WindH, 0, 0);
    cpgsfs((int) 2);
    cpgsch((float) 1.2);
    switch (p) {
    case 2: cpglab("X, cm","Y, cm","NEUTRON VISUALISATION - PLANE X0Y"); break;
    case 3: cpglab("X, cm","Z, cm","NEUTRON VISUALISATION - PLANE X0Z"); break;
    case 4: cpglab("Y, cm","Z, cm","NEUTRON VISUALISATION - PLANE Y0Z"); break;
    }
  }

  NumWrong=0;

  IntegralIntensity = 0.; Path = 0.;

  OutputAngleHoriz	*= M_PI/180.;
  OutputAngleVert	*= M_PI/180.;
  FillRotMatrixZY(RotMatrixOut, OutputAngleVert, OutputAngleHoriz);

  ReadParameterFile();

  if(Par_Field != NULL)
    fclose(Par_Field);

}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

static void OwnCleanup()
{
  if ((p == 2)||(p == 3)||(p == 4)||(p == 100))  cpgclos();
  if(NumWrong !=0) fprintf(LogFilePtr,"\nERROR:  %ld collided trajectories have wrong input spin.\n\n", NumWrong);
}


/* ReadParameterFile() reads the parameters from file */

static void ReadParameterFile()
{

  int i;

  for (i=0; i<nWallsMax; i++) {

    if (feof(Par_Field)) break;
    r1__[i][0] = r2__[i][0] = r3__[i][0] = r4__[i][0] = 0;
    if (! ReadParI(Par_Field)) break;  // no wall no fun
    Wallonoff__[i] = 1;
    r1__[i][1] = ReadParF(Par_Field);
    r1__[i][2] = ReadParF(Par_Field);
    r2__[i][1] = ReadParF(Par_Field);
    r2__[i][2] = ReadParF(Par_Field);
    r3__[i][1] = ReadParF(Par_Field);
    r3__[i][2] = ReadParF(Par_Field);
    r4__[i][1] = ReadParF(Par_Field);
    r4__[i][2] = ReadParF(Par_Field);
    WallOffset__[i][0] = ReadParF(Par_Field);
    WallOffset__[i][1] = ReadParF(Par_Field);
    WallOffset__[i][2] = ReadParF(Par_Field);
    WallHoriz__[i] = ReadParF(Par_Field);
    WallVert__[i] = ReadParF(Par_Field);
    mrangh__[i] = ReadParF(Par_Field);
    mrangv__[i] = ReadParF(Par_Field);
    thetaC__[i][0] = ReadParF(Par_Field);
    thetaCSM__[i][0] = ReadParF(Par_Field);
    RthetaCSM__[i][0] = ReadParF(Par_Field);
    mued__[i][0] = ReadParF(Par_Field);
    mued__[i][1] = ReadParF(Par_Field);
    thetaC__[i][1] = ReadParF(Par_Field);
    thetaCSM__[i][1]  = ReadParF(Par_Field);
    RthetaCSM__[i][1] = ReadParF(Par_Field);
    mued__[i][2] = ReadParF(Par_Field);
    if (feof(Par_Field)) break;
    mued__[i][3] = ReadParF(Par_Field);
    if(Wallonoff__[i]) {
      nWalls = i;  // highest wall index so far 
      fprintf(LogFilePtr,
	      "\nA:  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  "
	      "%10.5f%10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f%10.5f\n",
	      r1__[i][1], r1__[i][2], r2__[i][1], r2__[i][2], r3__[i][1], r3__[i][2], r4__[i][1], r4__[i][2],
	      WallOffset__[i][0], WallOffset__[i][1], WallOffset__[i][2], WallHoriz__[i],
	      WallVert__[i], mrangh__[i], mrangv__[i], thetaC__[i][0], thetaCSM__[i][0], RthetaCSM__[i][0],
	      mued__[i][0], mued__[i][1], thetaC__[i][1], thetaCSM__[i][1], RthetaCSM__[i][1], mued__[i][2], mued__[i][3]);
    }
    ReadParComment(Par_Field);
    WallHoriz__[i]	*= M_PI/180.;
    WallVert__[i]	*= M_PI/180.;
    mrangh__[i]	*= M_PI/180.;
    mrangv__[i]	*= M_PI/180.;
    FillRotMatrixZY(RotMatrixWall__[i], WallVert__[i], WallHoriz__[i]);
    EulerToCartesianZY(WallNormal__[i], &WallVert__[i], &WallHoriz__[i]);
  }

  fprintf(LogFilePtr, "\nABCDEFGHIJ:");
  for (i=0; i<nWalls; i++) 
    fprintf(LogFilePtr, " %d", Wallonoff__[i]);
  fprintf(LogFilePtr, "\n");

}/* End ReadParameterFile  */



/* Theta and Phi of a unit vector if Theta is the angle with the axis of lowest index  */

// static void CartesianToSpherical2(VectorType Vector, double *Theta, double *Phi)
// {
// 	*Theta	= (double) acos(Vector[0]);
// 
// 	if(Vector[2]>=0) *Phi	= (double) atan2(Vector[2], Vector[1]);
// 	if(Vector[2]< 0) *Phi	= 2. * M_PI + (double) atan2(Vector[2], Vector[1]);
// }

static double CartesianToPhi(VectorType Vector) {
  // same as CartesianToSpherical2, but does not compute theta
  double v = atan2(Vector[2], Vector[1]);
  if (Vector[2] < 0) v += 2. * M_PI;
  return v;
}

static int hittriangle(VectorType r1, double phi1,
		       VectorType r2, double phi2,
		       VectorType rt, double phit)
{
  if(phi1 < phi2) {
    if ((phi1 < phit) && (phit < phi2)
	&& (Area(r1,r2) > (Area(r1,rt) + Area(r2, rt))))
      return 1;
    return 0;
  }
  if(phi1 > phi2) {
    if((((phi1 < phit) && (phit < 2*M_PI)) || ((0. < phit) && (phit < phi2)))
       && (Area(r1,r2) > (Area(r1,rt) + Area(r2, rt))))
      return 1;
    return 0;
  }
  return 0;
}

/* hitwall:  calculate if the neutron hits the wall */

static int hitwall(VectorType r1, VectorType r2, VectorType r3, VectorType r4, VectorType rt)
{
  double phi1, phi2, phi3, phi4, phit;
  phi1 = CartesianToPhi(r1);
  phi2 = CartesianToPhi(r2);
  phit = CartesianToPhi(rt);
  if (hittriangle(r1, phi1, r2, phi2, rt, phit)
      return 1;
  phi3 = CartesianToPhi(r3);
  if (hittriangle(r2, phi2, r3, phi3, rt, phit)
      return 1;
  phi4 = CartesianToPhi(r4);
  if (hittriangle(r3, phi3, r4, phi4, rt, phit)
      return 1;
  if (hittriangle(r4, phi1, r1, phi1, rt, phit)
      return 1;

  return 0;
}


/* routine which computes the collision with a wall */

static double CollideWall(double *prob, VectorType pos,VectorType dir,VectorType spin,VectorType WallOffset,VectorType WallNormal,
			 double  RotMatrixWall[3][3],VectorType r1,VectorType r2,VectorType r3,VectorType r4,
			 double thetaC[2], double thetaCSM[2], double RthetaCSM[2], double mued[4], double mrangh, double mrangv)
{
  VectorType rt; double path;  double rangh=0., rangv=0., RotMatrixRang[3][3];

  /* random angular spread */

  rangh = MonteCarlo(- mrangh/2., mrangh/2.);
  rangv = MonteCarlo(- mrangv/2., mrangv/2.);
  FillRotMatrixZY(RotMatrixRang, rangv, rangh);

  if (PlaneLineIntersect(pos, dir, WallNormal, (double) ScalarProduct(WallOffset, WallNormal), rt)== 0)
    return 99999;

  { VectorType replacement;

    CopyVector(rt, replacement);
    SubVector(replacement, pos);
    if ((ScalarProduct(replacement, dir)< 0.)/*||(ScalarProduct(replacement, WallNormal)< 0.)*/)
      return 99999;
  }

  /* transform into frame of the wall  */

  SubVector(rt, WallOffset);
  RotVector(RotMatrixWall, rt);
  RotVector(RotMatrixWall, dir);

  if((mrangh!=0.)||(mrangv!=0.))   {
    RotVector(RotMatrixRang, rt);
    RotVector(RotMatrixRang, dir);
  }


  /* here comes to collision etc. */
  { double the, Choise, Refl[2], expon[2];

    Choise = MonteCarlo(0,1);
    the = M_PI_2 - acos(fabs(dir[0]));

    if (hitwall(r1, r2, r3, r4, rt)==0) {
      RotBackVector(RotMatrixWall, dir);
      return 99999;
    }

    if(fabs(SpinVector[quant_dir])!=1.) {
      NumWrong++;
      *prob = 0. ;
    }

    if(SpinVector[quant_dir] == 1.) {

      /* spin up */
      if(the <= thetaC[0] * WL) {
	dir[0] *= -1.;
      } else {
	expon[0] = (mued[0] * WL + mued[1]) /(double) sqrt(sq((double) sin(the)) - sq((double) sin(thetaC[0] * WL)));

	if(expon[0] > 500) expon[0] = 500;
	if((the > thetaC[0] * WL)&&(the <= thetaCSM[0] * WL))
	  {
	    Refl[0] = RthetaCSM[0] + (1. - RthetaCSM[0])/(thetaCSM[0] * WL - thetaC[0] * WL)*(thetaCSM[0] * WL - the);
	    if(Choise < Refl[0]) dir[0] *= -1.;
	    else *prob *= (double) exp(- expon[0]);
	  }
	if(the >  thetaCSM[0] * WL) *prob *= (double) exp(- expon[0]);
      }

    } else if (SpinVector[quant_dir] == -1.) {

      /* spin down */
      if(the <= thetaC[1] * WL) {
	dir[0] *= -1.;
      } else {
	expon[1] = (mued[2] * WL + mued[3]) /(double) sqrt(sq((double) sin(the)) - sq((double) sin(thetaC[1] * WL)));

	if(expon[1] > 500) expon[1] = 500;
	if((the > thetaC[1] * WL)&&(the <= thetaCSM[1] * WL))
	  {
	    Refl[1] = RthetaCSM[1] + (1. - RthetaCSM[1])/(thetaCSM[1] * WL - thetaC[1] * WL)*(thetaCSM[1] * WL - the);
	    if (Choise < Refl[1])
	      dir[0] *= -1.;
	    else
	      *prob *= (double) exp(- expon[1]);
	  }
	if (the >  thetaCSM[1] * WL)
	  *prob *= (double) exp(- expon[1]);
      }
    }
  }

  /* transform back into original frame  */

  if((mrangh!=0.)||(mrangv!=0.)) {
    RotBackVector(RotMatrixRang, rt);
    RotBackVector(RotMatrixRang, dir);
  }

  RotBackVector(RotMatrixWall, rt);
  AddVector(rt, WallOffset);
  RotBackVector(RotMatrixWall, dir);

  dir[0]	= (double) sqrt(1 - sq(dir[1]) - sq(dir[2]));

  /* compute pathlength up to collision */

  path = DistVector(rt, pos);

  CopyVector(rt, pos);
  return path;
}

