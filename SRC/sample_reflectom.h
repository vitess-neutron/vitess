#ifndef SAMPLE_REFL_H 
#define SAMPLE_REFL_H

#include "defines.h"
#include "general.h"


/**************************************************/
/** Prototypes                                   **/
/**************************************************/
void   OwnInit(int argc, char *argv[]);                                                               // Own initialization of the sample_reflectom module
void   CalcAndWritePar();                                                                             // calculates arrays from input parameters and writes to log file
void   ReadReflectivityFile();                                                                        // reads file containing reflectivity curve of the sample
void   SetSamplePar        ();                                                                        // Reads sample parameters and combines with input parameters 
void   SetGeometry(char* sColor);                                                                     // Fills the structure stGeometry for visualization 
void   OwnCleanup();                                                                                  // Does module specific cleanup    

short  CheckRefl  (Neutron* pNeutron, short int treatingReflection);                                  // checks, if neutron hits the sample and transfers into frame of the sample
short	 FindISPs   (VectorType ISP1, VectorType ISP2, Neutron* pNeutron, short bSubstrate);            // checks, if neutron hits sample or substrate, transfers into that frame of the sample and returns intersection points
short	 FindSubISPs(double*   Dist1, double*   Dist2, Neutron* pNeutron);                              // checks, if neutron hits the substrate and returns distances to intersection points
short  PassThruAll(Neutron* outNeutron, VectorType P1, VectorType P2);                                // writes a trajectory passing through the substrate 
short  PassThrough(Neutron* outNeutron, double PathLen, VtMirrMat eMat, short bWrite);                // writes a trajectory passing through the substrate 

double CalcReflect(const double Q);                                                                   // Returns reflectivity value interpolated from the reflectivity curve data read in ReadReflectivityFile()
void   AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert); // computes frame angles of output
void   CalculateThetaRange();                                                                         // Calculates minimal and maximal theta angle 
void   CalculatePhiRange(double theta, double *phiMin, double *phiMax, int *switchSign);              // For detectors close to the direct beam, deltaPhi is a function of theta
                                                                                                      //   Calculate corresponding deltaPhi for each trajectory individually.     
int    ScatterSpecular(double scatteringAngle, Neutron* inputNeutron, Neutron* outputNeutron);        // Calculate trajectory parameters after specular scattering
void   ScatterIncoherent(Neutron* outputNeutron, double PathInSmpl);                                  // Scatters the neutron isotropically into a given detector
void   ScatterOffspecular(double scatteringAngle, Neutron* inputNeutron,                              // Create new trajectories and calculate their parameters for offspecular scattering
                          Neutron* parentNeutron, Neutron* outputNeutron);   

int    FindQf(double Qin, int QfBin, double* Qf, double* refl);                                       // Finds the current Q_f value for the offspecular scattering
void   ScatterByQf(Neutron* ParentNeutron, Neutron* Neutrons, double dQin, double dQf);               // Determines the direction of the neutron at the scattering location
                                                                                                      // such that the direction vector matches the requires Q_f            
void  TransformBackToGlobalSystemAndWriteNeutron(Neutron* outNeutron, short bRefl, short bDepth);     // Neutron parameters are transferred back to the original coordinate system,  
                                                                                                      //   then to the user output frame, and written to the stream. 
void  Transf2OutputAndWriteNeutron(Neutron* outNeutron);                                              // Neutron parameters are transferred to the output frame and written to the stream
#endif
