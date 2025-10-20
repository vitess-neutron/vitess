/********************************************************************************************/
/*  VITESS module 'sample_elasticisotr.c'                                                   */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  MAY 2004  G. Zsigmond    normalise with repetition                                  */
/* 1.4  JUL 2004  G. Zsigmond    error message for sample position                          */
/* 1.5  JUL 2004  G. Zsigmond    including hollow cylinder sample                           */
/* 1.5a DEC 2004  K. Lieutenant  no attenuation by scattering, (error of count rates)       */
/* 1.5b DEC 2004  K. Lieutenant  correction: algorithm for repetitions                      */
/* 1.6  JAN 2011  K. Lieutenant  option to scatter only neutrons of a special color         */
/* 1.7  Apr 2020  K. Lieutenant  tidy up and new central visualization parameters           */
/* 1.8  Oct 2021  K. Lieutenant  option: parameters from input instead of from file         */
/* 1.9  Jan 2023  K. Lieutenant  scattering probability and visualization corrected         */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "convert.h"
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "sample.h"

#define  STRING_BUFFER 50


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void  OwnInit(int argc, char *argv[]);                  // Reads input parameters and sets global variables
void  OwnCleanup();                                     // Does module specific cleanup
void  SetSamplePar   ();                                // Reads sample parameters and combines with input parameters
void  CalcAndWritePar(SampleType *pSample);             // Calculates arrays from input parameters and writes to log file
void  SetGeometry    (char* sColor);                    // Fills the structure stGeometry for visualization
void  TransformIn2Smpl(VectorType pos, VectorType dir); // Co-ordinate transformation from input frame to sample frame
void  TransformSmpl2In(VectorType pos, VectorType dir); // Co-ordinate transformation from sample frame to input frame
void  TransformIn2Out (VectorType pos, VectorType dir); // Co-ordinate transformation from input frame to output frame


/******************************/
/**   Global Variables       **/
/******************************/
char      *pSmplFileName=NULL;         //      -P        [-]   pointer to the name of the sample file
long       Repetition=1;               //      -A        [-]   repetitions (how many trajectories to generate per incoming trajectory)
short      iColor=ANY_COLOR;           //      -c        [-]   enum color: if != ANY_COLOR, only neutrons of this colour are treated

VtSmplGeom eGeom=VT_NO_GEOM;           // file -G        [-]   geometry parameter: "cylinder" "hollow-cylinder" "sphere" "cuboid"
VectorType ScatMain ={0.0,0.0,0.0},    // file -E -F    [deg]  horizontal and vertical component (Theta, Phi) of the main scattering direction
           ScatRange={0.0,0.0,0.0};    // file -e -f    [deg]  hor. and vert. var. (DelTheta, DelPhi) determining scat. range [Phi-DelPhi, Phi+DelPhi], Theta analogous
double     AbsorptionC=0.0,            // file -m       [1/cm] Macroscopic absorption cross section
           ScatteringC=0.0;            // file -T       [1/cm] Macroscopic total scattering cross section
VectorType PosSample={0.0,0.0,0.0};    // file -x -y -z  [cm]  center position of the sample
double     Diameter = 0.0,             // file -t        [cm]  thickness or diameter of the sample
           Height   = 0.0,             // file -h        [cm]  height of the sample
           Width    = 0.0;             // file -w        [cm]  width of the sample
double     AnglSmplHor =0.0,           // file -o       [deg]  horizontal angle of the sample orientation, relative to standard orientation
           AnglSmplVert=0.0;           // file -O       [deg]  vertical angle of the sample orientation, relative to standard orientation
VectorType TranslOut={0.0,0.0,0.0};    // file -X -Y -Z  [cm]  center position of the output frame
double     AnglOutHor =0.0,            // file -u       [deg]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert=0.0;            // file -U       [deg]  vertical angle of the output frame, relative to input orientation

// Variables determined from input parameters or trajectory data
SampleType stSample;                   //                      sample geometry
VectorType DimSample   ={0.0,0.0,0.0}, //                      size of the sample
           DimSampleHol={0.0,0.0,0.0}; //                      array to use 'IntersectsWithCylinder()' for hollow cylinders
double     ProbCutoff=0.0;             //                      neutron weight, below which the trajectory is removed
double     RotMatrixSample[3][3],      //                      rotation matrix to transfer to coordinate system of the sample
           RotMatrixOut[3][3],         //                      rotation matrix to transfer to the output coordinate system
           RotMatrixScatter[3][3];     //                      rotation matrix to transfer into coordinate system of the scattering direction


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char **argv)
{
  long       repet=0, i=0;
  double     TOF=0.0, WL=0.0, Prob=0.0, TofOut=0.0,
             PathLength   =0.0, PathLengthHol   =0.0,
             MaxPathLength=0.0, MaxPathLengthHol=0.0;
  VectorType Pos1f ={0.0,0.0,0.0}, Pos2f  ={0.0,0.0,0.0}, Pos3f={0.0,0.0,0.0}, Pos4f={0.0,0.0,0.0},
             Pos1v ={0.0,0.0,0.0}, Pos2v  ={0.0,0.0,0.0}, Pos3v={0.0,0.0,0.0}, Pos4v={0.0,0.0,0.0},
             propag={0.0,0.0,0.0}, propag1={0.0,0.0,0.0},                                // propagation vectors e.g. from entry to point of scattering to calculate TOF
             Pos   ={0.0,0.0,0.0}, Dir    ={0.0,0.0,0.0}, Pos_final={0.0,0.0,0.0};
  Neutron    InNeutron, OutNeutron;

  // initialisation
  // --------------
  InitNeutron(&InNeutron);
  InitNeutron(&OutNeutron);

 _eModule = MCN_SMPL_EL_ISO;

  Init   (argc, argv, _eModule);
  PrintModuleName(_eModule, "1.9");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr)
    bBlowUp     = TRUE;

  /* Reads sample parameters and combines with input parameters */
  SetSamplePar();

  /* determines the dependent parameters and writes out important parameters */
  CalcAndWritePar(&stSample);

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  /* Get the neutrons from the file */
  while ((ReadNeutrons())!= 0)
  {
    for(i=0;i<NumNeutGot ;i++)
    {
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        if (iColor==ANY_COLOR || iColor==InputNeutrons[i].Color)
        {
          MaxPathLengthHol = PathLengthHol = 0.0;

          NormVectorX( InputNeutrons[i].Vector);
          CopyNeutron(&InputNeutrons[i], &InNeutron);

          /* translates and rotates into frame of the sample  */
          TransformIn2Smpl(InputNeutrons[i].Position, InputNeutrons[i].Vector) ;

          /* gives intersection positions with sample */
          if (eGeom==VT_CYL && IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0)
             goto getlost ;

          if (eGeom==VT_HOL_CYL)
          {
            if (IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos4f) == 0)
            { goto getlost ;
            }
            else
            {
              if (IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 0)
                CopyVector(Pos4f, Pos2f);

              if (IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 1)
              { double r=MonteCarlo(-1.,1);

                if ((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==1))
                  goto getlost;

                if ((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==0))
                {
                  if (r > 0.0)
                  {
                    SubVector(Pos2f, Pos1f);
                    PathLengthHol = LengthVector(Pos2f);
                    CopyVector(Pos3f, Pos1f); CopyVector(Pos4f, Pos2f);
                    MaxPathLengthHol = PathLengthHol;
                  }
                  else
                  {
                    SubVector(Pos4f, Pos3f);
                    MaxPathLengthHol = LengthVector(Pos4f);
                    PathLengthHol    = 0.;
                  }
                }
                if ((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==1))
                {
                  PathLengthHol    = 0.;
                  MaxPathLengthHol = 0.;
                }
                if ((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==0))
                {
                  CopyVector(Pos3f, Pos1f); CopyVector(Pos4f, Pos2f);
                  PathLengthHol    = 0.;
                  MaxPathLengthHol = 0.;
                }
              }
            }
          }

          if (eGeom==VT_CUBE && IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0)
            goto getlost ;

          if (eGeom==VT_SPHERE && IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0)
            goto getlost ;

          for (repet=0;repet<Repetition;repet++)
          {
            CHECK;

            CopyVector(Pos1f, Pos1v) ;
            CopyVector(Pos2f, Pos2v) ;
            CopyVector(Pos3f, Pos3v) ;
            CopyVector(Pos4f, Pos4v) ;

            TOF  = InputNeutrons[i].Time ;
            WL   = InputNeutrons[i].Wavelength ;
            Prob = InputNeutrons[i].Probability ;

            CopyVector(InputNeutrons[i].Position, Pos) ;
            CopyVector(InputNeutrons[i].Vector, Dir) ;

            /* scattering position and TOF until scattering */
            SubVector(Pos2v, Pos1v) ;                                   // Pos2v: vector from entry to exit of the path through the sample
            MaxPathLength = LengthVector(Pos2v) + MaxPathLengthHol ;
            MultiplyByScalar(Pos2v, MonteCarlo(0.,1.)) ;               // Pos2v now vector from entry into sample to point of scattering
            PathLength = LengthVector(Pos2v) + PathLengthHol;
            AddVector(Pos1v, Pos2v);                                   // Pos1v now vector to point of scattering

            /* TOF until scattering */
            CopyVector(Pos1v, propag);
            SubVector(propag, Pos);         // propag: vector from x=0 position to point of scattering
            TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;

            CopyVector(Pos1v, Pos) ;            /*scattering position */

            /* Scattering probability determined by straight line through the sample, attenuation by real path */
            // Prob *= MaxPathLength * ScatteringC;
            Prob *= (1.0 - exp(-MaxPathLength * ScatteringC));
            Prob *= exp(-PathLength * AbsorptionC * WL);         // attenuation part 1: entry until point of scattering

            /* point of scattering for trajectory visualization */
            if (bVisTraj==TRUE)
            {
              Neutron ScatNeut;
              CopyNeutron(&InputNeutrons[i], &ScatNeut);
              CopyVector (Pos1v, ScatNeut.Position);
              ScatNeut.Probability=Prob;

              WriteScatIAP(&ScatNeut, VT_SCATTERED, RotMatrixSample, PosSample);
            }

            /* scattering: new neutron variables*/
            {
              double  dir_fin[3], DeltaHoriz, DeltaVert ;

              /* new random direction  */

              DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= ScatRange[1] * M_PI/180. ;
              DeltaVert  = MonteCarlo(-1. , 1.) ; DeltaVert  *= ScatRange[2] * M_PI/180. ;

              EulerToCartesianZY( dir_fin,  &DeltaVert,  &DeltaHoriz);

              RotBackVector(RotMatrixScatter, dir_fin) ; CopyVector(dir_fin, Dir) ;
              RotVector    (RotMatrixSample, Dir) ;
            }


            /* Attenuation succeeding scattering */

            if (eGeom==VT_CYL && IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0)
              goto getlost2 ;

            if (eGeom==VT_HOL_CYL)
            {

              if (IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos4v) == 0)
              {
                goto getlost2 ;
              }
              else
              {
                if (IntersectionWithCylinder(DimSampleHol, Pos, Dir, Pos2v, Pos3v) == 0)
                {
                  CopyVector(Pos4v, Pos2v);
                }
                else
                { VectorType Propag;

                  CopyVector(Pos2v, Propag);
                  SubVector (Propag, Pos);

                  if (CompareVectors(Pos3v, Pos4v)==0)
                  {
                    if (ScalarProduct(Propag, Dir) > 0.)
                    { CopyVector(Pos4v, propag1);
                      SubVector (propag1, Pos3v);
                      PathLengthHol = LengthVector(propag1);
                    }
                    else
                    {
                      PathLengthHol = 0.;
                      CopyVector(Pos3v, Pos1v);
                      CopyVector(Pos4v, Pos2v);
                    }
                  }
                  else
                  {
                    PathLengthHol = 0.;
                  }
                }
              }
            }

            if (eGeom==VT_CUBE && IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0)
              goto getlost2 ;

            if (eGeom==VT_SPHERE && IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0)
              goto getlost2 ;


            /* path in the sample after scattering */
              CopyVector(Pos2v, Pos_final) ;
              SubVector (Pos_final, Pos) ;
              PathLength = LengthVector(Pos_final) + PathLengthHol;

            if (PathLengthHol != 0.0)
              CopyVector(Pos4v, Pos2v);  /* for hollow cylinder option: set output position to where it crosses the outer cylinder if crossed  */

            /* attenuation part 2: point of scattering until exit */
            Prob *= exp(-PathLength * AbsorptionC * WL);

          /* Output matters */
            CopyVector(Pos2v, propag);
            SubVector(propag, Pos);  // propag: vector point of scattering to sample exit position
            TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;

            TransformSmpl2In(Pos2v, Dir);
            TransformIn2Out (Pos2v, Dir);

            Prob *= ScatRange[1]/90. * sin(ScatRange[2]* M_PI/90.) /4.0;  /* solid angle / 4pi */
            if (Prob <= ProbCutoff) goto getlost2 ;

            /* transmit coordinates to the outgoing neutron */
            OutNeutron = InputNeutrons[i];

            OutNeutron.Time = TOF;
            OutNeutron.Probability = Prob/Repetition;

            CopyVector(Pos2v, OutNeutron.Position);
            CopyVector(Dir,   OutNeutron.Vector);

            /* writes output binary file */
            WriteNeutron(&OutNeutron) ;

          getlost2:
            WriteDIAP(&InNeutron, VT_ABSORBED, PosSample[0] - InNeutron.Position[0]);
          }  /*repetition*/
              /* here continues if neutron gets lost */

        getlost:
          WriteDIAP(&InNeutron, VT_OUTSIDE, PosSample[0] - InNeutron.Position[0]);
        }
        else  // neutron passes if: iColor >= 0 and iColor != neutron
        {
          OutNeutron = InputNeutrons[i];
          PropagateX(&OutNeutron, &TofOut, TranslOut[0]);
          WriteIAP(&OutNeutron, VT_PASSED);
          // SubVector(OutNeutron.Position, PosSample) ;
          TransformIn2Out(OutNeutron.Position, OutNeutron.Vector);
          WriteNeutron(&OutNeutron) ;
        }
      }
    }   // for loop over trajectories
  }     // do loop, read trajectories

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
  int i;
  InitRotMatrix(RotMatrixSample);
  InitRotMatrix(RotMatrixScatter);
  InitRotMatrix(RotMatrixOut);

  ProbCutoff=wei_min;

  /* Scan all command line parameters */
  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    {
      switch (argv[i][1])
      {
        /* Main window */
        case 'P':
          pSmplFileName=&argv[i][2];
          break;
        case 'A':
          sscanf(&argv[i][2], "%ld", &Repetition) ;
          break;
        case 'c':
          sscanf(&argv[i][2], "%hd", &iColor) ;
          break;

        /* Scattering parameters */
        case 'E':
          ScatMain[1] = atof(&argv[i][2]);
          break;
        case 'F':
          ScatMain[2] = atof(&argv[i][2]);
          break;
        case 'e':
          ScatRange[1] = atof(&argv[i][2]);
          break;
        case 'f':
          ScatRange[2] = atof(&argv[i][2]);
          break;

        case 'T':
          ScatteringC = atof(&argv[i][2]);
          break;
        case 'm':
          AbsorptionC = atof(&argv[i][2]);
          break;

        /* sample position, size and orientation */
        case 'G':
          eGeom = (VtSmplGeom) atoi(&argv[i][2]);
          break;

        case 'x':
          PosSample[0] = atof(&argv[i][2]);
          break;
        case 'y':
          PosSample[1] = atof(&argv[i][2]);
          break;
        case 'z':
          PosSample[2] = atof(&argv[i][2]);
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

        case 'o':
          AnglSmplHor  = atof(&argv[i][2]);
          break;
        case 'O':
          AnglSmplVert = atof(&argv[i][2]);
          break;

        case 'X':
          TranslOut[0] = atof(&argv[i][2]);
          break;
        case 'Y':
          TranslOut[1] = atof(&argv[i][2]);
          break;
        case 'Z':
          TranslOut[2] = atof(&argv[i][2]);
          break;

        case 'u':
          AnglOutHor  = atof(&argv[i][2]);
          break;
        case 'U':
          AnglOutVert = atof(&argv[i][2]);
          break;

        /* Output frame */

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (Repetition < 1)
    Error("Repetition rate must be >= 1") ;

  if (pSmplFileName==NULL)
    Error("Parameter file name missing") ;

  return;
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  return;
}


/*******************************************************/
/** Reads the sample parameters from file             **/
/*******************************************************/
void SetSamplePar()
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="", sGeom[20]="";
  int    nLen=sizeof(sLine)-1;
  double f_h   =0.0, f_v   =0.0,
         f_dh  =0.0, f_dv  =0.0,
         mu_sca=0.0, mu_abs=0.0,
         x     =0.0,  y    =0.0, z   =0.0,
         off_h =0.0, off_v =0.0,
         diamtr=0.0, height=0.0, width=0.0,
         out_x =0.0, out_y =0.0, out_z=0.0,
         out_h =0.0, out_v =0.0;
  VtSmplGeom geom=VT_NO_GEOM;

  /* Opens the parameter file if a file name is given */
  if (pSmplFileName!=NULL)
  {
    pFile = OpenInputFile(pSmplFileName, FALSE, "rt");

    /* Reads the parameters if the file can be opened */
    if (pFile != NULL)
    {
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &f_h,    &f_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &f_dh,   &f_dv);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf"    , &mu_sca, &mu_abs);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x,      &y,      &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &off_h,  &off_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sGeom);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &diamtr, &height, &width);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &out_x,  &out_y,  &out_z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &out_h,  &out_v);

      geom  = SmplGeom_Txt2ID(sGeom);

      fprintf (LogFilePtr,"sample data read from parameter file: '%s':\n", pSmplFileName) ;
      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeom ==VT_NO_GEOM   && geom !=VT_NO_GEOM)  eGeom   = geom;
      if (ScatMain [1]==0.0 && f_h   !=0.0) ScatMain [1]= f_h   ;
      if (ScatMain [2]==0.0 && f_v   !=0.0) ScatMain [2]= f_v   ;
      if (ScatRange[1]==0.0 && f_dh  !=0.0) ScatRange[1]= f_dh/2.0;
      if (ScatRange[2]==0.0 && f_dv  !=0.0) ScatRange[2]= f_dv/2.0;
      if (ScatteringC ==0.0 && mu_sca!=0.0) ScatteringC = mu_sca;
      if (AbsorptionC ==0.0 && mu_abs!=0.0) AbsorptionC = mu_abs;
      if (PosSample[0]==0.0 && x     !=0.0) PosSample[0]= x     ;
      if (PosSample[1]==0.0 && y     !=0.0) PosSample[1]= y     ;
      if (PosSample[2]==0.0 && z     !=0.0) PosSample[2]= z     ;
      if (AnglSmplHor ==0.0 && off_h !=0.0) AnglSmplHor = off_h ;
      if (AnglSmplVert==0.0 && off_v !=0.0) AnglSmplVert= off_v ;
      if (Diameter    ==0.0 && diamtr!=0.0) Diameter    = diamtr;
      if (Height      ==0.0 && height!=0.0) Height      = height;
      if (Width       ==0.0 && width !=0.0) Width       = width ;
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

  // checks
  if ((PosSample[0] < DimSample[0])||(PosSample[0] < DimSample[1])||(PosSample[0] < DimSample[2]))
    Warning("Distance to sample is smaller than at least one sample dimension");

  /* converts degs in radian etc. */
  AnglSmplHor  *= M_PI/180. ;
  AnglSmplVert *= M_PI/180. ;
  AnglOutHor   *= M_PI/180. ;
  AnglOutVert  *= M_PI/180. ;

  if (ScatRange[1] <= 0.0)
    Warning("Horizontal scattering range does not have a positive value. Intensity will be zero");
  if (ScatRange[2] <= 0.0)
    Warning("Vertical scattering range does not have a positive value. Intensity will be zero");

}/* End ReadParFile */


/***********************************************************************/
/** calculates variables from input parameters and writes to log file **/
/***********************************************************************/
void  CalcAndWritePar(SampleType* pSample)
{
  double scattered_dir[3];
  VectorType DirSample={0.0,0.0,0.0}; // sample orientation

  // sets sample dimension and orientation and checks if a valid geometry is given
  switch (eGeom)
  { case VT_CYL    :
    case VT_HOL_CYL: DirSample[2]=1.0; break;
    case VT_CUBE   : DirSample[0]=1.0; break;
    case VT_SPHERE :                   break;
    default        : Error("Sample geometry missing");
  }

  /* computes global reference values */
  scattered_dir[0]= (double) cos(ScatMain[2]*M_PI/180.) * (double) cos(ScatMain[1]*M_PI/180.) ;
  scattered_dir[1]= (double) cos(ScatMain[2]*M_PI/180.) * (double) sin(ScatMain[1]*M_PI/180.) ;
  scattered_dir[2]= (double) sin(ScatMain[2]*M_PI/180.) ;

  fprintf(LogFilePtr,"Scattered dir. :     %6.3f    %6.3f    %6.3f\n", scattered_dir[0], scattered_dir[1], scattered_dir[2]) ;

  FillRotMatrixZY(RotMatrixScatter, ScatMain[2]*M_PI/180., ScatMain[1]*M_PI/180.) ;
  FillRotMatrixZY(RotMatrixSample,  AnglSmplVert,          AnglSmplHor) ;
  FillRotMatrixZY(RotMatrixOut,     AnglOutVert,           AnglOutHor) ;

  RotBackVector(RotMatrixSample, DirSample);

  // fills data structures
  InitSample(pSample);
  FillSample(pSample, eGeom, PosSample[0], PosSample[1], PosSample[2], DirSample[0], DirSample[1], DirSample[2], Diameter, Height, Width, 0.0);

  if (eGeom==VT_HOL_CYL)
  {
    DimSample[0] = Diameter;  DimSampleHol[0] = Width;
    DimSample[1] = 0.0;       DimSampleHol[1] = 0.0;
    DimSample[2] = Height;    DimSampleHol[2] = Height;
  }
  else
  {
    DimSample[0] = Diameter;
    DimSample[1] = Width;
    DimSample[2] = Height;
  }

  fprintf(LogFilePtr,"Repetition rate:     %ld\n", Repetition) ;
  if(Repetition > 20)
    fprintf(LogFilePtr,"Warning: Excessive use of repetition rate >> 1 can lead to wrong results. Be sure that you have very good statistics\n"
                       "in wavelength, time, x,y,z and directions just before the sample\n") ;
}/* End OwnInit */


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


/************************************************************/
/** Co-ordinate transformations from sample to input frame **/
/**                         and from input to output frame **/
/************************************************************/
void TransformIn2Smpl(VectorType pos, VectorType dir)
{
  /* computes neutron variables in the sample frame */
  SubVector(pos, PosSample) ;
  RotVector(RotMatrixSample, pos) ;
  RotVector(RotMatrixSample, dir) ;
}
void TransformSmpl2In(VectorType pos, VectorType dir)
{
  /* computes neutron variables in the initial frame */
  RotBackVector(RotMatrixSample, pos) ;
  RotBackVector(RotMatrixSample, dir) ;
  AddVector(pos, PosSample) ;
}

void TransformIn2Out(VectorType pos, VectorType dir)
{
  /* computes neutron variables in the output frame */
  SubVector(pos, TranslOut) ;
  RotVector(RotMatrixOut, pos) ;
  RotVector(RotMatrixOut, dir) ;

}
