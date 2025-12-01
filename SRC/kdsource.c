/*********************************************************************************************/
/*  VITESS module 'read-in'                                                                  */
/*                                                                                           */
/* This module reads neutron events (trajectories) from files in different formats           */
/*   (so it replaces a source module)                                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Apr  2025  N. Schmidt      KDsource functionality in read_in                         */
/* 1.1  Nov  2025  F. Beule        Move KDSource to a separate module                        */
/*********************************************************************************************/

#include "kdsource.h"
#include "general.h"
#include "init.h"
#include "softabort.h"
#include <stdio.h>
#include <stdlib.h>

#define NF_MAX 3
#define MAX_HEADER 3000


/******************************/
/** Prototypes               **/
/******************************/
void OwnInit(int argc, char *argv[]); // Reads input parameters and sets global variables
void OwnCleanup();                    // Does module specific cleanup

static short ReadKDSTraj(Neutron *pNeutron, KDSource *pFile, int perturb, double wcrit); // Reads KDSource file
static short ConvertMcpl2Vitess(Neutron *pVitNeut,
                                const mcpl_particle_t *pMcplParticle); // Converts MCPL to VITESS trajectory
static void InitMcNeutr(Neutron *pNeutron);                            // Initializes a trajectory
static void GetId(TotalID *pID);                                       // Creates next ID for a trajectory


/******************************/
/** Global Variables    **/
/******************************/
// Input parameters
char const *sInputFileName = NULL; // -A  name of the input files
double maxEv = -1;                 // -M  maximal numver of events read, default: all events from the MCPL file
int use_kde = 1;                   // -K  Whether to use KDE or just read the particles from the MCPL file refered in the xml file

// Variables determined from input parameters or trajectory data
KDSource *hKDSFile; // handle to KDSource input file


/******************************************/
/**  Main Program                        **/
/******************************************/
int main(int argc, char **argv)
{
  short rc = TRUE;     // return code of the function reading the input file (TRUE/FALSE)
  double w_crit = 0.0; // weight to use KDE while calling KDSource
  Neutron InNeutron;

  // Initialisation
  // --------------
  _eModule = MCN_READ_IN;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.1");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp = FALSE;

  InitNeutron(&InNeutron);
  NumNeutRead = 0.0;

  // loop over trajectories
  // ----------------------
  rc = TRUE;
  w_crit = use_kde ? KDS_w_mean(hKDSFile, 1000, NULL) : -1;
  while (rc != VT_EOF && NumNeutRead < maxEv) {
    rc = ReadKDSTraj(&InNeutron, hKDSFile, use_kde, w_crit);
    if (rc) {
      NumNeutRead += rc;
      WriteNeutron(&InNeutron);
      if (NumNeutRead <= 10)
        fprintf(LogFilePtr, "%lu\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\t%.3e\n", InNeutron.ID.IDNo,
                InNeutron.Wavelength, InNeutron.Position[0], InNeutron.Position[1], InNeutron.Position[2],
                InNeutron.Vector[0], InNeutron.Vector[1], InNeutron.Vector[2], InNeutron.Time, InNeutron.Probability);
    }
  }

  // Do module specific cleanups
  OwnCleanup();

  // Do the general cleanup
  Cleanup(0.0, 0.0, 0.0, 0.0, 0.0);
  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] != '+') {
      switch (argv[i][1]) {
      case 'A':
        sInputFileName = &argv[i][2];
        break;
      case 'M':
        maxEv = atof(&argv[i][2]);
        break;
      case 'K':
        use_kde = atoi(&argv[i][2]);
        break;
      default:
        fprintf(LogFilePtr, "ERROR: unkown command option: %s\n", argv[i]);
        exit(-1);
      }
    }
  }

  // open input file(s)
  char sFullInputName[CHAR_BUF_SMALL];
  TotalPath(sFullInputName, sInputFileName, "", IN_DIR);
  KDS_setlogfile(LogFilePtr);
  hKDSFile = KDS_open(sFullInputName);
  if (maxEv < 0) {
    maxEv = hKDSFile->plist->npts;
  }
}


/******************************************/
/**  Does module specific cleanup        **/
/******************************************/
void OwnCleanup()
{
  KDS_destroy(hKDSFile);
}

short ReadKDSTraj(Neutron *pNeutron, KDSource *pfile, int perturb, double wcrit)
{
  mcpl_particle_t part;

  KDS_sample2(pfile, &part, perturb, wcrit, NULL, 1);

  return ConvertMcpl2Vitess(pNeutron, &part);
}

short ConvertMcpl2Vitess(Neutron *pVitNeutron, const mcpl_particle_t *pMcplParticle)
{
  if (pMcplParticle->pdgcode == NEUTRON_ID) {
    // initialization
    InitMcNeutr(pVitNeutron);

    pVitNeutron->Wavelength = LAMBDA_FROM_ENERGY(pMcplParticle->ekin * 1.0e12); // MeV -> microeV
    pVitNeutron->Time = pMcplParticle->time;
    pVitNeutron->Probability = pMcplParticle->weight;

    CopyVector(&pMcplParticle->position[0], &pVitNeutron->Position[0]);
    CopyVector(&pMcplParticle->direction[0], &pVitNeutron->Vector[0]);
    CopyVector(&pMcplParticle->polarisation[0], &pVitNeutron->Spin[0]);

    return (TRUE);
  } else {
    return (FALSE);
  }
}


/******************************************/
/**  Initializes a trajectory            **/
/******************************************/
void InitMcNeutr(Neutron *pNeutron)
{
  InitNeutron(pNeutron);
  GetId(&pNeutron->ID);
  pNeutron->Debug = 'N'; // no debugging
  pNeutron->Color = 0;   // no color
}


/******************************************/
/**  Creates next ID for a trajectory    **/
/******************************************/
void GetId(TotalID *pID)
{
  static unsigned long ig = 0;
  static char ig1 = 'A', ig2 = 'A';

  if (ig == 4294967295U) {
    ig = 0;
    if (ig2 == 'Z') {
      ig2 = 'A';
      ig1++;
    } else {
      ig2++;
    }
  } else {
    ig++;
  }
  pID->IDGrp[0] = ig1;
  pID->IDGrp[1] = ig2;
  pID->IDNo = ig;
}
