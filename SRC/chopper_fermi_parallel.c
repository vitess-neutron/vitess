/*******************************************************************************************************************************
  VITESS module 'chopper_fermi'

 The free non-commercial use of these routines is granted providing due credit is given to
 the authors.

 1.00  Jul 2002  G. Zsigmond 	 initial version
 1.01  Aug 2002  G. Zsigmond	 forward all coordinates
 1.02  Sep 2002  G. Zsigmond	 included more channel windows
 1.03  Apr 2003  G. Zsigmond	 sign correction
 1.04  May 2003  G. Zsigmond	 info changed
 1.05  Jun 2003  G. Zsigmond	 modulo function included for safety
 1.06  Jul 2003  G. Zsigmond	 generalised for optional number of pulses; warnings included
 1.07  Oct 2003  G. Zsigmond	 superfluous modulo function cancelled
 1.08  Nov 2003  G. Zsigmond	 put 2 more windows representing channels, now 6 windows
                               in the big IF loop ">=" changed to ">"
 1.09  Jan 2004  K. Lieutenant changes for 'instrument.dat'
 1.10  Jan 2004  G. Zsigmond   back to 4 windows representing channels
 1.11  Apr 2004  G. Zsigmond   negative time of flight defined
 1.12  Apr 2004  G. Zsigmond   negative time of flight - corrections, set zero time
 1.13  May 2004  G. Zsigmond   circular geom option and channel length included in curved fc
 1.14  JUL 2004  G. Zsigmond   small change in Init to adapt to new GUI
 1.15  OCT 2004  G. Zsigmond   changed to use both even or odd number of channels
 1.16  MAY 2005  G. Zsigmond   output changed to give trajectory coordinates at a plane crossing the center of the chopper
                               (to be compatible with zero time option, zero time option fixed to get one peak, shadowing cylinder opening activated
 1.17  MAY 2005  G. Zsigmond   new option choice of 4, 6(better,slower) or 8(much better, very slow) gates, 4 gates option adjusted
 1.18  SEP 2005  G. Zsigmond   optimisations to speed up the algorithm
 1.19  JAN 2010  M. Fromme     thread parallelisation
 1.20  Feb 2021  K. Lieutenant tidy up, new central visualization parameters                
 1.21  Sep 2022  K. Lieutenant correction broad channels                
*****************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "general.h"
#include "init.h"
#include "intersection.h"
#include "threadHelper.h"


/*********************************/
/** Defines and Structures      **/
/*********************************/
#define	STRING_BUFFER 100
#define MAX_GATE       10
#define MAX_CHAN     2000


/*********************************/
/** Global and Static Variables **/
/*********************************/
// Input parameters
VtFermiType eFmOption
            =VT_NO_FERMI_TYPE;   //   -O     [-]   type of Fermi chopper: 1: straight   2: curved   (not used in VITESS 4 anymore)
VtChnlShape  eGeomOption
           =VT_NO_CHN_SHAPE;     //   -g     [-]   channel shape: 1: ideal   2:circular 
const char *GeomFileName=NULL;   //   -G     [-]   output file of the curved channel geometry 
double pos_chp[3]={0.0,0.0,0.0}, //-X,-Y,-V [cm]   center position x of the Fermi chopper
       height=0.0,               //   -a    [cm]   height of the Fermi chopper
       width=0.0,                //   -b    [cm]   width of the Fermi chopper
       depth=0.0;                //   -c    [cm]   hchannel length of the Fermi chopper
                                 
long	 Nchannels=1;              //   -l     [-]   number of channels, max 2000
double wallwidth=0.0,            //   -m    [cm]   thickness of the walls between the channels  
       diameter =0.0,            //   -r    [cm]   diameter of the shadowing cylinder
                                 
       omega=0.0,                //   -n  [rad/ms] rotational speed, input: [rps] 
       Phase=0.0,                //   -q    [rad]  dephasing angle at zero time input [deg]
       optimal_wl=0.0;           //   -L    [Ang]  optimal wavelength to be transmitted
                                 
int    GatesNumber=4,            //   -p     [-]   number of gates representing the channels (4, 6 or 8)
       zerotime=FALSE;           //   -z     [-]   flag: chopper sets neutron time close to zero (after the chopper)
                               
// Variables determined from input parameters or file data
FILE* GeomFilePtr=NULL;         //                Pointer to geometry output file
FILE* GatesFilePtr=NULL;        //                Pointer to output file "gates.dat"
double w_ch=0.0,                //         [cm]   width of each channel
       expon = 0.3333,          //                parameter dfining the distribution of gate positions
       y_ch[MAX_GATE][MAX_CHAN],//         [cm]   y-positions of all gates and channels
       x_ch[MAX_GATE][MAX_CHAN],//         [cm]   x-positions of all gates and channels
       main_depth=0.0,          //         [cm]   distance from entrance window to center of the Fermi chopper           
       coef_pi   =0.0,          //         [rad]  reduzierte Phase
       radius_of_curv=0.0,      //         [cm]   radius of the curved channels   
       angle_channel =0.0;      //         [rad]  angle of the curved channels   


/******************************/
/** Prototypes               **/
/******************************/
void		OwnInit(int argc, char *argv[]);     // Reads module specific input parameters and sets global parameters
void		OwnCleanup();                        // Does module specific cleanup
void    processNeutron(int i, int thread_i); // processes 1 neutron trajectory
void    SetGeometry   (char* sColor);        // Fills the structure stGeometry for visualization 

static int inPhase   (int gates, double phase0, const double WL, const VectorType Dir, const VectorType Pos);                       // Checks if neutron passes through all gates
static int outOfPhase(int i, int j, int gates, const double phase0, const double WL, const VectorType Dir, const VectorType Pos);   // Checks if neutron is out of one gate  
static double CalcPhase(const double x_ch_k_j, const double y_ch_k_j, const double WL, const VectorType Dir, const VectorType Pos); // Calculates the chopper phase for which the gate edge hits the neutron trajectory


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  /* Initialize the program according to the parameters given  */
  /* --------------------------------------------------------  */
  _eModule=MCN_CHOP_FERMI;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.21");
  OwnInit(argc, argv);

  bVisInstalled = MISSING;  // needs to be done still
  if (bVisInstr) 
    bLengthCmpr = FALSE;

  // loop over all trajectories
  // --------------------------
  processPipedNeutrons(NThreads, processNeutron, 1, 0); 

  // Finish: print parameters, write geometry and instrument file, free memory
  // -----------------------------------------------------
  OwnCleanup();

  fprintf(LogFilePtr," \n");

  Cleanup(pos_chp[0],pos_chp[1],pos_chp[2], 0.0,0.0);	

  return 0;
}


/************************************/
/** processes 1 neutron trajectory **/
/************************************/
void processNeutron (int i, int thread_i) 
{
  double     WL=0.0, TOF=0.0, phase0=0.0, 
             pos[3]={0.0,0.0,0.0}, n[3]={0.0,0.0,0.0};
  VectorType Pos={0.0,0.0,0.0}, Dir={0.0,0.0,0.0}, Path={0.0,0.0,0.0};
  Neutron neutron;

  InitNeutron(&neutron);

  if (IsEOB(&(InputNeutrons[i]))==TRUE)
  {
    WriteNeutronParallel(&(InputNeutrons[i]), thread_i);
  }
  else
  { 
    TOF = InputNeutrons[i].Time;
    WL  = InputNeutrons[i].Wavelength;

    CopyVector(InputNeutrons[i].Position, Pos);
    CopyVector(InputNeutrons[i].Vector, Dir);

    Dir[0] = sqrt(1 - sq(Dir[1]) - sq(Dir[2]));

    /* shift to center of Fermi-Chopper */
    SubVector(Pos, pos_chp);

    /*trajectories which do not intersect the entrance and exit window */
    n[0] = 1.;
    n[1] = n[2] = 0.;

    if (PlaneLineIntersect(Pos, Dir, n, - diameter/2., pos) != 1) return;

    if (pos[2] >= height/2.   || pos[2] <= - height/2. ||
        pos[1] >= diameter/2. || pos[1] <= - diameter/2.) return;

    if (PlaneLineIntersect(Pos, Dir, n, diameter/2., pos) != 1) return;

    if( pos[2] >= height/2.   || pos[2] <= - height/2. ||
        pos[1] >= diameter/2. || pos[1]<= - diameter/2.) return;	

    /* translates neutron variables for X'= - diameter/2.  */

    TOF += (- diameter/2. - Pos[0]) / fabs(Dir[0]) / V_FROM_LAMBDA(WL);
			
    if (TOF<0 && Nchannels==1)
      Error("Single-slit Fermi chopper needs positive flight time at the chopper position!");

    CopyVector(Dir, Path);
    MultiplyByScalar(Path, (- diameter/2. - Pos[0]) / Dir[0]);
    AddVector(Pos, Path);  /*	 Path = displacement vector */
								
    /* calculate time entering-edge and exiting-edge of gates along the channels */
    phase0 = fmod(Phase + omega*TOF, coef_pi*M_PI);

    /* return (get lost) if the neutron is out of phase */
    if (! inPhase(GatesNumber, phase0, WL, Dir, Pos)) 
    {
      /* try one turn earlier */
      if (phase0 > 0 && omega > 0) 
      {
        if (! inPhase(GatesNumber, phase0 - coef_pi*M_PI, WL, Dir, Pos))
          return;
      } 
      else if (phase0 < 0 && omega < 0) 
      {
        if (! inPhase(GatesNumber, phase0 + coef_pi*M_PI, WL, Dir, Pos))
          return;
      } 
      else
      { return;
      }
    }

    /* Output matters */
    /* transmit coordinates which were not changed, the rest overwrite below */
    neutron = InputNeutrons[i];

    /* translates neutron variables for output - X'= 0. . */
    neutron.Time = TOF + (- Pos[0]) / Dir[0] / V_FROM_LAMBDA(WL);
			
    if (zerotime==1)
    {
      neutron.Time = fabs(fmod(neutron.Time + Phase/omega + coef_pi*M_PI/omega/2., coef_pi*M_PI/omega))
                     - coef_pi*M_PI/2./omega ;
    }

    CopyVector(Dir, Path);
    MultiplyByScalar(Path, (- Pos[0])/ Dir[0] );
    AddVector(Pos, Path);                             /* Path = displacement vector */		
    CopyVector(Pos, neutron.Position);

    WriteNeutronParallel(&neutron, thread_i);
  }
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  const char *intForm = "%d";
  double shift_y=0.0; 
  int k,  // index of gates 
      j;  // index of channels

  GeomFileName="chopper_fermi_g.dat";

  for (k=0; k < MAX_GATE; k++)
  {
    for (j=0; j < MAX_CHAN; j++)
    {
      x_ch[k][j]=0.0;
      y_ch[k][j]=0.0;
    }
  }

  while(argc>1) 
  {
    char *arg = &argv[1][2];

    switch (argv[1][1])	
    {
      case 'O':
        eFmOption = (VtFermiType)atoi(arg);
        if (eFmOption==VT_FERMI_STR)  
          eGeomOption = VT_CHN_STR;   // parameter not read for straight Fermi chopper in Vitess 3
        break;

      case 'G':
        GeomFileName=arg;
        break;

      case 'X':
        sscanf(arg, "%lf", &pos_chp[0]);
        break;
      case 'Y':
        sscanf(arg, "%lf", &pos_chp[1]);
        break;
      case 'V':
        sscanf(arg, "%lf", &pos_chp[2]);
        break;

      case 'a':
        sscanf(arg, "%lf", &height);
        break;
      case 'b':
        sscanf(arg, "%lf", &width);
        break;
      case 'c':
        sscanf(arg, "%lf", &depth);
        break;

      case 'l':
        sscanf(arg, "%ld", &Nchannels);
        break;
      case 'm':
        sscanf(arg, "%lf", &wallwidth);
        break;
      case 'r':
        sscanf(arg, "%lf", &diameter);
        break;

      case 'n':
        sscanf(arg, "%lf", &omega);
        if(omega == 0) omega = 1.E-6;
        omega = omega * 2. * M_PI / 1000.;
        break;
      case 'q':
        sscanf(arg, "%lf", &Phase);
        Phase = Phase * M_PI / 180.;
        break;
      case 'L':
        sscanf(arg, "%lf", &optimal_wl);
        break;

      case 'p':
        sscanf(arg, intForm, &GatesNumber);
        if ((GatesNumber!=4)&&(GatesNumber!=6)&&(GatesNumber!=8))
          Error("Good options: 4, 6 or 8 gates");
        break;
      case 'z':
        sscanf(arg, intForm, &zerotime);
        break;
      case 'g':
        eGeomOption = (VtChnlShape) atoi(arg);
        if (eGeomOption==VT_CHN_STR)
          eFmOption = VT_FERMI_STR;    // parameter not read in Vitess 4
        else
          eFmOption = VT_FERMI_CURV;
        break;
    }
    argc--;
    argv++;
  }

  if (eFmOption==VT_NO_FERMI_TYPE || eGeomOption==VT_NO_CHN_SHAPE)
    Error("Chopper geometry not defined");

  if(pos_chp[0] < diameter/2.)
    Error("Minimum position is diameter/2");

  if(Nchannels==1) wallwidth=0.;

  GatesFilePtr = OpenOutputFile("gates.dat", FALSE, "w");

  // calculate edge positions
  // ------------------------  
  if (eFmOption==VT_FERMI_STR)
  {
    fprintf(LogFilePtr,"Straight Fermi chopper option activated\n");

    /* diameter matter */
    main_depth = 2. * sqrt(sq(diameter/2.) - sq(width/2.));
    if (depth > main_depth) 
    {	fprintf(LogFilePtr, "ERROR: Diameter too small - not compatible with 'channel length' and 'width'!\nTake min %f cm\n", 2. * sqrt(sq(depth/2.) + sq(width/2.)));	
      exit(-1);
    }

    {
      double add;
      long m;

      if (( w_ch = ( width - (Nchannels + 1) * wallwidth ) / Nchannels ) <= 0.)
        Error("Channel width =< 0");

      fprintf(LogFilePtr,"Channel width: %f cm\nChannels represented by %d gates.\n",w_ch, GatesNumber);

      for (k=0;k<GatesNumber; k++) 
        y_ch[k][0] = - width/2.;

      m = 1;

      for (j=1; j< 2*Nchannels+2; j++)
      {
        if(m == 1) add = wallwidth; else add = w_ch;

        for(k=0;k<GatesNumber/2; k++) 
        {
          x_ch[k][j]                   = -depth/2. * (1. - pow(2.*k/GatesNumber, expon)); 
          x_ch[GatesNumber - 1 - k][j] =  depth/2. * (1. - pow(2.*k/GatesNumber, expon));
        }

        for(k=0;k<GatesNumber; k++) 
          y_ch[k][j] = y_ch[k][j-1] + add;

        m = m * (-1);
      }

      for(k=0;k<GatesNumber/2; k++) 
      {
          x_ch[k][0] 
        = x_ch[k][1] 
        = x_ch[k][2*Nchannels] 
        = x_ch[k][2*Nchannels+1]  = -depth/2. * (1. - pow(2.*k/GatesNumber, expon)); 

          x_ch[GatesNumber - 1 - k][0] 
        = x_ch[GatesNumber - 1 - k][1] 
        = x_ch[GatesNumber - 1 - k][2*Nchannels] 
        = x_ch[GatesNumber - 1 - k][2*Nchannels+1] = depth/2. * (1. - pow(2.*k/GatesNumber, expon));
      }

      /* activating shadowing cylinder */
        x_ch[0][0] 
      = x_ch[0][1] 
      = x_ch[0][2*Nchannels] 
      = x_ch[0][2*Nchannels+1] = -main_depth/2.;

        x_ch[GatesNumber - 1][0] 
      = x_ch[GatesNumber - 1][1] 
      = x_ch[GatesNumber - 1][2*Nchannels] 
      = x_ch[GatesNumber - 1][2*Nchannels+1] = main_depth/2.;
	
		
      /* print out gate coordinates */
      if (GatesFilePtr!=NULL)
        for(j=0; j< 2*Nchannels+2; j++) 
          for(k=0;k<GatesNumber; k++) 
            fprintf(GatesFilePtr,"%f   %f  \n", x_ch[k][j] , y_ch[k][j]);
    }
  }

  if(eFmOption==VT_FERMI_CURV)
  {	
    if (eGeomOption==VT_CHN_IDEAL)
    {
      fprintf(LogFilePtr,"Curved Fermi chopper activated \n");
      fprintf(LogFilePtr,"Geometry option: ideally shaped (close to parabolic) long channels ('channel length' inactive parameter) \n");
      fprintf(LogFilePtr,"Radius of curvature (parabolic approximation):\n" 
                         "%10.3f cm at center\n" 
                         "%10.3f cm at circumference\n", V_FROM_LAMBDA(optimal_wl)/2./omega, V_FROM_LAMBDA(optimal_wl)/2./omega*pow((1 + sq(omega*diameter/V_FROM_LAMBDA(optimal_wl))), 1.5));

      GeomFilePtr = OpenOutputFile(GeomFileName, FALSE, "w");
      {
        double add, xx[500], yy[500], tt;
        long   m;

        if ((w_ch = ( width - (Nchannels + 1L) * wallwidth ) / Nchannels    ) <= 0.)
          Error("Channel width =< 0 !"); 

        fprintf(LogFilePtr,"Channel width : %12.5f cm\nChannels represented by %d gates.\n", w_ch, GatesNumber);

        y_ch[0][0] =  - width/2.;
        x_ch[0][0] = sqrt(sq(diameter/2.) - sq(y_ch[0][0]));

        /* the circle */
        for (k=0; k < 500; k++)
        { tt = k /500. * 2 * M_PI; xx[k] = diameter/2. * cos(tt); yy[k] = diameter/2. * sin(tt); 
          if (GeomFilePtr!=NULL)
            fprintf(GeomFilePtr, " %d %lf  %lf\n", 0, xx[k], yy[k]);
        }

        m = 1;

        for (j=1; j< 2*Nchannels+2; j++)
        {
          if(m == 1) add = wallwidth; else add = w_ch;
							
          y_ch[0][j] = y_ch[0][j-1] + add;
          x_ch[0][j] = - sqrt(sq(diameter/2.) - sq(y_ch[0][j]));

          m = m * (-1);
        }

        for (j=0; j< 2*Nchannels+2; j++)
        {							
          for (k=0; k<500; k++)
          {
            tt= k /499. * 2. * fabs(x_ch[0][j])/V_FROM_LAMBDA(optimal_wl);
            xx[k] = y_ch[0][j] * sin(omega*tt) + (V_FROM_LAMBDA(optimal_wl)*tt - fabs(x_ch[0][j])) * cos(omega*tt);
            yy[k] = y_ch[0][j] * cos(omega*tt) - (V_FROM_LAMBDA(optimal_wl)*tt - fabs(x_ch[0][j])) * sin(omega*tt);

            if (GeomFilePtr!=NULL)
              fprintf(GeomFilePtr, "%ld   %lf  %lf\n", j, xx[k], yy[k]);		
          }

          for (k=0;k<GatesNumber/2; k++) 
          { int index;
										
            index = (int)(pow(2.*k/GatesNumber, expon)*250.);
            x_ch[k][j] = xx[index];
            y_ch[k][j] = yy[index];
            x_ch[GatesNumber - 1 - k][j] = xx[499 - index];
            y_ch[GatesNumber - 1 - k][j] = yy[499 - index];
          }
        }

        /* print out gate coordinates */
        if (GatesFilePtr!=NULL)
          for (j=0; j< 2*Nchannels+2; j++) 
            for (k=0;k<GatesNumber; k++) 
              fprintf(GatesFilePtr,"%f   %f  \n", x_ch[k][j] , y_ch[k][j]);
      }
    }

    if (eGeomOption==VT_CHN_CIRC)
    {
      /* diameter matter */
      main_depth = 2. * sqrt(sq(diameter/2.) - sq(width/2.));
      if(depth > main_depth) 
      { fprintf(LogFilePtr,"ERROR: Diameter too small - not compatible with 'channel length' and 'width'!\nTake min %f cm\n", 2. * sqrt(sq(depth/2.) + sq(width/2.))); 
        exit(-1);
      }
      fprintf(LogFilePtr,"Curved Fermi chopper activated \n");
      fprintf(LogFilePtr,"Geometry option: circular shaped channels with fixed length (via 'channel length') \n");

      radius_of_curv = V_FROM_LAMBDA(optimal_wl)/2./omega;
      angle_channel  = atan(depth/2./radius_of_curv);

      fprintf(LogFilePtr,"Radius of curvature : %10.3f cm \nAngle of the channel: %10.3f deg \n", radius_of_curv, 180./M_PI*angle_channel);

      GeomFilePtr = OpenOutputFile(GeomFileName, FALSE, "w");
      {
        double add, xx[500], yy[500], tt;
        long   m;

        if (( w_ch = ( width - (Nchannels + 1) * wallwidth ) / Nchannels ) <= 0.)
        { fprintf(LogFilePtr,"Channel width =< 0 !\n");
          exit(-1);
        }

        fprintf(LogFilePtr,"Channel width     : %12.5f cm\nChannels represented by %d gates.\n",w_ch, GatesNumber);

        for (k=0; k<500; k++)
        { tt = k /500. * 2 * M_PI; 
          xx[k] = diameter/2. * cos(tt); 
          yy[k] = diameter/2. * sin(tt); /* the circle */
          if (GeomFilePtr!=NULL)
            fprintf(GeomFilePtr, " %d %lf  %lf\n", -1, xx[k], yy[k]);
        }

        m = 1;

        for (j=0; j< 2*Nchannels+2; j++)
        {
          if (m == 1) add = wallwidth; else add = w_ch;
								
          for(k=0; k<500; k++)
          {
            xx[k] = (2.*k /499.  - 1.) * depth/2.;
            yy[k] = - width/2. - sqrt(sq(radius_of_curv) - sq(depth/2.)) + sqrt(sq(radius_of_curv) - sq(xx[k])) + shift_y;

            if (GeomFilePtr!=NULL)
              fprintf(GeomFilePtr, "%ld   %lf  %lf\n", j, xx[k], yy[k]);		
          }

          for (k=0;k<GatesNumber/2; k++) 
          { int index;
							
            index = (int)(pow(2.*k/GatesNumber, expon)*250.);
            x_ch[k][j] = xx[index];
            y_ch[k][j] = yy[index];
            x_ch[GatesNumber - 1 - k][j] = xx[499 - index];
            y_ch[GatesNumber - 1 - k][j] = yy[499 - index];
          }													  
          shift_y += add;
          m = m * (-1);
        }
        /* activating shadowing cylinder */
        x_ch[0][0] = 
        x_ch[0][1] = 
        x_ch[0][2*Nchannels] =
        x_ch[0][2*Nchannels+1] = -main_depth/2.;
				
        x_ch[GatesNumber - 1][0] =
        x_ch[GatesNumber - 1][1] =
        x_ch[GatesNumber - 1][2*Nchannels] =
        x_ch[GatesNumber - 1][2*Nchannels+1] = main_depth/2.;
	
        /* print out gate coordinates */
        if (GatesFilePtr!=NULL)
          for(j=0; j< 2*Nchannels+2; j++) 
            for(k=0;k<GatesNumber; k++) 
              fprintf(GatesFilePtr,"%f   %f  \n", x_ch[k][j] , y_ch[k][j]);
      }
    }
  }
		
  if      (eFmOption==VT_FERMI_STR)  coef_pi=1.0; 
  else if (eFmOption==VT_FERMI_CURV) coef_pi=2.0; 
  else                             Error("Invalid Fermi type option");
  
  fprintf(LogFilePtr,"Phase set is %f deg.\n", 180./M_PI*fmod(Phase , coef_pi*M_PI)); 

}/* End OwnInit */


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  if (GeomFilePtr) fclose(GeomFilePtr);
  if (GatesFilePtr) fclose(GatesFilePtr);
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  double theta     = 0.0,   // [rad] horizontal deviation of the Fermi chopper from x-axis in the figure
         opening   = 0.0,   // [rad] angle that is covered by one part of the shadowing cylinder 
         chan_dist = 0.0,   // [cm]  distance between 2 neighboring channels
         phi       = 0.0;   // [rad] direction to the center of the shadowing cylinder
  int k;

  // Visualisation of the velocity selector geometry
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    theta     = Phase;
    phi       = theta + M_PI_2;
    chan_dist = width/Nchannels;
    if (diameter > width)
      opening = 2.0 * acos(width/diameter);

    // straight Fermi chopper: channel walls are shown
    if (eFmOption==VT_FERMI_STR)
    { 
      stGeometry.nHulls = Nchannels+1; 
      stGeometry.pHull  = (VtHull*) calloc(stGeometry.nHulls, sizeof(VtHull));

      for (k=0; k <= Nchannels; k++)
      { 
        stGeometry.pHull[k].WidthIn   = wallwidth;
        stGeometry.pHull[k].WidthOut  = wallwidth;
        stGeometry.pHull[k].HeightIn  = height;
        stGeometry.pHull[k].HeightOut = height;
        stGeometry.pHull[k].Length    = depth;
        stGeometry.pHull[k].vCntr[0]  = pos_chp[0] + (k - Nchannels/2.0) * chan_dist * sin(theta);
        stGeometry.pHull[k].vCntr[1]  = pos_chp[1] - (k - Nchannels/2.0) * chan_dist * cos(theta);
        stGeometry.pHull[k].vCntr[2]  = pos_chp[2];
        stGeometry.pHull[k].vNormal[0]= cos(theta);
        stGeometry.pHull[k].vNormal[1]= sin(theta);
        stGeometry.pHull[k].vNormal[2]= 0.0;
        stGeometry.pHull[k].rotAngle  = 0.0;   // Rotation about x axis
      }
    }
    // curved Fermi chopper is shown as one block
    else if (eFmOption==VT_FERMI_CURV)
    {
      stGeometry.nCuboids = 1;
      stGeometry.pCuboid  = (VtCuboid*) calloc(stGeometry.nCuboids, sizeof(VtCuboid));

      stGeometry.pCuboid[0].Length     = depth;
      stGeometry.pCuboid[0].Width      = width;
      stGeometry.pCuboid[0].Height     = height;
      stGeometry.pCuboid[0].rotAngle   = 0.0;
      stGeometry.pCuboid[0].vCntr[0]   = pos_chp[0];
      stGeometry.pCuboid[0].vCntr[1]   = pos_chp[1];
      stGeometry.pCuboid[0].vCntr[2]   = pos_chp[2];
      stGeometry.pCuboid[0].vNormal[0] = cos(theta);
      stGeometry.pCuboid[0].vNormal[0] = sin(theta);
      stGeometry.pCuboid[0].vNormal[0] = 0.0;
    }
    else                             
    { Error("Invalid Fermi type option");
    }

    // Shadowing cylinder: 2 cylinder slices + 2 rectangles
    if (diameter > depth && diameter > width)
    {
      stGeometry.nCylSlices = 2; 
      stGeometry.pCylSlice  = (VtCylSlice*) calloc(stGeometry.nCylSlices, sizeof(VtCylSlice));

      stGeometry.pCylSlice[0].Radius     = 0.5*diameter; 
      stGeometry.pCylSlice[0].Width      = stGeometry.pCylSlice[0].Radius * opening;
      stGeometry.pCylSlice[0].Height     = height;
      stGeometry.pCylSlice[0].vCntr[0]   = pos_chp[0];
      stGeometry.pCylSlice[0].vCntr[1]   = pos_chp[1];
      stGeometry.pCylSlice[0].vCntr[2]   = pos_chp[2];
      stGeometry.pCylSlice[0].vSymAxis[0]= 0.0;
      stGeometry.pCylSlice[0].vSymAxis[1]= 0.0;
      stGeometry.pCylSlice[0].vSymAxis[2]= 1.0;
      stGeometry.pCylSlice[0].OpenAngle  = Degrees(opening);
      stGeometry.pCylSlice[0].Phi        = Degrees(phi);

      stGeometry.pCylSlice[1].Radius     = 0.5*diameter; 
      stGeometry.pCylSlice[1].Width      = stGeometry.pCylSlice[0].Radius * opening;
      stGeometry.pCylSlice[1].Height     = height;
      stGeometry.pCylSlice[1].vCntr[0]   = pos_chp[0];
      stGeometry.pCylSlice[1].vCntr[1]   = pos_chp[1];
      stGeometry.pCylSlice[1].vCntr[2]   = pos_chp[2];
      stGeometry.pCylSlice[1].vSymAxis[0]= 0.0;
      stGeometry.pCylSlice[1].vSymAxis[1]= 0.0;
      stGeometry.pCylSlice[1].vSymAxis[2]= 1.0;
      stGeometry.pCylSlice[1].OpenAngle  = Degrees(opening);
      stGeometry.pCylSlice[1].Phi        =-Degrees(phi);

      stGeometry.nRectangles = 2;
      stGeometry.pRectangle  = (VtRectangle*) calloc(stGeometry.nRectangles, sizeof(VtRectangle));

      stGeometry.pRectangle[0].Width      = sqrt(sq(diameter) - sq(width));
      stGeometry.pRectangle[0].Height     = height;
      stGeometry.pRectangle[0].rotAngle   = 0.0;
      stGeometry.pRectangle[0].vCntr[0]   = pos_chp[0] - width/2.0 * sin(theta);
      stGeometry.pRectangle[0].vCntr[1]   = pos_chp[1] + width/2.0 * cos(theta);
      stGeometry.pRectangle[0].vCntr[2]   = pos_chp[2];
      stGeometry.pRectangle[0].vNormal[0] =-sin(theta);
      stGeometry.pRectangle[0].vNormal[1] = cos(theta);
      stGeometry.pRectangle[0].vNormal[2] = 0.0;

      stGeometry.pRectangle[1].Width      = sqrt(sq(diameter) - sq(width));
      stGeometry.pRectangle[1].Height     = height;
      stGeometry.pRectangle[1].rotAngle   = 0.0;
      stGeometry.pRectangle[1].vCntr[0]   = pos_chp[0] + width/2.0 * sin(theta);
      stGeometry.pRectangle[1].vCntr[1]   = pos_chp[1] - width/2.0 * cos(theta);
      stGeometry.pRectangle[1].vCntr[2]   = pos_chp[2];
      stGeometry.pRectangle[1].vNormal[0] =-stGeometry.pRectangle[0].vNormal[0];
      stGeometry.pRectangle[1].vNormal[1] =-stGeometry.pRectangle[0].vNormal[1];
      stGeometry.pRectangle[1].vNormal[2] = 0.0;
    }
  }    

  return;
}


/*******************************************************/
/** Checks if neutron passes through all gates        **/
/*******************************************************/
static int inPhase(int gates, double phase0, const double WL, const VectorType Dir, const VectorType Pos)
{
  int i,j;

  for (j=2; j < 2*Nchannels+2; j += 2) 
  {
    for (i=0; i < gates; i++)
    { if (outOfPhase(i, j, gates, phase0, WL, Dir, Pos)) 
      {
        i = -1;
        break;  // go for next j
      }
    }
    // if all tests were successful the neutron is in phase
    if (i == gates) return 1;
  }
  return 0;
}


/*******************************************************/
/** Checks if neutron is out of one gate              **/
/*******************************************************/
static int outOfPhase(int i, int j, int gates, const double phase0, const double WL, const VectorType Dir, const VectorType Pos) 
{
  short  bOutPhase=FALSE;
  double phaseB=0.0, // phase to outer side of gate
         phaseT=0.0; // phase to inner side of gate

  phaseB = CalcPhase(x_ch[i][j-1], y_ch[i][j-1], WL, Dir, Pos);
  phaseT = CalcPhase(x_ch[i][j],   y_ch[i][j],   WL, Dir, Pos);
 
  if (i <= (gates-1) / 2)  // gates before chopper center
    bOutPhase = (phase0 <= phaseB || phase0 >= phaseT);
  else
    bOutPhase = (phase0 >= phaseB || phase0 <= phaseT);

  return bOutPhase;
}


/**********************************************************************************/
/** Calculates chopper phase for which the gate edge hits the neutron trajectory **/
/**********************************************************************************/
static double CalcPhase(const double x_ch_k_j, const double y_ch_k_j, const double WL, const VectorType Dir, const VectorType Pos)
{

  double phase=0.0,      // calculated chopper phase
         sq_x_ch_k_j,    // square of the x position of the gate edge
         y_ch_new_k_j,   // varied y position if neutron trajectory beam does not intersect with the trajectory of the gate
         Denom_k,        // distance of the gate edge to thd chopper center = radius of rotation 
         Arg_k, arg_k, 
         pha_k_j, 
         sq_D_0_1, 
         sq_term, 
         omega_fact,     // Chop_RotFreq / Neutron_Speed    [rad/cm] 
         dirpos,         // y position of the neutron at x=0, i.e. at the position of the chopper center
         vx_pos,         // flag for the horizontal hemisphere of the gate position : 1: after chopper center  -1: before chopper center
         vy_pos;         // flag for the vertical hemisphere of the neutron positio0: 1: above chopper center  -1: below chopper center

  sq_D_0_1 = sq(Dir[0]) + sq(Dir[1]);
  dirpos   = Dir[0]*Pos[1] - Dir[1]*Pos[0];
  sq_term  = sq(dirpos) / sq_D_0_1;
  omega_fact = omega / (V_FROM_LAMBDA(WL) * Dir[0]);
  vx_pos = x_ch_k_j > 0.0 ? 1.0 : -1.0;
  vy_pos = Pos[1]   > 0.0 ? 1.0 : -1.0; 

  sq_x_ch_k_j = sq(x_ch_k_j);
  Denom_k     = sqrt( sq_D_0_1 * (sq_x_ch_k_j + sq(y_ch_k_j)) );

  Arg_k = dirpos / Denom_k;

  // if the radius of the rotating gate edge is smaller than the distance to the neutron trajectory
  // there is no intersection with trajectory
  if (fabs(Arg_k) > 1.)  //
  {
    // original calculation only works if the gate is sufficiently far away from the center (relative to the channel width) 
    if (x_ch_k_j/w_ch > 0.5)
    { Arg_k = vy_pos; 
      y_ch_new_k_j = Arg_k * sqrt(sq_term - sq_x_ch_k_j);
    }
    else
    { phase = vx_pos*vy_pos * M_PI_2;
      return phase;
    }
  } 
  else 
  {
    y_ch_new_k_j = y_ch_k_j;
  }

  Denom_k = sqrt( sq_D_0_1 * (sq_x_ch_k_j + sq(y_ch_new_k_j)) );

  arg_k = (Dir[0]*y_ch_new_k_j - Dir[1]*x_ch_k_j) / Denom_k;

  if (fabs(arg_k) > 1.0) return  777;
			
  pha_k_j = asin(Arg_k) - asin(arg_k);

  if (x_ch_k_j < 0.0) pha_k_j = - pha_k_j;
								
  phase = pha_k_j - omega_fact * (x_ch_k_j * cos(pha_k_j) - y_ch_new_k_j * sin(pha_k_j) - Pos[0]);

  return phase;
}


