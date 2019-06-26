/********************************************************************************************/
/*  VITESS module 'sample_refl.c'                                                           */
/*    This module simulates a sample of a reflectometer                                     */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors.                                                                          */
/*                                                                                          */
/* 1.0  Dec  2001  K. Lieutenant  initial version                                           */
/* 1.1  Feb? 2002  K. Lieutenant  average of reflectivity value built in logarithmic scale  */
/* 1.2  Jul? 2002  K. Lieutenant  storing of reflectivity data                              *
/* 2.0  Jan  2002  K. Lieutenant  reorganisation                                            */
/* 2.1  Jul  2003  K. Lieutenant  correction time-of-flight calculation                     */
/* 2.2  Jan  2004  K. Lieutenant  changes for 'instrument.dat'                              */
/* 2.3  Jan  2004  K. Lieutenant  reduced output, only one angle, sign of angle changed     */
/*                                new message for 'Q not in range given by file'            */
/* 2.4  Feb  2004  K. Lieutenant  'FullParName' and 'ERROR' included                        */
/********************************************************************************************/

#include <string.h>

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "sample_reflectom.h"
#include "matrix.h"
#include "message.h"


int main(int argc, char **argv)
{
	double     arg, thr, dQ, dR, dAnglOutHoriz, dAnglOutVert,
	           mRotMatrixOut[3][3];
	short      nIndex;
	VectorType vPath, vDirIn={1.0, 0.0, 0.0}, vDirOut ;
	Neutron    Neutrons ;

	/* Initialize the program according to the parameters given   */
	Init   (argc, argv, VT_SMPL_REFL);
	print_module_name("sample_reflectom 2.4") ;
	OwnInit(argc, argv);
	MsgInit();
       
	/* computes rotation matrixes corresponding to CE offset angles */
	FillRotMatrixZY(RotMatrixCE, g_dRotVert, g_dRotHoriz) ;

	/* In reflection measurements the scattering angle must be determined from the orientation
		of the sample (RotHoriz, RotVert) */
	AnglesOutputFrame(180/M_PI*g_dRotHoriz, 180/M_PI*g_dRotVert, &dAnglOutHoriz, &dAnglOutVert) ;
	FillRotMatrixZY  (mRotMatrixOut, M_PI/180.*dAnglOutVert, M_PI/180.*dAnglOutHoriz) ;

	DECLARE_ABORT

	/* Get the neutrons from the file */
	while(ReadNeutrons() != 0)
	{

		/* Loop over all neutrons read */
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK

			/* corrections: 
				InputNeutrons[i].Position[0]	= 0. ; */
			InputNeutrons[i].Vector[0]	= (double) sqrt(1 - sq(InputNeutrons[i].Vector[1]) - sq(InputNeutrons[i].Vector[2])) ;

			/* copies input data to output data */
			Neutrons   = InputNeutrons[i];

			g_dProbIn  = InputNeutrons[i].Probability ;
			g_dProbOut = 0.0;


			/* selects CE on which the neutron is reflected and gives global variables in	the frame of CE */
			nIndex = Reflect(&Neutrons) ;

			if(nIndex == 0) /* no CE was found */ 
				continue;


			/* moment of arriving at the sample plane (x=0.0), new position */
			Neutrons.Time += (0.0 - Neutrons.Position[0]) / Neutrons.Vector[0] / V_FROM_LAMBDA(Neutrons.Wavelength)/**/ ;
			CopyVector(Neutrons.Vector, vPath) ;

			MultiplyByScalar(vPath, - Neutrons.Position[0] / Neutrons.Vector[0] ) ;
			AddVector  (Neutrons.Position, vPath) ; /* vPath = displacement vector */


			/* scattering angle */
			CopyVector(vDirIn, vDirOut) ;
			RotVector (mRotMatrixOut, vDirOut) ;
			SubVector (vDirOut, vDirIn) ;
			arg = LengthVector(vDirOut)/2.0 ;    /* arg is sin(scattering angle) */
			thr = (double) asin(arg) ;

			/* computes momentum transfer */
			dQ = 4*M_PI*sin(thr)/Neutrons.Wavelength;


			/* probability of reflection */
			if (g_nOption==2)
				dR = 1.0 ;           /* reference sample has reflectivity 1 */
			else 
				dR = ReadReflect(dQ);

			g_dProbOut = g_dProbIn * dR ;

			if(g_dProbOut <= wei_min)
				continue;

			Neutrons.Probability = g_dProbOut;

			/* prepares data for writeout and writes */
			/* computes reflected direction in the frame of CE */
			Neutrons.Vector[0] = -Neutrons.Vector[0];

			/* computes neutron variables in the initial frame */
			RotBackVector(RotMatrixCE, Neutrons.Position) ;
			RotBackVector(RotMatrixCE, Neutrons.Vector) ;
			AddVector(Neutrons.Position, PosCE) ;

			/* makes depth correction to get back to the old frame for Depth != 0 */
			RotBackVector(RotMatrixCE, Depth) ;
			AddVector(Neutrons.Position, Depth) ;

			/* computes neutron variables in the output frame */
			SubVector(Neutrons.Position, TranslFoc) ;
			RotVector(RotMatrixFoc, Neutrons.Position) ;  /* necessary only for user */
			RotVector(RotMatrixFoc, Neutrons.Vector) ;    /* defined output frame    */

			/*	writes output binary file */
			NumOut++ ;
			WriteNeutron(&Neutrons) ;
		}
	}

	/* Do the general cleanup */
  my_exit:
	OwnCleanup();
	Cleanup(TranslFoc[0], TranslFoc[1], TranslFoc[2], AnglFocHoriz, AnglFocVert);

	return 0;
}



/* controls, if neutron is reflected and gives output in frame of CE */

short	Reflect(Neutron* p_pNeutron)
{
	VectorType ISP1, ISP2,         /* Intersection points (entry and exit)  */
	           NeutPos, NeutDir;   /* Neutron position and flight direction */

	/* intermediate variables */
	CopyVector(p_pNeutron->Position, NeutPos);
	CopyVector(p_pNeutron->Vector,   NeutDir);


	/* computes neutron variables in the frame of the CE */
	SubVector(NeutPos, PosCE);
	RotVector(RotMatrixCE, NeutPos);
	RotVector(RotMatrixCE, NeutDir);


	/* here computes the depth where the neutron meets the reflecting
		  plane, equivalent to a parallel shift of a t=0 CE in the frame of CE */
	if(IntersectionWithRectangular(DimCE, NeutPos, NeutDir, ISP1, ISP2) == 0) 
		return(FALSE) ;

	Depth[0] = MonteCarlo(ISP1[0], ISP2[0]) ;
	Depth[1] = Depth[2] = 0 ;

	SubVector(NeutPos, Depth) ;


	/* here we have the CE and initialise the values */
	CopyVector(NeutPos, p_pNeutron->Position) ;
	CopyVector(NeutDir, p_pNeutron->Vector) ;

	return(TRUE) ;

}/* End Reflect */



double	ReadReflect(const double p_dQ)
{
	double dReflect=0.0, dTQn=0.0, dTQa;
	double dLTRd, dLTRa, dLTRn;
	short  n=0;

	while (n+1 < g_nLinesRefl  &&  g_pTabQ[n+1] < p_dQ) 
	{	n++;
	}

	if (n+1 < g_nLinesRefl)
	{	/* linear extrapolation in logarithmic scale */
		if (g_pTabQ[n+1] != g_pTabQ[n])
		{	dTQa     = g_pTabQ[n];
			dTQn     = g_pTabQ[n+1];
			dLTRa    = log(g_pTabR[n]);
			dLTRn    = log(g_pTabR[n+1]);
			dLTRd    = dLTRa  +  (dLTRn-dLTRa ) / (dTQn-dTQa) * (p_dQ - dTQa);
			dReflect = exp(dLTRd);
		}
		else
		{	dReflect = g_pTabR[n+1];
		}
	}
	else
	{	/* read error: momentum transfer higher than all values in the reflectivity file */
		CountMessageID(SMPL_Q_RANGE_TOO_SMALL, InputNeutrons[i].ID);
	}

	return dReflect;
}



/* own initialization of the monochromator/analyser module */

void OwnInit(int argc, char *argv[])
{
	char *arg = NULL;

	/* Initialize */
	g_pTabQ         = NULL;
	g_pTabR         = NULL;
	g_pReflFileName = NULL;
	g_pReflFile     = NULL;
	g_dProbIn       = 0.0;
	g_dProbOut      = 0.0;
	NumOut          = 0 ;
	g_nOption       = 1 ;
	g_nNoAngle      = 1 ;


	  /*    INPUT  */
	while(argc>1)
	{
		arg=&argv[1][2];   
		switch(argv[1][1])
		{
			case 'P':
				if((Par_Crys = fopen(FullParName(arg),"r"))==NULL)
				{
					fprintf(LogFilePtr,"\nERROR: parameter file '%s' not found!\n",arg);
					exit(0);
				}
				ParameterFileName=arg;
				break;

			case 'O':
				sscanf(arg, "%ld", &g_nOption) ;
				break;

			case 'I':
				g_pReflFileName = arg;
				break;

			case 'R':
				strcpy(g_sRotAxis, arg);
				break;

			case 'a':
				sscanf(arg, "%lf", &g_dRotAngle) ;
				break;

		}
		argc--;
		argv++;
	}

	/* prints to log file */
	fprintf(LogFilePtr,"initialised option: ") ;

	switch (g_nOption)
	{	case 1: 
			fprintf(LogFilePtr,"	'sample'\n") ;
			ReadReflectivityFile();
			break;
		case 2:
			fprintf(LogFilePtr,"	'reference'\n") ;
			break;
		default:
			fprintf(LogFilePtr,"ERROR: No valid option (1 or 2) found!\n") ;
			exit(0) ;
	}


	/* reads parameter file */
	fprintf(LogFilePtr,"data from parameter file: '%s'\n",ParameterFileName) ;
	ReadParameterFile() ;

	if (Par_Crys != NULL) fclose(Par_Crys) ;


	/* converts degs in radian etc. */
	g_dRotAngle  *= M_PI/180. ;
	g_dRotHoriz  *= M_PI/180. ;
	g_dRotVert   *= M_PI/180. ;
	AnglFocHoriz *= M_PI/180. ;
	AnglFocVert	 *= M_PI/180. ;

	/* computes rotation matrix corresponding to the output frame
	   (focus direction) */
	FillRotMatrixZY(RotMatrixFoc, AnglFocVert, AnglFocHoriz) ;

	/* determines rotation angles in horiz. and vert. direction */
	if (strcmp(g_sRotAxis, "Z")==0)
	{  g_dRotHoriz = M_PI_2 + g_dRotAngle;
		g_dRotVert  = 0.0;
	}
	else if (strcmp(g_sRotAxis, "Y")==0)
	{  g_dRotHoriz = 0.0;
		g_dRotVert  = M_PI_2 + g_dRotAngle;
	}
	else
	{  fprintf(LogFilePtr,"ERROR: wrong value for rotation axis: %s\n", g_sRotAxis); 
		exit(0);
	}

}/* End OwnInit */



/* own cleanup of the sample-reflection module */

void OwnCleanup()
{
	/* print error that might have occured many times */
	PrintMessage(SMPL_Q_RANGE_TOO_SMALL, "", ON);

	fprintf(LogFilePtr," \n");

	/* set description for instrument plot */
	stPicture.eType = (short) g_nOption;
	stPicture.dWPar = DimCE[1];
	stPicture.dHPar = DimCE[2];

	/* free allocated memory */
	if (g_pTabQ!=NULL)  free(g_pTabQ);
	if (g_pTabR!=NULL)  free(g_pTabR);

	/* closes reflection coefficient files */
	if (g_pReflFile != NULL) fclose(g_pReflFile) ;

}/* End OwnCleanup */



/* ReadParameterFile() reads the parameters from "crys.par" */

void ReadParameterFile()
{
	/* reads from file by using ReadParF(Par_Crys) and ReadParComment(Par_Crys) */
	PosCE[0]=ReadParF(Par_Crys); PosCE[1]=ReadParF(Par_Crys); PosCE[2]=ReadParF(Par_Crys); ReadParComment(Par_Crys) ;
	DimCE[0]=ReadParF(Par_Crys); DimCE[1]=ReadParF(Par_Crys); DimCE[2]=ReadParF(Par_Crys); ReadParComment(Par_Crys) ;

	User = ReadParI(Par_Crys);     ReadParComment(Par_Crys) ;

	if(User == 1)
	/* some input data for user defined output frame */
	{  
		AnglFocHoriz=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;
		AnglFocVert =ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;

		TranslFoc[0]=ReadParF(Par_Crys) ; TranslFoc[1]=ReadParF(Par_Crys) ; TranslFoc[2]=ReadParF(Par_Crys) ; ReadParComment(Par_Crys) ;
	}
	else
	/* sets default values if frame for output not user defined */
	{
		/* standard output frame is got by a shift along the x-axis (without change in direction) */
		AnglFocHoriz = 0.0;
		AnglFocVert  = 0.0;

		/* shifts output frame origin to center of focussing geometry */
		CopyVector(PosCE, TranslFoc) ;
	}

	/* prints parameters into log file for verification */
	fprintf(LogFilePtr,"  main position X, Y, Z    = %9.4f, %9.4f, %9.4f\n"
	                   "  thickness, width, height = %9.4f, %9.4f, %9.4f\n"
	                   "  angle                    = %9.4f around %1.1s-Axis \n",
	                   PosCE[0], PosCE[1], PosCE[2],  DimCE[0], DimCE[1], DimCE[2],  
	                   g_dRotAngle, g_sRotAxis);

	if(User == 1) 
	{	fprintf(LogFilePtr,"user defined frame:\n") ;
		fprintf(LogFilePtr,"  horizontal angle = %9.4f\n"
		                   "  vertical angle   = %9.4f\n"
		                   "  X',Y',Z'         = %9.4f, %9.4f, %9.4f\n",
								 AnglFocHoriz, AnglFocVert,  TranslFoc[0], TranslFoc[1], TranslFoc[2]) ;
	}
	else
	{	fprintf(LogFilePtr,"standard frame generation used\n") ;
	}


}/* End ReadParameterFile */

     

/* ReadReflectivityFile() reads the function R(Q) */

void ReadReflectivityFile()
{
	short n;
	char  c1, Buffer[CHAR_BUF_LENGTH];

	if(g_pReflFileName!=NULL) 
	{
		if((g_pReflFile=fopen(FullParName(g_pReflFileName),"rt"))==NULL) 
		{
			fprintf(LogFilePtr,"ERROR: reflection file '%s' not found!\n", g_pReflFileName);
			exit(0);
		}
		else
		{
			/* reads number of lines, allocates memory and then reads distribution file */
			g_nLinesRefl = LinesInFile(g_pReflFile);
			g_pTabQ      = calloc(g_nLinesRefl, sizeof(double));
			g_pTabR      = calloc(g_nLinesRefl, sizeof(double));
			for(n=0; n<g_nLinesRefl; n++)
			{  ReadLine(g_pReflFile, Buffer, CHAR_BUF_LENGTH);
				sscanf  (Buffer,"%le%c%le", &g_pTabQ[n], &c1, &g_pTabR[n]);
			}
		}
	}
	else
	{
		fprintf(LogFilePtr,"ERROR: no reflection file name given!\n");
		exit(0);
	}

	return;
}


/* 'AnglesOutputFrame' computes frame angles of output */
/*                                                     */
void AnglesOutputFrame(double RotHoriz, double RotVert, double *AnglFocHoriz, double *AnglFocVert)
{
	double n[3] ;

	FillRotMatrixZY(RotMatrixCE, M_PI/180.*RotVert, M_PI/180.*RotHoriz) ;

	n[0] = 1. ;		n[1] = 0. ;		n[2] = 0. ;

	RotVector(RotMatrixCE, n) ;      /* components of a vector parallel to X in the frame of the CE */

	n[0] *= -1. ;                    /* reflection on the CE */

	RotBackVector(RotMatrixCE, n) ;  /* new components in the frame of input */

	CartesianToEulerZY(n, AnglFocVert, AnglFocHoriz) ;

	if(*AnglFocHoriz == - M_PI) *AnglFocHoriz = M_PI ;
	if(*AnglFocVert  == - M_PI) *AnglFocVert  = M_PI ;

	*AnglFocHoriz	*= 180./M_PI ;
	*AnglFocVert	*= 180./M_PI ;
}
