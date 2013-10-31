/************************************************************************************************/
/*  VITESS module 'sm_ensemble_parallel'                                                        */
/*                                                                                              */
/* The free non-commercial use of these routines is granted providing due credit is given to    */
/* the authors.                                                                                 */
/*                                                                                              */
/* 0.1  Jul 2002  G. Zsigmond	  initial version                                               */
/* 1.0  Sep 2002  G. Zsigmond	  active on both sides                                          */
/* 1.1  Apr 2003  S. Manoshin	  visualisation included                                        */
/*              + G. Zsigmond	   and position filter                                          */
/* 1.2  May 2003  G. Zsigmond	  collision file name as input parameter, coll. output changed, */
/*                              quantisation direction incl.                                    */
/* 1.3  Jun 2003  G. Zsigmond	  included if when closing coll file; softabort included        */
/* 1.4  Jan 2004  K. Lieutenant changes for 'instrument.dat'                                    */
/* 1.5  Feb 2004  S. Manoshin   Visualise only first 10000 neutrons                             */
/*                              Choose the output device : screen, file or both                 */
/*                              New external variable gselec                                    */
/* 1.6  Apr 2004  G. Zsigmond	  Visualise only first 1000 neutrons, write sm_ensemble.ps      */
/* 1.7  Feb 2008  K. Lieutenant extension to MAX_MIRR (=13) mirrors                             */
/* 1.8  Apr 2008  M. Fromme changeable constant MAX_MIRR                                        */
/* 1.9  Feb 1010  M. Fromme helper threads                                                      */
/************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"
#include "softabort.h"
#include "threadHelper.h"


#ifdef VT_GRAPH
 #include "cpgplot.h"

/* MF: visualisation for Windows and generation of file for the picture */
 #ifdef DO_WIN32
  #define GDEV "sm_ensemble.ps"
 #else
  #define GDEV "sm_ensemble.png"
 #endif

 extern int gselec;
 // choose the output 1 - display only, 2 - file only,
 //                   3 - both, defined in cpgplot.c
#else
 int gselec;
#endif

/* START HEADER STORY */

#define MAX_MIRR 50
#define QcNI 0.0217434

FILE	   *COLLFILE;
char       *ParameterFileName, *ReflUpFileName, *ReflDownFileName;
const char *COLLFILEName = "collision.dat";
int	   p=0, datanumber, vistype=0, quant_dir=2;
int        max_mirr; // highest number of used mirror
long	   User, NumWrong[MAXWORKER], nocolM = 10000, Wallonoff, NoCh;
long	   number_vis_tr; // current number of visualised trajectories, used to limit output
double	   rupdata[1001], rdowndata[1001], OutputAngleHoriz, OutputAngleVert, RotMatrixOut[3][3];
int        mcperneutron = (MAX_MIRR*3);
int        increaseColor=0;
VectorType TranslOutput,
           WallOffset[MAX_MIRR+1], WallNormal[MAX_MIRR+1],
           r1[MAX_MIRR+1], r2[MAX_MIRR+1], r3[MAX_MIRR+1], r4[MAX_MIRR+1], WallOffsetShift[MAX_MIRR+1];
	
double	   thetaC[MAX_MIRR+1][2], thetaCSM[MAX_MIRR+1][2], RthetaCSM[MAX_MIRR+1][2], mued[MAX_MIRR+1][4],
           mrangh[MAX_MIRR+1], mrangv[MAX_MIRR+1],
           WallVert[MAX_MIRR+1], WallHoriz[MAX_MIRR+1],
           RotMatrixWall[MAX_MIRR+1][3][3],RotMatrixVisElements[MAX_MIRR+1][3][3],
           mNumber[MAX_MIRR+1][2], mirrThickness[MAX_MIRR+1], Qc[MAX_MIRR+1][2],
           Windw = -10.0, WindW = 200.0, Windh = -10.0, WindH = 10.0, wei_min1 = 0.0;

short int  mirrMaterial=0, mirrUsage[MAX_MIRR+1];
short int  fileFormat=0;

int     useQuantDir=1;

void SetGeometryData();
	
/* FINISH HEADER STORY */

#ifdef VT_GRAPH
static void moveIt(VectorType P) {
  switch (p) {
  case 2:
    cpgmove((float) P[0], (float) P[1]);
    cpgpt1 ((float) P[0], (float) P[1], -2);
    break;
  case 3:
    cpgmove((float) P[0], (float) P[2]);
    cpgpt1 ((float) P[0], (float) P[2], -2);
    break;
  case 4:
    cpgmove((float) P[1], (float) P[2]);
    cpgpt1 ((float) P[1], (float) P[2], -2);
  default:;
  }
}

static void drawIt(VectorType P) {
  switch (p) {
  case 2:
    if (vistype) cpgpt1 ((float) P[0], (float) P[1], -2);
    else         cpgdraw((float) P[0], (float) P[1]);
    break;
  case 3:
    if (vistype) cpgpt1 ((float) P[0], (float) P[2], -2);
    else         cpgdraw((float) P[0], (float) P[2]);
    break;
  case 4:
    if (vistype) cpgpt1 ((float) P[1], (float) P[2], -2);
    else         cpgdraw((float) P[1], (float) P[2]);
  default:;
  }
}
#endif



void ReadParameterFile(FILE *f)
{
  // You may give more than MAX_MIRR description lines in the parameter file,
  // but only the first MAX_MIRR mirrors which are marked valid are taken into account.
  // If less than MAX_MIRR description lines are given, max_mirr will be less
  // than MAX_MIRR.
  int l = 1;
  while (1) {
    int use_this_mirror;
    if (feof(f)) break;

    if (fileFormat == 0) {

      r1[l][0] = r2[l][0] = r3[l][0] = r4[l][0] = 0;
      use_this_mirror = ReadParI(f);
      r1[l][1]=ReadParF(f); r1[l][2]=ReadParF(f);
      r2[l][1]=ReadParF(f); r2[l][2]=ReadParF(f);
      r3[l][1]=ReadParF(f); r3[l][2]=ReadParF(f);
      r4[l][1]=ReadParF(f); r4[l][2]=ReadParF(f);
      WallOffset[l][0]=ReadParF(f); WallOffset[l][1]=ReadParF(f); WallOffset[l][2]=ReadParF(f);
      WallHoriz[l]=ReadParF(f); WallVert[l]=ReadParF(f);
      mrangh[l]=ReadParF(f); mrangv[l]=ReadParF(f);
      thetaC[l][0]=ReadParF(f); thetaCSM[l][0]=ReadParF(f); RthetaCSM[l][0]=ReadParF(f);
      mued[l][0]=ReadParF(f); mued[l][1]=ReadParF(f);
      thetaC[l][1]=ReadParF(f); thetaCSM[l][1]=ReadParF(f); RthetaCSM[l][1]=ReadParF(f);
      mued[l][2]=ReadParF(f); mued[l][3]=ReadParF(f);
      ReadParComment(f);
      
      if(use_this_mirror) {
	mirrUsage[l] = use_this_mirror;
	fprintf(LogFilePtr,
		"\n%c:  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f%10.5f  "
		"%10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f%10.5f\n",
		'A'+l-1,r1[l][1],r1[l][2],r2[l][1],r2[l][2],r3[l][1],r3[l][2],r4[l][1],r4[l][2],
		WallOffset[l][0],WallOffset[l][1],WallOffset[l][2], WallHoriz[l],WallVert[l],
		mrangh[l], mrangv[l], thetaC[l][0], thetaCSM[l][0], RthetaCSM[l][0],
		mued[l][0], mued[l][1], thetaC[l][1], thetaCSM[l][1], RthetaCSM[l][1], mued[l][2], mued[l][3]);
	WallHoriz[l] *= M_PI/180.;
	WallVert[l]  *= M_PI/180.;
	mrangh[l]	 *= M_PI/180.;
	mrangv[l]	 *= M_PI/180.;
	FillRotMatrixZY(RotMatrixWall[l],  WallVert[l],  WallHoriz[l]);
	EulerToCartesianZY(WallNormal[l], &WallVert[l], &WallHoriz[l]);
	l++;
	// terminate if we have MAX_MIRR valid mirrors
	if (l > MAX_MIRR)
	  break;
      }
      
    }

    else {

      r1[l][0] = r2[l][0] = r3[l][0] = r4[l][0] = 0;
      use_this_mirror = ReadParI(f);
      r1[l][1]=ReadParF(f); r1[l][2]=ReadParF(f);
      r2[l][1]=ReadParF(f); r2[l][2]=ReadParF(f);
      r3[l][1]=ReadParF(f); r3[l][2]=ReadParF(f);
      r4[l][1]=ReadParF(f); r4[l][2]=ReadParF(f);
      WallOffset[l][0]=ReadParF(f); WallOffset[l][1]=ReadParF(f); WallOffset[l][2]=ReadParF(f);
      WallHoriz[l]=ReadParF(f); WallVert[l]=ReadParF(f);
      mrangh[l]=ReadParF(f); mrangv[l]=ReadParF(f);
      mirrThickness[l] = ReadParF(f); mNumber[l][0]=ReadParF(f); mNumber[l][1]=ReadParF(f);
      ReadParComment(f);
            

      if(use_this_mirror) {
	mirrUsage[l] = use_this_mirror;
	fprintf(LogFilePtr,
		"\n%c:  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f%10.5f  "
		"%10.5f%10.5f  %10.5f%10.5f  %10.5f%10.5f %10.5f \n",
		'A'+l-1,r1[l][1],r1[l][2],r2[l][1],r2[l][2],r3[l][1],r3[l][2],r4[l][1],r4[l][2],
		WallOffset[l][0],WallOffset[l][1],WallOffset[l][2], WallHoriz[l],WallVert[l],
		mrangh[l], mrangv[l], mirrThickness[l], mNumber[l][0], mNumber[l][1]);
	WallHoriz[l] *= M_PI/180.;
	WallVert[l]  *= M_PI/180.;
	mrangh[l]	 *= M_PI/180.;
	mrangv[l]	 *= M_PI/180.;
	FillRotMatrixZY(RotMatrixWall[l],  WallVert[l],  WallHoriz[l]);
	EulerToCartesianZY(WallNormal[l], &WallVert[l], &WallHoriz[l]);

	Qc[l][0] = QcNI;
	Qc[l][1] = QcNI;

	if (mNumber[l][0] < 1.0)
	  Qc[l][0] *= mNumber[l][0];
	
	if (mNumber[l][1] < 1.0)
	  Qc[l][1] *= mNumber[l][1];
	
	thetaC[l][0] = asin(Qc[l][0]/(4*M_PI));
	thetaC[l][1] = asin(Qc[l][1]/(4*M_PI));

	l++;
	// terminate if we have MAX_MIRR valid mirrors
	if (l > MAX_MIRR)
	  break;
      } 

    }
    // else this mirror is to be skipped
  }
  max_mirr = l - 1;

  fprintf(LogFilePtr,"\n");
  for (l=1; l<=max_mirr; l++)
    fprintf(LogFilePtr,"%c", 'A'+l-1);

  fprintf(LogFilePtr,"\n");
  fclose(f);

}/* End ReadParameterFile  */


void OwnInit(int argc, char *argv[])
{
  FILE *Par_Field=0;
  int j;

  fprintf(LogFilePtr, "\n");
  print_module_name("supermirror_ensemble_parallel 1.9");

  for(j=0; j<3; j++)
    TranslOutput[j] = 0.;
  OutputAngleHoriz = OutputAngleVert = 0.;

  gselec = 1; /* Activate visualisation device -screen */

  while(argc>1) {
    char *arg = &argv[1][2];
    switch(argv[1][1]) {
				
    case 'C':
      COLLFILEName = arg;
      break;
    case 'h':
      sscanf(arg, "%lf", &OutputAngleHoriz);
      break;
    case 'M':
      sscanf(arg, "%ld", &nocolM);
      fprintf(LogFilePtr,"Stops at %ld collisions.\n", nocolM);
      break;
    case 'P':
      if((Par_Field = fopen(arg,"r")) == NULL) {
	fprintf(LogFilePtr,"\nERROR: Parameter file '%s' not found.\n", arg);
	exit(0);
      }
      ParameterFileName=arg;
      break;		
    case 'Q':
      sscanf(arg, "%d", &quant_dir);
      if (quant_dir < -1 || quant_dir > 2) {
	fprintf(LogFilePtr, "\nERROR:  wrong quantization direction definition.\n");
	exit(0);
      }
      else if (quant_dir == -1) {
	useQuantDir = 0;
      }
      break;
    case 'R':
      sscanf(arg, "%i", &increaseColor);
    case 'r':
      sscanf(arg, "%lf", &TranslOutput[0]);
      break;
    case 's':
      sscanf(arg, "%lf", &TranslOutput[1]);
      break;
    case 't':
      sscanf(arg, "%lf", &TranslOutput[2]);
      break;
    case 'T':
      sscanf(arg, "%d", &p);
      break;
    case 'v':
      sscanf(arg, "%lf", &OutputAngleVert);
      break;
    case 'S':
      sscanf(arg, "%hd", &mirrMaterial);
      break;
    case 'F':
      sscanf(arg, "%d", &fileFormat);
    break;

      // Visual data
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
    case 'w':
      sscanf(arg, "%lf", &Windw);
      break;					
    case 'W':
      sscanf(arg, "%lf", &WindW);
      break;
										
      // Monte Carlo prefetch count for helper threads
    case 'm':
      sscanf(arg, "%d", &mcperneutron);
      break;
    }
    argc--;
    argv++;
  }
	

  if (mirrMaterial == 3 && fileFormat == 1) {
    fprintf(LogFilePtr, "The new file format cannot be used with OTHER mirror material!\n");
    exit(-1);
  }

  if (p==1)
    fprintf(LogFilePtr, "Prints the coordinates of collisions to '%s'.\n", COLLFILEName);
	
#ifdef VT_GRAPH
  else if (p >= 2 && p <= 4) {
	
    /* initialize pgplot with device GraphDev */

    if (gselec == 1)
      fprintf(LogFilePtr,"Open visual output device - display\n");
    else if (gselec == 2)
      fprintf(LogFilePtr,"Open visual output device - file\n");
    else if (gselec == 3)
      fprintf(LogFilePtr,"Open visual output device - display+file\n");
    else {
      fprintf(LogFilePtr,"ERROR: Incorrect output device, correct option -o, value 1,2 or 3\n");
      exit(-1);
    }

    fprintf(LogFilePtr,"Visual activation for the first  %ld  trajectories.\n", BufferSize);

    /* initialize pgplot with device GraphDev  */
    if (cpgopen(GDEV) < 1) {
      fprintf(LogFilePtr,"ERROR: I cannot open graphical output device.\n");
      exit(-1);
    }

    fprintf(LogFilePtr,"Minimum neutron weigth for visualisation: %f.\n", wei_min1);

    cpgenv((float) Windw, (float) WindW, (float) Windh, (float) WindH, 0, 0);
    cpgsfs((int) 2);
    cpgsch((float) 1.2);
    switch (p) {
    case 2: cpglab("X, cm","Y, cm","NEUTRON VISUALISATION - PLANE X0Y"); break;
    case 3: cpglab("X, cm","Z, cm","NEUTRON VISUALISATION - PLANE X0Z"); break;
    case 4: cpglab("Y, cm","Z, cm","NEUTRON VISUALISATION - PLANE Y0Z"); break;
    }
  }
#endif	


  OutputAngleHoriz *= M_PI/180.;
  OutputAngleVert  *= M_PI/180.;
  FillRotMatrixZY(RotMatrixOut, OutputAngleVert, OutputAngleHoriz);
	
  if (Par_Field)
    ReadParameterFile(Par_Field);

  //Determine geometry for visualisation
  SetGeometryData();

} /* End OwnInit */


void UpdateMirrorElementOffset(VectorType edgeVector, VectorType diagonalVector, VectorType newOffset, int nMirror)
{

  int nComp;
  
  for (nComp = 0; nComp < 3; nComp++) newOffset[nComp] = edgeVector[nComp] + 0.5*diagonalVector[nComp];

  CopyVector(newOffset, WallOffsetShift[nMirror]);

}


void DetermineAndLogMirrorShape(int i)
{

	// Initialise all 6 vectors in a quadrangle
  VectorType v[6];
  VectorType elementCenterOffset;
  VectorType elementCenterOffsetTemp;
  int largestVectorIndex = -1;
  int secondLargestVectorIndex = -1;
  double largestVector = 0.;
  double width = 0.;
  int widthIndex = -1;
  double height = 0.;
  int heightIndex = -1;
  int j;
  int numRightAngles[4];
  int counter = 0;
  short isRectangle = 1;	

  CopyVector(r2[i], v[0]);  
  SubVector(v[0], r1[i]);

  CopyVector(r3[i], v[1]);  
  SubVector(v[1], r2[i]);
  
  CopyVector(r4[i], v[2]);  
  SubVector(v[2], r3[i]);

  CopyVector(r1[i], v[3]);  
  SubVector(v[3], r4[i]);

  CopyVector(r3[i], v[4]);  
  SubVector(v[4], r1[i]);

  CopyVector(r4[i], v[5]);  
  SubVector(v[5], r2[i]);

  fprintf(LogFilePtr, "%10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f  %10.5f %10.5f% 10.5f   %10.5f %10.5f %10.5f \n", 
	  r1[i][0], r1[i][1], r1[i][2], r2[i][0], r2[i][1], r2[i][2], r3[i][0], r3[i][1], r3[i][2], r4[i][0], r4[i][1], r4[i][2]);

  // Determine the two largest vectors of the mirror element
  // In a rectangle these are the two diagonals
  for (j = 0; j < 6; j++) {
    double lengthVector = LengthVector(v[j]);
    if (lengthVector >= largestVector) {
      secondLargestVectorIndex = largestVectorIndex;
      largestVectorIndex = j;
      largestVector = lengthVector;
    }
   /* if (widthIndex < 0) {
      width = lengthVector;
      widthIndex = j;
    }*/
    if (heightIndex < 0) {
      height = lengthVector;
      heightIndex = j;
    }
    if (lengthVector < height) {
      height = lengthVector;
      heightIndex = j;
    }
   /* else if (lengthVector > height && lengthVector < largestVector) {
      width = lengthVector;
      widthIndex = j;
    }*/
  }
  width = height;
  widthIndex = heightIndex;
  for (j = 0; j < 6; j++) {
	if (LengthVector(v[j]) < LengthVector(v[secondLargestVectorIndex]) && LengthVector(v[j]) > width) {
			width = LengthVector(v[j]);
			widthIndex = j;
		}
  }

  // Update the element offset, needed for a correct calculation of reflection points
  // The offset MUST reside within the element!

  CopyVector(WallOffset[i], elementCenterOffsetTemp);

  switch (largestVectorIndex) {
  case 0: 
    UpdateMirrorElementOffset(r1[i], v[0], elementCenterOffset, i);
    break;
  case 1: 
    UpdateMirrorElementOffset(r2[i], v[1], elementCenterOffset, i);
    break;  
  case 2: 
    UpdateMirrorElementOffset(r3[i], v[2], elementCenterOffset, i);
    break;  
  case 3: 
    UpdateMirrorElementOffset(r4[i], v[3], elementCenterOffset, i);
    break;
  case 4: 
    UpdateMirrorElementOffset(r1[i], v[4], elementCenterOffset, i);
    break;
  case 5: 
    UpdateMirrorElementOffset(r2[i], v[5], elementCenterOffset, i);
    break;
  default:
    fprintf(LogFilePtr, "Offset of mirror element Nr. %d could not be updated! \n", i+1);
    break;
  }
 
  // Determine whether we deal with a rectangle
  // 4 right angles have to be present
  
  for (j = 0; j < 4; j++) numRightAngles[j] = 0;
	
  for (j = 0; j < 6; j++) {
	  int k;	
	if (j == largestVectorIndex || j == secondLargestVectorIndex) continue;

    for (k = j; k < j+6; k++) {
      int kk = k%6;
	  double scalarProd = fabs(ScalarProduct(v[j], v[kk]));	
      double angle = acos(scalarProd/(LengthVector(v[j])*LengthVector(v[kk])))*180./M_PI;
      if (kk == largestVectorIndex || kk == secondLargestVectorIndex) continue;
      if (angle > 89.9)  numRightAngles[counter]++;
    }
    
    counter++;
  }

  for (j = 0; j < 4; j++) isRectangle &= (numRightAngles[j] == 2);
  
  // Set the rectangle parameters
  if (isRectangle) {

	VectorType zAxis = {0, 0, 1};
    double rotationAngle = acos(fabs(ScalarProduct(v[heightIndex], zAxis)/LengthVector(v[heightIndex])));		
    stGeometry.nRectangles++;

    RotBackVector(RotMatrixWall[i], elementCenterOffset);
    AddVector(elementCenterOffset, elementCenterOffsetTemp);
    CopyVector(elementCenterOffset, stGeometry.pRectangle[stGeometry.nRectangles-1].vCntr);
    CopyVector(WallNormal[i], stGeometry.pRectangle[stGeometry.nRectangles-1].vNormal);
    stGeometry.pRectangle[stGeometry.nRectangles-1].Width = width;
    stGeometry.pRectangle[stGeometry.nRectangles-1].Height = height;
    stGeometry.pRectangle[stGeometry.nRectangles-1].rotAngle = rotationAngle/M_PI*180.;

  }
  // Build the quadrangle with tho triangles, if mirror is a triangle, the second one is a line
  else {

    VectorType basePoint1, basePoint2, thirdPoint1, thirdPoint2;
    switch(largestVectorIndex) {
    
    case 0: 
      CopyVector(r1[i], basePoint1);
      CopyVector(r2[i], basePoint2);
      CopyVector(r3[i], thirdPoint1);
      CopyVector(r4[i], thirdPoint2);
      break;
   
    case 1:
      CopyVector(r2[i], basePoint1);
      CopyVector(r3[i], basePoint2);
      CopyVector(r4[i], thirdPoint1);
      CopyVector(r1[i], thirdPoint2);
      break;
   
     case 2: 
       CopyVector(r3[i], basePoint1);
       CopyVector(r4[i], basePoint2);
       CopyVector(r1[i], thirdPoint1);
       CopyVector(r2[i], thirdPoint2);
      break;
     
    case 3: 
      CopyVector(r4[i], basePoint1);
      CopyVector(r1[i], basePoint2);
      CopyVector(r2[i], thirdPoint1);
      CopyVector(r3[i], thirdPoint2);
      break;
     
    case 4:
      CopyVector(r1[i], basePoint1);
      CopyVector(r3[i], basePoint2);
      CopyVector(r2[i], thirdPoint1);
      CopyVector(r4[i], thirdPoint2);
      break;
    
    case 5: 
      CopyVector(r2[i], basePoint1);
      CopyVector(r4[i], basePoint2);
      CopyVector(r1[i], thirdPoint1);
      CopyVector(r3[i], thirdPoint2);
      break;
        
    default:
      fprintf(LogFilePtr, "Something went wrong when preparing mirror Nr. %d for visualization! \n", i+1);
      exit(-1);
      break;
    }

     /* fprintf(LogFilePtr, "Triangle points: %10.5f %10.5f %10.5f   %10.5f %10.5f %10.5f  %10.5f %10.5f% 10.5f   %10.5f %10.5f %10.5f \n",  */
     /* 	     basePoint1[0], basePoint1[1], basePoint1[2], basePoint2[0], basePoint2[1], basePoint2[2],  */
     /* 	     thirdPoint1[0], thirdPoint1[1], thirdPoint1[2], thirdPoint2[0], thirdPoint2[1], thirdPoint2[2]); */

    stGeometry.nTriangles++;    
    CopyVector(basePoint1, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0],  WallOffset[i]);
    CopyVector(basePoint2, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1],  WallOffset[i]);
    CopyVector(thirdPoint1, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2],  WallOffset[i]);

    

    stGeometry.nTriangles++;
    CopyVector(basePoint1, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[0],  WallOffset[i]);
    CopyVector(basePoint2, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[1],  WallOffset[i]);
    CopyVector(thirdPoint2, stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2]);
    RotBackVector(RotMatrixWall[i], stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2]);
    AddVector(stGeometry.pTriangle[stGeometry.nTriangles-1].vEdges[2],  WallOffset[i]);

  }

}

void SetGeometryData()
{
 
  bVisInstalled = TRUE;
 
  // Geometry data
 if (bVisInstr) {
   int i;

   stGeometry.pRectangle = calloc(max_mirr, sizeof(VtRectangle));
   stGeometry.pTriangle = calloc(max_mirr*2, sizeof(VtTriangle));
   
   for (i = 1; i <= max_mirr; i++) {
     DetermineAndLogMirrorShape(i);
   }

   stGeometry.pDescr  = "sm ensemble:green";
   stGeometry.eModule = VT_SM_ENSEMBLE;

 }

}

void OwnCleanup()
{
  
	int i,c;

  
#ifdef VT_GRAPH
  if (p >= 2) cpgclos();
#endif
  if (NThreads > 0)
    printMCStatistic(LogFilePtr);
  c = 0;
  for (i=0; i<=NThreads; i++)
    c += NumWrong[i];
  if (c)
    fprintf(LogFilePtr,"\nERROR:  %d collided trajectories have wrong input spin.\n\n", c);
}



/* Theta and Phi of a unit vector if Theta is the angle with the axis of lowest index  */

static VINLINE void CartesianToSpherical2(const VectorType Vector, double *Theta, double *Phi)
{
  *Theta = acos(Vector[0]);

  if (Vector[2] >= 0)
    *Phi = atan2(Vector[2], Vector[1]);
  else
    *Phi = 2. * M_PI + atan2(Vector[2], Vector[1]);
}

static VINLINE int cmpAreas (const VectorType r1, const VectorType r2, const VectorType rt) {
  // Area(r1,r2) > Area(r1,rt) + Area(r2, rt)
  double lv1,lv2,lvt, lva,lvb,lvc, sp12,sp1t,sp2t;
  lv1 = r1[0]*r1[0] + r1[1]*r1[1] + r1[2]*r1[2]; // LengthVector²
  lv2 = r2[0]*r2[0] + r2[1]*r2[1] + r2[2]*r2[2];
  lvt = rt[0]*rt[0] + rt[1]*rt[1] + rt[2]*rt[2];
  sp12 = r1[0]*r2[0] + r1[1]*r2[1] + r1[2]*r2[2]; // ScalarProduct(r1,r2)
  sp1t = r1[0]*rt[0] + r1[1]*rt[1] + r1[2]*rt[2]; // ScalarProduct(r1,rt)
  sp2t = r2[0]*rt[0] + r2[1]*rt[1] + r2[2]*rt[2]; // ScalarProduct(r2,rt)
  lva = sqrt(lv1*lv2);
  lvb = sqrt(lv1*lvt);
  lvc = sqrt(lv2*lvt);
  return lva*fabs(sin(acos(sp12/lva))) >
    lvb*fabs(sin(acos(sp1t/lvb))) + lvc*fabs(sin(acos(sp2t/lvc)));
}

int hittriangle(const VectorType r1, const VectorType r2, const VectorType rt)
{
  double the1, the2, thet, phi1, phi2, phit;

  CartesianToSpherical2(r1, &the1, &phi1);
  CartesianToSpherical2(r2, &the2, &phi2);

  if (phi1 == phi2) return 0;

  CartesianToSpherical2(rt, &thet, &phit);

  if (phi1 < phi2)
    return phi1 < phit && phit < phi2 && cmpAreas(r1,r2,rt);

  // phi1 > phi2
  return ((phi1 < phit && phit < 2*M_PI) || (0. < phit && phit < phi2)) && cmpAreas(r1,r2,rt);
}

/* hitwall computes if the neutron hits the wall */

int hitwall(const VectorType r1, const VectorType r2, const VectorType r3, const VectorType r4, const VectorType rt)
{
  return
    hittriangle(r1, r2, rt) ||
    hittriangle(r2, r3, rt) ||
    hittriangle(r3, r4, rt) ||
    hittriangle(r4, r1, rt);
}


/* CollideWall computes the collision with a wall */
static double CollideWall
(const int thread_i, const double WL, const VectorType SpinVector,
 double *prob, VectorType pos, VectorType dir, VectorType spin, short int l)
{
  VectorType rt, rt2;
  double path, RotMatrixRang[3][3];
  int angularSpread;

  if (PlaneLineIntersect(pos, dir, WallNormal[l], ScalarProduct(WallOffset[l], WallNormal[l]), rt) == 0)
    return 99999;

  { VectorType replacement;

    CopyVector(rt, replacement);
    SubVector(replacement, pos);

    if (ScalarProduct(replacement, dir) < 0.) return 99999;	
  }

  if ((angularSpread = mrangh != 0 || mrangv != 0)) {
    // random angular spread
    double rangh, rangv;
    rangh = mrangh[l] == 0 ? 0 : MonteCarloPar(- mrangh[l]/2., mrangh[l]/2., thread_i);
    rangv = mrangv[l] == 0 ? 0 : MonteCarloPar(- mrangv[l]/2., mrangv[l]/2., thread_i);
    FillRotMatrixZY(RotMatrixRang, rangv, rangh);
  }	

  /* transform into frame of the wall  */
	
  SubVector(rt, WallOffset[l]);
  RotVector(RotMatrixWall[l], rt);
  RotVector(RotMatrixWall[l], dir);

  if (angularSpread) {
    RotVector(RotMatrixRang, rt);
    RotVector(RotMatrixRang, dir);
  }

  /* now check for collision; compute  */
  {
    double the, Refl[2], expon[2];
    double Choice= MonteCarloPar(0,1, thread_i);
    //  double alphaQ=0, betaQ=0, Q=0, M2=0, W=0, R0=0.99;
    int index_expon=0, index_mued1=1, index_mued2=2, index_theta=0;
    double d = 0;
    
    // Shift vector according to the new offset of the mirror element
    SubVector(rt, WallOffsetShift[l]); 
    
    if (! hitwall(r1[l], r2[l], r3[l], r4[l], rt)) {		

      AddVector(rt, WallOffsetShift[l]); 
      RotBackVector(RotMatrixWall[l], dir);      
      return 99999;
    }
    
    // Now shift back to the correct frame
    AddVector(rt, WallOffsetShift[l]); 

    CopyVector(rt, rt2);
		
    the = M_PI_2 - acos(fabs(dir[0]));

    if (SpinVector[quant_dir] == 1. && useQuantDir) {/* spin up */
      index_expon=0; index_mued1=0; index_mued2=1; index_theta=0;
    } else if (SpinVector[quant_dir] == -1. && useQuantDir) {/* spin down */
      index_expon=1; index_mued1=2; index_mued2=3; index_theta=1;
    } else if (useQuantDir){
      NumWrong[thread_i]++;
      *prob = 0.;
    }

     
    if (mirrUsage[l] != 2) { // Mirror works in transmission mode
      if (mirrMaterial == 1) {
	// Silicon
	double x =  ENERGY_FROM_LAMBDA(WL)/1000.; //Energy in meV
	
	double p0 = 0.835703;
	double p1 = 1.05449;
	double p2 = 0.0418079;
	double p3 = -5.17319e-06;
	double p4 = -422.076;  
	
	if (fileFormat == 0) d = (mued[l][index_mued1] + mued[l][index_mued2]) / 0.037; // Calculate the thickness of the mirror element
	else d = mirrThickness[l];
	
	//Fit valid only between 0.4 A and 25 A
	if (WL < 0.4) x = ENERGY_FROM_LAMBDA(0.4)/1000.;
	else if (WL > 25) x = ENERGY_FROM_LAMBDA(25)/1000.;
	
	// Fit to the Si 295° curve obtained by Freund, NIM A 213 (1983), 495 - 501, used energy as input
	expon[index_expon] = 0.0499*(p0 + p1*sqrt(1./x) + p2*sqrt(x) + p3*pow(x + p4, 2.))*d/ sqrt(sq(sin(the)) - sq(sin(thetaC[l][index_theta] * WL))); // 0.0499: \sigma --> \mu in 1/cm
      }
      
      else if (mirrMaterial == 2) {
	//Sapphire

	double mu1 = 0.0055;
	double mu2 = -0.005;
	double mu3 = 0.00159;
	if (fileFormat == 0) d = mued[l][index_mued1] / mu1; // Calculation of the mirror thickness
	else d = mirrThickness[l];
	
	expon[index_expon] = (mu1* WL + mu2 + mu3/WL) / sqrt(sq(sin(the)) - sq(sin(thetaC[l][index_theta] * WL)));
      }
      else {
	expon[index_expon] = (mued[l][index_mued1] * WL + mued[l][index_mued2]) / sqrt(sq(sin(the)) - sq(sin(thetaC[l][index_theta] * WL)));
      }
    }
    else { // Mirror does not transmit, e.g. is a guide wall
      expon[index_expon] = 500.;
    }

    if (the <= thetaC[l][index_theta] * WL) {
      if (Choice < 0.99) 
	dir[0] *= -1.;
      else 
	*prob *= (double) exp(- expon[index_expon]);
    }	else  {	

      if(expon[index_expon] > 500)
	expon[index_expon] = 500;

      if (the > thetaC[l][index_theta] * WL) {

	// McStas reflection model:
	// Refl[index_theta] = R0 * 0.5*(1.0-tanh((Q-M2*Qc[l][index_theta])/W)) * (1.0 - alphaQ*(Q-Qc[l][index_theta]) + betaQ*(Q-Qc[l][index_theta])*(Q-Qc[l][index_theta]));

	Refl[index_theta] = ReflSN(WL, the*180./M_PI, mNumber[l][index_theta]);

	if (Choice < Refl[index_theta])
	  dir[0] *= -1.;
	else
	  *prob *= exp(- expon[index_expon]);
      }
     
    }
    
  }
  
  /* transform back into original frame  */
	
  if (angularSpread) {
    RotBackVector(RotMatrixRang, rt);
    RotBackVector(RotMatrixRang, dir);
  }

  RotBackVector(RotMatrixWall[l], rt);
  AddVector(rt, WallOffset[l]);
  RotBackVector(RotMatrixWall[l], dir);

  dir[0] = sqrt(1 - sq(dir[1]) - sq(dir[2]));

  /* compute pathlength until collision */

  path = DistVector(rt, pos);

  CopyVector(rt, pos);

  return path;
}


void processNeutron (int i, int thread_i) {

  VectorType
    Pos, Dir, SpinVector,
    pos[MAX_MIRR+1], dir[MAX_MIRR+1], spin[MAX_MIRR+1];
  double
    TOF, WL, Prob, Path0, Path,
    prob[MAX_MIRR+1],
    PathA[MAX_MIRR+1];
  int j,m, nocol;

  // dmf test
  memset(PathA, 0, sizeof(double)*(MAX_MIRR+1));

  if (p > 1 && number_vis_tr == 1000)
    p = 100;  // stop plotting trajectories

  InputNeutrons[i].Vector[0] = sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

  TOF  = InputNeutrons[i].Time;
  WL   = InputNeutrons[i].Wavelength;
  Prob = InputNeutrons[i].Probability;

  CopyVector(InputNeutrons[i].Position, Pos);
  CopyVector(InputNeutrons[i].Vector, Dir);
  CopyVector(InputNeutrons[i].Spin, SpinVector);

  /*  compute hit positions on the walls */

  Path0 = 0.;
  m = nocol = 0;

  if (p) {
    if (p==1)
      fprintf(COLLFILE,
	      "     %c%c%07ld %c %5d    %10d  %2d  0     %12.5f  %12.5f  %12.5f     %12.5f  %12.5f\n",
	      InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo,
	      InputNeutrons[i].Debug, InputNeutrons[i].Color, i, ((int) SpinVector[quant_dir]),
	      Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));

#ifdef VT_GRAPH
    else if (p>=2 && p<=4) {
      cpgsci((int)(InputNeutrons[i].Color));
      if(Prob > wei_min1)
	moveIt(Pos);
    }
#endif
  }

  Path = 0;

  for (j=0; j<1000; j++) { // loop over at most 1000 collisions
    int im, l;

    CHECK;

    for (l=1; l<=max_mirr; l++) { // loop over mirrors
      CopyVector(Pos, pos[l]);
      CopyVector(Dir, dir[l]);
      CopyVector(SpinVector, spin[l]);
      prob[l]= Prob;
      if (m != l) {
	PathA[l] = CollideWall(thread_i, WL, SpinVector,
			       &prob[l], pos[l], dir[l], spin[l], l);
      }
      else
	PathA[l] = 99999;
    }                            // end loop over mirrors

    for (im=1; im <= max_mirr; im++)
      if (PathA[im] != 99999.0)
	break;

    if (im > max_mirr) {
      // all PathA are 99999
      Path = 99999.0;
      break; // leave collsions loop
    }

    for (l=1; l<=max_mirr; l++) { // loop over mirrors
      Neutron myneutron, *n;
      n = &myneutron;
      CopyNeutron(&InputNeutrons[i], n);

      if (m == l) continue;
      for (im=1; im<=max_mirr; im++)
	if (im != l && PathA[l] > PathA[im])
	  break;

      if (im <= max_mirr) continue; // next mirror, because PathA[l] > PathA[im]

      CopyVector(pos[l], Pos);
      CopyVector(dir[l], Dir);
      Path = PathA[l];
      Prob = prob[l];
      m = l;
      nocol++;
	  
      CopyVector(Pos, n->Position);
      CopyVector(Dir, n->Vector);
      WriteIAP(n, VT_REFLECTED);
      if (increaseColor) InputNeutrons[i].Color++;

      if (p) {
	if (p==1)
	  fprintf(COLLFILE,
		  "     %c%c%07ld %c %5d    %10d  %2d  %d     %12.5f  %12.5f  %12.5f     %12.5f  %12.5f\n",
		  InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1],
		  InputNeutrons[i].ID.IDNo, InputNeutrons[i].Debug, InputNeutrons[i].Color, i,
		  ((int) SpinVector[quant_dir]),
		  m, Pos[0], Pos[1], Pos[2], 180./M_PI * atan2(Dir[1],Dir[0]), 180./M_PI * atan2(Dir[2],Dir[0]));
#ifdef VT_GRAPH
	else if (Prob > wei_min1)
	  drawIt(Pos);
#endif
      }

      //     fprintf(LogFilePtr, "ID: %d, End position: %f %f %f  Mirror: %d\n", InputNeutrons[i].ID.IDNo, n->Position[0], n->Position[1], n->Position[2], l);
    }                             // end loop over mirrors

    
	
    if (nocol == nocolM)
      break; // leave collsions loop

    if (Prob < wei_min) return;

    Path0 += Path;

  }  // end loop over collisions

  if (Prob < wei_min) return;

  /* transform into output frame */
  SubVector(Pos, TranslOutput);
  RotVector(RotMatrixOut, Pos);
  RotVector(RotMatrixOut, Dir);

  /* translate neutron variables for output - X'=0. */
  {
    VectorType path; // displacement vector

    if (Pos[0] > 0.0) return; // Filter if collision after output YZ plane

    TOF -= Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
      	
    CopyVector(Dir, path);
    MultiplyByScalar(path, - Pos[0] / Dir[0]);
    AddVector(Pos, path);
  }

  if (p>0) {
    /* transform into input frame to give the exit positions here */
    VectorType posex, direx;
    CopyVector(Pos, posex);
    CopyVector(Dir, direx);
    RotBackVector(RotMatrixOut, direx);
    RotBackVector(RotMatrixOut, posex);
    AddVector(posex, TranslOutput);

    if (p == 1)
      fprintf(COLLFILE,
	      "     %c%c%07ld %c %5d    %10d  %2d  0     %12.5f  %12.5f  %12.5f     %12.5f  %12.5f\n",
	      InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo,
	      InputNeutrons[i].Debug,       InputNeutrons[i].Color, i, ((int) SpinVector[quant_dir]),
	      posex[0], posex[1], posex[2],
	      180./M_PI * atan2(direx[1],direx[0]), 180./M_PI * atan2(direx[2], direx[0]));
#ifdef VT_GRAPH
    else if (Prob > wei_min1)
      drawIt(posex);
#endif
  }
	
  /* transmit coordinates which were not changed, overwrite the rest below */
  { Neutron neutron = InputNeutrons[i];

    neutron.Time = TOF + Path0 / V_FROM_LAMBDA(WL);
    neutron.Probability = Prob;

    CopyVector(Pos, neutron.Position);
    CopyVector(Dir, neutron.Vector);

    neutron.Vector[0] = sqrt(1 - sq(neutron.Vector[1]) - sq(neutron.Vector[2]));

    CopyVector(SpinVector, neutron.Spin);

    if (p) number_vis_tr++;

    WriteNeutronParallel(&neutron, thread_i);
  }
  if (p==1) fprintf(COLLFILE, "\n");

 my_exit:;
}


int main(int argc, char **argv)
{

  int m;

  Init(argc, argv, VT_SM_ENSEMBLE);
  OwnInit(argc, argv);

  if (p==1) {
    COLLFILE = fopen(COLLFILEName, "w");
    fprintf(COLLFILE,
	    "     ID      debug color          no  sp wall         x/cm          y/cm          z/cm           dir y/°       dir z/°\n\n");
  }

  // no helper threads when plotting or writing to file per neutron
  if (p)
    NThreads = 0;
  
  for (m=1; m<=max_mirr; m++) {
    
    SubVector(r1[m], WallOffsetShift[m]); 
    SubVector(r2[m], WallOffsetShift[m]); 
    SubVector(r3[m], WallOffsetShift[m]); 
    SubVector(r4[m], WallOffsetShift[m]); 
  }

  processPipedNeutrons(NThreads, processNeutron, 1, mcperneutron);

   for (m=1; m<=max_mirr; m++) {
     AddVector(r1[m], WallOffsetShift[m]); 
     AddVector(r2[m], WallOffsetShift[m]); 
     AddVector(r3[m], WallOffsetShift[m]); 
     AddVector(r4[m], WallOffsetShift[m]); 
   }

  if (p==1) fclose(COLLFILE);	
	
  OwnCleanup();
  Cleanup(TranslOutput[0], TranslOutput[1], TranslOutput[2], OutputAngleHoriz,OutputAngleVert);

  // dmf test
  exit(0);

  return 0;
}
