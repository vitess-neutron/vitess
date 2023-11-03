/*********************************************************************************************/
/*  VITESS module  SAMPLE_ENVIRONMENT                                                        */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Nov 2008  K. Lieutenant   initial version                                            */
/* 1.1  Oct 2012  K. Lieutenant   only 1 Bragg reflection                                    */
/* 1.2  Apr 2020  K. Lieutenant   new central visualization parameters                       */
/* 1.3  Aug 2021  K. Lieutenant   option: parameters from input instead of from file         */
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
// Input parameters
char   sStructFileP[CHAR_BUF_XS]=""; // -s       [-]   structure factor file name from input
char*  pSampleFileName=NULL;         // -F       [-]   parameter file name (located in argv)
VtDir  eDirEnv  = VT_NO_DIR;         // -r       [-]   direction for sample environment: 1:'in' or 2:'out' 
short  nColor   = NO_COLOR;          // -c       [-]   colour of the neutrons scattered from the environment
double Theta    = M_PI/2.0,          //    fix  [deg]  solid angles into which scattering takes place
       DelTheta = M_PI/2.0,          //    fix  [deg]  Theta has to be in the range of [0;PI]         
       Phi      = M_PI,              //    fix  [deg]  Phi has to be in the range of [0;2*PI] 
       DelPhi   = M_PI;              //    fix  [deg]  
double Xpos     = 0.0,               // -x       [cm]  position of the center of the sample environment
       Ypos     = 0.0,               // -y       [cm]  
       Zpos     = 0.0,               // -z       [cm]  
       Thickness= 0.0,               // -t file  [cm]  thickness of the sample environment  (= R_out - R_in)
       Diameter = 0.0,               // -d file  [cm]  outer diameter of the sample environment  (= 2*R_out)
       Height   = 0.0,               // -h file  [cm]  outer height of the sample environment
       MuInc    = 0.0,               // -i file [1/cm] incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm]
       UCV      = 0.0;               // -U file [Ang^3] UCV 
                                    
extern double MuTot,                 // -T file [1/cm] macrosc. scattering cross section, defined in 'sample.c'
              MuAbs;                 // -m file [1/cm] macrosc. absorption cross section, defined in 'sample.c'
  
// Variables determined from input parameters or from file
char   sStructFileF[CHAR_BUF_XS]="", //    file  [-]   structure factor file name from parameter file
       sStrFileName[CHAR_BUF_XS]=""; //          [-]   structure factor file name used for the simulation
SampleType stEnvironment;            //                properties of the sample environment    
                               


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit          (int argc, char* argv[]);                                                  // reads input parameters and sets global parameters
void  OwnCleanup       (DoublePair* pStrucFac);                                                   // Does module specific cleanup
short SetEnvironPar    (SampleType* pEnvironment);                                                // Sets parameters of the sample environment
long  ReadStrucFact    (DoublePair* pStrucFac[]);                                                 // Reads structure factor file
long  ChooseBraggReflex(long* pNrefl, long Nstrcfac, DoublePair* pStrucFac[], double Wavelength); // chooses one of the possible Bragg reflection
void  SetGeometry      (char* sColor);                                                            // fills the structure stGeometry for visualization


/******************************/
/**   Main program           **/
/******************************/
int main(int argc, char **argv)
{
  int        i=0,j=0;
  long       nisp=0,              /* number of intersection points to come               */
             Nth =0,              /* index of the structure factor                       */
             Nrefl=0;             /* number of reflections for the given wavelength      */
  double     Lbf=0.0;             /* full path length of the neutron in the sample       */
  double     Ls =0.0;             /* distance of the neutron in the sample before scat.  */
  long       NumStrucFac=0;       /* number of reflections in the structure factor file  */
  double     DetFacCoh=0.0,       /* cares about the detector coverage                   */
             DetFacInc=0.0,       /*  for coherent and incoherent scattering             */
             HelpFac  =0.0;       /* contains k independent term of the scattering       */
  DoublePair *pStrucFac=NULL;
  Neutron    OutNeutron;
  double     ScProbCoh=0.0, ScProbF=0.0, /* probability for coherent, incoh. scattering and transmission */
             ScProbInc=0.0, ScProbT=0.0,
             ScTheta  =0.0,              /* scattering angles (in neutron coordinate system) */
             ScPhi   = 0.0;
  double     RotMatrixSmpl[3][3];        /* Rotation matrix that transforms a Vector to the */
  double     RotMatrixNeut[3][3];
  VectorType SP={0.0,0.0,0.0},                       /* position of scattering event                        */
             InISP[2]={{0.0,0.0,0.0},{0.0,0.0,0.0}}; /* neutron intersection with sample without scattering */
             
	// initialisation and reading of input data
  // ----------------------------------------
  InitNeutron(&OutNeutron);
  InitRotMatrix(RotMatrixSmpl);
  InitRotMatrix(RotMatrixNeut);

  _eModule = MCN_SMPL_ENVIRO;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");

  /* Module specific initialization and reading of sample geometry */
  OwnInit      (argc, argv);
  SetEnvironPar(&stEnvironment);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  /* Now get the nuclear unit-cell structure factors |f_N(t)|^2 */
  if (strlen(sStructFileP) > 0)                                    // try the file name from the input parameters first
    NumStrucFac = ReadStructureFile(sStructFileP, 1, &pStrucFac);  
  if (NumStrucFac > 0)                                             // structure factors found in file from input parameters
  { strcpy(sStrFileName, sStructFileP);                            
  }
  else                                   
  { if (strlen(sStructFileF) > 0)                                  // now the file given in the file
      NumStrucFac = ReadStructureFile(sStructFileF, 1, &pStrucFac);
    if (NumStrucFac > 0)                                           // structure factors found in file from given in the parameter file
    { strcpy(sStrFileName, sStructFileF); 
    }
    else
    { fprintf(LogFilePtr,"ERROR: Can't read the structure factor data, neither from %s nor from %s\n", sStructFileP, sStructFileF);
      exit(-1);
    }
  }

  /* Factors that take care of the detector coverage */
  DetFacCoh = DelPhi/M_PI;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample */
  /* vector pointing along the z-axis 				     */
  RotMatrixX(stEnvironment.Direction, RotMatrixSmpl);  // RotMatrixSmpl = OneMatrix 

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  /* Get the neutrons from the file */
  while((ReadNeutrons())!= 0)
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

          //   First the coherent scattering           
          //--------------------------------
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

          //  Then the incoherent scattering
          //--------------------------------
          if (MuInc > 0.0)
          { /* Determine the scattering angle and the probability */
            OutNeutron.Color = (short)(nColor+1);
            if (eDirEnv==VT_IN)  OutNeutron.ID.IDGrp[0]++;
            if (eDirEnv==VT_OUT) OutNeutron.ID.IDGrp[1]++;
     
            ScPhi     = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
            ScTheta   = MonteCarlo(Theta-DelTheta,Theta+DelTheta);
            ScProbInc = Lbf*MuInc * sin(ScTheta);

            ProcessNeutronToEnd(&OutNeutron, SP, Ls, DetFacInc, ScProbInc,
                                ScTheta, ScPhi, &stEnvironment, RotMatrixNeut,  RotMatrixSmpl);
          }

          // The transmitted neutrons  
          //-----------------------------
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
  }

  // Finish: write geometry and instrument file, free memory
  // -------------------------------------------------------
  my_exit:

  /* Write parameters to log file */
  if (stEnvironment.Type!=VT_HOL_CYL) 
    Error("Sample environment can only have the shape of a vertical hollow cylinder");

  fprintf(LogFilePtr, "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, total scat; absorption)\n"
                      "unit cell volume   : %8.3f Ang³\n"
                      "struct. factor file: %s\n",         MuInc, MuTot, MuAbs, UCV, sStrFileName);

  /* write geometry file */
  SetGeometry("cyan");
  
  /* Do module specific cleanups */
  OwnCleanup(pStrucFac);

  /* Do the general cleanup */
  Cleanup(stEnvironment.Position[0],stEnvironment.Position[1],stEnvironment.Position[2], 0.0,0.0);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[]) 
{
  int   i=0;

  for(i=1; i<argc; i++) 
  { 
    if(argv[i][0]!='+') 
    {	
      switch(argv[i][1])
      {	
        case 'F':
          pSampleFileName = &argv[i][2];
          break;
        case 's':
          strcpy(sStructFileP, &argv[i][2]);
          break;

        case 'r':
          eDirEnv = (VtDir) atoi(&argv[i][2]);   // 1:in   2: out
          break;
        case 'c':
          nColor = (short) atoi(&argv[i][2]);
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

        case 'h':
          Height = atof(&argv[i][2]);
          break;
        case 'd':
          Diameter = atof(&argv[i][2]);
          break;
        case 't':
          Thickness = atof(&argv[i][2]);
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

  /* if( (detectortest != 0) && (detectortest != 15))
  {	Warning("You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI");
  Theta    = M_PI/2.0;
  DelTheta = M_PI/2.0;
  Phi      = M_PI;
  DelPhi   = M_PI;
  } */
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup(DoublePair *pStrucFac)
{
  /* print error that might have occured many times */
  PrintMessage(ENV_TRAJ_INSIDE,  "", ON);
  PrintMessage(ENV_TRAJ_OUTSIDE, "", ON);
  PrintMessage(ALL_NEGATIVE_INT, "", ON);
  fprintf(LogFilePtr, "\n");

  /* Release the allocated memory */
  if (pStrucFac!=NULL)
    free(pStrucFac);

  return;
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  /* Geometry data */
  if (bVisInstr && eDirEnv==VT_OUT)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    SetSampleGeometry(&stEnvironment, 0.0);
  }
}


/*******************************************************/
/** Reads input parameters from file                  **/
/*******************************************************/
short SetEnvironPar(SampleType* pEnvironment)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="";
  int    nLen=sizeof(sLine)-1;
  double thickness=0.0, diameter=0.0, height=0.0,
         muInc=0.0, muTot=0.0, muAbs=0.0, ucv=0.0;

  InitSample(&stEnvironment);

  // Read parameter data
  if (pSampleFileName!=NULL)
  { pFile=OpenInputFile(pSampleFileName, FALSE, "rt");
    if (pFile != NULL)
    {
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &thickness, &diameter, &height);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sStructFileF); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &muInc, &muTot, &muAbs); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf",         &ucv);

      fclose(pFile);

      // combines information from input and file
      if (Height   ==0.0 && height   !=0.0) Height    =  height   ;
      if (Diameter ==0.0 && diameter !=0.0) Diameter  =  diameter ;
      if (Thickness==0.0 && thickness!=0.0) Thickness =  thickness;
      if (MuInc    ==0.0 && muInc    !=0.0) MuInc     =  muInc    ;
      if (MuTot    ==0.0 && muTot    !=0.0) MuTot     =  muTot    ;
      if (MuAbs    ==0.0 && muAbs    !=0.0) MuAbs     =  muAbs    ;
      if (UCV      ==0.0 && ucv      !=0.0) UCV       =  ucv      ;
    }
    else
    {	
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSampleFileName);
    }
  }

  FillSample(pEnvironment, VT_HOL_CYL,  Xpos, Ypos, Zpos,  0.0, 0.0, 1.0, Diameter, Height, 0.0, Thickness);

  return(TRUE);
}


/*******************************************************/
/** Reads structure factor file                       **/
/*******************************************************/
long ReadStrucFact(DoublePair* pStrucFac[])
{
  FILE  *pStrucFile=NULL;
  long  NumLines, i, j;
  char  sBuffer[CHAR_BUF_LENGTH];

  /* first try the file name from parameter input, then that from the parameter file */
  if (strlen(sStructFileP) > 0)
    pStrucFile = OpenInputFile(sStructFileP, FALSE, "rt"); 
  if (pStrucFile==NULL)
  { 
    if (strlen(sStructFileF) > 0)
      pStrucFile = OpenInputFile(sStructFileF, FALSE, "rt"); 
    if (pStrucFile==NULL)
    { 
      fprintf(LogFilePtr,"ERROR: Can't read the structure factor data, neither from %s nor from %s\n", sStructFileP, sStructFileP);
      exit(-1);
    }
  }

  /* count lines in file */
  NumLines = LinesInFile(pStrucFile);

  /* get memory for StrucFac */
  if((*pStrucFac = (DoublePair *)calloc(NumLines, sizeof(DoublePair)))==NULL)
  { 
    fprintf(LogFilePtr,"ERROR: Can't allocate memory for structure factor data\n");
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
  {	
    if((*pStrucFac)[j][0]!=(*pStrucFac)[j-1][0])
    { 
      i++;
      (*pStrucFac)[i][0]=(*pStrucFac)[j][0];
      (*pStrucFac)[i][1]=(*pStrucFac)[j][1];
    } 
    else
    { 
      (*pStrucFac)[i][1]+= (*pStrucFac)[j][1];
    }
  }
  NumLines = i+1;

  return NumLines;
}


/*********************************************************/
// chooses one of the possible Bragg reflection
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





    
