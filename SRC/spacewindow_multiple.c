/*********************************************************************************************/
/*  VITESS module  spacewindow_multiple                                                      */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*  Modified by Manoshin Sergey in Jan 2001 for include gravity effect                       */	
/* Significant revision: multiple circular windows (only) collimator 05.02.01                */
/* Include multi circle windows (<100) possibility  07 Feb 2001                              */
/*                                                                                           */
/* 1.00  Feb  2001  S. Manoshin     initial version                                          */	
/* 1.01  June 2001  K. Lieutenant   SOFTABORT                                                */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation, center of beam                           */
/* 2.00  Jun  2003  S. Manoshin     Add possibility for simulations of material of           */      
/*	                                  collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, */
/*                                   3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber       */
/* 2.10  Mar  2004  S. Manoshin     Add "choosing" of material for inner part of collimator  */
/* 2.21  Jul  2004  S. Manoshin	   Corrected some bugs for thick collimator                 */
/* 2.22  Dec  2007  K. Lieutenant   Option to use rectangular windows added                  */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"


/******************************/
/** Definitions              **/
/******************************/

typedef enum
{	
	VT_AUTO_SHAPE = 0,
	VT_CIRCLE     = 1,
	VT_RECTANGLE  = 2
}
VtWndGeom;


/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit    (int argc, char *argv[]);
short ReadWndFile();


/******************************/
/** Global Variables         **/
/******************************/

VtWndGeom eShape=VT_AUTO_SHAPE,  /* kind of window: defined by input file, circular or rectangular   */
          eWinShape[101];  
char*     CollFileName=NULL;
double    Distance=0.0,
          OuterRadius=200.0; 
double    winradius[101], ywincenter[101], zwincenter[101], winwidth[101], winheight[101];
double    Thicknesscoll=0.0;
double    Thicknesscolli=0.0;
char	   *TransFileName0=NULL;
char	   *TransFileName1=NULL;
long      keymaterial0=6; 	  // Material of collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, 
long      keymaterial1=1;    //                         3 - Bor10,     4 - Eu, 5 - Silicon, 6 - ideal absorber 



int main(int argc, char *argv[])
{
	long	i, j, k;
	long	BufferIndex;
	long 	NumberOfHoles;
	long	key_abs;        /* key absorb: 1 - yes absorb, 0 - transmission  */	

	double  tempdistsquared;
	double  TimeOF;

	Plane	Endpoint;
	Plane	EndPoint1, EndPoint2 ;

	double	NewPositionY, NewPositionZ;
	double  CenterX, CenterY, CenterZ, SumProb;
	
	Neutron  Output;

	long ntfs=0, count;    	      
	long ntfss=0, ntfs1=0; /* internal */
	double WAVS[500], MUS[500], transm0[1001];
	FILE	*trans_file0=NULL; /* file for describing of transmission of outer material of collimator  */
	
	double WAV1[500], MU1[500], transm1[1001];
	FILE	*trans_file1=NULL; /* file for describing of transmission of inner material of collimator  */
       
	double VelocityReal, N_Wavelength , mu, prob=0.0, TOF3 ;
 	
 	
 	 BufferIndex=0;

	/***************************************************************************/
	/* Endpoint.D                distance to window along x-direction   [cm]   */
	/***************************************************************************/

	/*input*/
	Init(argc, argv, VT_WND_MULT);

	print_module_name("Space and Multiple Windows 2.22");

	OwnInit(argc, argv);


	if (TransFileName0 != NULL) trans_file0 = fopen(TransFileName0,"r");
	
	if (TransFileName1 != NULL) 
	{
	    trans_file1 = fopen(TransFileName1,"r");
	    fprintf(LogFilePtr,"MATERIAL OF OPEN PART OF COLLIMATOR IS DESCRIBING BY FILE:  %s \n", TransFileName1);
	    keymaterial1 = 0; /* activate this material */
	}    


	if (Distance < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Length of space must be >= 0.0 !!!\n");
		exit(-1);
	}
	
	
	if (Thicknesscoll < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Thickness of collimator < 0.0 !!!");
		exit(-1);
	}

	if (Thicknesscolli < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Thickness of open part of the collimator < 0.0 !!!");
		exit(-1);
	}
	
	
	
	
	if (Thicknesscoll != Thicknesscolli)
	{
		fprintf(LogFilePtr,"WARNING! It is recommended to have outer and inner thickness EQUAL \n");
	}
	
	
	if (Thicknesscoll == 0.0)
	{
		keymaterial0 = 6;
	}
	
	if (Thicknesscolli == 0.0)
	{
		keymaterial1 = 1;
	}
	
	
	
	if (keymaterial0 == 0)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Material transmission characteristics reading from file\n");
  	}

  	if (keymaterial0 == 1)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Gadolinium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary \n");
  	}
  	
  	if (keymaterial0 == 2)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Cadmium \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 3)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Bor10 \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 4)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Eu \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 0.3 .. 28 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 5)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Silicon \n");
  	    fprintf(LogFilePtr,"Wavelength range must be 1 .. 20 A, please correct if nesessary  \n");
  	}
  	
  	if (keymaterial0 == 6)
  	{
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: IDEAL ABSORBER \n");
  	}
  	
	if ((keymaterial0 != 0)&&(keymaterial0 != 1)&&(keymaterial0 != 2)&&(keymaterial0 != 3)&&(keymaterial0 != 4)&&(keymaterial0 != 5)&&(keymaterial0 != 6))
  	{
  	    fprintf(LogFilePtr,"ERROR: MATERIAL OF COLLIMATOR: No material: EXIT! Correct -c option \n");
	    exit(-1);
  	}


  
	  for(j=0; j<500; j++)
	  {	
		WAVS[j] = 0.0;
		MUS[j] = 0.0;
		WAV1[j] = 0.0;
		MU1[j] = 0.0;
	  }
	  
	  for(i=0; i<=1000; i++)
	  {	
		transm0[i] = 0.0;
		transm1[i] = 0.0;
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
	    ntfss = ntfs;
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
		    fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of the collimator \n");
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


////////////
    if (keymaterial1 == 0)
    {

          for(count=1; count<=1000; count++)
    	    {
    		if (fscanf(trans_file1,"%lf",&transm1[count])==EOF)
		    break;
	    }

    	  fclose(trans_file1);
	
	    k = 1;
	    ntfs = (long)((count-1)/2);
	    ntfs1 = ntfs;
    	    for(i = 1; i <= ntfs; i++)
	    {
		WAV1[i] = transm1[k];
		MU1[i] = transm1[k+1];
		k = k + 2;
	    }
	    
	/* check the input data */	    
	    for(i = 1; i <= (ntfs-1); i++)
	    {
		if (WAV1[i+1] <= WAV1[i]) 
		{
		    fprintf(LogFilePtr,"ERROR: incorrect data in open transmission file of the collimator \n");
		    fprintf(LogFilePtr,"The numbers in the wavelength columns must be increase!!! \n");
		    exit(-1);
		}    
	    }
	    
//	    fprintf(LogFilePtr,"transmfile for surface count = %d  num = %d \n", count, ntfs);
//	    for(i = 1; i <= ntfs; i++)
//	    fprintf(LogFilePtr," %f   %f  \n",WAV1[i], MU1[i]);
	    fprintf(LogFilePtr,"Minimal and maximal wavelengths must be %f ... %f \n",WAV1[1], WAV1[ntfs]);
	
    }	
/////////////////


 	
	Endpoint.A = 1.0;
	Endpoint.B = 0.0;
	Endpoint.C = 0.0;
	Endpoint.D = -1.0*Distance;
	
	EndPoint1.A = 1.0;
	EndPoint1.B = 0.0;
	EndPoint1.C = 0.0;
	EndPoint1.D = -1.0*(Thicknesscolli+Distance);
	
	EndPoint2.A = 1.0;
	EndPoint2.B = 0.0;
	EndPoint2.C = 0.0;
	EndPoint2.D = -1.0*(Thicknesscoll+Distance);	

 	
  
	for(j=1; j<=100; j++)
	{
		ywincenter[j] = 0.0; 
		zwincenter[j] = 0.0;
		winradius [j] = 0.0; 
		winwidth  [j] = 0.0; 
		winheight [j] = 0.0; 
		eWinShape [j] = VT_AUTO_SHAPE;
	}	

	CenterX   = 0.0; 
	CenterY   = 0.0; 
	CenterZ   = 0.0; 
	SumProb   = 0.0;
	
	  
  if(keygrav == 1)
  {
      fprintf(LogFilePtr,"Gravity is enabled \n");
  }
  else
  {
      fprintf(LogFilePtr,"Gravity is disabled \n");
  }

	
	/* Define number of lines in the collimator files 
	   Number of lines eq Numbers of holes in multiple collimator*/
	   

	
	/* Input data from collimator file */
	NumberOfHoles = ReadWndFile();

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

			
	if (keygrav == 1)
	{
		TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
	}
	else
	{
		TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
	}
	
	

		InputNeutrons[i].Time += (double)TimeOF;
		CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
		CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
		CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
		SumProb   += InputNeutrons[i].Probability;


				/* windows test */
				
			
		NewPositionY = InputNeutrons[i].Position[1];
		NewPositionZ = InputNeutrons[i].Position[2];
		tempdistsquared = NewPositionY*NewPositionY + NewPositionZ*NewPositionZ;
		
	
		key_abs = 0;
		TOF3 = 0.0;
				
		if (tempdistsquared <= OuterRadius*OuterRadius) 
		{
		
			key_abs = 1;
		
			for(j=1; j<=NumberOfHoles; j++) 
			{
				if (eWinShape[j]==VT_CIRCLE)
				{	tempdistsquared =  (NewPositionY - ywincenter[j])*(NewPositionY - ywincenter[j]) 
					                 + (NewPositionZ - zwincenter[j])*(NewPositionZ - zwincenter[j]);
					if (tempdistsquared <= winradius[j]*winradius[j]) 
						key_abs = 0;
				}
				else
				{	if (fabs(NewPositionY - ywincenter[j]) < 0.5*winwidth [j]  && 
					    fabs(NewPositionZ - zwincenter[j]) < 0.5*winheight[j]    ) 
						key_abs = 0;
				}
			}
		
			if (key_abs == 1) 
			{
				if (keymaterial0 == 6)
   continue;
				
				
					if (keygrav == 1)
					{
					    TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPoint2);
					}
					else
					{
					    TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPoint2);
					}	
					
//					fprintf(LogFilePtr,"OUT  TOF3 = %f  TimeOF = %f \n", TOF3, TimeOF);					
				
				     /* Attenuation during pass of collimator material */
					 N_Wavelength = InputNeutrons[i].Wavelength;    
					 mu = Interpolation(N_Wavelength,keymaterial0,WAVS,MUS,ntfss);
					 if (mu == -10000.0)
					 {
					    fprintf(LogFilePtr,"ERROR: module spacewindow_multiple, EXIT!\n");
					    exit(-1);
					 }
					 prob = exp(-mu*TOF3*VelocityReal);	
//	 				 fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
	 				 InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
					 InputNeutrons[i].Time += (double)TOF3;	
			
			}
		
		}
		
		if (key_abs == 0)
				{
					
				    /* case of transmission neutron */
				
				    
					if (keygrav == 1)
					{
						TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPoint1);
					}
					else
					{
						TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPoint1);
					}
	
//					fprintf(LogFilePtr,"INN  TOF3 = %f  TimeOF = %f \n", TOF3, TimeOF);
					

				    
				    if ((keymaterial1 == 0)&&(tempdistsquared <= OuterRadius*OuterRadius))				 
  				    {
				        /* Attenuation during pass of open collimator material */
					    N_Wavelength = InputNeutrons[i].Wavelength;    
					    mu = Interpolation(N_Wavelength,keymaterial1,WAV1,MU1,ntfs1);
					    if (mu == -10000.0)
					    {
						fprintf(LogFilePtr,"ERROR: module spacewindow: EXIT!\n");
						exit(-1);
					    }
					    prob = exp(-mu*TOF3*VelocityReal);	
//	 				 	fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
	 				    InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
	 				    InputNeutrons[i].Time += (double)TOF3;	
				    }
				}    	 
					


				InputNeutrons[i].Time += (double)TOF3;							
				InputNeutrons[i].Position[0] = 0.0;	
				Output = InputNeutrons[i];
				WriteNeutron(&Output);
				
	}
    }

	my_exit:

	fprintf(LogFilePtr,"\n Distance between plane x=0 and window plane  %f cm  \n", fabs(Endpoint.D));

	if (SumProb != 0.0)
	{
		CenterX   = CenterX/SumProb;
		CenterY   = CenterY/SumProb; 
		CenterZ   = CenterZ/SumProb; 
		
		fprintf(LogFilePtr,"Center of beam at the windows: X = %f cm Y = %f cm Z = %f cm \n",CenterX, CenterY, CenterZ );
	}
	else
	{
	fprintf(LogFilePtr,"No neutrons at the exit of this module \n");
	}

	fprintf(LogFilePtr," \n");


	
	if (Thicknesscolli >= Thicknesscoll)
	{	
		Cleanup((Thicknesscolli+Distance), 0.0, 0.0, 0.0, 0.0);
	}
	else
	{	
		Cleanup((Thicknesscoll+Distance), 0.0, 0.0, 0.0, 0.0);
	}	
	

	return(0);
}



void   OwnInit   (int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') 
		{
			switch(argv[i][1])
			{
				case 'I':
					CollFileName=&argv[i][2];
					break;

				case 'D':
					Distance =  atof(&argv[i][2]);
					break;

				case 'r':
					OuterRadius = atof(&argv[i][2]);
					break;

				case 'S':
					eShape = (short) atol(&argv[i][2]);  // Shape of the individuals apertures: 0 different,  1: circular, 2: rectangular
					break;


				case 'c':
					keymaterial0 = atol(&argv[i][2]);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
					break;

								
				case 'C':
					TransFileName0=&argv[i][2]; 
					break;

				case 'm':
					TransFileName1=&argv[i][2]; 
					break;

							
				case 't':
					Thicknesscoll = atof(&argv[i][2]);
					break;

				case 'T':
					Thicknesscolli = atof(&argv[i][2]);
					break;


				default:
					fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}
}
				

					
short ReadWndFile()
{
	char  sLine[256];
	short j, Nholes;            // index over holes and number of holes
	FILE  *coll_file;

	if (CollFileName !=NULL)
	{	if( (coll_file = fopen(CollFileName,"r"))==NULL)
		{
			fprintf(LogFilePtr, "ERROR: File %s could not be opened to read collimator data \n", CollFileName);
			exit(-1);
		}
		else
		{
			j=0;
			while (ReadLine(coll_file, sLine, sizeof(sLine)-1)==TRUE)
			{	
				j++;
				if (eShape==VT_CIRCLE)
				{	sscanf(sLine, "%lf %lf %lf",     &ywincenter[j], &zwincenter[j], &winradius[j]);
					eWinShape[j] = VT_CIRCLE;
				}
				else if (eShape==VT_RECTANGLE)
				{	sscanf(sLine, "%lf %lf %lf %lf", &ywincenter[j], &zwincenter[j], &winwidth[j], &winheight[j]);
					eWinShape[j] = VT_RECTANGLE;
				}
				else
				{	sscanf(sLine, "%lf %lf %lf %lf", &ywincenter[j], &zwincenter[j], &winwidth[j], &winheight[j]);
					if (winheight[j] > 0.0)
					{	eWinShape[j] = VT_RECTANGLE;
					}
					else
					{	winradius[j] = winwidth[j];
						eWinShape[j] = VT_CIRCLE;
					}
				}
			}
		}
		fclose(coll_file);
	}
	else
	{
		fprintf(LogFilePtr,"\n ERROR: No collimator file name given \n");
		exit(-1);
	}

	Nholes = j;

	fprintf(LogFilePtr,"\n Number of holes: %ld", Nholes);
	fprintf(LogFilePtr,"\n Outer radius of the collimator: %7.2lf cm", OuterRadius);


	for(j = 1; j <= Nholes; j++)
	{
		if (eWinShape[j]==VT_CIRCLE)
			fprintf(LogFilePtr,"\n Collimator data: center Y = %6.2lf cm  Z = %6.2lf cm radius = %6.2lf cm", 
									 ywincenter[j], zwincenter[j], winradius[j]);
		else 
			fprintf(LogFilePtr,"\n Collimator data: center Y = %6.2lf cm  Z = %6.2lf cm radius = %6.2lf cm", 
									 ywincenter[j], zwincenter[j], winwidth[j], winheight[j]);
	} 
	fprintf(LogFilePtr,"\n") ;

	return Nholes;
}
	
