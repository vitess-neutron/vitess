#ifndef DETECTOR_H
#define DETECTOR_H


#define VT_FLAT_AREA  0
#define VT_CYL_AREA   1
#define VT_FLAT_TUBES 2


typedef struct
{
  double r;
  VtAxis eAxis;
  short bCnstPhi;  // 0 = const. pixel size; 1 = const Delta phi 
}
CylinderDetectorType;

typedef struct
{
  double wallThickness;       // only tube det: thickness of tube walls
  short  bTubeShift;          // tube layers shifted against each other
  VtOrient    eTubeOrient;    // horizontal (0) or vertical (1) or tube axis
  VtTubeShape eTubeShape;     // circular (0) or rectangular (1) tube cross-section
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
  double Theta, Phi, Distance,  // detector coordinates (center) 
         Phi_n, Theta_n;        // inclination of detector surface w.r.t. position vector
  double AbsPar1,               // Absorption parameters: pressure and temp. for gas
         AbsPar2,               //                        thickness and density for solids
         GasPressure,           // only gas detectors: pressure 
         GasTemperature,        //                 and temperature of the converter gas
         SolidAbsThickness,     // only solid detectors: thickness 
         SolidAtomDensity,      //                   and density of the converter layer
         EfficiencyMod;         // modify efficiency
  VtDetGeom eGeom;              // geometry 1: cyl     2: flat   
  VtDetType eType;              // type     0: tubes   1: area/volume
  VtDetUse  eUsage;             // usage    0: realistic (normal)   1: monitor only   2: grid off
  VtDetAbs  eAbsMat;            // Boron10 (0,2), He3 (1), Li (3) or other (5)               
  short  bArray,                // first or intermediate part of detector array        should become type 'VtModAct'
         minColor,              /* colour necessary for the trajectory to be regarded
                                   colour -1 means: all trajectories are regarded  
                                   use neutrons with color >= minColour */
         maxColor,              /* colour necessary for the trajectory to be regarded
                                   colour -1 means: all trajectories are regarded  
                                   use neutrons with color <= maxColour */
         addColor;              // tag detected neutrons by adding addColor to color
}
DetectorType;


/* function pointers */
short (*NeutronIntersectsDetector)(Neutron *Nin, VectorType ISP[]);
void (*DetectorSpot)(VectorType SP, VectorType DetSpot);

/*******************************************************/
/** prototypes of local functions                     **/
/*******************************************************/
void  OwnInit     (int argc, char *argv[]);   // reads input parameters and initializes global variables
void  OwnCleanup  ();                         // does module specific cleanup
void  SetGeometry (char* sColor);             // fills the structure stGeometry for visualization
void  InitDetector(DetectorType* pDetector);  // Initializes the detector structure

/* Intersection of neutron with different geometries       */
short NeutronIntersectsCubeDetector(Neutron *Nin, VectorType ISP[]);
short NeutronIntersectsCylDetector (Neutron *Nin, VectorType ISP[]);
short NeutronIntersectsLayer(VectorType dir, VectorType pos, int l, VectorType jISP[]);
short NeutronIntersectsTube (VectorType dir, VectorType pos, int l, VectorType iISP[], VectorType kISP[]);

/* final detection position for different detector geometries */
void  CubeDetSpot     (VectorType SP, VectorType DetSpot);
void  CylinderDetSpot (VectorType SP, VectorType DetSpot);
void  CubeDetLayerSpot(VectorType SP);

/* efficiency and absorption cross-section as a function of wavelength   */
double GetLambdaProbFromEff(const double lambda, const TotalID NeutronID); 
double GetXsec(int h_absorbertype, double h_lambda);                        

/* checks and completes detector geometry   */
void  CheckAndAdjustDetectorInput(VtDetType type, VtDetGeom geom);

#endif
