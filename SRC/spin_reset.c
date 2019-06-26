/*********************************************************************************************/
/* VITESS module spin_reset                                                                  */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */

/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"


void  OwnInit(int argc, char *argv[]);
void  OwnCleanup();

double PolVecX     =  0.0,  /* polarisation            */
       PolVecY     =  0.0, 
       PolVecZ     =  0.0, 
       PolDegree   =  0.0,  /* degree of polarization [%] */
       FracPolDir  =  0.0;  /* fraction of neutrons in polarization direction */


int main(int argc, char **argv)
{
  int i;

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_WRITEOUT);
  print_module_name("spin_reset 0.1");

  /* module specific initialization */
  OwnInit(argc, argv);
  
  /* redefinition in terms of eigenvectors e.g. 0 % means 50% Up and 50% Down */
  FracPolDir  = 0.5 + 0.5*PolDegree/100.0;
 
  /* Get the neutrons from the file */
  DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
  {
    CHECK;    
    for(i=0; i<NumNeutGot; i++) 
    {
	/* Polarization - spin vectors selected for each trajectory 
	   from one of the eigenvectors  in the polarisation direction */
		if (Vran() <= FracPolDir) 
		{	/* spin eigenvector No 1 */
			InputNeutrons[i].Spin[0]= PolVecX; 
			InputNeutrons[i].Spin[1]= PolVecY; 
			InputNeutrons[i].Spin[2]= PolVecZ; 
		}
		else
		{	/* spin eigenvector No 2 */
			InputNeutrons[i].Spin[0]= -PolVecX; 
			InputNeutrons[i].Spin[1]= -PolVecY; 
			InputNeutrons[i].Spin[2]= -PolVecZ; 
		} 
             
      WriteNeutron(&(InputNeutrons[i]));
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
  int i;

	// arg=&argv[i][2];

  for(i=1; i<argc; i++) 
  { if(argv[i][0]!='+') 
    { switch(argv[i][1])
      { 
	      /*polarization*/
	    case 'X':
	      PolVecX = atof(&argv[i][2]);  
	      break;
	    case 'Y':
	      PolVecY = atof(&argv[i][2]);  
	      break;
	    case 'Z':
	      PolVecZ = atof(&argv[i][2]);  
	      break;
	    case 'P':
	      PolDegree = atof(&argv[i][2]); 
	      if(fabs(PolDegree) > 100.)
           Error("polarization degree must be <= 100 ");
	      break;
       default:
          fprintf(LogFilePtr,"ERROR: unkown command option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
  if (PolVecX==0.0 && PolVecY==0.0 && PolVecZ==0.0)
    PolVecZ=1.0;
}


void OwnCleanup()
{
  return;
}
