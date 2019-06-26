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
/************************************************************************************************/

#include <string.h>
#include <math.h>

#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"

#include "nxs.h"             /* from nxs library */
#include "read_table-lib.h"  /* from McStas (version 1.12) but requires fix for FLT_MAX and va_list */

/******************************/
/**   Global Variables       **/
/******************************/
double Theta, DelTheta,     /* these angles determine orientation and solid angles covered by the detector */
       Phi, DelPhi;
//double DENSITY;      /* unit cell volume                                          */
short  MAX_HKL = 6;         /* maximum hkl index value                                   */
short  nColor=0,            /* colour of the scattered neutrons                          */
       bIncohScat = FALSE,  /* shall incoherent scattering be done ?                     */
       bTreatAll  = FALSE,  /* shall neutrons not hitting the sample be treated ?        */
       bTransOnly = FALSE;  /* shall only transmission (imaging) be treated (=TRUE)? Or scattering also(=FALSE)? */
long   GenNeutrons=1;       /* how many trajectories to generate per incoming trajectory */
char   *SampleFileName;     /* pointer to the parameter file name (located in argv)      */
double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};



/******************************/
/**   Extern Variables       **/
/******************************/
extern double g_fMuTot, g_fMuAbs;    /* defined in 'sample.c' */


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit           (int argc, char *argv[]);
void OwnCleanup        (DoublePair *StrucFac);
void GetSample         (SampleType *Sample, char *StrFileName);

int readParameterFile  (char *fileName, UnitCell *uc);


/******************************/
/**   Program                **/
/******************************/

int main(int argc, char *argv[])
{
  /* sample */
  UnitCell   uc;                   /* unit cell definitions (from nxs.h) */
  char       nxsFileName[200];     /* nxs parameter file name */

  SampleType Sample;               /* sample geometry */

  VectorType InISP[2];             /* neutron intersection before scattering */
  double     DetFacCoh,            /* cares about the detector coverage      */
    DetFacInc;                     /* for coherent and incoherent scattering */
  double     Lbf;                  /* full path length of the neutron in the sample */
                                   /* with its initial direction */
  double     Ls;                   /* distance of the neutron in the sample before sc. */
  long       j;                    /* counting variable */
  VectorType SP;                   /* position of scattering event */
  double     ScTheta,              /* scattering angles (in neutron coordinate system) */
    ScPhi;
  double     RotMatrixSmpl[3][3];  /* Rotation matrix that transforms a Vector to the sample coordinate system */
  double     RotMatrixNeut[3][3];
  long       nisp,                 /* number of intersection points to come       */
    i,                    /* counting index of the incoming trajectories */
    iGen;                 /* counting index of the generated trajectories (see 'GenNeutrons') */
  DoublePair *StrucFac=NULL;
  double mu_factor;

  /* get several things done before programme start */
  /* which have actually nothing to do with physics */
  Init(argc, argv, VT_SMPL_POWDER);
  print_module_name("sample_nxs 1.0a");
  OwnInit(argc, argv);

  /* Go and get the sample geometry and name of nxs parameter file */
  InitSample(&Sample);
  GetSample (&Sample, nxsFileName);

  switch (Sample.Type)
    { case VT_CUBE:
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
    default: break;
    }
  fprintf(LogFilePtr, "NXS parameter file: %s\n", nxsFileName);

  /* read unit cell parameters from file */
  readParameterFile( nxsFileName, &uc );
  fprintf(LogFilePtr, "NXS unit cell definition is:\n"
          "space group number = %s\n"
          "a = %f \t\t alpha = %f\n"
          "b = %f \t\t beta  = %f\n"
          "c = %f \t\t gamma = %f\n"
          "# label  b_coherent  sigma_inc  sigma_abs  molar_mass  debye_temp  x  y  z\n",
          uc.spaceGroup, uc.a, uc.alpha, uc.b, uc.beta, uc.c, uc.gamma);
  for(i=0; i<uc.nAtomInfo; i++) {
    AtomInfo ai = uc.atomInfoList[i];
    fprintf(LogFilePtr, "%s  %f  %f  %f  %f  %f  %f  %f  %f\n",
            ai.label, ai.b_coherent, ai.sigmaIncoherent, ai.sigmaAbsorption, ai.molarMass, ai.debyeTemp, ai.x[0], ai.y[0], ai.z[0]);
  }

  uc.maxHKL_index = MAX_HKL;
  initHKL( &uc );

  /* factor for the calculation of the attenuation */
  mu_factor = 1.0 / uc.volume;

  /* Factors that take care of the detector coverage */
  DetFacCoh = DelPhi/M_PI;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample */
  /* vector pointing along the z-axis              */
  RotMatrixX(Sample.Direction, RotMatrixSmpl);
  DECLARE_ABORT;

  /* Start the main loop getting neutrons         */
  while(ReadNeutrons()!= 0) {
    for(i=0; i<NumNeutGot; i++) {
      CHECK;
      /* First, shift the origin of the system to the middle of the sample   */
      SubVector(InputNeutrons[i].Position, Sample.Position);

      /* Do anything to be done for the Scattering */
      if (NeutronIntersectsSample(&(InputNeutrons[i]), &Sample, RotMatrixSmpl, InISP, &nisp, VT_IN))
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

          xsect_coherent = nxsCoherentElastic(InputNeutrons[i].Wavelength, &uc ) +
            nxsCoherentInelastic(InputNeutrons[i].Wavelength, &uc );
          xsect_incoherent = nxsIncoherentElastic(InputNeutrons[i].Wavelength, &uc ) +
            nxsIncoherentInelastic(InputNeutrons[i].Wavelength, &uc );
          xsect_absorption = nxsAbsorption(InputNeutrons[i].Wavelength, &uc );
          xsect_total = xsect_coherent + xsect_incoherent + xsect_absorption;
		

          /* Lbf is in [cm] already */
          p_transmit = exp( -xsect_total * mu_factor * Lbf );

          /* Handle transmission only (imaging mode) */
          if (bTransOnly)
            {
              /* make mu = 0 and prob = p_transmit to make sure that ProcessNeutronToEnd function works properly */
              g_fMuTot = g_fMuAbs = 0.0;
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
                  g_fMuTot = g_fMuAbs = 0.0;
                  ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, 1, 1, 0.0, 0.0, &Sample, RotMatrixNeut, RotMatrixSmpl);
                }
              else
                {
                  double roulette_ball = MonteCarlo(0, xsect_coherent+xsect_incoherent+xsect_absorption);

                  /**********************/
                  /* SCATTER coherently */
                  /**********************/
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
                              /* in order to use ProcessNeutronToEnd set g_fMuTot and g_fMuAbs properly */
                              g_fMuTot = xsect_total * mu_factor;
                              g_fMuAbs = 0.0;
                              ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacCoh, 1.0, ScTheta, ScPhi,
                                                  &Sample, RotMatrixNeut, RotMatrixSmpl);
                            }
                        } /* end of if (ScTheta > Theta-DelTheta && ScTheta < Theta+DelTheta) */
                    }

                  /************************/
                  /* SCATTER incoherently */
                  /************************/
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

                              /* in order to use ProcessNeutronToEnd set g_fMuTot and g_fMuAbs properly */
                              g_fMuTot = xsect_total * mu_factor;
                              g_fMuAbs = 0.0;
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

 my_exit:
  OwnCleanup(StrucFac);
  stPicture.eType = (short) Sample.Type;
  Cleanup(Sample.Position[0],Sample.Position[1],Sample.Position[2], 0.0,0.0);

  return 0;
}



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

  long i;
  int  detectortest=0;

  /* some default values */
  Theta    = M_PI/2.0;
  DelTheta = M_PI/2.0;
  Phi      = M_PI;
  DelPhi   = M_PI;

  /* Ok, scan all command line parameters */
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    { switch(argv[i][1])
      {
      case 'S':
        /* what is the sample file called? */
        SampleFileName=&argv[i][2];
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


/* cleanup of this module */
/* ---------------------- */
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



int readParameterFile( char *fileName, UnitCell *uc )
{
  t_Table dataTable;
  int i;
  char **parsing;

  int column_label = 1;
  int column_b_coherent = 2;
  int column_sigma_inc = 3;
  int column_sigma_abs = 4;
  int column_molar_mass = 5;
  int column_debye_temp = 6;
  int column_x = 7;
  int column_y = 8;
  int column_z = 9;

  AtomInfo ai;
  /* setup a default Fe unit cell */
  uc->spaceGroup = "229";
  uc->a = uc->b = uc->c = 2.866;
  uc->alpha = uc->beta = uc->gamma = 90.0;

  /* read the file */
  Table_Read(&dataTable, FullParName(fileName), 1); /* read 1st block data from file into table */

  /* parsing of header for sample parameters */
  parsing = Table_ParseHeader(dataTable.header,
    "space_group","lattice_a","lattice_b","lattice_c","lattice_alpha","lattice_beta","lattice_gamma",NULL);
		
  if (parsing)
  {
    if (parsing[0]) uc->spaceGroup = &parsing[0][strspn(parsing[0], " ")];
      else {
        fprintf(LogFilePtr, "Error: No value for 'space_group' found in file %s\n", fileName);
        return -1;
      }
    if (parsing[1]) uc->a = atof(parsing[1]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_a' found in file %s\n", fileName);
        return -1;
      }
    if (parsing[2]) uc->b = atof(parsing[2]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_b' found in file %s\n", fileName);
      }
    if (parsing[3]) uc->c = atof(parsing[3]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_c' found in file %s\n", fileName);
      }
    if (parsing[4]) uc->alpha = atof(parsing[4]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_alpha' found in file %s\n", fileName);
      }
    if (parsing[5]) uc->beta = atof(parsing[5]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_beta' found in file %s\n", fileName);
      }
    if (parsing[6]) uc->gamma = atof(parsing[6]);
      else {
        fprintf(LogFilePtr, "Error: No value for 'lattice_gamma' found in file %s\n", fileName);
      }

  } else {
    fprintf(LogFilePtr, "Error: parsing file %s - using default values\n", fileName);
  }

  initUnitCell( uc );

  /* check if data exist (at least 9 columns) */
  if (dataTable.columns < 9)
  {

    fprintf(LogFilePtr, "Error: The number of columns in %s should be at least %d for "
            "[label,b_coherent,sigma_inc,sigma_abs,molar_mass,debye_temp,x,y,z] - using default values\n", fileName, 9);

    ai.label = "Fe";
    ai.b_coherent = 9.45;
    ai.sigmaIncoherent = 0.4;
    ai.sigmaAbsorption = 2.56;
    ai.molarMass = 55.85;
    ai.debyeTemp = 464.0;
    ai.x[0] = 0.0;
    ai.y[0] = 0.0;
    ai.z[0] = 0.0;
    addAtomInfo( uc, ai );
  }
  else
  {
    /* parsing of header for column assignment */
    parsing = Table_ParseHeader(dataTable.header,
                                "column_label","column_b_coherent","column_sigma_inc","column_sigma_abs",
                                "column_molar_mass","column_debye_temp",
                                "column_x","column_y","column_z",NULL);

    /* assign columns */
    if (parsing)
    {
      if (parsing[0]) column_label = atoi(parsing[0]);
      if (parsing[1]) column_b_coherent = atoi(parsing[1]);
      if (parsing[2]) column_sigma_inc = atoi(parsing[2]);
      if (parsing[3]) column_sigma_abs = atoi(parsing[3]);
      if (parsing[4]) column_molar_mass = atoi(parsing[4]);
      if (parsing[5]) column_debye_temp = atoi(parsing[5]);
      if (parsing[6]) column_x = atoi(parsing[6]);
      if (parsing[7]) column_y = atoi(parsing[7]);
      if (parsing[8]) column_z = atoi(parsing[8]);
      for (i=0; i<9; i++)
        if (parsing[i])
          free(parsing[i]);
      free(parsing);
    }

    for( i=0; i<dataTable.rows; i++ )
    {
      ai.label = "unknown";//Table_Index( dataTable, i, column_label-1 );
      ai.b_coherent = Table_Index( dataTable, i, column_b_coherent-1 );
      ai.sigmaIncoherent = Table_Index( dataTable, i, column_sigma_inc-1 );
      ai.sigmaAbsorption = Table_Index( dataTable, i, column_sigma_abs-1 );
      ai.molarMass = Table_Index( dataTable, i, column_molar_mass-1 );
      ai.debyeTemp = Table_Index( dataTable, i, column_debye_temp-1 );
      ai.x[0] = Table_Index( dataTable, i, column_x-1 );
      ai.y[0] = Table_Index( dataTable, i, column_y-1 );
      ai.z[0] = Table_Index( dataTable, i, column_z-1 );
      addAtomInfo( uc, ai );
    }
  }
  return 0;
}



void GetSample(SampleType *Sample, char *StrFileName)
{
  FILE *SampleFile;
  char Buffer[CHAR_BUF_LENGTH];

  if((SampleFile=fopen(FullParName(SampleFileName),"rt"))==NULL)
  { fprintf(LogFilePtr,"ERROR: Cannot open sample file %s\n", SampleFileName);
    exit(-1);
  }

  /* Read the file */
  if(ReadTilComment(Buffer, SampleFile))
  { /* first line: sample position     */
    sscanf(Buffer, "%lf %lf %lf", &(Sample->Position[0]), &(Sample->Position[1]), &(Sample->Position[2]));

    /* Next line should describe the type of geometry cylinder, cube, ball */
    if(ReadTilComment(Buffer, SampleFile))
    {
      if(strstr(Buffer, "cyl")!=NULL)
      { ReadCylinder(SampleFile, Sample);
        Sample->Type=VT_CYL;
      }
      else if(strstr(Buffer, "cub")!=NULL)
      { ReadCube(SampleFile, Sample);
        Sample->Type=VT_CUBE;
      }
      else if(strstr(Buffer, "bal")!=NULL)
      { ReadBall(SampleFile, Sample);
        Sample->Type=VT_SPHERE;
      }
      else
      { fprintf(LogFilePtr, "ERROR: Please denote the sample geometry by cyl, cub or bal in the second line of %s\n", SampleFileName);
        exit(-1);
      }

      /* the direction vector should have a positive z component  */
      /* this will make things easier with the rotations later on */
      if(Sample->Direction[2] < 0)
      { Sample->Direction[0] = -Sample->Direction[0];
        Sample->Direction[1] = -Sample->Direction[1];
        Sample->Direction[2] = -Sample->Direction[2];
      }

      /* Sample Geometry is read */
      if(ReadTilComment(Buffer, SampleFile))
      {
        sscanf(Buffer, "%s", StrFileName);

        // if(ReadTilComment(Buffer, SampleFile))
        // {
      // sscanf(Buffer,"%lf", &DENSITY);
        // }
        // else
        // { fprintf(LogFilePtr, "ERROR: Can't read volume of a unit cell of %s", SampleFileName);
            // exit(-1);
        // }
      }
      else
      { fprintf(LogFilePtr, "ERROR: Can't read name of the nxs parameter file of %s", SampleFileName);
        exit(-1);
      }
    }
    else
    { fprintf(LogFilePtr, "ERROR: Can't read second line of %s", SampleFileName);
      exit(-1);
    }
  }
  else
  { fprintf(LogFilePtr, "ERROR: Can't read first line of %s", SampleFileName);
    exit(-1);
  }
  fclose(SampleFile);
}

