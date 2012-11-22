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
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"

#define SP(var, form) if (csep) strcpy(var,sep); strncat(var, form, 13)

#define FP(s) if (csep++) fputs(sep, AsciiFile); fputs(s, AsciiFile)

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

FILE *AsciiFile;
short bF_format=FALSE,
  bF_Separator=FALSE,
  bF_cID=TRUE,
  bF_cTrc=TRUE,
  bF_cColor=TRUE,
  bF_cTOF=TRUE,
  bF_cLambda=TRUE,
  bF_cCounts=TRUE,
  bF_cPosition=TRUE,
  bF_cDirection=TRUE,
  bF_cSpin=TRUE,
  bF_Active=TRUE;

short DetectColor = 0; // WriteOut only neutrons with a given color, -1 means any
double filtLambdaMin=-1.0,          // filter
  filtLambdaMax=-1.0,
  filtYMin=-1.0e10,
  filtYMax=1.0e10,
  filtZMin=-1.0e10,
  filtZMax=1.0e10,
  filtYDivMin=-1.0,
  filtZDivMin=-1.0,
  filtDivMin=-1.0,
  filtYDivMax=-1.0,
  filtZDivMax=-1.0,
  filtDivMax=-1.0;
int  calcDivY = 0,
  calcDivZ = 0;

void OwnInit(int argc, char *argv[]);
void OwnCleanup();


int main(int argc, char **argv)
{
  int i, csep;
  static char form[15][15];
  const char *sep;
  double Divy, Divz, Div;

  Divy = Divz = Div = 0;
  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("writeout 1.5");

  /* module specific initialization */
  OwnInit(argc, argv);
 
  /* Get the neutrons from the file */
  DECLARE_ABORT;
  
  if (bF_Separator) // Tabular
    sep = "\t"; 
  else
    sep = " ";

  csep = 0;
  if (AsciiFile) {
    fputs("#", AsciiFile);
    if (bF_Separator) { // Tabular
      if (bF_format) { // float
        if (bF_cID)        { SP(form[cID],     "%c%c%09lu"); FP("___ID___ "); }
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
      } else { // exp
        if (bF_cID)        { SP(form[cID],     "%c%c%09lu"); FP("___ID___ "); }
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
    } else { //Space
      if (bF_format) { // float
        if (bF_cID)        { SP(form[cID],     "%c%c%09lu"); FP("___ID___ "); }
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
      } else { // exp
        if (bF_cID)        { SP(form[cID],     "%c%c%09lu"); FP("___ID___ "); }
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
    }
  }
  fputs("\n", AsciiFile);

  while((ReadNeutrons())!= 0)
    {
      CHECK;    
      for(i=0; i<NumNeutGot; i++) 
        {
          CHECK;

	  WriteNeutron(&(InputNeutrons[i]));
	  if (!bF_Active) continue;

	  if (filtLambdaMin >= 0. && InputNeutrons[i].Wavelength < filtLambdaMin) continue;
	  if (filtLambdaMax >= 0. && InputNeutrons[i].Wavelength > filtLambdaMax) continue;
	  if (InputNeutrons[i].Position[1] < filtYMin) continue;
	  if (InputNeutrons[i].Position[1] > filtYMax) continue;
	  if (InputNeutrons[i].Position[2] < filtZMin) continue;
	  if (InputNeutrons[i].Position[2] > filtZMax) continue;
	  
          if (calcDivY) {
            Divy = (double)atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);
            Divy*=180.0/M_PI;
            if ((InputNeutrons[i].Vector[1]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divy=0.0;
          }
          if (calcDivZ) {
            Divz = (double)atan2(InputNeutrons[i].Vector[2],InputNeutrons[i].Vector[0]);
            Divz*=180.0/M_PI;
            if ((InputNeutrons[i].Vector[2]==0.0) && (InputNeutrons[i].Vector[0]==0.0))
              Divz=0.0;
            if (calcDivY) Div = sqrt(sq(Divy) + sq(Divz));
          }

	  if (filtYDivMin >= 0.) {
            if (fabs(Divy) < filtYDivMin) continue;
	  }    
          if (filtYDivMax >= 0.) {
            if (fabs(Divy) > filtYDivMax) continue;
	  }
  
          if (filtZDivMin >= 0.) {
            if (fabs(Divz) < filtZDivMin) continue;
	  }
	  if (filtZDivMax >= 0.) {
            if (fabs(Divz) > filtZDivMax) continue;
	  }

          if (filtDivMin >= 0.) {
            if (fabs(Div) < filtDivMin) continue;
	  }
	  if (filtDivMax >= 0.) {
            if (fabs(Div) > filtDivMax) continue;
	  }

          if (DetectColor < 0 || InputNeutrons[i].Color == DetectColor) {
            if (bF_cID)        { fprintf(AsciiFile, form[cID],     InputNeutrons[i].ID.IDGrp[0], InputNeutrons[i].ID.IDGrp[1], InputNeutrons[i].ID.IDNo); }
            if (bF_cTrc)       { fprintf(AsciiFile, form[cTrc],    InputNeutrons[i].Debug); }
            if (bF_cColor)     { fprintf(AsciiFile, form[cColor],  InputNeutrons[i].Color); }
            if (bF_cTOF)       { fprintf(AsciiFile, form[cTOF],    InputNeutrons[i].Time); }
            if (bF_cLambda)    { fprintf(AsciiFile, form[cLambda], InputNeutrons[i].Wavelength); }
            if (bF_cCounts)    { fprintf(AsciiFile, form[cCounts], InputNeutrons[i].Probability); }
            if (bF_cPosition)  {
              fprintf(AsciiFile, form[cPosX],   InputNeutrons[i].Position[0]);
              fprintf(AsciiFile, form[cPosY],   InputNeutrons[i].Position[1]);
              fprintf(AsciiFile, form[cPosZ],   InputNeutrons[i].Position[2]); }
            if (bF_cDirection) {
              fprintf(AsciiFile, form[cDirX],   InputNeutrons[i].Vector[0]);
              fprintf(AsciiFile, form[cDirY],   InputNeutrons[i].Vector[1]);
              fprintf(AsciiFile, form[cDirZ],   InputNeutrons[i].Vector[2]); }
            if (bF_cSpin)      {
              fprintf(AsciiFile, form[cSpinX],  InputNeutrons[i].Spin[0]);
              fprintf(AsciiFile, form[cSpinY],  InputNeutrons[i].Spin[1]);
              fprintf(AsciiFile, form[cSpinZ],  InputNeutrons[i].Spin[2]); }
            fputs("\n", AsciiFile);
          }
        }
    }
  
  /* Do module specific cleanups */
 my_exit:
  OwnCleanup();
  
  /* Do the general cleanup */
  Cleanup(0.0,0.0,0.0, 0.0,0.0);
  
  return 0;
}


void  OwnInit(int argc, char *argv[]) 
{
  char *AsciiFileName=NULL;
  int i;

  for(i=1; i<argc; i++) 
    { if(argv[i][0]!='+') 
        { switch(argv[i][1])
            { case 'A':
                AsciiFileName = &argv[i][2];
                break;
            case 'F':
              bF_format = (short) atoi(&argv[i][2]);
              break;
            case 'a':
              sscanf(&(argv[i][2]),"%hd", &bF_Active);
              break;
            case 'S':
              bF_Separator = (short) atoi(&argv[i][2]);
              break;
	    case 'C':
              DetectColor = (short) atoi(&argv[i][2]);
              break;
            case 'c':
              sscanf(&(argv[i][2]),"%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd%1hd", &bF_cID, &bF_cTrc, &bF_cColor, &bF_cTOF, &bF_cLambda, &bF_cCounts, &bF_cPosition, &bF_cDirection, &bF_cSpin);
              break;
		
            case 'l':
              filtLambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
              break;

	    case 'L':
              filtLambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
              break;

            case 'y':
              filtYMin = atof(&argv[i][2]);   /* filter Y */
              break;

            case 'Y':
              filtYMax = atof(&argv[i][2]);   /* filter Y */
              break;

            case 'z':
              filtZMin = atof(&argv[i][2]);   /* filter Z */
              break;

            case 'Z':
              filtZMax = atof(&argv[i][2]);   /* filter Z */
              break;
	    
            case 'd':
              filtYDivMax = atof(&argv[i][2]);   /* filter DivYMax, -1 means any */
              break;

            case 'D':
              filtZDivMax = atof(&argv[i][2]);   /* filter DivZMax, -1 means any */
              break;

            case 'e':
              filtYDivMin = atof(&argv[i][2]);   /* filter DivYMin, -1 means any */
              break;

            case 'E':
              filtZDivMin = atof(&argv[i][2]);   /* filter DivZMin, -1 means any */
              break;

            case 'g':
              filtDivMin = atof(&argv[i][2]);   /* filter DivMin, -1 means any */
              break;

            case 'G':
              filtDivMax = atof(&argv[i][2]);   /* filter DivMax, -1 means any */
              break;

            default:
              fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
              exit(-1);
              break;
            }
        }
    }

  calcDivY = (filtYDivMin >= 0. || filtYDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);
  calcDivZ = (filtZDivMin >= 0. || filtZDivMax >= 0. || filtDivMin >= 0. || filtDivMax >= 0.);

  if (AsciiFileName != NULL)
    { if (bF_Active) 
        { if ((AsciiFile=fopen(FullParName(AsciiFileName),"wt"))==NULL) 
            { fprintf(LogFilePtr,"ERROR: Can't open file %s\n", AsciiFileName);
              exit(-1);
            }
        }
    } 
  else 
    { fputs("ERROR: The option -A to give the ascii file name is mandatory!\n", LogFilePtr);
      exit(-1);
    }
}


void OwnCleanup()
{
  if (bF_Active) fclose(AsciiFile);
}
