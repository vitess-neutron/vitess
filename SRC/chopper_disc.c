/*********************************************************************************************/
/*  VITESS module  SPACE and CHOPPER                                                         */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.00  Jun 1999  D. Wechsler                                                               */
/* 1.01  Mar 2001  S. Manoshin    include of gravity effect                                  */
/* 1.02  Jun 2001  K. Lieutenant  SOFTABORT                                                  */
/* 1.03  Jan 2002  G. Zsigmond    weighted center of beam                                    */
/* 1.04  Jan 2002  K. Lieutenant  reorganisation                                             */
/* 1.05  Apr 2003  K. Lieutenant  horizontal distance                                        */
/* 1.06  Nov 2003  K. Lieutenant  chopper sets colour to window number  and                  */
/*                                choice: treatment of neutrons outside chopper              */
/* 1.07  Jan 2004  K. Lieutenant  changes for 'instrument.dat'; correction 'time to zero'    */
/*                                new: absorption by Bor-10, absorption by Gd changed        */
/* 1.08  Feb 2004  K. Lieutenant  'message.h', 'ERROR' and 'FullParName' included; output of */
/*                                parameter file data; optimal phase into 'instrument.inf'   */
/* 1.09  Nov 2005  K. Lieutenant  option: equivalent windows added                           */
/*********************************************************************************************/

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "bender_inter_data.h"
#include "message.h"


/******************************/
/** Structures               **/
/******************************/

typedef struct
{
	double  Distance;
	double  Angle;
	double  Left, Right;
	double  Bottom;
	double  LiveZoneStart, LiveZoneEnd;
}
ChopperWindow;

typedef struct
{
	short          NumberOfWindows;
	CartesianPoint Centre;          /* centre of the chopper in the coordinate system of the beamline [cm] */
	double         Radius;          /* radius of the chopper  [cm] */
	double         Frequency;       /* rot.freq 2*pi*60*rpm  */
	double         Angle;           /* orientation of the center of beamline in the chopper system */
	ChopperWindow  *Window;
}
Chopper;



/******************************/
/** Prototypes               **/
/******************************/

void           OwnInit         (int argc, char *argv[]);
void           OwnCleanup      ();
void           ReadChopperData ();
double         RedAngle        (double angle, short dir);
double         ModPhase        (double phase, int nSect);
unsigned short BlockedByChopper(Chopper, Neutron*);


/***********************************/
/** global and static variables   **/
/***********************************/

Plane	 Endpoint;
double ChopperInitialOffset;
short  eAbsMaterial = FALSE,   /* absorption in chopper: 0: ideal  1:Gadolinium  2: Bor */
       bPassOutside = TRUE,    /* criterion: neutrons can pass outside the chopper      */
       bSetColour   = TRUE,    /* criterion: chopper sets colour to window number       */
       bZeroTime    = FALSE,   /* criterion: chopper sets neutron time to zero          */
		 bPhase       = FALSE,   /* criterion: write opt. chopper phases to instrument.inf*/
       NumEquWnds   = 1;       /* number of equivalent windows used to generate pulses  */
FILE	*ChopperFile=NULL;
char	*ChopperFileName=NULL;
Chopper ThisChopper;


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	long   i;

	double mu,    /* absorption coeffient of the absorbing material [1/cm] */
	       prob;  /* resulting attenuation inside absorbing material       */

	double TimeOF,
	       AveTimeOF,                 /* average time of flight to chopper weighted by count rate */
	       SumProb,                   /* sum of count rates of all traj. reaching the chopper */
	       CenterX, CenterY, CenterZ; /* center of beam of all traj. reaching the chopper at chopper weighted by count rate */

	 /* initialisation, definitions see Input below */
	ChopperInitialOffset = prob = 0.0;
	ThisChopper.Centre.X = ThisChopper.Centre.Y = 0.0;
	Endpoint.D           = 0.0;

	Init(argc, argv, VT_CHOP_DISC);
	print_module_name("Space and Chopper 1.9");
	OwnInit(argc, argv);

	CenterX   = 0.0;
	CenterY   = 0.0;
	CenterZ   = 0.0;
	AveTimeOF = 0.0;
	SumProb   = 0.0;

	/* Reading chopper file  */
	ReadChopperData();

	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK

			/****************************************************************************************/
			/* Submit both the Neutron and the plane to a subroutine and find the intercept.        */
			/****************************************************************************************/
			if (InputNeutrons[i].Position[0] > -Endpoint.D)
				CountMessageID(ALL_BEHIND_COMPONENT, InputNeutrons[i].ID);

			/****************************************************************************************/
			/* 	Move neutron to window with gravity effect and calculate Time of Flight (ms).   */
			/****************************************************************************************/
			if (keygrav == 1)
			{
				TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
			}
			else
			{
				TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);
			}

			InputNeutrons[i].Time += (double)TimeOF;

			AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0];
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1];
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2];
			SumProb   += InputNeutrons[i].Probability;

			InputNeutrons[i].Position[0]=0.0;


			/****************************************************************************************/
			/* This really is self explanatory; submit the neutron to a subroutine that works out if */
			/* the chopper gets in the way.															 */
			/****************************************************************************************/

			if(BlockedByChopper(ThisChopper, &InputNeutrons[i]))
			{
				/* non perfect absorption */

				switch (eAbsMaterial)
				{	/* ideally absorbing material */
					case 0:
						continue;
						break;

					/* gadolinium */
					case 1:
						if (InputNeutrons[i].Wavelength <= 0.35)
						{	prob = -0.184285714*InputNeutrons[i].Wavelength  + 1.0127875714286;
							if (prob >0.961) prob = 0.961;
						}
						else if ( InputNeutrons[i].Wavelength < 6.0)
						{	double *pLmbdList=NULL, *pMuList=NULL;

							mu   = Interpolation(InputNeutrons[i].Wavelength, 1, pLmbdList, pMuList, 44);
							prob = exp(-mu*0.02);  /* typical thickness 2 x 100 um */
						}
				 		else
						{	prob = 0.0;
						}
						break;

					/* Bor-10 */
					case 2:
						if (InputNeutrons[i].Wavelength < 0.29)
						{	double eV, mcnp;

							eV   = 1.0e-06*ENERGY_FROM_LAMBDA(InputNeutrons[i].Wavelength);
							mcnp = 612.07/sqrt(eV);
							mu   = mcnp * NA * 2.46E-24 / 10.811;
							prob = exp(-mu*0.05);  /* typical thickness 2 x 250 um */
						}
						else if ( InputNeutrons[i].Wavelength < 6.0)
						{	double *pLmbdList=NULL, *pMuList=NULL;

							mu   = Interpolation(InputNeutrons[i].Wavelength, 3, pLmbdList, pMuList, 44);
							prob = exp(-mu*0.05);  /* typical thickness 2 x 250 um */
						}
				 		else
						{	prob = 0.0;
						}
						break;

					default:
						Error("This kind of absorption is not supported");
				}
				InputNeutrons[i].Probability *= prob;
			}

			if (InputNeutrons[i].Probability < 0.0)
			{
				Error("NeutronProbability < 0");
			}
			else if (InputNeutrons[i].Probability < wei_min)
			{
				continue;
			}
			else
			{	// Output = InputNeutrons[i];
				WriteNeutron(&InputNeutrons[i]);
			}
		}
	}

 my_exit:
	if (SumProb != 0.0)
	{
		CenterX = CenterX/SumProb;
		CenterY = CenterY/SumProb;
		CenterZ = CenterZ/SumProb;
		AveTimeOF = AveTimeOF/SumProb;
		fprintf(LogFilePtr,"Center of beam before the chopper: X = %f cm Y = %f cm Z = %f cm TOF = %f ms \n",CenterX, CenterY, CenterZ, AveTimeOF);
	}
	else
	{
		Warning("No neutron hit the chopper");
	}

	OwnCleanup();
	Cleanup(-Endpoint.D,0.0,0.0, 0.0,0.0);

	return(0);
}



void OwnInit   (int argc, char *argv[])
{
	long   i;
	double Rpm, Period;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') {

			switch(argv[i][1])
			{
			case 'l':
				Endpoint.A = 1.0;
				Endpoint.B = 0.0;
				Endpoint.C = 0.0;
				Endpoint.D = -atof(&argv[i][2]);
				break;


			case 'g':
				/* g unequal 0 => non perfect chopper absorption activated */
				eAbsMaterial =  (short) atol(&argv[i][2]);
				switch (eAbsMaterial)
				{	case 0: fprintf(LogFilePtr,"Ideal absorption in chopper disk assumed\n"); break;
					case 1: fprintf(LogFilePtr,"Absorption of Gd in chopper disk assumed\n"); break;
					case 2: fprintf(LogFilePtr,"Absorption of Bor-10 in chopper disk assumed\n"); break;
					default: Error("This kind of absorption is not supported");
				}

				break;

			case 'z':
				/* z unequal 0 => chopper sets time to zero */
				bZeroTime = (short) atol(&argv[i][2]);
				if (bZeroTime)
					fprintf(LogFilePtr,"Time set to zero\n");
				break;

			case 'c':
				/* c unequal 0 => chopper sets colour to window number */
				bSetColour =  (short) atol(&argv[i][2]);
				if (bSetColour)
					fprintf(LogFilePtr,"Colours of trajectories will be set to window number\n");
				break;

			case 'p':
				/* z unequal 0 => chopper sets time to zero */
				bPassOutside = (short) atol(&argv[i][2]);
				if (bPassOutside==FALSE)
					fprintf(LogFilePtr,"Neutrons passing outside the chopper are removed\n");
				break;


			case 'C':
				if((ChopperFile = fopen(FullParName(&argv[i][2]),"r"))==NULL)
				{
					fprintf(LogFilePtr,"ERROR: Chopper parameter file %s could not be opened\n",&argv[i][2]);
					exit(-1);
				}
				ChopperFileName = &argv[i][2];

				break;

			case 'n':										/* no of windows for pulse generation */
				NumEquWnds = (short) atol(&argv[i][2]);
				break;

			case 'o':										/*Offset [deg] */
				ChopperInitialOffset = atof(&argv[i][2]);
				ChopperInitialOffset = 2.0*M_PI*ChopperInitialOffset/360.0;
				break;

			case 's':
				Rpm = atof(&argv[i][2]);					/*Rounds per minute*/
				ThisChopper.Frequency = 2.0*M_PI*Rpm/60.0;
				Period = 60.0/Rpm;
				break;

			default:
				fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
				exit(-1);
				break;
			}
		}
	}
}


/* own cleanup of this module */
/* -------------------------------- */
void OwnCleanup()
{
	long   nModuleNo;    /* number of the previous module (not needed) */
	short  dir=0;
	double time, phi0=0.0,
	       phi_wnd1, phi_wnd2, phi_wnd3;
	double dTimeMeas,    /* measuring time     (from simulation.inf, not needed here) */
	       dLmbdWanted,  /* desired wavelength (from simulation.inf)                  */
	       dFreq,        /* source frequency   (from simulation.inf, not needed here) */
	       dLength,      /* length of the instrument until chopper module */
	       dRotZ, dRotY; /* orientation of the output of the previous component (not needed) */
	VectorType EndPos;   /* position of the output of the previous component (not needed) */

	/* print error that might have occured many times */
	PrintMessage(CHOP_PASSED_OUTSIDE,  "", ON);
	PrintMessage(ALL_BEHIND_COMPONENT, "", ON);

	fprintf(LogFilePtr," \n");

	/* set description for instrument plot */
	ReadSimData  (&dTimeMeas, &dLmbdWanted, &dFreq);
	if (dLmbdWanted > 0.0)
	{	ReadInstrData(&nModuleNo, EndPos, &dLength, &dRotZ, &dRotY);
		time = (dLength-0.01*Endpoint.D) / (10.0*V_FROM_LAMBDA(dLmbdWanted)); /* velocity in m/s instead of cm/ms */
		phi0 = 180.0/M_PI * (ThisChopper.Angle - ThisChopper.Frequency * time);
		dir  = (short) (ThisChopper.Frequency < 0.0 ? -1 : 1);
		bPhase = TRUE;
	}
	/* phase for window 1 or diameter */
	if (bPhase)
	{	phi_wnd1        =  90.0/M_PI*(ThisChopper.Window[0].Right + ThisChopper.Window[0].Left);
		stPicture.dWPar = RedAngle(phi0-phi_wnd1, dir);
	}
	else
	{	stPicture.dWPar = 2*ThisChopper.Radius;
	}
	/* phase for window 2 or RPM */
	if (bPhase && ThisChopper.NumberOfWindows >= 2)
	{	phi_wnd2        =  90.0/M_PI*(ThisChopper.Window[1].Right + ThisChopper.Window[1].Left);
		stPicture.dHPar = RedAngle(phi0-phi_wnd2, dir);
	}
	else
	{	stPicture.dHPar = 180.0/M_PI *(ThisChopper.Window[0].Right - ThisChopper.Window[0].Left);
	}
	/* phase for window 3 or angle of aperture */
	if (bPhase && ThisChopper.NumberOfWindows >= 3)
	{	phi_wnd3        =  90.0/M_PI*(ThisChopper.Window[2].Right + ThisChopper.Window[2].Left);
		stPicture.dRPar = RedAngle(phi0-phi_wnd3, dir);
	}
	else
	{	stPicture.dRPar = ThisChopper.Frequency/(2.0*M_PI/60.0);
	}
	stPicture.nNumber  = ThisChopper.NumberOfWindows;

	/* free allocated memory */
	free(ThisChopper.Window);
}
/* End OwnCleanup */


/* RedAngle: reducing the angle to [0, 360] deg   
                                or [-360,0] deg (dir = -1)
*/
double RedAngle(double angle, short dir)
{
	double red_angle;

	red_angle = angle - floor(angle/360.0)*360.0;
	if (dir==1)
		red_angle -= 360.0;

	return(red_angle);
}

/* ModPhase: reducing the phase from a full circle to a section of a circle,
             i.e. from [-pi,pi] to [-pi/n, pi/n]
   phi_in: intial angle        [rad]
   nSect : number of sections
*/
double ModPhase(double phase, int nSect)
{
	double ph_mod, al, k;

	al     = 2*M_PI/nSect;
	k      = floor(phase/al + 0.5);
	ph_mod = phase - k * al;

	return ph_mod;
}


/* Read chopper file */
/* ----------------- */
void ReadChopperData()
{
	short  k;
	char	 Buffer[CHAR_BUF_LENGTH];
	double A, dY, dZ, Offset=0.0, WindowOpening, WindowHeight;

	fgets(Buffer,100,ChopperFile);
	sscanf(Buffer,"%d",&ThisChopper.NumberOfWindows);

	if((ThisChopper.Window=(ChopperWindow *)malloc(ThisChopper.NumberOfWindows*sizeof(ChopperWindow)))==NULL)
	{
		Error("Out of memory whilst reading chopper data");
	}

	fgets(Buffer,100,ChopperFile);
	sscanf(Buffer,"%lf",&ThisChopper.Radius);
	ThisChopper.Radius = fabs(ThisChopper.Radius);

	fgets(Buffer,100,ChopperFile);
	sscanf(Buffer,"%lf %lf", &ThisChopper.Centre.Z, &ThisChopper.Centre.Y);

	// ThisChopper.Centre.Z = -fabs(ThisChopper.Centre.Z);
	ThisChopper.Angle = atan2(-ThisChopper.Centre.Y, -ThisChopper.Centre.Z);
	fprintf(LogFilePtr, "Radius of chopper           :  %6.2f         cm\n", ThisChopper.Radius);
	fprintf(LogFilePtr, "Center of chopper axle (Z,Y): (%+6.2f,%+6.2f) cm\n", ThisChopper.Centre.Z, ThisChopper.Centre.Y);
	fprintf(LogFilePtr, "Chopper open at t=0 (without offset) for a window at %-5.1f deg\n", ThisChopper.Angle*180/M_PI);
	/* fprintf(LogFilePtr, "\n%d windows\n", ThisChopper.NumberOfWindows); */

	/* data of windows */
	for(k=0;k<(int)ThisChopper.NumberOfWindows;k++)
	{
		if(fgets(Buffer,100,ChopperFile)==NULL)
		{
			fprintf(LogFilePtr,"ERROR: File %s does not contain %d window definitions\n",ChopperFileName,ThisChopper.NumberOfWindows);
			fclose(ChopperFile);
			exit(-1);
		}
		sscanf(Buffer,"%lf %lf %lf %lf %lf",&Offset,&WindowHeight,&WindowOpening,&ThisChopper.Window[k].Left,&ThisChopper.Window[k].Right);

		fprintf(LogFilePtr, "Window %d: Position: %7.2f deg   Aperture: %6.2f deg   Height: %6.2f cm\n",
		                    k+1, Offset, WindowOpening, WindowHeight);
		if (ThisChopper.Window[k].Left > 0.0 || ThisChopper.Window[k].Right > 0.0)
			fprintf(LogFilePtr, "  Deviation: %6.2f deg left, %6.2f deg right\n",
			                    ThisChopper.Window[k].Left, ThisChopper.Window[k].Right);

		Offset        = 2.0*M_PI*Offset/360.0;
		WindowOpening = 2.0*M_PI*WindowOpening/360.0;

		ThisChopper.Window[k].Bottom = ThisChopper.Radius -WindowHeight;

		if((ThisChopper.Window[k].Left!=0.0)||(ThisChopper.Window[k].Right!=0.0))
		{
			ThisChopper.Window[k].Left =  2.0*M_PI*ThisChopper.Window[k].Left/360.0 +(WindowOpening/2.0);
			ThisChopper.Window[k].Right = 2.0*M_PI*ThisChopper.Window[k].Right/360.0+(WindowOpening/2.0);

			A = 2.0*ThisChopper.Window[k].Bottom*sin(WindowOpening/2.0)*cos(ThisChopper.Window[k].Left)
				/
				sin(ThisChopper.Window[k].Right+ThisChopper.Window[k].Left);

			dZ=ThisChopper.Window[k].Bottom*cos(WindowOpening/2.0)-A*cos(ThisChopper.Window[k].Right);
			dY=A*sin(ThisChopper.Window[k].Right)-ThisChopper.Window[k].Bottom*sin(WindowOpening/2.0);

			ThisChopper.Window[k].Distance = sqrt(dY*dY+dZ*dZ);
			ThisChopper.Window[k].Angle = atan2(dY,dZ)+Offset;
		}
		else
		{
			ThisChopper.Window[k].Left  = WindowOpening/2.0;
			ThisChopper.Window[k].Right = WindowOpening/2.0;
			ThisChopper.Window[k].Distance = 0.0;
			ThisChopper.Window[k].Angle = Offset;
		}
		ThisChopper.Window[k].Left = -ThisChopper.Window[k].Left +Offset;
		ThisChopper.Window[k].Right = ThisChopper.Window[k].Right+Offset;
	}

	fclose(ChopperFile);
}
/* chopper file read */


unsigned short BlockedByChopper(Chopper ThisChopper, Neutron* ThisNeutron)
{
	/***********************************************************************************/
	/*This subroutine accepts two structured variables containing information about the*/
	/*chopper in question and a single neutron incident on the plain of the chopper.   */
	/*It calculates the offset of the chopper at the time index of the incident neutron*/
	/*and returns FALSE if the neutron is incident on a window.                        */
	/***********************************************************************************/

	double ChopperOffset=0.0, ChopperOffsetRed=0.0, OriginNeutronDistance, Time;
	double Left, Right, WindowAngle=0.0, NeutronAngle, dY, dZ;
	short  i;
	int    RightTurns=0;
	int    LeftTurns=0;

	/***********************************************************************************/
	/* The main loop here cycles through each of the chopper's windows to find if any  */
	/* are open at the time the neutron strikes.                                       */
	/***********************************************************************************/


	if (bSetColour)
		ThisNeutron->Color = 0;

	for(i=0;i<ThisChopper.NumberOfWindows;i++)
	{
		RightTurns=0; LeftTurns=0; /*modified*/

		/***********************************************************************************/
		/* The first check is to see whether the incident neutron is "above" the bottom of  */
		/* the chopper window,i.e. the distance to the center of the chopper is calculated  */
		/* and then compared to the distance bottom of chopper window <-> center of chopper */
		/***********************************************************************************/
		OriginNeutronDistance = sqrt((ThisNeutron->Position[0]-ThisChopper.Centre.X)
											 *(ThisNeutron->Position[0]-ThisChopper.Centre.X)
											 +(ThisNeutron->Position[1]-ThisChopper.Centre.Y)
											 *(ThisNeutron->Position[1]-ThisChopper.Centre.Y)
											 +(ThisNeutron->Position[2]-ThisChopper.Centre.Z)
											 *(ThisNeutron->Position[2]-ThisChopper.Centre.Z));
		if(OriginNeutronDistance < ThisChopper.Window[i].Bottom) continue;

		/* second check: if neutron does not hit chopper at all*/
		if(OriginNeutronDistance > ThisChopper.Radius)
			goto passed_outside;

		/***********************************************************************************/
		/* The offset of this window at the time the neutron strikes is calculated. It is  */
		/* worth noting again here that the parameter Chopper.Window.Angle is defined as   */
		/* that between the centre of the chopper and the point of convergence of the two  */
		/* sides of the window in question; similarly the parameter Chopper.Window.Distance*/
		/* is the distance between those two points.                                        */
		/***********************************************************************************/

		/*msec. to sec. */
		Time = ThisNeutron->Time/1000.0;

		ChopperOffset = Time * ThisChopper.Frequency  + ChopperInitialOffset;
		WindowAngle = ThisChopper.Window[i].Angle + ChopperOffset;
		Left        = ThisChopper.Window[i].Left  + ChopperOffset;
		Right       = ThisChopper.Window[i].Right + ChopperOffset;

		/***********************************************************************************/
		/* The angles calculated above are now renormalized to lie between +PI and -PI     */
		/***********************************************************************************/
		while(Left>=M_PI)  {Left-=2.0*M_PI; LeftTurns--;}
		while(Left<=-M_PI) {Left+=2.0*M_PI; LeftTurns++;}

		while(Right>=M_PI) {Right-=2.0*M_PI; RightTurns--;}
		while(Right<=-M_PI){Right+=2.0*M_PI; RightTurns++;}

		while(WindowAngle>=M_PI)    WindowAngle-=2.0*M_PI;
		while(WindowAngle<=-M_PI)   WindowAngle+=2.0*M_PI;

		while(ChopperOffset>=M_PI)  ChopperOffset-=2.0*M_PI;
		while(ChopperOffset<=-M_PI) ChopperOffset+=2.0*M_PI;

		/***********************************************************************************/
		/* This next set of statements calculates the angle between the point of           */
		/* convergence of the window sides and the incident neutron. The calculations      */
		/* depending on whether the window sides diverge from the radial or not.           */
		/* You will notice that the X ordinate is assumed to zero...					   */
		/***********************************************************************************/
		if(ThisChopper.Window[i].Distance !=0.0)
		{
			dZ = ThisNeutron->Position[2]
			  -ThisChopper.Window[i].Distance*cos(WindowAngle)
			  -ThisChopper.Centre.Z;
			dY = ThisNeutron->Position[1]
			  -ThisChopper.Window[i].Distance*sin(WindowAngle)
			  -ThisChopper.Centre.Y;
		}
		else
		{
			dZ = ThisNeutron->Position[2] - ThisChopper.Centre.Z;
			dY = ThisNeutron->Position[1] - ThisChopper.Centre.Y;
		}
		NeutronAngle = atan2(dY,dZ);

		/***********************************************************************************/
		/* Check this angle against the angle of the window sides - if it lie between the  */
		/* window sides, the neutron is NOT BlockedByChopper.                              */
		/***********************************************************************************/
		if(RightTurns==LeftTurns)
		{
			if((NeutronAngle>Left)&&(NeutronAngle<Right))
			{
				if (bSetColour)
					ThisNeutron->Color = (short) (i+1);
				goto passed;
			}
		}
		else
		{
			if(((NeutronAngle<Left)&&(NeutronAngle<Right))||   /* why not "> Left or < Right" */
				((NeutronAngle>Left)&&(NeutronAngle>Right)))
			{
				if (bSetColour)
					ThisNeutron->Color = (short) (i+1);
				goto passed;
			}
		}
	}

	/***********************************************************************************/
	/* If the routine reaches this point, it has been found that none of the chopper   */
	/* is open at this time windows; the neutron IS BlockedByChopper.                  */
	/***********************************************************************************/

	return TRUE;

passed_outside:
	/* depending on criterion: treatment of neutrons outside the chopper or not */
	if (bPassOutside==FALSE)
	{	return TRUE;    /* treated as blocked though it passed outside the chopper */
	}
	else
	{	CountMessageID(CHOP_PASSED_OUTSIDE, ThisNeutron->ID);
	}

passed:
	/* set time (close to) zero, if demanded */
	if (bZeroTime)
	{
		// ChopperOffsetRed  = ModPhase(ChopperOffset+ThisChopper.Window[0].Angle, NumEquWnds) ;
		ChopperOffsetRed  = ModPhase(ChopperOffset, NumEquWnds) ;
		ThisNeutron->Time = 1000.0 * ChopperOffsetRed / ThisChopper.Frequency;
	}
	return FALSE;
}


