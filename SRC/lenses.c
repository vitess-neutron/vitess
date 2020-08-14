/********************************************************************************************/
/*  VITESS MODULE LENSES                                                                    */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* Written by Manoshin Sergey, start project May 2006                                       */
/*                                                                                          */
/* Brief history:                                                                           */
/* 1.00  5 Jul 2006  S. Manoshin    initial version no attenuation                          */
/* 1.01    Jul 2006  S. Manoshin    sphrerical surfaces and  attenuation are addded         */
/* 1.10    Jul 2006  S. Manoshin    movement algorithm improoved, refract bug is fixed	    */
/* 1.11    Nov 2006  S. Manoshin    parabolic lense is added, negative radius is allowed    */
/* 1.12    Nov 2006  S. Manoshin    Lenses can have the different parameters                */
/* 1.13    Feb 2007  S. Manoshin    some bugs fixed, tests are sucssesful                   */
/* 1.14	   Feb 2007  S. Manoshin    focal distance calculations are added, auto-flight	    */
/* 1.15    Feb 2007  S. Manoshin    ray-tracing visualisation after lenses added            */
/* 1.16    Mar 2007  S. Manoshin    fixed bug with refraction at the second lense surface   */
/* 1.17    Jun 2007  S. Manoshin    ray-tracing visualisation after lenses is improved	    */
/* 1.18    Jun 2007  S. Manoshin    planes are added as refractive surface with par and sph */
/* 1.19    Jul 2007  S. Manoshin    scattering cross section is added to the absorption CS  */
/* 1.20    Sep 2009  M. Fromme      adopted to newer vitess environment                     */  
/* 1.21    Apr 2020  K. Lieutenant  tidy up, new central visualization parameters           */
/********************************************************************************************/

#include "softabort.h"
#include "lense.h"
#include "matrix.h"


/******************************/
/** Global Variables         **/
/******************************/
McCompID _eModule=MCN_LENSE;
//                             1    2      3    4     5    6     7      8     9      10      11      12     13     14    15
const char *MaterialName[] = {"O", "CO2", "C", "Be", "F", "Bi", "MgO", "Pb", "MgF", "SiO2", "ZrO2", "Mg",  "Si",  "Zr", "Al"};
const char *GraphDev= "/xs"; /* for PGPLOT */

FILE*  COLLFILE=NULL;       //           pointer to the output file

const char *COLLFILEName="lensestrj.dat";   
                            // -p        name of the output file

long   NumberOfLenses=1;    // -I        number of lenses
long   MaterialOfLense=10;  // -i        enum: material of the lense (default 10: SiO2)
long   Attenkey      =0;    // -H        flag: attenuation inside the lenses (0: no  1: yes)
long   LenseType     =0;    // -K        enum: type of the lense (0: spherical, 1: parabolic) 
long   ServiceInfoK  =0,    // -z        flag: write coordinates in the file for the lense (0: no, 1: yes)
       LenseForOut   =0;    // -v        number of lenses used for the output file (0: all)
long   keyfocusflight=0;    // -Y        flag: Flight after lense until focus distance
long   keyfocusflightcalc=0;// -V        enum: choose the formula for focal distance flight after lense (0: thin  1: thick)

long   keyVisAct     =0,    // -y        flag: activation of visualization 
       LenseForOutVis=0;    // -E        number of lenses used for visualization
long   keyvisoutput  =0;    // -l        enum for Unix/Linux: device for output (0: xwin  1:ps file)
long   keyraytraceAL =0;    // -W        flag: visualization of flight paths after lense  (0: no, 1: yes)
long   raytraceALnum=10000; // -x        number of trajectories for ray-tracing after lense

double Xrtal      = 0.0;    // -S  [cm]  max. x-value for ray-tracing visualization after lenses

double Radius1    = 1.0;    // -a  [cm]  radius of the first lense surface 
double Radius2    = 1.0;    // -b  [cm]  radius of the second lense surface 
double RadiusMain = 1.0;    // -c  [cm]  radius of the lense 
double Thickness  = 1.0;    // -A  [cm]  thickness of the lense along ox axis 
double surfacerough=0.0;    // -q  [deg] surface roughness of the lense
double RefractInput=1.0;    // -R   [-]  User defined refractive index 
double RefractWave =1.0;    // -C  [Ang] wavelength for the user-defined refactive index
double AttenInput  =1.0;    // -D [1/cm] user defined macroscopic absorption cross section of the lense 
double AttScattering=0.0;   // -Q [1/cm] user defined macroscopic scattering cross section of the lense 
double DiafRadius1 =0.0;    // -m  [cm]  inner radius of diaphragm at the exit of the lense system
double DiafRadius2 =0.0;    // -M  [cm]  outer radius of diaphragm at the exit of the lense system
double WaveCalc   =20.0;    // -r  [Ang] Wavelength for focal distance calculation

VectorType 
PosMain ={0.0, 0.0, 0.0}, // -d -e -k  [cm] center position of the lense 
TransOut={0.0, 0.0, 0.0}; // -s -t -w  [cm] position of the output frame

double Materials[16];       // database with refractive indexes
double Materialsz[16];      // database for attenuation indexes, mu

double Refract=1.0;         // Calculated Refraction coeff for the lense material 
double Atten  =1.0;         // Calculated Attenuation coeff for the lense material

LenseSecond MyLense;
Plane       Endpoint;       // plane for flight after lense


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and sets global parameters
void  CalcAndWritePar();                    // calculates arrays from input parameters and writes to log file
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization    missing


/******************************/
/**      MAIN Program        **/
/******************************/
int main(int argc, char *argv[])
{
  long	i,j;
  long  CurrentLense; 
  long  NeutronLoss = 0; /* key for neutron loss */
  long  keyraytraceALoff=0; /* additional key for disactivation of ray-tracing */

  double temp1;     /* temporary variable */
  double TimeOF1, TimeOF1t; /* tof variables */
  double TimeOFspace; /* time of flight after lense */

  Neutron  Output, OutputRTAL;

  static long  idwin1, idwin2;

  /* DATEBASE: Index delta (for wavelength 1.8 Angs) for diff. materials.
     C.D. Dewhurst and I. Anderson, ILL
     Source: http://www.ill.fr/AR-01/p-104.htm				
     Refractive index can be calculated as (1-delta)
     Recalculation for other wavelengths: delta_new = wavelength^2*delta/(1.8^2)
     according:                           delta =(wavelength)^2*(N*Bcoh)/2*PI
  */

  Materials[1] = 1.28e-6;  Materialsz[1] = 0.00425; /*O*/
  Materials[2] = 1.60e-6;  Materialsz[2] = 0.0191;  /*CO2*/
  Materials[3] = 3.88e-6;  Materialsz[3] = 0.051;   /*C*/
  Materials[4] = 4.95e-6;  Materialsz[4] = 0.116;   /*Be*/
  Materials[5] = 1.02e-6;  Materialsz[5] = 0.0365;  /*F*/
  Materials[6] = 1.23e-6;  Materialsz[6] = 0.118;   /*Bi*/
  Materials[7] = 3.62e-6;  Materialsz[7] = 1.28;    /*MgO*/
  Materials[8] = 1.60e-6;  Materialsz[8] = 0.573;   /*Pb*/
  Materials[9] = 1.24e-6;  Materialsz[9] = 0.471;   /*MgF*/
  Materials[10]= 1.05e-6;  Materialsz[10]= 0.441;   /*SiO2*/ /* DEFAULT */
  Materials[11]= 1.60e-6;  Materialsz[11]= 0.812;   /*ZrO2*/
  Materials[12]= 1.19e-6;  Materialsz[12]= 0.615;   /*Mg*/
  Materials[13]= 1.06e-6;  Materialsz[13]= 0.796;   /*Si*/
  Materials[14]= 1.55e-6;  Materialsz[14]= 1.2;     /*Zr*/
  Materials[15]= 1.06e-6;  Materialsz[15]= 1.38;    /*Al*/

  /* attenuation indexes mu[m^-1] for 1.8 Ang, same source
     mu  = 4*PI*beta/lambda

     GENERAL REFRACTION COEFF: N = 1 - DELTA - I*BETA
     WHERE DELTA AND BETA ARE DESCRIBED ABOVE	I = sqrt(-1);
  */

  // initialisation
  // --------------
	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.23");
	OwnInit(argc, argv);

  bVisInstalled = FALSE;    // needs to be done still
  if (bVisInstr) 
    bLengthCmpr = TRUE;

  CalcAndWritePar();

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  while (ReadNeutrons()) 
  {
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK

      TimeOF1 = 0.0;
      TimeOF1t = 0.0;
      NeutronLoss = 0;
      InputNeutrons[i].Position[0] = 0.0;

      /* cycle for calculation of a lot of lenses */
      for(j = 1; j <= NumberOfLenses; j++)
      {
        CurrentLense = j;

        TimeOF1 = PathThroughLenseOrder2(&InputNeutrons[i], MyLense, Radius1, Radius2, RadiusMain,
	                                        Thickness, Refract, Atten, AttScattering, PosMain, TransOut, wei_min, surfacerough, keygrav, NeutronLoss,
	                                        Attenkey, CurrentLense, LenseForOut, LenseForOutVis,  ServiceInfoK, COLLFILE, LenseType);
        if (TimeOF1 == -1.0)
        {
          NeutronLoss = 1;
        }

        TimeOF1t = TimeOF1t + TimeOF1;
      }
      /* end cycle a lot of lenses */

      /* exclude such neutron */
      if (TimeOF1 == -1.0)  continue;
      if (NeutronLoss == 1) continue;
      if (InputNeutrons[i].Probability <= wei_min) continue;

      /* transform into output frame */
      InputNeutrons[i].Position[1] = InputNeutrons[i].Position[1] - TransOut[1];
      InputNeutrons[i].Position[2] = InputNeutrons[i].Position[2] - TransOut[2];

      /* Copy */
      Output = InputNeutrons[i];
      /****************************************************************************************/
      /* Add the time needed to travel inside Lense.                                   */
      /****************************************************************************************/
      Output.Time += TimeOF1t;
      /****************************************************************************************/
      /* Count this as a success.                                                             */
      /****************************************************************************************/
      OutputRTAL = InputNeutrons[i];

      /* activate diaphragm, if necessary */
      keyraytraceALoff = 0;

      if ((DiafRadius1 > 0.0)||(DiafRadius2 > 0.0))
      {
        temp1 =  InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1]
               + InputNeutrons[i].Position[2]*InputNeutrons[i].Position[2];
        temp1 = sqrt(temp1);
        if ((temp1 < DiafRadius1)||(temp1 > DiafRadius2))
        {
          keyraytraceALoff = 1;
          continue;
        }
      }

      /* Make ray-tracing visualisation after lenses */
  #ifdef VT_GRAPH

      if (do_visualise)
      {
        if (raytraceALcur <= raytraceALnum)
        {
          if ((keyraytraceAL == 1)||(keyraytraceAL == 2))
          {
            raytraceALcur++;

            if (keyraytraceAL == 1) raytryz = OutputRTAL.Position[2];
            if (keyraytraceAL == 2) raytryz = OutputRTAL.Position[1];

            raytracecolor = (long)(raytryz/(RadiusMain/7.0));

            cpgslct(idwin2);
            cpgsci(1+abs(raytracecolor));

            if (keyraytraceAL == 1) /* XZ */
	          {
              cpgpt1(OutputRTAL.Position[0], OutputRTAL.Position[2], -2);
	          }

            if (keyraytraceAL == 2) /* XY */
	          {
              cpgpt1(OutputRTAL.Position[0], OutputRTAL.Position[1], -2);
	          }

            /* Additional raytracing after lenses */
            if (keygrav == 1)
	          {
              tmptmp = NeutronPlaneIntersectionGrav(&OutputRTAL, EndpointRTAL);
	          }
            else
	          {
              tmptmp = NeutronPlaneIntersection1(&OutputRTAL, EndpointRTAL);
	          }

            if (keyraytraceAL == 1) /* XZ */
	          {
              cpgdraw(OutputRTAL.Position[0], OutputRTAL.Position[2]);
            }

            if (keyraytraceAL == 2) /* XY */
	          {
              cpgdraw(OutputRTAL.Position[0], OutputRTAL.Position[1]);
	          }

            cpgslct(idwin1);
          }
        }
      }
    #endif

      /* Additional flight on focus distance according analytical calculations */
      if (keyfocusflight == 1)
      {
        if (Output.Vector[0] <= 0.0) continue;

        TimeOFspace = 0.0;
        if (keygrav == 1)
        {
          TimeOFspace = NeutronPlaneIntersectionGrav(&Output, Endpoint);
          if (TimeOFspace < 0.0) continue;
        }
        else
        {
          TimeOFspace = NeutronPlaneIntersection1(&Output, Endpoint);
          if (TimeOFspace < 0.0) continue;
        }

        Output.Time = Output.Time + TimeOFspace;
        Output.Position[0]=0.0;
      }

      WriteNeutron(&Output);
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
  Cleanup(PosMain[0], PosMain[1], PosMain[2], 0.0, 0.0);

  if( ServiceInfoK == 1 ) fprintf(COLLFILE, "\n");
  if( ServiceInfoK == 1 ) fclose(COLLFILE);

  fprintf(LogFilePtr,"\n");

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
      case 'a':
        Radius1  =  atof(arg);
        break;
      case 'b':
        Radius2  =  atof(arg);
        break;
      case 'c':
        RadiusMain  =  atof(arg);
        break;

      case 'A':
        Thickness  =  atof(arg);
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
        PosMain[0]  =  atof(arg); /* Posmain0: center of the lense x */
        break;
      case 'e':
        PosMain[1]  =  atof(arg); /* Posmain1: center of the lense y */
        break;
      case 'k':
        PosMain[2]  =  atof(arg); /* Posmain2: center of the lense z */
        break;

      case 'i':
        MaterialOfLense  = atol(arg); /* values [1...15] */
        break;

      case 'q':
        surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degree */
        surfacerough  *= M_PI/180.0; /*Convert from degree to radian */
        surfacerough  =  tan(surfacerough);
        break;

      case 'R':
        RefractInput  =  atof(arg);
        break;

      case 'C':
        RefractWave  =  atof(arg);
        break;

      case 'D':
        AttenInput  =  atof(arg);
        break;

      case 'Q':
        AttScattering  =  atof(arg);
        break;

      case 'I':
        NumberOfLenses = atol(arg);
        break;

      case 'H':
        Attenkey = atol(arg);
        break;

      case 'K':
        LenseType = atol(arg);
        break;

      case 'm':
        DiafRadius1 = atof(arg);
        break;

      case 'M':
        DiafRadius2 = atof(arg);
        break;

      case 'y':
        keyVisAct = atol(arg);   /* for visualiztion activation */
  #ifdef VT_GRAPH
        do_visualise = keyVisAct != 0;
  #endif
        break;

      case 'z':
        ServiceInfoK = atol(arg);
        break;

      case 'v':
        LenseForOut = atol(arg);
        break;

      case 'p':
        COLLFILEName = arg;
        break;

      case 'E':
        LenseForOutVis = atol(arg);
        break;

      case 'l':
        keyvisoutput = atol(arg);
        break;

      case 'S':
        Xrtal =  atof(arg);
        break;

      case 'W':
        keyraytraceAL = atol(arg);
        break;

      case 'x':
        raytraceALnum = atol(arg);
        break;

      case 'r':
        WaveCalc =  atof(arg);
        break;

      case 'Y':
        keyfocusflight = atol(arg);
        break;

      case 'V':
        keyfocusflightcalc = atol(arg);
        break;

      default:
        fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n", a);
        exit(-1);
        break;
    }
  }
}


/********************************************************************/
/** calculates arrays from input parameters and writes to log file **/
/********************************************************************/
void  CalcAndWritePar()
{
  double   FocalLength; /* Calculated analytical focal length */
  double   FocalLength_thin; /* Calculated analytical focal length for thin lense */
  double   RadiusT; 
  double   Radius1_c, Radius2_c, Refract_c; /* For analytical calculation */
  double   XCEN1, XCEN2;
  double   Par1, Par2, Shift1, Shift2, Sign1=0, Sign2; /* additional pars for describing */

  if (ServiceInfoK == 1)
  {
    fprintf(LogFilePtr,"Output the cartesian coordinates of neutrons in the file for lense:  %ld\n", LenseForOut);

    if (LenseForOut == 0)
    {
      fprintf(LogFilePtr,"Output neutrons for all lenses (if it more than one)!\n");
    }
    if (LenseForOut < 0)
    {
      fprintf(LogFilePtr,"Number of the lense for output < 0 : exit! Correct option -v\n");
      exit(-1);
    }
    if (LenseForOut > NumberOfLenses)
    {
      fprintf(LogFilePtr,"Number of the lense is more than general number_of_lenses : exit! Correct option -v\n");
      exit(-1);
    }

    COLLFILE=OpenOutputFile(COLLFILEName, FALSE, "w");
  }

#ifdef VT_GRAPH
  if (do_visualise)
  {
    if (LenseForOutVis == 0)
    {
      fprintf(LogFilePtr,"Visualisation of neutrons for all lenses (if it more than one)!\n");
    }

    if (LenseForOutVis < 0)
    {
      fprintf(LogFilePtr,"Number of the lense for visualisation < 0 : exit! Correct option -E\n");
      exit(-1);
    }

    if (LenseForOutVis > NumberOfLenses)
    {
      fprintf(LogFilePtr,"Number of the lense is more than general number_of_lenses : exit! Correct option -E\n");
      exit(-1);
    }
  }
#endif

  if(keygrav == 1)
  {
    fprintf(LogFilePtr,"Lense: Gravity is enabled\n");
  }
  else
  {
    fprintf(LogFilePtr,"Lense: Gravity is disabled\n");
  }

  if (LenseType == 0) fprintf(LogFilePtr,"Lense surfaces have spherical form\n");
  if (LenseType == 1) fprintf(LogFilePtr,"Lense surfaces have parabolic  form\n");
  if ((LenseType < 0)||(LenseType > 1))
  {
    fprintf(LogFilePtr,"No such geometry form of the lense\n");
    exit(-1);
  }

  if (NumberOfLenses <= 0.0)
  {
    fprintf(LogFilePtr,"Error: Number of lenses is <= 0.0!!! Correct option -I\n");
    exit(-1);
  }
  fprintf(LogFilePtr,"Number of lenses: %ld\n", NumberOfLenses);

  if (WaveCalc <= 0.0)
  {
    fprintf(LogFilePtr,"Wavelength %f Angs for calculations <= 0.0, correct the option -r\n", WaveCalc);
    exit(-1);
  }

  if (NumberOfLenses >= 30)
  {
    fprintf(LogFilePtr,"Error: Number of lenses is >= 30. Use with care!!!\n");
  }

  if (LenseType == 0)
  {

    fprintf(LogFilePtr,"First radius = %f cm Second radius = %f cm Main radius = %f cm\n", Radius1, Radius2, RadiusMain);
    fprintf(LogFilePtr,"Thickness of the lense = %f  cm\n", Thickness);

    /* check data */

    if (Radius1 == 0.0)
	  {
	    fprintf(LogFilePtr,"Warning! Instead sphere the plane will take place for the first surface of the lenses!\n");
  	}

    if (Radius2 == 0.0)
	  {
	    fprintf(LogFilePtr,"Warning! Instead sphere the plane will take place for the second surface of the lenses!\n");
	  }

    if (RadiusMain <= 0.0)
	  {
	    fprintf(LogFilePtr,"Error: main radius of the lense <= 0.0!!! Correct option -c\n");
	    exit(-1);
	  }

    if (Thickness <= 0.0)
	  {
	    fprintf(LogFilePtr,"Error: Thickness of the lense <= 0.0!!! Correct option -c\n");
	    exit(-1);
	  }

    if (Thickness < 1.0)
	  {
	    fprintf(LogFilePtr,"WARNING! The thickness of the lense along main optical axis is SMALL (< 1.0 cm)! Please change it.\n");
	  }

    if (Thickness < 0.5)
	  {
	    fprintf(LogFilePtr,"WARNING! The thickness of the lense along main optical axis is SMALL (< 0.5 cm )! Please change it.\n");
	  }
  }


  if (LenseType == 1)
  {
    fprintf(LogFilePtr,"Parabolic deep  %f cm  %f cm  %f  cm\n",Radius1, Radius2, RadiusMain);
    fprintf(LogFilePtr,"Thickness of the lense %f  cm\n", Thickness);

    /* check data */

    if (Radius1 == 0.0)
    {
      fprintf(LogFilePtr,"Warning! Instead paraboloid the plane will take place for the first surface of the lenses!\n");
    }

    if (Radius2 == 0.0)
    {
      fprintf(LogFilePtr,"Warning! Instead paraboloid the plane will take place for the second surface of the lenses!\n");
    }

    if (RadiusMain <= 0.0)
    {
      fprintf(LogFilePtr,"Error: main radius of the lense <= 0.0!!! Correct option -c\n");
      exit(-1);
    }

    if (Thickness <= 0.0)
    {
      fprintf(LogFilePtr,"Error: Thickness of the lense <= 0.0!!! Correct option -c\n");
      exit(-1);
    }

    if (Thickness < 1.0)
    {
      fprintf(LogFilePtr,"WARNING! The thickness of the lense along main optical axis is SMALL (< 1.0 cm)! Please change it.\n");
    }

    if (Thickness < 0.5)
    {
      fprintf(LogFilePtr,"WARNING! The thickness of the lense along main optical axis is SMALL (< 0.5 cm )! Please change it.\n");
    //		exit(-1);
    }
  }


  /* Display material */
  if (MaterialOfLense >= 1 && MaterialOfLense <= 15)
  {
    fprintf(LogFilePtr, "Material of lense:   %s\n", MaterialName[MaterialOfLense - 1]);
    fprintf(LogFilePtr, "Reading data from DATABASE\n");
  }

  else if (MaterialOfLense == 99)
  {
    /* Material of lense is given by user */
    fprintf(LogFilePtr,"Refraction coeff is given by user\n");
    fprintf(LogFilePtr,"Coeff delta = %11.7E   for Wavelength  =  %lf  Angs\n", RefractInput, RefractWave);

    fprintf(LogFilePtr,"Attenuation (absorption part) coeff is given by user\n");
    fprintf(LogFilePtr,"Coeff mu = %11.7E 1/cm  for Wavelength = %lf  Angst\n", AttenInput, RefractWave);

    /* Check the correctness of given datas */

    if (RefractWave <= 0.0)
    {
      fprintf(LogFilePtr,"Wavelength for refraction coeff <= 0.0!\n");
      exit(-1);
    }
    if (RefractInput <= 0.0)
    {
      fprintf(LogFilePtr,"Refraction coeff <= 0.0!\n");
      exit(-1);
    }

    if (AttenInput < 0.0)
    {
      fprintf(LogFilePtr,"Attenuation coeff < 0.0!\n");
      exit(-1);
    }
  }
  else
  {
    fprintf(LogFilePtr,"No such material in the database. Values 1...15. Correct option -i\n");
    exit(-1);
  }

  /* Calculate delta  */
  if (MaterialOfLense == 99)
  {
    if (RefractInput <= 1e-50)
    {
      fprintf(LogFilePtr,"Refract Input value to small, SET to zero NO REFRACTION!\n");
      RefractInput = 0.0;
    }
    /* given by user and recalculated for 1.0 angst. */
    Refract = RefractInput/RefractWave/RefractWave;
    /* given by user and recalculated for 1.0 angst. */
    Atten = AttenInput/RefractWave;
  }
  else
  {
    /* from database for 1.8 angst */
    Refract = Materials[MaterialOfLense];
    /* recalcaculate for 1 angst  */
    Refract = Refract/(1.8*1.8);
    /* from database for 1.8 angst and recalculated cm^-1 */
    Atten =  0.01*Materialsz[MaterialOfLense]/(1.8);
  }

  /*     C.D. Dewhurst and I. Anderson, ILL
	 Source: http://www.ill.fr/AR-01/p-104.htm				*/

  fprintf(LogFilePtr,"!!! CALCULATATED delta=(1-refraction coeff) = %11.7E FOR wavelength 1.0 Ang\n", Refract);
  fprintf(LogFilePtr,"!!! CALCULATATED refraction coeff = %11.7E FOR wavelength 1.0 Ang\n", (1.0-Refract));


  /* calculate focal distance analytically */

  FocalLength = 0.0;
  FocalLength_thin = 0.0;

  if ((Radius1 != 0.0)&&(Radius2 != 0.0))
  {
    if (LenseType == 0)
    {
      Radius1_c = -1.0*Radius1;  /* adapt datas for the formula */
      Radius2_c = Radius2;       /* adapt datas for the formula */
      Refract_c = 1.0 - Refract;  /* adapt datas for the formula */
      FocalLength_thin = (Refract_c-1.0)*((1.0/Radius1_c) - (1.0/Radius2_c));  /* thin lense */
      if (FocalLength_thin != 0.0)
      {
        FocalLength_thin = 1.0/FocalLength_thin/NumberOfLenses;
        FocalLength_thin = FocalLength_thin/WaveCalc/WaveCalc;
      }
      fprintf(LogFilePtr,"!!!!!!!\n!!!!!!! For spherical and thin lense: Focal length  %f cm (for %f Angs) and for Number of Lenses = %ld\n",
      FocalLength_thin, WaveCalc, NumberOfLenses);
    }

    if ((LenseType == 1)&&(Radius1 == Radius2))
	  {
	    RadiusT = (RadiusMain*RadiusMain)/(Radius1+Radius2); /* source C. Schoroer and B. Lengeler, PRL 94, 054802 (2005) */
	    FocalLength_thin = RadiusT/(2.0*Refract*NumberOfLenses);  /* thin lense */
	    FocalLength_thin = FocalLength_thin/WaveCalc/WaveCalc;
	    fprintf(LogFilePtr,"!!!!!!! For parabolic and thin lense: radius of curvature = %f cm\n", RadiusT);
	    fprintf(LogFilePtr,"!!!!!!! For parabolic and thin lense: Focal length  %f cm (for %f Angs) and for Number of Lenses = %ld\n",
		                     FocalLength_thin, WaveCalc, NumberOfLenses);
	  }

    if (LenseType == 0)
    {
      Radius1_c = -1.0*Radius1;  /* adapt datas for the formula */
      Radius2_c = Radius2;       /* adapt datas for the formula */
      Refract_c = 1.0 - Refract;  /* adapt datas for the formula */
      if (Refract_c != 0.0)
      { /* standart formula */
        FocalLength = (Refract_c-1.0) * ((1.0/Radius1_c) - (1.0/Radius2_c) + (((Refract_c-1.0)*Thickness)/(Radius1_c*Radius2_c*Refract_c)));
        if (FocalLength != 0.0)
        {
          FocalLength = 1.0/FocalLength/NumberOfLenses;
          FocalLength = FocalLength/WaveCalc/WaveCalc;
          fprintf(LogFilePtr,"!!!P!!!\n!!!P!!! For spherical lense: Focal length  %f cm (for %f Angs) and for Number of Lenses = %ld\n",
                             FocalLength, WaveCalc, NumberOfLenses);
        }
      }
    }


    if ((LenseType == 1)&&(Radius1 == Radius2))
    {
      RadiusT = (RadiusMain*RadiusMain)/(Radius1+Radius2);   /* source C. Schoroer and B. Lengeler, PRL 94, 054802 (2005) */
      Radius1_c = -1.0*RadiusT;  /* adapt data for the formula */
      Radius2_c = RadiusT;       /* adapt data for the formula */
      Refract_c = 1.0 - Refract; /* adapt data for the formula */
      if (Refract_c != 0.0)
      { /* standart formula */
        FocalLength = (Refract_c-1.0) * ((1.0/Radius1_c) - (1.0/Radius2_c) + (((Refract_c-1.0)*Thickness)/(Radius1_c*Radius2_c*Refract_c)));
        if (FocalLength != 0.0)
        {
          FocalLength = 1.0/FocalLength/NumberOfLenses;
          FocalLength = FocalLength/WaveCalc/WaveCalc;
          fprintf(LogFilePtr,"!!!P!!! For parabolic lense: radius of curvature = %f cm\n", RadiusT);
          fprintf(LogFilePtr,"!!!P!!! For parabolic lense: Focal length  %f cm (for %f Angs) and for Number of Lenses = %ld\n",
                             FocalLength, WaveCalc, NumberOfLenses);
        }
      }
    }
  }

  /* check the calculations of the focal distances by analytical formulas,
     disactivate flight after lenses if necessary */

  if ((LenseType == 1)&&(Radius1 != Radius2))
  {
    fprintf(LogFilePtr,"Parabolic lense surfaces have different heights\n");
    fprintf(LogFilePtr,"No analytical formulas are included, so no flight\n");
    fprintf(LogFilePtr,"after lenses. Flight is disactivated by the code\n");
    keyfocusflight = 0;
  }

  if ((FocalLength_thin == 0.0)&&(keyfocusflightcalc == 0))
  {
    fprintf(LogFilePtr,"FocalLength %f for thin lense is not calculated\n", FocalLength_thin);
    fprintf(LogFilePtr,"Flight after lenses is disactivated by the code\n");
    keyfocusflight = 0;
  }

  if ((FocalLength == 0.0)&&(keyfocusflightcalc >= 1))
  {
    fprintf(LogFilePtr,"FocalLength %f for thick lense is not calculated\n", FocalLength);
    fprintf(LogFilePtr,"Flight after lenses is disactivated by the code\n");
    keyfocusflight = 0;
  }

  if (keyfocusflight == 1)
  {
    if (keyfocusflightcalc == 0)
    {
      fprintf(LogFilePtr,"??? Thin lense ??? ACTIVATE flight after lenses on focal distance  %f  cm  for wavelength  %f Anst\n", FocalLength_thin, WaveCalc);
      /* calculate plane for flight */
      Endpoint.A = 1.0;
      Endpoint.B = 0.0;
      Endpoint.C = 0.0;
      Endpoint.D = -1.0*fabs(FocalLength_thin);
      fprintf(LogFilePtr,"??? Thin lense ??? Move neutron after lenses at the calculated focal distance %f cm\n", Endpoint.D);
    }
    else
    {
      fprintf(LogFilePtr,"??? Thick lense ??? ACTIVATE flight after lenses on focal distance  %f  cm  for wavelength  %f Anst\n", FocalLength, WaveCalc);
      /* calculate plane for flight */
      Endpoint.A = 1.0;
      Endpoint.B = 0.0;
      Endpoint.C = 0.0;
      Endpoint.D = -1.0*fabs(FocalLength);
      fprintf(LogFilePtr,"??? Thick lense ??? Move neutron after lenses at the calculated focal distance %f cm\n", Endpoint.D);
    }
  }

  if(Attenkey == 1)
  {
    fprintf(LogFilePtr,"Attenuation (absorption + scattering ) in lenses is activated\n");
    fprintf(LogFilePtr,"Absorption coeff %11.7E for 1 angst  and cm^-1\n", Atten);
    fprintf(LogFilePtr,"Scattering coeff %11.7E for all waves and cm^-1\n", AttScattering);
    if (AttScattering < 0.0)
    {
      fprintf(LogFilePtr,"ERROR! Scattering coeff %11.7E < 0.0.\n", AttScattering);
      exit(-1);
    }
    if (AttScattering == 0.0)
      fprintf(LogFilePtr,"Scattering coeff %11.7E = 0.0. No such part of scattering!\n", AttScattering);
  }
  else
  {
    fprintf(LogFilePtr,"No attenuation in lenses\n");
  }

  if ((DiafRadius1 < 0.0)||(DiafRadius2 < 0.0))
  {
    fprintf(LogFilePtr,"ILLEGAL parameters: Diafragm with inner radius1 = %f and outer radius2 = %f\n", DiafRadius1, DiafRadius2);
    exit(-1);
  }

  if (DiafRadius1 > DiafRadius2)
  {
    fprintf(LogFilePtr,"ERROR! Outer radius is smaller than the Inner radius of the diaphragm\n");
    exit(-1);
  }

  if ((DiafRadius1 > 0.0)||(DiafRadius2 > 0.0))
    fprintf(LogFilePtr,"Diafragm with inner radius1 = %f and outer radius2 = %f activated\n", DiafRadius1, DiafRadius2);


  if ((DiafRadius1 == 0.0)&&(DiafRadius2 == 0.0))
    fprintf(LogFilePtr,"NO Diafragm with inner radius1 = %f and outer radius2 = %f activated\n", DiafRadius1, DiafRadius2);


  /* DESCRIBE THE SPHRERICAL SURFACES OF A LENSE */

  if (LenseType == 0)
  {
    if (Radius1 != 0.0)
    {
      /* Calculate the center of surfaces of lense */
      XCEN1 =  PosMain[0] - Radius1 - 0.5*Thickness; /* for the first surf. */
      fprintf(LogFilePtr,"Center of the first circle XCEN1 = %f cm\n",  XCEN1);

      /* Describe the first surface of the lense */
      MyLense.Surf[0].A = 1.0;
      MyLense.Surf[0].B = -2.0*XCEN1;
      MyLense.Surf[0].C = 1.0;
      MyLense.Surf[0].D = -2.0*PosMain[1];
      MyLense.Surf[0].E = 1.0;
      MyLense.Surf[0].F = -2.0*PosMain[2];
      MyLense.Surf[0].W = XCEN1*XCEN1 + PosMain[1]*PosMain[1] +
      PosMain[2]*PosMain[2] - Radius1*Radius1;
      MyLense.Surf[0].P = 0.0;
      MyLense.Surf[0].Q = 0.0;
      MyLense.Surf[0].R = 0.0;

      /*  Describe the additional surface  */
      MyLense.Surf[4].A = 0.0;
      MyLense.Surf[4].B = 1.0;
      MyLense.Surf[4].C = 0.0;
      MyLense.Surf[4].D = 0.0;
      MyLense.Surf[4].E = 0.0;
      MyLense.Surf[4].F = 0.0;
      MyLense.Surf[4].W = -1.0*XCEN1;
      MyLense.Surf[4].P = 0.0;
      MyLense.Surf[4].Q = 0.0;
      MyLense.Surf[4].R = 0.0;
    }
    else
    {
      MyLense.Surf[0].A = 0.0;
      MyLense.Surf[0].B = 1.0;
      MyLense.Surf[0].C = 0.0;
      MyLense.Surf[0].D = 0.0;
      MyLense.Surf[0].E = 0.0;
      MyLense.Surf[0].F = 0.0;
      MyLense.Surf[0].W = - PosMain[0] + 0.5*Thickness;
      MyLense.Surf[0].P = 0.0;
      MyLense.Surf[0].Q = 0.0;
      MyLense.Surf[0].R = 0.0;
    }

    if (Radius2 != 0.0)
    {
      /* Calculate the center of surfaces of lense */
      XCEN2 =  PosMain[0] + Radius2 + 0.5*Thickness; /* for the second surf. */
      fprintf(LogFilePtr,"Center of the second circle XCEN2 = %f cm\n", XCEN2);

      /* Describe the second surface of the lense */
      MyLense.Surf[1].A = 1.0;
      MyLense.Surf[1].B = -2.0*XCEN2;
      MyLense.Surf[1].C = 1.0;
      MyLense.Surf[1].D = -2.0*PosMain[1];
      MyLense.Surf[1].E = 1.0;
      MyLense.Surf[1].F = -2.0*PosMain[2];
      MyLense.Surf[1].W = XCEN2*XCEN2 + PosMain[1]*PosMain[1] +
      PosMain[2]*PosMain[2] - Radius2*Radius2;
      MyLense.Surf[1].P = 0.0;
      MyLense.Surf[1].Q = 0.0;
      MyLense.Surf[1].R = 0.0;
    }
    else
    {
      MyLense.Surf[1].A = 0.0;
      MyLense.Surf[1].B = 1.0;
      MyLense.Surf[1].C = 0.0;
      MyLense.Surf[1].D = 0.0;
      MyLense.Surf[1].E = 0.0;
      MyLense.Surf[1].F = 0.0;
      MyLense.Surf[1].W = -1.0*(PosMain[0] + 0.5*Thickness);
      MyLense.Surf[1].P = 0.0;
      MyLense.Surf[1].Q = 0.0;
      MyLense.Surf[1].R = 0.0;
    }

    /* Describe the main cylindrical surface of the lense */
    MyLense.Surf[2].A = 0.0;
    MyLense.Surf[2].B = 0.0;
    MyLense.Surf[2].C = 1.0;
    MyLense.Surf[2].D = -2.0*PosMain[1];
    MyLense.Surf[2].E = 1.0;
    MyLense.Surf[2].F = -2.0*PosMain[2];
    MyLense.Surf[2].W = PosMain[1]*PosMain[1] +
    PosMain[2]*PosMain[2] - RadiusMain*RadiusMain;
    MyLense.Surf[2].P = 0.0;
    MyLense.Surf[2].Q = 0.0;
    MyLense.Surf[2].R = 0.0;

    /*  Describe exit surface  */
    MyLense.Surf[3].A = 0.0;
    MyLense.Surf[3].B = 1.0;
    MyLense.Surf[3].C = 0.0;
    MyLense.Surf[3].D = 0.0;
    MyLense.Surf[3].E = 0.0;
    MyLense.Surf[3].F = 0.0;
    MyLense.Surf[3].W = -1.0*TransOut[0];
    MyLense.Surf[3].P = 0.0;
    MyLense.Surf[3].Q = 0.0;
    MyLense.Surf[3].R = 0.0;
  }

  /* DESCRIBE THE PARABOLIC SURFACES OF A LENSE */

  if (LenseType == 1)
  {
    if (Radius1 != 0.0)
    {
      Par1 = RadiusMain/sqrt(fabs(Radius1)); /* for the first surf. */
      if (Par1 == 0.0) exit(-1.0);
      Sign1 = Radius1/fabs(Radius1); /* for the first surf. */
      Shift1 =  PosMain[0] - 0.5*Thickness; /* for the first surf. */

      /* Describe the first surface of the lense */

      MyLense.Surf[0].A = 0.0;
      MyLense.Surf[0].B = Sign1;
      MyLense.Surf[0].C = 1.0/(Par1*Par1);
      MyLense.Surf[0].D = -2.0*PosMain[1]/(Par1*Par1);
      MyLense.Surf[0].E = 1.0/(Par1*Par1);
      MyLense.Surf[0].F = -2.0*PosMain[2]/(Par1*Par1);
      MyLense.Surf[0].W = -1.0*Sign1*Shift1+((PosMain[1]*PosMain[1])/(Par1*Par1))
	      + ((PosMain[2]*PosMain[2])/(Par1*Par1));
      MyLense.Surf[0].P = 0.0;
      MyLense.Surf[0].Q = 0.0;
      MyLense.Surf[0].R = 0.0;
    }
    else
    {
      MyLense.Surf[0].A = 0.0;
      MyLense.Surf[0].B = 1.0;
      MyLense.Surf[0].C = 0.0;
      MyLense.Surf[0].D = 0.0;
      MyLense.Surf[0].E = 0.0;
      MyLense.Surf[0].F = 0.0;
      MyLense.Surf[0].W = - PosMain[0] + 0.5*Thickness;
      MyLense.Surf[0].P = 0.0;
      MyLense.Surf[0].Q = 0.0;
      MyLense.Surf[0].R = 0.0;
    }

    if (Radius2 != 0.0)
    {
      Par2 = RadiusMain/sqrt(fabs(Radius2)); /* for the second surf. */
      if (Par2 == 0.0) exit(-1.0);
      Sign2 = Radius2/fabs(Radius2); /* for the second surf. */
      Shift2 =  PosMain[0] + 0.5*Thickness; /* for the second surf. */
      /* Describe the second surface of the lense */

      MyLense.Surf[1].A = 0.0;
      MyLense.Surf[1].B = -1.0*Sign2;
      MyLense.Surf[1].C = 1.0/(Par2*Par2);
      MyLense.Surf[1].D = -2.0*PosMain[1]/(Par2*Par2);
      MyLense.Surf[1].E = 1.0/(Par2*Par2);
      MyLense.Surf[1].F = -2.0*PosMain[2]/(Par2*Par2);
      MyLense.Surf[1].W = Sign2*Shift2+((PosMain[1]*PosMain[1])/(Par2*Par2))
      + ((PosMain[2]*PosMain[2])/(Par2*Par2));
      MyLense.Surf[1].P = 0.0;
      MyLense.Surf[1].Q = 0.0;
      MyLense.Surf[1].R = 0.0;
    }
    else
    {
      MyLense.Surf[1].A = 0.0;
      MyLense.Surf[1].B = 1.0;
      MyLense.Surf[1].C = 0.0;
      MyLense.Surf[1].D = 0.0;
      MyLense.Surf[1].E = 0.0;
      MyLense.Surf[1].F = 0.0;
      MyLense.Surf[1].W = -1.0*(PosMain[0] + 0.5*Thickness);
      MyLense.Surf[1].P = 0.0;
      MyLense.Surf[1].Q = 0.0;
      MyLense.Surf[1].R = 0.0;
    }

    /* Describe the main cylindrical surface of the lense */
    MyLense.Surf[2].A = 0.0;
    MyLense.Surf[2].B = 0.0;
    MyLense.Surf[2].C = 1.0;
    MyLense.Surf[2].D = -2.0*PosMain[1];
    MyLense.Surf[2].E = 1.0;
    MyLense.Surf[2].F = -2.0*PosMain[2];
    MyLense.Surf[2].W = PosMain[1]*PosMain[1] +	PosMain[2]*PosMain[2] - RadiusMain*RadiusMain;
    MyLense.Surf[2].P = 0.0;
    MyLense.Surf[2].Q = 0.0;
    MyLense.Surf[2].R = 0.0;

    /*  Describe exit surface  */
    MyLense.Surf[3].A = 0.0;
    MyLense.Surf[3].B = 1.0;
    MyLense.Surf[3].C = 0.0;
    MyLense.Surf[3].D = 0.0;
    MyLense.Surf[3].E = 0.0;
    MyLense.Surf[3].F = 0.0;
    MyLense.Surf[3].W = -1.0*TransOut[0];
    MyLense.Surf[3].P = 0.0;
    MyLense.Surf[3].Q = 0.0;
    MyLense.Surf[3].R = 0.0;
  }

  if(surfacerough == 0.0)
  {
    fprintf(LogFilePtr,"The reflected surface is an ideal lense\n");
  }
  else
  {
    fprintf(LogFilePtr,"The reflected surface is rough\n");
    if (surfacerough < 0.0)
    {
      fprintf(LogFilePtr,"ERROR: Parameter surface roughness is negative!\n");
      exit(-1);
    }
    else
    {
      fprintf(LogFilePtr,"Parameter surface roughness = %f\n", surfacerough);
    }
  }
  fprintf(LogFilePtr,"Minimal weight for tracing neutron   %e\n", wei_min);

#ifdef VT_GRAPH
  if (do_visualise)
  {
  #ifdef DO_WIN32
    GraphDev = "lensestrjwin.ps";
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
    idwin1 = cpgopen(GraphDev);
    if (idwin1 < 1)
    {
	    fprintf(LogFilePtr,"cannot open first plot device\n");
	    exit(-1);
    }

    /* open additional window for ray-tracing after a lenses */
    if ((keyraytraceAL == 1)||(keyraytraceAL == 2))
  	{
      if ((Xrtal == 0.0)&&(FocalLength_thin == 0.0))
        Xrtal = 10.0*TransOut[0];
      if ((Xrtal == 0.0)&&(FocalLength_thin != 0.0))
        Xrtal = fabs(FocalLength_thin);

      idwin2 = cpgopen(GraphDev);
      if (idwin2 < 1)
      {
        fprintf(LogFilePtr,"cannot open second plot device\n");
        exit(-1);
      }
	  
      cpgslct(idwin2);

      if (keyraytraceAL == 1) /* XZ */
      {
        cpgenv(0.0, (1.5*Xrtal), -1.5*RadiusMain+PosMain[2]-TransOut[2], 1.5*RadiusMain+PosMain[2]-TransOut[2], 0, 0);
        cpgsfs(2);
        cpgsch(1.2);
        cpglab("X, cm ", "Z, cm", "RAY TRAYCING AFTER LENSES XZ - VERTICAL PLANE");
      }

      if (keyraytraceAL == 2) /* XY */
      {
        cpgenv(0.0, (1.5*Xrtal), -1.5*RadiusMain+PosMain[1]-TransOut[1], 1.5*RadiusMain+PosMain[1]-TransOut[1], 0, 0);
        cpgsfs(2);
        cpgsch(1.2);
        cpglab("X, cm ", "Y, cm", "RAY TRAYCING AFTER LENSES XY - HORIZONTAL PLANE");
      }

      EndpointRTAL.A = 1.0;
      EndpointRTAL.B = 0.0;
      EndpointRTAL.C = 0.0;
      EndpointRTAL.D = -1.45*Xrtal; /* Trace neutron more than focal distance */
    }

    fprintf(LogFilePtr,"VISUALISATION: Opening graphic device %s\n", GraphDev);

    cpgslct(idwin1);
    cpgenv(0.0, 1.5*TransOut[0], -2.0*RadiusMain+PosMain[2], 2.0*RadiusMain+PosMain[2], 0, 0);
    cpgsfs(2);
    cpgsch(1.2);

    /* Make the visualisation of the circle */
    if  (LenseType == 0)
    {
      cpglab("X, cm ", "Z, cm", "SPHERICAL Lense Visualisation");
      cpgsci(2);

      if (Radius1 != 0.0)
      {
        XCEN1 =  PosMain[0] - Radius1 - 0.5*Thickness; /* for the first surf. */
        cpgcirc(XCEN1 , PosMain[2], fabs(Radius1));
      }
      else
      {
        cpgmove(PosMain[0]-0.5*Thickness, -2.0*RadiusMain+PosMain[2]);
        cpgdraw(PosMain[0]-0.5*Thickness,  2.0*RadiusMain+PosMain[2]);
      }

      if (Radius2 != 0.0)
      {
        XCEN2 =  PosMain[0] + Radius2 + 0.5*Thickness; /* for the second surf. */
        cpgcirc(XCEN2 , PosMain[2], fabs(Radius2));
      }
      else
      {
        cpgmove(PosMain[0]+0.5*Thickness, -2.0*RadiusMain+PosMain[2]);
        cpgdraw(PosMain[0]+0.5*Thickness,  2.0*RadiusMain+PosMain[2]);
      }

      cpgmove(0.0, RadiusMain+PosMain[2]); cpgdraw(1.5*TransOut[0], RadiusMain+PosMain[2]);
      cpgmove(0.0, -1.0*RadiusMain+PosMain[2]); cpgdraw(1.5*TransOut[0], -1.0*RadiusMain+PosMain[2]);
    }


    if  (LenseType == 1)
    {
      cpglab("X, cm ", "Z, cm", "PARABOLIC Lense Visualisation");
      cpgsci(2);

      zmin = -1.0*RadiusMain+PosMain[2];
      zmax = RadiusMain+PosMain[2];
      zstep = fabs(zmax - zmin)/10000.0;

      if (Radius1 != 0.0)
      {
        for (zcur = zmin; zcur <= zmax; zcur = zcur + zstep)
        {
          xcur = zcur * (zcur*MyLense.Surf[0].E + MyLense.Surf[0].F) + MyLense.Surf[0].W;
          xcur = -1.0*Sign1*xcur;
          cpgpt1(xcur, zcur, -1);
        }
      }
      else
      {
        cpgmove(PosMain[0]-0.5*Thickness, -2.0*RadiusMain+PosMain[2]);
        cpgdraw(PosMain[0]-0.5*Thickness,  2.0*RadiusMain+PosMain[2]);
      }

      if (Radius2 != 0.0)
      {
        for (zcur = zmin; zcur <= zmax; zcur = zcur + zstep)
        {
          xcur = zcur * (zcur*MyLense.Surf[1].E + MyLense.Surf[1].F) + MyLense.Surf[1].W;
          xcur = 1.0*Sign1*xcur;
          cpgpt1(xcur, zcur, -1);
        }
      }
      else
      {
        cpgmove(PosMain[0]+0.5*Thickness, -2.0*RadiusMain+PosMain[2]);
        cpgdraw(PosMain[0]+0.5*Thickness,  2.0*RadiusMain+PosMain[2]);
      }

    cpgmove(0.0, RadiusMain+PosMain[2]); cpgdraw(1.5*TransOut[0], RadiusMain+PosMain[2]);
    cpgmove(0.0, -1.0*RadiusMain+PosMain[2]); cpgdraw(1.5*TransOut[0], -1.0*RadiusMain+PosMain[2]);
    }
  }
#endif
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  return;
}