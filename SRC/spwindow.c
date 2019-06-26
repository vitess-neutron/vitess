/*********************************************************************************************/
/*  VITESS module  spacewindow                                                               */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*       June 1999  D. Wechsler                                                              */
/* 1.00  Feb  2001  S. Manoshin     include of gravity effect                                */	
/* 1.01  June 2001  K. Lieutenant   parameter S to simulate a beamstop + SOFTABORT           */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation                                           */
/* 2.00  Jun  2003  S. Manoshin     Add possibility for simulations of material of           */      
/*				    collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, 
 */
/*	      			    3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber        */
/*                                                                                           */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"


/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit(int argc, char *argv[]);


/******************************/
/** Global Variables         **/
/******************************/


long    bCircularWindow, /* criterion: kind of window, TRUE: circular, FALSE rectangular   */
	    bBeamStop;       /* criterion: beamstop        TRUE: beamstop, FALSE normal window */

Plane   Endpoint;        /* Endpoint.D: distance to window along x-axis            [cm] */
double  heightmin,       /* z-coordinate: bottom of rectangular window             [cm] */
        heightmax,       /* z-coordinate: top of rectangular window                [cm] */
        widthmin,        /* y-coordinate: lower frame value of rectangular window  [cm] */
        widthmax,        /* y-coordinate: higher frame value of rectangular window [cm] */
        winradius,       /* radius of circular window                              [cm] */
        ywincenter,      /* y coordinate: center of circular window                [cm] */
        zwincenter;      /* y coordinate: center of circular window                [cm] */
  
        
  long  keymaterial0=6; /*Material of collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, 
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
  long ntfs=0, count, k;    	      
  double WAVS[500], MUS[500], transm0[1000];
  char		*TransFileName0=NULL;
  FILE	*trans_file0=NULL; /* file for describing of transmission of material of collimator  */
       
  double VelocityReal, N_Wavelength , mu, prob=0.0, TimeOFmin, Thicknesscoll=1.0;
  
        


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	short bOutOfWindow=FALSE;

	long  i, BufferIndex;

	double Distance, tempdistsquared;
	double TimeOF,AveTimeOF;
	double NewPositionY, NewPositionZ;
	double CenterX, CenterY, CenterZ, SumProb;

	Neutron Output;


	/* initialisation */
	heightmin = heightmax = widthmin = widthmax = 0.0;
	winradius = ywincenter = zwincenter = 0.0;

	BufferIndex     = 0;
	bCircularWindow = TRUE,
	bBeamStop       = FALSE;

	Init   (argc, argv);
	OwnInit(argc, argv);

	print_module_name("Space and Window 2.00");
	
	if (TransFileName0 != NULL) trans_file0 = fopen(TransFileName0,"r");
	
	if (Thicknesscoll <= 0.0)
	{
		fprintf(LogFilePtr,"Thickness of collimator <= 0.0 !!!");
		exit(-1);
	}
	
	if (keymaterial0 == 0)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Material transmission characteristics reading from file\n");
  	}

  	if (keymaterial0 == 1)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Gadolinium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
  	}
  	
  	if (keymaterial0 == 2)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Cadmium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 3)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Bor10 \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 4)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Eu \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 5)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: Silicon \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 6)
  	{
	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: IDEAL ABSORBER \n");
  	}
  	
	if ((keymaterial0 != 0)&&(keymaterial0 != 1)&&(keymaterial0 != 2)&&(keymaterial0 != 3)&&(keymaterial0 != 4)&&(keymaterial0 != 5)&&(keymaterial0 != 6))
  	{
  	    fprintf(LogFilePtr,"MATERIAL OF COLLIMATOR: No material: EXIT! Correct -c option \n");
	    exit(-1);
  	}


  
	  for(i=0; i<500; i++)
	  {	
		WAVS[i] = 0.0;
		MUS[i] = 0.0;
	  }
	  
	  for(i=0; i<1000; i++)
	  {	
		transm0[i] = 0.0;
	  }
	  
  


    if (keymaterial0 == 0)
    {
      /* Read transmission file for collimator */

      if (TransFileName0 !=NULL)
        {
          for(count=1; count<=1000; count++)
    	    {
    		if (fscanf(trans_file0,"%lf",&transm0[count])==EOF)
		    break;
	    }

    	  fclose(trans_file0);
	
	    k = 1;
	    ntfs = (long)((count-1)/2);
    	    for(i = 1; i <= ntfs; i++)
	    {
		WAVS[i] = transm0[k];
		MUS[i] = transm0[k+1];
		k = k + 2;
	    }
	    
	/* check the input data */	    
	    for(i = 1; i <= (ntfs-1); i++)
	    {
		if (WAVS[i+1] <= WAVS[i]) 
		{
		    fprintf(LogFilePtr,"MISTAKE: incorrect data in transmission file of the collimator \n");
		    fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
		    exit(-1);
		}    
	    }
	    
//	    fprintf(LogFilePtr,"transmfile for surface count = %d  num = %d \n", count, ntfs);
//	    for(i = 1; i <= ntfs; i++)
//	    fprintf(LogFilePtr," %f   %f  \n",WAVS[i], MUS[i]);
	    fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAVS[1], WAVS[ntfs]);
	
      }		
      else
	fprintf(LogFilePtr,"No file, which description transmission of collimator \n");
    }	

	
	
	if(keygrav == 1)
	{
		fprintf(LogFilePtr,"Gravity is enabled \n");
	}
	else
	{
		fprintf(LogFilePtr,"Gravity is disabled \n");
	}

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

			/****************************************************************************************/
			/* 	Move neutron to window with gravity effect and calculate Time of Flight (ms).   */
			/****************************************************************************************/
			
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


			/****************************************************************************************/
			/* Calculate the distance to this intercept.                                            */
			/****************************************************************************************/
			/*Distance = sqrt( (NewPosition.X-InputNeutrons[i].Position[0])
									*(NewPosition.X-InputNeutrons[i].Position[0])
									+(NewPosition.Y-InputNeutrons[i].Position[1])
									*(NewPosition.Y-InputNeutrons[i].Position[1])
									+(NewPosition.Z-InputNeutrons[i].Position[2])
									*(NewPosition.Z-InputNeutrons[i].Position[2])
									); */

			/****************************************************************************************/
			/* Calculate the time needed to travel this distance.                                   */
			/****************************************************************************************/
			/*	DeltaTime = Distance/V_FROM_LAMBDA(InputNeutrons[i].Wavelength); */ 

			InputNeutrons[i].Time += (double)TimeOF;
			AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
			SumProb   += InputNeutrons[i].Probability;

			TimeOFmin = Thicknesscoll/((VelocityReal)*(InputNeutrons[i].Vector[0]));
			
			/* window test */
				
			NewPositionY = InputNeutrons[i].Position[1];
			NewPositionZ = InputNeutrons[i].Position[2];

			if(bCircularWindow==TRUE) 
			{	tempdistsquared =  (NewPositionY - ywincenter)*(NewPositionY - ywincenter)
				                 + (NewPositionZ - zwincenter)*(NewPositionZ - zwincenter);
				if (winradius*winradius < tempdistsquared)
					bOutOfWindow=TRUE;
				else
					bOutOfWindow=FALSE;
			}
			else 
			{	if((widthmin  > NewPositionY) || (widthmax < NewPositionY) || 
				   (heightmin > NewPositionZ) || (heightmax < NewPositionZ)  )
					bOutOfWindow=TRUE;
				else
					bOutOfWindow=FALSE;
			}
			
			/* ok, if out of beamstop or in window */
			if (bBeamStop==TRUE  && bOutOfWindow==TRUE ||
			    bBeamStop==FALSE && bOutOfWindow==FALSE)
			{
				if (bBeamStop==FALSE)
					InputNeutrons[i].Position[0]=0.0;
				InputNeutrons[i].Time += (double)TimeOFmin;	
				Output = InputNeutrons[i];
				WriteNeutron(&Output);
			}
			else
			{   	
				if (keymaterial0 != 6)
  				{
				     /* Attenuation during pass of collimator material */
					 N_Wavelength = InputNeutrons[i].Wavelength;    
					 mu = Interpolation(N_Wavelength,keymaterial0,WAVS,MUS,ntfs);
					 prob = exp(-mu*TimeOFmin*VelocityReal);	
//	 				 fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
	 				 InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
	 				 InputNeutrons[i].Time += (double)TimeOFmin;
	 				 InputNeutrons[i].Position[0]=0.0;
					 Output = InputNeutrons[i];
				 	 WriteNeutron(&Output);
				 }	 
			}
		}
	}	

 my_exit:
	Distance = fabs(Endpoint.D);
	fprintf(LogFilePtr,"Distance between plane x=0 and window plane  %f cm \n", Distance);

	if (SumProb != 0.0)
	{
		CenterX   = CenterX/SumProb;
		CenterY   = CenterY/SumProb; 
		CenterZ   = CenterZ/SumProb; 
		AveTimeOF = AveTimeOF/SumProb;
		
		fprintf(LogFilePtr,"Center of beam: X = %f cm Y = %f cm Z = %f cm TOF = %f ms  \n",CenterX, CenterY, CenterZ, AveTimeOF);
	}
	else
	{
		fprintf(LogFilePtr,"No neutrons at the exit of this module \n");
	}

	fprintf(LogFilePtr," \n");


	Cleanup();
	

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
			case 'l':
				Endpoint.A = 1.0;
				Endpoint.B = 0.0;
				Endpoint.C = 0.0;
				Endpoint.D = -atof(&argv[i][2]);
				break;

			case 'S':
				bBeamStop = atol(&argv[i][2]);
				break;
			case 'R':
				bCircularWindow = atol(&argv[i][2]);
				break;

			case 'h':
				heightmin =  atof(&argv[i][2]);
				break;
			case 'w':
				widthmin = atof(&argv[i][2]);
				break;

			case 'H':
				heightmax =  atof(&argv[i][2]);
				break;
			case 'W':
				widthmax = atof(&argv[i][2]);
				break;

			case 'r':
				winradius = atof(&argv[i][2]);
				break;
			case 'y':
				ywincenter = atof(&argv[i][2]);
				break;
			case 'z':
				zwincenter = atof(&argv[i][2]);
				break;
				
			case 'c':
				keymaterial0 = atol(&argv[i][2]);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
      				break;
      
      				
      			case 'C':
	    			TransFileName0=&argv[i][2];
	    			break;
      				
      			
      			case 't':
				Thicknesscoll = atof(&argv[i][2]);
 /* in cm */
				break;
	
      				

			default:
				fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
				exit(-1);
				break;
			}
		}
	}
}

  

	    

      
 



      



