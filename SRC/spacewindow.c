/*********************************************************************************************/
/*  VITESS module  spacewindow                                                               */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*       June 1999  D. Wechsler                                                              */
/* 1.00  Feb  2001  S. Manoshin     include of gravity effect                                */
/* 1.01  June 2001  K. Lieutenant   parameter S to simulate a beamstop + SOFTABORT           */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation                                           */
/* 2.00  Jun  2003  S. Manoshin     Add possibility for simulations of outer material of     */
/*	                                 collimator: 0 - from file, 1 - gadolinium, 2 - cadmium, */
/*                                  3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber        */
/* 2.10  Mar  2004  S. Manoshin     Add "choosing" of material for inner part of collimator  */
/* 2.21  Jul  2004  S. Manoshin     Corrected some bugs for thick collimator                 */
/* 2.22  Jan  2002  K. Lieutenant   correction:  position after beamstop                     */
/* 2.23  May  2010  A. Houben       "Rotation" of square window by counter rot of neutron pos*/
/* 2.24  Apr  2012  A. Houben       Treat only neutrons with a given color and phi angle     */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"
#include "matrix.h"


/******************************/
/** Prototypes               **/
/******************************/

void  OwnInit(int argc, char *argv[]);
void  SetGeometryData();

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
        zwincenter,      /* y coordinate: center of circular window                [cm] */
        rotang = 0.0;    /* Rotation angle (neutron pos is counter rot to window)   [°] */


  long  keymaterial0=6; /*Material of collimator: 0 - from file, 1 - gadolinium, 2 - cadmium,
	      3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
  long  keymaterial1=1; /* Absorbtion of open part of collimator, 1 -no(default) 0 - from file */
  long ntfs=0, count, k;
  long ntfss=0, ntfs1=0; /* internal */
  double WAVS[500], MUS[500], transm0[1001];
 /* Material of collimator */
  double WAV1[500], MU1[500], transm1[1001]; /* Material of open part of collimator */
  char		*TransFileName0=NULL;
  char		*TransFileName1=NULL;
  FILE	*trans_file0=NULL; /* file for describing of transmission of material of collimator  */
  FILE  *trans_file1=NULL; /* file for describing of transmission of open part of collimator */

  double VelocityReal, N_Wavelength , mu, prob=0.0;
  double Thicknesscoll=0.0;
  double Thicknesscolli=0.0;
  double DistMove=0.0;
  short  TreatColor = -1; // Treat only neutrons with a given color
  short  RemoveOtherColor = FALSE; // If treated only neutrons with a given color, all others are removed
  double minPhi=-1.0, maxPhi=-1.0;  //Angle in xz plane


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	short bOutOfWindow=FALSE;

	long  i, BufferIndex;

	double tempdistsquared;
	double TimeOF;
	double NewPositionY, NewPositionZ, Phi;
	double CenterX, CenterY, CenterZ, SumProb, TOF3 ;

	Neutron Output;

	/* for moving */
	Plane EndPoint1 ;
	Plane EndPoint2 ;


	/* initialisation */
	heightmin = heightmax = widthmin = widthmax = 0.0;
	winradius = ywincenter = zwincenter = 0.0;

	BufferIndex     = 0;
	bCircularWindow = TRUE,
	bBeamStop       = FALSE;

	Init(argc, argv, VT_WINDOW);
	OwnInit(argc, argv);

	print_module_name("Space and Window 2.24");

	if (TransFileName0 != NULL) trans_file0 = fopen(TransFileName0,"r");

	if (TransFileName1 != NULL)
	{
	    trans_file1 = fopen(TransFileName1,"r");
	    fprintf(LogFilePtr,"MATERIAL OF OPEN PART OF COLLIMATOR IS DESCRIBED BY FILE:  %s \n", TransFileName1);
	    keymaterial1 = 0; /* activate this material */
	}


	if (DistMove < 0.0 && bOldFrame == FALSE)
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
		fprintf(LogFilePtr,"WARNING: It is recommeded to have outer and inner thickness EQUAL! \n");
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
	    fprintf(LogFilePtr,"OUTER MATERIAL OF COLLIMATOR: Material transmission characteristics read from file\n");
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
	    // fprintf(LogFilePtr,"Outer material of  collimator: ideal absorber \n");
  	}

	if ((keymaterial0 != 0)&&(keymaterial0 != 1)&&(keymaterial0 != 2)&&(keymaterial0 != 3)&&(keymaterial0 != 4)&&(keymaterial0 != 5)&&(keymaterial0 != 6))
  	{
  	    fprintf(LogFilePtr,"ERROR: OUTER MATERIAL OF COLLIMATOR: No material: EXIT! Correct -c option \n");
	    exit(-1);
  	}


	if (Thicknesscoll > 0)
     fprintf(LogFilePtr,"Thickness of outer material of collimator %f cm \n", Thicknesscoll);
	if (Thicknesscolli > 0)
	  fprintf(LogFilePtr,"Thickness of inner material of collimator %f cm \n", Thicknesscolli);



	  for(i=0; i<500; i++)
	  {
		WAVS[i] = 0.0;
		MUS[i] = 0.0;
		WAV1[i] = 0.0;
		MU1[i] = 0.0;
	  }

	  for(i=0; i<=1000; i++)
	  {
		transm0[i] = 0.0;
		transm1[i] = 0.0;
	  }



////////////
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
	fprintf(LogFilePtr,"No file, which describes transmission of collimator \n");
    }
/////////////////



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
	Endpoint.D = -1.0*DistMove;


	EndPoint1.A = 1.0;
	EndPoint1.B = 0.0;
	EndPoint1.C = 0.0;
	EndPoint1.D = -1.0*(Thicknesscolli+DistMove);


	EndPoint2.A = 1.0;
	EndPoint2.B = 0.0;
	EndPoint2.C = 0.0;
	EndPoint2.D = -1.0*(Thicknesscoll+DistMove);


	SetGeometryData();

	/*if(keygrav == 1)
	{
		fprintf(LogFilePtr,"Gravity is enabled \n");
	}
	else
	{
		fprintf(LogFilePtr,"Gravity is disabled \n");
	}*/

	CenterX   = 0.0;
	CenterY   = 0.0;
	CenterZ   = 0.0;
	SumProb   = 0.0;

	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK

			/****************************************************************************************/
			/* 	Move neutron to window with gravity effect and calculate Time of Flight (ms).   */
			/****************************************************************************************/

			  if ((TreatColor >= 0) && (InputNeutrons[i].Color != TreatColor)) {
			    Output = InputNeutrons[i];
			    if (!RemoveOtherColor) {
			        WriteIAP(&Output, VT_EXITED);
			        WriteNeutron(&Output);
			    }
			    continue;
			  }
			
			if (bOldFrame==FALSE) {
			  if (InputNeutrons[i].Vector[0] <= 0.0) continue;
			  if (InputNeutrons[i].Wavelength == 0.0) continue;
			  VelocityReal = (double)(V_FROM_LAMBDA(InputNeutrons[i].Wavelength));
			  if (VelocityReal <= 0.0) continue;
			} 
			else {

			  Output = InputNeutrons[i];			

			}
			
			WriteIAP(&InputNeutrons[i], VT_ENTERED);

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


			InputNeutrons[i].Time += (double)TimeOF;
			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0];
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1];
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2];
			SumProb   += InputNeutrons[i].Probability;
			TOF3 = 0.0;

			/* window test */

			if (rotang != 0.0) {
				/*x' = x cos f - y sin f
			      y' = y cos f + x sin f */
				NewPositionY = InputNeutrons[i].Position[1] * cos(-rotang) - InputNeutrons[i].Position[2] * sin(-rotang);
				NewPositionZ = InputNeutrons[i].Position[2] * cos(-rotang) + InputNeutrons[i].Position[1] * sin(-rotang);
			} else {
				NewPositionY = InputNeutrons[i].Position[1];
				NewPositionZ = InputNeutrons[i].Position[2];
			}

      if (minPhi >= 0 && maxPhi <= 360) {
        //CartesianToSpherical(InputNeutrons[i].Vector, &TwoTheta, &Phi);
        //Phi = Phi*180.0/M_PI;
        Phi	= (double)atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[2])*180.0/M_PI+180.;
        if (Phi < minPhi || Phi > maxPhi) {
	  WriteIAP(&Output, VT_ABSORBED);
	  continue;
	}
      }

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
			if ((bBeamStop==TRUE  && bOutOfWindow==TRUE) ||
			    (bBeamStop==FALSE && bOutOfWindow==FALSE))
			{


				if (keygrav == 1)
				{
					TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPoint1);
				}
				else
				{
					TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPoint1);
				}


//				fprintf(LogFilePtr,"INN  TOF3 = %f  TimeOF = %f \n", TOF3, TimeOF);

				if (keymaterial1 == 0)
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
//	 				 fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
	 				 InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
				 }


        if (bOldFrame==FALSE) {
	  WriteIAP(&InputNeutrons[i], VT_EXITED);
	  InputNeutrons[i].Position[0]=0.0;
          InputNeutrons[i].Time += (double)TOF3 ;
	  Output = InputNeutrons[i];
        } 
	else {
           InputNeutrons[i]=Output;
	   WriteIAP(&InputNeutrons[i], VT_EXITED);
	}
				WriteNeutron(&Output);
      }
			else /* else, if hitting beamstop or out of window */
			{

				if (keymaterial0 != 6)
				{
					if (keygrav == 1)
					{
					    TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPoint2);
					}
					else
					{
					    TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPoint2);
					}

//					fprintf(LogFilePtr,"OUT  TOF3 = %f   TimeOF = %f \n", TOF3, TimeOF);

				     /* Attenuation during pass of collimator material */
					 N_Wavelength = InputNeutrons[i].Wavelength;
					 mu = Interpolation(N_Wavelength,keymaterial0,WAVS,MUS,ntfss);
					 if (mu == -10000.0)
					 {
					    fprintf(LogFilePtr,"ERROR: module spacewindow: EXIT!\n");
					    exit(-1);
					 }
					 prob = exp(-mu*TOF3*VelocityReal);
//	 				 fprintf(LogFilePtr,"surf prob = %f mu = %f \n",prob,mu);
	 				 InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
	 				 InputNeutrons[i].Time += (double)TOF3;
	 				 InputNeutrons[i].Position[0]=DistMove;
					 Output = InputNeutrons[i];
				 	 if (InputNeutrons[i].Probability <= wei_min) {
					   WriteIAP(&Output, VT_ABSORBED);
					 }
					 else {
					   WriteIAP(&Output, VT_EXITED);
					   WriteNeutron(&Output);
					 }
				 }
				else {
				  if (keygrav == 1)
				    {
				      TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPoint2);
				    }
				  else
				    {
				      TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPoint2);
				    }
				   InputNeutrons[i].Probability = 0.;
				   InputNeutrons[i].Position[0]=DistMove;
				   InputNeutrons[i].Time += (double)TOF3;
				   WriteIAP(&InputNeutrons[i], VT_ABSORBED);
				}
			}
		}
	}

 my_exit:
	if (bCircularWindow)
	  fprintf(LogFilePtr,"Window of %6.2f cm diameter in a distance of %7.2f cm \n", 2.0*winradius, DistMove);
	else
	  fprintf(LogFilePtr,"Window of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", 
	                     widthmax-widthmin, heightmax-heightmin, DistMove);

	if (SumProb != 0.0)
	{
		CenterX   = CenterX/SumProb;
		CenterY   = CenterY/SumProb;
		CenterZ   = CenterZ/SumProb;
		fprintf(LogFilePtr,"Center of beam: X = %f cm Y = %f cm Z = %f cm \n",CenterX, CenterY, CenterZ);
	}
	else
	{
		fprintf(LogFilePtr,"No neutrons at the exit of this module \n");
	}
  if (TreatColor >= 0) fprintf(LogFilePtr,"Only neutrons with color %hd are treated \n", TreatColor);

	fprintf(LogFilePtr," \n");


	if (Thicknesscolli >= Thicknesscoll)
	{
		Cleanup((Thicknesscolli+DistMove), 0.0, 0.0, 0.0, 0.0);
	}
	else
	{
		Cleanup((Thicknesscoll+DistMove), 0.0, 0.0, 0.0, 0.0);
	}



	return(0);
}



void  OwnInit(int argc, char *argv[])
{
	int i;
	short bOFrame=FALSE; /* default for window 'new frame' */

	bOldFrame = -1;      /* no default for frame in general */

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+')
		{
			switch(argv[i][1])
      	{
			case 'l':
				DistMove = atof(&argv[i][2]);
				break;

			case 'S':
				bBeamStop = (short) atol(&argv[i][2]);
            bOFrame   = TRUE;   /* default for beamstop 'prev. frame' */
				break;

			case 'F':
				bOldFrame = (short) atol(&argv[i][2]);
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
			case 'A':
				rotang = atof(&argv[i][2])*M_PI/180.;
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
			case 'm':
				TransFileName1=&argv[i][2];
				break;

			case 't':
				Thicknesscoll = atof(&argv[i][2]);
				/* in cm, outer material */
				break;
			case 'T':
				Thicknesscolli = atof(&argv[i][2]);
				/* in cm, inner material */
				break;
      case 'f':
				sscanf(&(argv[i][2]),"%hd", &TreatColor);
				break;
      case 'd':
				sscanf(&(argv[i][2]),"%hd", &RemoveOtherColor);
				break;
      case 'p':
				minPhi = atof(&argv[i][2]);
				/* in deg, min angle in yz plane */
				break;
      case 'P':
				maxPhi = atof(&argv[i][2]);
				/* in deg, max angle in yz plane */
				break;

			default:
				fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
				exit(-1);
				break;
			}
		}
	}

	/* take default value for frame, if not explicitely set */
	if (bOldFrame==-1)
		bOldFrame=bOFrame;
}







void SetGeometryData()
{

  double rotMatrixPi2[3][3];
  
  bVisInstalled = TRUE;
 // Geometry data
  if (bVisInstr)
  { 

    if (bCircularWindow) {
      
      stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
      stGeometry.nCylinders = 1;

      stGeometry.pCylinder[0].Radius = winradius;
      stGeometry.pCylinder[0].Length = Max(Thicknesscoll, Thicknesscolli);
      stGeometry.pCylinder[0].vCntr[0]  = DistMove + stGeometry.pCylinder[0].Length/2.;
      stGeometry.pCylinder[0].vCntr[1]  = ywincenter;
      stGeometry.pCylinder[0].vCntr[2]  = zwincenter;
      stGeometry.pCylinder[0].vSymAxis[0] = 1.;
      stGeometry.pCylinder[0].vSymAxis[1] = 0.;
      stGeometry.pCylinder[0].vSymAxis[2] = 0.;
      
      stGeometry.pDescr  = "space window";
      stGeometry.eModule = VT_WINDOW;

    }
    else {

    stGeometry.pCuboid = calloc(1, sizeof(VtCuboid));
    stGeometry.nCuboids = 1; 

    stGeometry.pCuboid[0].Length = widthmax - widthmin;
    stGeometry.pCuboid[0].Width  =  Max(Thicknesscoll, Thicknesscolli);
    stGeometry.pCuboid[0].Height = heightmax - heightmin;
    stGeometry.pCuboid[0].vCntr[0]  = DistMove + stGeometry.pCuboid[0].Width/2.;
    stGeometry.pCuboid[0].vCntr[1]  = ywincenter;
    stGeometry.pCuboid[0].vCntr[2]  = zwincenter;
    stGeometry.pCuboid[0].vNormal[0]= cos(rotang);
    stGeometry.pCuboid[0].vNormal[1]= 0.;
    stGeometry.pCuboid[0].vNormal[2]= sin(rotang);

    FillRotMatrixZY(rotMatrixPi2, 0, M_PI_2);
    RotVector(rotMatrixPi2, stGeometry.pCuboid[0].vNormal);

    stGeometry.pDescr  = "space window:cyan";
    stGeometry.eModule = VT_WINDOW;

    }
  }
}
