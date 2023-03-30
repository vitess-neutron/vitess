/******************************************************************************************************************************/
/* 0.8  Aug 2001  G. Zsigmond   Initial and original version precession_field                                                 */
/* 0.9  Jul 2002  S. Manoshin   Add possibility for simulating rotating magnetic field                                        */
/*                              and correct some mistakes  order  Alexander Ioffe                                             */
/* 1.0  Sep 2002  S. Manoshin   Add permanent magnetic field components in rotating field  (RF-flipper), order S. Klimko      */
/* 1.1  Oct 2002  S. Manoshin   Preparing and Realisation in the VITESS 2.3                                                   */
/* 1.2  Oct 2002  S. Manoshin   Some restructurisation, add possibility for simulation rectangular pulse field -              */
/*                              order G. Drabkin, protect against incorrect initial dates                                     */
/* 1.3  Nov 2002  S. Manoshin   Add output in file the polarisarion components during flight of domains,                      */ 
/*                              improve possibility for rotating of magnetic field volume	                                    */
/* 1.4  Dec 2002  S. Manoshin   Add possibility for simulations random magnetic field: rotating and permanent,                */
/*                              values and frequencyes are changing	 during simulations and rougth of surfaces, order A. Ioffe*/
/* 1.5  Jul 2003  S. Manoshin   Fixed some bugs, some parts was rewritten                                                     */
/* 1.6  Nov 2003  S. Manoshin   Add os field, pulse field was removed, saturation magnetic field was added.                   */
/*                        	    Distribution of amplitude of magnetic field is added	                                        */
/*                              Auto calculation of some parameters is added for RF-flipper and normal flipper                */
/*  	                          Improve the algorith of slice changing                                                        */
/* 1.7  Mar 2004  S. Manoshin   foil inclination -  bug solved, bootstrap option was added, some restructuration,             */
/*                              add function for field generation                                                             */
/* 1.8  May 2020  K. Lieutenant tidy up, new central visualization parameters                                                 */
/* 1.9  Mar 2023  K. Lieutenant correction and visualization                                                                  */
/******************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "rotating_field.h"


/******************************/
/** Global Variables         **/
/******************************/
/* input parameters */char       *Monitp=NULL,                // -O        [-]   output file for the polarization components  
           *Monitf=NULL;                // -N        [-]   output file for the magnetic field 
double      depth =100.0,               // -X        [cm]  x-component of the size of the homogeneous magnetic field 
            width =100.0,               // -Y        [cm]  y-component of the size of the homogeneous magnetic field 
            height=100.0;               // -V        [cm]  z-component of the size of the homogeneous magnetic field 
VectorType  PosMain,                    // -k -l -m  [cm]  centre of the magnetic field 
            TranslOut;                  // -p -r -s  [cm]  position of the new origin  (in the co-ordinate of the old origin)
double      AnglMainHoriz=0.0;          // -i       [deg]  horizontal rotation angle (about z axis) of the magnetic field
long        ind_x_max=2,                // -C        [-]   number of domains in X axis direction 
            ind_y_max=2,                // -D        [-]   number of domains in Y axis direction 
            ind_z_max=2;                // -E        [-]   number of domains in Z axis direction 
char       *Ampld=NULL;                 // -t        [-]   file for describing amplitude distribution of rmf 
double      FieldValueInit=0.0,         // -d        [Gs]  amplitude of the magnetic field
            FieldValueDevPer=0.0,       // -a        [%]   deviation of the amplitude of rotating (or oscillating) magnetic field in %!
            omegainit  =0.0,            // -w        [Hz]  number of rotations per second
            OmegaDevPer=0.0,            // -b        [%]   deviation of the frequency of the rotating field in % 
            phi0d      =0.0;            // -z       [deg]  initial phase
int         keyaxis   =0,               // -M        [-]   direction about which the field rotates: 0: x-axis, 1: y-axis, 2:z-axis
            DevLawAmpl=3,               // -e        [-]   distribution of the amplitude of the rotating field: 0 - Normal_ran, 1 - Uniform_ran, 2 - Normal, 3 - Uniform(default), 4 - from file
            DevLawFreq=2,               // -v        [-]   distribution of the frequency of the rotating field: 0 - Normal_ran, 1 - Uniform_ran, 2 - Uniform (default) 
            keyphase=1;                 // -n        [-]   neutron TOF from preceding modules is used for the rotating field phase: NO, YES
double      FieldValue0Init[3],         // -I -A -K  [Gs]  x-, y- and z-component  of the permanent magnetic field
            FieldValue0Dev=0.0,         // -q        [Gs]  amplitude of the additional random magnetic field  
            wavelen=20.0;               // -W       [Ang]  wavelength for calculation
int         keysph=0,                   // -S        [-]   flag: output of polarisation components during simulation: NO, YES
            keycalculate=0,             // -x        [-]   key for auto caluculation of some parameters, default zero 
            keybootstrap=0;             // -T        [-]   Use or do not use a bootstrap configuration

/* input parameters that are currently not used */double      FieldValueSat=1.0e99;       // -y        [Gs]  Saturation magnetic field, amplitude
int         keyrotos=0;                 // -c        [-]   flag: rotation or osciallation:  0: rotating  1:cos  2: sin

// Variables determined from input parameters or trajectory data
FILE*  fampld=NULL;                      //File to describe the amplitude distribution of the rotating magnetic field 
FILE*  fmonitf=NULL;                     // pointer to output file for  magnetic field
FILE*  fmonitp=NULL;                     // pointer to output file for polarization components
double FieldValue=0.0, Omega=0.0, 
       OmegaInit=0.0, phi0=0.0;         // Rotating magnetic field H=FieldValue*sin(Omega*t + phi0); Initial and current values 
long   ntfl=1;                          /* number of magnetic field values read from file */
double RotMatrixMain[3][3];
double PosD[FIELD_SIZE], FieldD[FIELD_SIZE], PFinput[FIELD_SIZE_2]; /* for input of rot. field distribution */
	
/* Variables for rotating(oscilating-os) and permanent magnetic field add Manoshin Sergey*/
double FieldValue0[3];     /* Additional permanent field, initial and current */
double OmegaFV0;          /* Internal variable */

/* For randomisation of amplitude and frequency of rotating (os) field */	
double FieldValueA=0.0, FieldValueB=0.0;  /* Internal variables */
double OmegaA=0.0,      OmegaB=0.0;       /* Internal variables */
double SigmaField=0.0, SigmaOmega=0.0;   /* Internal Variables */


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char **argv)
{
  double     Number_NOP=0.0;
  double     TOFP  =0.0;                   // Neutron TOF from preceding modules for synchro rotations
  long       NumOut=0;                     // number of successfully treated trajectories  
  long       ind_x=0, ind_y=0, ind_z=0,    // Number of domains in the X, Y and Z directions
             indp=1;  
  long       i=0, j=0, 
             wall_1=0, wall_2=0;
  double     TOF=0.0, TOF1=0.0, TOF2=0.0, TOF3=0.0, 
             WL=0.0, Prob=0.0, PhaseShift=0.0, PhaseShift0=0.0;
  double     nPrecTrj=0.0, nPrecDom=0, nPrecTot=0.0;
  double     PolX[FIELD_SIZE_FL], PolY[FIELD_SIZE_FL], PolZ[FIELD_SIZE_FL], ProbM[FIELD_SIZE_FL],
             FldX[FIELD_SIZE_FL], FldY[FIELD_SIZE_FL], FldZ[FIELD_SIZE_FL], FldM[FIELD_SIZE_FL]; /* For output spin and field during flight */
  long       code=0, keyexit=0;   
  double     RotMatrixField[3][3], LarmorMatrix[3][3];
  double     FieldValue00[3];
  double     X1=0.0, X2=0.0, Y1=0.0, Y2=0.0; /* For interpolation */
  VectorType Pos, Dir, SpinVector, 
             Pos1, Pos2, domain_field, 
             PosDomain, DimDomain;
  Neutron		 Neutrons;

  /* Local variable for rotation and saturation */
  VectorType RR, RR1, RRS;
  double TimeR=0.0, Rroty=0.0, Rrotz=0.0;
  double VX=0.0, VY=0.0, VZ=0.0;
  double F_MODULE=0.0, FieldCorr=1.0;
  int k;

  Neutron NeutronAdd1, NeutronAdd2;
  Plane EndPoint1, EndPoint2;

  /* initialisation */
  /* -------------- */
  _eModule=MCN_FIELD_ROT;
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.9");
  OwnInit(argc, argv);
  EvalInput();

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = FALSE;

  // local variables
  InitVector(Pos);  InitVector(Dir);  InitVector(SpinVector);
  InitVector(Pos1); InitVector(Pos2); InitVector(domain_field);
  InitVector(PosDomain);              InitVector(DimDomain);  
  InitVector(RR);   InitVector(RR1);  InitVector(RRS);

  Init3x3Matrix(RotMatrixField);
  Init3x3Matrix(LarmorMatrix);
  InitNeutron(&Neutrons); 
	
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

  for (k=0; k < 3 ; k++)
    FieldValue00[k]=0.0;

  DECLARE_ABORT;

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons()!=0)
  {
    for (i=0; i<NumNeutGot; i++)
    { 
      CHECK;	
      
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        /* here is what happens to the neutron */
        TOF = InputNeutrons[i].Time;
        WL = InputNeutrons[i].Wavelength;
        Prob = InputNeutrons[i].Probability;

        CopyVector(InputNeutrons[i].Position, Pos);
        CopyVector(InputNeutrons[i].Vector, Dir);
	
        /* Check incorrect neutrons */
        if ((Dir[0] <= 0.0)||(WL == 0.0)) goto getlost;
        if (TOF < 0.0) goto getlost;
        /* improve calculations, renormalize */
        InputNeutrons[i].Vector[0]	= (double) sqrt(fabs(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])));

        CopyVector(InputNeutrons[i].Spin, SpinVector); 

        /* translates into frame of the main field and rotates coordinates  */
        //		fprintf(LogFilePtr,"AAAAAAAA Pos before rot  X =  %f   Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);	
	
        /* Move neutron in the precession volume */
        NeutronAdd1.Position[0] = Pos[0];
        NeutronAdd1.Position[1] = Pos[1];
        NeutronAdd1.Position[2] = Pos[2];
	
        NeutronAdd1.Vector[0] = Dir[0];
        NeutronAdd1.Vector[1] = Dir[1];
        NeutronAdd1.Vector[2] = Dir[2];
	
        NeutronAdd1.Wavelength = WL;
	
        EndPoint1.A = cos(AnglMainHoriz);
        EndPoint1.B = sin(AnglMainHoriz);
        EndPoint1.C = 0.0;
        EndPoint1.D = 0.5*depth-PosMain[0]*cos(AnglMainHoriz);
	
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
        if((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost;
        ind_z = (long) floor(Pos[2] / DimDomain[2]) + 1 + ind_z_max/2;
        if((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost;
        ind_x = 1; 

        /******************** starts to scan ******************************/
        nPrecTrj = 0;
        Number_NOP = 0.0;
        TimeR = 0.0;
        indp = 1;
	
        RR[0] = 0.0;
        RR[1] = 0.0;
        RR[2] = 0.0;
	
        /* Use the previous TOF for rotating(os) field, synhro!, corrected */
        if (keyphase == 1)
          TOFP = InputNeutrons[i].Time + TOF1;
        else
          TOFP = 0.0;

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

          if ((ind_x == 1)||(wall_2 == 2)) /* if for changing slice */
          {
            /* Choose the law of amplitude distribution of rotating(os) magnetic field */
            switch(DevLawAmpl)
            {
              case 0:
              {
                /* Perform Gauss randomization the amplitude rotating or oscilating magnetic fields */		
                if (FieldValueDevPer > 0.0)
                  FieldValue = DistrGauss(FieldValueInit, SigmaField);
                break;
              }
              case 1:	
              {
                /* Perform Uniform randomization the amplitude rotating or oscilating magnetic fields */		
                if (FieldValueDevPer > 0.0)
                  FieldValue = MonteCarlo(FieldValueA, FieldValueB);
                break;
              }
              case 2:
              {
                /* Perform Normal distribution of amplitude */
                FieldValue = FieldValueInit*exp(Pos[0]*Pos[0]/(-2.0*SigmaField*SigmaField));
                break;
              }
              case 3:
              {
                /* Perform Uniform distribution of amplitude */
                FieldValue = FieldValueInit;
                break;
              }
              case 4:
              {
                /* Calculate distribution according datas, which was taken from file */
                code = 0;
			    
                for(j = 0; j < (ntfl-1); j++)
                {
                  //		     fprintf(LogFilePtr,"%ld  ", j);
                  if ((Pos[0] >= PosD[j])&&(Pos[0] <= PosD[j+1]))  
                  {
                    X1 = PosD[j];
                    X2 = PosD[j+1];
                    Y1 = FieldD[j];
                    Y2 = FieldD[j+1];
                    code = j;
                    break;
                  }
                }
    
                /* Check interval */
                if (!((Pos[0] >= PosD[code])&&(Pos[0] <= PosD[code+1])))
                {
                  fprintf(LogFilePtr,"ERROR: Current x position outside the precession volume. Exit!\n");
                  exit(-1);
                }	
    
                if (X1 == X2) 
                {
                  fprintf(LogFilePtr,"ERROR: Interpolation: Warning! Incorrect values for positions \n");
                  exit(-1);
                }	
 
                /* Linear interpolation */
                FieldValue = Y1 + ((Y2-Y1)*(Pos[0]-X1)/(X2-X1));
			    
                break;
              }
              default:
              {
                fprintf(LogFilePtr,"ERROR: No Law! Correct option -e \n");
                exit(-1);
                break;
              }
            }	

            switch(DevLawFreq)
            {
              case 0:
              {
                /* Perform Gauss randomization the frequency of rotating or oscilating magnetic fields */
                if (OmegaDevPer > 0.0)
                Omega = DistrGauss(OmegaInit, SigmaOmega);
                break;
              }
              case 1:	
              {
                /* Perform Uniform randomization the frequency of rotating or oscilating magnetic fields */
                if (OmegaDevPer > 0.0)
                Omega = MonteCarlo(OmegaA, OmegaB);
                break;
              }
              case 2:
              {
                /* Uniform distribution */
                Omega = OmegaInit;		
                break;
              }
              default:
              {
                fprintf(LogFilePtr,"ERROR: No Law! Correct option -v \n");
                exit(-1);
                break;
              }
            }
    
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

            /*	Calculate magnetic field components */        	
            /* 	Activate simulation of bootstrap  */	
            if (keybootstrap == 1)
            {
              if (ind_x <= (ind_x_max/2))
              {
                /* using initial frequency and permanent components */
                for(j=0;j<3;j++)
                {
                  FieldValue00[j] = FieldValue0[j];
                }
                keyexit = CalculateField(keyrotos, keyaxis, FieldValue, FieldValue00, Omega, TimeR, TOFP, phi0, RR);	
              }
              else
              {    
                /* make negative ! */
                for(j=0;j<3;j++)
                {
                  FieldValue00[j] = -1.0*FieldValue0[j];
                }	
                keyexit = CalculateField(keyrotos, keyaxis, FieldValue, FieldValue00, (-1.0*Omega), TimeR, TOFP, phi0, RR);	
              }    
            }    
            else
            {
              /* using initial frequency and permanent components */
              for(j=0;j<3;j++)
              {
                FieldValue00[j] = FieldValue0[j];
              }
              keyexit = CalculateField(keyrotos, keyaxis, FieldValue, FieldValue00, Omega, TimeR, TOFP, phi0, RR);	
            }

            if (keyexit == -1)  exit(-1);
          }/* end if for changing slice */
    		
          /* process field */	
          RRS[0] = RR[0];
          RRS[1] = RR[1];
          RRS[2] = RR[2];
	    
          RR1[0] = RR[0]; 
          RR1[1] = RR[1]; 
          RR1[2] = RR[2];
	    
          CartesianToEulerZY(RR, &Rroty, &Rrotz);

          /*	Saturation of amplitude of total magnetic field is activated */
          F_MODULE = LengthVector(RR);
 	    
          if (F_MODULE <= FieldValueSat)
          {
            domain_field[0] = F_MODULE;
            FieldCorr = 1.0;
          }
          else
          {
            domain_field[0] = FieldValueSat;
            if (F_MODULE != 0.0)
              FieldCorr = FieldValueSat/F_MODULE;
            else
              FieldCorr = 0.0;
          }	
	    
          domain_field[1] = Rrotz;
          domain_field[2] = Rroty;
    
          /* Rotating option */
          FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]); 

          /* translates into frame of the field domain */
          SubVector(Pos, PosDomain);

          /* calculate entrance end exit coordinates of domain*/
          { 
            VectorType pos, dir, V;	
            int wall;

            CopyVector(Pos, pos);	
            CopyVector(Dir, dir);
	
            /* gives intersection positions with domain */
            if (IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) goto getlost; 

            if (wall_2 == 0) goto getlost;

            /* ordering */
            if (Pos1[0] > Pos2[0]) 	
            { CopyVector(Pos1, V); CopyVector(Pos2, Pos1); CopyVector(V, Pos2); 	
		          wall = wall_1;       wall_1 = wall_2;        wall_2 = wall;
            }
          }

          /* moment of arriving at the domain wall, new position */
          CopyVector(Pos1, Pos);

          /* time of precession in the domain field - precession calculated in the field frame */
          TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

          RotVector(RotMatrixField, SpinVector); 
          RotVector(RotMatrixField, RR1);
          //	fprintf(LogFilePtr,"RR1  X = %f Y = %f  Z = %f\n",RR1[0],RR1[1],RR1[2]);

          PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]);
          PhaseShift0 = PhaseShift/(2.0*(M_PI));  
          nPrecTrj = nPrecTrj + PhaseShift0;
          Number_NOP = Number_NOP + 1.0;

          FillRotMatrixYX(LarmorMatrix, PhaseShift, 0);

          RotVector(LarmorMatrix, SpinVector);
          RotBackVector(RotMatrixField, SpinVector);

          PolX[indp] = PolX[indp] + Prob*SpinVector[0];
          PolY[indp] = PolY[indp] + Prob*SpinVector[1];
          PolZ[indp] = PolZ[indp] + Prob*SpinVector[2];
          ProbM[indp] = ProbM[indp] + Prob;
	
          FldX[indp] = FldX[indp] + (FieldCorr*RRS[0]);
          FldY[indp] = FldY[indp] + (FieldCorr*RRS[1]);
          FldZ[indp] = FldZ[indp] + (FieldCorr*RRS[2]);
          FldM[indp] = FldM[indp] + 1.0;
	
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

      exitfield:;
        /*******************************************************************************/
        nPrecTot =  nPrecTot + nPrecTrj;

        if (Number_NOP != 0.0)
          nPrecDom =  nPrecDom + nPrecTrj/Number_NOP;

        /* Output matters */
        NumOut++;

        RotBackVector(RotMatrixMain, Pos ); 
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
  }
   
  // Finish: write log, geometry and instrument file, free memory
  // ------------------------------------------------------------
  my_exit:
  /* write to log file */
  if (NumOut != 0) 
  { /*
    fprintf(LogFilePtr,"Full Number of precessions  : %f\n", nPrecTrj);
    if (Number_NOP != 0.0)
      fprintf(LogFilePtr,"Ave Number of precessions  : %f\n", nPrecTrj/Number_NOP); */    fprintf(LogFilePtr,"Average number of precession in resonator : %f\n",   nPrecTot/NumOut);    fprintf(LogFilePtr,"Average number of precession per domain   : %f\n\n", nPrecDom/NumOut);
	
    if (keysph == 1)	
    {
      for(ind_x=1; ind_x < indp; ind_x++) 
      {
        if (ProbM[ind_x] != 0.0)
        {
          fprintf(fmonitp,"%f     %f     %f     %f     %ld  \n", 
          PolX[ind_x]/ProbM[ind_x],
          PolY[ind_x]/ProbM[ind_x], 
          PolZ[ind_x]/ProbM[ind_x],
          sqrt((PolX[ind_x]/ProbM[ind_x])*(PolX[ind_x]/ProbM[ind_x]) + 
               (PolY[ind_x]/ProbM[ind_x])*(PolY[ind_x]/ProbM[ind_x]) + 
               (PolZ[ind_x]/ProbM[ind_x])*(PolZ[ind_x]/ProbM[ind_x])),
          ind_x);
        }
			
        if (FldM[ind_x] != 0.0)
        {
          fprintf(fmonitf,"%f     %f     %f     %f     %ld  \n", 
          FldX[ind_x]/FldM[ind_x], 
          FldY[ind_x]/FldM[ind_x], 
          FldZ[ind_x]/FldM[ind_x],
          sqrt((FldX[ind_x]/FldM[ind_x])*(FldX[ind_x]/FldM[ind_x]) +  
               (FldY[ind_x]/FldM[ind_x])*(FldY[ind_x]/FldM[ind_x]) + 
               (FldZ[ind_x]/FldM[ind_x])*(FldZ[ind_x]/FldM[ind_x])),
          ind_x);
        }	
      }	
    }
  }
  else
  { 
    fprintf(LogFilePtr,"ERROR: No neutrons in the exit of the precession volume, exit!!!\n");	    
    exit(-1);
  }

  /* write geometry file */
  SetGeometry("yellow");
  
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
  int j;
  /* INIT */
  InitVector(PosMain);
  InitVector(TranslOut);

  Init3x3Matrix(RotMatrixMain);
  
  /* permanent field components , projection in the axises OX, OY and OZ */
  /* current values */	
  FieldValue0[0] = 0.0; 
  FieldValue0[1] = 0.0;
  FieldValue0[2] = 0.0;
	
  /* initial values */
  FieldValue0Init[0] = 0.0; 
  FieldValue0Init[1] = 0.0;
  FieldValue0Init[2] = 0.0;	

  for (j=1; j<FIELD_SIZE; j++)
  {
    PosD   [j]=0.0;
    FieldD [j]=0.0;
    PFinput[j]=0.0;
  }

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

      /* Input parameters to characterize the rotating magnetic field */
      case 'w':
        sscanf(&argv[1][2], "%lf", &omegainit);     /* number of rotate per second */
        break;
			
      case 'b':
        sscanf(&argv[1][2], "%lf", &OmegaDevPer);  /* deviation for frequency of rotating field in % */
        break;
      case 'z':
        sscanf(&argv[1][2], "%lf", &phi0d);        /* begin phase, degree*/
        break;

      case 'd':
        sscanf(&argv[1][2], "%lf", &FieldValueInit);   /* magnetic filed, Gs = Gauss */
        break;
			
      case 'a':
        sscanf(&argv[1][2], "%lf", &FieldValueDevPer); /* deviation for amplitude of rotating field in % */
        break;			
			
/*    case 'c':
        sscanf(&argv[1][2], "%d", &keyrotos);   // Option for choosing rotating or oscillating field
        break;*/			
      case 'C':
        sscanf(&argv[1][2], "%ld", &ind_x_max);        /* Number of domains in X axis direction */
        break;
      case 'D':
        sscanf(&argv[1][2], "%ld", &ind_y_max);        /* Number of domains in Y axis direction */
        break;
      case 'E':
        sscanf(&argv[1][2], "%ld", &ind_z_max);        /* Number of domains in Z axis direction */
        break;

      case 'M':
        sscanf(&argv[1][2], "%d", &keyaxis);        /* Values 0,1,2 - Rotating field around axis 0x, 0y, 0z respectevly */
        break;
      case 'e':
        sscanf(&argv[1][2], "%d", &DevLawAmpl);        /* Law of distribution of amplitute of rotating field: 0 - Normal_ran, 1 - Uniform_ran, 2 - Normal, 3 - Uniform(default), 4 - from file */
        break;      case 'v':
        sscanf(&argv[1][2], "%d", &DevLawFreq);        /* Law of distribution of frequency of rotating field: 0 - Normal_ran, 1 - Uniform_ran, 2 - Uniform (default) */
        break;      case 'n':
        sscanf(&argv[1][2], "%d", &keyphase);        /* 1 -  Neutron TOF from preceding modules is use for rotating field phase; 0 - No */
        break;
						
      /* Input parameters for permanent magnetic field */
      case 'I':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[0]);        /* value in Gs, OX component */
        break;      case 'A':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[1]);        /* value in Gs, OY component */
        break;
      case 'K':
        sscanf(&argv[1][2], "%lf", &FieldValue0Init[2]);        /* value in Gs, OZ component */
        break;
      case 'q':
        sscanf(&argv[1][2], "%lf", &FieldValue0Dev);
        break;															
      case 'S':
        sscanf(&argv[1][2], "%d", &keysph);        /* key for output polarisation components in the file during flight of domens */
        break;
			
      case 'T':
        sscanf(&argv[1][2], "%d", &keybootstrap);
        break;			
/*      case 'y':
        sscanf(&argv[1][2], "%lf", &FieldValueSat); // Saturation magnetic field
        break;*/			
      case 't':
        Ampld=&argv[1][2]; /* file for describing amplitude distribution of rmf */
        break;			
			
      case 'x':
        sscanf(&argv[1][2], "%d", &keycalculate); /* key for auto caluculation of some parameters, default zero */
        break;			
      case 'W':
        sscanf(&argv[1][2], "%lf", &wavelen); /* Wavelength for caluculation */
        break;    }
    argc--;
    argv++;
  }
  return;
}


/********************************/
/** Analyses input parameters  **/
/********************************/
void EvalInput()
{
  long   i, k, count=1;
  long   fieldsize;
  double FieldValue0Length;

  /* Check initial dates */
  if (keysph == 1)
  {
    // fprintf(LogFilePtr,"Activate output in the file the components of polarisation \n");
	
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

  if ((AnglMainHoriz >= 90.0)||(AnglMainHoriz <= -90.0))
  {
    fprintf(LogFilePtr,"ERROR: angle must be (-90..90) degree! Correct option -i \n"); 
    exit (-1);
  }
		
  fieldsize = FIELD_SIZE;
	
  if ((ind_x_max <= 0)||(ind_x_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the X direction must be less than FIELD_SIZE=%ld and positiv! (Option -C) \n", fieldsize);
    exit(-1);
  }
	
  if ((ind_y_max <= 0)||(ind_y_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the Y direction must be less than FIELD_SIZE=%ld and positiv! (Option -D) \n", fieldsize);
    exit(-1);
  }
	
  if ((ind_z_max <= 0)||(ind_z_max >= FIELD_SIZE))
  {
    fprintf(LogFilePtr,"ERROR: Number of domains in the Z direction must be less than FIELD_SIZE=%ld and positiv! (Option -E) \n", fieldsize);
    exit(-1);
  }


  if (FieldValueSat <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Value of satutation magnetic field must be positive! Exit!\n");
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

  switch(keyrotos)
  {
    case 0: fprintf(LogFilePtr,"Rotating field activated \n"); break;
    case 1:	fprintf(LogFilePtr,"cos-OS field activated \n");   break;
    case 2:	fprintf(LogFilePtr,"sin-OS field activated \n");   break;
    default:Error("No field! Correct option -c (Values 0, 1, 2)\n");
  }	

  /* Overwrite center position and translation output*/
  if ((PosMain[0] == 0.0)&&(TranslOut[0] == 0.0))
  {
    fprintf(LogFilePtr,"Center position = 0.5 from depth of flipper \n");
    fprintf(LogFilePtr,"Transout X plane = depth of flipper \n");
    PosMain[0] = 0.5*depth	;	
    TranslOut[0] = depth;
  }

  /* define the permanent magnetic field */
  FieldValue0[0] = FieldValue0Init[0]; 
  FieldValue0[1] = FieldValue0Init[1]; 
  FieldValue0[2] = FieldValue0Init[2]; 
		
  /*  Output for RF-flipper, resonanse conditions  */	
  fprintf(LogFilePtr,"+++ Resonance and/or PI flipping conditions +++\n");

  if ((FieldValue0[0] != 0.0)||(FieldValue0[1] != 0.0)||(FieldValue0[2] != 0.0))
  {
    FieldValue0Length = LengthVector(FieldValue0);
    OmegaFV0 = (1000.0/(2.0*(M_PI)))*(FREQUENCY_FROM_FIELD(FieldValue0Length)); 
    fprintf(LogFilePtr, "NOTE: RF flipper frequency of rotating magnetic field has to be %7.1f Hz for a permanent magnetic field of %7.3f Gs\n", OmegaFV0, FieldValue0Length);
  }	

  OmegaFV0 = (2.0*(M_PI)*fabs(omegainit)/1000.0)/FREQ_B_RATIO;
  if (omegainit != 0.0)
  {
    fprintf(LogFilePtr, "NOTE: Permanent magnetic field of RF flipper has to be %7.3f Gs for a frequency %7.1f Hz!\n", OmegaFV0, omegainit);
  }

  if (wavelen <= 0.0)
    Error("Wavelength must be positive");
	
  OmegaFV0 = (M_PI)*395.60346/(wavelen*depth*FREQ_B_RATIO);
  fprintf(LogFilePtr,"Amplitude of rotating field must be %7.3f Gs for pi flipping and wavelength %6.3f Ang \n", OmegaFV0, wavelen);

  /*	end of calculation of resonance condiotion */	

  /* Overwrite amplitude of rotating(os) magnetic field */

  if (keycalculate == 1)
  {
    fprintf(LogFilePtr,"!!!! Amplitude of rotating(os) magnetic field is overwritten !!!!!\n");
    FieldValueInit = OmegaFV0;
  }

  /* define the rotating(os) magnetic field */	
  /* Prepeare values for deviation of the rotating(pulse) 
  field amplitude and frequency */
	    
  FieldValue = FieldValueInit;
  FieldValueA = FieldValueInit - 0.01*FieldValueDevPer*fabs(FieldValueInit);
  FieldValueB = FieldValueInit + 0.01*FieldValueDevPer*fabs(FieldValueInit);
	  
  SigmaField = 0.01*FieldValueDevPer*fabs(FieldValueInit);
	    
	    
	
  switch(DevLawAmpl)
  {
    case 0:
    {
      if (FieldValueDevPer > 0.0) 
      {
        fprintf(LogFilePtr,"Activate deviation of amplitude of rotating field  %f  percent,  ", FieldValueDevPer);
        fprintf(LogFilePtr,"Normal (Gauss) Distribution \n");
      }	
      break;
    }
		
    case 1:	
    {
      if (FieldValueDevPer > 0.0) 
      {
        fprintf(LogFilePtr,"Activate deviation of amplitude of rotating field  %f  percent,  ", FieldValueDevPer);	    
        fprintf(LogFilePtr,"Uniform Distribution \n");
      }	
      break;
    }
		    
    case 2:
    {
      fprintf(LogFilePtr,"Amplitude of magnetic field has a gauss distribution with sigma = %f Gs \n",SigmaField);
      break;
    }
		    
    case 3:
    {
      fprintf(LogFilePtr,"Amplitude of magnetic field is permanent \n");
      break;
    }
		    
    case 4:
    {
      fprintf(LogFilePtr,"Amplitude distribution is reading from file \n");
      /*	Amplitude distribution is reading from file 	*/    
    	
      /* Open magnetic field file */
    	fampld = OpenInputFile2(Ampld, "magnetic field along the flight path", "r");
 
      /* initialisation */
      for(i=0; i<FIELD_SIZE_2; i++)
      {	
        PFinput[i] = 0.0;
      }
  
      for(i=0; i<FIELD_SIZE; i++)
      {	
        PosD[i] = 0.0;
        FieldD[i] = 0.0;
      }
	    
      /* Read file of rot. field amplitude distribution */
      if (Ampld !=NULL)
      {
        for(count=0; count<FIELD_SIZE_2; count++)
        {
          if (fscanf(fampld,"%lf",&PFinput[count])==EOF)
          break;
        }

        fclose(fampld);
	
        k = 0;
        ntfl = (long)((count)/2); 
        fprintf(LogFilePtr,"File: Number of values = %ld  Number of rows = %ld \n",count, ntfl);
				    
        if (count == 0)
        {
          fprintf(LogFilePtr,"ERROR: No data in file, add at least two rows, EXIT!\n");
          exit(-1);
        }
				    
        if (count == 1)
        {
          fprintf(LogFilePtr,"ERROR: Only one value was found in file, EXIT\n");
          exit(-1);
        }
				    
        if (ntfl == 1)
        {
          fprintf(LogFilePtr,"ERROR: In file only one row was found, add at least one row, EXIT! \n");
          exit(-1);
        }
		    
        for(i = 0; i < ntfl; i++)
        {
          PosD[i] = PFinput[k];
          FieldD[i] = PFinput[k+1];
		
          if ((PosD[i] < -0.5*depth)||(PosD[i] > 0.5*depth))
          {
            fprintf(LogFilePtr,"ERROR: First column, value of out in precession volume was found, Correct the file! EXIT!\n");
            exit(-1);
          }
		
          if (FieldD[i] < 0.0)
          {
            fprintf(LogFilePtr,"ERROR: Non-Positive value of amplitude was found in second column. Correct file! EXIT!\n");
            exit(-1);
          }
          k = k + 2;
        }
	    
        /* check the input data */	    
        for(i = 0; i < (ntfl-1); i++)
        {
          if (PosD[i+1] <= PosD[i]) 
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in file. \n");
            fprintf(LogFilePtr,"The numbers in the position column must be increasing!!! \n");
            exit(-1);
          }    
        }	    
				    
        if (PosD[0] != -0.5*depth)
        {
          fprintf(LogFilePtr,"ERROR: Min position in the file=%f cm must be equal of the half of dimension of common field X= %f cm\n", PosD[0],(-0.5*depth));
          exit(-1);
        }
				    
        if (PosD[ntfl-1] != 0.5*depth)
        {
          fprintf(LogFilePtr,"ERROR: Max position in the file=%f cm must be equal of the half of dimension of common field X= %f cm\n",PosD[ntfl-1], (0.5*depth));
          exit(-1);
        }
      }		
      else
      {
        fprintf(LogFilePtr,"ERROR: No file, which describe the amplitude distribution. EXIT! \n");
        exit(-1);
      }	
	    
      break;
    }
     
    default:
    {
      fprintf(LogFilePtr,"ERROR: No Law! Correct option -e \n");
      exit(-1);
      break;
    }
  }	
			    
  /* Convert omegainit and phi*/
  OmegaInit = 2*(M_PI)*omegainit/1000.0;
  Omega = OmegaInit;
  OmegaA = OmegaInit - 0.01*OmegaDevPer*fabs(OmegaInit);
  OmegaB = OmegaInit + 0.01*OmegaDevPer*fabs(OmegaInit);
  SigmaOmega = 0.01*OmegaDevPer*fabs(OmegaInit);

			
  switch(DevLawFreq)
  {
    case 0:
    {
      if (OmegaDevPer > 0.0) 
      {
        fprintf(LogFilePtr,"Activate deviation of frequency of rotating field  %f  percent,   ", OmegaDevPer);
        fprintf(LogFilePtr,"Normal (Gauss) Distribution \n");
      }
      break;
    }
		
    case 1:	
    {
      if (OmegaDevPer > 0.0) 
      {
        fprintf(LogFilePtr,"Activate deviation of frequency of rotating field  %f  percent,   ", OmegaDevPer); 	
        fprintf(LogFilePtr,"Uniform Distribution \n");
      }
      break;
    }
			
    case 2:
    {
      fprintf(LogFilePtr,"Frequency of magnetic field is permanent \n");
      break;
    }

    default:
    {
      fprintf(LogFilePtr,"ERROR: No Law! Correct option -e \n");
      exit(-1);
      break;
    }
  }			    

  phi0 = phi0d*(M_PI)/180.0;

  switch(keyrotos)
  {
    case 0:
    {
      if (keyaxis == 0) 
        fprintf(LogFilePtr,"Rotating magnetic field about X axis: %7.3f(X,N) Gs * [0, cos(%7.1f(N) * time + %6.3f), sin(...)]\n", FieldValue, Omega, phi0);
      if (keyaxis == 1) 
        fprintf(LogFilePtr,"Rotating magnetic field about Y axis: %7.3f(X,N) Gs * [sin(%7.1f(N) * time + %6.3f), 0, cos(...)]\n", FieldValue, Omega, phi0);      if (keyaxis == 2) 
        fprintf(LogFilePtr,"Rotating magnetic field about Z axis: %7.3f(X,N) Gs * [cos(%7.1f(N) * time + %6.3f), sin(...), 0]\n", FieldValue, Omega, phi0);
      break;
    }
		
    case 1:	
    {
      if (keyaxis == 0) 
        fprintf(LogFilePtr,"COS-OS magnetic field parallel X axis: %7.3f(X,N) * cos(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);
      if (keyaxis == 1) 
        fprintf(LogFilePtr,"COS-OS magnetic field parallel Y axis: %7.3f(X,N) * cos(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);      if (keyaxis == 2) 
        fprintf(LogFilePtr,"COS-OS magnetic field parallel Z axis: %7.3f(X,N) * cos(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);
      break;
    }
		    
    case 2:	
    {
      if (keyaxis == 0) 
        fprintf(LogFilePtr,"SIN-OS magnetic field parallel X axis: %7.3f(X,N) * sin(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);
      if (keyaxis == 1) 
        fprintf(LogFilePtr,"SIN-OS magnetic field parallel Y axis: %7.3f(X,N) * sin(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);      if (keyaxis == 2) 
        fprintf(LogFilePtr,"SIN-OS magnetic field parallel Z axis: %7.3f(X,N) * sin(%7.1f(N) * time + %6.3f)\n", FieldValue, Omega, phi0);
      break;
    }

    default:
    {
      fprintf(LogFilePtr,"ERROR: No field! Correct option -c (Values 0, 1, 2)\n");
      exit(-1);
      break;
    }
  }	
			
	
  if ((FieldValue0[0] == 0.0)&&(FieldValue0[1] == 0.0)&&(FieldValue0[2] == 0.0))
  {
    if (FieldValue0Dev > 0.0) 
    {
      fprintf(LogFilePtr,"Rotating field plus random magnetic field! \n");
    }
    else
    {	
      fprintf(LogFilePtr,"ONLY rotating field! No additional permanent magnetic field! \n");
    }	
  }	


  if (FieldValue0Dev > 0.0) 
    fprintf(LogFilePtr,"Activate additional random magnetic field with amplitude  %f  Gs \n", FieldValue0Dev);	

  if (FieldValueSat != 1e99)
  {
    fprintf(LogFilePtr,"Module of saturation value =  %f Gs\n", FieldValueSat);
  }
	
  if (keybootstrap == 1)
  {
    fprintf(LogFilePtr,"---------------------------------------------------------------\n");
    fprintf(LogFilePtr,"*****************BOOTSTRAP*OPTION*IS*ACTIVATED**************** \n");
    fprintf(LogFilePtr,"  Precession volume is devided for two(2) parts. For the first \n");
    fprintf(LogFilePtr,"part frequency of a rotating field and all permanent components\n");
    fprintf(LogFilePtr,"   are chosen as input datas. For the second part all these    \n");
    fprintf(LogFilePtr,"       parameters are become negative (multiple by -1.0)       \n");
    fprintf(LogFilePtr,"Given Total Number of domains = %ld half = %ld \n", ind_x_max, (ind_x_max)/2);
    fprintf(LogFilePtr,"---------------------------------------------------------------\n");
	    /* Check total number of domains in the X direction */
    if ( (ind_x_max/2.0 - floor(ind_x_max/2.0)) > 0.0 ) 
    {	
      if ((ind_x_max+1) >= FIELD_SIZE)
      {
        ind_x_max = ind_x_max - 1; 
      }
      else
      {	
        ind_x_max = ind_x_max + 1; 
      }	
      fprintf(LogFilePtr,"\nWARNING: Number of domains in the X direction must be odd! New Set %ld. \n", ind_x_max);
      fprintf(LogFilePtr,"=======================================================================\n");
    }	
  }

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
	
  FillRotMatrixZY(RotMatrixMain, 0.0, AnglMainHoriz);

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  if (keysph == 1)    
  {
    fclose(fmonitp);
    fclose(fmonitf);
  }	

}/* End OwnCleanup */


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  /* Geometry data */
  if (bVisInstr)
  { 
    int        nX=1, nY=1, nZ=1,
               iX=0, iY=0, iZ=0, iM=0;
    VectorType vOrient={1.0,0.0,0.0}, vCntr={1.0,0.0,0.0};

    RotBackVector(RotMatrixMain, vOrient); 

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr   = sVisDescrpt;
    stGeometry.eModule  = _eModule;

    nX = ind_x_max; nY = ind_y_max; nZ = ind_z_max;     // all domains shown   
    stGeometry.nCuboids = nX * nY * nZ; 
    stGeometry.pCuboid  = calloc(stGeometry.nCuboids, sizeof(VtCuboid));

    for (iX=1; iX <= nX; iX++)
    { 
      for (iY=1; iY <= nY; iY++)
      { 
        for (iZ=1; iZ <= nZ; iZ++)
        { 
          vCntr[0]  = (2.0*iX-nX-1.0)/(2.0*nX)*depth;
          vCntr[1]  = (2.0*iY-nY-1.0)/(2.0*nY)*width ;
          vCntr[2]  = (2.0*iZ-nZ-1.0)/(2.0*nZ)*height;
          RotBackVector(RotMatrixMain, vCntr); 

          stGeometry.pCuboid[iM].Length    = depth/nX ; 
          stGeometry.pCuboid[iM].Width     = width/nY  * BlowUp;
          stGeometry.pCuboid[iM].Height    = height/nZ * BlowUp;
          stGeometry.pCuboid[iM].vCntr[0]  = PosMain[0] + vCntr[0];
          stGeometry.pCuboid[iM].vCntr[1]  =(PosMain[1] + vCntr[1]) * BlowUp;
          stGeometry.pCuboid[iM].vCntr[2]  =(PosMain[2] + vCntr[2]) * BlowUp;
          stGeometry.pCuboid[iM].vNormal[0]= vOrient[0];
          stGeometry.pCuboid[iM].vNormal[1]= vOrient[1];
          stGeometry.pCuboid[iM].vNormal[2]= vOrient[2];
          iM++;
        }
      }
    }
  }
}


/*********************************************************/
/** Generate common field rotating or oscillating field **/
/*********************************************************/
long	CalculateField(long l_keyrotos, long l_keyaxis, double l_FieldValue, double l_FieldValue0[3],
                     double l_Omega, double l_TimeR, double l_TOFP, double l_phi0, VectorType l_RR)	
{
  l_RR[0] = 0.0;
  l_RR[1] = 0.0;
  l_RR[2] = 0.0;

  switch(l_keyrotos)
  {
    case 0:
    {		
      //////////////////////////////////////////////////////////////////////	    
      /* Normal rotating field */
      switch(l_keyaxis)
      {
        case 0:
        {
          /* Choose rotation around axis 0X */
          l_RR[0] = l_FieldValue0[0];
          l_RR[1] = l_FieldValue0[1] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          l_RR[2] = l_FieldValue0[2] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          break;
        }
        case 1:	
        {
          /* Choose rotation around axis 0Y */
          l_RR[0] = l_FieldValue0[0] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          l_RR[1] = l_FieldValue0[1];
          l_RR[2] = l_FieldValue0[2] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          break;
        }
        case 2:
        {
          /* Choose rotation around axis 0Z */
          l_RR[0] = l_FieldValue0[0] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          l_RR[1] = l_FieldValue0[1] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          l_RR[2] = l_FieldValue0[2];
          break;
        }
        default:
        {
          fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
          return -1;
          break;
        }
      }	
      //////////////////////////////////////////////////////////////////////		    
      break;
    }

    case 1:
    {
      /////////////////////////////////////////////////////////////////////	    
      switch(l_keyaxis)
      {
        case 0:
        {
          /* Choose  parallel axis 0X */
          l_RR[0] = l_FieldValue0[0] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          l_RR[1] = l_FieldValue0[1];
          l_RR[2] = l_FieldValue0[2];
          break;
        }
        case 1:	
        {
          /* Choose parallel axis 0Y */
          l_RR[0] = l_FieldValue0[0];	
          l_RR[1] = l_FieldValue0[1] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          l_RR[2] = l_FieldValue0[2];	
          break;
        }
        case 2:
        {
          /* Choose parallel axis 0Z */
          l_RR[0] = l_FieldValue0[0];
          l_RR[1] = l_FieldValue0[1];
          l_RR[2] = l_FieldValue0[2] + l_FieldValue*cos(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          break;
        }
        default:
        {
          fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
          return -1;
          break;
        }
      }	
      ///////////////////////////////////////////////////////////////////////		    		    
			
      break;
    }	
		    
    case 2:
    {
      //////////////////////////////////////////////////////////////////////	    
      switch(l_keyaxis)
      {
        case 0:
        {
          /* Choose  parallel axis 0X */
          l_RR[0] = l_FieldValue0[0] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);
          l_RR[1] = l_FieldValue0[1];
          l_RR[2] = l_FieldValue0[2];
          break;
        }
        case 1:	
        {
          /* Choose parallel axis 0Y */
          l_RR[0] = l_FieldValue0[0];	
          l_RR[1] = l_FieldValue0[1] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          l_RR[2] = l_FieldValue0[2];	
          break;
        }
        case 2:
        {
          /* Choose parallel axis 0Z */
          l_RR[0] = l_FieldValue0[0];
          l_RR[1] = l_FieldValue0[1];
          l_RR[2] = l_FieldValue0[2] + l_FieldValue*sin(l_Omega*(l_TimeR+l_TOFP) + l_phi0);	
          break;
        }
        default:
        {
          fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
          return -1;
          break;
        }
      }	
      //////////////////////////////////////////////////////////////////////		    		    		    
		    
      break;
    }

    default:
    {
      fprintf(LogFilePtr,"ERROR: No such field, correct option -c: value 0,1,2 \n");
      return -1;
      break;
    }
  }    
	    
  return 1;    
}


/******************************************/
/** Intersection with rectangular object **/
/******************************************/
long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
  VectorType n, pos0, pos1, pos2, pos3, pos4, pos5;
  int        k=0;

  for (k=0; k<3; k++) Pos1[k] = Pos2[k] = 0.0;

  n[0] = 1.; n[1] = n[2] = 0.0; 
  *wall_1 = *wall_2 = 0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
  {
    if ((fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2) ) 
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos0, Pos1); *wall_1 = 1;}
      else                          {CopyVector(pos0, Pos2); *wall_2 = 1;}
    }	
  } 
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
  {
    if(  (fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2) )
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos1, Pos1); *wall_1 = 2;}
      else                          {CopyVector(pos1, Pos2); *wall_2 = 2;}
    }
  }

  n[1] = 1.; n[2] = n[0] = 0.0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
  {
    if( (fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2) )
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos2, Pos1); *wall_1 = 3;}
      else                          {CopyVector(pos2, Pos2); *wall_2 = 3;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
  {
    if( (fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2) )
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos3, Pos1); *wall_1 = 4;}
      else                          {CopyVector(pos3, Pos2); *wall_2 = 4;}
    }
  }

  n[2] = 1.; n[0] = n[1] = 0.0;

  if (PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
  {
    if( (fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)  ) 
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos4, Pos1); *wall_1 = 5;}
      else                          {CopyVector(pos4, Pos2); *wall_2 = 5;}
    }
  }
  if (PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
  {
    if( (fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)  ) 
    {
      if (LengthVector(Pos1) == 0.) {CopyVector(pos5, Pos1); *wall_1 = 6;}
      else                          {CopyVector(pos5, Pos2); *wall_2 = 6;}
    }
  }

  if ((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) 
    return 0;
	
  return 1;

}/* End IntersectionWithRectangularWallNumber() */
