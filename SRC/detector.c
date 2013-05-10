#/****************************************************************************************/
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
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "init.h"
#include "matrix.h"
#include "softabort.h"
#include "intersection.h"
#include "message.h"


typedef struct
{
  double Lambda, Eff;
}
  EffData;


typedef struct
{
  FILE    *pfile;
  char    *filename;
  EffData *data;
  long    maxdata;
}
  EffFile;

/* global variables */
SampleType Detector;
long       GenNeutrons=10;  // repetition: multiply neutrons to get diff. interaction lengths (probability from integration region L/GenNeutrons)
int        geom=0,          // detector geometry: 1: cylinder   2: flat
           cyl_axis=2,      // axis of cyl. detector
           NColumns=1,      // number of rows,
           NRows=1,         //           columns, 
           NLayers=1,       //           layers partitioning the detector volume
           usage,           // 0: normal, 1: monitor only, 2: grid off
           lost=0;          // give Warning if neutron intersetcs tube detector but tube in which interaction happens is not found
short      phimode=0,             // 0 = const. pixel size; 1 = const Delta phi 
           array=0,               // first or intermediate part of detector array 
           detectColor = -1,      // only detect neutrons of certain color; -1 = all neutrons
           addColor = -1,         // tag detected neutrons by adding addColor to color
           type=1,                // tube (0) or area/Volume (1) detector
           absorbertype=0,        // Boron10 (0,2), He3 (1), Li (3) or other (5)
           vertTubeOrientation=0, // vertical (1) or horizontal (0) tube axis
           rectXsec=0,            // circular (0) or rectangular (1) tube cross-section
           Tubeshift=0;           // tube layers shifted against each other

double     RotMatrix[3][3],
           PixelWidth[3]={0};                                // pixel size, e.g. tube diameter
double     Width=0, Height=0, Thickness=0,                   // detector extension
           Theta=0, Phi=0, Distance=0,                       // detector coordinates (center of surface) 
           wallThickness=0,                                  // only tube det: thickness of tube walls
           pressure=4, temperature=293,                      // only gas det: gas pressure and temperature
           atomicdensity=0, absorberthickness=0,             // only solid det: density (N) and thickness of converter layer
           EfficiencyMod=1,                                  // modify efficiency
           horResolution=0, vertResolution=0, xResolution=0; // resolution in y,z,x as standard deviation (Gauss)

EffFile Eff = {0};

/* function pointers */
long (*NeutronIntersectsDetector)(Neutron *Nin, SampleType *Detector, VectorType ISP[]);
void (*DetectorSpot)(VectorType SP, VectorType DetSpot, SampleType *Detector);

/* prototypes of local functions */
long NeutronIntersectsCubeDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[]);
long NeutronIntersectsCylDetector (Neutron *Nin, SampleType *Detector, VectorType ISP[]);
long NeutronIntersectsTube(VectorType direction, VectorType ISP0, int l, VectorType iISP[]);

void CubeDetSpot    (VectorType SP, VectorType DetSpot, SampleType *Detector);
void CylinderDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector);

void OwnInit   (int argc, char *argv[]);
void OwnCleanup();

double GetXsec(int h_absorbertype, double h_lambda);
double GetLambdaProbFromEff(const double lambda, const TotalID NeutronID);
static FILE * tryOpen(const char *fn, const char *s);

FILE *outFile;
char *DetectorOutputFileName;

int main(int argc, char *argv[])
{
  Neutron WorkNeutron,           // working copy
    OutNeutron,                  // working copy for WriteNeutron
    DrawNeutron;                 // working copy for visualization (draws scattering point instead of DetSpot)
  VectorType ISP[2],iISP[2];     // intersection points with the detector and intermediate intersection points with tubes in tube detector 
  VectorType DetSpot,            // final detection position given out by detector
    DetSignal,                   // intermediate detection position (including gaussian smear from resolution)
    SP,                          // interaction position
    vShift;                      // for visualization: vector in neutron direction with length equal to distance of (last) detector (part in array) center
  long   i,j,l;                  // loop counting
  long   NeutCount=10,           // count neutron copies for repetition (=GenNeutrons) > 1
         NumDetected=0;          // number of detected neutrons
  short FoundTube=0;             // tag for tube in which interaction happens
  double TimeTillScattering=0,   // neutron flight time until interaction
    FullLengthInDetector=0,      // length of neutron trajectorie in overall detector volume
    LengthInAbsorberMaterial=0,  // length of neutron trajectory in active detector volume
    LengthTillScattering=0,      // length of neutron trajectory in active detector volume until interaction
    ScatteringProb=0,            // probability of interaction
    N=0,                         // particle density
    sigma=0,                     // absorption cross-section
    NSigma=0;                    // for const. efficieny
  const double kB=1.3806504E-23; // boltzman constant in [JK-1]



  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_DETECTOR);
  print_module_name("detector 1.9");

  /* module specific initialization */
  OwnInit(argc, argv);

  /* Rotmatrix will rotate a Vector to a frame in which the middle of the */
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
      if ( (detectColor > -1) && (InputNeutrons[i].Color != detectColor) )  
	continue;

      //pass on neutrons detected by previous detector parts,
      //last detector in array removes tag and writes output file
      if( floor(InputNeutrons[i].Color/10000)==1 ){
	if(!array){
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
      N=(absorbertype<2)?(pressure*1e5/(temperature*kB)):(atomicdensity*1E27);

      if(NeutronIntersectsDetector(&(WorkNeutron),&Detector,ISP)) 
      {

        /* determine the length of the path through the whole detector volume */
        FullLengthInDetector = DistVector(ISP[0], ISP[1]);

	/* total cross-section (in m^2) */
	sigma=GetXsec(absorbertype,WorkNeutron.Wavelength);

	if (usage==1)
	  GenNeutrons=1;
	//------------------------------------------------------

        for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++){

	  /* determine the interaction point in the detector volume and the detector signal point*/
	  LengthTillScattering = MonteCarlo(0,FullLengthInDetector); 
	  for(j=0; j<3; j++)
	    SP[j]= ISP[0][j] +LengthTillScattering*WorkNeutron.Vector[j];

	  /*correct for dead zones in tube geometry */
	  if(type==0){ 
	    LengthTillScattering=0;
	    FullLengthInDetector=0;
	    FoundTube=0;
	    for (l=1; l<=NLayers; l++){
	      if( NeutronIntersectsTube(WorkNeutron.Vector,ISP[0],l,iISP) ){
		FullLengthInDetector+=DistVector(iISP[0],iISP[1]);
	      }
	    }		
	    LengthTillScattering=MonteCarlo(0,FullLengthInDetector);
	    
	    // find interaction point 
	    LengthInAbsorberMaterial=0;
	    for (l=1; l<=NLayers; l++){
	      if( NeutronIntersectsTube(WorkNeutron.Vector,ISP[0],l,iISP) ){
		LengthInAbsorberMaterial+=DistVector(iISP[0],iISP[1]);
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
	  if(horResolution>0 && (type==1 || !vertTubeOrientation) )
	    DetSignal[1]=DistrGauss(DetSignal[1],horResolution); 
	  if(vertResolution>0 && (type==1 || vertTubeOrientation) )
	    DetSignal[2]=DistrGauss(DetSignal[2],vertResolution);
	  if(xResolution>0 && type==1 )
	    DetSignal[0]=DistrGauss(DetSignal[0],xResolution);
	  

	  /* interaction probability */
	  if (Eff.maxdata > 0) { //efficiency from file
	    ScatteringProb = GetLambdaProbFromEff(WorkNeutron.Wavelength,WorkNeutron.ID);
	  } 
	  else {
	    switch(absorbertype){
	    case 0:  //BF3
	    case 1:  //He3
	      ScatteringProb = N*sigma*exp(-N*sigma*LengthTillScattering/100) * FullLengthInDetector/100 * EfficiencyMod;
	      break;
	      //solid layer approximation: scale length in material with layer_thickness/total_thickness
	    case 2:  //solid B10
	    case 3:  // Li6
	      LengthTillScattering*=absorberthickness/Thickness;
	      FullLengthInDetector*=absorberthickness/Thickness;
	      ScatteringProb = N*sigma*exp(-N*sigma*LengthTillScattering/100) * FullLengthInDetector/100* EfficiencyMod;
	      break; 
	    case 5:  // other; use wavelength-independent input eff
	    default:
	      NSigma=-log(1-EfficiencyMod)/Thickness;
	      ScatteringProb = NSigma*exp(-NSigma*LengthTillScattering) * FullLengthInDetector;
	      break;
	    }
	  }

	  if(type==0 && !FoundTube){
	    ScatteringProb=0;
	    lost++;
	  }

	  DetectorSpot(DetSignal, DetSpot, &Detector);  

          TimeTillScattering=DistVector(SP,WorkNeutron.Position)/
          V_FROM_LAMBDA(WorkNeutron.Wavelength);

          /* everythings done, so rot back the vectors and put all together 
          and set output data of the neutron */
          OutNeutron             = WorkNeutron;
          OutNeutron.Time        = WorkNeutron.Time + TimeTillScattering;
          OutNeutron.Probability = WorkNeutron.Probability * ScatteringProb / GenNeutrons;

	  // tag detected neutrons for detector array 
	  if(array)
	    OutNeutron.Color+=10000;
 
	  RotBackVector(RotMatrix,SP);
	  RotBackVector(RotMatrix,DetSignal);
	  RotBackVector(RotMatrix,DetSpot);	
	      		      
          if (usage==1){      //monitor only
	    OutNeutron.Probability = WorkNeutron.Probability;
	    CopyVector(SP, OutNeutron.Position);
	  }
	  else if (usage==2){ // grid off
	    CopyVector(DetSignal, OutNeutron.Position);
            NormVector(DetSignal);
            CopyVector(DetSignal, OutNeutron.Vector);
	  }
          else {              // normal
            CopyVector(DetSpot, OutNeutron.Position);
            NormVector(DetSpot);
            CopyVector(DetSpot, OutNeutron.Vector);
          }

	  // write out neutrons that shall be detected	
          if ( OutNeutron.Probability>wei_min )  {
	    if (addColor > 0) 
	      OutNeutron.Color += addColor;
	    WriteNeutron(&OutNeutron);	
	    NumDetected++;
	    if(!array && DetectorOutputFileName)
	      fprintf(outFile,"\n   %10.4f  %10.4f  %10.4f   %10.4f     %2.3e     %d",OutNeutron.Position[0],OutNeutron.Position[1],OutNeutron.Position[2],OutNeutron.Time,OutNeutron.Probability,OutNeutron.Color);
	  }

         // write interaction point - only once per incoming trajectory
          if (NeutCount < 1){
	    DrawNeutron=OutNeutron;
	    CopyVector(SP, DrawNeutron.Position);
	    if(!array)
	      DrawNeutron.Color+=10000;
            WriteIAP(&DrawNeutron, VT_DETECTED);
	  }

        } /* loop count */
      } 
      else /* if neutron does not intersect detector */ 
      {
	
	if(array)
	  WriteNeutron(&InputNeutrons[i]);
	else { //draw trajectories of undetected neutrons up to detector distance
	  DrawNeutron=WorkNeutron;
	  RotBackVector(RotMatrix, DrawNeutron.Vector);
	  RotBackVector(RotMatrix, DrawNeutron.Position);
	  CopyVector(DrawNeutron.Vector, vShift);
	  MultiplyByScalar(vShift, (Distance+Thickness/2));
	  AddVector(DrawNeutron.Position,vShift);
	  WriteIAP(&DrawNeutron, VT_OUTSIDE);
	}
	
      }
    } //for(i=0; i<NumNeutGot; i++)
  } //while((ReadNeutrons())!= 0)


 fprintf(LogFilePtr,"\n Neutrons detected in this detector: %ld \n",NumDetected);

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
  } else {
    for (i = 1; i<=Eff.maxdata; i++) {
      if (lambda < Eff.data[i].Lambda)
        return Eff.data[i].Eff + (Eff.data[i].Eff-Eff.data[i-1].Eff)/(Eff.data[i].Lambda-Eff.data[i-1].Lambda) * (lambda-Eff.data[i].Lambda);
    }
    if (i>Eff.maxdata) {
      CountMessageID(DET_L_RANGE_TOO_SMALL, NeutronID);
      return Eff.data[Eff.maxdata].Eff;
    }
  }
  return -1.;
}

long NeutronIntersectsCubeDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[])
{
	VectorType Position;
	double t[2];
	long i;

	/* move the origin of the coordinate system to the middle of the detector*/
	CopyVector(Nin->Position,Position);
	Position[0] -= (Detector->Position[0] + Detector->SG.Cube.thickness/2.0);

	if(LineIntersectsCube(Position, Nin->Vector, &(Detector->SG.Cube), t)) 
	{
		for(i=0; i<3; i++)
		{	ISP[1][i] = Position[i]+t[1]*Nin->Vector[i];
			ISP[0][i] = Position[i]+t[0]*Nin->Vector[i];
		}

		/* transform the ISPs to the old coordinate system*/
		ISP[0][0] += (Detector->Position[0]+Detector->SG.Cube.thickness/2.0);
		ISP[1][0] += (Detector->Position[0]+Detector->SG.Cube.thickness/2.0);
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



long NeutronIntersectsCylDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[])
{
  VectorType InISP[2], OutISP[2], Position, Direction, cut;
  double Int[2], Outt[2], ntheta, nphi, CTheta, GTheta;
  long   i;
  double dTheta=Width/(2.0*Distance);

  CopyVector(Nin->Position, Position);
  CopyVector(Nin->Vector, Direction);
 
  if(LineIntersectsCylinder(Position, Direction, &(Detector->SG.Cyl), Int))  {
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
  if(Detector->Direction[0]) CTheta=nphi;
  else if (Detector->Direction[1]) CTheta=atan(sin(nphi)*tan(ntheta));

  if(cyl_axis!=0 && ntheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
	
  GTheta= Detector->Direction[0] ? Phi : Theta;
	
  if( !(fabs(CTheta-GTheta) < dTheta || fabs(CTheta+2*M_PI-GTheta) < dTheta) )
    return FALSE; 

  Detector->SG.Cyl.r += Thickness;
  if(LineIntersectsCylinder(Position, Direction, &(Detector->SG.Cyl), Outt)){
    for(i=0; i<3; i++){
      OutISP[1][i] = Position[i]+Outt[1]*Direction[i];
      OutISP[0][i] = Position[i]+Outt[0]*Direction[i];
    }
  }
  else
    return FALSE;
			
  Detector->SG.Cyl.r-=Thickness;
  if( (fabs(InISP[1][0]) < Detector->SG.Cyl.height*0.99999/2.0) && (fabs(OutISP[1][0]) <Detector->SG.Cyl.height*0.99999/2.0) ){
    CopyVector(InISP[1],ISP[0]);
    CopyVector(OutISP[1],ISP[1]);
    return TRUE;
  }
  return FALSE;
}


long NeutronIntersectsTube(VectorType direction, VectorType Position, int l, VectorType iISP[]){
  SampleType DetectorTube={0};
  VectorType temp={0},temp2={0},dir={0};
  double tISP[2]={0};
  int i=0,j=0,k=0;
  double RotMatrixTemp[3][3]={{0}};
 
  if(rectXsec){ 
    DetectorTube.SG.Cube.thickness=PixelWidth[0]-2*wallThickness;
    DetectorTube.SG.Cube.height = vertTubeOrientation ? Height : (PixelWidth[2]-2*wallThickness) ;
    DetectorTube.SG.Cube.width = vertTubeOrientation ? (PixelWidth[1]-2*wallThickness) : Width ;
  }
  else {
    DetectorTube.SG.Cyl.r=PixelWidth[0]/2-wallThickness;
    DetectorTube.SG.Cyl.height = vertTubeOrientation ? Height : Width;
  }

  /* search right tube in layer */
  CopyVector(Position,temp);
  CopyVector(direction,dir);
  
  temp[0]+=(double)(l-0.5)*PixelWidth[0]-Distance-Thickness/2;
  temp[1]+=(double)(l-0.5)*PixelWidth[0]*( dir[1]/dir[0] );
  temp[2]+=(double)(l-0.5)*PixelWidth[0]*( dir[2]/dir[0] );
  
  j=(int)(floor(fabs(-Width/2-temp[1])/PixelWidth[1])+1);
  k=(int)(floor(fabs(-Height/2-temp[2])/PixelWidth[2])+1);

  if(vertTubeOrientation){
    FillRotMatrixY(RotMatrixTemp,M_PI_2);
    k=NRows/2+1;
    if(Tubeshift){
      if(l%2==0)
	j=(int)(floor(fabs(-Width/2+0.25*PixelWidth[1]-temp[1])/PixelWidth[1])+1);
      else
	j=(int)(floor(fabs(-Width/2-0.25*PixelWidth[1]-temp[1])/PixelWidth[1])+1);
    }
  }
  else {
    FillRotMatrixZ(RotMatrixTemp,M_PI_2);
    j=NColumns/2+1;
    if(Tubeshift){
      if(l%2==0)
	k=(int)(floor(fabs(-Height/2+0.25*PixelWidth[2]-temp[2])/PixelWidth[2])+1);
      else
	k=(int)(floor(fabs(-Height/2-0.25*PixelWidth[2]-temp[2])/PixelWidth[2])+1);
    }
  }

  DetectorTube.Position[0]=-Thickness/2+(l-0.5)*PixelWidth[0];
  DetectorTube.Position[1]= vertTubeOrientation ? -Width/2+(j-0.5)*PixelWidth[1] : 0;
  DetectorTube.Position[2]= vertTubeOrientation ? 0 : -Height/2+(k-0.5)*PixelWidth[2] ;
 
  if (Tubeshift){
    if(l%2==0){
      DetectorTube.Position[1]= vertTubeOrientation ? -Width/2+(j-0.5+0.25)*PixelWidth[1] : 0;
      DetectorTube.Position[2]= vertTubeOrientation ? 0 : -Height/2+(k-0.5+0.25)*PixelWidth[2] ;
    }
    else {
      DetectorTube.Position[1]= vertTubeOrientation ? -Width/2+(j-0.5-0.25)*PixelWidth[1] : 0;
      DetectorTube.Position[2]= vertTubeOrientation ? 0 : -Height/2+(k-0.5-0.25)*PixelWidth[2] ;
    }
  }
  
   /* intersect neutron with that tube */
  CopyVector(Position,temp2);
  temp2[0]-=(Distance+Thickness/2);
  SubVector (temp2, DetectorTube.Position);
  /*cylinder axis defined to be along x-axis in LineIntersectsCyliner */  
  if(!rectXsec){  
    RotVector     (RotMatrixTemp, temp2);
    RotVector     (RotMatrixTemp, dir);
  }
  
  if( (rectXsec && LineIntersectsCube(temp2, dir, &(DetectorTube.SG.Cube), tISP)) || (!rectXsec && LineIntersectsCylinder(temp2, dir, &(DetectorTube.SG.Cyl), tISP)) ){
    for(i=0; i<3; i++){
      iISP[1][i] = temp2[i]+tISP[1]*dir[i];
      iISP[0][i] = temp2[i]+tISP[0]*dir[i];
    }
  
    /* transform the ISPs to the old coordinate system*/
    if(!rectXsec){
      RotBackVector (RotMatrixTemp, iISP[0]);
      RotBackVector (RotMatrixTemp, iISP[1]);
    }
    AddVector (iISP[0], DetectorTube.Position);
    AddVector (iISP[1], DetectorTube.Position);
    iISP[0][0]+=(Distance+Thickness/2);
    iISP[1][0]+=(Distance+Thickness/2);
     
    return TRUE;
  }
   
  return FALSE;
}




void CubeDetSpot(VectorType iSP, VectorType DetSpot, SampleType *Detector)
{
  DetSpot[0]=(floor((iSP[0]+Thickness/2)/PixelWidth[0])+0.5)*PixelWidth[0]-Thickness/2;
  DetSpot[1]=(floor((iSP[1]+Width/2)/PixelWidth[1])+0.5)*PixelWidth[1]-Width/2;
  DetSpot[2]=(floor((iSP[2]+Height/2)/PixelWidth[2])+0.5)*PixelWidth[2]-Height/2;
	
}



void CylinderDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector)
{
  double     STheta, SPhi, CTheta, SpotTheta, SpotR,
    archpos, I, SPHeight;
  VectorType nsp;
  double dTheta=Width/(2.0*Distance);
  double GTheta=Theta;
  double constphi = atan(Height/2./Distance)*2./NRows;
  double PhiDet=constphi*NRows;

  CopyVector(SP,nsp);
  RotBackVector(RotMatrix,nsp);
  SPHeight = nsp[cyl_axis];

  SpotR=sqrt( sq(nsp[0])+sq(nsp[1]) ); 
  if(cyl_axis==0)	
    SpotR=sqrt( sq(nsp[1])+sq(nsp[2]) ); 
  else if(cyl_axis==1)	
    SpotR=sqrt( sq(nsp[0])+sq(nsp[2]) ); 
  SpotR= Detector->SG.Cyl.r + (floor((SpotR-Detector->SG.Cyl.r)*NLayers/Thickness)+0.5)*Thickness/NLayers;
	
  NormVector(nsp);
  CartesianToSpherical(nsp, &STheta, &SPhi);
	
  CTheta=atan(cos(SPhi)*tan(STheta)); 
  if(Detector->Direction[0]) { CTheta=SPhi; GTheta=Phi; }
  else if (Detector->Direction[1]) CTheta=atan(sin(SPhi)*tan(STheta));
  
  if(cyl_axis!=0 && STheta>M_PI_2)
    CTheta= (CTheta<=0) ? (M_PI + CTheta) : (-M_PI + CTheta);
  
  archpos= fabs(CTheta-(GTheta+dTheta));  //counted from 0 to radians of archlength
  I=floor(NColumns*archpos/(2.0*dTheta));
  SpotTheta=GTheta+dTheta -((I+0.5)/NColumns)*(2.0*dTheta);
  
  switch (cyl_axis){
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

  if (phimode == 0) 
    DetSpot[cyl_axis]=(floor((SPHeight+Height/2)/PixelWidth[2])+0.5)*PixelWidth[2]-Height/2;
  else 
    DetSpot[cyl_axis]=tan( (floor((PhiDet/2+atan2(SPHeight,Distance))/constphi)+0.5)*constphi-PhiDet/2 )*Distance;
  
  RotVector(RotMatrix,DetSpot);
}



void  OwnInit(int argc, char *argv[])
{
  long i;
  char sBuffer[512];
  long count = 0;

  /* some default values */
  GenNeutrons=10;
  NColumns = 1;
  NRows = 1;
  geom=0; 
  
  Detector.Direction[0]=0.0;
  Detector.Direction[1]=0.0;

  for(i=1; i<argc; i++) {
    if(argv[i][0]!='+') {
      switch(argv[i][1]){ 
	/* general */
      case 'B':
	array=atoi(&argv[i][2]); 
        break;

      case 'a':
	type=atoi(&argv[i][2]); //0=gas tubes, 1=area/volume
        break;
	
      case 'G':
	geom=atof(&argv[i][2]);
        break;

      case 'E':    /* efficiency file */
	Eff.pfile = tryOpen( (Eff.filename = &argv[i][2]), "efficiency file");
	break;

      case 'h':
	Height=atof(&argv[i][2]);
	break;

      case 'w':
	Width=atof(&argv[i][2]);
	break;

      case 't':
	Thickness=atof(&argv[i][2]);
	if(Thickness<=0.0) Thickness=1e-4;
	break;

      case 'T':
	/* Theta is the angle between the +x-axis and the vector*/
	Theta=atof(&argv[i][2])*M_PI/180;
	break;

      case 'P':
	/* Phi is the angle of the +y-axis and the projection of the vector to the yz-plane */
	Phi=atof(&argv[i][2])*M_PI/180;
	break;

      case 'z':
	/* Delta Phi */
	phimode=atof(&argv[i][2]);
	break;

      case 'x':
	/* cylinder axis orientation */
	cyl_axis=atof(&argv[i][2]);
	break;
	
      case 'D':
	Distance=atof(&argv[i][2]);
	break;

      case 'c':
	NColumns=atoi(&argv[i][2]);
	break;

      case 'r':
	NRows=atoi(&argv[i][2]);
	break;
      
      case 'n':
	NLayers=atoi(&argv[i][2]);
	break;

      case 'A':
	GenNeutrons=atoi(&argv[i][2]);
	break;

      case 'U':
	usage=atoi(&argv[i][2]);
	break;
	
      case 'C':
	detectColor=atoi(&argv[i][2]);
	break;
      
      case 'S':
	addColor=atoi(&argv[i][2]);
	break;

      case 'm':
	absorbertype=atoi(&argv[i][2]);//0=Bf3, 1=He3, 5=other(from file or const.)
	break;

      case 'e':
	EfficiencyMod=atof(&argv[i][2]);
	break;

      case 'k':
	temperature=atof(&argv[i][2]);
	break;

      case 'p':
	pressure=atof(&argv[i][2]);
	break;

	/* tube detector */

      case 'o':
	vertTubeOrientation=atoi(&argv[i][2]);
	break;

      case 'b':
	rectXsec=atoi(&argv[i][2]);
	break;

      case 'f':
	wallThickness=atof(&argv[i][2])/10;//in cm
	break;

      case 's':
	Tubeshift=atoi(&argv[i][2]);
	break;

	/* resolution: sigma_Gauss=FWHM/2.35482  */

      case 'u':
	horResolution=atof(&argv[i][2])/2.35482;
	break;

      case 'v':
	vertResolution=atof(&argv[i][2])/2.35482;
	break;

      case 'l':
	xResolution=atof(&argv[i][2])/2.35482;
	break;

	/* output file */
      case 'O':
	DetectorOutputFileName=(&argv[i][2]);
	if(!array){
	  outFile=fopen(FullParName(DetectorOutputFileName),"w+");
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

  PixelWidth[0]=Thickness/NLayers;
  PixelWidth[1]=Width/NColumns;
  PixelWidth[2]=Height/NRows;

  // input variable recycling:
  if(absorbertype==2 || absorbertype==3){ //solid layers
    absorberthickness=pressure;
    atomicdensity=temperature;
  }

  // check input parameters
  if(absorbertype==5 && EfficiencyMod==1)
    myExit("ERROR: Efficiency modifyer has to be <1 if used as total efficiency (with absorber type \"other\").");

  if(type==0){ //tube detector
    if(geom==1)
      myExit("ERROR: tube detector only possible for flat geometry. Use detector array to build cylindrical shape out of flat (sub)detectors.\n");
    if(xResolution!=0)
      fprintf(LogFilePtr,"\nWARNING: x-resolution not 0 but has no meaning in tube detector (hence will not be used).");

    if(vertTubeOrientation){
      if(PixelWidth[0]!=PixelWidth[1])
	myExit("ERROR: tube detector requires Thickness/number_of_layers=Width/number_of_columns (diameter of tube) for vertical tube orientation \n");
      if(horResolution!=0)
	fprintf(LogFilePtr,"\nWARNING: hor. resolution not 0 but has no meaning in vertical tubes (hence will not be used).");
    }
    else if(!vertTubeOrientation){
      if(PixelWidth[0]!=PixelWidth[2])
	myExit("ERROR: tube detector requires Thickness/number_of_layers=Height/number_of_rows (diameter of tube) for horizontal tube orientation \n");
      if(vertResolution!=0)
	fprintf(LogFilePtr,"\nWARNING: vert. resolution not 0 but has no meaning in horizontal tubes (hence will not be used).");
    }

    /*add tubes if shifted so whole area is covered */
    if(Tubeshift){
      if(vertTubeOrientation){
	NColumns+=2;
	Width+=2*PixelWidth[1];
      }
      else {
	NRows+=2;
	Height+=2*PixelWidth[2];
      }
    }
  }

  //extend detector to avoid intensity drop at borders
  if(vertResolution!=0){
    Height+=2*(floor(vertResolution/PixelWidth[2])+1)*PixelWidth[2];
    NRows+=2*(floor(vertResolution/PixelWidth[2])+1);
  }
  if(horResolution!=0){
    Width+=2*(floor(horResolution/PixelWidth[1])+1)*PixelWidth[1];
    NColumns+=2*(floor(horResolution/PixelWidth[1])+1);
  }
   if(xResolution!=0){
    Thickness+=2*(floor(xResolution/PixelWidth[0])+1)*PixelWidth[0];
    NLayers+=2*(floor(xResolution/PixelWidth[0])+1);
  }

  if(geom!=0)
    {	if(geom==1)
	{	/* cylinder */

	  if(cyl_axis==0 && fabs(Theta-M_PI/2)>0.0001){
	    fprintf(LogFilePtr,"\n WARNING: Theta is set to 90° for cylinder in x direction!");
	    Theta=M_PI/2;
	  }
 
	  Detector.SG.Cyl.r=Distance;
	  Detector.SG.Cyl.height = Height;
	 
	  Detector.Direction[0]=0.0;
	  Detector.Direction[1]=0.0;
	  Detector.Direction[2]=0.0;
	  Detector.Direction[cyl_axis]=1.0;

	  Detector.Position[0]=0.0;
	  Detector.Position[1]=0.0;
	  Detector.Position[2]=0.0;

	  if (cos(Phi) < 0.0) Theta=-Theta;

	  NeutronIntersectsDetector=NeutronIntersectsCylDetector;
	  DetectorSpot=CylinderDetSpot;

	  bVisInstalled = TRUE; 
	  // Geometry data
	  if (bVisInstr)
	    { 
	      double ry, rz;
	      stGeometry.pCylSlice = (VtCylSlice*) calloc(1, sizeof(VtCylSlice));
	      stGeometry.nCylSlices = 1; 
	      
	      stGeometry.pCylSlice[0].Radius = Distance; 
	      stGeometry.pCylSlice[0].Width  = Width;
	      stGeometry.pCylSlice[0].Height = Height;
	      stGeometry.pCylSlice[0].vCntr[0]  = 0.;
	      stGeometry.pCylSlice[0].vCntr[1]  = 0.;
	      stGeometry.pCylSlice[0].vCntr[2]  = 0.;
	      stGeometry.pCylSlice[0].vSymAxis[0]= 0;
	      stGeometry.pCylSlice[0].vSymAxis[1]= 0;
	      stGeometry.pCylSlice[0].vSymAxis[2]= 0;
	      stGeometry.pCylSlice[0].vSymAxis[cyl_axis]= 1;
	      stGeometry.pCylSlice[0].Phi = Theta/M_PI*180.;
	      //coordinates differently defined in visualization:
	      if(cyl_axis==0)
		stGeometry.pCylSlice[0].Phi = Phi/M_PI*180.+90.;
	      else if(cyl_axis==1)
		stGeometry.pCylSlice[0].Phi = -Theta/M_PI*180.;
	      
	      stGeometry.pCylSlice[0].OpenAngle = Width/(2.*M_PI*Distance)*360.;
	      RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
	      stGeometry.pCylSlice[0].Phi += rz/M_PI*180.;
	      stGeometry.pDescr  = "detector:cyan";
	      stGeometry.eModule = VT_DETECTOR;
	    }

	} 
      else 
	{
	  /* cube */

	  Detector.SG.Cube.height = Height;
	  Detector.SG.Cube.width =Width;
	  Detector.SG.Cube.thickness = Thickness;
	  Detector.Direction[0]=cos(Theta);
	  Detector.Direction[1]=sin(Theta)*cos(Phi);
	  Detector.Direction[2]=sin(Theta)*sin(Phi);
	  for(i=0; i<3;i++)
	    if(fabs(Detector.Direction[i])<1e-5) Detector.Direction[i]=0.0;

	  Detector.Position[0] = Distance;
	  Detector.Position[1] = 0.0;
	  Detector.Position[2] = 0.0;
	  NeutronIntersectsDetector=NeutronIntersectsCubeDetector;
	  DetectorSpot=CubeDetSpot;

	  bVisInstalled = TRUE;
	  // Geometry data
	  if (bVisInstr)
	    { 
	      stGeometry.pCuboid = (VtCuboid*) calloc(1, sizeof(VtCuboid));
	      stGeometry.nCuboids = 1; 

	      stGeometry.pCuboid[0].Length = Thickness; 
	      stGeometry.pCuboid[0].Width  = Width;
	      stGeometry.pCuboid[0].Height = Height;

	      stGeometry.pCuboid[0].vCntr[0]  = (Distance+Thickness/2)*cos(Theta);
	      stGeometry.pCuboid[0].vCntr[1]  = (Distance+Thickness/2)*sin(Theta)*cos(Phi);
	      stGeometry.pCuboid[0].vCntr[2]  = (Distance+Thickness/2)*sin(Theta)*sin(Phi);
	      stGeometry.pCuboid[0].vNormal[0]= cos(Theta);
	      stGeometry.pCuboid[0].vNormal[1]= sin(Theta)*cos(Phi);
	      stGeometry.pCuboid[0].vNormal[2]=  sin(Theta)*sin(Phi);
	      
	      stGeometry.pDescr  = "detector:cyan";
	      stGeometry.eModule = VT_DETECTOR;
	    }
	}
    } 
  else 
    {
      fprintf(LogFilePtr,"ERROR: You have to supply a detector geometry with -G!\n");
      exit(-1);
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
    fprintf(LogFilePtr,"\nWARNING: %d neutrons lost between tubes",lost);
 
  fprintf(LogFilePtr," \n");

  /* set description for instrument plot */
  stPicture.dWPar  = Width;
  stPicture.dHPar  = Width/NColumns;
  stPicture.dRPar  = Distance;
  stPicture.eType  = (short) geom;
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


