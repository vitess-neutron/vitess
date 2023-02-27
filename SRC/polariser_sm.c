/********************************************************************************************/
/*  VITESS module 'polariser_sm.c'                                                          */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            Géza Zsigmond                                                             */
/* 1.1  JUL 2002  Géza Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Jul 2020  K. Lieutenant  tidy up, new central visualization parameters              */
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


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define	FLD_SIZE	    1000
#define	STRING_BUFFER   50


/**************************/
/** Prototypes           **/
/**************************/
void  OwnInit(int argc, char *argv[]);              // Reads input parameters and sets global parameters
void  OwnCleanup();                                 // Does module specific cleanup
void  SetGeometry(char* sColor);                    // Fills the structure stGeometry for visualization 
void  ReadParameterFile();                          // Reads parameters from file
void  ReadReflFile(char* sFilename, double* pData); // Reads reflectivity data from file


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
char      *ParameterFileName=NULL,     // -P        [-]   pointer to the name of the parameter file  
          *ReflUpFileName=NULL,        // -U        [-]   pointer to the name of the reflectivity file for spin-up neutrons 
          *ReflDownFileName=NULL;      // -D        [-]   pointer to the name of the reflectivity file for spin-down neutrons
VectorType PosSM;                      // -a -b -c  [cm]  center position of the rectangular polariser
double     AngleSMHoriz=0.0,           // -H       [deg]  horizontal rotation angle (about z axis) of the polarizer
           AngleSMVert=0.0;            // -V       [deg]  vertical (second) rotation angle of the polarizer
VectorType TranslOut;                  // -R -E -G  [cm]  position of the new origin  (in the co-ordinate of the old origin)
double     AnglOutHoriz=0.0,           // -h       [deg]  horizontal angle of the output frame, relative to input orientation
           AnglOutVert=0.0;            // -v       [deg]  vertical angle of the output frame, relative to input orientation    

VectorType DimSM,                      //    file   [cm]  size of the rectangular polariser
           guide_field,                //    file   [Oe]  guide field
           analysis_dir;               //    file    [-]  analysis direction
int        NoCh=0;                     //    file    [-]  number of channels 
double     WallTh=0.0;                 //    file   [cm]  thickness of the blades dividing the channels

// Data read from file
double     rupdata  [FLD_SIZE],        //    file    [-]  reflectivity data for spin-up neutrons
           rdowndata[FLD_SIZE];        //    file    [-]  reflectivity data for spin-down neutrons

// Variables determined from input parameters or trajectory data
double     RotMatrixSM [3][3],         //            [-]  rotation matrix to tranform into the frame of the polarizer
           RotMatrixOut[3][3],         //            [-]  rotation matrix to tranform into the output frame  
           RotMatrixField[3][3],       //            [-]  rotation matrix to tranform into the frame of the guide field
           RotMatrixAnalysis[3][3],    //            [-]  rotation matrix to tranform into the frame of the quantization direction
           WidthCh=0.0,                //           [cm]  channel width and 
           ProbCutoff=0.0;             //            [-]  = wei_min: minimal accepted weight of the neutron trajectory


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  int        datanumber=0;
  long       NumOut=0, i=0, m=0;
  double		 TOF=0.0, TOFprec=0.0, WL=0.0, Prob=0.0, 
             phi=0.0, phinew=0.0, the=0.0, thenew=0.0,
             aUU=1.0, aDD=1.0;
  double     szog=0.0,  shift=0.0, 
             PhaseShift=0.0, 
             NumberPrecessions = 0.0,
             IntegralIntensity = 0.0;
  double     LarmorMatrix[3][3];
  VectorType Pos, Dir, SpinVector, 
             Path, n,                 /* displacement vector*/
             postop, posbot;
  Neutron		 Neutrons;

  // initialization
  // --------------
  _eModule=MCN_POL_SM;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = MISSING;
  if (bVisInstr) 
    bBlowUp = FALSE;

  InitVector(Pos);    InitVector(Dir);
  InitVector(SpinVector);
  InitVector(Path);   InitVector(n);
  InitVector(postop); InitVector(posbot);

  InitNeutron(&Neutrons);
  Init3x3Matrix(LarmorMatrix);

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while((ReadNeutrons())!= 0)
  {
    for(i=0;i<NumNeutGot;i++)
    { 
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /* InputNeutrons[i].Position[0]	= 0.0; */
        TOF  = InputNeutrons[i].Time;
        WL   = InputNeutrons[i].Wavelength;
        Prob = InputNeutrons[i].Probability;

        CopyVector(InputNeutrons[i].Position, Pos);
        CopyVector(InputNeutrons[i].Vector,   Dir);
        CopyVector(InputNeutrons[i].Spin, SpinVector); 

        InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

        /* translates into frame of the SM  */
        SubVector(Pos, PosSM);
        RotVector(RotMatrixSM, Pos);
        RotVector(RotMatrixSM, Dir);

        /* calculate spin vector in the direction of the analysis */
        RotVector(RotMatrixAnalysis, SpinVector);
        CartesianToSpherical(SpinVector, &the, &phi);

        /* select channel and shift vertically to its frame */n[1]=n[2]=0.;n[0]=1.;
        if ((PlaneLineIntersect(Pos, Dir, n, - DimSM[0]/2., posbot) == TRUE)&&(fabs(posbot[1]) < DimSM[1]/2.)&&(fabs(posbot[2]) < DimSM[2]/2.)&&(Dir[0] > 0.))
        {
          double epsilonZ = (posbot[2] + DimSM[2]/2.) / DimSM[2] * (double) NoCh;
          int    No = (int) floor(epsilonZ)+1; 

          shift = (WidthCh + WallTh) *(- (NoCh -1)/2. + (No - 1));
          Pos[2] += - shift; 

          if (fabs(posbot[2]-shift) > WidthCh/2.) 
            goto getlost;
        } 
        else 
        { goto getlost;                      
        }

        TOFprec = 0.0;

        /* reflecting in channels top/bottom */
        for (m = 1; m < 1000; m++)		
        {
          int r=0; 

          CHECK;
          /* reflection on top/bottom */n[0]=n[1]=0.;n[2]=1.;
	
          if ((PlaneLineIntersect(Pos, Dir, n, + WidthCh/2., postop) == TRUE)&&(postop[0] > (Pos[0]+0.1))&&(fabs(postop[0]) < DimSM[0]/2.)&&(Dir[2] > 0.))
          {
            /* polarizing */
	          szog= fabs((double) asin(Dir[2])); 

            datanumber = (int) (szog *180./M_PI * 1000./WL); 
            if (datanumber > 1000) goto getlost; 

            aUU = sqrt(rupdata  [datanumber]);
            aDD = sqrt(rdowndata[datanumber]);

            if ((aUU == 0.0) && (aDD == 0.0)) goto getlost;
			
            thenew = 2. * (double) atan(aDD/ aUU *(double) tan(the/2.));
            phinew = phi /* + Phipol */;

            Prob *= sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.));

            TOFprec += (postop[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

            CopyVector(postop, Pos);
            Dir[2] *= -1; r=1;				/*ps(i+1); ps(m); ps(+77);goto getlost;ps(the);*/
            the = thenew; phi = phinew;

            goto contin;  
          } 

          if ((PlaneLineIntersect(Pos, Dir, n, - WidthCh/2., posbot) == TRUE)&&(posbot[0] > (Pos[0]+0.1))&&(fabs(posbot[0]) < DimSM[0]/2.)&&(Dir[2] < 0.))
          {
            /* polarizing */
            szog= fabs((double) asin(Dir[2]));

            datanumber = (int) (szog *180./M_PI * 1000./WL);
            if (datanumber > 1000) goto getlost;  

            aUU = rupdata  [datanumber];
            aDD = rdowndata[datanumber];

            if ((aUU == 0.)&&(aDD == 0.)) goto getlost;
			
            thenew = 2.0 * (double) atan(aDD/ aUU *(double) tan(the/2.));
            phinew = phi /* + Phipol */;

            Prob *= sq(aUU * cos(the/2.)) + sq(aDD * sin(the/2.));

            TOFprec += (postop[0] - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

            CopyVector(posbot, Pos);
            Dir[2] *= -1; r=1;			
            the = thenew; phi = phinew;

            goto contin;  
          }

        contin:;	
          /* absorption on the sides */
          n[0]=n[2]=0.0; n[1]=1.0;
	
          if ((PlaneLineIntersect(Pos, Dir, n, + DimSM[1]/2, postop) == TRUE) && (Dir[1] > 0.) && (postop[0] > Pos[0])&&(fabs(postop[0]) < DimSM[0]/2.)) {goto getlost;} 
	        if ((PlaneLineIntersect(Pos, Dir, n, - DimSM[1]/2, posbot) == TRUE) && (Dir[1] < 0.) && (posbot[0] > Pos[0])&&(fabs(posbot[0]) < DimSM[0]/2.)) {goto getlost;} 
	
          if (r==0) goto leave; /* cannot be reflected anymore */
        }

        /* leave channel and shift vertically back to main SM frame */
      leave:;
	
        Pos[2] += shift;

        SphericalToCartesian(SpinVector, &the, &phi);

        /*n[0]=1.;n[1]=0.;n[2]=0.;
        if((PlaneLineIntersect(Pos, Dir, n, + DimSM[0]/2., posbot) == TRUE)) CopyVector(posbot, Pos); */

        if (Prob <= ProbCutoff) goto getlost;

        IntegralIntensity += Prob;
        NumOut++;

        /* translates into initial frame   */
        RotBackVector(RotMatrixSM, Pos);
        RotBackVector(RotMatrixSM, Dir);
        AddVector(Pos, PosSM);

        /* computes neutron variables in the output frame */
        SubVector(Pos, TranslOut);
        RotVector(RotMatrixOut, Pos);
        RotVector(RotMatrixOut, Dir);
        RotVector(RotMatrixOut, SpinVector);

        /* translates neutron variables for output - X'=0. */
        TOFprec += -Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

        CopyVector(Dir, Path);
        MultiplyByScalar(Path, - Pos[0]/ Dir[0] );
        AddVector(Pos, Path);  

        /* precession in the guide field */
        RotVector(RotMatrixField, SpinVector); 
        PhaseShift = TOFprec * FREQUENCY_FROM_FIELD(guide_field[0]);  
        NumberPrecessions = PhaseShift/2./M_PI;

        FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);
        RotVector    (LarmorMatrix, SpinVector);
        RotBackVector(RotMatrixField, SpinVector);

        /* transmit coordinates which were not changed, the rest overwrite below */
        Neutrons = InputNeutrons[i]; 
        Neutrons.Time = TOF + TOFprec;
        Neutrons.Probability = Prob;

        CopyVector(Pos, Neutrons.Position);
        CopyVector(Dir, Neutrons.Vector);
        CopyVector(SpinVector, Neutrons.Spin);

        /* writes output binary file */
        WriteNeutron(&Neutrons);

      getlost: ;
      }
    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:

  /* write geometry file */
  SetGeometry("magenta");
  
  /* Do module specific cleanups */
  OwnCleanup(); 

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], AnglOutHoriz, AnglOutVert);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  double roty=0.0, rotz=0.0;
  int j;

  /* initial values */
  InitVector(PosSM);
  InitVector(TranslOut);
  InitVector(DimSM);
  InitVector(guide_field);
  InitVector(analysis_dir);

  Init3x3Matrix(RotMatrixSM);
  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(RotMatrixOut);
  Init3x3Matrix(RotMatrixAnalysis);

  for (j=0; j < FLD_SIZE; j++)
  {
    rupdata[j]   = 0.0;
    rdowndata[j] = 0.0;
  }

  ProbCutoff = wei_min;
	
  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'P':
        ParameterFileName=&argv[1][2];
        break;
      case 'U':
        ReflUpFileName=&argv[1][2];
        break;
      case 'D':
        ReflDownFileName=&argv[1][2];
        break;

      case 'a':
        sscanf(&argv[1][2], "%lf", &PosSM[0]);
        break;
      case 'b':
        sscanf(&argv[1][2], "%lf", &PosSM[1]);
        break;
      case 'c':
        sscanf(&argv[1][2], "%lf", &PosSM[2]);
        break;

      case 'H':
        sscanf(&argv[1][2], "%lf", &AngleSMHoriz);
        break;
      case 'V':
        sscanf(&argv[1][2], "%lf", &AngleSMVert);
        break;

      case 'R':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]);
        break;
      case 'E':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]);
        break;
      case 'G':
        sscanf(&argv[1][2], "%lf", &TranslOut[2]);
        break;

      case 'h':
        sscanf(&argv[1][2], "%lf", &AnglOutHoriz);
        break;
      case 'v':
        sscanf(&argv[1][2], "%lf", &AnglOutVert);
        break;
    }
    argc--;
    argv++;
  }
	 
  /* reads parameter file */
  ReadParameterFile(); 

  /*	 checks some values */

  /* prints parameters into log file for verification */
  if (((NoCh+1)/2. - floor((NoCh+1)/2.)) > 0.0) 
  {	NoCh += -1; 
    fprintf(LogFilePtr,"\nWARNING: Number of channels must be odd! Set %ld. \n", NoCh);
  }
		
  fprintf(LogFilePtr,"	cutoff probability		=     %8.1e\n",  ProbCutoff);
  fprintf(LogFilePtr,"	position x, y, z		=  %9.4f, %9.4f, %9.4f\n	length, height, width  =  %9.4f, %9.4f, %9.4f\n	offset angle horiz		=  %9.4f\n	offset angle vert		=  %9.4f\n",			
                     PosSM[0], PosSM[1], PosSM[2], DimSM[0], DimSM[2], DimSM[1], AngleSMHoriz, AngleSMVert);
  fprintf(LogFilePtr,"	output horizontal angle	= %9.4f\n	output vertical angle	= %9.4f\n	X',Y',Z'			= %9.4f, %9.4f, %9.4f\n", 	
                     AnglOutHoriz, AnglOutVert, TranslOut[0], TranslOut[1], TranslOut[2]);

  /* converts degs in radian etc. */
  AngleSMHoriz *= M_PI/180.;
  AngleSMVert	 *= M_PI/180.;
  AnglOutHoriz *= M_PI/180.;
  AnglOutVert  *= M_PI/180.;

  FillRotMatrixZY(RotMatrixSM,  AngleSMVert, AngleSMHoriz);
  FillRotMatrixZY(RotMatrixOut, AnglOutVert, AnglOutHoriz);
	
  WidthCh = (DimSM[2] - (NoCh+1)*WallTh)/NoCh;

  CartesianToEulerZY(analysis_dir, &roty, &rotz); 
  FillRotMatrixZY(RotMatrixAnalysis, roty, rotz); 	

  CartesianToEulerZY(guide_field, &roty, &rotz); 
  FillRotMatrixZY(RotMatrixField, roty, rotz); 	

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{

}/* End OwnCleanup */


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
  }
}


/********************************************************/
/** ReadParameterFile() reads the parameters from file **/
/********************************************************/
void ReadParameterFile()
{
  FILE* pFile = OpenInputFile2(ParameterFileName, "Reflectivity data", "r");

  /* reads from file by using ReadParF and ReadParComment */
  DimSM[0]       =ReadParF(pFile); DimSM[2]       =ReadParF(pFile); DimSM[1]       =ReadParF(pFile); ReadParComment(pFile);
  NoCh           =ReadParI(pFile); WallTh         =ReadParF(pFile); ReadParComment(pFile);
  guide_field[0] =ReadParF(pFile); guide_field[1] =ReadParF(pFile); guide_field[2] =ReadParF(pFile); ReadParComment(pFile);
  analysis_dir[0]=ReadParF(pFile); analysis_dir[1]=ReadParF(pFile); analysis_dir[2]=ReadParF(pFile); ReadParComment(pFile);

  fprintf(LogFilePtr,"\ndata from parameter file: '%s':\n",ParameterFileName);
  fprintf(LogFilePtr,"	guide_fieldX			=  %9.4f\n	guide_fieldY			=  %9.4f\n	guide_fieldZ			=  %9.4f\n",  guide_field[0], guide_field[1], guide_field[2]);

  fclose(pFile);

  return;
}


/******************************/
/** reads reflectivity file  **/
/******************************/
void ReadReflFile(char* sFilename, double* pData)
{ 
  long count;
  FILE* pFile = OpenInputFile2(sFilename, "Reflectivity data", "r");

  for (count=0; count < FLD_SIZE; count++)
  {
    if (fscanf(pFile,"%lf", &pData[count])==EOF)
    break;
  }

  fclose(pFile);

  return;
}



