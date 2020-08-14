/********************************************************************************************/
/*  VITESS module 'flipper_gradient.c'   (developed from module 'rotating_field')           */
/*     simulation of a Drabkin spin-flip resonator with periodic plus guide magnetic fields */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/*                                                                                          */
/* 1.0  Aug 2003  R. Manoshin    Initial version, some simulations performed, alpha version */
/* 1.1  Oct 2003  R. Manoshin    Added spacing between domains, Alpha version               */
/* 1.2  Oct 2003  R. Manoshin    Added output of polarisation vector in cartesian,          */
/*                                 spherical and Euler coordinate systems                   */
/* 1.3  Dec 2003  R. Manoshin    Improve algorithm for +/- changing		                      */
/* 1.4  Feb 2004  R. Manoshin    Remove inlination of resonator                             */
/* 1.5  Jul 2020  K. Lieutenant  tidy up, new central visualization parameters              */
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
#include "resonator_drabkin.h"


/******************************/
/** Global Variables         **/
/******************************/
McCompID   _eModule=MCN_FIELD_ROT;
/* input parameters */double     depth = 10.0,               // -X        [cm]  x-component of the size of the Drabkin resonator 
           width = 10.0,               // -Y        [cm]  y-component of the size of the Drabkin resonator 
           height= 10.0;               // -V        [cm]  z-component of the size of the Drabkin resonator 
VectorType PosMain,                    // -k -l -m  [cm]  centre of the Drabkin resonator 
           TranslOut;                  // -p -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)
long       ind_x_max=2,                // -C        [-]   number of domains in X axis direction 
           ind_y_max=2,                // -D        [-]   number of domains in Y axis direction 
           ind_z_max=2;                // -E        [-]   number of domains in Z axis direction 
double     FieldValueInit=0.0,         // -d        [Oe]  amplitude of the periodic magnetic field
           FieldValueDevPer=0.0,       // -a        [%]   deviation of the amplitude of the periodic magnetic field in %!
           SigmaNorm =1.0;             // -x        [Oe]  deviation in the gauss distribution of the amplitude of the periodic magnetic field
int        keyaxis   =0,               // -M        [-]   enum: axis to which the periodic magnetic field is parallel 0: x  1: y  2: z
           keyampldistr=0,             // -v        [-]   enum: function of the variation of the periodic field amplitude: 0: Uniform, 1: Sinus,  2: Gauss
           DevLawAmpl  =0;             // -e        [-]   enum: distribution of the periodic magnetic field amplitude:     0: Normal,  1: Uniform
double     FieldValue0Init[3],         // -I -A -K  [Oe]  x-, y- and z-component  of the permanent magnetic field
           FieldValue0Dev=0.0;         // -q        [Oe]  amplitude of the additional random magnetic field  
int        keysph=0;                   // -S        [-]   flag: output of polarisation components during simulation: NO, YES
char      *Monitp=NULL,                // -O        [-]   output file for the polarization components  
          *Monitf=NULL;                // -N        [-]   output file for the magnetic field 
/* input parameters that are currently not used */double     spacemin=0.0,               // -y        [cm]  min. space between domains
           spacemax=0.0;               // -t        [cm]  max. space between domains
// Variables determined from input parameters or trajectory data
FILE*      fmonitf=NULL;                      //   [-]    pointer to output file for magnetic field
FILE*      fmonitp=NULL;                      //   [-]    pointer to output file for polarization components
double     FieldValue0[3];                    //   [Oe]   current additional permanent field
double     PolX[FIELD_SIZE_FL], PolY[FIELD_SIZE_FL], PolZ[FIELD_SIZE_FL], ProbM[FIELD_SIZE_FL],
           FldX[FIELD_SIZE_FL], FldY[FIELD_SIZE_FL], FldZ[FIELD_SIZE_FL], FldM [FIELD_SIZE_FL];


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  long   wall_1=0, wall_2=0, 
         ind_x=0, ind_y=0, ind_z=0;   // Number of domains in the X, Y and Z direction
  long   indp=1; 
  long   i=0, NumOut=0;
  long   keyperiod=0;                 // for periodical algorithm
  double FieldValue=0.0;
  double Number_NOP=0.0;              // numbe of domains passed by a trajectory
  double IntegralIntensity=0.0,
         spacecurr=0.0;
  double TOF=0.0, TOF1=0.0, TOF2=0.0, TOF3=0.0, 
         WL=0.0, Prob=0.0, 
         PhaseShift0=0.0, PhaseShift=0.0;
  double nPrecDom=0.0, nPrecTot=0.0, nPrecTrj=0.0;
  double RotMatrixField[3][3], LarmorMatrix[3][3];
  /* For random amplitude and frequency of magnetic field */	
  double FieldValueA=0.0, FieldValueB=0.0;  /* Internal variables */
  double SigmaField =0.0;                   /* Internal Variable */

  /* Variable for rotation */
  double Rroty=0.0, Rrotz=0.0, RRSM=0.0;
  double VX=0.0, VY=0.0, VZ=0.0;
  VectorType pos, dir;
  VectorType Pos, Dir, SpinVector, 
             Pos1, Pos2, domain_field, PosDomain, DimDomain;
  VectorType RR, RR1, RR2, RRS;
  Neutron    Neutrons;
  Neutron    NeutronAdd1, NeutronAdd2;
  Plane      EndPoint1, EndPoint2;


  // initialization
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.5");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  // local variables
  InitVector(pos);  InitVector(dir);
  InitVector(Pos);  InitVector(Dir);  InitVector(SpinVector);
  InitVector(Pos1); InitVector(Pos2); InitVector(domain_field);
  InitVector(PosDomain);              InitVector(DimDomain);  
  InitVector(RR);   InitVector(RR1);  InitVector(RR2);         InitVector(RRS);

  InitNeutron(&Neutrons); 
  InitNeutron(&NeutronAdd1); 
  InitNeutron(&NeutronAdd2);

  InitPlane(&EndPoint1); 
  InitPlane(&EndPoint2); 

  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(LarmorMatrix);

  FieldValue = FieldValueInit;

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons() != 0)
  {
    for(i=0;i<NumNeutGot;i++)
    { 
      CHECK;

      /*InputNeutrons[i].Position[0]	= 0.;*/
      TOF  = InputNeutrons[i].Time;
      WL   = InputNeutrons[i].Wavelength;
      Prob = InputNeutrons[i].Probability;

      CopyVector(InputNeutrons[i].Position, Pos);
      CopyVector(InputNeutrons[i].Vector, Dir);
	
      /* Check incorrect neutrons */
      if ((Dir[0] <= 0.0)||(WL == 0.0)) goto getlost;
      /* improve calculations, renormalize */
      InputNeutrons[i].Vector[0] = sqrt(fabs(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])));

      CopyVector(InputNeutrons[i].Spin, SpinVector); 

      /* translates into frame of the main field and rotates coordinates  */
      SubVector(Pos, PosMain);	
      /* Move neutron in the precession volume */
      NeutronAdd1.Position[0] = Pos[0];
      NeutronAdd1.Position[1] = Pos[1];
      NeutronAdd1.Position[2] = Pos[2];
	
      NeutronAdd1.Vector[0] = Dir[0];
      NeutronAdd1.Vector[1] = Dir[1];
      NeutronAdd1.Vector[2] = Dir[2];
	
      NeutronAdd1.Wavelength = WL;
	
      EndPoint1.A = 1.0;
      EndPoint1.B = 0.0;
      EndPoint1.C = 0.0;
      EndPoint1.D = 0.5*depth;
	
      TOF1 = NeutronPlaneIntersection1(&NeutronAdd1, EndPoint1);	
      if (TOF1 < 0.0) goto getlost;
	
      Pos[0] = NeutronAdd1.Position[0];
      Pos[1] = NeutronAdd1.Position[1];
      Pos[2] = NeutronAdd1.Position[2];
	
      Dir[2] = NeutronAdd1.Vector[2];
	
      TOF = TOF + TOF1;

      /* looks for first domain if dimension of domain changes only along X axis */
      DimDomain[0] = depth/ind_x_max; 
      DimDomain[1] = width/ind_y_max; 
      DimDomain[2] = height/ind_z_max; 

      ind_y = (long) floor(Pos[1] / DimDomain[1]) + 1 + ind_y_max/2;
      if ((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost;

      ind_z = (long) floor(Pos[2] / DimDomain[2]) + 1 + ind_z_max/2;
      if ((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost;

      ind_x = 1; 

      /******************** starts to scan ******************************/
      nPrecTrj = 0;
      Number_NOP = 0.0;
      indp = 1;
      keyperiod = 0;

      while (ind_x != (ind_x_max +1)) 
      {
        /* Generate geometry of magnetic field for precession */

        /* generate fieldsph domain size: uniform  */
        DimDomain[0] = depth/ind_x_max; 
        DimDomain[1] = width/ind_y_max; 
        DimDomain[2] = height/ind_z_max; 

        /* generate position */
        PosDomain[0] = ((ind_x -1)-(ind_x_max /2 - 0.5)) * DimDomain[0];
        PosDomain[1] = ((ind_y -1)-(ind_y_max /2 - 0.5)) * DimDomain[1];
        PosDomain[2] = ((ind_z -1)-(ind_z_max /2 - 0.5)) * DimDomain[2];

        /* define the periodical magnetic field */	
        /* Prepeare values for deviation of the magnetic field amplitude and frequency */
        switch(keyampldistr)
        {
          case 0:
          {
            /*			    fprintf(LogFilePtr,"Uniform Distribution of amplitude of periodical magnetic field. \n");	*/
            FieldValue = FieldValueInit;
            break;
          }
		
          case 1:	
          {
            /*		    	    fprintf(LogFilePtr,"Sinus Distribution of amplitude of periodical magnetic field. \n");	*/
            FieldValue = FieldValueInit*sin(M_PI*(Pos[0]+PosMain[0])/depth);
            break;
          }
		    
          case 2:	
          {
            /*		    	    fprintf(LogFilePtr,"Gauss Distribution of amplitude of periodical magnetic field. \n");	*/
            FieldValue = FieldValueInit*exp(Pos[0]*Pos[0]/(-2.0*SigmaNorm*SigmaNorm));
            break;
          }		    

          default:
          {
            fprintf(LogFilePtr,"ERROR: No Law! Correct option -v (Values 0, 1 or 2) \n");
            exit(-1);
            break;
          }
        }		
	
        FieldValueA = FieldValue - 0.01*FieldValueDevPer*fabs(FieldValue);
        FieldValueB = FieldValue + 0.01*FieldValueDevPer*fabs(FieldValue);
        SigmaField = 0.01*FieldValueDevPer*fabs(FieldValue);

        /* Perform Randomize of the magnetic field components, overload */
        if (FieldValueDevPer > 0.0)
        {
          switch(DevLawAmpl)
          {
            case 0:
            {
              FieldValue = DistrGauss(FieldValue, SigmaField);
              break;
            }
            case 1:	
            {
              FieldValue = MonteCarlo(FieldValueA, FieldValueB);
              break;
            }
            default:
            {
              fprintf(LogFilePtr,"ERROR: No Law! Correct option -e (Values 0, 1)\n");
              exit(-1);
              break;
            }
          }	
        }

        /* Activate periodical changing	*/
        if (keyperiod == 1) 
        {
          FieldValue = -1.0*FieldValue;
        }  
        /*	fprintf(LogFilePtr,"keyperiod = %d \n", keyperiod);  
        fprintf(LogFilePtr,"FieldValue = %f \n", FieldValue);	*/

        /* Activate permanent magnetic field */
        FieldValue0[0] = FieldValue0Init[0];
        FieldValue0[1] = FieldValue0Init[1];
        FieldValue0[2] = FieldValue0Init[2];	

        /* Perform random of the  permanent magentic field */
        if (FieldValue0Dev > 0.0)
        {
          // VLL = vector3rand(&VX, &VY, &VZ);
          gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);
          FieldValue0[0] = FieldValue0[0] + fabs(FieldValue0Dev)*VX;
          FieldValue0[1] = FieldValue0[1] + fabs(FieldValue0Dev)*VY;
          FieldValue0[2] = FieldValue0[2] + fabs(FieldValue0Dev)*VZ;
        }	

        /* Generate common field */
        switch(keyaxis)
        {
          case 0:
          {
            /* Choose parallel of axis 0X */
            RR[0] = FieldValue0[0] + FieldValue;
            RR[1] = FieldValue0[1];
            RR[2] = FieldValue0[2];
            break;
          }
          case 1:	
          {
            /* Choose parallel of axis 0Y */
            RR[0] = FieldValue0[0];
            RR[1] = FieldValue0[1] + FieldValue;
            RR[2] = FieldValue0[2];	
            break;
          }
          case 2:
          {
            /* Choose parallel of axis 0Z */
            RR[0] = FieldValue0[0];	
            RR[1] = FieldValue0[1];
            RR[2] = FieldValue0[2] + FieldValue;
            break;
          }
          default:
          {
            fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
            exit(-1);
            break;
          }
        }	
	
        /* process field */	
        RRS[0] = RR[0];
        RRS[1] = RR[1];
        RRS[2] = RR[2];
        RRSM = sqrt(RRS[0]*RRS[0] + RRS[1]*RRS[1] + RRS[2]*RRS[2]);
	    
        RR1[0] = RR[0]; 
        RR1[1] = RR[1]; 
        RR1[2] = RR[2];
	    
        RR2[0] = FieldValue0[0]; 
        RR2[1] = FieldValue0[1]; 
        RR2[2] = FieldValue0[2];
	    
        CartesianToEulerZY(RR, &Rroty, &Rrotz);
        domain_field[0] = LengthVector(RR);
        domain_field[1] = Rrotz;
        domain_field[2] = Rroty;
    
        /* Rotating option */
        FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]); 

        /* translates into frame of the field domain */
        SubVector(Pos, PosDomain);

        /* calculate entrance end exit coordinates of domain*/
       	CopyVector(Pos, pos);	CopyVector(Dir, dir);
	
        /* gives intersection positions with domain */
        if (IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) 
          goto getlost; 

        if (wall_2 == 0) goto getlost;

        /* ordering */
        if (Pos1[0] > Pos2[0]) 	
        {
          int wall=0; 
          VectorType V;

          CopyVector(Pos1, V);	CopyVector(Pos2, Pos1); CopyVector(V, Pos2); 	
          wall = wall_1; wall_1 = wall_2; wall_2 = wall;
        }		

        /* moment of arriving at the domain wall, new position */
        CopyVector(Pos1, Pos);

        /* time of precession in the domain field - precession calculated in the field frame */
        TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

        RotVector(RotMatrixField, SpinVector); 
        RotVector(RotMatrixField, RR1);

        PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]);
        PhaseShift0 = PhaseShift/(2.0*(M_PI));  
        nPrecTrj = nPrecTrj + PhaseShift0;

        FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);
        RotVector(LarmorMatrix, SpinVector);
        RotBackVector(RotMatrixField, SpinVector);

        if (RRSM != 0.0)
        {
          RRSM = 1.0;	
          FldX[indp] = FldX[indp] + (RRS[0]/RRSM);
          FldY[indp] = FldY[indp] + (RRS[1]/RRSM);
          FldZ[indp] = FldZ[indp] + (RRS[2]/RRSM);
          FldM[indp] = FldM[indp] + 1.0;
        }	

        /* moment of exiting at the domain wall, new position */
        TOF += TOF2;
        CopyVector(Pos2, Pos);

        /* translates back into main frame */
        AddVector(Pos, PosDomain);
	
        /* Passing via "between-domain" space */
        if (((spacemin != 0.0)||(spacemax != 0.0)) && (wall_2 == 2))
        {
          spacecurr=MonteCarlo(spacemin, spacemax);
          TOF2 = spacecurr/fabs(Dir[0])/V_FROM_LAMBDA(WL);
		
          /*		Perform additional precession only with guide field, begin */
          CartesianToEulerZY(RR2, &Rroty, &Rrotz);
          domain_field[0] = LengthVector(RR2);
          domain_field[1] = Rrotz;
          domain_field[2] = Rroty;
	    
          /* Rotating option */
          FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]); 

          RotVector(RotMatrixField, SpinVector); 
          PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]);
          PhaseShift0 = PhaseShift/(2.0*(M_PI));  
          nPrecTrj = nPrecTrj + PhaseShift0;
          FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);
          RotVector(LarmorMatrix, SpinVector);
          RotBackVector(RotMatrixField, SpinVector);		
          /*		End additional precession part */
        }
	
        PolX[indp] = PolX[indp] + Prob*SpinVector[0];
        PolY[indp] = PolY[indp] + Prob*SpinVector[1];
        PolZ[indp] = PolZ[indp] + Prob*SpinVector[2];
        ProbM[indp] = ProbM[indp] + Prob;
	
        /* Organize periodical changing */
        if (wall_2 == 2)
        {
          keyperiod = keyperiod + 1;
          if (keyperiod == 2) keyperiod = 0;
        }    
	
        indp = indp + 1;	
        Number_NOP = Number_NOP + 1.0;	
        /* searching new domain */ 
        if(wall_2 == 1) goto getlost;
        if(wall_2 == 2) {ind_x += 1; }
        if(wall_2 == 3) {ind_y += -1; }
        if(wall_2 == 4) {ind_y += 1; }
        if(wall_2 == 5) {ind_z += -1; }
        if(wall_2 == 6) {ind_z += 1;}

        /*if(ind_x > ind_x_max) goto exitfield; */

        if(ind_y == 0) goto exitfield; 
        if(ind_y > ind_y_max) goto exitfield; 
        if(ind_z == 0) goto exitfield; 
        if(ind_z > ind_z_max) goto exitfield;

        /*goto newdomain;*/
      }

    exitfield:;
      /*******************************************************************************/
      nPrecTot =  nPrecTot + nPrecTrj;

      if (Number_NOP != 0.0)
        nPrecDom =  nPrecDom + nPrecTrj/Number_NOP;

      /* Output matters */
      IntegralIntensity += Prob;
      NumOut++;

      AddVector(Pos, PosMain);	
      /* computes neutron variables in the output frame */ 
      SubVector(Pos, TranslOut);

      /* translates neutron variables for output - X'=0. */
      NeutronAdd2.Position[0] = Pos[0];
      NeutronAdd2.Position[1] = Pos[1];
      NeutronAdd2.Position[2] = Pos[2];
	
      NeutronAdd2.Vector[0] = Dir[0];
      NeutronAdd2.Vector[1] = Dir[1];
      NeutronAdd2.Vector[2] = Dir[2];
	
      NeutronAdd2.Wavelength = WL;
	
      EndPoint2.A = 1.0;
      EndPoint2.B = 0.0;
      EndPoint2.C = 0.0;
      EndPoint2.D = 0.0;

      TOF3 = NeutronPlaneIntersection1(&NeutronAdd2, EndPoint2);	
	
      Pos[0] = NeutronAdd2.Position[0];
      Pos[1] = NeutronAdd2.Position[1];
      Pos[2] = NeutronAdd2.Position[2];
	
      Dir[2] = NeutronAdd2.Vector[2];
		
      TOF = TOF + TOF3;

      Neutrons.ID.IDGrp[0]=InputNeutrons[i].ID.IDGrp[0];
      Neutrons.ID.IDGrp[1]=InputNeutrons[i].ID.IDGrp[1];
      Neutrons.ID.IDNo=InputNeutrons[i].ID.IDNo;
      Neutrons.Debug=InputNeutrons[i].Debug;

      Neutrons.Time = TOF;
      Neutrons.Wavelength = WL;
      Neutrons.Probability = Prob;

      CopyVector(Pos, Neutrons.Position);
      CopyVector(Dir, Neutrons.Vector);
      CopyVector(SpinVector, Neutrons.Spin);

      /* writes output binary file */
      WriteNeutron(&Neutrons);

  getlost:;

    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:
  /* write to log file */
  if (NumOut != 0) 
  {
    /* fprintf(LogFilePtr,"Full Number of precessions  : %f\n", nPrecTrj);
    if (Number_NOP != 0.0)
      fprintf(LogFilePtr,"Ave Number of precessions  : %f\n", nPrecTrj/Number_NOP); */    fprintf(LogFilePtr,"Average number of precession in resonator : %f\n", nPrecTot/NumOut);    fprintf(LogFilePtr,"Average number of precession per domain   : %f\n", nPrecDom/NumOut);
  }
  else
  { 
    fprintf(LogFilePtr,"ERROR: No neutrons in the exit of the precession volume, exit!!!\n");	    
    exit(-1);
  }

  /* write output file */
  WriteFiles(indp);

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
  long   ind_x=0;
  double FieldValue0Length=0.0, 
         OmegaFV0=0.0, OmegaFV0in=0.0;

  /* INIT */
  InitVector(PosMain);
  InitVector(TranslOut);

  /* permanent field components , projection in the axises OX, OY and OZ */
  /* current values */	
  FieldValue0[0] = 0.0; 
  FieldValue0[1] = 0.0;
  FieldValue0[2] = 0.0;
	
  /* initial values */
  FieldValue0Init[0] = 0.0; 
  FieldValue0Init[1] = 0.0;
  FieldValue0Init[2] = 0.0;	
	
  /* amplitude of additional random magnetic field */
	
  for(ind_x = 1; ind_x < FIELD_SIZE_FL; ind_x++) 
  {
    PolX[ind_x] = 0.0;
    PolY[ind_x] = 0.0;
    PolZ[ind_x] = 0.0;
    ProbM[ind_x] = 0.0;
		
    FldX[ind_x] = 0.0;
    FldY[ind_x] = 0.0;
    FldZ[ind_x] = 0.0;
    FldM[ind_x] = 0.0;
  }

  keyaxis = 0; /* activate rotation around 0x axis (beam direction) */	
  keysph = 0;  /* Not Activate output in the file the polarisation components */
  keyampldistr = 0; /* Key for choosing the distrinution of amplitude of the periodical magnetic field */

  while(argc>1)
  {
    switch(argv[1][1])
    {
      case 'O':
        Monitp=&argv[1][2];
        break;
      case 'N':
        Monitf=&argv[1][2];
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
      case 'p':
        sscanf(&argv[1][2], "%lf", &TranslOut[0]);
        break;
      case 'r':
        sscanf(&argv[1][2], "%lf", &TranslOut[1]);
        break;
      case 's':
        sscanf(&argv[1][2], "%lf", &TranslOut[2]);
        break;

      case 'X':
        sscanf(&argv[1][2], "%lf", &depth);
        break;
      case 'Y':
        sscanf(&argv[1][2], "%lf", &width);
        break;
      case 'V':
        sscanf(&argv[1][2], "%lf", &height);
        break;

        /* Input parameters characterizing the rotating magnetic field */
      case 'd':
        sscanf(&argv[1][2], "%lf", &FieldValueInit);        /* magnetic filed, Oe = Gauss */
        break;
      case 'a':
        sscanf(&argv[1][2], "%lf", &FieldValueDevPer);      /* deviation for amplitude of periodical field in % */
        break;      case 'e':
        sscanf(&argv[1][2], "%d", &DevLawAmpl);        /* Law of distribution of amplitute of periodical field: 0 - Normal(Default), 1 - Uniform */
        break;      case 'v':
        sscanf(&argv[1][2], "%d", &keyampldistr);        break;			
      case 'C':
        sscanf(&argv[1][2], "%ld", &ind_x_max);        /* Number of domains in X axis direction */
        break;
      case 'D':
        sscanf(&argv[1][2], "%ld", &ind_y_max);        /* Number of domains in Y axis direction */
        break;
      case 'E':
        sscanf(&argv[1][2], "%ld", &ind_z_max);        /* Number of domains in Z axis direction */
        break;
						
        /* Input parameters for permanent magnetic field */
      case 'I':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[0]);        /* value in Oe, OX component */
        break;      case 'A':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[1]);        /* value in Oe, OY component */
        break;
      case 'K':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[2]);        /* value in Oe, OZ component */
        break;
      case 'q':
        sscanf(&argv[1][2], "%lf", &FieldValue0Dev);
        break;			
      case 'M':
        sscanf(&argv[1][2], "%d", &keyaxis);        /* Values 0,1,2 - periodical magnetic field parallel axis 0x, 0y, 0z respectevly */
        break;      case 'S':
        sscanf(&argv[1][2], "%d", &keysph);        /* key for output polarisation components in the file during flight of domens */
        break;
			
        /* Spacing between domains */
      case 'y':
        sscanf(&argv[1][2], "%lf", &spacemin); /* in cm */
        break;      case 't':
        sscanf(&argv[1][2], "%lf", &spacemax); /* in cm */
        break;			
        /* SKO in the gauss distribution of the amplitude of periodical magnetic field */		
      case 'x':
        sscanf(&argv[1][2], "%lf", &SigmaNorm); 
        break;    }
    argc--;
    argv++;
  }

  /* Check initial dates */
  if (keysph == 1)
  {
    fprintf(LogFilePtr,"Activate output in the file the components of polarisation \n");
	
    if (Monitp == NULL)
      Error("You must define a file for polarisation output (Option -O)"); 
    if (Monitf == NULL)
      Error("You must define a file for the output of the magnetic field (Option -N)"); 
	    
    fmonitp = OpenOutputFile(Monitp, FALSE, "w");
    if (fmonitp==NULL)
      fprintf(LogFilePtr,"Warning: File %s could not be opened for output of polarisation components \n", Monitp);
	    
    fmonitf = OpenOutputFile(Monitf, FALSE, "w");
    if (fmonitf==NULL)
      fprintf(LogFilePtr,"Warning: File %s could not be opened for output of magnetic field \n",Monitf);
  }	
    
  if (depth <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Depth of magnetic field volume must be > 0 (option -X)\n");
    exit(-1);
  }
  if (width <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Width of magnetic field volume must be > 0 (option -Y)\n");
    exit(-1);
  }
  if (height <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Height of magnetic field volume must be > 0 (option -V)\n");
    exit(-1);
  }
	
  if ((ind_x_max <= 0)||(ind_x_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the X direction must be less than FIELD_SIZE=3000 and positiv! (Option -C) \n");
    exit(-1);
  }
  if ((ind_y_max <= 0)||(ind_y_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the Y direction must be less than FIELD_SIZE=3000 and positiv! (Option -D) \n");
    exit(-1);
  }
  if ((ind_z_max <= 0)||(ind_z_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the Z direction must be less than FIELD_SIZE=3000 and positiv! (Option -E) \n");
    exit(-1);
  }
	
  if (FieldValueDevPer < 0.0) 
  {
    fprintf(LogFilePtr,"ERROR: Deviation of amplitude of periodical field must be >= 0 \n");
    exit(-1);
  }		
  if (FieldValue0Dev < 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Amplitude of additional random magnetic field must be >= 0 \n");
    exit(-1);
  }

  switch (keyampldistr)
  {
    case 0:
    {
      fprintf(LogFilePtr,"Uniform Distribution of amplitude of periodical magnetic field. \n");
      break;
    }
    case 1:	
    {
      fprintf(LogFilePtr,"Sinus Distribution of amplitude of periodical magnetic field. \n");
      break;
    }
    case 2:	
    {		    
      if (SigmaNorm <= 0.0)
      {
        fprintf(LogFilePtr,"ERROR: SIGMA of Gauss Distribution of amplitude of periodical magnetic field must be positive! Exit \n");	
        exit(-1);
      }
      fprintf(LogFilePtr,"Gauss Distribution of amplitude of periodical magnetic field with sigma = %f \n", SigmaNorm);
      break;
    }		    
    default:
    {
      fprintf(LogFilePtr,"ERROR: No Law! Correct option -v (Values 0, 1 or 2) \n");
      exit(-1);
      break;
    }
  }	
	
  if (FieldValueDevPer > 0.0) 
  {
    fprintf(LogFilePtr,"Activate deviation of amplitude of periodical field  %f  percent,  ", FieldValueDevPer);
			
    switch(DevLawAmpl)
    {
      case 0:
      {
        fprintf(LogFilePtr,"Normal (Gauss) Distribution \n");
        break;
      }
      case 1:	
      {
        fprintf(LogFilePtr,"Uniform Distribution \n");
        break;
      }
      default:
      {
        fprintf(LogFilePtr,"ERROR: No Law! Correct option -e (Values 0, 1)\n");
        exit(-1);
        break;
      }
    }	
  }			    
	
  /* Display keyaxis */
  switch(keyaxis)
  {
    case 0:
    {
      /* Choose parallel of axis 0X */
      fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0X \n");
      break;
    }
    case 1:	
    {
      /* Choose parallel of axis 0Y */
      fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0Y \n");	
      break;
    }
    case 2:
    {
      /* Choose parallel of axis 0Z */
      fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0Z \n");			
      break;
    }
    default:
    {
      fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
      exit(-1);
      break;
    }
  }	
	    
  /* define the permanent(guide) magnetic field */
  FieldValue0[0] = FieldValue0Init[0]; 
  FieldValue0[1] = FieldValue0Init[1]; 
  FieldValue0[2] = FieldValue0Init[2]; 

  /*	Calculate first resonanse conditions for permanent field components	*/		
  if ((FieldValue0[0] != 0.0)||(FieldValue0[1] != 0.0)||(FieldValue0[2] != 0.0))
  {
    fprintf(LogFilePtr,"Drabkin RF flipper: Permanent magnetic field with values: Xo = %f Oe , Yo = %f Oe , Zo = %f Oe \n",FieldValue0[0], FieldValue0[1], FieldValue0[2]);
    FieldValue0Length = LengthVector(FieldValue0);
    fprintf(LogFilePtr,"Permanent magnetic field module =  %f  \n",FieldValue0Length);
    OmegaFV0 = (FREQUENCY_FROM_FIELD(FieldValue0Length));  /* KHz*2pi */
    OmegaFV0in = OmegaFV0*(depth/(double)(ind_x_max))/M_PI;
    if (OmegaFV0in == 0) 
    {
      fprintf(LogFilePtr,"ERROR: You have a zero velocity!!! exit...\n");
      exit(-1);
    }
    OmegaFV0in = 395.60346/OmegaFV0in;
    fprintf(LogFilePtr,"First resonance condition: Resonanse wavelength is expected = %f Ang \n", OmegaFV0in);
    fprintf(LogFilePtr,"----------------------------------------------------------------------------------------\n");
  }	
	
  /* Calculate PI-flipping condition, ONLY FOR Uniform Distribution of amplitude of periodical magnetic field. */	
  if (keyampldistr == 0)
  {
    fprintf(LogFilePtr,"Second resonance condition ONLY FOR Uniform Distribution of amplitude of periodical magnetic field.\n");
    OmegaFV0in = FieldValue0Length*M_PI/(2.0*ind_x_max);
    fprintf(LogFilePtr,"Amplitude of the periodical magnetic field  =  %f Oe\n", OmegaFV0in);
    fprintf(LogFilePtr,"---------------------------------------------------------------------------------------------------\n");    
  }
	
  if ((FieldValue0[0] == 0.0)&&(FieldValue0[1] == 0.0)&&(FieldValue0[2] == 0.0))
  {
    if (FieldValue0Dev > 0.0) 
    {
      fprintf(LogFilePtr,"WARNING!!! ONLY periodical magnetic field plus randomization. No additional permanent magnetic field! \n");
    }
    else
    {	
      fprintf(LogFilePtr,"WARNING!!! ONLY periodical magnetic field! No additional permanent magnetic field! \n");		
    }	
  }	

  if (FieldValue0Dev > 0.0) 
    fprintf(LogFilePtr,"Activate additional random magnetic field with amplitude  %f  Oe \n", FieldValue0Dev);	
	
  if ((spacemin < 0.0)||(spacemax < 0.0))				
  {
    fprintf(LogFilePtr,"ERROR: Spacing min and Spacing max must be both positive or zero! Exit.\n");
    exit(-1);
  }
	
  if (spacemin > spacemax)
  {
    fprintf(LogFilePtr,"ERROR: Spacemax must be more than Spacemin\n");
    exit(-1);
  }
	
  if ((spacemin > 0)||(spacemax > 0))
  {
    fprintf(LogFilePtr,"Spacing inside resonator is activated:\n");
    fprintf(LogFilePtr,"Uniform distribution between: Space_min = %f cm  Spacemax = %f cm\n",spacemin,spacemax);
  }    
}/* End OwnInit */

/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  if (fmonitp!=NULL)    
    fclose(fmonitp);

  if (fmonitf!=NULL)    
    fclose(fmonitf);
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


/*************************************************************/
/** Writes files containing polarisation and magnetic field **/
/*************************************************************/
void WriteFiles(long nDom)
{
  long       ind_x=0;
  VectorType PolP;
  double     roty=0.0,  rotz=0.0, 
             Theta=0.0, Phi=0.0, ModuleV=0.0;

  InitVector(PolP);

  if (keysph == 1)	
  {
    for(ind_x=1; ind_x < nDom; ind_x++) 
    {
      if (ProbM[ind_x] != 0.0)
      {
        /* Calculate polarisation vector in cartesian coordinates */	
        PolP[0] = PolX[ind_x]/ProbM[ind_x];
        PolP[1] = PolY[ind_x]/ProbM[ind_x]; 
        PolP[2] = PolZ[ind_x]/ProbM[ind_x];
				
        /* Calculate module of polarisation vector */
        ModuleV = sqrt((PolX[ind_x]/ProbM[ind_x])*(PolX[ind_x]/ProbM[ind_x]) + 
                       (PolY[ind_x]/ProbM[ind_x])*(PolY[ind_x]/ProbM[ind_x]) + 
                       (PolZ[ind_x]/ProbM[ind_x])*(PolZ[ind_x]/ProbM[ind_x]));			
				
        /* 'CartesianToSpherical' calculates Theta and Phi of a unit vector  */
        /* if Theta is the angle with the axis of lowest index               */
        CartesianToSpherical(PolP, &Theta, &Phi);
				
        /* 'CartesianToEulerZY' calculates Euler angles 'rotz' and 'roty'         */
        /* to transfer the x-axis to 'Vector' by rotation around y- and z-axis ZY */
        /* (cf. FillRotMatrixZY)                                                  */
        CartesianToEulerZY(PolP, &roty, &rotz);

        /* Ouput in file */
        fprintf(fmonitp,"%f   %f   %f   %f        %f    %f    %f        %f    %f   %f\n", 
                        PolP[0], PolP[1], PolP[2], ModuleV, Theta, Phi, ModuleV, roty, rotz, ModuleV);
      }
			
      if (FldM[ind_x] != 0.0)
      {
        fprintf(fmonitf,"%f     %f     %f     %f  \n", FldX[ind_x]/FldM[ind_x], 
                                                       FldY[ind_x]/FldM[ind_x], 
                                                       FldZ[ind_x]/FldM[ind_x],
                                                       sqrt((FldX[ind_x]/FldM[ind_x])*(FldX[ind_x]/FldM[ind_x]) +  
                                                            (FldY[ind_x]/FldM[ind_x])*(FldY[ind_x]/FldM[ind_x]) + 
                                                            (FldZ[ind_x]/FldM[ind_x])*(FldZ[ind_x]/FldM[ind_x])));
      }	
    }	
  }
}


/******************************************/
/** Intersection with rectangular object **/
/******************************************/
long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
  VectorType n, pos0, pos1, pos2, pos3, pos4, pos5;

  InitVector(pos0); InitVector(pos1); InitVector(pos2);
  InitVector(pos3); InitVector(pos4); InitVector(pos5);

  InitVector(Pos1); InitVector(Pos2);
  *wall_1 = *wall_2 = 0;

  n[0] = 1.0; n[1] = n[2] = 0.0; 
  // entrance
  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)            
  {
    if ((fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos0, Pos1); *wall_1 = 1;}
      else                           {CopyVector(pos0, Pos2); *wall_2 = 1;}
    }	
  } 
  // exit
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
  {
    if ((fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos1, Pos1); *wall_1 = 2;}
      else                           {CopyVector(pos1, Pos2); *wall_2 = 2;}
    }
  }

  n[1] = 1.0; n[2] = n[0] = 0.0;
  // right
  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
  {
    if ((fabs(pos2[0]) <= DimDomain[0]/2) && (fabs(pos2[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos2, Pos1); *wall_1 = 3;}
      else                           {CopyVector(pos2, Pos2); *wall_2 = 3;}
    }
  }
  // left
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
  {
    if ((fabs(pos3[0]) <= DimDomain[0]/2) && (fabs(pos3[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos3, Pos1); *wall_1 = 4;}
      else                           {CopyVector(pos3, Pos2); *wall_2 = 4;}
    }
  }

  n[2] = 1.0; n[0] = n[1] = 0.0;
  // bottom
  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
  {
    if ((fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos4, Pos1); *wall_1 = 5;}
      else                           {CopyVector(pos4, Pos2); *wall_2 = 5;}
    }
  }
  // top
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
  {
    if ((fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos5, Pos1); *wall_1 = 6;}
      else                           {CopyVector(pos5, Pos2); *wall_2 = 6;}
    }
  }

  if ((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) 
    return 0;
	
  return 1;

}/* End IntersectionWithRectangularWallNumber() */

