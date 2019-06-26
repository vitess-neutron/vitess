/*********************************************************************************************/
/*  VITESS module  space                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Mar 2002  K. Lieutenant   initial version                                            */
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

Plane  Endpoint;        /* Endpoint.D: distance to end of free flight path along x-axis [cm] */
long   ntfs=0, count, k;    	      
double VelocityReal, N_Wavelength , mu, prob=0.0;
  


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	long  i, BufferIndex;

	double TimeOF,AveTimeOF;
	double CenterX, CenterY, CenterZ, SumProb;

	/* initialisation */
	BufferIndex     = 0;

	Init(argc, argv, VT_SPACE);
	print_module_name("Space 1.0a");
	OwnInit(argc, argv);
	
	CenterX   = 0.0; 
	CenterY   = 0.0; 
	CenterZ   = 0.0; 
	SumProb   = 0.0;
	AveTimeOF = 0.0;

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
			/* Calculate center of beam  and  writeout new data set                  */
			/*************************************************************************/

			InputNeutrons[i].Time += (double)TimeOF;
			AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
			SumProb   += InputNeutrons[i].Probability;
			
			InputNeutrons[i].Position[0]=0.0;
			WriteNeutron(&InputNeutrons[i]);
		}
	}	

 my_exit:
	if (SumProb != 0.0)
	{
		CenterX   = CenterX/SumProb;
		CenterY   = CenterY/SumProb; 
		CenterZ   = CenterZ/SumProb; 
		AveTimeOF = AveTimeOF/SumProb;
		
		fprintf(LogFilePtr,"Center of beam at exit:  (%8.3f,%7.3f,%7.3f) cm, TOF = %8.4f ms \n",CenterX, CenterY, CenterZ, AveTimeOF);
	}
	else
	{
		fprintf(LogFilePtr,"No neutrons at the exit of this module \n");
	}

	fprintf(LogFilePtr," \n");


	Cleanup((-Endpoint.D), 0.0, 0.0, 0.0, 0.0);
	

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
					Endpoint.A = 1.0;
					Endpoint.B = 0.0;
					Endpoint.C = 0.0;
					Endpoint.D = -atof(&argv[i][2]);
					fprintf(LogFilePtr,"Distance between entrance and exit plane: %8.3f  cm \n", -Endpoint.D);
					break;
      
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}
}

  

	    

      
 



      



