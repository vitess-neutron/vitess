/*********************************************************************************************/
/*  VITESS module  SAMPLE_ENVIRONMENT                                                        */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Nov  2008  K. Lieutenant   initial version                                           */
/* 1.1  Oct  2012  K. Lieutenant   only 1 Bragg reflection                                   */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "intersection.h"
#include "message.h"
#include "matrix.h"


/******************************/
/**   Global Variables       **/
/******************************/

char  *SampleFileName;      // parameter file name (located in argv)
short  nColor  = 0;         // colour of the neutrons scattered from the environment
double DelTheta ,           // 
       Theta    ,           // solid angles into which scattering takes place        
       DelPhi   ,
       Phi      , 
       MuInc    = 0.0,      // incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm]
       UCV      =50.0,      // UCV 
       Diameter =10.0,      // outer diameter of the sample environment  (= 2*R_out)
       Thickness= 0.0;      // thickness of the sample environment  (= R_out - R_in)
VtDir  eDirEnv;             // direction for sample environment: 1:'in' or 2:'out' 
SampleType stEnvironment;   // properties of the sample environment    


/******************************/
/**   Extern Variables       **/
/******************************/

extern double g_fMuTot, g_fMuAbs; /* macrosc. scattering and absorption cross section, defined in 'sample.c' */
extern VtDir  g_eDir;             /* direction for sample environment: 'in' or 'out',  defined in 'sample.c' */



/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit          (SampleType* pEnvironment, int argc, char* argv[]);
void  OwnCleanup       (DoublePair* pStrucFac);
short ReadEnvironPar   (SampleType* pEnvironment, char* pStrucFileName);
long  ReadStrucFact    (char* StrFileName, DoublePair* pStrucFac[]);
long  ChooseBraggReflex(long* pNrefl, long Nstrcfac, DoublePair* pStrucFac[], double Wavelength);


/******************************/
/**   Program                **/
/******************************/

int main(int argc, char **argv)
{
  int        i,j;
  long       nisp,                /* number of intersection points to come               */
	           Nth,                 /* index of the structure factor                       */
             Nrefl;               /* number of reflections for the given wavelength      */
  double     Lbf;                 /* full path length of the neutron in the sample       */
  double     Ls;                  /* distance of the neutron in the sample before scat.  */
  long       NumStrucFac;         /* number of reflections in the structure factor file  */
  char       sStrucFileName[99];  /* structure factor file name                          */
  double     DetFacCoh,           /* cares about the detector coverage                   */
	           DetFacInc,           /*  for coherent and incoherent scattering             */
	           HelpFac;             /* contains k independent term of the scattering       */
  VectorType InISP[2],            /* neutron intersection with sample without scattering */
	           SP;                  /* position of scattering event                        */
  DoublePair *pStrucFac=NULL;
  Neutron    OutNeutron;
  double     ScProbCoh, ScProbF,  /* probability for coherent, incoh. scattering and transmission */
	           ScProbInc, ScProbT,
	           ScTheta,             /* scattering angles (in neutron coordinate system) */
	           ScPhi;
  double     RotMatrixSmpl[3][3]; /* Rotation matrix that transforms a Vector to the */
  double     RotMatrixNeut[3][3];

	/* Initialize the program according to the parameters given   */
	Init(argc, argv, VT_SMPL_ENVIRON);
	print_module_name("sample_environment 1.1");
  bVisInstalled = TRUE;

	/* Module specific initialization and reading of sample geometry and name of the structure factor file */
	InitSample    (&stEnvironment);
	OwnInit       (&stEnvironment, argc, argv);
	ReadEnvironPar(&stEnvironment, sStrucFileName);

	/* Write parameters to log file */
	switch (stEnvironment.Type)
	{ 
		case VT_HOL_CYL: 
			fprintf(LogFilePtr, "Vertical hollow cylinder around sample: \n"
			                    " radii, out and in : %7.2f, %7.2f cm \n height out and in : %7.2f, %7.2f cm\n",
			                    stEnvironment.SG.HCyl.r_out, stEnvironment.SG.HCyl.r_in, stEnvironment.SG.HCyl.h_out, stEnvironment.SG.HCyl.h_in);
			break;
	  default:;
			Error("Sample environment can only have the shape of a vertical hollow cylinder");
	}
	fprintf(LogFilePtr, " position          :(%7.2f,%7.2f,%7.2f ) cm\n"
							  "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, total scat; absorption)\n"
							  "unit cell volume   : %8.3f Ang³\n"
							  "struct. factor file: %s\n", 
							  stEnvironment.Position[0], stEnvironment.Position[1], stEnvironment.Position[2], 
							  MuInc, g_fMuTot, g_fMuAbs, UCV, sStrucFileName);

	/* Now get the nuclear unit-cell structure factors |f_N(t)|^2.       */
	/* The memory needed will be allocated inside 'GetStructureFactor()'.*/
	NumStrucFac = ReadStrucFact(sStrucFileName, &pStrucFac);

	/* Factors that take care of the detector coverage */
	DetFacCoh = DelPhi/M_PI;
	DetFacInc = DelPhi/M_PI*DelTheta;

	/* determine the rotation matrix to find new basis with the sample */
	/* vector pointing along the z-axis 				     */
	RotMatrixX(stEnvironment.Direction, RotMatrixSmpl);  // RotMatrixSmpl = OneMatrix 

	/* Get the neutrons from the file */
	DECLARE_ABORT;

	while((ReadNeutrons())!= 0)
	{
		for(i=0; i<NumNeutGot; i++) 
		{
			CHECK;

			/* First, shift the origin of the system to the middle of the sample   */
			SubVector(InputNeutrons[i].Position, stEnvironment.Position);
			OutNeutron=InputNeutrons[i];

			/* Do anything to be done for the Scattering */
			if (NeutronIntersectsSample(&(OutNeutron), &stEnvironment, RotMatrixSmpl, InISP, &nisp, eDirEnv))
			{
				if (eDirEnv==VT_IN && nisp < 2)
					CountMessageID(ENV_TRAJ_INSIDE, OutNeutron.ID);
				else if (eDirEnv==VT_INSIDE && nisp < 1)
					CountMessageID(ENV_TRAJ_OUTSIDE, OutNeutron.ID);

				/* the neutron may be scattered between InISP[0] and InISP[1] */
				/* Lfb full path length in the sample before scattering       */
				Lbf = DistVector(InISP[0], InISP[1]);

				/* MONTE CARLO CHOICE: Where is the neutron scattered         */
				/* Distance Ls between entrance of the neutron InISP[0] and   */
				/* the scattering point SP 				      */
				Ls = MonteCarlo(0, Lbf);

				/* which is the corresponding scattering point  	   */
				/*   SP = InISP[0] + Ls * OutNeutron.Vector		      */
				for(j=0; j<3; j++)
					SP[j] = InISP[0][j] + Ls*OutNeutron.Vector[j];

				/* Determine the rotation matrix to point the neutron along the +x axis   */
				NormVector(OutNeutron.Vector);
				RotMatrixX(OutNeutron.Vector, RotMatrixNeut);


				/**********************************************/
				/* Now the actual scattering                  */
				/*   First the coherent scattering            */
				/**********************************************/
				/* Helpfac contains the non direction dependent term                         */
				/* G.L. Squires, "Introduction to the theory of thermal neutron scattering", */
				/* (1978), equation (3.103)  (UCV is the unit cell volume)                   */
				HelpFac = Lbf*pow(OutNeutron.Wavelength,3)/(4.0*UCV*UCV);

				OutNeutron.Color = nColor;
        if (eDirEnv==VT_IN)  OutNeutron.ID.IDGrp[0]++;
        if (eDirEnv==VT_OUT) OutNeutron.ID.IDGrp[1]++;

        /* choose one suitable |F(k)| for Bragg scattering */
        Nth=ChooseBraggReflex(&Nrefl, NumStrucFac, &pStrucFac, OutNeutron.Wavelength);

        /* ScTheta is the angle of the scattered neutron with its original flight path */
        ScTheta = 2.0*asin(OutNeutron.Wavelength/(2.0*pStrucFac[Nth][0]));

        /* Only trajectories between Theta-DelTheta and Theta+DelTheta are regarded. 
           The deviation from straight direction (neutTheta) of the incoming neutrons
           is supposed to be neglectible                            */
        if (ScTheta > Theta-DelTheta && ScTheta < Theta+DelTheta) 
        {
          /* ScProb is the scattering-cross section (Squires 3.103) */
          /*  devided by the sample area                            */
          /* as I_sc = sigma * flux = sigma / area * current        */
          /* it contains the d-spacing dependent terms              */
          /*  and the d-spacing independent HelpFac terms s.o.      */
          ScProbF =  Nrefl * HelpFac / sin(0.5*ScTheta) * pStrucFac[Nth][1];

          /* ScPhi is the angle of the scattered neutron with the +y-axis */
          /*  of the neutron co-ordinate system                           */
          ScPhi = MonteCarlo(Phi-DelPhi,Phi+DelPhi);

          /* Ok, now everthing needed is known, put it together */
          ProcessNeutronToEnd(&(OutNeutron), SP, Ls, DetFacCoh, ScProbF,
							                ScTheta, ScPhi, &stEnvironment, RotMatrixNeut, RotMatrixSmpl);
        } 

        /* determine the total scattering cross-section */
				ScProbCoh = 0.0;
        for (Nth=0; Nth < Nrefl; Nth++)
          ScProbCoh += (HelpFac / (OutNeutron.Wavelength/(2.0*pStrucFac[Nth][0])) * pStrucFac[Nth][1]);

				/************************************/
				/*  Then the incoherent scattering  */
				/************************************/
				/* Determine the scattering angle and the probability */
				OutNeutron.Color = (short)(nColor+1);
        if (eDirEnv==VT_IN)  OutNeutron.ID.IDGrp[0]++;
        if (eDirEnv==VT_OUT) OutNeutron.ID.IDGrp[1]++;
     
				ScPhi     = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
				ScTheta   = MonteCarlo(Theta-DelTheta,Theta+DelTheta);
				ScProbInc = Lbf*MuInc * sin(ScTheta);

				ProcessNeutronToEnd(&OutNeutron, SP, Ls, DetFacInc, ScProbInc,
				                    ScTheta, ScPhi, &stEnvironment, RotMatrixNeut,  RotMatrixSmpl);

				/******************************/
				/* The transmitted neutrons   */
				/******************************/
				// move the neutron a little bit into the sample environment 
				// to avoid problems with wrong sign
				for(j=0; j<3; j++)
					SP[j] = InISP[0][j] + 1.0E-08*OutNeutron.Vector[j];

				/* Keep the flight direction and calculate the transmission probability */
				ScTheta = 0.0;
				ScPhi   = 0.0;
				ScProbT = Max(0.0, 1.0 - ScProbCoh - ScProbInc);
				if (ScProbT <= 0.0)
					CountMessageID(ALL_NEGATIVE_INT, OutNeutron.ID);

				ProcessNeutronToEnd(&InputNeutrons[i], SP, 1.0E-08, 1.0, ScProbT,
				                    ScTheta, ScPhi, &stEnvironment, RotMatrixNeut,  RotMatrixSmpl);
			
			} // end 'NeutronIntersect...         
			else
			{	if (eDirEnv==VT_OUT)
					WriteNeutron(&OutNeutron);			
			}
		}
	}

	/* Do module specific cleanups */
  my_exit:
	OwnCleanup(pStrucFac);

	/* Do the general cleanup */
	stPicture.eType = (short) stEnvironment.Type;
	Cleanup(stEnvironment.Position[0],stEnvironment.Position[1],stEnvironment.Position[2], 0.0,0.0);

	return 0;
}


/* Init of this module */
/* ------------------- */
void  OwnInit(SampleType* pEnvironment, int argc, char *argv[]) 
{
	int   i /*, detectortest=0 */;

	/* Theta has to be in the range of [0;PI] */
	/* Phi has to be in the range of [0;2*PI] */
	DelTheta = M_PI/2.0; // solid angles covered by 
	Theta    = M_PI/2.0; //    the detectors	     
	DelPhi   = M_PI;
	Phi      = M_PI;

	for(i=1; i<argc; i++) 
	{ if(argv[i][0]!='+') 
		{	
			switch(argv[i][1])
			{	
				case 'F':
				  SampleFileName = &argv[i][2];
				  break;

				case 'x':
				  pEnvironment->Position[0] = atof(&argv[i][2]);
				  break;
				case 'y':
				  pEnvironment->Position[1] = atof(&argv[i][2]);
				  break;
				case 'z':
				  pEnvironment->Position[2] = atof(&argv[i][2]);
				  break;

				case 'c':
				  nColor = (short) atoi(&argv[i][2]);
				  break;
				case 'r':
				  eDirEnv = (VtDir) atoi(&argv[i][2]);   // 1:in   2: out
				  break;

			/*	case 'D':
				  Theta = M_PI/180.0 * atof(&argv[i][2]);
				  detectortest &= 1000L;
				  if (Theta < 0.0 || Theta > M_PI)
					 Error("Theta has to be in the range of [0;PI] ");
				  break;
				case 'd':
				  DelTheta = M_PI/180.0 * atof(&argv[i][2]);
				  detectortest &= 0100L;
				  break;
				case 'P':
				  Phi = M_PI/180.0 * atof(&argv[i][2]);
				  detectortest &= 0010L;
				  if (Phi < 0.0 || Phi > 2.0*M_PI)
					 Error("Phi has to be in the range of [0;2*PI] ");
				  break;
				case 'p':
				  DelPhi = M_PI/180.0 * atof(&argv[i][2]);
				  detectortest &= 0001L;
				  break; */

				default:
				  fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
				  exit(-1);
			}
		}
	}

	if (SampleFileName == NULL)
		Error("The option -F to give the ascii file name is mandatory!");

	/* if( (detectortest != 0) && (detectortest != 15))
	{	Warning("You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI");
		Theta    = M_PI/2.0;
		DelTheta = M_PI/2.0;
		Phi      = M_PI;
		DelPhi   = M_PI;
	} */
}


/* Cleanup of this module */
/* ---------------------- */
void OwnCleanup(DoublePair *pStrucFac)
{
	/* print error that might have occured many times */
	PrintMessage(ENV_TRAJ_INSIDE,  "", ON);
	PrintMessage(ENV_TRAJ_OUTSIDE, "", ON);
	PrintMessage(ALL_NEGATIVE_INT, "", ON);
	fprintf(LogFilePtr, "\n");

  /* Geometry data */
  if (bVisInstr && eDirEnv==VT_OUT)
  { 
	  /* stGeometry.pHolCyl  = (VtHolCyl*) calloc(1, sizeof(VtHolCyl));
	  stGeometry.nHolCyls = 1; 
	      
	  stGeometry.pHolCyl[0].Radius     = stEnvironment.SG.HCyl.r_out; 
	  stGeometry.pHolCyl[0].InnerRadius= stEnvironment.SG.HCyl.r_in / stEnvironment.SG.HCyl.r_out;
	  stGeometry.pHolCyl[0].Length     = stEnvironment.SG.HCyl.h_out;
	  stGeometry.pHolCyl[0].vCntr[0]   = 0.0;
	  stGeometry.pHolCyl[0].vCntr[1]   = 0.0;
	  stGeometry.pHolCyl[0].vCntr[2]   = 0.0;
	  stGeometry.pHolCyl[0].vSymAxis[0]= 0.0;
	  stGeometry.pHolCyl[0].vSymAxis[1]= 0.0;
	  stGeometry.pHolCyl[0].vSymAxis[2]= 1.0;
	  stGeometry.pHolCyl  = (VtHolCyl*) calloc(1, sizeof(VtHolCyl));
	  stGeometry.nHolCyls = 1; */
	      
	  stGeometry.pCylinder  = (VtCylinder*) calloc(1, sizeof(VtCylinder));
	  stGeometry.nCylinders = 1; 
	      
	  stGeometry.pCylinder[0].Radius     = stEnvironment.SG.HCyl.r_out; 
	  stGeometry.pCylinder[0].Length     = stEnvironment.SG.HCyl.h_out;
	  stGeometry.pCylinder[0].vCntr[0]   = 0.0;
	  stGeometry.pCylinder[0].vCntr[1]   = 0.0;
	  stGeometry.pCylinder[0].vCntr[2]   = 0.0;
	  stGeometry.pCylinder[0].vSymAxis[0]= 0.0;
	  stGeometry.pCylinder[0].vSymAxis[1]= 0.0;
	  stGeometry.pCylinder[0].vSymAxis[2]= 1.0;

	  stGeometry.pDescr  = "sample_environment:cyan";
	  stGeometry.eModule = VT_SMPL_ENVIRON;
  }

	/* Release the allocated memory */
	if (pStrucFac!=NULL)
		free(pStrucFac);
}


short ReadEnvironPar(SampleType* pEnvironment, char* pStrucFileName)
{
	FILE  *pFile;
	char   sLine[256];
	int    nLen=sizeof(sLine)-1;

	pEnvironment->Type = VT_HOL_CYL;
	pEnvironment->Direction[0] = 0.0;
	pEnvironment->Direction[1] = 0.0;
	pEnvironment->Direction[2] = 1.0;

	if((pFile=fopen(FullParName(SampleFileName),"rt")) != NULL)
	{
		// ReadLine(pFile, sLine, nLen); sscanf(sLine, "%lf %lf %lf", &pEnvironment->Position[0], &pEnvironment->Position[1], &pEnvironment->Position[2]); 
		ReadLine(pFile, sLine, nLen); sscanf(sLine, "%lf %lf %lf", &Thickness, &Diameter, &pEnvironment->SG.HCyl.h_out);
		ReadLine(pFile, sLine, nLen); sscanf(sLine, "%s",          pStrucFileName); 
		ReadLine(pFile, sLine, nLen); sscanf(sLine, "%lf %lf %lf", &MuInc, &g_fMuTot, &g_fMuAbs); 
		ReadLine(pFile, sLine, nLen); sscanf(sLine, "%lf",         &UCV);

		pEnvironment->SG.HCyl.r_out = 0.5 * Diameter; 
		pEnvironment->SG.HCyl.r_in  = pEnvironment->SG.HCyl.r_out - Thickness; 
		pEnvironment->SG.HCyl.h_in  = pEnvironment->SG.HCyl.h_out - 2.0*Thickness; 

		fclose(pFile);
		return(TRUE);
	}
	else
	{	fprintf(LogFilePtr,"ERROR: Cannot open sample file %s\n", SampleFileName);
		return(FALSE);
	}
}


long ReadStrucFact(char* sStrFileName, DoublePair* pStrucFac[])
{
	FILE  *pStrucFile;
	long  NumLines, i, j;
	char  sBuffer[CHAR_BUF_LENGTH];

	/* first open the file, add path if missing */
	pStrucFile=fopen(FullParName(sStrFileName),"rt"); 
	if(pStrucFile==NULL)
	{ fprintf(LogFilePtr,"ERROR: Can't read the structure factor data from %s\n", sStrFileName);
		exit(-1);
	}

	/* count lines in file */
	NumLines = LinesInFile(pStrucFile);

	/* get memory for StrucFac */
	if((*pStrucFac = (DoublePair *)calloc(NumLines, sizeof(DoublePair)))==NULL)
	{ fprintf(LogFilePtr,"ERROR: Can't allocate memory for structure factor data\n");
		exit(-1);
	}

	/* get back to the start of the File */
	rewind(pStrucFile);

	/* and read the data */
	for(i=0; i < NumLines; i++)
	{	
		ReadLine(pStrucFile, sBuffer, sizeof(sBuffer)-1); 
		sscanf  (sBuffer, "%lf %lf", &((*pStrucFac)[i][0]), &((*pStrucFac)[i][1]));
	}
	qsort((void *)*pStrucFac, (size_t) NumLines, sizeof(DoublePair), CompPair);
	fclose(pStrucFile);

	/* Sum up all equal d-spacings */
	i=0;
	for(j=1; j<NumLines; j++)
	{	if((*pStrucFac)[j][0]!=(*pStrucFac)[j-1][0])
		{ i++;
			(*pStrucFac)[i][0]=(*pStrucFac)[j][0];
			(*pStrucFac)[i][1]=(*pStrucFac)[j][1];
		} 
		else
		{ (*pStrucFac)[i][1]+= (*pStrucFac)[j][1];
		}
	}
	NumLines = i+1;

	return NumLines;
}

/*********************************************************/
// *pNrefl       number of reflections for the given wavelength
//  Nstrcfac     number of reflections in the structure factor file
//  pStrucFac[]  list of structure factors
//  Wavelength   wavelength of the neutron
/*********************************************************/
long  ChooseBraggReflex(long* pNrefl, long Nstrcfac, DoublePair* pStrucFac[], double Wavelength)
{
  long Irefl,    // chosen index of reflection
       Ifac;     // Index of reflection

  // calculate number of possible reflections
  *pNrefl = 0;
  for (Ifac=0; Ifac < Nstrcfac; Ifac++)
  { 
    if ((*pStrucFac)[Ifac][0] > 0.5*Wavelength)
      (*pNrefl)++;
  }

  // choose one of these reflections
  Irefl = (long) floor(MonteCarlo(0.0, (double) *pNrefl));

  return Irefl;
}





    