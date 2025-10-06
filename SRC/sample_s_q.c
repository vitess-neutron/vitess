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
/* 1.5  Aug 2021  K. Lieutenant  option: parameters from input instead of from file           */
/* 1.5a Sep 2023  K. Lieutenant  correction: par. for spherical sample, visualisation improved*/
/**********************************************************************************************/

#include <string.h>

#include "convert.h"
#include "init.h"
#include "sample.h"
#include "softabort.h"
#include "matrix.h"
#include "sq_calc.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/
VtDataSrc eFunction=VT_NO_SRC;// -F file          source of the S(Q) function: F: analytical function   D: data from file
char  *pStrFileNameI=NULL,    // -s               pointer to S(Q) file name (from input parameter)
      *pSmplFileName=NULL;    // -S               pointer to the name of the sample file
short  bIncScat =FALSE,       // -I        [-]    shall incoherent scattering be done ?
       bTreatAll=FALSE,       // -a vsn4   [-]    shall neutrons not hitting the sample be treated ?
       nColor   =NO_COLOR;    // -c vsn4   [-]    colour of the scattered neutrons
long   GenNeutrons =1;        // -A        [-]    how many neutrons to generate on the "cone"
double Theta    = M_PI/2.0,   // -D       [deg]   these angles determine orientation and solid angles covered by the detector
       DelTheta = M_PI/2.0,   // -d       [deg]     Theta has to be in the range of [0;PI]
       Phi      = M_PI,       // -P       [deg]     Phi has to be in the range of [0;2*PI]
       DelPhi   = M_PI;       // -p       [deg]
                              //                  if Freq > 0.0, S(Q,t) = S(Q) 1/2 (1 + cos(2*pi*Freq*t + Offset))
double Freq     = 0.0,        // -f        [Hz]   modulation frequency of the sample response
       Offset   = 0.0;        // -o       [deg]   phase of the sample response at t=0
VtSmplGeom eGeomS=VT_NO_GEOM; // -G file   [-]    sample shape: VT_NO_GEOM, VT_CUBE, VT_CYL, VT_SPHERE, VT_HOL_CYL
double Xpos     = 0.0,        // -x file   [cm]   position of the center of the sample
       Ypos     = 0.0,        // -y file   [cm]
       Zpos     = 0.0,        // -z file   [cm]
       Diameter = 0.0,        // -t file   [cm]   thickness or diameter of the sample
       Height   = 0.0,        // -h file   [cm]   height of the sample
       Width    = 0.0,        // -w file   [cm]   width of the sample
       Xdir     = 0.0,        // -X file   [-]    orientation of the sample
       Ydir     = 0.0,        // -Y file   [-]
       Zdir     = 0.0;        // -Z file   [-]
extern
double MuAbs;                 // -m file [1/cm/Ang]  macrosc. absorption cross section, defined in 'sample.c'
double MuCoh    = 0.0,        // -C file  [1/cm]  coherent macroscopic scattering cross-section (= sigma_coh/UCV) [1/cm] */
       MuInc    = 0.0;        // -i file  [1/cm]  incoher. macroscopic scattering cross-section (= sigma_inc/UCV) [1/cm] */

// Variables determined from input parameters or from file
SampleType stSample;          //           [-]    sample geometry and position
long   nLinesStr= 0;          //           [-]    number of lines in the S(Q) file
extern
double MuTot;                 //          [1/cm]  macrosc. scattering cross section, defined in 'sample.c'
double *aSF=NULL,             //                  array of structure factors as a function of momentum transfer Q
       *aQ =NULL;             //                  corresponding array of momentum transfer data [1/Ang]
char  *pStrFileName="not found",      //   [-]    pointer to S(Q) file name that is used
       sStrFileNameF[CHAR_BUF_XS]=""; //          S(Q) file name from parameter file
double OneMatrix[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void   OwnInit     (int argc, char *argv[]);       // Reads input parameters and sets global
void   OwnCleanup  ();                             // Does module specific cleanup
void   SetSamplePar(SampleType *Sample);           // Sets sample parameters
int    LoadSofQFile(const char* pFileName);        // Reads file S(Q) into array
double GetStructureFactor (double dQ, long index); // Gets S(Q) from the structure factor file
double CalcStructureFactor(double dQ);             // Calculates S(Q) from an analytical function
void   SetGeometry        (char* sColor);          // fills the structure stGeometry for visualization


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char *argv[])
{
  VectorType InISP[2];       /* neutron intersection before scattering */
  double     qValue   =0.0,  /* absolute value of momentum transfer    */
             ThetaMin =0.0,  /* minimal and maximal values of the           */
             ThetaMax =0.0,  /* scattering angle according to Theta, DelTheta */
             neutTheta=0.0,
             neutPhi  =0.0;
  double     DetFacCoh=0.0,   /* cares about the detector coverage      */
             DetFacInc=0.0,   /* for coherent and incoherent scattering */
             Lbf=0.0;         /* full path length of the neutron in the sample with its initial direction */
  double     Ls =0.0,         /* flight path length of the neutron in the sample before scattering */
             Lbs=0.0;         /* flight path length of the neutron before scattering */
  long       j=0;             /* counting variable */
  VectorType SP={0.0,0.0,0.0};/* position of scattering event */
  double     ScTheta=0.0,     /* angle of coherent Scattering */
             ScProb =0.0,     /* scattering probability */
             ModFact=1.0,     /* Time modulation factor */
             TimeScat=0.0,    /* absolute time of scattering */
             OutTheta=0.0,    /* Final angles of the neutron in the sample system */
             OutPhi  =0.0;
  double     RotMatrixSmpl[3][3], /* Rotation matrix that transforms a Vector to the */
             RotMatrixNeut[3][3];
  /* sample coordinate system */
  long       i=0,         /* counting variable of the neutrons */
             nisp=0,      /* number of intersection points to come */
             NeutCount=0;
  Neutron    InNeutron;

  // initialisation
  // --------------
  InitNeutron(&InNeutron);
  InitRotMatrix(RotMatrixSmpl); InitVector(InISP[0]);
  InitRotMatrix(RotMatrixNeut); InitVector(InISP[1]),

  _eModule = MCN_SMPL_S_Q;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.5a");
  OwnInit   (argc, argv);

  InitSample  (&stSample);
  SetSamplePar(&stSample);

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp     = TRUE;

  /* Load file S(Q), if needed;  try the name from paramter input first, then the name from file */
  if (eFunction == VT_FR_FILE)
  {
    nLinesStr = LoadSofQFile(pStrFileNameI);
    if (nLinesStr == 0)
    { nLinesStr = LoadSofQFile(sStrFileNameF);
      if (nLinesStr == 0)
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
  }

  /* Factors that take care of the dectector coverage */
  DetFacCoh = DelPhi/M_PI*DelTheta;
  DetFacInc = DelPhi/M_PI*DelTheta;

  /* determine the rotation matrix to find new basis with the sample */
  /* vector pointing along the z-axis              */
  RotMatrixX(stSample.Direction, RotMatrixSmpl);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
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
        /* First, shift the origin of the system to the center of the sample */
        CopyNeutron(&InputNeutrons[i], &InNeutron);
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
          /* SP = InISP[0] + Ls*InputNeutrons[i].Vector     */
          for(j=0; j<3; j++)
            SP[j] = InISP[0][j] + Ls*InputNeutrons[i].Vector[j];


          //   First the coherent scattering
          //--------------------------------
          if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
            InputNeutrons[i].Color = nColor;

          /* determine Theta and Phi of the neutrons direction Theta should be small                             */
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
          /* original flight path              */
          OutTheta=ScTheta;

          /* ScProb corresponds to the sample form factor considering hard sphere scattering */
          switch (eFunction)
          {
            case VT_AS_FCT:
              ScProb *= CalcStructureFactor(qValue)*Lbf*MuCoh / GenNeutrons;
              break;
            case VT_FR_FILE:
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
          if (bIncScat)
          {
            if (nColor!=NO_COLOR && nColor!=ANY_COLOR)
              InputNeutrons[i].Color = (short)(nColor+1);

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
  fprintf(LogFilePtr, "macr. cross section: %10.5f,%10.5f,%10.5f  1/cm (incoh, coh scat; absorption)\n", MuInc, MuCoh, MuAbs);

  if (eFunction==VT_AS_FCT)
    fprintf(LogFilePtr, "Q-values calculated");
  else if (eFunction==VT_FR_FILE)
    fprintf(LogFilePtr, "Q-values from S(Q) file: %s\n", pStrFileName);

  if (Freq > 0.0)
    fprintf(LogFilePtr, "modulation         :%7.1f Hz %7.2f deg offset\n", Freq, Offset);
  else
    fprintf(LogFilePtr, "no modulation\n");

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
  long i=0;
  int  detectortest=0;

  /* Ok, scan all commandline parameters */
  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        /* sample and S(Q) file name */
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

        /* get the solid angle covered by the detectors if other than 4*PI */
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

        /* parameters for signal modulation */
        case 'f':
          sscanf(&(argv[i][2]),"%lf", &Freq);
          break;
        case 'o':
          sscanf(&(argv[i][2]),"%lf", &Offset);
          break;

        case 'G':
          eGeomS = (VtSmplGeom) atoi(&argv[i][2]);
          break;
        case 'F':
          eFunction = (VtDataSrc) atoi(&argv[i][2]);
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

        case 'i':
          MuInc = atof(&argv[i][2]);
          break;
        case 'C':
          MuCoh = atof(&argv[i][2]);
          break;
        case 'm':
          MuAbs = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"\nERROR: unkown command option: %s\n", argv[i]);
          exit(-1);
      }
    }
  }

  /* Check, whether all 4 angles are given; if not, initial values are set again */
  if ( detectortest!=0 && detectortest!=15)
  {
    Error("You have to specify -P,-p,-D,-d together in order to set the detector range.");
  /*  Theta   = M_PI/2.0;
    DelTheta= M_PI/2.0;
    Phi     = M_PI;
    DelPhi  = M_PI; */
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
  PrintMessage(SMPL_Q_RANGE_TOO_SMALL, pStrFileName, ON);
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
void SetSamplePar(SampleType *pSample)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="",
         sGeomS[20]="",        // string: sample shape
         sFct[2]   =" ";       // char  : source of S(Q) function
  int    nLen=sizeof(sLine)-1;
  double x    = 0.0, y     = 0.0, z    = 0.0,
         xdir = 0.0, ydir  = 0.0, zdir = 0.0,
         d_par= 0.0, height= 0.0, width= 0.0,
         muInc= 0.0, muCoh = 0.0, muAbs= 0.0;
  VtSmplGeom eGeo;           // enum  sample shape
  VtDataSrc  eFct;
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
      eGeo = SmplGeom_Txt2ID (sGeomS);
      if (eGeo!=VT_SPHERE)
      { if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &xdir,  &ydir,  &zdir);}
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          &sFct[0]);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sStrFileNameF);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &muInc, &muCoh, &muAbs);

      if (eGeo==VT_NO_GEOM)
        Error2("Sample geometry could not be identified", sGeomS);
      eFct = (VtDataSrc) sFct[0];
      //eFct = (VtDataSrc) cFct;
      if (eFct==VT_NO_SRC)
        Error2("Source of structure factor function could not be identified", sFct);

      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeomS   ==VT_NO_GEOM && eGeo!=VT_NO_GEOM) eGeomS    = eGeo;
      if (eFunction==VT_NO_SRC  && eFct!=VT_NO_SRC)  eFunction = eFct;
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
      if (MuInc   == 0.0 && muInc != 0.0) MuInc   = muInc;
      if (MuCoh   == 0.0 && muCoh != 0.0) MuCoh   = muCoh;
      if (MuAbs   == 0.0 && muAbs != 0.0) MuAbs   = muAbs;
      // if (pStrFileNameI==NULL && strlen(sStrFileNameF) > 0) pStrFileNameI=sStrFileNameF;
    }
    else
    {
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSmplFileName);
    }
  }

  /* Total macroscopic scattering cross-section */
  MuTot = MuCoh + MuInc;

  // checks if geometry and source of S(Q) were given
  if (eGeomS==VT_NO_GEOM)
    Error2("Sample geometry could not be determined", sGeomS);
  if (eFunction==VT_NO_SRC)
    Error2("Source of structure factor function could not be determined", sFct);

  /* Fills sample structure */
  FillSample(pSample, eGeomS, Xpos, Ypos, Zpos,  Xdir, Ydir, Zdir, Diameter, Height, Width, 0.0);

  /* the direction vector should have a positive z component  this will make things easier with the rotations later on */
  if(pSample->Direction[2] < 0)
  {
    pSample->Direction[0] = -pSample->Direction[0];
    pSample->Direction[1] = -pSample->Direction[1];
    pSample->Direction[2] = -pSample->Direction[2];
  }

  return;
}


/*********************************************************************/
/* 'CalcStructureFactor'                                             */
/* Calculate S(Q) from an analytical function            */
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
/* Get S(Q) from the structure factor file               */
/*********************************************************************/
double GetStructureFactor(double p_dQ, long index)
{
  long   n=-1;
  double dS=0.0, dSN, dSN1;

  while (n+1 < nLinesStr  &&  aQ[n+1] < p_dQ)
  {  n++;
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
/* load S(Q) file                                        */
/*********************************************************************/
int LoadSofQFile(const char* pFileName)
{
  FILE  *pStrFacFile=NULL;   //                  pointer to S(Q) file
  long   nLines= 0;          //           [-]    number of lines in the S(Q) file
  char sBuffer[CHAR_BUF_LENGTH];

  /* If there is a S(Q) file go and load the file */
  if (pFileName!=NULL && strlen(pFileName) >0)
  {
    /* opens distribution file */
    pStrFacFile = OpenInputFile(pFileName, FALSE, "rt");
    if (pStrFacFile!=NULL)
    {
      long   n;

      /* reads number of lines, allocates memory and then reads data */
      nLines = LinesInFile(pStrFacFile);
      aQ  = calloc(nLines, sizeof(double));
      aSF = calloc(nLines, sizeof(double));

      for(n=0; n<nLines; n++)
      {
        ReadLine(pStrFacFile, sBuffer, CHAR_BUF_LENGTH);
        sscanf  (sBuffer, "%lf %lf", &aQ[n], &aSF[n]);
      }

      /* closes trace file */
      fclose(pStrFacFile) ;
    }
    else
    {
      fprintf(LogFilePtr, "\nERROR: Can't open %s to read S(Q) file\n", pFileName);
      exit (-1);
    }
  }
  return nLines;
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
