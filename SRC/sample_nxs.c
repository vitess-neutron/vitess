/************************************************************************************************/
/*  VITESS module sample_nxs                                                                    */
/*                                                                                              */
/* This program simulates coherent and incoherent scattering, absorption and transmission based */
/* on the calculation of wavelength-dependent neutron cross sections and the unit cell          */
/* definition, i.e. the crystallographic structure, of the sample.                              */
/*                                                                                              */
/* DETAILS: The module was originally developed for McStas and is now translated to VITESS. As  */
/*          a result the same input files can be used for both programs.                        */
/*          The sample module calls the neutron cross section functions from the nxs library,   */
/*          which needs to be linked for compilation. Moreover, the module includes input file  */
/*          reading routines (read_table-lib) from the McStas software, that must also be       */
/*          included.                                                                           */
/*          The sample module can be used with different modes (using the bTransOnly switch):   */
/*          If bTransOnly=True (YES), then all neutrons intersecting with the sample will be    */
/*          transmitted to the forward flight direction. Then, the neutron weight is changed by */
/*          the exponential attenuation. Therefore, the total neutron cross section is used:    */
/*             I/I_0 = p_transmit = exp( -sigma_total * N * thickness )                         */
/*          No scattering or absorption will be performed.                                      */
/*                                                                                              */
/*          If bTransOnly=False (NO), then the calculated scattering and absorption cross       */
/*          sections will be used as probabilities to determine whether a neutron should be     */
/*          transmitted, absorbed or scattered (coherently or incoherently).                    */
/*                                                                                              */
/*                                                                                              */
/*  The sample module makes use of the SgInfo library, whose free usage is granted by the       */
/*  following notice:                                                                           */
/*                                                                                              */
/*  Copyright Notice:                                                                           */
/*  Space Group Info (c) 1994-96 Ralf W. Grosse-Kunstleve                                       */
/*  Permission to use and distribute this software and its documentation for noncommercial      */
/*  use and without fee is hereby granted, provided that the above copyright notice appears     */
/*  in all copies and that both that copyright notice and this permission notice appear in      */
/*  the supporting documentation. It is not allowed to sell this software in any way. This      */
/*  software is not in the public domain.                                                       */
/*                                                                                              */
/*                                                                                              */
/* The free non-commercial use of these routines is granted providing due credit is given to    */
/* the authors.                                                                                 */
/*                                                                                              */
/* 1.0  Nov 2011  M. Boin    1st official release                                               */
/*                           Transmission, absorption, coherent and incoherent scattering       */
/*                           implemented.                                                       */
/* 1.0a May 2012  A. Houben  Color is also set for coherently scattered neutrons                */
/* 1.1  Oct 2014  M. Boin    Updated nxs library and removed dependence from read_table-lib.h   */
/* 1.2  Apr 2020  K. Lieutenant  visualisation and new central visualization parameters         */
/* 1.3  Oct 2021  K. Lieutenant  option: parameters from input instead of from file             */
/************************************************************************************************/

#include <string.h>
#include <math.h>

#include "convert.h"
#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"

#include "nxs.h"             /* from nxs library */

/******************************/
/**   Global Variables       **/
/******************************/
// Input parameters
char  *pNxsFileNameI=NULL;       // -s       [-]    pointer to nxs parameter file name (from input parameter)
char  *pSmplFileName=NULL;       // -S       [-]    name of the sample parameter file      
double Theta    = M_PI/2.0,      // -D      [deg]   these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,      // -d      [deg]      Theta has to be in the range of [0;PI]         
       Phi      = M_PI,          // -P      [deg]      Phi has to be in the range of [0;2*PI] 
       DelPhi   = M_PI;          // -p      [deg]
short  nColor=NO_COLOR,          // -c       [-]   colour of the scattered neutrons  
       bIncohScat = FALSE,       // -I       [-]   shall incoherent scattering be done ?             
       bTreatAll  = FALSE,       // -a       [-]   shall neutrons not hitting the sample be treated ?
       bTransOnly = FALSE;       // -T       [-]   shall only transmission (imaging) be treated (=TRUE)? Or scattering also(=FALSE)?
long   GenNeutrons=1;            // -A       [-]   how many trajectories to generate per incoming trajectory
double Xpos     = 0.0,           // -x file  [cm]  position of the center of the sample 
       Ypos     = 0.0,           // -y file  [cm]  
       Zpos     = 0.0,           // -z file  [cm]  
       Diameter = 0.0,           // -t file  [cm]  thickness or radius of the sample 
       Height   = 0.0,           // -h file  [cm]  height of the sample 
       Width    = 0.0,           // -w file  [cm]  width of the sample
       Xdir     = 0.0,           // -X file  [-]   orientation of the sample 
       Ydir     = 0.0,           // -Y file  [-]  
       Zdir     = 0.0;           // -Z file  [-]  
VtSmplGeom eGeom= VT_NO_GEOM;    // -G file  [-]   sample shape: VT_NO_GEOM, VT_CUBE, VT_CYL, VT_SPHERE, VT_HOL_CYL

// Variables determined from input parameters or from file
SampleType Sample;                  //            sample geometry and position
char *pNxsFileName="not found",     //       [-]   pointer to NXS file name that is used
      sNxsFileNameF[CHAR_BUF_XS]="";// file  [-]   NXS file name from parameter file

// constants
short  MAX_HKL = 8;                   //        maximum hkl index value                           
double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/******************************/
/**   Extern Variables       **/
/******************************/
extern 
double MuTot,    // file  macrosc. scattering cross section, defined in 'sample.c'
       MuAbs;    // file  macrosc. absorption cross section, defined in 'sample.c'


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit    (int argc, char *argv[]);     // reads input parameters and sets global 
void  OwnCleanup (DoublePair *StrucFac);       // Does module specific cleanup
void SetSamplePar(SampleType *pSample);        // sets sample parameters 
void  SetGeometry(char* sColor);               // fills the structure stGeometry for visualization

char* FullInName (const char* filename);       // path to input directory + file name  (from init.c)


/******************************/
/**   Program                **/
/******************************/
int main(int argc, char *argv[])
{
  /* sample */
  NXS_UnitCell   uc;               /* unit cell definitions (from nxs.h) */
  NXS_AtomInfo   *atomInfoList;    /* list for atom parameters */
  double     DetFacCoh=0.0,        /* cares about the detector coverage      */
             DetFacInc=0.0;        /* for coherent and incoherent scattering */
  double     Lbf=0.0;              /* full path length of the neutron in the sample with its initial direction */
  double     Ls=0.0;               /* distance of the neutron in the sample before sc. */
  long       j=0;                  /* counting variable */
  VectorType SP;                   /* position of scattering event */
  double     ScTheta=0.0,          /* scattering angles (in neutron coordinate system) */
             ScPhi=0.0;
  double     RotMatrixSmpl[3][3];  /* Rotation matrix that transforms a Vector to the sample coordinate system */
  double     RotMatrixNeut[3][3];
  long       nisp=0,               /* number of intersection points to come       */
             i=0,                  /* counting index of the incoming trajectories */
             iGen=0;               /* counting index of the generated trajectories (see 'GenNeutrons') */
  DoublePair *StrucFac=NULL;
  double     mu_factor = 0.0;
  int        numAtoms  = 0;
  int        nxs_init_success = 0;
  VectorType InISP[2]={{0.0,0.0,0.0},{0.0,0.0,0.0}};             /* neutron intersection before scattering */
  
  // initialisation
  // --------------
  InitRotMatrix(RotMatrixSmpl);
  InitRotMatrix(RotMatrixNeut);

  _eModule = MCN_SMPL_NXS;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  /* Go and get the sample geometry and name of nxs parameter file */
  InitSample  (&Sample);
  SetSamplePar(&Sample);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = TRUE;

  fprintf(LogFilePtr, "NXS parameter file: %s\n", pNxsFileName);

  /* read unit cell parameters from file 
     try the name from paramter input first, then the name from file */
  numAtoms = nxs_readParameterFile( FullInName(pNxsFileNameI), &uc, &atomInfoList ); 
  if( numAtoms < 1 )
  {
    numAtoms = nxs_readParameterFile( FullInName(sNxsFileNameF), &uc, &atomInfoList ); 
    if( numAtoms < 1 )
    {
      NXS_AtomInfo ai;
    
      /* fallback solution: if no file exists, use alpha_iron */
      fprintf(LogFilePtr, "WARNING: nxs parameter file NOT found, neither in %s nor in %s!\n! Using default values...\n", pNxsFileNameI, sNxsFileNameF);
      strncpy(uc.spaceGroup,"229",MAX_CHARS_SPACEGROUP);
      uc.a = 2.866; uc.alpha = 90.0; uc.debyeTemp = 464.0;
      strncpy(ai.label,"Fe",MAX_CHARS_ATOMLABEL); ai.b_coherent = 9.45;
      ai.sigmaIncoherent = 0.4; ai.sigmaAbsorption = 2.56;
      ai.molarMass = 55.85;
      ai.x[0] = ai.y[0] = ai.z[0] = 0.0;
      atomInfoList = (NXS_AtomInfo*)realloc( atomInfoList, sizeof(NXS_AtomInfo) );
      atomInfoList[0] = ai;
      numAtoms = 1;
    }
    else
    { pNxsFileName=sNxsFileNameF;
    }
  }
  else
  { pNxsFileName=pNxsFileNameI;
  }
  
  if( NXS_ERROR_OK != nxs_initUnitCell(&uc) )
  {
    fprintf(LogFilePtr, "WARNING: No nxs parameters set! Sample will be transparent!\n");
    nxs_init_success = 0;
  }
  else
  {
    unsigned int k;
    uc.temperature = 293.0;
       
    fprintf(LogFilePtr, "NXS unit cell definition is:\n"
          "space group number = %s\n"
          "a = %f \t\t alpha = %f\n"
          "b = %f \t\t beta  = %f\n"
          "c = %f \t\t gamma = %f\n"
          "debye_temp = %f\n"
          "# label  b_coherent  sigma_inc  sigma_abs  molar_mass  x  y  z\n",
          uc.spaceGroup, uc.a, uc.alpha, uc.b, uc.beta, uc.c, uc.gamma, uc.debyeTemp);
          
    for( k=0; k<numAtoms; k++ )
    {
      NXS_AtomInfo ai = atomInfoList[k];
      nxs_addAtomInfo( &uc, ai );
      fprintf(LogFilePtr, "%s  %f  %f  %f  %f  %f  %f  %f\n",
          ai.label, ai.b_coherent, ai.sigmaIncoherent, ai.sigmaAbsorption, ai.molarMass, ai.x[0], ai.y[0], ai.z[0]);
    }

    uc.maxHKL_index = MAX_HKL;
    nxs_initHKL( &uc );
    
      /* factor for the calculation of the attenuation */
    mu_factor = 1.0 / uc.volume;
  
    nxs_init_success = 1;
  }

  /* Factors that take care of the detector coverage */
  DetFacCoh = DelPhi/M_PI;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample */
  /* vector pointing along the z-axis              */
  RotMatrixX(Sample.Direction, RotMatrixSmpl);
  DECLARE_ABORT;

  /* Start the main loop getting neutrons         */
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
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
        SubVector(InputNeutrons[i].Position, Sample.Position);

        /* Do anything to be done for the Scattering */
        if( NeutronIntersectsSample(&(InputNeutrons[i]), &Sample, RotMatrixSmpl, InISP, &nisp, VT_IN) && nxs_init_success )
        {
          double norm, xsect_coherent, xsect_incoherent, xsect_absorption, xsect_total, p_transmit;
          if (nisp < 2)
            CountMessageID(SMPL_TRAJ_INSIDE, InputNeutrons[i].ID);

          /* the neutron may be scattered between InISP[0] and InISP[1] */
          /* Lfb full path length in the sample before scattering       */
          Lbf = DistVector(InISP[0], InISP[1]);

          /* MONTE CARLO CHOICE: Where is the neutron scattered         */
          /* Distance Ls between entrance of the neutron InISP[0] and   */
          /* the scattering point SP               */
          Ls = MonteCarlo(0, Lbf);

          /* which is the corresponding scattering point    */
          /*   SP = InISP[0] + Ls * OutNeutron.Vector       */
          for(j=0; j<3; j++)
            SP[j] = InISP[0][j]+Ls*InputNeutrons[i].Vector[j];

          /* Determine the rotation matrix to point the neutron along the +x axis */
          NormVector(InputNeutrons[i].Vector);
          RotMatrixX(InputNeutrons[i].Vector, RotMatrixNeut);

          norm = InputNeutrons[i].Wavelength*InputNeutrons[i].Wavelength * 1E-2 / (2.0*uc.volume);

          xsect_coherent = nxs_CoherentElastic(InputNeutrons[i].Wavelength, &uc );
          xsect_incoherent = nxs_IncoherentElastic(InputNeutrons[i].Wavelength, &uc ) +
            nxs_IncoherentInelastic(InputNeutrons[i].Wavelength, &uc ) +
            nxs_CoherentInelastic(InputNeutrons[i].Wavelength, &uc );
          xsect_absorption = nxs_Absorption(InputNeutrons[i].Wavelength, &uc );
          xsect_total = xsect_coherent + xsect_incoherent + xsect_absorption;
  

          /* Lbf is in [cm] already */
          p_transmit = exp( -xsect_total * mu_factor * Lbf );

          /* Handle transmission only (imaging mode) */
          if (bTransOnly)
          {
            /* make mu = 0 and prob = p_transmit to make sure that ProcessNeutronToEnd function works properly */
            MuTot = MuAbs = 0.0;
            ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, 1, p_transmit, 0.0, 0.0, &Sample, RotMatrixNeut, RotMatrixSmpl);
          }

          /* ...also handle scattering events */
          else
          {
            /* check if neutron transmits through or interacts with the sample */
            if( p_transmit > MonteCarlo(0, 1) )
            {
              /* TRANSMIT (no scattering) */
              /* make mu = 0 and prob = 1 to make sure that ProcessNeutronToEnd function works properly */
              MuTot = MuAbs = 0.0;
              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, 1, 1, 0.0, 0.0, &Sample, RotMatrixNeut, RotMatrixSmpl);
            }
            else
            {
              double roulette_ball = MonteCarlo(0, xsect_total);

              // SCATTER coherently
              // ------------------
              if (roulette_ball <= xsect_coherent)
              {
                double contrib;
                /* determine lattice plane (for scattering) */
                roulette_ball = MonteCarlo(0, xsect_coherent / norm);
                contrib = 0.0;
                for( j=0; j<uc.nHKL; j++ )
                {
                  contrib += uc.hklList[j].FSquare * uc.hklList[j].multiplicity * uc.hklList[j].dhkl;
                  if( roulette_ball < contrib )
                    break;
                }

                /* get scattering angle */
                ScTheta = 2.0*asin( InputNeutrons[i].Wavelength / 2.0 / uc.hklList[j].dhkl );
                if( ISNAN(ScTheta) )
                  ScTheta = M_PI;

                if (ScTheta > Theta-DelTheta && ScTheta < Theta+DelTheta)
                {
                  InputNeutrons[i].Color = (short)(nColor);
                  /* Bring the neutron several times on the cone */
                  for(iGen=0; iGen<GenNeutrons; iGen++)
                  {
                    /* ScPhi is the angle of the scattered neutron with the +y-axis */
                    ScPhi = MonteCarlo(Phi-DelPhi,Phi+DelPhi);

                    /* Ok, now everything needed is known, put it together */
                    /* in order to use ProcessNeutronToEnd set MuTot and MuAbs properly */
                    MuTot = xsect_total * mu_factor;
                    MuAbs = 0.0;
                    ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, 1.0, ScTheta, ScPhi,
                                        &Sample, RotMatrixNeut, RotMatrixSmpl);
                  }
                } /* end of if (ScTheta > Theta-DelTheta && ScTheta < Theta+DelTheta) */
              }

              // SCATTER incoherently 
              // --------------------
              else if (roulette_ball <= xsect_coherent+xsect_incoherent)
              {
                /* check the incoherent switch */
                if (bIncohScat)
                {
                  InputNeutrons[i].Color = (short)(nColor+1);
                  for(iGen=0; iGen<GenNeutrons; iGen++)
                  {
                    /* Determine the scattering angle */
                    ScPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
                    ScTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

                    /* in order to use ProcessNeutronToEnd set MuTot and MuAbs properly */
                    MuTot = xsect_total * mu_factor;
                    MuAbs = 0.0;
                    ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, 1.0/GenNeutrons,
                                        ScTheta, ScPhi, &Sample, RotMatrixNeut,  RotMatrixSmpl);
                  }
                } /* end of if (bIncohScat) */

              } /* end of if( roulette_ball <= xsect_coherent ) */

              // else /* ABSORPTION & and do not call WriteNeutron(&OutNeut) */

            } /* end of if( p_transmit < MonteCarlo(0, 1) ) */

          } /* end of if (bTransOnly) */
        }
        /* check if neutron passing the sample shall be treated */
        else if (bTreatAll==TRUE)
        {
          WriteNeutron(&InputNeutrons[i]);
        }
      }
    }
  }

  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
 my_exit:
  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup(StrucFac);

  /* Do the general cleanup */
  Cleanup(Sample.Position[0],Sample.Position[1],Sample.Position[2], 0.0,0.0);

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
  /*  -S sample geometry file                                          */
  /*  -D detector information       (default: 4*M_PI)                  */
  /*  -A Neutron repitition rate   (default: 10)                       */
  /*  -T Transmission only     (default: no = 0)                       */
  /*********************************************************************/

  long i=0;
  int  detectortest=0;

  /* some default values */
  Theta    = M_PI/2.0;
  DelTheta = M_PI/2.0;
  Phi      = M_PI;
  DelPhi   = M_PI;

  /* Ok, scan all command line parameters */
  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    { switch (argv[i][1])
      {
        case 'S':
          /* what is the sample file called? */
          pSmplFileName=&argv[i][2];
          break;
        case 's':
          pNxsFileNameI=&argv[i][2];
          break;

        /* get the solid angle covered by the detectors if other than 4PI */
        /* read four numbers            */
        case 'D':
          sscanf(&(argv[i][2]),"%lf", &Theta);
          Theta*=M_PI/180.0;
          /* Theta has to be in the range of [0;PI] */
          if (Theta < 0.0 || Theta > M_PI)
            Error("Theta has to be in the range of [0;PI] ");
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
          /* Phi has to be in the range of [0;2*PI] */
          if (Phi < 0.0 || Phi > 2.0*M_PI)
            Error("Phi has to be in the range of [0;2*PI] ");
          detectortest &= 0010L;
          break;
        case 'p':
          sscanf(&(argv[i][2]),"%lf", &DelPhi);
          DelPhi*=M_PI/180.0;
          detectortest &= 0001L;
          break;

        case 'A':
          sscanf(&(argv[i][2]),"%ld",&GenNeutrons);
          break;
        case 'c':
          nColor = (short) atoi(&argv[i][2]);
          break;
        case 'I':
          if(argv[i][2]=='1') bIncohScat=TRUE;
          break;
        case 'a':
          if(argv[i][2]=='1') bTreatAll=TRUE;
          break;
        case 'T':
          if(argv[i][2]=='1') bTransOnly=TRUE;
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

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if( (detectortest != 0) && (detectortest != 15))
  { Warning("You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI ");
    Theta    = M_PI/2.0;
    DelTheta = M_PI/2.0;
    Phi      = M_PI;
    DelPhi   = M_PI;
  }

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
/* End OwnCleanup */


/*******************************************************/
/** Reads sample parameters from file                 **/
/*******************************************************/
void SetSamplePar(SampleType *pSample)
{
  FILE  *pSampleFile;
  char   sLine[CHAR_BUF_SMALL]="", sGeom[20]="";
  int    nLen=sizeof(sLine)-1;
  double x=0.0, y=0.0, z=0.0, 
         xdir  =0.0, ydir  =0.0, zdir =0.0,
         d_par=0.0, height=0.0, width=0.0;
  VtSmplGeom geom=VT_NO_GEOM;
  SampleType sample;         // file  sample geometry

  InitSample(pSample);
  InitSample(&sample);

  /* Opens the parameter file if a file name is given */
  if (pSmplFileName!=NULL)
  { 
    pSampleFile = OpenInputFile(pSmplFileName, FALSE, "rt");

    /* Reads the parameters if the file can be opened */
    if (pSampleFile != NULL)
    { 
      /* First line: sample position     */
      if (ReadLine(pSampleFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x, &y, &z);
      if (ReadLine(pSampleFile, sLine, nLen)) sscanf(sLine, "%s",          sGeom); 
      if (ReadLine(pSampleFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &d_par, &height, &width);
      if (ReadLine(pSampleFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &xdir,  &ydir,  &zdir);
      if (ReadLine(pSampleFile, sLine, nLen)) sscanf(sLine, "%s",          sNxsFileNameF); 

      geom = SmplGeom_Txt2ID(sGeom);

      fclose(pSampleFile);

      // combines information from input and file, input parameters have priority
      if (eGeom==VT_NO_GEOM && geom!=VT_NO_GEOM) eGeom = geom; 
      if (Xpos    ==0.0 && x     !=0.0) Xpos    = x;
      if (Ypos    ==0.0 && y     !=0.0) Ypos    = y;
      if (Zpos    ==0.0 && z     !=0.0) Zpos    = z;
      if (Diameter==0.0 && d_par !=0.0)
      { if (eGeom==VT_CUBE) Diameter = d_par; else Diameter = 2.0 * d_par;}
      if (Height  ==0.0 && height!=0.0) Height  = height;
      if (Width   ==0.0 && width !=0.0) Width   = width;
      if (Xdir    ==0.0 && xdir  !=0.0) Xdir    = xdir;
      if (Ydir    ==0.0 && ydir  !=0.0) Ydir    = ydir;
      if (Zdir    ==0.0 && zdir  !=0.0) Zdir    = zdir;
    }
    else
    {	
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSmplFileName);
    }
  }

  // checks if geometry was given
  if (eGeom==VT_NO_GEOM)
    Error2("Sample geometry could not be identified", sGeom);

  // fills data structures
  FillSample(pSample, eGeom, Xpos, Ypos, Zpos, Xdir, Ydir, Zdir, Diameter, Height, Width, 0.0);

  /* the direction vector should have a positive z component  this will make things easier with the rotations later on */
  if(pSample->Direction[2] < 0)
  { pSample->Direction[0] = -pSample->Direction[0];
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

    SetSampleGeometry(&Sample);
  }
}
