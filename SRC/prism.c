/*********************************************************************************************/
/*  VITESS module 'prism'                                                               */
/*                                                                                           */
/* This module simulates a matrix of identical prisms                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 10 Oct 2024  N. Violini   initial version                                                 */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]); // reads input parameters and initializes global variables
void CalcPar();                       // calculates the parameters that are dependent from input parameters (optional, can also be carried out in OwnInit())
void SetGeometry(char *sColor);       // fills the structure stGeometry for visualization  (needed for components representing hardware)
void OwnCleanup();                    // does module specific clean up

/*************************** ***/
/** Global Variables          **/
/*************************** ***/

double DistMove = 0.0;

double
    ScatteringLength, // -N   [10^-6 Ang^-2]   Scattering length density of the material
    BaseWidth,        // -b   [cm]        Width of the base of each prism
    BaseHeight,       // -h   [cm]        Height of the base of each prism
    s_i,              // -S   []          Incoherent scattering cross section
    s_a,              // -s   []          Absorption cross section
    rho,              // -D   []          Density of the material
    PrismHeight;      // -z   []          Width of the Energy Analyzer

int
    nRows,     // -k   []          Number of layers of prisms in vertical direction
    absorption, // -y   []          Absorbing layer yes or no
    nCols;     // -P   []          Number of columns in the prisms matrix

// Parameters determined from input parameters
Plane Endpoint; //      [cm]    4D vector defining the plane of the component

int y, j, ni, NeutronIn, n_hit, n_nohit;

double DeflAngle1, DeflAngle2, DeflSmallAngle, AngleIn1,
    AngleIn2, HorAngle, HorAngleNew, HorAngle2, Dist_1, Dist1_a, Dist1_b, Dist_2a,
    Dist_2b, Dist_2, Dist_3, Dist3_a, Dist3_b, Dist, EAHeight, Attenuation, Transmission,
    CritAngle, IndexRefraction, tof1, tof2c, tof2, tof2a, tof2b, tof3, tof1_a, tof1_b,
    tof3_a, tof3_b, wall, tof_0, tof, RefAngle;

Plane Prismhalf1, wall1, Prismhalf2, exitwall, UpperWall, entrancewall, DownWall, EAexit;

VectorType Prism, Prism2, NeutronPosition_1, NeutronPosition_0, NeutronPosition_2a,
    NeutronPosition_2b, NeutronPosition_1c, NeutronPosition_1d, NeutronPosition_2,
    NeutronPosition_3, NeutronPosition_3_a, NeutronPosition_3_b, NeutronPosition_1_a,
    NeutronPosition_1_b, NeutronVector_0, NeutronVector_2, UpperBorder;


/* Functions */

double  Reflexion (double w),
        ReflexionPrism(double a1, double a),
        Refraction1(double a1, double a),
        Refraction2(double w, double a),
        RefractionSmallAngle(double w),
        RefreshAngle(Neutron *ThisNeutron, double a6);

/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  long i = 0; // index of trajectories
  n_hit = 0;
  n_nohit = 0;
  Neutron OutNeutron;

  // Initialisation
  // --------------
  InitNeutron(&OutNeutron); // many structures have functions for initialization (see 'general.h')

  _eModule = MCN_PRISM; // this parameter defines the module
                        // a new module requires a new value of the enum 'McCompID' in 'defines.h'

  // basic functions to read and assess input data
  Init(argc, argv, _eModule);       // this assesses the general parameters (from 'init.h') like log file (corresponding to parameters defined as --X in the pipe)
  PrintModuleName(_eModule, "1.0"); // give the version of this module here
  OwnInit(argc, argv);              // this assesses the module specific parameters defined above (corresponding to parameters defined as -X in the pipe)

  // calculates the parameters that are dependent from input parameters (optional, can also be carried out in OwnInit()
  CalcPar();

  // bInit         = TRUE;             // initialization has been executed once  (may be needed to choose e.g. 'realloc' instead of 'alloc')
  bVisInstalled = TRUE; // this module represents a piece of hardware that has to be shown in the instrument and trajectory visualization, otherwisie FALSE
  if (bVisInstr)
    bBlowUp = TRUE; // if 'blowup' is wished, do it for this module    (blowup means an enlargement of the cross-section in the visualization)

  DECLARE_ABORT

  // Loop over all events trajectories
  // ---------------------------------
  while (ReadNeutrons() != 0)
  {
    for (i = 0; i < NumNeutGot; i++)
    {
      CHECK

      // If an EOB or a RESET line is found, only write out the event and skip the rest, since that is only for monitors
      if (IsEOB(&InputNeutrons[i]) == TRUE)
      {
        WriteNeutron(&InputNeutrons[i]);
      }
      else
      {
        // checks of the neutron trajectory (optional)
        if (InputNeutrons[i].Wavelength == 0.0 || InputNeutrons[i].Probability <= wei_min )
          continue;

        OutNeutron.Spin[0] = InputNeutrons[i].Spin[0];
        OutNeutron.Spin[1] = InputNeutrons[i].Spin[1];
        OutNeutron.Spin[2] = InputNeutrons[i].Spin[2];
        OutNeutron.Color = InputNeutrons[i].Color;
        OutNeutron.Debug = InputNeutrons[i].Debug;
        OutNeutron.ID = InputNeutrons[i].ID;
        OutNeutron.Wavelength = InputNeutrons[i].Wavelength;

        /* Calculation of the first prism position for the incoming neutron */
        NeutronIn = 1;
        tof_0 = NeutronPlaneIntersection1(&InputNeutrons[i], entrancewall);
        InputNeutrons[i].Time = InputNeutrons[i].Time + tof_0;

        /* Check if the neutron does not hit the matrix of prisms */
        if (InputNeutrons[i].Position[1] >=   PrismHeight / 2    ||
            InputNeutrons[i].Position[1] <=  -PrismHeight / 2    ||
            InputNeutrons[i].Position[2] >=   EAHeight / 2 ||
            InputNeutrons[i].Position[2] <=  -EAHeight / 2   )
        {
          NeutronIn = 0;
          n_nohit ++;
        }

        // if the neutron is inside the volume occupied by the prism matrix, cycle over the matrix columns
        if (NeutronIn >= 1)
        {
          n_hit++;
          /* Calculates refraction index, critical angle and attenuation probability */
          IndexRefraction = 1 - (sq(InputNeutrons[i].Wavelength) / (2 * M_PI) * ScatteringLength);
          CritAngle = acos(IndexRefraction) * 180 / M_PI;
          Attenuation = ((s_a * (InputNeutrons[i].Wavelength / 1.8) + s_i) * rho) / 1000;

          // ni represents the number of row the neutron is currently in, nRows the total number of rows
          ni = (int)(floor((nRows * BaseHeight / 2 - InputNeutrons[i].Position[2]) / BaseHeight) + 1);
          InputNeutrons[i].Position[2] = ni * BaseHeight - (nRows * BaseHeight / 2 - InputNeutrons[i].Position[2]);

          for (j = 1; j <= nCols; j++)
          {
            HorAngle = atan(InputNeutrons[i].Vector[2] / InputNeutrons[i].Vector[0]) * 180 / M_PI;
            HorAngleNew = HorAngle * M_PI / 180;
            RefreshAngle(&InputNeutrons[i], HorAngleNew);
            NormVector(InputNeutrons[i].Vector);

            if (InputNeutrons[i].Position[2] <= 0) /* Neutrons from a prismbox below */
            {
              if (absorption >= 1)
              {
                InputNeutrons[i].Probability = 0;
                /* Why is this zero and not attenuated by the absorption x-section?*/
              }

              CopyVector(InputNeutrons[i].Position, NeutronPosition_1);

              NeutronVector_0[2] = InputNeutrons[i].Vector[2];
              HorAngle = AngleVectors(InputNeutrons[i].Vector, UpperBorder);
              DeflSmallAngle = RefractionSmallAngle(HorAngle);

              RefreshAngle(&InputNeutrons[i], DeflSmallAngle);
              NormVector(InputNeutrons[i].Vector);

              /* Intersection with the second prismhalf: */
              tof2 = NeutronPlaneIntersection1(&InputNeutrons[i], Prismhalf2);

              CopyVector(InputNeutrons[i].Position, NeutronPosition_2);

              Dist_2 = DistVector(NeutronPosition_1, NeutronPosition_2);

              NeutronPosition_0[2] = NeutronPosition_1[2];
              goto RefractionPrism2;
            }

            if (InputNeutrons[i].Position[2] >= BaseHeight) /* Neutrons from prismbox above */
            {
              if (absorption >= 1)
              {
                InputNeutrons[i].Probability = 0;
              }

              CopyVector(InputNeutrons[i].Position, NeutronPosition_1);

              NeutronVector_0[2] = InputNeutrons[i].Vector[2];
              HorAngle = AngleVectors(InputNeutrons[i].Vector, UpperBorder);
              DeflSmallAngle = RefractionSmallAngle(HorAngle);

              RefreshAngle(&InputNeutrons[i], DeflSmallAngle);
              NormVector(InputNeutrons[i].Vector);

              tof2 = NeutronPlaneIntersection1(&InputNeutrons[i], Prismhalf1);

              CopyVector(InputNeutrons[i].Position, NeutronPosition_2);

              Dist_2 = DistVector(NeutronPosition_1, NeutronPosition_2);

              NeutronPosition_0[2] = NeutronPosition_1[2];
              goto RefractionPrism1;
            }

            if (InputNeutrons[i].Position[2] > 0) /* the regular case */
            {
              if (InputNeutrons[i].Position[2] >= BaseHeight)
              {
                if (InputNeutrons[i].Position[0] >= BaseWidth / 2)
                {
                  tof3 = NeutronPlaneIntersection1(&InputNeutrons[i], exitwall);

                  CopyVector(InputNeutrons[i].Position, NeutronPosition_3);
                  goto Exit;
                }
              }

              CopyVector(InputNeutrons[i].Position, NeutronPosition_0);

              NeutronVector_0[2] = InputNeutrons[i].Vector[2];
              tof1 = NeutronPlaneIntersection1(&InputNeutrons[i], Prismhalf1);

              CopyVector(InputNeutrons[i].Position, NeutronPosition_1);

              if (InputNeutrons[i].Position[2] >= BaseHeight)
              {
                InputNeutrons[i].Position[0] = NeutronPosition_0[0];
                InputNeutrons[i].Position[1] = NeutronPosition_0[1];
                InputNeutrons[i].Position[2] = NeutronPosition_0[2];
                InputNeutrons[i].Vector[2] = NeutronVector_0[2];
                HorAngle = AngleVectors(InputNeutrons[i].Vector, UpperBorder);

                if (ni <= 1)
                {
                  tof1_a = NeutronPlaneIntersection1(&InputNeutrons[i], UpperWall);
                  NeutronIn = 0;
                  goto Exit;
                }

                if (HorAngle < CritAngle)
                {
                  if (absorption >= 1)
                  {
                    InputNeutrons[i].Probability = 0;
                  }

                  RefAngle = Reflexion(HorAngle);
                  tof1_a = NeutronPlaneIntersection1(&InputNeutrons[i], UpperWall);

                  if (InputNeutrons[i].Position[0] > PrismHeight / 2)
                  {
                    InputNeutrons[i].Position[0] = 0;
                    InputNeutrons[i].Position[1] = NeutronPosition_0[1];
                  }

                  CopyVector(InputNeutrons[i].Position, NeutronPosition_1_a);

                  Dist1_a = DistVector(NeutronPosition_0, NeutronPosition_1_a);

                  RefreshAngle(&InputNeutrons[i], RefAngle);
                  NormVector(InputNeutrons[i].Vector);

                  tof1_b = NeutronPlaneIntersection1(&InputNeutrons[i], Prismhalf1);

                  CopyVector(InputNeutrons[i].Position, NeutronPosition_1_b);

                  NeutronPosition_1[0] = NeutronPosition_1_b[0];
                  NeutronPosition_1[1] = NeutronPosition_1_b[1];
                  NeutronPosition_1[2] = NeutronPosition_1_b[2];

                  Dist1_b = DistVector(NeutronPosition_1_a, NeutronPosition_1_b);
                  Dist_1 = Dist1_a + Dist1_b;
                  tof1 = tof1_a + tof1_b;
                  goto RefractionPrism1;
                }

                else if (HorAngle >= CritAngle)
                {
                  tof1_a = NeutronPlaneIntersection1(&InputNeutrons[i], UpperWall);

                  CopyVector(InputNeutrons[i].Position, NeutronPosition_1_a);

                  if (InputNeutrons[i].Position[0] > PrismHeight / 2)
                  {
                    InputNeutrons[i].Position[0] = 0;
                    InputNeutrons[i].Position[1] = NeutronPosition_0[1];

                    CopyVector(InputNeutrons[i].Position, NeutronPosition_1_a);
                  }

                  Dist1_a = DistVector(NeutronPosition_0, NeutronPosition_1_a);
                  Dist_1 = Dist1_a;
                  tof1 = tof1_a;
                  wall = 2;
                  goto Exit;
                }
              }

              else if (InputNeutrons[i].Position[2] < BaseHeight)
              {
                Dist_1 = DistVector(NeutronPosition_0, NeutronPosition_1);
                goto RefractionPrism1;
              }
            }

            RefractionPrism1:
            {
              AngleIn1 = AngleVectors(InputNeutrons[i].Vector, Prism);
              HorAngle = atan(InputNeutrons[i].Vector[2] / InputNeutrons[i].Vector[0]) * 180 / M_PI;

              if (AngleIn1 <= CritAngle) /* Reflection / Refraction at the first prismhalf */
              {
                ReflexionPrism(AngleIn1, HorAngle);
              }

              else
              {
                DeflAngle1 = Refraction1(AngleIn1, HorAngle);
              }

              RefreshAngle(&InputNeutrons[i], DeflAngle1);
              NormVector(InputNeutrons[i].Vector);
              CopyVector(InputNeutrons[i].Position, NeutronPosition_1c);

              tof2 = NeutronPlaneIntersection1(&InputNeutrons[i], Prismhalf2);

              if (InputNeutrons[i].Position[2] <= -BaseHeight / 2)
              {
                InputNeutrons[i].Position[0] = NeutronPosition_1c[0];
                InputNeutrons[i].Position[1] = NeutronPosition_1c[1];
                InputNeutrons[i].Position[2] = NeutronPosition_1c[2];

                tof2c = NeutronPlaneIntersection1(&InputNeutrons[i], DownWall);

                CopyVector(InputNeutrons[i].Position, NeutronPosition_1d);

                Dist_2 = DistVector(NeutronPosition_1c, NeutronPosition_1d);
                wall = -2;
                goto Exit;
              }

              CopyVector(InputNeutrons[i].Position, NeutronPosition_2);
              NeutronVector_2[2] = InputNeutrons[i].Vector[2];
              Dist_2 = DistVector(NeutronPosition_1, NeutronPosition_2);
            }

            RefractionPrism2:
            {
              AngleIn2 = AngleVectors(InputNeutrons[i].Vector, Prism2);
              HorAngle2 = atan(InputNeutrons[i].Vector[2] / InputNeutrons[i].Vector[0]) * 180 / M_PI;
              DeflAngle2 = Refraction2(AngleIn2, HorAngle2);

              RefreshAngle(&InputNeutrons[i], DeflAngle2);
              NormVector(InputNeutrons[i].Vector);

              tof3 = NeutronPlaneIntersection1(&InputNeutrons[i], exitwall);

              CopyVector(InputNeutrons[i].Position, NeutronPosition_3);

              if (InputNeutrons[i].Position[2] >= BaseHeight)
              {

                InputNeutrons[i].Position[0] = NeutronPosition_2[0];
                InputNeutrons[i].Position[1] = NeutronPosition_2[1];
                InputNeutrons[i].Position[2] = NeutronPosition_2[2];
                InputNeutrons[i].Vector[2] = NeutronVector_2[2];

                tof3_a = NeutronPlaneIntersection1(&InputNeutrons[i], UpperWall);

                CopyVector(InputNeutrons[i].Position, NeutronPosition_3_a);

                HorAngle = AngleVectors(InputNeutrons[i].Vector, UpperBorder);

                if (ni <= 1)
                {
                  NeutronIn = 0;
                  goto Exit;
                }

                if (HorAngle < CritAngle)
                {
                  if (absorption >= 1)
                  {
                    InputNeutrons[i].Probability = 0;
                  }

                  RefAngle = Reflexion(HorAngle);

                  RefreshAngle(&InputNeutrons[i], RefAngle);
                  NormVector(InputNeutrons[i].Vector);

                  Dist3_a = DistVector(NeutronPosition_2, NeutronPosition_3_a);
                  tof3_b = NeutronPlaneIntersection1(&InputNeutrons[i], exitwall);

                  NeutronPosition_3_b[0] = InputNeutrons[i].Position[0];
                  NeutronPosition_3_b[1] = InputNeutrons[i].Position[1];
                  NeutronPosition_3_b[2] = InputNeutrons[i].Position[2];

                  Dist3_b = DistVector(NeutronPosition_3_a, NeutronPosition_3_b);
                  Dist_3 = Dist3_a + Dist3_b;
                  tof3 = tof3_a + tof3_b;
                }

                else if (HorAngle >= CritAngle)
                {
                  Dist_3 = Dist3_a;
                  tof3 = tof3_a;

                  if (InputNeutrons[i].Position[0] > BaseWidth / 2)
                  {
                    InputNeutrons[i].Position[0] = NeutronPosition_2[0];
                    InputNeutrons[i].Position[1] = NeutronPosition_2[1];
                  }

                  wall = 2;
                  goto Exit;
                }
              }

              else if (InputNeutrons[i].Position[2] < BaseHeight)
              {
                Dist_3 = DistVector(NeutronPosition_2, NeutronPosition_3);
                goto Exit;
              }
            }

            Exit:
            {
              Transmission = exp(-Attenuation * Dist_2);
              Dist = Dist_1 + Dist_2 + Dist_3; /* Total flight pass through the material */

              InputNeutrons[i].Time = InputNeutrons[i].Time + tof1 + tof2 + tof3;
              InputNeutrons[i].Probability = InputNeutrons[i].Probability * Transmission;

              if (wall != 2) /* regular case: the neutron will travell to the next prism */
              {
                InputNeutrons[i].Position[0] = 0;
              }

              if (wall == 2) /* The neutron travels to a prismarray below or up the actual one */
              {
                InputNeutrons[i].Position[2] = 0;
                ni = ni - 1;
                j = j - 1;
                wall = 1;
              }

              if (wall == -2)
              {
                InputNeutrons[i].Position[2] = BaseHeight;
                ni = ni + 1;
                j = j - 1;
                wall = 1;
              }

              /* Check wether the neutron is still inside the lense: */
              if (InputNeutrons[i].Position[1] >= PrismHeight / 2  ||
                  InputNeutrons[i].Position[1] <= -PrismHeight / 2 ||
                  ni > nRows                                 ||
                  ni < 1                                       )
              {
                NeutronIn = 0;
              }

              HorAngle = atan(InputNeutrons[i].Vector[2] / InputNeutrons[i].Vector[0]) * 180 / M_PI;
              HorAngleNew = (HorAngle) * M_PI / 180;
              RefreshAngle(&InputNeutrons[i], HorAngleNew);
              NormVector(InputNeutrons[i].Vector);
            }

            if (NeutronIn <= 0)
            {
              break;
            }
          }
          // end of cycle over the columns
        }

        /* Writing out the neutrons */
        if (InputNeutrons[i].Probability <= wei_min)
          continue;

        InputNeutrons[i].Position[0] = j * BaseWidth;
        InputNeutrons[i].Position[2] = InputNeutrons[i].Position[2] + nRows * BaseHeight / 2 - ni * BaseHeight;

        tof_0 = NeutronPlaneIntersection1(&InputNeutrons[i], EAexit);

        OutNeutron.Probability = InputNeutrons[i].Probability;
        OutNeutron.Time = InputNeutrons[i].Time;
        OutNeutron.Vector[0] = InputNeutrons[i].Vector[0];
        OutNeutron.Vector[1] = InputNeutrons[i].Vector[1];
        OutNeutron.Vector[2] = InputNeutrons[i].Vector[2];
        OutNeutron.Position[2] = InputNeutrons[i].Position[2];
        OutNeutron.Position[1] = InputNeutrons[i].Position[1];
        OutNeutron.Position[0] = 0;
        OutNeutron.Wavelength = InputNeutrons[i].Wavelength;

        WriteNeutron(&OutNeutron);
      }
      // end if (NeutronIn >= 1)
    }
    // end of cycle i over neutrons
  }
  // Finish: print parameters, write geometry and instrument file, free memory
  // -----------------------------------------------------
  my_exit:
  // write geometry data for visualization
  SetGeometry("yellow");

  // write into log file, free memory
  OwnCleanup();

  // print intensity, write instrument.inf   (see init.c)
  Cleanup(DistMove, 0.0, 0.0, 0.0, 0.0);

  return (0);
}
  /*******************************************************/
  /** Reads input parameters and sets global variables  **/
  /*******************************************************/

  // abcdefghijklmnopqrstuvwxyz
  //    D         N P  S
  //  b     h  k       s     yz
  void OwnInit(int argc, char *argv[])
  {
    int i = 0;

    InitPlane(&Endpoint);

    for (i = 1; i < argc; i++)
    {
      if (argv[i][0] != '+')
      {
        switch (argv[i][1])
        {
        case 'N':
          ScatteringLength = atof(&argv[i][2]);
          ScatteringLength *= 0.000001;
          break;

        case 'b':
          BaseWidth = atof(&argv[i][2]);
          break;

        case 'h':
          BaseHeight = atof(&argv[i][2]);
          break;

        case 'S':
          s_i = atof(&argv[i][2]);
          break;

        case 's':
          s_a = atof(&argv[i][2]);
          break;

        case 'D':
          rho = atof(&argv[i][2]);
          break;

        case 'z':
          PrismHeight = atof(&argv[i][2]);
          break;

        case 'k':
          nRows = atol(&argv[i][2]);
          break;

        case 'y':
          absorption = atol(&argv[i][2]);
          break;

        case 'P':
          nCols = atol(&argv[i][2]);
          break;

        default:
          Error("unknown command option");
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
    /* Calculation of all vectors and planes */
    UpperBorder[0] = 1;
    UpperBorder[1] = 0;
    UpperBorder[2] = 0;

    Prism[0] = BaseWidth / 2;
    Prism[1] = 0;
    Prism[2] = BaseHeight;

    Prism2[0] = -BaseWidth / 2;
    Prism2[1] = 0;
    Prism2[2] = BaseHeight;

    Prismhalf1.A = BaseHeight;
    Prismhalf1.B = 0;
    Prismhalf1.C = (-1) * BaseWidth / 2;
    Prismhalf1.D = 0;

    Prismhalf2.A = BaseHeight;
    Prismhalf2.B = 0;
    Prismhalf2.C = BaseWidth / 2;
    Prismhalf2.D = -BaseHeight * BaseWidth;

    UpperWall.A = 0;
    UpperWall.B = 0;
    UpperWall.C = 1;
    UpperWall.D = -BaseHeight;

    exitwall.A = 1;
    exitwall.B = 0;
    exitwall.C = 0;
    exitwall.D = -BaseWidth;

    EAexit.A = 1;
    EAexit.B = 0;
    EAexit.C = 0;
    EAexit.D = -(nCols + 1) * BaseWidth;

    entrancewall.A = -1;
    entrancewall.B = 0;
    entrancewall.C = 0;
    entrancewall.D = 0;

    DownWall.A = 0;
    DownWall.B = 1;
    DownWall.C = 0;
    DownWall.D = -1;

    EAHeight = nRows * BaseHeight;
  }




/*******************************************************/
/** Fills the structure stGeometry for visualization  **/
/*******************************************************/

void SetGeometry(char* sColor)
{
  /* Geometry data */
  if (bVisInstr)
  {
    int        nX=1, nZ=1,
               iX=0, iZ=0, iM=0;
    VectorType vOrient={0.0,0.0,1.0},
               vCntr  ={1.0,0.0,0.0};

    double gapX = 0, gapZ = 0;

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr   = sVisDescrpt;
    stGeometry.eModule  = _eModule;

    nX = nCols;
    nZ = nRows;
    stGeometry.nPrisms = nX * nZ;
    stGeometry.pPrism  = calloc(stGeometry.nPrisms, sizeof(VtPrism)); // Memory allocation for prisms

    for (iX = 1; iX <= nX; iX++)
    {
      for (iZ = 1; iZ <= nZ; iZ++)
      {
        // Set the center position of the prism
        vCntr[0]  = (2 * iX - 1) * BaseWidth / 2.0 ;
        vCntr[1]  = 0 ;
        vCntr[2] = (iZ - (nZ + 1) / 2.0) * BaseHeight ;

        // Small gaps between prisms
        gapX = BaseWidth / 10.0;
        gapZ = BaseHeight / 10.0;

        vCntr[0] += gapX;
        vCntr[2] += gapZ;

        // Prism Height
        stGeometry.pPrism[iM].PrismHeight = PrismHeight;

        // Bottom triangular face vertices
        stGeometry.pPrism[iM].vVertices[0][0] = vCntr[0] - BaseWidth / 2.0;
        stGeometry.pPrism[iM].vVertices[0][1] = vCntr[1] - PrismHeight / 2.0;
        stGeometry.pPrism[iM].vVertices[0][2] = vCntr[2] - BaseHeight / 2.0;

        stGeometry.pPrism[iM].vVertices[1][0] = vCntr[0] + BaseWidth / 2.0;
        stGeometry.pPrism[iM].vVertices[1][1] = vCntr[1] - PrismHeight / 2.0;
        stGeometry.pPrism[iM].vVertices[1][2] = vCntr[2] - BaseHeight / 2.0;

        stGeometry.pPrism[iM].vVertices[2][0] = vCntr[0];
        stGeometry.pPrism[iM].vVertices[2][1] = vCntr[1] - PrismHeight / 2.0;
        stGeometry.pPrism[iM].vVertices[2][2] = vCntr[2] + BaseHeight / 2.0;

        // Top triangular face vertices (offset along Y axis)
        stGeometry.pPrism[iM].vVertices[3][0] = stGeometry.pPrism[iM].vVertices[0][0];
        stGeometry.pPrism[iM].vVertices[3][1] = vCntr[1] + PrismHeight / 2.0;;
        stGeometry.pPrism[iM].vVertices[3][2] = stGeometry.pPrism[iM].vVertices[0][2];

        stGeometry.pPrism[iM].vVertices[4][0] = stGeometry.pPrism[iM].vVertices[1][0];
        stGeometry.pPrism[iM].vVertices[4][1] = vCntr[1] + PrismHeight / 2.0;;
        stGeometry.pPrism[iM].vVertices[4][2] = stGeometry.pPrism[iM].vVertices[1][2];

        stGeometry.pPrism[iM].vVertices[5][0] = stGeometry.pPrism[iM].vVertices[2][0];
        stGeometry.pPrism[iM].vVertices[5][1] = vCntr[1] + PrismHeight / 2.0;;
        stGeometry.pPrism[iM].vVertices[5][2] = stGeometry.pPrism[iM].vVertices[2][2];

        // Set the orientation (normal vector) for the prism
        stGeometry.pPrism[iM].vNormal[0] = vOrient[0];
        stGeometry.pPrism[iM].vNormal[1] = vOrient[1];
        stGeometry.pPrism[iM].vNormal[2] = vOrient[2];

        iM++;
      }
    }
  }
}


  /*******************************************************/
  /** Does module specific cleanup (free memory, etc.)  **/
  /*******************************************************/
  void OwnCleanup()
  {
    fprintf(LogFilePtr, "Dimensions of each prism along x, y, z : %6.4f x %6.4f x %6.4f cm \n", BaseWidth, PrismHeight, BaseHeight);
    fprintf(LogFilePtr, "The prisms matrix is made of %d columns and %d rows.\n", nRows, nCols);
    fprintf(LogFilePtr, "Critical angle : %6.2f\n", CritAngle);
    fprintf(LogFilePtr, "Neutron hits   : %d\n", n_hit);
    fprintf(LogFilePtr, "Neutron no-hits: %d\n", n_nohit);
  }

  /*******************************************************/
  /** Module specific functions  **/
  /*******************************************************/

  double Reflexion(double w) /* Calculation of the reflected angle */
  {
    double e1, w1, w3, w4;
    w1 = 90 - w;
    w3 = 180 - 2 * w1;
    w4 = w - w3;
    e1 = w4 * M_PI / 180;
    return e1;
  }

  double ReflexionPrism(double a1, double a) /* Reflection at the prisms surface */
  {
    double e1, a2, a4, a5;
    a2 = 90 - a1;
    a4 = 180 - 2 * a2;
    a5 = a + a4;
    e1 = a5 * M_PI / 180;
    return e1;
  }

  double Refraction1(double a1, double a) /* Refraction at the first prism side */
  {
    double e2, a2, a3, a4, a5;
    a2 = 90 - a1;
    a3 = asin(sin(a2 * M_PI / 180) / IndexRefraction) * 180 / M_PI;
    a4 = a3 - a2;

    if (a < -(90 - atan((BaseHeight / (BaseWidth / 2))) * 180 / M_PI))
    {
      a5 = a - sqrt(sq(a4));
    }

    else
    // if (a >= -(90 - atan((BaseHeight / (BaseWidth / 2))) * 180 / M_PI))
    {
      a5 = a + sqrt(sq(a4));
    }

    e2 = a5 * M_PI / 180;
    return e2;
  }

  double Refraction2(double w, double a) /* Refraction at the second prism side */
  {
    double e3, a12, a13, a14, a15;
    a12 = 90 - (180 - w);
    a13 = asin(IndexRefraction * sin(a12 * M_PI / 180)) * 180 / M_PI;
    a14 = a12 - a13;

    if (a >= 90 - (atan((BaseHeight / (BaseWidth / 2))) * 180 / M_PI))
    {
      a15 = a - sqrt(sq(a14));
    }

    else // if(a < 90 - (atan((BaseHeight / (BaseWidth / 2))) * 180 / M_PI))
    {
      a15 = a + sqrt(sq(a14));
    }

    e3 = a15 * M_PI / 180;
    return e3;
  }

  double RefractionSmallAngle(double w) /* Refraction at small angles */
  {
    double e4, w1, w2, w3, w4;
    w1 = 90 - w;
    w2 = asin(sqrt(sq(sin(w1 * M_PI / 180)) - sq(sin(CritAngle * M_PI / 180)))) *
         180 / M_PI / IndexRefraction;
    w3 = w2 - w1;
    w4 = w - sqrt(sq(w3));
    e4 = w4 * M_PI / 180;
    return e4;
  }

  double RefreshAngle(Neutron * ThisNeutron, double a6) /* Calculates the new angle */
  {
    ThisNeutron->Vector[0] = cos(a6);
    ThisNeutron->Vector[1] = ThisNeutron->Vector[1];
    ThisNeutron->Vector[2] = sin(a6);
    return a6;
  }
