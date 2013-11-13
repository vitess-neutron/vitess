/********************************************************************************************/
/*  VITESS MODULE BENDER (with support converging, diverging options                        */
/*  SURFACES IS SITUATED IN  XZ PLANE (VERTICAL SURFACES OF BENDER)                         */
/*  ALSO 3 EXTERNAL FUNCTIONS MUST BE INCLUDED 						    */
/*											    */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* Writied by Manoshin Sergey, start project Apr 2001 for include GRAVITY                   */
/* Hahn-Meitner-Institut, Berlin: manochine@hmi.de                   			    */
/* Fixed some major bugs... Manoshin Sergey 08.04.2002.                                     */
/* Add key -g for gravity off or on                                                         */
/* Add key -a for abutment error on or off                                                  */
/* surface, top and bottom planes of Bender:                                                */
/* first reflectivity file describe left plane of Bender  -i                                */
/* second reflectivity file describe right plane of Bender -I                               */
/* third reflectivity file describe top and bottom planes of Bender -j                      */
/* add key -r for simulation the rough reflecting surface                                   */
/* include polarization, magnetic field is vertical, direction up                           */
/*   											    */
/* Compile script for Linux:								    */
/* gcc -DVT_GRAPH -g -O3 -c bender.c init.c general.c intersection.c bendchtr.c bendtest.c  */
/*			    bendertr.c  						    */
/* g77 -DVT_GRAPH -g -O3 -o bend05v_Linux bender.o init.o general.o  bendchtr.o bendtest.o  */
/*			    bendertr.o intersection.o -L/usr/local/lib -lpgplot -lcpgplot   */
/*			    -L/usr/X11/lib -lX11  					    */
/* rm *.o                                                                                   */
/*											    */
/* Brief history:									    */
/* 1.00  Jun 2001  S. Manoshin    initial version                                           */
/* 1.01  Dec 2001  K. Lieutenant  new  : surface file generation, softabort, loss in bender */
/*                                impr.: graphics acceleration, new scale for the picture   */
/*                                corr.: transformation, calc. of bender width and step size*/
/*                                simpl: all radii identical                                */
/*                 M. Fromme      graphics for Windows                                      */
/* 1.02  Jan 2002  K. Lieutenant  reorganisation, radii centered                            */
/* 1.03  Mar 2002  S. Manoshin	  Feature: if radius of curvature zero => straight 	    */
/* 				  line instead circle for bender surfaces and axis.	    */
/*				  Also surfaces radius is negative possible		    */
/*				  New option: Spin quantisation (Sx,Sy,Sz) is defined by    */
/*				  user -V option; 0,1,2 - axis 0X, OY, OZ;		    */
/*				  Some reorganisation of data structure, fixed graphics bugs*/
/*				  Test part of program moved in separated function and	    */
/*				  included comparing test value with near zero	    	    */
/*				  New option: -t : 1 - test activated 0 - test disactivated */
/*				  Fixed bug in visualize part				    */
/* 1.1alpha Apr 2002 S. Manoshin  Neutron crosstalk between channels under construction     */
/* 1.1beta  May 2002 S. Manoshin  Initial version: neutrons are travels via bender channel  */
/*				  WITH CROSSTALK Between channels; Under testing	    */
/*				  bAbsTransCrit: Behaviour of not reflected neutrons 	    */
/*				  REORGANISATION, add some comments, abutment length add    */
/*				  The module is divided into 3 functions and main program   */
/*				  Neutron, which pass via all extreme surfaces - ABSORBING  */
/*							top, bottom, right, left 	    */
/*				  Add materials for neutron flux attenuation between 	    */
/*				  bender channels					    */
/* v1.3alpha June 2002 S. Manoshin Add possibility to read transmission characteristics of  */
/*				  materials between channels from file			    */
/*				  Generation surface part is remove from module		    */
/*				  Add neutron flux attenuation inside bender channels	    */
/* v1.3beta  July 2002 S. Manoshin   Pre-Realization in the VITESS 2.3			    */
/* v1.4	     July 2002 S. Manoshin   Realization in the VITESS 2.3			    */
/* v1.5	     Mar  2003 S. Manoshin   Fixed bug for gravity, gravity may apply ONLY for first*/
/*				     order planes					    */
/*				     Number of channels (or planes) now defing, not fixed!  */
/* v1.6	     Oct 2003  S. Manoshin   Corrected some mistakes with output: Interpolation     */
/*				     func. Improve checking of input datas for interpolation*/
/* v1.7	     Feb 2004  S. Manoshin   Visualise only first 10000 trajectories,               */
/*				     if visualisation was activated			    */
/*				     Choose the output device : screen, file or both	    */
/*				     New external variable gselec 			    */
/********************************************************************************************/


#include <string.h>

#ifdef VT_GRAPH
# include "cpgplot.h"
  double timev, timevmin, timevmax;  /* */
  double timestep; /* step for all surfaces */
  double timestep1; /* step for extreme (left and right) surfaces */
  int do_visualise; /* default : no visualisation */
  long number_vis_tr=0; /* counter : number of trajectories, which was visualised */
  long	cancel_vis=0; /* cancel visualisation */
  extern int gselec; /* choose the output 1 - display only, 2 - file only,
			    3 - both, defined in cpgplot.c */
#endif


#include "intersection.h"
#include "init.h"
#include "softabort.h"

/* Please REMEMBER: This definitions also exists
in the external functions!!! IF YOU CHANGED HERE, PLEASE CHANDE IN
OTHER FUNCTIONS! */

#define  N_SURF		601
#define  N_SURF_S	600
#define  N_SURF_M3	1801
#define  N_SURF_M3_S	1800

#define thetaCNi 0.099138

/****************************************/
/** Structures (changed in March 2002) **/
/****************************************/

/* Please REMEMBER: Structure definition also exists
in the external functions!!! IF YOU CHANGED HERE, PLEASE CHANDE IN
OTHER FUNCTIONS! */

/* Structure for describe bender geometry */

typedef struct
{
/* double CriticalAngles;
  double CutoffAngles; */
  SurfaceSecond SurfLeft [N_SURF];
  SurfaceSecond SurfRight[N_SURF];
  SurfaceSecond SurfExit[N_SURF];
  SurfaceSecond SurfTopBottom[N_SURF];
}
Bender;

/* Structure for describe bender channel */

typedef struct
{
/* double CriticalAngles;
  double CutoffAngles; */
  SurfaceSecond Surf[5];
}
BenderChannel;


/******************************/
/** Inline Functions         **/
/******************************/

FILE * openNFile (char *name) {return fileOpen (name, "r");}


/**************************************/
/** Prototypes of internal functions **/
/**************************************/

short LoadReflFile(FILE* pReflFile, double* pData, char* sWall, char* sSpin);


/**************************************/
/** Prototypes of external functions **/
/**************************************/

double PathThroughChannelGravOrder2(Neutron *, Bender, BenderChannel, long, long, double, double, double *, double *,
                                   double *, double *, double *, double *, double,
                                   long, long, long, double *, double *, double);


double PathThroughBenderGravOrder2(Neutron *, Bender, BenderChannel, long, long, double, double, double *, double *,
                                   double *, double *, double *, double *, double,
                                   long, long, long, double *, double *, double,
                                   long, long, long,
				   double *, double *, long,
				   double *, double *, long,
				   double *, double *, long);

void GeometryTestBender(Bender, double *, double *, double *, double *,
			       double *, double *, double *, double *,
		    	       double, double , double , long);


void FillReflContainer(double array[1000], double m);


/******************************/
/**      MAIN Program        **/
/******************************/

int main(int argc, char *argv[])
{
  /********************************************************************************************/
  /* This module reads in a file of neutron structures, and defines a neutron Bender as a set  */
  /* of five infinite planes with a global critical angle. It outputs the coordinates and time */
  /* displacement of any neutrons that pass through the Bender without being absorbed.	       */
  /*                                                                                           */
  /* Anything not directly commented is an InputNeutrons or an output routine.                 */
  /********************************************************************************************/
  long	i, j, count, numberch = 0, keypol =0 ;
  long	BufferIndex;
  long  Nchannels;
  long	k, counter;
  long 	NumberOfSurfaces;
  long  qspin = 0; /* 0 - axis X, 1 - axis Y, 2 - axis Z magnetic field direction */
  long  keymaterial0=6; /*Material of bender channels: 0 - from file, 1 - gadolinium, 2 - cadmium,
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
  long  keymaterial1=1; /*IN LEFT: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium,
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
  long  keymaterial2=1; /*IN RIGHT: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium,
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
  long ntfr=0, ntfl=0, ntfs=0;


  /* KL */
  long    bAbsTransCrit=0;  /* behaviour of not reflected neutrons (0 = absorbed) */

  double BenderEntranceHeight, BenderExitHeight, BenderEntranceWidth;
  double BenderExitWidth, EntranceHelp,rightend,leftend,spacer,channelwidth;
  double Radius, length, dX, dZ;
  double beta;
  static double rdatalup[1000], rdatarup[1000], rdatatbup[1000];
  static double rdataldo[1000], rdatardo[1000], rdatatbdo[1000];
  double TimeOF1;
  double rdate[N_SURF_M3];
  double transm0[1001]; /* file, which describe transmission of material of bender channel */
  double transm1[1001], transm2[1001]; /* file, which describe transmission of material between channels */
  double WAVL[500], WAVR[500], WAVS[500], MUL[500], MUR[500], MUS[500];
  double surradius[N_SURF], entrancediscenter[N_SURF], exitdiscenter[N_SURF];
  double surfacerough;  /* The parameter which characterized bender surface rought */
  double x1, y1, x2, y2, xr, yr, COSB, SINB; /* for definig base circle */

  double XRL[N_SURF], YRL[N_SURF]; /* for calculating conveging surface */
  double XENL[N_SURF], XEXL[N_SURF], YENL[N_SURF], YEXL[N_SURF], RADL[N_SURF];
  double XCENL[N_SURF], YCENL[N_SURF], XTMPL[N_SURF], YTMPL[N_SURF], ALPHAL[N_SURF];

  double XRR[N_SURF], YRR[N_SURF]; /* for calculating conveging surface */
  double XENR[N_SURF], XEXR[N_SURF], YENR[N_SURF], YEXR[N_SURF], RADR[N_SURF];
  double XCENR[N_SURF], YCENR[N_SURF], XTMPR[N_SURF], YTMPR[N_SURF], ALPHAR[N_SURF];

  double mNumber[2][3];

  double TMP1, TMP2, TMP3, TMP4; /* Temporary for surfaces tests variables */
  double temp1, temp2;
  long   keytest = 0; /* Key for test geometry of bender */

  double ytemp=2.0;
  double entdismin, entdismax;
  double extdismin, extdismax;
  double disabut=0.0; /* Abutment distance, cm */


  char		*ReflFileNamelup=NULL;
  char 		*ReflFileNameldo=NULL;

  char		*TransFileName0=NULL;
  char		*TransFileName1=NULL;
  char		*TransFileName2=NULL;

  char		*ReflFileNamerup=NULL;
  char		*ReflFileNamerdo=NULL;

  char  	*ReflFileNametbup=NULL;
  char		*ReflFileNametbdo=NULL;

  char		*SurfacesFileName=NULL;
  char 		*AsciiFileName=NULL;


  BenderChannel	BenderCh;
  Bender  BenderMy;

  Neutron  Output;

  FILE	*refl_filelup=NULL; /* file for describing left surfaces of bender, spin up */
  FILE  *refl_fileldo=NULL; /* file for describing left surface of bender, spin down */

  FILE	*trans_file0=NULL; /* file for describing of transmission of material of bender channel  */
  FILE	*trans_file1=NULL; /* file for describing of transmission of bender left surface (see from entrance) */
  FILE  *trans_file2=NULL; /* file for describing of transmission of bender right surface(see from entrance) */

  FILE  *refl_filerup=NULL; /* file for describing right surfaces of bender, spin up */
  FILE  *refl_filerdo=NULL; /* file for describing right surfaces of bender, spin down */

  FILE  *refl_filetbup=NULL; /* file for describing top and bottom planes of bender, spin up */
  FILE  *refl_filetbdo=NULL; /* file for describing top and bottom planes of bender, spin down */

  FILE	*surfaces_file=NULL; /* file for describing of surfaces of converging bender*/
  FILE  *AsciiFile=NULL; /* file for output characteristics of bender surfaces */

  /* MF: visualisation for Windows and generation of file for the picture */

#ifdef DO_WIN32
  const char *GraphDev = "bender.ps";
#else
  const char *GraphDev = "bender.png";
#endif

  BenderEntranceHeight = BenderExitHeight = BenderEntranceWidth = 0.0;
  BenderExitWidth = Radius = length = EntranceHelp = 0.0;
  spacer = channelwidth =0.0;
  Nchannels = 0;
  BufferIndex = 0;
  surfacerough = 0.0; /*set by default */

  mNumber[0][0] = -1; mNumber[0][1] = -1; mNumber[0][2] = -1;
  mNumber[1][0] = -1; mNumber[1][1] = -1; mNumber[1][2] = -1;

#ifdef VT_GRAPH
  gselec = 1 ; /* Activate visualisation device -screen */
#endif

  /*input*/
  Init(argc, argv, VT_BENDER);

  for(i=1; i<argc; i++) {
    char *a, *arg;
    a = argv[i];
    if (*a != '-') continue;
    arg = a + 2;
    switch(a[1]) {

    case 'b':  /* up, left plane */
      mNumber[0][0] = atof(&argv[i][2]);  
      FillReflContainer(rdatalup, mNumber[0][0]);
      break; 
      
    case 'B':  /* up, right plane */
      mNumber[0][1] = atof(&argv[i][2]);  
      FillReflContainer(rdatarup, mNumber[0][1]);
      break; 

     case 'd':  /* up, top/bottom plane */
      mNumber[0][2] = atof(&argv[i][2]);  
      FillReflContainer(rdatatbup, mNumber[0][2]);
      break;   

    case 'e':  /* up, left plane */
      mNumber[1][0] = atof(&argv[i][2]);  
      FillReflContainer(rdataldo, mNumber[1][0]);
      break; 
      
    case 'E':  /* up, right plane */
      mNumber[1][1] = atof(&argv[i][2]);  
      FillReflContainer(rdatardo, mNumber[1][1]);
      break; 

     case 'f':  /* up, top/bottom plane */
      mNumber[1][2] = atof(&argv[i][2]);  
      FillReflContainer(rdatatbdo, mNumber[1][2]);
      break;    

    case 'i':
      refl_filelup = openNFile((ReflFileNamelup = arg));
      break;

    case 'I':
      refl_fileldo = openNFile((ReflFileNameldo = arg));
      break;

    case 'm':
      refl_filerup = openNFile((ReflFileNamerup = arg));
      break;

    case 'M':
      refl_filerdo = openNFile((ReflFileNamerdo = arg));
      break;

    case 'u':
      SurfacesFileName = arg;
      if (AsciiFileName==NULL) {
	int len = strlen(SurfacesFileName);
	AsciiFileName = (char*)malloc(len+1);
	memcpy(AsciiFileName, SurfacesFileName, len-3);
	AsciiFileName[len-3] = '\0';
	strcat(AsciiFileName, "Log");
      }
      break;

    case 'k':
      refl_filetbup = openNFile((ReflFileNametbup = arg));
      break;

    case 'K':
      refl_filetbdo = openNFile((ReflFileNametbdo = arg));
      break;

    case 'T':
      trans_file1 = openNFile((TransFileName1 = arg));
      break;

    case 'O':
      trans_file2 = openNFile((TransFileName2 = arg));
      break;

    case 'A':
      AsciiFileName = arg;
      break;

    case 'h':
      BenderEntranceHeight =  atof(arg); /* in cm */
      break;

    case 'H':
      BenderExitHeight = atof(arg);	/* in cm */
      break;

    case 'R':
      Radius =  atof(arg); /* in cm */
      break;

    case 'l':
      length = atof(arg); /* length  of bender in cm */
      break;

    case 's':
      spacer =  atof(arg); /* width of bender channel border in cm */
      break;

    case 'a':
      disabut = atof(arg); /* abutment length, cm*/
      break;

    case 'c':
      keymaterial0 = atol(arg);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
      break;

    case 'C':
      trans_file0 = openNFile((TransFileName0 = arg));
      break;


    case 'z':
      keymaterial1 = atol(arg);  /* IN LEFT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
      break;

    case 'w':
      keymaterial2 = atol(arg);  /* IN RIGHT SIDE: Material between channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - Vacuum */
      break;

    case 'p':
      keypol = atol(arg); /* using(1) or no(0) polarization */
      break;

    case 't':
      keytest = atol(arg); /* test activated - 1 or disactivated -0 */
      break;

    case 'o':
#ifdef VT_GRAPH
      gselec = atol(&argv[i][2]);
#endif
      break;

    case 'y':
      ytemp = atof(arg);   /* for visualiztion */
#ifdef VT_GRAPH
      do_visualise = ytemp != 0;
#endif
      break;

    case 'r':
      surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
      surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
      surfacerough  =  tan(surfacerough);
      break;



    case 'g':
      bAbsTransCrit =  atol(arg);    /* behaviour of not reflected neutrons  */
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

  if (SurfacesFileName)
  {
 	surfaces_file = fileOpen( SurfacesFileName, "r");
  }
  else
  {
    	fprintf(LogFilePtr, "ERROR: no surface file name given\n");
    	exit(-1);
  }


  print_module_name("Bender 1.6");


if (bAbsTransCrit != 0)
{
	if (keymaterial0 == 0)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Material transmission characteristics reading from file\n");
  	}

  	if (keymaterial0 == 1)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Gadolinium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
  	}

  	if (keymaterial0 == 2)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Cadmium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial0 == 3)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Bor10 \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial0 == 4)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Eu \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial0 == 5)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Silicon \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.5 .. 20 A, please correct if nesessary  \n");
  	}

  	if (keymaterial0 == 6)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF BENDER CHANNES: Vacuum \n");
  	}

	if ((keymaterial0 != 0)&&(keymaterial0 != 1)&&(keymaterial0 != 2)&&(keymaterial0 != 3)&&(keymaterial0 != 4)&&(keymaterial0 != 5)&&(keymaterial0 != 6))
  	{
  	    fprintf(LogFilePtr,"ERROR: MATERIAL OF BENDER CHANNES: No material: EXIT! Correct -c option \n");
	    exit(-1);
  	}


  	if (keymaterial1 == 0)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material transmission characteristics reading from file\n");
  	}

  	if (keymaterial1 == 1)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Gadolinium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
  	}

  	if (keymaterial1 == 2)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Cadmium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial1 == 3)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Bor10 \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial1 == 4)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Eu \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial1 == 5)
  	{
	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Silicon \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
  	}

  	if (keymaterial1 == 6)
  	{
  	    fprintf(LogFilePtr,"IN LEFT SIDE OF CHANNEL(see from entrance): Material between channels: Vacuum \n");
  	}

	if ((keymaterial1 != 0)&&(keymaterial1 != 1)&&(keymaterial1 != 2)&&(keymaterial1 != 3)&&(keymaterial1 != 4)&&(keymaterial1 != 5)&&(keymaterial1 != 6))
  	{
  	    fprintf(LogFilePtr,"ERROR: IN LEFT SIDE OF CHANNEL(see from entrance): No material between channels: EXIT! Correct -z option \n");
	    exit(-1);
  	}

  	if (keymaterial2 == 0)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material transmission characteristics read from file \n");
  	}

  	if (keymaterial2 == 1)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Gadolinium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
  	}

  	if (keymaterial2 == 2)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Cadmium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial2 == 3)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Bor10 \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial2 == 4)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Eu \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}

  	if (keymaterial2 == 5)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Silicon \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
  	}

  	if (keymaterial2 == 6)
  	{
	    fprintf(LogFilePtr,"IN RIGHT SIDE OF CHANNEL(see from entrance): Material between channels: Vacuum \n");
  	}

	if ((keymaterial2 != 0)&&(keymaterial2 != 1)&&(keymaterial2 != 2)&&(keymaterial2 != 3)&&(keymaterial2 != 4)&&(keymaterial2 != 5)&&(keymaterial2 != 6))
  	{
  	    fprintf(LogFilePtr,"ERROR: IN RIGHT SIDE OF CHANNEL(see from entrance): No material between channels: EXIT! Correct -w option \n");
	    exit(-1);
  	}
}



  if(disabut > 0.0)
  {
    fprintf(LogFilePtr,"Inside bender abutment loss is enabled \n");
    fprintf(LogFilePtr,"Abutment distance =  %f cm \n",disabut);
  }
  else
  {
    fprintf(LogFilePtr,"Inside bender abutment loss is disabled \n");
  }

  if(keypol == 1)
  {
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is enabled \n");
  }
  else
  {
    fprintf(LogFilePtr,"The dependance of reflectivity from polarization of neutron is disabled \n");
  }

  if(surfacerough == 0.0)
  {
    fprintf(LogFilePtr,"The reflected surface is an ideal mirror \n");
  }
  else
  {
    fprintf(LogFilePtr,"The reflected surface is rough \n");
  }

  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e \n", wei_min);


  /* Read bender data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, a.s.o., deg,  */
  /* reference wavelength 1 A */

  /* initial initialization */

  /* for(i=0;i<1000; i++) */
  /* { */
  /*   rdatalup[i]=0.0; */
  /*   rdatarup[i]=0.0; */
  /*   rdatatbup[i]=0.0; */
  /*   rdataldo[i]=0.0; */
  /*   rdatardo[i]=0.0; */
  /*   rdatatbdo[i]=0.0; */
  /* } */


  for(i=0;i<=1000; i++)
  {
    transm1[i]=0.0;
    transm2[i]=0.0;
    transm0[i]=0.0;
  }


  for(i=0; i<500; i++)
  {
	WAVL[i] = 0.0;
	WAVR[i] = 0.0;
	WAVS[i] = 0.0;
	MUL[i] = 0.0;
	MUR[i] = 0.0;
	MUS[i] = 0.0;
  }


  for(i=0; i<=N_SURF_S; i++)
  {
    entrancediscenter[i] = 0.0;
    exitdiscenter[i] = 0.0;
    surradius[i] = 0.0;
  }


  for(i=0; i<=N_SURF_M3_S; i++)
    rdate[i] = 0.0;


  if(BenderEntranceHeight == 0.0)
  {
    fprintf(LogFilePtr,"ERROR: You must enter the height of the bender\n");
    return(0);
  }

  if(Radius < 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Value of radius is negative\n");
    return(0);
  }

  if(length <= 0.0)
  {
    fprintf(LogFilePtr,"ERROR: Valus length is incorrect, negative or zero\n");
    return(0);
  }


  if(BenderExitHeight == 0.0)
    BenderExitHeight = BenderEntranceHeight;


  /* Read reflectivity files for all walls, spin up and down */
  if (mNumber[0][0] < 0) LoadReflFile(refl_filelup,  rdatalup, "left",  "up");
  if (mNumber[0][1] < 0) LoadReflFile(refl_filerup,  rdatarup, "right", "up");
  if (mNumber[0][2] < 0) LoadReflFile(refl_filetbup, rdatatbup,"top and bottom", "up");
  if (mNumber[1][0] < 0) LoadReflFile(refl_fileldo,  rdataldo, "left", "down");
  if (mNumber[1][1] < 0) LoadReflFile(refl_filerdo,  rdatardo, "right", "down");
  if (mNumber[1][2] < 0) LoadReflFile(refl_filetbdo, rdatatbdo,"top and bottom", "down");


 if (bAbsTransCrit != 0)
 {

    if (keymaterial0 == 0)
    {
      /* Read transmission file for  bender channel */

      if (TransFileName0 !=NULL)
        {
          for(count=1; count<=1000; count++)
    	    {
    		if (fscanf(trans_file0,"%lf",&transm0[count])==EOF)
		    break;
	    }

    	  fclose(trans_file0);

	    k = 1;
	    ntfs = (long)((count-1)/2);
    	    for(i = 1; i <= ntfs; i++)
	    {
		WAVS[i] = transm0[k];
		MUS[i] = transm0[k+1];
		if ((MUS[i] < 0.0)||(WAVS[i] <= 0.0))
		{
		    fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of the bender channel, EXIT!\n");
		    exit(-1);
		}
		k = k + 2;
	    }

	/* check the input data */
	    for(i = 1; i <= (ntfs-1); i++)
	    {
		if (WAVS[i+1] <= WAVS[i])
		{
		    fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of the bender channel \n");
		    fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
		    exit(-1);
		}
	    }

//	    fprintf(LogFilePtr,"transmfile for surface count = %d  num = %d \n", count, ntfs);
//	    for(i = 1; i <= ntfs; i++)
//	    fprintf(LogFilePtr," %f   %f  \n",WAVS[i], MUS[i]);
	    fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAVS[1], WAVS[ntfs]);

      }
      else
	fprintf(LogFilePtr,"No file, which description transmission of bender channel \n");
    }

    if (keymaterial1 == 0)
    {
      /* Read transmission file for left bender surface */

      if (TransFileName1 !=NULL)
        {
          for(count=1; count<=1000; count++)
    	    {
    		if (fscanf(trans_file1,"%lf",&transm1[count])==EOF)
		    break;
	    }

    	  fclose(trans_file1);

	    k = 1;
	    ntfl = (long)((count-1)/2);
    	    for(i = 1; i <= ntfl; i++)
	    {
		WAVL[i] = transm1[k];
		MUL[i] = transm1[k+1];
		if ((MUL[i] < 0.0)||(WAVL[i] <= 0.0))
		{
		    fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of left bender surface. EXIT!\n");
		    exit(-1);
		}
		k = k + 2;
	    }

	/* check the input data */
	    for(i = 1; i <= (ntfl-1); i++)
	    {
		if (WAVL[i+1] <= WAVL[i])
		{
		    fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of left bender surfaces \n");
		    fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
		    exit(-1);
		}
	    }

//	    fprintf(LogFilePtr,"transmfile for left count = %d  num = %d \n", count, ntfl);
//	    for(i = 1; i <= ntfl; i++)
//	    fprintf(LogFilePtr," %f   %f  \n",WAVL[i], MUL[i]);
	    fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAVL[1], WAVL[ntfl]);

      }
      else
	fprintf(LogFilePtr,"No file, which description transmission of LEFT bender surface of channel \n");
    }

    if (keymaterial2 == 0)
    {
      /* Read transmission file for left bender surface */

      if (TransFileName2 !=NULL)
        {
          for(count=1; count<=1000; count++)
    	    {
    		if (fscanf(trans_file2,"%lf",&transm2[count])==EOF)
		    break;
	    }

    	  fclose(trans_file2);

	    k = 1;
	    ntfr = (long)((count-1)/2);

    	    for(i = 1; i <= ntfr; i++)
	    {
		WAVR[i] = transm2[k];
		MUR[i] = transm2[k+1];
		if ((MUR[i] < 0.0)||(WAVR[i] <= 0.0))
		{
		    fprintf(LogFilePtr,"ERROR: Non-Positive value was found in transmission file of right bender surfaces. EXIT!\n");
		    exit(-1);
		}
		k = k + 2;
	    }

	/* check the input data */
	    for(i = 1; i <= (ntfr-1); i++)
	    {
		if (WAVR[i+1] <= WAVR[i])
		{
		    fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of right bender surfaces \n");
		    fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
		    exit(-1);
		}
	    }

//	    fprintf(LogFilePtr,"transmfile for right count = %d  num = %d \n", count, ntfr);
//	    for(i = 1; i <= ntfr; i++)
//	    fprintf(LogFilePtr," %f   %f  \n",WAVR[i], MUR[i]);
	    fprintf(LogFilePtr,"Minimum and maximum wavelength must be  %f ... %f\n",WAVR[1], WAVR[ntfr]);
      }
      else
	fprintf(LogFilePtr,"No file, which description transmission of RIGHT bender surface of channel \n");
    }

 }

  if (SurfacesFileName !=NULL)
    {
      for(counter = 1; counter <= N_SURF_M3_S; counter++)
	if (fscanf(surfaces_file,"%lf",&rdate[counter])==EOF)
	  break;

      fclose(surfaces_file);
    }
  else
    {
      fprintf(LogFilePtr,"\n ERROR: No Surface data. Check the data in the file. \n");
      exit(-1);
    }


  if(AsciiFileName)
    AsciiFile = fileOpen( AsciiFileName, "wt");
  else
  {
    fprintf(LogFilePtr,"ERROR: The open -A with a file name of the ascii file is mandatory!\n");
    exit(-1);
  }


  NumberOfSurfaces = (long)((counter-1)/3);

  /*	fprintf(LogFilePtr,"Number of in %d \n", (counter-1)); */
  fprintf(LogFilePtr,"Number of surfaces: %ld\n", NumberOfSurfaces);

  k = 1;
  for(i = 1; i <= NumberOfSurfaces; i++)
    {
      entrancediscenter[i] = rdate[k];
      exitdiscenter[i] = rdate[k+1];
      surradius[i] = rdate[k+2];
      /* Automaticly disable gravity, if plane have a curvature */
    	if (surradius[i] != 0.0)
	{
	    keygrav = 0;
	}
      k = k + 3;
    }

if ((bAbsTransCrit != 0)&&(NumberOfSurfaces == 2))
 {
 	fprintf(LogFilePtr,"ERROR: you have guide for simulation, \n"
 	"which have only one channel and choosed transmission between channels. \n"
 	"Please not use the transmission between channels!\n");
 	exit(-1);
 }

  /* define bender entrance and exit width and radius of curvature */


  entdismin =  9999999.0;
  entdismax = -9999999.0;

  extdismin =  9999999.0;
  extdismax = -9999999.0;

  for(i = 1; i <= NumberOfSurfaces; i++)
    {
      if (entrancediscenter[i] <= entdismin)
	{
	  entdismin = entrancediscenter[i];
	}

      if (entrancediscenter[i] >= entdismax)
	{
	  entdismax = entrancediscenter[i];
	}

      if (exitdiscenter[i] <= extdismin)
	{
	  extdismin = exitdiscenter[i];
	}

      if (exitdiscenter[i] >= extdismax)
	{
	  extdismax = exitdiscenter[i];
	}
    }

  fprintf(LogFilePtr,"entrance: max = %f  min = %f \n", entdismax, entdismin);
  fprintf(LogFilePtr,"exit    : max = %f  min = %f \n", extdismax, extdismin);

  /* KL: correction fabs(x - y) instead of fabs(x) + fabs(y) */

  BenderEntranceWidth = fabs(entdismax - entdismin);
  BenderExitWidth     = fabs(extdismax - extdismin);

  fprintf(LogFilePtr,"BENW= %f  BEXW= %f \n",BenderEntranceWidth,BenderExitWidth);


  /****************************************************************************************/
  /* Set up the parameters of the surfaces from the InputNeutrons data...                 */
  /****************************************************************************************/



  /* Define base circle or line */
  beta = 0.0;
  if (Radius != 0)
  {
    /* base circle */
    beta = length/Radius;

    /* the entrance center point*/
    x1 = 0.0;
    y1 = 0.0;

    /* the origin of base circle */
    xr = x1;
    yr = y1 + Radius;

    /* the exit center point */
    x2 = xr + Radius*sin(beta);
    y2 = yr - Radius*cos(beta);
  }
  else
  {
    /* base line */
    beta = 0.0;

    /* the entrance center point*/
    x1 = 0.0;
    y1 = 0.0;

    /* the origin of base circle, not actually */
    xr = 0.0;
    yr = 0.0;

    /* the exit center point */
    x2 = x1 + length;
    y2 = 0.0;
 }



  /* for transform of system of coordimate */
  COSB = cos(beta);
  SINB = sin(beta);

  /* NOTE! x2, y2, beta is base for TRANSFORM system of coordimats */
/*  fprintf(LogFilePtr,"entp x1= %f  y1= %f beta= %f \n", x1,y1,beta);
  fprintf(LogFilePtr,"exip x2= %f  y2= %f beta= %f \n", x2,y2,beta);
  fprintf(LogFilePtr,"cent base ci xr= %f yr= %f \n",xr,yr); */

  TMP1 = sqrt((xr-x1)*(xr-x1) + (yr-y1)*(yr-y1));
  TMP2 = sqrt((xr-x2)*(xr-x2) + (yr-y2)*(yr-y2));
//  fprintf(LogFilePtr,"DISTBC  %f  %f\n",TMP1, TMP2);

  /* top and bottom planes, possible converging or diverging */

  dX = length;
  dZ = (BenderExitHeight - BenderEntranceHeight)/2.0;

  BenderCh.Surf[0].A = 0.0;
  BenderCh.Surf[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].C = 0.0;
  BenderCh.Surf[0].D = 0.0;
  BenderCh.Surf[0].E = 0.0;
  BenderCh.Surf[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[0].W = -BenderCh.Surf[0].F*(BenderEntranceHeight/2.0);
  BenderCh.Surf[0].P = 0.0;
  BenderCh.Surf[0].Q = 0.0;
  BenderCh.Surf[0].R = 0.0;

  BenderMy.SurfTopBottom[0].A = 0.0;
  BenderMy.SurfTopBottom[0].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[0].C = 0.0;
  BenderMy.SurfTopBottom[0].D = 0.0;
  BenderMy.SurfTopBottom[0].E = 0.0;
  BenderMy.SurfTopBottom[0].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[0].W = -BenderMy.SurfTopBottom[0].F*(BenderEntranceHeight/2.0);
  BenderMy.SurfTopBottom[0].P = 0.0;
  BenderMy.SurfTopBottom[0].Q = 0.0;
  BenderMy.SurfTopBottom[0].R = 0.0;


  dZ=-dZ;

  BenderCh.Surf[1].A = 0.0;
  BenderCh.Surf[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].C = 0.0;
  BenderCh.Surf[1].D = 0.0;
  BenderCh.Surf[1].E = 0.0;
  BenderCh.Surf[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderCh.Surf[1].W = BenderCh.Surf[1].F*(BenderEntranceHeight/2.0);
  BenderCh.Surf[1].P = 0.0;
  BenderCh.Surf[1].Q = 0.0;
  BenderCh.Surf[1].R = 0.0;

  BenderMy.SurfTopBottom[1].A = 0.0;
  BenderMy.SurfTopBottom[1].B = (dZ/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[1].C = 0.0;
  BenderMy.SurfTopBottom[1].D = 0.0;
  BenderMy.SurfTopBottom[1].E = 0.0;
  BenderMy.SurfTopBottom[1].F = -(dX/(sqrt(dX*dX+dZ*dZ)));
  BenderMy.SurfTopBottom[1].W = BenderMy.SurfTopBottom[1].F*(BenderEntranceHeight/2.0);
  BenderMy.SurfTopBottom[1].P = 0.0;
  BenderMy.SurfTopBottom[1].Q = 0.0;
  BenderMy.SurfTopBottom[1].R = 0.0;



  /* Begin to build curved or straight surfaces */

  for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {

      /* calculating the center of surface on given entrance, exit points and radius
	 of curvature */
      /* calculating poins for circle FOR LEFT SURFACES*/

      /* fprintf(LogFilePtr,"Make channel %d \n",i); */

      XENL[i] = x1;
      YENL[i] = y1 + (entrancediscenter[i+1]-0.5*spacer);

      XEXL[i] = x2 - (sin(beta))*(exitdiscenter[i+1]-0.5*spacer);
      YEXL[i] = y2 + (cos(beta))*(exitdiscenter[i+1]-0.5*spacer);
      RADL[i] = surradius[i+1];

     if (RADL[i] != 0.0)
     {
          ALPHAL[i] = atan2((YEXL[i]-YENL[i]),(XEXL[i]-XENL[i]));

	  XCENL[i] = 0.5*(XENL[i]+XEXL[i]);
          YCENL[i] = 0.5*(YENL[i]+YEXL[i]);
          XTMPL[i] = 0.0;
          YTMPL[i] = sqrt(RADL[i]*RADL[i]-0.25*(XENL[i]-XEXL[i])*(XENL[i]-XEXL[i])-0.25*(YENL[i]-YEXL[i])*(YENL[i]-YEXL[i]));

	  /* feature for radius is negative		*/

          YTMPL[i] = YTMPL[i]*RADL[i]/(fabs(RADL[i]));

          /* calculate the center of circle via transform of coordinate system  */

          XRL[i] = XTMPL[i]*cos(ALPHAL[i]) - YTMPL[i]*sin(ALPHAL[i]) + XCENL[i];
          YRL[i] = XTMPL[i]*sin(ALPHAL[i]) + YTMPL[i]*cos(ALPHAL[i]) + YCENL[i];

          TMP1 = sqrt((XRL[i]-XENL[i])*(XRL[i]-XENL[i]) + (YRL[i]-YENL[i])*(YRL[i]-YENL[i]));
          TMP2 = sqrt((XRL[i]-XEXL[i])*(XRL[i]-XEXL[i]) + (YRL[i]-YEXL[i])*(YRL[i]-YEXL[i]));

          BenderMy.SurfLeft[i].A = 1.0;
          BenderMy.SurfLeft[i].B = -2.0*XRL[i];
          BenderMy.SurfLeft[i].C = 1.0;
          BenderMy.SurfLeft[i].D = -2.0*YRL[i];
          BenderMy.SurfLeft[i].E = 0.0;
          BenderMy.SurfLeft[i].F = 0.0;
          BenderMy.SurfLeft[i].W = XRL[i]*XRL[i] + YRL[i]*YRL[i] - (RADL[i])*(RADL[i]);
          BenderMy.SurfLeft[i].P = 0.0;
          BenderMy.SurfLeft[i].Q = 0.0;
          BenderMy.SurfLeft[i].R = 0.0;
      }
      else
      {
       /*	Straight Line  */
      	BenderMy.SurfLeft[i].A = 0.0;
      	BenderMy.SurfLeft[i].B = YENL[i] - YEXL[i];
      	BenderMy.SurfLeft[i].C = 0.0;
      	BenderMy.SurfLeft[i].D = XEXL[i] - XENL[i];
      	BenderMy.SurfLeft[i].E = 0.0;
      	BenderMy.SurfLeft[i].F = 0.0;
      	BenderMy.SurfLeft[i].W = YEXL[i]*XENL[i] - YENL[i]*XEXL[i];
      	BenderMy.SurfLeft[i].P = 0.0;
      	BenderMy.SurfLeft[i].Q = 0.0;
      	BenderMy.SurfLeft[i].R = 0.0;
      }

      /* FOR RIGHT SURFACES */

      XENR[i] = x1;
      YENR[i] = y1 + (entrancediscenter[i]+0.5*spacer);

      XEXR[i] = x2 - (sin(beta))*(exitdiscenter[i]+0.5*spacer);
      YEXR[i] = y2 + (cos(beta))*(exitdiscenter[i]+0.5*spacer);
      RADR[i] = surradius[i];

     if (RADR[i] != 0.0)
     {
          ALPHAR[i] = atan2((YEXR[i]-YENR[i]),(XEXR[i]-XENR[i]));

          XCENR[i] = 0.5*(XENR[i]+XEXR[i]);
          YCENR[i] = 0.5*(YENR[i]+YEXR[i]);
          XTMPR[i] = 0.0;
          YTMPR[i] = sqrt(RADR[i]*RADR[i]-0.25*(XENR[i]-XEXR[i])*(XENR[i]-XEXR[i])-0.25*(YENR[i]-YEXR[i])*(YENR[i]-YEXR[i]));

          /* feature for radius is negative		*/

          YTMPR[i] = YTMPR[i]*RADR[i]/(fabs(RADR[i]));

          /* calculate the center of circle via transform of coordinate system*/

          XRR[i] = XTMPR[i]*cos(ALPHAR[i]) - YTMPR[i]*sin(ALPHAR[i]) + XCENR[i];
          YRR[i] = XTMPR[i]*sin(ALPHAR[i]) + YTMPR[i]*cos(ALPHAR[i]) + YCENR[i];


          BenderMy.SurfRight[i].A = 1.0;
          BenderMy.SurfRight[i].B = -2.0*XRR[i];
          BenderMy.SurfRight[i].C = 1.0;
          BenderMy.SurfRight[i].D = -2.0*YRR[i];
          BenderMy.SurfRight[i].E = 0.0;
          BenderMy.SurfRight[i].F = 0.0;
          BenderMy.SurfRight[i].W = XRR[i]*XRR[i] + YRR[i]*YRR[i] - (RADR[i])*(RADR[i]);
          BenderMy.SurfRight[i].P = 0.0;
          BenderMy.SurfRight[i].Q = 0.0;
          BenderMy.SurfRight[i].R = 0.0;


          TMP3 = sqrt((XRR[i]-XENR[i])*(XRR[i]-XENR[i]) + (YRR[i]-YENR[i])*(YRR[i]-YENR[i]));
          TMP4 = sqrt((XRR[i]-XEXR[i])*(XRR[i]-XEXR[i]) + (YRR[i]-YEXR[i])*(YRR[i]-YEXR[i]));

      }
      else
      {
           /*  Straight line  */
          BenderMy.SurfRight[i].A = 0.0;
          BenderMy.SurfRight[i].B = YENR[i] - YEXR[i];
          BenderMy.SurfRight[i].C = 0.0;
          BenderMy.SurfRight[i].D = XEXR[i] - XENR[i];
          BenderMy.SurfRight[i].E = 0.0;
          BenderMy.SurfRight[i].F = 0.0;
          BenderMy.SurfRight[i].W = YEXR[i]*XENR[i] - YENR[i]*XEXR[i];
          BenderMy.SurfRight[i].P = 0.0;
          BenderMy.SurfRight[i].Q = 0.0;
          BenderMy.SurfRight[i].R = 0.0;
      }

/*      fprintf(LogFilePtr,"\n LEFT data: XEN = %e  YEN = %e  %e", XENL[i], YENL[i], RADL[i]);
      fprintf(LogFilePtr,"\n LEFT data: XEX = %e  YEX = %e  %e", XEXL[i], YEXL[i], RADL[i]);
      if (RADL[i] != 0.0)
      fprintf(LogFilePtr,"\nL DIST CI %e %e %e al = %e",TMP1, TMP2, RADL[i], ALPHAL[i]);

      fprintf(LogFilePtr,"\n RIGT data: XEN = %e  YEN = %e  %e", XENR[i], YENR[i], RADR[i]);
      fprintf(LogFilePtr,"\n RIGT data: XEX = %e  YEX = %e  %e", XEXR[i], YEXR[i], RADR[i]);
      if (RADR[i] != 0.0)
      fprintf(LogFilePtr,"\nR DIST CI %e %e %e al = %e",TMP1, TMP2, RADR[i], ALPHAR[i]); */

	/*  Exit surface  */

	  BenderMy.SurfExit[i].A = 0.0;
  	  BenderMy.SurfExit[i].B = cos(beta);
  	  BenderMy.SurfExit[i].C = 0.0;
  	  BenderMy.SurfExit[i].D = sin(beta);
  	  BenderMy.SurfExit[i].E = 0.0;
  	  BenderMy.SurfExit[i].F = 0.0;
  	  BenderMy.SurfExit[i].W = -1.0*(x2*cos(beta) + y2*sin(beta));
  	  BenderMy.SurfExit[i].P = 0.0;
  	  BenderMy.SurfExit[i].Q = 0.0;
  	  BenderMy.SurfExit[i].R = 0.0;

    }

  /* Output in file some of parameters of surfaces */
  fprintf(AsciiFile,"******************* UNIVERSAL BENDER module ************************ \n");
  fprintf(AsciiFile,"********** INFORMATION FOR FABRICATE OF CONVERGING BENDER ********** \n");
  fprintf(AsciiFile,"******************************************************************** \n");
  fprintf(AsciiFile,"***USER*INPUT*DATA***\n");
     for(i = 1; i <= NumberOfSurfaces; i++)
    {
      fprintf(AsciiFile,"Displace data: N = %ld  EN = %e cm  EX = %e cm  RAD = %e cm \n", i,
	      entrancediscenter[i], exitdiscenter[i], surradius[i]);
    }
  fprintf(AsciiFile,"Bender Entrance Width = %f cm ;Bender Exit Width = %f cm \n",BenderEntranceWidth,BenderExitWidth);
  fprintf(AsciiFile,"Bender Entrance Height = %f cm ;Bender Exit Height = %f cm \n",BenderEntranceHeight,BenderExitHeight);
  fprintf(AsciiFile,"Bender Length = %f cm; Bender surfaces thickness = %f cm \n",length,spacer);

if (Radius != 0.0)
    {
      fprintf(AsciiFile,"Base bender axis - circle\n");
      fprintf(AsciiFile,"Radius of Curvature for base bender axis = %f cm \n",Radius);
      fprintf(AsciiFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",x1, y1);
      fprintf(AsciiFile,"EXIT: Base points x = %f cm  y = %f cm \n",x2, y2);
      fprintf(AsciiFile,"The coordinates of center of base circle x = %f cm  y = %f cm \n",xr,yr);
    }
else
    {
      fprintf(AsciiFile,"Base bender axis - line\n");
      fprintf(AsciiFile,"ENTRANCE: Base points x = %f cm  y = %f cm \n",x1, y1);
      fprintf(AsciiFile,"EXIT: Base points x = %f cm  y = %f cm \n",x2, y2);
    }

  fprintf(AsciiFile,"****************************SURFACES******************************* \n");
  fprintf(AsciiFile,"******************************************************************* \n");
  fprintf(AsciiFile,"*****************THICKNESS*OF*SURFACES*IS*INCLUDED***************** \n");


  for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {

      fprintf(AsciiFile,"Begin Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",
	      XENR[i], YENR[i]);

      fprintf(AsciiFile,"Begin Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",
	      XENL[i], YENL[i]);
      fprintf(AsciiFile,"------------------------------------------------------------------------------\n");

      fprintf(AsciiFile,"Exit Points of RIGHT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",
	      XEXR[i], YEXR[i]);

      fprintf(AsciiFile,"Exit Points of LEFT surfaces %ld  X, Y;\n",i);
      fprintf(AsciiFile,"X = %f cm  Y = %f cm  \n",
	      XEXL[i], YEXL[i]);

      fprintf(AsciiFile,"------------------------------------------------------------------------------\n");
      fprintf(AsciiFile,"==============================================================================\n");


    }

  fprintf(AsciiFile,"==============================================================================\n");


  for(i = 1; i <= (NumberOfSurfaces-1); i++) {

    fprintf(AsciiFile,"Center of RIGHT surface %ld X, Y; Radius\n",i);
    fprintf(AsciiFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",
	    XRR[i], YRR[i], RADR[i]);

    fprintf(AsciiFile,"Center of LEFT surface %ld X, Y; Radius\n",i);
    fprintf(AsciiFile,"X = %f cm  Y = %f cm  RAD = %f cm \n",
	    XRL[i], YRL[i], RADL[i]);

    fprintf(AsciiFile,"------------------------------------------------------------------------------\n");

  }


#ifdef VT_GRAPH

  if (do_visualise)
  {
    /* visualise bender surfaces */

  /* choose device for output visualisation */

    if (!((gselec == 1)||(gselec == 2)||(gselec == 3)))
    {
	fprintf(LogFilePtr,"ERROR: Incorrect output device, correct option -o, value 1,2 or 3 \n");
	exit(-1);
    }


    if (gselec == 1)
    {
	fprintf(LogFilePtr,"Open visual output device - display\n");
    }

    if (gselec == 2)
    {
	fprintf(LogFilePtr,"Open visual output device - file \n");
    }

    if (gselec == 3)
    {
	fprintf(LogFilePtr,"Open visual output device - display+file \n");
    }


    /* open graphical device */
    if (cpgopen(GraphDev) < 1)
    {
      fprintf(LogFilePtr,"ERROR: I cannot open plot device \n");
      exit(-1);
    }
    fprintf(LogFilePtr,"Number of visualised neutrons = %ld \n", BufferSize);

    /* KL: improvement: using a scale that brings the total bender on the screen */
    cpgenv(0.0,1.2*(length),
	   1.2*Min(entdismin,extdismin+y2),1.2*Max(entdismax,extdismax+y2),0,0);

    cpgsfs(2);

    cpgsch(1.2);


      if (bAbsTransCrit == 0)
      {
        	/* Neutrons are travels WITHOUT crosstalk between channels */
           cpglab("X, cm ; Red Line - Device Axis","Y, cm", NumberOfSurfaces == 2 ?
		   "Guide Surface Visualisation" :
		   "Bender Surfaces Visualisation WITHOUT crosstalk between channels");
      }
      else
      {
         	 /* Neutrons are travels WITH crosstalk between channels */

          cpglab("X, cm ; Red Line - Device Axis","Y, cm", NumberOfSurfaces == 2 ?
		   "Guide Surface Visualisation" :
		   "Bender Surfaces Visualisation WITH crosstalk between channels ");
      }


    cpgsci(2);
    cpgpt1(x1,y1,-1);
    cpgpt1(x2,y2,-1);

    /* Draw bender axis */

    if (Radius != 0.0)
    {
	/*  draw cirlcle axis */
	timevmin = 0.0;
	timevmax = 2*M_PI;
	timestep = (timevmax - timevmin)/5000.0;

        for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
	{
          temp1 = xr + Radius*cos(timev);
          temp2 = yr + Radius*sin(timev);
          cpgpt1(temp1,temp2,-1);
        }
   }
   else
   {
       /* draw line axis */
       cpgmove(x1,y1);
       cpgdraw(1.2*(length),y2);

   }

    /* visualize entrance and exit line */

    for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {
	cpgsci(1);
    	/* set poins */
        cpgpt1(XENL[i], YENL[i], 22);
        cpgpt1(XEXL[i], YEXL[i], 22);

        cpgpt1(XENR[i], YENR[i], 22);
        cpgpt1(XEXR[i], YEXR[i], 22);

      	/* entrance line*/
	cpgmove(XENL[i],YENL[i]);
	cpgdraw(XENR[i],YENR[i]);

	/* exit line */
	cpgmove(XEXL[i],YEXL[i]);
	cpgdraw(XEXR[i],YEXR[i]);


      /* draw left surfaces */

      if (RADL[i] != 0.0)
      {
          /* SM: revised */
          if (RADL[i] > 0.0)
	  {
              /* KL: acceleration of the graphics (see above) */
	      timevmin = 2*M_PI - (acos((XENL[i] - XRL[i]) / (RADL[i]))); /* 0.0;      */
              timevmax = 2*M_PI - (acos((XEXL[i] - XRL[i]) / (RADL[i]))); /* 2.0*M_PI; */
              timestep = (timevmax - timevmin) / 100.0;                  /* 0.0001,   /(RADL[i]+RADR[i]) */
	      timestep1 = timestep/2.0;
	  }
	  else
	  {
	      timevmin = acos((XEXL[i] - XRL[i])/fabs(RADL[i]));
	      timevmax = acos((XENL[i] - XRL[i])/fabs(RADL[i]));
	      timestep = (timevmax - timevmin) / 100.0;
	      timestep1 = timestep/2.0;
	  }

	  /* draw all left surfaces*/

          cpgsci(4);
          for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
	  {
		temp1 = XRL[i] + (fabs(RADL[i]))*cos(timev);
		temp2 = YRL[i] + (fabs(RADL[i]))*sin(timev);
		cpgpt1(temp1,temp2,-1);
          }

          /* draw extreme left surfaces */

	  if (i == (NumberOfSurfaces-1))
	  {
	    cpgsci(1);
	        for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
	  	{
			temp1 = XRL[i] + (fabs(RADL[i]))*cos(timev);
			temp2 = YRL[i] + (fabs(RADL[i]))*sin(timev);
			cpgpt1(temp1,temp2,-1);
          	}
	  }

      }
      else
      {
    	 /*	draw all surfaces lines	 */
	     cpgsci(4);
	     cpgmove(XENL[i],YENL[i]);
	     cpgdraw(XEXL[i],YEXL[i]);

	 /*    draw left extreme surface line */

	     if (i == (NumberOfSurfaces-1))
	     {
	     	 cpgsci(1);
	     	 cpgmove(XENL[i],YENL[i]);
	     	 cpgdraw(XEXL[i],YEXL[i]);
	     }
      }

	/* draw right surfaces */

      if (RADR[i] != 0.0)
      {
      	          /* SM: revised */
          if (RADR[i] > 0.0)
	  {
              /* KL: acceleration of the graphics (see above) */
	      timevmin = 2*M_PI - (acos((XENR[i] - XRR[i]) / (RADR[i]))); /* 0.0;      */
              timevmax = 2*M_PI - (acos((XEXR[i] - XRR[i]) / (RADR[i]))); /* 2.0*M_PI; */
              timestep = (timevmax - timevmin) / 100.0;                  /* 0.0001,   /(RADL[i]+RADR[i]) */
	      timestep1 = timestep/2.0;
	  }

	  else
	  {
	      timevmin = acos((XEXR[i] - XRR[i])/fabs(RADR[i]));
	      timevmax = acos((XENR[i] - XRR[i])/fabs(RADR[i]));
	      timestep = (timevmax - timevmin) / 100.0;
	      timestep1 = timestep/2.0;
	  }

       	  /* draw all right surfaces*/

          cpgsci(5);
          for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
	  {
		temp1 = XRR[i] + (fabs(RADR[i]))*cos(timev);
        	temp2 = YRR[i] + (fabs(RADR[i]))*sin(timev);
		cpgpt1(temp1,temp2,-1);
          }

          /* draw extreme right surface */

	  if (i == 1)
	  {
	    cpgsci(2);
	        for(timev = timevmin; timev <= timevmax; timev = timev + timestep)
	  	{
			temp1 = XRR[i] + (fabs(RADR[i]))*cos(timev);
        		temp2 = YRR[i] + (fabs(RADR[i]))*sin(timev);
			cpgpt1(temp1,temp2,-1);
          	}
	  }

     }
     else
     {
         /*	draw all right surfaces line	*/

	cpgsci(5);
    	cpgmove(XENR[i],YENR[i]);
	cpgdraw(XEXR[i],YEXR[i]);

	 /*    draw right extreme surface line */

	if (i == 1)
	{
	   cpgsci(2);
	   cpgmove(XENR[i],YENR[i]);
	   cpgdraw(XEXR[i],YEXR[i]);
	}
     }

    }
    /* end visualise bender surfaces */
  }


#endif


    /* test geometry of bender */
    if (keytest == 1)
    {
	GeometryTestBender(BenderMy, XENR, YENR, XENL, YENL, XEXR, YEXR, XEXL, YEXL,
    			BenderEntranceHeight, BenderExitHeight, beta, NumberOfSurfaces);
    }


  /*  Protect agains illegal value qspin must be  0 or 1 or 2 ONLY */

  if (qspin < 0 || qspin > 2) qspin = 0;

  if (qspin == 0)
      fprintf(LogFilePtr,"Magnetic field direction - AXIS OX \n");

  if (qspin == 1)
      fprintf(LogFilePtr,"Magnetic field direction - AXIS OY \n");

  if (qspin == 2)
      fprintf(LogFilePtr,"Magnetic field direction - AXIS OZ \n");

  if(keygrav == 1)
  {
    fprintf(LogFilePtr,"Inside bender gravity is enabled \n");
    fprintf(LogFilePtr,"Please remember, that gravity only working for first order planes!!! \n");
    fprintf(LogFilePtr,"If your planes have curvature, gravity is disabled!!! \n");
  }
  else
  {
    fprintf(LogFilePtr,"Inside bender gravity is disabled \n");
  }


  DECLARE_ABORT

  while(ReadNeutrons()) {
    for(i=0; i<NumNeutGot; i++) {


#ifdef VT_GRAPH
  if (do_visualise)
  {
	if  (number_vis_tr == BufferSize)
	{
	    do_visualise = 0 != 0; /* stop visualisation */
	    cancel_vis = 1;
	    fprintf(LogFilePtr,"Visualisation is stopped\n");
	}
  }
#endif

	CHECK

      TimeOF1 = 0.0;

      /*	InputNeutrons[i].Position.X = 0.0;   !!!!!!!! */
      /****************************************************************************************/
      /* Check to see if the neutron is initially in the entrance to the bender...             */
      /****************************************************************************************/

      if (fabs(InputNeutrons[i].Position[2])>BenderEntranceHeight/2.0) continue;

      /****************choose the channel******************/
      /* include a phickness	*/

      for (j=1;j<=(NumberOfSurfaces-1);j++)
      {

	rightend=YENR[j];
	leftend=YENL[j];
	if((rightend<InputNeutrons[i].Position[1])&&(leftend>InputNeutrons[i].Position[1]))
	  {
	    numberch = j;
	    break;
	  }
      }

//      	fprintf(LogFilePtr,"J end =  %d  %d  \n",j, numberch);

      if(j==NumberOfSurfaces)
	continue;  /*neutron blocked by spacer*/


      /* Check the quantization of polarization */

      if (keypol == 1)
	if (fabs(InputNeutrons[i].Spin[qspin]) != 1.0)
	  {
	        fprintf(LogFilePtr,"ERROR: Illegal Spin Quantisation!!! Check the Spin value \n");
		exit(-1);
	  }



      /******************************************************************************************/
      /* Pass a pointer to the neutron and the Bender structure variable to a subroutine to do  */
      /* the donkey work. The return value is the total value of the time of flight through the */
      /* Bender, or -1.0 if it missed all plates and the exit (should be impossible).           */
      /******************************************************************************************/

	        /* Choose the behavior of neutrons between channels */

      if (bAbsTransCrit == 0)
      {
        	/* Neutrons are travels WITHOUT crosstalk between channels */
          TimeOF1 = PathThroughChannelGravOrder2(&InputNeutrons[i], BenderMy, BenderCh, numberch, NumberOfSurfaces, wei_min, disabut,
					    rdatalup, rdatarup, rdatatbup, rdataldo, rdatardo, rdatatbdo, surfacerough,
					    keygrav, keypol, qspin,
					    entrancediscenter, exitdiscenter, spacer);
      }
      else
      {
         	 /* Neutrons are travels WITH crosstalk between channels */
	  TimeOF1 = PathThroughBenderGravOrder2(&InputNeutrons[i], BenderMy, BenderCh, numberch, NumberOfSurfaces, wei_min, disabut,
					    rdatalup, rdatarup, rdatatbup, rdataldo, rdatardo, rdatatbdo, surfacerough,
					    keygrav, keypol, qspin,
					    entrancediscenter, exitdiscenter, spacer,
					    keymaterial0, keymaterial1, keymaterial2,

					    WAVS, MUS, ntfs,
					    WAVL, MUL, ntfl,
					    WAVR, MUR, ntfr);

      }

      if(TimeOF1 == -1.0)  continue;
      if(TimeOF1 == -10000.0) exit(-1)
;


      /****************************************************************************************/
      /* Transform the coordinates.   					         	      */
      /* X must be always renormalized to zero...                                                  */
      /****************************************************************************************/

      /* KL: correction: recursion found in calculation of InputNeutrons[i].Position[1], ...Vector[1]
                         InputNeutrons[i].Position[0], ...Vector[0]  were already changed !           */

      Output = InputNeutrons[i];

          /* KL: correction: transformation: move coordinate system to the center of rotation
	                                     rotate
    	                                     move coordinate system back       */
/*          InputNeutrons[i].Position[1] -= Radius;
            Output.Position[0] =  (InputNeutrons[i].Position[0])*COSB + (InputNeutrons[i].Position[1])*SINB;
            Output.Position[1] = -(InputNeutrons[i].Position[0])*SINB + (InputNeutrons[i].Position[1])*COSB;
            Output.Position[1] += Radius;  */

        /* SM: Similary */

            Output.Position[0] =  (InputNeutrons[i].Position[0]-x2)*COSB + (InputNeutrons[i].Position[1]-y2)*SINB;
            Output.Position[1] = -(InputNeutrons[i].Position[0]-x2)*SINB + (InputNeutrons[i].Position[1]-y2)*COSB;

   	    Output.Vector[0] =  (InputNeutrons[i].Vector[0])*COSB + (InputNeutrons[i].Vector[1])*SINB;
            Output.Vector[1] = -(InputNeutrons[i].Vector[0])*SINB + (InputNeutrons[i].Vector[1])*COSB;


//      fprintf(LogFilePtr,"Out x = %f  y = %f  z = %f \n",Output.Position[0], Output.Position[1], Output.Position[2]);


      if (fabs(Output.Position[2])>BenderExitHeight/2.0) continue;


      /****************************************************************************************/
      /* Add the time needed to travel inside Bender.                                   */
      /****************************************************************************************/
      Output.Time = Output.Time + TimeOF1;
      /****************************************************************************************/
      /* Count this as a success.                                                             */
      /****************************************************************************************/

#ifdef VT_GRAPH
  if (do_visualise)
  {
	number_vis_tr = number_vis_tr + 1;
  }
#endif

      WriteNeutron(&Output);
    }
  }

 my_exit:

#ifdef VT_GRAPH
  if ((do_visualise)||(cancel_vis == 1))
  {
    fprintf(LogFilePtr,"Close graphical window\n");
    cpgclos();
  }
#endif

  Cleanup(x2, y2, 0.0, beta, 0.0);

  fclose(AsciiFile);

  return(0);
}

short LoadReflFile(FILE* pReflFile, double* pData, char* sWall, char* sSpin)
{
  short rc;
  int  nLines, iLine;
  char sBuffer[100];

  if (pReflFile != NULL) 
  {
    nLines = LinesInFile(pReflFile);

    for(iLine=0; iLine < nLines; iLine++) 
    {
      ReadLine(pReflFile, sBuffer, sizeof(sBuffer)-1);
      StrgScanLF(sBuffer, &pData[10*iLine], 10, 0);
    }
	fclose(pReflFile);
	rc=TRUE;
  }
  else
  { fprintf(LogFilePtr,"case of no reflectivity for %s surfaces of bender for spin %s neutrons \n", sWall, sSpin);
	rc=FALSE;
  }
  return rc;
}



void FillReflContainer(double array[1000], double m)
{
  int i;
  double lambda = 1./thetaCNi;

   if (m < 0) {
    fprintf(LogFilePtr,"m-Value below 0 is given! Module stops!");
    exit(-1);
  }

   for (i = 0; i < 1000; i++) array[i] = ReflSN(lambda, (double)i*0.01, m);

  return;

}






