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
/*********************************************************************************************/

#include "softabort.h"
#include "mirror.h"
#include "matrix.h"


/******************************/
/** Global Variables         **/
/******************************/
McCompID _eModule=MCN_MIRROR_ELLI;

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
double halfaxis[3]={1.0,1.0,1.0},   // -a -b -c  half axes of the ellipsoid
       RotAngl    = 0.0,            // -Q        angle of rotation of the mirror
       humidity   =55.0;            // -r        Humidity of the air, in % 

VectorType PosMain ={0.0,0.0,0.0},  // -d -e -k  center position of the elliptic mirror
           TransOut={0.0,0.0,0.0};  // -s -t -w  position of the output frame

double mu1, mu2,                    //           additional variable for air attenuation calculations
       rdatalup[1000],              //           reflectvitiy table for spin-up neutrons
       rdataldo[1000];              //           reflectvitiy table for spin-down neutrons
double RotMatrixMirror[3][3];       //           Rotation matrix corresponding to axis and angle of mirror rotation
MirrorSecond MyMirror;              //           mirror data obtained from input parameters


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and sets global parameters
void  CalcAndWritePar();                    // calculates arrays from input parameters and writes to log file
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization   missing !!!


/******************************/
/**      MAIN Program        **/
/******************************/
int main(int argc, char *argv[])
{
  long	i;
  int reflp;

  double TimeOF1;

  VectorType   Pos, Dir;
  Neutron      Output;

  // initialisation
  // --------------
	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.23");
	OwnInit(argc, argv);

  bVisInstalled = MISSING;    // needs to be done still
  if (bVisInstr) 
    bLengthCmpr = TRUE;

  CalcAndWritePar();

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while(ReadNeutrons()) 
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK
	
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
      TimeOF1 = PathThroughMirrorGravOrder2(&InputNeutrons[i], MyMirror, X_MIN, X_MAX, Y_MIN, Y_MAX, Z_MIN, Z_MAX, halfaxis, PosMain,
                                            wei_min, rdatalup, rdataldo, surfacerough, keygrav, keypol,
                                            qspin, vistype, keyVisAll, keyreflect, &reflp, RotMatrixMirror, keyfluxair, mu1, mu2);

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
      if (keyExcl == 0)
      {
        WriteNeutron(&Output);
      }
      else
      {
        if (reflp == 1) WriteNeutron(&Output);
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

  if (keyreflect == 0)
    fprintf(LogFilePtr,"The reflectivity of the mirror is choosen according refl. files\n");
  else
    fprintf(LogFilePtr,"The ideal reflectivity of mirror!\n");

  if(keyExcl == 0)
    fprintf(LogFilePtr,"All neutrons are going later in the next module\n");
  else
    fprintf(LogFilePtr,"Only reflected neutrons are going later in the next module\n");

  if(keypol == 1)
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is enabled\n");
  else
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is disabled\n");

  if(surfacerough == 0.0)
    fprintf(LogFilePtr,"The reflected surface is an ideal mirror\n");
  else
    fprintf(LogFilePtr,"The reflected surface is rough\n");

  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e\n", wei_min);

  if (keyfluxair == 1)
  {
    fprintf(LogFilePtr, "Air attenuation is activated!\n");
    if ((humidity < 0.0)||(humidity > 100.0)) humidity = 55.0;
    fprintf(LogFilePtr, "Air Conditions: Temperature = 20 degree C, Himidity = %lf percent\n", humidity);
    humidity = humidity/100.0;
	
    /* Warning! Datas adapted for central Russia conditions */
    /* Nitrogen Oxygen N_2 and O_2, T=293K (20DegC) p=101.3kPa (1atm)  */
    /* Hydrogen H_2, Himidity = 55% - default, p=2.344kPa (20degC) */					
	
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
    FillRotMatrixYX(RotMatrixMirror, RotAngl, 0.0);
  }
  if (keyrotaxis == 1)
  {
    FillRotMatrixZY(RotMatrixMirror, RotAngl, 0.0);
  }
  if (keyrotaxis == 2)
  {
    FillRotMatrixZY(RotMatrixMirror, 0.0, RotAngl) ;
  }

  X_MIN = X_MIN - PosMain[0];
  X_MAX = X_MAX - PosMain[0];
  Y_MIN = Y_MIN - PosMain[1];
  Y_MAX = Y_MAX - PosMain[1];
  Z_MIN = Z_MIN - PosMain[2];
  Z_MAX = Z_MAX - PosMain[2];

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
  return;
}