/*********************************************************************************************/
/*  VITESS module  space                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Mar 2002  K. Lieutenant   initial version                                            */
/* 1.1  May 2008  K. Lieutenant   improvements for length = 0.0                              */
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

long   ntfs=0, count, k;    	      
double VelocityReal, N_Wavelength, mu, prob=0.0,
       Length=0.0;          /* distance to end of free flight path along x-axis [cm]    */
Plane  Endpoint;            /* plane vertical to x-axis through end of free flight path */
  


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	long  i;

	double TimeOF,AveTimeOF;
	double CenterX, CenterY, CenterZ, SumProb;

	/* initialisation */
	Init(argc, argv, VT_SPACE);
	print_module_name("Space 1.1");
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
			
			if (InputNeutrons[i].Vector[0]  <= 0.0) continue;
			if (InputNeutrons[i].Wavelength <= 0.0) continue;

			if (fabs(Length) > 0.0)
			{	
				VelocityReal = (double)(V_FROM_LAMBDA(InputNeutrons[i].Wavelength)); 
				
				if (keygrav == 1)
				{
					TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
				}
				else
				{
					TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
				}
				InputNeutrons[i].Time += (double)TimeOF;
			}

			/*************************************************************************/
			/* Calculate center of beam  and  writeout new data set                  */
			/*************************************************************************/

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


	Cleanup(Length,0.0,0.0, 0.0,0.0);
	

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
					Length = atof(&argv[i][2]);
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
	Endpoint.D = -Length;
	fprintf(LogFilePtr,"Distance between entrance and exit plane: %8.3f  cm \n", Length);
}

  

	    

      
 



      



