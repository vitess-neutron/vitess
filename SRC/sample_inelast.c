/********************************************************************************************/
/*  VITESS module 'sample_inelast.c'                                                        */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2  MAY 2004  G. Zsigmond    normalize with repetition                                  */
/* 1.3  JUL 2004  G. Zsigmond    error warning for sample position                          */
/* 1.4  JUL 2004  G. Zsigmond    including hollow cylinder sample                           */
/* 1.5  Feb 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.6  Oct 2021  K. Lieutenant  option: parameters from input instead of from file         */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "convert.h"
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "sample.h"

#define	STRING_BUFFER 50


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit(int argc, char *argv[]);                       // Reads input parameters and sets global variables
void   OwnCleanup();                                          // Does module specific cleanup
void   SetSamplePar   (SampleType *pSample);                  // Reads sample parameters and combines with input parameters 
void   CalcAndWritePar();                                     // Calculates arrays from input parameters and writes to log file
void   SetGeometry(char* sColor);                             // Fills the structure stGeometry for visualization 
void   OutputTransform(VectorType Pos, VectorType Dir);       // Co-ordinate transformation to output frame
long   S_q_w(double *wl, double *prob, VectorType Dir);       // S(q,w) scattering: new neutron variables
double FunctionS_q_w(VectorType q, double energy);            // FunctionS_q_w(q, energy) 
double Dispersion(VectorType q);                              // Energy dispersion   
double BoseFactor(double T, double w);                        // Bose factor if w in ueV


/******************************/
/** Global Variables         **/
/******************************/
char      *pSmplFileName=NULL;         //      -P        [-]   pointer to the name of the sample file  
double     P1=0.0,                     //      -a       [ueV]  reference energy (= peak position for vanishing energy dispersion)
           P2=0.0,                     //      -b       [ueV]  Lorentzian width (HWHM)
           P3=0.0,                     //      -c        [-]   scaling factor (for both peaks)
           P4=0.0,                     //      -d        [-]   intensity of the second peak relative to the first
           D1=0.0,                     //      -x        [cm]  linear energy dispersion coefficient of momentum component 
           D2=0.0,                     //      -y       [deg]  linear energy dispersion coefficient of momentum component y
           D3=0.0,                     //      -z       [deg]  linear energy dispersion coefficient of momentum component z
           Temp=0.0;                   //      -T        [cm]  temperature of the sample
int        bBoseF=FALSE;               //      -D       [deg]  flag: multiplication with the Bose factor
long       Repetition=1;               //      -A        [-]   repetitions (how many trajectories to generate per incoming trajectory)

VtSmplGeom eGeom=VT_NO_GEOM;           // file -G        [-]   geometry parameter: "cylinder" "hollow-cylinder" "sphere" "rectangular" 
VectorType ScatMain ={0.0,0.0,0.0}, // file -L -E -F [deg]  mean wavelength and hor. and vert. direction (Theta, Phi) of the scattered neutron
           ScatRange={0.0,0.0,0.0}; // file -l -e -f [deg]  variation in wavelength, hor. and vert. direction of the scattered neutron
double     AbsorptionC=0.0,            // file -m       [1/cm] Macroscopic absorption cross section 
           ScatteringC=0.0;            // file -s       [1/cm] Macroscopic total scattering cross section 
VectorType PosSample={0.0,0.0,0.0};    // file -X -Y -Z  [cm]  center position of the sample
double     AnglSmplHor =0.0,           // file -o       [rad]  horizontal angle of the sample orientation, relative to standard orientation
           AnglSmplVert=0.0;           // file -O       [rad]  vertical angle of the sample orientation, relative to standard orientation
double     Diameter = 0.0,             // file -t        [cm]  thickness or diameter of the sample 
           Height   = 0.0,             // file -h        [cm]  height of the sample 
           Width    = 0.0;             // file -w        [cm]  width of the sample
VtFrameGen eFrame=VT_NO_FRAME;         // file -g        [-]   flag: user defined output frame or standard frame generation
double     LmbdInit =0.0,              // file -q       [Ang]  initial wavelength
           DirInHor =0.0,              // file -i       [deg]  horizontal orientation of the incoming neutron
           DirInVert=0.0;              // file -I       [deg]  vertical orientation of the incoming neutron
VectorType TranslOut={0.0,0.0,0.0};    // file -R -S -W  [cm]  center position of the output frame
double     AnglOutHor =0.0,            // file -u       [rad]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert=0.0;            // file -U       [rad]  vertical angle of the output frame, relative to input orientation    

// Variables determined from input parameters or trajectory data
SampleType stSample;                   //                      sample geometry
VectorType DimSample   ={0.0,0.0,0.0}, //                      size of the sample
           DimSampleHol={0.0,0.0,0.0}, //                      array to use 'IntersectsWithCylinder()' for hollow cylinders  
           k_reference ={0.0,0.0,0.0}; //                      initial k-vector
double     ProbCutoff=0.0,             //                      neutron weight, below which the trajectory is removed
           Beta      =0.0;             //                      1/kT  (to calculae Bose factor for given temperature)
double     RotMatrixSample [3][3],     //                      rotation matrix to transfer to coordinate system of the sample
           RotMatrixOut    [3][3],     //                      rotation matrix to transfer to the output coordinate system
           RotMatrixScatter[3][3];     //                      rotation matrix to transfer into coordinate system of the scattering direction


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long       repet=0,   i=0;
  double     TOF=0.0,   WL=0.0, Prob=0.0, 
             PathLength   =0.0, PathLengthHol   =0.0, 
             MaxPathLength=0.0, MaxPathLengthHol=0.0;
  VectorType Pos1f ={0.0,0.0,0.0}, Pos2f=  {0.0,0.0,0.0}, Pos3f={0.0,0.0,0.0}, Pos4f={0.0,0.0,0.0}, 
             Pos1v ={0.0,0.0,0.0}, Pos2v=  {0.0,0.0,0.0}, Pos3v={0.0,0.0,0.0}, Pos4v={0.0,0.0,0.0},   
             propag={0.0,0.0,0.0}, propag1={0.0,0.0,0.0}, 
             Pos   ={0.0,0.0,0.0}, Dir=    {0.0,0.0,0.0}, Pos_final={0.0,0.0,0.0};
  Neutron	   Neutrons ;

  // initialisation
  // --------------
  InitNeutron(&Neutrons);

  _eModule = MCN_SMPL_INELAST;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.6a");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* Reads sample parameters and combines with input parameters */
  InitSample  (&stSample);
  SetSamplePar(&stSample);

  /* Determines the dependent parameters and write out important parameters */
  CalcAndWritePar();

  DECLARE_ABORT

  // Loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
  {
    for (i=0;i<NumNeutGot ;i++)
    { 
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        MaxPathLengthHol = PathLengthHol = 0.; 
        InputNeutrons[i].Vector[0]		= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

        /* translates and rotates into frame of the sample  */
        SubVector(InputNeutrons[i].Position, PosSample) ;
        RotVector(RotMatrixSample, InputNeutrons[i].Position) ;
        RotVector(RotMatrixSample, InputNeutrons[i].Vector) ;

        /* gives intersection positions with sample */
        if (eGeom==VT_CYL)
        {
          if (IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) 
          goto getlost ; 
        }

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
            {	double r=MonteCarlo(-1.,1);

              if ((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==1)) goto getlost;
				
              if ((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==0))
              {
                if(r>0.)
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
                  PathLengthHol = 0.;
                } 
              }
              if ((CompareVectors(Pos1f, Pos2f)==0)&&(CompareVectors(Pos3f, Pos4f)==1))
              {
                PathLengthHol = 0.; 
                MaxPathLengthHol = 0.;
              }
              if((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==0))
              {
                CopyVector(Pos3f, Pos1f); CopyVector(Pos4f, Pos2f); 
                PathLengthHol = 0.; 
                MaxPathLengthHol = 0.;
              }
            }
          }
        }


        if (eGeom==VT_CUBE)
        {
          if(IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
        }

        if (eGeom==VT_SPHERE)
        {
          if(IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
        }

        for (repet=0;repet<Repetition;repet++)
        {/**/

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

          /* scattering position and TOF untill scattering */	
          SubVector(Pos2v, Pos1v) ;					/*maximal path vector*/ 
          MaxPathLength = LengthVector(Pos2v) + MaxPathLengthHol ; 
          MultiplyByScalar(Pos2v, MonteCarlo(0.,1.)) ;	 /*random path vector in cylinder untill scattering */
          PathLength = LengthVector(Pos2v) + PathLengthHol ;
          AddVector(Pos1v, Pos2v) ;	
		
          CopyVector(Pos1v, propag);
          SubVector(propag, Pos);
          TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;

          CopyVector(Pos1v, Pos) ;						/*scattering position */

          /* attenuation untill scattering normalized to maximal path */
          Prob *= (double) exp( - PathLength * AbsorptionC * WL - PathLength * ScatteringC) ;
          Prob *= MaxPathLength * ScatteringC ; 

          /* S(q,w) scattering: new neutron variables*/ 
          Prob *= WL ;
          if (S_q_w(&WL, &Prob, Dir) == 0) goto getlost2 ;
          Prob *= 1/WL ;

          CHECK;

          /* attenuation succeeding scattering */
          if (eGeom==VT_CYL)
          {
            if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) 
              goto getlost2 ; 
          }

          if (eGeom==VT_HOL_CYL)
          {
		
            if (IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos4v) == 0)
            {
              goto getlost2;
            }
            else 
            {
              if (IntersectionWithCylinder(DimSampleHol, Pos, Dir, Pos2v, Pos3v) == 0) 
              {
                CopyVector(Pos4v, Pos2v);
              }
              else
              {
                VectorType Propag; 
                CopyVector(Pos2v, Propag); 
                SubVector(Propag, Pos);
				
                if (CompareVectors(Pos3v, Pos4v)==0)
                {
                  if (ScalarProduct(Propag, Dir) > 0.)
                  { 
                    CopyVector(Pos4v, propag1);
                    SubVector(propag1, Pos3v);
                    PathLengthHol = LengthVector(propag1); 
                  }
                  else 
                  {
                    PathLengthHol = 0.; 
                    CopyVector(Pos3v, Pos1v); CopyVector(Pos4v, Pos2v);
                  }
                }
                else 
                { 
                  PathLengthHol = 0.; 
                }
              }
            }
          }

          if (eGeom==VT_CUBE)
          {
            if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
          }
          if (eGeom==VT_SPHERE)
          {
            if(IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
          }

          /* path in the sample after scattering */
          CopyVector(Pos2v, Pos_final) ;
          SubVector(Pos_final, Pos) ;
          PathLength = LengthVector(Pos_final) + PathLengthHol ;  

          if (PathLengthHol != 0.) CopyVector(Pos4v, Pos2v);  /* for hollow cylinder option: set output position to where it crosses the outer cylinder if crossed  */
	
          /* Test: set output to scattering positions:  CopyVector(Pos, Pos2v); */
          Prob *= (double) exp( - PathLength * AbsorptionC * WL - PathLength * ScatteringC);

          /* Output matters */
          CopyVector(Pos2v, propag);
          SubVector(propag, Pos);
          TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;

          OutputTransform(Pos2v, Dir) ;

          Prob *= ScatRange[1]/90. * sin(ScatRange[2]* M_PI/90.)/4.;  /* solid angle / 4pi */
          if (Prob <= ProbCutoff) 
            goto getlost2 ;

          /* transmit coordinates which were not changed, the rest overwrite below */
          Neutrons = InputNeutrons[i]; 
          Neutrons.Time = TOF ;
          Neutrons.Wavelength = WL ;
          Neutrons.Probability = Prob/Repetition ;

          CopyVector(Pos2v, Neutrons.Position) ;
          CopyVector(Dir, Neutrons.Vector) ;

          /* writes output binary file */
          WriteNeutron(&Neutrons) ;

          getlost2: ;

        }/*repetition*/
        /* here continues if neutron gets lost */

	  getlost: ;
      }
    }
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
/** Own initialization of the sample_inelast module   **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  InitRotMatrix(RotMatrixSample);
  InitRotMatrix(RotMatrixScatter);
  InitRotMatrix(RotMatrixOut);

  ProbCutoff=wei_min ;
	
  /* Scan all command line parameters */
  for (int i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    { 
      switch (argv[i][1])
      {
        case 'P':
          pSmplFileName=&argv[i][2];
          break;

        case 'a':
          sscanf(&argv[i][2], "%lf", &P1) ;
          break;
        case 'b':
          sscanf(&argv[i][2], "%lf", &P2) ;
          break;
        case 'c':
          sscanf(&argv[i][2], "%lf", &P3) ;
          break;
        case 'd':
          sscanf(&argv[i][2], "%lf", &P4) ;
          break;

        case 'x':
          sscanf(&argv[i][2], "%lf", &D1) ;
          break;
        case 'y':
          sscanf(&argv[i][2], "%lf", &D2) ;
          break;
        case 'z':
          sscanf(&argv[i][2], "%lf", &D3) ;
          break;
		
        case 'D':
          sscanf(&argv[i][2], "%d", &bBoseF) ;
          break;
        case 'T':
          sscanf(&argv[i][2], "%lf", &Temp) ;
          break;
        case 'A':
          sscanf(&argv[i][2], "%ld", &Repetition) ;
          break;

        /* Scattering parameters */
        case 'L':
          ScatMain[0] = atof(&argv[i][2]);
          break;
        case 'E':
          ScatMain[1] = atof(&argv[i][2]);
          break;
        case 'F':
          ScatMain[2] = atof(&argv[i][2]);
          break;
        case 'l':
          ScatRange[0] = atof(&argv[i][2]);
          break;
        case 'e':
          ScatRange[1] = atof(&argv[i][2]);
          break;
        case 'f':
          ScatRange[2] = atof(&argv[i][2]);
          break;

        case 's':
          ScatteringC = atof(&argv[i][2]);
          break;
        case 'm':
          AbsorptionC = atof(&argv[i][2]);
          break;

        /* sample position, size and orientation */
        case 'G':
          eGeom = (VtSmplGeom) atoi(&argv[i][2]);
          break;

        case 'X':
          PosSample[0] = atof(&argv[i][2]);
          break;
        case 'Y':
          PosSample[1] = atof(&argv[i][2]);
          break;
        case 'Z':
          PosSample[2] = atof(&argv[i][2]);
          break;

        case 't':
          Diameter = atof(&argv[i][2]);
          break;
        case 'h':
          Height = atof(&argv[i][2]);
          break;
        case 'w':
          Width  = atof(&argv[i][2]);
          break;

        case 'o':
          AnglSmplHor  = atof(&argv[i][2]);
          break;
        case 'O':
          AnglSmplVert = atof(&argv[i][2]);
          break;

        /* Output frame */
        case 'g':
          eFrame = (VtFrameGen) atoi(&argv[i][2]);
          break;

        case 'q':
          LmbdInit  = atof(&argv[i][2]);
          break;
        case 'i':
          DirInHor  = atof(&argv[i][2]);
          break;
        case 'I':
          DirInVert = atof(&argv[i][2]);
          break;

        case 'R':
          TranslOut[0] = atof(&argv[i][2]);
          break;
        case 'S':
          TranslOut[1] = atof(&argv[i][2]);
          break;
        case 'W':
          TranslOut[2] = atof(&argv[i][2]);
          break;

        case 'u':
          AnglOutHor  = atof(&argv[i][2]);
          break;
        case 'U':
          AnglOutVert = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (Temp > 0.0)
    Beta = 1.0e-06 / (KB/E_C * Temp);   // 1/kT in 1/µeV 

  if (pSmplFileName==NULL)
    Error("Parameter file name missing") ;  
	
  if((bBoseF != 0) &&	(bBoseF != 1)) 
    Error("Wrong Option for Bose Factor!");

  if(Repetition == 0)
    Error("Repetition rate must be > 0 ");

  return;

}/* End OwnInit */


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
void SetSamplePar(SampleType* pSample)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="", sGeom[20]="";
  int    nLen=sizeof(sLine)-1, iFrm=-1;
  double f_lmd =0.0, f_h   =0.0, f_v  =0.0,
         f_dlmd=0.0, f_dh  =0.0, f_dv =0.0,
         mu_sca=0.0, mu_abs=0.0,
         x     =0.0,  y    =0.0, z    =0.0, 
         diamtr=0.0, height=0.0, width=0.0,
         off_h =0.0, off_v =0.0,
         in_lmd=0.0, in_h  =0.0, in_v =0.0,
         out_x =0.0, out_y =0.0, out_z=0.0, 
         out_h =0.0, out_v =0.0;
  VtSmplGeom eGeo=VT_NO_GEOM;
  VtFrameGen eFrm=VT_NO_FRAME; 
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
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &f_lmd,  &f_h,    &f_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &f_dlmd, &f_dh,   &f_dv);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf"    , &mu_sca, &mu_abs); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x,      &y,      &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &off_h,  &off_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sGeom); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &diamtr, &height, &width);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%d",          &iFrm); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &in_lmd, &in_h,   &in_v);
      eFrm = (VtFrameGen) iFrm; 
      if (eFrm==VT_FRAME_USER)
      { 
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &out_x,  &out_y,  &out_z);
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &out_h,  &out_v);
      }

      eGeo  = SmplGeom_Txt2ID(sGeom);

      fprintf (LogFilePtr,"sample data read from parameter file: '%s':\n", pSmplFileName) ;
      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeom ==VT_NO_GEOM   && eGeo!=VT_NO_GEOM)  eGeom  = eGeo; 
      if (eFrame==VT_NO_FRAME  && eFrm!=VT_NO_FRAME) eFrame = eFrm; 
      if (ScatMain [0]==0.0 && f_lmd !=0.0) ScatMain [0]= f_lmd ;
      if (ScatMain [1]==0.0 && f_h   !=0.0) ScatMain [1]= f_h   ;
      if (ScatMain [2]==0.0 && f_v   !=0.0) ScatMain [2]= f_v   ;
      if (ScatRange[0]==0.0 && f_dlmd!=0.0) ScatRange[0]= f_dlmd/2.0;
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
      if (LmbdInit    ==0.0 && in_lmd!=0.0) LmbdInit    = in_lmd;
      if (DirInHor    ==0.0 && in_h  !=0.0) DirInHor    = in_h  ;
      if (DirInVert   ==0.0 && in_v  !=0.0) DirInVert   = in_v  ;
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

  // checks if geometry was given
  if (eGeom==VT_NO_GEOM)
    Error2("Sample geometry could not be identified", sGeom);

  // fills data structures
  FillSample(pSample, eGeom, PosSample[0], PosSample[1], PosSample[2], 0.0, 0.0, 1.0, Diameter, Height, Width, 0.0);
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

  if (PosSample[0] < 0.5*DimSample[0] || PosSample[0] < 0.5*DimSample[1] || PosSample[0] < 0.5*DimSample[2]) 
    Error("Distance to sample smaller than half the sample size in at least one dimension");

  k_reference[0] = 2.* M_PI / LmbdInit * (double) cos(DirInVert) * (double) cos(DirInHor) ;
  k_reference[1] = 2.* M_PI / LmbdInit * (double) cos(DirInVert) * (double) sin(DirInHor) ;
  k_reference[2] = 2.* M_PI / LmbdInit * (double) sin(DirInVert) ;

  if (LengthVector(k_reference) == 0.)
    Error("Zero reference wavevector not allowed");

  /* sets default values if frame for output not user defined */
  if (eFrame==VT_FRAME_STD)
  {
    /* computes angles corresponding to the output frame */
    AnglOutHor	= ScatMain[1] ;
    AnglOutVert	= ScatMain[2] ;

    /* shifts output frame origin to center of sample */
    CopyVector(PosSample, TranslOut) ;
  }

  /* converts degs in radian etc. */
  AnglSmplHor  *= M_PI/180. ;
  AnglSmplVert *= M_PI/180. ;
  AnglOutHor   *= M_PI/180. ;
  AnglOutVert  *= M_PI/180. ;

  return;
} /* End ReadParFile */


/**********************************************************************/
/** calculates ariables from input parameters and writes to log file **/
/**********************************************************************/
void  CalcAndWritePar()
{
  char   sFrm[30]="";
  double scattered_dir[3];
  double	wl, wl_scattered, q_length, scattering_angle, energy_transfer ;
  VectorType	k_scattered ;

  /* prints parameters into log file for verification */
  fprintf(LogFilePtr, "S(q,omega) parameters\n");
  fprintf(LogFilePtr, "  P1 (peak center)    : %9.4f ueV\n  P2 (peak width)     : %9.4f ueV\n  P3 (scale factor)   : %9.4f\n  P4 (ampl. 2nd peak) : %9.4f\n", P1, P2, P3, P4); 
  fprintf(LogFilePtr, "  dispersion (x,y,z)  : %9.4f  %9.4f  %9.4f ueV*Ang\n", D1, D2, D3) ;
  if (P2 <= 0.0002) fprintf(LogFilePtr,"WARNING: P2 <= 0.0002 converted to P2 = 0.0\n") ;

  if (bBoseF == TRUE)
  { fprintf(LogFilePtr,"  multiplied by Bose-factor\n  Temperature         : %9.4f K\n", Temp);
    if(Temp == 0.0) 
      Warning("T = 0 means 1 for w > 0 and 0 for w < 0");
  }
  else
  { fprintf(LogFilePtr,"  not multiplied by Bose-factor\n");
  }
	
  fprintf(LogFilePtr, "Scattering parameters\n");
  fprintf(LogFilePtr, "  final wavelength    : %9.4f +/-%9.4f Ang\n", ScatMain[0], ScatRange[0]);
  fprintf(LogFilePtr, "  final hor. angle    : %9.4f +/-%9.4f deg\n", ScatMain[1], ScatRange[1]);
  fprintf(LogFilePtr, "  final vert. angle   : %9.4f +/-%9.4f deg\n", ScatMain[2], ScatRange[2]);
  fprintf(LogFilePtr, "  scat. & abs. coeff. : %9.4f    %9.4f 1/cm\n", ScatteringC, AbsorptionC) ;
  fprintf(LogFilePtr, "  offset angles (h,v) : %9.4f %9.4f           deg\n", Degrees(AnglSmplHor), Degrees(AnglSmplVert));
  fprintf(LogFilePtr, "  reference-k (x,y,z) : %9.4f %9.4f %9.4f 1/Ang\n",   k_reference[0], k_reference[1], k_reference[2]);

  /* computes global reference values */
  scattered_dir[0]= (double) cos(ScatMain[2]*M_PI/180.) * (double) cos(ScatMain[1]*M_PI/180.) ;
  scattered_dir[1]= (double) cos(ScatMain[2]*M_PI/180.) * (double) sin(ScatMain[1]*M_PI/180.) ;
  scattered_dir[2]= (double) sin(ScatMain[2]*M_PI/180.) ;

  wl = 2 * M_PI / LengthVector(k_reference) ;
  wl_scattered = ScatMain[0] ;

  scattering_angle = AngleVectors(k_reference, scattered_dir) ;

  CopyVector(scattered_dir, k_scattered) ;
  MultiplyByScalar(k_scattered, 2 * M_PI / wl_scattered) ;
  SubVector(k_scattered, k_reference) ;
  q_length = LengthVector(k_scattered) ;

  energy_transfer = ENERGY_FROM_LAMBDA(wl) - ENERGY_FROM_LAMBDA(wl_scattered) ;

  fprintf(LogFilePtr,	"scattering triangle corresponding to q-transfer and reference-k:\n  reference wavelength: %9.4f Ang\n  scattered wavelength: %9.4f Ang\n  scattering angle    : %9.4f deg\n  |q-transfer|        : %9.4f 1/Ang\n  energy transfer     : %9.4f ueV\n",
                      wl, wl_scattered, scattering_angle, q_length, energy_transfer) ;

  FrameGen_ID2Txt(sFrm, eFrame);
  fprintf(LogFilePtr, "%s:\n", sFrm);
  fprintf(LogFilePtr, "  output position (X',Y',Z'): %9.4f %9.4f %9.4f cm \n",     TranslOut[0], TranslOut[1], TranslOut[2]);
  fprintf(LogFilePtr, "  output angles   (hor,vert): %9.4f %9.4f           deg\n", Degrees(AnglOutHor), Degrees(AnglOutVert));

  FillRotMatrixZY(RotMatrixScatter, ScatMain[2]*M_PI/180., ScatMain[1]*M_PI/180.) ; 
  FillRotMatrixZY(RotMatrixSample,  AnglSmplVert, AnglSmplHor) ;
  FillRotMatrixZY(RotMatrixOut,     AnglOutVert,  AnglOutHor) ;

  RotVector(RotMatrixSample, scattered_dir) ;

  fprintf(LogFilePtr,"repetition         : %ld\n", Repetition) ;
  if(Repetition > 1) fprintf(LogFilePtr,"\nWarning: Excessive use of repetition rate > 1 can lead to wrong results. Be sure that you have very good statistics\n" 
                                        "\nin wavelength, time, x,y,z and directions just before the sample\n") ;
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


/*******************************************************/
/** Co-ordinate transformation to output frame        **/
/*******************************************************/
void OutputTransform(VectorType Pos, VectorType Dir)
{
  /* computes neutron variables in the initial frame */
  RotBackVector(RotMatrixSample, Pos) ;
  RotBackVector(RotMatrixSample, Dir) ;
  AddVector(Pos, PosSample) ;

  /* computes neutron variables in the output frame */
  SubVector(Pos, TranslOut) ;
  RotVector(RotMatrixOut, Pos) ;
  RotVector(RotMatrixOut, Dir) ;

  /* translates neutron variables for output - X'=0. 
  {
  VectorType Path ;
  *tof = *tof - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA_PT(wl) ;
  CopyVector(Dir, Path) ;
  MultiplyByScalar(Path, - Pos[0]/ Dir[0] ) ;
  AddVector(Pos, Path) ;  
  }		     Path = displacement vector
  */

} /* End OutputTransform()*/


/*******************************************************/
/** S(q,w) scattering: new neutron variables          **/
/*******************************************************/
long S_q_w(double *wl, double *prob, VectorType Dir)
{
  int		 j ;
  double q[3], dir_fin[3], dir_inc[3], DeltaHoriz, DeltaVert, energy ;

  if(*wl == 0.0) return 0 ; 

  /* new random direction */
  CopyVector(Dir, dir_inc) ;

  DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= ScatRange[1] * M_PI/180. ;
  DeltaVert  = MonteCarlo(-1. , 1.) ; DeltaVert  *= ScatRange[2] * M_PI/180. ;

  EulerToCartesianZY( dir_fin,  &DeltaVert,  &DeltaHoriz);

  RotBackVector(RotMatrixScatter, dir_fin) ; CopyVector(dir_fin, Dir) ; 
  RotVector(RotMatrixSample, Dir) ;

  /* new wavelength*/
  if (P2 <= 0.0002)
  {
	  *wl = 1 / (double) sqrt (1 / sq(*wl) - P1 / L_2_E) ;
  }
  else
  {
    double fact; double wl_old = *wl;

    *wl = ScatMain[0] + MonteCarlo(- ScatRange[0], ScatRange[0]) ;
    energy = ENERGY_FROM_LAMBDA(wl_old) - ENERGY_FROM_LAMBDA(*wl) ;

    MultiplyByScalar(dir_inc, 2 * M_PI / wl_old) ;
    MultiplyByScalar(dir_fin, 2 * M_PI / *wl) ;

    SubVector(dir_inc, dir_fin) ; 
    for(j=0;j<3;j++) q[j]=dir_inc[j];

    fact = 1. / (*wl * sq(*wl)) /**/;
    *prob	*= fact * fabs(FunctionS_q_w(q, energy)) ;
  }

  return 1 ;
}


/*******************************************************/
/**	FunctionS_q_w(q, energy)                          **/
/*******************************************************/
double	FunctionS_q_w(VectorType q, double energy)
{
  double p ;

  /* 2 DHO 
  p =		2 * ( P3 * P2 /(sq(energy - P1) + sq(P2)) 
            - P4 * P2 /(sq(energy + P1) + sq(P2)));

  p +=	2 * ( D3 * D2 /(sq(energy - D1) + sq(D2)) 
            - D3 * D2 /(sq(energy + D1) + sq(D2))); 
  */

  /* normal vitess */
  p =	2 * P3 * (       sq(P2) /(sq(energy - P1 - Dispersion(q)) + sq(P2)) 
                 - P4 * sq(P2) /(sq(energy + P1 + Dispersion(q)) + sq(P2)));

  if (bBoseF == 1)
    p *= BoseFactor(Temp, energy);

  return p;
}


/*******************************************************/
/** Energy dispersion                                 **/
/*******************************************************/
double	Dispersion(VectorType q)
{
  /* normal vitess */
  return	D1 * q[0] + D2 * q[1] + D3 * q[2] ;

  /* helium dispersion */
  /*return	D1 + sq(LengthVector(q) - D2) / 2. /(0.9575E-03 * D3) ;  mHe4=0.9575 (2PIh)^2 [Angstrom^(-2)/meV] mass of free atom  */
}


/*******************************************************/
/** Bose factor if w in ueV                           **/
/*******************************************************/
double	BoseFactor(double T, double w)
{
  double factor=1.0;

  if (T == 0.0) 
  {
    if (w > 0.0) factor = 1.0 ;
    if (w < 0.0) factor = 0.0 ;
  }
  else
  {
         if (w > 0.0) factor = 1.0 / (exp( Beta * w) - 1.0) + 1.0;
    else if (w < 0.0) factor = 1.0 / (exp(-Beta * w) - 1.0);
    else              factor = 0.0 ;
  }
  return factor;
}
	


