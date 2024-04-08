/*********************************************************************************************/
/*  VITESS module 'sample_refl.c'                                                            */
/*    This module simulates a sample of a reflectometer                                      */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given    */
/* to the authors.                                                                           */
/*                                                                                           */
/* 1.0  Dec  2001  K. Lieutenant  initial version                                            */
/* 1.1  Feb? 2002  K. Lieutenant  average of reflectivity value built in logarithmic scale   */
/* 1.2  Jul? 2002  K. Lieutenant  storing of reflectivity data                               */
/* 2.0  Jan  2002  K. Lieutenant  reorganisation                                             */
/* 2.1  Jul  2003  K. Lieutenant  correction time-of-flight calculation                      */
/* 2.2  Jan  2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 2.3  Jan  2004  K. Lieutenant  reduced output, only one angle, sign of angle changed      */
/*                                new message for 'Q not in range given by file'             */
/* 2.4  Feb  2004  K. Lieutenant  'FullParName' and 'ERROR' included                         */
/*      Aug  2012  M. Fromme      clean up, variable definition a block start                */
/* 3.1	Mar  2013  D. Nekrassov   Calculation of scattering process takes place in separate  */
/*                                functions, offspecular scattering added                    */
/* 3.2  Nov  2019  K. Lieutenant  tidy up, global variable, visualization , length <-> width */
/* 3.3  May  2022  K. Lieutenant  substrate, neutrons passing by, attenuation                */
/* 3.4  Sep  2023  K. Lieutenant  substrate visualization                                    */
/*********************************************************************************************/

#include <string.h>
#include <stdio.h>

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "sample_reflectom.h"
#include "matrix.h"
#include "message.h"
#include "sample.h"
#include "bender_inter_data.h"
#include "convert.h"


/**************************************************/
/* global variables and constants                 */
/**************************************************/
char  *pSmplFileName=NULL;         // -P               name of the sample parameter file  
char  *sReflFileName=NULL;         // -I               name of the file containing the theoretical spectrum  
VtMeasMode eOption =VT_SAMPLE;     // -O               option:  1: reflection of sample       2: reflection of reference  
VtRotAxis  eSmplAxis=NO_ROT_AX;    // -R               rotation axis of sample "Y" or"Z"            
VtMirrMat  eSubMat=VT_NO_MIRR_MAT; // -M               material of the substrate  
short  bIncoh   =FALSE,            // -B               flag: whether to use incoherent  scattering: 0 for "not use", 1 "for use" 
       bOffSpec =FALSE,            // -o               flag: whether to use offspecular scattering: 0 for "not use", 1 "for use"
       bTreatAll=FALSE;            // -A               flag: treat neutrons that are not reflected on the sample surface 
double SmplAngle=0.0,              // -a        [deg]  angle of reflection 
       SubThick =0.0,              // -h        [cm]   thickness of the substrate
       SubMuScaT=0.0,              // -s       [1/cm]  Macroscopic total scattering cross section of the substrate 
       SubMuAbs =0.0,              // -m       [1/cm]  Macroscopic absorption cross section of the substrate for 1.798 Ang 
       MuInc =0.0,                 // -X       [1/cm]  Macroscopic incoherent cross section 
       DetDist  =0.0,              // -d        [cm]   Distance to the detector for solid angle calculation 
       DetWidth =0.0,              // -p        [cm]   Width of the detector for solid angle calculation 
       DetHeight=0.0,              // -t        [cm]   Height of the detector for solid angle calculation 
       SignToBkgAreaFact=0.0;      // -S               Relates the area on the detector with signal counts to total detector area. 
                                                       
VtFrameGen eFrame=VT_NO_FRAME;// file -g               flag: user defined output frame
double PosSmpl[3]={0.0,0.0,0.0},// file -x -y -z  [cm]   center position X of the sample
       SmplThick =0.0,        // file -T        [cm]   thickness of (the reflecting area) of the sample 
       SmplWidth =0.0,        // file -W        [cm]   width of the sample
       SmplLength=0.0,        // file -L        [cm]   length of the sample
       AnglOutHor =0.0,       // file -k        [deg]  horizontal angle of the output frame, relative to input orientation
       AnglOutVert=0.0,       // file -K        [deg]  vertical angle of the output frame, relative to input orientation
       TranslOut[3]           // file -u -v -w  [cm]   center position of the output frame
          ={0.0,0.0,0.0};      

// Variables determined from input parameters or trajectory data
FILE	*pReflFile=NULL;        //       pointer on file for theoretical spectrum 
SampleType stSample,          //       sample geometry
           stSubstr;          //       substrate geometry
long   nLinesRefl=0,          //       number of lines in reflectivity file  
       nQinPoints=0;          //       number of lines in a specular reflectivity file or number of Qin points in a offspecular reflectivity file.  
double *pTabQ,                //       pointer on table of Q-values                   
       *pTabR,                //       pointer on table of the respective R-values   
      **pTab_Qin_Qout,        //       pointer on table of Qin and Qout values for offspecular scattering 
      **pTab_RoffSpec,        //       pointer on table of the respective offspecular Q-values  
       DimSmpl[3]={0.0,0.0,0.0},//     size (thickness, width, length) of the sample
       DimSub [3]={0.0,0.0,0.0},//     size (thickness, width, length) of sample+substrate
       PosSub [3]={0.0,0.0,0.0},//     position of the center of sample+substrate
       TotThick=0.0,          //       thickness of sample + substrate 
       ProbIn =0.0,           //       input probabilities for one angle   
       ProbOut=0.0,           //       output probabilities for one angle   
       SmplHor=0.0,           //       horizontal angle of reflection 
       SmplVert=0.0,          //       vertical angle of reflection   
       MinTheta=0.0,          //       Minimum and maximum theta angles for the incoherent scattering 
       MaxTheta=0.0,
       Qmin    =1.0e99,       //       Minimum and maximum theta Q-values in the table
       Qmax   =-1.0e99,
       Depth[3]={0.0,0.0,0.0};//       Depth of reflection point inside the reflecting sample
double RotMatrixSmpl[3][3],   //       rotation matrix for sample orientation
       RotMatrixOut [3][3];   //       rotation matrix for output frame
double RotMatOffSpec[3][3];   //       rotation matrix for offspeclar scattering
char   sAxis[2]="";           //       text describing sample axis
 

/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char **argv)
{
  double     arg=0.0, ReflAnglHor=0.0, ReflAnglVert=0.0,
             RotMatrixRefl[3][3],
             pathlen=0.0,          // length of the path inside the sample
             tof  =0.0, dist =0.0, // TOF and distance to point of reflection (for visualization)
             dist1=0.0, dist2=0.0; // distances to intersections points with the substrate
  long       i=0;                  // index of trajectories
  short      bHitSmpl=FALSE,       // flag: sample is hit
             bHitSub =FALSE;       // flag: substrate is hit
  VectorType vPath={0.0,0.0,0.0}, vDirIn={1.0, 0.0, 0.0}, vDirOut={0.0,0.0,0.0},
             ISP1 ={0.0,0.0,0.0}, ISP2  ={0.0,0.0,0.0}; 
  Neutron    ReflNeutron, TrnsNeutron, InNeutron;
  Neutron    parentNeutron;     // Neutron needed to store the location of the intersection 
                                // point for offspecular/incoherent scattering
  short bScat=0;
  short doCoherent=0, doOffspecular=0, doIncoherent=0;

  // initialisation
  // --------------
  InitNeutron(&ReflNeutron);  InitNeutron(&parentNeutron);
  InitNeutron(&TrnsNeutron);  InitNeutron(&InNeutron);
  InitRotMatrix(RotMatrixRefl);

  _eModule = MCN_SMPL_REFL;

  Init   (argc, argv, _eModule);
  PrintModuleName(_eModule, "3.3a");
  OwnInit(argc, argv);
  MsgInit();

  if (eOption==VT_SAMPLE)
    ReadReflectivityFile();

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp     = TRUE;

  /* reads file containing sample parameters */
  SetSamplePar(); // ReadParameterFile() ;

  /* determines the dependent parameters and write out important parameters */
  CalcAndWritePar();

  /* computes rotation matrixes corresponding to sample offset angles */
  FillRotMatrixZY(RotMatrixSmpl, SmplVert, SmplHor) ;

  /* In reflection measurements the scattering angle must be determined from the orientation
     of the sample (SmplHor, SmplVert) */
  AnglesOutputFrame(180/M_PI*SmplHor, 180/M_PI*SmplVert, &ReflAnglHor, &ReflAnglVert) ;
  FillRotMatrixZY  (RotMatrixRefl, M_PI/180.*ReflAnglVert, M_PI/180.*ReflAnglHor) ;

  CopyVector(vDirIn, vDirOut) ;
  RotVector (RotMatrixRefl, vDirOut) ;
  SubVector (vDirOut, vDirIn) ;
  arg = LengthVector(vDirOut)/2.0 ;    /* arg is sin(scattering angle) */
  
  DECLARE_ABORT;

  doOffspecular = bOffSpec && (eOption==VT_SAMPLE);
  doCoherent    = TRUE     && (!doOffspecular);
  doIncoherent  = bIncoh   && (eOption==VT_SAMPLE);
  
  // loop over all trajectories
  // --------------------------
  /* Get the neutrons from the file */
  while(ReadNeutrons() != 0)
  {
    /* Loop over all neutrons read */
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
        /* copies and normalizes input data for neutron reflection, transmission and visualization */
        CopyNeutron(&InputNeutrons[i], &InNeutron);
        NormVector(InNeutron.Vector);
        CopyNeutron(&InNeutron, &ReflNeutron);	  	  
        CopyNeutron(&InNeutron, &TrnsNeutron);	  	  
	  
        ProbIn  = InputNeutrons[i].Probability ;
        ProbOut = 0.0;	      
	      
        /* checks if neutron hits the surface of the sample */
        bHitSmpl = FindISPs(ISP1, ISP2, &ReflNeutron, FALSE);
	  
        if (bHitSmpl == TRUE) /* neutron hits the sample surface */
        {
          /* moment of arriving at the sample plane (x=0.0), new position */
          dist = (0.0 - ReflNeutron.Position[0]) / ReflNeutron.Vector[0];
          tof  = dist / V_FROM_LAMBDA(ReflNeutron.Wavelength);
          ReflNeutron.Time +=  tof;
          CopyVector(ReflNeutron.Vector, vPath) ;
          MultiplyByScalar(vPath, dist);
          AddVector  (ReflNeutron.Position, vPath) ; /* vPath = displacement vector */
	  
          // Save the neutron with primary direction and weight for offspecular scattering
          CopyNeutron(&ReflNeutron, &parentNeutron);	  	  
	    
          // Here, the specular reflection case is treated
          if (doCoherent) 
          {
            bScat = ScatterSpecular(arg, &InNeutron, &ReflNeutron);
            if (bScat != 0)
            {	    
              // Here, the incoherent scattering is treated
              if (doIncoherent) 
              {
                ReflNeutron = InNeutron;
                /* checks if neutron hits the surface of the sample and gives global variables in	the frame of sample */
                CheckRefl(&ReflNeutron, 1) ;

                /* moment of arriving at the sample plane (x=0.0), new position */
                ReflNeutron.Time += (0.0 - ReflNeutron.Position[0]) / ReflNeutron.Vector[0] / V_FROM_LAMBDA(ReflNeutron.Wavelength)/**/ ;
                CopyVector(ReflNeutron.Vector, vPath) ;  /* vPath = displacement vector */
	  
                MultiplyByScalar(vPath, - ReflNeutron.Position[0] / ReflNeutron.Vector[0] ) ;
                AddVector  (ReflNeutron.Position, vPath) ;

                pathlen = DistVector(ISP1, ISP2);
                ScatterIncoherent(&ReflNeutron, pathlen);
              }
            }
          }
          // Here, the offspecular scattering is treated
          else if (doOffspecular) 
          {
            ScatterOffspecular(arg, &InNeutron, &parentNeutron, &ReflNeutron);
          }
        }
        
        // transmitted neutron
        if (bTreatAll == TRUE )
        { 
          bHitSub = FindSubISPs(&dist1, &dist2, &TrnsNeutron);
          if (bHitSub==TRUE)
          { PassThrough(&TrnsNeutron, dist1, VT_MIRR_VACUUM, FALSE);
            PassThrough(&TrnsNeutron, dist2-dist1,  eSubMat, TRUE);
          }
          else
          { PassThrough(&TrnsNeutron, LengthVector(PosSmpl), VT_MIRR_VACUUM, TRUE);
          }
        }
        else
        { WriteDIAP(&InNeutron, VT_OUTSIDE, PosSmpl[0] - InNeutron.Position[0]); 
        }
      }
    }  // end loop trajectories
  }
  
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
 my_exit:

  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHor, AnglOutVert);

  return 0;
}


/*******************************************************/
/** Own initialization of the sample_reflectom module **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  char *arg = NULL;

  /* Initialize */
  pTabQ         = NULL;
  pTabR         = NULL;
  pTab_Qin_Qout = NULL;
  pTab_RoffSpec = NULL;
  SignToBkgAreaFact = 1.0;

  InitRotMatrix(RotMatrixSmpl);
  InitRotMatrix(RotMatrixOut);
  InitRotMatrix(RotMatOffSpec);
  InitSample(&stSample);
  InitSample(&stSubstr);

  /*    INPUT  */
  while(argc>1)
  {
    arg=&argv[1][2];
    switch(argv[1][1])
    {
      case 'P':
        pSmplFileName=arg;
        break;
      case 'I':
        sReflFileName=arg;
        break;

      case 'O':
        eOption = (VtMeasMode) atoi(arg) ;
        break;
      case 'M':
        eSubMat = (VtMirrMat) atoi(arg) ;
        break;
      case 'R':
        eSmplAxis = RotAxis_Txt2ID(arg) ;
        break;

      case 'B':
        bIncoh   = (short) atoi(arg);
        break;
      case 'o':
        bOffSpec = (short) atoi(arg);
        break;
      case 'A':
        bTreatAll = (short) atoi(arg);
        break;

      case 'a':
        SmplAngle = atof(arg);
        break;
      case 'h':
        SubThick = atof(arg);
        break;

      case 'X':
        MuInc  = atof(arg);
        break;
      case 'm':
        SubMuAbs  = atof(arg);
        break;
      case 's':
        SubMuScaT = atof(arg);
        break;

      case 'x':
        PosSmpl[0] = atof(arg);
        break;
      case 'y':
        PosSmpl[1] = atof(arg);
        break;
      case 'z':
        PosSmpl[2] = atof(arg);
        break;

      case 'T':
        SmplThick = atof(arg);
        break;	
      case 'W':
        SmplWidth = atof(arg);
        break;
      case 'L':
        SmplLength = atof(arg);
        break;

      case 'p':
        DetWidth = atof(arg);
        break;	
      case 't':
        DetHeight = atof(arg);
        break;
      case 'd':
        DetDist = atof(arg);
        break;

      case 'S':	
        SignToBkgAreaFact = atof(arg);
        break;

      /* Output frame */
      case 'g':
        eFrame = (VtFrameGen) atoi(arg);
        break;

      case 'u':
        TranslOut[0] = atof(arg);
        break;
      case 'v':
        TranslOut[1] = atof(arg);
        break;
      case 'w':
        TranslOut[2] = atof(arg);
        break;

      case 'k':
        AnglOutHor  = atof(arg);
        break;
      case 'K':
        AnglOutVert = atof(arg);
        break;
    }
    argc--;
    argv++;
  }

  TotThick = SmplThick + SubThick;
}


/********************************************************************/
/** calculates arrays from input parameters and writes to log file **/
/********************************************************************/
void  CalcAndWritePar()
{
  char sMode[11]="", sMat[15]="";

  /* prints to log file */
  RotAxis_ID2Txt (sAxis, eSmplAxis);
  MeasMode_ID2Txt(sMode, eOption);
  MirrMat_ID2Txt (sMat,  eSubMat);

  fprintf  (LogFilePtr, "initialised option: '%s'\n", sMode) ;
  fprintf  (LogFilePtr, "sample rotated by : %10.5f° about %s-axis\n", SmplAngle, sAxis) ;
  if (pSmplFileName!=NULL)
    fprintf(LogFilePtr, "parameter file    : '%s'\n", pSmplFileName) ;
  if (bIncoh)
  { fprintf(LogFilePtr, "incoherent scattering added\n");
    fprintf(LogFilePtr, "mu_inc            : %10.5f  1/cm \n", MuInc);
  }
  if (bTreatAll)
  { fprintf(LogFilePtr, "substrate         : %10s  %10.5f  cm thick\n", sMat, SubThick);
    fprintf(LogFilePtr, "  mu_scat, mu_abs : %10.5f  %10.5f  1/cm\n", SubMuScaT, SubMuAbs);
  }

  /* converts degs in radian etc. */
  SmplAngle  *= M_PI/180. ;
  AnglOutHor *= M_PI/180. ;
  AnglOutVert*= M_PI/180. ;

  if (bIncoh) CalculateThetaRange();

  /* computes rotation matrix corresponding to the output frame
  (focus direction) */
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHor) ;

  /* determines rotation angles in horiz. and vert. direction */
  if (eSmplAxis==VT_ROT_Z)
  {  
    SmplHor = M_PI_2 + SmplAngle;
    SmplVert  = 0.0;
  }
  else if (eSmplAxis==VT_ROT_Y)
  {  
    SmplHor = 0.0;
    SmplVert  = M_PI_2 + SmplAngle;
  }
  else
  {  
    fprintf(LogFilePtr,"ERROR: wrong value for rotation axis: %s\n", sAxis);
    exit(0);
  }

  // fills the structure to visualize the sample
  stSample.Type = VT_CUBE;
  stSample.Position[0] = PosSmpl[0];
  stSample.Position[1] = PosSmpl[1];
  stSample.Position[2] = PosSmpl[2];

  if (SmplHor  > 0) stSample.Direction[1] = tan(SmplHor  - M_PI_2);
  if (SmplVert > 0) stSample.Direction[2] = tan(SmplVert - M_PI_2);
  stSample.Direction[0] = 1.0 - sqrt(sq(stSample.Direction[1]) + sq(stSample.Direction[1]));

  if (eSmplAxis==VT_ROT_Y)
  {
    stSample.SG.Cube.thickness = DimSmpl[2];
    stSample.SG.Cube.width     = DimSmpl[1];
    stSample.SG.Cube.height    = DimSmpl[0];
  }
  else if (eSmplAxis==VT_ROT_Z)
  {
    stSample.SG.Cube.thickness = DimSmpl[1];
    stSample.SG.Cube.width     = DimSmpl[0];
    stSample.SG.Cube.height    = DimSmpl[2];
  }
  else
  {
    Error2("wrong value for rotation axis:", sAxis);
  }

  // fills the structure to visualize the substrate
  memcpy(&stSubstr, &stSample, sizeof(SampleType));
  stSample.Position[0] = PosSub[0];
  stSample.Position[1] = PosSub[1];
  stSample.Position[2] = PosSub[2];
  if (eSmplAxis==VT_ROT_Y)
    stSample.SG.Cube.height = DimSub[0];
  else if (eSmplAxis==VT_ROT_Z)
    stSample.SG.Cube.width  = DimSub[0];

  return;
}/* End OwnInit */


/*******************************************************/
/** ReadReflectivityFile() reads the function R(Q)    **/
/*******************************************************/
void ReadReflectivityFile()
{
  short n;
  char  c1, Buffer[CHAR_BUF_LENGTH];

  if (sReflFileName!=NULL)
  {
    // open reflectivity file
    pReflFile=OpenInputFile(sReflFileName, FALSE, "rt");
    if (pReflFile==NULL)
    {
      fprintf(LogFilePtr,"ERROR: reflection file '%s' not found!\n", sReflFileName);
      exit(0);
    }
    else
    {
      nLinesRefl = LinesInFile(pReflFile);
      if (bOffSpec == 0) 
      {
        /* reads number of lines, allocates memory and then reads the specular reflectivity file */
        //	    nLinesRefl = LinesInFile(pReflFile);
        pTabQ      = calloc(nLinesRefl, sizeof(double));
        pTabR      = calloc(nLinesRefl, sizeof(double));
        for(n=0; n<nLinesRefl; n++)
        { ReadLine(pReflFile, Buffer, CHAR_BUF_LENGTH);
          sscanf  (Buffer,"%le%c%le", &pTabQ[n], &c1, &pTabR[n]);
          Qmin = fmin(Qmin, pTabQ[n]);
          Qmax = fmax(Qmax, pTabQ[n]);
        }
      }
      /* reads the offspecular reflectivity file (q_i, q_f,ij, R) */
      else 
      {
        double q_i = 0;
        double q_f = 0;
        double refl = 0;
        int numColumnsFound = -1;
        unsigned int innerCounter = 0;
        unsigned int outerCounter = 0;
        double q_i_prev = 0;
        double* q_f_array = calloc(nLinesRefl, sizeof(double));
        double* refl_array = calloc(nLinesRefl, sizeof(double));
        int i;

        pTab_Qin_Qout = calloc(nLinesRefl, sizeof(double*));
        pTab_RoffSpec = calloc(nLinesRefl, sizeof(double*));

        while (!feof(pReflFile)) 
        {
          numColumnsFound = fscanf(pReflFile, "%le %le %le", &q_i, &q_f, &refl);

          if (numColumnsFound < 2) 
          {
            fgets(Buffer, CHAR_BUF_LENGTH, pReflFile);
            continue;
          }
	      
          if (innerCounter == 0 && outerCounter == 0) q_i_prev = q_i;
		
          q_f_array[innerCounter] = q_f;
          refl_array[innerCounter] = refl;

          if (q_i_prev != q_i) 
          {
            pTab_Qin_Qout[outerCounter] = calloc(innerCounter+2, sizeof(double));
            pTab_RoffSpec[outerCounter] = calloc(innerCounter, sizeof(double));
            pTab_Qin_Qout[outerCounter][0] = (double) innerCounter;
            pTab_Qin_Qout[outerCounter][1] = (double) q_i_prev;

            for (i = 0; i < innerCounter; i++) 
            {
              pTab_Qin_Qout[outerCounter][i+2] = q_f_array[i];
              pTab_RoffSpec[outerCounter][i] = refl_array[i];
            }
		
            innerCounter=0;
            outerCounter++;
            q_i_prev = q_i;
		
          }
          else 
          {
            q_i_prev = q_i;
            innerCounter++;
          }

        }

        if (innerCounter > 0) {
        pTab_Qin_Qout[outerCounter] = calloc(innerCounter+2, sizeof(double));
        pTab_RoffSpec[outerCounter] = calloc(innerCounter, sizeof(double));
        pTab_Qin_Qout[outerCounter][0] = (double) innerCounter;
        pTab_Qin_Qout[outerCounter][1] =  q_i_prev;
	      
        for (i = 0; i < innerCounter; i++) {
        pTab_Qin_Qout[outerCounter][i+2] = q_f_array[i];
        pTab_RoffSpec[outerCounter][i] = refl_array[i];
        }
	      
        outerCounter++;
	      
        }
        nQinPoints = outerCounter;

        free (q_f_array);
        free (refl_array);
      }

      fclose(pReflFile) ;
    }
  }
  else
  {
    fprintf(LogFilePtr,"ERROR: no reflection file name given!\n");
    exit(0);
  }

  return;
}


/****************************************************************/
/** Reads sample parameters and combines with input parameters **/
/****************************************************************/
void SetSamplePar()
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="";
  int    nLen=sizeof(sLine)-1, iFrm=-1;
  double x     =0.0,  y   =0.0, z     =0.0, 
         thickn=0.0, width=0.0, length=0.0, 
         out_x =0.0, out_y=0.0, out_z =0.0, 
         out_h =0.0, out_v=0.0;
  VtFrameGen eFrm=VT_NO_FRAME; 

  /* Opens the parameter file if a file name is given */
  if (pSmplFileName!=NULL)
  { 
    pFile = OpenInputFile(pSmplFileName, FALSE, "rt");

    /* Reads the parameters if the file can be opened */
    if (pFile != NULL)
    { 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x,      &y,     &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &thickn, &width, &length);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%d",          &iFrm); 
      eFrm = (VtFrameGen) iFrm; 
      if (eFrm==VT_FRAME_USER)
      { 
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf",         &out_h);
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf",         &out_v);
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &out_x,  &out_y,  &out_z);
      }

      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eFrame==VT_NO_FRAME  && eFrm!=VT_NO_FRAME) eFrame = eFrm; 
      if (PosSmpl[0]  ==0.0 && x     !=0.0) PosSmpl[0]  = x     ;
      if (PosSmpl[1]  ==0.0 && y     !=0.0) PosSmpl[1]  = y     ;
      if (PosSmpl[2]  ==0.0 && z     !=0.0) PosSmpl[2]  = z     ;
      if (SmplThick   ==0.0 && thickn!=0.0) SmplThick   = thickn;
      if (SmplWidth   ==0.0 && width !=0.0) SmplWidth   = width ;
      if (SmplLength  ==0.0 && length!=0.0) SmplLength  = length;
      if (TranslOut[0]==0.0 && out_x !=0.0) TranslOut[0]= out_x ;
      if (TranslOut[1]==0.0 && out_y !=0.0) TranslOut[1]= out_y ;
      if (TranslOut[2]==0.0 && out_z !=0.0) TranslOut[2]= out_z ;
      if (AnglOutHor  ==0.0 && out_h !=0.0) AnglOutHor  = out_h ;
      if (AnglOutVert ==0.0 && out_v !=0.0) AnglOutVert = out_v ;
    }
    else
    {	
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", pSmplFileName);
    }
  }

  // fills the data structures giving position and size of both the sample and the substrate in original frame, with the sample normal pointing in x-direction
  CopyVector(PosSmpl, PosSub);
  DimSmpl[0] = SmplThick;  DimSub[0] = SubThick;  

  CopyVector(PosSmpl, PosSub);
  if (eSmplAxis==VT_ROT_Y)
  { PosSub [0] = PosSmpl[0] + 0.5*(SmplThick+SubThick)*sin(Radians(SmplAngle));
    PosSub [2] = PosSmpl[2] - 0.5*(SmplThick+SubThick)*cos(Radians(SmplAngle));
    DimSmpl[1] = SmplWidth;  DimSub[1] = SmplWidth; 
    DimSmpl[2] = SmplLength; DimSub[2] = SmplLength;
  }
  else if (eSmplAxis==VT_ROT_Z)
  { PosSub [0] = PosSmpl[0] + 0.5*(SmplThick+SubThick)*sin(Radians(SmplAngle));
    PosSub [1] = PosSmpl[1] - 0.5*(SmplThick+SubThick)*cos(Radians(SmplAngle));
    DimSmpl[1] = SmplLength; DimSub[1] = SmplLength;
    DimSmpl[2] = SmplWidth;  DimSub[2] = SmplWidth; 
  }

  return;
}

/* void ReadParameterFile()
{
  // opens file containing sample parameters (program exit in case of error)
  FILE* pSmplFile = OpenInputFile2(pSmplFileName, "sample data", "r");

  // reads from file by using ReadParF(pSmplFile) and ReadParComment(pSmplFile)
  PosSmpl[0]=ReadParF(pSmplFile); PosSmpl[1]=ReadParF(pSmplFile); PosSmpl[2]=ReadParF(pSmplFile); ReadParComment(pSmplFile) ;
  DimSmpl[0]=ReadParF(pSmplFile); DimSmpl[1]=ReadParF(pSmplFile); DimSmpl[2]=ReadParF(pSmplFile); ReadParComment(pSmplFile) ;

  eFrame = ReadParI(pSmplFile);     ReadParComment(pSmplFile) ;

  if (eFrame == VT_FRAME_USER)
  // some input data for user defined output frame 
  {
    AnglOutHor=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
    AnglOutVert =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;

    TranslOut[0]=ReadParF(pSmplFile) ; TranslOut[1]=ReadParF(pSmplFile) ; TranslOut[2]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  }
  else
  // sets default values if frame for output not user defined 
  {
    // standard output frame is got by a shift along the x-axis (without change in direction) 
    AnglOutHor = 0.0;
    AnglOutVert  = 0.0;

   // shifts output frame origin to center of focussing geometry
    CopyVector(PosSmpl, TranslOut) ;
  }

  // prints parameters into log file for verification 
  fprintf(LogFilePtr,"  main position X, Y, Z    = %9.4f, %9.4f, %9.4f\n"
                     "  thickness, width, height = %9.4f, %9.4f, %9.4f\n"
                     "  angle                    = %9.4f around %s-Axis \n",
                     PosSmpl[0], PosSmpl[1], PosSmpl[2],  DimSmpl[0], DimSmpl[1], DimSmpl[2],  SmplAngle, sAxis);

  if (eFrame == VT_FRAME_USER)
  {	
    fprintf(LogFilePtr,"user defined frame:\n") ;
    fprintf(LogFilePtr,"  horizontal angle = %9.4f\n"
    "  vertical angle   = %9.4f\n"
    "  X',Y',Z'         = %9.4f, %9.4f, %9.4f\n",
    AnglOutHor, AnglOutVert,  TranslOut[0], TranslOut[1], TranslOut[2]) ;
  }
  else
  {	
    fprintf(LogFilePtr,"standard frame generation used\n") ;
  }

  fclose(pSmplFile);

} */


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

    stGeometry.nCuboids = 2; 
    stGeometry.pCuboid = calloc(stGeometry.nCuboids, sizeof(VtCuboid));
      
    stGeometry.pCuboid[0].Length    = BlowUp * stSample.SG.Cube.thickness; 
    stGeometry.pCuboid[0].Width     = BlowUp * stSample.SG.Cube.width;
    stGeometry.pCuboid[0].Height    = BlowUp * stSample.SG.Cube.height;
    stGeometry.pCuboid[0].vCntr[0]  = stSample.Position[0];
    stGeometry.pCuboid[0].vCntr[1]  = stSample.Position[1];
    stGeometry.pCuboid[0].vCntr[2]  = stSample.Position[2];
    stGeometry.pCuboid[0].vNormal[0]= stSample.Direction[0];
    stGeometry.pCuboid[0].vNormal[1]= stSample.Direction[1];
    stGeometry.pCuboid[0].vNormal[2]= stSample.Direction[2];
      
    stGeometry.pCuboid[1].Length    = BlowUp * stSubstr.SG.Cube.thickness; 
    stGeometry.pCuboid[1].Width     = BlowUp * stSubstr.SG.Cube.width;
    stGeometry.pCuboid[1].Height    = BlowUp * stSubstr.SG.Cube.height;
    stGeometry.pCuboid[1].vCntr[0]  = stSubstr.Position[0];
    stGeometry.pCuboid[1].vCntr[1]  = stSubstr.Position[1];
    stGeometry.pCuboid[1].vCntr[2]  = stSubstr.Position[2];
    stGeometry.pCuboid[1].vNormal[0]= stSubstr.Direction[0];
    stGeometry.pCuboid[1].vNormal[1]= stSubstr.Direction[1];
    stGeometry.pCuboid[1].vNormal[2]= stSubstr.Direction[2];
  }
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  int k;

  /* print error that might have occured many times */
  PrintMessage(SMPL_Q_RANGE_TOO_SMALL, "", ON);

  fprintf(LogFilePtr," \n");

  /* free allocated memory */
  if (pTabQ!=NULL)  free(pTabQ);
  if (pTabR!=NULL)  free(pTabR);
	
  if (bOffSpec) 
  {
    if (pTab_Qin_Qout[0]!=NULL) free (pTab_Qin_Qout[0]);
    if (pTab_Qin_Qout[1]!=NULL) free (pTab_Qin_Qout[1]);

    for (k=0; k < nLinesRefl; k++) 
    {
      if (pTab_Qin_Qout[k+2]!=NULL) free (pTab_Qin_Qout[k+2]);
      if (pTab_RoffSpec[k]!=NULL)   free (pTab_RoffSpec[k]);
    }
  }

}/* End OwnCleanup */


/********************************************************************************************************/
/* controls, if neutron hits the sample (= reflecting layer) and tranfers into sample frame             */
/********************************************************************************************************/
short	CheckRefl(Neutron* pNeutron, short int treatingReflection)
{
  VectorType ISP1, ISP2,         /* Intersection points (entry and exit)  */
    NeutPos, NeutDir;   /* Neutron position and flight direction */

  /* intermediate variables */
  CopyVector(pNeutron->Position, NeutPos);
  CopyVector(pNeutron->Vector,   NeutDir);

  /* computes neutron variables in the frame of the sample */
  SubVector(NeutPos, PosSmpl);
  RotVector(RotMatrixSmpl, NeutPos);
  RotVector(RotMatrixSmpl, NeutDir);

  /* here computes the depth where the neutron meets the reflecting
     plane, equivalent to a parallel shift of a t=0 sample in the frame of sample */
  if(IntersectionWithRectangular(DimSmpl, NeutPos, NeutDir, ISP1, ISP2) == 0)
    return(FALSE) ;

  Depth[0] = MonteCarlo(ISP1[0], ISP2[0]) ;
  Depth[1] = Depth[2] = 0 ;

  SubVector(NeutPos, Depth) ;

  // Do the incoherent scattering here
  if (!treatingReflection) {
    double pathInSample, prob;
    SubVector(ISP2, ISP1);
    pathInSample = MonteCarlo(0, LengthVector(ISP2));
    prob = 1. - exp(-MuInc*pathInSample);
    pNeutron->Probability *= prob;
    // if (prob > MaxProb) MaxProb = prob;
    if (pNeutron->Probability <= wei_min) return (FALSE);
  }

  /* here we have the neutron in the frame of the sample*/
  CopyVector(NeutPos, pNeutron->Position) ;
  CopyVector(NeutDir, pNeutron->Vector) ;

  return(TRUE) ;

}/* End CheckRefl */

/**************************************************************************************************************/
/* controls, if neutron hits sample or substrate, tranfers into that frame and returns intersection points */
/**************************************************************************************************************/
short	FindISPs(VectorType ISP1, VectorType ISP2, Neutron* pNeutron, short bSubstrate)
{
  short rc=FALSE;
  VectorType NeutPos, NeutDir;   /* Neutron position and flight direction */

  /* intermediate variables */
  CopyVector(pNeutron->Position, NeutPos);
  CopyVector(pNeutron->Vector,   NeutDir);

  RotVector(RotMatrixSmpl, NeutDir);

  /* here computes where the neutron hits the reflecting plane */
  if (bSubstrate==TRUE)
  { 
    /* computes neutron variables in the frame of the sample */
    SubVector(NeutPos, PosSub);
    RotVector(RotMatrixSmpl, NeutPos);

    rc = IntersectionWithRectangular(DimSub, NeutPos, NeutDir, ISP1, ISP2);

    Depth[0] = Depth[1] = Depth[2] = 0 ;
  }
  else
  { 
    /* computes neutron variables in the frame of the sample */
    SubVector(NeutPos, PosSmpl);
    RotVector(RotMatrixSmpl, NeutPos);

    rc = IntersectionWithRectangular(DimSmpl, NeutPos, NeutDir, ISP1, ISP2);

    /* reflection takes place in a certain depth, equivalent to a parallel shift of a t=0 sample in the frame of sample */
    Depth[0] = MonteCarlo(ISP1[0], ISP2[0]) ;
    Depth[1] = Depth[2] = 0 ;
    SubVector(NeutPos, Depth) ;
  }

  if (rc==TRUE && bSubstrate==FALSE)
  {
    /* here we have the neutron in the frame of the sample*/
    CopyVector(NeutPos, pNeutron->Position) ;
    CopyVector(NeutDir, pNeutron->Vector) ;
  }

  return(rc) ;

}/* End FindISPs */


/********************************************************************************************************/
/* controls, if neutron is reflected, tranfers into sample frame (and returns intersection points)      */
/********************************************************************************************************/
short	FindSubISPs(double* Dist1, double* Dist2, Neutron* pNeutron)
{
  short rc=FALSE;
  VectorType ISP1, ISP2;
  VectorType NeutPos, NeutDir;   /* Neutron position and flight direction */

  /* intermediate variables */
  CopyVector(pNeutron->Position, NeutPos);
  CopyVector(pNeutron->Vector,   NeutDir);

  /* computes neutron variables in the frame of the sample */
  SubVector(NeutPos, PosSub);
  RotVector(RotMatrixSmpl, NeutPos);
  RotVector(RotMatrixSmpl, NeutDir);

  /* here computes where the neutron hits the reflecting plane */
  rc = IntersectionWithRectangular(DimSub, NeutPos, NeutDir, ISP1, ISP2);

  if (rc==TRUE)
  { *Dist1 = DistVector(ISP1, NeutPos);
    *Dist2 = DistVector(ISP2, NeutPos);
  }
  else
  { *Dist1 = 0.0;
    *Dist2 = 0.0;
  }

  return(rc) ;

}/* End ChekcReflect */


/*********************************************************************/
/* writes a trajectory passing through the substrate                */
/*********************************************************************/
short PassThruAll(Neutron* outNeutron, VectorType P1, VectorType P2)
{
  double     tot_len=0.0,   // distance from current position to end of sample
             pathlen=0.0,   // pathlength in th 
             mu_tot=0.0;
  VectorType vPath={0.0,0.0,0.0};

  /* propagation till the end of the sample */
  tot_len = (P2[0] - outNeutron->Position[0]) / outNeutron->Vector[0];
  outNeutron->Time += (tot_len / V_FROM_LAMBDA(outNeutron->Wavelength));
  CopyVector(outNeutron->Vector, vPath) ;
  MultiplyByScalar(vPath, tot_len);
  AddVector  (outNeutron->Position, vPath) ; /* vPath = displacement vector */

  /* path length through substrate determines attenuation */
  SubVector(P2, P1);
  pathlen = LengthVector(P2);

  if (eSubMat==VT_MIRR_OTHER)
    mu_tot = SubMuScaT + SubMuAbs*outNeutron->Wavelength/1.798;
  else
    mu_tot = AttenuationMirr(outNeutron->Wavelength, eSubMat);

  ProbOut = ProbIn * exp(-pathlen * mu_tot);

  // Convert back to global coordinate system and write neutron to the output stream
  TransformBackToGlobalSystemAndWriteNeutron(outNeutron, FALSE, FALSE);

  return TRUE;
}


/*********************************************************************/
/* writes a trajectory propagating without attenuation               */
/*********************************************************************/
short PassThrough(Neutron* outNeutron, double PathLen, VtMirrMat eMat, short bWrite)
{
  double     mu_tot=0.0;
  VectorType vPath={0.0,0.0,0.0};

  // propagate neutron
  outNeutron->Time += (PathLen / V_FROM_LAMBDA(outNeutron->Wavelength));
  CopyVector(outNeutron->Vector, vPath);
  MultiplyByScalar(vPath, PathLen);
  AddVector  (outNeutron->Position, vPath) ; /* vPath = displacement vector */

  // consider attenuation
  if (eMat==VT_MIRR_VACUUM)
    mu_tot = 0.0;
  else if (eMat==VT_MIRR_OTHER)
    mu_tot = SubMuScaT + SubMuAbs*outNeutron->Wavelength/1.798;
  else
    mu_tot = AttenuationMirr(outNeutron->Wavelength, eMat);

  ProbOut = ProbIn * exp(-PathLen * mu_tot);

  // Convert back to global coordinate system and write neutron to the output stream
  if (bWrite==TRUE && ProbOut > wei_min)
    Transf2OutputAndWriteNeutron(outNeutron);

  return TRUE;
}


/*******************************************************/
/** Returns interpolated reflectivity value           **/
/*******************************************************/
double	CalcReflect(const double Qneut)
{
  double Refl=0.0, TQn=0.0, TQa;
  double LTRd, LTRa, LTRn;
  short  n=0;

  while (n+1 < nLinesRefl  &&  pTabQ[n+1] < Qneut)
  {	n++;
  }

  if (n+1 < nLinesRefl)
  {	/* linear extrapolation in logarithmic scale */
    if (pTabQ[n+1] != pTabQ[n])
    {	TQa     = pTabQ[n];
      TQn     = pTabQ[n+1];
      LTRa    = log(pTabR[n]);
      LTRn    = log(pTabR[n+1]);
      LTRd    = LTRa  +  (LTRn - LTRa) / (TQn - TQa) * (Qneut - TQa);
      Refl = exp(LTRd);
    }
    else
    {	Refl = pTabR[n+1];
    }
  }

  return Refl;
}


/********************************************************************/
/* 'AnglesOutputFrame' computes frame angles of output */
/********************************************************************/
void AnglesOutputFrame(double RotH, double RotV, double *OutH, double *OutV)
{
  double n[3] ;

  FillRotMatrixZY(RotMatrixSmpl, M_PI/180.*RotV, M_PI/180.*RotH) ;

  n[0] = 1. ;		n[1] = 0. ;		n[2] = 0. ;

  RotVector(RotMatrixSmpl, n) ;      /* components of a vector parallel to X in the frame of the sample */

  n[0] *= -1. ;                    /* reflection on the sample */

  RotBackVector(RotMatrixSmpl, n) ;  /* new components in the frame of input */

  CartesianToEulerZY(n, OutV, OutH) ;

  if(*OutH == - M_PI) *OutH = M_PI;
  if(*OutV == - M_PI) *OutV = M_PI;

  *OutH	*= 180./M_PI ;
  *OutV	*= 180./M_PI ;
}


/********************************************************************/
/** Calculates minimal and maximal theta angle                     **/
/********************************************************************/
void CalculateThetaRange()
{
  // Calculate the theta range covered by the detector
  double theta;

  theta = atan(sqrt(sq(DetWidth/2.) + sq(DetHeight/2.))/DetDist);

  MinTheta = 2.*SmplAngle - theta;
  MaxTheta = 2.*SmplAngle + theta;
  
}


/***************************************************************************/
/* For detectors close to the direct beam, deltaPhi is a function of theta */
/* Calculate corresponding deltaPhi for each trajectory individually.      */
/****************************************************************************/
void CalculatePhiRange(double theta, double* phiMin, double* phiMax, int* switchSign)
{
  double h0, h, h0Dist, hDist, largestDist1, largestDist2, det_X, det_Y;

  if (eSmplAxis==VT_ROT_Z) 
  {
    det_X = DetHeight;
    det_Y = DetWidth;
  }
  else 
  {
    det_X = DetWidth;
    det_Y = DetHeight;
  }
  
  // Calculate the location at the detector which is hit by the direkt beam
  h0 = -DetDist * tan(SmplAngle*2.);

  // Calculate the location that is hit by the trajectory with the angle of theta
  h = DetDist * tan(theta - SmplAngle*2.);

  // Distance between end of detector and the direct beam position
  h0Dist = fabs(fabs(h0) - det_Y/2.);

  // Distance between direct beam and currect trajectory position
  hDist = fabs(h0 - h);

  largestDist1 = sqrt(pow(det_X/2., 2) + pow(h0Dist, 2));
  largestDist2 = sqrt(pow(det_X/2., 2) + pow(fabs(fabs(h0) + det_Y/2.), 2));

  if (fabs(h0) >= det_Y/2.) 
  {
    if (largestDist1 >= hDist) 
    {
      *phiMin = -acos(h0Dist/hDist);
      *phiMax = -*phiMin;
    }
    else {
      *phiMin = -asin(det_X/2./hDist);
      *phiMax = -*phiMin;
    }
  }
  else 
  {
    if (hDist <= h0Dist) 
    {
      *phiMin = -M_PI;
      *phiMax = M_PI;
    }
    else if (hDist > h0Dist && hDist < largestDist1) {

      //      *phiMin = (-1.)*(M_PI/2. + asin(h0Dist/hDist));
      // *phiMax = -*phiMin;
      *phiMin = acos(h0Dist/hDist);
      *phiMax = M_PI*2 - *phiMin;
      if (h/h0 < 0) 
      {
        *phiMin -= M_PI;
        *phiMax -= M_PI;
      }
    }
    else if (hDist >=largestDist1 && hDist < largestDist2) 
    { 
      *phiMin = M_PI/2. + acos(det_X/(2.*hDist)); 
      *phiMax = M_PI*2 -*phiMin; 

      if (h/h0 < 0) 
      {
        *phiMin -= M_PI;
        *phiMax -= M_PI;
      } 
    } 
    /* else if (hDist > det_X/2. && hDist <= (det_Y - h0Dist)) */
    /* { *phiMin = (-1.)*(M_PI/2. - acos(det_X/(2.*hDist))); */
    /*   *phiMax = -*phiMin; */
    /* } */
    /* else if (hDist > (det_Y - h0Dist) && hDist <= largestDist2) */
    /* { *phiMin = (-1.)*(M_PI/2. - acos(det_X/(2.*hDist))); */
    /*   *phiMax = (-1.)*(acos((det_Y - h0Dist)/hDist)); */
    /*   *switchSign = 1.; */
    /* } */
    else 
    {
      *phiMin = 0.;
      *phiMax = 0.;
    }
  }
  //  fprintf(LogFilePtr,"theta: %f, h: %f, h0: %f, phiMin: %f, phiMax: %f \n", theta * 180./M_PI, h, h0, *phiMin* 180./M_PI, *phiMax* 180./M_PI);
}


/********************************************************************/
/* Calculate trajectory parameters after specular scattering        */
/********************************************************************/
int ScatterSpecular(double scatteringAngle, Neutron* inputNeutron, Neutron* outputNeutron)
{
  double divy=0.0, divz=0.0, theta=0.0, 
         Q=0.0, R=0.0;

  divy = (double) asin(inputNeutron->Vector[1] / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
                                                      inputNeutron->Vector[2]*inputNeutron->Vector[2]));
  
  divz = (double) asin(inputNeutron->Vector[2] / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
                                                      inputNeutron->Vector[1]*inputNeutron->Vector[1]));
  
  if (eSmplAxis==VT_ROT_Z) 
    theta = (double) asin(scatteringAngle) - divy;
  else 
    theta = (double) asin(scatteringAngle) - divz;
  
  /* computes momentum transfer */
  Q = 4.0*M_PI*sin(theta)/outputNeutron->Wavelength;
  
  /* probability of reflection */
  if (eOption==VT_REFERENCE)
  { R = 1.0 ;           /* reference sample has reflectivity 1 */
  }
  else
  { if (Q >= Qmin && Q <= Qmax)
    {
      R = CalcReflect(Q);
    }
    else
    {	/* read error: momentum transfer lower or higher than all values in the reflectivity file */
      CountMessageID(SMPL_Q_RANGE_TOO_SMALL, outputNeutron->ID);     
      R = 0.0;
    }
  }
  
  ProbOut = ProbIn * R ;
  ProbIn -= ProbOut;
  if(ProbOut <= wei_min)
    return 0;
  
  //Convert back to global coordinate system and write neutron to the output stream
  TransformBackToGlobalSystemAndWriteNeutron(outputNeutron, TRUE, TRUE);

  return 1;
}


/**************************************************************************************/
/* Create new trajectories and calculate their parameters for offspecular scattering  */
/**************************************************************************************/
void ScatterOffspecular(double scatteringAngle, Neutron* inputNeutron, Neutron* parentNeutron, Neutron* outputNeutron)
{
  double divy, divz, theta, Q, R;
  double currentQf = 0;
  short int offspecularRunning = 1;
  long int currentQfBin = 0;

  divy = (double) asin(inputNeutron->Vector[1] / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
                                                      inputNeutron->Vector[2]*inputNeutron->Vector[2]));
  
  divz = (double) asin(inputNeutron->Vector[2] / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
                                                      inputNeutron->Vector[1]*inputNeutron->Vector[1]));
  
  if (eSmplAxis==VT_ROT_Z) 
    theta = (double) asin(scatteringAngle) - divy;
  else 
    theta = (double) asin(scatteringAngle) - divz;
  
  /* computes momentum transfer */
  Q = 2.*M_PI*sin(theta)/outputNeutron->Wavelength;
 
  //Loop through all q_f entries of the corresponding q_i value, 
  //create one neutron per entry
  while (offspecularRunning) 
  {
    currentQfBin = FindQf(Q, currentQfBin, &currentQf, &R);
    
    if (currentQfBin < 0) 
    {
      offspecularRunning = 0;
      break;
    }
    
    ScatterByQf(parentNeutron, outputNeutron, Q, currentQf);
    
    ProbOut = ProbIn * R ;
    ProbIn -= ProbOut;
    if(ProbOut <= wei_min)
      continue;

    //Convert back to global coordinate system and write neutron to the output stream
    TransformBackToGlobalSystemAndWriteNeutron(outputNeutron, TRUE, TRUE);
  }
  
  return;
}


/********************************************************************/
/* Scatters the neutron isotropically into a given detector         */
/********************************************************************/
void ScatterIncoherent(Neutron* outputNeutron, double PathInSmpl)
{
  int    switchSign=0;
  double prob_geom=1.0, prob_scat=1.0;
  double theta=0.0, phi=0.0;
  double phiMin=0.0, phiMax=0.0, deltaPhi=0.0;
  double deltaTheta = MaxTheta - MinTheta;
    
  if (MaxTheta > 0 && MinTheta < 0) 
  {
    deltaTheta = Max(MaxTheta, fabs(MinTheta));
    if (fabs(MinTheta) > MaxTheta)
      theta = MonteCarlo(MinTheta, 0);
    else
      theta = MonteCarlo(0, MaxTheta);
  } 
  else
  {
    theta = MonteCarlo(MinTheta, MaxTheta);
  }
    
  switchSign = 0;
    
  CalculatePhiRange(theta, &phiMin, &phiMax, &switchSign);
    
  deltaPhi = (phiMax - phiMin) * (switchSign + 1.0);
  phi = MonteCarlo(phiMin, phiMax);
    
  if (switchSign) 
  {
    double random = MonteCarlo(-1, 1);
    phi *= fabs(random)/random;
  }
  
  prob_geom = fabs(sin(theta))*deltaTheta*deltaPhi/M_PI*SignToBkgAreaFact;
  prob_scat = 1. - exp(-MuInc * PathInSmpl);
  outputNeutron->Probability *= (prob_geom * prob_scat);
  ProbIn -= outputNeutron->Probability;

  outputNeutron->Vector[0] = cos(theta);
  if (eSmplAxis==VT_ROT_Y) 
  { 
    outputNeutron->Vector[1] = sin(theta)*sin(phi);
    outputNeutron->Vector[2] = sin(theta)*cos(phi);
  }
  else 
  {
    outputNeutron->Vector[1] = sin(theta)*cos(phi);
    outputNeutron->Vector[2] = sin(theta)*sin(phi);
  }
  outputNeutron->Color = 100;
    
  /* makes depth correction to get back to the old frame for Depth != 0 */
  RotBackVector(RotMatrixSmpl, Depth) ;
  AddVector(outputNeutron->Position, Depth) ;

  /* writes point of scattering for trajectory visualization */
  WriteIAP(outputNeutron, VT_SCATTERED);
    
  /* computes neutron variables in the output frame */
  SubVector(outputNeutron->Position, TranslOut) ;
  RotVector(RotMatrixOut, outputNeutron->Position) ;  /* necessary only for user */
  RotVector(RotMatrixOut, outputNeutron->Vector) ;    /* defined output frame    */
    
  /*	writes output binary file */
  WriteNeutron(outputNeutron) ;

  return;    
}


/*************************************************************************************/
/* Finds the current Q_f value for the mQf-the range of offspecular scattering       */
/*************************************************************************************/
int FindQf(double Qin, int mQf, double* Qf, double* refl)
{
  double TQin1=0.0, TQin2, TQf1, TQf2, TQfd;
  double LTRd, LTRa, LTRn;
  short  nQi=0;
  
  // Find the lines nQi and nQi+1 where k_in is in between
  while (nQi+1 < nQinPoints  &&  pTab_Qin_Qout[nQi+1][1] < Qin)
  {	nQi++;
  }

  if (nQi == 0) 
  {
    *refl=1;
    *Qf = 0;
    return -1;
  }

  if (nQi+1 < nQinPoints)
  {	
    // interpolate if the k_in values differ 
    if (pTab_Qin_Qout[nQi+1][1] != pTab_Qin_Qout[nQi][1])
    {	
      TQin1 = pTab_Qin_Qout[nQi][1];
      TQin2 = pTab_Qin_Qout[nQi+1][1];

      TQf1  = pTab_Qin_Qout[nQi][mQf +2];
      TQf2  = pTab_Qin_Qout[nQi+1][mQf +2];

      if (Qin > TQin1 && Qin < TQin2) 
        TQfd = (TQf1/(Qin - TQin1)  + TQf2/(TQin2 - Qin)) / (1./(Qin - TQin1) + 1./(TQin2 - Qin));
      else if (Qin == TQin1) 
        TQfd = TQf1;
      else 
        TQfd = TQf2;

      *Qf = TQfd;
      if (pTab_RoffSpec[nQi][mQf] == 0 || pTab_RoffSpec[nQi+1][mQf] == 0) 
      {
        *refl = 0;
        return -1;
      }
      // linear extrapolation between neighbouring Qin and Qf bins in logarithmic scale
      LTRa    = log(pTab_RoffSpec[nQi][mQf]);
      LTRn    = log(pTab_RoffSpec[nQi+1][mQf]);
      LTRd    = LTRa  +  (LTRn - LTRa) / (TQin2 -TQin1) * (Qin - TQin1);
      *refl = exp(LTRd);
    }
    else
    {	
      *refl = pTab_RoffSpec[nQi+1][mQf];
    }
  }
  else
  { return -1;
  }

  if ((mQf+1) < pTab_Qin_Qout[nQi][0] && (mQf+1) < pTab_Qin_Qout[nQi+1][0]) 
    return (mQf+1);
  else 
    return -1;
}


/**********************************************************************/
/* Determines the direction of the neutron at the scattering location */
/* such that the direction vector matches the requires Q_f            */
/**********************************************************************/
void ScatterByQf(Neutron* ParentNeutron, Neutron* Neutron, double dQin, double dQf)
{
  VectorType nDir;  
  long double vDiff0;
  long double vDiff1;
  int i;
  short switchSign = 0;
  double Q_SC = 0;

  //  fprintf(LogFilePtr,"Direction in sample frame: %f %f %f\n", ParentNeutron->Vector[0], ParentNeutron->Vector[1], ParentNeutron->Vector[2]);

  for (i=0; i < 3; i++) nDir[i] = ParentNeutron->Vector[i];

  // Change final neutron vector dQf to scattering vector Q_SC
  Q_SC = dQf + dQin;

  vDiff0 = nDir[0];
  //  nDir[0] *= dQf/dQin;
  nDir[0] *= Q_SC/dQin;
  if (fabs(nDir[0]) > fabs(vDiff0)) switchSign = 1;
  vDiff0 = vDiff0 - nDir[0];
  
  if (switchSign == 1) vDiff0 = fabs(vDiff0) * (-1.);
  else vDiff0 = fabs(vDiff0);

  if (eSmplAxis==VT_ROT_Z) 
  {
    FillRotMatrixZY(RotMatOffSpec, 0, (SmplHor-M_PI_2));
    
    vDiff1 = -1.*fabs(nDir[1]) + sqrt(nDir[1]*nDir[1] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    if ((switchSign && nDir[2] < 0) || (!switchSign && nDir[2] < 0 && vDiff1 > 0)) vDiff1*=-1.;
    nDir[1] += vDiff1;
    
    RotBackVector(RotMatOffSpec, nDir);
  }
  else 
  {
    FillRotMatrixZY(RotMatOffSpec, (SmplVert-M_PI_2), 0);

    vDiff1 = -1.*fabs(nDir[2]) + sqrt(nDir[2]*nDir[2] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    //if ((vDiff1*nDir[2] < 0 && switchSign == 0) || (vDiff1*nDir[2] > 0 && switchSign == 1)) vDiff1 *= -1.*fabs(nDir[2]) - sqrt(nDir[2]*nDir[2] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    if ((switchSign && nDir[2] < 0) || (!switchSign && nDir[2] < 0 && vDiff1 > 0)) vDiff1*=-1.;
    nDir[2] += vDiff1;

    RotBackVector(RotMatOffSpec, nDir);

  }

  for (i=0; i < 3; i++) Neutron->Vector[i] = nDir[i];

  //  fprintf(LogFilePtr,"Direction in sample frame after re-orientation: %f %f %f %f %f\n", Neutron->Vector[0], Neutron->Vector[1], Neutron->Vector[2], dQin, dQf);

  return;
}


/********************************************************************************/
/* Neutron parameters are transferred back to the original coordinate system,   */
/*   then to the  output frame, and written to the stream.                      */
/********************************************************************************/
void TransformBackToGlobalSystemAndWriteNeutron(Neutron* outputNeutron, short bRefl, short bDepth)
{
  outputNeutron->Probability = ProbOut;

  /* computes reflected direction in the frame of sample */
  if (bRefl)
    outputNeutron->Vector[0] = -outputNeutron->Vector[0];
    
  /* computes neutron variables in the initial frame */
  RotBackVector(RotMatrixSmpl, outputNeutron->Position) ;
  RotBackVector(RotMatrixSmpl, outputNeutron->Vector) ;
  AddVector(outputNeutron->Position, PosSmpl) ;
    
  /* makes depth correction to get back to the old frame for Depth != 0 */
  if (bDepth)
  { RotBackVector(RotMatrixSmpl, Depth) ;
    AddVector(outputNeutron->Position, Depth) ;
  }

  /* writes point of scattering for trajectory visualization */
  WriteIAP(outputNeutron, VT_SCATTERED);
    
  /* computes neutron variables in the output frame */
  SubVector(outputNeutron->Position, TranslOut) ;
  RotVector(RotMatrixOut, outputNeutron->Position) ;  /* necessary only for user */
  RotVector(RotMatrixOut, outputNeutron->Vector) ;    /* defined output frame    */
    
  /*	writes output binary file */
  WriteNeutron(outputNeutron) ;
    
  return;   
}


/*************************************************************************************/
/* Neutron parameters are transferred to the output frame and written to the stream  */
/*************************************************************************************/
void  Transf2OutputAndWriteNeutron(Neutron* outNeutron)
{
  outNeutron->Probability = ProbOut;

  /* writes point of scattering for trajectory visualization */
  WriteIAP(outNeutron, VT_SCATTERED);

  /* computes neutron variables in the output frame */
  SubVector(outNeutron->Position, TranslOut) ;
  RotVector(RotMatrixOut, outNeutron->Position) ;  /* necessary only for user */
  RotVector(RotMatrixOut, outNeutron->Vector) ;    /* defined output frame    */
    
  /*	writes output binary file */
  WriteNeutron(outNeutron) ;
    
  return;   
}
