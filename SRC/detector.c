/****************************************************************************************/
/*  VITESS module DETECTOR                                                              */
/* The free non-commercial use of these routines is granted providing due credit        */
/* is given to the authors.                                                             */
/*                                                                                      */
/* 1.0                           initial version                                        */
/* 1.1  Jun 2001  G. Zsigmond    SOFTABORT                                              */
/* 1.2  Jan 2002  K. Lieutenant  reorganisation                                         */
/* 1.3  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                           */
/* 1.4  Feb 2004  K. Lieutenant  'message' and 'ERROR' included                         */
/* 1.5  Apr 2004  K. Lieutenant  probability divided by repetition                      */
/* 1.5a Dec 2004  K. Lieutenant  option 'no TOF' added                                  */
/* 1.6  Nov 2005  K. Lieutenant  direction instead of scattering angles written         */
/*                               correction: grid for cylindrical geometry              */
/* 1.6a Dec 2009  A. Houben      Detect only neutrons with a given color and add        */
/*                               a value to the color property after detection          */
/* 1.7  Oct 2012  D. Nekrassov                                                          */
/*              & K. Lieutenant  visualization                                          */
/* 1.8  Jul 2012  A. Houben      Cylindrical detector with constant angle in phi        */
/*                               (pixel size will vary with phi)                        */
/* 1.8a Jul 2012  A. Houben      Wavelength dependant probability is read from file     */
/* 1.9  Apr 2013  C. Zendler     Correct wavelength dependence, removed 'no TOF' option,*/
/*                               add tube geometry, 3D detector, cyl. axis in y and x,  */
/*                               detector array, resolution; correct const.phi;         */
/*                               visual. of SP instead of det. signal                   */
/* 1.10 Sep 2013  C. Zendler     Correct efficiency when x-resolution>0,                */
/*                               correct efficiency in solid layers if Nrep>1           */
/* 1.11 Oct 2013  C. Zendler     Possibility to incline flat detector wrt pos. vector;  */
/*                               eff. modifyer can be >1 and also used with input file  */
/* 1.11a Oct 2013 A. Houben      Detect only neutrons within min and max color and      */
/*                               option for exclusive counts                            */
/* 1.12 Apr 2014  C. Zendler     Solid layer detection time quantized in area detector, */
/*                               probability for 2 B/Li layers per detector layer       */
/* 1.13 Mar 2020  K. Lieutenant  new central visualization parameters                   */
/* 1.14 Jan 2022  K. Lieutenant  tidying up                                             */
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "init.h"
#include "convert.h"
#include "matrix.h"
#include "softabort.h"
#include "intersection.h"
#include "message.h"
#include "detector.h"


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
int    GenNeutrons=1;             // -A   [-]  repetition: multiply neutrons to get diff. interaction lengths (probability from integration region L/GenNeutrons)
short  bKeepWrongColour =FALSE;   // -d   [-]  flag: TRUE: neutrons outside selected color windows are passed on; FALSE (default): only neutrons complying with the evaluate requirements are written to the output

DetectorType Detector;            // -B   [-]  flag: detector array
                                  // -G   [-]  enum: detector geometry   0: flat    1: cyl,          2: tube
                                  // -a   [-]  enum: detector type       0: tubes   1: area/volume
                                  // -U   [-]  enum: detector usage      0: normal  1: monitor only  2: grid off
                                                
                                  // -q   [-]  min. color for the trajectory to be regarded
                                  // -Q   [-]  max. color for the trajectory to be regarded
                                  // -S   [-]  value added to color for a detected neutron
                                  //           Detector position by direction and distance from sample 
                                  // -P  [deg] phi:   angle of the +y-axis and the projection of the vector to the yz-plane
                                  // -T  [deg] theta: angle between the +x-axis and the vector
                                  // -D  [cm]  Distance of the center of the detector from the origin (usually the sample center)
                                  // -h  [cm]  detector height
                                  // -w  [cm]  detector width
                                  // -t  [cm]  detector thickness
                                  //
                                  // -r   [-]  number of detector rows
                                  // -c   [-]  number of detector columns
                                  // -n   [-]  number of detector layers
                                  // -u  [cm]  FWHM resolution in horizontal direction
                                  // -v  [cm]  FWHM resolution in vertical direction
                                  // -l  [cm]  FWHM resolution in the direction of the surface normal
                                  //
                                  // -m   [-]  enum: material absorbing the neutrons for detection
                                  // -p  [bar] parameter1: pressure/thickness of the absorbing material
                                  // -k   [K]  parameter2: temperature/density of the absorbing material
                                  // -e   [-]  detector efficiency
                                  //
                                  // -o   [-]  enum: orientation of the tubes
                                  // -b   [-]  enum: shape of the tubes
                                  // -f  [cm]  thickness of the tube walls
                                  // -s   [K]  flag: layers shifted against each other by half the diameter
                               
                                  // -V  [deg] phi angle of a flat detector with an inclined detector surface
                                  // -W  [deg] theta angle of a flat detector with an inclined detector surface
                               
                                  // -x   [-]  enum: axis orienation of a cylindrical detector (x, y or z)
                                  // -z   [-]  flag: vertical cell size set by constant phi (not by constant height)
                               
EffFile Eff;                      // -E   [-]  input file determining efficiency
char*   DetOutFileName=NULL;      // -O   [-]  output file name to store events (3D position, time, weight).

// Variables determined from input parameters or trajectory data
double RotMatrix [3][3],          //           rotation matrix to rotate a vector to a frame in which the middle of the detector sits on the x-axis,
       RotSurface[3][3];          //           rotation matrix such that the detector surface is perpendicular to x  (from phi_n, theta_n)
FILE*  pOutFile=NULL;             //           File pointer to the output file
short  eDetProp=-1;               //           enum: detector geometry   0: flat area   1: cyl. area  2: flat tubes
                       
// Other global variables determined from input parameters or trajectory data
long   lost=0;                  //     [-]   number of neutrons lost between tubes


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  Neutron WorkNeutron,           // working copy
    OutNeutron,                  // working copy for WriteNeutron
    DrawNeutron;                 // working copy for visualization (draws scattering point instead of DetSpot)
  VectorType 
    ISP [2]={0.0,0.0,0.0},
    iISP[2]={0.0,0.0,0.0},       // intersection points with the detector
    jISP[2]={0.0,0.0,0.0}, 
    kISP[2]={0.0,0.0,0.0};       // intermediate intersection points with layers/tubes in tube detector 
  VectorType 
    DetSpot  ={0.0,0.0,0.0},     // final detection position given out by detector
    DetSignal={0.0,0.0,0.0},     // intermediate detection position (including gaussian smear from resolution)
    SP    ={0.0,0.0,0.0},        // interaction position
    vShift={0.0,0.0,0.0};        // for visualization: vector in neutron direction with length equal to distance of (last) detector (part in array) center
  double TimeTillScattering=0.0, // neutron flight time until interaction
    FullLengthInDetector=0.0,    // length of neutron trajectorie in overall detector volume
    LengthInAbsorberMaterial=0.0,// length of neutron trajectory in active detector volume
    LengthTillScattering=0.0,    // length of neutron trajectory in active detector volume until interaction
    LengthModifyer=0.0,          // scale lengths in case of thin converter layers
    ScatteringProb=0.0,          // probability of interaction
    N=0.0,                       // particle density
    sigma=0.0,                   // absorption cross-section
    NSigma=0.0,                  // for const. efficieny
    FluxDetected=0.0,            // detected flux
    DistanceTubeLayerExits=0.0;  // distance between tube exit and layer exit points
  long  i=0,j=0,l=0,z=0;         // loop counting
  int   NeutCount=0,             // count neutron copies for repetition (=GenNeutrons) > 1
        NumDetected=0;           // number of detected neutrons
  short FoundTube=0;             // tag for tube in which interaction happens

  // initialisation
  // --------------
  InitNeutron(&WorkNeutron);
  InitNeutron(&OutNeutron);
  InitNeutron(&DrawNeutron);

  _eModule=MCN_DETECTOR;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.14");
  OwnInit(argc, argv);    // module specific initialization
 
  bVisInstalled = TRUE;
  bLengthCmpr   = FALSE;

  /* RotMatrix will rotate a Vector to a frame in which the middle of the detector sits on the x-axis,
     i.e. Detector.Direction (cyl-axis for cyl. det) defines new x axis*/
  RotMatrixX(Detector.Direction, RotMatrix);

  DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
  /* Get the neutrons from the file */
  while((ReadNeutrons())!= 0)
  {
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK
	
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // drop neutrons that don't pass the color filter
        if ( (Detector.minColor >= 0 && InputNeutrons[i].Color < Detector.minColor) ||
             (Detector.maxColor >= 0 && InputNeutrons[i].Color > Detector.maxColor) ) 
        {
          if (bKeepWrongColour==TRUE)
          WriteNeutron(&InputNeutrons[i]);
          continue;
        }

        // pass on neutrons detected by previous detector parts,
        // last detector in array removes tag and writes output file
        if( floor(InputNeutrons[i].Color/10000)==1 )
        {
          if(!Detector.bArray)
          {
            InputNeutrons[i].Color-=10000;
            if(DetOutFileName)
            fprintf(pOutFile,"   %10.4f  %10.4f  %10.4f   %10.4f     %2.3e     %d\n",InputNeutrons[i].Position[0],InputNeutrons[i].Position[1],InputNeutrons[i].Position[2],InputNeutrons[i].Time,InputNeutrons[i].Probability,InputNeutrons[i].Color);
          }
          WriteNeutron(&InputNeutrons[i]);
          continue;
        }

          // Use a copy to work on
        WorkNeutron = InputNeutrons[i];

        /* First rotate the position and direction of the neutron to the detector frame */
        RotVector(RotMatrix, WorkNeutron.Position);
        RotVector(RotMatrix, WorkNeutron.Vector);

        /* absorber/converter particle density in m^-3 */
        // N=(Detector.eAbsMat<2)?(Detector.GasPressure*1e5/(Detector.GasTemperature*KB)):(Detector.SolidAtomDensity*1E27);
        if (Detector.eAbsMat==VT_GAS_BF3 || Detector.eAbsMat==VT_GAS_HE3)
          N = Detector.GasPressure*1e5/(Detector.GasTemperature*KB);
        else if (Detector.eAbsMat==VT_SOLID_B10 || Detector.eAbsMat==VT_SOLID_LI6)
          N = Detector.SolidAtomDensity*1E27;

        if(NeutronIntersectsDetector(&(WorkNeutron),ISP)) 
        {

          /* determine the length of the path through the whole detector volume */
          FullLengthInDetector = DistVector(ISP[0], ISP[1]);

          /*correct for dead zones in tube geometry */
          if (eDetProp==VT_FLAT_TUBES)
          { 
            FullLengthInDetector=0;
            for (l=1; l<=Detector.NLayers; l++)
            {
              z=0; DistanceTubeLayerExits=0;
              if(NeutronIntersectsLayer(WorkNeutron.Vector,ISP[0],l,jISP))
              {	      
                CopyVector(jISP[0],kISP[1]);
                while (DistVector(jISP[1],kISP[1])>0.001*Detector.PixelWidth[0])
                {
                  if (fabs(DistanceTubeLayerExits-DistVector(jISP[1],kISP[1])) < 0.000001)
                    break;
                  ++z; 
                  DistanceTubeLayerExits=DistVector(jISP[1],kISP[1]);
                  if(NeutronIntersectsTube(WorkNeutron.Vector,kISP[1],l,iISP,kISP))
                    FullLengthInDetector+=DistVector(iISP[0],iISP[1]);
                  if (z>Detector.NLayers+1 && z>Detector.NColumns+1 && z>Detector.NRows+1)
                    Error("tube with exit point close to layer exit not found!");
                }
              }		
            } 
          }
	
          /* total cross-section (in m^2) */
          sigma=GetXsec(Detector.eAbsMat, WorkNeutron.Wavelength);

          if (Detector.eUsage==VT_MON_ONLY)
            GenNeutrons=1;
          //------------------------------------------------------

          for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++)
          {
            /* determine the interaction point in the detector volume and the detector signal point*/
            LengthTillScattering = MonteCarlo(0,FullLengthInDetector); 
            for(j=0; j<3; j++)
              SP[j]= ISP[0][j] +LengthTillScattering*WorkNeutron.Vector[j];

            /* find interaction point in tubes */
            if (eDetProp==VT_FLAT_TUBES)
            { 
              LengthInAbsorberMaterial=0; FoundTube=0;
              for (l=1; l<=Detector.NLayers; l++)
              {
                z=0; DistanceTubeLayerExits=0;
                if(NeutronIntersectsLayer(WorkNeutron.Vector,ISP[0],l,jISP))
                {	      
                  CopyVector(jISP[0],kISP[1]);
                  while (DistVector(jISP[1],kISP[1])>0.0001)
                  {
                    if( fabs(DistanceTubeLayerExits-DistVector(jISP[1],kISP[1])) < 0.000001)
                      break;
                    ++z; 
                    DistanceTubeLayerExits=DistVector(jISP[1],kISP[1]);
                    if(NeutronIntersectsTube(WorkNeutron.Vector,kISP[1],l,iISP,kISP))
                      LengthInAbsorberMaterial+=DistVector(iISP[0],iISP[1]);
                    if(z>Detector.NLayers+1 && z>Detector.NColumns+1 && z>Detector.NRows+1)
                      Error("tube with exit point close to layer exit not found!");
                  }
                  if(LengthInAbsorberMaterial>LengthTillScattering)
                  {
                    FoundTube=1;
                    for(j=0; j<3; j++)
                    {
                      SP[j]=iISP[1][j]-WorkNeutron.Vector[j]*(LengthInAbsorberMaterial-LengthTillScattering);
                    }
                    break;
                  }
                }		
              } 
            }

            /* solid layers, only in non-tube geometry so far: cast SP onto solid layer for correct time quantization*/
            if( (Detector.eAbsMat==VT_SOLID_B10 || Detector.eAbsMat==VT_SOLID_LI6) && Detector.eUsage==VT_DET_REAL )
            {
              CubeDetLayerSpot(SP);
            }

            CopyVector(SP,DetSignal);
	  
            /* resolution */
            if (Detector.Resolution[1] > 0 && (Detector.eType==VT_DET_AREA || Detector.DG.Tube.eTubeOrient==HORIZONTAL) )
              DetSignal[1]=DistrGauss(DetSignal[1],Detector.Resolution[1]); 
            if (Detector.Resolution[2] > 0 && (Detector.eType==VT_DET_AREA || Detector.DG.Tube.eTubeOrient==VERTICAL) )
              DetSignal[2]=DistrGauss(DetSignal[2],Detector.Resolution[2]);
            if (Detector.Resolution[0] > 0 &&  Detector.eType==VT_DET_AREA )
              DetSignal[0]=DistrGauss(DetSignal[0],Detector.Resolution[0]);
	  
            /* interaction probability */
            if (Eff.maxdata > 0) 
            { //efficiency from file
              ScatteringProb = GetLambdaProbFromEff(WorkNeutron.Wavelength,WorkNeutron.ID)* Detector.EfficiencyMod;
            } 
            else 
            {
              switch(Detector.eAbsMat)
              {
                case VT_GAS_BF3:  //BF3
                case VT_GAS_HE3:  //He3
                  ScatteringProb = N*sigma*exp(-N*sigma*LengthTillScattering/100) * FullLengthInDetector/100 * Detector.EfficiencyMod;
                  break;
                case VT_SOLID_B10:  //solid B10
                case VT_SOLID_LI6:  // Li6
                  // solid layer approximation: scale length in material with layer_thickness/total_thickness
                  // 2 solid layers per tube/anode layer
                  LengthModifyer=2*Detector.NLayers*Detector.SolidAbsThickness/Detector.Thickness;
                  ScatteringProb = N*sigma*exp(-N*sigma*(LengthModifyer*LengthTillScattering)/100) * (LengthModifyer*FullLengthInDetector)/100* Detector.EfficiencyMod;
                  break; 
                case VT_ABS_OTHER:  // other; use wavelength-independent input eff
                default:
                  NSigma=-log(1-Detector.EfficiencyMod)/Detector.Thickness;
                  ScatteringProb = NSigma*exp(-NSigma*LengthTillScattering) * FullLengthInDetector;
                  break;
              }
            }

            if (eDetProp==VT_FLAT_TUBES && !FoundTube)
            {
              ScatteringProb=0;
              lost++;
            }

            DetectorSpot(DetSignal, DetSpot);  

            TimeTillScattering=DistVector(SP,WorkNeutron.Position)/
            V_FROM_LAMBDA(WorkNeutron.Wavelength);

            /* everythings done, so rot back the vectors and put all together 
            and set output data of the neutron */
            OutNeutron             = WorkNeutron;
            OutNeutron.Time        = WorkNeutron.Time + TimeTillScattering;
            OutNeutron.Probability = WorkNeutron.Probability * ScatteringProb / GenNeutrons;


            // tag detected neutrons for detector array 
            if(Detector.bArray)
            OutNeutron.Color+=10000;
 
            RotBackVector(RotMatrix,SP);
            RotBackVector(RotMatrix,DetSignal);
            RotBackVector(RotMatrix,DetSpot);	
	      		      
            if (Detector.eUsage==0) 
            {           // normal
              CopyVector(DetSpot, OutNeutron.Position);
              NormVector(DetSpot);
              CopyVector(DetSpot, OutNeutron.Vector);
            }
            else if (Detector.eUsage==1)
            { //monitor only
              OutNeutron.Probability = WorkNeutron.Probability;
              CopyVector(SP, OutNeutron.Position);
              RotBackVector(RotMatrix,OutNeutron.Vector);
            }
            else if (Detector.eUsage==2)
            { // grid off
              CopyVector(DetSignal, OutNeutron.Position);
              NormVector(DetSignal);
              CopyVector(DetSignal, OutNeutron.Vector);
            }

            // write out neutrons that shall be detected	
            if ( OutNeutron.Probability>wei_min )  
            {
              if (Detector.addColor > 0) 
              OutNeutron.Color += Detector.addColor;
              WriteNeutron(&OutNeutron);	
              NumDetected++;
              FluxDetected+=OutNeutron.Probability;
              if(!Detector.bArray && DetOutFileName)
                fprintf(pOutFile,"   %10.4f  %10.4f  %10.4f   %10.4f     %2.3e     %d\n",OutNeutron.Position[0],OutNeutron.Position[1],OutNeutron.Position[2],OutNeutron.Time,OutNeutron.Probability,OutNeutron.Color);
            }

            // write interaction point - only once per incoming trajectory
            if (NeutCount < 1)
            {
              DrawNeutron=OutNeutron;
              CopyVector(SP, DrawNeutron.Position);
              if(!Detector.bArray)
              DrawNeutron.Color+=10000;
              WriteIAP(&DrawNeutron, VT_DETECTED);
            }

          } /* loop count */
        } 
        else /* if neutron does not intersect detector */ 
        {
	        if(Detector.bArray)
            WriteNeutron(&InputNeutrons[i]);
          else 
          { //draw trajectories of undetected neutrons up to detector end
            DrawNeutron=WorkNeutron;
            RotBackVector(RotMatrix, DrawNeutron.Vector);
            RotBackVector(RotMatrix, DrawNeutron.Position);
            CopyVector(DrawNeutron.Vector, vShift);
            MultiplyByScalar(vShift, (Detector.Distance+Detector.Thickness/2));
            AddVector(DrawNeutron.Position,vShift);
            WriteIAP(&DrawNeutron, VT_OUTSIDE);
          }
        }
      }
    } //for(i=0; i<NumNeutGot; i++)
  } //while((ReadNeutrons())!= 0)

  fprintf(LogFilePtr,"Neutrons detected in this detector: %d trajectories (%11.4e n/s) \n",NumDetected,FluxDetected);

  // Finish: writes and closes monitor files, writes to log and instrument file, frees memory
  // ----------------------------------------------------------------------------------------
my_exit:
   // write geometry data for visualization
  SetGeometry("cyan");                      

  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  // the origin of the co-ordinate system remains at the sample  
    Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  VectorType SurfaceInclination;
  long i=0;
  long count = 0;
  char sBuffer[512]="";
 
  InitDetector(&Detector);

  Eff.pfile   =NULL;
  Eff.data    =NULL;
  Eff.filename=NULL;
  Eff.maxdata = 0;

  InitRotMatrix(RotMatrix);
  InitRotMatrix(RotSurface);

  for(i=1; i<argc; i++) 
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      { 
        // ABCDEFGHIJKLMNOPQRSTUVWXYZ
        // ABCDE-G-------OPQ-STUVW---
        // abcdef-h--klmnopqrstuvwx-z
        case 'B':
          Detector.bArray=atoi(&argv[i][2]); 
          break;

        case 'G':
          Detector.eGeom = (VtDetGeom) atoi(&argv[i][2]); // 1: cylindrical   2: flat
          break;
        case 'a':
          Detector.eType = (VtDetType) atoi(&argv[i][2]); // 0: tubes   1: area/volume
          break;
        case 'U':
          Detector.eUsage = (VtDetUse) atoi(&argv[i][2]);
          break;
        case 'm':
          Detector.eAbsMat= (VtDetAbs) atoi(&argv[i][2]);//0=Bf3, 1=He3, 5=other(from file or const.)
          break;

        case 'A':
          GenNeutrons=atoi(&argv[i][2]);
          break;

        case 'q':
          Detector.minColor = atol(&argv[i][2]);       /*  use neutrons with color >= minColour */
          break;
        case 'Q':
          Detector.maxColor = atoi(&argv[i][2]);       /*  use neutrons with color <= maxColour */
          break;
        case 'S':
          Detector.addColor=atoi(&argv[i][2]);
          break;
        case 'd':
          if (atol(&argv[i][2])==1)               /* if activated, neutrons outside the colour selection  */
            bKeepWrongColour = TRUE;              /* are passed to the next module */
          break;

        case 'P':
          /* Phi is the angle of the +y-axis and the projection of the vector to the yz-plane */
          Detector.Phi=atof(&argv[i][2])*M_PI/180;
          break;
        case 'T':
          /* Theta is the angle between the +x-axis and the vector*/
          Detector.Theta=atof(&argv[i][2])*M_PI/180;
          break;
        case 'D':
          Detector.Distance=atof(&argv[i][2]);
          break;

        case 'h':
          Detector.Height=atof(&argv[i][2]);
          break;
        case 'w':
          Detector.Width=atof(&argv[i][2]);
          break;
        case 't':
          Detector.Thickness=atof(&argv[i][2]);
          if(Detector.Thickness<=0.0) Detector.Thickness=1e-4;
          break;

        case 'r':
          Detector.NRows=atoi(&argv[i][2]);
          break;
        case 'c':
          Detector.NColumns=atoi(&argv[i][2]);
          break;
        case 'n':
          Detector.NLayers=atoi(&argv[i][2]);
          break;

          /* resolution: sigma_Gauss=FWHM/2.35482  */
        case 'u':
          Detector.Resolution[1]=atof(&argv[i][2])/2.35482;
          break;
        case 'v':
          Detector.Resolution[2]=atof(&argv[i][2])/2.35482;
          break;
        case 'l':
          Detector.Resolution[0]=atof(&argv[i][2])/2.35482;
          break;

        case 'p':
          Detector.AbsPar1=atof(&argv[i][2]);
          break;
        case 'k':
          Detector.AbsPar2=atof(&argv[i][2]);
          break;
        case 'E':    /* efficiency file */
          Eff.pfile = OpenInputFile2( (Eff.filename = &argv[i][2]), "efficiency data", "r");
          break;
        case 'e':
          Detector.EfficiencyMod=atof(&argv[i][2]);
          break;

        /* tube detector */
        case 'o':
          Detector.DG.Tube.eTubeOrient= (VtOrient) atoi(&argv[i][2]);
          break;
        case 'b':
          Detector.DG.Tube.eTubeShape = (VtTubeShape) atoi(&argv[i][2]);
          break;
        case 'f':
          Detector.DG.Tube.wallThickness=atof(&argv[i][2])/10;//in cm
          break;
        case 's':
          Detector.DG.Tube.bTubeShift=atoi(&argv[i][2]);
          break;

        /* Flat detector: Theta_n, phi_n describe the normal vector of the detector surface W.r.t. the rotated system with x-axis on distance vector*/
        case 'V':
          Detector.Phi_n=atof(&argv[i][2])*M_PI/180;
          break;
        case 'W':
          Detector.Theta_n=atof(&argv[i][2])*M_PI/180;
          break;

        /* Cylindrical detector */
        case 'x':
          /* cylinder axis orientation */
          Detector.DG.Cyl.eAxis = (VtScAxis) atoi(&argv[i][2]);
          break;
        case 'z':
          /* flag: constant phi */
          Detector.DG.Cyl.bCnstPhi=atof(&argv[i][2]);
          break;

        /* output file */
        case 'O':
          DetOutFileName=(&argv[i][2]);
          if (!Detector.bArray)
          {
            pOutFile=OpenInputFile(DetOutFileName, TRUE, "w+");
            fprintf(pOutFile,"#Trajectories detector_eventmode \n");
            fprintf(pOutFile,"#    pos_x [cm]   pos_y [cm]   pos_z [cm]  time [ms]    weight     color \n");
            fprintf(pOutFile,"#-------------------------------------------------------------------------\n");
          }
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  /* check if all necessary input has been given */
  CheckAndAdjustDetectorInput(Detector.eType, Detector.eGeom);

  SurfaceInclination[0]=cos(Detector.Theta_n);
  SurfaceInclination[1]=sin(Detector.Theta_n)*cos(Detector.Phi_n);
  SurfaceInclination[2]=sin(Detector.Theta_n)*sin(Detector.Phi_n);
  RotMatrixX(SurfaceInclination,RotSurface);
 
 
  if(Detector.eGeom==VT_DET_CYL)
  {	/* cylinder */
    if (Detector.DG.Cyl.eAxis== VT_SC_X && fabs(Detector.Theta-M_PI/2)>0.0001)
    {
      fprintf(LogFilePtr,"\n WARNING: Theta is set to 90 deg for cylinder in x direction!");
      Detector.Theta=M_PI/2;
    }

    Detector.DG.Cyl.r=Detector.Distance;	 	 
    Detector.Direction[Detector.DG.Cyl.eAxis]=1.0;   

    if (cos(Detector.Phi) < 0.0) Detector.Theta=-Detector.Theta;

    NeutronIntersectsDetector=NeutronIntersectsCylDetector;
    DetectorSpot=CylinderDetSpot;
  } 
  else 
  { /* cube */
    Detector.Direction[0]=cos(Detector.Theta);
    Detector.Direction[1]=sin(Detector.Theta)*cos(Detector.Phi);
    Detector.Direction[2]=sin(Detector.Theta)*sin(Detector.Phi);
    for(i=0; i<3;i++)
    if(fabs(Detector.Direction[i])<1e-5) Detector.Direction[i]=0.0;
    
    Detector.Position[0] = Detector.Distance;
    NeutronIntersectsDetector=NeutronIntersectsCubeDetector;
    DetectorSpot=CubeDetSpot;
  }

  if (Eff.pfile!=NULL) 
  {
    Eff.maxdata = LinesInFile(Eff.pfile);
    Eff.data = calloc(Eff.maxdata+1, sizeof(EffData));
    for(count=0; count < Eff.maxdata; count++) 
    {
      ReadLine(Eff.pfile, sBuffer, sizeof(sBuffer)-1);
      sscanf(sBuffer, "%lf %lf", &Eff.data[count].Lambda, &Eff.data[count].Eff);
    }
    fclose(Eff.pfile);
  }
}


/*******************************************************/
/** Does module specific cleanup                      **/
/*******************************************************/
void OwnCleanup()
{
  /* close event output file */
  if(pOutFile)
    fclose(pOutFile);

  /* print error that might have occured many times */
  PrintMessage(DET_TRAJ_INSIDE, "", ON);
  PrintMessage(DET_L_RANGE_TOO_SMALL,Eff.filename, ON);

  if(lost>0)
    fprintf(LogFilePtr,"WARNING: %ld neutrons lost between tubes\n",lost);
 
  // fprintf(LogFilePtr," \n");
} 


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the slit geometry
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    // Geometry data
    if (Detector.eGeom==VT_DET_CYL)
    { // cylinder
      double ry, rz;
      stGeometry.pCylSlice = (VtCylSlice*) calloc(1, sizeof(VtCylSlice));
      stGeometry.nCylSlices = 1; 
	
      stGeometry.pCylSlice[0].Radius = Detector.Distance; 
      if(Detector.Width > (int) 2*M_PI*Detector.DG.Cyl.r) 
      {
        Warning("Width of cylindrical detector > 2*pi*distance. Width is set to 2*pi*distance for visualisation!");
        stGeometry.pCylSlice[0].Width  = 2*M_PI*Detector.DG.Cyl.r*0.999;
      }
      else 
      {
        stGeometry.pCylSlice[0].Width = Detector.Width;
      }
      stGeometry.pCylSlice[0].Height = Detector.Height;
      stGeometry.pCylSlice[0].vCntr[0]  = 0.;
      stGeometry.pCylSlice[0].vCntr[1]  = 0.;
      stGeometry.pCylSlice[0].vCntr[2]  = 0.;
      stGeometry.pCylSlice[0].vSymAxis[0]= 0;
      stGeometry.pCylSlice[0].vSymAxis[1]= 0;
      stGeometry.pCylSlice[0].vSymAxis[2]= 0;
      stGeometry.pCylSlice[0].vSymAxis[Detector.DG.Cyl.eAxis]= 1;
      stGeometry.pCylSlice[0].Phi = Detector.Theta/M_PI*180.;
      //coordinates differently defined in visualization:
      if(Detector.DG.Cyl.eAxis==VT_SC_X)
        stGeometry.pCylSlice[0].Phi = Detector.Phi/M_PI*180.+90.;
        else if(Detector.DG.Cyl.eAxis==VT_SC_Y)
      stGeometry.pCylSlice[0].Phi = -Detector.Theta/M_PI*180.;
	
      stGeometry.pCylSlice[0].OpenAngle = Detector.Width/(2.*M_PI*Detector.Distance)*360.;
      RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
      stGeometry.pCylSlice[0].Phi += rz/M_PI*180.;
    }
    else
    { // cube
      sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
      stGeometry.pDescr  =  sVisDescrpt;
      stGeometry.eModule = _eModule;

      stGeometry.pCuboid = (VtCuboid*) calloc(1, sizeof(VtCuboid));
      stGeometry.nCuboids = 1; 
	
      stGeometry.pCuboid[0].Length = Detector.Thickness; 
      stGeometry.pCuboid[0].Width  = Detector.Width;
      stGeometry.pCuboid[0].Height = Detector.Height;

      stGeometry.pCuboid[0].vCntr[0]  = (Detector.Distance)*cos(Detector.Theta);
      stGeometry.pCuboid[0].vCntr[1]  = (Detector.Distance)*sin(Detector.Theta)*cos(Detector.Phi);
      stGeometry.pCuboid[0].vCntr[2]  = (Detector.Distance)*sin(Detector.Theta)*sin(Detector.Phi);
      stGeometry.pCuboid[0].vNormal[0]= cos(Detector.Theta);
      stGeometry.pCuboid[0].vNormal[1]= sin(Detector.Theta)*cos(Detector.Phi);
      stGeometry.pCuboid[0].vNormal[2]= sin(Detector.Theta)*sin(Detector.Phi);
      if(Detector.Theta_n!=0)
      {
        RotBackVector(RotSurface,stGeometry.pCuboid[0].vNormal);
      }
    }

  }

}


/*******************************************************/
/** initializes the 'DetectorType' structure          **/
/*******************************************************/
void  InitDetector(DetectorType* pDetector)
{
  int i;

  pDetector->Width    = 0.0; pDetector->Height = 0.0; pDetector->Thickness = 0.0;
  pDetector->NColumns = 1;   pDetector->NRows  = 1;   pDetector->NLayers   = 1;

  pDetector->Distance=0.0;
  pDetector->Theta   =0.0; pDetector->Phi  =0.0;        
  pDetector->Theta_n =0.0; pDetector->Phi_n=0.0;   

  pDetector->AbsPar1=0.0; pDetector->GasPressure   =0.0; pDetector->SolidAbsThickness=0.0;
  pDetector->AbsPar2=0.0; pDetector->GasTemperature=0.0; pDetector->SolidAtomDensity =0.0;  

  pDetector->bArray =FALSE;         
  pDetector->eGeom  =VT_NO_DET_GEOM;
  pDetector->eType  =VT_NO_DET_TYPE;
  pDetector->eUsage =VT_NO_DET_USE;
  pDetector->eAbsMat=VT_NO_ABS_MAT;
 
  for(i=0; i<3; i++)
  {
    pDetector->PixelWidth[i]=0.0;  
    pDetector->Direction [i]=0.0;
    pDetector->Position  [i]=0.0;
    pDetector->Resolution[i]=0.0;
  }

  pDetector->EfficiencyMod = 1.0;
     
  pDetector->addColor = 0;  
  pDetector->minColor =ANY_COLOR; 
  pDetector->maxColor =ANY_COLOR;

  pDetector->DG.Tube.wallThickness=0.0;
  pDetector->DG.Tube.eTubeOrient=NO_ORIENT; 
  pDetector->DG.Tube.eTubeShape=VT_NO_TUBE_SHAPE;  
  pDetector->DG.Tube.bTubeShift=FALSE; 
  pDetector->DG.Cyl.r=0.0; 
  pDetector->DG.Cyl.eAxis=VT_NO_SC_AXIS; 
  pDetector->DG.Cyl.bCnstPhi=FALSE;


}


/*****************************************************************************/
/* Intersection of neutron with different geometries:                        */
/*****************************************************************************/
short NeutronIntersectsCubeDetector(Neutron *Nin, VectorType ISP[])
{
  VectorType NeutronPosition, NeutronVector;
  double t[2];
  int i;
  CubeType DetectorCube;

  /* working copies */
  CopyVector(Nin->Position,NeutronPosition);
  CopyVector(Nin->Vector,NeutronVector);

  /* move the origin of the coordinate system to the middle of the detector*/
  NeutronPosition[0] -= Detector.Position[0];

  /* rotate coordinate system such that x-axis is normal to detector surface*/
  if(Detector.Theta_n!=0)
  {
    RotVector(RotSurface, NeutronPosition);
    RotVector(RotSurface, NeutronVector);
  }

  DetectorCube.height = Detector.Height;
  DetectorCube.width = Detector.Width;
  DetectorCube.thickness = Detector.Thickness;

  if(LineIntersectsCube(NeutronPosition, NeutronVector, &DetectorCube, t)) 
  {
    for(i=0; i<3; i++)
    {	
      ISP[1][i] = NeutronPosition[i]+t[1]*NeutronVector[i];
      ISP[0][i] = NeutronPosition[i]+t[0]*NeutronVector[i];
    }

    /* transform the ISPs to the old coordinate system*/
    if(Detector.Theta_n!=0)
    {
      RotBackVector(RotSurface, ISP[0]);
      RotBackVector(RotSurface, ISP[1]);
    }
    ISP[0][0] += Detector.Position[0];
    ISP[1][0] += Detector.Position[0];
      
    if(t[1]>=0.0) 
    {	
      return TRUE;
    } 
    else 
    {
      RotBackVector(RotMatrix,NeutronVector);
      if(ScalarProduct(Detector.Direction,NeutronVector)>0)
        CountMessageID(DET_TRAJ_INSIDE, Nin->ID);
      RotVector(RotMatrix,NeutronVector);
      return FALSE;
    }
  } 
  else 
  {	return FALSE;
  }
}

short NeutronIntersectsCylDetector(Neutron *Nin, VectorType ISP[])
{
  CylinderType CylDetector;
  VectorType InISP[2], OutISP[2], Position, Direction, cut;
  double Int[2], Outt[2], ntheta, nphi, CTheta, GTheta;
  long   i;
  double dTheta=Detector.Width/(2.0*Detector.Distance);

  CopyVector(Nin->Position, Position);
  CopyVector(Nin->Vector, Direction);

  CylDetector.r=Detector.DG.Cyl.r;
  CylDetector.height=Detector.Height;
 
  if(LineIntersectsCylinder(Position, Direction, &CylDetector, Int))  
  {
    for(i=0; i<3; i++) 
    {	
      InISP[1][i] = Position[i]+Int[1]*Direction[i];
      InISP[0][i] = Position[i]+Int[0]*Direction[i];
    }
  }
  else
    return FALSE;
	
  CopyVector(InISP[1],cut);
  RotBackVector(RotMatrix,cut);
  NormVector(cut);
  CartesianToSpherical(cut, &ntheta, &nphi);

  CTheta=atan(cos(nphi)*tan(ntheta)); 
  if(Detector.Direction[0]) CTheta=nphi;
  else if (Detector.Direction[1]) CTheta=atan(sin(nphi)*tan(ntheta));

  if(Detector.DG.Cyl.eAxis!=VT_SC_X && ntheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
	
  GTheta= Detector.Direction[0] ? Detector.Phi : Detector.Theta;
	
  if( !(fabs(CTheta-GTheta) < dTheta || fabs(CTheta+2*M_PI-GTheta) < dTheta) )
    return FALSE; 

  CylDetector.r += Detector.Thickness;
  if(LineIntersectsCylinder(Position, Direction, &CylDetector, Outt))
  {
    for(i=0; i<3; i++)
    {
      OutISP[1][i] = Position[i]+Outt[1]*Direction[i];
      OutISP[0][i] = Position[i]+Outt[0]*Direction[i];
    }
  }
  else
    return FALSE;
			
  CylDetector.r-=Detector.Thickness;
  if( (fabs(InISP[1][0]) < CylDetector.height*0.99999/2.0) && (fabs(OutISP[1][0]) <CylDetector.height*0.99999/2.0) )
  {
    CopyVector(InISP[1],ISP[0]);
    CopyVector(OutISP[1],ISP[1]);
    return TRUE;
  }
  return FALSE;
}

short NeutronIntersectsLayer(VectorType dir, VectorType pos, int l, VectorType jISP[])
{
  VectorType NeutronDirection_tmp, NeutronPosition_tmp;
  double tISP[2]={0};
  CubeType DetectorLayer={0};
  int i=0;

  CopyVector(dir,NeutronDirection_tmp);
  CopyVector(pos,NeutronPosition_tmp);

  DetectorLayer.thickness = Detector.PixelWidth[0];
  DetectorLayer.height = Detector.Height;
  DetectorLayer.width =  Detector.Width;
  
  /* rotate coord. syst. parallel to cube axes and                                
     move the origin of the coordinate system to the middle of the detector layer */
  NeutronPosition_tmp[0] -= Detector.Distance;
  if(Detector.Theta_n!=0)
  {
    RotVector(RotSurface, NeutronPosition_tmp);
    RotVector(RotSurface, NeutronDirection_tmp);
  }
  NeutronPosition_tmp[0] += (Detector.Thickness/2 - (l-0.5)*Detector.PixelWidth[0]);
  if(Detector.DG.Tube.bTubeShift)
  {
    if (Detector.DG.Tube.eTubeOrient==VERTICAL)
      (l%2==0) ? (NeutronPosition_tmp[1] -= 0.25*Detector.PixelWidth[1]) : (NeutronPosition_tmp[1] += 0.25*Detector.PixelWidth[1]);
    else
      (l%2==0) ? (NeutronPosition_tmp[2] -= 0.25*Detector.PixelWidth[2]) : (NeutronPosition_tmp[2] += 0.25*Detector.PixelWidth[2]);
  }

  if( LineIntersectsCube(NeutronPosition_tmp, NeutronDirection_tmp, &DetectorLayer, tISP) )
  {
    for(i=0; i<3; i++)
    {
      jISP[1][i] = NeutronPosition_tmp[i]+tISP[1]*NeutronDirection_tmp[i];
      jISP[0][i] = NeutronPosition_tmp[i]+tISP[0]*NeutronDirection_tmp[i];
    }
    /* back to orig. coordinates */
    jISP[0][0] -= (Detector.Thickness/2 - (l-0.5)*Detector.PixelWidth[0]);
    jISP[1][0] -= (Detector.Thickness/2 - (l-0.5)*Detector.PixelWidth[0]);
    if(Detector.Theta_n!=0){
      RotBackVector(RotSurface, jISP[0]);
      RotBackVector(RotSurface, jISP[1]);
    }
    jISP[0][0] += Detector.Distance;
    jISP[1][0] += Detector.Distance;

    return TRUE;
  }
  return FALSE;
}

short NeutronIntersectsTube(VectorType IncomingNeutronDirection, VectorType NeutronStartingPosition, int l, VectorType iISP[], VectorType kISP[])
{
  /* Intersection of neutron with single tube:                                 */
  /* Identify entry/exit points kISP of rectangular tube with 0 mm walls first */
  /* to track neutrons through all touched tubes                               */
  /* Then iISP of the real tube                                                */

  VectorType NeutronPosition_tmp={0},NeutronPosition_tmp2={0},NeutronDirection_tmp={0};
  SampleType DetectorTubeSq={0}, DetectorTubeCirc={0};
  double tISP[2]={0};
  double RotMatrixTmp[3][3]={{0}}; //rotate circular tubes to match LineIntersectsCylinder
  int i=0,                         // counting variable
    j=0,k=0,                       // pixel position: j=column, k=row
    l_calc=0;                      // calc. layer from position for cross-checks 

  CopyVector(NeutronStartingPosition,NeutronPosition_tmp);
  CopyVector(IncomingNeutronDirection,NeutronDirection_tmp);
  
  /* rotate coordinate system such that x-axis is normal to detector surface and   
     move the origin of the coordinate system to the surface of the detector layer*/
  NeutronPosition_tmp[0] -= Detector.Distance;
  if(Detector.Theta_n!=0)
  {
    RotVector(RotSurface, NeutronPosition_tmp);
    RotVector(RotSurface, NeutronDirection_tmp);
  }
  NeutronPosition_tmp[0] += (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
  
  if(NeutronDirection_tmp[0]<0)
    Error("neutron coming from behind!");
  
  if(NeutronPosition_tmp[0]>0.0000001)
  { //traj. enters from side
    l_calc=floor(NeutronPosition_tmp[0]/Detector.PixelWidth[0])+1;
    if(l_calc!=1)
    { //layer edge
      return FALSE;
    }
  }

  /* move slighlty into the pixel and find tube l,j,k */
  NeutronPosition_tmp[0] += 0.001*( NeutronDirection_tmp[0]/fabs(NeutronDirection_tmp[0]) );
  NeutronPosition_tmp[1] += 0.001*( NeutronDirection_tmp[1]/fabs(NeutronDirection_tmp[0]) );
  NeutronPosition_tmp[2] += 0.001*( NeutronDirection_tmp[2]/fabs(NeutronDirection_tmp[0]) );

  j=(int)(floor(fabs(-Detector.Width/2-NeutronPosition_tmp[1])/Detector.PixelWidth[1])+1);
  k=(int)(floor(fabs(-Detector.Height/2-NeutronPosition_tmp[2])/Detector.PixelWidth[2])+1);

  if (j>Detector.NColumns || k>Detector.NRows)
    return FALSE;

  if (Detector.DG.Tube.eTubeOrient==VERTICAL)
  {
    FillRotMatrixY(RotMatrixTmp,M_PI_2);
    if(Detector.DG.Tube.bTubeShift)
    {
      if(l%2==0)
        j = (int)(floor(fabs(-Detector.Width/2+0.25*Detector.PixelWidth[1]-NeutronPosition_tmp[1])/Detector.PixelWidth[1])+1);
      else
        j = (int)(floor(fabs(-Detector.Width/2-0.25*Detector.PixelWidth[1]-NeutronPosition_tmp[1])/Detector.PixelWidth[1])+1);
    }
  }
  else 
  {
    FillRotMatrixZ(RotMatrixTmp,M_PI_2);
    if(Detector.DG.Tube.bTubeShift)
    {
      if(l%2==0)
        k=(int)(floor(fabs(-Detector.Height/2+0.25*Detector.PixelWidth[2]-NeutronPosition_tmp[2])/Detector.PixelWidth[2])+1);
      else
        k=(int)(floor(fabs(-Detector.Height/2-0.25*Detector.PixelWidth[2]-NeutronPosition_tmp[2])/Detector.PixelWidth[2])+1);
    }
  }
  
  DetectorTubeSq.Position[0] = 0.5*Detector.PixelWidth[0];
  DetectorTubeSq.Position[1] = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? -Detector.Width/2+(j-0.5)*Detector.PixelWidth[1] : 0;
  DetectorTubeSq.Position[2] = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? 0 : -Detector.Height/2+(k-0.5)*Detector.PixelWidth[2] ;
  
  if (Detector.DG.Tube.bTubeShift)
  {
    if(l%2==0)
    {
      DetectorTubeSq.Position[1] = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? -Detector.Width/2+(j-0.5+0.25)*Detector.PixelWidth[1] : 0;
      DetectorTubeSq.Position[2] = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? 0 : -Detector.Height/2+(k-0.5+0.25)*Detector.PixelWidth[2] ;
    }
    else 
    {
      DetectorTubeSq.Position[1]= (Detector.DG.Tube.eTubeOrient==VERTICAL) ? -Detector.Width/2+(j-0.5-0.25)*Detector.PixelWidth[1] : 0;
      DetectorTubeSq.Position[2]= (Detector.DG.Tube.eTubeOrient==VERTICAL) ? 0 : -Detector.Height/2+(k-0.5-0.25)*Detector.PixelWidth[2] ;
    }
  }

  DetectorTubeSq.SG.Cube.thickness = Detector.PixelWidth[0];
  DetectorTubeSq.SG.Cube.height = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? Detector.Height : Detector.PixelWidth[2];
  DetectorTubeSq.SG.Cube.width  = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? Detector.PixelWidth[1] : Detector.Width ;
  
  /* intersect neutron with that tube */
  CopyVector(NeutronStartingPosition,NeutronPosition_tmp2);
  NeutronPosition_tmp2[0]-=Detector.Distance;
  if(Detector.Theta_n!=0)
    RotVector(RotSurface, NeutronPosition_tmp2);

  NeutronPosition_tmp2[0] += (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
  SubVector (NeutronPosition_tmp2, DetectorTubeSq.Position);
   
  if ( LineIntersectsCube(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeSq.SG.Cube), tISP) )
  {
    for(i=0; i<3; i++)
    {
      kISP[1][i] = NeutronPosition_tmp2[i]+tISP[1]*NeutronDirection_tmp[i];
      kISP[0][i] = NeutronPosition_tmp2[i]+tISP[0]*NeutronDirection_tmp[i];
    }
    /* transform the kISPs to the old coordinate system*/
    AddVector (kISP[0], DetectorTubeSq.Position);
    AddVector (kISP[1], DetectorTubeSq.Position);
    kISP[0][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    kISP[1][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    if(Detector.Theta_n!=0)
    {
      RotBackVector(RotSurface, kISP[0]);
      RotBackVector(RotSurface, kISP[1]);
    }
    kISP[0][0] += Detector.Distance;
    kISP[1][0] += Detector.Distance;

    if (Detector.DG.Tube.eTubeShape==VT_TUBE_SQUARE && Detector.DG.Tube.wallThickness < 0.00000001)
    {
      CopyVector(kISP[0],iISP[0]);
      CopyVector(kISP[1],iISP[1]);
      return TRUE;
    }
    else if (Detector.DG.Tube.eTubeShape==VT_TUBE_SQUARE)
    {
      DetectorTubeSq.SG.Cube.thickness=Detector.PixelWidth[0]-2*Detector.DG.Tube.wallThickness;
      DetectorTubeSq.SG.Cube.height = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? Detector.Height : (Detector.PixelWidth[2]-2*Detector.DG.Tube.wallThickness) ;
      DetectorTubeSq.SG.Cube.width  = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? (Detector.PixelWidth[1]-2*Detector.DG.Tube.wallThickness) : Detector.Width ;
      if( LineIntersectsCube(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeSq.SG.Cube), tISP) )
      {
        for(i=0; i<3; i++)
        {
          iISP[1][i] = NeutronPosition_tmp2[i]+tISP[1]*NeutronDirection_tmp[i];
          iISP[0][i] = NeutronPosition_tmp2[i]+tISP[0]*NeutronDirection_tmp[i];
        }
      }
      else
        return FALSE;
    }
    else 
    { /* circular tube */ 
      DetectorTubeCirc.SG.Cyl.r=Detector.PixelWidth[0]/2-Detector.DG.Tube.wallThickness;
      DetectorTubeCirc.SG.Cyl.height = (Detector.DG.Tube.eTubeOrient==VERTICAL) ? Detector.Height : Detector.Width;
      CopyVector(DetectorTubeSq.Position,DetectorTubeCirc.Position);
      /* cylinder axis defined to be along x-axis in LineIntersectsCyliner */ 
      RotVector     (RotMatrixTmp, NeutronPosition_tmp2);
      RotVector     (RotMatrixTmp, NeutronDirection_tmp);

      if(LineIntersectsCylinder(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeCirc.SG.Cyl), tISP) )
      {
        for(i=0; i<3; i++)
        {
          iISP[1][i] = NeutronPosition_tmp2[i]+tISP[1]*NeutronDirection_tmp[i];
          iISP[0][i] = NeutronPosition_tmp2[i]+tISP[0]*NeutronDirection_tmp[i];
        }
      }
      else
        return FALSE;
      
      RotBackVector (RotMatrixTmp, iISP[0]);
      RotBackVector (RotMatrixTmp, iISP[1]);
    }

    /* transform the ISPs to the old coordinate system */
    AddVector (iISP[0], DetectorTubeSq.Position);
    AddVector (iISP[1], DetectorTubeSq.Position);
    iISP[0][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    iISP[1][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    if(Detector.Theta_n!=0)
    {
      RotBackVector(RotSurface, iISP[0]);
      RotBackVector(RotSurface, iISP[1]);
    }
    iISP[0][0] += Detector.Distance;
    iISP[1][0] += Detector.Distance;

    return TRUE;    
  }
 
  return FALSE;
}


/*****************************************************************************/
/* final detection position for different detector geometries                */
/*****************************************************************************/
void CubeDetLayerSpot(VectorType SP)
{
  /* cast SP on B/Li layer: 2 converter layers per detector layers gives NLayers+1 detection time spots  */

  VectorType Spot={0};
  /* Rotate coord. system around detector center such that coord. axes are parallel to cube axes */
  SP[0] -= Detector.Distance;
 
  if(Detector.Theta_n!=0)
  {
    RotVector(RotSurface, SP);
  }
  Spot[0]=(SP[0]+Detector.Thickness/2)/Detector.PixelWidth[0]+1;
  Spot[1]=(SP[1]+Detector.Width/2)/Detector.PixelWidth[1]+1;
  Spot[2]=(SP[2]+Detector.Height/2)/Detector.PixelWidth[2]+1;

  SP[0]= (Spot[0]-(floor(Spot[0]))>0.5) ?  (floor(Spot[0]))*Detector.PixelWidth[0]-Detector.Thickness/2 : (floor(Spot[0])-1)*Detector.PixelWidth[0]-Detector.Thickness/2 ;
  SP[1]= (Spot[1]-(floor(Spot[1]))>0.5) ?  (floor(Spot[1]))*Detector.PixelWidth[1]-Detector.Width/2 : (floor(Spot[1])-1)*Detector.PixelWidth[1]-Detector.Width/2 ;
  SP[2]= (Spot[2]-(floor(Spot[2]))>0.5) ?  (floor(Spot[2]))*Detector.PixelWidth[2]-Detector.Height/2 : (floor(Spot[2])-1)*Detector.PixelWidth[2]-Detector.Height/2 ;

  /*Rotate back to orig. coordinates */
  if(Detector.Theta_n!=0)
  {
    RotBackVector(RotSurface, SP);
  }
  SP[0] += Detector.Distance;
}

void CubeDetSpot(VectorType iSP, VectorType DetSpot)
{
  int l=0; //layer
  VectorType iSP_copy={0};

  CopyVector(iSP,iSP_copy);

  /* Rotate coord. system around detector center such that coord. axes are parallel to cube axes */
  iSP_copy[0] -= Detector.Distance;
  if(Detector.Theta_n!=0)
  {
    RotVector(RotSurface, iSP_copy);
  }

  DetSpot[0]=(floor((iSP_copy[0]+Detector.Thickness/2)/Detector.PixelWidth[0])+0.5)*Detector.PixelWidth[0]-Detector.Thickness/2;
  DetSpot[1]=(floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.5)*Detector.PixelWidth[1]-Detector.Width/2;
  DetSpot[2]=(floor((iSP_copy[2]+Detector.Height/2)/Detector.PixelWidth[2])+0.5)*Detector.PixelWidth[2]-Detector.Height/2;

  if(Detector.DG.Tube.bTubeShift)
  {
    l=floor((iSP_copy[0]+Detector.Thickness/2)/Detector.PixelWidth[0]);
    if(Detector.DG.Tube.eTubeOrient==VERTICAL)
    {
      DetSpot[1]= (l%2) ? 
        (floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.75)*Detector.PixelWidth[1]-Detector.Width/2 :
        (floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.25)*Detector.PixelWidth[1]-Detector.Width/2;
    }
    else 
    {
      DetSpot[2]= (l%2)?
        (floor((iSP_copy[2]+Detector.Height/2)/Detector.PixelWidth[2])+0.75)*Detector.PixelWidth[2]-Detector.Height/2 :
        (floor((iSP_copy[2]+Detector.Height/2)/Detector.PixelWidth[2])+0.25)*Detector.PixelWidth[2]-Detector.Height/2;
    }
  }

  /*Rotate back to orig. coordinates */
  if(Detector.Theta_n!=0){
    RotBackVector(RotSurface, DetSpot);
  }
  DetSpot[0] += Detector.Distance;
}

void CylinderDetSpot(VectorType SP, VectorType DetSpot)
{
  double     STheta, SPhi, CTheta, SpotTheta, SpotR,
    archpos, I, SPHeight;
  VectorType nsp;
  double dTheta=Detector.Width/(2.0*Detector.Distance);
  double GTheta=Detector.Theta;
  double constphi = atan(Detector.Height/2./Detector.Distance)*2./Detector.NRows;
  double PhiDet=constphi*Detector.NRows;

  CopyVector(SP,nsp);
  RotBackVector(RotMatrix,nsp);
  SPHeight = nsp[Detector.DG.Cyl.eAxis];

  SpotR=sqrt( sq(nsp[0])+sq(nsp[1]) ); 
  if (Detector.DG.Cyl.eAxis==VT_SC_X)	
    SpotR=sqrt( sq(nsp[1])+sq(nsp[2]) ); 
  else if(Detector.DG.Cyl.eAxis==VT_SC_Y)	
    SpotR=sqrt( sq(nsp[0])+sq(nsp[2]) ); 
  SpotR= Detector.DG.Cyl.r + (floor((SpotR-Detector.DG.Cyl.r)*Detector.NLayers/Detector.Thickness)+0.5)*Detector.Thickness/Detector.NLayers;
	
  NormVector(nsp);
  CartesianToSpherical(nsp, &STheta, &SPhi);
	
  CTheta=atan(cos(SPhi)*tan(STheta)); 
  if(Detector.Direction[0]) { CTheta=SPhi; GTheta=Detector.Phi; }
  else if (Detector.Direction[1]) CTheta=atan(sin(SPhi)*tan(STheta));
  
  if(Detector.DG.Cyl.eAxis!=VT_SC_X && STheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
  
  archpos= fabs(CTheta-(GTheta+dTheta));  //counted from 0 to radians of archlength
  I=floor(Detector.NColumns*archpos/(2.0*dTheta));
  SpotTheta=GTheta+dTheta -((I+0.5)/Detector.NColumns)*(2.0*dTheta);
  
  switch (Detector.DG.Cyl.eAxis)
  {
    case VT_SC_X:
      DetSpot[1]=SpotR*cos(SpotTheta);
      DetSpot[2]=SpotR*sin(SpotTheta);
      break;
    case VT_SC_Y:
      DetSpot[0]=SpotR*cos(SpotTheta);
      DetSpot[2]=SpotR*sin(SpotTheta);
      break;
    case VT_SC_Z: 
      DetSpot[0]=SpotR*cos(SpotTheta);
      DetSpot[1]=SpotR*sin(SpotTheta);
      break;	
  }

  if (Detector.DG.Cyl.bCnstPhi == 0) 
    DetSpot[Detector.DG.Cyl.eAxis]=(floor((SPHeight+Detector.Height/2)/Detector.PixelWidth[2])+0.5)*Detector.PixelWidth[2]-Detector.Height/2;
  else 
    DetSpot[Detector.DG.Cyl.eAxis]=tan( (floor((PhiDet/2+atan2(SPHeight,Detector.Distance))/constphi)+0.5)*constphi-PhiDet/2 )*Detector.Distance;
  
  RotVector(RotMatrix,DetSpot);
}


/******************************************************************************/
/** GetLambdaProbFromEff: returns efficiency as a function of wavelength     **/
/** GetXsec: returns absorption cross section as a function of wavelength    **/
/******************************************************************************/
double GetLambdaProbFromEff(const double lambda, const TotalID NeutronID) 
{
  int i;

  if (lambda < Eff.data[0].Lambda) 
  {
    CountMessageID(DET_L_RANGE_TOO_SMALL, NeutronID);
    return Eff.data[0].Eff;
  } 
  else if (lambda > Eff.data[Eff.maxdata-1].Lambda)
  { 
    CountMessageID(DET_L_RANGE_TOO_SMALL, NeutronID);
    return Eff.data[Eff.maxdata-1].Eff;
  }
  else 
  {
    for (i = 1; i<=Eff.maxdata; i++) 
    {
      if (lambda < Eff.data[i].Lambda)
        return Eff.data[i].Eff + (Eff.data[i].Eff-Eff.data[i-1].Eff)/(Eff.data[i].Lambda-Eff.data[i-1].Lambda) * (lambda-Eff.data[i].Lambda);
    }
  }
  return -1.;
}

double GetXsec(int h_absorbertype, double h_lambda)
{
  double h_sigma=0;

  switch (h_absorbertype)
  {
    case 0: //BF3
    case 2: //solid B
      h_sigma=3845.241+2139*(h_lambda-1.798);
      break;
    case 1: //He3
      h_sigma=5319.31+2955*(h_lambda-1.798);
      break;
    case 3: //Li6
      h_sigma=939.223+521.62*(h_lambda-1.798);
      break;
    default: //other, from file
      h_sigma=0;
      break;
  }
  h_sigma*=1E-28; //return in SI unit: b -> m^2

  return h_sigma;
}


/*****************************************************************************/
/* checks and completes detector geometry                                    */
/*****************************************************************************/
void CheckAndAdjustDetectorInput(VtDetType type, VtDetGeom geom)
{
  if (geom == VT_NO_DET_GEOM)
   Error("You have to specify a detector geometry (flat vs cylindrical) with -G.");

  if (type == VT_NO_DET_TYPE)
  {
    Error("You have to specify a detector type (area/volume vs. tube) with -a.");
  }
  else if (geom==VT_DET_CYL && type==VT_DET_TUBE)
  {
    Error("tube detector only possible for flat geometry. Use detector array to build cylindrical shape out of tubes");
  }
  else if (geom==VT_DET_FLAT && type==VT_DET_TUBE)
  {
    eDetProp = VT_FLAT_TUBES;
  }
  else if (geom==VT_DET_CYL && type==VT_DET_AREA)
  {
    eDetProp = VT_CYL_AREA;
  }
  else if (geom==VT_DET_FLAT && type==VT_DET_AREA)
  {
    eDetProp = VT_FLAT_AREA;
  }

  if(Detector.Phi<0)
    Error("Angle phi hast to be between 0 and 360 deg (specify with -P).");
  if(Detector.Theta<0 || Detector.Theta>180)
    Error("Angle theta has to be between 0 and 180 deg (specify with -T).");
  if(Detector.Distance<0)
    Error("You have to specify a distance with -D.");
  if(eDetProp!=VT_CYL_AREA && Detector.Distance<Detector.Thickness/2)
    Error("Detector distance smaller than half the detector thickness!");
  if(Detector.Height<0)
    Error("You have to specify a height with -h.");
  if(Detector.Width<0)
    Error("You have to specify a width with -w.");
  if(Detector.Thickness<0)
    Error("You have to specify a thickness with -t.");
  if(Detector.NRows<0)
    Error("You have to specify the number of rows with -r.");
  if(Detector.NColumns<0)
    Error("You have to specify the number of columns with -c.");
  if(Detector.NLayers<0)
    Error("You have to specify the number of layers with -n.");

  Detector.PixelWidth[0]=Detector.Thickness/Detector.NLayers;
  Detector.PixelWidth[1]=Detector.Width/Detector.NColumns;
  Detector.PixelWidth[2]=Detector.Height/Detector.NRows;

  if (Detector.eAbsMat==VT_NO_ABS_MAT)
  {
    Error("You have to specify an absorber/converter type with -m.");
  }
  else if (Detector.eAbsMat==VT_GAS_BF3 || Detector.eAbsMat==VT_GAS_HE3)
  {
    Detector.GasPressure    = Detector.AbsPar1;
    Detector.GasTemperature = Detector.AbsPar2;
    if (Detector.GasPressure < 0.0 || Detector.GasTemperature < 0.0)
      Error("For a gaseous detector you have to specify gas pressure (-p) and temperature (-k).");
  }
  else if (Detector.eAbsMat == VT_SOLID_B10 || Detector.eAbsMat <= VT_SOLID_LI6)
  {
    Detector.SolidAbsThickness = Detector.AbsPar1;
    Detector.SolidAtomDensity  = Detector.AbsPar2;
    if (Detector.SolidAbsThickness < 0.0 || Detector.SolidAtomDensity < 0.0)
      Error("For a solid detection material you have to specify the solid layer thickness (-p) and atom density (-k).");
  }
  if (Detector.eAbsMat==VT_ABS_OTHER && Detector.EfficiencyMod>=1)
    Error("Efficiency modifyer is used as total efficiency (with absorber type \"other\") and has to be < 1");

  if (eDetProp==VT_FLAT_TUBES)
  { 
    if(Detector.Resolution[0]!=0)
      Warning("x-resolution not 0 but has no meaning in tube detector (hence will not be used).");
    if (Detector.DG.Tube.eTubeOrient==VERTICAL)
    {
      if(Detector.PixelWidth[0]!=Detector.PixelWidth[1])
        Error("tube detector requires Thickness/number_of_layers=Width/number_of_columns (diameter of tube) for vertical tube orientation \n");
      if(Detector.Resolution[1]!=0)
        Warning("hor. resolution not 0 but has no meaning in vertical tubes (hence will not be used).");
    }
    else if (Detector.DG.Tube.eTubeOrient==HORIZONTAL)
    {
      if(Detector.PixelWidth[0]!=Detector.PixelWidth[2])
        Error("tube detector requires Thickness/number_of_layers=Height/number_of_rows (diameter of tube) for horizontal tube orientation");
      if(Detector.Resolution[2]!=0)
        Warning("vert. resolution not 0 but has no meaning in horizontal tubes (hence will not be used).");
    }
    /*add tubes if shifted so whole area is covered */
    /*only if resolution=0 to avoid adding tubes twice*/
    if(Detector.DG.Tube.bTubeShift)
    {
      if (Detector.DG.Tube.eTubeOrient==VERTICAL && Detector.Resolution[1]==0)
      {
        Detector.NColumns += 2;
        Detector.Width += 2*Detector.PixelWidth[1];
      }
      else if (Detector.DG.Tube.eTubeOrient==HORIZONTAL && Detector.Resolution[2]==0)
      {
        Detector.NRows += 2;
        Detector.Height += 2*Detector.PixelWidth[2];
      }
    }
  }
  else if(eDetProp==VT_CYL_AREA)
  { 
    if (Detector.DG.Cyl.eAxis==VT_NO_SC_AXIS)
    {
      Warning("Cylinder geometry chosen but no cylinder axis given (specify with -x). Cylinder axis is set to be z axis per default.");
      Detector.DG.Cyl.eAxis=VT_SC_Z;
    }
    if(Detector.Theta_n!=0 || Detector.Phi_n!=0)
      Error("Surface inclination only possible for flat detector! For cylindrical geometry, theta_n and phi_n have to be 0 deg.");
  }

  //extend detector to avoid intensity drop at borders
  // only in y,z: extension in x leads to higher efficiency!
  if(Detector.Resolution[2]!=0)
  {
     Detector.Height+=2*(floor(Detector.Resolution[2]/ Detector.PixelWidth[2])+1)* Detector.PixelWidth[2];
     Detector.NRows +=2*(floor(Detector.Resolution[2]/ Detector.PixelWidth[2])+1);
  }
  if(Detector.Resolution[1]!=0)
  {
     Detector.Width   +=2*(floor(Detector.Resolution[1]/ Detector.PixelWidth[1])+1)* Detector.PixelWidth[1];
     Detector.NColumns+=2*(floor(Detector.Resolution[1]/ Detector.PixelWidth[1])+1);
  }
}

