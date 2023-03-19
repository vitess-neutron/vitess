/********************************************************************************************/
/*  VITESS module 'flipper_coil.c'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  JUL 2004  G. Zsigmond    corrections for tilted flipper option                      */
/* 1.4  Jul 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.5  Mar 2023  K. Lieutenant  visualization                                              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "intersection.h"
#include "softabort.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define	STRING_BUFFER 50
#define	FLD_SIZE	   100


/******************************/
/** Prototypes               **/
/******************************/
void		OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global parameters
void		OwnCleanup();                      // Does module specific cleanup
void    SetGeometry(char* sColor);         // Fills the structure stGeometry for visualization 
void    InitArrays();                      // Initializes the arrays 'domain_field_F' and 'PosDomain_F' 

/* copy matrix/vector to 3D array of matrices/vectors or back */
void		CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FLD_SIZE][FLD_SIZE][FLD_SIZE], double Result[3][3]) ;
void		CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FLD_SIZE][FLD_SIZE][FLD_SIZE]) ;
void		CopyVectorsToVector3 (long i, long j, long k, double Vector[3][FLD_SIZE][FLD_SIZE][FLD_SIZE], double Result[3]) ;
void		CopyVectorToVectors3 (long i, long j, long k, double Vector[3], double Result[3][FLD_SIZE][FLD_SIZE][FLD_SIZE]) ;
/* Intersection with rectangular object */
long		IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2) ;


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
VectorType  PosMain;                    // -k -l -m  [cm]  centre of the coil
long        coildir=1;                  // -y        [-]   orientation of the coil axis (1:Y, 2:Z)
double      AnglMainHoriz=0.0,          // -i       [deg]  horizontal (first) rotation angle of the coil axis
            AnglMainVert=0.0;           // -j       [deg]  vertical (second) rotation angle of the coil axis
double      depth= 0.0,                 // -X        [cm]  x-component of the size of the coil 
            width= 0.0,                 // -Y        [cm]  y-component of the size of the coil 
            height=0.0;                 // -V        [cm]  z-component of the size of the coil
double      field_guide[3],             // -G        [Oe]  strength of the guide magnetic field which is considered parallel to the beam axis
            field_coil=0.0,             // -H        [Oe]  strength of the coil magnetic field which is considered parallel to the coil axis
            wall_thickness=0.0;         // -t        [cm]  thickness of the coil wire (wall)
long        ind_x_max;                  // -N        [-]   number of 'boxes' along the beam in which the field is devided (max:100)
VectorType  TranslOut;                  // -p -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)

// Variables determined from input parameters or trajectory data
long        ind_x=0, ind_y=0,   ind_z=0,                     //  [-]   indices of magnetic field elements in x-, y- and z-direction
                     ind_y_max, ind_z_max;                   //  [-]   max. number of magnetic field elements in x-, y- and z-direction
VectorType  SizeDomain;
double      domain_field_F[3][FLD_SIZE][FLD_SIZE][FLD_SIZE], //        arrays of strengths, positions and sizes 
            PosDomain_F   [3][FLD_SIZE][FLD_SIZE][FLD_SIZE], //         of magnetic field elements
            RotMatrixMain [3][3];                            //  [deg]  Matrix to rotate the magnetic field


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  int    i=0;
  long   NumOut=0,
         wall_1=0, wall_2=0;
  double IntegralIntensity=0.0,
         RotMatrixField[3][3], 
         LarmorMatrix  [3][3], 
         TotNumPrec1=0.0, NumPrec1=0.0, 
         TotNumPrec2=0.0, NumPrec2=0.0, 
         TotNumPrec3=0.0, NumPrec3=0.0;
  double TOF, WL, Prob, 
         TOF1=0.0, TOF2=0.0, TOF3=0.0, 
         PhaseShift=0.0;
  VectorType Pos, Dir, SpinVector, 
             Path,                 /* displacement vector*/
             Pos1, Pos2, 
             PosDomain, domain_field;
  Neutron		 Neutrons ;

  // initialisation
  // --------------
  _eModule=MCN_FLIP_COIL;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.5");
  OwnInit(argc, argv);

  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(LarmorMatrix);
  InitVector(Path);
  InitVector(Pos1);
  InitVector(Pos2);
  InitVector(PosDomain);
  InitVector(domain_field);
  InitNeutron(&Neutrons);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
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
        /*InputNeutrons[i].Position[0]	= 0.0 ;*/
        TOF  = InputNeutrons[i].Time;
        WL   = InputNeutrons[i].Wavelength;
        Prob = InputNeutrons[i].Probability;

        CopyVector(InputNeutrons[i].Position, Pos) ;
        CopyVector(InputNeutrons[i].Vector,   Dir) ;
        CopyVector(InputNeutrons[i].Spin,     SpinVector) ;

        InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

        /* translates into frame of the main field and rotates coordinates  */
        SubVector(Pos, PosMain) ;
        RotVector(RotMatrixMain, Pos ) ;
        RotVector(RotMatrixMain, Dir ) ;

        /* enter position and TOF 	*/
        TOF1 = (- depth/2. - Pos[0])/ fabs(Dir[0]) / V_FROM_LAMBDA(WL) ;

        /* precession calculated  */
        PhaseShift = TOF1 * FREQUENCY_FROM_FIELD(LengthVector(field_guide));  
        NumPrec1 = PhaseShift/2./M_PI ;
        TotNumPrec1 += NumPrec1;

        FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;
        RotVector(LarmorMatrix, SpinVector) ;
        RotVector(RotMatrixMain, SpinVector) ;

        CopyVector(Dir, Path) ;
        MultiplyByScalar(Path, (- depth/2. - Pos[0])/ Dir[0] ) ;
        AddVector(Pos, Path) ;  TOF += TOF1 ;

        /* looks for first domain */
        ind_y = (long) floor(Pos[1] / SizeDomain[1]) + 1 + ind_y_max/2 ;
        if ((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost ;

        ind_z = (long) floor(Pos[2] / SizeDomain[2]) + 1 + ind_z_max/2 ;
        if ((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost ;

        ind_x = 1 ;

        /******************** starts to scan ******************************/

        NumPrec2 = 0 ;

        while (ind_x != (ind_x_max +1))
        {
          CopyVectorsToVector3(ind_x, ind_y, ind_z, PosDomain_F, PosDomain) ;
          CopyVectorsToVector3(ind_x, ind_y, ind_z, domain_field_F, domain_field) ;

          /* calculate field matrix */
          FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ;

          /* translates into frame of the field domain */
          SubVector(Pos, PosDomain) ;

          /* calculate entrance end exit coordinates of domain*/
          {
            VectorType pos, dir;	
            CopyVector(Pos, pos) ;	CopyVector(Dir, dir) ;
	
            /* gives intersection positions with domain */
            if (IntersectionWithRectangularWallNumber(SizeDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) goto getlost ;

            if (wall_2 == 0) goto getlost ;

            /* ordering */
            if(Pos1[0] > Pos2[0]) 	
            { VectorType V ;	int wall; CopyVector(Pos1, V) ;	CopyVector(Pos2, Pos1) ; CopyVector(V, Pos2) ; 	
		
              wall = wall_1 ; wall_1 = wall_2 ; wall_2 = wall ;
            }
          }

          /* moment of arriving at the domain wall, new position */
          CopyVector(Pos1, Pos) ;

          /* time of precession in the domain field - precession calculated in the field frame */
          TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

          RotVector(RotMatrixField, SpinVector) ;

          PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]) ;  
          NumPrec2 += PhaseShift/2./M_PI ;

          FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;
          RotVector    (LarmorMatrix,   SpinVector) ;
          RotBackVector(RotMatrixField, SpinVector) ;

          /* moment of exiting at the domain wall, new position */
          TOF += TOF2 ;

          CopyVector(Pos2, Pos) ;

          /* translates back into main frame */
          AddVector(Pos, PosDomain) ;

          /* searching new domain */ 
          if (wall_2 == 1) goto getlost;
          if (wall_2 == 2) {ind_x += 1 ; }
          if (wall_2 == 3) {ind_y += -1 ; }
          if (wall_2 == 4) {ind_y += 1 ; }
          if (wall_2 == 5) {ind_z += -1 ; }
          if (wall_2 == 6) {ind_z += 1 ;}

          /*if(ind_x > ind_x_max) goto exitfield ; */
          if (ind_y == 0)        goto exitfield ;
          if (ind_y > ind_y_max) goto exitfield ;
          if (ind_z == 0)        goto exitfield;
          if (ind_z > ind_z_max) goto exitfield;

          /*goto newdomain ;*/
        }

      exitfield: ;
        TotNumPrec2 += NumPrec2;

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

        /* precession calculated  */
        PhaseShift = TOF3 * FREQUENCY_FROM_FIELD(LengthVector(field_guide));  
        NumPrec3   = PhaseShift/2./M_PI ;
        TotNumPrec3 += NumPrec3;

        FillRotMatrixYX(LarmorMatrix, -PhaseShift, 0) ;
        RotVector(LarmorMatrix, SpinVector) ;
	
        CopyVector(Dir, Path) ;
        MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;
        AddVector(Pos, Path) ;  TOF += TOF3 ;

        /* transmit coordinates which were not changed, the rest overwrite below */
        Neutrons = InputNeutrons[i];

        Neutrons.Time = TOF ;

        CopyVector(Pos, Neutrons.Position) ;
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
	if (NumOut > 0) 
  { fprintf(LogFilePtr,"Average number of precessions in guide field : %10.3lf\n", TotNumPrec1/NumOut);
	  fprintf(LogFilePtr,"Average Number of precessions in flipper coil: %10.3lf\n", TotNumPrec2/NumOut);
	  fprintf(LogFilePtr,"Average Number of precessions in guide field : %10.3lf\n", TotNumPrec3/NumOut);
  }
  fprintf(LogFilePtr," \n") ;

  /* write geometry file */
  SetGeometry("magenta");
  
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
  VectorType field_cart ;

  field_guide[0]  = field_guide[1]  = field_guide[2]  = 0.0;
  InitVector(PosMain);
  InitVector(TranslOut);
  InitVector(SizeDomain);
  InitArrays();
  Init3x3Matrix(RotMatrixMain);

  /* Flipper */
  ind_x_max = 98; ind_y_max = ind_z_max = 2 ;

  while(argc>1)
  {
    switch(argv[1][1])
    {
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

      case 'y':
        sscanf(&argv[1][2], "%ld", &coildir) ;
        break;

      case 'N':
        sscanf(&argv[1][2], "%ld", &ind_x_max) ;
        if((ind_x_max/2. - floor(ind_x_max/2.)) > 0.) {	ind_x_max += 1 ; fprintf(LogFilePtr,"\nWARNING: Number of domains must be even! Set %ld. \n", ind_x_max) ;}
        if(ind_x_max > 100) {	ind_x_max = 100 ; fprintf(LogFilePtr,"\nWARNING: Number of domains must be < 102 ! Set %ld. \n", ind_x_max) ;}
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

      case 't':
        sscanf(&argv[1][2], "%lf", &wall_thickness) ;
        if(wall_thickness < 0.001) wall_thickness = 0.001 ;
        break;

      case 'G':
        sscanf(&argv[1][2], "%lf", &field_guide[0]) ;
        break;

      case 'H':
        sscanf(&argv[1][2], "%lf", &field_coil) ;
        break;

    }
    argc--;
    argv++;
  }

	if(PosMain[0] < depth/2.) {fprintf(LogFilePtr,"\nERROR: X position must be larger than depth/2 ! \n\n") ; exit (-1);}
	if(TranslOut[0] < (PosMain[0] + depth/2.)) {fprintf(LogFilePtr,"\nERROR: output position must be outside of flipper ! \n\n") ; exit (-1);}
	
	FillRotMatrixZY(RotMatrixMain, AnglMainVert, AnglMainHoriz) ;

	SizeDomain[0] = depth/ind_x_max ;
	SizeDomain[1] = width/ind_y_max ;
	SizeDomain[2] = height/ind_z_max ;

	/* inhomogeneous field */
	for(ind_x=1;ind_x<(ind_x_max+1);ind_x++) 
  { for(ind_y=1;ind_y<(ind_y_max+1);ind_y++) 
    { for(ind_z=1;ind_z<(ind_z_max+1);ind_z++) 
      {
			  PosDomain_F[0][ind_x][ind_y][ind_z] = ((ind_x -1)-(ind_x_max /2 - 0.5)) * SizeDomain[0] ;
			  PosDomain_F[1][ind_x][ind_y][ind_z] = ((ind_y -1)-(ind_y_max /2 - 0.5)) * SizeDomain[1] ;
			  PosDomain_F[2][ind_x][ind_y][ind_z] = ((ind_z -1)-(ind_z_max /2 - 0.5)) * SizeDomain[2] ;

			  /* Flipper  */
			  if (PosDomain_F[0][ind_x][ind_y][ind_z] < (double) (- (ind_x_max/2 * SizeDomain[0] - wall_thickness)))
			  {
				  field_cart[0] = 0. ;
				  field_cart[1] = 0. ;
				  field_cart[2] = field_coil/wall_thickness * (PosDomain_F[0][ind_x][ind_y][ind_z]  + ind_x_max/2 * SizeDomain[0]) ;
			  }
			  else if (PosDomain_F[0][ind_x][ind_y][ind_z] > (double) (ind_x_max/2 * SizeDomain[0] - wall_thickness))
			  {
				  field_cart[0] = 0. ;
				  field_cart[1] = 0. ;
				  field_cart[2] =  field_coil * (1. - (PosDomain_F[0][ind_x][ind_y][ind_z]  - (ind_x_max/2 * SizeDomain[0] - wall_thickness))/wall_thickness ) ;
			  }
			  else
			  {
				  field_cart[0] = 0. ; field_cart[1] = 0 ; field_cart[2] = field_coil  ;
			  }					

			  if(coildir != 1)
			  {
  			  field_cart[1] = field_cart[2] ; field_cart[2] = 0.;
			  }

			  RotVector(RotMatrixMain, field_guide ) ;
			  AddVector(field_cart, field_guide) ;

			  CartesianToEulerZY(field_cart, &domain_field_F[2][ind_x][ind_y][ind_z], &domain_field_F[1][ind_x][ind_y][ind_z]) ;	
			
			  domain_field_F[0][ind_x][ind_y][ind_z] = LengthVector(field_cart) ;
		  }
    }
  }

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  return;
}/* End OwnCleanup */


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

    stGeometry.nCuboids = 1; 
    stGeometry.pCuboid  = calloc(stGeometry.nCuboids, sizeof(VtCuboid));
      
    stGeometry.pCuboid[0].Length    = depth; 
    stGeometry.pCuboid[0].Width     = BlowUp * width;
    stGeometry.pCuboid[0].Height    = BlowUp * height;
    stGeometry.pCuboid[0].vCntr[0]  = PosMain[0];
    stGeometry.pCuboid[0].vCntr[1]  = PosMain[1];
    stGeometry.pCuboid[0].vCntr[2]  = PosMain[2];
    stGeometry.pCuboid[0].vNormal[0]= 1.0;
    stGeometry.pCuboid[0].vNormal[1]= 0.0;
    stGeometry.pCuboid[0].vNormal[2]= 0.0;
  }
}


/***************************************************************/
/** Initializes the arrays 'domain_field_F' and 'PosDomain_F' **/
/***************************************************************/
void InitArrays() 
{
  int i,j,k,l;

  for (i=0; i < 3; i++)
  { for (j=0; j < FLD_SIZE; j++)
    { for (k=0; k < FLD_SIZE; k++)
      { for (l=0; l < FLD_SIZE; l++)
        { domain_field_F[i][j][k][l] = 0.0;  
          PosDomain_F   [i][j][k][l] = 0.0;  
        }
      }
    }
  }

  return;
}


/******************************************************************/
/** copies matrix/vector to 3D array of matrices/vectors or back **/
/******************************************************************/
void  CopyMatricesToMatrix3(long i, long j, long k, double Matrix[3][3][FLD_SIZE][FLD_SIZE][FLD_SIZE], double Result[3][3])
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

void	CopyMatrixToMatrices3(long i, long j, long k, double Result[3][3], double Matrix[3][3][FLD_SIZE][FLD_SIZE][FLD_SIZE])
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

void	CopyVectorsToVector3(long i, long j, long k, double Vector[3][FLD_SIZE][FLD_SIZE][FLD_SIZE], double Result[3])
{
  long l ;

  for(l = 0;l<3;l++)
  {
    Result[l] = Vector[l][i][j][k] ;
  }
}

void	CopyVectorToVectors3(long i, long j, long k, double Vector[3], double Result[3][FLD_SIZE][FLD_SIZE][FLD_SIZE])
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
  VectorType	n, pos0, pos1, pos2, pos3, pos4, pos5 ;
  int			k ;

  for (k=0; k<3; k++) 
    Pos1[k] = Pos2[k] = 0.0;

  n[0] = 1.0; n[1] = n[2] = 0.0; 
  *wall_1 = *wall_2 = 0 ;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
  {
    if ((fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2))
    { if (LengthVector(Pos1) == 0.0) {CopyVector(pos0, Pos1); *wall_1 = 1;}
      else                           {CopyVector(pos0, Pos2); *wall_2 = 1;}
    }
  }

  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
  {
    if ((fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos1, Pos1); *wall_1 = 2;}
      else                           {CopyVector(pos1, Pos2); *wall_2 = 2;}
    }
  }

  n[1] = 1.0; n[2] = n[0] = 0.0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
  {
    if ((fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos2, Pos1); *wall_1 = 3;}
      else                           {CopyVector(pos2, Pos2); *wall_2 = 3;}
    }
  }

  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
  {
    if ((fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos3, Pos1); *wall_1 = 4;}
      else                           {CopyVector(pos3, Pos2); *wall_2 = 4;}
    }
  }

  n[2] = 1. ; n[0] = n[1] = 0. ;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
  {
    if ((fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos4, Pos1); *wall_1 = 5;}
      else                           {CopyVector(pos4, Pos2); *wall_2 = 5;}
    }
  }

  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
  {
    if ((fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos5, Pos1); *wall_1 = 6;}
      else                           {CopyVector(pos5, Pos2); *wall_2 = 6;}
    }
  }

  if ((LengthVector(Pos1) == 0.0) || (LengthVector(Pos2) == 0.0)) return 0 ;

  return 1 ;

}/* End IntersectionWithRectangularWallNumber() */
