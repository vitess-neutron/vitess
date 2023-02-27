/********************************************************************************************/
/*  VITESS module 'flipper_gradient.c'   (for gradient flipper used in TOF-NSE)             */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/*                                                                                          */
/*	  1. allows simulations of the gradient magnetic field (linear changing),               */        
/*       cos-changing and permanent for all components,                                     */    
/*       additional values: Oe/cm for each axis 0X, 0Y, 0Z,                                 */
/*    2. Amplitude of rotating field can change in the sinus law or permanent or solenoid   */
/*                                                                                          */
/* 1.0  Jul 2003  R. Manoshin    initial version (converted from rotating_field v1.5        */ 
/* 1.1  Dec 2003  R. Manoshin    Improve algorith for slice changing                        */
/* 1.2  Feb 2004  R. Manoshin    flipper inclination -  bug solved                          */
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
#include "flipper_gradient.h"


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
double     depth= 0.0,                 // -X        [cm]  x-component of the size of the flipper 
           width= 0.0,                 // -Y        [cm]  y-component of the size of the flipper
           height=0.0;                 // -V        [cm]  z-component of the size of the flipper
VectorType PosMain;                    // -k -l -m  [cm]  centre of the flipper
VectorType TranslOut;                  // -p -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)
double     AnglMainHoriz=0.0;          // -i       [deg]  horizontal rotation angle (about z axis) of the magnetic field
long       ind_x_max=2,                // -C        [-]   Number of domains (='boxes' in which the field is devided) in the X direction
           ind_y_max=2,                // -D        [-]   Number of domains (='boxes' in which the field is devided) in the X direction
           ind_z_max=2;                // -E        [-]   Number of domains (='boxes' in which the field is devided) in the X direction
double     FieldValueInit=0.0,         // -d        [Oe]  amplitude of the magnetic field
           FieldValueDevPer=0.0,       // -a        [%]   deviation of the amplitude of the magnetic field in %!
           omegainit  =0.0,            // -w        [Hz]  number of rotations per second
           OmegaDevPer=0.0,            // -b        [%]   deviation of the frequency of the magnetic field in % 
           phi0d      =0.0;            // -z       [deg]  initial phase
int        keyaxis   =0,               // -M        [-]   Direction about which the field rotates: x-, y-, z-axis (values 0, 1, 2 resp.)
           keyrotampl=0,               // -h        [-]   Function of the variation of the rotating field amplitude: SINUS, PERMANENT, SOLENOID (values 0, 1, 2 resp.)
           keyrotampldir=0,            // -y        [-]   Direction of changing of the rotating field amplitude: x-, y-, z-axis (values 0, 1, 2 resp.) 
           DevLawAmpl=0,               // -e        [-]   Distribution of the amplitude of the rotating field: 0: Normal (default), 1: Uniform
           DevLawFreq=0,               // -v        [-]   Distribution of the frequency of the rotating field: 0: Normal (default), 1: Uniform
           keyphase  =1;               // -n        [-]   Neutron TOF from preceding modules is used for the rotating field phase: NO, YES  

int        keyguidech=0,               // -u        [-]   Function of the variation of the guide field amplitude: COSINUS, LINEAR, PERMANENT, (values 0, 1, 2 resp.)
           keyguidechdir=0;            // -t        [-]   Direction of changing of the guide field amplitude: x-, y-, z-axis (values 0, 1, 2 resp.)
double     FieldValue0Init[3],         // -I -A -K  [Oe]  x-, y- and z-component  of the permanent or initial magnetic field
           FieldValue0Grlin[3],        // -P -Q -R  [Oe]  x-, y- and z-component  of the amplitude or final   magnetic field
           FieldValue0Dev=0.0;         // -q        [Oe]  Amplitude of the additional random magnetic field  
int        keysph=0;                   // -S        [-]   flag: output of intermediate simulation results (spin and total magn. field):  NO, YES
char      *Monitp=NULL,                // -O        [-]   output file for the polarization components  
          *Monitf=NULL;                // -N        [-]   output file for the magnetic field 

/* input parameters that are currently not used */int        keyrot=0;                   // -c        [-]   flag: rectangular pulse field (instead of rotating field):  NO, YES
	
// Variables determined from input parameters or trajectory data
/* general */  
FILE*      fmonitf=NULL;               //           [-]   pointer to output file for  magnetic field
FILE*      fmonitp=NULL;               //           [-]   pointer to output file for polarization components
long       ind_x=0, ind_y=0, ind_z=0;  //           [-]   indices of magnetic field elements in x-, y- and z-direction
double     Omega=0.0, OmegaInit=0.0,   //        [rad/ms] angular frequency
           phi0=0.0;                   //          [rad]  Rotating magnetic field FieldValue*sin(Omega*t + phi0); initial and current value
double     PeriodInit;                 //           [ms]  Period of rectangular pulse field 

/* For random amplitude and frequency of rotating (pulse) field */	
double FieldValueA=0.0, FieldValueB=0.0;                 // Internal variables
double OmegaA=0.0,      OmegaB=0.0;                      // Internal variables
double PeriodA=0.0,     PeriodB=0.0;                     // Internal variables 
double SigmaField=0.0,  SigmaOmega=0.0, SigmaPeriod=0.0; // Internal Variables


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  int     i=0;
  long    NumOut=0,
          indp  =0,
          wall_1=0, wall_2=0;
  double  IntegralIntensity   =0.0,
          NumberPrecessions   =0.0,
          NumberPrecessionsave=0.0, 
          NumberPrecessionssum=0.0, 
          PhaseShift =0.0,
          PhaseShift0=0.0;
  double  TOF=0.0, WL=0.0, Prob=0.0, 
          TOF1=0.0, TOF2=0.0, TOF3=0.0; 
  double  FieldValue;                            // Rotating magnetic field FieldValue*sin(Omega*t + phi0); Initial and current values 
  double  Period;                                // Period of rectangular pulse field 
  double  Number_NOP=0.0;
  double  TOFP;                                  // Neutron TOF from preceding modules for synhro rotations
  double  FieldValue0[3];                        // Additional permanent field to the rotating or initial value for linear changing
  double  PolX[FIELD_SIZE_FL], PolY[FIELD_SIZE_FL], PolZ[FIELD_SIZE_FL], ProbM[FIELD_SIZE_FL],
          FldX[FIELD_SIZE_FL], FldY[FIELD_SIZE_FL], FldZ[FIELD_SIZE_FL], FldM[FIELD_SIZE_FL];
  double  RotMatrixMain[3][3], RotMatrixField[3][3], LarmorMatrix[3][3];
  VectorType Pos, Dir, SpinVector, Pos1, Pos2, domain_field, PosDomain, DimDomain;
  Neutron	Neutrons, NeutronAdd1, NeutronAdd2;
  Plane   EndPoint1,   EndPoint2;

  /* Variables for rotation */
  double     TimeR=0.0, Rroty=0.0, Rrotz=0.0, RRSM=0.0;
  double     VX=0.0, VY=0.0, VZ=0.0;
  VectorType RR, RR1, RRS;
  int k;

  // initialization
  // --------------
  _eModule=MCN_FLIP_GRAD;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);

  bVisInstalled = MISSING;
  if (bVisInstr) 
    bBlowUp = TRUE;

  // local variables
  for(ind_x = 1; ind_x < FIELD_SIZE_FL; ind_x++) 
  {
    PolX[ind_x] = 0.0;
    PolY[ind_x] = 0.0;
    PolZ[ind_x] = 0.0;
    ProbM[ind_x]= 0.0;
    FldX[ind_x] = 0.0;
    FldY[ind_x] = 0.0;
    FldZ[ind_x] = 0.0;
    FldM[ind_x] = 0.0;
  }

  for (k=0; k < 3; k++)
    FieldValue0[k] = 0.0; 
	
  InitVector(Pos1); InitVector(Pos2); InitVector(domain_field);
  InitVector(PosDomain);              InitVector(DimDomain);  
  InitVector(Pos);  InitVector(Dir);  InitVector(SpinVector);
  InitVector(RR);   InitVector(RR1);  InitVector(RRS);

  InitNeutron(&Neutrons); 
  InitNeutron(&NeutronAdd1); 
  InitNeutron(&NeutronAdd2); 

  FieldValue = FieldValueInit;	
  Period     = PeriodInit;

  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(LarmorMatrix);
  FillRotMatrixZY(RotMatrixMain, 0.0, AnglMainHoriz);
	
  EndPoint1.A = cos(AnglMainHoriz);
  EndPoint1.B = sin(AnglMainHoriz);
  EndPoint1.C = 0.0;
  EndPoint1.D = 0.5*depth-PosMain[0]*cos(AnglMainHoriz);
	
  EndPoint2.A = 1.0;
  EndPoint2.B = 0.0;
  EndPoint2.C = 0.0;
  EndPoint2.D = 0.0;

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons()!=0)
  {
    for (i=0; i < NumNeutGot; i++)
    { 
      CHECK;

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /*InputNeutrons[i].Position[0]	= 0.0;*/
        TOF  = InputNeutrons[i].Time;
        WL   = InputNeutrons[i].Wavelength;
        Prob = InputNeutrons[i].Probability;

        CopyVector(InputNeutrons[i].Position, Pos);
        CopyVector(InputNeutrons[i].Vector, Dir);
        CopyVector(InputNeutrons[i].Spin, SpinVector); 
	
        /* Check incorrect neutrons */
        if ((Dir[0] <= 0.0)||(WL == 0.0)) goto getlost;
        /* improve calculations, renormalize */
        InputNeutrons[i].Vector[0]	= (double) sqrt(fabs(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])));

        /* Move neutron in the precession volume */
        NeutronAdd1.Position[0] = Pos[0];
        NeutronAdd1.Position[1] = Pos[1];
        NeutronAdd1.Position[2] = Pos[2];
	
        NeutronAdd1.Vector[0] = Dir[0];
        NeutronAdd1.Vector[1] = Dir[1];
        NeutronAdd1.Vector[2] = Dir[2];
	
        NeutronAdd1.Wavelength = WL;
	
        TOF1 = NeutronPlaneIntersection1(&NeutronAdd1, EndPoint1);	
		
        if (TOF1 < 0.0) goto getlost;
	
        Pos[0] = NeutronAdd1.Position[0];
        Pos[1] = NeutronAdd1.Position[1];
        Pos[2] = NeutronAdd1.Position[2];
	
        Dir[2] = NeutronAdd1.Vector[2];
	
        TOF = TOF + TOF1;

        SubVector(Pos, PosMain);				
        RotVector(RotMatrixMain, Pos ); 
        RotVector(RotMatrixMain, Dir ); 

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
        NumberPrecessions = 0.0;
        Number_NOP = 0.0;
        TimeR = 0.0;
        indp = 1;
	
        /* Use the previous TOF for rotating field, synhro!, corrected */
        if (keyphase == 1)
        {
          TOFP = InputNeutrons[i].Time + TOF1;
        }
        else
        {
          TOFP = 0.0;
        }

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

          if ((ind_x == 1)||(wall_2 == 2))
          {
            /* Perform Randomize the frequency of rotating or pulse magnetic fields, overload */
            if (keyrot == 0)
            {
              Omega = OmegaInit;
            }
            else
            {
              Period = PeriodInit;    
            }			
	
            if (OmegaDevPer > 0.0)
            {
              switch(DevLawFreq)
              {
                case 0:
                {
                  if (keyrot == 0)
                  {
                    Omega = DistrGauss(OmegaInit, SigmaOmega);
                  }
                  else
                  {    
                    Period = DistrGauss(PeriodInit, SigmaPeriod);
                  }			
                  break;
                }
                case 1:	
                {
                  if (keyrot == 0)
                  {
                    Omega = MonteCarlo(OmegaA, OmegaB);
                  }
                  else
                  {    
                    Period = MonteCarlo(PeriodA, PeriodB);
                  }    			
                  break;
                }
                default:
                {
                  fprintf(LogFilePtr,"ERROR: No Law! Correct option -v (Values 0, 1)\n");
                  exit(-1);
                  break;
                }
              }
            }    	

            /* Calculate the amplitude rotating pr pulse magnetic field */
            switch(keyrotampl) /* key for changing of amplitude of rotating field */
            {
              case 0:
              {
                /*			fprintf(LogFilePtr,"  Sinus Distribution \n"); 		*/
                if (keyrotampldir == 0)
                  FieldValue = FieldValueInit*sin(M_PI*(Pos[0]+PosMain[0])/depth);
                if (keyrotampldir == 1)
                  FieldValue = FieldValueInit*sin(M_PI*(Pos[1]+PosMain[1])/width);			
                if (keyrotampldir == 2)
                  FieldValue = FieldValueInit*sin(M_PI*(Pos[2]+PosMain[2])/height);			
                break;
              }
              case 1:	
              {
                /*   Permanent Distribution   */
                FieldValue = FieldValueInit;	
                break;
              }
              case 2:
              {
                fprintf(LogFilePtr,"ERROR: Solenoid Formula (not yet included), exit \n");
                exit(-1);
                break;
              }		    	
              default:
              {
                fprintf(LogFilePtr,"ERROR: No Law! Correct option -h (Values 0, 1, 2)\n");
                exit(-1);
                break;
              }
            }

            /* Perform Randomize the amplitude rotating or pulse magnetic fields, overload */
            FieldValueA = FieldValue - 0.01*FieldValueDevPer*fabs(FieldValue);
            FieldValueB = FieldValue + 0.01*FieldValueDevPer*fabs(FieldValue);
            SigmaField = 0.01*FieldValueDevPer*fabs(FieldValue);

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

            switch(keyguidech)  /* key for changing of guide field */		
            {
              case 0:
              {
                /*				fprintf(LogFilePtr,"  Cosinus Distribution \n"); */			
                if (keyguidechdir == 0)
                {
                  FieldValue0[0] = FieldValue0Init[0] + FieldValue0Grlin[0]*cos(M_PI*(Pos[0] + PosMain[0])/depth);			
                  FieldValue0[1] = FieldValue0Init[1] + FieldValue0Grlin[1]*cos(M_PI*(Pos[0] + PosMain[0])/depth);
                  FieldValue0[2] = FieldValue0Init[2] + FieldValue0Grlin[2]*cos(M_PI*(Pos[0] + PosMain[0])/depth);
                }	
                if (keyguidechdir == 1)
                {
                  FieldValue0[0] = FieldValue0Init[0] + FieldValue0Grlin[0]*cos(M_PI*(Pos[1] + PosMain[1])/width);			
                  FieldValue0[1] = FieldValue0Init[1] + FieldValue0Grlin[1]*cos(M_PI*(Pos[1] + PosMain[1])/width);
                  FieldValue0[2] = FieldValue0Init[2] + FieldValue0Grlin[2]*cos(M_PI*(Pos[1] + PosMain[1])/width);
                }	
                if (keyguidechdir == 2)
                {
                  FieldValue0[0] = FieldValue0Init[0] + FieldValue0Grlin[0]*cos(M_PI*(Pos[2] + PosMain[2])/height);			
                  FieldValue0[1] = FieldValue0Init[1] + FieldValue0Grlin[1]*cos(M_PI*(Pos[2] + PosMain[2])/height);
                  FieldValue0[2] = FieldValue0Init[2] + FieldValue0Grlin[2]*cos(M_PI*(Pos[2] + PosMain[2])/height);
                }	
                break;
              }
		
              case 1:	
              {
                if (keyguidechdir == 0)
                {
                FieldValue0[0] = FieldValue0Init[0] + (((Pos[0] + PosMain[0])*(FieldValue0Grlin[0] - FieldValue0Init[0]))/depth);
                FieldValue0[1] = FieldValue0Init[1] + (((Pos[0] + PosMain[0])*(FieldValue0Grlin[1] - FieldValue0Init[1]))/depth);
                FieldValue0[2] = FieldValue0Init[2] + (((Pos[0] + PosMain[0])*(FieldValue0Grlin[2] - FieldValue0Init[2]))/depth);
                }
                if (keyguidechdir == 1)
                {
                FieldValue0[0] = FieldValue0Init[0] + (((Pos[1] + PosMain[1])*(FieldValue0Grlin[0] - FieldValue0Init[0]))/width);
                FieldValue0[1] = FieldValue0Init[1] + (((Pos[1] + PosMain[1])*(FieldValue0Grlin[1] - FieldValue0Init[1]))/width);
                FieldValue0[2] = FieldValue0Init[2] + (((Pos[1] + PosMain[1])*(FieldValue0Grlin[2] - FieldValue0Init[2]))/width);
                }
                if (keyguidechdir == 2)
                {
                FieldValue0[0] = FieldValue0Init[0] + (((Pos[2] + PosMain[2])*(FieldValue0Grlin[0] - FieldValue0Init[0]))/height);
                FieldValue0[1] = FieldValue0Init[1] + (((Pos[2] + PosMain[2])*(FieldValue0Grlin[1] - FieldValue0Init[1]))/height);
                FieldValue0[2] = FieldValue0Init[2] + (((Pos[2] + PosMain[2])*(FieldValue0Grlin[2] - FieldValue0Init[2]))/height);
                }
                break;
              }
		    	
              case 2:
              {
                /*				fprintf(LogFilePtr,"  Permanent Distribution \n"); */
                FieldValue0[0] = FieldValue0Init[0];
                FieldValue0[1] = FieldValue0Init[1];
                FieldValue0[2] = FieldValue0Init[2];			
                break;
              }		    	

              default:
              {
                fprintf(LogFilePtr,"ERROR: No Law! Correct option -u (Values 0, 1, 2)\n");
                exit(-1);
              }
            }		

            /* Perform randomize of the gradient (or permanent) magentic field */
            if (FieldValue0Dev > 0.0)
            {
              // VLL = vector3rand(&VX, &VY, &VZ);
              gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);
              FieldValue0[0] = FieldValue0[0] + fabs(FieldValue0Dev)*VX;
              FieldValue0[1] = FieldValue0[1] + fabs(FieldValue0Dev)*VY;
              FieldValue0[2] = FieldValue0[2] + fabs(FieldValue0Dev)*VZ;
            }	
	    
            /* rotating or pulse field activated */
            if (keyrot == 0)
            {
              /* Normal rotating field */
              switch(keyaxis)
              {
                case 0:
                {
                  /* Choose rotation around axis 0X */
                  RR[0] = FieldValue0[0];
                  RR[1] = FieldValue0[1] + FieldValue*sin(Omega*(TimeR+TOFP) + phi0);
                  RR[2] = FieldValue0[2] + FieldValue*cos(Omega*(TimeR+TOFP) + phi0);
                  break;
                  }
                case 1:	
                {
                  /* Choose rotation around axis 0Y */
                  RR[0] = FieldValue0[0] + FieldValue*sin(Omega*(TimeR+TOFP) + phi0);
                  RR[1] = FieldValue0[1];
                  RR[2] = FieldValue0[2] + FieldValue*cos(Omega*(TimeR+TOFP) + phi0);	
                  break;
                  }
                case 2:
                {
                  /* Choose rotation around axis 0Z */
                  RR[0] = FieldValue0[0] + FieldValue*cos(Omega*(TimeR+TOFP) + phi0);	
                  RR[1] = FieldValue0[1] + FieldValue*sin(Omega*(TimeR+TOFP) + phi0);
                  RR[2] = FieldValue0[2];
                  break;
                }
                default:
                {
                  fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
                  exit(-1);
                  break;
                }
              }	
            }
            else
            {
              switch(keyaxis)
              {
                /* rectangular pulse field */
                case 0:
                {
                  /* field parallel of axis 0X */
                  RR[0] = FieldValue0[0] + RectangularF((TimeR+TOFP), FieldValue, Period);
                  RR[1] = FieldValue0[1];
                  RR[2] = FieldValue0[2];
                  break;
                }
                case 1:	
                {
                  /* field parallel  of axis 0Y */
                  RR[0] = FieldValue0[0];
                  RR[1] = FieldValue0[1] + RectangularF((TimeR+TOFP), FieldValue, Period);
                  RR[2] = FieldValue0[2];	
                  break;
                }
                case 2:
                {
                  /* field parallel of axis 0Z */
                  RR[0] = FieldValue0[0];	
                  RR[1] = FieldValue0[1];
                  RR[2] = FieldValue0[2] + RectangularF((TimeR+TOFP), FieldValue, Period); 
                  break;
                }
                default:
                {	
                  fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
                  exit(-1);
                  break;
                }
              }	
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
	    
          CartesianToEulerZY(RR, &Rroty, &Rrotz);
          domain_field[0] = LengthVector(RR);
          domain_field[1] = Rrotz;
          domain_field[2] = Rroty;
          //	    fprintf(LogFilePtr,"Field: roty = %f rotz = %f\n",Rroty,Rrotz);
    
          /* Rotating option */
          FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]); 

          /* translates into frame of the field domain */
          SubVector(Pos, PosDomain);

          /* calculate entrance end exit coordinates of domain*/
          { 
            VectorType pos, dir;	CopyVector(Pos, pos);	CopyVector(Dir, dir);
	
            /* gives intersection positions with domain */

            if (IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) 
              goto getlost; 

            if (wall_2 == 0) 
              goto getlost;

            /* ordering */
            if(Pos1[0] > Pos2[0]) 	
            { VectorType V;	int wall; CopyVector(Pos1, V);	CopyVector(Pos2, Pos1); CopyVector(V, Pos2); 	
		
              wall = wall_1; wall_1 = wall_2; wall_2 = wall;
            }
          }

          /* moment of arriving at the domain wall, new position */
          CopyVector(Pos1, Pos);

          /* time of precession in the domain field - precession calculated in the field frame */
          TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

          RotVector(RotMatrixField, SpinVector); 
	
          RotVector(RotMatrixField, RR1);

          PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]);
          PhaseShift0 = PhaseShift/(2.0*(M_PI));  
          NumberPrecessions = NumberPrecessions + PhaseShift0;
          Number_NOP = Number_NOP + 1.0;
	
          FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);
          RotVector    (LarmorMatrix,   SpinVector);
          RotBackVector(RotMatrixField, SpinVector);

          PolX[indp] = PolX[indp] + Prob*SpinVector[0];
          PolY[indp] = PolY[indp] + Prob*SpinVector[1];
          PolZ[indp] = PolZ[indp] + Prob*SpinVector[2];
          ProbM[indp] = ProbM[indp] + Prob;

          if (RRSM != 0.0)
          {
            RRSM = 1.0;	
            FldX[indp] = FldX[indp] + (RRS[0]/RRSM);
            FldY[indp] = FldY[indp] + (RRS[1]/RRSM);
            FldZ[indp] = FldZ[indp] + (RRS[2]/RRSM);
            FldM[indp] = FldM[indp] + 1.0;
          }	
	
          indp = indp + 1;

          /* moment of exiting at the domain wall, new position */
          TimeR = TimeR + TOF2;
          TOF += TOF2;

          CopyVector(Pos2, Pos);

          /* translates back into main frame */
          AddVector(Pos, PosDomain);

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
      exitfield:

        /*******************************************************************************/
        NumberPrecessionssum =  NumberPrecessionssum + NumberPrecessions;

        if (Number_NOP != 0.0)
        {
          NumberPrecessionsave =  NumberPrecessionsave + NumberPrecessions/Number_NOP;
        }

        /* Output matters */
        IntegralIntensity += Prob;
        NumOut++;

        //	fprintf(LogFilePtr,"BBBBB PRECESSION Pos before rota  X =  %f   Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);	        RotBackVector(RotMatrixMain, Pos ); 
        RotBackVector(RotMatrixMain, Dir ); 
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

        Neutrons.Time        = TOF;
        Neutrons.Wavelength  = WL;
        Neutrons.Probability = Prob;

        CopyVector(Pos, Neutrons.Position);
        CopyVector(Dir, Neutrons.Vector);
        CopyVector(SpinVector, Neutrons.Spin);

        /* writes output binary file */
        WriteNeutron(&Neutrons);

      getlost:;
      }
    }
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
my_exit:
  /* write to log file */
  if (NumOut != 0) 
  {
    fprintf(LogFilePtr,"Full Number of precessions  : %f\n", NumberPrecessions);
    if (Number_NOP != 0.0)
      fprintf(LogFilePtr,"Ave Number of precessions  : %f\n", NumberPrecessions/Number_NOP);    fprintf(LogFilePtr,"All Number of precessions  : %f\n", NumberPrecessionssum/NumOut);    fprintf(LogFilePtr,"Average number of precession per domains : %f\n", NumberPrecessionsave/NumOut);
	
    if (keysph == 1)	
    {
      for(ind_x=1; ind_x < indp; ind_x++) 
      {
        if (ProbM[ind_x] != 0.0)
          fprintf(fmonitp,"%f     %f     %f     %f  \n", PolX[ind_x]/ProbM[ind_x], 
                                                         PolY[ind_x]/ProbM[ind_x], 
                                                         PolZ[ind_x]/ProbM[ind_x],
                                                   sqrt((PolX[ind_x]/ProbM[ind_x])*(PolX[ind_x]/ProbM[ind_x]) + 
                                                        (PolY[ind_x]/ProbM[ind_x])*(PolY[ind_x]/ProbM[ind_x]) + 
                                                        (PolZ[ind_x]/ProbM[ind_x])*(PolZ[ind_x]/ProbM[ind_x])   ));
			
        if (FldM[ind_x] != 0.0)
          fprintf(fmonitf,"%f     %f     %f     %f  \n", FldX[ind_x]/FldM[ind_x], 
                                                         FldY[ind_x]/FldM[ind_x], 
                                                         FldZ[ind_x]/FldM[ind_x],
                                                   sqrt((FldX[ind_x]/FldM[ind_x])*(FldX[ind_x]/FldM[ind_x]) +  
                                                        (FldY[ind_x]/FldM[ind_x])*(FldY[ind_x]/FldM[ind_x]) + 
                                                        (FldZ[ind_x]/FldM[ind_x])*(FldZ[ind_x]/FldM[ind_x])  ) );
      }	
    }
  }
  else
  { 
    fprintf(LogFilePtr,"ERROR: No neutrons in the exit of the precession volume, exit!!!\n");	    
    exit(-1);
  }

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
  /* Variables for rotating and gradient (permanent) magnetic field */
  double FieldValue0Ave[3];            // variables for calculations 
  double FieldValue0Length=0.0, 
         OmegaFV0=0.0, OmegaFV0in=0.0; 
  int k;

  /* initial values */
  InitVector(PosMain);
  InitVector(TranslOut);

  for (k=0; k < 3; k++)
  { FieldValue0Init [k] = 0.0; 
    FieldValue0Grlin[k] = 0.0; 
    FieldValue0Ave  [k] = 0.0;
  }

  /* read parameter list */
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
				
      case 'i':
        sscanf(&argv[1][2], "%lf", &AnglMainHoriz);
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

      /* Input parameters for characterized rotaing magnetic field */
      case 'w':
        sscanf(&argv[1][2], "%lf", &omegainit);        /* number of rotate per second */
        break;
      case 'b':
        sscanf(&argv[1][2], "%lf", &OmegaDevPer);        /* deviation for frequency of rotating field in % */
        break;      case 'z':
        sscanf(&argv[1][2], "%lf", &phi0d);        /* begin phase, degree*/
        break;
      case 'd':
        sscanf(&argv[1][2], "%lf", &FieldValueInit);        /* magnetic filed, Oe = Gauss */
        break;
      case 'a':
        sscanf(&argv[1][2], "%lf", &FieldValueDevPer);        /* deviation for amplitude of rotating field in % */
        break;							
      case 'M':
        sscanf(&argv[1][2], "%d", &keyaxis);        /* Rotating field around axis 0x, 0y, 0z, Values 0,1,2 respectevly */
        break;
      case 'h':
        sscanf(&argv[1][2], "%d", &keyrotampl);     /* Key for law of changing of rotating field amplitude   */
        break;      case 'y':
        sscanf(&argv[1][2], "%d", &keyrotampldir); /* key for direction changing of amplitude of rotating field */
        break;      case 'e':
        sscanf(&argv[1][2], "%d", &DevLawAmpl);        /* Law of distribution of amplitute of rotating field: 0 - Normal(Default), 1 - Uniform */
        break;      case 'v':
        sscanf(&argv[1][2], "%d", &DevLawFreq);        /* Law of distribution of frequency of rotating field: 0 - Normal(Default), 1 - Uniform */
        break;      case 'n':
        sscanf(&argv[1][2], "%d", &keyphase);        /* 1 -  Neutron TOF from preceding modules is use for rotating field phase; 0 - No */
        break;
      case 'u':
        sscanf(&argv[1][2], "%d", &keyguidech);
        break;      case 't':
        sscanf(&argv[1][2], "%d", &keyguidechdir); /* key for direction changing of guide field */
        break;			
      case 'c':
        sscanf(&argv[1][2], "%d", &keyrot);        /* Option for activate rectangular pulse field, Prof. Drabkin */
        break;			
      case 'C':
        sscanf(&argv[1][2], "%ld", &ind_x_max);        /* Number of domains in X axis direction */
        break;
      case 'D':
        sscanf(&argv[1][2], "%ld", &ind_y_max);        /* Number of domains in Y axis direction */
        break;
      case 'E':
        sscanf(&argv[1][2], "%ld", &ind_z_max);        /* Number of domains in Z axis direction */
        break;
						
      /* Input parameters for gradient magnetic field */
      case 'I':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[0]);        /* Initial value in Oe, OX component */
        break;      case 'A':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[1]);        /* Initial value in Oe, OY component */
        break;
      case 'K':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[2]);        /* Initial value in Oe, OZ component */
        break;
      case 'P':
        sscanf(&argv[1][2], "%lf", &FieldValue0Grlin[0]);        /* Linear changing, Oe/cm, OX component */
        break;      case 'Q':
        sscanf(&argv[1][2], "%lf", &FieldValue0Grlin[1]);        /* Linear changing, Oe/cm, OY component */
        break;
      case 'R':
        sscanf(&argv[1][2], "%lf", &FieldValue0Grlin[2]);        /* Linear changing, Oe/cm, OZ component */
        break;			
      case 'q':
        sscanf(&argv[1][2], "%lf", &FieldValue0Dev);
        break;											
      case 'S':
        sscanf(&argv[1][2], "%d", &keysph);        /* key for output polarisation components in the file during flight of domens */
        break;
    }
    argc--;
    argv++;
  }

  /* Check initial data */
  if (keysph == 1)
  {
    fprintf(LogFilePtr,"Activate output in the file the components of polarisation \n");
	
    if (Monitp == NULL)
    {
      fprintf(LogFilePtr,"\n ERROR: You must define a MonitorOutputFile for output polarisation (Option -O)!"); 
      exit(-1);
    }
	    
    if (Monitf == NULL)
    {
      fprintf(LogFilePtr,"\n ERROR: You must define a MonitorOutputFile for output magnetic field (Option -N)!"); 
      exit(-1);
    }

    fmonitp = OpenOutputFile(Monitp, FALSE, "w");
    if (fmonitp==NULL)
      fprintf(LogFilePtr,"Warning: File %s could not be opened for output of polarisation components \n", Monitp);
	    
    fmonitf = OpenOutputFile(Monitf, FALSE, "w");
    if (fmonitf==NULL)
      fprintf(LogFilePtr,"Warning: File %s could not be opened for output of magnetic field \n", Monitf);
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

  if ((AnglMainHoriz >= 90.0)||(AnglMainHoriz <= -90.0))
  {
    fprintf(LogFilePtr,"ERROR: angle must be (-90..90) degree! Correct option -i \n"); 
    exit (-1);
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
    fprintf(LogFilePtr,"ERROR: Deviation of amplitude of rotating field must be >= 0 \n");
    exit(-1);
  }		
			
  if (OmegaDevPer < 0.0) 
  {
    fprintf(LogFilePtr,"ERROR: Deviation of frequency of rotating field  must be >= 0 \n");
    exit(-1);
  }	
		
  if (FieldValue0Dev < 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Amplitude of additional random magnetic field must be >= 0 \n");
    exit(-1);
  }
	
  /* Prepare values for deviation of the rotating(pulse) field frequency */
  if (keyrot == 0)	
  {
    /* Rotating field activated */
    /* Convert omegainit and phi*/
    OmegaInit = 2*(M_PI)*omegainit/1000.0;
    Omega = OmegaInit;
    OmegaA = Omega - 0.01*OmegaDevPer*fabs(Omega);
    OmegaB = Omega + 0.01*OmegaDevPer*fabs(Omega);
    SigmaOmega = 0.01*OmegaDevPer*fabs(Omega);
			
    phi0 = phi0d*(M_PI)/180.0;
		
    if (keyaxis == 0) 
      fprintf(LogFilePtr,"Rotating magnetic field activated around X axis H =  %f *sin( %f * Time +  %f )\n", FieldValueInit, Omega, phi0);
    if (keyaxis == 1) 
      fprintf(LogFilePtr,"Rotating magnetic field activated around Y axis H =  %f *sin( %f * Time +  %f )\n", FieldValueInit, Omega, phi0);    if (keyaxis == 2) 
      fprintf(LogFilePtr,"Rotating magnetic field activated around Z axis H =  %f *sin( %f * Time +  %f )\n", FieldValueInit, Omega, phi0);
  }
  else
  {
    /* Rectangular pulse field activated*/
			
    /* Convert omegainit */
    if (omegainit <= 0.0)
    {
      fprintf(LogFilePtr,"ERROR! For rectangular pulse field frequency must be > 0.0 (Option -w)\n");
      exit(-1);
    }
			
    PeriodInit = 1000.0/omegainit;    /* Period in ms */
    PeriodA = PeriodInit - 0.01*OmegaDevPer*fabs(PeriodInit);
    PeriodB = PeriodInit + 0.01*OmegaDevPer*fabs(PeriodInit);
    SigmaPeriod = 0.01*OmegaDevPer*fabs(PeriodInit);
			
    if (OmegaDevPer > 0.0) 	fprintf(LogFilePtr,"PERIOD_DEV: NORMAL DIST A =  %f  B =  %f ; GAUSS SIG =  %f \n",PeriodA, PeriodB, SigmaPeriod);
		
    if (keyaxis == 0) fprintf(LogFilePtr,"Rectangular pulse field parallel of Axis 0X with amplitude  %f  Oe and frequency  %f  Hz\n", FieldValueInit, omegainit);
    if (keyaxis == 1) fprintf(LogFilePtr,"Rectangular pulse field parallel of Axis 0Y with amplitude  %f  Oe and frequency  %f  Hz\n", FieldValueInit, omegainit);
    if (keyaxis == 2) fprintf(LogFilePtr,"Rectangular pulse field parallel of Axis 0Z with amplitude  %f  Oe and frequency  %f  Hz\n", FieldValueInit, omegainit);
  }		
	
  if (OmegaDevPer > 0.0) 
  {
    if (keyrot == 0)
    {	
      fprintf(LogFilePtr,"OMEGA_DEV: NORMAL DIST A =  %f  B =  %f ; GAUSS SIG =  %f \n",OmegaA, OmegaB, SigmaOmega);	
      fprintf(LogFilePtr,"Activate deviation of frequency of rotating field  %f  percent,   ", OmegaDevPer);			
    }
    else
    {
      fprintf(LogFilePtr,"PERIOD_DEV: NORMAL DIST A =  %f  B =  %f ; GAUSS SIG =  %f \n", PeriodA, PeriodB, SigmaPeriod);	
      fprintf(LogFilePtr,"Activate deviation of frequency of pulse field  %f  percent,   ", OmegaDevPer);		
    }
			
    switch(DevLawFreq)
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
	
  /* inform about imput data */
  if (keyrot == 0)
  {
    fprintf(LogFilePtr,"Amplitude of rotating field have a ");
  }
  else
  {	
    fprintf(LogFilePtr,"Amplitude of pulse field have a ");
  }			

  switch(keyrotampl) /* key for changing of amplitude of rotating field */
  {
    case 0:
    {
      fprintf(LogFilePtr,"  Sinus Distribution \n");
      fprintf(LogFilePtr,"Amplitude = %f Oe \n", FieldValueInit);
      if (keyrotampldir == 0) fprintf(LogFilePtr,"Sinus semi-period along axis X = %f cm \n", depth);
      if (keyrotampldir == 1) fprintf(LogFilePtr,"Sinus semi-period along axis Y = %f cm \n", width);				
      if (keyrotampldir == 2) fprintf(LogFilePtr,"Sinus semi-period along axis Z = %f cm \n", height);				
      break;
    }
		
    case 1:	
    {
      fprintf(LogFilePtr,"  Permanent Distribution \n");
      fprintf(LogFilePtr,"Value = %f Oe \n", FieldValueInit);		  
    break;
    }

    case 2:
    {
      fprintf(LogFilePtr,"ERROR: Solenoid Formula (not yet included), exit \n");
      exit(-1);
      break;
    }		    	

    default:
    {
      fprintf(LogFilePtr,"ERROR: No Law! Correct option -h (Values 0, 1, 2)\n");
      exit(-1);
      break;
    }
  }


  fprintf(LogFilePtr,"--------------------------------------------------\n");
  fprintf(LogFilePtr,"Guide field have a ");
	
  switch (keyguidech)  /* key for changing of guide field */		
  {
    case 0:
    {
      fprintf(LogFilePtr,"  Cosinus Distribution \n");
      fprintf(LogFilePtr,"Cosinus amplitude components  X = %f Oe,  Y = %f Oe,  Z = %f Oe \n", FieldValue0Grlin[0], FieldValue0Grlin[1], FieldValue0Grlin[2]); 
				
      if (keyguidechdir == 0) fprintf(LogFilePtr,"Cosinus semi-period along axis X = %f cm \n", depth);
      if (keyguidechdir == 1) fprintf(LogFilePtr,"Cosinus semi-period along axis Y = %f cm \n", width);				
      if (keyguidechdir == 2) fprintf(LogFilePtr,"Cosinus semi-period along axis Z = %f cm \n", height);				
				
      fprintf(LogFilePtr,"Plus additional permanent components  X = %f Oe,  Y = %f Oe,  Z = %f Oe \n", FieldValue0Init[0], FieldValue0Init[1], FieldValue0Init[2]);	
      FieldValue0Length = LengthVector(FieldValue0Init);
      fprintf(LogFilePtr,"Permanent magnetic field components module =  %f  \n",FieldValue0Length);
      OmegaFV0 = (FREQUENCY_FROM_FIELD(FieldValue0Length)); 
      OmegaFV0in = OmegaFV0*1000.0/(2.0*(M_PI));
      fprintf(LogFilePtr,"For RF flipper frequency of rotating magnetic field must be = %f Hz \n", OmegaFV0in);				
				
      break;
    }
		
    case 1:	
    {
      fprintf(LogFilePtr,"  Linear Distribution \n");
      fprintf(LogFilePtr,"Initial values components X = %f Oe,  Y = %f Oe,  Z = %f Oe \n", FieldValue0Init[0], FieldValue0Init[1], FieldValue0Init[2]);	
      fprintf(LogFilePtr,"Final values components  X = %f Oe,  Y = %f Oe,  Z = %f Oe \n", FieldValue0Grlin[0], FieldValue0Grlin[1], FieldValue0Grlin[2]); 	
				
      if (keyguidechdir == 0)  fprintf(LogFilePtr,"In lengths along axis:  X = %f cm \n", depth);				 
      if (keyguidechdir == 1)  fprintf(LogFilePtr,"In lengths along axis:  Y = %f cm \n", width);				 				
      if (keyguidechdir == 2)  fprintf(LogFilePtr,"In lengths along axis:  Z = %f cm \n", height);				 				
				
      FieldValue0Ave[0] = 0.5*(FieldValue0Init[0] + FieldValue0Grlin[0]);
      FieldValue0Ave[1] = 0.5*(FieldValue0Init[1] + FieldValue0Grlin[1]);
      FieldValue0Ave[2] = 0.5*(FieldValue0Init[2] + FieldValue0Grlin[2]);
      FieldValue0Length = LengthVector(FieldValue0Ave);
      fprintf(LogFilePtr,"Average magnetic field module =  %f  \n",FieldValue0Length);
      OmegaFV0 = (FREQUENCY_FROM_FIELD(FieldValue0Length)); 
      OmegaFV0in = OmegaFV0*1000.0/(2.0*(M_PI));
      fprintf(LogFilePtr,"NOTE for NRSE users! For RF flipper frequency of rotating magnetic field must be = %f Hz \n", OmegaFV0in);
      break;
    }
		    	
    case 2:
    {
      fprintf(LogFilePtr,"  Permanent Distribution \n");
      fprintf(LogFilePtr,"ONLY Permanent components components  X = %f Oe  Y = %f Oe  Z = %f Oe \n", FieldValue0Init[0], FieldValue0Init[1], FieldValue0Init[2]);					
      FieldValue0Length = LengthVector(FieldValue0Init);
      fprintf(LogFilePtr,"Permanent magnetic field module =  %f  \n",FieldValue0Length);
      OmegaFV0 = (FREQUENCY_FROM_FIELD(FieldValue0Length)); 
      OmegaFV0in = OmegaFV0*1000.0/(2.0*(M_PI));
      fprintf(LogFilePtr,"For RF flipper frequency of rotating magnetic field must be = %f Hz \n", OmegaFV0in);
      break;
    }		    	

    default:
    {
      fprintf(LogFilePtr,"ERROR: No Law! Correct option -u (Values 0, 1, 2)\n");
      exit(-1);
    }
  }		

  if (FieldValueDevPer > 0.0) 
  {
    fprintf(LogFilePtr,"Activate deviation of amplitude of rotating(pulse) field  %f  percent,  ", FieldValueDevPer);
			
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
      }
    }	
  }			    

  if (FieldValue0Dev > 0.0) 
    fprintf(LogFilePtr,"Activate additional RANDOM magnetic field with amplitude  %f  Oe \n", FieldValue0Dev);		
				
  if (keyphase == 1)
  {
    fprintf(LogFilePtr,"Neutron TOF from preceding modules is use for rotating field phase\n");
  }
  else
  {
    fprintf(LogFilePtr,"Neutron TOF from preceding modules is NOT use for rotating field phase\n");
  }	

  /* Convert from degree(GUI) to radian */
  AnglMainHoriz  = AnglMainHoriz*(M_PI)/180.0;

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


/******************************************/
/** Intersection with rectangular object **/
/******************************************/
long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
  VectorType	n, pos0, pos1, pos2, pos3, pos4, pos5;
  int			k;

  for (k=0; k<3; k++) 
    Pos1[k] = Pos2[k] = 0.0;

  n[0] = 1.0; n[1] = n[2] = 0.0; 
  *wall_1 = *wall_2 = 0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
  {
    if ((fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos0, Pos1); *wall_1 = 1;}
      else                           {CopyVector(pos0, Pos2); *wall_2 = 1;}
    }	
  } 

  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
  {
    if ((fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos1, Pos1); *wall_1 = 2;}
      else                           {CopyVector(pos1, Pos2); *wall_2 = 2;}
    }
  }

  n[1] = 1.0; n[2] = n[0] = 0.0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
  {
    if ((fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos2, Pos1); *wall_1 = 3;}
      else                           {CopyVector(pos2, Pos2); *wall_2 = 3;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
  {
    if ((fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2))
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos3, Pos1); *wall_1 = 4;}
      else                           {CopyVector(pos3, Pos2); *wall_2 = 4;}
    }
  }

  n[2] = 1.0; n[0] = n[1] = 0.0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
  {
    if ((fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos4, Pos1); *wall_1 = 5;}
      else                           {CopyVector(pos4, Pos2); *wall_2 = 5;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
  {
    if ((fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)) 
    {
      if (LengthVector(Pos1) == 0.0) {CopyVector(pos5, Pos1); *wall_1 = 6;}
      else                           {CopyVector(pos5, Pos2); *wall_2 = 6;}
    }
  }

  if ((LengthVector(Pos1) == 0.0) || (LengthVector(Pos2) == 0.0)) return 0;

	
  return 1;

}/* End IntersectionWithRectangularWallNumber() */
	    

/************************************************************************************************/
/** Function that describes rectangular pulses with period 'Period' and Amplitude 'FieldValue' **/
/************************************************************************************************/
double RectangularF(double Time, double FieldValue, double Period)
{	
/* Time - ms, FieldValue - Oe, Period - ms */

  double PerN;
  double TimeStrip;
  double Ampl=0.0;

  PerN = floor((Time)/(Period));

  TimeStrip = fabs(Time - Period*PerN);
  if ((TimeStrip >= 0.0)   &&(TimeStrip <= (0.5*Period))) Ampl = FieldValue; 
  if ((TimeStrip <= Period)&&(TimeStrip >  (0.5*Period))) Ampl = -1.0*FieldValue;		
		
  return Ampl;
}	


/********************************************************************************************************************************/
/** Function describing rectangular pulses with period 'Period' and Amplitude 'FieldValue' via Fourier transform - first order **/
/********************************************************************************************************************************/
double RectangularFTr(double Time, double FieldValue, double Period)
{	
  /* Time - ms, FieldValue - Oe, Period - ms */

  double Ampl=0.0;
	
  Ampl = FieldValue*sin((2*M_PI/Period)*Time);

  return Ampl;
}	



	    		


