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
/* 2.12  May 2005  K. Lieutenant  elliptic shape by focus point                             */
/* 2.13  May 2008  K. Lieutenant  shape defined in file                                     */
/* 2.14  Oct 2008  K. Lieutenant  attenuation included                                      */
/* 2.15  Aug 2009  A. Houben      Simple calculation of the guide area                      */
/* 2.16  Aug 2009  A. Houben      Write out reflection parameters of each trajectory        */
/*                                (data is complementary to traceing and writeout)          */
/* 2.17  Sep 2009  A. Houben      Extended writeout of reflection parameters                */
/* 2.18  Sep 2009  A. Houben      Changes to shape defined by file & some minor things      */
/* 2.19  Oct 2009  A. Houben      Shape by file for nonäquidistant planes & minor things    */
/*                                (introduced rounding of XYZ positions but left commented) */
/* 2.20  Dez 2009  A. Houben      -FROM FILE mode allows to give mirror filenames           */
/*                                -GuidePieces are managed by array of struct GuidePiece    */
/*                                -Mirror files are requested/loaded by GetReflFile and     */
/*                                 stored in array of structs. Filename is key for reuse.   */
/********************************************************************************************/

#include "intersection.h"
#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "message.h"
#include "string.h"

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

/* GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT must be 0 to 3 */
typedef enum
{	GW_TOP      = 0,
	GW_BOTTOM   = 1,
	GW_LEFT     = 2,
	GW_RIGHT    = 3,
	GW_EXIT     = 4,
	GW_INIT     = 5,
}
eGuideWall;

typedef enum
{	VT_CONSTANT = 0,
	VT_LINEAR   = 1,
	VT_CURVED   = 2,
	VT_PARABOLIC= 3,
	VT_ELLIPTIC = 4,
	VT_FROM_FILE= 5,
}
VtShape;

typedef struct
{
	int     RefCount;   /* Counts the number of reflections. 
						   If the last reflection did not occure for any reason, 
						   RefCount is increased by one, and multiplied by -1 */
	int     RefCountY;  /* Number of reflection on horizontal guide planes */
	int     RefCountZ;  /* Number of reflection on vertical guide planes */
	char    *Output;    /* One line of text for each reflection */
}
ReflCond;

typedef struct
{
	FILE   *pfile;
	char   *filename;
	double *Rdata;
	long   maxdata;
	double area;
}
ReflFile;

typedef struct
{
	double Xpce, Ypce, Zpce,  /* list of x-pos., width and height at beginning and end of pieces */
       Wchan;                 /* list of widths of channel at beginning and end of each piece */
	//ReflFile *RDataL,         /* Table of reflectivity for guide surface on left side        */
    //   *RDataR,               /* Table of reflectivity for guide surface on right side       */
    //   *RDataT,               /* Table of reflectivity for guide surface on top side         */
    //   *RDataB;               /* Table of reflectivity for guide surface on bottom side      */
	ReflFile *RData[4];        /* use GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT */
}
GuidePiece;

/******************************/
/** Prototypes               **/
/******************************/

void   OwnInit   (int argc, char *argv[]);
void   OwnCleanup();
ReflFile *GetReflFile(char *Filename, FILE *file);
void   LoadReflFile(ReflFile *pReflFile);
double Height    (double length);
double Width     (double length);
double PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide ThisGuide, double  wei_min,
                           GuidePiece *Pce, double surfacerough, long keygrav, long keyabut, ReflCond *RefOut);
void   WriteReflParam(ReflCond *RefOut, int Mode, Neutron *pNeutron, NeutronGuide *ThisGuide, GuidePiece *Pce, eGuideWall ThisCollision, double degangular, double reflectivity);
void   PrintMaximalM(double *RData, long i);


/******************************/
/** Global variables         **/
/******************************/

long   keyabut  =0,
       nPieces  =1,
       nChannels=1,
       nSpacers =0;
int    keyReflParam = -1;     /* Trajectories to be written out:
								 1 = only those leaving the guide;
								 2 = all successfull reflections; no matter if the trajectory reaches the guide end
								 3 = only those with at least one successful scattering event (tracjectory may end with an unsuccessfull event)
								 4 = all
								 a negative number adds a line feed between each trajectory */
int    keyReflMinCnt = 0;     /* Minimum number of reflections within the guide for reflection list output */
int    keyReflMaxCnt = 0;     /* Maximum number of reflections within the guide for reflection list output */
int    keyReflMinCntY = 0;    /* Minimum number of reflections on the horizontal guide for reflection list output */
int    keyReflMaxCntY = 0;    /* Maximum number of reflections on the horizontal guide for reflection list output */
int    keyReflMinCntZ = 0;    /* Minimum number of reflections on the vertical guide for reflection list output */
int    keyReflMaxCntZ = 0;    /* Maximum number of reflections on the vertical guide for reflection list output */
int    keyReflVerbose = 0;    /* Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated. */

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
       AparY   = 0.0,        /* factor of quadratic term in parabola  */
       AparZ   = 0.0,
       Radius  = 0.0,
       piecelength=0.0,      /* length of 1 piece of the guide */
       dTotalLength,         /* total length of the guide  */
       dDeltaX, dDeltaY,     /* length in x- and y-direction of the total guide  */
       beta, beta_ges,       /* angle of declination between 2 pieces  and of the total guide */
       spacer=0.0,
       surfacerough=0.0,     /* parameter which characterizes the waviness of the guide surface */
       MuScat=0.0,           /* total macroscopic scattering coeff. in 1/cm */
       MuAbs =0.0;           /* macroscopic absorption coeff. in 1/cm */
double AreaY=0., AreaZ=0.;   /* Approximate area of guide planes in cm**2 */
//double *Xpce, *Ypce, *Zpce,  /* list of x-pos., width and height at beginning and end of pieces */
//       *Wchan;               /* list of widths of channel at beginning and end of each piece */
GuidePiece *pPieces;         /* Holds piece Informations. Replaces Xpce, Ypce, Zpce */

VtShape eGuideShapeY=1,      /* shape of guide in y- and z-direction */
        eGuideShapeZ=1;

char  *ShapeFileName="guide_shape.dat";
char  *ReflParamFileName=NULL;
char  *ReflFileNameL=NULL;
char  *ReflFileNameR=NULL;
char  *ReflFileNameT=NULL;
char  *ReflFileNameB=NULL;

FILE  *pReflParam=NULL; /* file for writing each reflection */
FILE  *pReflL=NULL; /* file for describing left plane of guide */
FILE  *pReflR=NULL; /* file for describing right plane of guide */
FILE  *pReflT=NULL; /* file for describing top plane of guide */
FILE  *pReflB=NULL; /* file for describing bottom plane of guide */

/*double *RDataL=NULL,         // Table of reflectivity for guide surface on left side
       *RDataR=NULL,         // Table of reflectivity for guide surface on right side
       *RDataT=NULL,         // Table of reflectivity for guide surface on top side
       *RDataB=NULL;         // Table of reflectivity for guide surface on bottom side */

/* Extended FROM FILE */
ReflFile *pReflFiles = {NULL};
long     cReflFiles  = 0;
/**********************/

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
	long   i, j, k, kChan;
	short  test;

	double pathlen,            /* total neutron pathlength in the guide              */
	       dXpce,              /* length of a piece incl. diff. in y- or z- position */
	       dDelY,  dDelZ,      /* difference in y- or z-position of a piece          */
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

	NeutronGuide Guide;
	Neutron      Output;
	ReflCond     RefOut = {0};


	/* Initialisation */
	Init(argc, argv, VT_GUIDE);
	print_module_name("guide 2.20");
	OwnInit(argc, argv);

	/* Writing to log file */
	fprintf(LogFilePtr, "\nTotal length of guide   : %8.3f  m\n", dTotalLength/100.);
	if (nChannels > 1)
		fprintf(LogFilePtr, " with %ld channels", nChannels);
	fprintf(LogFilePtr, "Width x Height          : %8.3f  x %7.3f cm²", GuideEntranceWidth, GuideEntranceHeight);
	if (GuideExitWidth != GuideEntranceWidth || GuideExitHeight != GuideEntranceHeight)
		fprintf(LogFilePtr, " -> %7.3f x %7.3f cm²", GuideExitWidth, GuideExitHeight);
	fprintf(LogFilePtr, "\n\nHorizontal: ");
	switch (eGuideShapeY)
	{	case VT_ELLIPTIC:
			fprintf(LogFilePtr, "elliptic shape\n");
			fprintf(LogFilePtr, " maximal width     :%8.3f cm  at %8.2f m from entrance\n", GuideMaxWidth, LcntrY/100.);
			fprintf(LogFilePtr, " long half axis    :%8.3f m\n", AxisY/100.);
			fprintf(LogFilePtr, " focal points      :%8.3f m from entrance, %8.3f m after exit\n", D_Foc1Y/100., FocusY/100.);
			break;
		case VT_PARABOLIC:
			fprintf(LogFilePtr, "parabolic shape    : focal point:%8.3f m after exit\n", 
			                    (sq(GuideEntranceWidth)*AparY-dTotalLength-1.0/AparY/16.0)/100.);
			break;
		case VT_CURVED  :
			fprintf(LogFilePtr, "curved guide\n");
			break;
		case VT_FROM_FILE:
			fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
			fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
			break;
		case VT_CONSTANT:
		case VT_LINEAR  :
			if      (GuideExitWidth > GuideEntranceWidth) fprintf(LogFilePtr, "linearly diverging\n");
			else if (GuideExitWidth < GuideEntranceWidth) fprintf(LogFilePtr, "linearly converging\n");
			else    fprintf(LogFilePtr, "constant width\n");
			break;
	}
	fprintf(LogFilePtr, " area (top+bottom) :%8.3f m²\n", AreaY*2./1e4);
	fprintf(LogFilePtr, "Vertical  : ");
	switch (eGuideShapeZ)
	{	case VT_ELLIPTIC:
			fprintf(LogFilePtr, "elliptic shape\n");
			fprintf(LogFilePtr, " max. height       :%8.3f cm  at %8.2f m from entrance\n", GuideMaxHeight, LcntrZ/100.);
			fprintf(LogFilePtr, " long half axis    :%8.3f m\n", AxisZ/100.);
			fprintf(LogFilePtr, " focal points      :%8.3f m from entrance, %8.3f m after exit\n", D_Foc1Z/100., FocusZ/100.);
			break;
		case VT_PARABOLIC:
			fprintf(LogFilePtr, "parabolic shape    : focal point:%8.3f m after exit\n", 
			                    (sq(GuideEntranceHeight)*AparZ-dTotalLength-1.0/AparZ/16.0)/100.);
			break;
		case VT_CURVED  :
			fprintf(LogFilePtr, "WARNING: vertically curved guide not supported\n"); 
			break;
		case VT_FROM_FILE:
			fprintf(LogFilePtr, "guide shape from file %s\n", ShapeFileName);
			fprintf(LogFilePtr, " number of pieces  :%8ld \n", nPieces);
			break;
		case VT_CONSTANT:
		case VT_LINEAR  :
			if      (GuideExitHeight > GuideEntranceHeight) fprintf(LogFilePtr, "linearly diverging\n");
			else if (GuideExitHeight < GuideEntranceHeight) fprintf(LogFilePtr, "linearly converging\n");
			else    fprintf(LogFilePtr, "constant height\n");
			break;
	default: ;
	}
	fprintf(LogFilePtr, " area (left+right) :%8.3f m²\n", AreaZ*2./1e4);

	if (Radius != 0.0)  /* curved guide */
	{	beta = 2.0*asin(piecelength/(2.0*Radius));
		dCosBetH = cos(beta/2.0);
		dSinBetH = sin(beta/2.0);
		FillRotMatrixZ(RotMatrix, beta);
		fprintf(LogFilePtr,"\n%ld kink(s) with an angle of %8.4f deg  each", nPieces-1, 180.0/M_PI*beta);
	}
	else
	{	beta = 0.0;
	}

	for(i=0; i < cReflFiles; i++)
	{
		if (pReflFiles[i].filename != NULL) {
			fprintf(LogFilePtr,"\nReflectivity file  : %s\n", pReflFiles[i].filename);
			if (pReflFiles[i].pfile != NULL) {
				PrintMaximalM(pReflFiles[i].Rdata, pReflFiles[i].maxdata);
			} else {
				fprintf(LogFilePtr,"WARNING: Case of zero reflectivity for this file! Most probably the file was not found!\n");
			}
			fprintf(LogFilePtr,  " surface area      :%8.3f m²\n", pReflFiles[i].area/1.e4);
		} else {
			break;
		}
	}

	if (keyabut == 1)
		fprintf(LogFilePtr,"\nInside guide abutment loss is enabled \n");
	else
		fprintf(LogFilePtr,"\nInside guide abutment loss is disabled \n");

	if (surfacerough == 0.0)
		fprintf(LogFilePtr,"The walls have no waviness \n");
	else
		fprintf(LogFilePtr,"The walls have a waviness of %10.3e° \n", atan(surfacerough)*180.0/M_PI);	

	

	/****************************************************************************************/
	/* Set up the parameters of the planes from the input data...                           */
	/****************************************************************************************/

	dXpce = piecelength;
	dDelY = (GuideExitWidth  - GuideEntranceWidth)  / (2.0*nPieces);
	dDelZ = (GuideExitHeight - GuideEntranceHeight) / (2.0*nPieces);
	Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
	Length2 = sqrt(dXpce*dXpce+dDelY*dDelY);

	/* top plane */
	Guide.Wall[GW_TOP].A =  dDelZ/Length1;
	Guide.Wall[GW_TOP].B =  0.0;
	Guide.Wall[GW_TOP].C = -dXpce/Length1;
	Guide.Wall[GW_TOP].D = -Guide.Wall[GW_TOP].C * (GuideEntranceHeight/2.0);

	/* bottom plane */
	Guide.Wall[GW_BOTTOM].A = -dDelZ/Length1;
	Guide.Wall[GW_BOTTOM].B =  0.0;
	Guide.Wall[GW_BOTTOM].C = -dXpce/Length1;
	Guide.Wall[GW_BOTTOM].D =  Guide.Wall[GW_BOTTOM].C * (GuideEntranceHeight/2.0);

	/* left plane */
	Guide.Wall[GW_LEFT].A = -dDelY/Length2;
	Guide.Wall[GW_LEFT].B =  dXpce/Length2;
	Guide.Wall[GW_LEFT].C =  0.0;
	Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B * (GuideEntranceWidth/2.0);

	/* right plane */
	Guide.Wall[GW_RIGHT].A =  dDelY/Length2;
	Guide.Wall[GW_RIGHT].B =  dXpce/Length2;
	Guide.Wall[GW_RIGHT].C =  0.0;
	Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B * (GuideEntranceWidth/2.0);

	/* exit plane */
	Guide.Wall[GW_EXIT].A =  1.0;
	Guide.Wall[GW_EXIT].B =  0.0;
	Guide.Wall[GW_EXIT].C =  0.0;
	Guide.Wall[GW_EXIT].D = -dXpce;

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

			if (dTotalLength == 0.0) goto zerolength;

			/************** start bender option *********************/
			if (nChannels > 1)
			{
				for (k=0; k < nChannels; k++)
				{
					right_beg = -GuideEntranceWidth/2.0 + k*(pPieces[0].Wchan + spacer);
					left_beg  = right_beg + pPieces[0].Wchan;

					if(   (right_beg < InputNeutrons[i].Position[1])
					   && (left_beg  > InputNeutrons[i].Position[1]))
					{
						kChan = k;
						InputNeutrons[i].Color = (short)(k+1);
						/* left and right walls of guide exchanged by the channel walls             */
						/* for elliptical parabolic shape, this has to be calculated for each piece */
						if (eGuideShapeY!=VT_PARABOLIC && eGuideShapeY!=VT_ELLIPTIC && eGuideShapeY!=VT_FROM_FILE)
						{	right_end = -GuideExitWidth/2.0 + kChan*(pPieces[nPieces].Wchan + spacer);
							left_end  =  right_end + pPieces[nPieces].Wchan;
							dDelYr    =  right_end - right_beg;
							dDelYl    =  left_end  - left_beg;
							Length2r  =  sqrt(dXpce*dXpce+dDelYr*dDelYr);
							Length2l  =  sqrt(dXpce*dXpce+dDelYl*dDelYl);
							Guide.Wall[GW_LEFT].A = -dDelYl/Length2l;
							Guide.Wall[GW_LEFT].B =  dXpce /Length2l;
							Guide.Wall[GW_LEFT].D =  Guide.Wall[GW_LEFT].B*(-left_beg);
							Guide.Wall[GW_RIGHT].A = -dDelYr/Length2r;
							Guide.Wall[GW_RIGHT].B =  dXpce /Length2r;
							Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B*(-right_beg);
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
			RefOut.RefCount = 0;
			RefOut.RefCountY = 0;
			RefOut.RefCountZ = 0;
			if (RefOut.Output != NULL) free(RefOut.Output);
			RefOut.Output = NULL;
			for(j=0; j < nPieces; j++)
			{
				CHECK

				/* In case of several pieces:
				   planes must be adjusted for each piece (depending on the guide shape) */
				if (nPieces > 1)
				{
					dXpce = pPieces[j+1].Xpce - pPieces[j].Xpce;

					switch (eGuideShapeY)
					{	case VT_CURVED:
							/* last piece has an output plane normal to the guide direction, the others are tilted  */
							if (j == nPieces-1)
							{	Guide.Wall[GW_EXIT].A =  1.0;
								Guide.Wall[GW_EXIT].B =  0.0;
								Guide.Wall[GW_EXIT].D = -dXpce;
							}
							else
							{	Guide.Wall[GW_EXIT].A =  dCosBetH;
								Guide.Wall[GW_EXIT].B =  dSinBetH;
								Guide.Wall[GW_EXIT].D = -Guide.Wall[GW_EXIT].A * dXpce;
							}
							break;

						case VT_LINEAR:
							/* left and right walls are moved  */
							Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B * pPieces[j].Ypce;
							Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B * pPieces[j].Ypce;
							break;

						case VT_PARABOLIC:
						case VT_ELLIPTIC:
						case VT_FROM_FILE:
							/* left and right walls are moved  */
							if (nChannels > 1)
							{	right_beg = -pPieces[j].Ypce   + kChan*(pPieces[j].Wchan + spacer);
								left_beg  =  right_beg + pPieces[j].Wchan;
								right_end = -pPieces[j+1].Ypce + kChan*(pPieces[j+1].Wchan + spacer);
								left_end  =  right_end + pPieces[j+1].Wchan;
								dDelYr    = right_end - right_beg;
								dDelYl    = left_end  - left_beg;
								Length2r  = sqrt(dXpce*dXpce+dDelYr*dDelYr);
								Length2l  = sqrt(dXpce*dXpce+dDelYl*dDelYl);
							}
							else
							{	right_beg = -pPieces[j].Ypce;
								left_beg  =  pPieces[j].Ypce;
								dDelYl    =  pPieces[j+1].Ypce-pPieces[j].Ypce;
								dDelYr    = -dDelYl;
								Length2l  = Length2r = sqrt(dXpce*dXpce+dDelYr*dDelYr);
							}
							Guide.Wall[GW_LEFT].A = -dDelYl/Length2l;
							Guide.Wall[GW_LEFT].B =  dXpce /Length2l;
							Guide.Wall[GW_RIGHT].A = -dDelYr/Length2r;
							Guide.Wall[GW_RIGHT].B =  dXpce /Length2r;

							Guide.Wall[GW_LEFT].D = -Guide.Wall[GW_LEFT].B *   left_beg;
							Guide.Wall[GW_RIGHT].D =  Guide.Wall[GW_RIGHT].B *(-right_beg);

							Guide.Wall[GW_EXIT].D = -dXpce;
							break;
						default:
							;
					}

					switch (eGuideShapeZ)
					{
						case VT_PARABOLIC:
						case VT_ELLIPTIC:
						case VT_FROM_FILE:
							/* new shift is calculated to move walls */
							dDelZ   = pPieces[j+1].Zpce-pPieces[j].Zpce;
							Length1 = sqrt(dXpce*dXpce+dDelZ*dDelZ);
							Guide.Wall[GW_TOP].A =  dDelZ/Length1;
							Guide.Wall[GW_TOP].C = -dXpce/Length1;
							Guide.Wall[GW_BOTTOM].A = -dDelZ/Length1;
							Guide.Wall[GW_BOTTOM].C = -dXpce/Length1;
							/* no break at this point !!! */
						case VT_LINEAR:
							/* top and bottom walls are moved */
							Guide.Wall[GW_TOP].D = -Guide.Wall[GW_TOP].C * pPieces[j].Zpce;
							Guide.Wall[GW_BOTTOM].D =  Guide.Wall[GW_BOTTOM].C * pPieces[j].Zpce;
							break;
						default:
							;
					}
				}
				
				TimeOF1 = PathThroughGuideGravOrder1(&(InputNeutrons[i]), Guide, wei_min, &pPieces[j], surfacerough, keygrav, keyabut, &RefOut);
				
				if (TimeOF1 == -1.0) /* trajectory is lost */
				{	test=FALSE;
					j=nPieces;
					continue;
				}

				/****************************************************************************************/
				/* Update the coordinates.                                                              */
				/****************************************************************************************/

				InputNeutrons[i].Position[0] -= dXpce;

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
			
			if (pReflParam != NULL)
			{
				if ((abs(RefOut.RefCount) >= keyReflMinCnt && (abs(RefOut.RefCount) <= keyReflMaxCnt || keyReflMaxCnt == 0)) &&
					(RefOut.RefCountY >= keyReflMinCntY && (RefOut.RefCountY <= keyReflMaxCntY || keyReflMaxCntY == 0)) &&
					(RefOut.RefCountZ >= keyReflMinCntZ && (RefOut.RefCountZ <= keyReflMaxCntZ || keyReflMaxCntZ == 0)))
				{
					switch (abs(keyReflParam))
					{
					case 1:
						if (RefOut.RefCount > 0) {
							fprintf(pReflParam, "%s", RefOut.Output);
							if (keyReflParam < 0) fprintf(pReflParam, "\n");
						}
						break;
					case 2:
						if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
							fprintf(pReflParam, "%s", RefOut.Output);
							if (keyReflParam < 0) fprintf(pReflParam, "\n");
						}
						break;
					case 3:
						if (RefOut.RefCount != -1 && RefOut.RefCount != 0) {
							fprintf(pReflParam, "%s", RefOut.Output);
							if (keyReflParam < 0) fprintf(pReflParam, "\n");
						}
						break;
					case 4:
						fprintf(pReflParam, "%s", RefOut.Output);
						if (keyReflParam < 0) fprintf(pReflParam, "\n");
						break;
					}
				}
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

			pathlen = V_FROM_LAMBDA(Output.Wavelength)*TimeOF2;

			Output.Position[0]=0.0;
			Output.Time += TimeOF2;
			Output.Probability*=exp(-(MuScat+MuAbs*Output.Wavelength/1.798)*pathlen);

			WriteNeutron(&Output);
		}
	}

 my_exit:
	if (RefOut.Output != NULL) free(RefOut.Output);
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
	char  *arg=NULL, sLine[512];
	FILE* pFile=NULL;
	char sRefFileL[512] = "", sRefFileR[512] = "", sRefFileT[512] = "", sRefFileB[512] = "";
	ReflFile *pRefFileLast;

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
				case 'o':  /* Reflection parameter writeout */
					if( (pReflParam = fopen(FullParName(arg),"w"))!=NULL)
					{	/*fprintf(LogFilePtr,"ERROR: File %s for creating reflection parameter output could not be created\n",arg);
						exit(-1);*/
						fprintf(pReflParam, "#____ID____ Scattered plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z\n"
						                    "#    1           2      3       4      5          6          7        8     9    10     11      12         13           14         15         16         17        18        19       20   21   23 \n");
						ReflParamFileName=arg;
					}
					break;
				case 'O':
					keyReflParam = atoi(arg); /* Trajectories to be written out:
								 1 = only those leaving the guide;
								 2 = all successfull reflections; no matter if the trajectory reaches the guide end
								 3 = only those with at least one successful scattering event (tracjectory may end with an unsuccessfull event)
								 4 = all
								 a negative number adds a line feed between each trajectory */
					break;
				case 'v':    /* Print position of trajectory for every guide peace until the trajectory leaves the guide or is terminated. */
					keyReflVerbose = atoi(arg); 
					break;
				case 'e':    /* Minimum number of reflections within the guide for reflection list output */
					keyReflMinCnt = atoi(arg); 
					break;
				case 'E':    /* Maximum number of reflections within the guide for reflection list output */
					keyReflMaxCnt = atoi(arg); 
					break;
				case 'c':    /* Minimum number of reflections on the horizontal guide for reflection list output */
					keyReflMinCntY = atoi(arg); 
					break;
				case 'C':    /* Maximum number of reflections on the horizontal guide for reflection list output */
					keyReflMaxCntY = atoi(arg); 
					break;
				case 'd':    /* Minimum number of reflections on the vertical guide for reflection list output */
					keyReflMinCntZ = atoi(arg); 
					break;
				case 'D':    /* Maximum number of reflections on the vertical guide for reflection list output */
					keyReflMaxCntZ = atoi(arg); 
					break;

				case 'S':    /* shape file */
					ShapeFileName=arg;
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

				case 'M':
				  MuScat =  atof(arg); /* macroscopic scattering coeff. in 1/cm */
				  break;
				case 'm':
				  MuAbs  =  atof(arg); /* macroscopic absorption coeff. in 1/cm */
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
	/* ------------ */
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
	/* ---------------- */
	if (   eGuideShapeZ==VT_PARABOLIC || eGuideShapeZ==VT_ELLIPTIC
	    || eGuideShapeY==VT_PARABOLIC || eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_CURVED)
	{	if (nPieces <= 1)
			Error("More than 1 piece is necessary for this shape");
	}

	if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
	{	
		pFile = fopen(FullParName(ShapeFileName), "r");
		if (pFile != NULL)
		{	nPieces = LinesInFile(pFile) - 1;
			cReflFiles = (nPieces+1) * 4;
		}
		else
		{	fprintf(LogFilePtr,"ERROR: Input file %s could not be read !\n", ShapeFileName);
			exit(-1);
		}
	} else {
		cReflFiles = 4;
	}

	pReflFiles = calloc(cReflFiles, sizeof(ReflFile));
	if (!pReflFiles) { fprintf(LogFilePtr,"ERROR: Not enough memory for reflecitvity data!\n");
		exit(-1);
	}

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
		ReflFileNameB = ReflFileNameT;
		fprintf(LogFilePtr,"\nNOTE: coating of top wall also used for bottom wall \n");
	}

	/* Calculation of height, width and channel-width of beginning and end of pieces */
	/* ----------------------------------------------------------------------------- */
	/*Xpce  = calloc(nPieces+1, sizeof(double));
	Ypce  = calloc(nPieces+1, sizeof(double));
	Zpce  = calloc(nPieces+1, sizeof(double));
	Wchan = calloc(nPieces+1, sizeof(double));*/
	pPieces = calloc(nPieces+1, sizeof(GuidePiece));
	pRefFileLast = GetReflFile(ReflFileNameL, pReflL);

	if (eGuideShapeY==VT_FROM_FILE || eGuideShapeZ==VT_FROM_FILE)
	{	
		double XpceZero = 0.;
		for(j=0; j <= nPieces; j++)
		{	
			ReadLine(pFile, sLine, sizeof(sLine)-1);
			
			sRefFileL[0] = '\0'; sRefFileR[0] = '\0'; sRefFileT[0] = '\0'; sRefFileB[0] = '\0';
			sscanf(sLine, "%lf %lf %lf %s %s %s %s", &pPieces[j].Xpce, &pPieces[j].Ypce, &pPieces[j].Zpce, (char *)&sRefFileL, (char *)&sRefFileR, (char *)&sRefFileT, (char *)&sRefFileB);
			pPieces[j].Xpce *= 100.0;
			if (j==0) XpceZero = pPieces[0].Xpce;
			pPieces[j].Xpce -= XpceZero;
			pPieces[j].Ypce *=   0.5;
			pPieces[j].Zpce *=   0.5;
			/*pPieces[j].Xpce = RoundP(pPieces[j].Xpce, 7);
			pPieces[j].Ypce = RoundP(pPieces[j].Ypce, 7);
			pPieces[j].Zpce = RoundP(pPieces[j].Zpce, 7);*/
			pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
			if (pPieces[j].Wchan <= 0.0)
				Error("Geometry impossible. Channel width gets zero (or less)");
			if (j>0) {
				AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
			}
			

			/* Calculate Area for this reflectivity file */
			if (j > 0) {
				if (pPieces[j-1].RData[GW_LEFT]!=NULL)
					pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_RIGHT]!=NULL) 
					pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_TOP]!=NULL) 
					pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_BOTTOM]!=NULL)
					pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
			}
			
			/* Either load standard reflectivity file or use userdefined one */
			switch (sRefFileL[0]) {
				case ':':  pPieces[j].RData[GW_LEFT]   = pRefFileLast;break;
				case '\0': pPieces[j].RData[GW_LEFT]   = GetReflFile(ReflFileNameL, pReflL);break;
				default:   pPieces[j].RData[GW_LEFT]   = GetReflFile(FullParName((char *)&sRefFileL), NULL);
					pRefFileLast = pPieces[j].RData[GW_LEFT];
					break;
			}
			

			switch (sRefFileR[0]) {
				case ':':  pPieces[j].RData[GW_RIGHT]  = pRefFileLast;break;
				case '\0': pPieces[j].RData[GW_RIGHT]  = GetReflFile(ReflFileNameR, pReflR);break;
				default:   pPieces[j].RData[GW_RIGHT]  = GetReflFile(FullParName((char *)&sRefFileR), NULL);
					pRefFileLast = pPieces[j].RData[GW_RIGHT];
					break;
			}
			

			switch (sRefFileT[0]) {
				case ':':  pPieces[j].RData[GW_TOP]    = pRefFileLast;break;
				case '\0': pPieces[j].RData[GW_TOP]    = GetReflFile(ReflFileNameT, pReflT);break;
				default:   pPieces[j].RData[GW_TOP]    = GetReflFile(FullParName((char *)&sRefFileT), NULL);
					pRefFileLast = pPieces[j].RData[GW_TOP];
					break;
			}
			

			switch (sRefFileB[0]) {
				case ':':  pPieces[j].RData[GW_BOTTOM] = pRefFileLast;break;
				case '\0': pPieces[j].RData[GW_BOTTOM] = GetReflFile(ReflFileNameB, pReflB);break;
				default:   pPieces[j].RData[GW_BOTTOM] = GetReflFile(FullParName((char *)&sRefFileB), NULL);
					pRefFileLast = pPieces[j].RData[GW_BOTTOM];
					break;
			}
		}
		dTotalLength = RoundP(pPieces[nPieces].Xpce - pPieces[0].Xpce, 7);
		GuideEntranceWidth = pPieces[0].Ypce*2.;
		GuideEntranceHeight = pPieces[0].Zpce*2.;
		GuideExitWidth = pPieces[nPieces].Ypce*2.;
		GuideExitHeight = pPieces[nPieces].Zpce*2.;
		piecelength  = dTotalLength / (double)nPieces; /* Use piecelength with care in the case of nonäquidistant planes */
	}
	else
	{
		/* if (eGuideShapeY==VT_ELLIPTIC || eGuideShapeY==VT_PARABOLIC ||
			 eGuideShapeZ==VT_ELLIPTIC || eGuideShapeZ==VT_PARABOLIC   ) */
		pFile = fopen(FullParName(ShapeFileName), "w+");

		dTotalLength = nPieces*piecelength;

		for(j=0; j <= nPieces; j++)
		{	
			pPieces[j].Xpce  = j*piecelength;
			pPieces[j].Ypce  = Width (pPieces[j].Xpce)/2.0;
			pPieces[j].Zpce  = Height(pPieces[j].Xpce)/2.0;
			pPieces[j].Wchan = (2.0*pPieces[j].Ypce - nSpacers*spacer)/(double)nChannels;
			if (pPieces[j].Wchan <= 0.0)
				Error("Geometry impossible. Channel width gets zero (or less)");
			if (pFile != NULL)
			{	if (j==0)
				{	fprintf(pFile, "# length [m]  width [cm]  height [cm]   reflectivity filenames (left, right, top, bottom) \n");
					fprintf(pFile, "#-----------------------------------------------------------------------------------------\n");
				} else {
					AreaY += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
					AreaZ += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				}
				fprintf(pFile, "%10.3f  %10.4f  %10.4f\n", pPieces[j].Xpce/100.0, 2.0*pPieces[j].Ypce, 2.0*pPieces[j].Zpce);
			}

			/* Calculate Area for this reflectivity file */
			if (j > 0) {
				if (pPieces[j-1].RData[GW_LEFT]!=NULL)
					pPieces[j-1].RData[GW_LEFT]->area   += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_RIGHT]!=NULL) 
					pPieces[j-1].RData[GW_RIGHT]->area  += (pPieces[j-1].Zpce+pPieces[j].Zpce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_TOP]!=NULL) 
					pPieces[j-1].RData[GW_TOP]->area    += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
				if (pPieces[j-1].RData[GW_BOTTOM]!=NULL)
					pPieces[j-1].RData[GW_BOTTOM]->area += (pPieces[j-1].Ypce+pPieces[j].Ypce)*(pPieces[j].Xpce-pPieces[j-1].Xpce);
			}

			/* Load reflectivity file or reuse already loaded file */
			pPieces[j].RData[GW_LEFT]   = GetReflFile(ReflFileNameL, pReflL);
			pPieces[j].RData[GW_RIGHT]  = GetReflFile(ReflFileNameR, pReflR);
			pPieces[j].RData[GW_TOP]    = GetReflFile(ReflFileNameT, pReflT);
			pPieces[j].RData[GW_BOTTOM] = GetReflFile(ReflFileNameB, pReflB);
		}
	}
	if (pFile != NULL)
		fclose(pFile);
}

/* Read guide data; data are encoded as reflectivities corresponding to 0.000,0.001, 0.002, ... deg,  */
/* reference wavelength 1 A */
ReflFile *GetReflFile(char *Filename, FILE *file)
{
	//GetReflFile = NULL;
	long  cFiles = 0;
	if (Filename == NULL) return NULL;

	for (cFiles = 0; cFiles < cReflFiles; cFiles++) {
		if (pReflFiles[cFiles].filename != NULL) {
#ifdef _MSC_VER
			if (_stricmp(pReflFiles[cFiles].filename, Filename)==0) {
				return &pReflFiles[cFiles];
#else
			if (strcasecmp(pReflFiles[cFiles].filename, Filename)==0) {
				return &pReflFiles[cFiles];
#endif
			}
		} else {
			break;
		}
	}
	if (pReflFiles[cFiles].filename == NULL) {
		pReflFiles[cFiles].filename = Filename;
		if (file != NULL) pReflFiles[cFiles].pfile = file;
		LoadReflFile(&pReflFiles[cFiles]);
		return &pReflFiles[cFiles];
	}

	return NULL;
}

/* Load Reflectivity data from file. Give pReflFile as input */
void   LoadReflFile(ReflFile *pReflFile)
{
	long   count = 0, i = 0, nLines = 0;
	char   sBuffer[512]="";

	if (pReflFile!=NULL) {
		if (pReflFile->filename != NULL) {
			if (pReflFile->pfile == NULL) pReflFile->pfile = fopen(FullParName(pReflFile->filename), "r");
			if (pReflFile->pfile != NULL) {
				nLines = LinesInFile(pReflFile->pfile);
				pReflFile->maxdata = nLines * 10;
				pReflFile->Rdata = calloc(pReflFile->maxdata, sizeof(double));
				for(count=0; count < nLines; count++) {
					ReadLine(pReflFile->pfile, sBuffer, sizeof(sBuffer)-1);
					i += StrgScanLF(sBuffer, &pReflFile->Rdata[10*count], pReflFile->maxdata-10*count, 0);
				}
				fclose(pReflFile->pfile);
			}
		}
	}
}

/* own cleanup of the guide module */
/* --------------------------------*/
void OwnCleanup()
{
	long cFiles = 0;

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

	if (pPieces!=NULL) free(pPieces);

	if (pReflFiles!=NULL) {
		for (cFiles = 0; cFiles < cReflFiles; cFiles++)
		{
			if (pReflFiles[cFiles].Rdata!=NULL) free(pReflFiles[cFiles].Rdata);
		}
		free(pReflFiles);
	}
	if (pReflParam!=NULL) fclose(pReflParam);
}/* End OwnCleanup */


double Height(double dLength)
{
	double dHeight=0.0,
	       L_end,           /* end of parabel or 2nd part of ellipse (center to exit) */
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
			AparZ   = dTotalLength/(sq(GuideEntranceHeight) - sq(GuideExitHeight));
			L_end   = AparZ * sq(GuideEntranceHeight);
			dHeight = sqrt((L_end-dLength)/AparZ);
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
			AparY  = dTotalLength/(sq(GuideEntranceWidth) - sq(GuideExitWidth));
			L_end  = AparY * sq(GuideEntranceWidth);
			dWidth = sqrt((L_end-dLength)/AparY);
			break;
		case VT_ELLIPTIC:
			/* first approximation */
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


double PathThroughGuideGravOrder1(Neutron *ThisNeutron, NeutronGuide ThisGuide, double  wei_min,
                                  GuidePiece *Pce, double surfacerough, long keygrav, long keyabut, ReflCond *RefOut)
{
	/***********************************************************************************/
	/* This routine calculates the trajectory a neutron follows through a simple       */
	/* neutron guide. It accepts two structured variables; a pointer to a neutron      */
	/* structure and a simple Guide structure. This latter consists simply of four     */
	/* infinite planes describing the two walls floor and ceiling of the guide and a   */
	/* fifth infinite plane at the exit of the guide. The structure has an assosciated */
	/* critical angle; any neutron that intercepts a wall at an angle greater than this*/
	/* is absorbed.                                                                    */
	/* Neutron flight by parabolic trajectories with GRAVITY                           */
	/* Significant Rewrited by Manoshin Sergey Feb 2001                                */
	/* Note! The function is return Time Of Flight                                     */
	/***********************************************************************************/

	int     datanumber;
	eGuideWall k = GW_INIT, ThisCollision = GW_INIT;
	double  degangular, ThisReflectivity=0.;
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
		for(k=GW_TOP;k<GW_INIT;k++) /* GW_TOP = 0, GW_INIT = 5 */
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

		if(ThisCollision == GW_EXIT)
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
			if (keyReflVerbose != 0)
				WriteReflParam(RefOut, 5, ThisNeutron, &ThisGuide, Pce, ThisCollision, 0., 0.);

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

		/* Choose the reflectivity file/value and multiply probability by reflectivity value */
		if (ThisCollision == GW_TOP || ThisCollision == GW_BOTTOM || ThisCollision == GW_LEFT || ThisCollision == GW_RIGHT) {
			if (Pce->RData[ThisCollision]==NULL || datanumber >= Pce->RData[ThisCollision]->maxdata) {
				WriteReflParam(RefOut, 10, &NearestNeutron, &ThisGuide, Pce, ThisCollision, degangular, 0.);
				return(-1.0);
			} else {
				ThisReflectivity = Pce->RData[ThisCollision]->Rdata[datanumber]; }
		} else {
			CountMessageID(GUID_NO_PLANE, NearestNeutron.ID);
			return(-1.0);
		}

		NearestNeutron.Probability *= ThisReflectivity;

		if (NearestNeutron.Probability < wei_min){
			NearestNeutron.Probability = 0.;
			WriteReflParam(RefOut, 10, &NearestNeutron, &ThisGuide, Pce, ThisCollision, degangular, ThisReflectivity);
			return(-1.0);
		}


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
		WriteReflParam(RefOut, 0, ThisNeutron, &ThisGuide, Pce, ThisCollision, degangular, ThisReflectivity);
	}
}

void   WriteReflParam(ReflCond *RefOut, int Mode, Neutron *pNeutron, NeutronGuide *ThisGuide, GuidePiece *Pce, eGuideWall ThisCollision, double degangular, double reflectivity)
{
	if (pReflParam!=NULL)
	{
  //fprintf(pReflParam, "#____ID____ Scattered plane refangle  m_Ni  reflectivity   DivY     DivZ   Trc color   TOF    lambda   count rate     pos_x      pos_y      pos_z      dir_x     dir_y     dir_z     sp_x sp_y sp_z\n");
		char       *fstr="%c%c%09lu     %c     %3d   %8.5f %6.2f %12.5f %8.4f %8.4f  %c %5d  %7.3f %8.5f %11.3e  %10.4f %10.4f %10.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n";
		double     DivY, DivZ, mVal, Qz;
		char       buffer[256] = "";
		
		/*double l = 0.;
		int ii = 0;
		VectorType p;

		l = (-ThisGuide->Wall[ThisCollision].D-ThisGuide->Wall[ThisCollision].A*pNeutron->Position[0]-
			ThisGuide->Wall[ThisCollision].B*pNeutron->Position[1]-ThisGuide->Wall[ThisCollision].C*pNeutron->Position[2])/
			(ThisGuide->Wall[ThisCollision].A*pNeutron->Vector[0]+ThisGuide->Wall[ThisCollision].B*pNeutron->Vector[1]+
			ThisGuide->Wall[ThisCollision].C*pNeutron->Vector[2]);
		for (ii = 0; ii<3; ii++){
			p[ii] = pNeutron->Position[ii]+l*pNeutron->Vector[ii];
		}*/

		DivY = (double)atan2(pNeutron->Vector[1], pNeutron->Vector[0]);
	    DivY *= 180.0/M_PI;
	    if ((pNeutron->Vector[1]==0.0) && (pNeutron->Vector[0]==0.0))
	      DivY = 0.0;

	    DivZ = (double)atan2(pNeutron->Vector[2], pNeutron->Vector[0]);
	    DivZ *= 180.0/M_PI;
	    if ((pNeutron->Vector[2]==0.0) && (pNeutron->Vector[0]==0.0))
	      DivZ = 0.0;

		Qz = 4.*M_PI/pNeutron->Wavelength*sin(degangular*M_PI/180.);
		mVal = Qz/0.02174;
		
		if (Mode == 10)
		{
			sprintf(buffer, fstr,
				pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo,
				'F', ThisCollision, degangular, mVal, reflectivity, DivY, DivZ,
				pNeutron->Debug,       pNeutron->Color,
				pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
				pNeutron->Position[0]+Pce->Xpce, pNeutron->Position[1], pNeutron->Position[2],
				//p[0], p[1], p[2],
				pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
				pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]
				);
		}
		if (Mode == 5)
		{
			sprintf(buffer, fstr,
				pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo,
				'-', ThisCollision, degangular, mVal, reflectivity, DivY, DivZ,
				pNeutron->Debug,       pNeutron->Color,
				pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
				pNeutron->Position[0]+Pce->Xpce, pNeutron->Position[1], pNeutron->Position[2],
				//p[0], p[1], p[2],
				pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
				pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]
				);
		}
		if (Mode == 0)
		{
			sprintf(buffer, fstr,
				pNeutron->ID.IDGrp[0], pNeutron->ID.IDGrp[1], pNeutron->ID.IDNo,
				'T', ThisCollision, degangular, mVal, reflectivity, DivY, DivZ,
				pNeutron->Debug,       pNeutron->Color,
				pNeutron->Time,        pNeutron->Wavelength,  pNeutron->Probability,
				pNeutron->Position[0]+Pce->Xpce, pNeutron->Position[1], pNeutron->Position[2],
				//p[0], p[1], p[2],
				pNeutron->Vector[0],   pNeutron->Vector[1],   pNeutron->Vector[2],
				pNeutron->Spin[0],     pNeutron->Spin[1],     pNeutron->Spin[2]
				);
		}

		if (RefOut != NULL)
		{
			if ((Mode == 0) || (Mode == 5 && RefOut->RefCount >= 0) || (Mode != 0 && abs(keyReflParam) > 2))
			{
				char   *tmp = NULL;
				size_t curlen = (RefOut->Output)?strlen((RefOut->Output)):0;

				if((tmp = realloc(RefOut->Output,curlen+strlen(buffer)+1)) != NULL)
				{
					if (curlen == 0) *tmp = '\0';
					RefOut->Output = tmp;
					strcat(RefOut->Output, buffer);
				}
			}

			/*if (RefOut->RefCount < 0)
				return;*/
			if (Mode != 5)
			{
				RefOut->RefCount++;
				if (ThisCollision <= GW_BOTTOM) /* GW_TOP || GW_BOTTOM */
					RefOut->RefCountZ++;
				else 
					if (ThisCollision <= GW_RIGHT) RefOut->RefCountY++; /* GW_LEFT || GW_RIGHT */
				if (Mode != 0) RefOut->RefCount *= -1;
			}
		}
	}
}

void   PrintMaximalM(double *RData, long i)
{
	long count;
	for(count=i-1; count >= 0; count--)
		if (RData[count] != 0.)
			break;
	if (count >= 0)
		fprintf(LogFilePtr," maximal defined m : %8.2f\n", sin(count/180000.*M_PI)*4*M_PI/0.02174);
	else
		fprintf(LogFilePtr," maximal defined m : absorber\n");
}

