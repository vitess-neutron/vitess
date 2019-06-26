/**********************************************************************************************/
/*  VITESS module sample_s_q                                                                  */
/* This program  simulates the coherent elastic diffraction of neutrons at a S(Q) sample.     */
/*                                                                                            */
/* The free non-commercial use of these routines is granted providing due credit is given to  */
/* the authors.                                                                               */
/*                                                                                            */
/* 1.0  Feb 2002  K. Lieutenant  initial version                                              */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                 */
/* 1.2  Feb 2004  K. Lieutenant  'FullParName', 'message' & 'ERROR' included; output extended */
/**********************************************************************************************/

#include <string.h>

#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "sq_calc.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/

double DelTheta=M_PI/2.0,     /* angles determining the detector coverage */
       Theta   =M_PI/2.0,   
       DelPhi  =M_PI,
       Phi     =M_PI,
      *g_pS=NULL,              /* table of structure factor data as a function of mom. transfer Q */
      *g_pQ=NULL;              /* corresponding table of momentum transfer data [1/Ang]           */
long   GenNeutrons =1,        /* how many neutrons to generate on the "cone"  */
       g_nLinesStr =0;        /* number of lines in the structure factor file */
char  *SampleFileName,       /* pointer to the name of the sample file           */
       g_sStrucFileName[200], /* structure factor file name     */
       g_cFunction = ' ';     /* parameter of the S(Q) function: F: analytical function
                                                                 D: data from file      */ 
double g_fMuCoh    =0.0,      /* coherent macroscopic scattering cross-section (= sigma_coh/UCV) [1/cm] */
       g_fMuInc    =0.0;      /* incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] */
long   g_bIncohScat=FALSE;    /* should incoherent scattering be done */

double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};

FILE   *g_pStrFacFile=NULL;



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

void   LoadStrucFactFile();
double ReadStructureFactor(double dQ, long index);
double CalcStructureFactor(double dQ);



/******************************/
/**   Program                **/
/******************************/

int main(int argc, char *argv[])
{
	SampleType Sample;      /* sample geometry */
	VectorType InISP[2];    /* neutron intersection before scattering */
	double     qValue,      /* absolute value of momentum transfer    */
	           dThetaMin,   /* minimal and maximal values of the           */
	           dThetaMax,   /* scattering angle according to Theta, DelTheta */
	           neutTheta,
				  neutPhi;
	double     DetFacCoh,   /* cares about the detector coverage      */
	           DetFacInc,   /* for coherent and incoherent scattering */
	           Lbf;         /* full path length of the neutron in the sample */
									/* with its initial direction */
	double     Ls;          /* distance of the neutron in the sample before sc. */
	long       j;           /* counting variable */
	VectorType SP;          /* position of scattering event */
	double     ScTheta,     /* angle of coherent Scattering */
	           ScProb,      /* scattering probability */
	           OutTheta,    /* Final angles of the neutron in the sample system */
				  OutPhi;
	double     RotMatrixSmpl[3][3], /* Rotation matrix that transforms a Vector to the */
	           RotMatrixNeut[3][3];
									/* sample coordinate system */
	long       i,           /* counting variable of the neutrons */
	           nisp,        /* number of intersection points to come */
	           NeutCount;

	/* get several things done before program starts  */
	/* which have actually nothing to do with physics */
	Init(argc, argv, VT_SMPL_S_Q);
	print_module_name("sample_s_q 1.2");
	OwnInit(argc, argv);

	/* Go and get the geometry of the sample and the scattering objects */
	InitSample(&Sample);
	GetSample (&Sample);
	switch (Sample.Type)
	{	case VT_CUBE: 
			fprintf(LogFilePtr, "Cubic sample, sizes: %7.2f,%7.2f,%7.2f   cm  (thickness, height, width)\n"
			                    "  direction        :(%8.3f,%7.3f,%7.3f)   \n",
			                    Sample.SG.Cube.thickness, Sample.SG.Cube.height, Sample.SG.Cube.width,
			                    Sample.Direction[0], Sample.Direction[1], Sample.Direction[2]);
			break;
		case VT_CYL: 
			fprintf(LogFilePtr, "Cylindrical sample : %7.2f cm radius%6.2f cm height\n"
			                    "  direction        :(%8.3f,%7.3f,%7.3f)   \n",
			                    Sample.SG.Cyl.r, Sample.SG.Cyl.height,
			                    Sample.Direction[0], Sample.Direction[1], Sample.Direction[2]);
			break;
		case VT_SPHERE: 
			fprintf(LogFilePtr, "Spherical sample   : %7.2f cm radius\n", 
			                    Sample.SG.Ball.r);
		break;
	}
	fprintf(LogFilePtr, "  position         :(%7.2f,%7.2f,%7.2f ) cm\n"
							  "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, coh scat; absorption)\n",
							  Sample.Position [0], Sample.Position [1], Sample.Position [2], 
							  g_fMuInc, g_fMuCoh, g_fMuAbs);
	if (g_cFunction=='F')
	{	fprintf(LogFilePtr, "Q-values calculated");
	}
	else
	{	/* Load structure factor file, if needed */
		fprintf(LogFilePtr, "Q-values from struct.factor file: %s\n", g_sStrucFileName);
		LoadStrucFactFile();
	}

	/* Factors that take care of the dectector coverage */
	DetFacCoh = DelPhi/M_PI*DelTheta;
	DetFacInc = DelPhi/M_PI*DelTheta;

	/* determine the rotation matrix to find new basis with the sample */
	/* vector pointing along the z-axis 				     */
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

				
				/************************************/
				/* First the coherent scattering    */
				/************************************/
				/* determine Theta and Phi of the neutrons direction */
				/* Theta should be small                             */
				NormVector          (InputNeutrons[i].Vector);
				CartesianToSpherical(InputNeutrons[i].Vector, &neutTheta, &neutPhi);

				/* Determine the rotation matrix to point the neutron along the +x axis */
				RotMatrixX(InputNeutrons[i].Vector,RotMatrixNeut);

				/* Determine min. and max. scattering angle (due to the detector coverage */
				dThetaMin = Theta-DelTheta;
				dThetaMax = Theta+DelTheta;

				CHECK

				// Choose the scattering angle and calculate Q-value 
				ScTheta = MonteCarlo(dThetaMin, dThetaMax);
				qValue  = 4.0*M_PI*sin(ScTheta/2.0)/InputNeutrons[i].Wavelength;

				// Determine the probability for this scattering angle
				ScProb  = sin(ScTheta);

				/* OutTheta is the angle of the scattered neutron with its */
				/* original flight path 				     */
				OutTheta=ScTheta;

				/* ScProb corresponds to the sample form factor considering hard sphere scattering */
				switch (g_cFunction)
				{
					case 'F': 
						ScProb *= CalcStructureFactor(qValue)*Lbf*g_fMuCoh / GenNeutrons;
						break;
					case 'D': 
						ScProb *= ReadStructureFactor(qValue,i)*Lbf*g_fMuCoh / GenNeutrons;
						break;
					default:
						ScProb = 0.0; 
				}

				/* Bring the neutron several times on the cone           */
				for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
				{
					/* OutPhi is the angle of the scattered neutron with the +y-axis */
					/* The expression for the focussin is not staight forward,       */
					/* rather lengthy (and probably buggy) it may take a while       */
					OutPhi = MonteCarlo(Phi-DelPhi, Phi+DelPhi);

					/* Ok, now everthing needed is known, put it together */
					ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, ScProb,
							              OutTheta, OutPhi, &Sample, RotMatrixNeut, RotMatrixSmpl);
				}
				
				/************************************/
				/* Second the incoherent scattering */
				/************************************/
				if (g_bIncohScat)
				{ 
					for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
					{
						/* Determine the scattering angle */
						OutPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
						OutTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

						/* Scattering probability */
						ScProb = Lbf*g_fMuInc * sin(OutTheta)/GenNeutrons;

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



/*********************************************************************/
/* Here we will set some global variables to get things going        */
/* If there is no sample specification the program is aborted        */
/* Known commandline parameters:                                     */
/*  -A     Neutron repetition rate on the cone    (default: 1)       */
/*  -R     Neutron repetition rate in the Q-range (default: 1)       */
/*  -D -d  theta-range [D-d, D+d]   (default: [0, pi])               */
/*  -p -P  phi-range   [P-p, P+p]   (default: [0,2*pi])              */
/*  -q -Q  Q-range [q, Q]                                            */
/*  -S     sample geometry file                                      */
/*********************************************************************/
void  OwnInit(int argc, char *argv[])
{
	
	long i;
	int  detectortest=0;
	
	/* Ok, scan all commandline parameters */
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


				/* read sample file name */
				case 'S':
					SampleFileName=&argv[i][2];
					break;

				default:
					fprintf(LogFilePtr,"\nERROR: unkown command option: %s\n", argv[i]);
					exit(-1);
			}
		}
	}

	/* Total macroscopic scattering cross-section */
	g_fMuTot = g_fMuCoh + g_fMuInc;

	/* Check, whether all 4 angles are given; if not, initial values are set again */	
	if( detectortest!=0 && detectortest!=15) 
	{
		Error("You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI ");
		Theta   = M_PI/2.0;
		DelTheta= M_PI/2.0;
		Phi     = M_PI;
		DelPhi  = M_PI;
	}

	/* Theta has to be in the range of [0;PI] */
	if((Theta-DelTheta < 0.0) || (Theta+DelTheta > M_PI)) 
		Error("Theta has to be in the range of [0;PI]");

	/* Phi has to be in the range of [0;2*PI] */
	if((Phi-DelPhi < 0.0) || (Phi+DelPhi > 2.0*M_PI)) 
		Error("Phi has to be in the range of [0;2*PI]");
}


/* own cleanup of this module */
/* -------------------------- */
void OwnCleanup()
{
	/* print error that might have occured many times */
	PrintMessage(SMPL_Q_RANGE_TOO_SMALL, g_sStrucFileName, ON);
	PrintMessage(SMPL_TRAJ_INSIDE, "", ON);

	fprintf(LogFilePtr," \n");

	/* free allocated memory */
	if (g_pQ!=NULL)  free(g_pQ);
	if (g_pS!=NULL)  free(g_pS);
}
/* End OwnCleanup */


/*********************************************************************/
/* 'GetSample'                                                       */
/* Read data of the sample (position, geometry, size, orientation)   */
/*********************************************************************/
void GetSample(SampleType *Sample)
{
	FILE *SampleFile;
	char Buffer[CHAR_BUF_LENGTH];

	/* open file */
	if((SampleFile=fopen(FullParName(SampleFileName),"rt"))==NULL) 
	{
		fprintf(LogFilePtr,"ERROR: Cannot open sample file %s\n", SampleFileName);
		exit(-1);
	}

	/* read data, if file could be opened */
	if(ReadTilComment(Buffer, SampleFile)) 
	{
		sscanf(Buffer, "%lf %lf %lf", &(Sample->Position[0]), 
		                              &(Sample->Position[1]), 
		                              &(Sample->Position[2]));
	} 
	else 
	{	fprintf(LogFilePtr, "ERROR: Can't read first line of %s",SampleFileName);
		exit(-1);
	}
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
		{	
			ReadBall(SampleFile, Sample);
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
	} 
	else 
	{
		fprintf(LogFilePtr, "ERROR: Can't read one of the lines 2 - 4 of %s",SampleFileName);
		exit(-1);
	}
	/* Read criterion for getting S(Q) data */
	if(ReadTilComment(Buffer, SampleFile))
	{	
		sscanf(Buffer, "%c", &g_cFunction);
		/*	Allowed char. for function parameter: 
			F: analytic function  D: function data from file   */
		if(g_cFunction!='F' && g_cFunction!='D')  
			Error("Wrong character for function to determine structure factor");
	}
	else 
	{	fprintf(LogFilePtr, "ERROR: Can't read criterion for getting S(Q) data %s", SampleFileName);
		exit(-1);
	}
	/* Read structure factor file */
	if(ReadTilComment(Buffer, SampleFile))
	{	
		sscanf(Buffer, "%s", g_sStrucFileName);
	}
	else 
	{	if (g_cFunction == 'D')
		{	fprintf(LogFilePtr, "ERROR: Can't read structure factor file name %s",SampleFileName);
			exit(-1);
		}
	}
	/* Read macroscopic cross sections */
	if(ReadTilComment(Buffer, SampleFile)) 
	{
		sscanf(Buffer,"%lf%lf%lf", &g_fMuInc, &g_fMuCoh, &g_fMuAbs);
	} 
	else 
	{	fprintf(LogFilePtr, "ERROR: Can't read cross sections %s",SampleFileName);
		exit(-1);
	}

	/* Seems as everything needed could be read */
	fclose(SampleFile);
}



/*********************************************************************/
/* 'CalcStructureFactor'                                             */
/* Calculate structure factor from an analytical function            */
/*                                                                   */
/*  CALCULATION OF A STRUCTURE FACTOR WITH THE                       */
/*      PERCUS-YEVICK HARD SPHERE MODEL                              */
/*                                                                   */
/*********************************************************************/
double CalcStructureFactor(double p_dQ)
{
	double dSc, dSigma, dRho;
	long   nInt;

	nInt   =   10;
	dSigma =    2.29;
	dRho   =    0.75;
	
	dSc = pyshm(nInt, p_dQ, dSigma, dRho);;

	return dSc;
}



/*********************************************************************/
/* 'ReadStructureFactor'                                             */
/* Get structure factor from the structure factor file               */
/*********************************************************************/
double ReadStructureFactor(double p_dQ, long index)
{
	long   n=-1;
	double dS=0.0, dSN, dSN1;
	
	while (n+1 < g_nLinesStr  &&  g_pQ[n+1] < p_dQ) 
	{	n++;
	}

	if (n >= 0 && n+1 < g_nLinesStr)
	{	/* linear  extrapolation */
		dSN  = g_pS[n];
		dSN1 = g_pS[n+1];
		dS   = dSN + (dSN1-dSN ) / (g_pQ[n+1]-g_pQ[n]) * (p_dQ-g_pQ[n]);
	}
	else
	{	/* read error: Q-value smaller or larger than all values in the distribution file */
		CountMessageID(SMPL_Q_RANGE_TOO_SMALL, InputNeutrons[index].ID);
	}

	return dS;
}



/*********************************************************************/
/* 'LoadStrucFacFile'                                                */
/* load structure factor file                                        */
/*********************************************************************/
void LoadStrucFactFile()
{
	char sBuffer[CHAR_BUF_LENGTH]; 

	/* If there is a structure factor file go and load the file */
	if(g_sStrucFileName!=NULL) 
	{
		/* opens distribution file */
		if((g_pStrFacFile=fopen(FullParName(g_sStrucFileName),"rt"))!=NULL) 
		{
			long   n;

			/* reads number of lines, allocates memory and then reads data */
			g_nLinesStr = LinesInFile(g_pStrFacFile);
			g_pQ = calloc(g_nLinesStr, sizeof(double));
			g_pS = calloc(g_nLinesStr, sizeof(double));

			for(n=0; n<g_nLinesStr; n++)
			{  
				ReadLine(g_pStrFacFile, sBuffer, CHAR_BUF_LENGTH);
				sscanf  (sBuffer, "%lf %lf", &g_pQ[n], &g_pS[n]);
			}

			/* closes trace file */
			fclose(g_pStrFacFile) ;
		} 
		else 
		{	fprintf(LogFilePtr, "\nERROR: Can't open %s to read structure factor file\n", g_sStrucFileName);
			exit (-1);
		}
	}
}

