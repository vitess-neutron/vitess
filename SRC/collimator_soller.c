
/********************************************************************************************/
/*  VITESS module collimator                                                                */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Sep  1999  D. Wechsler    initial version                                           */
/* 1.1  Jan  2004  K. Lieutenant  changes for 'instrument.dat'                              */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>



#include "init.h"
#include "softabort.h"
#include "general.h"




 int main(int argc, char *argv[])
{


	long	i,j, kind, NoAngles;
	long	BufferIndex;
	long    Transmit;
	double  efficiency, 
	        CollimatorDivergence=0.0, 
	        CollimProb=0.0, 
	        anglemin  =0.0, 
	        anglespacing;
	double  Angle[10000], SollerInput;
	Neutron Output;


	/* initilisation */
	NoAngles    =  1;
	BufferIndex =  0;
	kind        =  0;
	efficiency  =1.0;
	anglespacing=0.0;

	Init(argc, argv, VT_COLLIMATOR);
	print_module_name("collimator 1.1");


	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') {

			switch(argv[i][1])
			{
				case 'k':
					kind = atol(&argv[i][2]); 	/* kind =1 ==> angular collimation is activated */
					break;

				case 'd':
					CollimatorDivergence = atof(&argv[i][2]); /* allowed divergence FWHM[deg]; always with respect to y-direction*/
					CollimatorDivergence *=M_PI/180.0;
					break;

				case 'a':
					anglespacing = atof(&argv[i][2]); /* angular distance between collimation channels, only needed for the case of angular collimation [deg]*/
					anglespacing *=M_PI/180.0;
					break;

				case 'm': 
					anglemin = atof(&argv[i][2]); /*minimum of angle range [deg]*/
					anglemin *=M_PI/180.0;
					break;

				case 'n': 
					NoAngles = atol(&argv[i][2]); /* number of collimation channels = Angle grid */
					break;

				case  'e':
					efficiency = atof(&argv[i][2]); /* maximal probability for passing through the collimator (considers effectively the blocking due to width of collimator spacers */
					break;

				default:
				  fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
				  exit(-1);
				  break;
			}
		}
	}

	/*initialisation */
	if(kind==1)
	{	Angle[0] = anglemin;
		for(i=1; i<NoAngles; i++)
		{	Angle[i] = Angle[i-1] + 2.0*CollimatorDivergence +anglespacing;}
	}
	else
	{	NoAngles=1; Angle[0]=0.0;
	}

	DECLARE_ABORT;

	while(ReadNeutrons()!= 0)
	{
		CHECK;
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK;

			SollerInput = (double) atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]);

			Transmit=FALSE;

			for(j=0;j<NoAngles; j++)
			{
				if(fabs(Angle[j]-SollerInput) < CollimatorDivergence)
				{ 	CollimProb = efficiency*(1.0 - (fabs(Angle[j]-SollerInput) / CollimatorDivergence));
					Transmit=TRUE;
					break;
				}
			}

			if(Transmit) 
			{	InputNeutrons[i].Probability*=CollimProb;
			}
			else 
			{	continue;
			}

			Output = InputNeutrons[i];

			WriteNeutron(&Output);
		}
	}

my_exit:
	stPicture.dWPar   = CollimatorDivergence;
	stPicture.dHPar   = anglemin;
	stPicture.dRPar   = anglespacing;
	stPicture.eType   = (short) kind;
	stPicture.nNumber = NoAngles;
	Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return(0);
}
