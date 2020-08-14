/********************************************************************************************/
/*  VITESS module 'sample_singcryst.c'                                                      */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Apr 2003  Géza Zsigmond  initial version                                            */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2  Nov 2013  D. Nekrassov   Visualisation, flexible input file formats introduced      */
/* 1.3  Apr 2020  K. Lieutenant  tidy up, new central visualization parameters              */
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

#define	STRING_BUFFER 200


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit(int argc, char *argv[]);                       // Reads input parameters and sets global variables
void   OwnCleanup();                                          // Does module specific cleanup
void   ReadParFile(SampleType* pSample);                      // Reads sample parameters from file
void   ReadStructFile();                                      // Reads the structure factor file
void   SetGeometry(char* sColor);                             // Fills the structure stGeometry for visualization 
void   OutputTransform(VectorType Pos, VectorType Dir);       // Co-ordinate transformation to output frame
double dSpreadLorentzian(double rdelta);                      // probability of finding a d-spacing in Lorentzian approximation
double dSpreadGaussian(double rdelta);                        // probability of finding a d-spacing in Gaussian approximation 


/******************************/
/** Global Variables         **/
/******************************/
McCompID   _eModule=MCN_SMPL_SNGL_X;

// Input parameters
char      *SampleFileName=NULL,     // -P     [-]   pointer to the name of the sample file  
          *StructFileName=NULL;     // -S     [-]   pointer to the name of the structure factor file  
long       d_spr_option=1;          // -o     [-]   function describing the d-spread distribution:  LORENTZIAN, GAUSSIAN
double     Ddperd=0.0001;           // -d     [-]   d-spacing spread d_FWHM/d  

double     A_reciproc[3],           // file [1/Ang] reciprocal unit vector A
           B_reciproc[3],           // file [1/Ang] reciprocal unit vector A 
           C_reciproc[3];           // file [1/Ang] reciprocal unit vector A
double     Normalisation=1.0,       // file   [-]   normalisation factor
           AbsorptionC=0.0;         // file  [1/cm] Macroscopic absorption cross section 
double		 AnglPhi,                 // file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the Z-axis (1st rot)
           AnglChi,                 // file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the X-axis (2nd rot)
           AnglOmega;               // file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the Z-axis (3rd rot)
char       Option[STRING_BUFFER];   // file   [-]   geometry parameter: "cylinder"  "hollow-cylinder"  "cuboid"  "ball" 
VectorType PosSample,               // file   [cm]  center position of the sample
           DimSample;               // file   [cm]  size of the sample
double     AnglOutHoriz=0.0,        // file  [rad]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert =0.0;        // file  [rad]  vertical angle of the output frame, relative to input orientation    

// Variables determined from input parameters or trajectory data
SampleType stSample;                //        [-]   sample geometry
long       Repetition=1;            //        [-]   number of reflections found in the structure factor file
double     *Fhkl2,                  //        [-]   array of |F|² values of the Bragg reflections
           *hh, *kk, *ll;           //        [-]   array of (h,k.l) numbers of the Bragg reflections
int        *no;                     //        [-]   array of sequential numbers of the Bragg reflections
double		 RotMatrixPhi  [3][3],    //        [-]   rotation angle of the first rotation  (about the Z-axis) 
           RotMatrixChi  [3][3],    //        [-]   rotation angle of the second rotation (about the X-axis)
           RotMatrixOmega[3][3],    //        [-]   rotation angle of the third rotation  (about the Z-axis)
           RotMatrixOut  [3][3];    //        [-]   rotation matrix to transfer to the output coordinate system


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long       repet, i;
  double     TOF, WL, Prob ;
  double     k_inc[3], GGact[3]; 
  double     MaxPathLength, PathLength;
  VectorType Pos, Dir, 
             GG, Pos_final;
  VectorType Pos1f, Pos2f,
             Pos1v, Pos2v;
  Neutron    Neutrons ;

  // Initialisation
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* reads file containing sample parameters */
  ReadParFile(&stSample);

  /* Reads the structure factor file */
  ReadStructFile();

  DECLARE_ABORT;

  // Loop over all trajectories
  // --------------------------
  while((ReadNeutrons())!= 0)
  {
    for (i=0;i<NumNeutGot ;i++)
    { 
      InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

      if(InputNeutrons[i].Probability <= wei_min) goto getlost ;

      /* translates and rotates into frame of the sample, where phi-khi-omega are consecutive rotations of the sample(!) in the initial frame */
      SubVector(InputNeutrons[i].Position, PosSample);

      RotBackVector(RotMatrixOmega, InputNeutrons[i].Position);
      RotBackVector(RotMatrixChi, InputNeutrons[i].Position);
      RotBackVector(RotMatrixPhi, InputNeutrons[i].Position);
	
      RotBackVector(RotMatrixOmega, InputNeutrons[i].Vector);
      RotBackVector(RotMatrixChi, InputNeutrons[i].Vector);
      RotBackVector(RotMatrixPhi, InputNeutrons[i].Vector);

      /* gives intersection positions with sample */
      if(Option[1] == 'y')
      {
        if(IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
      }
      if(Option[1] == 'u')
      {
        if(IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
      }
      if(Option[1] == 'a')
      {
        if(IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
      }
	
      for(repet=0;repet<Repetition;repet++)
      {
        CHECK;
        CopyVector(Pos1f, Pos1v) ;
        CopyVector(Pos2f, Pos2v) ;
									
        GG[0] = hh[repet] * A_reciproc[0] + kk[repet] * B_reciproc[0] + ll[repet] * C_reciproc[0];
        GG[1] = hh[repet] * A_reciproc[1] + kk[repet] * B_reciproc[1] + ll[repet] * C_reciproc[1];
        GG[2] = hh[repet] * A_reciproc[2] + kk[repet] * B_reciproc[2] + ll[repet] * C_reciproc[2];
			
        TOF  = InputNeutrons[i].Time ;
        WL   = InputNeutrons[i].Wavelength ;
        Prob = InputNeutrons[i].Probability ;

        CopyVector(InputNeutrons[i].Position, Pos);
        CopyVector(InputNeutrons[i].Vector, Dir);

        /* scattering position and TOF untill scattering */	
        SubVector(Pos2v, Pos1v);					/*maximal path vector*/ 
        MaxPathLength = LengthVector(Pos2v); 
        MultiplyByScalar(Pos2v, MonteCarlo(0.,1.));	 /*random path vector untill scattering */
        PathLength = LengthVector(Pos2v);
        AddVector(Pos1v, Pos2v);	
					
        TOF += (Pos1v[0] - Pos[0])/ fabs(Dir[0]) / V_FROM_LAMBDA(WL);

        CopyVector(Pos1v, Pos);						/*scattering position */

        if ((WL*LengthVector(GG)/4./M_PI)>1) Prob = 0.; 

        /* attenuation untill scattering normalized to maximal path and probability */
        Prob *= exp( - PathLength * AbsorptionC * WL );
				
        // Take into account the number of hkl-entries in the look-up file
        // for correct normalisation.
        Prob *= MaxPathLength * Normalisation * (1./((double) Repetition))* Fhkl2[repet] * 4. * M_PI * sq(WL/LengthVector(GG)) ; 

        /* scattering: new neutron variables*/ 
        CopyVector(Dir, k_inc);
        MultiplyByScalar(k_inc, 2 * M_PI / WL);
        CopyVector(GG, GGact); 
        MultiplyByScalar(GGact, (-2.* ScalarProduct(GG, k_inc)/ScalarProduct(GG,GG)));

        if(d_spr_option == 1) Prob *= dSpreadLorentzian((1. - LengthVector(GGact)/LengthVector(GG)));
        if(d_spr_option == 2) Prob *=   dSpreadGaussian((1. - LengthVector(GGact)/LengthVector(GG)));

        /* defines now outgoing k direction */
        AddVector(k_inc, GGact); 
        MultiplyByScalar(k_inc, 1./LengthVector(k_inc));
        CopyVector(k_inc, Dir);

        /* attenuation succeeding scattering */
        if(Option[1] == 'y')
        {
          if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.; 
        }
        if(Option[1] == 'u')
        {
          if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.;  
        }
        if(Option[1] == 'a')
        {
          if(IntersectionWithSphere(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.; 
        }

        /* path in the sample after scattering */
        CopyVector(Pos2v, Pos_final);
        SubVector(Pos_final, Pos);
        PathLength = LengthVector(Pos_final);  

        Prob *= (double) exp( - PathLength * AbsorptionC * WL );

        /* Output matters */
        TOF +=  PathLength / V_FROM_LAMBDA(WL);
        OutputTransform(Pos2v, Dir);

        /* transmit coordinates which were not changed, the rest overwrite below */
        Neutrons = InputNeutrons[i]; 

        Neutrons.Time = TOF ;
        Neutrons.Probability = Prob ;

        CopyVector(Pos2v, Neutrons.Position);
        CopyVector(Dir, Neutrons.Vector);

        //		Neutrons.Color = (short) no[repet]; 

        /*	 writes output binary file */
        if(Prob > wei_min) WriteNeutron(&Neutrons);

      }/* repet */

    /* here continues if neutron gets lost */
    getlost:;

    } /* i */
  }   /* ReadNeutrons*/
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
 my_exit:
  /* write geometry file */
  SetGeometry("white");
  
  /* Do module specific cleanups */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup(PosSample[0], PosSample[1], PosSample[2], AnglOutHoriz, AnglOutVert);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  Ddperd=0.01;
  d_spr_option = 1 ;

  colh = -1; colk = -1; coll = -1; colD = -1;
  colF = -1; colF2 = -1; colM = -1; colDW = -1;
  scaleF2 = 1.;
	
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        SampleFileName=&argv[1][2];
        break;
      case 'S':
        StructFileName=&argv[1][2];
        break;
		
      case 'd':
        sscanf(&argv[1][2], "%lf", &Ddperd); /* d-spacing spread, this gives the relative 'thickness' of the Ewald sphere */
        break;

      case 'o':
        sscanf(&argv[1][2], "%ld", &d_spr_option);
        break;
    }
    argc--;
    argv++;
  }
}

 
/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  if (no!=NULL) 
  {
    free(no); 
    free(hh);
    free(kk);
    free(ll);
    free(Fhkl2);
  }

  return;
}


/*******************************************************/
/** Reads the sample parameters from file             **/
/*******************************************************/
void ReadParFile(SampleType* pSample)
{
  // opens file containing sample parameters (program exit in case of error)
  FILE* pSmplFile = OpenInputFile2(SampleFileName, "sample data", "r");

  fprintf(LogFilePtr, "sample data read from parameter file: '%s':\n", SampleFileName) ;

  /* reads from file by using ReadParF(pSmplFile) and ReadParComment(pSmplFile) */
  A_reciproc[0]=ReadParF(pSmplFile); A_reciproc[1]=ReadParF(pSmplFile); A_reciproc[2]=ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  B_reciproc[0]=ReadParF(pSmplFile); B_reciproc[1]=ReadParF(pSmplFile); B_reciproc[2]=ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  C_reciproc[0]=ReadParF(pSmplFile); C_reciproc[1]=ReadParF(pSmplFile); C_reciproc[2]=ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  Normalisation=ReadParF(pSmplFile); AbsorptionC  =ReadParF(pSmplFile);                                     ReadParComment(pSmplFile);
  PosSample[0] =ReadParF(pSmplFile); PosSample[1] =ReadParF(pSmplFile); PosSample[2] =ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  AnglPhi      =ReadParF(pSmplFile); AnglChi      =ReadParF(pSmplFile); AnglOmega    =ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  ReadParString(pSmplFile, Option) ;                                                                        ReadParComment(pSmplFile) ;
  DimSample[0] =ReadParF(pSmplFile); DimSample[2] =ReadParF(pSmplFile); DimSample[1] =ReadParF(pSmplFile);  ReadParComment(pSmplFile);
  AnglOutHoriz =ReadParF(pSmplFile); AnglOutVert  =ReadParF(pSmplFile);                                     ReadParComment(pSmplFile);

  colh = ReadParF(pSmplFile); colk  = ReadParF(pSmplFile); coll = ReadParF(pSmplFile);
  colF = ReadParF(pSmplFile); colF2 = ReadParF(pSmplFile); colDW= ReadParF(pSmplFile);
  scaleF2 = ReadParF(pSmplFile);
		
  /*	 checks some values */
  if ((Option[1] != 'y') && (Option[1] != 'u') && (Option[1] != 'a'))
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
  AnglOmega   *= M_PI/180.;
  AnglChi     *= M_PI/180.;
  AnglPhi     *= M_PI/180.;
  AnglOutHoriz*= M_PI/180.;
  AnglOutVert *= M_PI/180.;

  /* calculates rotation matrices */
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz);
  FillRotMatrixZY(RotMatrixPhi,   0.0, AnglPhi); 
  FillRotMatrixXZ(RotMatrixChi,   0.0, AnglChi); 
  FillRotMatrixZY(RotMatrixOmega, 0.0, AnglOmega); 

  fclose(pSmplFile);

}/* End ReadParameterFile() */

 
/*******************************************************/
/** Reads the structure factor file                   **/
/*******************************************************/
void ReadStructFile()
{
  long nLines=1,
       count =0;

  // .dat file treated here, the rest in ReadStructureFile() in sample.c
  // this case should also be treated there
  if (strstr(StructFileName, ".dat") == &StructFileName[strlen(StructFileName)-4])
  {
    // opens file containing structure factore parameters (program exit in case of error)
    FILE* pStructFile = OpenInputFile2(StructFileName, "structure factors", "r");

    nLines = LinesInFile(pStructFile);
    rewind(pStructFile);
     
    no  = (int*) calloc(nLines, sizeof(int));
    hh = (double*) calloc(nLines, sizeof(double));
    kk = (double*) calloc(nLines, sizeof(double));
    ll = (double*) calloc(nLines, sizeof(double));
    Fhkl2 = (double*) calloc(nLines, sizeof(double));
      
    for (count=0; count<nLines; count++)
    {
      no[count] = ReadParF(pStructFile);
      hh[count] = ReadParF(pStructFile);
      kk[count] = ReadParF(pStructFile);
      ll[count] = ReadParF(pStructFile);
			 
      Fhkl2[count] = ReadParF(pStructFile); 
      if(Fhkl2[count] == 0.) break ;

      ReadParComment(pStructFile);

      Repetition = count +1 ; 
    }

    fclose(pStructFile);

    fprintf(LogFilePtr, "Structure factors read from file: '%s'\n", StructFileName) ;
    fprintf(LogFilePtr, "Number of reflections           : %ld\n",  Repetition);
  }
  else 
  {
    int jj;

    /* ReadStructureFile() in sample.c */
    Repetition = ReadStructureFile(StructFileName, 2, 0);

    /* array of sequential numbers */
    no  = (int*) calloc(Repetition, sizeof(int));
    for (jj = 0; jj < Repetition; jj++) no[jj] = jj+1;

    /* pointer to other arrays copied from sample.c */
    hh = hVal;
    kk = kVal;
    ll = lVal;
    Fhkl2 = F2Val;
  }

}/* End  ReadStructFile() */


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
	RotVector(RotMatrixPhi, Dir);
	RotVector(RotMatrixChi, Dir);
	RotVector(RotMatrixOmega, Dir);
	
	RotVector(RotMatrixPhi, Pos);
	RotVector(RotMatrixChi, Pos);
	RotVector(RotMatrixOmega, Pos);
	
	AddVector(Pos, PosSample);

	/* computes neutron variables in the output frame */
	SubVector(Pos, PosSample);
	RotVector(RotMatrixOut, Pos);
	RotVector(RotMatrixOut, Dir);

	/* translates neutron variables for output - X'=0. 
	{
	VectorType Path ;
	*tof = *tof - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA_PT(wl);
	CopyVector(Dir, Path);
	MultiplyByScalar(Path, - Pos[0]/ Dir[0] );
	AddVector(Pos, Path);  
	}			Path = displacement vector 
	*/

} /* End OutputTransformations()*/


/*******************************************************************/
/* probability of finding a d-spacing in Lorentzian approximation. */
/* Maximum probability amplitude = 1                               */
/*******************************************************************/
double	dSpreadLorentzian(double rdelta)
{
  return	sq(Ddperd) / ( 4*sq(rdelta) + sq(Ddperd) );

}/* End dSpreadLorentzian */


/*******************************************************************/
/* probability of finding a d-spacing in Gaussian approximation.   */
/* Maximum probability amplitude = 1                               */
/*******************************************************************/
double	dSpreadGaussian(double rdelta)
{
  double argd ;

  argd = - sq(rdelta / Ddperd) * 4. *log(2);
  if (argd < - 100.) 
    argd = -100.;

  return (double) exp(argd);

}/* End dSpreadGaussian */
