#ifndef SAMPLE_REFL_H
#define SAMPLE_REFL_H

#include "defines.h"
#include "general.h"


/**************************************************/
/** Prototypes                                   **/
/**************************************************/
void   OwnInit(int argc, char *argv[]);                                                                   // Own initialization of the sample_reflectom module
void   CalcAndWritePar();                                                                                 // calculates arrays from input parameters and writes to log file
void   ReadReflectivityFile();                                                                            // reads file containing reflectivity curve of the sample
void   SetSamplePar        ();                                                                            // Reads sample parameters and combines with input parameters
void   SetGeometry(char* sColor);                                                                         // Fills the structure stGeometry for visualization
void   OwnCleanup();                                                                                      // Does module specific cleanup

short   FindSmplISPs  (VectorType ISP1, VectorType ISP2, Neutron* pNeutron);                               // checks, if neutron hits sample or substrate, transfers into that frame of the sample and returns intersection points
short   FindSubstrISPs(double*   Dist1, double*   Dist2, const Neutron* pNeutron);                         // checks, if neutron hits the substrate and returns distances to intersection points
short  PassThrough(Neutron* OutNeutron, double PathLen, VtMirrMat eMat, short bWrite);                    // writes a trajectory passing through the substrate

double CalcReflect(const double Q);                                                                       // Returns reflectivity value interpolated from the reflectivity curve data read in ReadReflectivityFile()
void   AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert);     // computes frame angles of output
void   CalculateThetaRange();                                                                             // Calculates minimal and maximal theta angle
void   CalculatePhiRange(double theta, double *phiMin, double *phiMax);                                   // For detectors close to the direct beam, deltaPhi is a function of theta
                                                                                                          //   Calculate corresponding deltaPhi for each trajectory individually.
int    ScatterSpecular   (const Neutron* InNeutron, const Neutron* ScatNeutron, const double ScatAngle);  // Calculate trajectory parameters after specular scattering
void   ScatterOffspecular(const Neutron* InNeutron, const Neutron* ScatNeutron, const double ScatAngle);  // Create new trajectories and calculate their parameters for offspecular scattering
void   ScatterIncoherent (const Neutron* InNeutron, const double TofToRefl, const double PathInSmpl);     // Scatters the neutron isotropically into a given detector

int    FindQf     (double Qin, int QfBin, double* Qf, double* Refl);                                      // Finds the current Q_f value for the offspecular scattering
void   ScatterByQf(Neutron* ScatNeutron,  const double Qin, const double Qf);                             // Determines the direction of the neutron at the scattering location
                                                                                                          // such that the direction vector matches the requires Q_f
void   TransfBack2Input(Neutron* OutNeutron, short bDepth);                                               // Neutron parameters are transferred back to the original coordinate system,
void   Transf2Output   (Neutron* OutNeutron);                                                             // Neutron parameters are transferred to the output frame
#endif
