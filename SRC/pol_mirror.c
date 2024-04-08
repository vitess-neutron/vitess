/********************************************************************************************/
/*  VITESS module 'pol_mirror.c'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Nov 2004  K. Lieutenant  initial version                                            */
/* 1.1  Apr 2020  K. Lieutenant  new central visualization parameters                       */
/* 1.2  Mar 2023  K. Lieutenant  visualization                                              */
/* 1.2a Nov 2023  K. Lieutenant  visualization corrected                                    */
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


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);       // Reads input parameters and sets global parameters
void  OwnCleanup();                          // Does module specific cleanup
void  SetGeometry(char* sColor);             // Fills the structure stGeometry for visualization 



/******************************/
/** Global Variables         **/
/******************************/
FILE       *pReflUpFile=NULL,         //        pointer to the file containing reflectivity data for spin up neutrons
           *pReflDownFile=NULL;       //        pointer to the file containing reflectivity data for spin down neutrons
char       *ReflUpFileName=NULL,      // -U     name of the file containing reflectivity data for spin up neutrons
           *ReflDownFileName=NULL;    // -D     name of the file containing reflectivity data for spin down neutrons
int        bVertPlane=FALSE;          // -O     flag: 0: mirror is rotated about y-axis (vert. incl.)   1:z-axis (hor. decl.)
int        bTransm=TRUE,              // -T     flag: mode: 1: transmitted beam treated   0: reflected beam treated
           nD=0;                      //        co-ordinate to which mirror extends (1 for horizontal mirror, 2 for vertical mirror)           
double     /* RotMatrixField[3][3], LarmorMatrix[3][3] */
           RotMatrixSM[3][3],         //           rotation matrix to transfer to mirror co-ordinate system
           RotMatrixOut[3][3],        //           rotation matrix to transfer to output system
           RotMatrixAnalysis[3][3];   //           rotation matrix to transfer to quantization direction
double     Size=0.0,                  // -W        width or height of the mirror
           AngleSMHor =0.0,           // -V        rotation angle of the mirror
           AngleSMVert=0.0,           //        
           AnglOutHor =0.0,           // -h        hor. rotation angle of the output frame
           AnglOutVert=0.0,           // -v        vert. rotation angle of the output frame
           dDistCntr=0;               //           distance to center of mirror */
double     *aReflUp=NULL,             //           Table of reflectivity data for spin up neutrons   */
           *aReflDn=NULL;             //           Table of reflectivity data for spin down neutrons */
long       nDataMax=0;                //           number of values in reflectivity tables */
VectorType analysis_dir={0,0,0},      // -a -b -c  quantization direction
           vMirrNormal ={0,0,0},      //           normal to mirror in co-ordinate system of mirror */
           PosSM   = {0,0,0},         // -X -Y -Z  centre position of the mirror            
           DimSM   = {0,0,0},         // -L        size of the mirror in x, y and z direction            
           TranslOut={0,0,0};         // -x -y -z  centre position of the mirror


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  short      bIS=FALSE;                 // Boolean: mirror plane hit or not 
  long       i=0, datanumber=0;
  double     TOFip=0.0, TOFprec=0.0,    // time-of-flight until intersection point and from there to output position
             aUU=1.0, aDD=1.0;          // square root of up- and down-reflectivity resp. to calculate reflectivity for the given spin orientation
  double     dIncl=0.0,                 // inclination angle on mirror 
             phi=0.0, the=0.0,          // spin orientation in spherical co-ordinates
             ProbRefl=0.0;              // probability of reflection  
  VectorType Pos={0,0,0}, Dir={0,0,0},  // position, flight direction and
             Spin={0,0,0},              //   spin of neutron under consideration 
             Path={0,0,0},              // displacement vector
             vItsPnt={0,0,0};           // intersection point of trajectory with mirror
  Neutron    OutNeutron,                // outgoing neutron
             ScatNeut;                  // neutron parameters for trajectory visualization

  // initialisation
  // --------------
  _eModule=MCN_MIRROR_POL;

	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.2a");
	OwnInit(argc, argv);

  bVisInstalled = TRUE;    // needs to be done still
  if (bVisInstr) 
    bBlowUp = TRUE;

  InitNeutron(&OutNeutron);
  InitNeutron(&ScatNeut);
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

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        CopyNeutron(&InputNeutrons[i], &OutNeutron);
        CopyVector(OutNeutron.Position, Pos);
        CopyVector(OutNeutron.Vector, Dir);
        CopyVector(OutNeutron.Spin, Spin);

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
          datanumber = (int) floor(dIncl * 180./M_PI * 1000./OutNeutron.Wavelength + 0.5); 
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
          TOFip = (vItsPnt[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(OutNeutron.Wavelength) ;
          CopyVector(vItsPnt, Pos);

          /* point of interaction for trajectory visualization */
          if (bVisTraj==TRUE)
          { CopyNeutron(&InputNeutrons[i], &ScatNeut);
            CopyVector (vItsPnt, ScatNeut.Position);
            ScatNeut.Time += TOFip;
          }

          if (bTransm)
          {	
            if (aUU == 1.0 && aDD == 1.0)
            { 
              WriteScatIAP(&ScatNeut, VT_ABSORBED, RotMatrixSM, PosSM);
              goto getlost;
            }
            else	
            { 
              the  =  2.0 * atan2((1.0-aDD)*tan(the/2.0), (1.0-aUU));
              OutNeutron.Probability *= (1.0 - ProbRefl);
              ScatNeut.Probability   *= (1.0 - ProbRefl);
              WriteScatIAP(&ScatNeut, VT_TRANSMITTED, RotMatrixSM, PosSM);
            }
          }
          else
          {
            if (aUU == 0.0 && aDD == 0.0)
            {	
              WriteScatIAP(&ScatNeut, VT_ABSORBED, RotMatrixSM, PosSM);
              goto getlost;
            }
            else	
            {	
              Dir[3-nD] *= -1.0;			
              the  =  2.0 * atan2(aDD*tan(the/2.0), aUU);
              OutNeutron.Probability *= ProbRefl;
              ScatNeut.Probability   *= (1.0 - ProbRefl);
              WriteScatIAP(&ScatNeut, VT_REFLECTED, RotMatrixSM, PosSM);
            }
          }
        } 
        else 
        {	
          goto getlost;                      
        }

        if (OutNeutron.Probability <= wei_min) goto getlost ;

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
        TOFprec = - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(OutNeutron.Wavelength) ;
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
        OutNeutron.Time += (TOFip+TOFprec);
        CopyVector(Pos,  OutNeutron.Position) ;
        CopyVector(Dir,  OutNeutron.Vector) ;
        CopyVector(Spin, OutNeutron.Spin) ;

        /* writes output binary file */
        WriteNeutron(&OutNeutron) ;

        /* point of exit for trajectory visualization */
        WriteScatIAP(&OutNeutron, VT_EXITED, RotMatrixOut, TranslOut);

      getlost:;
      }
    }
  }
   
// Finish: print parameters, write geometry and instrument file, free memory
// -------------------------------------------------------------------------
my_exit:
  /* write geometry file */
  SetGeometry("orange");

  /* Do module specific cleanup */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHor, AnglOutVert);	

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  double roty =0.0, rotz=0.0,
         angle=0.0;
  long   count=0, nLinesUp=0, nLinesDn=0;
  char   sBuffer[CHAR_BUF_LENGTH]="";

  // guide_field[0] = guide_field[1] = guide_field[2] = 0.0;

  InitRotMatrix(RotMatrixSM);
  InitRotMatrix(RotMatrixOut);
  InitRotMatrix(RotMatrixAnalysis);
	
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
        pReflDownFile = OpenInputFile2(ReflDownFileName, "reflectivity data for spin-down neutrons", "r");;
        break;

      case 'L':
        sscanf(&argv[1][2], "%lf", &DimSM[0]);
        break;
      case 'W':
        sscanf(&argv[1][2], "%lf", &Size);
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
        sscanf(&argv[1][2], "%lf", &AnglOutHor);
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
    AngleSMHor = angle;
    AngleSMVert  = 0.0;
  }
  else
  {	
    AngleSMHor = 0.0;
    AngleSMVert  = angle;
  }
  DimSM[ nD ] = Size;
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
  fprintf(LogFilePtr, " orientation horiz. :  %10.4f deg, vertical:%10.4f deg\n", AngleSMHor, AngleSMVert) ;
  fprintf(LogFilePtr, " length width height:  %10.4f,%10.4f,%10.4f  cm\n",  DimSM[0], DimSM[1], DimSM[2]);
  // fprintf(LogFilePtr, "guide_field         : (%10.4f,%10.4f,%10.4f) Oe\n",  guide_field[0],  guide_field[1], guide_field[2]);
  fprintf(LogFilePtr, "analysis_dir        : (%10.4f,%10.4f,%10.4f)\n",     analysis_dir[0], analysis_dir[1], analysis_dir[2]);
  fprintf(LogFilePtr, "output frame:\n");
  fprintf(LogFilePtr, " translation        : (%10.4f,%10.4f,%10.4f) cm\n",  TranslOut[0], TranslOut[1], TranslOut[2]); 
  fprintf(LogFilePtr, " rotation horizontal:  %10.4f deg, vertical:%10.4f deg\n", AnglOutHor, AnglOutVert) ;

  /* converts degs in radian etc. */
  AngleSMHor *= M_PI/180. ;
  AngleSMVert  *= M_PI/180. ;
  AnglOutHor *= M_PI/180. ;
  AnglOutVert  *= M_PI/180. ;

  FillRotMatrixZY(RotMatrixSM,  AngleSMVert, AngleSMHor) ;
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHor) ;

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

  /* free allocated memory */
  if (aReflUp!=NULL) free(aReflUp);
  if (aReflDn!=NULL) free(aReflDn);
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  /* Geometry data */
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.nHulls = 1;
    stGeometry.pHull = calloc(stGeometry.nHulls, sizeof(VtHull));

    if (nD==1)  // nearly horizontal mirror causing vertical inclination
    { 
      stGeometry.pHull[0].vNormal[0] = cos(AngleSMVert);
      stGeometry.pHull[0].vNormal[1] = 0.0;
      stGeometry.pHull[0].vNormal[2] = sin(AngleSMVert);
      stGeometry.pHull[0].Length     = DimSM[0];
      stGeometry.pHull[0].WidthIn    = Size*BlowUp;
      stGeometry.pHull[0].HeightIn   = 0.1;
    }
    else
    { 
      stGeometry.pHull[0].vNormal[0] = cos(AngleSMHor);
      stGeometry.pHull[0].vNormal[1] = sin(AngleSMHor);
      stGeometry.pHull[0].vNormal[2] = 0.0;
      stGeometry.pHull[0].Length     = DimSM[0];
      stGeometry.pHull[0].WidthIn    = 0.1;
      stGeometry.pHull[0].HeightIn   = Size*BlowUp;
    }
    stGeometry.pHull[0].WidthOut  = stGeometry.pHull[0].WidthIn;
    stGeometry.pHull[0].HeightOut = stGeometry.pHull[0].HeightIn;
    stGeometry.pHull[0].rotAngle  = 0.0;
    stGeometry.pHull[0].vCntr[0]  = PosSM[0];
    stGeometry.pHull[0].vCntr[1]  = PosSM[1];
    stGeometry.pHull[0].vCntr[2]  = PosSM[2];
  }
}


