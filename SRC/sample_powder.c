/************************************************************************************************/
/*  VITESS module sample_powder                                                                 */
/*                                                                                              */
/* This program simulates the elastic coherent diffraction and incoherent scattering            */
/* of neutrons at a powder sample.                                                              */
/*                                                                                              */
/* The free non-commercial use of these routines is granted providing due credit is given to    */
/* the authors.                                                                                 */
/*                                                                                              */
/* 1.0  Jan 1999  F. Streffer                                                                   */
/* 1.1  Nov 2001  K. Lieutenant  absolute current values, SOFTABORT, corrections in             */
/*                               NeutronIntersectsSphere                                        */
/* 1.2  Jan 2002  K. Lieutenant  reorganisation                                                 */
/* 1.3  Jul 2002  K. Lieutenant  corr.: UCV reading; check: output dir. in [theta_min,theta_max]*/
/* 1.4  Jan 2004  K. Lieutenant  changes for 'instrument.dat', FullName() for struct.fac.file   */
/* 1.5  Feb 2004  K. Lieutenant  'FullParName', 'message' and 'ERROR' included; output extended */
/* 1.6  Nov 2008  K. Lieutenant  Corr. inc. scat., colour, treat neutrons not hitting the sample*/
/************************************************************************************************/

#include <string.h>

#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/

double Theta, DelTheta,     /* these angles determine orientation and solid angles covered by the detector */
       Phi, DelPhi;
double MuInc =  0.0,        /* incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] */
       UCV   = 50.0;        /* unit cell volume                                          */
short  nColor=NO_COLOR,     /* colour of the scattered neutrons                          */
       bIncohScat=FALSE,    /* shall incoherent scattering be done ?                     */
       bTreatAll =FALSE;    /* shall neutrons not hitting the sample be treated ?        */
long   GenNeutrons=1;       /* how many trajectories to generate per incoming trajectory for each structure factor */
char   *SampleFileName;     /* pointer to the parameter file name (located in argv)      */
double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};



/******************************/
/**   Extern Variables       **/
/******************************/

extern double g_fMuTot, g_fMuAbs;                   /* defined in 'sample.c' */



/******************************/
/** Prototypes               **/
/******************************/

void OwnInit           (int argc, char *argv[]);
void OwnCleanup        (DoublePair *StrucFac);
void GetSample         (SampleType *Sample, char *StrFileName);
long GetStructureFactor(char *StrFileName,  DoublePair *StrucFac[]);



/******************************/
/**   Program                **/
/******************************/

int main(int argc, char *argv[])
{
  SampleType Sample;               /* sample geometry */
  char       StrucFacFileName[200];/* structure factor file name     */
  long       NumStrucFac;          /* number of reflections in the structure factor */
  VectorType InISP[2];             /* neutron intersection before scattering */
  double     DetFacCoh,            /* cares about the detector coverage      */
             DetFacInc;            /* for coherent and incoherent scattering */
  double     Lbf;                  /* full path length of the neutron in the sample */
                                   /* with its initial direction */
  double     Ls;                   /* distance of the neutron in the sample before sc. */
  long       j;                    /* counting variable */
  VectorType SP;                   /* position of scattering event */
  double     HelpFac;              /* contains k independent term of the scattering */
  long       Nth;                  /* counting variable of structure factor */
  double     ScProb,               /* scattering probability */
             ScTheta,              /* scattering angles (in neutron coordinate system) */
             ScPhi;
  double     RotMatrixSmpl[3][3];  /* Rotation matrix that transforms a Vector to the sample coordinate system */
  double     RotMatrixNeut[3][3];
  long       nisp,                 /* number of intersection points to come       */
             i,                    /* counting index of the incoming trajectories */
             iGen;                 /* counting index of the generated trajectories (see 'GenNeutrons') */
  DoublePair *StrucFac=NULL;

  /* get several things done before programme start */
  /* which have actually nothing to do with physics */
  Init(argc, argv, VT_SMPL_POWDER);
  print_module_name("sample_powder 1.6b");
  OwnInit(argc, argv);

  /* Go and get the sample geometry and name of structure factor file */
  InitSample(&Sample);
  GetSample (&Sample, StrucFacFileName);
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
    default :;
  }
  fprintf(LogFilePtr, "  position         :(%7.2f,%7.2f,%7.2f ) cm\n"
                      "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, total scat; absorption)\n"
                      "unit cell volume   : %8.3f Ang³\n"
                      "struct. factor file: %s\n", 
                      Sample.Position [0], Sample.Position [1], Sample.Position [2], 
                      MuInc, g_fMuTot, g_fMuAbs, UCV, StrucFacFileName);

  /* Now get the nuclear unit-cell structure factors |f_N(t)|^2.       */
  /* The memory needed will be allocated inside 'GetStructureFactor()'.*/
  NumStrucFac = ReadStructureFile(StrucFacFileName, 1, &StrucFac);

  /* Factors that take care of the detector coverage */
  DetFacCoh = DelPhi/M_PI;
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
      /* First, shift the origin of the system to the middle of the sample   */
      SubVector(InputNeutrons[i].Position, Sample.Position);

      /* Do anything to be done for the Scattering */
      if (NeutronIntersectsSample(&(InputNeutrons[i]), &Sample, RotMatrixSmpl, InISP, &nisp, VT_IN))
      {
        if (nisp == 1)
          CountMessageID(SMPL_TRAJ_INSIDE, InputNeutrons[i].ID);

        /* the neutron may be scattered between InISP[0] and InISP[1] */
        /* Lfb full path length in the sample before scattering       */
        Lbf = DistVector(InISP[0], InISP[1]);

        /* MONTE CARLO CHOICE: Where is the neutron scattered         */
        /* Distance Ls between entrance of the neutron InISP[0] and   */
        /* the scattering point SP 				      */
        Ls = MonteCarlo(0, Lbf);

        /* which is the corresponding scattering point  	   */
        /*   SP = InISP[0] + Ls * OutNeutron.Vector		   */
        for(j=0; j<3; j++)
          SP[j] = InISP[0][j]+Ls*InputNeutrons[i].Vector[j];

        /* Determine the rotation matrix to point the neutron along the +x axis				      		    */
        NormVector(InputNeutrons[i].Vector);
        RotMatrixX(InputNeutrons[i].Vector, RotMatrixNeut);


        /************************************************/
        /* Now the actual scattering                    */
        /*   First take care of the coherent scattering */
        /*   Scatter at each suitable |F(k)|            */
        /************************************************/

        /* Helpfac contains the non direction dependent term                         */
        /* G.L. Squires, "Introduction to the theory of thermal neutron scattering", */
        /* (1978), equation (3.103)  (UCV is the unit cell volume)                   */
        HelpFac = Lbf*pow(InputNeutrons[i].Wavelength,3)/(4.0*UCV*UCV);

        if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
          InputNeutrons[i].Color = nColor;

        /* Do the scattering for each StrucFac */
        for(Nth=0; StrucFac[Nth][0] > 0.5*InputNeutrons[i].Wavelength && Nth < NumStrucFac; Nth++)
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
                                  ScTheta, ScPhi, &Sample, RotMatrixNeut, RotMatrixSmpl);
            }
          } 
        }  

        /************************************/
        /* Second the incoherent scattering */
        /************************************/
        if (bIncohScat)
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
                                ScTheta, ScPhi, &Sample, RotMatrixNeut,  RotMatrixSmpl);
          }
        }
      } // end 'NeutronIntersect...
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
  /*  -S sample geometry file	                                         */
  /*  -D detector information	     (default: 4*M_PI)                   */
  /*  -A Neutron repitition rate   (default: 10)                       */
  /*********************************************************************/

  long i;
  int  detectortest=0;

  colh = -1; colk = -1; coll = -1; colD = -1;
  colF = -1; colF2 = -1; colM = -1; colDW = -1;
  scaleF2 = 1.;
  
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
      /* read four numbers						*/
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
    sscanf(Buffer, "%lf %lf %lf", &(Sample->Position[0]), 
                                  &(Sample->Position[1]), 
                                  &(Sample->Position[2]));

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

        if(ReadTilComment(Buffer, SampleFile))
        { 
          sscanf(Buffer,"%lf %lf %lf", &MuInc, &g_fMuTot, &g_fMuAbs);
          /* Mua remains to be scaled by the neutron velocity     */

          if(ReadTilComment(Buffer, SampleFile))
          { 
            sscanf(Buffer,"%lf", &UCV);
            /* Seems as everything needed could be read             */

	     if(ReadTilComment(Buffer, SampleFile)) 
	       sscanf(Buffer,"%d %d %d %d %d", &colD, &colF, &colF2, &colDW, &colM);

          } 
          else 
          { fprintf(LogFilePtr, "ERROR: Can't read volume of a unit cell of %s", SampleFileName);
            exit(-1);
          }
        } 
        else 
        { fprintf(LogFilePtr, "ERROR: Can't read the scattering cross sections of %s", SampleFileName);
          exit(-1);
        }
      } 
      else 
      { fprintf(LogFilePtr, "ERROR: Can't read name of the structure factor file of %s", SampleFileName);
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



long GetStructureFactor(char *StrFileName, DoublePair *StrucFac[])
{
  FILE  *StrucFacFile;
  long  NumLines, i, j;
  char  sBuffer[CHAR_BUF_LENGTH];

  /* first open the file, add path if missing */
  StrucFacFile=fopen(FullParName(StrFileName),"rt"); 
  if(StrucFacFile==NULL)
  { fprintf(LogFilePtr,"ERROR: Can't read the structure factor data from %s\n", StrFileName);
    exit(-1);
  }

  /* count lines in file */
  NumLines = LinesInFile(StrucFacFile);

  /* get memory for StrucFac */
  if((*StrucFac = (DoublePair *)calloc(NumLines, sizeof(DoublePair)))==NULL)
  { fprintf(LogFilePtr,"ERROR: Can't allocate memory for structure factor data\n");
    exit(-1);
  }

  /* get back to the start of the File */
  rewind(StrucFacFile);

  /* and read the data */
  for(i=0; i < NumLines; i++)
  { ReadLine(StrucFacFile, sBuffer, sizeof(sBuffer)-1); 
    sscanf  (sBuffer, "%lf %lf", &((*StrucFac)[i][0]), &((*StrucFac)[i][1]));
  }
  qsort((void *)*StrucFac, (size_t) NumLines, sizeof(DoublePair), CompPair);
  fclose(StrucFacFile);

  /* Sum up all equal d-spacings */
  i=0;
  for(j=1; j<NumLines; j++)
    if((*StrucFac)[j][0]!=(*StrucFac)[j-1][0])
    { i++;
      (*StrucFac)[i][0]=(*StrucFac)[j][0];
      (*StrucFac)[i][1]=(*StrucFac)[j][1];
    } 
    else
    { (*StrucFac)[i][1]+= (*StrucFac)[j][1];
    }
  NumLines = i+1;

  return NumLines;
}



