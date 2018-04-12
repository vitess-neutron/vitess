/*********************************************************************************************/
/*  VITESS module  WRITEOUT                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  June 1999  ???             initial version                                           */
/* 1.1  Mar  2001  K. Lieutenant   headline                                                  */	
/* 1.2  Jan  2004  K. Lieutenant   changes for 'instrument.dat' and changed headline         */
/* 1.3  Feb  2004  K. Lieutenant   'FullParName' and 'ERROR' included                        */
/* 1.4  Mar  2004  K. Lieutenant   F-Format Option                                           */
/* 1.4e Jul  2005  M. Fromme       headline, simplification                                  */
/* 1.4f Jan  2010  A. Houben       WriteOut only if given color matches Neutron color        */
/* 1.4g Feb  2010  A. Houben       Added wavelength, Div and yz position filter              */
/* 1.4h Feb  2010  K. Lieutenant   colour = 0 means all                                      */
/* 1.4i Jul  2011  A. Houben       colour = -1 means all; colour = 0 means only untaged      */
/*                                 neutrons by previous modules                              */
/* 1.4j Aug  2011  A. Houben       extended divergence filters                               */
/* 1.5  May  2012  A. Houben       select output columns (reduces file size for long simul.) */
/*      Aug  2012  M. Fromme       clean up                                                  */
/* 1.6  Jan  2013  K. Lieutenant   tidy up; McStas and MCNPX format                          */
/* 1.7  Apr  2018  K. Lieutenant   MCPL format                                               */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "in_out.h"
#include "softabort.h"
#include "mcpl.h"


/******************************/
/** Definitions and Enums    **/
/******************************/
#define SP(var, form) if (csep) strcpy(var,sep); strncat(var, form, 13)
#define FP(s) if (csep++) fputs(sep, pOutFile); fputs(s, pOutFile)

#define cID       0
#define cTrc      1
#define cColor    2
#define cTOF      3
#define cLambda   4
#define cCounts   5
#define cPosX     6
#define cPosY     7
#define cPosZ     8
#define cDirX     9
#define cDirY    10
#define cDirZ    11
#define cSpinX   12
#define cSpinY   13
#define cSpinZ   14


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);
void  OwnCleanup();
void  SetFormatsAndHeader(int iSsep, const char *sSep);
short CalcDivergence(double *pFullDiv, double *pHorDiv, double *pVertDiv, const VectorType Direction);

short McStasParameters();
short MCNPXParameters();
short ConvertVitess2McStas(Neutron*         pMcNeutron,   const Neutron* pVitNeutron);
short ConvertVitess2MCNPX (Neutron*         pMcnpNeutron, const Neutron* pVitNeutron);
short ConvertVitess2MCPL  (mcpl_particle_t* pMCPLNeutron, const Neutron* pVitNeutron);

void  RotVit2Mc(VectorType* pMcVector, const VectorType* pVitVector);


/******************************/
/** Global Variables    **/
/******************************/
FILE*          pOutFile; // pointer to output file
mcpl_outfile_t hOutFile; // handle to output file for MCPL format

char form[15][15];    // formats to print data of the different parameters using VITESS
char* outform;        // format for the whole line using McStas or MCNPX
char* header;         // header: parameters of the event file
char* units;          // header: units used in the event file

VtPrgFormat  ePrgFormat=VT_VITESS_FMT; // output format (VITESS, McStas, MCNPX)
VtDataFormat eDatFormat=VT_FLOAT;      // output format (exponential, float)
VtSeparator  eSeparator=VT_BLANK;      // separator between columns (space, tab)

short  DetectColor  = 0;    // WriteOut only neutrons with a given color, -1 means any
int    calcDivY = 0,        // boolean: calculation of hor. divergence 
       calcDivZ = 0;        //                      or vert. divergence necessary

short bF_cID=TRUE,
      bF_cTrc=TRUE,
      bF_cColor=TRUE,
      bF_cTOF=TRUE,
      bF_cLambda=TRUE,
      bF_cCounts=TRUE,
      bF_cPosition=TRUE,
      bF_cDirection=TRUE,
      bF_cSpin=TRUE,
      bF_Active=TRUE;

double filtLambdaMin=-1.0,   // filter
       filtLambdaMax=-1.0,
       filtYMin     =-1.0e10,
       filtYMax     = 1.0e10,
       filtZMin     =-1.0e10,
       filtZMax     = 1.0e10,
       filtYDivMin  =-1.0,
       filtZDivMin  =-1.0,
       filtDivMin   =-1.0,
       filtYDivMax  =-1.0,
       filtZDivMax  =-1.0,
       filtDivMax   =-1.0;


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  int             i,                // index of trajectories
                  csep;             // index of formats (useless)
  const char     *sep;              // separator
  double          Divy, Divz, Div;  // divergence of actual trajectory
  Neutron         OutNeutron;
  mcpl_particle_t OutParticle;

  Divy = Divz = Div = 0.0;

  // Initialize the program according to the parameters given 
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("writeout 1.7");

  // module specific initialization 
  OwnInit(argc, argv);
  
  if (eSeparator==VT_TABULATOR) 
    sep = "\t"; 
  else if (eSeparator==VT_BLANK) 
    sep = " ";
  else
    Error("Separator has unknown value");
  csep = 0;

  // define format for variables in output file and print headline
  if (bF_Active)
  {
    switch (ePrgFormat)
    { case VT_MCSTAS_FMT:
        header  = "#     weight        pos_x     pos_y      pos_z     speed_x   speed_y   speed_z     TOF       S_x  S_y  S_z \n";  
        units   = "#      [n/s]         [m]       [m]        [m]       [m/s]     [m/s]     [m/s]      [s]       [1]  [1]  [1] \n";  
        outform = "%15.3f  %9.6f %9.6f %10.6f  %9.2f %9.2f %9.2f  %10.8f  %4.1f %4.1f %4.1f";
	      fprintf(pOutFile,"#Trajectories writeout_McStas \n");
        fprintf(pOutFile, "%s%s", header, units);
        break;
      case VT_MCPL_FMT:
        mcpl_hdr_set_srcname    (hOutFile, "VITESS 3.4  module writeout 1.7"); /* Name of the generating application         */
        mcpl_hdr_add_comment    (hOutFile, "first test");                      /* Add one or more human-readable comments    */
        mcpl_enable_polarisation(hOutFile);                                    /* to write the "polarisation" info           */
        break;
      case VT_MCNPX_FMT:
        header  = "#    pos_x          pos_y          pos_z          dir_x          dir_y          dir_z            E           weight          time  \n";  
        units   = "#     [cm]           [cm]           [cm]           [1]            [1]            [1]           [MeV]           [1]         [1e-8s] \n";  
        outform = "%14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e";
	      fprintf(pOutFile,"#Trajectories writeout_MCNPX \n");
        fprintf(pOutFile, "%s%s", header, units);
        break;
      default:   // nothing to do for VITESS
	      fprintf(pOutFile, "#Trajectories writeout_Vitess \n");
        SetFormatsAndHeader(csep, sep);
    }
  }
 
  // Get the neutrons from file
  DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
  {
    CHECK;    
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

      // write all trajectories to pipe
	    WriteNeutron(&(InputNeutrons[i]));

      // skip the rest if the module is not active 
	    if (!bF_Active) continue;

	    // Filter wavelength and position
      if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	    if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;

	    if (InputNeutrons[i].Position[1] < filtYMin) continue;
	    if (InputNeutrons[i].Position[1] > filtYMax) continue;
	    if (InputNeutrons[i].Position[2] < filtZMin) continue;
	    if (InputNeutrons[i].Position[2] > filtZMax) continue;
	  
      // filter divergence
      CalcDivergence(&Div, &Divy, &Divz, InputNeutrons[i].Vector);

	    if (filtYDivMin >= 0.) if (fabs(Divy) < filtYDivMin) continue;
      if (filtYDivMax >= 0.) if (fabs(Divy) > filtYDivMax) continue;
      if (filtZDivMin >= 0.) if (fabs(Divz) < filtZDivMin) continue;
	    if (filtZDivMax >= 0.) if (fabs(Divz) > filtZDivMax) continue;
      if (filtDivMin  >= 0.) if (fabs(Div)  < filtDivMin)  continue;
	    if (filtDivMax  >= 0.) if (fabs(Div)  > filtDivMax)  continue;

      // transform to wanted format
      switch (ePrgFormat)
      { case VT_MCSTAS_FMT:
          ConvertVitess2McStas(&OutNeutron, &InputNeutrons[i]);
          break;
        case VT_MCPL_FMT:
          ConvertVitess2MCPL (&OutParticle, &InputNeutrons[i]);
          break;
        case VT_MCNPX_FMT:
          ConvertVitess2MCNPX(&OutNeutron,  &InputNeutrons[i]);
          break;
        default:   // nothing to do for VITESS
          memcpy(&OutNeutron, &InputNeutrons[i], sizeof(Neutron));
      }

      // write out neutrons of proper colour
      if (DetectColor < 0 || OutNeutron.Color == DetectColor) 
      {
        switch (ePrgFormat)
        {
          case VT_MCSTAS_FMT:
            fprintf(pOutFile, outform, OutNeutron.Probability, 
			                                 OutNeutron.Position[0], OutNeutron.Position[1], OutNeutron.Position[2],
			                                 OutNeutron.Vector  [0], OutNeutron.Vector  [1], OutNeutron.Vector  [2],
		                                   OutNeutron.Time,        
			                                 OutNeutron.Spin    [0], OutNeutron.Spin    [1], OutNeutron.Spin    [2]);
            break;

          case VT_MCPL_FMT:
            mcpl_add_particle(hOutFile, &OutParticle);
            break;

          case VT_MCNPX_FMT:
            fprintf(pOutFile, outform, OutNeutron.Position[0], OutNeutron.Position[1], OutNeutron.Position[2],
			                                 OutNeutron.Vector  [0], OutNeutron.Vector  [1], OutNeutron.Vector  [2],
		                                   OutNeutron.Wavelength,  OutNeutron.Probability, OutNeutron.Time);
            break;

          default:    // VITESS
            if (bF_cID)        { fprintf(pOutFile, form[cID],     OutNeutron.ID.IDGrp[0], OutNeutron.ID.IDGrp[1], OutNeutron.ID.IDNo); }
            if (bF_cTrc)       { fprintf(pOutFile, form[cTrc],    OutNeutron.Debug); }
            if (bF_cColor)     { fprintf(pOutFile, form[cColor],  OutNeutron.Color); }
            if (bF_cTOF)       { fprintf(pOutFile, form[cTOF],    OutNeutron.Time); }
            if (bF_cLambda)    { fprintf(pOutFile, form[cLambda], OutNeutron.Wavelength); }
            if (bF_cCounts)    { fprintf(pOutFile, form[cCounts], OutNeutron.Probability); }
            if (bF_cPosition)  {
              fprintf(pOutFile, form[cPosX],   OutNeutron.Position[0]);
              fprintf(pOutFile, form[cPosY],   OutNeutron.Position[1]);
              fprintf(pOutFile, form[cPosZ],   OutNeutron.Position[2]); }
            if (bF_cDirection) {
              fprintf(pOutFile, form[cDirX],   OutNeutron.Vector[0]);
              fprintf(pOutFile, form[cDirY],   OutNeutron.Vector[1]);
              fprintf(pOutFile, form[cDirZ],   OutNeutron.Vector[2]); }
            if (bF_cSpin)      {
              fprintf(pOutFile, form[cSpinX],  OutNeutron.Spin[0]);
              fprintf(pOutFile, form[cSpinY],  OutNeutron.Spin[1]);
              fprintf(pOutFile, form[cSpinZ],  OutNeutron.Spin[2]); }
        }

        if (ePrgFormat!=VT_MCPL_FMT)
          fputs("\n", pOutFile);
      }
    }
  }
  
  // Do module specific cleanups
 my_exit:
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
        case 'a':
          sscanf(&(argv[i][2]),"%hd", &bF_Active);
          break;
        case 'c':
          sscanf(&(argv[i][2]),"%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd", &bF_cID, &bF_cTrc, &bF_cColor, &bF_cTOF, &bF_cLambda, &bF_cCounts, &bF_cPosition, &bF_cDirection, &bF_cSpin);
          break;
        case 'C':
          DetectColor = (short) atoi(&argv[i][2]);
          break;

        case 'f':
          ePrgFormat = (VtPrgFormat) atoi(&argv[i][2]);
          break;
        case 'F':
          eDatFormat = (VtDataFormat) atoi(&argv[i][2]);
          break;
        case 'S':
          eSeparator = (VtSeparator) atoi(&argv[i][2]);
          break;
		
        case 'l':
          filtLambdaMin = atof(&argv[i][2]);  /* filter lambda, -1 means any */
          break;
        case 'L':
          filtLambdaMax = atof(&argv[i][2]);  /* filter lambda, -1 means any */
          break;

        case 'y':
          filtYMin = atof(&argv[i][2]);       /* filter Y */
          break;
        case 'Y':
          filtYMax = atof(&argv[i][2]);       /* filter Y */
          break;
        case 'z':
          filtZMin = atof(&argv[i][2]);       /* filter Z */
          break;
        case 'Z':
          filtZMax = atof(&argv[i][2]);       /* filter Z */
          break;
	    
        case 'd':
          filtYDivMax = atof(&argv[i][2]);    /* filter DivYMax, -1 means any */
          break;
        case 'D':
          filtZDivMax = atof(&argv[i][2]);    /* filter DivZMax, -1 means any */
          break;
        case 'e':
          filtYDivMin = atof(&argv[i][2]);    /* filter DivYMin, -1 means any */
          break;
        case 'E':
          filtZDivMin = atof(&argv[i][2]);    /* filter DivZMin, -1 means any */
          break;
        case 'g':
          filtDivMin = atof(&argv[i][2]);     /* filter DivMin,  -1 means any */
          break;
        case 'G':
          filtDivMax = atof(&argv[i][2]);     /* filter DivMax,  -1 means any */
          break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  calcDivY = (filtYDivMin >= 0. || filtYDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);
  calcDivZ = (filtZDivMin >= 0. || filtZDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);

  if (AsciiFileName != NULL)
  { if (bF_Active) 
    { if (ePrgFormat== VT_MCPL_FMT)
      {
         hOutFile = mcpl_create_outfile(FullParName(AsciiFileName));
      }
      else  
      { if ((pOutFile=fopen(FullParName(AsciiFileName),"wt"))==NULL) 
        { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", AsciiFileName);
          exit(-1);
        }
      }
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
  /* close the file if it was openend */
  if (bF_Active) 
  { if (ePrgFormat== VT_MCPL_FMT)
    {
      mcpl_close_outfile(hOutFile);
    }
    else  
    { fclose(pOutFile);
    }
  }
}


// -----------------------------------------------
// calculation of divergence from flight direction
// in : Direction: normalized flight direction
// out: Div    : total divergence
//    : HorDiv : horizontal divergence
//    : VrtDiv : vertical divergence
// return: rc  : TRUE/FALSE
// -----------------------------------------------
short CalcDivergence(double *pDiv, double *pHorDiv, double *pVrtDiv, const VectorType Direction)
{
  short rc=FALSE;

  if (calcDivY) 
  { *pHorDiv  = atan2(Direction[1], Direction[0]);
    *pHorDiv *= 180.0/M_PI;
    if ((Direction[1]==0.0) && (Direction[0]==0.0))
      *pHorDiv=0.0;
    rc=TRUE;
  }
  if (calcDivZ) 
  { *pVrtDiv  = atan2(Direction[2], Direction[0]);
    *pVrtDiv *= 180.0/M_PI;
    if ((Direction[2]==0.0) && (Direction[0]==0.0))
      *pVrtDiv=0.0;
    if (calcDivY) *pDiv = sqrt(sq(*pHorDiv) + sq(*pVrtDiv));
    rc=TRUE;
  }

  return(rc);
}



// -------------------------------------------------------------
// define format for variables in output file and print headline
// -------------------------------------------------------------
void SetFormatsAndHeader(int csep, const char *sep)
{
  switch (ePrgFormat)
  { case VT_MCSTAS_FMT: McStasParameters(); break;
    case VT_MCNPX_FMT : MCNPXParameters();  break;
    default: ; // nothing to do for VITESS
  }

  if (pOutFile)
  {
    fputs("#", pOutFile);
    if (eSeparator==VT_TABULATOR) 
    { // Tabular
      if (eDatFormat==VT_FLOAT) 
      { // float
        if (bF_cID)        { SP(form[cID],     "%c%c%010lu"); FP("___ID___ "); }
        if (bF_cTrc)       { SP(form[cTrc],    "%c");        FP("Trc"); }
        if (bF_cColor)     { SP(form[cColor],  "%5d");       FP("color"); }
        if (bF_cTOF)       { SP(form[cTOF],    "%7.3f");     FP("TOF"); }
        if (bF_cLambda)    { SP(form[cLambda], "%8.5f");     FP("lambda"); }
        if (bF_cCounts)    { SP(form[cCounts], "%11.3e");    FP("count_rate"); }
        if (bF_cPosition)  {
          SP(form[cPosX],   "%8.4f");     FP("pos_x");
          SP(form[cPosY],   "%8.4f");     FP("pos_y");
          SP(form[cPosZ],   "%8.4f");     FP("pos_z"); }
        if (bF_cDirection) {
          SP(form[cDirX],   "%9.6f");     FP("dir_x");
          SP(form[cDirY],   "%9.6f");     FP("dir_y");
          SP(form[cDirZ],   "%9.6f");     FP("dir_z"); }
        if (bF_cSpin)      {
          SP(form[cSpinX],  "%4.1f");     FP("sp_x");
          SP(form[cSpinY],  "%4.1f");     FP("sp_y");
          SP(form[cSpinZ],  "%4.1f");     FP("sp_z"); }
      } 
      else if (eDatFormat==VT_EXPONENTIAL)
      { // exp
        if (bF_cID)        { SP(form[cID],     "%c%c%010lu"); FP("___ID___ "); }
        if (bF_cTrc)       { SP(form[cTrc],    "%c");        FP("Trc"); }
        if (bF_cColor)     { SP(form[cColor],  "%5d");       FP("color"); }
        if (bF_cTOF)       { SP(form[cTOF],    "%.5e");      FP("TOF"); }
        if (bF_cLambda)    { SP(form[cLambda], "%.5e");      FP("lambda"); }
        if (bF_cCounts)    { SP(form[cCounts], "%.5e");      FP("count_rate"); }
        if (bF_cPosition)  {
          SP(form[cPosX],   "% .5e");     FP("pos_x");
          SP(form[cPosY],   "% .5e");     FP("pos_y");
          SP(form[cPosZ],   "% .5e");     FP("pos_z"); } 
        if (bF_cDirection) {
          SP(form[cDirX],   "% .5e");     FP("direction_x");
          SP(form[cDirY],   "% .5e");     FP("direction_y");
          SP(form[cDirZ],   "% .5e");     FP("direction_z"); }
        if (bF_cSpin)      {
          SP(form[cSpinX],  "% .5e");     FP("spin_x");
          SP(form[cSpinY],  "% .5e");     FP("spin_y");
          SP(form[cSpinZ],  "% .5e");     FP("spin_z"); }
      }
      else
      { Error("Data format not yet implemented");
      } // end eDatFormat
    } 
    else if (eSeparator==VT_BLANK) 
    { // Space
      if (eDatFormat==VT_FLOAT) 
      { // float
        if (bF_cID)        { SP(form[cID],     "%c%c%010lu"); FP("___ID___ "); }
        if (bF_cTrc)       { SP(form[cTrc],    "%c");        FP("Trc"); }
        if (bF_cColor)     { SP(form[cColor],  "%5d");       FP("color"); }
        if (bF_cTOF)       { SP(form[cTOF],    " %7.3f");    FP("    TOF"); }
        if (bF_cLambda)    { SP(form[cLambda], "%8.5f");     FP("  lambda"); }
        if (bF_cCounts)    { SP(form[cCounts], "%11.3e");    FP(" count_rate"); }
        if (bF_cPosition)  { SP(form[cPosX],   " %8.4f");    FP("    pos_x");
                             SP(form[cPosY],   "%8.4f");     FP("   pos_y");
                             SP(form[cPosZ],   "%8.4f");     FP("   pos_z"); }
        if (bF_cDirection) { SP(form[cDirX],   " %9.6f");    FP("     dir_x");
                             SP(form[cDirY],   "%9.6f");     FP("    dir_y");
                             SP(form[cDirZ],   "%9.6f");     FP("    dir_z"); }
        if (bF_cSpin)      { SP(form[cSpinX],  "  %4.1f");   FP("  sp_x");
                             SP(form[cSpinY],  "%4.1f");     FP("sp_y");
                             SP(form[cSpinZ],  "%4.1f");     FP("sp_z"); }
      } 
      else if (eDatFormat==VT_EXPONENTIAL)
      { // exp
        if (bF_cID)        { SP(form[cID],     "%c%c%010lu"); FP("___ID___ "); }
        if (bF_cTrc)       { SP(form[cTrc],    "%c");        FP("Trc"); }
        if (bF_cColor)     { SP(form[cColor],  "%5d");       FP("color"); }
        if (bF_cTOF)       { SP(form[cTOF],    " %.5e");     FP("     TOF"); }
        if (bF_cLambda)    { SP(form[cLambda], "%.5e");      FP("       lambda"); }
        if (bF_cCounts)    { SP(form[cCounts], "%.5e");      FP(" count_rate"); }
        if (bF_cPosition)  { SP(form[cPosX],   " % .5e");    FP("         pos_x");
                             SP(form[cPosY],   "% .5e");     FP("       pos_y");
                             SP(form[cPosZ],   "% .5e");     FP("       pos_z"); }
        if (bF_cDirection) { SP(form[cDirX],   " % .5e");    FP("  direction_x");
                             SP(form[cDirY],   "% .5e");     FP(" direction_y");
                             SP(form[cDirZ],   "% .5e");     FP(" direction_z"); }
        if (bF_cSpin)      { SP(form[cSpinX],  " % .5e");    FP("       spin_x");
                             SP(form[cSpinY],  "% .5e");     FP("      spin_y");
                             SP(form[cSpinZ],  "% .5e");     FP("      spin_z"); }
      }
      else
      { Error("Data format not yet implemented");
      } // end eDatFormat
    } 
    else
    { Error("Separator has unknown value");
    } // end Separator

    fputs("\n", pOutFile);
  } // end if (pOutFile)
}


short McStasParameters()
{
  bF_cID  = bF_cTrc    = bF_cColor    = bF_cLambda    = FALSE;
  bF_cTOF = bF_cCounts = bF_cPosition = bF_cDirection = bF_cSpin = TRUE;

  return(TRUE);
}

short MCNPXParameters()
{
  bF_cID  = bF_cTrc    = bF_cColor    = bF_cLambda    = FALSE;
  bF_cTOF = bF_cCounts = bF_cPosition = bF_cDirection = bF_cSpin = TRUE;

  return(TRUE);
}


// ------------------------------------
//  Convert VITESS to McStas trajectory 
// ------------------------------------

short ConvertVitess2McStas(Neutron* pMcNeutron, const Neutron* pVitNeutron)
{
	double  velocity;      // velocity of the neutron  [m/s]

	// initialization			                      
	memcpy(pMcNeutron, pVitNeutron, sizeof(Neutron));        

	pMcNeutron->Time /= 1000.0;            // unit ms -> s

	RotVit2Mc(&pMcNeutron->Position, &pVitNeutron->Position);
	RotVit2Mc(&pMcNeutron->Vector,   &pVitNeutron->Vector);
	RotVit2Mc(&pMcNeutron->Spin,     &pVitNeutron->Spin);

	velocity = 10.0 * V_FROM_LAMBDA(pVitNeutron->Wavelength); // unit cm/ms -> m/s
	MultiplyByScalar(pMcNeutron->Vector, velocity);     
	MultiplyByScalar(pMcNeutron->Position, 0.01);             // unit    cm -> m

  return(TRUE);
}

// ------------------------------------------
// conversion from VITESS to MCPL parameters
// ------------------------------------------
short ConvertVitess2MCPL(mcpl_particle_t* pMCPLNeutron, const Neutron* pVitNeutron)
{
	// initialization			                      
	memset(pMCPLNeutron, '\0', sizeof(mcpl_particle_t));        

	pMCPLNeutron->pdgcode= NEUTRON_ID;
	pMCPLNeutron->ekin   = 1.0e-12 * ENERGY_FROM_LAMBDA(pVitNeutron->Wavelength); // lambda -> energy;  unit µeV -> MeV
	pMCPLNeutron->time   = pVitNeutron->Time;                                     // unit ms -> s
	pMCPLNeutron->weight = pVitNeutron->Probability;

	RotVit2Mc(&pMCPLNeutron->position,     &pVitNeutron->Position);
	RotVit2Mc(&pMCPLNeutron->direction,    &pVitNeutron->Vector);
	RotVit2Mc(&pMCPLNeutron->polarisation, &pVitNeutron->Spin);

  return(TRUE);
}

// ------------------------------------------
// conversion from VITESS to MCNPX parameters
// ------------------------------------------
short ConvertVitess2MCNPX (Neutron* pMcnpNeutron, const Neutron* pVitNeutron)
{
  memcpy(pMcnpNeutron, pVitNeutron, sizeof(Neutron));

  pMcnpNeutron->Wavelength  =  ENERGY_FROM_LAMBDA(pVitNeutron->Wavelength) // lambda -> energy
                             * 1.0e-12;     // unit µeV -> MeV
  pMcnpNeutron->Time        *= 1.0e+05;     // unit  ms -> shakes = 1.0e-08 s

  return(TRUE);
}


void RotVit2Mc(VectorType* pMcVector, const VectorType* pVitVector)
{
	(*pMcVector)[0] = (*pVitVector)[1];
	(*pMcVector)[1] = (*pVitVector)[2];
	(*pMcVector)[2] = (*pVitVector)[0];
}
