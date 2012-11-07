/**********************************************************************************************/
/*  VITESS module sample_sans                                                                 */
/* This program  simulates the coherent elastic  diffraction of neutrons at a SANS sample.    */
/*                                                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to  */
/* the authors.                                                                               */
/*                                                                                            */
/* 1.0      1999  F. Streffer                                                                 */
/* 1.1  May 2001  K. Lieutenant  adding ellipsoids, cylinders, parallelepipeds                */
/* 1.2  Jun 2001  K. Lieutenant  absolute current values, input data for Q ignored,           */
/*                                  SOFTABORT                                                 */
/* 1.3  Nov 2001  K. Lieutenant  corrections in NeutronIntersectsCylinder                     */
/* 1.4  Jan 2002  K. Lieutenant  reorganisation                                               */
/* 1.5  Feb 2002  K. Lieutenant  correction detector coverage,                                */
/*                                deletion of Q-range, adding of incoher. scattering          */
/* 1.6  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                 */
/* 1.7  Feb 2004  K. Lieutenant  'FullParName', 'message' & 'ERROR' included; output extended */
/* 1.8  Nov 2012  K. Lieutenant  size distribution of spheres                                 */
/**********************************************************************************************/

#include <string.h>

#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/

double DelTheta= M_PI/2.0,  /* angles determining the detector coverage */
       Theta   = M_PI/2.0,      
       DelPhi  = M_PI,
       Phi     = M_PI;
long   GenNeutrons =1;      /* how many trajectories to generate per incoming trajectory */
char  *SampleFileName,      /* pointer to the name of the sample file      */
       g_cGeometry = ' ';   /* geometry parameter: 
                               S: spheres,        R  = SizeA
                               s: size dstr. sph. Rmin=SizeA, Rmax=SizeB
                               E: ellipsoids      Rx = SizeA, Ry = SizeB, Rz = SizeC
                               C: cylinders,      Rx = SizeA, Ry = SizeB, L  = SizeC
                               P: parallelepiped, a  = SizeA, b  = SizeB, c  = SizeC
                               I: no scattering objects, isotropic scattering */ 
double g_fSizeA   = -1.0, 
       g_fSizeB   = -1.0, 
       g_fSizeC   = -1.0,   /* size of the particles [Angstr.] e.g. hard spere radius in x-, y-, and z-direction */
       g_fRho1    = 1.0e10, /* scattering length density of particles */
       g_fRho2    = 1.0e10, /* scattering length density of solution */
       g_fFracPtkl= 0.01,   /* volume fraction of the particles */
       g_fMuInc   = 0.0;    /* incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] */
long   g_bIncohScat=FALSE;  /* should incoherent scattering be done */

double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};



/******************************/
/**   Extern Variables       **/
/******************************/

extern double g_fMuTot, g_fMuAbs;                   /* defined in 'sample.c' */



/***********************************/
/** Prototypes of local functions **/
/***********************************/

void   OwnInit   (int argc, char *argv[]);
void   OwnCleanup();
void   GetSample (SampleType *Sample);

double FormFactorSphere   (double dQ,  double dR);
double FormFactorEllipsoid(double dQx, double dQy,    double dQz, 
                           double dRx, double dRy,    double dRz);
double FormFactorCylinder (double dQx, double dQy,    double dQz, 
                           double dRx, double dRy,    double dHeight);
double FormFactorEpiped   (double dQx, double dQy,    double dQz, 
                           double dLen,double dWidth, double dHeight);
double FormFactorLayer    (double dQ,  double dThick);

double FktA   (double u);
double FktB   (double u);
double FktC   (double u);
double Bessel1(double x);



/******************************/
/**   Program                **/
/******************************/

int main(int argc, char *argv[])
{
	SampleType Sample;      /* sample geometry */
	VectorType InISP[2];    /* neutron intersection before scattering */
	double     qValue,      /* absolute value of momentum transfer    */
	           fThetaMin,   /* minimal and maximal values of the           */
	           fThetaMax,   /* scattering angle according to Theta, DelTheta */
	           fVolPtkl=0.0,/* Volume of the particle [cm³] */
	           fFacCtrPtkl, /* factor considering contrast and particle size */ 
	           fFormFac,    /* normalized form factor for the partical shape and size */
	           fFac, 
	           neutTheta,
	           neutPhi;
	double     DetFacInc,   /* care about the detector coverage  */
	           DetFacCoh,     
	           Lbf;         /* full path length of the neutron in the sample */
									/* with its initial direction */
	double     Ls;          /* distance of the neutron in the sample before sc. */
	long       j;           /* counting variable */
	VectorType SP,          /* position of scattering event */
	           dQ;          /* momentum transfer in the particle coordinate system   */
	long       Nth;         /* counting variable of structure factor */
	double     ScTheta,     /* angle of coherent Scattering */
	           ScProb,      /* scattering probability */
             Radius,      /* radius of a sphere     */
	           OutTheta,    /* Final angles of the neutron in the sample system */
             OutPhi;
	double     RotMatrixSmpl[3][3], /* Rotation matrices that transform a Vector to the */
	           RotMatrixNeut[3][3]; /* sample coordinate system                         */
							
	long       i,           /* counting variable of the neutrons */
	           nisp,        /* number of intersection points to come */
	           NeutCount;

	/* Initialization */
	Init(argc, argv, VT_SMPL_SANS);
	print_module_name("sample_sans 1.8");
	OwnInit(argc, argv);

	/* Go and get the geometry of the sample and the scattering objects */
	InitSample(&Sample);
	GetSample (&Sample);
	switch (Sample.Type)
	{	case VT_CUBE: 
			fprintf(LogFilePtr, "Cubic sample, sizes : %8.2f,%8.2f,%8.2f   cm  (thickness, height, width)\n"
			                    "  direction         :(%9.3f,%8.3f,%8.3f)   \n",
			                    Sample.SG.Cube.thickness, Sample.SG.Cube.height, Sample.SG.Cube.width,
			                    Sample.Direction[0], Sample.Direction[1], Sample.Direction[2]);
			break;
		case VT_CYL: 
			fprintf(LogFilePtr, "Cylindrical sample  : %8.2f cm radius%6.2f cm height\n"
			                    "  direction         :(%9.3f,%8.3f,%8.3f)   \n",
			                    Sample.SG.Cyl.r, Sample.SG.Cyl.height,
			                    Sample.Direction[0], Sample.Direction[1], Sample.Direction[2]);
			break;
		case VT_SPHERE: 
			fprintf(LogFilePtr, "Spherical sample    : %8.2f cm radius\n", 
			                    Sample.SG.Ball.r);
			break;
	default: ;
	}
	fprintf(LogFilePtr, "  position          :(%8.2f,%8.2f,%8.2f ) cm\n",
							  Sample.Position [0], Sample.Position [1], Sample.Position [2]);
	switch (g_cGeometry)
	{	case 'S': 
			fprintf(LogFilePtr, "Spherical particles : %8.2f Ang radius\n", g_fSizeA); 
			break;
		case 'D': 
			fprintf(LogFilePtr, "Spherical particles from %8.2f to %8.2f Ang radius\n", g_fSizeA, g_fSizeB); 
			break;
		case 'E': 
			fprintf(LogFilePtr, "Elliptic particles  : radii %8.2f,%8.2f,%8.2f Ang\n", 
			                    g_fSizeA, g_fSizeB, g_fSizeC); 
			break;
		case 'P': 
			fprintf(LogFilePtr, "Cubic particles     : %8.2f,%8.2f,%8.2f Ang length\n", 
			                    g_fSizeA, g_fSizeB, g_fSizeC); 
			break;
		case 'C': 
			fprintf(LogFilePtr, "Cylindric particles : radii %8.2f,%8.2f, length:%8.2f Ang\n", 
			                    g_fSizeA, g_fSizeB, g_fSizeC); 
			break;
		case 'I': 
			fprintf(LogFilePtr, "Particles scattering isotropically\n"); 
			break;
	}
	fprintf(LogFilePtr, "scat. length density: %13.3e (particle) %10.3e 1/cm² (solvent)\n"
							  "vol.fract. of part. : %8.3f\n"
							  "macr. cross section : %10.5f,%10.5f;%10.5f  1/cm (incoh, total scat; absorption)\n",
							  g_fRho1, g_fRho2, g_fFracPtkl, g_fMuInc, g_fMuTot, g_fMuAbs);

	/* Factor that takes care of the dectector coverage */
	DetFacInc = DelPhi/M_PI*DelTheta;
	DetFacCoh = DelPhi/M_PI*DelTheta;

	/* determine the rotation matrix to find a new basis */
	/* with the sample vector pointing along the z-axis  */
	RotMatrixX(Sample.Direction, RotMatrixSmpl);

	DECLARE_ABORT

	/* Start the main loop getting neutrons         */
	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			/* First, shift the origin of the system to the center of the sample */
			SubVector(InputNeutrons[i].Position, Sample.Position);

			/* Test if the Neutron hits the Sample */
			if (NeutronIntersectsSample(&(InputNeutrons[i]), &Sample, RotMatrixSmpl, InISP, &nisp, VT_IN))
			{
				if (nisp < 2)
					CountMessageID(SMPL_TRAJ_INSIDE, InputNeutrons[i].ID);
				
				/* the neutron may be scattered between InISP[0] and InISP[1] */
				/* Lfb full path length in the sample before scattering       */
				Lbf=DistVector(InISP[0], InISP[1]);

				/* MONTE CARLO CHOICE: Where is the neutron scattered         */
				/* Distance Ls between entrance of the neutron InISP[0] and   */
				/* the scattering point SP                                    */
				Ls = MonteCarlo(0, Lbf);

				/* which is the corresponding scattering point     */
				/* SP = InISP[0] + Ls*InputNeutrons[i].Vector	   */
				for(j=0; j<3; j++)
					SP[j] = InISP[0][j] + Ls*InputNeutrons[i].Vector[j];

				/* Now the actual scattering */
				/* determine Theta and Phi of the neutrons direction */
				/* Theta should be small */
				NormVector          (InputNeutrons[i].Vector);
				CartesianToSpherical(InputNeutrons[i].Vector, &neutTheta, &neutPhi);

				/* Determine the rotation matrix to point the neutron along */
				/* the +x axis				      		    */
				RotMatrixX(InputNeutrons[i].Vector,RotMatrixNeut);

				for(Nth=0; Nth < GenNeutrons; Nth++) 
				{
					CHECK

					fThetaMin = Theta-DelTheta;
					fThetaMax = Theta+DelTheta;

					// Choose the scattering angle and calculate Q-value 
					ScTheta = MonteCarlo(fThetaMin, fThetaMax);
					qValue  = 4.0*M_PI*sin(ScTheta/2.0)/InputNeutrons[i].Wavelength;

					// Choose the components along the particle axes
					for (j=0; j<=2; j++)
					{	dQ[j] = MonteCarlo(-1.0,1.0);
					}
					fFac = sqrt(qValue*qValue/(dQ[0]*dQ[0] + dQ[1]*dQ[1] + dQ[2]*dQ[2]));
					for (j=0; j<=2; j++)
					{	dQ[j] *= fFac;
					}

					/* OutTheta is the angle of the scattered neutron with its original flight path */
					OutTheta=ScTheta;

					/* OutPhi is the angle of the scattered neutron with the +y-axis */
					OutPhi = MonteCarlo(Phi-DelPhi, Phi+DelPhi);

					/* ScProb corresponds to the sample form factor considering hard sphere scattering */
					switch (g_cGeometry)
					{
						case 'S': 
							fFormFac = FormFactorSphere(qValue, g_fSizeA);
							fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(g_fSizeA,3);
							break;
						case 'D': 
              Radius   = MonteCarlo(g_fSizeA, g_fSizeB);
							fFormFac = FormFactorSphere(qValue, Radius);
							fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(Radius,3);
							break;
						case 'E': 
							fFormFac = FormFactorEllipsoid(dQ[0], g_fSizeA, dQ[1], g_fSizeB, dQ[2], g_fSizeC);
							fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * g_fSizeA*g_fSizeB*g_fSizeC;
							break;
						case 'C': 
							fFormFac = FormFactorCylinder(dQ[0], g_fSizeA, dQ[1], g_fSizeB, dQ[2], g_fSizeC);
							fVolPtkl = 1.0e-24 * M_PI * g_fSizeA*g_fSizeB * g_fSizeC;
							break;
						case 'P': 
							fFormFac = FormFactorEpiped(dQ[0], g_fSizeA, dQ[1], g_fSizeB, dQ[2], g_fSizeC);
							fVolPtkl = 1.0e-24 * g_fSizeA*g_fSizeB*g_fSizeC;
							break;
						default:
							fFormFac = 1.0; 
							break;
					}

					// Determine the scattering probability from the form factor, 
					// contrast and particle size, the sample size, and the solid angle factor,
					if (g_cGeometry=='I')
					{	fFacCtrPtkl = 1.0;
					}
					else
					{	fFacCtrPtkl = g_fFracPtkl* pow((g_fRho1-g_fRho2),2) * fVolPtkl;
					}

					/* Scattering probability */
					ScProb = 4*M_PI * Lbf * fFormFac * fFacCtrPtkl * sin(ScTheta) / GenNeutrons;

					/* Ok, now everthing needed is known, put it together */
					if (ScProb > 0.0 && DetFacCoh > 0.0)
						ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, ScProb,
												  OutTheta, OutPhi, &Sample, RotMatrixNeut, RotMatrixSmpl);
				}
				
				/************************************/
				/* Second the incoherent scattering */
				/************************************/
				if (g_bIncohScat && g_fMuInc > 0.0)
				{ 
					for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
					{
						/* Determine the scattering angle */
						OutPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
						OutTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

						/* Scattering probability */
						ScProb  = Lbf*g_fMuInc * sin(OutTheta) / GenNeutrons;

						ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, ScProb,
												  OutTheta, OutPhi, &Sample, OneMatrix,  RotMatrixSmpl);
					}
				}
			}
		}
	}

	/* Release the allocated memory */
	my_exit:
	OwnCleanup();
	stPicture.eType = (short) Sample.Type;
	Cleanup(Sample.Position[0],Sample.Position[1],Sample.Position[2], 0.0,0.0);

	return 0;
}



void  OwnInit(int argc, char *argv[])
{
	/*********************************************************************/
	/* Here we will set some global variables to get things going        */
	/* If there is no sample specification the program is aborted        */
	/* Known command line parameters:                                    */
	/*  -A     Neutron repetition rate on the cone    (default: 1)       */
	/*  -R     Neutron repetition rate in the Q-range (default: 1)       */
	/*  -D -d  theta-range [D-d, D+d]   (default: [0, pi])               */
	/*  -p -P  phi-range   [P-p, P+p]   (default: [0,2*pi])              */
	/*  -q -Q  Q-range [q, Q]                                            */
	/*  -S     sample geometry file                                      */
	/*********************************************************************/
	
	long i;
	int  detectortest=0;
	
	/* Ok, scan all command line parameters */
	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') 
		{
			switch(argv[i][1])
			{
				/* Consider incoherent scattering or not */
				case 'I':
					if(argv[i][2]=='1') g_bIncohScat=TRUE;
					break;

				/* Repetition rate and multiplicity */
				case 'A':
					sscanf(&(argv[i][2]),"%ld", &GenNeutrons);
					break;

				/* get the solid angle covered by the detectors if other than 4*PI */
				/* read four numbers                                               */
				case 'D':
					sscanf(&(argv[i][2]),"%lf", &Theta);
					Theta*=M_PI/180.0;
					detectortest &= 1000L;
					break;

				case 'd':
					sscanf(&(argv[i][2]),"%lf", &DelTheta);
					DelTheta*=M_PI/180.0;
					detectortest &= 0100L;
					break;
				
				case 'P':
					sscanf(&(argv[i][2]),"%lf", &Phi);
					Phi*=M_PI/180.0;
					detectortest &= 0010L;
					break;

				case 'p':
					sscanf(&(argv[i][2]),"%lf", &DelPhi);
					DelPhi*=M_PI/180.0;
					detectortest &= 0001L;
					break;


				case 'S':
					/* what is the sample file called? */
					SampleFileName=&argv[i][2];
					break;

				default:
					fprintf(LogFilePtr,"ERROR: unkown command option: %s\n", argv[i]);
					exit(-1);
			}
		}
	}

	/* Check, whether all 4 angles are given; if not, initial values are set again */	
	if( detectortest!=0 && detectortest!=15) 
	{
		fprintf(LogFilePtr,"ERROR: You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI!\n");
		Theta = M_PI/2.0;
		DelTheta= M_PI/2.0;
		Phi   = M_PI;
		DelPhi  = M_PI;
	}

	/* Theta has to be in the range of [0;PI] */
	if((Theta-DelTheta < 0.0) || (Theta+DelTheta > M_PI)) 
		Error("Theta has to be in the range of [0;PI]");

	/* Phi has to be in the range of [0;2*PI] */
	if((Phi-DelPhi < 0.0) || (Phi+DelPhi > 2.0*M_PI)) 
		Error("Phi has to be in the range of [0;2*PI]");
}


/* cleanup of this module */
/* ---------------------- */
void OwnCleanup()
{
  /* print error that might have occured many times */
  PrintMessage(SMPL_TRAJ_INSIDE, "", ON);
  fprintf(LogFilePtr, "\n");
}
/* End OwnCleanup */



void GetSample(SampleType *Sample)
{
	FILE *SampleFile;
	char Buffer[CHAR_BUF_LENGTH];

	if((SampleFile=fopen(FullParName(SampleFileName),"rt"))==NULL) 
	{
		fprintf(LogFilePtr,"\nERROR: Cannot open sample file %s\n", SampleFileName);
		exit(-1);
	}

	/* Lets get started read first line */
	if(ReadTilComment(Buffer, SampleFile)) 
	{
		/* got first line, lets see what in there 	*/
		sscanf(Buffer, "%lf %lf %lf", &(Sample->Position[0]), &(Sample->Position[1]), 
		                              &(Sample->Position[2]));
		
		/* Next line should decribe the type of geometry */
		/* cylinder, cube, ball			     */
		if(ReadTilComment(Buffer, SampleFile)) 
		{
			if(strstr(Buffer, "cyl")!=NULL) 
			{
				ReadCylinder(SampleFile, Sample);
				Sample->Type=VT_CYL;
			} 
			else if(strstr(Buffer, "cub")!=NULL) 
			{
				ReadCube(SampleFile, Sample);
				Sample->Type=VT_CUBE;
			} 
			else if(strstr(Buffer, "bal")!=NULL) 
			{	ReadBall(SampleFile, Sample);
				Sample->Type=VT_SPHERE;
			}
			else 
			{	fprintf(LogFilePtr, "ERROR: Please denote the sample geometry by cyl, cub or bal on the second line of %s\n", SampleFileName);
				exit(-1);
			}

			/* the direction vector should have a positive z component  */
			/* this will make things easier with the rotations later on */
			if(Sample->Direction[2] < 0) 
			{
				Sample->Direction[0] = -Sample->Direction[0];
				Sample->Direction[1] = -Sample->Direction[1];
				Sample->Direction[2] = -Sample->Direction[2];
			}
			/* Sample Geometry is read */
		
			/* Read geometry and sizes of the scattering particles */
			if(ReadTilComment(Buffer, SampleFile)) 
			{
				sscanf(Buffer,"%c%lf%lf%lf", &g_cGeometry, &g_fSizeA, &g_fSizeB, &g_fSizeC);
			} 
		
			/* Read scattering density and volume fraction of the scattering particles */
			if(ReadTilComment(Buffer, SampleFile)) 
			{
				sscanf(Buffer,"%le%le%lf", &g_fRho1, &g_fRho2, &g_fFracPtkl);
			} 
		
			/* Read macroscopic scattering cross sections */
			if(ReadTilComment(Buffer, SampleFile)) 
			{
				sscanf(Buffer,"%lf%lf%lf", &g_fMuInc, &g_fMuTot, &g_fMuAbs);
			} 

			/*	Allowed char. for geometry parameter: 
			   S: spheres  E: ellipsoids  C: cylinders  P: epiped  F: flat objects  I: isotropic sample */
			switch (g_cGeometry)
			{	case 'E': 
				case 'C':
				case 'P': 
					if (g_fSizeA==-1.0 || g_fSizeB==-1.0 || g_fSizeC==-1.0)
					{	fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
						                    SampleFileName);
						exit(-1);
					}
					break;
				case 'D': 
					if (g_fSizeA==-1.0 || g_fSizeB==-1.0)
					{	fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
						                    SampleFileName);
						exit(-1);
					}
					break;
				case 'S':
					if (g_fSizeA==-1.0)
					{	fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
						                    SampleFileName);
						exit(-1);
					}
					break;
				case 'I':
					break;
				default:
					fprintf(LogFilePtr, "ERROR: No or wrong value given for geometry in %s", SampleFileName);
					exit(-1);
			}
			
			/* Seems as everything needed could be read */
		} 
		else 
		{
			fprintf(LogFilePtr, "ERROR: Can't read second line of %s",SampleFileName);
			exit(-1);
		}
	} 
	else 
	{
		fprintf(LogFilePtr, "ERROR: Can't read first line of %s",SampleFileName);
		exit(-1);
	}
	fclose(SampleFile);
}



double FormFactorSphere(double p_dQ, double p_dR)
{
	double dSc;
	
	dSc = pow(FktB(p_dQ*p_dR), 2);

	return dSc;
}


double FormFactorEllipsoid(double p_dQx, double p_dQy, double p_dQz, 
                           double p_dRx, double p_dRy, double p_dRz)
{
	double dSc, dU2;
	
	dU2 = pow(p_dQx*p_dRx, 2) + pow(p_dQy*p_dRy, 2) + pow(p_dQz*p_dRz, 2);
	dSc = pow(FktB(sqrt(dU2)), 2);

	return dSc;
}

double FormFactorCylinder(double p_dQx, double p_dQy, double p_dQz, 
                          double p_dRx, double p_dRy, double p_dHeight)
{
	double dSc, dRz, dU2;

	dRz = 0.5 * p_dHeight;
	dU2 = pow(p_dQx*p_dRx, 2) + pow(p_dQy*p_dRy, 2);
	dSc = pow(FktC(sqrt(dU2)), 2) * pow(FktA(p_dQz*dRz), 2) ;

	return dSc;
}

double FormFactorEpiped(double p_dQx, double p_dQy, double p_dQz, 
                        double p_dLength, double p_dWidth, double p_dHeight)
{
	double dSc, dRx, dRy, dRz;
	
	dRx = 0.5 * p_dLength;
	dRy = 0.5 * p_dWidth;
	dRz = 0.5 * p_dHeight;
	dSc = pow(FktA(p_dQx*dRx),2) * pow(FktA(p_dQy*dRy),2) * pow(FktA(p_dQz*dRz),2);

	return dSc;
}

/*
double FormFactorLayer(double p_dQ, double p_dThick)
{
	double dSc, dRz;
	
	dRz = 0.5 * p_dThick;
	dSc = pow(FktA(p_dQ*dRz),2);

	return dSc;
}
*/

double FktA(double u)
{
	double A=1.0;

	if (u!=0)
		A = sin(u)/u;

	return A;
}

double FktB(double u)
{
	double B=1.0;

	if (u!=0)
		B = 3.0*(sin(u)-u*cos(u))/pow(u,3);

	return B;
}

double FktC(double u)
{
	double C=1.0;

	if (u!=0)
		C = 2.0*Bessel1(u)/u;

	return C;
}

double Bessel1(double x)
{
	double j1=0.0;

	if (x!=0)
		j1=sin(x)/(x*x) - cos(x)/x;

	return j1;
}
