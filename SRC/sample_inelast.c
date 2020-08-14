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
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
void   ReadParFile(SampleType* pSample);                      // Reads sample parameters from file
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
McCompID   _eModule=MCN_SMPL_INELAST;

char      *SampleFileName=NULL;     // -P     [-]   pointer to the name of the sample file  
long       Repetition=1;            // -A     [-]   repetitions (how many trajectories to generate per incoming trajectory)
double     P1=0.0,                  // -a    [ueV]  reference energy (= peak position for vanishing energy dispersion)
           P2=0.0,                  // -b    [ueV]  Lorentzian width (HWHM)
           P3=0.0,                  // -c     [-]   scaling factor (for both peaks)
           P4=0.0,                  // -d     [-]   intensity of the second peak relative to the first
           D1=0.0,                  // -x     [cm]  linear energy dispersion coefficient of momentum component 
           D2=0.0,                  // -y    [deg]  linear energy dispersion coefficient of momentum component y
           D3=0.0,                  // -z    [deg]  linear energy dispersion coefficient of momentum component z
           Temp=0.0;                // -T     [cm]  temperature of the sample
int        bBoseF=FALSE;                // -D    [deg]  flag: multiplication with the Bose factor

char       Option[STRING_BUFFER];   // file   [-]   geometry parameter: "cylinder"  "hollow-cylinder"  "cuboid"  "ball" 
VectorType ScatterMain,             // file  [deg]  mean wavelength and hor. and vert. direction (Theta, Phi) of the scattered neutron
           ScatterRange;            // file  [deg]  variation in wavelength, hor. and vert. direction of the scattered neutron
double     AbsorptionC=0.0,         // file  [1/cm] Macroscopic absorption cross section 
           ScatteringC=0.0;         // file  [1/cm] Macroscopic total scattering cross section 
VectorType PosSample,               // file   [cm]  center position of the sample
           DimSample;               // file   [cm]  size of the sample
double     AnglSampleHoriz=0.0,     // file  [rad]  horizontal angle of the sample orientation, relative to standard orientation
           AnglSampleVert=0.0;      // file  [rad]  vertical angle of the sample orientation, relative to standard orientation
int        bUser=FALSE;             // file   [-]   flag: user defined output frame
double     LmbdInit =0.0,           // file  [Ang]  initial wavelength
           DirInHor =0.0,           // file  [deg]  horizontal orientation of the incoming neutron
           DirInVert=0.0;           // file  [deg]  vertical orientation of the incoming neutron
VectorType TranslOut;               // file   [cm]  center position of the output frame
double     AnglOutHoriz=0.0,        // file  [rad]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert =0.0;        // file  [rad]  vertical angle of the output frame, relative to input orientation    

// Variables determined from input parameters or trajectory data
SampleType stSample;                //       sample geometry
VectorType DimSampleHol,            //       array to use 'IntersectsWithCylinder()' for hollow cylinders  
           k_reference;             //       initial k-vector
double     ProbCutoff=0.0;          //       neutron weight, below which the trajectory is removed
double     RotMatrixSample[3][3],   //       rotation matrix to transfer to coordinate system of the sample
           RotMatrixOut[3][3],      //       rotation matrix to transfer to the output coordinate system
           RotMatrixScatter[3][3];  //       rotation matrix to transfer into coordinate system of the scattering direction


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long       repet=0, i=0;
  double     TOF, WL, Prob, 
             PathLength   =0.0, PathLengthHol   =0.0, 
             MaxPathLength=0.0, MaxPathLengthHol=0.0;
  VectorType Pos1f, Pos2f, Pos3f, Pos4f, 
             Pos1v, Pos2v, Pos3v, Pos4v,   
             propag, propag1, 
             Pos, Dir,
             Pos_final;
  Neutron	   Neutrons ;

  // initialisation
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.5");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* reads file containing sample parameters */
  ReadParFile(&stSample);

  /* determines the dependent parameters and write out important parameters */
  CalcAndWritePar();

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
  {
    for (i=0;i<NumNeutGot ;i++)
    { 
      CHECK;

      MaxPathLengthHol = PathLengthHol = 0.; 
      InputNeutrons[i].Vector[0]		= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

      /* translates and rotates into frame of the sample  */
      SubVector(InputNeutrons[i].Position, PosSample) ;
      RotVector(RotMatrixSample, InputNeutrons[i].Position) ;
      RotVector(RotMatrixSample, InputNeutrons[i].Vector) ;

      /* gives intersection positions with sample */
      if(Option[1] == 'y')
      {
        if (IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) 
        goto getlost ; 
      }

      if(Option[1] == 'o')
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


      if(Option[1] == 'u')
      {
        if(IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
      }

      if(Option[1] == 'a')
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
        if (Option[1] == 'y')
        {
          if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) 
            goto getlost2 ; 
        }

        if (Option[1] == 'o')
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

        if (Option[1] == 'u')
        {
          if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) goto getlost2 ; 
        }
        if (Option[1] == 'a')
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

        Prob *= ScatterRange[1]/180. * sin(ScatterRange[2]* M_PI/180.)/4.;  /* solid angle / 4pi */
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
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:
  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup(); 

 /* Do the general cleanup */
	Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);	

  return 0;
}


/*******************************************************/
/** Own initialization of the sample_inelast module   **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  ProbCutoff=wei_min ;
	
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        SampleFileName=&argv[1][2];
        break;
		
      case 'A':
        sscanf(&argv[1][2], "%ld", &Repetition) ;
        break;

      case 'D':
        sscanf(&argv[1][2], "%d", &bBoseF) ;
        break;

      case 'a':
        sscanf(&argv[1][2], "%lf", &P1) ;
        break;
      case 'b':
        sscanf(&argv[1][2], "%lf", &P2) ;
        break;
      case 'c':
        sscanf(&argv[1][2], "%lf", &P3) ;
        break;
      case 'd':
        sscanf(&argv[1][2], "%lf", &P4) ;
        break;

      case 'T':
        sscanf(&argv[1][2], "%lf", &Temp) ;
        break;

      case 'x':
        sscanf(&argv[1][2], "%lf", &D1) ;
        break;
      case 'y':
        sscanf(&argv[1][2], "%lf", &D2) ;
        break;
      case 'z':
        sscanf(&argv[1][2], "%lf", &D3) ;
        break;
    }
    argc--;
    argv++;
  }

  if (SampleFileName==NULL)
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
void ReadParFile(SampleType* pSample)
{
  // opens file containing sample parameters (program exit in case of error)
  FILE* pSmplFile = OpenInputFile2(SampleFileName, "sample data", "r");

  fprintf(LogFilePtr,"sampple data read from parameter file: '%s':\n",SampleFileName) ;

  /* reads from file by using ReadParF(pSmplFile) and ReadParComment(pSmplFile) */
  ScatterMain [0]=ReadParF(pSmplFile) ; ScatterMain [1]=ReadParF(pSmplFile) ; ScatterMain [2]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  ScatterRange[0]=ReadParF(pSmplFile) ; ScatterRange[1]=ReadParF(pSmplFile) ; ScatterRange[2]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  ScatteringC    =ReadParF(pSmplFile) ; AbsorptionC    =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  PosSample[0]   =ReadParF(pSmplFile) ; PosSample[1]   =ReadParF(pSmplFile) ; PosSample[2]   =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  AnglSampleHoriz=ReadParF(pSmplFile) ; AnglSampleVert =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  ReadParString(pSmplFile, Option) ; ReadParComment(pSmplFile) ;
  DimSample[0]   =ReadParF(pSmplFile) ; DimSample[2]   =ReadParF(pSmplFile) ; DimSample[1]   =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  bUser          =ReadParI(pSmplFile) ; ReadParComment(pSmplFile) ;
  LmbdInit       =ReadParF(pSmplFile) ; DirInHor       =ReadParF(pSmplFile) ; DirInVert      =ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;

  if (bUser == 1)
  {
    TranslOut[0]=ReadParF(pSmplFile) ; TranslOut[1]=ReadParF(pSmplFile) ; TranslOut[2]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
    AnglOutHoriz=ReadParF(pSmplFile) ; AnglOutVert=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  }

  if ((PosSample[0] < DimSample[0])||(PosSample[0] < DimSample[1])||(PosSample[0] < DimSample[2])) 
    Error("Distance to sample smaller than at least one sample dimension");
		
  /*	 checks some values */
  if ((Option[1] != 'y') && (Option[1] != 'o') && (Option[1] != 'u') && (Option[1] != 'a'))
    Error("No valid geometry option") ;

  k_reference[0] = 2.* M_PI / LmbdInit * (double) cos(DirInVert) * (double) cos(DirInHor) ;
  k_reference[1] = 2.* M_PI / LmbdInit * (double) cos(DirInVert) * (double) sin(DirInHor) ;
  k_reference[2] = 2.* M_PI / LmbdInit * (double) sin(DirInVert) ;

  if (LengthVector(k_reference) == 0.)
    Error("Zero reference wavevector not allowed");

  /* sets default values if frame for output not user defined */
  if (bUser != 1.)
  {
    if(LengthVector(k_reference) == 0.)
      Error("Zero reference wavevector not allowed");

    /* computes angles corresponding to the output frame */
    AnglOutHoriz	= ScatterMain[1] ;
    AnglOutVert		= ScatterMain[2] ;

    /* shifts output frame origin to center of sample */
    CopyVector(PosSample, TranslOut) ;
  }
	
  /* prints parameters into log file for verification */

  fprintf(LogFilePtr,"	random main w, y, z		=  %9.4f, %9.4f, %9.4f\n	range x, y, z		=  %9.4f, %9.4f, %9.4f\n	absorption constant	=     %9.4e\n	cutoff probability		=     %8.1e\n",
                     ScatterMain[0], ScatterMain[1], ScatterMain[2], ScatterRange[0], ScatterRange[1], ScatterRange[2], AbsorptionC, ProbCutoff) ;
  fprintf(LogFilePtr,"	position x, y, z		=  %9.4f, %9.4f, %9.4f\n	thickn./radius, height, width  =  %9.4f, %9.4f, %9.4f\n	offset angle horiz		=  %9.4f\n	offset angle vert		=  %9.4f\n",
                     PosSample[0], PosSample[1], PosSample[2], DimSample[0], DimSample[2], DimSample[1], AnglSampleHoriz, AnglSampleVert) ;
  fprintf(LogFilePtr,"	reference-k x, y, z		=  %9.4f, %9.4f, %9.4f\n",
                     k_reference[0], k_reference[1], k_reference[2]) ;

  pSample->Position[0] = PosSample[0];
  pSample->Position[1] = PosSample[1];
  pSample->Position[2] = PosSample[2];

  if(Option[1] == 'y') 
  {
    pSample->SG.Cyl.r      = DimSample[0];
    pSample->SG.Cyl.height = DimSample[1];
    pSample->Type = VT_CYL;

    fprintf(LogFilePtr,"             sample geometry:	'cylinder'\n") ;
  }
  if(Option[1] == 'o') 
  {
    pSample->SG.Cyl.r      = DimSample[0];
    pSample->SG.Cyl.height = DimSample[1];
    pSample->Type = VT_CYL;

    fprintf(LogFilePtr,"             sample geometry:	'hollow cylinder'\n") ;
  }
  if(Option[1] == 'u') 
  {
    pSample->SG.Cube.thickness = DimSample[0];
    pSample->SG.Cube.width     = DimSample[1];
    pSample->SG.Cube.height    = DimSample[2];
    pSample->Type = VT_CUBE;

    fprintf(LogFilePtr,"             sample geometry:	'cuboid'\n") ;
  }
  if(Option[1] == 'a') 
  {
    pSample->SG.Ball.r = DimSample[0];
    pSample->Type = VT_SPHERE;

    fprintf(LogFilePtr,"             sample geometry:	'sphere'\n") ;
  }

  if(bUser != 1) fprintf(LogFilePtr,"standard frame generation:\n") ;
  if(bUser == 1) fprintf(LogFilePtr,"user defined frame:\n") ;

  fprintf(LogFilePtr,"	output horizontal angle	= %9.4f\n	output vertical angle	= %9.4f\n	X',Y',Z'			= %9.4f, %9.4f, %9.4f\n", 
                     AnglOutHoriz, AnglOutVert, TranslOut[0], TranslOut[1], TranslOut[2]) ;

  /* converts degs in radian etc. */
  AnglSampleHoriz *= M_PI/180. ;
  AnglSampleVert  *= M_PI/180. ;
  AnglOutHoriz    *= M_PI/180. ;
  AnglOutVert     *= M_PI/180. ;

  /* Hollow cylinder option */
  CopyVector(DimSample, DimSampleHol); 
  DimSampleHol[0] = DimSample[1];

  fclose(pSmplFile);

} /* End ReadParFile */


/**********************************************************************/
/** calculates ariables from input parameters and writes to log file **/
/**********************************************************************/
void  CalcAndWritePar()
{
  double scattered_dir[3];
  double	wl, wl_scattered, q_length, scattering_angle, energy_transfer ;
  VectorType	k_scattered ;

  fprintf(LogFilePtr,"	P1			=  %9.4f\n	P2			=  %9.4f\n	P3			=  %9.4f\n	P4			=  %9.4f\n	D1			=  %9.4f\n	D2			=  %9.4f\n	D3			=  %9.4f\n	temperature		=  %9.4f\n",
                     P1, P2, P3, P4, D1, D2, D3, Temp) ;
  if (P2 <= 0.0002) fprintf(LogFilePtr,"WARNING: P2 <= 0.0002 converted to P2 = 0.0\n") ;

  if(bBoseF == 1) fprintf(LogFilePtr,"multiplied by Bose-factor");
  if(bBoseF != 1) fprintf(LogFilePtr,"not multiplied by Bose-factor");
  if(bBoseF == 1 && Temp == 0.0) fprintf(LogFilePtr," (T = 0 means 1 for w > 0 and 0 for w < 0)");
  fprintf(LogFilePtr,"\n");

  /* computes global reference values */
  scattered_dir[0]= (double) cos(ScatterMain[2]*M_PI/180.) * (double) cos(ScatterMain[1]*M_PI/180.) ;
  scattered_dir[1]= (double) cos(ScatterMain[2]*M_PI/180.) * (double) sin(ScatterMain[1]*M_PI/180.) ;
  scattered_dir[2]= (double) sin(ScatterMain[2]*M_PI/180.) ;

  wl = 2 * M_PI / LengthVector(k_reference) ;
  wl_scattered = ScatterMain[0] ;

  scattering_angle = AngleVectors(k_reference, scattered_dir) ;

  CopyVector(scattered_dir, k_scattered) ;
  MultiplyByScalar(k_scattered, 2 * M_PI / wl_scattered) ;
  SubVector(k_scattered, k_reference) ;
  q_length = LengthVector(k_scattered) ;

  energy_transfer = ENERGY_FROM_LAMBDA(wl) - ENERGY_FROM_LAMBDA(wl_scattered) ;

  fprintf(LogFilePtr,	"scattering triangle corresponding to q-transfer and reference-k:\n	reference wavelength	= %9.4f A\n	scattered wavelength	= %9.4f A\n	scattering angle		= %9.4f deg\n	|q-transfer|		= %9.4f A-1\n	energy transfer		= %9.4f ueV\n",
                      wl, wl_scattered, scattering_angle, q_length, energy_transfer) ;

  FillRotMatrixZY(RotMatrixScatter, ScatterMain[2]*M_PI/180., ScatterMain[1]*M_PI/180.) ; 
  FillRotMatrixZY(RotMatrixSample, AnglSampleVert, AnglSampleHoriz) ;
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz) ;

  RotVector(RotMatrixSample, scattered_dir) ;

  fprintf(LogFilePtr,"	repetition  		=     %ld\n", Repetition) ;
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

  DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= ScatterRange[1]/2. * M_PI/180. ;
  DeltaVert = MonteCarlo(-1. , 1.) ; DeltaVert *= ScatterRange[2]/2. * M_PI/180. ;

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

    *wl = ScatterMain[0] + MonteCarlo(- ScatterRange[0]/2, ScatterRange[0]/2) ;
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
  double beta;

  if (T == 0.0) 
  {
    if (w > 0.0) return 1.0 ;
    if (w < 0.0) return 0.0 ;
  }

  beta = 11.605 / T / 1.e3 ; /* unit 1/ueV ! */

  if (w > 0.0) return 1.0 / (exp( beta * w) - 1.0) + 1.0;
  if (w < 0.0) return 0.0 / (exp(-beta * w) - 1.0);
  else         return 0.0 ;
}
	


