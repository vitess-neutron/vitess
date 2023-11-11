/*********************************************************************************************/
/*  VITESS module 'sesansfield.c'                                                            */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.9  Jul 2008  K. Lieutenant  preliminary version, developed from 'precessionfield.c'     */
/* 1.0  May 2020  K. Lieutenant  new central visualization parameters                        */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"

#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */
#define MAX_WALLS    6
#define MAX_EDGES    8 
#define NMAX         3


/******************************/
/** Prototypes               **/
/******************************/
void	OwnInit       (int argc, char *argv[]);                                      // Reads input parameters and sets global variables
void	OwnCleanup    () ;                                                           // Does module specific cleanup
short ReadFieldArea (const char* sFileName);                                       // Reads the positions of the corners of the field
void  SetGeometry   (char* sColor);                                                // Fills the structure stGeometry for visualization
short DetermineWalls();                                                            // Determines planes for all walls by calling 'DeterminePlane'
short DeterminePlane(Plane* wall, VectorType vCorner1, VectorType vCorner2,     
                                  VectorType vCorner3, VectorType vCorner4);       // Determines plane from the corner positions
short IsPointInside (VectorType p, short iC1, short iC2, short iC3, short iC4);    // Checks if position is inside the field area given by the corners
void  Invert        (double Ni[NMAX+1][NMAX+1], double Nm[NMAX+1][NMAX+1]);        // Inversion of a matrix (NM to NI)
void  DefineTriangle(VtTriangle* ta, VectorType v1, VectorType v2, VectorType v3); // Creates triangle for visualization from 3 points



/******************************/
/** Global Variables         **/
/******************************/
// input parameters
char*       FieldFileName=NULL;         // -P        [-]   name of the file containing the corners of the magnetic field range
double      field_hom[3]={0.0,0.0,50.0};// -F -G -H  [Oe]  x-, y- and z-component of the magnetic field
VectorType  TranslOut,                  // -q -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)
            vCorner[MAX_EDGES];         // file      [cm]  position of the corners (limiting the magnetic field area)

// Variables determined from input parameters or trajectory data
short       nCorners=0,                 //                 number of points limiting the magnetic field area
            nWalls=0;                   //                 number of walls limiting the magnetic field area
Plane       vWall[MAX_WALLS],           //                 structure describing the walls (limiting the magnetic field area)
            vExit;                      //                 structure describing a plane through the new origin
double      RotMatrixField[3][3];       //                 Matrix to rotate a neutron into the quantization direction of the magnetic field
VectorType  domain_field;               //                 magnetic field [Ørsted] in Euler co-ordinates
                                                           
// constants                                               
short      mOut=0,                      //                 parameter to control output of inverted matrix
           nX  =3;                      //                 dimension of matrix to invert
short      iCorner[MAX_WALLS][4]        //                 indices of the corners defining the walls
           ={{0,1,2,3},{4,5,1,0},{5,6,2,1},{6,7,3,2},{7,4,0,3},{4,5,6,7}};  


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  short      k, kHit;
  long       i, NumOut=0;
  double     LarmorMatrix[3][3],
             PhaseShift, Precessions, TotNumPrec=0.0,
             TOF, TOF1, TOF2=1.0E99, TOF3,     
             lambda;
  Neutron    InNeutron, Out1Neutron, Out2Neutron, TestNeutron;

  // initialisation
  // --------------
  _eModule = MCN_FIELD_SESANS;
     
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.0");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons() != 0)
  {
    for (i=0; i<NumNeutGot; i++)
    { 
      CHECK;	

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

        InNeutron   = InputNeutrons[i]; 
        Out1Neutron = InputNeutrons[i]; 
        Out2Neutron = InputNeutrons[i]; 

        lambda = InNeutron.Wavelength ;

        /* translatation into the frame of the main field and rotates coordinates 
        SubVector(InNeutron.Position, PosMain);
        RotVector(RotMatrixMain, InNeutron.Position); 
        RotVector(RotMatrixMain, InNeutron.Vector); 
        RotVector(RotMatrixMain, InNeutron.Spin);  */

        /* propagation to intersection with magnetic field walls */
        TOF1 = 1.0E99;
        kHit = 0; 
        for (k=0; k < nWalls; k++)
        {	
          TestNeutron = InNeutron;

          if (keygrav == 1)
          TOF = NeutronPlaneIntersectionGrav(&TestNeutron, vWall[k]);
          else
          TOF = NeutronPlaneIntersection1   (&TestNeutron, vWall[k]);

          if (TOF > 0.0 && TOF < TOF1 && IsPointInside(TestNeutron.Position, iCorner[k][0], iCorner[k][1], iCorner[k][2], iCorner[k][3]))
          {	
            kHit++;
            if (kHit==2) 
            {	// if H-field range is hit twice with t2 < t1, then the first hit must have been be the exit
              TOF2 = TOF1-TOF;
              Out2Neutron = Out1Neutron;
              TOF1 = TOF;
              Out1Neutron = TestNeutron;
              k=nWalls;                // there are only 2 intersection points possible
            }
            else
            {
              TOF1 = TOF;
              Out1Neutron = TestNeutron;
            }
          }
        }
        if (kHit == 0) goto getlost;
			
        Out1Neutron.Time += TOF1;

        /* propagation through the magnetic field */
        if (kHit == 2) 
        {
          Out2Neutron.Time += TOF1;
        }
        else
        {	
          InNeutron = Out1Neutron; 
          TOF2 = 1.0E99;
          for (k=0; k < nWalls; k++)
          {	
            TestNeutron = InNeutron; 

            if (keygrav == 1)
            TOF = NeutronPlaneIntersectionGrav(&TestNeutron, vWall[k]);
            else
            TOF = NeutronPlaneIntersection1   (&TestNeutron, vWall[k]);

            if (TOF > 0.0 && TOF < TOF2 && IsPointInside(TestNeutron.Position, iCorner[k][0], iCorner[k][1], iCorner[k][2], iCorner[k][3]))
            {	
              TOF2 = TOF;
              Out2Neutron = TestNeutron;
              k=nWalls;                    // there is only one exit possible
            }
          }
          if (TOF2 > 1.0E98) goto getlost;
        }

        Out2Neutron.Time += TOF2;

        /* precession in the magnetic field */
        PhaseShift  = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]);
        Precessions = PhaseShift/2./M_PI; 
        FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;

        /* precession carried out in the field frame */
        RotVector    (RotMatrixField, Out2Neutron.Spin); 
        RotVector    (LarmorMatrix,   Out2Neutron.Spin);
        RotBackVector(RotMatrixField, Out2Neutron.Spin);

        /* translatation back into the original frame 
        AddVector(Out2Neutron.Position, PosMain) ;
        RotBackVector(RotMatrixMain, Out2Neutron.Position); 
        RotBackVector(RotMatrixMain, Out2Neutron.Vector);  
        RotBackVector(RotMatrixMain, Out2Neutron.Spin);    */

        /* propagation to a plane through the new origin */ 
        if (keygrav == 1)
          TOF3 = NeutronPlaneIntersectionGrav(&Out2Neutron, vExit);
        else
          TOF3 = NeutronPlaneIntersection1   (&Out2Neutron, vExit);

        /* computation of co-ordinates in the output frame (x'=0) */
        Out2Neutron.Time += TOF3;
        SubVector(Out2Neutron.Position, TranslOut);

        /* output binary file and statistics */
        WriteNeutron(&Out2Neutron);

        NumOut++;
        TotNumPrec += Precessions;

      getlost: ;
      }
    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
  my_exit:
  /* write to log file */
  if (NumOut != 0)
    fprintf(LogFilePtr,"Average number of precessions  : %9.2lf\n", TotNumPrec/NumOut);

  /* write geometry file */
  SetGeometry("yellow");
  
  /* Do module specific cleanups */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup   (TranslOut[0], TranslOut[1], TranslOut[2], 0.0,0.0);	
  fprintf(LogFilePtr," \n");

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int l=0;

  /* Init */
  InitVector(TranslOut);
  InitVector(domain_field);
  for (l=0; l < MAX_EDGES; l++)
    InitVector(vCorner[l]);

  InitPlane(&vExit);
  for (l=0; l < MAX_WALLS; l++)
    InitPlane(&vWall[l]);

  Init3x3Matrix(RotMatrixField);

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        FieldFileName = &argv[1][2];
        break;
			
    /*  case 'i':
        sscanf(&argv[1][2], "%lf", &AnglMainHoriz) ;
        break;
      case 'j':
        sscanf(&argv[1][2], "%lf", &AnglMainVert) ;
        break;  */

      case 'q':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
        break;
      case 'r':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
        break;
      case 's':
        sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
        break;

      case 'F':
        sscanf(&argv[1][2], "%lf", &field_hom[0]) ;
        break;
      case 'G':
        sscanf(&argv[1][2], "%lf", &field_hom[1]) ;
        break;
      case 'H':
        sscanf(&argv[1][2], "%lf", &field_hom[2]) ;
        break;
    }
    argc--;
    argv++;
  }

  fprintf(LogFilePtr, "magnetic field : %9.2lf %9.2lf %9.2lf Oe\n", field_hom[0], field_hom[1], field_hom[2]) ;

  /* generate field vectors in Euler representation */
  domain_field[0] = LengthVector(field_hom) ;
  MultiplyByScalar  (field_hom, 1./domain_field[0]) ;
  CartesianToEulerZY(field_hom, &domain_field[2], &domain_field[1]) ;

  /* calculate field matrix and main matrix */
  // AnglMainVert  = AnglMainVert *(M_PI)/180.0;
  // AnglMainHoriz = AnglMainHoriz*(M_PI)/180.0;

  FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 
  // FillRotMatrixZY(RotMatrixMain,  AnglMainVert, AnglMainHoriz) ;

  /* Determine range and edges of magnetic field */
  nCorners = ReadFieldArea (FieldFileName);
  nWalls   = DetermineWalls();

  /* Determine exit plane */
  vExit.A = 1.0;
  vExit.B = 0.0;
  vExit.B = 0.0;
  vExit.D = -TranslOut[0];

  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  return;
}


/*******************************************************/
/** Reads the positions of the corners of the field   **/
/*******************************************************/
short ReadFieldArea(const char* sFileName)
{
  short nCrnr=0;
  char  sLine[256];
  FILE* pFieldFile = OpenInputFile2(sFileName, "precession field corners", "r");

  if (pFieldFile != NULL)
  {
    while (ReadLine(pFieldFile, sLine, sizeof(sLine)-1))
    {	// StrgScanLF(sLine, &(vCorner[nCrnr]), 3, 0);
      sscanf(sLine, "%lf %lf %lf", &vCorner[nCrnr][0], &vCorner[nCrnr][1], &vCorner[nCrnr][2]);
      nCrnr++;
    }
    fprintf(LogFilePtr, "range from file: '%s'\n", sFileName);
    fclose(pFieldFile);
  }

  return nCrnr;
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

    stGeometry.nTriangles = 12;
    stGeometry.pTriangle = calloc(stGeometry.nTriangles, sizeof(VtTriangle));

    DefineTriangle(&stGeometry.pTriangle[ 0], vCorner[0], vCorner[1], vCorner[2]);
    DefineTriangle(&stGeometry.pTriangle[ 1], vCorner[0], vCorner[3], vCorner[2]);
    DefineTriangle(&stGeometry.pTriangle[ 2], vCorner[0], vCorner[1], vCorner[5]);
    DefineTriangle(&stGeometry.pTriangle[ 3], vCorner[0], vCorner[4], vCorner[5]);
    DefineTriangle(&stGeometry.pTriangle[ 4], vCorner[1], vCorner[2], vCorner[6]);
    DefineTriangle(&stGeometry.pTriangle[ 5], vCorner[1], vCorner[5], vCorner[6]);
    DefineTriangle(&stGeometry.pTriangle[ 6], vCorner[2], vCorner[3], vCorner[7]);
    DefineTriangle(&stGeometry.pTriangle[ 7], vCorner[2], vCorner[6], vCorner[7]);
    DefineTriangle(&stGeometry.pTriangle[ 8], vCorner[0], vCorner[3], vCorner[7]);
    DefineTriangle(&stGeometry.pTriangle[ 9], vCorner[0], vCorner[4], vCorner[7]);
    DefineTriangle(&stGeometry.pTriangle[10], vCorner[4], vCorner[5], vCorner[6]);
    DefineTriangle(&stGeometry.pTriangle[11], vCorner[4], vCorner[7], vCorner[6]);
  }
}


/*****************************************************************/
/** Determines planes for all walls by calling 'DeterminePlane' **/
/*****************************************************************/
short DetermineWalls()
{
  short k, nPln=0,
	      bPlane;

  for (k=0; k < MAX_WALLS; k++)	
  {	
    bPlane = DeterminePlane(&vWall[k], vCorner[iCorner[k][0]], vCorner[iCorner[k][1]], vCorner[iCorner[k][2]], vCorner[iCorner[k][3]]);
    if (bPlane)
      nPln++;
    else
      continue;
  }

  return nPln;
}


/*******************************************************/
/** Determines plane from the corner positions        **/
/*******************************************************/
short DeterminePlane(Plane* pWall,  VectorType vCorner1, VectorType vCorner2, VectorType vCorner3, VectorType vCorner4)
{
  short   j, rc=TRUE;
  double  Mat[NMAX+1][NMAX+1], InvMat[NMAX+1][NMAX+1];
  double  d[4]={1.0,1.0,1.0,1.0}, V[4], v[4];

  // fill matrix of linear equation system
  Mat[1][1] = vCorner1[0]; Mat[1][2] = vCorner1[1]; Mat[1][3] = vCorner1[2]; 
  Mat[2][1] = vCorner2[0]; Mat[2][2] = vCorner2[1]; Mat[2][3] = vCorner2[2]; 
  Mat[3][1] = vCorner3[0]; Mat[3][2] = vCorner3[1]; Mat[3][3] = vCorner3[2]; 

  // invert matrix
  Invert(InvMat, Mat);

  // determine solution vector
  for (j=1; j<=3; j++)
  {	
    V[j] = InvMat[j][1]*d[1] + InvMat[j][2]*d[2] + InvMat[j][3]*d[3]; 
  }
 
  // normalize solution vector
  for (j=1; j<=3; j++)
  {	
    v[j] = V[j] / sqrt(sq(V[1])+sq(V[2])+sq(V[3]));
    if (fabs(v[j]) < 1.0E-10) v[j]=0.0; 
  } 

  // calculate plane parameters
  pWall->A =   v[1];		
  pWall->B =   v[2];		
  pWall->C =   v[3];		
  pWall->D = -(v[1]*vCorner1[0] +  v[2]*vCorner1[1] +  v[3]*vCorner1[2]);	

  if (fabs(pWall->A * vCorner4[0] + pWall->B * vCorner4[1] + pWall->C * vCorner4[2] + pWall->D) > 1.0E-8) 
  {
    Warning("Points do not lie in one plane");
    rc = FALSE;
  }
	
  return rc;
}


/***************************************************************************/
/** Checks if position is inside the field area given by the corners      **/
/***************************************************************************/
short IsPointInside(VectorType vP, short iC1, short iC2, short iC3, short iC4)
{
  short  rc=FALSE;
  double Angle,  AngleL, AngleR;
  VectorType vA, vB, vC;

  CopyVector(vCorner[iC4], vA);  SubVector(vA, vCorner[iC1]);
  CopyVector(vCorner[iC2], vB);  SubVector(vB, vCorner[iC1]);
  CopyVector(vP          , vC);  SubVector(vC, vCorner[iC1]);
  Angle  = acos(ScalarProduct(vA, vB) / (LengthVector(vA)*LengthVector(vB)));
  AngleL = acos(ScalarProduct(vA, vC) / (LengthVector(vA)*LengthVector(vC)));
  AngleR = acos(ScalarProduct(vC, vB) / (LengthVector(vC)*LengthVector(vB)));

  if (fabs(AngleL) < fabs(Angle)  &&  fabs(AngleR) < fabs(Angle))       // P-C1 in between C4-C1 and C2-C1
  {
    CopyVector(vCorner[iC1], vA);  SubVector(vA, vCorner[iC2]);
    CopyVector(vCorner[iC3], vB);  SubVector(vB, vCorner[iC2]);
    CopyVector(vP,           vC);  SubVector(vC, vCorner[iC2]);
    Angle  = acos(ScalarProduct(vA, vB) / (LengthVector(vA)*LengthVector(vB)));
    AngleL = acos(ScalarProduct(vA, vC) / (LengthVector(vA)*LengthVector(vC)));
    AngleR = acos(ScalarProduct(vC, vB) / (LengthVector(vC)*LengthVector(vB)));

    if (fabs(AngleL) < fabs(Angle)  &&  fabs(AngleR) < fabs(Angle))     // P-C2 in between C1-C2 and C3-C2
    {
      CopyVector(vCorner[iC2], vA);  SubVector(vA, vCorner[iC3]);
      CopyVector(vCorner[iC4], vB);  SubVector(vB, vCorner[iC3]);
      CopyVector(vP,           vC);  SubVector(vC, vCorner[iC3]);
      Angle  = acos(ScalarProduct(vA, vB) / (LengthVector(vA)*LengthVector(vB)));
      AngleL = acos(ScalarProduct(vA, vC) / (LengthVector(vA)*LengthVector(vC)));
      AngleR = acos(ScalarProduct(vC, vB) / (LengthVector(vC)*LengthVector(vB)));

      if (fabs(AngleL) < fabs(Angle)  &&  fabs(AngleR) < fabs(Angle))  // P-C3 in between C2-C3 and C4-C3
        rc = TRUE;
    }
  }

  return rc;
}


/***********************************************************/
/* Inversion of a matrix (NM to NI)                        */
/*  NI: pointer to inverted matrix                         */
/*  NM: pointer to matrix                                  */
/***********************************************************/
void Invert(double NI[NMAX+1][NMAX+1], double NM[NMAX+1][NMAX+1])
{
  short  i,j,m;
  double PM, D;

  for (i=1; i<=nX; i++)
  {	
    NM[i][i] += 1.0;
  }

  for (m=nX; m>=1; m--)
  {	
    PM=NM[m][m]-1;
    if (PM == 0.0) 
    {	
      fprintf(LogFilePtr,"\nERROR: Matrix diagonal element is 0\n");
      exit(99);
    }
    for (j=1; j<=nX; j++)
    {	
      NM[m][j]=NM[m][j]/PM;
    }
    for (i=1; i<=nX; i++)
    {	
      if (i != m) 
      {	
        D = NM[i][m];
        for (j=1; j<=nX; j++)
        {	
          NM[i][j] = NM[i][j] - D*NM[m][j];
        }
      }
    }
  }

  for (m=1; m<=nX; m++)
  {	
    for (j=1; j<=nX; j++)
    {	
      NI[m][j]=NM[m][j];
    }
    NI[m][m] -= 1.0;
    if (NI[m][m] == 0.0) 
    {	
      fprintf(LogFilePtr, "\nERROR: Normal matrix singular\n");
      return;
    }
  }

  /* Parameter output of this step*/
  if (mOut == 3) 
  {	
    fprintf(LogFilePtr, "\n---------------------------\n");
    for (i=1; i<=nX; i++)
    {	
      for (j=1; j<=nX; j++)
      {	
        fprintf(LogFilePtr, "%12.4f", NI[i][j]);
      }
      fprintf(LogFilePtr, "\n");
    }
    fprintf(LogFilePtr, "\n---------------------------\n");
  }
}


/***************************************************************************/
/** Creates triangle for visualization from 3 points                      **/
/***************************************************************************/
void DefineTriangle(VtTriangle* triangle, VectorType v1, VectorType v2, VectorType v3)
{
  CopyVector(v1, triangle->vEdges[0]);
  CopyVector(v2, triangle->vEdges[1]);
  CopyVector(v3, triangle->vEdges[2]);
}
