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
/* 1.3  Apr 2020  K. Lieutenant   new central visualization parameters                        */
/* 1.4  Apr 2020  K. Lieutenant   modulation of sample response                               */
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
McCompID _eModule=MCN_SMPL_S_Q;

FILE  *pStrFacFile=NULL;      //            pointer to structure factor file
char   sStrucFileName[200],   // file       structure factor file name  
       cFunction = ' ',       // file       parameter of the S(Q) function: F: analytical function   D: data from file 
       *SampleFileName;       // -S         pointer to the name of the sample file  
short  bIncohScat=FALSE;      // -I   [-]   should incoherent scattering be done
long   GenNeutrons =1,        // -A   [-]   how many neutrons to generate on the "cone" 
       nLinesStr =0;          //      [-]   number of lines in the structure factor file 
double Theta    = M_PI/2.0,   // -D  [deg]  these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,   // -d  [deg]     Theta has to be in the range of [0;PI]         
       Phi      = M_PI,       // -P  [deg]     Phi has to be in the range of [0;2*PI] 
       DelPhi   = M_PI;       // -p  [deg]
                              //            if Freq > 0.0, S(Q,t) = S(Q) 1/2 (1 + cos(2*pi*Freq*t + Offset))
double Freq     =  0.0,       // -f  [Hz]   modulation frequency of the sample response
       Offset   =  0.0,       // -o  [deg]  phase of the sample response at t=0 
      *aSF=NULL,              //            array of structure factor data as a function of mom. transfer Q
      *aQ =NULL;              //            corresponding array of momentum transfer data [1/Ang]          
extern 
double MuTot,                 // calc       macrosc. scattering cross section, defined in 'sample.c'
       MuAbs;                 // file       macrosc. absorption cross section, defined in 'sample.c'
double MuCoh    = 0.0,        // file       coherent macroscopic scattering cross-section (= sigma_coh/UCV) [1/cm] */
       MuInc    = 0.0;        // file       incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] */
SampleType stSample;          // file       sample geometry

double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void   OwnInit   (int argc, char *argv[]);         // reads input parameters and sets global 
void   OwnCleanup();                               // Does module specific cleanup
void   GetSample (SampleType *Sample);             // Reads sample parameters from file
void   LoadStrucFactFile();                        // Reads structure factor file into array
double GetStructureFactor (double dQ, long index); // Gets structure factor from the structure factor file 
double CalcStructureFactor(double dQ);             // Calculates structure factor from an analytical function
void   SetGeometry        (char* sColor);          // fills the structure stGeometry for visualization


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char *argv[])
{
  VectorType InISP[2];    /* neutron intersection before scattering */
  double     qValue,      /* absolute value of momentum transfer    */
             ThetaMin,   /* minimal and maximal values of the           */
             ThetaMax,   /* scattering angle according to Theta, DelTheta */
             neutTheta,
             neutPhi;
  double     DetFacCoh,   /* cares about the detector coverage      */
             DetFacInc,   /* for coherent and incoherent scattering */
             Lbf;         /* full path length of the neutron in the sample */
  /* with its initial direction */
  double     Ls,          /* flight path length of the neutron in the sample before scattering */
             Lbs=0.0;     /* flight path length of the neutron before scattering */
  long       j;           /* counting variable */
  VectorType SP;          /* position of scattering event */
  double     ScTheta,     /* angle of coherent Scattering */
             ScProb,      /* scattering probability */
             ModFact=1.0, /* Time modulation factor */
             TimeScat=0.0,/* absolute time of scattering */
             OutTheta,    /* Final angles of the neutron in the sample system */
             OutPhi;
  double     RotMatrixSmpl[3][3], /* Rotation matrix that transforms a Vector to the */
             RotMatrixNeut[3][3];
  /* sample coordinate system */
  long       i,           /* counting variable of the neutrons */
             nisp,        /* number of intersection points to come */
  NeutCount;

  // initialisation
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.4");

  InitSample(&stSample);
  OwnInit   (argc, argv);
  GetSample (&stSample);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* Load structure factor file, if needed */
  if (cFunction == 'D')
    LoadStrucFactFile();

  /* Factors that take care of the dectector coverage */
  DetFacCoh = DelPhi/M_PI*DelTheta;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample */
  /* vector pointing along the z-axis 				     */
  RotMatrixX(stSample.Direction, RotMatrixSmpl);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;

      /* First, shift the origin of the system to the center of the sample */
      SubVector(InputNeutrons[i].Position, stSample.Position);

      /* Test if the Neutron hits the stSample */
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

				
        //   First the coherent scattering           
        //--------------------------------
        /* determine Theta and Phi of the neutrons direction */
        /* Theta should be small                             */
        NormVector          (InputNeutrons[i].Vector);
        CartesianToSpherical(InputNeutrons[i].Vector, &neutTheta, &neutPhi);

        /* Determine the rotation matrix to point the neutron along the +x axis */
        RotMatrixX(InputNeutrons[i].Vector,RotMatrixNeut);

        /* Determine min. and max. scattering angle (due to the detector coverage */
        ThetaMin = Theta-DelTheta;
        ThetaMax = Theta+DelTheta;

        CHECK

        // Choose the scattering angle and calculate Q-value 
        ScTheta = MonteCarlo(ThetaMin, ThetaMax);
        qValue  = 4.0*M_PI*sin(ScTheta/2.0)/InputNeutrons[i].Wavelength;

        // Determine the probability for this scattering angle
        ScProb  = sin(ScTheta);

        /* OutTheta is the angle of the scattered neutron with its */
        /* original flight path 				     */
        OutTheta=ScTheta;

        /* ScProb corresponds to the sample form factor considering hard sphere scattering */
        switch (cFunction)
        {
          case 'F': 
            ScProb *= CalcStructureFactor(qValue)*Lbf*MuCoh / GenNeutrons;
            break;
          case 'D': 
            ScProb *= GetStructureFactor (qValue,i)*Lbf*MuCoh / GenNeutrons;
            break;
          default:
            ScProb = 0.0; 
        }

        /* Response modulation */
        if (Freq > 0.0)
        { 
          /* flight path inside this module until scattering and absolute time of scattering */
          Lbs      = DistVector(InputNeutrons[i].Position, SP);                 
          TimeScat = (InputNeutrons[i].Time + Lbs / V_FROM_LAMBDA(InputNeutrons[i].Wavelength)) / 1000.0; // [ms] -> s
          ModFact  = 0.5*(1.0 + cos(2*M_PI*Freq*TimeScat + Radians(Offset)));
          ScProb  *= ModFact;
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
                              OutTheta, OutPhi, &stSample, RotMatrixNeut, RotMatrixSmpl);
        }
				
        // Second the incoherent scattering 
        //--------------------------------
        if (bIncohScat)
        { 
          for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++) 
          {
            /* Determine the scattering angle */
            OutPhi    = MonteCarlo(Phi  -DelPhi,  Phi  +DelPhi);
            OutTheta  = MonteCarlo(Theta-DelTheta,Theta+DelTheta);

            /* Scattering probability */
            ScProb = Lbf*MuInc * sin(OutTheta)/GenNeutrons;

            ProcessNeutronToEnd(&(InputNeutrons[i]), SP, Ls, DetFacInc, ScProb,
                                OutTheta, OutPhi, &stSample, OneMatrix,  RotMatrixSmpl);
          }
        }
      }
    }
  }

  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
 my_exit:

  /* Write parameters to log file */
  switch (stSample.Type)
  {	
    case VT_CUBE: 
      fprintf(LogFilePtr, "Cubic sample, sizes: %7.2f,%7.2f,%7.2f   cm  (thickness, height, width)\n"
                          "  direction        :(%8.3f,%7.3f,%7.3f)   \n",
      stSample.SG.Cube.thickness, stSample.SG.Cube.height, stSample.SG.Cube.width,
      stSample.Direction[0], stSample.Direction[1], stSample.Direction[2]);
      break;
    case VT_CYL: 
      fprintf(LogFilePtr, "Cylindrical sample : %7.2f cm radius%6.2f cm height\n"
                          "  direction        :(%8.3f,%7.3f,%7.3f)   \n",
      stSample.SG.Cyl.r, stSample.SG.Cyl.height,
      stSample.Direction[0], stSample.Direction[1], stSample.Direction[2]);
      break;
    case VT_SPHERE: 
      fprintf(LogFilePtr, "Spherical sample   : %7.2f cm radius\n", 
      stSample.SG.Ball.r);
      break;
    default :;
  }
  fprintf(LogFilePtr, "  position         :(%7.2f,%7.2f,%7.2f ) cm\n"
                      "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, coh scat; absorption)\n",
                      stSample.Position [0], stSample.Position [1], stSample.Position [2], MuInc, MuCoh, MuAbs);

  if (Freq > 0.0)
    fprintf(LogFilePtr, "  modulation       :%7.1f Hz %7.2f deg offset\n", Freq, Offset);
  else
    fprintf(LogFilePtr, "  no modulation\n");

  if (cFunction=='F')
  	fprintf(LogFilePtr, "Q-values calculated");
  else
    fprintf(LogFilePtr, "Q-values from structure factor file: %s\n", sStrucFileName);

  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  Cleanup(stSample.Position[0],stSample.Position[1],stSample.Position[2], 0.0,0.0);

  return 0;
}


/*********************************************************************/
/* Here we will set some global variables to get things going        */
/* If there is no sample specification the program is aborted        */
/* Known commandline parameters:                                     */
/*  -A     Neutron repetition rate on the cone    (default: 1)       */
/*  -I     flag: treat incoherent scattering      (default: FALSE)   */
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
          if(argv[i][2]=='1') bIncohScat=TRUE;
          break;

        /* Repetition rate  */
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

        /* parameters for signal modulation                                */
        case 'f':
          sscanf(&(argv[i][2]),"%lf", &Freq);
          break;
        case 'o':
          sscanf(&(argv[i][2]),"%lf", &Offset);
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
  MuTot = MuCoh + MuInc;

  /* Check, whether all 4 angles are given; if not, initial values are set again */	
  if ( detectortest!=0 && detectortest!=15) 
  {
    Error("You have to specify -P,-p,-D,-d together in order to set the detector range.\n The detector range is reset to 4*PI ");
    Theta   = M_PI/2.0;
    DelTheta= M_PI/2.0;
    Phi     = M_PI;
    DelPhi  = M_PI;
  }

  /* Theta has to be in the range of [0;PI] */
  if ((Theta-DelTheta < 0.0) || (Theta+DelTheta > M_PI)) 
    Error("Theta has to be in the range of [0;PI]");

  /* Phi has to be in the range of [0;2*PI] */
  if ((Phi-DelPhi < 0.0) || (Phi+DelPhi > 2.0*M_PI)) 
    Error("Phi has to be in the range of [0;2*PI]");

  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  /* print error that might have occured many times */
  PrintMessage(SMPL_Q_RANGE_TOO_SMALL, sStrucFileName, ON);
  PrintMessage(SMPL_TRAJ_INSIDE, "", ON);

  fprintf(LogFilePtr," \n");

  /* free allocated memory */
  if (aQ!=NULL)  free(aQ);
  if (aSF!=NULL)  free(aSF);
}


/*********************************************************************/
/* 'GetSample'                                                       */
/* Read data of the sample (position, geometry, size, orientation)   */
/*********************************************************************/
void GetSample(SampleType *pSample)
{
  FILE* pSampleFile;
  char Buffer[CHAR_BUF_LENGTH];

  /* open file */
  pSampleFile=OpenInputFile(SampleFileName, FALSE, "rt");
  if (pSampleFile==NULL) 
  {
    fprintf(LogFilePtr,"ERROR: Cannot open sample file %s\n", SampleFileName);
    exit(-1);
  }
  else
  {
    /* read data, if file could be opened */
    if (ReadTilComment(Buffer, pSampleFile)) 
    {
      sscanf(Buffer, "%lf %lf %lf", &(pSample->Position[0]), 
                                    &(pSample->Position[1]), 
                                    &(pSample->Position[2]));
    } 
    else 
    {	
      fprintf(LogFilePtr, "ERROR: Can't read first line of %s",SampleFileName);
      exit(-1);
    }

    /* Next line should decribe the type of geometry cylinder, cube, sphere    */
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
    } 
    else 
    {
      fprintf(LogFilePtr, "ERROR: Can't read one of the lines 2 - 4 of %s",SampleFileName);
      exit(-1);
    }
    /* Read criterion for getting S(Q) data */
    if(ReadTilComment(Buffer, pSampleFile))
    {	
      sscanf(Buffer, "%c", &cFunction);
      /*	Allowed char. for function parameter: 
      F: analytic function  D: function data from file   */
      if(cFunction!='F' && cFunction!='D')  
      Error("Wrong character for function to determine structure factor");
    }
    else 
    {	
      fprintf(LogFilePtr, "ERROR: Can't read criterion for getting S(Q) data %s", SampleFileName);
      exit(-1);
    }
    /* Read structure factor file */
    if(ReadTilComment(Buffer, pSampleFile))
    {	
      sscanf(Buffer, "%s", sStrucFileName);
    }
    else 
    {	
      if (cFunction == 'D')
      {	
        fprintf(LogFilePtr, "ERROR: Can't read structure factor file name %s",SampleFileName);
        exit(-1);
      }
    }
    /* Read macroscopic cross sections */
    if(ReadTilComment(Buffer, pSampleFile)) 
    {
      sscanf(Buffer,"%lf%lf%lf", &MuInc, &MuCoh, &MuAbs);
    } 
    else 
    {	
      fprintf(LogFilePtr, "ERROR: Can't read cross sections %s",SampleFileName);
      exit(-1);
    }

    /* Seems as everything needed could be read */
    fclose(pSampleFile);
  }
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
/* 'GetStructureFactor'                                             */
/* Get structure factor from the structure factor file               */
/*********************************************************************/
double GetStructureFactor(double p_dQ, long index)
{
  long   n=-1;
  double dS=0.0, dSN, dSN1;
	
  while (n+1 < nLinesStr  &&  aQ[n+1] < p_dQ) 
  {	n++;
  }

  if (n >= 0 && n+1 < nLinesStr)
  { /* linear  extrapolation */
    dSN  = aSF[n];
    dSN1 = aSF[n+1];
    dS   = dSN + (dSN1-dSN ) / (aQ[n+1]-aQ[n]) * (p_dQ-aQ[n]);
  }
  else
  { /* read error: Q-value smaller or larger than all values in the distribution file */
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
  if (sStrucFileName!=NULL) 
  {
    /* opens distribution file */
    pStrFacFile = OpenInputFile(sStrucFileName, FALSE, "rt");
    if (pStrFacFile!=NULL) 
    {
      long   n;

      /* reads number of lines, allocates memory and then reads data */
      nLinesStr = LinesInFile(pStrFacFile);
      aQ  = calloc(nLinesStr, sizeof(double));
      aSF = calloc(nLinesStr, sizeof(double));

      for(n=0; n<nLinesStr; n++)
      {  
        ReadLine(pStrFacFile, sBuffer, CHAR_BUF_LENGTH);
        sscanf  (sBuffer, "%lf %lf", &aQ[n], &aSF[n]);
      }

      /* closes trace file */
      fclose(pStrFacFile) ;
    } 
    else 
    {	
      fprintf(LogFilePtr, "\nERROR: Can't open %s to read structure factor file\n", sStrucFileName);
      exit (-1);
    }
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

     SetSampleGeometry(&stSample);
  }
}
