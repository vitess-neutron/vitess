/********************************************************************************************/
/*  VITESS module guide                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.xx   Sep 1999  D. Wechsler                                                             */
/*                                                                                          */
/*                  Rewritten by Manoshin Sergey Feb 2001 for include GRAVITY               */
/*                  Fixed some major bugs... Manoshin Sergey 28.02.01.                      */
/*                  Add key -g for gravity off or on                                        */
/*                  Add key -a for abutment error on or off                                 */
/*                  Add possibility for simulate guide with different coated matherial in   */
/*                  left and right and top and bottom planes of guide:                      */
/*                  first reflectivity file describe left plane of guide -i                 */
/*                  second reflectivity file describe right plane of guide -I               */
/*                  third reflectivity file describe top and bottom planes of guide -j      */
/* 2.3              add key -r for simulation the rough reflecting surface                  */
/*                                                                                          */
/* 2.4   Dec 2001  K. Lieutenant  adaption to changes in YTSDefs and wei_min as general     */
/*                                parameter, improvement in printing                        */
/* 2.5   Jan 2002  K. Lieutenant  reorganisation                                            */
/* 2.6   Apr 2003  K. Lieutenant  more precise calculation of curved guide; changed output  */
/*                                OwnInit() and other changes in style                      */
/* 2.6a  Jun 2003  K. Lieutenant  small corrections: output of waviness, free memory, 'maxi'*/
/* 2.7   Jul 2003  K. Lieutenant  correction: loss of trajectories by check 'previous coll.'*/
/*                                correction: wrong direction because of high waviness for  */
/*                                            straight guides                               */
/*                                condition: trajectory must end inside exit plane          */
/* 2.7a  Jan 2004  K. Lieutenant  4 different coatings                                      */
/* 2.7b  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                              */
/* 2.8   Jan 2004  K. Lieutenant  correction: wrong direction because of high waviness      */
/* 2.9   Feb 2004  K. Lieutenant  'FullParName'; 'message' included                         */
/* 2.10  Mar 2004  K. Lieutenant  parabolic and elliptic shape                              */
/* 2.11  Oct 2004  K. Lieutenant  curvature to the right by negative radius                 */
/* 2.12  May 2004  K. Lieutenant  elliptic shape by focus point                             */
/********************************************************************************************/

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);

/******************************/
/** Structures and Enums     **/
/******************************/

typedef struct
{
	double  CriticalAngle;
	double  CutoffAngle;
	Plane	Wall[5];
}
NeutronGuide;

typedef enum
{	VT_CONSTANT = 0,
	VT_LINEAR   = 1,
	VT_CURVED   = 2,
	VT_PARABOLIC= 3,
	VT_ELLIPTIC = 4,
}
VtShape;


/******************************/
/** Prototypes               **/
/******************************/

void   OwnInit   (int argc, char *argv[]);
void   OwnCleanup();
double Height    (double length);
double Width     (double length);
double PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide ThisGuide, double wei_min ,
                                  double *reflectivityl, double* reflectivityr, double* reflectivityt, double* reflectivityb,
                                  long nDataMax,         double surfacerough,   long keygrav,          long keyabut);


/******************************/
/** Global variables         **/
/******************************/

long   keyabut  =0,
       nPieces  =1,
       nChannels=1,
       nSpacers =0;

double GuideEntranceHeight=0.0,
       GuideEntranceWidth=0.0,
       GuideExitHeight=0.0,
       GuideExitWidth=0.0,
       GuideMaxHeight=0.0,   /* max. height and width of guide for elliptic shape */
       GuideMaxWidth=0.0,
       FocusY=0.0,D_Foc1Y=0.0, /* pos. of focus points for elliptic shape in horizontal dir. */
       FocusZ=0.0,D_Foc1Z=0.0, /* pos. of focus points for elliptic shape in vertical dir.   */
       PhiAnfY =90.0,        /* phases for elliptic shape */
       PhiAnfZ =90.0,
       LcntrY  = 0.0,        /* centre positions of ellipse */
       LcntrZ  = 0.0,
       AxisY   = 0.0,        /* long axes of ellipse */
       AxisZ   = 0.0,
       Radius  = 0.0,
       piecelength=0.0,      /* length of 1 piece of the guide */
       dTotalLength,         /* total length of the guide  */
       dDeltaX, dDeltaY,     /* length in x- and y-direction of the total guide  */
       beta, beta_ges,       /* angle of declination between 2 pieces  and of the total guide */
       spacer=0.0,
       surfacerough=0.0;     /* parameter which characterizes the waviness of the guide surface */
double *Ypce, *Zpce,         /* list of widths and heights at beginning and end of pieces       */
       *Wchan;               /* list of widths of channel at beginning and end of each piece */

VtShape eGuideShapeY=1,      /* shape of guide in y- and z-direction */
        eGuideShapeZ=1;

double *RDataL=NULL,         /* Table of reflectivity for guide surface on left side        */
       *RDataR=NULL,         /* Table of reflectivity for guide surface on right side       */
       *RDataT=NULL,         /* Table of reflectivity for guide surface on right side       */
       *RDataB=NULL;         /* Table of reflectivity for top and bottom plane of the guide */

char  *ReflFileNameL=NULL;
char  *ReflFileNameR=NULL;
char  *ReflFileNameT=NULL;
char  *ReflFileNameB=NULL;

FILE  *pReflL=NULL; /* file for describing left plane of guide */
FILE  *pReflR=NULL; /* file for describing right plane of guide */
FILE  *pReflT=NULL; /* file for describing top plane of guide */
FILE  *pReflB=NULL; /* file for describing bottom plane of guide */


/******************************/
/** Program                  **/
/******************************/

int main(int argc, char *argv[])
{
	/********************************************************************************************/
	/* This module reads in a file of neutron structures, and defines a neutron guide as a set  */
	/* of five infinite planes with a global critical angle. It outputs the coordinates and time*/
	/* displacement of any neutrons that pass through the guide without being absorbed.			*/
	/*                                                                                          */
	/* Anything not directly commented is an InputNeutrons or an output routine.                        */
	/********************************************************************************************/
	long   i, j, k, count,
	       kChan,
	       nLinesL, nLinesR, nLinesT, nLinesB,
	       nDataMax;
	short  test;

	double dXpce,              /* length of a piece incl. diff. in y- or z- position */
	       dDelY,  dDelZ,      /* difference in y- or z-position of a piece         */
	       dDelYr, dDelYl,     /* difference in y-position of the left and right side of a piece resp. */
	       Length1, Length2,   /* length of a piece incl. diff. in z- or y-position resp. */
	       Length2r,Length2l,  /* length of a piece incl. diff. in y-position.
	                                 for left and right side of a piece resp. */
	       right_beg,left_beg, /* right and left position of the beginning of a channel of a piece */
	       right_end,left_end; /* right and left position of the end of a channel of a piece */
	double dCosBetH = 1.0,
	       dSinBetH = 0.0,     /* cos(beta/2) and sin(beta/2)            */
	       TimeOF1, TimeOF2,
			 RotMatrix[3][3]={{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};

	char   sBuffer[512]="";

	NeutronGuide Guide;
	Neutron      Output;


	/* Initialisation */
	Init(argc, argv, VT_GUIDE);
	print_module_name("guide 2.12c");
	OwnInit(argc, argv);

	/* Writing to log file */
	fprintf(LogFilePtr, "\nTotal length of guide   : %8.3f  m\n", dTotalLength/100.);
	if (nChannels > 1)
		fprintf(LogFilePtr, " with %d channels", nChannels);
	fprintf(LogFilePtr, "Width x Height          : %8.3f  x %7.3f cm²", GuideEntranceWidth, GuideEntranceHeight);
	if (GuideExitWidth != GuideEntranceWidth || GuideExitHeight != GuideEntranceHeight)
		fprintf(LogFilePtr, " -> %7.3f x %7.3f cm²", GuideExitWidth, GuideExitHeight);
	fprintf(LogFilePtr, "\n\nHorizontal: ");
	switch (eGuideShapeY)
	{	case VT_ELLIPTIC:
			fprintf(LogFilePtr, "elliptic shape\n");
			fprintf(LogFilePtr, " maximal width  :%8.3f cm  at %8.2f m from entrance\n", GuideMaxWidth, LcntrY/100.);
			fprintf(LogFilePtr, " long half axis :%8.3f m\n", AxisY/100.);
			fprintf(LogFilePtr, " focus points   :%8.3f m from entrance, %8.2f m after exit\n", D_Foc1Y/100., FocusY/100.);
			break;
		case VT_PARABOLIC:
			fprintf(LogFilePtr, "parabolic shape\n");
			break;
		case VT_CURVED  :
			break;
		case VT_CONSTANT:
		case VT_LINEAR  :
			if      (GuideExitWidth > GuideEntranceWidth) fprintf(LogFilePtr, "linearly diverging\n");
			else if (GuideExitWidth < GuideEntranceWidth) fprintf(LogFilePtr, "linearly converging\n");
			else    fprintf(LogFilePtr, "constant width\n");
			break;
	}
	fprintf(LogFilePtr, "Vertical  : ");
	switch (eGuideShapeZ)
	{	case VT_ELLIPTIC:
			fprintf(LogFilePtr, "elliptic shape\n");
			fprintf(LogFilePtr, " max. height    :%8.3f cm  at %8.2f m from entrance\n", GuideMaxHeight, LcntrZ/100.);
			fprintf(LogFilePtr, " long half axis :%8.3f m\n", AxisZ/100.);
			fprintf(LogFilePtr, " focus points   :%8.3f m from entrance, %8.2f m after exit\n", D_Foc1Z/100., FocusZ/100.);
			break;
		case VT_PARABOLIC:
			fprintf(LogFilePtr, "parabolic shape\n");
			break;
		case VT_CONSTANT:
		case VT_LINEAR  :
			if      (GuideExitHeight > GuideEntranceHeight) fprintf(LogFilePtr, "linearly diverging\n");
			else if (GuideExitHeight < GuideEntranceHeight) fprintf(LogFilePtr, "linearly converging\n");
			else    fprintf(LogFilePtr, "constant height\n");
			break;
	}

	if (Radius != 0.0)  /* curved guide */
	{	beta = 2.0*asin(piecelength/(2.0*Radius));
		dCosBetH = cos(beta/2.0);
		dSinBetH = sin(beta/2.0);
		FillRotMatrixZ(RotMatrix, beta);
		fprintf(LogFilePtr,"\n%d kink(s) with an angle of %8.4f deg  each", nPieces-1, 180/M_PI*beta);
	}
	else
	{	beta = 0.0;
	}

	if (keyabut == 1)
		fprintf(LogFilePtr,"\nInside guide abutment loss is enabled \n");
	else
		fprintf(LogFilePtr,"\nInside guide abutment loss is disabled \n");

	if (surfacerough == 0.0)
		fprintf(LogFilePtr,"The walls have no waviness \n");
	else
		fprintf(LogFilePtr,"The walls have a waviness of %10.3e° \n", atan(surfacerough)*180.0/M_PI);


	/* Read guide data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, ... deg,  */
	/* reference wavelength 1 A */

	/* Allocate memory for reflectivity files and initialize with 0.0 */
	nLinesL = LinesInFile(pReflL);
	nLinesR = LinesInFile(pReflR);
	nLinesT = LinesInFile(pReflT);
	nLinesB = LinesInFile(pReflB);
	nDataMax = maxi(maxi(maxi(10*nLinesL, 10*nLinesR), maxi(10*nLinesT, 10*nLinesB)), 1000);

	RDataL = calloc(nDataMax, sizeof(double));
	RDataR = calloc(nDataMax, sizeof(double));
	RDataT = calloc(nDataMax, sizeof(double));
	RDataB = calloc(nDataMax, sizeof(double));
	for (count=0; count < nDataMax; count++)
	{	RDataL[count] = 0.0;
		RDataR[count] = 0.0;
		RDataT[count] = 0.0;
		RDataB[count] = 0.0;
	}

	/****************************************************************************************/
	/* read reflectivity files for left plane, right plane and top and bottom planes        */
	/****************************************************************************************/
	/* left plane */
	if (pReflL != NULL)
	{	for(count=0; count < nLinesL; count++)
		{	ReadLine  (pReflL, sBuffer, sizeof(sBuffer)-1);
			StrgScanLF(sBuffer, &RDataL[10*count], nDataMax-10*count, 0);
		}
		fclose(pReflL);
	}
	/* right plane */
	if (pReflR != NULL)
	{	for(count=0; count < nLinesR; count++)
		{	ReadLine  (pReflR, sBuffer, sizeof(sBuffer)-1);
			StrgScanLF(sBuffer, &RDataR[10*count], nDataMax-10*count, 0);
		}
		fclose(pReflR);
	}
	/* top plane */
	if (pReflT != NULL)
	{	for(count=0; count < nLinesT; count++)
		{	ReadLine  (pReflT, sBuffer, sizeof(sBuffer)-1);
			StrgScanLF(sBuffer, &RDataT[10*count], nDataMax-10*count, 0);
		}
		if (pReflT != pReflB)  /* file pointer pReflB and pReflT might be identical,                     */
			fclose(pReflT);     /* in this case, the file must not be closed before bottom data are read  */
	}
	/* bottom plane */
	if (pReflB != NULL)
	{	rewind(pReflB);
		for(count=0; count < nLinesB; count++)
		{	ReadLine  (pReflB, sBuffer, sizeof(sBuffer)-1);
			StrgScanLF(sBuffer, &RDataB[10*count], nDataMax-10*count, 0);
		}
		fclose(pReflB);
	}


	/****************************************************************************************/
	/* Set up the parameters of the planes from the input data...                           */
	/****************************************************************************************/

	dXpce = piecelength;
	dDelY = (GuideExitWidth  - GuideEntranceWidth)  / (2.0*nPieces);
	dDelZ = (GuideExitHeight - GuideEntranceHeight) / (2.0*nPieces);
	Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
	Length2 = sqrt(dXpce*dXpce+dDelY*dDelY);

	/* top plane */
	Guide.Wall[0].A =  dDelZ/Length1;
	Guide.Wall[0].B =  0.0;
	Guide.Wall[0].C = -dXpce/Length1;
	Guide.Wall[0].D = -Guide.Wall[0].C * (GuideEntranceHeight/2.0);

	/* bottom plane */
	Guide.Wall[1].A = -dDelZ/Length1;
	Guide.Wall[1].B =  0.0;
	Guide.Wall[1].C = -dXpce/Length1;
	Guide.Wall[1].D =  Guide.Wall[1].C * (GuideEntranceHeight/2.0);

	/* left plane */
	Guide.Wall[2].A = -dDelY/Length2;
	Guide.Wall[2].B =  dXpce/Length2;
	Guide.Wall[2].C =  0.0;
	Guide.Wall[2].D = -Guide.Wall[2].B * (GuideEntranceWidth/2.0);

	/* right plane */
	Guide.Wall[3].A =  dDelY/Length2;
	Guide.Wall[3].B =  dXpce/Length2;
	Guide.Wall[3].C =  0.0;
	Guide.Wall[3].D =  Guide.Wall[3].B * (GuideEntranceWidth/2.0);

	/* exit plane */
	Guide.Wall[4].A =  1.0;
	Guide.Wall[4].B =  0.0;
	Guide.Wall[4].C =  0.0;
	Guide.Wall[4].D = -piecelength;

	/*****************************************************/

	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			test = TRUE;
			kChan = 0;
			TimeOF1 = 0.0;
			TimeOF2 = 0.0;

			/*	InputNeutrons[i].Position.X = 0.0;   !!!!!!!! */
			/****************************************************************************************/
			/* Check to see if the neutron is initially in the entrance to the guide...             */
			/****************************************************************************************/
			if (fabs(InputNeutrons[i].Position[1]) > GuideEntranceWidth/2.0)  continue;
			if (fabs(InputNeutrons[i].Position[2]) > GuideEntranceHeight/2.0) continue;

			if (piecelength ==0.0) goto zerolength;

			/************** start bender option *********************/
			if (nChannels > 1)
			{
				for (k=0; k < nChannels; k++)
				{
					right_beg = -GuideEntranceWidth/2.0 + k*(Wchan[0] + spacer);
					left_beg  = right_beg + Wchan[0];

					if(   (right_beg < InputNeutrons[i].Position[1])
					   && (left_beg  > InputNeutrons[i].Position[1]))
					{
						kChan = k;
						InputNeutrons[i].Color = k+1;
						/* left and right walls of guide exchanged by the channel walls             */
						/* for elliptical parabolic shape, this has to be calculated for each piece */
						if (eGuideShapeY!=VT_PARABOLIC && eGuideShapeY!=VT_ELLIPTIC)
						{	right_end = -GuideExitWidth/2.0 + kChan*(Wchan[nPieces] + spacer);
							left_end  =  right_end + Wchan[nPieces];
							dDelYr    =  right_end - right_beg;
							dDelYl    =  left_end  - left_beg;
							Length2r  =  sqrt(dXpce*dXpce+dDelYr*dDelYr);
							Length2l  =  sqrt(dXpce*dXpce+dDelYl*dDelYl);
							Guide.Wall[2].A = -dDelYl/Length2l;
							Guide.Wall[2].B =  dXpce /Length2l;
							Guide.Wall[2].D =  Guide.Wall[2].B*(-left_beg);
							Guide.Wall[3].A = -dDelYr/Length2r;
							Guide.Wall[3].B =  dXpce /Length2r;
							Guide.Wall[3].D =  Guide.Wall[3].B*(-right_beg);
						}
						break;
					}
				}
				if(k==nChannels) continue;  /* neutron blocked by spacer */
			}
			/************** end bender option **************************/


			/****************************************************************************************/
			/* Pass a pointer to the neutron and the guide structure variable to a subroutine to do */
			/* the donkey work. The return value is the total length of the flight path through the */
			/* guide, or -1.0 if it missed all plates and the exit (should be impossible).          */
			/****************************************************************************************/
			for(j=0; j < nPieces; j++)
			{
				CHECK

				/* In case of several pieces:
				   planes must be adjusted for each piece (depending on the guide shape) */
				if (nPieces > 1)
				{
					switch (eGuideShapeY)
					{	case VT_CURVED:
							/* last piece has an output plane normal to the guide direction, the others are tilted  */
							if (j == nPieces-1)
							{	Guide.Wall[4].A =  1.0;
								Guide.Wall[4].B =  0.0;
								Guide.Wall[4].D = -piecelength;
							}
							else
							{	Guide.Wall[4].A =  dCosBetH;
								Guide.Wall[4].B =  dSinBetH;
								Guide.Wall[4].D = -Guide.Wall[4].A * piecelength;
							}
							break;

						case VT_LINEAR:
							/* left and right walls are moved  */
							Guide.Wall[2].D = -Guide.Wall[2].B * Ypce[j];
							Guide.Wall[3].D =  Guide.Wall[3].B * Ypce[j];
							break;

						case VT_PARABOLIC:
						case VT_ELLIPTIC:
							/* left and right walls are moved  */
							if (nChannels > 1)
							{	right_beg = -Ypce[j]   + kChan*(Wchan[j] + spacer);
								left_beg  =  right_beg + Wchan[j];
								right_end = -Ypce[j+1] + kChan*(Wchan[j+1] + spacer);
								left_end  =  right_end + Wchan[j+1];
								dDelYr    = right_end - right_beg;
								dDelYl    = left_end  - left_beg;
								Length2r  = sqrt(dXpce*dXpce+dDelYr*dDelYr);
								Length2l  = sqrt(dXpce*dXpce+dDelYl*dDelYl);
							}
							else
							{	right_beg = -Ypce[j];
								left_beg  =  Ypce[j];
								dDelYl    =  Ypce[j+1]-Ypce[j];
								dDelYr    = -dDelYl;
								Length2l  = Length2r = sqrt(dXpce*dXpce+dDelYr*dDelYr);
							}
							Guide.Wall[2].A = -dDelYl/Length2l;
							Guide.Wall[2].B =  dXpce /Length2l;
							Guide.Wall[3].A = -dDelYr/Length2r;
							Guide.Wall[3].B =  dXpce /Length2r;

							Guide.Wall[2].D = -Guide.Wall[2].B *   left_beg;
							Guide.Wall[3].D =  Guide.Wall[3].B *(-right_beg);
					}

					switch (eGuideShapeZ)
					{
						case VT_PARABOLIC:
						case VT_ELLIPTIC:
							/* new shift is calculated to move walls */
							dDelZ   = Zpce[j+1]-Zpce[j];
							Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
							Guide.Wall[0].A =  dDelZ/Length1;
							Guide.Wall[0].C = -dXpce/Length1;
							Guide.Wall[1].A = -dDelZ/Length1;
							Guide.Wall[1].C = -dXpce/Length1;
							/* no break at this point !!! */
						case VT_LINEAR:
							/* top and bottom walls are moved */
							Guide.Wall[0].D = -Guide.Wall[0].C * Zpce[j];
							Guide.Wall[1].D =  Guide.Wall[1].C * Zpce[j];
					}
				}

				TimeOF1 = PathThroughGuideGravOrder1(&InputNeutrons[i], Guide, wei_min,
				                                     RDataL, RDataR, RDataT, RDataB, nDataMax,
				                                     surfacerough, keygrav, keyabut);

				if (TimeOF1 == -1.0) /* trajectory is lost */
				{	test=FALSE;
					j=nPieces;
					continue;
				}

				/****************************************************************************************/
				/* Update the coordinates.                                                              */
				/****************************************************************************************/

				InputNeutrons[i].Position[0] -= piecelength;

				/* For curved guide: frame rotated for next piece, but not after last piece */
				if (Radius != 0.0 && j < nPieces-1)
				{
					/* horizontal position and flight direction adjusted */
					RotVector(RotMatrix, InputNeutrons[i].Position);
					RotVector(RotMatrix, InputNeutrons[i].Vector);
					RotVector(RotMatrix, InputNeutrons[i].Spin);
				}
				TimeOF2 += TimeOF1;
			}

			if (test==FALSE) continue;

			if (fabs(InputNeutrons[i].Position[1]) > 0.5*GuideExitWidth ||
			    fabs(InputNeutrons[i].Position[2]) > 0.5*GuideExitHeight)
			{
				CountMessageID(GUID_OUT_OF_EXIT, InputNeutrons[i].ID);
				continue;
			}

	zerolength:
			/****************************************************************************************/
			/* Add the time needed to travel all guide and writeout this trajectory                 */
			/****************************************************************************************/
			Output = InputNeutrons[i];

			Output.Position[0]=0.0;
			Output.Time += TimeOF2;

			WriteNeutron(&Output);
		}
	}

 my_exit:
	OwnCleanup();
	Cleanup(sqrt(sq(dTotalLength)-sq(dDeltaY)),dDeltaY,0.0, beta_ges, 0.0);

	return(0);
}


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit   (int argc, char *argv[])
{
	long  i,j ;
	char  *arg=NULL;
	FILE* pFile=NULL;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+')
		{
			arg=&argv[i][2];
			switch(argv[i][1])
			{
				case 'i':  /* left plane */
					if( (pReflL = fopen(FullParName(arg),"r"))==NULL)
					{	fprintf(LogFilePtr,"ERROR: File %s containing coating of left plane could not be opened\n",arg);
						exit(-1);
					}
					ReflFileNameL=arg;
					break;
				case 'I':  /* right plane */
					if( (pReflR = fopen(FullParName(arg),"r"))==NULL)
					{	fprintf(LogFilePtr,"ERROR: File %s containing coating of right plane could not be opened\n",arg);
						exit(-1);
					}
					ReflFileNameR=arg;
					break;
				case 'j':    /* top plane */
					if( (pReflT = fopen(FullParName(arg),"r"))==NULL)
					{
						fprintf(LogFilePtr,"ERROR: File %s containing coating of top plane could not be opened\n",arg);
						exit(-1);
					}
					ReflFileNameT=arg;
					break;
				case 'J':    /* bottom plane */
					if( (pReflB = fopen(FullParName(arg),"r"))==NULL)
					{
						fprintf(LogFilePtr,"ERROR: File %s containing coating of bottom plane could not be opened\n",arg);
						exit(-1);
					}
					ReflFileNameB=arg;
				break;

				case 'h':
				  GuideEntranceHeight =  atof(arg);
				  break;
				case 'H':
				  GuideExitHeight = atof(arg);
				  break;
				case 'w':
				  GuideEntranceWidth = atof(arg);
				  break;
				case 'W':
				  GuideExitWidth = atof(arg);
				  break;

				case 'f':
				  FocusY = atof(arg);      /* distance of focus point behind guide exit */
				  break;
				case 'F':
				  FocusZ = atof(arg);      /* distance of focus point behind guide exit */
				  break;
				case 'z':
				  PhiAnfZ = atof(arg);    /* Phase of ellipse for height at guide entrance (in deg) */
				  PhiAnfZ *= M_PI/180.0;  /* 90 deg means: max. width of ellipse       */
				  break;
				case 'y':
				  PhiAnfY = atof(arg);    /* Phase of ellipse for width at guide entrance (in deg) */
				  PhiAnfY *= M_PI/180.0;
				  break;

				case 'N':
				  nPieces = atol(arg); /* number of pieces */
				  break;
				case 'R':
				  Radius =  atof(arg); /* in m */
				  Radius *= 100.0;
				  break;
				case 'p':
				  piecelength = atof(arg); /* length of 1 piece of guide in cm */
				  break;
				case 'Y':                   /* Shape of guide: 0: constant                           */
				  eGuideShapeY = atol(arg); /*                 1: (linearly) converging or diverging */
				  break;                    /*                 2: curved (circular)                  */
				case 'Z':                   /*                 3: parabolic                          */
				  eGuideShapeZ = atol(arg); /*                 4: elliptic                           */
				  break;

				case 'b':
				  nChannels = atol(arg); /* bender: No. of channels  */
				  nSpacers  = nChannels - 1;
				  break;
				case 's':
				  spacer =  atof(arg); /* width of bender channel border in cm */
				  break;

				case 'a':
				  keyabut =  atol(arg); /* key for abutment loss 1 -yes (default), 0 - no  */
				  break;
				case 'r':
				  surfacerough  =  atof(arg); /* Maximal angle of deviation of normal in degre */
				  surfacerough  =  surfacerough*M_PI/180.0; /*Convert from degree to radian */
				  surfacerough  =  tan(surfacerough);
				  break;

				default:
				  fprintf(LogFilePtr,"ERROR: Unknown command option: %s\n",argv[i]);
				  exit(-1);
				  break;
			}
		}
	}

	/* Input checks */
	/* combination: orientation - shape */
	if (eGuideShapeZ==VT_CURVED)
		Error("Vertical curving of the guide not supported");

	/* Radius  */
	if (eGuideShapeY==VT_CURVED)
	{	if (Radius==0.0)
			Error("Radius of curvature is missing");
	}
	else
	{	if (Radius!=0.0)
			Error("Curvature of guide only supported in option 'curved', please delete radius or switch to 'curved'");
	}

	/* Exit and entrance size */
	if (eGuideShapeZ==VT_CONSTANT)
	{	if (GuideExitHeight != GuideEntranceHeight)
			Error("In guides of constant height, exit and entrance height have to be equal");
		else
			GuideExitHeight = GuideEntranceHeight;
	}
	else
	{	if (GuideExitHeight== 0.0)
			Error("You must enter the height of the guide exit");
	}

	if (eGuideShapeY==VT_CONSTANT || eGuideShapeY==VT_CURVED)
	{	if (GuideExitWidth != GuideEntranceWidth)
			Error("In curved and constant guides, exit and entrance width have to be equal");
		else
			GuideExitWidth  = GuideEntranceWidth;
	}
	else
	{	if (GuideExitWidth == 0.0)
			Error("You must enter the width of the guide exit");
	}

	if ((eGuideShapeY==VT_ELLIPTIC && PhiAnfY < 0.5*M_PI && GuideExitWidth  > GuideEntranceWidth) ||
	    (eGuideShapeZ==VT_ELLIPTIC && PhiAnfZ < 0.5*M_PI && GuideExitHeight > GuideEntranceHeight)   )
	{
		Error("The ellipse must widen at the guide entrance (angle > 90 deg) to achieve a exit width larger than the entrance width");
	}

	/* number of pieces */
	if (   eGuideShapeZ==VT_PARABOLIC || eGuideShapeZ==VT_ELLIPTIC
	    || eGuideShapeY==VT_PARABOLIC || eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_CURVED)
	{	if (nPieces <= 1)
			Error("More than 1 piece is necessary for this shape");
	}

	/* Calculation of height, width and channel-width of beginning and end of pieces */
	dTotalLength = nPieces*piecelength;

	Ypce  = calloc(nPieces+1, sizeof(double));
	Zpce  = calloc(nPieces+1, sizeof(double));
	Wchan = calloc(nPieces+1, sizeof(double));

	if (eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_PARABOLIC ||
	    eGuideShapeZ==VT_ELLIPTIC || eGuideShapeZ==VT_PARABOLIC   )
		pFile = fopen(FullParName("guide_shape.dat"), "w+");
	for(j=0; j <= nPieces; j++)
	{	Ypce [j] = Width (j*piecelength)/2.0;
		Zpce [j] = Height(j*piecelength)/2.0;
		Wchan[j] = (2*Ypce[j]- nSpacers*spacer)/(double)nChannels;
		if (Wchan[j] <= 0.0)
			Error("Geometry impossible. Channel width gets zero (or less)");
		if (pFile != NULL)
		{	if (j==0)
			{	fprintf(pFile, "# length [m]  width [cm]  height [cm]\n");
				fprintf(pFile, "#-------------------------------------\n");
			}
			fprintf(pFile, "%10.2f  %10.3f  %10.3f\n", j*piecelength/100.0, 2*Ypce[j], 2*Zpce[j]);
		}
	}
	if (pFile != NULL)
		fclose(pFile);

	/* left plane */
	if (pReflL == NULL)
	{	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the left wall \n");
	}
	/* right plane */
	if (pReflR == NULL)
	{	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the right wall \n");
	}
	/* top plane */
	if (pReflT == NULL)
	{	fprintf(LogFilePtr,"\nWARNING: Case of zero reflectivity for the top wall \n");
	}
	/* bottom plane */
	if (pReflB == NULL)
	{	pReflB = pReflT;
		fprintf(LogFilePtr,"\nNOTE: coating of top wall also used for bottom wall \n");
	}
}


/* own cleanup of the guide module */
/* --------------------------------*/
void OwnCleanup()
{
	/* print error that might have occured many times */
	PrintMessage(GUID_OUT_OF_EXIT, "", ON);
	PrintMessage(GUID_NO_PLANE, "", ON);

	fprintf(LogFilePtr," \n");

	/* set description for instrument plot */
	stPicture.dWPar = GuideEntranceWidth;
	stPicture.dHPar = GuideExitWidth;
	stPicture.dRPar = Radius/100.0;
	stPicture.eType = (short)(10*eGuideShapeY + eGuideShapeZ);
	if (nChannels > 1)
		stPicture.nNumber = - nChannels;
	else
		stPicture.nNumber = nPieces;
	beta_ges = (nPieces-1)*beta;
	if (Radius != 0.0)
	{	dDeltaX = Radius*sin(beta_ges)       + 0.5*piecelength*(cos(beta_ges)+1.0);
		dDeltaY = Radius*(1.0-cos(beta_ges)) + 0.5*piecelength* sin(beta_ges);
	}
	else
	{	dDeltaX = dTotalLength;
		dDeltaY = 0.0;
	}

	/* free allocated memory */
	if (RDataL!=NULL) free(RDataL);
	if (RDataR!=NULL) free(RDataR);
	if (RDataT!=NULL) free(RDataT);
	if (RDataB!=NULL) free(RDataB);

	if (Ypce !=NULL) free(Ypce );
	if (Zpce !=NULL) free(Zpce );
	if (Wchan!=NULL) free(Wchan);
}/* End OwnCleanup */


double Height(double dLength)
{
	double dHeight=0.0,
	       L_end,           /* end of parabel or 2nd part of ellipse (center to exit) */
	       A,               /* factor of quadratic term in parabola  */
	       eps,             /* correction value =(b*b)/(2a*a)        */
	       Phi,
	       Phi_anf,Phi_end; /* phases in ellipse                     */

	switch (eGuideShapeZ)
	{
		case VT_CONSTANT:
		case VT_CURVED:
			dHeight = GuideEntranceHeight;
			break;
		case VT_LINEAR:
			dHeight = GuideEntranceHeight + (GuideExitHeight-GuideEntranceHeight)/dTotalLength * dLength;
			break;
		case VT_PARABOLIC:
			A      = dTotalLength/(sq(GuideEntranceHeight) - sq(GuideExitHeight));
			L_end  = A * sq(GuideEntranceHeight);
			dHeight = sqrt((L_end-dLength)/A);
			break;
		case VT_ELLIPTIC:
			/* first approximation */
			AxisZ   = 0.5*fabs((sq(dTotalLength+FocusZ)*sq(GuideExitHeight) - sq(FocusZ*GuideEntranceHeight))
			                   /(FocusZ*sq(GuideEntranceHeight) - (dTotalLength+FocusZ)*sq(GuideExitHeight)));
			L_end   = AxisZ - FocusZ;
			LcntrZ  = dTotalLength - L_end;
			Phi_anf = acos(-LcntrZ/AxisZ);
			Phi_end = acos(L_end/AxisZ);
			GuideMaxHeight = GuideEntranceHeight/sin(Phi_anf);
			/* second approximation */
			eps     = 0.5 * sq(GuideMaxHeight/AxisZ);
			AxisZ   = 0.5*fabs((1.0+eps)*(sq(dTotalLength+FocusZ)*sq(GuideExitHeight) - sq(FocusZ*GuideEntranceHeight))
			                   /(FocusZ*sq(GuideEntranceHeight) - (dTotalLength+FocusZ)*sq(GuideExitHeight) + eps*AxisZ*(sq(GuideEntranceHeight)-sq(GuideExitHeight)) ));
			L_end   = AxisZ/(1.0+eps) - FocusZ;
			LcntrZ  = dTotalLength - L_end;
			Phi_anf = acos(-LcntrZ/AxisZ);
			Phi_end = acos(L_end/AxisZ);
			GuideMaxHeight = GuideEntranceHeight/sin(Phi_anf);
			D_Foc1Z = LcntrZ - AxisZ/(1.0+eps);

			Phi     = acos((dLength - LcntrZ)/AxisZ);
			dHeight = GuideMaxHeight*sin(Phi);
			break;
		default:
			Error("Shape unknown");
	}

	return dHeight;
}

double Width(double dLength)
{
	double dWidth=0.0,
	       L_end,           /* end of parabel or 2nd part of ellipse (center to exit) */
	       A,               /* factor of quadratic term in parabola  */
	       eps,             /* correction value =(b*b)/(2a*a)        */
	       Phi,
	       Phi_anf,Phi_end; /* phases in ellipse                     */

	switch (eGuideShapeY)
	{
		case VT_CONSTANT:
		case VT_CURVED:
			dWidth = GuideEntranceWidth;
			break;
		case VT_LINEAR:
			dWidth = GuideEntranceWidth + (GuideExitWidth-GuideEntranceWidth)/dTotalLength * dLength;
			break;
		case VT_PARABOLIC:
			A      = dTotalLength/(sq(GuideEntranceWidth) - sq(GuideExitWidth));
			L_end  = A * sq(GuideEntranceWidth);
			dWidth = sqrt((L_end-dLength)/A);
			break;
		case VT_ELLIPTIC:
			AxisY   = 0.5*fabs((sq(dTotalLength+FocusY)*sq(GuideExitWidth) - sq(FocusY*GuideEntranceWidth))
			                   /(FocusY*sq(GuideEntranceWidth) - (dTotalLength+FocusY)*sq(GuideExitWidth)));
			L_end   = AxisY - FocusY;
			LcntrY  = dTotalLength - L_end;
			Phi_anf = acos(-LcntrY/AxisY);
			Phi_end = acos(L_end/AxisY);
			GuideMaxWidth = GuideEntranceWidth/sin(Phi_anf);
			/* second approximation */
			eps     = 0.5 * sq(GuideMaxWidth/AxisY);
			AxisY   = 0.5*fabs((1.0+eps)*(sq(dTotalLength+FocusY)*sq(GuideExitWidth) - sq(FocusY*GuideEntranceWidth))
			                   /(FocusY*sq(GuideEntranceWidth) - (dTotalLength+FocusY)*sq(GuideExitWidth) + eps*AxisY*(sq(GuideEntranceWidth)-sq(GuideExitWidth)) ));
			L_end   = AxisY/(1.0+eps) - FocusY;
			LcntrY  = dTotalLength - L_end;
			Phi_anf = acos(-LcntrY/AxisY);
			Phi_end = acos(L_end/AxisY);
			GuideMaxWidth = GuideEntranceWidth/sin(Phi_anf);
			D_Foc1Y = LcntrY - AxisY/(1.0+eps);

			Phi     = acos((dLength - LcntrY)/AxisY);
			dWidth = GuideMaxWidth*sin(Phi);
			break;
		default:
			Error("Shape unknown");
	}

	return dWidth;
}


double
PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide ThisGuide, double  wei_min ,
									double *reflectivityl, double* reflectivityr, double* reflectivityt, double* reflectivityb,
                           long nDataMax,         double surfacerough,   long    keygrav,       long    keyabut)
{
	/***********************************************************************************/
	/* This routine calculates the trajectory a neutron follows through a simple       */
	/* neutron guide. It accepts two structured variables; a pointer to a neutron      */
	/* structure and a simple Guide structure. This latter consists simply of four     */
	/* infinite planes describing the two walls floor and ceiling of the guide and a   */
	/* fifth infinite plane at the exit of the guide. The structure has an assosciated */
	/* critical angle; any neutron that intercepts a wall at an angle greater than this*/
	/* is absorbed. 								   */
	/* Neutron flight by parabolic trajectories with GRAVITY		    	   */
	/* Significant Rewrited by Manoshin Sergey Feb 2001			           */
	/* Note! The function is return Time Of Flight					   */
	/***********************************************************************************/

	int     k, ThisCollision=5, datanumber;
	double  degangular;
	double  TimeOF, TimeOFmin;
	double  TimeOFTotal=0.0;
	double  VelocityReal, DOTP;
	double  VX, VY, VZ;
	VectorType vWallN,  /* normal to the plane wall             */
	           vWaviN;  /* normal to the wall with its waviness */
	Neutron TempNeutron, NearestNeutron; /* Local copies of actual trajectory for loops */


	/***********************************************************************************/
	/* The main loop here is continuous: the neutron will continue to bounce around,   */
	/* until it is absorbed or intercepts with the exit plane.                         */
	/***********************************************************************************/

	while(TRUE)
	{
		TimeOFmin = 99999999999999999.9;

		/***********************************************************************************/
		/* Loop through all five planes....                                                */
		/***********************************************************************************/
		for(k=0;k<5;k++)
		{
			/***********************************************************************************/
			/* Find the point where this neutron trajectory intercepts the current plane       */
			/***********************************************************************************/

			/*Save current neutron, because the function 'NeutronPlaneIntersectionGrav' has
				modified trajectory data  */
			CopyNeutron(ThisNeutron, &TempNeutron);

			if (keygrav == 1)
				TimeOF = NeutronPlaneIntersectionGrav(&TempNeutron, ThisGuide.Wall[k]);
			else
				TimeOF = NeutronPlaneIntersection1   (&TempNeutron, ThisGuide.Wall[k]);

			/***********************************************************************************/
			/* If this intercept point is behind the neutrons current position, pass control to*/
			/* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */
			/***********************************************************************************/
			if ((TimeOF<=0.0) || (TempNeutron.Position[0] < ThisNeutron->Position[0]))
				continue;

			/***********************************************************************************/
			/* If this calculated distance is not the shortest so far, return to the top of the*/
			/* loop.  TimeOF -> min                                                            */
			/***********************************************************************************/
			if (TimeOF > TimeOFmin)
				continue;

			/***********************************************************************************/
			/* The intercept of the neutron with this wall is the nearest so far, so accept it */
			/* temporarily.                                                                    */
			/***********************************************************************************/

			CopyNeutron(&TempNeutron, &NearestNeutron);
			TimeOFmin = TimeOF;
			ThisCollision = k;
		}

		/***********************************************************************************/
		/* Having looped through all five planes, the current values of NearestNeutron,    */
		/* TimeOFmin and ThisCollision, reflect the coordinates, distance and index        */
		/* of the neutrons interaction with a guide wall. If this guide wall is index 4    */
		/*(i.e. the exit window) reset the neutron coordinates to this point, add the path */
		/* length to this point to the running total and return that total.                */
		/***********************************************************************************/

		if(ThisCollision == 4)
		{
			if(NearestNeutron.Vector[0] < 0.0)
				return(-1.0);

			/*  This feature rejects neutrons, which make reflections close to the guide exit */
			if (keyabut == 1)
			{
				VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength));
				if (TimeOFmin*VelocityReal <= 0.5)
				{
					return(-1.0);
				}
			}

			if (NearestNeutron.Probability < wei_min)
			{
				return(-1.0);
			}

			ThisNeutron->Position[0] = NearestNeutron.Position[0];
			ThisNeutron->Position[1] = NearestNeutron.Position[1];
			ThisNeutron->Position[2] = NearestNeutron.Position[2];
			ThisNeutron->Vector  [2] = NearestNeutron.Vector  [2];
			ThisNeutron->Vector  [0] = sqrt(1.0 - sq(ThisNeutron->Vector[1])
			                                    - sq(ThisNeutron->Vector[2]));
			ThisNeutron->Probability = NearestNeutron.Probability;

			TimeOFTotal =  TimeOFTotal + TimeOFmin;

			return TimeOFTotal;
		}


		/***********************************************************************************/
		/* If the angle of intersection of the flight path and the guide wall exceeds the  */
		/* critical angle of the guide, the neutron is absorbed.                           */
		/* Otherwise the probability is reduced by the reflectivity of the plane.          */
		/***********************************************************************************/

		vWallN[0] = ThisGuide.Wall[ThisCollision].A;
		vWallN[1] = ThisGuide.Wall[ThisCollision].B;
		vWallN[2] = ThisGuide.Wall[ThisCollision].C;

		/* Normalize normal vector to the reflection plane */
		if (LengthVector(vWallN) == 0.0)
			return(-1.0);
		else
			NormVector(vWallN);

		/* influence of rough surface */
		if (surfacerough != 0.0)
		{
			/* rough surface must not alter the side from which the neutron comes */
			do
			  {	// len = vector3rand(&VX, &VY, &VZ);
			    gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);
			    vWaviN[0] = vWallN[0] + surfacerough*VX;
			    vWaviN[1] = vWallN[1] + surfacerough*VY;
			    vWaviN[2] = vWallN[2] + surfacerough*VZ;

			    /* Renormalize normal vector */
			    if (LengthVector(vWaviN) == 0.0)
			      return(-1.0);
			    else
			      NormVector(vWaviN);
			  }
			while (  ScalarProduct(NearestNeutron.Vector, vWallN)
		          * ScalarProduct(NearestNeutron.Vector, vWaviN) < 0.0);
		}
		else
		{	CopyVector(vWallN, vWaviN);
		}

		/* angle between normal vector and neutron flight direction (in degree) */
		degangular = fabs(90 - AngleVectors(NearestNeutron.Vector, vWaviN));

		/* Determine number of reflectivity value in reflectivty file */
		datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength));
		if (datanumber >= nDataMax) return(-1.0);

		/*Choose the reflectivity file and multiply probability by  reflectivity value */
		switch(ThisCollision)
		{
			case 0: NearestNeutron.Probability *= reflectivityt[datanumber]; break; /* top plane */
			case 1: NearestNeutron.Probability *= reflectivityb[datanumber]; break; /* bottom plane */
			case 2: NearestNeutron.Probability *= reflectivityl[datanumber]; break; /* left plane */
			case 3: NearestNeutron.Probability *= reflectivityr[datanumber]; break; /* right plane */
			default:
			{	CountMessageID(GUID_NO_PLANE, NearestNeutron.ID);
				return(-1.0);
			}
		}

		if (NearestNeutron.Probability < wei_min)
			return(-1.0);


		/***********************************************************************************/
		/* Calculate the direction of the reflected neutron.                               */
		/* Set the neutron coordinates to coordinates of the collision                     */
		/* Correct if waviness has prevented a change in flight direction                  */
		/* calculate new count rate                                                        */
		/* return to the begining of the loop and find the next collision                  */
		/***********************************************************************************/

		DOTP = ScalarProduct(vWaviN, NearestNeutron.Vector);

		/* Reflection must alter direction relative to wall orientation */
		ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*vWaviN[0];
		ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*vWaviN[1];
		ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*vWaviN[2];

		while (  ScalarProduct(ThisNeutron->Vector,   vWallN)
		       * ScalarProduct(NearestNeutron.Vector, vWallN) > 0.0)
		{
			ThisNeutron->Vector[0] -= 2.0*DOTP*vWaviN[0];
			ThisNeutron->Vector[1] -= 2.0*DOTP*vWaviN[1];
			ThisNeutron->Vector[2] -= 2.0*DOTP*vWaviN[2];
		}
		NormVector(ThisNeutron->Vector);

		/* CopyVector(NearestNeutron.Position, ThisNeutron->Position); */
		ThisNeutron->Position[0] = NearestNeutron.Position[0];
		ThisNeutron->Position[1] = NearestNeutron.Position[1];
		ThisNeutron->Position[2] = NearestNeutron.Position[2];

		ThisNeutron->Probability = NearestNeutron.Probability;

		TimeOFTotal =  TimeOFTotal + TimeOFmin;
	}
}
