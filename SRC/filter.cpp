/********************************************************************************************/
/*  VITESS module 'filter.cpp'                                                              */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0  Apr 2013  D. Nekrassov  initial version                                             */
/* 1.1  Nov 2013  D. Nekrassov  "OR" mode bug fixed                                         */
/* 1.2  Feb 2015  K. Lieutenant "AND OR AND" added                                          */
/* 1.3  Mar 2020  K. Lieutenant  new central visualization parameters, UNUSED -> TRUE       */
/********************************************************************************************/


#include <math.h>

extern "C" {
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

  // reading of input data and initilisation
  // ---------------------------------------
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, "1.3");
  OwnInit(argc, argv);
 
  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

  DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK;
      
      // bRregistered = TRUE if neutron was considered in the monitor
      bRegistered=CheckFilter(&InputNeutrons[i]);
	  
      if (bRegistered) 
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
    }
  }

// Finish: writes to log and instrument file, frees memory
// -------------------------------------------------------
my_exit:
  // writes to instrument and log file
  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void OwnInit(int argc, char *argv[])
{
  filterVarMin[0] = -1;
  filterVarMin[1] = -1;
  filterVarMin[2] = -1;
  filterVarMin[3] = -1;
  filterVarMax[0] = -1;
  filterVarMax[1] = -1;
  filterVarMax[2] = -1;
  filterVarMax[3] = -1;

  filterParam[0] = -1;
  filterParam[1] = -1;
  filterParam[2] = -1;
  filterParam[3] = -1;
  filterComb = -1;

  // Read the command line arguments
  for(int i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
	    switch(argv[i][1])
	    {
	      case 'I':  
	        filterParam[0] = atoi(&argv[i][2]); // filter parameter 1, input parameter
	        break;
	      case 'J':  
	        filterParam[1] = atoi(&argv[i][2]); // filter parameter 2, optional input parameter
	        break;
	      case 'K':  
	        filterParam[2] = atoi(&argv[i][2]); // filter parameter 3, optional input parameter
	        break; 
	      case 'L':  
	        filterParam[3] = atoi(&argv[i][2]); // filter parameter 4, optional input parameter
	        break; 

	      case 'C':  
	        filterComb = atoi(&argv[i][2]); // filter combination (AND,OR)
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
int CheckFilter(Neutron* n)
{

  double filterValue[4]={0.0,0.0,0.0,0.0};
  short  bPass[4]={UNUSED,UNUSED,UNUSED,UNUSED},
         rc=FALSE;
  int i;

  // Determine individual pass conditions
  for (i = 0; i < 4; i++) 
  {
    if (filterParam[i] > 0) 
    { filterValue[i] = DetermineParameter(filterParam[i], n);
      if (filterValue[i] >= filterVarMin[i] && filterValue[i] <= filterVarMax[i]) 
        bPass[i]=TRUE;  
      else
        bPass[i]=FALSE;
    }
    else
    { bPass[i]=TRUE; 
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
double DetermineParameter(int id, Neutron* n)
{
  double paramValue = 0;
  double divy = 0;
  double divz = 0;
  
  MathVector neutronVector (n->Vector[0], n->Vector[1], n->Vector[2]);
  MathVector neutronPosition (n->Position[0], n->Position[1], n->Position[2]);
  MathVector neutronPositionProjYZ (n->Position[1], n->Position[2], 0);
  
  switch (id) 
  {
    case 1:
      paramValue = n->Position[1]; // y-pos
      break;
    
    case 2:
      paramValue = n->Position[2]; // z-pos
      break;
    
    case 3:
      paramValue = neutronVector.Phi()*180./M_PI; // y divergence
      break;
    
    case 4:
      paramValue = atan(neutronVector.x[2]/neutronVector.x[0])*180./M_PI;     // z divergence
      break;
    
    case 5:
      paramValue = n->Wavelength; // wavelength
      break;
    
    case 6:
      paramValue = ENERGY_FROM_LAMBDA(n->Wavelength);  //energy
      break;
    
    case 7:
      paramValue = n->Time; // time
      break;
    
    case 8:
      divy = neutronVector.Phi();
      paramValue = divy * 2. * M_PI / n->Wavelength; // ky: y component of the wave vector 
      break;
    
    case 9:
      neutronVector.x[1] = 0;
      if (neutronVector.x[2] > 0) divz = M_PI/2. - neutronVector.Theta(); 
      else divz = M_PI/2. - (neutronVector.Theta() + M_PI);
      paramValue = divz * 2. * M_PI / n->Wavelength;  // kz: z component of the wave vector
      break;
    
    case 10:
      neutronPosition.x[0] = 0;
      paramValue = neutronPosition.Mod(); // r: projection of the neutron vector on the y-z plane
      break;
    
    case 11:
      // phi angle of the r-phi cylindrical coordinate system corresponding to the y-z plane
      paramValue = neutronPositionProjYZ.Phi()*180./M_PI; 
      break;

    case 12:
      paramValue = (n->Color %100); //  colorTB: number of reflections at top or bottom plane
      break;

    case 13:
      paramValue = (n->Color - (n->Color%100) ) / 100;//  colorLR: number of reflections at left or right plane
      break;

    case 14:
      paramValue = (n->Color - (n->Color%100) ) / 100 + (n->Color %100); // color: number of reflections (colorTB+colorLR)
      break;
    
    default:
      fprintf(LogFilePtr,"unknown parameter ID: %d\n", id);
      exit(-1);
      break;
  }

  return paramValue;
}
