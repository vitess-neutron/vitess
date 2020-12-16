/*********************************************************************************************/
/*  VITESS module 'writeout'                                                                 */
/*                                                                                           */
/* This module writes neutron events (trajectories) info files in different formats          */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0   Jun 1999  ???             initial version                                           */
/* 1.1   Mar 2001  K. Lieutenant   headline                                                  */	
/* 1.2   Jan 2004  K. Lieutenant   changes for 'instrument.dat' and changed headline         */
/* 1.3   Feb 2004  K. Lieutenant   'FullParName' and 'ERROR' included                        */
/* 1.4   Mar 2004  K. Lieutenant   F-Format Option                                           */
/* 1.4e  Jul 2005  M. Fromme       headline, simplification                                  */
/* 1.4f  Jan 2010  A. Houben       WriteOut only if given color matches Neutron color        */
/* 1.4g  Feb 2010  A. Houben       Added wavelength, Div and yz position filter              */
/* 1.4h  Feb 2010  K. Lieutenant   colour = 0 means all                                      */
/* 1.4i  Jul 2011  A. Houben       colour = -1 means all; colour = 0 means only untaged      */
/*                                 neutrons by previous modules                              */
/* 1.4j  Aug 2011  A. Houben       extended divergence filters                               */
/* 1.5   May 2012  A. Houben       select output columns (reduces file size for long simul.) */
/*       Aug 2012  M. Fromme       clean up                                                  */
/* 1.6   Jan 2013  K. Lieutenant   tidy up; McStas and MCNPX format                          */
/* 1.7   Apr 2018  K. Lieutenant   MCPL format                                               */
/* 1.8   Jul 2018  K. Lieutenant   McStas and MCNPX format use their own structures          */
/* 1.9   Mar 2020  K. Lieutenant   new central visualization parameters                      */
/* 1.10  Mar 2020  K. Lieutenant   binary output                                             */
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
#define SP(var, form) if (iSep) strcpy(var,pSep); strncat(var, form, 13)
#define FP(s) if (iSep++) fputs(pSep, pOutFile); fputs(s, pOutFile)

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

#define BLOCK_SIZE 20.0

/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);                                           // Reads input parameters and sets global variables
void  OwnCleanup();                                                              // Does module specific cleanup
void  SetFormatsAndHeader(int iSsep, const char *sSep);                          // Defines format for variables in output file and print headline
short CalcDivergence(double *pFullDiv, double *pHorDiv, double *pVertDiv,        // calculation of divergence from flight direction
                     const VectorType Direction);                                
short McStasParameters();                                                        // Sets output parameters for McStas  
short MCNPParameters();                                                         // Sets output parameters for MCNPX  
short ConvertVitess2McStas(McNeutron*       pMcNeut,   const Neutron* pVitNeut); // Conversion from VITESS to McStas trajectory
short ConvertVitess2MCPL  (mcpl_particle_t* pMcplNeut, const Neutron* pVitNeut); // Conversion from VITESS to MCPL  trajectory
short ConvertVitess2MCNP  (McnpNeutron*     pMcnpNeut, const Neutron* pVitNeut); // Conversion from VITESS to MCNP  trajectory

void  RotVit2Mc(VectorType* pMcVector, const VectorType* pVitVector);            // Vector transfer from VITESS to McStas co-ordinate system
                                                                               
char* FullParName(const char* filename);                                         // function in init.c, adds parameter directory to file name 


/******************************/
/** Global Variables         **/
/******************************/
McCompID       _eModule=MCN_WRITEOUT;

// Input parameters
char*        sOutFileName=NULL;         //  -A   [-]   output file name
short        bActive=TRUE;              //  -a   [-]   flag: YES: writeout is active   NO: output file is not written
VtPrgFormat  ePrgFormat=VT_VITESS_ASC;  //  -f   [-]   output format: VT_VITESS_FMT: VITESS ASCII format   VT_MCSTAS_FMT: McStas   VT_MCPL_FMT: MCPL   VT_MCNP_FMT: MCNP   VT_VITESS_BIN: VITESS binary format)
short        iDetectColor = -1;         //  -C   [-]   write out only neutrons with a given color, -1 means any
short        bF_cID=TRUE,               //  -c   [-]   string containing 9 flags to determine which parameters are written: ID, trace flag, color, TOF, wavelength, intensity, position, direction, spin.
             bF_cTrc=TRUE,
             bF_cColor=TRUE,
             bF_cTOF=TRUE,
             bF_cLambda=TRUE,
             bF_cCounts=TRUE,
             bF_cPosition=TRUE,
             bF_cDirection=TRUE,
             bF_cSpin=TRUE;

VtDataFormat eDatFormat=VT_FLOAT;       //  -F   [-]   output format (exponential, float)
VtSeparator  eSeparator=VT_BLANK;       //  -S   [-]   separator between columns (space, tab)
double       FactInt   = 1.0;           //  -I   [-]   Factor to normalize to the source intensity from MCNPX data

double       filtLambdaMin=-1.0,        //  -l  [Ang]  minimal wavelength to be taken into account
             filtLambdaMax= 1.0e10,     //  -L  [Ang]  maximal wavelength to be taken into account
             filtYMin     =-1.0e10,     //  -y   [cm]  minimal horizontal position to be taken into account
             filtYMax     = 1.0e10,     //  -Y   [cm]  maximal horizontal position to be taken into account
             filtZMin     =-1.0e10,     //  -z   [cm]  minimal vertical position to be taken into account
             filtZMax     = 1.0e10,     //  -Z   [cm]  maximal vertical position to be taken into account
             filtYDivMin  =-1.0e10,     //  -e  [deg]  minimal horizontal divergence to be taken into account
             filtYDivMax  = 1.0e10,     //  -d  [deg]  maximal horizontal divergence to be taken into account
             filtZDivMin  =-1.0e10,     //  -E  [deg]  minimal vertical divergence to be taken into account
             filtZDivMax  = 1.0e10,     //  -D  [deg]  maximal vertical divergence to be taken into account
             filtDivMin   =-1.0e10,     //  -E  [deg]  minimal divergence to be taken into account
             filtDivMax   = 1.0e10;     //  -D  [deg]  maximal divergence to be taken into account

// Variables determined from input parameters or trajectory data
FILE*          pOutFile=NULL;           //             pointer to output file
mcpl_outfile_t hOutFile;                //             handle to output file for MCPL format
char*          sOutform=NULL;           //             format for the whole line using McStas or MCNPX
char*          sHeader =NULL;           //             header: parameters of the event file
char*          sUnits  =NULL;           //             header: units used in the event file
short          bCalcDivY = FALSE,       //             flag: calculation of hor. divergence 
               bCalcDivZ = FALSE;       //                               or vert. divergence necessary
char           form[15][15]={"","","","","","","","","","","","","","",""};            
                                        // formats to print data of the different parameters using VITESS


/******************************/
/**  Main Program            **/
/******************************/
int main(int argc, char **argv)
{
  int             i=0,               // index of trajectories
                  nBytesW=0;         // number of bytes written to output file
  double          Divy=0.0, 
                  Divz=0.0, Div=0.0; // divergence of the current trajectory
  Neutron         OutNeutron;
  McNeutron       OutMcNeutron;
  McnpNeutron     OutMpNeutron;
  mcpl_particle_t OutParticle;

  Divy = Divz = Div = 0.0;
  memset(&OutNeutron,   '\0', sizeof(Neutron));
  memset(&OutMcNeutron, '\0', sizeof(McNeutron));
  memset(&OutMpNeutron, '\0', sizeof(McnpNeutron));
  memset(&OutParticle,  '\0', sizeof(mcpl_particle_t));

  // Initialization 
  // --------------
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.10");
  OwnInit(argc, argv);
  
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;
  // if (bVisInstr) stGeometry.pDescr = "writeout:white";
 
  DECLARE_ABORT;

  // loop over trajectories
  // ----------------------
	while((ReadNeutrons())!= 0)
  {
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK;

      // write all trajectories to pipe
	    WriteNeutron(&(InputNeutrons[i]));

      // skip the rest if the module is not active 
	    if (!bActive) continue;

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
          ConvertVitess2McStas(&OutMcNeutron, &InputNeutrons[i]);
          break;
        case VT_MCPL_FMT:
          ConvertVitess2MCPL (&OutParticle,   &InputNeutrons[i]);
          break;
        case VT_MCNP_FMT:
          ConvertVitess2MCNP(&OutMpNeutron,  &InputNeutrons[i]);
          break;
        default:   // nothing to do for VITESS
          memcpy(&OutNeutron, &InputNeutrons[i], sizeof(Neutron));
      }

      // write out neutrons of proper colour
      if (iDetectColor < 0 || OutNeutron.Color == iDetectColor) 
      {
        switch (ePrgFormat)
        {
          case VT_MCSTAS_FMT:
            fprintf(pOutFile, sOutform, OutMcNeutron.Weight, 
			                                 OutMcNeutron.Position[0], OutMcNeutron.Position[1], OutMcNeutron.Position[2],
			                                 OutMcNeutron.Speed   [0], OutMcNeutron.Speed   [1], OutMcNeutron.Speed   [2],
		                                   OutMcNeutron.Time,        
			                                 OutMcNeutron.Spin    [0], OutMcNeutron.Spin    [1], OutMcNeutron.Spin    [2]);
            break;

          case VT_MCPL_FMT:
            mcpl_add_particle(hOutFile, &OutParticle);
            break;

          case VT_MCNP_FMT:
            fprintf(pOutFile, sOutform, OutNeutron.Position[0], OutNeutron.Position[1], OutNeutron.Position[2],
			                                 OutNeutron.Vector  [0], OutNeutron.Vector  [1], OutNeutron.Vector  [2],
		                                   OutNeutron.Wavelength,  OutNeutron.Probability, OutNeutron.Time);
            break;

          case VT_VITESS_BIN:
            nBytesW = fwrite(&OutNeutron, sizeof(Neutron)-1, 1, pOutFile);
            break;

          default:    // VITESS ASCII format
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
        { 
          double nBlocks, rest;
          fputs("\n", pOutFile);

          // flush output file to make it available for other applications
          rest = modf(i/BLOCK_SIZE, &nBlocks);
          if (rest==0.0)
            fflush(pOutFile);
        }
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


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void  OwnInit(int argc, char *argv[]) 
{
  int         i=0,         // index of parameter list  
              iSep=0;      // index of formats
  const char* pSep=NULL;   // separator

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
        case 'A':
          sOutFileName = &argv[i][2];
          break;
        case 'a':
          sscanf(&(argv[i][2]),"%hd", &bActive);
          break;
        case 'c':
          sscanf(&(argv[i][2]),"%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd", &bF_cID, &bF_cTrc, &bF_cColor, &bF_cTOF, &bF_cLambda, &bF_cCounts, &bF_cPosition, &bF_cDirection, &bF_cSpin);
          break;
        case 'C':
          iDetectColor = (short) atoi(&argv[i][2]);
          break;
        case 'I':
          FactInt    = atof(&argv[i][2]);
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
	    
        case 'e':
          filtYDivMin = atof(&argv[i][2]);    /* filter DivYMin, -1 means any */
          break;
        case 'd':
          filtYDivMax = atof(&argv[i][2]);    /* filter DivYMax, -1 means any */
          break;
        case 'E':
          filtZDivMin = atof(&argv[i][2]);    /* filter DivZMin, -1 means any */
          break;
        case 'D':
          filtZDivMax = atof(&argv[i][2]);    /* filter DivZMax, -1 means any */
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

  bCalcDivY = (filtYDivMin >= 0. || filtYDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);
  bCalcDivZ = (filtZDivMin >= 0. || filtZDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);

  if (sOutFileName != NULL)
  { if (bActive) 
    { if (ePrgFormat== VT_MCPL_FMT)
      { hOutFile = mcpl_create_outfile(FullParName(sOutFileName));
      }
      else if (ePrgFormat== VT_VITESS_BIN)
      { pOutFile=OpenOutputFile(sOutFileName, TRUE, "wb");
      }
      else  
      { pOutFile=OpenOutputFile(sOutFileName, TRUE, "wt");
      }
      fprintf(LogFilePtr,"Trajectories written to output file %s\n", FullParName(sOutFileName));
    }
  } 
  else 
  { fputs("ERROR: The option -A to give the ascii file name is mandatory!\n", LogFilePtr);
    exit(-1);
  }
  
  if (eSeparator==VT_TABULATOR) 
    pSep = "\t"; 
  else if (eSeparator==VT_BLANK) 
    pSep = " ";
  else
    Error("Separator has unknown value");

  // define format for variables in output file and print headline
  if (bActive)
  {
    switch (ePrgFormat)
    { case VT_MCSTAS_FMT:
        sHeader  = "#     weight        pos_x     pos_y      pos_z      speed_x      speed_y      speed_z        TOF       S_x  S_y  S_z \n";  
        sUnits   = "#      [n/s]         [m]       [m]        [m]        [m/s]        [m/s]        [m/s]         [s]       [1]  [1]  [1] \n";  
        sOutform = "%15.3f  %9.6f %9.6f %10.6f  %12.2f %12.2f %12.2f  %11.9f  %4.1f %4.1f %4.1f";
	      fprintf(pOutFile,"#Trajectories writeout_McStas \n");
        fprintf(pOutFile, "%s%s", sHeader, sUnits);
        break;
      case VT_MCPL_FMT:
        mcpl_hdr_set_srcname    (hOutFile, "VITESS 3.4  module writeout 1.10"); /* Name of the generating application         */
        mcpl_hdr_add_comment    (hOutFile, "first test");                       /* Add one or more human-readable comments    */
        mcpl_enable_polarisation(hOutFile);                                     /* to write the "polarisation" info           */
        break;
      case VT_MCNP_FMT:
        sHeader  = "#    pos_x          pos_y          pos_z          dir_x          dir_y          dir_z            E           weight          time  \n";  
        sUnits   = "#     [cm]           [cm]           [cm]           [1]            [1]            [1]           [MeV]           [1]         [1e-8s] \n";  
        sOutform = "%14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e %14.6e";
	      fprintf(pOutFile,"#Trajectories writeout_MCNPX \n");
        fprintf(pOutFile, "%s%s", sHeader, sUnits);
        break;
      case VT_VITESS_BIN:
        // nothing to do for VITESS binary output
        break;
      default:   
        // VITESS ASCII output
	      fprintf(pOutFile, "#Trajectories writeout_Vitess \n");
        SetFormatsAndHeader(iSep, pSep);
    }
  }

}


/***************************************************/
/**  Does module specific cleanup                 **/
/***************************************************/
void OwnCleanup()
{
  /* close the file if it was openend */
  if (bActive) 
  { if (ePrgFormat== VT_MCPL_FMT)
    {
      mcpl_close_outfile(hOutFile);
    }
    else  
    { fclose(pOutFile);
    }
  }
}


/***************************************************
** calculation of divergence from flight direction
** in : Direction: normalized flight direction
** out: Div    : total divergence
**    : HorDiv : horizontal divergence
**    : VrtDiv : vertical divergence
** return: rc  : TRUE/FALSE
****************************************************/
short CalcDivergence(double *pDiv, double *pHorDiv, double *pVrtDiv, const VectorType Direction)
{
  short rc=FALSE;

  if (bCalcDivY) 
  { *pHorDiv  = atan2(Direction[1], Direction[0]);
    *pHorDiv *= 180.0/M_PI;
    if ((Direction[1]==0.0) && (Direction[0]==0.0))
      *pHorDiv=0.0;
    rc=TRUE;
  }
  if (bCalcDivZ) 
  { *pVrtDiv  = atan2(Direction[2], Direction[0]);
    *pVrtDiv *= 180.0/M_PI;
    if ((Direction[2]==0.0) && (Direction[0]==0.0))
      *pVrtDiv=0.0;
    if (bCalcDivY) *pDiv = sqrt(sq(*pHorDiv) + sq(*pVrtDiv));
    rc=TRUE;
  }

  return(rc);
}


/**********************************************************************/
/**  Defines format for variables in output file and print headline  **/
/**********************************************************************/
void SetFormatsAndHeader(int iSep, const char *pSep)
{
  switch (ePrgFormat)
  { case VT_MCSTAS_FMT: McStasParameters(); break;
    case VT_MCNP_FMT  : MCNPParameters();   break;
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
        if (bF_cTOF)       { SP(form[cTOF],    "%9.5f");     FP("TOF"); }
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
        if (bF_cTOF)       { SP(form[cTOF],    " %9.5f");    FP("     TOF "); }
        if (bF_cLambda)    { SP(form[cLambda], "%8.5f");     FP("  lambda "); }
        if (bF_cCounts)    { SP(form[cCounts], "%11.3e");    FP(" count_rate"); }
        if (bF_cPosition)  { SP(form[cPosX],   " %8.4f");    FP("   pos_x");
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


/********************************************************/
/**  Sets output parameters for McStas and MCNPX resp. **/
/********************************************************/
short McStasParameters()
{
  bF_cID  = bF_cTrc    = bF_cColor    = bF_cLambda    = FALSE;
  bF_cTOF = bF_cCounts = bF_cPosition = bF_cDirection = bF_cSpin = TRUE;

  return(TRUE);
}

short MCNPParameters()
{
  bF_cID  = bF_cTrc    = bF_cColor    = bF_cLambda    = FALSE;
  bF_cTOF = bF_cCounts = bF_cPosition = bF_cDirection = bF_cSpin = TRUE;

  return(TRUE);
}


/***************************************************/
/**  Conversion from VITESS to McStas trajectory  **/
/***************************************************/
short ConvertVitess2McStas(McNeutron* pMcNeutron, const Neutron* pVitNeutron)
{
	double  velocity;      // velocity of the neutron  [m/s]

	// initialization			                      
	memset(pMcNeutron, '\0', sizeof(McNeutron));        

	velocity = 10.0 * V_FROM_LAMBDA(pVitNeutron->Wavelength); // unit cm/ms -> m/s
	pMcNeutron->Time   = pVitNeutron->Time/1000.0;            // unit ms -> s
  pMcNeutron->Weight = pVitNeutron->Probability;

	RotVit2Mc(&pMcNeutron->Position, &pVitNeutron->Position);
	RotVit2Mc(&pMcNeutron->Speed,    &pVitNeutron->Vector);
	RotVit2Mc(&pMcNeutron->Spin,     &pVitNeutron->Spin);

	MultiplyByScalar(pMcNeutron->Speed, velocity);     
	MultiplyByScalar(pMcNeutron->Position, 0.01);             // unit    cm -> m

  return(TRUE);
}

/**************************************************/
/** Conversion from VITESS to MCPL parameters    **/
/**************************************************/
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

/**************************************************/
/**  Conversion from VITESS to MCNPX parameters  **/
/**************************************************/
short ConvertVitess2MCNP(McnpNeutron* pMcnpNeutron, const Neutron* pVitNeutron)
{
  CopyVector(pVitNeutron->Position, pMcnpNeutron->Position);
  CopyVector(pVitNeutron->Vector  , pMcnpNeutron->Vector  );
  pMcnpNeutron->Energy = ENERGY_FROM_LAMBDA(pVitNeutron->Wavelength) * 1.0e-12;   // lambda -> energy,  unit µeV -> MeV
  pMcnpNeutron->Counts = pVitNeutron->Probability / FactInt;                      //  n/s -> counts
  pMcnpNeutron->Shakes = 1.0e+05 *pVitNeutron->Time;                              // unit  ms -> shakes = 1.0e-08 s

  return(TRUE);
}


/****************************************************************/
/**  Vector transfer from VITESS to McStas co-ordinate system  **/
/****************************************************************/
void RotVit2Mc(VectorType* pMcVector, const VectorType* pVitVector)
{
	(*pMcVector)[0] = (*pVitVector)[1];
	(*pMcVector)[1] = (*pVitVector)[2];
	(*pMcVector)[2] = (*pVitVector)[0];
}
