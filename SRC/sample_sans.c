/***********************************************************************************************/
/*  VITESS module sample_sans                                                                  */
/* This program  simulates the coherent elastic  diffraction of neutrons at a SANS sample.     */
/*                                                                                             */
/* The free non-commercial use of these routines is granted providing due credit is given to   */
/* the authors.                                                                                */
/*                                                                                             */
/* 1.0       1999  F. Streffer                                                                 */
/* 1.1   May 2001  K. Lieutenant  adding ellipsoids, cylinders, parallelepipeds                */
/* 1.2   Jun 2001  K. Lieutenant  absolute current values, input data for Q ignored, SOFTABORT */
/* 1.3   Nov 2001  K. Lieutenant  corrections in NeutronIntersectsCylinder                     */
/* 1.4   Jan 2002  K. Lieutenant  reorganisation                                               */
/* 1.5   Feb 2002  K. Lieutenant  correction detector coverage,                                */
/*                                deletion of Q-range, adding of incoher. scattering           */
/* 1.6   Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                 */
/* 1.7   Feb 2004  K. Lieutenant  'FullParName', 'message' & 'ERROR' included; output extended */
/* 1.8   Nov 2012  K. Lieutenant  size distribution of spheres                                 */
/* 1.9   Oct 2013  K. Lieutenant  only theta_max variable                                      */
/* 1.10  Apr 2020  K. Lieutenant  new central visualization parameters                         */
/* 1.11  Oct 2021  K. Lieutenant  option: parameters from input instead of from file           */
/* 1.11a Sep 2023  K. Lieutenant  correction: par. for spherical sample, visualisation improved*/
/***********************************************************************************************/

#include <string.h>

#include "convert.h"
#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/
char  *pSmplFileName=NULL;      // -S              pointer to the name of the sample file 
short  bIncScat =FALSE,         // -I       [-]    shall incoherent scattering be done ?  
       bTreatAll=FALSE,         // -a vsn4  [-]    shall neutrons not hitting the sample be treated ?  
       nColor   =NO_COLOR;      // -c vsn4  [-]    colour of the scattered neutrons  
long   GenNeutrons =1;          // -A              repetitions (how many trajectories to generate per incoming trajectory)
double ThetaMax = 0.0,          // -M              maximum scattering angle to be considered
       Theta    = M_PI/2.0,     //    calc         these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,     //    calc            Theta has to be in the range of [0;PI]         
       Phi      = M_PI,         //    fix             Phi has to be in the range of [0;2*PI] 
       DelPhi   = M_PI;         //    fix
double Xpos     = 0.0,          // -x file  [cm]   position of the center of the sample 
       Ypos     = 0.0,          // -y file  [cm]  
       Zpos     = 0.0,          // -z file  [cm]  
       Diameter = 0.0,          // -t file  [cm]   thickness or diameter of the sample 
       Height   = 0.0,          // -h file  [cm]   height of the sample 
       Width    = 0.0,          // -w file  [cm]   width of the sample
       Xdir     = 0.0,          // -X file  [-]    orientation of the sample 
       Ydir     = 0.0,          // -Y file  [-]  
       Zdir     = 0.0;          // -Z file  [-]  
VtSmplGeom eGeomS= VT_NO_GEOM;  // -G file  [-]    sample shape: VT_NO_GEOM, VT_CUBE, VT_CYL, VT_SPHERE, VT_HOL_CYL
VtPtclGeom eGeomP= VT_NO_PTCL;  /* -O file  [-]    particle shape: 
                                                   S: spheres,        R  = SizeA
                                                   D: size dstr. sph. Rmin=SizeA, Rmax=SizeB
                                                   E: ellipsoids      Rx = SizeA, Ry = SizeB, Rz = SizeC
                                                   C: cylinders,      Rx = SizeA, Ry = SizeB, L  = SizeC
                                                   P: parallelepiped, a  = SizeA, b  = SizeB, c  = SizeC
                                                   I: no scattering objects, isotropic scattering */ 
double SizeA   = -1.0,          // -U file  [Ang]  size of the particles
       SizeB   = -1.0,          // -V file           e.g. hard spere radius
       SizeC   = -1.0,          // -W file           in x-, y-, and z-direction 
       Rho1    =  0.0,          // -s file         scattering length density of the particles 
       Rho2    =  0.0,          // -S file         scattering length density of the solvemt 
       FracPtcl=  0.0;          // -f file         volume fraction of the particles 
extern                                             
double MuTot,                   // -T file         macrosc. scattering cross section, defined in 'sample.c'
       MuAbs;                   // -m file         macrosc. absorption cross section, defined in 'sample.c'
double MuInc   = 0.0;           // -i file         incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] 
                                                   
// Variables determined from input parameters or from file
SampleType stSample;            //    file         sample geometry and position

double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void   OwnInit     (int argc, char *argv[]);   // reads input parameters and sets global variables
void   OwnCleanup  ();                         // does module specific cleanup
void   SetSamplePar(SampleType *pSample);      // sets sample parameters
void   WritePar    ();                         // writes input parameters to log file
void   SetGeometry (char* sColor);             // fills the structure stGeometry for visualization    missing

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
  VectorType InISP[2];            /* neutron intersection before scattering */
  double     qValue=0.0,          /* absolute value of momentum transfer    */
             fThetaMin=0.0,       /* minimal and maximal values of the           */
             fThetaMax=0.0,       /* scattering angle according to Theta, DelTheta */
             fVolPtkl=0.0,        /* Volume of the particle [cm³] */
             fFacCtrPtkl=0.0,     /* factor considering contrast and particle size */ 
             fFormFac=0.0,        /* normalized form factor for the partical shape and size */
             fFac=0.0, 
             neutTheta=0.0,
             neutPhi=0.0;
  double     DetFacInc=0.0,       /* care about the detector coverage  */
             DetFacCoh=0.0,     
             Lbf=0.0;             /* full path length of the neutron in the sample with its initial direction */
  double     Ls=0.0;              /* distance of the neutron in the sample before sc. */
  long       j=0;                 /* counting variable */
  VectorType SP={0.0,0.0,0.0},          /* position of scattering event */
             dQ={0.0,0.0,0.0};          /* momentum transfer in the particle coordinate system   */
  long       Nth=0;               /* counting variable of structure factor */
  double     ScTheta=0.0,         /* angle of coherent Scattering */
             ScProb=0.0,          /* scattering probability */
             Radius=0.0,          /* radius of a sphere     */
             OutTheta=0.0,        /* Final angles of the neutron in the sample system */
             OutPhi=0.0;
  double     RotMatrixSmpl[3][3], /* Rotation matrices that transform a Vector to the */
             RotMatrixNeut[3][3]; /* sample coordinate system                         */
  long       i=0,                 /* counting variable of the neutrons */
             nisp=0,              /* number of intersection points to come */
             NeutCount=0;
  Neutron    InNeutron;

  // initialisation
  // --------------
  InitNeutron(&InNeutron);
  InitVector(InISP[0]); InitVector(InISP[1]),
  InitRotMatrix(RotMatrixSmpl);
  InitRotMatrix(RotMatrixNeut);

  _eModule = MCN_SMPL_SANS;
  
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.11a");
  OwnInit(argc, argv);

  /* Go and get the geometry of the sample and the scattering objects */
  InitSample  (&stSample);
  SetSamplePar(&stSample);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  /* Factor that takes care of the dectector coverage */
  DetFacInc = DelPhi/M_PI*DelTheta;
  DetFacCoh = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find a new basis */
  /* with the sample vector pointing along the z-axis  */
  RotMatrixX(stSample.Direction, RotMatrixSmpl);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons()!= 0)
  {
    for (i=0; i<NumNeutGot; i++)
    {
      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /* First, shift the origin of the system to the center of the sample */
        CopyNeutron(&InputNeutrons[i], &InNeutron);
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
          if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
            InputNeutrons[i].Color = nColor;

          for (Nth=0; Nth < GenNeutrons; Nth++) 
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
            switch (eGeomP)
            {
              case VT_PTCL_SPHERE: 
                fFormFac = FormFactorSphere(qValue, SizeA);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(SizeA,3);
                break;
              case VT_PTCL_POLY_SPH: 
                Radius   = MonteCarlo(SizeA, SizeB);
                fFormFac = FormFactorSphere(qValue, Radius);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * pow(Radius,3);
                break;
              case VT_PTCL_ELLIPS: 
                fFormFac = FormFactorEllipsoid(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * 4.0/3.0 * M_PI * SizeA*SizeB*SizeC;
                break;
              case VT_PTCL_CYL: 
                fFormFac = FormFactorCylinder(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * M_PI * SizeA*SizeB * SizeC;
                break;
              case VT_PTCL_EPIPED: 
                fFormFac = FormFactorEpiped(dQ[0], SizeA, dQ[1], SizeB, dQ[2], SizeC);
                fVolPtkl = 1.0e-24 * SizeA*SizeB*SizeC;
                break;
              default:
                fFormFac = 1.0; 
                break;
            }

            // Determine the scattering probability from the form factor, 
            // contrast and particle size, the sample size, and the solid angle factor,
            if (eGeomP==VT_ISOTROPIC)
            {	
              fFacCtrPtkl = 1.0;
            }
            else
            {	
              fFacCtrPtkl = FracPtcl* pow((Rho1-Rho2),2) * fVolPtkl;
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
            if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
              InputNeutrons[i].Color = (short)(nColor+1);

            for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
            {
              /* Determine the scattering angle */
              OutPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
              OutTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

              /* Scattering probability */
              ScProb  = Lbf*MuInc * sin(OutTheta) / GenNeutrons;

              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, ScProb, OutTheta, OutPhi, &stSample, OneMatrix,  RotMatrixSmpl);
            }
          }
        }
        else if (bTreatAll==TRUE)
        {	
          WriteNeutron(&InputNeutrons[i]);
        }
        else
        {
          WriteDIAP(&InNeutron, VT_OUTSIDE, Xpos - InNeutron.Position[0]);
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
	
  long i=0;
	
  /* Ok, scan all command line parameters */
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'S':
        /* what is the sample file called? */
          pSmplFileName=&argv[i][2];
          break;

        /* Consider incoherent scattering? neutrons not hitting the sample? mark scattered neutrons? Multiply trajectories */
        case 'I':
          if(argv[i][2]=='1') bIncScat=TRUE;
          break;
        case 'a':
          if(argv[i][2]=='1') bTreatAll=TRUE;
          break;
        case 'c':
          nColor = (short) atoi(&argv[i][2]);
          break;
        case 'A':
          GenNeutrons = atol(&argv[i][2]);
          break;

        /* get the solid angle covered by the detector */
        case 'M':
          sscanf(&(argv[i][2]),"%lf", &ThetaMax);
          Theta   =0.5*ThetaMax*M_PI/180.0;
          DelTheta=Theta;
          break;

        case 'x':
          Xpos = atof(&argv[i][2]);
          break;
        case 'y':
          Ypos = atof(&argv[i][2]);
          break;
        case 'z':
          Zpos = atof(&argv[i][2]);
          break;

        case 't':
          Diameter = atof(&argv[i][2]);
          break;
        case 'h':
          Height = atof(&argv[i][2]);
          break;
        case 'w':
          Width = atof(&argv[i][2]);
          break;

        case 'X':
          Xdir = atof(&argv[i][2]);
          break;
        case 'Y':
          Ydir = atof(&argv[i][2]);
          break;
        case 'Z':
          Zdir = atof(&argv[i][2]);
          break;

        case 'G':
          eGeomS = (VtSmplGeom) atoi(&argv[i][2]);
          break;
        case 'O':
          eGeomP = (VtPtclGeom) atoi(&argv[i][2]);
          break;

        case 'U':
          SizeA = atof(&argv[i][2]);
          break;
        case 'V':
          SizeB = atof(&argv[i][2]);
          break;
        case 'W':
          SizeC = atof(&argv[i][2]);
          break;

        case 'l':
          Rho1 = atof(&argv[i][2]);
          break;
        case 'L':
          Rho2 = atof(&argv[i][2]);
          break;
        case 'f':
          FracPtcl = atof(&argv[i][2]);
          break;

        case 'i':
          MuInc = atof(&argv[i][2]);
          break;
        case 'T':
          MuTot = atof(&argv[i][2]);
          break;
        case 'm':
          MuAbs = atof(&argv[i][2]);
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
  char sGeomP[30]="";

  PtclGeom_ID2Txt(sGeomP, eGeomP);
  switch (eGeomP)
  {	
    case VT_PTCL_SPHERE  : fprintf(LogFilePtr, "%s: %8.2f Ang radius\n",                    sGeomP, SizeA);               break;
    case VT_PTCL_POLY_SPH: fprintf(LogFilePtr, "%s from %8.2f to %8.2f Ang radius\n",       sGeomP, SizeA, SizeB);        break;
    case VT_PTCL_ELLIPS  : fprintf(LogFilePtr, "%s: radii %8.2f,%8.2f,%8.2f Ang\n",         sGeomP, SizeA, SizeB, SizeC); break;
    case VT_PTCL_EPIPED  : fprintf(LogFilePtr, "%s: %8.2f,%8.2f,%8.2f Ang length\n",        sGeomP, SizeA, SizeB, SizeC); break;
    case VT_PTCL_CYL     : fprintf(LogFilePtr, "%s: radii %8.2f,%8.2f, length:%8.2f Ang\n", sGeomP, SizeA, SizeB, SizeC); break;
    case VT_ISOTROPIC    : fprintf(LogFilePtr, "%s \n",                                     sGeomP);                      break;
  }
  fprintf(LogFilePtr, "scat. length density: %13.3e (particle) %10.3e 1/cm^2 (solvent)\n"
                      "vol.fract. of part. : %8.3f\n"
                      "macr. cross section : %10.5f,%10.5f;%10.5f  1/cm (incoh, total scat; absorption)\n",
                      Rho1, Rho2, FracPtcl, MuInc, MuTot, MuAbs);
}


/*********************************************************************/
/* Reads data of the sample (position, geometry, size, orientation)   */
/*********************************************************************/
void SetSamplePar(SampleType* pSample)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="", 
         sGeomS[20]="",             // string: sample shape
         cGeomP    =' ';            // char  : particle shape
  int    nLen=sizeof(sLine)-1;
  double x     = 0.0, y     = 0.0, z    = 0.0, 
         xdir  = 0.0, ydir  = 0.0, zdir = 0.0,
         d_par = 0.0, height= 0.0, width= 0.0,
         sizeA =-1.0, sizeB =-1.0, sizeC=-1.0,
         rho1  = 0.0, rho2  = 0.0, frac = 0.0,
         muInc = 0.0, muTot = 0.0, muAbs= 0.0; 
  VtSmplGeom geomS;           // enum  sample shape
  VtPtclGeom geomP;           // enum  particle shape
  SampleType sample;          // structure  sample geometry

  InitSample(pSample);
  InitSample(&sample);

  /* Opens the parameter file if a file name is given */
  if (pSmplFileName!=NULL)
  { 
    pFile = OpenInputFile(pSmplFileName, FALSE, "rt");

    /* Reads the parameters if the file can be opened */
    if (pFile != NULL)
    { 
      /* First line: sample position     */
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x, &y, &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sGeomS); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &d_par, &height, &width);
      geomS = SmplGeom_Txt2ID (sGeomS);
      if (geomS!=VT_SPHERE)
      { if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &xdir,  &ydir,  &zdir);}
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%c %lf %lf %lf", &cGeomP, &sizeA, &sizeB, &sizeC);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &rho1,  &rho2,  &frac);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &muInc, &muTot, &muAbs); 

      geomP = PtclGeom_Char2ID(cGeomP);

      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeomS==VT_NO_GEOM && geomS!=VT_NO_GEOM) eGeomS = geomS; 
      if (eGeomP==VT_NO_PTCL && geomP!=VT_NO_PTCL) eGeomP = geomP; 
      if (Xpos    == 0.0 && x     != 0.0) Xpos    = x;
      if (Ypos    == 0.0 && y     != 0.0) Ypos    = y;
      if (Zpos    == 0.0 && z     != 0.0) Zpos    = z;
      if (Diameter== 0.0 && d_par != 0.0)
      { if (eGeomS==VT_CUBE) Diameter = d_par; else Diameter = 2.0 * d_par;}
      if (Height  == 0.0 && height!= 0.0) Height  = height;
      if (Width   == 0.0 && width != 0.0) Width   = width;
      if (Xdir    == 0.0 && xdir  != 0.0) Xdir    = xdir;
      if (Ydir    == 0.0 && ydir  != 0.0) Ydir    = ydir;
      if (Zdir    == 0.0 && zdir  != 0.0) Zdir    = zdir;
      if (SizeA   ==-1.0 && sizeA !=-1.0) SizeA   = sizeA;
      if (SizeB   ==-1.0 && sizeB !=-1.0) SizeB   = sizeB;
      if (SizeC   ==-1.0 && sizeC !=-1.0) SizeC   = sizeC;
      if (Rho1    == 0.0 && rho1  != 0.0) Rho1    = rho1;
      if (Rho2    == 0.0 && rho2  != 0.0) Rho2    = rho2;
      if (FracPtcl== 0.0 && frac  != 0.0) FracPtcl= frac;
      if (MuInc   == 0.0 && muInc != 0.0) MuInc   = muInc;
      if (MuTot   == 0.0 && muTot != 0.0) MuTot   = muTot;
      if (MuAbs   == 0.0 && muAbs != 0.0) MuAbs   = muAbs;
    }
    else
    {	
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSmplFileName);
    }
  }

  // checks if geometry was given
  if (eGeomS==VT_NO_GEOM)
    Error2("Sample geometry could not be identified", sGeomS);

  // fills data structures
  FillSample(pSample, eGeomS, Xpos, Ypos, Zpos,  Xdir, Ydir, Zdir, Diameter, Height, Width, 0.0);

  /* the direction vector should have a positive z component this will make things easier with the rotations later on */
  if (pSample->Direction[2] < 0) 
  {
    pSample->Direction[0] = -pSample->Direction[0];
    pSample->Direction[1] = -pSample->Direction[1];
    pSample->Direction[2] = -pSample->Direction[2];
  }

  /*	Allowed char. for geometry parameter: S: spheres  D: distrib. of spheres  E: ellipsoids  C: cylinders  P: epipeds  I: isotropic sample */
  switch (eGeomP)
  {
    case VT_PTCL_ELLIPS: 
    case VT_PTCL_CYL   :
    case VT_PTCL_EPIPED: 
      if (SizeA==-1.0 || SizeB==-1.0 || SizeC==-1.0)
      {
        fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s", pSmplFileName);
        exit(-1);
      }
      break;
    case VT_PTCL_POLY_SPH: 
      if (SizeA==-1.0 || SizeB==-1.0)
      {	
        fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s", pSmplFileName);
        exit(-1);
      }
      break;
    case VT_PTCL_SPHERE:
      if (SizeA==-1.0)
      {	
        fprintf(LogFilePtr, "ERROR: Can't read sufficient information about particles in %s", pSmplFileName);
        exit(-1);
      }
      break;
    case VT_NO_PTCL:
    case VT_ISOTROPIC:
      break;
    default:
      fprintf(LogFilePtr, "ERROR: No or wrong character given for the particle geometry in %s", pSmplFileName);
      exit(-1);
  }
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

    SetSampleGeometry(&stSample,  0.0);
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
