#ifndef DETECTOR_H
#define DETECTOR_H

typedef struct
{
  double r;
  int axis;
  short phimode;  // 0 = const. pixel size; 1 = const Delta phi 
}
  CylinderDetectorType;

typedef struct
{
  double wallThickness;       // only tube det: thickness of tube walls
  short  vertTubeOrientation, // vertical (1) or horizontal (0) tube axis
         rectXsec,            // circular (0) or rectangular (1) tube cross-section
         tubeshift;           // tube layers shifted against each other
}
  TubeDetectorType;

typedef struct
{
  double Lambda, Eff;
}
  EffData;


typedef struct
{
  FILE    *pfile;
  EffData *data;
  char    *filename;
  long    maxdata;
}
  EffFile;

typedef struct
{
  CylinderDetectorType Cyl;
  TubeDetectorType Tube;
}
  DetectorGeomType;

typedef struct 
{
  DetectorGeomType DG;
  VectorType Position;
  VectorType Direction;
  double PixelWidth[3];
  double Resolution[3];  
  double Thickness, Width, Height;
  double NLayers, NColumns, NRows;
  double Theta, Phi, Distance,       // detector coordinates (center) 
         Phi_n, Theta_n;             // inclination of detector surface w.r.t. position vector
  double GasPressure, GasTemperature,               // only gas det: gas pressure and temperature
         SolidAtomDensity, SolidAbsorberthickness,  // only solid det: density (N) and thickness of converter layer
         EfficiencyMod;                             // modify efficiency
  int Geom,         // 0: flat,   1: cyl,          2: tube
      usage,        // 0: normal, 1: monitor only, 2: grid off
      Absorbertype; // Boron10 (0,2), He3 (1), Li (3) or other (5)               
  short array,          // first or intermediate part of detector array 
        minColor,       /* colour necessary for the trajectory to be regarded
                           colour -1 means: all trajectories are regarded  
                           use neutrons with color >= minColour */
        maxColor,       /* colour necessary for the trajectory to be regarded
                           colour -1 means: all trajectories are regarded  
                           use neutrons with color <= maxColour */
        addColor;       // tag detected neutrons by adding addColor to color
 }
  DetectorType;





/* function pointers */
short (*NeutronIntersectsDetector)(Neutron *Nin, VectorType ISP[]);
void (*DetectorSpot)(VectorType SP, VectorType DetSpot);

/* prototypes of local functions */
short NeutronIntersectsCubeDetector(Neutron *Nin, VectorType ISP[]);
short NeutronIntersectsCylDetector (Neutron *Nin, VectorType ISP[]);
short NeutronIntersectsLayer(VectorType dir, VectorType pos, int l, VectorType jISP[]);
short NeutronIntersectsTube(VectorType dir, VectorType pos, int l, VectorType iISP[], VectorType kISP[]);

void CubeDetSpot    (VectorType SP, VectorType DetSpot);
void CylinderDetSpot(VectorType SP, VectorType DetSpot);

void OwnInit   (int argc, char *argv[]);
void OwnCleanup();
void CheckAndAdjustDetectorInput(int type);

double GetXsec(int h_absorbertype, double h_lambda);
double GetLambdaProbFromEff(const double lambda, const TotalID NeutronID);

#endif
