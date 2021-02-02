/*********************************************************************************************/
/*  VITESS module 'read-in'                                                                  */
/*                                                                                           */
/* This module reads neutron events (trajectories) from files in different formats           */
/*   (so it replaces a source module)                                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 0.9  Jan  2013  K. Lieutenant   initial version                                           */
/* 1.0  Aug  2013  K. Lieutenant   correction read format %09lu -> %lu                       */
/* 1.1  Sep  2013  K. Lieutenant   several input files                                       */
/* 1.1a Apr  2014  K. Lieutenant   repetition corrected                                      */
/* 1.2  Apr  2018  K. Lieutenant   MCPL and MCNP format                                      */
/* 1.3  May  2019  K. Lieutenant   option to read only trajectories marked for tracing       */
/* 1.3a Jul  2019  K. Lieutenant   MCNPX format uses its own structure                       */
/* 1.3b Jul  2019  K. Lieutenant   smart trajectory search algorithm only for long lists     */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "in_out.h"
#include "softabort.h"
#include "mcpl.h"
#include "trace.h"

#define NF_MAX 3


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);                                           // Reads input parameters and sets global variables
void  OwnCleanup();                                                              // Does module specific cleanup

short ScanVitessTraj(Neutron* pInNeutron, const char* sLine, double Weight);     // Reads VITESS trajectory 
short ScanMcStasTraj(Neutron* pInNeutron, const char* sLine, double Weight);     // Reads McStas trajectory 
short ScanMcnpxTraj (Neutron* pInNeutron, const char* sLine, double Weight);     // Reads MCNPX trajectory 
short ReadMcplTraj  (Neutron* pInNeutron, double Weight);                        // Reads MCPL trajectory 

short ConvertMcStas2Vitess(Neutron* pVitNeut, const McNeutron*       pMcNeut);   // Converts McStas to VITESS trajectory 
short ConvertMcpl2Vitess  (Neutron* pVitNeut, const mcpl_particle_t* pMcplPtcl); // Converts MCPL to VITESS trajectory 

void  RotMc2Vit  (VectorType* pVitVector, const VectorType* pMcVector);          // Vector transfer from McStas to VITESS co-ordinate system 
void  InitMcNeutr(Neutron* pNeutron);                                            // Initializes a trajectory
void  GetId      (TotalID* pID);                                                 // Creates next ID for a trajectory 

extern char* FullParName(const char* filename);                                  // function in init.c, adds parameter directory to file name 


/******************************/
/** Global Variables    **/
/******************************/
McCompID     _eModule=MCN_READ_IN;
 
// Input parameters
char*        sInputFileName[NF_MAX];     // -A -B -D  names of the NF_MAX input files
short        iDetectColor=-1;            // -C        Only for VITESS format: Read only events with a given color.
int          nRep=1;                     // -R        Number of times the input is read
double       FactInt=1.0,                // -I        Factor to normalize to the source intensity from MCNPX data
             Weight[NF_MAX];             // -a -b -d  Weights of the input files
VtPrgFormat  ePrgFormat=VT_VITESS_ASC;   // -f        format of data to read (VT_VITESS_FMT: VITESS ASCII format   VT_MCSTAS_FMT: McStas   VT_MCPL_FMT: MCPL   VT_MCNP_FMT: MCNP   VT_VITESS_BIN: VITESS binary format)

extern char* sInstrInfIn;                // --I       instrument file that is read (default 'instrument.inf') 
extern char* _sTraceFileName;            // -T        name of the file containing the trajectories to be traced or started
extern short _eTraceMode;                // -t        NO_TRACING     : no tracing 
                                         //           WRITE_TRC_FILES: write trace files for traj. of interest
                                         //           ONLY_TRC_TRAJ  : simulation only with traj. of interest 

// Variables determined from input parameters or trajectory data
FILE*        pInFile[NF_MAX];            //           pointer to input file
mcpl_file_t  hInFile;                    //           handle to MCPL input file


/******************************************/
/**  Main Program                        **/
/******************************************/
int main(int argc, char **argv)
{
  int             i=0,              // index of trajectories
                  m=0,              // index of input files
                  iRep=0,           // counter for number of repetitions
                  rc=TRUE;          // return code of the function reading the input file (TRUE/FALSE)
  short           Nt=0;             // number of trajectories identified
  char            sLine[256]="";    // one line in input file
  Neutron         InNeutron;

  // Initialisation
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3b");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
  // if (bVisInstr) stGeometry.pDescr = "read_in:white";
  InitNeutron(&InNeutron);

  // loop over trajectories
  // ----------------------
	if (ePrgFormat==VT_MCPL_FMT)
  {
    if (hInFile.internal)
    {  
      iRep=0;
      rc=TRUE;
      while (rc != VT_EOF)
      {
        rc=ReadMcplTraj(&InNeutron, 1.0);
        if (rc==TRUE) 
        {    
          NumNeutRead += rc;       
          WriteNeutron(&InNeutron);
        }
        else if (rc==VT_EOF)
        {
          iRep++;
          if (iRep < nRep)
          { mcpl_rewind(hInFile);
            rc=TRUE;
          }
        }
      }
    }
  }
  else 
  { for (m=0; m < NF_MAX; m++)
    { 
      if (pInFile[m])
      {  
        iRep=0;
        rc=TRUE;
        for(i=0; i<1e14 && rc==TRUE; i++)
        {
          rc=ReadLine(pInFile[m], sLine, sizeof(sLine));
          if (rc==TRUE)
          { 
            switch (ePrgFormat)
            {
              case VT_VITESS_ASC: Nt=ScanVitessTraj(&InNeutron, sLine, Weight[m]); 
                                  InNeutron.Debug = _eTraceMode==WRITE_TRC_FILES ? GetTraceState(InNeutron.ID) : 'N';
                                  if (_eTraceMode==ONLY_TRC_TRAJ && GetTraceState(InNeutron.ID)=='N' || iDetectColor > -1 && InNeutron.Color!=iDetectColor) 
                                    Nt=0;                                        
                                  break;
              case VT_MCSTAS_FMT: Nt=ScanMcStasTraj(&InNeutron, sLine, Weight[m]); break;
              case VT_MCNP_FMT  : Nt=ScanMcnpxTraj (&InNeutron, sLine, Weight[m]); break;
              default: Error("Data format for this program not (yet) implemented");
            }
            if (Nt>=1) 
            {    
              NumNeutRead += Nt;       
              WriteNeutron(&InNeutron);
            }
          }
          else
          { 
            iRep++;
            if (iRep < nRep)
            { rewind(pInFile[m]);
              rc=TRUE;
            }
          }
        }
      }
    }
  }

  // Do module specific cleanups
  OwnCleanup();
  
  // Do the general cleanup
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void OwnInit(int argc, char *argv[]) 
{
  int i=0,m=0;

  for (m=0; m < NF_MAX; m++)
  { 
    Weight [m]=0.0;
    pInFile[m]=NULL;
    sInputFileName[m]=NULL;
  }

  for (i=1; i<argc; i++) 
  { 
    if (argv[i][0]!='+') 
    { 
      switch(argv[i][1])
      { 
        case 'A':
          sInputFileName[0] = &argv[i][2];
          break;
        case 'B':
          sInputFileName[1] = &argv[i][2];
          break;
        case 'D':
          sInputFileName[2] = &argv[i][2];
          break;

        case 'a':
          Weight[0] = (double)atof(&argv[i][2]);
          break;
        case 'b':
          Weight[1] = (double)atof(&argv[i][2]);
          break;
        case 'd':
          Weight[2] = (double)atof(&argv[i][2]);
          break;
        case 'I':
          FactInt =   (double)atof(&argv[i][2]);
          break;
        case 't':
          _eTraceMode = atoi(&argv[i][2]);
          break;
        case 'T':
          _sTraceFileName = &argv[i][2];  
          break;

        case 'f':
          ePrgFormat = (VtPrgFormat) atoi(&argv[i][2]);
          break;
        /* case 'F':
          eDatFormat = (VtDataFormat) atoi(&argv[i][2]);
          break; */
        case 'C':
          iDetectColor = (short) atoi(&argv[i][2]);
          break;
        case 'R':
          nRep = atoi(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (ePrgFormat== VT_MCPL_FMT)
  { 
    if (sInputFileName[0] != NULL)
    { hInFile = mcpl_open_file(FullParName(sInputFileName[0]));
      fprintf(LogFilePtr, mcpl_hdr_srcname(hInFile));       // Name of the generating application 
    }
    else 
    { Error("Input file 1 not given");
    }
    if (sInputFileName[1] != NULL || sInputFileName[2] != NULL)
      Warning("Input file 2 and 3 cannot be treated.");
   }
  else  
  { 
    for (m=0; m < NF_MAX; m++)
    { if (sInputFileName[m] != NULL)
      { if ((pInFile[m] = OpenInputFile(sInputFileName[m], FALSE, "rt"))==NULL) 
        { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", sInputFileName[m]);
          exit(-1);
        }
        else
        { fprintf(LogFilePtr,"Input file %s used with weight %7.5f\n", sInputFileName[m], Weight[m]);
        } 
      } 
    }
    if (pInFile[0]==NULL && pInFile[1]==NULL && pInFile[2]==NULL)
    { fputs("ERROR: At least one ascii input file name is mandatory!\n", LogFilePtr);
      exit(-1);
    }
  }

  if (_eTraceMode==ONLY_TRC_TRAJ && _sTraceFileName!=NULL)
    LoadTraceFile();
  
  if (_sTraceFileName!=NULL)
    fprintf(LogFilePtr, "trace file used              : %s\n", _sTraceFileName);
}


/******************************************/
/**  Does module specific cleanup        **/
/******************************************/
void OwnCleanup()
{
  int m;

  if (ePrgFormat== VT_MCPL_FMT)
  { // Deallocate memory and release file-handle
    mcpl_close_file(hInFile);
  }
  else  
  { // Close all files
    for (m=0; m < NF_MAX; m++)
      if (pInFile[m])
        fclose(pInFile[m]);
  }

  if (_aTrace!=NULL) free(_aTrace);
}


/******************************************/
/**  Reads VITESS trajectory             **/
/******************************************/
short ScanVitessTraj(Neutron* pNeutron, const char* sLine, double weight)
{
  int    rc=0;
	char*  pForm="%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf";

  /* switch (eDatFormat)
  {
    case  VT_EXPONENTIAL: form = "%c%c%lu %c %hd %le %le %le %le %le %le %le %le %le %le %le %le"; break;
    case  VT_FLOAT      : form = "%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf"; break;
    default             : Error("Data format not implemented");
  }*/

  rc=sscanf(sLine, pForm, &pNeutron->ID.IDGrp[0], &pNeutron->ID.IDGrp[1], &pNeutron->ID.IDNo, 
                          &pNeutron->Debug,       &pNeutron->Color, 
                          &pNeutron->Time,        &pNeutron->Wavelength,  &pNeutron->Probability, 
                          &pNeutron->Position[0], &pNeutron->Position[1], &pNeutron->Position[2], 
                          &pNeutron->Vector[0],   &pNeutron->Vector[1],   &pNeutron->Vector[2], 
                          &pNeutron->Spin[0],     &pNeutron->Spin[1],     &pNeutron->Spin[2]   ); 
  if (rc)
    pNeutron->Probability *= (weight/nRep);        // reduction of weight if data are read more than once or more than 1 file is read

  if (rc > 0)
    return(1);
  else
    return(0);
}

/******************************************/
/**  Reads McStas trajectory             **/
/******************************************/
short ScanMcStasTraj(Neutron* pNeutron, const char* sLine, double weight)
{
  McNeutron McNeutr;
  short rc=FALSE, rs=0;

  rs=sscanf(sLine, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
                   &McNeutr.Weight, 
                   &McNeutr.Position[0], &McNeutr.Position[1], &McNeutr.Position[2], 
                   &McNeutr.Speed[0],    &McNeutr.Speed[1],    &McNeutr.Speed[2], 
                   &McNeutr.Time, 
                   &McNeutr.Spin[0],     &McNeutr.Spin[1],     &McNeutr.Spin[2]    ); 
  if (rs > 0)
    rc= ConvertMcStas2Vitess(pNeutron, &McNeutr);
  if (rc)
     pNeutron->Probability *= (weight/nRep);       // reduction of weight if data are read more than once or more than 1 file is read

  return(rc);
}


/******************************************/
/**  Reads MCPL trajectory               **/
/******************************************
** rc: 1: neutron found
**     0: other particle
**    -1: EOF
*******************************************/
short ReadMcplTraj(Neutron* pNeutron, double weight)
{
  const mcpl_particle_t* pMcplPtcl;
  short rc=FALSE;

  pMcplPtcl = mcpl_read(hInFile);

  if (pMcplPtcl==NULL)
  { return VT_EOF;
  }
  else
  { rc= ConvertMcpl2Vitess(pNeutron, pMcplPtcl);
    if (rc)
      pNeutron->Probability *= (weight/nRep);       // reduction of weight if data are read more than once or more than 1 file is read
  }

  return(rc);
}


/******************************************/
/**  Read MCNPX trajectory               **/
/******************************************/
short ScanMcnpxTraj(Neutron* pNeutron, const char* sLine, double weight)
{
  McnpNeutron McnpNeutr;
  short       rc=FALSE, rs=0;

  rs=sscanf(sLine, "%le %le %le %le %le %le %le %le %le", 
                   &McnpNeutr.Position[0], &McnpNeutr.Position[1], &McnpNeutr.Position[2], 
                   &McnpNeutr.Vector[0],   &McnpNeutr.Vector[1],   &McnpNeutr.Vector[2], 
                   &McnpNeutr.Energy,      &McnpNeutr.Counts,      &McnpNeutr.Shakes); 

  if (rs > 0)
  { 
  	// initialization
    InitMcNeutr(pNeutron);			                      

    CopyVector(McnpNeutr.Position, pNeutron->Position);
    CopyVector(McnpNeutr.Vector,   pNeutron->Vector);
    
    pNeutron->Wavelength  = LAMBDA_FROM_ENERGY(1.0e+12 * McnpNeutr.Energy); // unit MeV -> µeV,  lambda -> energy
    pNeutron->Probability = McnpNeutr.Counts * FactInt * weight/nRep;       // normalisation counts -> n/s and reduction of weight if data are read more than once or more than 1 file is read
    pNeutron->Time        = McnpNeutr.Shakes * 1.0e-05;                     // unit  shakes (=1.0e-08 s) -> ms

    rc=TRUE;
  }

  return(rc);
}


/******************************************/
/** Converts McStas to VITESS trajectory **/
/******************************************/
short ConvertMcStas2Vitess(Neutron* pVitNeutron, const McNeutron* pMcNeutron)
{
	double  velocity;      // velocity of the neutron  [cm/ms]

	// initialization
  InitMcNeutr(pVitNeutron);			                      

	velocity = 0.1 * sqrt(  sq(pMcNeutron->Speed[0])     // unit m/s -> cm/ms
			                  + sq(pMcNeutron->Speed[1])
			                  + sq(pMcNeutron->Speed[2]));
	pVitNeutron->Wavelength  = LAMBDA_FROM_V(velocity);   
	pVitNeutron->Time        = pMcNeutron->Time*1000.0;  // unit s -> ms
	pVitNeutron->Probability = pMcNeutron->Weight;

	RotMc2Vit(&pVitNeutron->Position, &pMcNeutron->Position);
	RotMc2Vit(&pVitNeutron->Vector,   &pMcNeutron->Speed);
	RotMc2Vit(&pVitNeutron->Spin,     &pMcNeutron->Spin);

	MultiplyByScalar(pVitNeutron->Position, 100.0);      // unit   m -> cm
	NormVector      (pVitNeutron->Vector);               // velocity -> direction

  return(TRUE);
}

/******************************************/
/**  Converts MCPL to VITESS trajectory  **/
/******************************************/
short ConvertMcpl2Vitess(Neutron* pVitNeutron, const mcpl_particle_t* pMcplParticle)
{
  if (pMcplParticle->pdgcode==NEUTRON_ID)                    
  {
  	// initialization
    InitMcNeutr(pVitNeutron);			  

	  pVitNeutron->Wavelength  = LAMBDA_FROM_ENERGY(pMcplParticle->ekin*1.0e12);    // MeV -> µeV   
	  pVitNeutron->Time        = pMcplParticle->time; 
	  pVitNeutron->Probability = pMcplParticle->weight;

	  RotMc2Vit(&pVitNeutron->Position, &pMcplParticle->position);
	  RotMc2Vit(&pVitNeutron->Vector,   &pMcplParticle->direction);
	  RotMc2Vit(&pVitNeutron->Spin,     &pMcplParticle->polarisation);

    return(TRUE);
  }
  else
  { return(FALSE);
  }
}


/****************************************************************/
/**  Vector transfer from McStas to VITESS co-ordinate system  **/
/****************************************************************/
void RotMc2Vit(VectorType* pVitVector, const VectorType* pMcVector)
{
	(*pVitVector)[0] = (*pMcVector)[2];
	(*pVitVector)[1] = (*pMcVector)[0];
	(*pVitVector)[2] = (*pMcVector)[1];
}


/******************************************/
/**  Initializes a trajectory            **/
/******************************************/
void InitMcNeutr(Neutron* pNeutron)
{
	memset(pNeutron, '\0', sizeof(Neutron));        

	GetId(&pNeutron->ID);
	pNeutron->Debug ='N';                             // no debugging
	pNeutron->Color = 0;                              // no color
}


/******************************************/
/**  Creates next ID for a trajectory    **/
/******************************************/
void GetId(TotalID* pID)
{
  static unsigned long ig=0;
  static char          ig1='A', ig2='A';

	if (ig==4294967295U)
	{	ig=0; 
		if (ig2=='Z')
		{	ig2='A'; ig1++;
		}
	   else
		{	ig2++;
		}
	}
	else
	{	ig++;
	}
	pID->IDGrp[0] = ig1;
	pID->IDGrp[1] = ig2;
	pID->IDNo     = ig;
}


