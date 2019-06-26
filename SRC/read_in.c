/*********************************************************************************************/
/*  VITESS module  READ_IN                                                                   */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Jan  2013  K. Lieutenant   initial version                                           */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "in_out.h"
#include "softabort.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);
void  OwnCleanup();

short ScanVitessTraj(Neutron* pInNeutron, const char* sLine);
short ScanMcStasTraj(Neutron* pInNeutron, const char* sLine);
short ScanMcnpxTraj (Neutron* pInNeutron, const char* sLine);

short ConvertMcStas2Vitess(Neutron* pVitNeutron, const McNeutron* pMcNeutron);

void  RotMc2Vit  (VectorType* pVitVector, const VectorType* pMcVector);
void  InitNeutron(Neutron* pNeutron);
void  GetId      (TotalID* pID);


/******************************/
/** Global Variables    **/
/******************************/
FILE*        pInFile=NULL;              // pointer to input file
short        DetectColor=-1;            // ReadInt only neutrons with a given color, -1 means any
int          Nrep=1;                    // Number of times the input is read
double       FactInt=1.0;               // Factor to normmalize to the source intensity
VtPrgFormat  ePrgFormat=VT_VITESS_FMT;  // format of data to read (VITESS, McStas, MCNPX)
VtDataFormat eDatFormat=VT_FLOAT;       // output format (exponential, float)


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int         i,                // index of trajectories
              Irep=0,           // counter for number of repetitions
              rc=TRUE;          // return code of the function reading the input file (TRUE/FALSE)
  short       Nt=0;             // number of trajectories identified
  char        sLine[256];       // one line in input file
  Neutron     InNeutron;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("read_in 0.9");

  /* module specific initialization */
  OwnInit(argc, argv);
 
  for(i=0; i<1e14 && rc==TRUE; i++)
  {
    rc=ReadLine(pInFile, sLine, sizeof(sLine));
    if (rc==TRUE)
    { 
      switch (ePrgFormat)
      {
        case VT_VITESS_FMT: Nt=ScanVitessTraj(&InNeutron, sLine); break;
        case VT_MCSTAS_FMT: Nt=ScanMcStasTraj(&InNeutron, sLine); break;
        case VT_MCNPX_FMT : Nt=ScanMcnpxTraj (&InNeutron, sLine); break;
        default: Error("Data format for this program not (yet) implemented");
      }
      if (DetectColor < 0 || InNeutron.Color == DetectColor) 
      {    
        NumNeutRead += Nt;       
        WriteNeutron(&InNeutron);
      }
    }
    else
    { 
      Irep++;
      if (Irep < Nrep)
      { 
         rewind(pInFile);
         rc=TRUE;
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
  char *AsciiFileName=NULL;
  int i;

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
        case 'A':
          AsciiFileName = &argv[i][2];
          break;

        case 'f':
          ePrgFormat = (VtPrgFormat) atoi(&argv[i][2]);
          break;
        case 'F':
          eDatFormat = (VtDataFormat) atoi(&argv[i][2]);
          break;
        case 'C':
          DetectColor = (short) atoi(&argv[i][2]);
          break;
        case 'R':
          Nrep = atoi(&argv[i][2]);
          break;

        case 'I':
          FactInt = (double)atof(&argv[i][2]);
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  if (AsciiFileName != NULL)
  { if ((pInFile=fopen(FullParName(AsciiFileName),"rt"))==NULL) 
    { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", AsciiFileName);
      exit(-1);
    }
  } 
  else 
  { fputs("ERROR: The option -A to give the ascii file name is mandatory!\n", LogFilePtr);
    exit(-1);
  }
}


// -----------------------
// module specific cleanup
// -----------------------
void OwnCleanup()
{
  if (pInFile)
    fclose(pInFile);
}


// ----------------------
// Read VITESS trajectory
// ----------------------
short ScanVitessTraj(Neutron* pNeutron, const char* sLine)
{
  int     rc=0;
	char*   form=NULL;

  switch (eDatFormat)
  {
    case  VT_EXPONENTIAL: form = "%c%c%9lu %c %hd %le %le %le %le %le %le %le %le %le %le %le %le"; break;
    case  VT_FLOAT      : form = "%c%c%9lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf"; break;
    default             : Error("Data format not implemented");
  }

  rc=sscanf(sLine, form, &pNeutron->ID.IDGrp[0], &pNeutron->ID.IDGrp[1], &pNeutron->ID.IDNo, 
                         &pNeutron->Debug,       &pNeutron->Color, 
                         &pNeutron->Time,        &pNeutron->Wavelength,  &pNeutron->Probability, 
                         &pNeutron->Position[0], &pNeutron->Position[1], &pNeutron->Position[2], 
                         &pNeutron->Vector[0],   &pNeutron->Vector[1],   &pNeutron->Vector[2], 
                         &pNeutron->Spin[0],     &pNeutron->Spin[1],     &pNeutron->Spin[2]   ); 
  if (Nrep > 1)
    pNeutron->Probability /= (double)Nrep;       // reduction of weight if data are read more than once

  if (rc > 0)
    return(1);
  else
    return(0);
}

// -----------------------
//  Read McStas trajectory 
// -----------------------
short ScanMcStasTraj(Neutron* pNeutron, const char* sLine)
{
  McNeutron McNeut;
  short rc=FALSE, rs=0;

  rs=sscanf(sLine, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", 
                   &McNeut.Weight, 
                   &McNeut.Position[0], &McNeut.Position[1], &McNeut.Position[2], 
                   &McNeut.Speed[0],    &McNeut.Speed[1],    &McNeut.Speed[2], 
                   &McNeut.Time, 
                   &McNeut.Spin[0],     &McNeut.Spin[1],     &McNeut.Spin[2]    ); 
  if (rs > 0)
    rc= ConvertMcStas2Vitess(pNeutron, &McNeut);
  if (Nrep > 1)
    pNeutron->Probability /= (double)Nrep;         // reduction of weight if data are read more than once

  return(rc);
}


// -----------------------
//  Read MCNPX trajectory 
// -----------------------
short ScanMcnpxTraj(Neutron* pNeutron, const char* sLine)
{
  double energy;
  short rc=FALSE, rs=0;

	// initialization
  InitNeutron(pNeutron);			                      

  rs=sscanf(sLine, "%le %le %le %le %le %le %le %le %le", 
                   &pNeutron->Position[0], &pNeutron->Position[1], &pNeutron->Position[2], 
                   &pNeutron->Vector[0],   &pNeutron->Vector[1],   &pNeutron->Vector[2], 
                   &energy,                &pNeutron->Probability, &pNeutron->Time); 
  if (rs > 0)
  { 
    rc=TRUE;
    pNeutron->Wavelength  =  LAMBDA_FROM_ENERGY(1.0e+12*energy); // unit MeV -> µeV,  lambda -> energy
    pNeutron->Probability *= FactInt/Nrep;                       // normalisation counts -> n/s and reduction of weight if data are read more than once
    pNeutron->Time        *= 1.0e-05;                            // unit  shakes (=1.0e-08 s) -> ms
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
  InitNeutron(pVitNeutron);			                      

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


void RotMc2Vit(VectorType* pVitVector, const VectorType* pMcVector)
{
	(*pVitVector)[0] = (*pMcVector)[2];
	(*pVitVector)[1] = (*pMcVector)[0];
	(*pVitVector)[2] = (*pMcVector)[1];
}


void InitNeutron(Neutron* pNeutron)
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


