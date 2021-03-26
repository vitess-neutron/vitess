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
/* 1.9  Oct 2013  K. Lieutenant  only theta_max variable                                      */
/* 1.10 Apr 2020  K. Lieutenant  new central visualization parameters                         */
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
char  *SampleFileName;       // -S    pointer to the name of the sample file 
short  bIncScat=FALSE;       // -I    should incoherent scattering be done 
long   GenNeutrons =1;       // -A    repetitions (how many trajectories to generate per incoming trajectory)
double ThetaMax = 0.0,       // -M    maximum scattering angle to be considered
       Theta    = M_PI/2.0,  // calc  these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,  // calc     Theta has to be in the range of [0;PI]         
       Phi      = M_PI,      // fix      Phi has to be in the range of [0;2*PI] 
       DelPhi   = M_PI;      // fix
char   cGeometry = ' ';      /* file  geometry parameter: 
                                      S: spheres,        R  = SizeA
                                      D: size dstr. sph. Rmin=SizeA, Rmax=SizeB
                                      E: ellipsoids      Rx = SizeA, Ry = SizeB, Rz = SizeC
                                      C: cylinders,      Rx = SizeA, Ry = SizeB, L  = SizeC
                                      P: parallelepiped, a  = SizeA, b  = SizeB, c  = SizeC
                                      I: no scattering objects, isotropic scattering */ 
double SizeA   = -1.0, 
       SizeB   = -1.0, 
       SizeC   = -1.0,       // file  size of the particles [Angstr.] e.g. hard spere radius in x-, y-, and z-direction 
       rho1    =  1.0e10,    // file  scattering length density of the particles 
       rho2    =  1.0e10,    // file  scattering length density of the solvemt 
       FracPtcl=  0.01;      // file  volume fraction of the particles 
extern 
double MuTot,                // file  macrosc. scattering cross section, defined in 'sample.c'
       MuAbs;                // file  macrosc. absorption cross section, defined in 'sample.c'
double MuInc   = 0.0;        // file  incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] 
SampleType stSample;         // file  sample geometry 

double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void   OwnInit   (int argc, char *argv[]);   // reads input parameters and sets global variables
void   OwnCleanup();                         // does module specific cleanup
void   GetSample (SampleType *pSample);      // reads sample parameters from file
void   WritePar();                           // writes input parameters to log file
void   SetGeometry(char* sColor);            // fills the structure stGeometry for visualization    missing

// Functions to calculate form factor         
double FormFactorSphere   (double dQ,  double dR);
double FormFactorEllipsoid(double dQx, double dQy,    double dQz, 
                           double dRx, double dRy,    double dRz);
double FormFactorCylinder (double dQx, double dQy,    double dQz, 
                           double dRx, double dRy,    double dHeight);
double FormFactorEpiped   (double dQx, double dQy,    double dQz, 
                           double dLen,double dWidth, double dHeight);
double FormFactorLayer    (double dQ,  double dThick);

// Help functions to calculate form factor         
double FktA   (double u);
double FktB   (double u);
double FktC   (double u);
double Bessel1(double x);


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char *argv[])
{
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

  // initialisation
  // --------------
  _eModule = MCN_SMPL_SANS;
  
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.10");
  OwnInit(argc, argv);

  /* Go and get the geometry of the sample and the scattering objects */
  InitSample(&stSample);
  GetSample (&stSample);
  WritePar();

  /* Factor that takes care of the dectector coverage */
  DetFacInc = DelPhi/M_PI*DelTheta;
  DetFacCoh = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find a new basis */
  /* with the sample vector pointing along the z-axis  */
  RotMatrixX(stSample.Direction, RotMatrixSmpl);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /* First, shift the origin of the system to the center of the sample */
        SubVector(InputNeutrons[i].Position, stSample.Position);

        /* Test if the Neutron hits the Sample */
        if (NeutronIntersectsSample(&(InputNeutrons[i]), &stSample, RotMatrixSmpl, InISP, &nisp, VT_IN))
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

          /* determine Theta and Phi of the neutrons direction */
          /* Theta should be small */
          NormVector          (InputNeutrons[i].Vector);
          CartesianToSpherical(InputNeutrons[i].Vector, &neutTheta, &neutPhi);

          /* Determine the rotation matrix to point the neutron along */
          /* the +x axis				      		    */
          RotMatrixX(InputNeutrons[i].Vector,RotMatrixNeut);

          //   First the coherent scattering           
          //--------------------------------
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
            {	
              dQ[j] = MonteCarlo(-1.0,1.0);
            }
            fFac = sqrt(qValue*qValue/(dQ[0]*dQ[0] + dQ[1]*dQ[1] + dQ[2]*dQ[2]));
            for (j=0; j<=2; j++)
            {	
              dQ[j] *= fFac;
            }

            /* OutTheta is the angle of the scattered neutron with its original flight path */
            OutTheta=ScTheta;

            /* OutPhi is the angle of the scattered neutron with the +y-axis */
            OutPhi = MonteCarlo(Phi-DelPhi, Phi+DelPhi);

            /* ScProb corresponds to the sample form factor considering hard sphere scattering */
            switch (cGeometry)
            {
              case 'S': 
                fFormFac = FormFactorSphere(qValue, SizeA);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(SizeA,3);
                break;
              case 'D': 
                Radius   = MonteCarlo(SizeA, SizeB);
                fFormFac = FormFactorSphere(qValue, Radius);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(Radius,3);
                break;
              case 'E': 
                fFormFac = FormFactorEllipsoid(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * SizeA*SizeB*SizeC;
                break;
              case 'C': 
                fFormFac = FormFactorCylinder(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * M_PI * SizeA*SizeB * SizeC;
                break;
              case 'P': 
                fFormFac = FormFactorEpiped(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * SizeA*SizeB*SizeC;
                break;
              default:
                fFormFac = 1.0; 
                break;
            }

            // Determine the scattering probability from the form factor, 
            // contrast and particle size, the sample size, and the solid angle factor,
            if (cGeometry=='I')
            {	
              fFacCtrPtkl = 1.0;
            }
            else
            {	
              fFacCtrPtkl = FracPtcl* pow((rho1-rho2),2) * fVolPtkl;
            }

            /* Scattering probability */
            ScProb = 4*M_PI * Lbf * fFormFac * fFacCtrPtkl * sin(ScTheta) / GenNeutrons;

            /* Ok, now everthing needed is known, put it together */
            if (ScProb > 0.0 && DetFacCoh > 0.0)
              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, ScProb, OutTheta, OutPhi, &stSample, RotMatrixNeut, RotMatrixSmpl);
          }
				
          // Second the incoherent scattering 
          //--------------------------------
          if (bIncScat && MuInc > 0.0)
          { 
            for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
            {
              /* Determine the scattering angle */
              OutPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
              OutTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

              /* Scattering probability */
              ScProb  = Lbf*MuInc * sin(OutTheta) / GenNeutrons;

              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, ScProb,
              OutTheta, OutPhi, &stSample, OneMatrix,  RotMatrixSmpl);
            }
          }
        }
      }
    }
  }

  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
  my_exit:

  /* Write parameters to log file */
  WritePar();

  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  Cleanup(stSample.Position[0],stSample.Position[1],stSample.Position[2], 0.0,0.0);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  /*********************************************************************/
  /* Here we will set some global variables to get things going        */
  /* If there is no sample specification the program is aborted        */
  /* Known command line parameters:                                    */
  /*  -S     sample geometry file                                      */
  /*  -I     Flag: Incoherent scattering            (default: FALSE)   */
  /*  -A     Neutron repetition rate on the cone    (default: 1)       */
  /*  -M     Max. Q-value                                              */
  /*********************************************************************/
	
  long i;
	
  /* Ok, scan all command line parameters */
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        /* Consider incoherent scattering or not */
        case 'I':
          if(argv[i][2]=='1') bIncScat=TRUE;
          break;

        /* Repetition rate and multiplicity */
        case 'A':
          sscanf(&(argv[i][2]),"%ld", &GenNeutrons);
          break;

        /* get the solid angle covered by the detector */
        case 'M':
          sscanf(&(argv[i][2]),"%lf", &ThetaMax);
          Theta   =0.5*ThetaMax*M_PI/180.0;
          DelTheta=Theta;
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

  /* Theta has to be in the range of [0;PI] */
  if(Theta+DelTheta > M_PI) 
  Error("Theta has to be in the range of [0;PI]");
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  /* print error that might have occured many times */
  PrintMessage(SMPL_TRAJ_INSIDE, "", ON);
  fprintf(LogFilePtr, "\n");
}


/*******************************************************/
/** writes input parameters to log file               **/
/*******************************************************/
void  WritePar()
{
  switch (stSample.Type)
  {
    case VT_CUBE: 
      fprintf(LogFilePtr, "Cubic sample, sizes : %8.2f,%8.2f,%8.2f   cm  (thickness, height, width)\n"
                          "  direction         :(%9.3f,%8.3f,%8.3f)   \n",
                          stSample.SG.Cube.thickness, stSample.SG.Cube.height, stSample.SG.Cube.width,
                          stSample.Direction[0], stSample.Direction[1], stSample.Direction[2]);
      break;
    case VT_CYL: 
      fprintf(LogFilePtr, "Cylindrical sample  : %8.2f cm radius%6.2f cm height\n"
                          "  direction         :(%9.3f,%8.3f,%8.3f)   \n",
                          stSample.SG.Cyl.r, stSample.SG.Cyl.height,
                          stSample.Direction[0], stSample.Direction[1], stSample.Direction[2]);
      break;
    case VT_SPHERE: 
      fprintf(LogFilePtr, "Spherical sample    : %8.2f cm radius\n", stSample.SG.Ball.r);
      break;
    default: ;
  }
  fprintf(LogFilePtr, "  position          :(%8.2f,%8.2f,%8.2f ) cm\n",
                      stSample.Position [0], stSample.Position [1], stSample.Position [2]);
  switch (cGeometry)
  {	
    case 'S': 
      fprintf(LogFilePtr, "Spherical particles : %8.2f Ang radius\n", SizeA); 
      break;
    case 'D': 
      fprintf(LogFilePtr, "Spherical particles from %8.2f to %8.2f Ang radius\n", SizeA, SizeB); 
      break;
    case 'E': 
      fprintf(LogFilePtr, "Elliptic particles  : radii %8.2f,%8.2f,%8.2f Ang\n",  SizeA, SizeB, SizeC); 
      break;
    case 'P': 
      fprintf(LogFilePtr, "Cubic particles     : %8.2f,%8.2f,%8.2f Ang length\n", SizeA, SizeB, SizeC); 
      break;
    case 'C': 
      fprintf(LogFilePtr, "Cylindric particles : radii %8.2f,%8.2f, length:%8.2f Ang\n", SizeA, SizeB, SizeC); 
      break;
    case 'I': 
      fprintf(LogFilePtr, "Particles scattering isotropically\n"); 
      break;
  }
  fprintf(LogFilePtr, "scat. length density: %13.3e (particle) %10.3e 1/cm^2 (solvent)\n"
                      "vol.fract. of part. : %8.3f\n"
                      "macr. cross section : %10.5f,%10.5f;%10.5f  1/cm (incoh, total scat; absorption)\n",
                      rho1, rho2, FracPtcl, MuInc, MuTot, MuAbs);
}


/*********************************************************************/
/* Reads data of the sample (position, geometry, size, orientation)   */
/*********************************************************************/
void GetSample(SampleType* pSample)
{
  FILE* pSampleFile;
  char Buffer[CHAR_BUF_LENGTH];

  pSampleFile = OpenInputFile2(SampleFileName, "sample data", "rt");

  /* Lets get started read first line */
  if (ReadTilComment(Buffer, pSampleFile)) 
  {
    /* got first line, lets see what in there 	*/
    sscanf(Buffer, "%lf %lf %lf", &(pSample->Position[0]), &(pSample->Position[1]), &(pSample->Position[2]));
		
    /* Next line should decribe the type of geometry */
    /* cylinder, cube, ball			     */
    if (ReadTilComment(Buffer, pSampleFile)) 
    {
      if(strstr(Buffer, "cyl")!=NULL) 
      {
        ReadCylinder(pSampleFile, pSample);
        pSample->Type=VT_CYL;
      } 
      else if(strstr(Buffer, "cub")!=NULL) 
      {
        ReadCube(pSampleFile, pSample);
        pSample->Type=VT_CUBE;
      } 
      else if(strstr(Buffer, "bal")!=NULL) 
      {
        ReadBall(pSampleFile, pSample);
        pSample->Type=VT_SPHERE;
      }
      else 
      {	
        fprintf(LogFilePtr, "ERROR: Please denote the sample geometry by cyl, cub or bal on the second line of %s\n", SampleFileName);
        exit(-1);
      }

      /* the direction vector should have a positive z component  */
      /* this will make things easier with the rotations later on */
      if(pSample->Direction[2] < 0) 
      {
        pSample->Direction[0] = -pSample->Direction[0];
        pSample->Direction[1] = -pSample->Direction[1];
        pSample->Direction[2] = -pSample->Direction[2];
      }
      /* Sample Geometry is read */
		
      /* Read geometry and sizes of the scattering particles */
      if(ReadTilComment(Buffer, pSampleFile)) 
      {
        sscanf(Buffer,"%c%lf%lf%lf", &cGeometry, &SizeA, &SizeB, &SizeC);
      } 
		
      /* Read scattering density and volume fraction of the scattering particles */
      if(ReadTilComment(Buffer, pSampleFile)) 
      {
        sscanf(Buffer,"%le%le%lf", &rho1, &rho2, &FracPtcl);
      } 
		
      /* Read macroscopic scattering cross sections */
      if(ReadTilComment(Buffer, pSampleFile)) 
      {
        sscanf(Buffer,"%lf%lf%lf", &MuInc, &MuTot, &MuAbs);
      } 

      /*	Allowed char. for geometry parameter: 
      S: spheres  D: distrib. of spheres  E: ellipsoids  C: cylinders  P: epipeds  I: isotropic sample */
      switch (cGeometry)
      {
        case 'E': 
        case 'C':
        case 'P': 
          if (SizeA==-1.0 || SizeB==-1.0 || SizeC==-1.0)
          {
            fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
            SampleFileName);
            exit(-1);
          }
          break;
        case 'D': 
          if (SizeA==-1.0 || SizeB==-1.0)
          {	
            fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
            SampleFileName);
            exit(-1);
          }
          break;
        case 'S':
          if (SizeA==-1.0)
          {	
            fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s",
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
  fclose(pSampleFile);
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

     SetSampleGeometry(&stSample);
  }
}


/*******************************************************/
/** Functions to calculate form factor                **/
/*******************************************************/
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


/*******************************************************/
/** Help functions to calculate form factor           **/
/*******************************************************/
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
