/**********************************************************************************************/
/*  VITESS module grid                                                      		      */
/* The free non-commercial use of these routines is granted providing due credit is given to  */
/* the authors.                                                                               */
/*                                                                                            */
/* Written by Manoshin Sergey in Jun 2003 for VSANS simulations, HMI Berlin		      */	
/* Born from module spacewindow_muliplie						      */
/* Simplified alg.									      */
/*                                                                                            */
/* 1.00  Jun  2003  S. Manoshin     initial version                                           */	
/* 				    with possibility for simulations of material of           */     
/*				    collimator: 0 - from file, 1 - gadolinium, 2 - cadmium,   */
/*	      			    3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber         */
/* 1.01  Jul  2004  S. Manoshin	    Color tracking was added				      */
/* 1.02  Aug  2004  S. Manoshin	    Add deviation of distance and shifts of collimator, in %  */
/*				    Auto calculation for gravity monochromator and collimators*/
/*				    Add reducing of grid sizes for convergent gridset system  */
/* 1.1  Oct  2004  S. Manoshin	    Add and correct the deviation of grid system	      */
/*				    DistanceDev, ShiftHorDev, ShiftVerDev, Pos and Size  hole */
/**********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"

double Interpolation(double, long, double *, double *, long);


int main(int argc, char *argv[])
{
	char	*CollFileName=NULL;
	FILE	*coll_file=NULL;

	long	i, j, k, counter;
	long	BufferIndex;
	long 	NumberOfHoles;
	long	key_abs=1; 
	long    key_outer=0; /* Form of collimators 0 - square form; 1 - circle form */
	long    key_output=0;
	
	
	long	key_colortracking=0; /* Activate color tracking, default no(0) */
	short	color_current, hole_current=0 ;
	

	double	Distance=0.0, DistanceDev=0.0; /* distance and deviation */
	
	
	/* Options for simulating the grid system: for gravity monochromator*/
	double	DistanceAbs = 0.0; /* absolute distance from first grid, 
				    for simualtion of system of grid, cm */
	double  TotalLength = 1200; /* total length of grid system, cm */			    
	
	double  WaveMonoch = 0.0; /* Wavelength (Ang) for monokhromatisation, if zero disactivated */
	
	
	double	rdate[3003];
	double  winradius[1001], ywincenter[1001], zwincenter[1001]; 
	double  winradiusdev=0.0; /* Deviation of hole of size*/
	double	wincenterdev=0.0; /* Deviation of position of hole */
	double  OuterA=0.0, OuterB=0.0, OuterRadius=0.0; /* Outer Sizes of collimator */ 
	
	double  ShiftHor=0.0, ShiftVer=0.0; /* displacement of collimator */
	double  ShiftHorDev=0.0, ShiftVerDev=0.0; /* deviation of displacement of collimator */	
	
	double  tempdistsquared;
	double  TimeOF;
	
	Plane	Endpoint, Endpointcol, Pcalc;
	double	NewPositionY, NewPositionZ;
	Neutron  Output, Ncalc;
	


 	long  keymaterial0=6; /* Material of collimator */
	  /* Material of collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, 
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
	long ntfs=0, count;    	      
	double WAVS[500], MUS[500], transm0[1000];
	char	*TransFileName0=NULL;
	FILE	*trans_file0=NULL; /* file for describing of transmission of material of collimator  */
       
	double VelocityReal, N_Wavelength , mu, prob=0.0, Thicknesscoll=0.1;


	/* Initialisation */

 	BufferIndex = 0;
	DistanceAbs = 0.0;
	WaveMonoch = 0.0;
	
	winradiusdev=0.0; 
	wincenterdev=0.0; 
	
	DistanceDev=0.0;
	ShiftHorDev=0.0; 
	ShiftVerDev=0.0;
	
	keymaterial0=6;

	/***************************************************************************/
	/* Endpoint.D                distance to window along x-direction   [cm]   */
        /***************************************************************************/

  /*input*/
  Init(argc, argv, VT_GRID);


  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {

	switch(argv[i][1])
	  {
	  case 'I':
	    if( (coll_file = fopen(&argv[i][2],"r"))==NULL)
	      {
		fprintf(LogFilePtr,"File %s could not be opened for InputNeutrons\n",&argv[i][2]);
		exit(-1);
	      }

	    CollFileName=&argv[i][2];
	    break;

	  case 'D':
	    Distance =  atof(&argv[i][2]);
	    break;
	    
	    
	  case 'M':
	    DistanceAbs =  atof(&argv[i][2]);
	    break;	    
	    
	    
	  case 'm':
	    TotalLength =  atof(&argv[i][2]);
	    break;	    	    
	    
	    
	  case 'n':
	    WaveMonoch =  atof(&argv[i][2]);
	    break;	    	    	    

	    
	  case 'X':
	    DistanceDev =  atof(&argv[i][2]);
	    break;	    
	    
	    
	  case 'a':
	    OuterA = atof(&argv[i][2]);
	    break;
	    
	  case 'b':
	    OuterB = atof(&argv[i][2]);
	    break;	    
	    
	  case 'd':
	    ShiftHor = atof(&argv[i][2]);
	    break;	    	    

	    
	  case 'y':
	    ShiftHorDev = atof(&argv[i][2]);
	    break;	    	    	    
	    
	    
	  case 'e':
	    ShiftVer = atof(&argv[i][2]);
	    break;

	    
	  case 'q':
	    ShiftVerDev = atof(&argv[i][2]);
	    break;	    	    	    
	    
	    
	  case 'h':
	    winradiusdev = atof(&argv[i][2]);
	    break;	    	    	    	    
	    
	    
	  case 'H':
	    wincenterdev = atof(&argv[i][2]);
	    break;	    	    	    	    	    
	    
	    
	  case 'k':
	    key_output = atol(&argv[i][2]); 
	    break;    	    	    

	    
	  case 'K':
	    key_colortracking = atol(&argv[i][2]); 
	    break;    	    	    	    
	    
	    	    	    	    
	  case 'N':
	    key_outer = atol(&argv[i][2]); /* Form of collimators 0 - square form; 1 - circle form */
	    break;    
	    
	  case 'c':
	    keymaterial0 = atol(&argv[i][2]);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
      	    break;
      
      				
      	  case 'C':
	    TransFileName0=&argv[i][2]; 
	    break;
      				
      	  case 't':
	    Thicknesscoll = atof(&argv[i][2]);
	    break;
		    

	    
	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;
	  }
      }
    }
    
    print_module_name("Space and Grid 1.1");
    
    
    	if (key_colortracking == 1)
	{
		fprintf(LogFilePtr, "=====TRACKING OF CROSSTALK OF TRAJECTORIES IS ACTIVATED.=====  \n");
		fprintf(LogFilePtr, "PLEASE SET INITIAL COLOR OF ALL NEUTRONS IN SOURCE ON ZERO (0) \n");
		fprintf(LogFilePtr, "IDEAL ABSORBTION IS ACTIVATED. \n");
		keymaterial0 = 6 ;
	}    
    
    	if (key_outer == 0)
    	fprintf(LogFilePtr,"Form of collimators 0 - square form\n"); 
    	
    	if (key_outer == 1)
    	{
    		fprintf(LogFilePtr,"Form of collimators 1 - circle form\n"); 
    		OuterRadius = OuterA;
    	}	
    	
    
	if (TransFileName0 != NULL) trans_file0 = fopen(TransFileName0,"r");
	
	
	if (WaveMonoch < 0.0)
	{
	   fprintf(LogFilePtr,"Wavelength for monhromatisation must be positive!!! \n");
	   exit(-1);
	}
	
	
	if (DistanceAbs < 0.0)
	{
	   fprintf(LogFilePtr,"Absolute distance must be positive!!! \n");
	   exit(-1);
	}
	
	
	if (TotalLength <= 0.0)
	{
	   fprintf(LogFilePtr,"Total length of gridset must be positive!!! \n");
	   exit(-1);
	}

	
	if (Thicknesscoll <= 0.0)
	{
		fprintf(LogFilePtr,"Thickness of collimator <= 0.0 !!!");
		exit(-1);
	}
	
	
	if (DistanceDev < 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of distance must be positive!!! \n");
	   exit(-1);
	}
	

	if (ShiftHorDev < 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of horizontal shift must be positive!!! \n");
	   exit(-1);
	}	

	
	if (ShiftVerDev < 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of vertical shift must be positive!!! \n");
	   exit(-1);
	}		

	
	if (winradiusdev < 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of size of hole must be positive!!! \n");
	   exit(-1);
	}			
	

	if (wincenterdev < 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of position of hole must be positive!!! \n");
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


	if (DistanceAbs > 0.0)
	{
		fprintf(LogFilePtr,"----------FOR GRIDSET simulations ONLY=========\n");
		fprintf(LogFilePtr, "I  Total length of instrument =  %f  cm  \n", TotalLength);
		fprintf(LogFilePtr, "I  Absolute distance from first grid = %f  cm  \n", DistanceAbs);

		if (WaveMonoch > 0.0)
		{
	   		fprintf(LogFilePtr,"WARNING: Gravity monochromatisation is activated for  %f  Ang \n", WaveMonoch);
	   		fprintf(LogFilePtr,"WARNING: All grids in system will be shifted in vertical direction, down!!!! \n");
	   
			/* Calculate vertical shifting */
			
			Ncalc.Position[0] = 0.0 ;
			Ncalc.Position[1] = 0.0 ;
			Ncalc.Position[2] = 0.0 ;
			Ncalc.Vector[0] = 1.0 ;
			Ncalc.Vector[1] = 0.0 ;			
			Ncalc.Vector[2] = 0.0 ; 
			Ncalc.Wavelength = WaveMonoch ; 
			
			Pcalc.A = 1.0;
		 	Pcalc.B = 0.0;
		   	Pcalc.C = 0.0;
		  	Pcalc.D = -1.0*DistanceAbs;

			TimeOF = NeutronPlaneIntersectionGrav(&Ncalc, Pcalc);
			
			ShiftVer = Ncalc.Position[2] ;
	
			fprintf(LogFilePtr,"WARNING: OVERRIDDING! New value for the down shifting  %f  cm \n", ShiftVer);
	
		}	
		fprintf(LogFilePtr,"---------------------------------==========================\n");
	}


	if (DistanceDev > 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of distance is activated  +-  %f  cm \n", DistanceDev);
	   Distance   =   Distance +  (MonteCarlo(-1.0, 1.0)*DistanceDev) ;
	}
	

	if (ShiftHorDev > 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of horizontal shift is activated  +-  %f  cm \n", ShiftHorDev);
	   ShiftHor   =   ShiftHor +  (MonteCarlo(-1.0, 1.0)*ShiftHorDev) ;	   
	   fprintf(LogFilePtr,"New value for horizontal shift = %f cm \n", ShiftHor);
	}	

	
	if (ShiftVerDev > 0.0)
	{
	   fprintf(LogFilePtr,"Deviation of vertical shift is activated  +-  %f cm \n", ShiftVerDev);
	   ShiftVer   =   ShiftVer +  (MonteCarlo(-1.0, 1.0)*ShiftVerDev) ;	   
	   fprintf(LogFilePtr,"New value for vertical shift = %f cm \n", ShiftVer);	   
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



     
 	Endpoint.A = 1.0;
 	Endpoint.B = 0.0;
 	Endpoint.C = 0.0;
 	Endpoint.D = -1.0*Distance;
 	
 	fprintf(LogFilePtr,"Distance between plane x=0 and window plane  %f cm  \n", fabs(Endpoint.D));
 	
 	
 	Endpointcol.A = 1.0;
 	Endpointcol.B = 0.0;
 	Endpointcol.C = 0.0;
 	Endpointcol.D = -1.0*(Thicknesscoll+Distance);
 	
 	fprintf(LogFilePtr,"Thickness+dist of collimator plate  %f cm  \n", fabs(Endpointcol.D));
 	
  
	for(i=0; i<=1000; i++)
	{
		winradius[i] = 0.0; 
		ywincenter[i] = 0.0; 
		zwincenter[i] = 0.0;
	}	
	
	for(i=0; i<=3000; i++) 
	rdate[i] = 0.0;

	
	  
  if(keygrav == 1)
  {
      fprintf(LogFilePtr,"Grid module: Gravity is enabled \n");
  }
  else
  {
      fprintf(LogFilePtr,"Grid module: Gravity is disabled \n");
  }

	
	
	/* Input data from collimator file */
	
	

  if (CollFileName !=NULL)
    {
      for(counter = 1; counter <= 3000; counter++)
	{
	  if (fscanf(coll_file,"%lf",&rdate[counter])==EOF)
	    break;
	}

      fclose(coll_file);
    }
  else
    {
    fprintf(LogFilePtr,"\n No data for grid description. Check the input file. \n");
    exit(-1);
    }
    
/*    for(i = 1; i <= counter; i++)
    {
    fprintf(LogFilePtr," %d   %f \n", i, rdate[i]);
    }   */

    NumberOfHoles = (long)((counter-1)/3);

/*	fprintf(LogFilePtr,"Number of in %d \n", (counter-1)); */
	fprintf(LogFilePtr,"\n Number of holes: %ld \n", NumberOfHoles);
	
	k = 1;
	for(i = 1; i <= NumberOfHoles; i++) 
	{
	
	    
	    	if (DistanceAbs > 0.0)
		{
		
		/* reducing the grids sizes according the converging to the detector */
		
		    if ((2.0*TotalLength) <= DistanceAbs)
		    {
		    	fprintf(LogFilePtr,"EXIT: Incorrect absolute distance and Total length of grid system \n");
			exit(-1);
		    }
		
			ywincenter[i] = ((rdate[k])*(2.0*TotalLength-DistanceAbs)/(2.0*TotalLength)) + (MonteCarlo(-1.0, 1.0)*wincenterdev); 
	    		zwincenter[i] = ((rdate[k+1])*(2.0*TotalLength-DistanceAbs)/(2.0*TotalLength)) + (MonteCarlo(-1.0, 1.0)*wincenterdev);
	    		winradius[i] = ((rdate[k+2])*(2.0*TotalLength-DistanceAbs)/(2.0*TotalLength)) + (MonteCarlo(0.0, 1.0)*winradiusdev);   

		}
		else
		{	
	    		ywincenter[i] = rdate[k] + (MonteCarlo(-1.0, 1.0)*wincenterdev); 
	    		zwincenter[i] = rdate[k+1] + (MonteCarlo(-1.0, 1.0)*wincenterdev);
	    		winradius[i] = rdate[k+2] + (MonteCarlo(0.0, 1.0)*winradiusdev);	    
	    	}	
	    
	    k = k + 3;
	    
	}
	
	
	for(i = 1; i <= NumberOfHoles; i++)
	{
//	fprintf(LogFilePtr,"\n Grid data: center Y = %f cm  Z = %f cm radius = %f cm", ywincenter[i], 
//		zwincenter[i], winradius[i]);
	} 
	

	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {

/****************************************************************************************/
/* 	Move neutron to window with gravity effect and calculate Time of Flight (ms).   */
/****************************************************************************************/
	
      CHECK
      
        if (InputNeutrons[i].Vector[0] <= 0.0) continue;
      	if (InputNeutrons[i].Wavelength == 0.0) continue;
	VelocityReal = (double)(V_FROM_LAMBDA(InputNeutrons[i].Wavelength)); 
	if (VelocityReal <= 0.0) continue;
	color_current = InputNeutrons[i].Color;
      
			
	if (keygrav == 1)
	{
		TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
	}
	else
	{
		TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
	}

		InputNeutrons[i].Time += (double)TimeOF;


		if (key_output==1)
		{
		    fprintf(LogFilePtr,"Pos  %f    %f    %f   \n", InputNeutrons[i].Position[0],
		    InputNeutrons[i].Position[1], InputNeutrons[i].Position[2]);
		}    

				/* windows test */

			
		NewPositionY = InputNeutrons[i].Position[1] - ShiftHor;
		NewPositionZ = InputNeutrons[i].Position[2] - ShiftVer;
		tempdistsquared = NewPositionY*NewPositionY + NewPositionZ*NewPositionZ;		

		key_abs = 0;		

		if (key_outer==0) /* Choose Form of collimators 0 - square form; 1 - circle form */
		{
		    if ((-0.5*OuterA < NewPositionY)&&(0.5*OuterA > NewPositionY)&&(-0.5*OuterB < NewPositionZ)&&(0.5*OuterB > NewPositionZ))
		    {
		/*	Square form     */
	    		key_abs = 1;
			for(j=1; j<=NumberOfHoles; j++) 
			{
				if ((-0.5*winradius[j] < (NewPositionY-ywincenter[j]))&&
				(0.5*winradius[j] > (NewPositionY-ywincenter[j]))&&
				(-0.5*winradius[j] < (NewPositionZ-zwincenter[j]))&&
				(0.5*winradius[j] > (NewPositionZ-zwincenter[j]))) 
							{
									    key_abs = 0 ;
									    hole_current = j ;
							}		    
			}		
		    }	
		}
		else
		{ 
		/*      Circle form	*/
		    if (tempdistsquared <= OuterRadius*OuterRadius) 
		    {
			key_abs = 1;
			for(j=1; j<=NumberOfHoles; j++) 
			{
			    tempdistsquared = (NewPositionY - ywincenter[j])*(NewPositionY - ywincenter[j]) +
			    (NewPositionZ - zwincenter[j])*(NewPositionZ - zwincenter[j]);
			    if (tempdistsquared <= winradius[j]*winradius[j]) 
						    {
									key_abs = 0;
									hole_current = j ;
						    }	
			}
		    }
		}
	
				
	 /* case of transmission neutron */

	 if (keygrav == 1)
	 {
		TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpointcol);
	 }
	 else
	 {
		TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpointcol);
	 }
	 
	    InputNeutrons[i].Time += (double)TimeOF;


	 /* Attenuation in the collimator material */	 

		if (key_abs == 1) 
		{
			if (keymaterial0 == 6)
 continue;
		     /* Attenuation during pass of collimator material */
				 N_Wavelength = InputNeutrons[i].Wavelength;    
				 mu = Interpolation(N_Wavelength,keymaterial0,WAVS,MUS,ntfs);
				 if (mu == -10000.0)
				 {
				    fprintf(LogFilePtr,"EXIT! Interpolation mistake\n");
				    exit(-1);
				 }
				 prob = exp(-mu*TimeOF*VelocityReal);	
// 				 fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
 				 InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
		
		}


	if (key_colortracking == 1)
	{
	    if ((hole_current != color_current)&&(color_current != 0.0))
	    {
		fprintf(LogFilePtr,"WARNING: CROSSTALK OF TRAJECTORIES IS FOUND!  DistanceAbs:   %f    hole_current:   %d   color_current:   %d    I \n", DistanceAbs, hole_current, color_current);
	    }
	}    
	
	InputNeutrons[i].Color = hole_current;	 
	InputNeutrons[i].Position[0] = 0.0;
	Output = InputNeutrons[i];
	WriteNeutron(&Output);
	}
    }

	my_exit:

	fprintf(LogFilePtr," \n");

	Cleanup((Thicknesscoll+Distance), 0.0, 0.0, 0.0, 0.0);

	return(0);
}

////////////////////////////////////////////////////


 	
	    

	    
			
		
