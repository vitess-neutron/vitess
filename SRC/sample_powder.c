/**************************************************************************************************/
/*  VITESS module sample_powder                                                                   */
/*                                                                                                */
/* This program simulates the elastic coherent diffraction and incoherent scattering              */
/* of neutrons at a powder sample.                                                                */
/*                                                                                                */
/* The free non-commercial use of these routines is granted providing due credit is given to      */
/* the authors.                                                                                   */
/*                                                                                                */
/* 1.0   Jan 1999  F. Streffer                                                                    */
/* 1.1   Nov 2001  K. Lieutenant  absolute current values, SOFTABORT, corrections in              */
/*                               NeutronIntersectsSphere                                          */
/* 1.2   Jan 2002  K. Lieutenant  reorganisation                                                  */
/* 1.3   Jul 2002  K. Lieutenant  corr.: UCV reading; check: output dir. in [theta_min,theta_max] */
/* 1.4   Jan 2004  K. Lieutenant  changes for 'instrument.dat', FullName() for struct.fac.file    */
/* 1.5   Feb 2004  K. Lieutenant  'FullParName', 'message' and 'ERROR' included; output extended  */
/* 1.6   Nov 2008  K. Lieutenant  Corr. inc. scat., colour, treat neutrons not hitting the sample */
/* 1.7   Nov 2013  D. Nekrassov   Visualisation, flexible input file formats introduced           */
/* 1.8   Nov 2015  K. Lieutenant  phi angle of cone separated from phi detector angle             */
/* 1.9   Apr 2020  K. Lieutenant  new central visualization parameters                            */
/* 1.10  Aug 2021  K. Lieutenant  option: parameters from input instead of from file              */
/* 1.10a Sep 2023  K. Lieutenant  correction: par. for spherical sample, visualisation improved   */
/**************************************************************************************************/

#include <string.h>

#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"
#include "convert.h"


/******************************/
/**   Global Variables       **/
/******************************/
// Input parameters
char  *pStrFileNameI=NULL;           // -s       [-]    pointer to structure factor file name (from input parameter)
char  *pSmplFileName=NULL;           // -S       [-]    pointer to the parameter file name (located in argv)
short  bIncScat =FALSE,              // -I       [-]    shall incoherent scattering be done ?
       bTreatAll=FALSE,              // -a       [-]    shall neutrons not hitting the sample be treated ?
       nColor   =NO_COLOR;           // -c       [-]    colour of the scattered neutrons
long   GenNeutrons=1;                // -A       [-]    repetitions (number of trajectories generated per incoming trajectory for each structure factor)
double Theta    = M_PI/2.0,          // -D      [deg]   these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,          // -d      [deg]    Theta has to be in the range of [0;PI]
       Phi      = M_PI,              // -P      [deg]    Phi has to be in the range of [0;2*PI]
       DelPhi   = M_PI;              // -p      [deg]
double Xpos     = 0.0,               // -x file  [cm]   position of the center of the sample
       Ypos     = 0.0,               // -y file  [cm]
       Zpos     = 0.0,               // -z file  [cm]
       Diameter = 0.0,               // -t file  [cm]   thickness or diameter of the sample
       Height   = 0.0,               // -h file  [cm]   height of the sample
       Width    = 0.0,               // -w file  [cm]   width of the sample
       Xdir     = 0.0,               // -X file  [-]    orientation of the sample
       Ydir     = 0.0,               // -Y file  [-]
       Zdir     = 0.0;               // -Z file  [-]
VtSmplGeom eGeom= VT_NO_GEOM;        // -G file  [-]    sample shape: VT_NO_GEOM, VT_CUBE, VT_CYL, VT_SPHERE, VT_HOL_CYL
extern
double MuTot,                        // -T file [1/cm]  macrosc. scattering cross section, defined in 'sample.c'
       MuAbs;                        // -m file [1/cm]  macrosc. absorption cross section, defined in 'sample.c'
double MuInc =  0.0,                 // -i file [1/cm]  incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm]
       UCV   =  0.0;                 // -U file [Ang^3] unit cell volume
extern
int    colD,                         // -C file  [-]    column where d-spacing is
       colF,                         // -F file  [-]    column where structure factor F is
       colF2,                        // -Q file  [-]    column where |F^2| is
       colM,                         // -M file  [-]    column where multiplicity is
       colDW;                        // -W file  [-]    column where Debye-Waller factor is
extern
double scaleF2;                      // -f file  [-]    normalization factor for structure factor

// Variables determined from input parameters or from file
SampleType stSample;                 //                 sample geometry and position
char  *pStrFileName="not found",     //          [-]    pointer to structure factor file name that is used
       sStrFileNameF[CHAR_BUF_XS]="";//    file  [-]    structure factor file name from parameter file
double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit           (int argc, char *argv[]);  // reads input parameters and sets global variables
void OwnCleanup        (DoublePair *StrucFac);    // does module specific cleanup
void SetSamplePar      (SampleType *pSample);     // sets sample parameters
void SetGeometry       (char* sColor);            // fills the structure stGeometry for visualization


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char *argv[])
{
  long       NumStrucFac=0;        /* number of reflections in the structure factor */
  double     DetFacCoh=0.0,        /* cares about the detector coverage      */
             DetFacInc=0.0;        /* for coherent and incoherent scattering */
  double     Lbf=0.0;              /* full path length of the neutron in the sample */
                                   /* with its initial direction */
  double     Ls=0.0;               /* distance of the neutron in the sample before sc. */
  long       j=0;                  /* counting variable */
  double     HelpFac=0.0;          /* contains k independent term of the scattering */
  long       Nth=0;                /* counting variable of structure factor */
  double     ScProb =0.0,          /* scattering probability */
             ScTheta=0.0,          /* scattering angles (in neutron coordinate system) */
             ScPhi  =0.0;
  double     RotMatrixSmpl[3][3];  /* Rotation matrix that transforms a Vector to the sample coordinate system */
  double     RotMatrixNeut[3][3];
  long       nisp=0,               /* number of intersection points to come       */
             i=0,                  /* counting index of the incoming trajectories */
             iGen=0;               /* counting index of the generated trajectories (see 'GenNeutrons') */
  DoublePair *StrucFac=NULL;
  VectorType SP={0.0,0.0,0.0},                       /* position of scattering event                        */
             InISP[2]={{0.0,0.0,0.0},{0.0,0.0,0.0}}; /* neutron intersection with sample before scattering */
  Neutron    InNeutron;

  // initialisation
  // --------------
  InitNeutron(&InNeutron);
  InitRotMatrix(RotMatrixSmpl);
  InitRotMatrix(RotMatrixNeut);

  _eModule = MCN_SMPL_POWDER;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.10a");
  OwnInit   (argc, argv);

  InitSample  (&stSample);
  SetSamplePar(&stSample);

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp     = TRUE;

  /* Get the unit-cell structure factors |f_N(t)|^2, try the name from paramter input first, then the name from file */
  NumStrucFac = ReadStructureFile(pStrFileNameI, 1, &StrucFac);
  if (NumStrucFac == 0)
  { NumStrucFac = ReadStructureFile(sStrFileNameF, 1, &StrucFac);
    if (NumStrucFac == 0)
    { fprintf(LogFilePtr,"ERROR: Can't read the structure factor data, neither from %s nor from %s\n", pStrFileNameI, sStrFileNameF);
      exit(-1);
    }
    else
    { pStrFileName=sStrFileNameF;
    }
  }
  else
  { pStrFileName=pStrFileNameI;
  }

  /* Factors that take care of the detector coverage */
  DetFacCoh = DelPhi/M_PI;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample vector pointing along the z-axis */
  RotMatrixX(stSample.Direction, RotMatrixSmpl);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons()!= 0)
  {
    for (i=0; i<NumNeutGot; i++)
    {
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        /* First, shift the origin of the system to the middle of the sample   */
        CopyNeutron(&InputNeutrons[i], &InNeutron);
        SubVector(InputNeutrons[i].Position, stSample.Position);

        /* Do anything to be done for the Scattering */
        if (NeutronIntersectsSample(&(InputNeutrons[i]), &stSample, RotMatrixSmpl, InISP, &nisp, VT_IN))
        {
          if (nisp == 1)
            CountMessageID(SMPL_TRAJ_INSIDE, InputNeutrons[i].ID);

          /* the neutron may be scattered between InISP[0] and InISP[1] */
          /* Lfb full path length in the sample before scattering       */
          Lbf = DistVector(InISP[0], InISP[1]);

          /* MONTE CARLO CHOICE: Where is the neutron scattered         */
          /* Distance Ls between entrance of the neutron InISP[0] and   */
          /* the scattering point SP               */
          Ls = MonteCarlo(0, Lbf);

          /* which is the corresponding scattering point       */
          /*   SP = InISP[0] + Ls * OutNeutron.Vector       */
          for (j=0; j<3; j++)
            SP[j] = InISP[0][j]+Ls*InputNeutrons[i].Vector[j];

          /* Determine the rotation matrix to point the neutron along the +x axis                      */
          NormVector(InputNeutrons[i].Vector);
          RotMatrixX(InputNeutrons[i].Vector, RotMatrixNeut);


          //   First the coherent scattering
          //--------------------------------
          /*   Scatter at each suitable |F(k)|            */
          /* Helpfac contains the non direction dependent term                         */
          /* G.L. Squires, "Introduction to the theory of thermal neutron scattering", */
          /* (1978), equation (3.103)  (UCV is the unit cell volume)                   */
          HelpFac = Lbf*pow(InputNeutrons[i].Wavelength,3)/(4.0*UCV*UCV);

          if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
            InputNeutrons[i].Color = nColor;

          /* Do the scattering for each StrucFac */
          for (Nth=0; StrucFac[Nth][0] > 0.5*InputNeutrons[i].Wavelength && Nth < NumStrucFac; Nth++)
          {
            CHECK

            /* ScTheta is the angle of the scattered neutron with its original flight path */
            ScTheta = 2.0*asin(InputNeutrons[i].Wavelength/(2.0*StrucFac[Nth][0]));

            /* Only trajectoris between Theta-DelTheta and Theta+DelTheta are regarded.
               The deviation from straight direction (neutTheta) of the incoming neutrons
               is supposed to be neglectible                            */
            if (ScTheta > Theta-DelTheta && ScTheta < Theta+DelTheta)
            {
              /* ScProb is the scattering-cross section (Squires 3.103) */
              /*  devided by the sample area                            */
              /* as I_sc = sigma * flux = sigma / area * current        */
              /* it contains the d-spacing dependent terms              */
              /*  and the d-spacing independent HelpFac terms s.o.      */
              ScProb = HelpFac / sin(0.5*ScTheta) * StrucFac[Nth][1] / GenNeutrons;

              /* Bring the neutron several times on the cone           */
              for(iGen=0; iGen<GenNeutrons; iGen++)
              {
                /* ScPhi is the angle of the scattered neutron with the +y-axis */
                /* The expression for the focussing is not staight forward,       */
                /* rather lengthy (and probably buggy) it may take a while       */
                ScPhi = MonteCarlo(Phi-DelPhi,Phi+DelPhi);

                /* Ok, now everthing needed is known, put it together */
                ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, ScProb,
                                    ScTheta, ScPhi, &stSample, RotMatrixNeut, RotMatrixSmpl);
              }
            }
          }

          // Second the incoherent scattering
          //--------------------------------
          if (bIncScat)
          {
            if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
              InputNeutrons[i].Color = (short)(nColor+1);

            for(iGen=0; iGen<GenNeutrons; iGen++)
            {
              /* Determine the scattering angle */
              ScPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
              ScTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

              /* Scattering probability */
              ScProb = Lbf*MuInc * sin(ScTheta) / GenNeutrons;

              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, ScProb,
                                  ScTheta, ScPhi, &stSample, RotMatrixNeut,  RotMatrixSmpl);
            }
          }
        } // end 'NeutronIntersect...
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
  fprintf(LogFilePtr, "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, total scat; absorption)\n"
                      "unit cell volume   : %8.3f Ang³\n"
                      "struct. factor file: %s\n",
                      MuInc, MuTot, MuAbs, UCV, pStrFileName);

  /* write geometry file */
  SetGeometry("white");

  /* Do module specific cleanups */
  OwnCleanup(StrucFac);

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
  /* Here we will set some mostly global variables to get things going */
  /* If there is no sample secification the program is aborted         */
  /* Known commanline parameters:                                      */
  /*  -S sample geometry file                                           */
  /*  -D detector information       (default: 4*M_PI)                   */
  /*  -A Neutron repitition rate   (default: 10)                       */
  /*********************************************************************/

  long i;

  colh = -1; colk = -1; coll  = -1;
  colD = -1; colF = -1; colF2 = -1; colM = -1; colDW = -1;
  scaleF2 = 1.0;

  /* Scan all command line parameters */
  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    { switch (argv[i][1])
      {
        /* what is the sample and the structure factore file called? */
        case 'S':
          pSmplFileName=&argv[i][2];
          break;
        case 's':
          pStrFileNameI=&argv[i][2];
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

        case 'G':
          eGeom = (VtSmplGeom) atoi(&argv[i][2]);
          break;

        /* sample position, size and orientation */
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

        /* get the solid angle covered by the detectors if other than 4PI read four numbers */
        case 'D':
          sscanf(&(argv[i][2]),"%lf", &Theta);
          Theta*=M_PI/180.0;
          /* Theta has to be in the range of [0;PI] */
          if (Theta < 0.0 || Theta > M_PI)
            Error("Theta has to be in the range of [0;PI] ");
          break;
        case 'd':
          sscanf(&(argv[i][2]),"%lf", &DelTheta);
          DelTheta*=M_PI/180.0;
          if (DelTheta < 0.0 || DelTheta > 0.5*M_PI)
            Error("DelTheta has to be in the range of [0;PI/2] ");
          break;
        case 'P':
          sscanf(&(argv[i][2]),"%lf", &Phi);
          Phi*=M_PI/180.0;
          /* Phi has to be in the range of [0;2*PI] */
          if (Phi < 0.0 || Phi > 2.0*M_PI)
            Error("Phi has to be in the range of [0;2*PI] ");
          break;
        case 'p':
          sscanf(&(argv[i][2]),"%lf", &DelPhi);
          DelPhi*=M_PI/180.0;
          if (DelPhi < 0.0 || DelPhi > M_PI)
            Error("DelPhi has to be in the range of [0, PI] ");
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
        case 'U':
          UCV = atof(&argv[i][2]);
          break;

        case 'C':
          colD = atof(&argv[i][2]);
          break;
        case 'F':
          colF = atof(&argv[i][2]);
          break;
        case 'Q':
          colF2 = atof(&argv[i][2]);
          break;
        case 'M':
          colM = atof(&argv[i][2]);
          break;
        case 'W':
          colDW = atof(&argv[i][2]);
          break;
        case 'f':
          scaleF2 = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  // check of theta and phi range
  if (Theta-DelTheta < 0.0 || Theta+DelTheta > M_PI)
    Error("wrong theta range: [Theta-DelTheta, Theta+DelThet] has to be in the range of [0, PI] ");

  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup(DoublePair *StrucFac)
{
  /* print error that might have occured many times */
  PrintMessage(SMPL_TRAJ_INSIDE, "", ON);
  fprintf(LogFilePtr, "\n");

  /* Release the allocated memory */
  if (StrucFac!=NULL)
    free(StrucFac);
}


/*******************************************************/
/** Reads sample parameters from file                 **/
/*******************************************************/
void  SetSamplePar(SampleType* pSample)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="", sGeom[20]="";
  int    col_d=0, col_f=0, col_f2=0, col_m=0, col_dw=0,
         nLen=sizeof(sLine)-1;
  double x=0.0, y=0.0, z=0.0,
         xdir  =0.0, ydir  =0.0, zdir =0.0,
         d_par =0.0, height=0.0, width=0.0,
         muInc =0.0, muTot =0.0, muAbs=0.0,
         ucv =0.0, scale_f2=0.0;
  VtSmplGeom geom=VT_NO_GEOM;
  SampleType sample;         // file  sample geometry

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
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sGeom);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &d_par, &height, &width);
      geom = SmplGeom_Txt2ID(sGeom);
      if (geom!=VT_SPHERE)
      { if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &xdir,  &ydir,  &zdir);}
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sStrFileNameF);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &muInc, &muTot, &muAbs);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf",         &ucv);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%d %d %d %d %d %lf", &col_d, &col_f, &col_f2, &col_m, &col_dw, &scale_f2);

      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeom==VT_NO_GEOM && geom!=VT_NO_GEOM) eGeom = geom;
      if (Xpos    ==0.0 && x     !=0.0) Xpos    = x;
      if (Ypos    ==0.0 && y     !=0.0) Ypos    = y;
      if (Zpos    ==0.0 && z     !=0.0) Zpos    = z;
      if (Diameter==0.0 && d_par != 0.0)
      { if (eGeom==VT_CUBE) Diameter = d_par; else Diameter = 2.0 * d_par;}
      if (Height  ==0.0 && height!=0.0) Height  = height;
      if (Width   ==0.0 && width !=0.0) Width   = width;
      if (Xdir    ==0.0 && xdir  !=0.0) Xdir    = xdir;
      if (Ydir    ==0.0 && ydir  !=0.0) Ydir    = ydir;
      if (Zdir    ==0.0 && zdir  !=0.0) Zdir    = zdir;
      if (MuInc   ==0.0 && muInc !=0.0) MuInc   = muInc;
      if (MuTot   ==0.0 && muTot !=0.0) MuTot   = muTot;
      if (MuAbs   ==0.0 && muAbs !=0.0) MuAbs   = muAbs;
      if (UCV     ==0.0 && ucv   !=0.0) UCV     = ucv  ;
      if (colD    ==0.0 && col_d !=0.0) colD    = col_d ;
      if (colF    ==0.0 && col_f !=0.0) colF    = col_f ;
      if (colF2   ==0.0 && col_f2!=0.0) colF2   = col_f2;
      if (colM    ==0.0 && col_m !=0.0) colM    = col_m ;
      if (colDW   ==0.0 && col_dw!=0.0) colDW   = col_dw;
      if (scaleF2==0.0 && scale_f2!=0.0) scaleF2 = scale_f2;
      // if (pStrFileNameI==NULL && strlen(sStrFileNameF) > 0) pStrFileNameI=sStrFileNameF;
    }
    else
    {
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSmplFileName);
    }
  }

  // checks if needed parameters were given
  if (eGeom==VT_NO_GEOM)
    Error2("Sample geometry could not be identified", sGeom);
  if (UCV==0.0)
    Error("Unit cell volume is not given");

  // fills data structures
  FillSample(pSample, eGeom, Xpos, Ypos, Zpos, Xdir, Ydir, Zdir, Diameter, Height, Width, 0.0);

  /* the direction vector should have a positive z component  */
  /* this will make things easier with the rotations later on */
  if (pSample->Direction[2] < 0)
  {
    pSample->Direction[0] = -pSample->Direction[0];
    pSample->Direction[1] = -pSample->Direction[1];
    pSample->Direction[2] = -pSample->Direction[2];
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

    SetSampleGeometry(&stSample, 0.0);
  }
}
