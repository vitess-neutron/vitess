/********************************************************************************************/
/*  VITESS module 'polariser_he3.c'                                                         */
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
#define	FLD_SIZE	    5000
#define FREQUENCY_FROM_FIELD(x)  ( 18.324282 * x ) /* rad*kHz from Oe=Gauss */ 


/**************************/
/** Functions prototypes **/
/**************************/
void  OwnInit(int argc, char *argv[]);              // Reads input parameters and sets global parameters
void  OwnCleanup();                                 // Does module specific cleanup
void  SetGeometry(char* sColor);                    // Fills the structure stGeometry for visualization 
void  WritePolAndTrans();                           // Writes polarization and transmission data from file  
void  ReadPolAndTrans();                            // Reads polarization and transmission data from file  


/******************************/
/** Global Variables         **/
/******************************/
McCompID   _eModule=MCN_POL_HE3;

// Input parameters
int        Option=0;                   // -a        [-]    flag: analytical calculation of polarization and tranmission data
double     polHe3=0.0,                 // -b        [%]    polarisation of the He3 (for analytical calculation)
           polXsection=0.0,            // -c       [barn]  polarisation cross section of neutrons in He3
           density=0.0;                // -d      [1/cm3]  density of the He3 gas 
char      *PolarizationFileName=NULL,  // -P        [-]    pointer to the name of the data file of the wavelength dependent polarisation
          *TransmissionFileName=NULL;  // -T        [-]    pointer to the name of the data file of the wavelength dependent transmission
VectorType PosMain,                    // -k -l -m  [cm]   center position of the cylindrical polariser
           DimMain,                    // -Y    -X  [cm]   radius and length of the cylindrical polariser
           field_guide,                // -G -H -K  [Oe]   strength of the magnetic guide field
           field_pol,                  // -M -N -O  [Oe]   field in the polarization chamber (which is added to the guide) 
           TranslOut;                  // -p -r -s  [cm]   position of the new origin  (in the co-ordinate of the old origin)

// Input parameters that are currently not used
double     AngleMainHoriz=0.0,         //          [deg]   horizontal rotation angle (about z axis) of the polarizer
           AngleMainVert=0.0;          //          [deg]   vertical (second) rotation angle of the polarizer
double     AnglOutHoriz=0.0,           //          [deg]   horizontal angle of the output frame, relative to input orientation
           AnglOutVert=0.0;            //          [deg]   vertical angle of the output frame, relative to input orientation    

// Variables determined from input parameters or trajectory data
VectorType guide_field_pol;            //           [Oe]   sum of polarization and guide field
double     RotMatrixMain[3][3],        //           [-]    rotation matrix to tranform into the frame of the polarizer
           RotMatrixOut [3][3],        //           [-]    rotation matrix to tranform into the output frame  
           RotMatrixG_Field [3][3],    //           [-]    rotation matrix to tranform into frame of the guide field
           RotMatrixGM_Field[3][3],    //           [-]    rotation matrix to tranform into frame of guide + polarization field
           ProbCutoff=0.0;             //           [-]    = wei_min: minimal accepted weight of the neutron trajectory

// Data read from file
double     polardata[FLD_SIZE], 
           transdata[FLD_SIZE];


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long   NumOut=0, i=0 , datanumber=0;
  double TOF=0.0, TOF1=0.0, TOF2=0.0, TOF3=0.0, 
         WL=0.0, Prob=0.0, phi=0.0, the=0.0, PhaseShift=0.0, 
         NumberPrecessions1=0.0, NumberPrecessions2=0.0, NumberPrecessions3=0.0;
  double pDown=0.0, 
         IntegralIntensity=0.0; 
  double LarmorMatrix[3][3];
  VectorType Pos, Dir, SpinVector, Pos1, Pos2;
  VectorType Path;     /* Path = displacement vector */
  VectorType pos, dir;	
  VectorType V;	
  Neutron    Neutrons;

  // initialization
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  InitVector(pos);    InitVector(dir);
  InitVector(Pos);    InitVector(Dir);
  InitVector(Pos1);   InitVector(Pos2);
  InitVector(SpinVector);
  InitVector(Path);   InitVector(V);

  InitNeutron(&Neutrons);
  Init3x3Matrix(LarmorMatrix);

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
  {
    for (i=0;i<NumNeutGot;i++)
    { 
      CHECK;

      /*InputNeutrons[i].Position[0]	= 0.;*/
      TOF = InputNeutrons[i].Time;
      WL = InputNeutrons[i].Wavelength;
      Prob = InputNeutrons[i].Probability;

      CopyVector(InputNeutrons[i].Position, Pos);
      CopyVector(InputNeutrons[i].Vector, Dir);
      CopyVector(InputNeutrons[i].Spin, SpinVector); 

      InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

      /* compute polarization and transmission location corresponding to the wavelength */
      datanumber = (int) (WL * 100.); 
      if(datanumber > FLD_SIZE) goto getlost;   

      /* translates into frame of the field domain */
      SubVector(Pos, PosMain); 

      /* calculate entrance end exit coordinates of domain*/

      /* rotates into the frame of the field domain   */
      CopyVector(Pos, pos);	
      CopyVector(Dir, dir);
	
      RotVector(RotMatrixMain, pos); 	
      RotVector(RotMatrixMain, dir); 

      /* gives intersection positions with domain */	
      if(IntersectionWithCylinder(DimMain, pos, dir, Pos1, Pos2) == 0) goto getlost; 

      /* rotates coordinates to previous frame */
      RotBackVector(RotMatrixMain, Pos1 );
      RotBackVector(RotMatrixMain, Pos2);

      /* ordering */
      if(Pos1[0] > Pos2[0]) 	
      {	CopyVector(Pos1, V);	CopyVector(Pos2, Pos1);	CopyVector(V, Pos2);}

      /* time of precession in the guide field - precession calculated in the field frame */
      TOF1 = fabs(Pos1[0] - Pos[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
      PhaseShift = TOF1 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)); NumberPrecessions1 = PhaseShift/2./M_PI;

      FillRotMatrixZY(LarmorMatrix, PhaseShift, 0); 

      RotVector(RotMatrixG_Field, SpinVector); 
      RotVector(LarmorMatrix, SpinVector);
      RotBackVector(RotMatrixG_Field, SpinVector); 

      /* moment of arriving at the domain wall, new position */
      TOF += TOF1;

      CopyVector(Pos1, Pos);

      /* time of precession in the domain field - precession calculated in the field frame */
      RotVector(RotMatrixGM_Field, SpinVector); 

      TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
      PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(LengthVector(guide_field_pol));  NumberPrecessions2 = PhaseShift/2./M_PI;

      FillRotMatrixZY(LarmorMatrix, PhaseShift, 0);

      RotVector(LarmorMatrix, SpinVector);
      RotBackVector(RotMatrixGM_Field, SpinVector);

      /* moment of exiting at the domain wall, new position */
      TOF += TOF2;

      CopyVector(Pos2, Pos);

      /* translates into original frame */
      AddVector(Pos, PosMain); 

      /* computes neutron variables in the output frame */
      SubVector(Pos, TranslOut);
      RotVector(RotMatrixOut, Pos);
      RotVector(RotMatrixOut, Dir);
      RotVector(RotMatrixOut, SpinVector);

      /* translates neutron variables for output - X'=0. */
      TOF3 =  - Pos[0] / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

      CopyVector(Dir, Path);
      MultiplyByScalar(Path, - Pos[0]/ Dir[0] );
      AddVector(Pos, Path);  

      /* time of precession in the guide field again - precession calculated in the field frame */
      RotVector(RotMatrixG_Field, SpinVector);

      PhaseShift = TOF3 * FREQUENCY_FROM_FIELD(LengthVector(field_guide));  NumberPrecessions3 = PhaseShift/2./M_PI;

      FillRotMatrixZY(LarmorMatrix, PhaseShift, 0);
      RotVector(LarmorMatrix, SpinVector);
      RotBackVector(RotMatrixG_Field, SpinVector);
	
      /* flipping process at some time */ 
      CartesianToSpherical(SpinVector, &the, &phi);

      pDown = sqrt(1. - polardata[datanumber]);
      the = 2. * (double) asin(pDown *(double) sin(the/2.));

      SphericalToCartesian(SpinVector, &the, &phi);

      /* Output matters */
      Prob *= transdata[datanumber]; 
      if(Prob <= ProbCutoff) goto getlost;

      IntegralIntensity += Prob;
      NumOut++;							/*goto jumpwrite;	jumpwrite :;*/

      /* transmit coordinates which were not changed, the rest overwrite below */
      Neutrons = InputNeutrons[i]; 
      Neutrons.Time = TOF+TOF3;
      Neutrons.Probability = Prob;

      CopyVector(Pos, Neutrons.Position);
      CopyVector(SpinVector, Neutrons.Spin);

      /* writes output binary file */ 
      WriteNeutron(&Neutrons);

  getlost:;

    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:
  if(NumOut != 0) fprintf(LogFilePtr,"Number of precessions  1   : %lf\n", NumberPrecessions1);
  if(NumOut != 0) fprintf(LogFilePtr,"Number of precessions  2   : %lf\n", NumberPrecessions2);
  if(NumOut != 0) fprintf(LogFilePtr,"Number of precessions  3   : %lf\n\n", NumberPrecessions3);

  /* write geometry file */
  SetGeometry("magenta");
  
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], 0.0, 0.0);	

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  double     roty_g =0.0, rotz_g =0.0; 
  double     roty_gm=0.0, rotz_gm=0.0; 

  /* initial values */
  InitVector(PosMain);
  InitVector(TranslOut);
  InitVector(DimMain);
  InitVector(field_guide);
  InitVector(field_pol);
  InitVector(guide_field_pol);

  Init3x3Matrix(RotMatrixMain);
  Init3x3Matrix(RotMatrixOut);
  Init3x3Matrix(RotMatrixG_Field);
  Init3x3Matrix(RotMatrixGM_Field);

  for (int j=0; j < FLD_SIZE; j++)
  { polardata[j] = 0.0;
    transdata[j] = 0.0;
  }

  ProbCutoff= wei_min;

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'a':
        sscanf(&argv[1][2], "%d", &Option);
        break;

      case 'b':
        sscanf(&argv[1][2], "%lf", &polHe3);
        break;
      case 'c':
        sscanf(&argv[1][2], "%lf", &polXsection);
        break;
      case 'd':
        sscanf(&argv[1][2], "%lf", &density);  
        break;

      case 'P':
        PolarizationFileName=&argv[1][2];
        break;
      case 'T':
        TransmissionFileName=&argv[1][2];
        break;
				
      case 'k':
        sscanf(&argv[1][2], "%lf", &PosMain[0]);
        break;
      case 'l':
        sscanf(&argv[1][2], "%lf", &PosMain[1]);
        break;
      case 'm':
        sscanf(&argv[1][2], "%lf", &PosMain[2]);
        break;

      case 'X':
        sscanf(&argv[1][2], "%lf", &DimMain[2]);
        break;
      case 'Y':
        sscanf(&argv[1][2], "%lf", &DimMain[0]);
        break;

      case 'G':
        sscanf(&argv[1][2], "%lf", &field_guide[0]);
        break;
      case 'H':
        sscanf(&argv[1][2], "%lf", &field_guide[1]);
        break;
      case 'K':
        sscanf(&argv[1][2], "%lf", &field_guide[2]);
        break;

      case 'M':
        sscanf(&argv[1][2], "%lf", &field_pol[0]);
        break;
      case 'N':
        sscanf(&argv[1][2], "%lf", &field_pol[1]);
        break;
      case 'O':
        sscanf(&argv[1][2], "%lf", &field_pol[2]);
        break;

      case 'p':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]);
        if (TranslOut[0] < (PosMain[0] + DimMain[0]/2.)) 
        {fprintf(LogFilePtr,"\nERROR: output position must be outside of flipper ! \n\n"); exit (-1);}
        break;
      case 'r':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]);
        break;
      case 's':
        sscanf(&argv[1][2], "%lf", &TranslOut[2]);
        break;
    }
    argc--;
    argv++;
  }
	
  AngleMainHoriz = 0.0;
  AngleMainVert	 = - M_PI/2. *(1+0.1e-10);

  FillRotMatrixZY(RotMatrixMain, AngleMainVert, AngleMainHoriz);
  FillRotMatrixZY(RotMatrixOut,  AnglOutVert, AnglOutHoriz);

  /* calculate field matrixes */
  if (LengthVector(field_guide) !=0) 
  { CartesianToEulerZY(field_guide, &roty_g, &rotz_g); 
  }
  else  
  { roty_g = 0.0; rotz_g = 0.0; 
  }
  FillRotMatrixZY(RotMatrixG_Field, roty_g, rotz_g); 

  CopyVector(field_guide,     guide_field_pol); 
  AddVector (guide_field_pol, field_pol);
  
  if (LengthVector(guide_field_pol) !=0) 
  { CartesianToEulerZY(guide_field_pol, &roty_gm, &rotz_gm); 
  }
  else 
  { roty_gm = 0; rotz_gm = 0; 
  }
  FillRotMatrixZY(RotMatrixGM_Field, roty_gm, rotz_gm); 

  /* case: calculation of polarization and transmissiom */
  if (Option == 1)
    WritePolAndTrans();

  ReadPolAndTrans();
 
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


/************************************************/
/** writes polarization and transmission file  **/
/************************************************/
void WritePolAndTrans()
{ 
  long   l=0;
  double Polarization=0.0, Transmission=0.0, 
         wavel=0.0, Mue=0.0, Nue=0.0;
  FILE  *pFileP=NULL,
        *pFileT=NULL;

  pFileP=OpenOutputFile(PolarizationFileName, TRUE, "r");
  pFileT=OpenOutputFile(TransmissionFileName, TRUE, "r");

  for (l=0; l < FLD_SIZE; l++)
  {
    wavel = 0.01 * (l+1); /*energy = ENERGY_FROM_LAMBDA(wavel)/1.E6;*/

    Mue = (polXsection * wavel /*5327 * sqrt(0.025/energy)  polarization cross section*/)* 1.E-24  * (density /*2.7E20  atomic density cm-3 at 10 atm pressure*/);
    Nue = (polHe3/100./*0.50 Polarization of He3*/) * Mue;
					
    Polarization = tanh(Nue * DimMain[0]);
    Transmission = exp(-Mue * DimMain[0]) * cosh(Nue * DimMain[0]);

    fprintf(pFileP, "  %le \n", Polarization);
    fprintf(pFileT, "  %le \n", Transmission);
  }
			
  fclose(pFileP);
  fclose(pFileT);
}


/************************************************/
/** reads polarization and transmission file   **/
/************************************************/
void ReadPolAndTrans()
{
  long  count=0;
  FILE* pFile=NULL;
  
  pFile=OpenInputFile2(PolarizationFileName, "polarization data", "r");

  for(count=0; count<FLD_SIZE; count++)
  {
    if (fscanf(pFile,"%le",&polardata[count])==EOF)
    break;
  }
  fclose(pFile);


  pFile = OpenInputFile2(TransmissionFileName, "transmission data", "r");

  for(count=0; count<FLD_SIZE; count++)
  {
    if (fscanf(pFile,"%le",&transdata[count])==EOF)
    break;
  }
  fclose(pFile);
}
