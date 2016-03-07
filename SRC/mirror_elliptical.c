/********************************************************************************************/
/*  VITESS MODULE ELLIPTIC MIRROR				                                            */
/*											                                                */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* Written by Manoshin Sergey, start project May 2003 without GRAVITY                       */
/* Hahn-Meitner-Institut, Berlin:                                           			    */
/*											                                                */
/* Brief history:									                                        */
/* 1.00  Jun 2003  S. Manoshin    initial version                                           */
/* 1.22  Dec 2007  S. Manoshin    Add air attenuation of flux for travel before collimator  */
/********************************************************************************************/

#include <string.h>

#ifdef VT_GRAPH
# include "cpgplot.h"
int do_visualise; /* default : no visualisation */

#endif

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"

/****************************************/
/** Structures (changed in March 2002) **/
/****************************************/

/* Please REMEMBER: Structure definition also exists
   in the external functions!!! IF YOU CHANGED HERE, PLEASE CHANDE IN
   OTHER FUNCTIONS! */


/* Structure for describe mirror */

typedef struct
{
  SurfaceSecond Surf[2];
}
  MirrorSecond;


/******************************/
/** Inline Functions         **/
/******************************/

FILE * openNFile (char *name) {return fileOpen (name, "r");}

double  PathThroughMirrorGravOrder2(Neutron *, MirrorSecond, double, double, double, double, double, double, double halfaxis[3], VectorType,
				    double, double reflectivitylup[1000],
				    double reflectivityldo[1000], double, long, long, long, int, long, long, int *,
				    double RotMatrixMirror[3][3], long, double, double);


/**************************************/
/** Prototypes of external functions **/
/**************************************/



/******************************/
/**      MAIN Program        **/
/******************************/

int main(int argc, char *argv[])
{

  long	i, count, keypol=0, keyreflect=0;
  long	BufferIndex;
  long  qspin = 0; /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
  long visall=0; /* variable for visualisations */

  int keyvisoutput=0, vistype=0; /* variable for visualisation */
  int reflp, keyreflp=0;
  int keyrotaxis=0; /* choose the axis of rotation of mirror */

  double RotAngl=0.0; /* choose the angle of rotation */
  double rdatalup[1000];
  double rdataldo[1000];
  double TimeOF1;
  double surfacerough=0.0;  /* The parameter which characterized mirror surface rought */
  double ytemp=2.0;
  double halfaxis[3];
  double X_MIN=-100.0, X_MAX=100.0, Y_MIN=-100.0, Y_MAX=100.0, Z_MIN=-100.0, Z_MAX=100.0;
  double RotMatrixMirror[3][3];

  long  keyfluxair=0;   /* Key to activate (1) the attenuation of the flux at air    */
  static double mu1, mu2; /* additional variable for air attenuation calculations */
  double himidity=55.0; /* Himidity of the air, in % */


  VectorType PosMain, TransOut, Pos, Dir;

  char	 *ReflFileNamelup=NULL;
  char 	 *ReflFileNameldo=NULL;

  MirrorSecond MyMirror;
  Neutron  Output;

  FILE	*refl_filelup=NULL; /* file for describing mirror surface, spin up */
  FILE  *refl_fileldo=NULL; /* file for describing mirror surface, spin down */


  /*set by default */

  BufferIndex=0;
  surfacerough=0.0;
  halfaxis[0]=1.0;
  halfaxis[1]=1.0;
  halfaxis[2]=1.0;
  vistype=0;
  visall=0;
  keyreflp=0;
  keyvisoutput=0;
  keyreflect=0;
  RotAngl=0.0;
  keyrotaxis=0;


  /*input*/
  Init(argc, argv, VT_ELMIRROR);

  for(i=1; i<argc; i++) {
    char *a, *arg;
    a = argv[i];
    if (*a != '-') continue;
    arg = a + 2;
    switch(a[1]) {

	
    case 'i':
      refl_filelup = openNFile((ReflFileNamelup = arg));
      break;
		
    case 'I':
      refl_fileldo = openNFile((ReflFileNameldo = arg));
      break;

    case 'R':
      keyreflect = atol(arg); /* ideal reflection or via file*/
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

    case 'r':
      himidity  =  atof(arg);
      break;

    case 'z':
      keyfluxair = atol(arg);
      break;


      /*    case 'W':
	    break;

	    case 'S':
	    break;            */

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

    case 'p':
      keypol = atol(arg); /* using(1) or no(0) polarization */
      break;    	

    case 'y':
      ytemp = atof(arg);   /* for visualiztion activation */
#ifdef VT_GRAPH
      do_visualise = ytemp != 0;
#endif
      break;

    case 'u':
      keyreflp = atoi(arg);   /* exculisve counter */
      break;

    case 'g':
      keyrotaxis = atoi(arg);   /* rotate around axis */
      break;

    case 'Y':
      vistype = atol(arg);   /* for visualiztion, choose axis */
      break;

    case 'l':
      keyvisoutput = atol(arg);   /* for visualiztion, choose output device */
      break;

    case 'v':
      visall = atol(arg);   /* for visualiztion, all visualisation or partial */
      break;
            	  	 	
    case 'q':
      surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
      surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
      surfacerough  =  tan(surfacerough);
      break;
		
    case 'V':
      qspin  =  atol(arg);  /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
      break;	

	
    default:
      fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n", a);
      exit(-1);
      break;
    }
  }


  /* Open surface for reading */


  print_module_name("Elliptic Mirror 1.22");


  fprintf(LogFilePtr,"Halfaxis of ellipsoid   %f   %f   %f\n",halfaxis[0], halfaxis[1], halfaxis[2]);

  if (halfaxis[0] < 0.0)
    {
      fprintf(LogFilePtr,"Error: halfaxis A of elliptic mirror <= 0.0!!! Correct option -a\n");
      exit(-1);
    }

  if (halfaxis[0] == 0.0)
    {
      fprintf(LogFilePtr,"Warning! Special case is activated: X variable is excluded from the ellipsoid equation!\n");
    }
	
  if (halfaxis[1] <= 0.0)
    {
      fprintf(LogFilePtr,"Error: halfaxis B of elliptic mirror <= 0.0!!! Correct option -b\n");
      exit(-1);
    }	
	
  if (halfaxis[2] <= 0.0)
    {
      fprintf(LogFilePtr,"Error: halfaxis C of elliptic mirror <= 0.0!!! Correct option -c\n");
      exit(-1);
    }	



  if(keyreflect == 0)
    {
      fprintf(LogFilePtr,"The reflectivity of the mirror is choosen according refl. files\n");
    }
  else
    {
      fprintf(LogFilePtr,"The ideal reflectivity of mirror!\n");
    }


  if(keyreflp == 0)
    {
      fprintf(LogFilePtr,"All neutrons are going later in the next module\n");
    }
  else
    {
      fprintf(LogFilePtr,"Only reflected neutrons are going later in the next module\n");
    }


  if(keypol == 1)
    {
      fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is enabled\n");
    }
  else
    {
      fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is disabled\n");
    }

  if(surfacerough == 0.0)
    {
      fprintf(LogFilePtr,"The reflected surface is an ideal mirror\n");
    }
  else
    {
      fprintf(LogFilePtr,"The reflected surface is rough\n");
    }

  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e\n", wei_min);

  /* air block 1 */
  if ((keyfluxair < 0)||(keyfluxair > 1))
    {
      fprintf(LogFilePtr,"ERROR: Key for activate attnuation in air wrong!\n");
      exit(-1);
    }
  if (keyfluxair == 1)
    {
      fprintf(LogFilePtr, "Air attenuation is activated!\n");
      if ((himidity < 0.0)||(himidity > 100.0)) himidity = 55.0;
      fprintf(LogFilePtr, "Air Conditions: Temperature = 20 degree C, Himidity = %lf percent\n", himidity);
      himidity = himidity/100.0;
	
      /* Warning! Datas adapted for central Russia conditions */
      /* Nitrogen Oxygen N_2 and O_2, T=293K (20DegC) p=101.3kPa (1atm)  */
      /* Hydrogen H_2, Himidity = 55% - default, p=2.344kPa (20degC) */					
	
      mu1 = 4.6137 + 0.4241 + (0.52302*himidity/0.55); /* scattring N_2, O_2, H_2 */
      mu2 = 0.42378 + 0.000011+ (0.00118*himidity/0.55);  /* absorption N_2, O_2, H_2 */	
    }
  if (keyfluxair == 0)
    {
      fprintf(LogFilePtr,"No air attenuation\n");
    }
  /* end air block 1 */


  /* Read  data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, a.s.o., deg,  */
  /* reference wavelength 1 A */

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
    fprintf(LogFilePtr,"case of no reflectivity of mirror, for spin up\n");


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
    fprintf(LogFilePtr,"case of no reflectivity of mirror, for spin down\n");


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
      if (keyvisoutput == 0)
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
    {
      fprintf(LogFilePtr,"Elliptic mirror: Gravity is enabled\n");
    }
  else
    {
      fprintf(LogFilePtr,"Elliptic mirror: Gravity is disabled\n");
    }

  DECLARE_ABORT

    while(ReadNeutrons()) {
      for(i=0; i<NumNeutGot; i++) {

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
					      qspin, vistype, visall, keyreflect, &reflp, RotMatrixMirror, keyfluxair, mu1, mu2);

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
	if (keyreflp == 0)
	  {
	    WriteNeutron(&Output);
	  }
	else
	  {
	    if (reflp == 1) WriteNeutron(&Output);
	  }
      }
    }

 my_exit:

#ifdef VT_GRAPH
  if (do_visualise)
    {
      /* Close graphic window */
      cpgclos();
    }
#endif

  Cleanup(TransOut[0], TransOut[1], TransOut[2], 0.0, 0.0);


  return(0);
}


/*
  CopyVector(InputNeutrons[i].Position, Pos) ;
  CopyVector(InputNeutrons[i].Vector, Dir) ;
	
  SubVector(Pos, PosMain) ;
  RotVector(RotMatrixMain, Pos ) ;
  RotVector(RotMatrixMain, Dir ) ; 		

  RotBackVector(RotMatrixMain, Pos ) ;
  RotBackVector(RotMatrixMain, Dir ) ;

  AddVector(Pos, PosMain) ;

*/
