/*********************************************************************************************/
/*  VITESS module 'sesansfield.c'                                                            */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.9  Jul 2008  K. Lieutenant  preliminary version, developed from 'precessionfield.c'     */
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


short ReadFieldArea (char* sFileName);
short DetermineWalls();
short DeterminePlane(Plane* wall,  VectorType vCorner1, VectorType vCorner2, VectorType vCorner3, VectorType vCorner4);
void  Invert        (double Ni[NMAX+1][NMAX+1],   double Nm[NMAX+1][NMAX+1]);
short IsPointInside (VectorType p, short iC1, short iC2, short iC3, short iC4);
void	OwnInit       (int argc, char *argv[]) ;
void	OwnCleanup    () ;

static 
double	  RotMatrixMain[3][3], RotMatrixField[3][3];

short      mOut=0,             // parameter to control output of inverted matrix
	        nX  =3;             // dimension of matrix to invert
short      nCorners=0,         // number of points limiting the magnetic field area
           nWalls=0,           // number of walls limiting the magnetic field area
           iCorner[MAX_WALLS][4]={{0,1,2,3},{4,5,1,0},{5,6,2,1},{6,7,3,2},{7,4,0,3},{4,5,6,7}};
                               // indices of the corners defining the walls
VectorType PosMain,            // centre of the magnetic field (not used at the moment)
           TranslOut,          // position of the new origin  (in the co-ordinate of the old origin)
           domain_field,       // magnetic field [Ørsted] in Euler co-ordinates
           vCorner[MAX_EDGES]; // position of the corners (limiting the magnetic field area)
Plane      vWall[MAX_WALLS],   // structure describing the walls (limiting the magnetic field area)
           vExit;              // structure describing plane through new origin

int main(int argc, char **argv)
{
	short      k, kHit;
	long       i, NumOut=0;
	double     LarmorMatrix[3][3],
	           PhaseShift, Precessions, TotNumPrec=0.0,
	           TOF, TOF1, TOF2=1.0E99, TOF3,     
	           lambda;
	Neutron    InNeutron, Out1Neutron, Out2Neutron, TestNeutron;


	/* Initialize the program according to the parameters given  */ 
	Init   (argc, argv, VT_SESANS_FIELD);  
	print_module_name("sesansfield 0.9");
	OwnInit(argc, argv);

	 /* Get the neutrons from the file */
	DECLARE_ABORT;

	while((ReadNeutrons())!= 0)
	{
		CHECK;	

		for(i=0;i<NumNeutGot ;i++)
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
   
	/* Do the general cleanup */
  my_exit:
	if(NumOut != 0) fprintf(LogFilePtr,"Average number of precessions  : %9.2lf\n", TotNumPrec/NumOut);
	fprintf(LogFilePtr," \n");

	OwnCleanup(); 
	Cleanup   (TranslOut[0], TranslOut[1], TranslOut[2], 0.0,0.0);	

	return 0;
}



/* own initialization of the Precession Field module */

void OwnInit(int argc, char *argv[])
{
	double field_hom[3]={0.0,0.0,50.0}, 
	       AnglMainHoriz=0.0, AnglMainVert=0.0;
	const char*  FieldFileName="";

	PosMain[0] = PosMain[1] = PosMain[2] = 0.0;  

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
	AnglMainVert  = AnglMainVert *(M_PI)/180.0;
	AnglMainHoriz = AnglMainHoriz*(M_PI)/180.0;

	FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 
	FillRotMatrixZY(RotMatrixMain,  AnglMainVert, AnglMainHoriz) ;

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
/* End OwnInit */


/* own cleanup of the sesansfield module */

void OwnCleanup()
{

}
/* End OwnCleanup */



short ReadFieldArea(char* sFileName)
{
	short nCrnr=0;
	FILE* pFieldFile;
	char  sLine[256];

	if ((pFieldFile = fopen(sFileName,"r")) == NULL)
	{
		fprintf(LogFilePtr,"\nERROR: precession field file '%s' not found! \n", sFileName);
		exit(-1);
	}
	else
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
	{	V[j] = InvMat[j][1]*d[1] + InvMat[j][2]*d[2] + InvMat[j][3]*d[3]; 
	}
 
	// normalize solution vector
	for (j=1; j<=3; j++)
	{	v[j] = V[j] / sqrt(sq(V[1])+sq(V[2])+sq(V[3]));
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
	{	NM[i][i] += 1.0;
	}
	for (m=nX; m>=1; m--)
	{	PM=NM[m][m]-1;
      if (PM == 0.0) 
		{	fprintf(LogFilePtr,"\nERROR: Matrix diagonal element is 0\n");
         exit(99);
      }
		for (j=1; j<=nX; j++)
		{	NM[m][j]=NM[m][j]/PM;
		}
		for (i=1; i<=nX; i++)
		{	if (i != m) 
			{	D = NM[i][m];
				for (j=1; j<=nX; j++)
				{	NM[i][j] = NM[i][j] - D*NM[m][j];
				}
         }
		}
	}

	for (m=1; m<=nX; m++)
	{	for (j=1; j<=nX; j++)
		{	NI[m][j]=NM[m][j];
		}
      NI[m][m] -= 1.0;
      if (NI[m][m] == 0.0) 
		{	fprintf(LogFilePtr, "\nERROR: Normal matrix singular\n");
         return;
      }
	}

	/* Parameter output of this step*/
   if (mOut == 3) 
	{	fprintf(LogFilePtr, "\n---------------------------\n");
		for (i=1; i<=nX; i++)
		{	for (j=1; j<=nX; j++)
			{	fprintf(LogFilePtr, "%12.4f", NI[i][j]);
			}
			fprintf(LogFilePtr, "\n");
		}
		fprintf(LogFilePtr, "\n---------------------------\n");
	}
}





