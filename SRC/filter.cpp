/********************************************************************************************/
/*  VITESS module 'filter.cpp'                                                                */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Apr 2013  D. Nekrassov  initial version                                              */
/* 1.1 Nov 2013  D. Nekrassov  "OR" mode bug fixed                                          */
/********************************************************************************************/


#include <math.h>

extern "C" {
#include "init.h"
#include "softabort.h"
#include "general.h"
}

#include "filter.h"

int main(int argc, char *argv[])
{
 
  long	i , registered, BufferIndex;
  
  BufferIndex = 0;
  registered=0;

  /*input*/
  Init(argc, argv, VT_FILTER);
  print_module_name("filter 1.0");

  
  OwnInit(argc, argv);

 
DECLARE_ABORT;

  while(ReadNeutrons()!= 0)
  {
  CHECK;
  for(i=0; i<NumNeutGot; i++)
	{
      CHECK;
      // registered = 1 if neutron was considered in the monitor
      registered=CheckFilter(&InputNeutrons[i]);
	  
	  if(registered) {
	      WriteNeutron(&(InputNeutrons[i]));
	  }
    }
  }
my_exit:


  Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return(0);
}


void OwnInit(int argc, char *argv[])
{

  filterVarMin[0] = -1;
  filterVarMin[1] = -1;
  filterVarMin[2] = -1;
  filterVarMax[0] = -1;
  filterVarMax[1] = -1;
  filterVarMax[2] = -1;

  filterParam[0] = -1;
  filterParam[1] = -1;
  filterParam[2] = -1;
  filterComb = -1;

  // Read the command line arguments
  for(int i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
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

	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;
	  }
      }
    }

 return;

}



int CheckFilter(Neutron* n)
{

  double filterValue[3];
  int i;
  
  
  // Dismiss if outside the range of filter parameter i, if defined (independent of other two filters: combined with AND)
  for (i = 0; i < 3; i++) {
    filterValue[i] = 0;
    if (filterParam[i] > 0 && (filterParam[(i+1)%3] <= 0 || filterComb==1) && (filterParam[(i+2)%3] <= 0 || filterComb==1)) {
      filterValue[i] = DetermineParameter(filterParam[i], n);
      if (filterValue[i] < filterVarMin[i] || filterValue[i] > filterVarMax[i]) return 0;  
   }
  }  

 // pass if fulfilled 1 OR 2 OR 3
  if (filterComb==0) {
    for (i = 0; i < 3; i++) {
      if (filterParam[i] > 0) {
	filterValue[i] = DetermineParameter(filterParam[i], n);
	if  (filterValue[i] >= filterVarMin[i] && filterValue[i] < filterVarMax[i]) return 1;
      }
    }
  }
  
  if (filterParam[0] > 0 || filterParam[1] > 0 || filterParam[2] > 0) return 0;
  else return 1;

}


double DetermineParameter(int id, Neutron* n)
{

  // Return the parameter value identified by 'id'

  double paramValue = 0;
  double divy = 0;
  double divz = 0;
  
  MathVector neutronVector (n->Vector[0], n->Vector[1], n->Vector[2]);
  MathVector neutronPosition (n->Position[0], n->Position[1], n->Position[2]);
  MathVector neutronPositionProjYZ (n->Position[1], n->Position[2], 0);
  

  switch (id) {
  case 1:
    paramValue = n->Position[1]; // y-pos
    break;
    
  case 2:
    paramValue = n->Position[2]; // z-pos
    break;
    
  case 3:
    paramValue = neutronVector.Phi()*180./M_PI; //y divergence
    break;
    
  case 4:
    neutronVector.x[1] = 0;
    if (neutronVector.x[2] > 0) paramValue = 90. - (neutronVector.Theta()*180./M_PI); //z divergence
    else paramValue = 90. - (neutronVector.Theta()*180./M_PI +180.);
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
