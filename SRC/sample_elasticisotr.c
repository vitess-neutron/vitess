/********************************************************************************************/
/*  VITESS module 'sample_elasticisotr.c'                                                   */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant changes for 'instrument.dat'                                */
/* 1.3  MAY 2004  G. Zsigmond   normalise with repetition                                   */
/* 1.4  JUL 2004  G. Zsigmond   error message for sample position                           */
/* 1.5  JUL 2004  G. Zsigmond   including hollow cylinder sample                            */
/* 1.5a DEC 2004  K. Lieutenant no attenuation by scattering, (error of count rates)        */
/* 1.5b DEC 2004  K. Lieutenant correction: algorithm for repetitions                       */
/* 1.6  JAN 2011  K. Lieutenant option to scatter only neutrons of a special color          */
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

/* START HEADER STORY */

#define	STRING_BUFFER 50

FILE        *Par_Sample, *XFILE; 
char        Option[STRING_BUFFER], *ParameterFileName, XFileName[STRING_BUFFER], *SampleFileName;
long        User, Repetition, BoseF, repet,  i ;
short       iColor=0;                // if != 0, only neutrons of this colour are treated 
double      TOF, WL, Prob, MaxPathLength, MaxPathLengthHol=0., PathLength, PathLengthHol=0., scattered_dir[3], l_reference, h_reference, v_reference;
double      P1, P2, P3, P4, Temperature, D1, D2, D3, AnglSampleHoriz, AnglSampleVert, AnglOutHoriz, AnglOutVert ;
double      RotMatrixSample[3][3], RotMatrixScatter[3][3], RotMatrixOut[3][3], RotMatrixDelta[3][3];
double      AbsorptionC, ScatteringC, ProbCutoff;
VectorType  random_main, random_range, k_reference, PosSample, DimSample, DimSampleHol, TranslOut;
VectorType  Pos1f, Pos2f, Pos3f, Pos4f, 
            Pos1v, Pos2v, Pos3v, Pos4v, Pos, Dir ;
Neutron     Neutrons ;


long        S_q_w(double *wl, double *prob, VectorType Dir);
double      FunctionS_q_w(VectorType q, double energy);
double      Dispersion(VectorType q);
double      BoseFactor(double T, double w);
void        SampleS_q_w();
void        OutputTransformations(VectorType Pos, VectorType Dir);
void        ReadParameterFile() ;
void        OwnInit(int argc, char *argv[]) ;
void        OwnCleanup() ;

/* FINISH HEADER STORY */


int main(int argc, char **argv) {
  
  /* Initialize the program according to the parameters given  */ 
  
  Init(argc, argv, VT_SMPL_EL_ISO); 
  print_module_name("sample_elasticisotr 1.6a") ;
  OwnInit(argc, argv);

  DECLARE_ABORT;
	
  /* Get the neutrons from the file */

  while ((ReadNeutrons())!= 0) {
    /* here is what happens to the neutron */
    CHECK;
      
    for(i=0;i<NumNeutGot ;i++) {
      CHECK;

      if (iColor==0 || iColor==InputNeutrons[i].Color)
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
            goto getlost ; 
          else 
          {
            if(IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 0) 
              CopyVector(Pos4f, Pos2f);

            if(IntersectionWithCylinder(DimSampleHol, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos2f, Pos3f) == 1)
            { double r=MonteCarlo(-1.,1);

              if ((CompareVectors(Pos1f, Pos2f)==1)&&(CompareVectors(Pos3f, Pos4f)==1)) 
                goto getlost;
      	
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
            
            DeltaHoriz = MonteCarlo(-1. , 1.) ; DeltaHoriz *= random_range[1]/2. * M_PI/180. ;
            DeltaVert  = MonteCarlo(-1. , 1.) ; DeltaVert  *= random_range[2]/2. * M_PI/180. ;
             
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
              goto getlost2 ; 
            else 
            {
              if (IntersectionWithCylinder(DimSampleHol, Pos, Dir, Pos2v, Pos3v) == 0) 
                CopyVector(Pos4v, Pos2v);
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
                  PathLengthHol = 0.; 
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

          OutputTransformations(Pos2v, Dir) ;
		
          Prob *= random_range[1]/180. * sin(random_range[2]* M_PI/180.) /4.;  /* solid angle / 4pi */
          if(Prob <= ProbCutoff) goto getlost2 ;

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
    }   // for loop over trajectories
  }     // do loop, read trajectories
     

  /* Do the general cleanup */

 my_exit: 
  fprintf(LogFilePtr," \n") ;

  OwnCleanup(); 

  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);	

  return 0;
}


	

/* Output matters */

void OutputTransformations(VectorType Pos, VectorType Dir)
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
  }    Path = displacement vector 
  */

} /* End OutputTransformations() */


/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
  ProbCutoff=wei_min ;

  /*    INPUT  */
	
  while(argc>1)
  {
    switch(argv[1][1])
    {
      			
      case 'P':
        if((Par_Sample = fopen(&argv[1][2],"r"))==NULL)
          {
            fprintf(LogFilePtr,"\nParameter file '%s' not found\n",&argv[1][2]);
            exit(0);
          }
        ParameterFileName=&argv[1][2];
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
	

  if(Repetition == 0)
  {
    fprintf(LogFilePtr,"Repetition rate must be > 0 !\n") ;  
    exit(0) ;
  }

  /* reads parameter file */
  ReadParameterFile() ; 

  if (Par_Sample != NULL) fclose(Par_Sample) ;

	
  /* computes global reference values */

  scattered_dir[0]= (double) cos(random_main[2]*M_PI/180.) * (double) cos(random_main[1]*M_PI/180.) ;
  scattered_dir[1]= (double) cos(random_main[2]*M_PI/180.) * (double) sin(random_main[1]*M_PI/180.) ;
  scattered_dir[2]= (double) sin(random_main[2]*M_PI/180.) ;

  fprintf(LogFilePtr,"\n	scattered dir.		:     %lf    %lf    %lf", scattered_dir[0], scattered_dir[1], scattered_dir[2]) ;

  FillRotMatrixZY(RotMatrixScatter, random_main[2]*M_PI/180., random_main[1]*M_PI/180.) ; 
  FillRotMatrixZY(RotMatrixSample,  AnglSampleVert,           AnglSampleHoriz) ;
  FillRotMatrixZY(RotMatrixOut,     AnglOutVert,              AnglOutHoriz) ;

}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{


}/* End OwnCleanup */


/* ReadParameterFile() reads the parameters from file */

void ReadParameterFile()
{

  SampleType sample;

  fprintf(LogFilePtr,"\n	repetition rate		=     %ld", Repetition) ;
  if(Repetition > 20)
    fprintf(LogFilePtr,"\nWarning: Excessive use of repetition rate >> 1 can lead to wrong results. Be sure that you have very good statistics" 
	                    "\nin wavelength, time, x,y,z and directions just before the sample") ;

  /* reads from file by using ReadParF(Par_Sample) and ReadParComment(Par_Sample) */
  random_main [1]=ReadParF(Par_Sample); random_main [2]=ReadParF(Par_Sample); ReadParComment(Par_Sample);
  random_range[1]=ReadParF(Par_Sample); random_range[2]=ReadParF(Par_Sample); ReadParComment(Par_Sample);

  ScatteringC =ReadParF(Par_Sample);    AbsorptionC =ReadParF(Par_Sample);    ReadParComment(Par_Sample) ;
  PosSample[0]=ReadParF(Par_Sample);    PosSample[1]=ReadParF(Par_Sample);    PosSample[2]=ReadParF(Par_Sample); ReadParComment(Par_Sample);

  AnglSampleHoriz=ReadParF(Par_Sample); AnglSampleVert=ReadParF(Par_Sample);  ReadParComment(Par_Sample);
  ReadParString(Par_Sample, Option);    ReadParComment(Par_Sample);

  DimSample[0]=ReadParF(Par_Sample); DimSample[2]=ReadParF(Par_Sample); DimSample[1]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;
  TranslOut[0]=ReadParF(Par_Sample); TranslOut[1]=ReadParF(Par_Sample); TranslOut[2]=ReadParF(Par_Sample) ; ReadParComment(Par_Sample) ;
  AnglOutHoriz=ReadParF(Par_Sample); AnglOutVert =ReadParF(Par_Sample); ReadParComment(Par_Sample) ;


  if((PosSample[0] < DimSample[0])||(PosSample[0] < DimSample[1])||(PosSample[0] < DimSample[2])) 
    fprintf(LogFilePtr,"\nWarning:Distance to sample is smaller than at least one sample dimension!"); 
		
		
  /*	 checks some values */
  if((Option[1] != 'y') && (Option[1] != 'o')  && (Option[1] != 'u') && (Option[1] != 'a'))
  {
    fprintf(LogFilePtr,"\nNo valid geometry option!\n") ;
    exit(0) ;
  }

  sample.Position[0] = PosSample[0];
  sample.Position[1] = PosSample[1];
  sample.Position[2] = PosSample[2];
  

  if(Option[1] == 'y') {

    sample.SG.Cyl.r = DimSample[0];
    sample.SG.Cyl.height = DimSample[1];
    sample.Type = VT_CYL;

    fprintf(LogFilePtr,"\n             sample geometry:	'cylinder'") ;

  }
  if(Option[1] == 'o') {
    sample.SG.Cyl.r = DimSample[0];
    sample.SG.Cyl.height = DimSample[1];
    sample.Type = VT_CYL;

    fprintf(LogFilePtr,"\n             sample geometry:	'hollow cylinder'") ;

  }
  if(Option[1] == 'u') {
   
    sample.SG.Cube.thickness = DimSample[0];
    sample.SG.Cube.width = DimSample[1];
    sample.SG.Cube.height = DimSample[2];
    sample.Type = VT_CUBE;

    fprintf(LogFilePtr,"\n             sample geometry:	'cuboid'") ;

  }
  if(Option[1] == 'a') {

    sample.SG.Ball.r = DimSample[0];
    sample.Type = VT_SPHERE;

    fprintf(LogFilePtr,"\n             sample geometry:	'sphere'") ;

  }

  SetSampleGeometry(&sample);

  /* converts degs in radian etc. */
  AnglSampleHoriz *= M_PI/180. ;
  AnglSampleVert  *= M_PI/180. ;
  AnglOutHoriz    *= M_PI/180. ;
  AnglOutVert     *= M_PI/180. ;

  /* Hollow cylinder option */
  CopyVector(DimSample, DimSampleHol); DimSampleHol[0] = DimSample[1];

  
}/* End ReadParameterFile */



