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


/***********************************/
/** Prototypes of local functions **/
/***********************************/
void   OwnInit(int argc, char *argv[]);                 // Reads input parameters and sets global variables
void   OwnCleanup();                                    // Does module specific cleanup
void   ReadParFile(SampleType* pSample);                // Reads sample parameters from file
void   CalcAndWritePar();                               // Calculates arrays from input parameters and writes to log file
void   SetGeometry(char* sColor);                       // Fills the structure stGeometry for visualization 
void   OutputTransform(VectorType Pos, VectorType Dir); // Co-ordinate transformation to output frame


/******************************/
/**   Global Variables       **/
/******************************/
char      *SampleFileName=NULL;     // -P     [-]   pointer to the name of the sample file  
long       Repetition=1;            // -A     [-]   repetitions (how many trajectories to generate per incoming trajectory)
short      iColor=ANY_COLOR;        // -c     [-]   enum color: if != ANY_COLOR, only neutrons of this colour are treated 
                                             
char       Option[STRING_BUFFER];   // file   [-]   geometry parameter: "cylinder"  "hollow-cylinder"  "cuboid"  "ball" 
VectorType ScatterMain,             // file  [deg]  horizontal and vertical component (Theta, Phi) of the main scattering direction
           ScatterRange;            // file  [deg]  hor. and vert. var. (DelTheta, DelPhi) determining scat. range [Phi-DelPhi/2, Phi+DelPhi/2], Theta analogous  
double     AbsorptionC,             // file  [1/cm] Macroscopic absorption cross section 
           ScatteringC;             // file  [1/cm] Macroscopic total scattering cross section 
VectorType PosSample,               // file   [cm]  center position of the sample
           DimSample;               // file   [cm]  size of the sample
double     AnglSampleHoriz,         // file  [deg]  horizontal angle of the sample orientation, relative to standard orientation
           AnglSampleVert;          // file  [deg]  vertical angle of the sample orientation, relative to standard orientation
VectorType TranslOut;               // file   [cm]  center position of the output frame
double     AnglOutHoriz,            // file  [deg]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert;             // file  [deg]  vertical angle of the output frame, relative to input orientation                       

// Variables determined from input parameters or trajectory data
SampleType stSample;               //       sample geometry
VectorType DimSampleHol;           //       array to use 'IntersectsWithCylinder()' for hollow cylinders  
double     ProbCutoff=0.0;         //       neutron weight, below which the trajectory is removed
double     RotMatrixSample[3][3],  //       rotation matrix to transfer to coordinate system of the sample
           RotMatrixOut[3][3],     //       rotation matrix to transfer to the output coordinate system
           RotMatrixScatter[3][3]; //       rotation matrix to transfer into coordinate system of the scattering direction


/******************************/
/**   Main Program           **/
/******************************/
int main(int argc, char **argv) 
{
  long       repet=0, i=0;
  double     TOF, WL, Prob, 
             PathLength   =0.0, PathLengthHol   =0.0, 
             MaxPathLength=0.0, MaxPathLengthHol=0.0;
  VectorType Pos1f, Pos2f, Pos3f, Pos4f, 
             Pos1v, Pos2v, Pos3v, Pos4v, Pos, Dir ;
  Neutron    Neutrons ;
  
  // initialisation
  // --------------
 _eModule = MCN_SMPL_EL_ISO;

  Init   (argc, argv, _eModule);
  PrintModuleName(_eModule, "2.7");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* reads file containing sample parameters */
  ReadParFile(&stSample);

  /* determines the dependent parameters and write out important parameters */
  CalcAndWritePar();

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
          MaxPathLengthHol = PathLengthHol = 0.; 
            
          InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;
        
          /* translates and rotates into frame of the sample  */
          SubVector(InputNeutrons[i].Position, PosSample) ;
          RotVector(RotMatrixSample, InputNeutrons[i].Position) ;
          RotVector(RotMatrixSample, InputNeutrons[i].Vector) ;
        
          /* gives intersection positions with sample */
          if (Option[1] == 'y' && IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) 
             goto getlost ;

          if (Option[1] == 'o')
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

          if(Option[1] == 'u' && IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) 
            goto getlost ; 
	      
          if(Option[1] == 'a' && IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) 
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

            /* scattering position and TOF untill scattering */	

            SubVector(Pos2v, Pos1v) ;					/*maximal path vector*/ 
            MaxPathLength = LengthVector(Pos2v) + MaxPathLengthHol ; 

            MultiplyByScalar(Pos2v, MonteCarlo(0.,1.)) ;	 /*random path vector in cylinder untill scattering */

            PathLength = LengthVector(Pos2v) + PathLengthHol;
            AddVector(Pos1v, Pos2v) ;

            { VectorType propag;

              CopyVector(Pos1v, propag);
              SubVector(propag, Pos);

              TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;
            }

            CopyVector(Pos1v, Pos) ;						/*scattering position */

            /* attenuation untill scattering normalized to maximal path */

            // Prob *= (double) exp( - PathLength * (AbsorptionC * WL + ScatteringC));
            Prob *= (double) exp( - PathLength * (AbsorptionC * WL));
            Prob *= MaxPathLength * ScatteringC ; 

            /* scattering: new neutron variables*/ 
            {
              double	dir_fin[3], DeltaHoriz, DeltaVert ;
            
              /* new random direction  */
            
              DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= ScatterRange[1]/2. * M_PI/180. ;
              DeltaVert  = MonteCarlo(-1. , 1.) ; DeltaVert  *= ScatterRange[2]/2. * M_PI/180. ;
             
              EulerToCartesianZY( dir_fin,  &DeltaVert,  &DeltaHoriz);
            
              RotBackVector(RotMatrixScatter, dir_fin) ; CopyVector(dir_fin, Dir) ; 
              RotVector    (RotMatrixSample, Dir) ;
            }


            /* Attenuation succeeding scattering */

            if (Option[1] == 'y' && IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) 
              goto getlost2 ; 
		
            if(Option[1] == 'o')
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
                    { VectorType Propag1;

                      CopyVector(Pos4v, Propag1);
                      SubVector (Propag1, Pos3v);
                      PathLengthHol = LengthVector(Propag1); 
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

            if (Option[1] == 'u' && IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) 
              goto getlost2 ; 
        	
            if (Option[1] == 'a' && IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) 
              goto getlost2 ; 


            /* path in the sample after scattering */
            {
              VectorType Pos_final ;
            
              CopyVector(Pos2v, Pos_final) ;	  
              SubVector (Pos_final, Pos) ;	  
              PathLength = LengthVector(Pos_final) + PathLengthHol;  
            }

            if(PathLengthHol != 0.) CopyVector(Pos4v, Pos2v);  /* for hollow cylinder option: set output position to where it crosses the outer cylinder if crossed  */

            /* Test: set output to scattering positions:  CopyVector(Pos, Pos2v); */

            // Prob *= (double) exp( - PathLength * (AbsorptionC * WL + ScatteringC));
            Prob *= (double) exp( - PathLength * (AbsorptionC * WL));

            /* Output matters */
            { VectorType propag;

              CopyVector(Pos2v, propag);
              SubVector(propag, Pos);

              TOF += LengthVector(propag) / V_FROM_LAMBDA(WL) ;
            }

            OutputTransform(Pos2v, Dir) ;
		
            Prob *= ScatterRange[1]/180. * sin(ScatterRange[2]* M_PI/180.) /4.;  /* solid angle / 4pi */
            if (Prob <= ProbCutoff) goto getlost2 ;

            /* transmit coordinates which were not changed, the rest overwrite below */
            Neutrons = InputNeutrons[i]; 

            Neutrons.Time = TOF ;	
            Neutrons.Probability = Prob/Repetition ;

            CopyVector(Pos2v, Neutrons.Position) ;	
            CopyVector(Dir, Neutrons.Vector) ;	

            /* writes output binary file */
            WriteNeutron(&Neutrons) ;

          getlost2: ;

          }  /*repetition*/
              /* here continues if neutron gets lost */
              
        getlost: ;   
        }
        else  // icolor != 0
        {
          Neutrons = InputNeutrons[i]; 
          SubVector(Neutrons.Position, PosSample) ;
          WriteNeutron(&Neutrons) ;
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
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);	

  return 0;
}


/*******************************************************/
/** Own initialization of the sample_reflectom module **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  ProbCutoff=wei_min;
	
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
      	
      case 'c':
        sscanf(&argv[1][2], "%hd", &iColor) ;
        break;

    }
    argc--;
    argv++;
  }
	
  if (Repetition < 1)
    Error("Repetition rate must be >= 1") ;  

  if (SampleFileName==NULL)
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
void ReadParFile(SampleType* pSample)
{
  // opens file containing sample parameters (program exit in case of error)
  FILE* pSmplFile = OpenInputFile2(SampleFileName, "sample data", "r");
  int i;

  /* reads from file by using ReadParF(pSmplFile) and ReadParComment(pSmplFile) */
  ScatterMain [1]=ReadParF(pSmplFile); ScatterMain [2]=ReadParF(pSmplFile); ReadParComment(pSmplFile);
  ScatterRange[1]=ReadParF(pSmplFile); ScatterRange[2]=ReadParF(pSmplFile); ReadParComment(pSmplFile);

  ScatteringC =ReadParF(pSmplFile);    AbsorptionC =ReadParF(pSmplFile);    ReadParComment(pSmplFile) ;
  PosSample[0]=ReadParF(pSmplFile);    PosSample[1]=ReadParF(pSmplFile);    PosSample[2]=ReadParF(pSmplFile); ReadParComment(pSmplFile);

  AnglSampleHoriz=ReadParF(pSmplFile); AnglSampleVert=ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  ReadParString(pSmplFile, Option);    ReadParComment(pSmplFile);

  DimSample[0]=ReadParF(pSmplFile); DimSample[2]=ReadParF(pSmplFile); DimSample[1]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  TranslOut[0]=ReadParF(pSmplFile); TranslOut[1]=ReadParF(pSmplFile); TranslOut[2]=ReadParF(pSmplFile) ; ReadParComment(pSmplFile) ;
  AnglOutHoriz=ReadParF(pSmplFile); AnglOutVert =ReadParF(pSmplFile); ReadParComment(pSmplFile) ;

  if ((PosSample[0] < DimSample[0])||(PosSample[0] < DimSample[1])||(PosSample[0] < DimSample[2])) 
    Warning("Distance to sample is smaller than at least one sample dimension"); 

  /* changes geometry text to small letters */
  for (i=0; i < strlen(Option); i++)
  { if (isupper(Option[i]))
      Option[i] = tolower(Option[i]);
  }

  /*	 checks some values */
  if ((Option[1] != 'y') && (Option[1] != 'o')  && (Option[1] != 'u') && (Option[1] != 'a'))
    Error("No valid geometry option");

  pSample->Position[0] = PosSample[0];
  pSample->Position[1] = PosSample[1];
  pSample->Position[2] = PosSample[2];
 
  if(Option[1] == 'y') 
  {
    pSample->SG.Cyl.r = DimSample[0];
    pSample->SG.Cyl.height = DimSample[1];
    pSample->Type = VT_CYL;

    fprintf(LogFilePtr,"             sample geometry:	'cylinder'\n") ;
  }
  if(Option[1] == 'o') 
  {
   pSample->SG.Cyl.r = DimSample[0];
   pSample->SG.Cyl.height = DimSample[1];
   pSample->Type = VT_HOL_CYL;

    fprintf(LogFilePtr,"             sample geometry:	'hollow cylinder'\n") ;
  }
  if(Option[1] == 'u')
  { 
    pSample->SG.Cube.thickness = DimSample[0];
    pSample->SG.Cube.width = DimSample[1];
    pSample->SG.Cube.height = DimSample[2];
    pSample->Type = VT_CUBE;

    fprintf(LogFilePtr,"             sample geometry:	'cuboid'\n") ;
  }
  if(Option[1] == 'a') 
  {
    pSample->SG.Ball.r = DimSample[0];
    pSample->Type = VT_SPHERE;

    fprintf(LogFilePtr,"             sample geometry:	'sphere'\n") ;
  }

  /* converts degs in radian etc. */
  AnglSampleHoriz *= M_PI/180. ;
  AnglSampleVert  *= M_PI/180. ;
  AnglOutHoriz    *= M_PI/180. ;
  AnglOutVert     *= M_PI/180. ;

  /* Hollow cylinder option */
  CopyVector(DimSample, DimSampleHol); 
  DimSampleHol[0] = DimSample[1];

  fclose(pSmplFile);
 
}/* End ReadParFile */


/**********************************************************************/
/** calculates ariables from input parameters and writes to log file **/
/**********************************************************************/
void  CalcAndWritePar()
{
  double scattered_dir[3];

  fprintf(LogFilePtr,"  repetition rate		=     %ld\n", Repetition) ;
  if(Repetition > 20)
    fprintf(LogFilePtr,"Warning: Excessive use of repetition rate >> 1 can lead to wrong results. Be sure that you have very good statistics\n" 
                       "in wavelength, time, x,y,z and directions just before the sample\n") ;

  /* computes global reference values */
  scattered_dir[0]= (double) cos(ScatterMain[2]*M_PI/180.) * (double) cos(ScatterMain[1]*M_PI/180.) ;
  scattered_dir[1]= (double) cos(ScatterMain[2]*M_PI/180.) * (double) sin(ScatterMain[1]*M_PI/180.) ;
  scattered_dir[2]= (double) sin(ScatterMain[2]*M_PI/180.) ;

  fprintf(LogFilePtr,"  scattered dir.		:     %lf    %lf    %lf\n", scattered_dir[0], scattered_dir[1], scattered_dir[2]) ;

  FillRotMatrixZY(RotMatrixScatter, ScatterMain[2]*M_PI/180., ScatterMain[1]*M_PI/180.) ; 
  FillRotMatrixZY(RotMatrixSample,  AnglSampleVert,           AnglSampleHoriz) ;
  FillRotMatrixZY(RotMatrixOut,     AnglOutVert,              AnglOutHoriz) ;

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

    SetSampleGeometry(&stSample);
  }
}


/*******************************************************/
/** Co-ordinate transformation to output frame        **/
/*******************************************************/
void OutputTransform(VectorType pos, VectorType dir)
{
  /* computes neutron variables in the initial frame */
  RotBackVector(RotMatrixSample, pos) ;
  RotBackVector(RotMatrixSample, dir) ;
  AddVector(pos, PosSample) ;

  /* computes neutron variables in the output frame */
  SubVector(pos, TranslOut) ;
  RotVector(RotMatrixOut, pos) ;
  RotVector(RotMatrixOut, dir) ;

} /* End OutputTransform() */


