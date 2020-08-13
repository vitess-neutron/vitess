/*******************************************************************************************/
/* Tool EllipticGuide:                                                                     */
/*  Calculates shape of an elliptical guide from entrance size, exit size and distance     */
/*  to focus point                                                                         */
/*                                                                                         */
/* 1.0  Sep 2003  K. Lieutenant  initial version                                           */
/*******************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "init.h"

//#define THETA_NI 0.099138
#define PI       3.1415926535898 

typedef enum
{	VT_CONSTANT = 0,
	VT_LINEAR   = 1,
	VT_CURVED   = 2,
	VT_PARABOLIC= 3,
	VT_ELLIPTIC = 4,
}
VtShape;


double Height   (double length);
double Width    (double length);
double GetDouble(const char* pText);
long   GetLong  (const char* pText);
short  GetShort (const char* pText);
void   GetString(char* pString, const char* pText);


short   eGuideShapeY, eGuideShapeZ;
long    nPieces;
double  GuideEntranceHeight, GuideExitHeight, FocusZ, D_Foc1Z,
	     GuideEntranceWidth,  GuideExitWidth,  FocusY, D_Foc1Y,
	     piecelength, dTotalLength;
double  GuideMaxWidth, GuideMaxHeight,
        LcntrY, LcntrZ,
		  AxisY, AxisZ;


int main(int argc, char* argv[])
{
	FILE*   pFile;
	char    sString[9], sFileName[50], 
	       *pFullName;

	Init(argc, argv, VT_TOOL);

	printf("---------------------------------------------------------------------\n");
	printf("Calculation of guide shape from entrance, exit size and focus point  \n");
	printf("---------------------------------------------------------------------\n\n");

	eGuideShapeY        = GetShort ("Shape in horizontal direction\n1:linear 3:parabolic 4:elliptic ");
	eGuideShapeZ        = GetShort ("Shape in vertical direction\n1:linear 3:parabolic 4:elliptic ");
	nPieces             = GetLong  ("\nnumber of pieces                         ");
	piecelength         = GetDouble("length of each piece                [cm] ");

	do
	{	GuideEntranceWidth  = GetDouble("entrance width                      [cm] ");
		GuideExitWidth      = GetDouble("exit width                          [cm] ");
		if (eGuideShapeY==VT_ELLIPTIC)
			FocusY           = GetDouble("distance to focus for hor. dir.     [cm] ");
		GuideEntranceHeight = GetDouble("entrance height                     [cm] ");
		GuideExitHeight     = GetDouble("exit height                         [cm] ");
		if (eGuideShapeZ==VT_ELLIPTIC)
			FocusZ           = GetDouble("distance to focus for vert. dir.    [cm] ");
		GetString           (sFileName, "Name of the mirror file                  ");

		dTotalLength = nPieces * piecelength;

		/* write to parameter directory or to FILES in install directory */
		pFullName = FullParName(sFileName);
		if (strcmp(pFullName, sFileName)==0)
			pFullName = FullInstallName(sFileName, "FILES/");
		pFile = fopen(pFullName, "w");
		
		if (pFile!=NULL) 
		{
			double Y,Z;
			int j;

			for(j=0; j <= nPieces; j++)
			{	Y = Width (j*piecelength)/2.0;
				Z = Height(j*piecelength)/2.0;
				if (pFile != NULL)
				{	if (j==0)
					{	fprintf(pFile, "# length [m]  width [cm]  height [cm]\n");
						fprintf(pFile, "#-------------------------------------\n");
					}
					fprintf(pFile, "%10.2f  %10.3f  %10.3f\n", j*piecelength/100.0, 2*Y, 2*Z);
				}
			}
			fclose(pFile)
				;
			/* Writing to log file */
			printf("\n\nTotal length of guide   : %8.3f  m\n", dTotalLength/100.);
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
		}
		else
		{	printf("\nERROR: Output file could not be generated\n");
		}


      printf("\n\n fertig - nochmal (j/n) ");
      scanf ("%s", sString);
      printf("\n\n\n");
   }
   while (sString[0] != 'n');
  
	/* release the buffer memory */
	free(InputNeutrons);
	free(OutputNeutrons);

	return 1;
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


double GetDouble(const char* pText)
{
	double dValue;
	
	printf("%s ", pText);
	scanf ("%lf", &dValue);

	return dValue;
}

long GetLong(const char* pText)
{
	long nValue;
	
	printf("%s ", pText);
	scanf ("%ld", &nValue);

	return nValue;
}


short GetShort(const char* pText)
{
  int nValue;
	
  printf("%s ", pText);
  scanf ("%d", &nValue);

  return (short) nValue;
}


void GetString(char* pString, const char* pText)
{
	printf("%s ", pText);
	scanf ("%s", pString);
}

