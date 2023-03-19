/*********************************************************************************************/
/*  VITESS module 'precessionfield.c'                                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00            G. Zsigmond   initial version                                             */
/* 1.01  Jul 2002  G. Zsigmond   change                                                      */
/* 1.02  Oct 2002  S. Manoshin   change                                                      */
/* 1.03  Jan 2004  K. Lieutenant changes for 'instrument.dat'                                */
/* 1.04  Jul 2004  G. Zsigmond   change for rotation of the field map                        */
/* 1.05  May 2020  K. Lieutenant tidy up, new central visualization parameters               */
/* 1.06  Mar 2023  K. Lieutenant visualization                                               */
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
#include "precessionfield.h"


/******************************/
/** Global Variables         **/
/******************************/
// input parameters
char*       FieldFileName=NULL;         // -P        [-]   name of the file containing the map of the inhomogeneous magnetic field
long        Option=0;                   // -O        [-]   0: homogenenous   1: inhomogeneous   magnetic field
double      depth =0.0,                 // -X        [cm]  x-component of the size of the homogeneous magnetic field 
            width =0.0,                 // -Y        [cm]  y-component of the size of the homogeneous magnetic field 
            height=0.0;                 // -V        [cm]  z-component of the size of the homogeneous magnetic field 
double      field_hom[3]={1.0,0.0,0.0}; // -T -G -H  [Gs]  x-, y- and z-component of the homogeneous magnetic field
double      AnglMainHoriz=0.0,          // -i       [deg]  horizontal (first) rotation angle of the magnetic field map
            AnglMainVert =0.0;          // -j       [deg]  vertical (second) rotation angle of the magnetic field map
VectorType  PosMain,                    // -k -l -m  [cm]  centre of the magnetic field 
            TranslOut;                  // -p -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)
double      domain_field_F[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], // arrays of strengths, positions and sizes 
            PosDomain_F   [3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], // of magnetic field elements
            DimDomain_F   [3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]; // from file

// Variables determined from input parameters or trajectory data
FILE*  FieldMapFile=NULL;                     //     [-]   pointer to magnetic field map file 
long   ind_x=0, ind_y=0, ind_z=0,             //     [-]   indices of magnetic field elements in x-, y- and z-direction
       ind_x_max=2, ind_y_max=2, ind_z_max=2; //     [-]   max. number of magnetic field elements in x-, y- and z-direction
double RotMatrixMain[3][3];                   //    [deg]  Matrix to rotate the magnetic field


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long       NumOut=0, wall_1=0, wall_2=0;
  double     TOF=0.0, TOF1=0.0, TOF2=0.0, TOF3=0.0, 
             WL=0.0, Prob=0.0, PhaseShift=0.0, NumberPrecessions=0.0;
  double     IntegralIntensity=0.0;
  VectorType SpinVector, domain_field;
  VectorType Path, Pos, Dir, Pos1, Pos2, PosDomain, DimDomain;
  Neutron    Neutrons;
  double     RotMatrixField[3][3],  // Matrix to rotate a neutron into the quantization direction of the magnetic field
             LarmorMatrix  [3][3];

  // initialisation
  // --------------
  _eModule=MCN_FIELD_PREC;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.06");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  // local variables
  InitVector(Pos);  InitVector(Dir);       InitVector(SpinVector);
  InitVector(Pos1); InitVector(Pos2);      InitVector(domain_field);
  InitVector(Path); InitVector(PosDomain); InitVector(DimDomain);  

  InitNeutron(&Neutrons); 

  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(LarmorMatrix);

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
  {
    int i=0;

    for (i=0; i<NumNeutGot; i++)
    { 
      CHECK;	

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /*InputNeutrons[i].Position[0]	= 0. ;*/

        TOF = InputNeutrons[i].Time ;
        WL  = InputNeutrons[i].Wavelength ;
        Prob = InputNeutrons[i].Probability ;

        CopyVector(InputNeutrons[i].Position, Pos) ;
        CopyVector(InputNeutrons[i].Vector, Dir) ;
        CopyVector(InputNeutrons[i].Spin, SpinVector) ; 

        InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

        /* translates into frame of the main field and rotates coordinates  */
        SubVector(Pos, PosMain) ;

        RotVector(RotMatrixMain, Pos ) ; 
        RotVector(RotMatrixMain, Dir ) ; 
        RotVector(RotMatrixMain, SpinVector) ; 

        /* enter position and TOF 	*/
        TOF1 = (- depth/2. - Pos[0])/ fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

        CopyVector(Dir, Path) ;
        MultiplyByScalar(Path, (- depth/2. - Pos[0])/ Dir[0] ) ;
        AddVector(Pos, Path) ;  TOF += TOF1 ;

        /* looks for first domain if dimension of domain changes only along X axis */
        DimDomain[1] = DimDomain_F[1][1][1][1];
        DimDomain[2] = DimDomain_F[2][1][1][1];

        ind_y = (long) floor(Pos[1] / DimDomain[1]) + 1 + ind_y_max/2 ;
        if ((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost ;

        ind_z = (long) floor(Pos[2] / DimDomain[2]) + 1 + ind_z_max/2 ;
        if ((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost ;

        ind_x = 1 ; 

        /******************** starts to scan ******************************/
        // NumberPrecessions = 0 ;

        while (ind_x != (ind_x_max +1)) 
        {
          CopyVectorsToVector3(ind_x, ind_y, ind_z, PosDomain_F, PosDomain) ;
          CopyVectorsToVector3(ind_x, ind_y, ind_z, DimDomain_F, DimDomain) ;
          CopyVectorsToVector3(ind_x, ind_y, ind_z, domain_field_F, domain_field) ;

          /* calculate field matrix */
          FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 

          /* translates into frame of the field domain */
          SubVector(Pos, PosDomain) ;

          /* calculate entrance end exit coordinates of domain*/
          { 
            VectorType pos, dir;	CopyVector(Pos, pos) ;	CopyVector(Dir, dir) ;
	
            /* gives intersection positions with domain */
            if(IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) goto getlost ; 

            if(wall_2 == 0) goto getlost ;

            /* ordering */
            if(Pos1[0] > Pos2[0]) 	
            { VectorType V ;	
              int wall; 

              CopyVector(Pos1, V); CopyVector(Pos2, Pos1); CopyVector(V, Pos2) ; 	
              wall = wall_1;       wall_1 = wall_2;        wall_2 = wall ;
            }
          }

          /* moment of arriving at the domain wall, new position */
          CopyVector(Pos1, Pos) ;

          /* time of precession in the domain field - precession calculated in the field frame */
          TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
          RotVector(RotMatrixField, SpinVector) ; 
          PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]) ;  NumberPrecessions += PhaseShift/2./M_PI ;

          FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;

          RotVector(LarmorMatrix, SpinVector) ;
          RotBackVector(RotMatrixField, SpinVector) ;

          /* moment of exiting at the domain wall, new position */
          TOF += TOF2 ;
          CopyVector(Pos2, Pos) ;

          /* translates back into main frame */
          AddVector(Pos, PosDomain) ;

          /* searching new domain */
          if(wall_2 == 1) goto getlost;
          if(wall_2 == 2) {ind_x += 1 ; }
          if(wall_2 == 3) {ind_y += -1 ; }
          if(wall_2 == 4) {ind_y += 1 ; }
          if(wall_2 == 5) {ind_z += -1 ; }
          if(wall_2 == 6) {ind_z += 1 ;}

          /*if(ind_x > ind_x_max) goto exitfield ; */
          if(ind_y == 0) goto exitfield ; 
          if(ind_y > ind_y_max) goto exitfield ; 
          if(ind_z == 0) goto exitfield; 
          if(ind_z > ind_z_max) goto exitfield;

          /*goto newdomain ;*/
        }

    exitfield: ;

        /*******************************************************************************/

        /* Output matters */
        AddVector(Pos, PosMain) ;

        RotBackVector(RotMatrixMain, Pos ) ; 
        RotBackVector(RotMatrixMain, Dir ) ; 
        RotBackVector(RotMatrixMain, SpinVector) ; 

        IntegralIntensity += Prob ;

        NumOut++ ;

        /* computes neutron variables in the output frame */ 
        SubVector(Pos, TranslOut) ;

        /* translates neutron variables for output - X'=0. */
        TOF3 = - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;
	
        CopyVector(Dir, Path) ;
        MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;
        AddVector(Pos, Path) ;  TOF += TOF3 ;

        /*jumpwrite :; goto jumpwrite ;*/

        /* transmit coordinates which were not changed, the rest overwrite below */
        Neutrons = InputNeutrons[i]; 
        Neutrons.Time = TOF ;

        CopyVector(Pos, Neutrons.Position) ;
        CopyVector(Dir, Neutrons.Vector) ;
        CopyVector(SpinVector, Neutrons.Spin) ;

        /* writes output binary file */
        WriteNeutron(&Neutrons) ;

      getlost: ;
      }
    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
 my_exit:
  /* write to log file */
  if (NumOut != 0) 
    fprintf(LogFilePtr,"Average number of precessions  : %10.3lf\n", NumberPrecessions/NumOut) ;
  else
    fprintf(LogFilePtr," \n") ;

  /* write geometry file */
  SetGeometry("yellow");
  
  /* Do module specific cleanups */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], 0.0,0.0);	

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int j=0, k=0, l=0, m=0;

  /* INIT */
  InitVector(PosMain);
  InitVector(TranslOut);

  Init3x3Matrix(RotMatrixMain);

  for (j=0; j < 3; j++)
  { for (k=0; k < FIELD_SIZE; k++)
    { for (l=0; l < FIELD_SIZE; l++)
      { for (m=0; m < FIELD_SIZE; m++)
        {
          domain_field_F[j][k][l][m]=0.0;
          PosDomain_F   [j][k][l][m]=0.0;
          DimDomain_F   [j][k][l][m]=0.0;
        }
      }
    }
  }

  /* input parameters */
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        FieldFileName = &argv[1][2];
        break;
			
      case 'i':
        sscanf(&argv[1][2], "%lf", &AnglMainHoriz) ;
        break;
      case 'j':
        sscanf(&argv[1][2], "%lf", &AnglMainVert) ;
        break;

      case 'k':
        sscanf(&argv[1][2], "%lf", &PosMain[0]) ;
        break;
      case 'l':
        sscanf(&argv[1][2], "%lf", &PosMain[1]) ;
        break;
      case 'm':
        sscanf(&argv[1][2], "%lf", &PosMain[2]) ;
        break;

      case 'p':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
        break;
      case 'r':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
        break;
      case 's':
        sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
        break;

      case 'O':
        sscanf(&argv[1][2], "%ld", &Option) ;
        break;

      case 'X':
        sscanf(&argv[1][2], "%lf", &depth) ;
        break;
      case 'Y':
        sscanf(&argv[1][2], "%lf", &width) ;
        break;
      case 'V':
        sscanf(&argv[1][2], "%lf", &height) ;
        break;

      case 'T':
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

  /* homogeneous field  */
  if (Option != 1)
  {
    writemagneticmap();
  }

  readmagneticmap();/**/

  AnglMainVert = AnglMainVert*(M_PI)/180.0;
  AnglMainHoriz = AnglMainHoriz*(M_PI)/180.0;

  FillRotMatrixZY(RotMatrixMain, AnglMainVert, AnglMainHoriz) ;

  if (Option == 1)  // from file
  { fprintf(LogFilePtr, "inhomogeneous field from file: '%s'\n", FieldFileName) ;
    fprintf(LogFilePtr, "length: %9.4f cm\n", depth);
  }
  else             // homogeneous
  { fprintf(LogFilePtr, "homogeneous field:  (%9.3lf %9.3lf %9.3lf) Gs\n", field_hom[0], field_hom[1], field_hom[2]) ;
    fprintf(LogFilePtr, "length, width, height: (%9.4f, %9.4f, %9.4f) cm\n", depth, width, height);
  }
	
  if (PosMain[0] < depth/2.) {fprintf(LogFilePtr,"\nERROR: X position must be larger than depth/2 ! \n\n") ; exit (-1);}

  if (TranslOut[0] < (PosMain[0] + depth/2.)) {fprintf(LogFilePtr,"\nERROR: output position must be outside of field domain ! \n\n") ; exit (-1);}

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
}


/**********************************/
/** reads the magnetic map file  **/
/**********************************/
void readmagneticmap() 
{
  VectorType Vec; 
  double Theta, Phi;

  FieldMapFile=OpenInputFile2(FieldFileName, "field map", "r");

  /* read number of matrix lines, columns from first line */
  ind_x_max = ReadParI(FieldMapFile); ind_y_max = ReadParI(FieldMapFile); ind_z_max = ReadParI(FieldMapFile); ReadParComment(FieldMapFile) ;

  depth=0.0;
  /* inhomogeneous field */
  for (ind_x=1; ind_x<(ind_x_max+1); ind_x++) 
  { 
    for (ind_y=1; ind_y<(ind_y_max+1); ind_y++) 
    { 
      for (ind_z=1; ind_z<(ind_z_max+1); ind_z++) 
      {
        /* reads position */
        PosDomain_F[0][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);
        PosDomain_F[1][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);
        PosDomain_F[2][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);

        /* reads domain size */
        DimDomain_F[0][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);
        DimDomain_F[1][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);
        DimDomain_F[2][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);

        /* This version only works  with domain dimension change along X */
        if(DimDomain_F[1][ind_x][ind_y][ind_z] != DimDomain_F[1][1][1][1])
        {
          fprintf(LogFilePtr,"\nERROR: This version only works with domain dimension change along X  ! \n\n") ;
          exit (-1);
        }

        if(DimDomain_F[2][ind_x][ind_y][ind_z] != DimDomain_F[2][1][1][1])
        {
          fprintf(LogFilePtr,"\nERROR: This version only works with domain dimension change along X  ! \n\n") ;
          exit (-1);
        }

        /* reads field vectors in spherical representation */
        domain_field_F[0][ind_x][ind_y][ind_z] = ReadParF(FieldMapFile);
        Phi = ReadParF(FieldMapFile);
        Theta = ReadParF(FieldMapFile);

        SphericalToCartesian(Vec, &Theta, &Phi) ;
        CartesianToEulerZY(Vec, &Theta, &Phi) ; 

        domain_field_F[1][ind_x][ind_y][ind_z] = Phi ;
        domain_field_F[2][ind_x][ind_y][ind_z] = Theta ;

        /* reads rest of line */
        ReadParComment(FieldMapFile) ;
      }
    }
    depth +=  DimDomain_F[0][ind_x][1][1]; 
  }

  fclose(FieldMapFile) ;

}/*  end readmagneticmap  */


/**********************************/
/** writes the magnetic map file **/
/**********************************/
void writemagneticmap() 
{ 
  long i_x, i_y, i_z, i_x_max, i_y_max, i_z_max ;
  double Dim[3], Posit[3], fieldsph[3] ;

  FieldMapFile=OpenInputFile2(FieldFileName, "field map", "w");

  /*    INPUT  */
  i_x_max = i_y_max = i_z_max = 2 ;

  /* print number of matrix lines, columns */
  fprintf(FieldMapFile, " %ld    %ld    %ld\n", i_x_max, i_y_max, i_z_max) ;

  /* inhomogeneous fieldsph */
  for(i_x=1;i_x<(i_x_max+1);i_x++)
  { for(i_y=1;i_y<(i_y_max+1);i_y++)
    { for(i_z=1;i_z<(i_z_max+1);i_z++) 
      {
        /* generate fieldsph domain size: uniform  */
        Dim[0] = depth/i_x_max ; 
        Dim[1] = width/i_y_max ; 
        Dim[2] = height/i_z_max ; 

        /* generate position */
        Posit[0] = ((i_x -1)-(i_x_max /2 - 0.5)) * Dim[0] ;		
        Posit[1] = ((i_y -1)-(i_y_max /2 - 0.5)) * Dim[1] ;		
        Posit[2] = ((i_z -1)-(i_z_max /2 - 0.5)) * Dim[2] ;

        /* generate field vectors in spherical representation */
        fieldsph[0] = LengthVector(field_hom) ;

        { VectorType fieldh; 
          CopyVector(field_hom, fieldh);
          MultiplyByScalar(fieldh, 1./fieldsph[0]) ;

          CartesianToSpherical(fieldh, &fieldsph[2], &fieldsph[1]) ;
        }

        /* print out to file */
        fprintf(FieldMapFile, " %le    %le    %le    %le    %le    %le    %le    %le    %le \n", 
                              Posit[0], Posit[1], Posit[2],
                              Dim[0], Dim[1], Dim[2],
                              fieldsph[0], fieldsph[1], fieldsph[2]);		
      }
    }
  }

  fclose(FieldMapFile);
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  /* Geometry data */
  if (bVisInstr)
  { 
    int iX=0, iY=0, iZ=0, iM=0;
    VectorType vOrient={1.0,0.0,0.0}, vCntr={1.0,0.0,0.0};

    RotBackVector(RotMatrixMain, vOrient); 

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr   = sVisDescrpt;
    stGeometry.eModule  = _eModule;

    stGeometry.nCuboids = ind_x_max * ind_y_max * ind_z_max; 
    stGeometry.pCuboid  = calloc(stGeometry.nCuboids, sizeof(VtCuboid));

    for (iX=1; iX <= ind_x_max; iX++)
    { 
      for (iY=1; iY <= ind_y_max; iY++)
      { 
        for (iZ=1; iZ <= ind_z_max; iZ++)
        { 
          vCntr[0]  = PosDomain_F[0][iX][iY][iZ];
          vCntr[1]  = PosDomain_F[1][iX][iY][iZ];
          vCntr[2]  = PosDomain_F[2][iX][iY][iZ];
          RotBackVector(RotMatrixMain, vCntr); 

          stGeometry.pCuboid[iM].Length    = DimDomain_F[0][iX][iY][iZ]; 
          stGeometry.pCuboid[iM].Width     = DimDomain_F[1][iX][iY][iZ] * BlowUp;
          stGeometry.pCuboid[iM].Height    = DimDomain_F[2][iX][iY][iZ] * BlowUp;
          stGeometry.pCuboid[iM].vCntr[0]  = PosMain[0] + vCntr[0];
          stGeometry.pCuboid[iM].vCntr[1]  = PosMain[1] + vCntr[1];
          stGeometry.pCuboid[iM].vCntr[2]  = PosMain[2] + vCntr[2];
          stGeometry.pCuboid[iM].vNormal[0]= vOrient[0];
          stGeometry.pCuboid[iM].vNormal[1]= vOrient[1];
          stGeometry.pCuboid[iM].vNormal[2]= vOrient[2];
          iM++;
        }
      }
    }
  }
}


/******************************************************************/
/** copies matrix/vector to 3D array of matrices/vectors or back **/
/******************************************************************/
void  CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3][3])
{
  long m, l ;
	
  for(m = 0;m<3;m++)
  {
    for(l = 0;l<3;l++)
    {
	    Result[m][l] = Matrix[m][l][i][j][k] ;
    }
  }
}

void	CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE])
{
  long m, l ;
	
  for(m = 0;m<3;m++)
  {
    for(l = 0;l<3;l++)
    {
      Matrix[m][l][i][j][k] = Result[m][l] ;
    }
  }
}

void	CopyVectorsToVector3(long i, long j, long k, double Vector[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE], double Result[3])
{
  long l ;

  for(l = 0;l<3;l++)
  {
    Result[l] = Vector[l][i][j][k] ;
  }
}

void	CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FIELD_SIZE][FIELD_SIZE][FIELD_SIZE]) 
{
  long l ;

  for(l = 0;l<3;l++)
  {
    Result[l][i][j][k] = Vector[l] ;
  }
}


/******************************************/
/** Intersection with rectangular object **/
/******************************************/
long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
  VectorType n, pos0, pos1, pos2, pos3, pos4, pos5 ;
  int			   k ;

  for (k=0; k<3; k++) Pos1[k] = Pos2[k] = 0. ;

  n[0] = 1. ; n[1] = n[2] = 0. ; *wall_1 = *wall_2 = 0 ;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
  {
    if(  (fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2) ) 
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos0, Pos1) ; *wall_1 = 1;}
      else {CopyVector(pos0, Pos2) ; *wall_2 = 1;}
    }
  } 
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
  {
    if(  (fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2) )
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos1, Pos1) ; *wall_1 = 2;}
      else {CopyVector(pos1, Pos2) ; *wall_2 = 2;}
    }
  }

  n[1] = 1. ; n[2] = n[0] = 0. ;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
  {
    if( (fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2) )
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos2, Pos1) ; *wall_1 = 3;}
      else {CopyVector(pos2, Pos2) ; *wall_2 = 3;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
  {
    if( (fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2) )
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos3, Pos1) ; *wall_1 = 4;}
      else {CopyVector(pos3, Pos2) ; *wall_2 = 4;}
    }
  }

  n[2] = 1. ; n[0] = n[1] = 0. ;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
  {
    if( (fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)  ) 
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos4, Pos1) ; *wall_1 = 5;}
      else {CopyVector(pos4, Pos2) ; *wall_2 = 5;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
  {
    if( (fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)  ) 
    {
      if(LengthVector(Pos1) == 0.) {CopyVector(pos5, Pos1) ; *wall_1 = 6;}
      else {CopyVector(pos5, Pos2) ; *wall_2 = 6;}
    }
  }

  if ((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) return 0 ;
	
  return 1 ;

}/* End IntersectionWithRectangularWallNumber() */
