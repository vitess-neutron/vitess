/********************************************************************************************/
/*  VITESS module 'polariser_he3.c'                                                         */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0            G�za Zsigmond                                                             */
/* 1.1  JUL 2002  G�za Zsigmond  change                                                     */
/* 1.2  JAN 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/* 1.3  Jul 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.4  Feb 2023  K. Lieutenant  correction length and diameter; improved log file output   */
/* 1.4a Nov 2023  K. Lieutenant  improved visualization                                     */
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


/**************************/
/** Functions prototypes **/
/**************************/
void  OwnInit(int argc, char *argv[]);              // Reads input parameters and sets global parameters
void  OwnCleanup();                                 // Does module specific cleanup
void  SetGeometry(char* sColor);                    // Fills the structure stGeometry for visualization 
void  WritePolAndTrans();                           // Writes polarization and transmission data from file  
void  ReadPolAndTrans();                            // Reads polarization and transmission data from file  
double AbsorptionProbability(double wavelength, int spinState);   // calculates absorption (returns 0) or transmission (returns 1)

/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
short      bCalc=FALSE;                  // -a        [-]    flag: analytical calculation of polarization and tranmission data
double     polHe3=0.0,                   // -b        [%]    polarisation of the He3 (for analytical calculation)
           polXsection=2945.0,           // -c       [barn]  polarisation cross section of 1 Ang neutrons in He3
           density=0.0;                  // -d      [1/cm3]  density of the He3 gas 
char      *sPolFile  ="polarization.dat",// -P        [-]    pointer to the name of the data file of the wavelength dependent polarisation
          *sTransFile="transmission.dat";// -T        [-]    pointer to the name of the data file of the wavelength dependent transmission
VectorType PosMain,                      // -k -l -m  [cm]   center position of the cylindrical polariser
           DimMain,                      // -X    -Y  [cm]   length and diameter of the cylindrical polariser
           field_guide,                  // -G -H -K  [Oe]   strength of the magnetic guide field
           field_pol,                    // -M -N -O  [Oe]   field in the polarization chamber (which is added to the guide) 
           TranslOut;                    // -p -r -s  [cm]   position of the new origin  (in the co-ordinate of the old origin)

// Input parameters that are currently not used
double     AngleMainHoriz=0.0,           //          [deg]   horizontal rotation angle (about z axis) of the polarizer
           AngleMainVert=0.0;            //          [deg]   vertical (second) rotation angle of the polarizer
double     AnglOutHoriz=0.0,             //          [deg]   horizontal angle of the output frame, relative to input orientation
           AnglOutVert=0.0;              //          [deg]   vertical angle of the output frame, relative to input orientation    

// Variables determined from input parameters or trajectory data
char       sOption[13]="";               //                  text for origin of transmission and polarization data (from -a bCalc) 
VectorType guide_field_pol;              //           [Oe]   sum of polarization and guide field
double     RotMatrixMain[3][3],          //           [-]    rotation matrix to tranform into the frame of the polarizer
           RotMatrixOut [3][3],          //           [-]    rotation matrix to tranform into the output frame  
           RotMatrixG_Field [3][3],      //           [-]    rotation matrix to tranform into frame of the guide field
           RotMatrixGM_Field[3][3],      //           [-]    rotation matrix to tranform into frame of guide + polarization field
           ProbCutoff=0.0;               //           [-]    = wei_min: minimal accepted weight of the neutron trajectory

// Data read from file
double     aPolData  [FLD_SIZE], 
           aTransData[FLD_SIZE];

// value of Polarisation at the neutron wavelentgth taken from datafile when option numerical is given
double aPolData_wl;


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long   NumOut=0, i=0 , datanumber=0;
  double TOF=0.0, TOF1=0.0, TOF2=0.0, TOF3=0.0, 
         WL=0.0, Prob=0.0, PhaseShift=0.0, 
         nPrecessions1=0.0, nTraj1=0, nTotPrec1=0,  // number of precessions of a neutron, 
         nPrecessions2=0.0, nTraj2=0, nTotPrec2=0,  //   number of trajectories (=neutrons)
         nPrecessions3=0.0, nTraj3=0, nTotPrec3=0;  //   total number of precessions of the neutrons passing through field i (1,2,3) 
  double TotIntensityIn =0.0, TotIntensityOut=0.0,  // incoming and outgoing beam intensity 
         IntPolarization=0.0,                       // integration of all polarization values 
         T_avrg, P_avrg;                            // average transmission and polarization 
  double LarmorMatrix[3][3];
  VectorType Pos, Dir, SpinVector, Pos1, Pos2;
  VectorType Path;     /* Path = displacement vector */
  VectorType pos, dir;	
  VectorType V;	
  Neutron    OutNeutron, ScatNeutron;
  int spinState = 0;
  int n_abs = 0;
  VectorType pol_dir;
  VectorType Spin_dir;
  const double cutoff = 1e-6;  // Precision threshold for spin alignment
  double dot_product = 0.0;   // dot product of spin and pol field

  // initialization
  // --------------
  _eModule=MCN_POL_HE3;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.4a");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = TRUE;

  InitVector(pos);    InitVector(dir);
  InitVector(Pos);    InitVector(Dir);
  InitVector(Pos1);   InitVector(Pos2);
  InitVector(SpinVector);
  InitVector(Path);   InitVector(V);
  InitVector(pol_dir);
  InitVector(Spin_dir);

  InitNeutron(&OutNeutron); InitNeutron(&ScatNeutron);
  Init3x3Matrix(LarmorMatrix);

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while ((ReadNeutrons())!= 0)
  {
    for (i=0;i<NumNeutGot;i++)
    { 
      CHECK;

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      {
        InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]));

        TOF  = InputNeutrons[i].Time;
        WL   = InputNeutrons[i].Wavelength;
        Prob = InputNeutrons[i].Probability;
        TotIntensityIn += Prob;

        CopyVector(InputNeutrons[i].Position, Pos);
        CopyVector(InputNeutrons[i].Vector, Dir);
        CopyVector(InputNeutrons[i].Spin, SpinVector);
        CopyVector(InputNeutrons[i].Spin, Spin_dir); 

        /* Check whether the neutron is absorbed or transmitted */
        CopyVector(guide_field_pol, pol_dir);
        NormVector(pol_dir);

        dot_product = ScalarProduct(Spin_dir, pol_dir);
        if (fabs(dot_product - 1.0) < cutoff)   // Close to 1 → Parallel
          spinState = 1;  
        else if (fabs(dot_product + 1.0) < cutoff)  // Close to -1 → Antiparallel
          spinState = -1;  
        else
          spinState = 0;  // Neither fully aligned nor fully anti-aligned

        /* case where neutron spin and field are perpendicular */
        if(spinState == 0) {
          fprintf(LogFilePtr,"Warning: neutron spin polarization and field are perpendicular.\n");
          fprintf(LogFilePtr,"Spin vector: %4.3f, %4.3f, %4.3f\n", Spin_dir[0], Spin_dir[1], Spin_dir[2]);
          fprintf(LogFilePtr,"Pol dir: %4.3f, %4.3f, %4.3f\n", pol_dir[0], pol_dir[1], pol_dir[2]);
          fprintf(LogFilePtr,"Hint: check them and change the direction of one of them.\n");
          return 1;
        }

        /* compute polarization and transmission location corresponding to the wavelength */
        datanumber = (int) (WL * 100.);
        if(datanumber > FLD_SIZE) goto getlost; 
        if(!bCalc) aPolData_wl = aPolData[datanumber];


        if ( AbsorptionProbability( WL, spinState) == 1) {
          n_abs = n_abs + 1;
          goto getlost;
        }

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
        RotBackVector(RotMatrixMain, Pos1);
        RotBackVector(RotMatrixMain, Pos2);

        /* ordering */
        if(Pos1[0] > Pos2[0]) 	
        {	CopyVector(Pos1, V);	CopyVector(Pos2, Pos1);	CopyVector(V, Pos2);}

        /* time of precession in the guide field - precession calculated in the field frame */
        TOF1 = fabs(Pos1[0] - Pos[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
        PhaseShift = TOF1 * FREQUENCY_FROM_FIELD(LengthVector(field_guide)); 
        nPrecessions1 = PhaseShift/2./M_PI;
        nTotPrec1 += nPrecessions1;
        nTraj1++;

        FillRotMatrixX(LarmorMatrix, PhaseShift); 

        RotVector(RotMatrixG_Field, SpinVector);
        RotVector(LarmorMatrix, SpinVector);
        RotBackVector(RotMatrixG_Field, SpinVector);

        /* moment of arriving at the domain wall, new position */
        TOF += TOF1;

        CopyVector(Pos1, Pos);

        /* time of precession in the domain field - precession calculated in the field frame */
        RotVector(RotMatrixGM_Field, SpinVector);

        TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
        PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(LengthVector(guide_field_pol));  
        nPrecessions2 = PhaseShift/2./M_PI;
        nTotPrec2 += nPrecessions2;
        nTraj2++;

        FillRotMatrixX(LarmorMatrix, PhaseShift); 

        RotVector(LarmorMatrix, SpinVector);
        RotBackVector(RotMatrixGM_Field, SpinVector);

        /* moment of exiting at the domain wall, new position */
        TOF += TOF2;

        CopyVector(Pos2, Pos);

        /* translates into original frame */
        AddVector(Pos, PosMain); 

        /* Write point of exit from field */
        if (bVisTraj)        
        { 
          CopyNeutron(&InputNeutrons[i], &ScatNeutron);
          CopyVector(Pos, ScatNeutron.Position);
          CopyVector(SpinVector, ScatNeutron.Spin);
          WriteWWP(&ScatNeutron, VT_EXITED);    // direction and TOF not needed
        }

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
        

        /* time of precession in the guide field - precession calculated in the field frame */
        RotVector(RotMatrixG_Field, SpinVector);
      
        PhaseShift = TOF3 * FREQUENCY_FROM_FIELD(LengthVector(field_guide));  
        nPrecessions3 = PhaseShift/2./M_PI;
        nTotPrec3 += nPrecessions3;
        nTraj3++;

        FillRotMatrixX(LarmorMatrix, PhaseShift); 
        RotVector(LarmorMatrix, SpinVector);
        RotBackVector(RotMatrixG_Field, SpinVector);

        /* Output matters */
        Prob *= aTransData[datanumber]; 
        if(Prob <= ProbCutoff) goto getlost;

        TotIntensityOut += Prob;
        IntPolarization += Prob*aPolData[datanumber];
        NumOut++;							/*goto jumpwrite;	jumpwrite :;*/

        /* transmit coordinates which were not changed, the rest overwrite below */
        OutNeutron = InputNeutrons[i]; 
        OutNeutron.Time = TOF+TOF3;
        OutNeutron.Probability = Prob;

        CopyVector(Pos, OutNeutron.Position);
        CopyVector(SpinVector, OutNeutron.Spin);

        /* writes output binary file */ 
        WriteNeutron(&OutNeutron);

        /* point of exit for trajectory visualization */
        WriteScatIAP(&OutNeutron, VT_EXITED, RotMatrixOut, TranslOut);

      getlost:;
      }
    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:
  P_avrg = IntPolarization/TotIntensityOut;
  T_avrg = TotIntensityOut/TotIntensityIn; 
  fprintf(LogFilePtr, "Polarization file: %s %s\n",                      sPolFile,   sOption);
  fprintf(LogFilePtr, "Transmission file: %s %s\n",                      sTransFile, sOption);
  if (NumOut > 0) fprintf(LogFilePtr,"Average polarization P: %7.5lf\n", P_avrg);
  if (NumOut > 0) fprintf(LogFilePtr,"Average transmission T: %7.5lf\n", T_avrg);
  if (NumOut > 0) fprintf(LogFilePtr,"Figure of merit T*P^2 : %7.5lf\n", T_avrg*sq(P_avrg));
  if (nTraj1 > 0) fprintf(LogFilePtr,"Average number of precessions before polarizer: %7.2lf\n",   nTotPrec1/nTraj1);
  if (nTraj2 > 0) fprintf(LogFilePtr,"                              in polarizer    : %7.2lf\n",   nTotPrec2/nTraj2);
  if (nTraj3 > 0) fprintf(LogFilePtr,"                              after polarizer : %7.2lf\n\n", nTotPrec3/nTraj3);

  fprintf(LogFilePtr, "Number of neutrons absorbed: %d\n",n_abs);
  /* write geometry file */
  SetGeometry("orange");
  
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
  int j;

  /* initial values */
  InitVector(PosMain);
  InitVector(TranslOut);
  InitVector(DimMain);
  InitVector(field_guide);
  InitVector(field_pol);
  InitVector(guide_field_pol);

  InitRotMatrix(RotMatrixMain);
  InitRotMatrix(RotMatrixOut);
  InitRotMatrix(RotMatrixG_Field);
  InitRotMatrix(RotMatrixGM_Field);

  for (j=0; j < FLD_SIZE; j++)
  { aPolData[j] = 0.0;
    aTransData[j] = 0.0;
  }

  ProbCutoff= wei_min;

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'a':
        sscanf(&argv[1][2], "%hd", &bCalc);
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
        sPolFile=&argv[1][2];
        break;
      case 'T':
        sTransFile=&argv[1][2];
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
        sscanf(&argv[1][2], "%lf", &DimMain[2]);  // length of the polarizer
        break;
      case 'Y':
        sscanf(&argv[1][2], "%lf", &DimMain[0]);  // diameter of the polarizer
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
        if (TranslOut[0] < (PosMain[0] + DimMain[2]/2.)) 
        {fprintf(LogFilePtr,"\nERROR: output position must be beyond the polarizer ! \n\n"); exit (-1);}
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
  FillRotMatrixZY(RotMatrixG_Field,roty_g,rotz_g); 

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
  if (bCalc == TRUE)
  { WritePolAndTrans();
    strncpy(sOption, "calculated", sizeof(sOption)-1);
  }
  else
  { Note("P(lambda) and T(lambda) taken from file.  He-3 polarization, He-3 particle density and polarizer length are not considered");
  }

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

    stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
    stGeometry.nCylinders = 1;

    stGeometry.pCylinder[0].Radius      = DimMain[0]/2.0*BlowUp;
    stGeometry.pCylinder[0].Length      = DimMain[2];
    stGeometry.pCylinder[0].vCntr[0]    = PosMain[0];
    stGeometry.pCylinder[0].vCntr[1]    = PosMain[1]*BlowUp;
    stGeometry.pCylinder[0].vCntr[2]    = PosMain[2]*BlowUp;
    stGeometry.pCylinder[0].vSymAxis[0] = 1.0;
    stGeometry.pCylinder[0].vSymAxis[1] = 0.0;
    stGeometry.pCylinder[0].vSymAxis[2] = 0.0;
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

  pFileP=OpenOutputFile(sPolFile,   TRUE, "w");
  pFileT=OpenOutputFile(sTransFile, TRUE, "w");

  for (l=0; l < FLD_SIZE; l++)
  {
    wavel = 0.01 * (l+1); /*energy = ENERGY_FROM_LAMBDA(wavel)/1.E6;*/

    Mue = (polXsection * wavel )*1.E-24  * density;  /* 5327 * sqrt(0.025/energy)  polarization cross section, barn->cm^2, 2.7E19 particle density cm-3 per atm pressure */
    Nue = (polHe3/100.0) * Mue;                      /* Polarization of He3*/
					
    Polarization = tanh(Nue * DimMain[2]);
    Transmission = exp(-Mue * DimMain[2]) * cosh(Nue * DimMain[2]);

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
  
  pFile=OpenInputFile2(sPolFile, "polarization data", "r");

  for(count=0; count<FLD_SIZE; count++)
  {
    if (fscanf(pFile,"%le",&aPolData[count])==EOF)
    break;
  }
  fclose(pFile);


  pFile = OpenInputFile2(sTransFile, "transmission data", "r");

  for(count=0; count<FLD_SIZE; count++)
  {
    if (fscanf(pFile,"%le",&aTransData[count])==EOF)
    break;
  }
  fclose(pFile);
}

double AbsorptionProbability(double wavelength, int spinState) {
    double Mue = 0.0, Nue = 0.0;
    double Polarization = 0.0;
    double AbsProb = 0.0;
    double tmp_rand = 0.0;

    tmp_rand = MonteCarlo(0.,1.);
    
    if (bCalc) {  
        /* Analytical Calculation */
        Mue = (polXsection * wavelength) * 1.E-24 * density;  
        Nue = (polHe3 / 100.0) * Mue;  
        
        Polarization = tanh(Nue * DimMain[2]);
    } else {  
        /* Uses Polarisation from file */
        Polarization = aPolData_wl;
    }

    /* Define absorption probability based on spin state */
    if (spinState == 1) {  // Spin parallel to field
        AbsProb = 0.5 * (1 - Polarization);
    } else {  // Spin antiparallel
        AbsProb = 0.5 * (1 + Polarization);
    }

    /* Absorb trajectory with probability AbsProb */
    if (tmp_rand < AbsProb) {
        return 1.0;  // Absorbed
    } else {
        return 0.0;  // Not absorbed
    }
}
