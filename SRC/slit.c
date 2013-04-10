/*********************************************************************************************/
/*  VITESS module  slit                                                                      */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Dec 2006  K. Lieutenant   initial version                                            */
/* 1.1  Jan 2012  K. Lieutenant   visualization                                              */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit(int argc, char *argv[]);


/******************************/
/** Global Variables         **/
/******************************/

Plane  Endpoint;                /* Endpoint.D: distance to end of free flight path along x-axis [cm] */
double VelocityReal,            /* velocity of the neutron                    */
       Width=0.0, Height=0.0,   /* width and Height of the (rectangular) slit */
       DistMove;                /* distance between starting point and slit   */

/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	long  i;

	double TimeOF,               /* time of flight of the neutron to the window */
	       NewPosY, NewPosZ;     /* hor. and vert. position of neutron at slit  */

	/* initialisation */
        bVisInstalled = TRUE;

	Init(argc, argv, VT_SLIT);
	print_module_name("Slit 1.1");
	OwnInit(argc, argv);

	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK

			/*************************************************************************/
			/* 	Move neutron to end of space and calculate Time of Flight (ms).    */
			/*************************************************************************/
			
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


			/*************************************************************************/
			/* Calculate  and  writeout new data set, if slit is hit                 */
			/*************************************************************************/

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

 my_exit:
	fprintf(LogFilePtr, "Window of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", 
	                    Width, Height, DistMove);

  // Geometry data
  if (bVisInstr)
  { stGeometry.pRectangle =calloc(1, sizeof(VtRectangle));
    stGeometry.nRectangles=1; 

    stGeometry.pRectangle[0].Width     = Width;
    stGeometry.pRectangle[0].Height    = Height;
    stGeometry.pRectangle[0].vCntr[0]  = DistMove;
    stGeometry.pRectangle[0].vCntr[1]  = 0.0;
    stGeometry.pRectangle[0].vCntr[2]  = 0.0;
    stGeometry.pRectangle[0].vNormal[0]= 1.0;
    stGeometry.pRectangle[0].vNormal[1]= 0.0;
    stGeometry.pRectangle[0].vNormal[2]= 0.0;

    stGeometry.pDescr  = "slit:cyan";
    stGeometry.eModule = VT_SLIT;
  }

	Cleanup(DistMove,0.0,0.0, 0.0,0.0);	

	return(0);
}



void  OwnInit(int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') 
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

	Endpoint.A = 1.0;
	Endpoint.B = 0.0;
	Endpoint.C = 0.0;
	Endpoint.D = -1.0*DistMove;
}

  

	    

      
 



      



