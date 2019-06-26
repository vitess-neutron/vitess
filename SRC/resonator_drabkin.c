#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"
#include "resonator_drabkin.h"

/* This module was born from module rotating_field, started in July 2003 

    The main goal of this module to simulate a Drabkin spin-flip resonator
    with periodical plus guide magnetic fields. Manoshin Sergey, HMI
    
Aug 2003:	 Initial version, some simulations performed, alpha version
Oct 2003:  	 Added spacing between domains, Alpha version
Oct 2003:	 Added output of polarisation vector in cartesian, sphereical 
		 and euler coordinate systems
Dec 2003:	 Improve algorithm for +/- changing		 
Feb 2004:	 Remove inlination of resonator

   */


int main(int argc, char **argv)
{

/* Variable for rotation */
double Rroty, Rrotz, RRSM;
VectorType RR, RR1, RR2, RRS;
double VX, VY, VZ, VLL;

Neutron NeutronAdd1, NeutronAdd2;
Plane EndPoint1, EndPoint2;

 /* Initialize the program according to the parameters given  */ 

	Init(argc, argv, VT_RES_DRABKIN); 

	OwnInit(argc, argv);


 /* Get the neutrons from the file */
DECLARE_ABORT;

  while((ReadNeutrons())!= 0)
  {
CHECK;	/* here is what happens to the neutron */

for(i=0;i<NumNeutGot ;i++)

{ 


	/*InputNeutrons[i].Position[0]	= 0. ;*/

	TOF = InputNeutrons[i].Time ;

	WL = InputNeutrons[i].Wavelength ;

	Prob = InputNeutrons[i].Probability ;

	CopyVector(InputNeutrons[i].Position, Pos) ;

	CopyVector(InputNeutrons[i].Vector, Dir) ;
	
	/* Check incorrect neutrons */
	if ((Dir[0] <= 0.0)||(WL == 0.0)) goto getlost;

	/* improve calculations, renormalize */
	InputNeutrons[i].Vector[0]	= (double) sqrt(fabs(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2]))) ;

	CopyVector(InputNeutrons[i].Spin, SpinVector) ; 


/* translates into frame of the main field and rotates coordinates  */

//		fprintf(LogFilePtr,"AAAAA Pos before rot  X =  %f   Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);	

		SubVector(Pos, PosMain) ;		
	
		/* Move neutron in the precession volume */
	
		NeutronAdd1.Position[0] = Pos[0];
		NeutronAdd1.Position[1] = Pos[1];
		NeutronAdd1.Position[2] = Pos[2];
	
		NeutronAdd1.Vector[0] = Dir[0];
		NeutronAdd1.Vector[1] = Dir[1];
		NeutronAdd1.Vector[2] = Dir[2];
	
		NeutronAdd1.Wavelength = WL;
	
	
		EndPoint1.A = 1.0;
		EndPoint1.B = 0.0;
		EndPoint1.C = 0.0;
		EndPoint1.D = 0.5*depth;
	
	
		TOF1 = NeutronPlaneIntersection1(&NeutronAdd1, EndPoint1);	
		if (TOF1 < 0.0) goto getlost;
		
	
		Pos[0] = NeutronAdd1.Position[0];
		Pos[1] = NeutronAdd1.Position[1];
		Pos[2] = NeutronAdd1.Position[2];
	
		Dir[2] = NeutronAdd1.Vector[2];
	
	
		TOF = TOF + TOF1;
		
		
//		fprintf(LogFilePtr,"TOF1  =  %f  \n", TOF1);		
//		fprintf(LogFilePtr,"AAAAA Pos after rot  X =  %f   Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);	

/* looks for first domain if dimension of domain changes only along X axis */

	
	DimDomain[0] = depth/ind_x_max ; 
	DimDomain[1] = width/ind_y_max ; 
	DimDomain[2] = height/ind_z_max ; 


	ind_y = (long) floor(Pos[1] / DimDomain[1]) + 1 + ind_y_max/2 ;

	if((ind_y <= 0)||(ind_y > ind_y_max)) goto getlost ;

	ind_z = (long) floor(Pos[2] / DimDomain[2]) + 1 + ind_z_max/2 ;

	if((ind_z <= 0)||(ind_z > ind_z_max)) goto getlost ;

	ind_x = 1 ; 

/*	
******************* starts to scan ******************************/

	NumberPrecessions = 0 ;
	Number_NOP = 0.0 ;
	indp = 1 ;
	keyperiod = 0;
	


while (ind_x != (ind_x_max +1)) 
{


/* Generate geometry of magnetic field for precession */

/* generate fieldsph domain size: uniform  */

	DimDomain[0] = depth/ind_x_max ; 
	DimDomain[1] = width/ind_y_max ; 
	DimDomain[2] = height/ind_z_max ; 
			 

/* generate position */
			
	PosDomain[0] = ((ind_x -1)-(ind_x_max /2 - 0.5)) * DimDomain[0] ;
	PosDomain[1] = ((ind_y -1)-(ind_y_max /2 - 0.5)) * DimDomain[1] ;
	PosDomain[2] = ((ind_z -1)-(ind_z_max /2 - 0.5)) * DimDomain[2] ;



/* define the periodical magnetic field */	
/* Prepeare values for deviation of the magnetic 
field amplitude and frequency */



	
    switch(keyampldistr)
	    {
		    case 0:
		    {
/*			    fprintf(LogFilePtr,"Uniform Distribution of amplitude of periodical magnetic field. \n");	*/
			    FieldValue = FieldValueInit ;
		    	    break;
		    }
		
		    case 1:	
		    {
/*		    	    fprintf(LogFilePtr,"Sinus Distribution of amplitude of periodical magnetic field. \n");	*/
			    FieldValue = FieldValueInit*sin(M_PI*(Pos[0]+PosMain[0])/depth) ;
			    break;
		    }
		    
		    case 2:	
		    {
/*		    	    fprintf(LogFilePtr,"Gauss Distribution of amplitude of periodical magnetic field. \n");	*/
			    FieldValue = FieldValueInit*exp(Pos[0]*Pos[0]/(-2.0*SigmaNorm*SigmaNorm));
			    break;
		    }		    

		    default:
		    {
		    	    fprintf(LogFilePtr,"ERROR: No Law! Correct option -v (Values 0, 1 or 2) \n");
		    	    exit(-1);
		    	    break;
		    }
	    }		
	
	
	
	FieldValueA = FieldValue - 0.01*FieldValueDevPer*fabs(FieldValue) ;
	FieldValueB = FieldValue + 0.01*FieldValueDevPer*fabs(FieldValue) ;
	SigmaField = 0.01*FieldValueDevPer*fabs(FieldValue);
	
	

	/* Perform Randomize of the magnetic field components, overload */
	
	if (FieldValueDevPer > 0.0)
	{
	    switch(DevLawAmpl)
	    {
		case 0:
		{
			FieldValue = DistrGauss(FieldValue, SigmaField);
		break;
		}
		
		case 1:	
		{
			FieldValue = MonteCarlo(FieldValueA, FieldValueB);
		break;
		}

		default:
		{
		    	fprintf(LogFilePtr,"ERROR: No Law! Correct option -e (Values 0, 1)\n");
		    	exit(-1);
		    	break;
		}
	    }	
	}

	/* Activate periodical changing	*/
	if (keyperiod == 1) 
	{
	    FieldValue = -1.0*FieldValue;
	}  
/*	fprintf(LogFilePtr,"keyperiod = %d \n", keyperiod);  
	fprintf(LogFilePtr,"FieldValue = %f \n", FieldValue);	*/

	/* Activate permanent magnetic field */

	FieldValue0[0] = FieldValue0Init[0] ;
	FieldValue0[1] = FieldValue0Init[1] ;
	FieldValue0[2] = FieldValue0Init[2] ;	


	/* Perform random of the  permanent magentic field */
	

	 if (FieldValue0Dev > 0.0)
	 {
	 	VLL = vector3rand(&VX, &VY, &VZ);
		FieldValue0[0] = FieldValue0[0] + fabs(FieldValue0Dev)*VX;
		FieldValue0[1] = FieldValue0[1] + fabs(FieldValue0Dev)*VY;
		FieldValue0[2] = FieldValue0[2] + fabs(FieldValue0Dev)*VZ;
	 }	
	 
//	fprintf(LogFilePtr,"GF  %f  %f  %f \n", FieldValue0[0], FieldValue0[1], FieldValue0[2]);

	    /* Generate common field */
	    
		    switch(keyaxis)
		    {
		    case 0:
			{
			/* Choose parallel of axis 0X */
	    		    RR[0] = FieldValue0[0] + FieldValue ;
	    		    RR[1] = FieldValue0[1] ;
	    		    RR[2] = FieldValue0[2] ;
			    break;
			}
		    case 1:	
			{
			/* Choose parallel of axis 0Y */
	    		    RR[0] = FieldValue0[0] ;
	    		    RR[1] = FieldValue0[1] + FieldValue ;
	    		    RR[2] = FieldValue0[2] ;	
			    break;
			}
		    case 2:
			{
			/* Choose parallel of axis 0Z */
	    		    RR[0] = FieldValue0[0] ;	
	    		    RR[1] = FieldValue0[1] ;
	    		    RR[2] = FieldValue0[2] + FieldValue ;
			    break;
			}
		    default:
			{
			    fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
			    exit(-1);
			    break;
			}
		    }	
	    		
	
	/* process field */	
		
	    RRS[0] = RR[0];
	    RRS[1] = RR[1];
	    RRS[2] = RR[2];
	    RRSM = sqrt(RRS[0]*RRS[0] + RRS[1]*RRS[1] + RRS[2]*RRS[2]);
	    
	    RR1[0] = RR[0]; 
	    RR1[1] = RR[1]; 
	    RR1[2] = RR[2];
	    
	    RR2[0] = FieldValue0[0]; 
	    RR2[1] = FieldValue0[1]; 
	    RR2[2] = FieldValue0[2];
	    
	    
	    CartesianToEulerZY(RR, &Rroty, &Rrotz);
	    domain_field[0] = LengthVector(RR);
	    domain_field[1] = Rrotz;
	    domain_field[2] = Rroty;
//	    fprintf(LogFilePtr,"Field: roty = %f rotz = %f\n",Rroty,Rrotz);
    
	/* Rotating option */

	FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 
		


/* translates into frame of the field domain */
	
	SubVector(Pos, PosDomain) ;


/* calculate entrance end exit coordinates of domain*/


	{ 
		
		VectorType pos, dir;	CopyVector(Pos, pos) ;	CopyVector(Dir, dir) ;
	
		/* gives intersection positions with domain */

		if(IntersectionWithRectangularWallNumber(DimDomain, pos, dir, Pos1, Pos2, &wall_1, &wall_2) == 0) goto getlost ; 

	    if(wall_2 == 0) goto getlost ;

		/* ordering */

		if(Pos1[0] > Pos2[0]) 	
		
		{VectorType V ;	int wall; CopyVector(Pos1, V) ;	CopyVector(Pos2, Pos1) ; CopyVector(V, Pos2) ; 	
		
		wall = wall_1 ; wall_1 = wall_2 ; wall_2 = wall ;}

	}



	/* moment of arriving at the domain wall, new position */


	CopyVector(Pos1, Pos) ;

	/* time of precession in the domain field - precession calculated in the field frame */

	TOF2 = fabs(Pos1[0] - Pos2[0])  / fabs(Dir[0]) / V_FROM_LAMBDA(WL);

	RotVector(RotMatrixField, SpinVector) ; 
	
	RotVector(RotMatrixField, RR1) ;
//	fprintf(LogFilePtr,"RR1  X = %f Y = %f  Z = %f\n",RR1[0],RR1[1],RR1[2]);

	PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]) ;
	PhaseShift0 = PhaseShift/(2.0*(M_PI));  
	NumberPrecessions = NumberPrecessions + PhaseShift0 ;


	FillRotMatrixYX(LarmorMatrix, PhaseShift, 0) ;
	RotVector(LarmorMatrix, SpinVector) ;
	RotBackVector(RotMatrixField, SpinVector) ;


	if (RRSM != 0.0)
	{
		RRSM = 1.0;	
		FielX[indp] = FielX[indp] + (RRS[0]/RRSM);
		FielY[indp] = FielY[indp] + (RRS[1]/RRSM);
		FielZ[indp] = FielZ[indp] + (RRS[2]/RRSM);
		FielM[indp] = FielM[indp] + 1.0;
	}	



	/* moment of exiting at the domain wall, new position */

    
	TOF += TOF2 ;

	CopyVector(Pos2, Pos) ;

/* translates back into main frame */
	
	AddVector(Pos, PosDomain) ;
	
	/* Passing via "between-domain" space */
	if (((spacemin != 0.0)||(spacemax != 0.0))&&(wall_2 == 2))
	{
		spacecurr=MonteCarlo(spacemin, spacemax);
		
/*		{
		VectorType Path22  ; displacement vector 	*/
		TOF2 = spacecurr/fabs(Dir[0])/V_FROM_LAMBDA(WL) ;
/*		CopyVector(Dir, Path22) ;
		MultiplyByScalar(Path22, (spacecurr)/ Dir[0] ) ;
		AddVector(Pos, Path22) ;  TOF += TOF2 ;
		}			*/
		
/*		Perform additional precession only with guide field, begin */
		    CartesianToEulerZY(RR2, &Rroty, &Rrotz);
		    domain_field[0] = LengthVector(RR2);
		    domain_field[1] = Rrotz;
		    domain_field[2] = Rroty;
	    
		/* Rotating option */
		FillRotMatrixZY(RotMatrixField, domain_field[2], domain_field[1]) ; 
	
//		RotVector(RotMatrixField, RR2) ;
//		fprintf(LogFilePtr,"RR2  X = %f Y = %f  Z = %f\n",RR2[0],RR2[1],RR2[2]);

		RotVector(RotMatrixField, SpinVector) ; 
		PhaseShift = TOF2 * FREQUENCY_FROM_FIELD(domain_field[0]) ;
		PhaseShift0 = PhaseShift/(2.0*(M_PI));  
		NumberPrecessions = NumberPrecessions + PhaseShift0 ;
		FillRotMatrixYX(LarmorMatrix, PhaseShift, 0) ;
		RotVector(LarmorMatrix, SpinVector) ;
		RotBackVector(RotMatrixField, SpinVector) ;		

/*		End additional precession part */
	}
	
	PolX[indp] = PolX[indp] + Prob*SpinVector[0];
	PolY[indp] = PolY[indp] + Prob*SpinVector[1];
	PolZ[indp] = PolZ[indp] + Prob*SpinVector[2];
	ProbM[indp] = ProbM[indp] + Prob;
	
	/* Organize periodical changing */
	if (wall_2 == 2)
	{
	    keyperiod = keyperiod + 1;
	    if (keyperiod == 2) keyperiod = 0;
	}    
	
	indp = indp + 1;	
	Number_NOP = Number_NOP + 1.0 ;	

	/* searching new domain */ if(wall_2 == 1) goto getlost;

	if(wall_2 == 2) {ind_x += 1 ; }

	if(wall_2 == 3) {ind_y += -1 ; }

	if(wall_2 == 4) {ind_y += 1 ; }

	if(wall_2 == 5) {ind_z += -1 ; }

	if(wall_2 == 6) {ind_z += 1 ;}


	/*if(ind_x > ind_x_max) goto exitfield ; */

	if(ind_y == 0) goto exitfield ; 
	if(ind_y > ind_y_max) goto exitfield ; 
	if(ind_z == 0) goto exitfield; 
	if(ind_z > ind_z_max) goto exitfield;
	


/*goto newdomain ;*/}

	exitfield: ;



/*******************************************************************************/

	    NumberPrecessionssum =  NumberPrecessionssum + NumberPrecessions;

	    if (Number_NOP != 0.0)
	    {
	    	NumberPrecessionsave =  NumberPrecessionsave + NumberPrecessions/Number_NOP;
	    }

	/* Output matters */
	
	IntegralIntensity += Prob ;

	NumOut++ ;


//		fprintf(LogFilePtr,"BBBBB PRECESSION Pos before rota  X =  %f   Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);		
	
		AddVector(Pos, PosMain) ;	
	
		/* computes neutron variables in the output frame */ 

		SubVector(Pos, TranslOut) ;


		/* translates neutron variables for output - X'=0. */
	
		NeutronAdd2.Position[0] = Pos[0];
		NeutronAdd2.Position[1] = Pos[1];
		NeutronAdd2.Position[2] = Pos[2];
	
		NeutronAdd2.Vector[0] = Dir[0];
		NeutronAdd2.Vector[1] = Dir[1];
		NeutronAdd2.Vector[2] = Dir[2];
	
		NeutronAdd2.Wavelength = WL;
	
	
		EndPoint2.A = 1.0;
		EndPoint2.B = 0.0;
		EndPoint2.C = 0.0;
		EndPoint2.D = 0.0;

		TOF3 = NeutronPlaneIntersection1(&NeutronAdd2, EndPoint2);	
	
		Pos[0] = NeutronAdd2.Position[0];
		Pos[1] = NeutronAdd2.Position[1];
		Pos[2] = NeutronAdd2.Position[2];
	
		Dir[2] = NeutronAdd2.Vector[2];
	
		
		TOF = TOF + TOF3 ;
		
//		fprintf(LogFilePtr,"TOF3  =  %f  \n", TOF3);


//		fprintf(LogFilePtr,"BBBBB PRECESSION Pos after rota transl X =  %f  Y =  %f   Z =  %f  \n", Pos[0], Pos[1], Pos[2]);	
//		fprintf(LogFilePtr,"===================================================================================================\n");		


	Neutrons.Time = TOF ;

	Neutrons.Wavelength = WL ;

	Neutrons.Probability = Prob ;

	CopyVector(Pos, Neutrons.Position) ;

	CopyVector(Dir, Neutrons.Vector) ;

	CopyVector(SpinVector, Neutrons.Spin) ;


	/* writes output binary file */

	WriteNeutron(&Neutrons) ;

	getlost: ;

}


  }
   
 /* Do the general cleanup */
my_exit:
	OwnCleanup(); 

	Cleanup(TranslOut[0], TranslOut[1], TranslOut[2], 0.0, 0.0);	

	fprintf(LogFilePtr," \n") ;


  return 0;
}



/* own initialization of the Precession Field module */

void OwnInit(int argc, char *argv[])
{

	fprintf(LogFilePtr," \n") ;
	print_module_name("Resonator of Drabkin v1.0") ;

	/* INIT */

	depth = 100.0;
	width = 100.0;
	height = 100.0;
	

	NumberPrecessionsave = 0;
	NumberPrecessionssum = 0;

	FieldValue = 0.0;
	FieldValueInit = 0.0;
	FieldValueDevPer = 0.0;
	DevLawAmpl = 0 ;
	SigmaNorm = 1.0 ;
	
	
	
	/* permanent field components , projection in the axises OX, OY and OZ */
	/* current values */	
	FieldValue0[0] = 0.0; 
	FieldValue0[1] = 0.0;
	FieldValue0[2] = 0.0;
	
	/* initial values */
	FieldValue0Init[0] = 0.0; 
	FieldValue0Init[1] = 0.0;
	FieldValue0Init[2] = 0.0;	
	
	/* amplitude of additional random magnetic field */
	FieldValue0Dev = 0.0;
	
	for(ind_x = 1; ind_x < FIELD_SIZE_FL; ind_x++) 
	{
		PolX[ind_x] = 0.0;
		PolY[ind_x] = 0.0;
		PolZ[ind_x] = 0.0;
		ProbM[ind_x] = 0.0;
		
		FielX[ind_x] = 0.0;
		FielY[ind_x] = 0.0;
		FielZ[ind_x] = 0.0;
		FielM[ind_x] = 0.0;
	}
	
	spacemin = 0.0; /* spacing min value between domains */
	spacemax = 0.0; /* spacing max value between domains */
	spacecurr = 0.0;

    	keyaxis = 0; /* activate rotation around 0x axis (beam direction) */	

	keysph = 0;  /* Not Activate output in the file the polarisation components */
	
	keyampldistr = 0; /* Key for choosing the distrinution of amplitude of the periodical magnetic field */
	

	while(argc>1)
	{
		switch(argv[1][1])
		{
		
			case 'O':
			Monitp=&argv[1][2];
			break;
			
			
			case 'N':
			Monitf=&argv[1][2];
			break;			
			
				
/*			case 'i':
			sscanf(&argv[1][2], "%lf", &) ;
			break;	*/


			case 'k':
			sscanf(&argv[1][2], "%lf", &PosMain[0]) ;
			break;

			case 'l':
			sscanf(&argv[1][2], "%lf", &PosMain[1]) ;
			break;

			case 'm':
			sscanf(&argv[1][2], "%lf", &PosMain[2]) ;
			break; 

			case 'p':
			sscanf(&argv[1][2], "%lf", &TranslOut[0]) ;
			break;

			case 'r':
			sscanf(&argv[1][2], "%lf", &TranslOut[1]) ;
			break;

			case 's':
			sscanf(&argv[1][2], "%lf", &TranslOut[2]) ;
			break;

			case 'X':
			sscanf(&argv[1][2], "%lf", &depth) ;
			break;

			case 'Y':
			sscanf(&argv[1][2], "%lf", &width) ;
			break;

			case 'V':
			sscanf(&argv[1][2], "%lf", &height) ;
			break;

	/* Input parameters for characterized rotaing magnetic field */

/*			case 'w':
			sscanf(&argv[1][2], "%lf", &) ;   
			break;
			
			case 'b':
			sscanf(&argv[1][2], "%lf", &) ;  
			break;						
		    

			case 'z':
			sscanf(&argv[1][2], "%lf", &) ;  
			break;	*/

			case 'd':
			sscanf(&argv[1][2], "%lf", &FieldValueInit) ; /* magnetic filed, Oe = Gauss */
			break;
			
			case 'a':
			sscanf(&argv[1][2], "%lf", &FieldValueDevPer) ; /* deviation for amplitude of periodical field in % */
			break;			

			
			case 'e':
			sscanf(&argv[1][2], "%ld", &DevLawAmpl) ; /* Law of distribution of amplitute of periodical field: 0 - Normal(Default), 1 - Uniform */
			break;						

			case 'v':
			sscanf(&argv[1][2], "%ld", &keyampldistr) ;  
			break;		
										
			
/*			case 'c':
			sscanf(&argv[1][2], "%ld", &) ; 
			break;			*/
			
			
			case 'C':
			sscanf(&argv[1][2], "%ld", &ind_x_max) ; /* Number of domains in X axis direction */
			break;
			
			
			case 'D':
			sscanf(&argv[1][2], "%ld", &ind_y_max) ; /* Number of domains in Y axis direction */
			break;
			
			case 'E':
			sscanf(&argv[1][2], "%ld", &ind_z_max) ; /* Number of domains in Z axis direction */
			break;
		

/*			case 'n':
			sscanf(&argv[1][2], "%ld", &) ;  
			break;	*/
						
		/* Input parameters for permanent magnetic field */
			case 'I':
			sscanf(&argv[1][2], "%lf", &FieldValue0Init[0]) ; /* value in Oe, OX component */
			break;						
			
			case 'A':
			sscanf(&argv[1][2], "%lf", &FieldValue0Init[1]) ; /* value in Oe, OY component */
			break;
			
			case 'K':
			sscanf(&argv[1][2], "%lf", &FieldValue0Init[2]) ; /* value in Oe, OZ component */
			break;	
			

			case 'q':
			sscanf(&argv[1][2], "%lf", &FieldValue0Dev) ;
			break;				
			
			case 'M':
			sscanf(&argv[1][2], "%ld", &keyaxis) ;  /* Values 0,1,2 - periodical magnetic field parallel axis 0x, 0y, 0z respectevly */
			break;	
											
			case 'S':
			sscanf(&argv[1][2], "%ld", &keysph) ; /* key for output polarisation components in the file during flight of domens */
			break;
			
			
/*			case 'T':
			sscanf(&argv[1][2], "%ld", &) ; 
			break;			
			
			case 'H':
			sscanf(&argv[1][2], "%ld", &keytrafter) ; 
			break;			*/
			
	/* Spacing between domains */

			case 'y':
			sscanf(&argv[1][2], "%lf", &spacemin) ; /* in cm */
			break;			
			
			case 't':
			sscanf(&argv[1][2], "%lf", &spacemax) ; /* in cm */
			break;						
			
	/* SKO in the gauss distribution of the amplitude of periodical magnetic field */		
			case 'x':
			sscanf(&argv[1][2], "%lf", &SigmaNorm) ; 
			break;					

			

		}
		argc--;
		argv++;
	}


	

    /* Check initial dates */
    
        if (keysph == 1)
	{
	    fprintf(LogFilePtr,"Activate output in the file the components of polarisation \n");
	
	    if (Monitp == NULL)
	    {
		fprintf(LogFilePtr,"\n ERROR: You must define a MonitorOutputFile for output polarisation (Option -O)!"); 
		exit(-1);
	    }
	    
	    if (Monitf == NULL)
	    {
		fprintf(LogFilePtr,"\n ERROR: You must define a MonitorOutputFile for output magnetic field (Option -N)!"); 
		exit(-1);
	    }
	    
	    if((fmonitp = fopen(Monitp,"w"))==NULL)
	    {
		fprintf(LogFilePtr,"\n ERROR: File %s could not be opened for output of polarisation components \n",Monitp);
		exit(-1);
	    }
	    
	    
	    if((fmonitf = fopen(Monitf,"w"))==NULL)
	    {
		fprintf(LogFilePtr,"\n ERROR: File %s could not be opened for output of magnetic field \n",Monitf);
		exit(-1);
	    }	    	    
	    
	}	
    
    
	if (depth <= 0.0)
	{
	    fprintf(LogFilePtr,"ERROR: Depth of magnetic field volume must be > 0 (option -X)\n");
	    exit(-1);
	}
	
	if (width <= 0.0)
	{
	    fprintf(LogFilePtr,"ERROR: Width of magnetic field volume must be > 0 (option -Y)\n");
	    exit(-1);
	}
	
	if (height <= 0.0)
	{
	    fprintf(LogFilePtr,"ERROR: Height of magnetic field volume must be > 0 (option -V)\n");
	    exit(-1);
	}


	
	
	if ((ind_x_max <= 0)||(ind_x_max >= FIELD_SIZE))
	{
	    fprintf(LogFilePtr,"ERROR: Number of domains in the X direction must be less than FIELD_SIZE=3000 and positiv! (Option -C) \n");
	    exit(-1);
	}
	
	if ((ind_y_max <= 0)||(ind_y_max >= FIELD_SIZE))
	{
	    fprintf(LogFilePtr,"ERROR: Number of domains in the Y direction must be less than FIELD_SIZE=3000 and positiv! (Option -D) \n");
	    exit(-1);
	}
	
	if ((ind_z_max <= 0)||(ind_z_max >= FIELD_SIZE))
	{
	    fprintf(LogFilePtr,"ERROR: Number of domains in the Z direction must be less than FIELD_SIZE=3000 and positiv! (Option -E) \n");
	    exit(-1);
	}

	
	if (FieldValueDevPer < 0.0) 
	{
		fprintf(LogFilePtr,"ERROR: Deviation of amplitude of periodical field must be >= 0 \n");
		exit(-1);
	}		
	
		
	if (FieldValue0Dev < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Amplitude of additional random magnetic field must be >= 0 \n");
		exit(-1);
	}


    	    switch(keyampldistr)
	    {
		    case 0:
		    {
			    fprintf(LogFilePtr,"Uniform Distribution of amplitude of periodical magnetic field. \n");
		    	    break;
		    }
		
		    case 1:	
		    {
		    	    fprintf(LogFilePtr,"Sinus Distribution of amplitude of periodical magnetic field. \n");
			    break;
		    }
		    
		    case 2:	
		    {
		    
		    		if (SigmaNorm <= 0.0)
				{
					fprintf(LogFilePtr,"ERROR: SIGMA of Gauss Distribution of amplitude of periodical magnetic field must be positive! Exit \n");	
					exit(-1);
				}
		    	    	fprintf(LogFilePtr,"Gauss Distribution of amplitude of periodical magnetic field with sigma = %f \n", SigmaNorm);
			    	break;
		    }		    

		    default:
		    {
		    	    fprintf(LogFilePtr,"ERROR: No Law! Correct option -v (Values 0, 1 or 2) \n");
		    	    exit(-1);
		    	    break;
		    }
	    }	


	
	if (FieldValueDevPer > 0.0) 
	{
		fprintf(LogFilePtr,"Activate deviation of amplitude of periodical field  %f  percent,  ", FieldValueDevPer);
			
    	    switch(DevLawAmpl)
	    {
		    case 0:
		    {
			    fprintf(LogFilePtr,"Normal (Gauss) Distribution \n");
		    	    break;
		    }
		
		    case 1:	
		    {
		    	    fprintf(LogFilePtr,"Uniform Distribution \n");
			    break;
		    }

		    default:
		    {
		    	    fprintf(LogFilePtr,"ERROR: No Law! Correct option -e (Values 0, 1)\n");
		    	    exit(-1);
		    	    break;
		    }
	    }	
			    
	}			    
			
	
		/* Display keyaxis */

		    switch(keyaxis)
		    {
		    case 0:
			{
			/* Choose parallel of axis 0X */
			    fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0X \n");
			    break;
			}
		    case 1:	
			{
			/* Choose parallel of axis 0Y */
			    fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0Y \n");	
			    break;
			}
		    case 2:
			{
			/* Choose parallel of axis 0Z */
			    fprintf(LogFilePtr,"Periodical magnetic field parallel of axis 0Z \n");			
			    break;
			}
		    default:
			{
			    fprintf(LogFilePtr,"ERROR: No axis! Correct option -M (Values 0, 1, 2)\n");
			    exit(-1);
			    break;
			}
		    }	

	

	    
	FieldValue = FieldValueInit ;

    /* define the permanent(guide) magnetic field */

	FieldValue0[0] = FieldValue0Init[0]; 
	FieldValue0[1] = FieldValue0Init[1]; 
	FieldValue0[2] = FieldValue0Init[2]; 
	
	

	/*	Calculate first resonanse conditions for permanent field components	*/		
	
	if ((FieldValue0[0] != 0.0)||(FieldValue0[1] != 0.0)||(FieldValue0[2] != 0.0))
	{
		    fprintf(LogFilePtr,"Drabkin RF flipper: Permanent magnetic field with values: Xo = %f Oe , Yo = %f Oe , Zo = %f Oe \n",FieldValue0[0], FieldValue0[1], FieldValue0[2]);
		    FieldValue0Length = LengthVector(FieldValue0);
		    fprintf(LogFilePtr,"Permanent magnetic field module =  %f  \n",FieldValue0Length);
		    OmegaFV0 = (FREQUENCY_FROM_FIELD(FieldValue0Length));  /* KHz*2pi */
		    OmegaFV0in = OmegaFV0*(depth/(double)(ind_x_max))/M_PI;
		    if (OmegaFV0in == 0) 
		    {
			fprintf(LogFilePtr,"ERROR: You have a zero velocity!!! exit...\n");
			exit(-1);
		    }
		    OmegaFV0in = 395.60346/OmegaFV0in;
		    fprintf(LogFilePtr,"First resonance condition: Resonanse wavelength is expected = %f Ang \n", OmegaFV0in);
		    fprintf(LogFilePtr,"----------------------------------------------------------------------------------------\n");
	}	

	
    /* Calculate PI-flipping condition, ONLY FOR Uniform Distribution of amplitude of periodical magnetic field. */	
    if (keyampldistr == 0)
    {
    	fprintf(LogFilePtr,"Second resonance condition ONLY FOR Uniform Distribution of amplitude of periodical magnetic field.\n");
	OmegaFV0in = FieldValue0Length*M_PI/(2.0*ind_x_max);
	fprintf(LogFilePtr,"Amplitude of the periodical magnetic field  =  %f Oe\n", OmegaFV0in);
	fprintf(LogFilePtr,"---------------------------------------------------------------------------------------------------\n");    
    }

	
	if ((FieldValue0[0] == 0.0)&&(FieldValue0[1] == 0.0)&&(FieldValue0[2] == 0.0))
	{
		if (FieldValue0Dev > 0.0) 
		{
			fprintf(LogFilePtr,"WARNING!!! ONLY periodical magnetic field plus randomization. No additional permanent magnetic field! \n");
		}
		else
		{	
			fprintf(LogFilePtr,"WARNING!!! ONLY periodical magnetic field! No additional permanent magnetic field! \n");		
		}	
	}	


	if (FieldValue0Dev > 0.0) 
	fprintf(LogFilePtr,"Activate additional random magnetic field with amplitude  %f  Oe \n", FieldValue0Dev);	
	
	if ((spacemin < 0.0)||(spacemax < 0.0))				
	{
	    fprintf(LogFilePtr,"ERROR: Spacing min and Spacing max must be both positive or zero! Exit.\n");
	    exit(-1);
	}
	
	if (spacemin > spacemax)
	{
	    fprintf(LogFilePtr,"ERROR: Spacemax must be more than Spacemin\n");
	    exit(-1);
	}
	
	if ((spacemin > 0)||(spacemax > 0))
	{
	    fprintf(LogFilePtr,"Spacing inside resonator is activated:\n");
	    fprintf(LogFilePtr,"Uniform distribution between: Space_min = %f cm  Spacemax = %f cm\n",spacemin,spacemax);
	}    


/*	 init for ASCII output */

	NumOut=0 ;


	IntegralIntensity = 0. ;

}/* End OwnInit */


/* own cleanup of the monochromator/analyser module */

void OwnCleanup()
{
	VectorType PolP;
	double roty, rotz, Theta, Phi, ModuleV;

	if (NumOut != 0) 
	{
	    fprintf(LogFilePtr,"Full Number of precessions  : %f\n", NumberPrecessions) ;
	    if (Number_NOP != 0.0)
	    fprintf(LogFilePtr,"Ave Number of precessions  : %f\n", NumberPrecessions/Number_NOP) ;		    
	    
	    
	    fprintf(LogFilePtr,"All Number of precessions  : %f\n", NumberPrecessionssum/NumOut) ;	    
	    fprintf(LogFilePtr,"Average number of precession per domains : %f\n", NumberPrecessionsave/NumOut);
	
	
	    if (keysph == 1)	
	    {

		for(ind_x=1; ind_x < indp; ind_x++) 
		{
			if (ProbM[ind_x] != 0.0)
			{
		
				/* Calculate polarisation vector in cartesian coordinates */	
	    			PolP[0] = PolX[ind_x]/ProbM[ind_x];
				PolP[1] = PolY[ind_x]/ProbM[ind_x]; 
				PolP[2] = PolZ[ind_x]/ProbM[ind_x];
				
				/* Calculate module of polarisation vector */
				ModuleV = sqrt((PolX[ind_x]/ProbM[ind_x])*(PolX[ind_x]/ProbM[ind_x]) + 
				(PolY[ind_x]/ProbM[ind_x])*(PolY[ind_x]/ProbM[ind_x]) + 
				(PolZ[ind_x]/ProbM[ind_x])*(PolZ[ind_x]/ProbM[ind_x]));			
				
				/* 'CartesianToSpherical' calculates Theta and Phi of a unit vector  */
				/* if Theta is the angle with the axis of lowest index               */
				CartesianToSpherical(PolP, &Theta, &Phi);
				
				/* 'CartesianToEulerZY' calculates Euler angles 'rotz' and 'roty'         */
				/* to transfer the x-axis to 'Vector' by rotation around y- and z-axis ZY */
				/* (cf. FillRotMatrixZY)                                                  */
				CartesianToEulerZY(PolP, &roty, &rotz);

		/* Ouput in file */
		fprintf(fmonitp,"%f   %f   %f   %f        %f    %f    %f        %f    %f   %f\n", 
		PolP[0], PolP[1], PolP[2], ModuleV, Theta, Phi, ModuleV, roty, rotz, ModuleV);
			}
			
			
			if (FielM[ind_x] != 0.0)
			{
			
			
				fprintf(fmonitf,"%f     %f     %f     %f  \n", 
				FielX[ind_x]/FielM[ind_x], 
				FielY[ind_x]/FielM[ind_x], 
				FielZ[ind_x]/FielM[ind_x],
				sqrt((FielX[ind_x]/FielM[ind_x])*(FielX[ind_x]/FielM[ind_x]) +  
			    	(FielY[ind_x]/FielM[ind_x])*(FielY[ind_x]/FielM[ind_x]) + 
			    	(FielZ[ind_x]/FielM[ind_x])*(FielZ[ind_x]/FielM[ind_x])));
			}	
			
			
				
		}	
	
	    }
	}
	else
	{ 
	    fprintf(LogFilePtr,"ERROR: No neutrons in the exit of the precession volume, exit!!!\n");	    
	    exit(-1);
	}

    	if (keysph == 1)    
	{
		fclose(fmonitp);
		fclose(fmonitf);
	}	

}/* End OwnCleanup */




/* Intersection with rectangular object */

long IntersectionWithRectangularWallNumber(VectorType DimDomain, VectorType Pos, VectorType Dir, VectorType Pos1, VectorType Pos2, long *wall_1, long *wall_2)
{
VectorType	n, pos0, pos1, pos2, pos3, pos4, pos5 ;
int			k ;

for(k=0;k<3;k++) Pos1[k] = Pos2[k] = 0. ;

n[0] = 1. ; n[1] = n[2] = 0. ; *wall_1 = *wall_2 = 0 ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[0]/2, pos0) == TRUE)
	{
		if(  (fabs(pos0[1]) <= DimDomain[1]/2) && (fabs(pos0[2]) <= DimDomain[2]/2) ) 
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos0, Pos1) ; *wall_1 = 1;}
			else {CopyVector(pos0, Pos2) ; *wall_2 = 1;}
		}	
	} 
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[0]/2, pos1) == TRUE)
	{
		if(  (fabs(pos1[1]) <= DimDomain[1]/2) && (fabs(pos1[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos1, Pos1) ; *wall_1 = 2;}
			else {CopyVector(pos1, Pos2) ; *wall_2 = 2;}
		}
	}

n[1] = 1. ; n[2] = n[0] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[1]/2, pos2) == TRUE)
	{
		if( (fabs(pos2[0]) <= DimDomain[0]/2) &&  (fabs(pos2[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos2, Pos1) ; *wall_1 = 3;}
			else {CopyVector(pos2, Pos2) ; *wall_2 = 3;}
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[1]/2, pos3) == TRUE)
	{
		if( (fabs(pos3[0]) <= DimDomain[0]/2) &&  (fabs(pos3[2]) <= DimDomain[2]/2) )
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos3, Pos1) ; *wall_1 = 4;}
			else {CopyVector(pos3, Pos2) ; *wall_2 = 4;}
		}
	}

n[2] = 1. ; n[0] = n[1] = 0. ;

	if(PlaneLineIntersect2(Pos, Dir, n, - DimDomain[2]/2, pos4) == TRUE)
	{
		if( (fabs(pos4[0]) <= DimDomain[0]/2) && (fabs(pos4[1]) <= DimDomain[1]/2)  ) 
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos4, Pos1) ; *wall_1 = 5;}
			else {CopyVector(pos4, Pos2) ; *wall_2 = 5;}
		}
	}
	if(PlaneLineIntersect2(Pos, Dir, n, + DimDomain[2]/2, pos5) == TRUE)
	{
		if( (fabs(pos5[0]) <= DimDomain[0]/2) && (fabs(pos5[1]) <= DimDomain[1]/2)  ) 
		{
			if(LengthVector(Pos1) == 0.) {CopyVector(pos5, Pos1) ; *wall_1 = 6;}
			else {CopyVector(pos5, Pos2) ; *wall_2 = 6;}
		}
	}

	if((LengthVector(Pos1) == 0.) || (LengthVector(Pos2) == 0.)) return 0 ;

	
	return 1 ;

}/* End IntersectionWithRectangularWallNumber() */
	
	    

 	
double DistrGauss(double Module, double Sigma)
{
    double Res, Norm;
    long i;
    
    Norm = 0.0;
    for(i = 1; i <= 12; i++)
    {
	Norm = Norm + ran3(&idum);
    }
    Norm = (fabs(Sigma))*(Norm - 6.0);
    Res = Module + Norm;
    return Res;
}







	
	
