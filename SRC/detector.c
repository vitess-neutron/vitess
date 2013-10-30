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
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "init.h"
#include "matrix.h"
#include "softabort.h"
#include "intersection.h"
#include "message.h"
#include "detector.h"

/* global variables */
DetectorType Detector; 
long       GenNeutrons=10,   // repetition: multiply neutrons to get diff. interaction lengths (probability from integration region L/GenNeutrons)
           lost=0;           // give Warning if neutron intersetcs tube detector but tube in which interaction happens is not found
double     RotMatrix[3][3],
           RotSurface[3][3]; // matrix rotation coordinate system such that detector surface is perpendicular to x

EffFile Eff = {0};
FILE *outFile;
char *DetectorOutputFileName;

static FILE * tryOpen(const char *fn, const char *s);

int main(int argc, char *argv[])
{
  Neutron WorkNeutron,           // working copy
    OutNeutron,                  // working copy for WriteNeutron
    DrawNeutron;                 // working copy for visualization (draws scattering point instead of DetSpot)
  VectorType ISP[2],iISP[2],     // intersection points with the detector
    jISP[2]={{0}}, kISP[2]={{0}};// intermediate intersection points with layers/tubes in tube detector 
  VectorType DetSpot,            // final detection position given out by detector
    DetSignal,                   // intermediate detection position (including gaussian smear from resolution)
    SP,                          // interaction position
    vShift;                      // for visualization: vector in neutron direction with length equal to distance of (last) detector (part in array) center
  double TimeTillScattering=0,   // neutron flight time until interaction
    FullLengthInDetector=0,      // length of neutron trajectorie in overall detector volume
    LengthInAbsorberMaterial=0,  // length of neutron trajectory in active detector volume
    LengthTillScattering=0,      // length of neutron trajectory in active detector volume until interaction
    LengthModifyer=0,            // scale lengths in case of thin converter layers
    ScatteringProb=0,            // probability of interaction
    N=0,                         // particle density
    sigma=0,                     // absorption cross-section
    NSigma=0,                    // for const. efficieny
    FluxDetected=0,              // detected flux
    DistanceTubeLayerExits=0;    // distance between tube exit and layer exit points
  const double kB=1.3806504E-23; // boltzman constant in [JK-1]
  long   i,j,l,z;                // loop counting
  int   NeutCount=0,           // count neutron copies for repetition (=GenNeutrons) > 1
         NumDetected=0;          // number of detected neutrons
  short FoundTube=0;             // tag for tube in which interaction happens

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_DETECTOR);
  print_module_name("detector 1.11");

  /* module specific initialization */
  OwnInit(argc, argv);

  /* RotMatrix will rotate a Vector to a frame in which the middle of the */
  /* Detector sits on the x-axis, i.e. Detector.Direction (cyl-axis for cyl. det) defines new x axis*/
  RotMatrixX(Detector.Direction, RotMatrix);


  /* Get the neutrons from the file */
  DECLARE_ABORT

  while((ReadNeutrons())!= 0)
  {
    CHECK

    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK
	
      //drop neutrons that don't pass the color filter
      if ( (Detector.detectColor > -1) && (InputNeutrons[i].Color != Detector.detectColor) )  
	continue;

      //pass on neutrons detected by previous detector parts,
      //last detector in array removes tag and writes output file
      if( floor(InputNeutrons[i].Color/10000)==1 ){
	if(!Detector.array){
	  InputNeutrons[i].Color-=10000;
	  if(DetectorOutputFileName)
	    fprintf(outFile,"\n   %10.4f  %10.4f  %10.4f   %10.4f     %2.3e     %d",InputNeutrons[i].Position[0],InputNeutrons[i].Position[1],InputNeutrons[i].Position[2],InputNeutrons[i].Time,InputNeutrons[i].Probability,InputNeutrons[i].Color);
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
      N=(Detector.Absorbertype<2)?(Detector.GasPressure*1e5/(Detector.GasTemperature*kB)):(Detector.SolidAtomDensity*1E27);

      if(NeutronIntersectsDetector(&(WorkNeutron),ISP)) 
      {

        /* determine the length of the path through the whole detector volume */
        FullLengthInDetector = DistVector(ISP[0], ISP[1]);

	/*correct for dead zones in tube geometry */
	if(Detector.Geom==2){ 
	  FullLengthInDetector=0;
	  for (l=1; l<=Detector.NLayers; l++){
	    z=0; DistanceTubeLayerExits=0;
	    if(NeutronIntersectsLayer(WorkNeutron.Vector,ISP[0],l,jISP)){	      
	      CopyVector(jISP[0],kISP[1]);
	      while (DistVector(jISP[1],kISP[1])>0.001*Detector.PixelWidth[0]){
		if( fabs(DistanceTubeLayerExits-DistVector(jISP[1],kISP[1]))<0.000001)
		  break;
		++z; DistanceTubeLayerExits=DistVector(jISP[1],kISP[1]);
		if(NeutronIntersectsTube(WorkNeutron.Vector,kISP[1],l,iISP,kISP))
		  FullLengthInDetector+=DistVector(iISP[0],iISP[1]);
		if(z>Detector.NLayers+1 && z>Detector.NColumns+1 && z>Detector.NRows+1)
		  myExit("\n ERROR: tube with exit point close to layer exit not found!");
	      }
	    }		
	  } 
	}
	
	/* total cross-section (in m^2) */
	sigma=GetXsec(Detector.Absorbertype,WorkNeutron.Wavelength);

	if (Detector.usage==1)
	  GenNeutrons=1;
	//------------------------------------------------------

        for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++){

	  /* determine the interaction point in the detector volume and the detector signal point*/
	  LengthTillScattering = MonteCarlo(0,FullLengthInDetector); 
	  for(j=0; j<3; j++)
	    SP[j]= ISP[0][j] +LengthTillScattering*WorkNeutron.Vector[j];

	  /* find interaction point in tubes */
	  if(Detector.Geom==2){ 
	    LengthInAbsorberMaterial=0; FoundTube=0;
	    for (l=1; l<=Detector.NLayers; l++){
	      z=0; DistanceTubeLayerExits=0;
	      if(NeutronIntersectsLayer(WorkNeutron.Vector,ISP[0],l,jISP)){	      
		CopyVector(jISP[0],kISP[1]);
     		while (DistVector(jISP[1],kISP[1])>0.0001){
		  if( fabs(DistanceTubeLayerExits-DistVector(jISP[1],kISP[1]))<0.000001)
		    break;
		  ++z; DistanceTubeLayerExits=DistVector(jISP[1],kISP[1]);
		  if(NeutronIntersectsTube(WorkNeutron.Vector,kISP[1],l,iISP,kISP))
		    LengthInAbsorberMaterial+=DistVector(iISP[0],iISP[1]);
		  if(z>Detector.NLayers+1 && z>Detector.NColumns+1 && z>Detector.NRows+1)
		    myExit("ERROR: tube with exit point close to layer exit not found!");
		}
		if(LengthInAbsorberMaterial>LengthTillScattering){
		  FoundTube=1;
		  for(j=0; j<3; j++){
		    SP[j]=iISP[1][j]-WorkNeutron.Vector[j]*(LengthInAbsorberMaterial-LengthTillScattering);
		  }
		  break;
		}
	      }		
	    } 
	  }

	  CopyVector(SP,DetSignal);
	  
	  /* resolution */
	  if(Detector.Resolution[1]>0 && (Detector.Geom==0 || !Detector.DG.Tube.vertTubeOrientation) )
	    DetSignal[1]=DistrGauss(DetSignal[1],Detector.Resolution[1]); 
	  if(Detector.Resolution[2]>0 && (Detector.Geom==0 || Detector.DG.Tube.vertTubeOrientation) )
	    DetSignal[2]=DistrGauss(DetSignal[2],Detector.Resolution[2]);
	  if(Detector.Resolution[0]>0 && Detector.Geom==0 )
	    DetSignal[0]=DistrGauss(DetSignal[0],Detector.Resolution[0]);
	  
	  /* interaction probability */
	  if (Eff.maxdata > 0) { //efficiency from file
	    ScatteringProb = GetLambdaProbFromEff(WorkNeutron.Wavelength,WorkNeutron.ID)* Detector.EfficiencyMod;
	  } 
	  else {
	    switch(Detector.Absorbertype){
	    case 0:  //BF3
	    case 1:  //He3
	      ScatteringProb = N*sigma*exp(-N*sigma*LengthTillScattering/100) * FullLengthInDetector/100 * Detector.EfficiencyMod;
	      break;
	    case 2:  //solid B10
	    case 3:  // Li6
	      // solid layer approximation: scale length in material with layer_thickness/total_thickness
	      // 2 solid layers per tube layer
	      LengthModifyer=Detector.NLayers*Detector.SolidAbsorberthickness/Detector.Thickness;
	      if(Detector.Geom==2)           
		LengthModifyer*=2;
	      ScatteringProb = N*sigma*exp(-N*sigma*(LengthModifyer*LengthTillScattering)/100) * (LengthModifyer*FullLengthInDetector)/100* Detector.EfficiencyMod;
	      break; 
	    case 5:  // other; use wavelength-independent input eff
	    default:
	      NSigma=-log(1-Detector.EfficiencyMod)/Detector.Thickness;
	      ScatteringProb = NSigma*exp(-NSigma*LengthTillScattering) * FullLengthInDetector;
	      break;
	    }
	  }

	  if(Detector.Geom==2 && !FoundTube){
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
	  if(Detector.array)
	    OutNeutron.Color+=10000;
 
	  RotBackVector(RotMatrix,SP);
	  RotBackVector(RotMatrix,DetSignal);
	  RotBackVector(RotMatrix,DetSpot);	
	      		      
          if (Detector.usage==0) {           // normal
            CopyVector(DetSpot, OutNeutron.Position);
            NormVector(DetSpot);
            CopyVector(DetSpot, OutNeutron.Vector);
          }
          else if (Detector.usage==1){      //monitor only
	    OutNeutron.Probability = WorkNeutron.Probability;
	    CopyVector(SP, OutNeutron.Position);
	    RotBackVector(RotMatrix,OutNeutron.Vector);
	  }
	  else if (Detector.usage==2){ // grid off
	    CopyVector(DetSignal, OutNeutron.Position);
            NormVector(DetSignal);
            CopyVector(DetSignal, OutNeutron.Vector);
	  }


	  // write out neutrons that shall be detected	
          if ( OutNeutron.Probability>wei_min )  {
	    if (Detector.addColor > 0) 
	      OutNeutron.Color += Detector.addColor;
	    WriteNeutron(&OutNeutron);	
	    NumDetected++;
	    FluxDetected+=OutNeutron.Probability;
	    if(!Detector.array && DetectorOutputFileName)
	      fprintf(outFile,"\n   %10.4f  %10.4f  %10.4f   %10.4f     %2.3e     %d",OutNeutron.Position[0],OutNeutron.Position[1],OutNeutron.Position[2],OutNeutron.Time,OutNeutron.Probability,OutNeutron.Color);
	  }

         // write interaction point - only once per incoming trajectory
          if (NeutCount < 1){
	    DrawNeutron=OutNeutron;
	    CopyVector(SP, DrawNeutron.Position);
	    if(!Detector.array)
	      DrawNeutron.Color+=10000;
            WriteIAP(&DrawNeutron, VT_DETECTED);
	  }

        } /* loop count */
      } 
      else /* if neutron does not intersect detector */ 
      {
	
	if(Detector.array)
	  WriteNeutron(&InputNeutrons[i]);
	else { //draw trajectories of undetected neutrons up to detector end
	  DrawNeutron=WorkNeutron;
	  RotBackVector(RotMatrix, DrawNeutron.Vector);
	  RotBackVector(RotMatrix, DrawNeutron.Position);
	  CopyVector(DrawNeutron.Vector, vShift);
	  MultiplyByScalar(vShift, (Detector.Distance+Detector.Thickness/2));
	  AddVector(DrawNeutron.Position,vShift);
	  WriteIAP(&DrawNeutron, VT_OUTSIDE);
	}
	
      }
    } //for(i=0; i<NumNeutGot; i++)
  } //while((ReadNeutrons())!= 0)


  fprintf(LogFilePtr,"\n Neutrons detected in this detector: %du trajectories (%11.4e n/s) \n",NumDetected,FluxDetected);

 my_exit:
  /* Do module specific cleanups */
  OwnCleanup();

  if(outFile)
    fclose(outFile);

  /* Do the general cleanup */
  // the origin of the co-ordinate system remains at the sample  
    Cleanup(0.0,0.0,0.0, 0.0,0.0);
  

  return 0;
}

double GetLambdaProbFromEff(const double lambda, const TotalID NeutronID) {
  int i;

  if (lambda < Eff.data[0].Lambda) {
    CountMessageID(DET_L_RANGE_TOO_SMALL, NeutronID);
    return Eff.data[0].Eff;
  } 
  else if (lambda > Eff.data[Eff.maxdata-1].Lambda){ 
    CountMessageID(DET_L_RANGE_TOO_SMALL, NeutronID);
    return Eff.data[Eff.maxdata-1].Eff;
  }
  else {
    for (i = 1; i<=Eff.maxdata; i++) {
      if (lambda < Eff.data[i].Lambda)
        return Eff.data[i].Eff + (Eff.data[i].Eff-Eff.data[i-1].Eff)/(Eff.data[i].Lambda-Eff.data[i-1].Lambda) * (lambda-Eff.data[i].Lambda);
    }
  }
  return -1.;
}

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
  if(Detector.Theta_n!=0){
    RotVector(RotSurface, NeutronPosition);
    RotVector(RotSurface, NeutronVector);
  }

  DetectorCube.height = Detector.Height;
  DetectorCube.width = Detector.Width;
  DetectorCube.thickness = Detector.Thickness;

  if(LineIntersectsCube(NeutronPosition, NeutronVector, &DetectorCube, t)) 
    {
      for(i=0; i<3; i++){	
	ISP[1][i] = NeutronPosition[i]+t[1]*NeutronVector[i];
	ISP[0][i] = NeutronPosition[i]+t[0]*NeutronVector[i];
      }

      /* transform the ISPs to the old coordinate system*/
      if(Detector.Theta_n!=0){
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
	  CountMessageID(DET_TRAJ_INSIDE, Nin->ID);
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
 
  if(LineIntersectsCylinder(Position, Direction, &CylDetector, Int))  {
    for(i=0; i<3; i++) {	
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

  if(Detector.DG.Cyl.axis!=0 && ntheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
	
  GTheta= Detector.Direction[0] ? Detector.Phi : Detector.Theta;
	
  if( !(fabs(CTheta-GTheta) < dTheta || fabs(CTheta+2*M_PI-GTheta) < dTheta) )
    return FALSE; 

  CylDetector.r += Detector.Thickness;
  if(LineIntersectsCylinder(Position, Direction, &CylDetector, Outt)){
    for(i=0; i<3; i++){
      OutISP[1][i] = Position[i]+Outt[1]*Direction[i];
      OutISP[0][i] = Position[i]+Outt[0]*Direction[i];
    }
  }
  else
    return FALSE;
			
  CylDetector.r-=Detector.Thickness;
  if( (fabs(InISP[1][0]) < CylDetector.height*0.99999/2.0) && (fabs(OutISP[1][0]) <CylDetector.height*0.99999/2.0) ){
    CopyVector(InISP[1],ISP[0]);
    CopyVector(OutISP[1],ISP[1]);
    return TRUE;
  }
  return FALSE;
}

short NeutronIntersectsLayer(VectorType dir, VectorType pos, int l, VectorType jISP[]){
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
  if(Detector.Theta_n!=0){
    RotVector(RotSurface, NeutronPosition_tmp);
    RotVector(RotSurface, NeutronDirection_tmp);
  }
  NeutronPosition_tmp[0] += (Detector.Thickness/2 - (l-0.5)*Detector.PixelWidth[0]);
  if(Detector.DG.Tube.tubeshift){
    if(Detector.DG.Tube.vertTubeOrientation)
      (l%2==0) ? (NeutronPosition_tmp[1] -= 0.25*Detector.PixelWidth[1]) : (NeutronPosition_tmp[1] += 0.25*Detector.PixelWidth[1]);
    else
      (l%2==0) ? (NeutronPosition_tmp[2] -= 0.25*Detector.PixelWidth[2]) : (NeutronPosition_tmp[2] += 0.25*Detector.PixelWidth[2]);
  }

  if( LineIntersectsCube(NeutronPosition_tmp, NeutronDirection_tmp, &DetectorLayer, tISP) ){
    for(i=0; i<3; i++){
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

/* Intersection of neutron with single tube:                                 */
/* Identify entry/exit points kISP of rectangular tube with 0 mm walls first */
/* to track neutrons through all touched tubes                               */
/* Then iISP of the real tube                                                */
short NeutronIntersectsTube(VectorType IncomingNeutronDirection, VectorType NeutronStartingPosition, int l, VectorType iISP[], VectorType kISP[]){
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
  if(Detector.Theta_n!=0){
    RotVector(RotSurface, NeutronPosition_tmp);
    RotVector(RotSurface, NeutronDirection_tmp);
  }
  NeutronPosition_tmp[0] += (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
  
  if(NeutronDirection_tmp[0]<0)
    fprintf(LogFilePtr,"\nERROR: neutron coming from behind!");
  
  if(NeutronPosition_tmp[0]>0.0000001){ //traj. enters from side
    l_calc=floor(NeutronPosition_tmp[0]/Detector.PixelWidth[0])+1;
    if(l_calc!=1){ //layer edge
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

  if(Detector.DG.Tube.vertTubeOrientation){
    FillRotMatrixY(RotMatrixTmp,M_PI_2);
    if(Detector.DG.Tube.tubeshift){
      if(l%2==0)
	j = (int)(floor(fabs(-Detector.Width/2+0.25*Detector.PixelWidth[1]-NeutronPosition_tmp[1])/Detector.PixelWidth[1])+1);
      else
	j = (int)(floor(fabs(-Detector.Width/2-0.25*Detector.PixelWidth[1]-NeutronPosition_tmp[1])/Detector.PixelWidth[1])+1);
    }
  }
  else {
    FillRotMatrixZ(RotMatrixTmp,M_PI_2);
    if(Detector.DG.Tube.tubeshift){
      if(l%2==0)
	k=(int)(floor(fabs(-Detector.Height/2+0.25*Detector.PixelWidth[2]-NeutronPosition_tmp[2])/Detector.PixelWidth[2])+1);
      else
	k=(int)(floor(fabs(-Detector.Height/2-0.25*Detector.PixelWidth[2]-NeutronPosition_tmp[2])/Detector.PixelWidth[2])+1);
    }
  }
  
  DetectorTubeSq.Position[0]= 0.5*Detector.PixelWidth[0];
  DetectorTubeSq.Position[1]= Detector.DG.Tube.vertTubeOrientation ? -Detector.Width/2+(j-0.5)*Detector.PixelWidth[1] : 0;
  DetectorTubeSq.Position[2]= Detector.DG.Tube.vertTubeOrientation ? 0 : -Detector.Height/2+(k-0.5)*Detector.PixelWidth[2] ;
  
  if (Detector.DG.Tube.tubeshift){
    if(l%2==0){
      DetectorTubeSq.Position[1]= Detector.DG.Tube.vertTubeOrientation ? -Detector.Width/2+(j-0.5+0.25)*Detector.PixelWidth[1] : 0;
      DetectorTubeSq.Position[2]= Detector.DG.Tube.vertTubeOrientation ? 0 : -Detector.Height/2+(k-0.5+0.25)*Detector.PixelWidth[2] ;
    }
    else {
      DetectorTubeSq.Position[1]= Detector.DG.Tube.vertTubeOrientation ? -Detector.Width/2+(j-0.5-0.25)*Detector.PixelWidth[1] : 0;
      DetectorTubeSq.Position[2]= Detector.DG.Tube.vertTubeOrientation ? 0 : -Detector.Height/2+(k-0.5-0.25)*Detector.PixelWidth[2] ;
    }
  }

  DetectorTubeSq.SG.Cube.thickness = Detector.PixelWidth[0];
  DetectorTubeSq.SG.Cube.height = Detector.DG.Tube.vertTubeOrientation ? Detector.Height : Detector.PixelWidth[2];
  DetectorTubeSq.SG.Cube.width = Detector.DG.Tube.vertTubeOrientation ? Detector.PixelWidth[1] : Detector.Width ;
  
  /* intersect neutron with that tube */
  CopyVector(NeutronStartingPosition,NeutronPosition_tmp2);
  NeutronPosition_tmp2[0]-=Detector.Distance;
  if(Detector.Theta_n!=0)
    RotVector(RotSurface, NeutronPosition_tmp2);

  NeutronPosition_tmp2[0] += (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
  SubVector (NeutronPosition_tmp2, DetectorTubeSq.Position);
   
  if( LineIntersectsCube(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeSq.SG.Cube), tISP) ){
    for(i=0; i<3; i++){
      kISP[1][i] = NeutronPosition_tmp2[i]+tISP[1]*NeutronDirection_tmp[i];
      kISP[0][i] = NeutronPosition_tmp2[i]+tISP[0]*NeutronDirection_tmp[i];
    }
    /* transform the kISPs to the old coordinate system*/
    AddVector (kISP[0], DetectorTubeSq.Position);
    AddVector (kISP[1], DetectorTubeSq.Position);
    kISP[0][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    kISP[1][0] -= (Detector.Thickness/2 - (l-1)*Detector.PixelWidth[0]);
    if(Detector.Theta_n!=0){
      RotBackVector(RotSurface, kISP[0]);
      RotBackVector(RotSurface, kISP[1]);
    }
    kISP[0][0] += Detector.Distance;
    kISP[1][0] += Detector.Distance;
    if(Detector.DG.Tube.rectXsec && Detector.DG.Tube.wallThickness<0.00000001){
      CopyVector(kISP[0],iISP[0]);
      CopyVector(kISP[1],iISP[1]);
      return TRUE;
    }
    else if (Detector.DG.Tube.rectXsec){
      DetectorTubeSq.SG.Cube.thickness=Detector.PixelWidth[0]-2*Detector.DG.Tube.wallThickness;
      DetectorTubeSq.SG.Cube.height = Detector.DG.Tube.vertTubeOrientation ? Detector.Height : (Detector.PixelWidth[2]-2*Detector.DG.Tube.wallThickness) ;
      DetectorTubeSq.SG.Cube.width = Detector.DG.Tube.vertTubeOrientation ? (Detector.PixelWidth[1]-2*Detector.DG.Tube.wallThickness) : Detector.Width ;
      if( LineIntersectsCube(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeSq.SG.Cube), tISP) ){
	for(i=0; i<3; i++){
	  iISP[1][i] = NeutronPosition_tmp2[i]+tISP[1]*NeutronDirection_tmp[i];
	  iISP[0][i] = NeutronPosition_tmp2[i]+tISP[0]*NeutronDirection_tmp[i];
	}
      }
      else
	return FALSE;
    }
    else { /* circular tube */ 
      DetectorTubeCirc.SG.Cyl.r=Detector.PixelWidth[0]/2-Detector.DG.Tube.wallThickness;
      DetectorTubeCirc.SG.Cyl.height = Detector.DG.Tube.vertTubeOrientation ? Detector.Height : Detector.Width;
      CopyVector(DetectorTubeSq.Position,DetectorTubeCirc.Position);
      /* cylinder axis defined to be along x-axis in LineIntersectsCyliner */ 
      RotVector     (RotMatrixTmp, NeutronPosition_tmp2);
      RotVector     (RotMatrixTmp, NeutronDirection_tmp);

      if(LineIntersectsCylinder(NeutronPosition_tmp2, NeutronDirection_tmp, &(DetectorTubeCirc.SG.Cyl), tISP) ){
	for(i=0; i<3; i++){
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
    if(Detector.Theta_n!=0){
      RotBackVector(RotSurface, iISP[0]);
      RotBackVector(RotSurface, iISP[1]);
    }
    iISP[0][0] += Detector.Distance;
    iISP[1][0] += Detector.Distance;

    return TRUE;    
  }
 
  return FALSE;
}




void CubeDetSpot(VectorType iSP, VectorType DetSpot)
{
  int l=0; //layer
  VectorType iSP_copy={0};

  CopyVector(iSP,iSP_copy);

  /* Rotate coord. system around detector center such that coord. axes are parallel to cube axes */
  iSP_copy[0] -= Detector.Distance;
  if(Detector.Theta_n!=0){
    RotVector(RotSurface, iSP_copy);
  }

  DetSpot[0]=(floor((iSP_copy[0]+Detector.Thickness/2)/Detector.PixelWidth[0])+0.5)*Detector.PixelWidth[0]-Detector.Thickness/2;
  DetSpot[1]=(floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.5)*Detector.PixelWidth[1]-Detector.Width/2;
  DetSpot[2]=(floor((iSP_copy[2]+Detector.Height/2)/Detector.PixelWidth[2])+0.5)*Detector.PixelWidth[2]-Detector.Height/2;

  if(Detector.DG.Tube.tubeshift){
    l=floor((iSP_copy[0]+Detector.Thickness/2)/Detector.PixelWidth[0]);
    if(Detector.DG.Tube.vertTubeOrientation){
      DetSpot[1]= (l%2) ? 
	(floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.75)*Detector.PixelWidth[1]-Detector.Width/2 :
	(floor((iSP_copy[1]+Detector.Width/2)/Detector.PixelWidth[1])+0.25)*Detector.PixelWidth[1]-Detector.Width/2;
    }
    else {
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
  SPHeight = nsp[Detector.DG.Cyl.axis];

  SpotR=sqrt( sq(nsp[0])+sq(nsp[1]) ); 
  if(Detector.DG.Cyl.axis==0)	
    SpotR=sqrt( sq(nsp[1])+sq(nsp[2]) ); 
  else if(Detector.DG.Cyl.axis==1)	
    SpotR=sqrt( sq(nsp[0])+sq(nsp[2]) ); 
  SpotR= Detector.DG.Cyl.r + (floor((SpotR-Detector.DG.Cyl.r)*Detector.NLayers/Detector.Thickness)+0.5)*Detector.Thickness/Detector.NLayers;
	
  NormVector(nsp);
  CartesianToSpherical(nsp, &STheta, &SPhi);
	
  CTheta=atan(cos(SPhi)*tan(STheta)); 
  if(Detector.Direction[0]) { CTheta=SPhi; GTheta=Detector.Phi; }
  else if (Detector.Direction[1]) CTheta=atan(sin(SPhi)*tan(STheta));
  
  if(Detector.DG.Cyl.axis!=0 && STheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
  
  archpos= fabs(CTheta-(GTheta+dTheta));  //counted from 0 to radians of archlength
  I=floor(Detector.NColumns*archpos/(2.0*dTheta));
  SpotTheta=GTheta+dTheta -((I+0.5)/Detector.NColumns)*(2.0*dTheta);
  
  switch (Detector.DG.Cyl.axis){
  case 0:
    DetSpot[1]=SpotR*cos(SpotTheta);
    DetSpot[2]=SpotR*sin(SpotTheta);
    break;
  case 1:
    DetSpot[0]=SpotR*cos(SpotTheta);
    DetSpot[2]=SpotR*sin(SpotTheta);
    break;
  case 2: 
    DetSpot[0]=SpotR*cos(SpotTheta);
    DetSpot[1]=SpotR*sin(SpotTheta);
    break;	
  }

  if (Detector.DG.Cyl.phimode == 0) 
    DetSpot[Detector.DG.Cyl.axis]=(floor((SPHeight+Detector.Height/2)/Detector.PixelWidth[2])+0.5)*Detector.PixelWidth[2]-Detector.Height/2;
  else 
    DetSpot[Detector.DG.Cyl.axis]=tan( (floor((PhiDet/2+atan2(SPHeight,Detector.Distance))/constphi)+0.5)*constphi-PhiDet/2 )*Detector.Distance;
  
  RotVector(RotMatrix,DetSpot);
}



void  OwnInit(int argc, char *argv[])
{
  VectorType SurfaceInclination;
  long i;
  long count = 0;
  int type=-1;
  char sBuffer[512];
 
  Detector.Width = -1; Detector.Height = -1; Detector.Thickness = -1;
  Detector.NColumns = -1;  Detector.NRows = -1;  Detector.NLayers = -1;
  Detector.Distance=-1; 
  Detector.Geom=-1; 
  Detector.Absorbertype=-1;
  Detector.GasPressure=-1; Detector.GasTemperature=-1; Detector.SolidAtomDensity=-1; Detector.SolidAbsorberthickness=-1;
  Detector.DG.Cyl.axis=-1; 

  /* some default values */
  GenNeutrons=10;
  Detector.usage=0;
 
  for(i=0; i<3; i++){
    Detector.PixelWidth[i]=0;  
    Detector.Direction[i]=0;
    Detector.Position[i]=0;
    Detector.Resolution[i]=0;
  }

  Detector.Theta=0; Detector.Phi=0;        
  Detector.Phi_n=0; Detector.Theta_n=0;    

 Detector.EfficiencyMod=1;
     
  Detector.array=0;         
  Detector.detectColor = -1;  Detector.addColor = -1;
  Detector.DG.Tube.vertTubeOrientation=0; Detector.DG.Tube.rectXsec=0;  Detector.DG.Tube.tubeshift=0; Detector.DG.Tube.wallThickness=0;
  Detector.DG.Cyl.phimode=0;Detector.DG.Cyl.r=0;

  for(i=1; i<argc; i++) {
    if(argv[i][0]!='+') {
      switch(argv[i][1]){ 
	/* general */
      case 'B':
	Detector.array=atoi(&argv[i][2]); 
        break;

      case 'a':
	type=atoi(&argv[i][2]); //0=tubes, 1=area/volume
        break;
	
      case 'G':
	Detector.Geom=atof(&argv[i][2]);
        break;

      case 'E':    /* efficiency file */
	Eff.pfile = tryOpen( (Eff.filename = &argv[i][2]), "efficiency file");
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

      case 'T':
	/* Theta is the angle between the +x-axis and the vector*/
	Detector.Theta=atof(&argv[i][2])*M_PI/180;
	break;

      case 'P':
	/* Phi is the angle of the +y-axis and the projection of the vector to the yz-plane */
	Detector.Phi=atof(&argv[i][2])*M_PI/180;
	break;

      case 'z':
	/* Delta Phi */
	Detector.DG.Cyl.phimode=atof(&argv[i][2]);
	break;

      case 'x':
	/* cylinder axis orientation */
	Detector.DG.Cyl.axis=atof(&argv[i][2]);
	break;
	
      case 'D':
	Detector.Distance=atof(&argv[i][2]);
	break;

      case 'c':
	Detector.NColumns=atoi(&argv[i][2]);
	break;

      case 'r':
	Detector.NRows=atoi(&argv[i][2]);
	break;
      
      case 'n':
	Detector.NLayers=atoi(&argv[i][2]);
	break;

      case 'A':
	GenNeutrons=atoi(&argv[i][2]);
	break;

      case 'U':
	Detector.usage=atoi(&argv[i][2]);
	break;
	
      case 'C':
	Detector.detectColor=atoi(&argv[i][2]);
	break;
      
      case 'S':
	Detector.addColor=atoi(&argv[i][2]);
	break;

      case 'm':
	Detector.Absorbertype=atoi(&argv[i][2]);//0=Bf3, 1=He3, 5=other(from file or const.)
	break;

      case 'e':
	Detector.EfficiencyMod=atof(&argv[i][2]);
	break;

      case 'k':
	Detector.GasTemperature=atof(&argv[i][2]);
	break;

      case 'p':
	Detector.GasPressure=atof(&argv[i][2]);
	break;

	/* Theta_n, phi_n describe the normal vector of the detector surface 
	   w.r.t. the rotated system with x-axis on distance vector*/
      case 'V':
	Detector.Phi_n=atof(&argv[i][2])*M_PI/180;
	break;

      case 'W':
	Detector.Theta_n=atof(&argv[i][2])*M_PI/180;
	break;

	/* tube detector */

      case 'o':
	Detector.DG.Tube.vertTubeOrientation=atoi(&argv[i][2]);
	break;

      case 'b':
	Detector.DG.Tube.rectXsec=atoi(&argv[i][2]);
	break;

      case 'f':
	Detector.DG.Tube.wallThickness=atof(&argv[i][2])/10;//in cm
	break;

      case 's':
	Detector.DG.Tube.tubeshift=atoi(&argv[i][2]);
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

	/* output file */
      case 'O':
	DetectorOutputFileName=(&argv[i][2]);
	if(!Detector.array){
	  outFile=fopen(FullParName(DetectorOutputFileName),"w+");
	  fprintf(outFile,"#Trajectories detector_eventmode \n");
	  fprintf(outFile,"#    pos_x [cm]   pos_y [cm]   pos_z [cm]  time [ms]    weight     color \n");
	  fprintf(outFile,"#-------------------------------------------------------------------------");
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
  CheckAndAdjustDetectorInput(type);

  SurfaceInclination[0]=cos(Detector.Theta_n);
  SurfaceInclination[1]=sin(Detector.Theta_n)*cos(Detector.Phi_n);
  SurfaceInclination[2]=sin(Detector.Theta_n)*sin(Detector.Phi_n);
  RotMatrixX(SurfaceInclination,RotSurface);
 
  if(Detector.Geom==1){	/* cylinder */

    if(Detector.DG.Cyl.axis==0 && fabs(Detector.Theta-M_PI/2)>0.0001){
      fprintf(LogFilePtr,"\n WARNING: Theta is set to 90° for cylinder in x direction!");
      Detector.Theta=M_PI/2;
    }
 
    Detector.DG.Cyl.r=Detector.Distance;	 	 
    Detector.Direction[Detector.DG.Cyl.axis]=1.0;

    if (cos(Detector.Phi) < 0.0) Detector.Theta=-Detector.Theta;

    NeutronIntersectsDetector=NeutronIntersectsCylDetector;
    DetectorSpot=CylinderDetSpot;

    bVisInstalled = TRUE; 
    // Geometry data
    if (bVisInstr)
      { 
	double ry, rz;
	stGeometry.pCylSlice = (VtCylSlice*) calloc(1, sizeof(VtCylSlice));
	stGeometry.nCylSlices = 1; 
	
	stGeometry.pCylSlice[0].Radius = Detector.Distance; 
	stGeometry.pCylSlice[0].Width  = Detector.Width;
	stGeometry.pCylSlice[0].Height = Detector.Height;
	stGeometry.pCylSlice[0].vCntr[0]  = 0.;
	stGeometry.pCylSlice[0].vCntr[1]  = 0.;
	stGeometry.pCylSlice[0].vCntr[2]  = 0.;
	stGeometry.pCylSlice[0].vSymAxis[0]= 0;
	stGeometry.pCylSlice[0].vSymAxis[1]= 0;
	stGeometry.pCylSlice[0].vSymAxis[2]= 0;
	stGeometry.pCylSlice[0].vSymAxis[Detector.DG.Cyl.axis]= 1;
	stGeometry.pCylSlice[0].Phi = Detector.Theta/M_PI*180.;
	//coordinates differently defined in visualization:
	if(Detector.DG.Cyl.axis==0)
	  stGeometry.pCylSlice[0].Phi = Detector.Phi/M_PI*180.+90.;
	else if(Detector.DG.Cyl.axis==1)
	  stGeometry.pCylSlice[0].Phi = -Detector.Theta/M_PI*180.;
	
	stGeometry.pCylSlice[0].OpenAngle = Detector.Width/(2.*M_PI*Detector.Distance)*360.;
	RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
	stGeometry.pCylSlice[0].Phi += rz/M_PI*180.;
	stGeometry.pDescr  = "detector:cyan";
	stGeometry.eModule = VT_DETECTOR;
      }
  } 
  else { /* cube */
    Detector.Direction[0]=cos(Detector.Theta);
    Detector.Direction[1]=sin(Detector.Theta)*cos(Detector.Phi);
    Detector.Direction[2]=sin(Detector.Theta)*sin(Detector.Phi);
    for(i=0; i<3;i++)
      if(fabs(Detector.Direction[i])<1e-5) Detector.Direction[i]=0.0;
    
    Detector.Position[0] = Detector.Distance;
    NeutronIntersectsDetector=NeutronIntersectsCubeDetector;
    DetectorSpot=CubeDetSpot;
    
    bVisInstalled = TRUE;
    // Geometry data
    if (bVisInstr)
      { 
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
	stGeometry.pCuboid[0].vNormal[2]=  sin(Detector.Theta)*sin(Detector.Phi);
	if(Detector.Theta_n!=0){
	  RotBackVector(RotSurface,stGeometry.pCuboid[0].vNormal);
	}
	      
	stGeometry.pDescr  = "detector:cyan";
	stGeometry.eModule = VT_DETECTOR;
      }
  }
    
  if (Eff.pfile!=NULL) {
      Eff.maxdata = LinesInFile(Eff.pfile);
      Eff.data = calloc(Eff.maxdata+1, sizeof(EffData));
      for(count=0; count < Eff.maxdata; count++) {
        ReadLine(Eff.pfile, sBuffer, sizeof(sBuffer)-1);
        sscanf(sBuffer, "%lf %lf", &Eff.data[count].Lambda, &Eff.data[count].Eff);
      }
      fclose(Eff.pfile);
    }

}

static FILE * tryOpen(const char *fn, const char *s) {
  FILE *f;
  char *fulln = FullParName(fn);
  f = fopen(fulln, "r");
  if (f) 
    return f;
  myExit2("ERROR: File %s containing %s could not be opened\n", fulln, s);
  return 0;
}

void OwnCleanup()
{
  /* print error that might have occured many times */
  PrintMessage(DET_TRAJ_INSIDE, "", ON);
  PrintMessage(DET_L_RANGE_TOO_SMALL,Eff.filename, ON);

  if(lost>0)
    fprintf(LogFilePtr,"\nWARNING: %ld neutrons lost between tubes",lost);
 
  fprintf(LogFilePtr," \n");

  /* set description for instrument plot */
  stPicture.dWPar  = Detector.Width;
  stPicture.dHPar  = Detector.Width/Detector.NColumns;
  stPicture.dRPar  = Detector.Distance;
  stPicture.eType  = (short) Detector.Geom;
} 

double GetXsec(int h_absorbertype, double h_lambda){
  double h_sigma=0;
  switch (h_absorbertype){
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


void CheckAndAdjustDetectorInput(int type){
  if(Detector.Geom<0)
    myExit("\nERROR: You have to specify a detector geometry (flat vs cylindrical) with -G.");
  if(type<0){
    myExit("\nERROR: You have to specify a detector type (area/volume vs. tube) with -a.");}
  else if(type==0 && Detector.Geom==1){
    myExit("\nERROR: tube detector only possible for flat geometry. Use detector array to build cylindrical shape out of flat (sub)detectors.\n");}
  else if(type==0){
    Detector.Geom=2;}
  else if(type==1 && Detector.Geom==2){
    Detector.Geom=0;}

  if(Detector.Phi<0)
    myExit("\nERROR: Angle phi hast to be between 0 and 360 deg (specify with -P).");
  if(Detector.Theta<0 || Detector.Theta>180)
    myExit("\nERROR: Angle theta has to be between 0 and 180 deg (specify with -T).");
  if(Detector.Distance<0)
    myExit("\nERROR: You have to specify a distance with -D.");
  if(Detector.Geom!=1 && Detector.Distance<Detector.Thickness/2)
    myExit("\nERROR: Detector distance smaller than half the detector thickness!");
  if(Detector.Height<0)
    myExit("\nERROR: You have to specify a height with -h.");
  if(Detector.Width<0)
    myExit("\nERROR: You have to specify a width with -w.");
  if(Detector.Thickness<0)
    myExit("\nERROR: You have to specify a thickness with -t.");
  if(Detector.NRows<0)
    myExit("\nERROR: You have to specify the number of rows with -r.");
  if(Detector.NColumns<0)
    myExit("\nERROR: You have to specify the number of rows with -c.");
  if(Detector.NLayers<0)
    myExit("\nERROR: You have to specify the number of rows with -n.");
  Detector.PixelWidth[0]=Detector.Thickness/Detector.NLayers;
  Detector.PixelWidth[1]=Detector.Width/Detector.NColumns;
  Detector.PixelWidth[2]=Detector.Height/Detector.NRows;
  if(Detector.Absorbertype<0){
    myExit("\nERROR: You have to specify an absorber/converter type with -m.");}
  else if(Detector.Absorbertype<2 && (Detector.GasPressure<0 || Detector.GasTemperature<0)){
    myExit("\nERROR: For a gaseous detector you have to specify gas pressure (-p) and temperature (-k).");}
  else if(Detector.Absorbertype>1 && Detector.Absorbertype<4){
    Detector.SolidAbsorberthickness=Detector.GasPressure;
    Detector.SolidAtomDensity=Detector.GasTemperature;
    if (Detector.SolidAtomDensity<0 || Detector.SolidAbsorberthickness<0)
      myExit("\nERROR: For a solid detection material you have to specify the solid layer thickness (-p) and atom density (-k).");
  }
  if(Detector.Absorbertype==4 && Detector.EfficiencyMod>=1)
    myExit("\nERROR: Efficiency modifyer is used as total efficiency (with absorber type \"other\") and has to be <1.");

  if(Detector.Geom==2){ 
    if(Detector.Resolution[0]!=0)
      fprintf(LogFilePtr,"\nWARNING: x-resolution not 0 but has no meaning in tube detector (hence will not be used).");
    if(Detector.DG.Tube.vertTubeOrientation){
      if(Detector.PixelWidth[0]!=Detector.PixelWidth[1])
	myExit("\nERROR: tube detector requires Thickness/number_of_layers=Width/number_of_columns (diameter of tube) for vertical tube orientation \n");
      if(Detector.Resolution[1]!=0)
	fprintf(LogFilePtr,"\nWARNING: hor. resolution not 0 but has no meaning in vertical tubes (hence will not be used).");
    }
    else if(!Detector.DG.Tube.vertTubeOrientation){
      if(Detector.PixelWidth[0]!=Detector.PixelWidth[2])
	myExit("\nERROR: tube detector requires Thickness/number_of_layers=Height/number_of_rows (diameter of tube) for horizontal tube orientation \n");
      if(Detector.Resolution[2]!=0)
	fprintf(LogFilePtr,"\nWARNING: vert. resolution not 0 but has no meaning in horizontal tubes (hence will not be used).");
    }
    /*add tubes if shifted so whole area is covered */
    /*only if resolution=0 to avoid adding tubes twice*/
    if(Detector.DG.Tube.tubeshift){
      if(Detector.DG.Tube.vertTubeOrientation && Detector.Resolution[1]==0){
	Detector.NColumns += 2;
	Detector.Width += 2*Detector.PixelWidth[1];
      }
      else if (!Detector.DG.Tube.vertTubeOrientation && Detector.Resolution[2]==0){
	Detector.NRows += 2;
	Detector.Height += 2*Detector.PixelWidth[2];
      }
    }
  }
  else if(Detector.Geom==1){ 
    if(Detector.DG.Cyl.axis<0){
      fprintf(LogFilePtr,"\nWARNING: Cylinder geometry chosen but no cylinder axis given (specify with -x). Cylinder axis is set to be z axis per default.");
      Detector.DG.Cyl.axis=2;
    }
    if(Detector.Theta_n!=0 || Detector.Phi_n!=0)
      myExit("\nERROR: Surface inclination only possible for flat detector! For cylindrical geometry, theta_n and phi_n have to be 0 deg.");
  }

  //extend detector to avoid intensity drop at borders
  // only in y,z: extension in x leads to higher efficiency!
  if(Detector.Resolution[2]!=0){
     Detector.Height+=2*(floor(Detector.Resolution[2]/ Detector.PixelWidth[2])+1)* Detector.PixelWidth[2];
     Detector.NRows+=2*(floor(Detector.Resolution[2]/ Detector.PixelWidth[2])+1);
  }
  if(Detector.Resolution[1]!=0){
     Detector.Width+=2*(floor(Detector.Resolution[1]/ Detector.PixelWidth[1])+1)* Detector.PixelWidth[1];
     Detector.NColumns+=2*(floor(Detector.Resolution[1]/ Detector.PixelWidth[1])+1);
  }
}
