/*********************************************************************************************/
/*  VITESS MODULE ELLIPTIC MIRROR                                                            */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* Written by Manoshin Sergey, start project May 2003 without GRAVITY                        */
/* Hahn-Meitner-Institut, Berlin:                                                            */
/*                                                                                           */
/* Brief history:                                                                            */
/* 1.00  Jun 2003  S. Manoshin    initial version                                            */
/* 1.22  Dec 2007  S. Manoshin    Add air attenuation of flux for travel before collimator   */
/* 1.23  Apr 2020  K. Lieutenant  tidy up, new central visualization parameters              */
/* 1.24  Nov 2023  K. Lieutenant  visualization and checks improved                          */
/*********************************************************************************************/

#include "softabort.h"
#include "mirror.h"
#include "matrix.h"


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
FILE  *refl_filelup=NULL;           //           pointer to file for spin up reflectivity  
FILE  *refl_fileldo=NULL;           //           pointer to file for spin down reflectivity

char  *ReflFileNamelup=NULL;        // -i        file containing spin up reflectivity
char  *ReflFileNameldo=NULL;        // -I        file containing spin up reflectivity

int    keyVis    =2,                // -y        flag: activation of visualization
       keyVisAll =0,                // -v        flag: visualisation of all neutrons (or only reflected)
       visoutput =0,                // -l        enum: output device for visualization (0: x-Windows  1: ps file)
       vistype   =0,                // -Y        enum: visualisation plane (0: XZ  1: XY  2: YZ)
       keyfluxair=0,                // -z        flag: activation of flux attenuation in air
       keyreflect=0,                // -R        flag: ideal reflection (instead of reflectivity from file)
       keypol    =0,                // -p        flag: use of polarization
       keyExcl  =0,                 // -u        flag: exclusive counts (only reflected neutrons are written to the output)
       qspin     =0,                // -V        enum: spin quantisation direction: 0 - axis X, 1 - axis Y, 2 - axis Z 
       keyrotaxis=0;                // -g        enum: axis of rotation of the mirror  (0: X  1: Y  2: Z)
double surfacerough=0.0;            // -q        maximal angle of the mirror waviness
double X_MIN=-100.0, X_MAX=100.0,   // -A -C     limits of the mirror x-direction
       Y_MIN=-100.0, Y_MAX=100.0,   // -D -E                          y-direction
       Z_MIN=-100.0, Z_MAX=100.0;   // -H -K                          z-direction
double halfaxis[3]={100.0,1.0,1.0}, // -a -b -c  half axes of the ellipsoid
       RotAngl    = 0.0,            // -Q        angle of rotation of the mirror
       humidity   =55.0;            // -r        Humidity of the air, in % 

VectorType PosMain ={0.0,0.0,0.0},  // -d -e -k  center position of the elliptic mirror
           TransOut={0.0,0.0,0.0};  // -s -t -w  position of the output frame

// Variables determined from input parameters or trajectory data
double Xmin=0.0, Xmax=0.0,          //           ranges relative to center
       Ymin=0.0, Ymax=0.0,      
       Zmin=0.0, Zmax=0.0,      
       mu1, mu2,                    //           additional variable for air attenuation calculations
       rdatalup[1000],              //           reflectvitiy table for spin-up neutrons
       rdataldo[1000];              //           reflectvitiy table for spin-down neutrons
double RotMatMirr  [3][3];          //           Rotation matrix corresponding to axis and angle of mirror rotation
MirrorSecond MyMirror;              //           mirror data obtained from input parameters

#ifdef VT_GRAPH
  int do_visualise; /* default : no visualisation */
#endif

/******************************/
/** Prototypes               **/
/******************************/ 
void  OwnInit(int argc, char *argv[]);          // reads input parameters and sets global parameters
void  CalcAndWritePar();                        // calculates arrays from input parameters and writes to log file
void  SetGeometry(char* sColor);                // fills the structure stGeometry for visualization   missing !!!
void  CalcEdges(VectorType A, VectorType B, // calculates edges and center points of the mirror from the given ranges 
                VectorType C, VectorType D,
                VectorType Y, VectorType Z);

/******************************/
/**      MAIN Program        **/
/******************************/
int main(int argc, char *argv[])
{
  long	i=0;
  int   reflp=0;

  double TimeOF1=0.0,
         RotMatrixOut[3][3];          //  [-]      rotation matrix to tranform into the output frame (not used here)

  VectorType   Pos, Dir;
  Neutron      Output;

  // initialisation
  // --------------
  _eModule=MCN_MIRROR_ELLI;

  InitRotMatrix(RotMatrixOut);
  InitNeutron  (&Output);

	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.24");
	OwnInit(argc, argv);

  bVisInstalled = TRUE;    // needs to be done still
  if (bVisInstr) 
    bBlowUp = TRUE;

  CalcAndWritePar();

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()) 
  {
    for (i=0; i < NumNeutGot; i++)
    {
      CHECK
	
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        TimeOF1 = 0.0;
        reflp = 0;
	
        //	InputNeutrons[i].Vector[0] = (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

        /* Check the quantization of polarization */
        if (keypol == 1)
          if (fabs(InputNeutrons[i].Spin[qspin]) != 1.0)
          {	
            fprintf(LogFilePtr,"WARNING! Illegal Spin Quantisation!!! Check the Spin value\n");
            exit(-1);
          }

        /******************************************************************************************/
        /* Pass a pointer to the neutron and the Mirror structure variable to a subroutine to do  */
        /* the donkey work. The return value is the total value of the time of flight through the */
        /* Mirror, or -1.0 if it missed all plates and the exit (should be impossible).           */
        /******************************************************************************************/
        InputNeutrons[i].Position[0] = InputNeutrons[i].Position[0] - PosMain[0];
        InputNeutrons[i].Position[1] = InputNeutrons[i].Position[1] - PosMain[1];
        InputNeutrons[i].Position[2] = InputNeutrons[i].Position[2] - PosMain[2];

        /* Choose the behavior of neutrons between channels */

        /* Neutrons are travels WITHOUT crosstalk between channels */
        TimeOF1 = PathThroughMirrorGravOrder2(&InputNeutrons[i], MyMirror, Xmin, Xmax, Ymin, Ymax, Zmin, Zmax, halfaxis, PosMain,
                                              wei_min, rdatalup, rdataldo, surfacerough, keygrav, keypol,
                                              qspin, vistype, keyVisAll, keyreflect, &reflp, RotMatMirr, keyfluxair, mu1, mu2);

        if(TimeOF1 == -1.0)  continue;

        InputNeutrons[i].Position[0] = InputNeutrons[i].Position[0] + PosMain[0];
        InputNeutrons[i].Position[1] = InputNeutrons[i].Position[1] + PosMain[1];
        InputNeutrons[i].Position[2] = InputNeutrons[i].Position[2] + PosMain[2];	

        /****************************************************************************************/
        /* Transform the coordinates.   					         	      */
        /* X must be always renormalized to zero...                                                  */
        /****************************************************************************************/
        Output = InputNeutrons[i];

        Pos[0] = InputNeutrons[i].Position[0];
        Pos[1] = InputNeutrons[i].Position[1];
        Pos[2] = InputNeutrons[i].Position[2];

        Dir[0] = InputNeutrons[i].Vector[0];
        Dir[1] = InputNeutrons[i].Vector[1];
        Dir[2] = InputNeutrons[i].Vector[2];

        /* computes neutron variables in the output frame */
        SubVector(Pos, TransOut);
	
        Output.Position[0] = Pos[0];
        Output.Position[1] = Pos[1];
        Output.Position[2] = Pos[2];	
	
        Output.Vector[0] = Dir[0];
        Output.Vector[1] = Dir[1];
        Output.Vector[2] = Dir[2];	

        /*      fprintf(LogFilePtr,"Out x = %f  y = %f  z = %f\n",Output.Position[0], Output.Position[1], Output.Position[2]); */

        /****************************************************************************************/
        /* Add the time needed to travel inside Mirror.                                   */
        /****************************************************************************************/
        Output.Time = Output.Time + TimeOF1;

        /****************************************************************************************/
        /* Count this as a success.                                                             */
        /****************************************************************************************/
        if (keyExcl == 0 || reflp == 1) 
        {  
          WriteNeutron(&Output);

          /* point of exit for trajectory visualization */
          WriteScatIAP(&Output, VT_EXITED, RotMatrixOut, TransOut);
        }
      }
    }
  }

// Finish: write geometry and instrument file, free memory
// -------------------------------------------------------
 my_exit:

#ifdef VT_GRAPH
  if (do_visualise)
    {
      /* Close graphic window */
      cpgclos();
    }
#endif

  /* write geometry file */
  SetGeometry("green");

  /* Do the general cleanup */
  Cleanup(TransOut[0], TransOut[1], TransOut[2], 0.0, 0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i;

  for(i=1; i<argc; i++) 
  {
    char *a, *arg;

    a = argv[i];
    if (*a != '-') continue;
    arg = a + 2;
    switch(a[1]) 
    {
      case 'i':
        refl_filelup = OpenInputFile2((ReflFileNamelup = arg), "reflectivity data for spin-up neutrons", "r");
        break;
      case 'I':
        refl_fileldo = OpenInputFile2((ReflFileNameldo = arg), "reflectivity data for spin-down neutrons", "r");
        break;

      case 'y':
        keyVis = atoi(arg);      /* for visualization activation */
        #ifdef VT_GRAPH
        do_visualise = keyVis != 0;
        #endif
        break;
      case 'v':
        keyVisAll = atoi(arg);   /* for visualization, all visualisation or partial */
        break;
      case 'Y':
        vistype = atoi(arg);     /* for visualization, choose axis */
        break;
      case 'l':
        visoutput = atoi(arg);   /* for visualiztion, choose output device */
        break;

      case 'z':
        keyfluxair = atoi(arg);
        break;
      case 'R':
        keyreflect = atoi(arg); /* ideal reflection or via file*/
        break;    	
      case 'p':
        keypol = atoi(arg);     /* using(1) or no(0) polarization */
        break;    	
      case 'u':
        keyExcl = atoi(arg);    /* exculisve counter */
        break;
      case 'V':
        qspin  =  atoi(arg);  /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
        break;	
      case 'g':
        keyrotaxis = atoi(arg);   /* rotate around axis */
        break;

      case 'a':
        halfaxis[0]  =  atof(arg); /* Half axis a */
        break;
      case 'b':
        halfaxis[1]  =  atof(arg); /* Half axis b */
        break;
      case 'c':
        halfaxis[2]  =  atof(arg); /* Half axis c */
        break;

      case 'Q':
        RotAngl  =  atof(arg); /* rotate mirror */
        RotAngl  =  RotAngl*(M_PI)/180.0;
        break;
            	  	 	
      case 'q':
        surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
        surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
        surfacerough  =  tan(surfacerough);
        break;

      case 'r':
        humidity  =  atof(arg);
        break;

      case 's':
        TransOut[0]  =  atof(arg); /* TransOut 0 */
        break;     		
      case 't':
        TransOut[1]  =  atof(arg); /* TransOut 1 */
        break;     		
      case 'w':
        TransOut[2]  =  atof(arg); /* TransOut 2 */
        break;     		

      case 'd':
        PosMain[0]  =  atof(arg); /* Posmain0 */
        break;     		
      case 'e':
        PosMain[1]  =  atof(arg); /* Posmain1 */
        break;
      case 'k':
        PosMain[2]  =  atof(arg); /* Posmain2 */
        break;

      case 'A':
        X_MIN  = atof(arg); /*  */
        break;
      case 'C':
        X_MAX  = atof(arg); /*  */
        break;
      case 'D':
        Y_MIN  = atof(arg); /*  */
        break;
      case 'E':
        Y_MAX = atof(arg); /*  */
        break;
      case 'H':
        Z_MIN  = atof(arg); /*  */
        break;
      case 'K':
        Z_MAX = atof(arg); /*  */
        break;

      default:
        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n", a);
        exit(-1);
        break;
    }
  }
}


/********************************************************************/
/** Calculates arrays from input parameters and writes to log file **/
/********************************************************************/
void  CalcAndWritePar()
{
  long i, count;

  fprintf(LogFilePtr,"Halfaxis of ellipsoid   %f   %f   %f\n", halfaxis[0], halfaxis[1], halfaxis[2]);

  if (halfaxis[0] < 0.0)
    Error("halfaxis A of elliptic mirror <= 0.0!!! Correct option -a");
  if (halfaxis[0] == 0.0)
    Warning("Special case is activated: X variable is excluded from the ellipsoid equation!");
  if (halfaxis[1] <= 0.0)
    Error("halfaxis B of elliptic mirror <= 0.0!!! Correct option -b");
  if (halfaxis[2] <= 0.0)
    Error("halfaxis C of elliptic mirror <= 0.0!!! Correct option -c");
  if (halfaxis[0] > 0.0 && PosMain[0] >= halfaxis[0] )
    Error("calculation only works correctly, if the neutron is already inside the ellipsoid, when it enters this module");

  if (keyreflect == 0)
    fprintf(LogFilePtr,"The reflectivity of the mirror is choosen according refl. files\n");
  else
    fprintf(LogFilePtr,"The ideal reflectivity of mirror!\n");

  if(keyExcl == 0)
    fprintf(LogFilePtr,"All neutrons are propagated to the next module\n");
  else
    fprintf(LogFilePtr,"Only reflected neutrons are propagated to  the next module\n");

  if(keypol == 0)
    fprintf(LogFilePtr,"Polarization not treated, spin-up reflectivity for all neutrons\n");
  // else
  //  fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is enabled\n");

  if(surfacerough == 0.0)
    fprintf(LogFilePtr,"The reflecting surface is assumed to be perfectly flat\n");
  else
    fprintf(LogFilePtr,"The reflecting surface is rough\n");

  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e\n", wei_min);

  if (keyfluxair == 1)
  {
    fprintf(LogFilePtr, "Air attenuation is activated!\n");
    if ((humidity < 0.0)||(humidity > 100.0)) humidity = 55.0;
    fprintf(LogFilePtr, "Air Conditions: Temperature = 20 degree C, Himidity = %lf percent\n", humidity);
    humidity = humidity/100.0;
	
    /* Warning! Datas adapted for central Russia conditions */
    /* Nitrogen Oxygen N_2 and O_2, T=293K (20DegC) p=101.3kPa (1atm)  */
    /* Hydrogen H_2, Humidity = 55% - default, p=2.344kPa (20degC) */
	
    mu1 = 4.6137 + 0.4241 + (0.52302*humidity/0.55); /* scattring N_2, O_2, H_2 */
    mu2 = 0.42378 + 0.000011+ (0.00118*humidity/0.55);  /* absorption N_2, O_2, H_2 */	
  }
  else if (keyfluxair == 0)
  {
    fprintf(LogFilePtr,"No air attenuation\n");
  }
  else
  {
    fprintf(LogFilePtr,"ERROR: Key for activate attnuation in air wrong!\n");
    exit(-1);
  }

  /* initial initialization */
  for(i=0;i<1000; i++)
  {
    rdatalup[i] = 0.0;
    rdataldo[i] = 0.0;
  }

  /* Read reflectivity file for mirror, spin up */
  if (ReflFileNamelup !=NULL)
  {
    for(count=0; count<1000; count++)
    {
      if (fscanf(refl_filelup,"%lf",&rdatalup[count])==EOF)
        break;
    }
    fclose(refl_filelup);
  }
  else
  { fprintf(LogFilePtr,"case of no reflectivity of mirror, for spin up\n");
  }

  /* Read reflectivity file for mirror, spin down */
  if (ReflFileNameldo != NULL)
  {
    for(count=0; count<1000; count++)
    {
      if (fscanf(refl_fileldo,"%lf",&rdataldo[count])==EOF)
        break;
    }
    fclose(refl_fileldo);
  }
  else
  { fprintf(LogFilePtr,"case of no reflectivity of mirror, for spin down\n");
  }

  if (keyrotaxis == 0)
  {
    FillRotMatrixYX(RotMatMirr, RotAngl, 0.0);
  }
  if (keyrotaxis == 1)
  {
    FillRotMatrixZY(RotMatMirr, RotAngl, 0.0);
  }
  if (keyrotaxis == 2)
  {
    FillRotMatrixZY(RotMatMirr, 0.0, RotAngl) ;
  }

  Xmin = X_MIN - PosMain[0];  Xmax = X_MAX - PosMain[0];
  Ymin = Y_MIN - PosMain[1];  Ymax = Y_MAX - PosMain[1];
  Zmin = Z_MIN - PosMain[2];  Zmax = Z_MAX - PosMain[2];

  if (halfaxis[0] == 0.0)
  {
    MyMirror.Surf[0].A = 0.0; /* special case */
  }
  else
  {
    MyMirror.Surf[0].A = 1.0/(halfaxis[0]*halfaxis[0]);
  }
  MyMirror.Surf[0].B = 0.0;
  MyMirror.Surf[0].C = 1.0/(halfaxis[1]*halfaxis[1]);
  MyMirror.Surf[0].D = 0.0;
  MyMirror.Surf[0].E = 1.0/(halfaxis[2]*halfaxis[2]);
  MyMirror.Surf[0].F = 0.0;
  MyMirror.Surf[0].W = -1.0;
  MyMirror.Surf[0].P = 0.0;
  MyMirror.Surf[0].Q = 0.0;
  MyMirror.Surf[0].R = 0.0;

  /*  Exit surface  */
  MyMirror.Surf[1].A = 0.0;
  MyMirror.Surf[1].B = 1.0;
  MyMirror.Surf[1].C = 0.0;
  MyMirror.Surf[1].D = 0.0;
  MyMirror.Surf[1].E = 0.0;
  MyMirror.Surf[1].F = 0.0;
  MyMirror.Surf[1].W = -1.0*(TransOut[0] - PosMain[0]);
  MyMirror.Surf[1].P = 0.0;
  MyMirror.Surf[1].Q = 0.0;
  MyMirror.Surf[1].R = 0.0;

#ifdef VT_GRAPH
  if (do_visualise)
  {
    /* MF: visualisation for Windows and generation of file for the picture */
    /* SM: GraphDev are /xs or /cps for pgplot 		*/

#ifdef DO_WIN32
    char *GraphDev = "ellipticmirror.ps";
#else
    char *GraphDev;
    if (visoutput == 0)
    {
      GraphDev = "/xs";
    }
    else
    {		
      GraphDev = "/cps";
    }	
#endif

    /* initialize pgplot with devics  /xs  for Linux */
    if (cpgopen(GraphDev) < 1)
    {
      fprintf(LogFilePtr,"I cannot open plot device\n");
      exit(-1);
    }
    fprintf(LogFilePtr,"Open graphic device %s\n", GraphDev);

    if (vistype==0)
   	{
      cpgenv(0.0, 1.5*TransOut[0], -2.0*halfaxis[2], 2.0*halfaxis[2], 0, 0);
      cpgsfs(2);
      cpgsch(1.2);
      cpglab("X, cm ", "Z, cm", "Elliptic mirror Visualisation");
   	}
	
    if (vistype==1)
   	{
      cpgenv(0.0, 1.5*TransOut[0], -2.0*halfaxis[1], 2.0*halfaxis[1], 0, 0);
      cpgsfs(2);
      cpgsch(1.2);
      cpglab("X, cm ", "Y, cm", "Elliptic mirror Visualisation");
    }    	
	
    if (vistype==2)
   	{
      cpgenv(-2.0*halfaxis[1], 2.0*halfaxis[1], -2.0*halfaxis[2],2.0*halfaxis[2], 0, 0);
      cpgsfs(2);
      cpgsch(1.2);
      cpglab("Y, cm ", "Z, cm", "Elliptic mirror Visualisation");
    }    	

    if ((vistype!=0)&&(vistype!=1)&&(vistype!=2))
    {
      fprintf(LogFilePtr,"No such type of visualisation!!!\n");
      exit(-1);
    }
  }
#endif

  /*  Protect agains illegal value qspin must be  0 or 1 or 2 ONLY */
  if (qspin < 0 || qspin > 2) qspin = 0;

  if (qspin == 0)
    fprintf(LogFilePtr,"Magnetic field direction - AXIS OX\n");

  if (qspin == 1)
    fprintf(LogFilePtr,"Magnetic field direction - AXIS OY\n");

  if (qspin == 2)
    fprintf(LogFilePtr,"Magnetic field direction - AXIS OZ\n");

  if(keygrav == 1)
    fprintf(LogFilePtr,"Elliptic mirror: Gravity is enabled\n");
  else
    fprintf(LogFilePtr,"Elliptic mirror: Gravity is disabled\n");
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{ 
  /* As there is currently no possibiilty to visualize a radial slice of an ellipsid,
     the visualization is approached by a cylinder slice              */

  VectorType A={0,0,0}, B={0,0,0},   //        edges of the mirror
             C={0,0,0}, D={0,0,0},
             Y={0,0,0}, Z={0,0,0};   //        center of the mirror on connection A-B and on the surface (Z) 
  double     alpha=0.0,              // [rad]  hor. deviation of the direction to the mirror center from y axis
             beta=0.0,               // [rad]  vert. deviation of the direction to the mirror center from y axis
             zeta=0.0,               // [rad]  half axis of the cylinder slice
             x   =0.0,               // [ ]    cos(zeta)
             l   =0.0,               // [cm]   half length of the mirror (straight line)      
             d   =0.0,               // [cm]   distance between straight line and mirror surface      
             Rcyl=0.0,               // [cm]   radius of the cylinder
             Hcyl=0.0;               // [cm]   height of the cylinder

  // calculate points on the mirror defining the 4 edges 
  CalcEdges(A, B, C, D, Y, Z);         

  // determine height, radius and half opening zeta of the cylinder slice, and the axis orientation
  // and the deviation from y axis
  l = DistVector(A,B)/2.0;
  d = DistVector(Y,Z);
  x = l*l/(l*l+d*d) - sqrt(sq(l*l/(l*l+d*d)) - (l*l-d*d)/(l*l+d*d));
  zeta = acos(x);
  Rcyl = l/sin(zeta);
  Hcyl = DistVector(A,C);
   
  alpha = asin(Z[0]/Rcyl);      // atan((A[1]-B[1])/(A[0]-B[0]));
  beta  = asin(Z[2]/Rcyl);      // atan((A[2]-C[2])/(A[0]-C[0]));

  sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
  stGeometry.pDescr  =  sVisDescrpt;
  stGeometry.eModule = _eModule;

  stGeometry.nCylSlices = 1; 
  stGeometry.pCylSlice  = (VtCylSlice*) calloc(stGeometry.nCylSlices, sizeof(VtCylSlice));
	      
  stGeometry.pCylSlice[0].Radius     = BlowUp * Rcyl; 
  stGeometry.pCylSlice[0].Width      = BlowUp * Rcyl*2.0*zeta;
  stGeometry.pCylSlice[0].Height     = BlowUp * Hcyl;
  stGeometry.pCylSlice[0].vCntr[0]   = PosMain[0]+Z[0]-Rcyl*sin(alpha);
  stGeometry.pCylSlice[0].vCntr[1]   = PosMain[1]+Z[1]-Rcyl*cos(alpha);
  stGeometry.pCylSlice[0].vCntr[2]   = PosMain[2]+Z[2]-Rcyl*sin(beta);
  stGeometry.pCylSlice[0].vSymAxis[0]= 0.0;
  stGeometry.pCylSlice[0].vSymAxis[1]= sin(beta);
  stGeometry.pCylSlice[0].vSymAxis[2]= cos(beta);
  stGeometry.pCylSlice[0].OpenAngle  = 2.0*Degrees(zeta);
  stGeometry.pCylSlice[0].Phi        = 90.0 + Degrees(alpha);

  RotBackVector(RotMatMirr, stGeometry.pCylSlice[0].vSymAxis);

  return;
}


/***********************************************************/
/** Calculates edges of the mirror from the given ranges  **/
/***********************************************************/
void CalcEdges(VectorType A, VectorType B, VectorType C, VectorType D, VectorType Y, VectorType Z)
{
  double Ry_fwd=0.0, Ry_bak=0.0,  // radii of the ellipse cut at x = Xmin and x = Xmax 
         Rz_fwd=0.0, Rz_bak=0.0,
         a = halfaxis[0],
         b = halfaxis[1],
         c = halfaxis[2]; 
    
  Ry_fwd = b * sqrt(1.0 - sq(Xmax/a));
  Ry_bak = b * sqrt(1.0 - sq(Xmin/a));
  Rz_fwd = c * sqrt(1.0 - sq(Xmax/a));
  Rz_bak = c * sqrt(1.0 - sq(Xmin/a));

  // front end 
  A[0] = Xmax;                                                                              // cut ellipsoid at max. x-value
  C[0] = Xmax;
  if      (Ymin > 0.0) {A[2] = Zmax;  A[1] =  b * sqrt(1.0 - sq(A[0]/a) - sq(A[2]/c));     // if hor. range is complete on the left side, cut ellipsoid here and calculate z-positions 
                        C[2] = Zmin;  C[1] =  b * sqrt(1.0 - sq(C[0]/a) - sq(C[2]/c));}
  else if (Ymax < 0.0) {A[2] = Zmax;  A[1] = -b * sqrt(1.0 - sq(A[0]/a) - sq(A[2]/c));     // if hor. range is complete on the right side, cut ellipsoid here and calculate z-positions 
                        C[2] = Zmin;  C[1] = -b * sqrt(1.0 - sq(C[0]/a) - sq(C[2]/c));}

  else if (Zmin > 0.0) {A[1] = Ymax;  A[2] =  c * sqrt(1.0 - sq(A[0]/a) - sq(A[1]/b));     // if vert range is complete on the upper side, cut ellipsoid here and calculate z-positions 
                        C[1] = Ymin;  C[2] =  c * sqrt(1.0 - sq(C[0]/a) - sq(C[1]/b));}
  else if (Zmax < 0.0) {A[1] = Ymax;  A[2] = -c * sqrt(1.0 - sq(A[0]/a) - sq(A[1]/b));     // if hor. range is complete on the left side, cut ellipsoid here and calculate z-positions 
                        C[1] = Ymin;  C[2] = -c * sqrt(1.0 - sq(C[0]/a) - sq(C[1]/b));}

  else                 {A[1] = 0.0;   A[2] =  c * sqrt(1.0 - sq(A[0]/a) - sq(A[1]/b));      // otherwise assume vertical cut in the middle
                        C[1] = 0.0;   C[2] = -c * sqrt(1.0 - sq(C[0]/a) - sq(C[1]/b));}

  // back end 
  B[0] = Xmin;                                                                              // cut ellipsoid at min. x-value   
  D[0] = Xmin;
  if      (Ymin > 0.0) {B[2] = Zmax;  B[1] =  b * sqrt(1.0 - sq(B[0]/a) - sq(B[2]/c));     // if hor. range is complete on the left side, cut ellipsoid here and calculate z-positions 
                        D[2] = Zmin;  D[1] =  b * sqrt(1.0 - sq(D[0]/a) - sq(D[2]/c));}
  else if (Ymax < 0.0) {B[2] = Zmax;  B[1] = -b * sqrt(1.0 - sq(B[0]/a) - sq(B[2]/c));     // if hor. range is complete on the right side, cut ellipsoid here and calculate z-positions 
                        D[2] = Zmin;  D[1] = -b * sqrt(1.0 - sq(D[0]/a) - sq(D[2]/c));}

  else if (Zmin > 0.0) {B[1] = Ymax;  B[2] =  c * sqrt(1.0 - sq(B[0]/a) - sq(B[1]/b));     // if vert range is complete on the upper side, cut ellipsoid here and calculate z-positions 
                        D[1] = Ymin;  D[2] =  c * sqrt(1.0 - sq(D[0]/a) - sq(D[1]/b));}
  else if (Zmax < 0.0) {B[1] = Ymax;  B[2] = -c * sqrt(1.0 - sq(B[0]/a) - sq(B[1]/b));     // if hor. range is complete on the left side, cut ellipsoid here and calculate z-positions 
                        D[1] = Ymin;  D[2] = -c * sqrt(1.0 - sq(D[0]/a) - sq(D[1]/b));}

  else                 {B[1] = 0.0;   B[2] =  c * sqrt(1.0 - sq(B[0]/a) - sq(B[1]/b));      // otherwise assume vertical cut in the middle
                        D[1] = 0.0;   D[2] = -c * sqrt(1.0 - sq(D[0]/a) - sq(D[1]/b));}

  // determine center on the ABCD plane (Y) and on the mirror surface
  Y[0] = 0.25*(A[0] + B[0] + C[0] + D[0]);
  Y[1] = 0.25*(A[1] + B[1] + C[1] + D[1]);
  Y[2] = 0.25*(A[2] + B[2] + C[2] + D[2]);

  Z[0] = Y[0];
  if (Ymin > 0.0 || Ymax < 0.0)
  { Z[2] = Y[2];
    if (Z[2] < 0.0)
      Z[1] = -b * sqrt(1.0 - sq(Z[0]/a) - sq(Z[2]/c));
    else
      Z[1] =  b * sqrt(1.0 - sq(Z[0]/a) - sq(Z[2]/c));
  }
  else
  { Z[1] = Y[1];
    if (Z[1] < 0.0)
      Z[2] = -c * sqrt(1.0 - sq(Z[0]/a) - sq(Z[1]/c));
    else
      Z[2] =  c * sqrt(1.0 - sq(Z[0]/a) - sq(Z[1]/c));
  }

  return;
}
