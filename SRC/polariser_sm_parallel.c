/********************************************************************************************/
/*  VITESS module polariser_sm_oarallel.c                                                  */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  JAN 2010  M. Fromme helper threads                                                  */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "threadHelper.h"

#define STRING_BUFFER 50

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */

char       Option[STRING_BUFFER], *ParameterFileName, *ReflUpFileName, *ReflDownFileName, XFileName[STRING_BUFFER];
double     RotMatrixSM[3][3], RotMatrixField[3][3], RotMatrixOut[3][3], RotMatrixAnalysis[3][3];
double     rupdata[1001], rdowndata[1001], NumberPrecessions, PhaseShift;
double     AngleSMHoriz, AngleSMVert, AnglOutHoriz, AnglOutVert, smearfUp, smearfDown;
double     WidthCh, WallTh;
double     ProbCutoff;
VectorType analysis_dir, guide_field, PosSM, DimSM, TranslOut;
long       NoCh;


void processNeutron (int i, int thread_i) {

  double TOF, TOFprec, WL, Prob, aUU, aDD, n[3];
  double phi, the, phinew, thenew, shift;
  double LarmorMatrix[3][3];
  VectorType Pos, Dir, SpinVector, pos;
  int m, datanumber;
  Neutron neutron;

  TOF = InputNeutrons[i].Time;
  WL = InputNeutrons[i].Wavelength;
  Prob = InputNeutrons[i].Probability;

  CopyVector(InputNeutrons[i].Position, Pos);
  CopyVector(InputNeutrons[i].Vector, Dir);
  CopyVector(InputNeutrons[i].Spin, SpinVector);

  InputNeutrons[i].Vector[0] = sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

  /* translate into frame of the SM  */
  SubVector(Pos, PosSM);
  RotVector(RotMatrixSM, Pos);
  RotVector(RotMatrixSM, Dir);

  /* calculate spin vector in the direction of the analysis */
  RotVector(RotMatrixAnalysis, SpinVector);
  CartesianToSpherical(SpinVector, &the, &phi);

  /* select channel and shift vertically to its frame */
  n[1]=n[2]=0.;
  n[0]=1.;

  if (PlaneLineIntersect(Pos, Dir, n, - DimSM[0]/2., pos) == TRUE &&
      fabs(pos[1]) < DimSM[1]/2. &&
      fabs(pos[2]) < DimSM[2]/2. &&
      Dir[0] > 0.) {

    double epsilonZ = (pos[2] + DimSM[2]/2.) / DimSM[2] * (double) NoCh;

    int No = (int) floor(epsilonZ)+1;

    shift = (WidthCh + WallTh) *(- (NoCh -1)/2. + (No - 1));
    Pos[2] -= shift;

    if (fabs(pos[2] - shift) > WidthCh/2.) return;

  }
  else return;

  TOFprec = 0.;

  /* reflecting in channels top/bottom */

  for (m = 1; m < 1000; m++) {

    int r=0;

    CHECK;

    /* reflection on top/bottom */
    n[0]=n[1]=0.;
    n[2]=1.;

    if (
	(PlaneLineIntersect(Pos, Dir, n, + WidthCh/2., pos) == TRUE &&
	 pos[0] > Pos[0]+0.1 &&
	 fabs(pos[0]) < DimSM[0]/2. &&
	 Dir[2] > 0.)
	||
	(PlaneLineIntersect(Pos, Dir, n, - WidthCh/2., pos) == TRUE &&
	 pos[0] > (Pos[0]+0.1) &&
	 fabs(pos[0]) < DimSM[0]/2. &&
	 Dir[2] < 0.)
	) {

      // polarizing
      double szog = fabs(asin(Dir[2]));
      
      datanumber = (int) (szog *180./M_PI * 1000./WL); 
      if (datanumber > 1000) return;

      aUU = sqrt(rupdata[datanumber]);    // 1.
      aDD = sqrt(rdowndata[datanumber]);  // 0.
      if (aUU == 0. && aDD == 0.) return;

      thenew = 2. * atan(aDD/ aUU * tan(the/2.));
      phinew = phi;
      Prob *= sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.));

      TOFprec += (pos[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
      
      CopyVector(pos, Pos);

      Dir[2] *= -1;
      r = 1;

      the = thenew;
      phi = phinew;
    }

    /* absorbtion on the sides */
    n[0]=n[2]=0.;
    n[1]=1.;
    
    if (PlaneLineIntersect(Pos, Dir, n, + DimSM[1]/2, pos) == TRUE &&
	Dir[1] > 0. &&
	pos[0] > Pos[0] &&
	fabs(pos[0]) < DimSM[0]/2.)
      return;

    if (PlaneLineIntersect(Pos, Dir, n, - DimSM[1]/2, pos) == TRUE &&
	Dir[1] < 0. &&
	pos[0] > Pos[0] &&
	fabs(pos[0]) < DimSM[0]/2.)
      return;

    if (r==0) break; // cannot be reflected anymore

  } // end for m loop

  /* leave channel and shift vertically back to main SM frame */
  Pos[2] += shift;
  SphericalToCartesian(SpinVector, &the, &phi);

  if (Prob <= ProbCutoff) return;

  /* translate into initial frame   */
  RotBackVector(RotMatrixSM, Pos);
  RotBackVector(RotMatrixSM, Dir);
  AddVector(Pos, PosSM);

  /* compute neutron variables in the output frame */
  SubVector(Pos, TranslOut);
  RotVector(RotMatrixOut, Pos);
  RotVector(RotMatrixOut, Dir);
  RotVector(RotMatrixOut, SpinVector);

  /* translate neutron variables for output - X'=0. */
  {
    VectorType dpath; // displacement vector

    TOFprec -=  Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
    CopyVector(Dir, dpath);
    MultiplyByScalar(dpath, - Pos[0]/ Dir[0] );
    AddVector(Pos, dpath);
  }

  /* precession in the guide field */
  RotVector(RotMatrixField, SpinVector);

  PhaseShift = TOFprec * FREQUENCY_FROM_FIELD(guide_field[0]);
  NumberPrecessions = PhaseShift/2./M_PI;

  FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);
  RotVector(LarmorMatrix, SpinVector);
  RotBackVector(RotMatrixField, SpinVector);

  /* transmit coordinates which were not changed, overwrite the rest */
  neutron = InputNeutrons[i];

  neutron.Time = TOF + TOFprec;
  neutron.Probability = Prob;

  CopyVector(Pos, neutron.Position);
  CopyVector(Dir, neutron.Vector);
  CopyVector(SpinVector, neutron.Spin);

  WriteNeutronParallel(&neutron, thread_i);

 my_exit:;
}


double reflectivity(double angle, double CritUp, double WL, double smearf, double ReflUp)
{
  return ReflUp/(exp((angle - CritUp*WL)/CritUp/WL/smearf)+1);
}


void ReadParameterFile(FILE *f) {

  DimSM[0] = ReadParF(f); 
  DimSM[2] = ReadParF(f);
  DimSM[1] = ReadParF(f);
  ReadParComment(f);

  NoCh    = ReadParI(f);
  WallTh  = ReadParF(f);
  ReadParComment(f);

  guide_field[0] = ReadParF(f);
  guide_field[1] = ReadParF(f);
  guide_field[2] = ReadParF(f);
  ReadParComment(f);

  analysis_dir[0] = ReadParF(f);
  analysis_dir[1] = ReadParF(f);
  analysis_dir[2] = ReadParF(f);
  ReadParComment(f);

  fprintf(LogFilePtr,
	  "\ndata from parameter file: '%s':\n"
	  "\tguide_fieldX=  %9.4f\n"
	  "\tguide_fieldY=  %9.4f\n"
	  "\tguide_fieldZ=  %9.4f\n",
	  ParameterFileName,
	  guide_field[0], guide_field[1], guide_field[2]);
}


void OwnInit(int argc, char *argv[])
{
  FILE *Par_Field=0, *reflup_file=0, *refldown_file=0;
  int count;

  fprintf(LogFilePtr, "\n");
  print_module_name("polarizer_sm_parallel 1.3");

  guide_field[0] = guide_field[1] = guide_field[2] = 0.;

  ProbCutoff=wei_min;

  while(argc>1)
    {
      switch(argv[1][1]) {

	case 'P':
	  if((Par_Field = fopen(&argv[1][2],"r"))==NULL)
	    {
	      fprintf(LogFilePtr,"\nParameter file '%s' not found\n", &argv[1][2]);
	      exit(0);
	    }
	  ParameterFileName=&argv[1][2];
	  break;

	case 'U':
	  if( (reflup_file = fopen(&argv[1][2],"r"))==NULL)
	    {
	      fprintf(LogFilePtr,"File %s could not be opened for InputNeutrons\n",&argv[1][2]);
	      exit(-1);}

	  ReflUpFileName=&argv[1][2];
	  break;

	case 'D':
	  if( (refldown_file = fopen(&argv[1][2],"r"))==NULL)
	    {
	      fprintf(LogFilePtr,"File %s could not be opened for InputNeutrons\n",&argv[1][2]);
	      exit(-1);}

	  ReflDownFileName=&argv[1][2];
	  break;

	case 'a':
	  sscanf(&argv[1][2], "%lf", &PosSM[0]);
	  break;

	case 'b':
	  sscanf(&argv[1][2], "%lf", &PosSM[1]);
	  break;

	case 'c':
	  sscanf(&argv[1][2], "%lf", &PosSM[2]);
	  break;

	case 'H':
	  sscanf(&argv[1][2], "%lf", &AngleSMHoriz);
	  break;

	case 'V':
	  sscanf(&argv[1][2], "%lf", &AngleSMVert);
	  break;

	case 'R':
	  sscanf(&argv[1][2], "%lf", &TranslOut[0]);
	  break;

	case 'E':
	  sscanf(&argv[1][2], "%lf", &TranslOut[1]);
	  break;

	case 'G':
	  sscanf(&argv[1][2], "%lf", &TranslOut[2]);
	  break;

	case 'h':
	  sscanf(&argv[1][2], "%lf", &AnglOutHoriz);
	  break;

	case 'v':
	  sscanf(&argv[1][2], "%lf", &AnglOutVert);
	  break;

	}
      argc--;
      argv++;
    }

  /* read reflectivity files */
  if (reflup_file) {
    for(count=0; count<1000; count++)
      if (fscanf(reflup_file,"%lf",&rupdata[count])==EOF)
	break;
    fclose(reflup_file);
  }

  if (refldown_file) {
    for(count=0; count<1000; count++)
	if (fscanf(refldown_file,"%lf",&rdowndata[count])==EOF)
	  break;
    fclose(refldown_file);
  }

  if (Par_Field) {
    ReadParameterFile(Par_Field);
    fclose(Par_Field);
  }

  /* check some values */
  /* print parameters to log file for verification */

  if ( (NoCh+1)/2. - floor((NoCh+1)/2.) > 0.) {
    NoCh -= 1;
    fprintf(LogFilePtr,"\nWARNING: Number of channels must be odd! Set %ld.\n", NoCh);
  }

  fprintf(LogFilePtr, "\n"
	  "\tcutoff probability      = %8.1e\n"
	  "\tposition x, y, z        = %9.4f, %9.4f, %9.4f\n"
	  "\tlength, height, width   = %9.4f, %9.4f, %9.4f\n"
	  "\toffset angle horiz      = %9.4f\n"
	  "\toffset angle vert       = %9.4f\n"
	  "\toutput horizontal angle = %9.4f\n"
	  "\toutput vertical angle   = %9.4f\n"
	  "\tX',Y',Z'                = %9.4f, %9.4f, %9.4f\n",
	  ProbCutoff,
	  PosSM[0], PosSM[1], PosSM[2], DimSM[0], DimSM[2], DimSM[1], AngleSMHoriz, AngleSMVert,
	  AnglOutHoriz, AnglOutVert, TranslOut[0], TranslOut[1], TranslOut[2]);

  /* convert angles to radian etc. */
  AngleSMHoriz *= M_PI/180.;
  AngleSMVert  *= M_PI/180.;
  AnglOutHoriz *= M_PI/180.;
  AnglOutVert  *= M_PI/180.;

  FillRotMatrixZY(RotMatrixSM, AngleSMVert, AngleSMHoriz);
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz);

  WidthCh = (DimSM[2] - (NoCh+1)*WallTh)/NoCh;

  {
    double roty, rotz;
    CartesianToEulerZY(analysis_dir, &roty, &rotz);
    FillRotMatrixZY(RotMatrixAnalysis, roty, rotz);

    CartesianToEulerZY(guide_field, &roty, &rotz);
    FillRotMatrixZY(RotMatrixField, roty, rotz);
  }

} /* End OwnInit */


void OwnCleanup() { }



int main(int argc, char **argv)
{

  Init(argc, argv, VT_POL_SM);
  OwnInit(argc, argv);

  processPipedNeutrons(NThreads, processNeutron, 1, 0);

  OwnCleanup();
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);

  return 0;
}
