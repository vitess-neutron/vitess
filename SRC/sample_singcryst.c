/********************************************************************************************/
/*  VITESS module 'sample_singcryst.c'                                                      */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Apr 2003  Géza Zsigmond  initial version                                            */
/* 1.1  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.2  Nov 2013  D. Nekrassov   Visualisation, flexible input file formats introduced      */
/* 1.3  Apr 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.4  Oct 2021  K. Lieutenant  option: parameters from input instead of from file         */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "general.h"
#include "init.h"
#include "convert.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "sample.h"

#define	STRING_BUFFER 200


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit   (int argc, char *argv[]);                    // Reads input parameters and sets global variables
void   OwnCleanup();                                          // Does module specific cleanup
void   SetSamplePar   (SampleType *pSample);                  // Reads sample parameters and combines with input parameters
void   ReadStructFile ();                                     // Reads the structure factor file
void   SetGeometry    (char* sColor);                         // Fills the structure stGeometry for visualization 
void   OutputTransform(VectorType Pos, VectorType Dir);       // Co-ordinate transformation to output frame
double dSpreadLorentzian(double rdelta);                      // probability of finding a d-spacing in Lorentzian approximation
double dSpreadGaussian(double rdelta);                        // probability of finding a d-spacing in Gaussian approximation 


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
char      *pSmplFileName=NULL,      // -P     [-]   pointer to the name of the sample file  
          *pStrFileName =NULL;      // -S     [-]   pointer to the name of the structure factor file  
VtDistr    d_spr_option=LORENTZIAN; // -o     [-]   function describing the d-spread distribution:  LORENTZIAN, GAUSSIAN
double     Ddperd=0.0001;           // -d     [-]   d-spacing spread d_FWHM/d  

double     A_recip[3]={0.0,0.0,0.0},// -A -B -C file [1/Ang] reciprocal unit vector A
           B_recip[3]={0.0,0.0,0.0},// -T -U -V file [1/Ang] reciprocal unit vector A 
           C_recip[3]={0.0,0.0,0.0};// -X -Y -Z file [1/Ang] reciprocal unit vector A
double     NormFact=1.0,            // -N       file   [-]   normalisation factor
           AbsorptionC=0.0;         // -m       file  [1/cm] Macroscopic absorption cross section 
double		 AnglPhi=0.0,             // -p       file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the Z-axis (1st rot)
           AnglChi=0.0,             // -q       file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the X-axis (2nd rot)
           AnglOmega=0.0;           // -O       file  [deg]  rotation angle of sample i.e. the reciprocal unit vectors about the Z-axis (3rd rot)
VtSmplGeom eGeom=VT_NO_GEOM;        // -G       file   [-]   sample shape: VT_NO_GEOM, VT_CUBE, VT_CYL, VT_SPHERE, VT_HOL_CYL
VectorType PosSample={0.0,0.0,0.0}; // -x -y -z file   [cm]  center position of the sample
double     Diameter = 0.0,          // -t       file  [cm]   thickness or diameter of the sample 
           Height   = 0.0,          // -h       file  [cm]   height of the sample 
           Width    = 0.0,          // -w       file  [cm]   width of the sample
           AnglOutHoriz=0.0,        // -u       file  [rad]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert =0.0;        // -v       file  [rad]  vertical angle of the output frame, relative to input orientation    
extern                               
int        colh, colk, coll,        // -H -K -L file   [-]   columns where the miller indices (h,k,l) are listed
           colF,                    // -F       file   [-]   column where structure factor F is
           colF2,                   // -Q       file   [-]   column where |F^2| is
           colDW;                   // -W       file   [-]   column where Debye-Waller factor is
extern                                                       
double     scaleF2;                 // -f       file   [-]   normalization factor for structure factor

// Variables determined from input parameters or trajectory data
SampleType stSample;                     //            [-]   sample geometry
VectorType DimSample={0.0,0.0,0.0};      //            [cm]  size of the sample
long       nReflect=1;                   //            [-]   number of reflections found in the structure factor file
double     *Fhkl2=NULL,                  //            [-]   array of |F|² values of the Bragg reflections
           *hh=NULL,*kk=NULL,*ll=NULL;   //            [-]   array of (h,k.l) numbers of the Bragg reflections
int        *no=NULL;                     //            [-]   array of sequential numbers of the Bragg reflections
double		 RotMatrixPhi  [3][3],         //            [-]   rotation angle of the first rotation  (about the Z-axis) 
           RotMatrixChi  [3][3],         //            [-]   rotation angle of the second rotation (about the X-axis)
           RotMatrixOmega[3][3],         //            [-]   rotation angle of the third rotation  (about the Z-axis)
           RotMatrixOut  [3][3];         //            [-]   rotation matrix to transfer to the output coordinate system


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long       repet=0, i=0;
  double     TOF=0.0, WL, Prob=0.0 ;
  double     k_inc[3]={0.0,0.0,0.0}, 
             GGact[3]={0.0,0.0,0.0}; 
  double     MaxPathLength=0.0, PathLength=0.0;
  VectorType Pos={0.0,0.0,0.0}, Dir={0.0,0.0,0.0}, 
             GG ={0.0,0.0,0.0}, Pos_final={0.0,0.0,0.0};
  VectorType Pos1f={0.0,0.0,0.0}, Pos2f={0.0,0.0,0.0},
             Pos1v={0.0,0.0,0.0}, Pos2v={0.0,0.0,0.0};
  Neutron    Neutrons;

  // Initialisation
  // --------------
  InitNeutron(&Neutrons);

  _eModule = MCN_SMPL_SNGL_X;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.4");
  OwnInit(argc, argv);

  /* Reads sample parameters and combines with input parameters */
  InitSample  (&stSample);
  SetSamplePar(&stSample);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  /* Reads the structure factor file */
  ReadStructFile();

  DECLARE_ABORT;

  // Loop over all trajectories
  // --------------------------
  while (ReadNeutrons() != 0)
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
        InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

        if(InputNeutrons[i].Probability <= wei_min) goto getlost ;

        /* translates and rotates into frame of the sample, where phi-khi-omega are consecutive rotations of the sample(!) in the initial frame */
        SubVector(InputNeutrons[i].Position, PosSample);

        RotBackVector(RotMatrixOmega, InputNeutrons[i].Position);
        RotBackVector(RotMatrixChi,   InputNeutrons[i].Position);
        RotBackVector(RotMatrixPhi,   InputNeutrons[i].Position);
	
        RotBackVector(RotMatrixOmega, InputNeutrons[i].Vector);
        RotBackVector(RotMatrixChi,   InputNeutrons[i].Vector);
        RotBackVector(RotMatrixPhi,   InputNeutrons[i].Vector);

        /* gives intersection positions with sample */
        if (eGeom==VT_CYL)
        {
          if(IntersectionWithCylinder(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
        }
        if (eGeom==VT_CUBE)
        {
          if(IntersectionWithRectangular(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
        }
        if (eGeom==VT_SPHERE)
        {
          if(IntersectionWithSphere(DimSample, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1f, Pos2f) == 0) goto getlost ; 
        }
	
        for(repet=0;repet<nReflect;repet++)
        {
          CHECK;
          CopyVector(Pos1f, Pos1v) ;
          CopyVector(Pos2f, Pos2v) ;
									
          GG[0] = hh[repet] * A_recip[0] + kk[repet] * B_recip[0] + ll[repet] * C_recip[0];
          GG[1] = hh[repet] * A_recip[1] + kk[repet] * B_recip[1] + ll[repet] * C_recip[1];
          GG[2] = hh[repet] * A_recip[2] + kk[repet] * B_recip[2] + ll[repet] * C_recip[2];
			
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
          Prob *= MaxPathLength * NormFact * (1./((double) nReflect))* Fhkl2[repet] * 4. * M_PI * sq(WL/LengthVector(GG)) ; 

          /* scattering: new neutron variables*/ 
          CopyVector(Dir, k_inc);
          MultiplyByScalar(k_inc, 2 * M_PI / WL);
          CopyVector(GG, GGact); 
          MultiplyByScalar(GGact, (-2.* ScalarProduct(GG, k_inc)/ScalarProduct(GG,GG)));

          if(d_spr_option == LORENTZIAN) Prob *= dSpreadLorentzian((1. - LengthVector(GGact)/LengthVector(GG)));
          if(d_spr_option == GAUSSIAN)   Prob *=   dSpreadGaussian((1. - LengthVector(GGact)/LengthVector(GG)));

          /* defines now outgoing k direction */
          AddVector(k_inc, GGact); 
          MultiplyByScalar(k_inc, 1./LengthVector(k_inc));
          CopyVector(k_inc, Dir);

          /* attenuation succeeding scattering */
          if (eGeom==VT_CYL)
          {
            if(IntersectionWithCylinder(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.; 
          }
          if (eGeom==VT_CUBE)
          {
            if(IntersectionWithRectangular(DimSample, Pos, Dir, Pos1v, Pos2v) == 0) Prob = 0.;  
          }
          if (eGeom==VT_SPHERE)
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

          Neutrons.Color = (short) no[repet]; 

          /*	 writes output binary file */
          if(Prob > wei_min) WriteNeutron(&Neutrons);

        }/* repet */

      /* here continues if neutron gets lost */
      getlost:;
      }
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
  int i;
  InitRotMatrix(RotMatrixPhi);
  InitRotMatrix(RotMatrixChi);
  InitRotMatrix(RotMatrixOmega);
  InitRotMatrix(RotMatrixOut);

  Ddperd=0.01;
  d_spr_option = LORENTZIAN;

  colh = -1; colk = -1; coll = -1; colD = -1;
  colF = -1; colF2= -1; colM = -1; colDW= -1;
  scaleF2 = 1.0;
	
  /* Scan all command line parameters */
  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    { switch (argv[i][1])
      {
        /* main window up to version 3 */
        case 'P':
          pSmplFileName=&argv[i][2];
          break;
        case 'S':
          pStrFileName=&argv[i][2];
          break;
		
        case 'd':
          sscanf(&argv[i][2], "%lf", &Ddperd); /* d-spacing spread, this gives the relative 'thickness' of the Ewald sphere */
          break;
        case 'o':
          sscanf(&argv[i][2], "%ld", &d_spr_option);
          break;

          /* crystal parameters */
        case 'A':
          A_recip[0] = atof(&argv[i][2]);
          break;
        case 'B':
          A_recip[1] = atof(&argv[i][2]);
          break;
        case 'C':
          A_recip[2] = atof(&argv[i][2]);
          break;

        case 'T':
          B_recip[0] = atof(&argv[i][2]);
          break;
        case 'U':
          B_recip[1] = atof(&argv[i][2]);
          break;
        case 'V':
          B_recip[2] = atof(&argv[i][2]);
          break;

        case 'X':
          C_recip[0] = atof(&argv[i][2]);
          break;
        case 'Y':
          C_recip[1] = atof(&argv[i][2]);
          break;
        case 'Z':
          C_recip[2] = atof(&argv[i][2]);
          break;

        case 'N':
          NormFact = atof(&argv[i][2]);
          break;
        case 'm':
          AbsorptionC = atof(&argv[i][2]);
          break;

        /* sample position, size and orientation */
        case 'G':
          eGeom = (VtSmplGeom) atoi(&argv[i][2]);
          break;

        case 'p':
          AnglPhi = atof(&argv[i][2]);
          break;
        case 'q':
          AnglChi = atof(&argv[i][2]);
          break;
        case 'O':
          AnglOmega = atof(&argv[i][2]);
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

        case 'u':
          AnglOutHoriz = atof(&argv[i][2]);
          break;
        case 'v':
          AnglOutVert  = atof(&argv[i][2]);
          break;

        case 'H':
          colh = atof(&argv[i][2]);
          break;
        case 'K':
          colk = atof(&argv[i][2]);
          break;
        case 'L':
          coll = atof(&argv[i][2]);
          break;
        case 'F':
          colF = atof(&argv[i][2]);
          break;
        case 'Q':
          colF2 = atof(&argv[i][2]);
          break;
        case 'W':
          colDW = atof(&argv[i][2]);
          break;
        case 'f':
          scaleF2 = atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
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
void SetSamplePar(SampleType* pSample)
{
  FILE*  pFile=NULL;
  char   sLine[CHAR_BUF_SMALL]="", sGeom[20]="";
  int    col_h=-1, col_k=-1,  col_l=-1, 
         col_f=-1, col_f2=-1, col_dw=-1, 
         nLen=sizeof(sLine)-1;
  double Ax  =0.0,  Ay  =0.0, Az  =0.0, 
         Bx  =0.0,  By  =0.0, Bz  =0.0, 
         Cx  =0.0,  Cy  =0.0, Cz  =0.0, 
         x   =0.0,  y   =0.0, z   =0.0, 
         phi =0.0,  chi =0.0, omega=0.0,
         outh=0.0,  outv=0.0,
         diamtr=0.0,height=0.0, width=0.0,
         norm=0.0,  muAbs =0.0, scale_f2=1.0;
  VtSmplGeom geom=VT_NO_GEOM;
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
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &Ax, &Ay, &Az);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &Bx, &By, &Bz);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &Cx, &Cy, &Cz);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf"    , &norm, &muAbs); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x, &y, &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &phi, &chi, &omega);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%s",          sGeom); 
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &diamtr, &width, &height);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &outh,  &outv);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%d %d %d %d %d %d %lf", &col_h, &col_k, &col_l, &col_f, &col_f2, &col_dw, &scale_f2);

      geom = SmplGeom_Txt2ID(sGeom);

      fprintf (LogFilePtr,"sample data read from parameter file: '%s':\n", pSmplFileName) ;
      fclose(pFile);

      // combines information from input and file, input parameters have priority
      if (eGeom==VT_NO_GEOM && geom!=VT_NO_GEOM) eGeom = geom; 
      if (A_recip[0]  ==0.0 && Ax    !=0.0) A_recip[0]  = Ax;
      if (A_recip[1]  ==0.0 && Ay    !=0.0) A_recip[1]  = Ay;
      if (A_recip[2]  ==0.0 && Az    !=0.0) A_recip[2]  = Az;
      if (B_recip[0]  ==0.0 && Bx    !=0.0) B_recip[0]  = Bx;
      if (B_recip[1]  ==0.0 && By    !=0.0) B_recip[1]  = By;
      if (B_recip[2]  ==0.0 && Bz    !=0.0) B_recip[2]  = Bz;
      if (C_recip[0]  ==0.0 && Cx    !=0.0) C_recip[0]  = Cx;
      if (C_recip[1]  ==0.0 && Cy    !=0.0) C_recip[1]  = Cy;
      if (C_recip[2]  ==0.0 && Cz    !=0.0) C_recip[2]  = Cz;
      if (NormFact    ==1.0 && norm  !=0.0
                            && norm  !=1.0) NormFact    = norm;
      if (AbsorptionC ==0.0 && muAbs !=0.0) AbsorptionC = muAbs;
      if (PosSample[0]==0.0 && x     !=0.0) PosSample[0]= x;
      if (PosSample[1]==0.0 && y     !=0.0) PosSample[1]= y;
      if (PosSample[2]==0.0 && z     !=0.0) PosSample[2]= z;
      if (AnglPhi     ==0.0 && phi   !=0.0) AnglPhi     = phi;
      if (AnglChi     ==0.0 && chi   !=0.0) AnglChi     = chi;
      if (AnglOmega   ==0.0 && omega !=0.0) AnglOmega   = omega;
      if (Diameter    ==0.0 && diamtr!=0.0) Diameter    = diamtr;
      if (Height      ==0.0 && height!=0.0) Height      = height;
      if (Width       ==0.0 && width !=0.0) Width       = width;
      if (AnglOutHoriz==0.0 && outh  !=0.0) AnglOutHoriz= outh;
      if (AnglOutVert ==0.0 && outv  !=0.0) AnglOutVert = outv;
      if (colh        ==-1  && col_h !=-1 ) colh        = col_h ;
      if (colk        ==-1  && col_k !=-1 ) colk        = col_k ;
      if (coll        ==-1  && col_l !=-1 ) coll        = col_l ;
      if (colF        ==-1  && col_f !=-1 ) colF        = col_f ;
      if (colF2       ==-1  && col_f2!=-1 ) colF2       = col_f2;
      if (colDW       ==-1  && col_dw!=-1 ) colDW       = col_dw;
      if (scaleF2     ==1.0 && scale_f2!=0.0
                            && scale_f2!=1.0) scaleF2   = scale_f2;
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
  DimSample[0] = Diameter;
  DimSample[1] = Width;
  DimSample[2] = Height;

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

}/* End ReadParameterFile() */

 
/*******************************************************/
/** Reads the structure factor file                   **/
/*******************************************************/
void ReadStructFile()
{
  char sLine[CHAR_BUF_SMALL];
  long nLines=1,
       count =0;

  // .dat file treated here, the rest in ReadStructureFile() in sample.c
  // this case should also be treated there
  if (strstr(pStrFileName, ".dat") == &pStrFileName[strlen(pStrFileName)-4])
  {
    // opens file containing structure factore parameters (program exit in case of error)
    FILE* pStructFile = OpenInputFile2(pStrFileName, "structure factors", "r");

    nLines = LinesInFile(pStructFile);
    rewind(pStructFile);
     
    no  = (int*) calloc(nLines, sizeof(int));
    hh = (double*) calloc(nLines, sizeof(double));
    kk = (double*) calloc(nLines, sizeof(double));
    ll = (double*) calloc(nLines, sizeof(double));
    Fhkl2 = (double*) calloc(nLines, sizeof(double));
      
    nReflect=0;
    for (count=0; count < nLines; count++)
    {
    /*no[count] = ReadParF(pStructFile);
      hh[count] = ReadParF(pStructFile);
      kk[count] = ReadParF(pStructFile);
      ll[count] = ReadParF(pStructFile);
      Fhkl2[count] = ReadParF(pStructFile); 
      ReadParComment(pStructFile); */

      ReadLine(pStructFile, sLine, sizeof(sLine)-1);
      sscanf(sLine, "%d %lf %lf %lf %lf", &no[nReflect], &hh[nReflect], &kk[nReflect], &ll[nReflect], &Fhkl2[nReflect]);

      if (Fhkl2[nReflect] != 0.0) 
        nReflect++; 
    }

    fclose(pStructFile);

    fprintf(LogFilePtr, "Structure factors read from file: '%s'\n", pStrFileName) ;
    fprintf(LogFilePtr, "Number of reflections           : %ld\n",  nReflect);
  }
  else 
  {
    int jj;

    /* ReadStructureFile() in sample.c */
    nReflect = ReadStructureFile(pStrFileName, 2, 0);

    /* array of sequential numbers */
    no  = (int*) calloc(nReflect, sizeof(int));
    for (jj = 0; jj < nReflect; jj++) no[jj] = jj+1;

    /* pointer to other arrays copied from sample.c */
    hh = hVal;
    kk = kVal;
    ll = lVal;
    Fhkl2 = F2Val;
  }

  return;
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
