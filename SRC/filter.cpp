/********************************************************************************************/
/*  VITESS module 'filter.cpp'                                                              */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Apr 2013  D. Nekrassov  initial version                                             */
/* 1.1  Nov 2013  D. Nekrassov  "OR" mode bug fixed                                         */
/* 1.2  Feb 2015  K. Lieutenant "AND OR AND" added                                          */
/* 1.3  Mar 2020  K. Lieutenant  new central visualization parameters, UNUSED -> TRUE       */
/* 1.3a Oct 2021  K. Lieutenant  enums used for parameters and their combination            */
/* 1.3b Oct 2021  K. Lieutenant  parameter list completed                                   */
/********************************************************************************************/


#include <math.h>

extern "C" 
{
 #include "convert.h"
 #include "init.h"
 #include "softabort.h"
 #include "general.h"
}

/**************************************************/
/** Definitions, Global Variables and Prototypes **/
/**************************************************/
#include "filter.h"


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  short  bRegistered=FALSE;
  long	 i=0 ;
  char   sTxt[5][12]={"","","","",""};

  // reading of input data and initialisation
  // ----------------------------------------
  _eModule=MCN_FILTER;

  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3b");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bBlowUp       = FALSE;

  DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // bRregistered = TRUE if neutron was considered in the monitor
        bRegistered=CheckFilter(&InputNeutrons[i]);
	  
        if (bRegistered) 
        {
          WriteNeutron(&(InputNeutrons[i]));
        }
      }
    }
  }

// Finish: writes to log and instrument file, frees memory
// -------------------------------------------------------
my_exit:
  // writes to instrument and log file
  for (int k=0; k < 4; k++)
    MonPar_ID2Txt(sTxt[k], filterParam[k]);
  FiltComb_ID2Txt(sTxt[4], filterComb);

  fprintf(LogFilePtr, "Parameter: %s %s %s %s, combined by %s\n", sTxt[0], sTxt[1], sTxt[2], sTxt[3], sTxt[4]);

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  filterVarMin[0] = -1.0e10;
  filterVarMin[1] = -1.0e10;
  filterVarMin[2] = -1.0e10;
  filterVarMin[3] = -1.0e10;
  filterVarMax[0] =  1.0e10;
  filterVarMax[1] =  1.0e10;
  filterVarMax[2] =  1.0e10;
  filterVarMax[3] =  1.0e10;

  filterParam[0]  =  NO_PAR;
  filterParam[1]  =  NO_PAR;
  filterParam[2]  =  NO_PAR;
  filterParam[3]  =  NO_PAR;

  filterComb      =  NO_FCOMB;

  // Read the command line arguments
  for(int i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'I':  
          filterParam[0] = (VtMonPar) atoi(&argv[i][2]); // filter parameter 1, input parameter
          break;
        case 'J':  
          filterParam[1] = (VtMonPar) atoi(&argv[i][2]); // filter parameter 2, optional input parameter
          break;
        case 'K':  
          filterParam[2] = (VtMonPar) atoi(&argv[i][2]); // filter parameter 3, optional input parameter
          break; 
        case 'L':  
          filterParam[3] = (VtMonPar) atoi(&argv[i][2]); // filter parameter 4, optional input parameter
          break; 

        case 'C':  
          filterComb  =  (VtFiltComb) atoi(&argv[i][2]); // filter combination (AND,OR)
          break;
	    
        case 'u':
          filterVarMin[0] = atof(&argv[i][2]);   /* minimum value of filter parameter 1 */
          break;
        case 'U':
          filterVarMax[0] = atof(&argv[i][2]);   /* maximum value of filter parameter 1 */
          break;
	    
        case 'v':
          filterVarMin[1] = atof(&argv[i][2]);   /* minimum value of filter parameter 2 */
          break;
        case 'V':
          filterVarMax[1] = atof(&argv[i][2]);   /* maximum value of filter parameter 2 */
          break;

        case 'w':
          filterVarMin[2] = atof(&argv[i][2]);   /* minimum value of filter parameter 3 */
          break;
        case 'W':
          filterVarMax[2] = atof(&argv[i][2]);   /* maximum value of filter parameter 3 */
          break;

        case 'x':
          filterVarMin[3] = atof(&argv[i][2]);   /* minimum value of filter parameter 4 */
          break;
        case 'X':
          filterVarMax[3] = atof(&argv[i][2]);   /* maximum value of filter parameter 4 */
          break;

        default:
          fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }

  return;
}


/************************************************************/
/** Checks if neutron complies with combination of filters **/
/************************************************************/
int CheckFilter(Neutron* pNeutron)
{

  double filterValue[4]={0.0,0.0,0.0,0.0};
  short  bPass[4]={UNUSED,UNUSED,UNUSED,UNUSED},
         rc=FALSE;
  int i;

  // Determine individual pass conditions
  for (i = 0; i < 4; i++) 
  {
    if (filterParam[i] != NO_PAR) 
    { filterValue[i] = DetermineParameter(filterParam[i], pNeutron);
      if (filterValue[i] >= filterVarMin[i] && filterValue[i] <= filterVarMax[i]) 
        bPass[i]=TRUE;  
      else
        bPass[i]=FALSE;
    }
  }  

 // check combination
  switch (filterComb) 
  {
    case OR_OR_OR:    if (bPass[0]==TRUE  || bPass[1]==TRUE  || bPass[2]==TRUE  || bPass[3]==TRUE)  
                        rc=TRUE;
                      else 
                        rc=FALSE;
                      break;
    case AND_AND_AND: if (bPass[0]==FALSE || bPass[1]==FALSE || bPass[2]==FALSE || bPass[3]==FALSE)
                        rc=FALSE;
                      else
                        rc=TRUE;
                      break;
    case AND_OR_AND:  if (bPass[0]==TRUE && bPass[1]==TRUE  ||  bPass[2]==TRUE  && bPass[3]==TRUE)
                        rc=TRUE;
                      else
                        rc=FALSE;
                      break;
    default      : Error("Filter combination not defined");
  }

  return rc;
}


/*******************************************************/
/** Returns the parameter value identified by 'id'    **/
/*******************************************************/
double DetermineParameter(int id, Neutron* pNeut)
{
  double paramValue = 0;
  double divy = 0;
  double divz = 0;
  
  MathVector neutronVector        (pNeut->Vector[0],   pNeut->Vector[1],   pNeut->Vector[2]);
  MathVector neutronPosition      (pNeut->Position[0], pNeut->Position[1], pNeut->Position[2]);
  MathVector neutronPositionProjYZ(pNeut->Position[1], pNeut->Position[2], 0);
  
  switch (id) 
  {
    case POS_X:
      paramValue = pNeut->Position[0]; // y-pos
      break;
    case POS_Y:
      paramValue = pNeut->Position[1]; // y-pos
      break;
    case POS_Z:
      paramValue = pNeut->Position[2]; // z-pos
      break;
    
    case DIV_Y:
      paramValue = neutronVector.Phi()*180./M_PI; // y divergence
      break;
    case DIV_Z:
      paramValue = atan(neutronVector.x[2]/neutronVector.x[0])*180./M_PI;     // z divergence
      break;
    
    case LAMBDA:
      paramValue = pNeut->Wavelength; // wavelength
      break;
    case ENERGY:
      paramValue = ENERGY_FROM_LAMBDA(pNeut->Wavelength);  //energy
      break;
    case TIME:
      paramValue = pNeut->Time; // time
      break;
    
    case K_Y:
      divy = neutronVector.Phi();
      paramValue = divy * 2. * M_PI / pNeut->Wavelength; // ky: y component of the wave vector 
      break;
    case K_Z:
      neutronVector.x[1] = 0;
      if (neutronVector.x[2] > 0) divz = M_PI/2. - neutronVector.Theta(); 
      else divz = M_PI/2. - (neutronVector.Theta() + M_PI);
      paramValue = divz * 2. * M_PI / pNeut->Wavelength;  // kz: z component of the wave vector
      break;
    
    case POS_R:
      neutronPosition.x[0] = 0;
      paramValue = neutronPosition.Mod(); // r: projection of the neutron vector on the y-z plane
      break;
    case POS_PHI:
      // phi angle of the r-phi cylindrical coordinate system corresponding to the y-z plane
      paramValue = neutronPositionProjYZ.Phi()*180./M_PI; 
      break;
    
    case DIR_PHI:  
      paramValue = neutronVector.PhiSc()*180./M_PI;
      break;
    case DIR_THETA:  
      paramValue = neutronVector.ThetaSc()*180./M_PI;
      break;

    case COL_VERT:
      paramValue = (pNeut->Color %100); //  colorTB: number of reflections at top or bottom plane
      break;
    case COL_HOR:
      paramValue = (pNeut->Color - (pNeut->Color%100) ) / 100;//  colorLR: number of reflections at left or right plane
      break;
    case COLOR:
      paramValue = (pNeut->Color - (pNeut->Color%100) ) / 100 + (pNeut->Color %100); // color: number of reflections (colorTB+colorLR)
      break;
    
    default:
      fprintf(LogFilePtr,"Parameter not (yet) treated in the filter module: ID: %d\n", id);
      exit(-1);
      break;
  }

  return paramValue;
}
