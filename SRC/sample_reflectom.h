#ifndef SAMPLE_REFL_H 
#define SAMPLE_REFL_H

#include "common.h"
#include "general.h"


/**************************************************/
/** Prototypes                                   **/
/**************************************************/
short  Reflect    (Neutron* pNeutron, short int treatingReflection);                                  // controls, if neutron is reflected and gives output in frame of CE
double ReadReflect(const double dQ, TotalID eID);                                                     // Returns interpolated reflectivity value
void   OwnInit(int argc, char *argv[]);                                                               // Own initialization of the sample_reflectom module
void   CalcAndWritePar();                                                                             // calculates arrays from input parameters and writes to log file
void   SetGeometry(char* sColor);                                                                     // Fills the structure stGeometry for visualization 
void   OwnCleanup();                                                                                  // Does module specific cleanup    

void   ReadParameterFile();                                                                           // reads file containing sample parameters
void   ReadReflectivityFile();                                                                        // reads file containing reflectivity curve of the sample

void   AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert); // computes frame angles of output
void   CalculateThetaRange();                                                                         // Calculates minimal and maximal theta angle 
void   CalculatePhiRange(double theta, double *phiMin, double *phiMax, int *switchSign);              // For detectors close to the direct beam, deltaPhi is a function of theta
                                                                                                      //   Calculate corresponding deltaPhi for each trajectory individually.     
int    ScatterSpecular(double scatteringAngle, Neutron* inputNeutron, Neutron* outputNeutron);        // Calculate trajectory parameters after specular scattering
void   ScatterIncoherent(Neutron* outputNeutron);                                                     // Scatters the neutron isotropically into a given detector
void   ScatterOffspecular(double scatteringAngle, Neutron* inputNeutron,                              // Create new trajectories and calculate their parameters for offspecular scattering
                          Neutron* parentNeutron, Neutron* outputNeutron);   
int    FindQf(double Qin, int QfBin, double* Qf, double* refl);                                       // Finds the current Q_f value for the offspecular scattering
void   ScatterByQf(Neutron* ParentNeutron, Neutron* Neutrons, double dQin, double dQf);               // Determines the direction of the neutron at the scattering location
                                                                                                      // such that the direction vector matches the requires Q_f            
void  TransformBackToGlobalSystemAndWriteNeutron(Neutron* outputNeutron);                             // Neutron parameters are transformed back to the original coordinate system,  
                                                                                                      // taking into account a possible user outpur frame, and written to the stream. 
#endif
