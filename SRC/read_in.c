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
void  OwnInit(int argc, char *argv[]);
void  OwnCleanup();

short ScanVitessTraj(Neutron* pInNeutron, const char* sLine, double Weight);
short ScanMcStasTraj(Neutron* pInNeutron, const char* sLine, double Weight);
short ScanMcnpxTraj (Neutron* pInNeutron, const char* sLine, double Weight);

short ReadMcplTraj  (Neutron* pInNeutron, double Weight);

short ConvertMcStas2Vitess(Neutron* pVitNeutron, const McNeutron*       pMcNeutron);
short ConvertMcpl2Vitess  (Neutron* pVitNeutron, const mcpl_particle_t* pMcplParticle);

void  RotMc2Vit  (VectorType* pVitVector, const VectorType* pMcVector);
void  InitMcNeutr(Neutron* pNeutron);
void  GetId      (TotalID* pID);

extern char* FullParName(const char* filename);     // this function should only be used exceptionally outside init.c


/******************************/
/** Global Variables    **/
/******************************/
McCompID     _eModule=MCN_READ_IN;

FILE*        pInFile[NF_MAX]={NULL,NULL,NULL}; // pointer to input file
mcpl_file_t  hInFile;                          // handle to MCPL input file
short        DetectColor=-1;                   // Flag: read only neutrons that are marked for 'trace'
int          Nrep=1;                           // Number of times the input is read
double       FactInt=1.0,                      // Factor to normalize to the source intensity from MCNPX data
             Weight[NF_MAX];                   // Weights of the input files
VtPrgFormat  ePrgFormat=VT_VITESS_FMT;         // format of data to read (VITESS, McStas, MCNPX)
// VtDataFormat eDatFormat=VT_FLOAT;           // output format (exponential, float)


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int             i,                // index of trajectories
                  m,                // index of input files
                  Irep=0,           // counter for number of repetitions
                  rc=TRUE;          // return code of the function reading the input file (TRUE/FALSE)
  short           Nt=0;             // number of trajectories identified
  char            sLine[256]="";    // one line in input file
  Neutron         InNeutron;

  /* Initialize the program according to the parameters given   */
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.3b");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
  	if (bVisInstr)
  { stGeometry.pDescr = "read_in:white";
  }
  
  if (__pTraceFileName!=NULL)
    fprintf(LogFilePtr, "trace file used              : %s\n", __pTraceFileName);

  if (ePrgFormat==VT_MCPL_FMT)
  {
    if (hInFile.internal)
    {  
      Irep=0;
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
          Irep++;
          if (Irep < Nrep)
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
        Irep=0;
        rc=TRUE;
        for(i=0; i<1e14 && rc==TRUE; i++)
        {
          rc=ReadLine(pInFile[m], sLine, sizeof(sLine));
          if (rc==TRUE)
          { 
            switch (ePrgFormat)
            {
              case VT_VITESS_FMT: Nt=ScanVitessTraj(&InNeutron, sLine, Weight[m]); 
                                  InNeutron.Debug = __eTraceMode==WRITE_TRC_FILES ? GetTraceState(InNeutron.ID) : 'N';
                                  if (__eTraceMode==ONLY_TRC_TRAJ && GetTraceState(InNeutron.ID)=='N' || DetectColor > -1 && InNeutron.Color!=DetectColor) 
                                    Nt=FALSE;                                        
                                  break;
              case VT_MCSTAS_FMT: Nt=ScanMcStasTraj(&InNeutron, sLine, Weight[m]); break;
              case VT_MCNPX_FMT : Nt=ScanMcnpxTraj (&InNeutron, sLine, Weight[m]); break;
              default: Error("Data format for this program not (yet) implemented");
            }
            if (Nt==TRUE) 
            {    
              NumNeutRead += Nt;       
              WriteNeutron(&InNeutron);
            }
          }
          else
          { 
            Irep++;
            if (Irep < Nrep)
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


// ------------------------------
// module specific initialization
// ------------------------------
void  OwnInit(int argc, char *argv[]) 
{
  char *AsciiFileName[NF_MAX]={NULL,NULL,NULL};
  int i,m;

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
        case 'A':
          AsciiFileName[0] = &argv[i][2];
          break;
        case 'B':
          AsciiFileName[1] = &argv[i][2];
          break;
        case 'D':
          AsciiFileName[2] = &argv[i][2];
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
          __eTraceMode = atoi(&argv[i][2]);
          break;
        case 'T':
          __pTraceFileName = &argv[i][2];  
          break;

        case 'f':
          ePrgFormat = (VtPrgFormat) atoi(&argv[i][2]);
          break;
        /* case 'F':
          eDatFormat = (VtDataFormat) atoi(&argv[i][2]);
          break; */
        case 'C':
          DetectColor = (short) atoi(&argv[i][2]);
          break;
        case 'R':
          Nrep = atoi(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (ePrgFormat== VT_MCPL_FMT)
  { 
    if (AsciiFileName[0] != NULL)
    { hInFile = mcpl_open_file(FullParName(AsciiFileName[0]));
      fprintf(LogFilePtr, mcpl_hdr_srcname(hInFile));       // Name of the generating application 
    }
    else 
    { Error("Input file 1 not given");
    }
    if (AsciiFileName[1] != NULL || AsciiFileName[2] != NULL)
      Warning("Input file 2 and 3 cannot be treated.");
   }
  else  
  { 
    for (m=0; m < NF_MAX; m++)
    { if (AsciiFileName[m] != NULL)
      { if ((pInFile[m] = OpenInputFile(AsciiFileName[m], FALSE, "rt"))==NULL) 
        { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", AsciiFileName[m]);
          exit(-1);
        }
      } 
    }
    if (pInFile[0]==NULL && pInFile[1]==NULL && pInFile[2]==NULL)
    { fputs("ERROR: At least one ascii input file name is mandatory!\n", LogFilePtr);
      exit(-1);
    }
  }

  if (__eTraceMode==ONLY_TRC_TRAJ && __pTraceFileName!=NULL)
    LoadTraceFile();
}


// -----------------------
// module specific cleanup
// -----------------------
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
}


// ----------------------
// Read VITESS trajectory
// ----------------------
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
    pNeutron->Probability *= (weight/Nrep);        // reduction of weight if data are read more than once or more than 1 file is read

  if (rc > 0)
    return(1);
  else
    return(0);
}

// -----------------------
//  Read McStas trajectory 
// -----------------------
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
     pNeutron->Probability *= (weight/Nrep);       // reduction of weight if data are read more than once or more than 1 file is read

  return(rc);
}


// -----------------------
//  Read MCPL trajectory 
// -----------------------
// rc: 1: neutron found
//     0: other particle
//    -1: EOF
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
      pNeutron->Probability *= (weight/Nrep);       // reduction of weight if data are read more than once or more than 1 file is read
  }

  return(rc);
}


// -----------------------
//  Read MCNPX trajectory 
// -----------------------
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
    pNeutron->Probability = McnpNeutr.Counts * FactInt * weight/Nrep;       // normalisation counts -> n/s and reduction of weight if data are read more than once or more than 1 file is read
    pNeutron->Time        = McnpNeutr.Shakes * 1.0e-05;                     // unit  shakes (=1.0e-08 s) -> ms

    rc=TRUE;
  }

  return(rc);
}


// ------------------------------------
//  Convert McStas to VITESS trajectory 
// ------------------------------------
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

// ------------------------------------
//  Convert MCPL to VITESS trajectory 
// ------------------------------------
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


void RotMc2Vit(VectorType* pVitVector, const VectorType* pMcVector)
{
	(*pVitVector)[0] = (*pMcVector)[2];
	(*pVitVector)[1] = (*pMcVector)[0];
	(*pVitVector)[2] = (*pMcVector)[1];
}


void InitMcNeutr(Neutron* pNeutron)
{
	memset(pNeutron, '\0', sizeof(Neutron));        

	GetId(&pNeutron->ID);
	pNeutron->Debug ='N';                             // no debugging
	pNeutron->Color = 0;                              // no color
}


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


