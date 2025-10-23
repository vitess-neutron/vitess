/*********************************************************************************************/
/*  VITESS module 'new_module'                                                               */
/*                                                                                           */
/* This module simulates  ..............                                                     */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Dec 2024  X. Familyname   initial version                                            */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]); // reads input parameters and initializes global variables
void  CalcPar();                       // calculates the parameters that are dependent from input parameters (optional, can also be carried out in OwnInit())
void  ReadXyzFile(char* sFileName);    // reads data from input file (optional) */
void  SetGeometry(char* sColor);       // fills the structure stGeometry for visualization  (needed for components representing hardware)
void  OwnCleanup();                    // does module specific clean up


/*************************** ***/
/** Global Variables          **/
/*************************** ***/
char  *pFileName=NULL;                 // -F   [-]     pointer to the name of the input file
short  bFlag1=OFF;                     // -a   [-]     Flag: description of the flag      (0: off  1: on)
int    nItems =0;                      // -n   [-]     number of ....
long   nValues=1;                      // -N   [-]     number of ....
double ParA=0.0,                       // -A  [unit]   description of parameter A
       ParB=0.0,                       // -B  [unit]   description of parameter B
       Dist=0.0;                       // -D   [cm]    distance of (the center of) the module from entry point of the component
VtAxis direction=NO_AXIS;              // -Q    [-]    axis of ... direction
                                       //              see 'convert.h' for all existing enums related to radio buttons

// Parameters determined from input parameters
double *Array=NULL;                    //      [-]     array of values from file
Plane   Endpoint;                      //      [cm]    4D vector defining the plane of the component (e.g chopper, slit)


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  long       i=0;                              // index of trajectories
  double     ToF=0.0;                          // flight time from entry to point of intersection
  Neutron    OutNeutron;                       // parameter set of the outgoing neutron
  ParChange* pChange=NULL;                     // only needed for LiveMode

  /* the following parameters will be needed for the live mode realized in VITESS 4
  char       aChng[3][CHAR_BUF_XS]={"","",""}, // strings defining change and pointers to it
            *pChng[3]={NULL,NULL,NULL};
  pChng[0]=aChng[0]; pChng[1]=aChng[1]; pChng[2]=aChng[2]; */

  // Initialisation
  // --------------
  InitNeutron(&OutNeutron);         // many structures have functions for initialization (see 'general.h')

  _eModule = MCN_TEMPLATE;          // this parameter defines the module
                                    // a new module requires a new value of the enum 'McCompID' in 'defines.h'
                                    // and a conversion between text and enum value in CompID2Name() and Name2CompID()

  // basic functions to read and assess input data
  Init(argc,argv, _eModule);        // this assesses the general parameters (from 'init.h') like log file (corresponding to parameters defined as --X in the pipe)
  PrintModuleName(_eModule, "1.0"); // give the version of this module here
  OwnInit(argc, argv);              // this assesses the module specific parameters defined above (corresponding to parameters defined as -X in the pipe)

  // calculates the parameters that are dependent from input parameters (optional, can also be carried out in OwnInit()
  CalcPar();

  // reads data from input file (optional) */
  ReadXyzFile(pFileName);

  bInit         = TRUE;             // initialization has been executed once  (needed for LiveMode, e.g. for 'realloc' instead of 'alloc')
  bVisInstalled = TRUE;             // this module represents a piece of hardware that has to be shown in the instrument and trajectory visualization, otherwisie FALSE
  if (bVisInstr)
    bBlowUp = TRUE;                 // if 'blowup' is wished, do it for this module    (blowup means an enlargement of the cross-section in the visualization)

  DECLARE_ABORT

  // Loop over all events trajectories
  // ---------------------------------
  while (ReadNeutrons()!= 0)
  {
    for (i=0; i<NumNeutGot; i++)
    {
      CHECK

      pChange = (ParChange*)&InputNeutrons[i];      // an event can be a neutron trajectory or a command for a module

      // If an EOB or a RESET line is found, only write out the event and skip the rest, since that is only for monitors
      if (IsEOB(&InputNeutrons[i])==TRUE || IsReset(pChange)==TRUE)
      {
        WriteNeutron(&InputNeutrons[i]);
      }
      /* in LiveMode: check if the event is a change
      else if (IsChngRes(pChange)==TRUE)
      {
        // evaluate changes for this module
        if (IsMyChange(pChange)==TRUE)
        {
          ConvertEvent2Strg(pChng, pChange);   // converts the event 'pChange' to a string 'pChng' that can be handled by the init routines
          Init   (2, pChng, _eModule);
          OwnInit(3, pChng);
          CalcPar();
          if (.....)
            ReadFile();
        }
        else
        { // write changes for following modules to pipe
          WriteChange(pChange);
        }
      } */
      else
      {
        // checks of the neutron trajectory (optional)
        if (InputNeutrons[i].Wavelength == 0.0) continue;

        // copy input to output trajectory   (not necessary, but useful)
        CopyNeutron(&InputNeutrons[i], &OutNeutron);

        // move neutron to a plane of intersection within the component and calculate Time of Flight (ToF)
        // parameters in OutNeutron are updated, except for the ToF
        if (keygrav == 1)
          ToF = NeutronPlaneIntersectionGrav(&OutNeutron, Endpoint);   // gravity considered
        else
          ToF = NeutronPlaneIntersection1(&OutNeutron, Endpoint);      // gravity ignored

        // Write out neutron data for the next module and the trajectory visualization, if the neutro passed the component
        // example here: passed through a slit of 'ParA' width and 'ParB' height
        if (fabs(OutNeutron.Position[1]) < 0.5*ParA  &&  fabs(OutNeutron.Position[2]) < 0.5*ParB)
        {
          WriteIAP(&OutNeutron, VT_PASSED);

          OutNeutron.Time += ToF;
          OutNeutron.Position[0]=0.0;   // x position (along the beamline) is set to zero after each module

          WriteNeutron(&OutNeutron);
        }
        // Write not, write out data for the trajectory visualization anyway
        else
        {
          WriteIAP(&OutNeutron, VT_OUT_OF_WND);
        }
      }
    }
  }

// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
  // write geometry data for visualization
  SetGeometry("grey");

  // write into log file, free memory ... (optional, can also be done here)
  OwnCleanup();

  // print intensity, write instrument.inf   (see init.c)
  Cleanup(Dist,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int i=0;

  InitPlane(&Endpoint);

  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!='+')
    {
      switch(argv[i][1])
      {
        case 'F':
          pFileName=&argv[i][2];
          break;

        case 'a':
          bFlag1 = (short) atoi(&argv[i][2]);
          break;
        case 'n':
          nItems  = atoi(&argv[i][2]);
          break;
        case 'N':
          nValues = atol(&argv[i][2]);
          break;

        case 'A':
          ParA  = atof(&argv[i][2]);
          break;
        case 'B':
          ParB  = atof(&argv[i][2]);
          break;
        case 'D':
          Dist  = atof(&argv[i][2]);
          break;

        case 'Q':
          direction = (VtAxis)atoi(&argv[i][2]);
          break;

        default:
          Error2("unknown command option", argv[i]);
          break;
      }
    }
  }
}


/*************************************************************************/
/** calculates the parameters that are dependent from input parameters  **/
/*************************************************************************/
void CalcPar()
{
  Endpoint.A =  1.0;
  Endpoint.D = -1.0*Dist;
}


/*******************************************************/
/** Reads data from input file                        **/
/*******************************************************/
void ReadXyzFile(char* sFileName)
{
  int iCnt=0,                    // index of the lines in the file
      nLines;                    // number of lines in the input file
  char sLine[CHAR_BUF_SMALL]=""; // content of 1 line

  // opens input file (program exit in case of error)
  FILE* pFile = OpenParameterFile(sFileName, TRUE, "r");
  //         or OpenParameterFile (sFileName, TRUE, "r");

  if (pFile!=NULL)
  {
    nLines = LinesInFile(pFile);

    Array = (double*) calloc(nLines, sizeof(double));

    rewind(pFile);
    for (iCnt=0; iCnt < nLines; iCnt++)
    {
      ReadLine(pFile, sLine, sizeof(sLine)-1);
      sscanf(sLine, "%lf", &Array[iCnt]);
    }

    fclose(pFile);
  }
}


/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the component geometry (see init.h for the possible geometrical elements)
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.pHull  = calloc(1, sizeof(VtHull));
    stGeometry.nHulls = 1;

    stGeometry.pHull[0].Length     = 0.0;
    stGeometry.pHull[0].WidthIn    = BlowUp * ParA;       // BlowUp allows stretching the visualization
    stGeometry.pHull[0].WidthOut   = BlowUp * ParA * 3.0; // perpendicular to the beamline
    stGeometry.pHull[0].HeightIn   = BlowUp * ParB;
    stGeometry.pHull[0].HeightOut  = BlowUp * ParB * 3.0;
    stGeometry.pHull[0].vCntr[0]   = Dist;
    stGeometry.pHull[0].vCntr[1]   = 0.0;
    stGeometry.pHull[0].vCntr[2]   = 0.0;
    stGeometry.pHull[0].vNormal[0] = 1.0;
    stGeometry.pHull[0].vNormal[1] = 0.0;
    stGeometry.pHull[0].vNormal[2] = 0.0;
    stGeometry.pHull[0].rotAngle   = 0.0;
  }

  return;
}


/*******************************************************/
/** Does module specific cleanup (free memory, etc.)  **/
/*******************************************************/
void OwnCleanup(int argc, char *argv[])
{
  fprintf(LogFilePtr, "parameters: %6.2f x %6.2f cm \n", ParA, ParB);
}
