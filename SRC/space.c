/*********************************************************************************************/
/*  VITESS module 'space'                                                                    */
/*                                                                                           */
/* This module simulates neutron propagation incl. attenuation over a certain distance       */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Mar 2002  K. Lieutenant   initial version                                            */
/* 1.1  May 2008  K. Lieutenant   improvements for length = 0.0                              */
/* 1.2  Sep 2009  K. Lieutenant   attenuation included                                       */
/* 1.3  Aug 2012  K. Lieutenant   visualization included                                     */
/* 1.4  Jul 2019  K. Lieutenant   new central visualization parameters                       */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);   // Reads input parameters and sets global parameters


/******************************/
/** Global Variables         **/
/******************************/
double Length=0.0,        // -d   [cm]   /* distance to end of free flight path along x-axis  [cm]   */
       MuScat=0.0,        // -M  [1/cm]  /* macroscopic scattering coeff.                    [1/cm]  */
       MuAbs=0.0;         // -m  [1/cm]  /* macroscopic absorption coeff.                    [1/cm]  */
                       
Plane  Endpoint;          //      [cm]   /* plane vertical to x-axis through end of free flight path */  


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
  double VelocityReal=0.0;        // speed of the neutron  [km/s] 
  long  i=0;

  double TimeOF=0.0,  AveTimeOF=0.0;
  double CenterX=0.0, CenterY=0.0, CenterZ=0.0, SumProb=0.0;

  // Initialisation
  // --------------
  _eModule = MCN_SPACE;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.4");
  OwnInit(argc, argv);

  bVisInstalled = FALSE;
  if (bVisInstr)
  { bBlowUp     = TRUE;
    stGeometry.pDescr = "space";
  }

  DECLARE_ABORT

  // Loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // 	Move neutron to end of space and calculate Time of Flight (ms)
        // ---------------------------------------------------------------
        if (InputNeutrons[i].Vector[0]  <= 0.0) continue;
        if (InputNeutrons[i].Wavelength <= 0.0) continue;

        if (fabs(Length) > 0.0)
        {	
          VelocityReal = V_FROM_LAMBDA(InputNeutrons[i].Wavelength); 
				
          if (keygrav == 1)
          {
            TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
          }
          else
          {
            TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
          }
          InputNeutrons[i].Time += TimeOF;
        }

        // Calculate center of beam  and  writeout new data set
        // ----------------------------------------------------
        InputNeutrons[i].Probability*=exp(-(MuScat+MuAbs*InputNeutrons[i].Wavelength/1.798)*Length);

        AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
        CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
        CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
        CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
        SumProb   += InputNeutrons[i].Probability;
			
        WriteIAP(&InputNeutrons[i], VT_EXITED);
        InputNeutrons[i].Position[0]=0.0;

        WriteNeutron(&InputNeutrons[i]);
      }
    }
  }	

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
my_exit:
  /* Writeout */
  if (SumProb != 0.0)
  {
    CenterX   = CenterX/SumProb;
    CenterY   = CenterY/SumProb; 
    CenterZ   = CenterZ/SumProb; 
    AveTimeOF = AveTimeOF/SumProb;
		
    fprintf(LogFilePtr,"Center of beam at exit:  (%8.3f,%7.3f,%7.3f) cm, TOF = %8.4f ms \n\n", CenterX, CenterY, CenterZ, AveTimeOF);
  }
  else
  {
    fprintf(LogFilePtr,"No neutrons at the exit of this module \n\n");
  }
  
  /* Do the general cleanup */
  Cleanup(Length,0.0,0.0, 0.0,0.0);
	
  return(0);
}


/***************************************************************/
/* OwnInit: Reads input parameters and sets global parameters  */
/***************************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i=0;

  InitPlane(&Endpoint);

  for(i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1])
      {
        case 'd':
          Length = atof(&argv[i][2]); /* distance to fly  */
          break; 

        case 'M':
          MuScat = atof(&argv[i][2]); /* macroscopic scattering coeff. in 1/cm */
          break;
        case 'm':
          MuAbs  = atof(&argv[i][2]); /* macroscopic absorption coeff. in 1/cm */
          break;
      
        default:
          fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
          exit(-1);
      }
    }
  }

  Endpoint.A = 1.0;
  Endpoint.D = -Length;

  fprintf(LogFilePtr,"Distance between entrance and exit plane: %8.3f  cm \n", Length);

  return;
}

  

	    

      
 



      



