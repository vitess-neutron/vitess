/*********************************************************************************************/
/*  VITESS module 'slit'                                                                     */
/*                                                                                           */
/* This module simulates a rectangular aperture                                              */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Dec 2006  K. Lieutenant   initial version                                            */
/* 1.1  Jan 2012  K. Lieutenant   visualization                                              */
/* 1.2  Jul 2019  K. Lieutenant   new central visualization parameters                       */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and initializes global variables
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
double Width=0.0,               // -W   [cm]  width of the (rectangular) slit 
       Height=0.0,              // -H   [cm]  height of the (rectangular) slit 
       DistMove=0.0;            // -d   [cm]  distance between starting point and slit 

Plane  Endpoint;                //      [cm]  Endpoint.D: distance to end of free flight path along x-axis [cm]


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
	long  i=0;

	double VelocityReal=0.0,          // velocity of the neutron    
         TimeOF=0.0,                // time of flight of the neutron to the window 
	       NewPosY=0.0, NewPosZ=0.0;  // hor. and vert. position of neutron at slit 

  // initialisation
  // --------------
  _eModule = MCN_SLIT;

	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.2");
	OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = TRUE;

	DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
	while (ReadNeutrons()!= 0)
	{
		for (i=0; i<NumNeutGot; i++)
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
			  if (InputNeutrons[i].Vector[0] <= 0.0) continue;
			  if (InputNeutrons[i].Wavelength == 0.0) continue;
			  VelocityReal = (double)(V_FROM_LAMBDA(InputNeutrons[i].Wavelength)); 
			  if (VelocityReal <= 0.0) continue;
			
			  if (keygrav == 1)
			  {
				  TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
			  }
			  else
			  {
				  TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
			  }


			  // Calculate  and  writeout new data set, if slit is hit
			  // -----------------------------------------------------
			  NewPosY = InputNeutrons[i].Position[1];
			  NewPosZ = InputNeutrons[i].Position[2];
			
			  if (fabs(NewPosY) < 0.5*Width  &&  fabs(NewPosZ) < 0.5*Height)
			  {	
          WriteIAP(&InputNeutrons[i], VT_PASSED);

				  InputNeutrons[i].Time += (double)TimeOF;
				  InputNeutrons[i].Position[0]=0.0;

				  WriteNeutron(&InputNeutrons[i]);
			  }
        else
        { WriteIAP(&InputNeutrons[i], VT_OUT_OF_WND);
        }
      }
		}
	}	

// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
	fprintf(LogFilePtr, "Window of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", 
	                    Width, Height, DistMove);

  SetGeometry("blue");                       // write geometry data for visualization
  Cleanup(DistMove, 0.0, 0.0, 0.0, 0.0);     // print intensity, write instrument.inf, free memory

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
	int i=0;

  InitPlane(&Endpoint);

	for (i=1; i<argc; i++)
	{
		if (argv[i][0]!='+') 
		{
			switch(argv[i][1])
      {
				case 'd':
					DistMove = atof(&argv[i][2]);
					break;

				case 'W':
					Width  = atof(&argv[i][2]);
					break;
				case 'H':
					Height = atof(&argv[i][2]);
					break;
      
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}

	Endpoint.A =  1.0;
	Endpoint.D = -1.0*DistMove;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the slit geometry
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.pRectangle = calloc(1, sizeof(VtRectangle));
    stGeometry.nRectangles = 1;

    stGeometry.pRectangle[0].Width    = Width;
    stGeometry.pRectangle[0].Height   = Height;
    stGeometry.pRectangle[0].vCntr[0] = DistMove/CmprFact;
    stGeometry.pRectangle[0].vCntr[1] = 0.0;
    stGeometry.pRectangle[0].vCntr[2] = 0.0;
    stGeometry.pRectangle[0].vNormal[0] = 1.0;
    stGeometry.pRectangle[0].vNormal[1] = 0.0;
    stGeometry.pRectangle[0].vNormal[2] = 0.0;
  }

  return;
}


	    

      
 



      



