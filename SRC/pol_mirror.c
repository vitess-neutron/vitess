/********************************************************************************************/
/*  VITESS module 'pol_mirror.c'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Nov 2004  K. Lieutenant  initial version                                             */
/* 1.1 Apr 2020  K. Lieutenant  new central visualization parameters                        */
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


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit(int argc, char *argv[]) ;
void   OwnCleanup() ;


/******************************/
/** Global Variables         **/
/******************************/
McCompID   _eModule=MCN_MIRROR_POL;

FILE       *pReflUpFile,              //        pointer to the file containing reflectivity data for spin up neutrons
           *pReflDownFile;            //        pointer to the file containing reflectivity data for spin down neutrons
char       *ReflUpFileName,           // -U     name of the file containing reflectivity data for spin up neutrons
           *ReflDownFileName;         // -D     name of the file containing reflectivity data for spin down neutrons
int        bTransm=TRUE,              // -T     flag: mode: 1: transmitted beam treated   0: reflected beam treated
           nD;                        //        co-ordinate to which mirror extends (1 for horizontal mirror, 2 for vertical mirror)           
double     /* RotMatrixField[3][3], LarmorMatrix[3][3] */
           RotMatrixSM[3][3],         //           rotation matrix to transfer to mirror co-ordinate system
           RotMatrixOut[3][3],        //           rotation matrix to transfer to output system
           RotMatrixAnalysis[3][3];   //           rotation matrix to transfer to quantization direction
double     AngleSMHoriz, AngleSMVert, // -V  & -O  rotation angles of the mirror
           AnglOutHoriz, AnglOutVert, // -h -v     hor. and vert. rotation angle of the output frame
           dDistCntr;                 //           distance to center of mirror */
double     *aReflUp,                  //           Table of reflectivity data for spin up neutrons   */
           *aReflDn;                  //           Table of reflectivity data for spin down neutrons */
long       nDataMax;                  //           number of values in reflectivity tables */
VectorType analysis_dir,              // -a -b -c  quantization direction
           vMirrNormal = {0,0,0},     //           normal to mirror in co-ordinate system of mirror */
           PosSM = {0,0,0},           // -X -Y -Z  centre position of the mirror            
           DimSM = {0,0,0},           // -L -W     size of the mirror            
           TranslOut ;                // -x -y -z  centre position of the mirror


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  short      bIS=FALSE;             // Boolean: mirror plane hit or not 
  long       i, datanumber;
  double     TOFip, TOFprec,        // time-of-flight until intersection point and from there to output position
             aUU=1.0, aDD=1.0;      // square root of up- and down-reflectivity resp. to calculate reflectivity for the given spin orientation
  double     dIncl,                 // inclination angle on mirror 
             phi, the,              // spin orientation in spherical co-ordinates
             ProbRefl;              // probability of reflection  
  VectorType Pos, Dir, Spin,        // position, flight direction and spin of neutron under investig. 
             Path,                  // displacement vector
             vItsPnt;               // intersection point of trajectory with mirror
  Neutron    Neutrons ;

  // initialisation
  // --------------
	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.1");
	OwnInit(argc, argv);

  bVisInstalled = MISSING;    // needs to be done still
  if (bVisInstr) 
    bLengthCmpr = TRUE;

  /* transfers guide_field into frame in which guide field is along x-axis */
  // RotVector(RotMatrixField, guide_field) ; 

	DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  /* gets the neutrons from file */
  while((ReadNeutrons())!= 0)
  {
    for(i=0;i<NumNeutGot ;i++)
    { 
			CHECK

      Neutrons = InputNeutrons[i]; 
      CopyVector(Neutrons.Position, Pos) ;
      CopyVector(Neutrons.Vector, Dir) ;
      CopyVector(Neutrons.Spin, Spin) ; 

      /* translates into frame of the mirror */
      SubVector(Pos, PosSM) ;
      RotVector(RotMatrixSM, Pos) ;
      RotVector(RotMatrixSM, Dir) ;
      // RotVector(RotMatrixSM, Spin) ;

      /* transfers spin into frame in which analysis direction is along x-axis */
      RotVector(RotMatrixAnalysis, Spin);
      CartesianToSpherical(Spin, &the, &phi);

      /* calculate intersection point with mirror plane, 
      check if mirror is hit and transfer neutron to intersection point */
      bIS = (short) PlaneLineIntersect(Pos, Dir, vMirrNormal, 0.0, vItsPnt);
      if (bIS==TRUE && (fabs(vItsPnt[nD]) < DimSM[nD]/2.) 
                    && (fabs(vItsPnt[ 0]) < DimSM[ 0]/2.) && (Dir[0] > 0.))
      {
        /* calculate inclination angle, reflectivity for spin up and down neutrons */
        dIncl      = fabs(asin(Dir[3-nD])); 
        datanumber = (int) floor(dIncl * 180./M_PI * 1000./Neutrons.Wavelength + 0.5); 
        if (datanumber > nDataMax) 
        {	
          aUU      = 0.0;
          aDD      = 0.0;
          ProbRefl = 0.0; 
        }
        else
        {	
          aUU = sqrt(aReflUp[datanumber]) ;
          aDD = sqrt(aReflDn[datanumber]) ;
          ProbRefl = sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.)) ;
        }

        /* TOF until intersection point with mirror,  
        new position, direction, spin orientation, change in count rate */
        TOFip = (vItsPnt[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(Neutrons.Wavelength) ;
        CopyVector(vItsPnt, Pos);
        if (bTransm)
        {	
          if (aUU == 1.0 && aDD == 1.0)
          { 
            goto getlost;
          }
          else	
          { 
            Neutrons.Probability *= (1.0 - ProbRefl);
            the  =  2.0 * atan2((1.0-aDD)*tan(the/2.0), (1.0-aUU));
          }
        }
        else
        {
          if (aUU == 0.0 && aDD == 0.0)
          {	
            goto getlost;
          }
          else	
          {	
            Neutrons.Probability *= ProbRefl;
            Dir[3-nD] *= -1.0;			
            the  =  2.0 * atan2(aDD*tan(the/2.0), aUU);
          }
        }
      } 
      else 
      {	
        goto getlost;                      
      }

      if (Neutrons.Probability <= wei_min) goto getlost ;

      /* translates back to cartesian representation of spin */
      SphericalToCartesian(Spin, &the, &phi);
      RotBackVector(RotMatrixAnalysis, Spin);

      /* translates into initial frame   */
      RotBackVector(RotMatrixSM, Pos) ;
      RotBackVector(RotMatrixSM, Dir) ;
      RotBackVector(RotMatrixSM, Spin) ;
      AddVector(Pos, PosSM) ;

      /* computes neutron variables in the output frame */
      SubVector(Pos, TranslOut) ;
      RotVector(RotMatrixOut, Pos) ;
      RotVector(RotMatrixOut, Dir) ;
      RotVector(RotMatrixOut, Spin) ;

      /* translates neutrons to output plane (x'=0) */
      TOFprec = - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(Neutrons.Wavelength) ;
      CopyVector(Dir, Path) ;
      MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;
      AddVector(Pos, Path) ;  

      /* precession in the guide field */
      /* transfers into frame in which guide field is along x-axis and calculates phase shift */
      // RotVector(RotMatrixField, Spin) ; 
      // PhaseShift = TOFprec * FREQUENCY_FROM_FIELD(guide_field[0]) ;  
      // NumberPrecessions = PhaseShift/2./M_PI ;
      /* rotates about this x-axis */
      // FillRotMatrixYX(LarmorMatrix, PhaseShift, 0) ;
      // RotVector      (LarmorMatrix, Spin) ;
      /* transfers back to output frame */
      // RotBackVector  (RotMatrixField, Spin) ;

      /* transmit coordinates which were not changed, the rest overwrite below */
      Neutrons.Time += (TOFip+TOFprec);
      CopyVector(Pos,  Neutrons.Position) ;
      CopyVector(Dir,  Neutrons.Vector) ;
      CopyVector(Spin, Neutrons.Spin) ;

      /* writes output binary file */
      WriteNeutron(&Neutrons) ;

   getlost:;
    }
  }
   
// Finish: print parameters, write geometry and instrument file, free memory
// -------------------------------------------------------------------------
my_exit:
	/* Do module specific cleanup */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);	

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  double roty, rotz,
  size=0.0, angle=0.0;
  long   count, nLinesUp=0, nLinesDn=0;
  int    bVertPlane=FALSE;
  char   sBuffer[CHAR_BUF_LENGTH];

  // guide_field[0] = guide_field[1] = guide_field[2] = 0.0;
	
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'T':
        sscanf(&argv[1][2], "%d", &bTransm) ;  /* 0: reflection   */
        break;                                 /* 1: transmission */
		
      case 'O':
        sscanf(&argv[1][2], "%d", &bVertPlane); /* 0: horizontal mirror */
        if (bVertPlane)                         /* 1: vertical mirror   */
          nD=2;
        else
          nD=1;
				
        vMirrNormal[3-nD]=1.0;
        break;                                
		
      case 'U':
        ReflUpFileName=&argv[1][2];
        pReflUpFile = OpenInputFile2(ReflUpFileName, "reflectivity data for spin-up neutrons", "r");
        break;
      case 'D':
        ReflDownFileName=&argv[1][2];
        pReflDownFile = OpenInputFile2(ReflUpFileName, "reflectivity data for spin-down neutrons", "r");;
        break;

      case 'L':
        sscanf(&argv[1][2], "%lf", &DimSM[0]);
        break;
      case 'W':
        sscanf(&argv[1][2], "%lf", &size);
        break;

      case 'X':
        sscanf(&argv[1][2], "%lf", &PosSM[0]);
        dDistCntr = PosSM[0];
        break;
      case 'Y':
        sscanf(&argv[1][2], "%lf", &PosSM[1]);
        break;
      case 'Z':
        sscanf(&argv[1][2], "%lf", &PosSM[2]);
        break;

      case 'V':
        sscanf(&argv[1][2], "%lf", &angle);
        break;

      case 'a':
        sscanf(&argv[1][2], "%lf", &analysis_dir[0]);
        break;
      case 'b':
        sscanf(&argv[1][2], "%lf", &analysis_dir[1]);
        break;
      case 'c':
        sscanf(&argv[1][2], "%lf", &analysis_dir[2]);
        break;

      case 'x':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]);
        break;
      case 'y':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]);
        break;
      case 'z':
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

  if (bVertPlane)
  {	
    AngleSMHoriz = angle;
    AngleSMVert  = 0.0;
  }
  else
  {	
    AngleSMHoriz = 0.0;
    AngleSMVert  = angle;
  }
  DimSM[ nD ] = size;
  DimSM[3-nD] = 0.0;
		
  /* Allocate memory for reflectivity files and initialize with 0.0 */
  if (pReflUpFile!=NULL)
    nLinesUp = LinesInFile(pReflUpFile);
  else
    Error("Reflectivity file for spin-up neutrons not read");

  if (pReflUpFile!=NULL)
    nLinesDn = LinesInFile(pReflDownFile);
  else
    Error("Reflectivity file for spin-down neutrons not read");

  nDataMax = maxi(10*nLinesUp, 10*nLinesDn);

  aReflUp = calloc(nDataMax, sizeof(double));
  aReflDn = calloc(nDataMax, sizeof(double));
  for (count=0; count < nDataMax; count++)
  {
    aReflUp[count] = 0.0;
    aReflDn[count] = 0.0;
  }

  /* read reflectivity files for spin up and down neutrons     */
  /* Read reflectivity data; data are encoded as reflectivities 
  corresponding to 0.000,0.001, 0.002, ... deg,  reference wavelength 1 A */
  /* spin up */
  if (pReflUpFile != NULL)
  {	for(count=0; count < nLinesUp; count++)
    {	ReadLine  (pReflUpFile, sBuffer, sizeof(sBuffer)-1);
      StrgScanLF(sBuffer, &aReflUp[10*count], nDataMax-10*count, 0);
    }
    fclose(pReflUpFile);
  }
  /* spin down */
  if (pReflDownFile != NULL)
  {	for(count=0; count < nLinesDn; count++)
    {	ReadLine  (pReflDownFile, sBuffer, sizeof(sBuffer)-1);
      StrgScanLF(sBuffer, &aReflDn[10*count], nDataMax-10*count, 0);
    }
    fclose(pReflDownFile);
  }

  /* prints parameters into log file for verification */
  if (bTransm==TRUE)
    fprintf(LogFilePtr, "mode  : Transmission\n");
  else
    fprintf(LogFilePtr, "mode  : Reflection\n");

  fprintf(LogFilePtr, "mirror: \n");
  fprintf(LogFilePtr, " position           : (%10.4f,%10.4f,%10.4f) cm\n",  PosSM[0], PosSM[1], PosSM[2]);
  fprintf(LogFilePtr, " orientation horiz. :  %10.4f deg, vertical:%10.4f deg\n", AngleSMHoriz, AngleSMVert) ;
  fprintf(LogFilePtr, " length width height:  %10.4f,%10.4f,%10.4f  cm\n",  DimSM[0], DimSM[1], DimSM[2]);
  // fprintf(LogFilePtr, "guide_field         : (%10.4f,%10.4f,%10.4f) Oe\n",  guide_field[0],  guide_field[1], guide_field[2]);
  fprintf(LogFilePtr, "analysis_dir        : (%10.4f,%10.4f,%10.4f)\n",     analysis_dir[0], analysis_dir[1], analysis_dir[2]);
  fprintf(LogFilePtr, "output frame:\n");
  fprintf(LogFilePtr, " translation        : (%10.4f,%10.4f,%10.4f) cm\n",  TranslOut[0], TranslOut[1], TranslOut[2]); 
  fprintf(LogFilePtr, " rotation horizontal:  %10.4f deg, vertical:%10.4f deg\n", AnglOutHoriz, AnglOutVert) ;

  /* converts degs in radian etc. */
  AngleSMHoriz *= M_PI/180. ;
  AngleSMVert  *= M_PI/180. ;
  AnglOutHoriz *= M_PI/180. ;
  AnglOutVert  *= M_PI/180. ;

  FillRotMatrixZY(RotMatrixSM,  AngleSMVert, AngleSMHoriz) ;
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz) ;

  CartesianToEulerZY(analysis_dir, &roty, &rotz); 
  FillRotMatrixZY(RotMatrixAnalysis, roty, rotz); 

  // CartesianToEulerZY(guide_field, &roty, &rotz); 
  // FillRotMatrixZY(RotMatrixField, roty, rotz); 

}/* End OwnInit */


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnCleanup()
{
  fprintf(LogFilePtr," \n") ;

  /* set description for instrument plot */
  stPicture.dWPar = DimSM[1];
  stPicture.dHPar = DimSM[2];

  /* free allocated memory */
  if (aReflUp!=NULL) free(aReflUp);
  if (aReflDn!=NULL) free(aReflDn);
}

