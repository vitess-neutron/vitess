/********************************************************************************************/
/*  VITESS module 'sample_refl.c'                                                           */
/*    This module simulates a sample of a reflectometer                                     */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors.                                                                          */
/*                                                                                          */
/* 1.0  Dec  2001  K. Lieutenant  initial version                                           */
/* 1.1  Feb? 2002  K. Lieutenant  average of reflectivity value built in logarithmic scale  */
/* 1.2  Jul? 2002  K. Lieutenant  storing of reflectivity data                              */
/* 2.0  Jan  2002  K. Lieutenant  reorganisation                                            */
/* 2.1  Jul  2003  K. Lieutenant  correction time-of-flight calculation                     */
/* 2.2  Jan  2004  K. Lieutenant  changes for 'instrument.dat'                              */
/* 2.3  Jan  2004  K. Lieutenant  reduced output, only one angle, sign of angle changed     */
/*                                new message for 'Q not in range given by file'            */
/* 2.4  Feb  2004  K. Lieutenant  'FullParName' and 'ERROR' included                        */
/*      Aug  2012  M. Fromme     clean up, variable definition a block start                */
/* 3.1	Mar  2013  D. Nekrassov  Calculation of scattering process takes place in separate  */
/*                               functions, offspecular scattering added                    */
/********************************************************************************************/

#include <string.h>
#include <stdio.h>

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "sample_reflectom.h"
#include "matrix.h"
#include "message.h"
#include "sample.h"

int main(int argc, char **argv)
{
  double     arg, dAnglOutHoriz, dAnglOutVert,
    mRotMatrixOut[3][3];
  short      nIndex;
  VectorType vPath, vDirIn={1.0, 0.0, 0.0}, vDirOut ;
  Neutron    Neutrons ;
  Neutron    parentNeutron; // Neutron needed to store the location of the intersection 
                            // point for offspecular/incoherent scattering
  int resultScattering;
  short int doReflection, doOffspecular, doIncoherent;

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

  CopyVector(vDirIn, vDirOut) ;
  RotVector (mRotMatrixOut, vDirOut) ;
  SubVector (vDirOut, vDirIn) ;
  arg = LengthVector(vDirOut)/2.0 ;    /* arg is sin(scattering angle) */
  
  DECLARE_ABORT;

  doOffspecular = offspecularScattering && (g_nOption==1);
  doReflection = 1 && (!doOffspecular);
  doIncoherent = useIncoherent && (g_nOption==1);
  

  /* Get the neutrons from the file */
  while(ReadNeutrons() != 0)
    {
      /* Loop over all neutrons read */
      for(i=0; i<NumNeutGot; i++)
        {
         
	  double mod;
          CHECK;

	  mod = sqrt(  sq(InputNeutrons[i].Vector[0]) + sq(InputNeutrons[i].Vector[1]) + sq(InputNeutrons[i].Vector[2]));	  

	  /* copies input data to output data */
	  Neutrons   = InputNeutrons[i];
	  
	  Neutrons.Vector[0] = Neutrons.Vector[0]/mod;
	  Neutrons.Vector[1] = Neutrons.Vector[1]/mod;
	  Neutrons.Vector[2] = Neutrons.Vector[2]/mod;
	  
	  g_dProbIn  = InputNeutrons[i].Probability ;
	  g_dProbOut = 0.0;	      
	      
	  /* selects CE on which the neutron is reflected and gives global variables in	the frame of CE */
	  nIndex = Reflect(&Neutrons, 1) ;
	  
	  if(nIndex == 0) /* no CE was found */
	    continue;
	  
	  /* moment of arriving at the sample plane (x=0.0), new position */
	  Neutrons.Time += (0.0 - Neutrons.Position[0]) / Neutrons.Vector[0] / V_FROM_LAMBDA(Neutrons.Wavelength)/**/ ;
	  CopyVector(Neutrons.Vector, vPath) ;
	  
	  MultiplyByScalar(vPath, - Neutrons.Position[0] / Neutrons.Vector[0] ) ;
	  AddVector  (Neutrons.Position, vPath) ; /* vPath = displacement vector */
	  
	  // Save the neutron with primary direction and weight for offspecular scattering
	  parentNeutron = Neutrons;	  	  
	    
	  // Here, the specular reflection case is treated
	  if (doReflection) {

	    resultScattering = ScatterSpecular(arg, &InputNeutrons[i], &Neutrons);
	    if (resultScattering == 0) continue;
	    
	    // Here, the incoherent scattering is treated
	    if (doIncoherent) {

	      Neutrons   = InputNeutrons[i];
	      /* selects CE on which the neutron is reflected and gives global variables in	the frame of CE */
	      Reflect(&Neutrons, 1) ;

	      /* moment of arriving at the sample plane (x=0.0), new position */
	      Neutrons.Time += (0.0 - Neutrons.Position[0]) / Neutrons.Vector[0] / V_FROM_LAMBDA(Neutrons.Wavelength)/**/ ;
	      CopyVector(Neutrons.Vector, vPath) ;  /* vPath = displacement vector */
	  
	      MultiplyByScalar(vPath, - Neutrons.Position[0] / Neutrons.Vector[0] ) ;
	      AddVector  (Neutrons.Position, vPath) ;

	      ScatterIncoherent(&Neutrons);
	    }

	  }
	  
	  // Here, the offspecular scattering is treated
	  else if (doOffspecular) 
	    ScatterOffspecular(arg, &InputNeutrons[i], &parentNeutron, &Neutrons);
	  
        }
    }
  
  /* Do the general cleanup */
 my_exit:
  OwnCleanup();
  Cleanup(TranslFoc[0], TranslFoc[1], TranslFoc[2], AnglFocHoriz, AnglFocVert);

  return 0;
}


/* controls, if neutron is reflected and gives output in frame of CE */

short	Reflect(Neutron* p_pNeutron, short int treatingReflection)
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

  // Do the incoherent scattering here
  if (!treatingReflection) {
    double pathInSample, prob;
    SubVector(ISP2, ISP1);
    pathInSample = MonteCarlo(0, LengthVector(ISP2));;
    prob = 1. - exp(-muInc*pathInSample);
    p_pNeutron->Probability *= prob;
    if (prob > maxProb) maxProb = prob;
    if (p_pNeutron->Probability <= wei_min) return (FALSE);
  }

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

/* own initialization of the sample_reflectom module */

void OwnInit(int argc, char *argv[])
{
	char *arg = NULL;
	SampleType sample;

	/* Initialize */
	g_pTabQ         = NULL;
	g_pTabR         = NULL;
	g_pTab_Qin_Qout = NULL;
	g_pTab_RoffSpec = NULL;
	g_pReflFileName = NULL;
	g_pReflFile     = NULL;
	g_dProbIn       = 0.0;
	g_dProbOut      = 0.0;
	NumOut          = 0 ;
	g_nOption       = 1 ;
	g_nNoAngle      = 1 ;
	maxProb         = 0.;
	detWidth        = 0.;
	detHeight       = 0.;
        signalToBkgAreaFactor = 1.;
        minTheta        = 0;
	maxTheta        = 0;
	offspecularScattering = 0;
	numQinPoints    = 0;

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
			case 'B':
                          { int v;
                            sscanf(arg, "%d", &v) ;
                            useIncoherent = (short)v;
                            break;
                          }
			case 'X':
			        muInc =atof(&argv[1][2]);
				fprintf(LogFilePtr,"mu Incoherent: %f \n", muInc);
				break;	
			case 'p':
			        detWidth = atof(&argv[1][2]);
				break;	
		        case 't':
			        detHeight = atof(&argv[1][2]);
				break;
		        case 'd':
			        detDist = atof(&argv[1][2]);
				break;
		        case 'S':	
			        signalToBkgAreaFactor = atof(&argv[1][2]);
				break;
		        case 'o':
        			offspecularScattering = atoi(&argv[1][2]);
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

	
	if (useIncoherent) CalculateThetaRange();

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

	
	
	sample.Position[0] = PosCE[0];
	sample.Position[1] = PosCE[1];
	sample.Position[2] = PosCE[2];

	sample.Direction[0] = 1.;
	if (g_dRotHoriz > 0) sample.Direction[1] = tan(g_dRotHoriz - M_PI_2);
	if (g_dRotVert > 0) sample.Direction[2] = tan(g_dRotVert - M_PI_2);

	sample.SG.Cube.thickness = DimCE[2];
	sample.SG.Cube.width = DimCE[1];
	sample.SG.Cube.height = DimCE[0];
	sample.Type = VT_CUBE;

	SetSampleGeometry(&sample);

	return;

}/* End OwnInit */



/* own cleanup of the sample-reflection module */

void OwnCleanup()
{
	/* print error that might have occured many times */
	PrintMessage(SMPL_Q_RANGE_TOO_SMALL, "", ON);

	fprintf(LogFilePtr,"Maximum scattering probability reached: %f \n", maxProb);
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
	  g_nLinesRefl = LinesInFile(g_pReflFile);
	  if (offspecularScattering == 0) {
	    /* reads number of lines, allocates memory and then reads the specular reflectivity file */
	    //	    g_nLinesRefl = LinesInFile(g_pReflFile);
	    g_pTabQ      = calloc(g_nLinesRefl, sizeof(double));
	    g_pTabR      = calloc(g_nLinesRefl, sizeof(double));
	    for(n=0; n<g_nLinesRefl; n++)
	      {  ReadLine(g_pReflFile, Buffer, CHAR_BUF_LENGTH);
		sscanf  (Buffer,"%le%c%le", &g_pTabQ[n], &c1, &g_pTabR[n]);
	      }
	  }

	  /* reads the offspecular reflectivity file (q_i, q_f,ij, R) */
	  else {
	 
	    double q_i = 0;
	    double q_f = 0;
	    double refl = 0;
	    int numColumnsFound = -1;
	    unsigned int innerCounter = 0;
	    unsigned int outerCounter = 0;
	    double q_i_prev = 0;
	    double q_f_array[g_nLinesRefl];
	    double refl_array[g_nLinesRefl];
	    int i;

	    g_pTab_Qin_Qout = calloc(g_nLinesRefl, sizeof(double*));
	    g_pTab_RoffSpec = calloc(g_nLinesRefl, sizeof(double*));

	    while (!feof(g_pReflFile)) {
	    
	      numColumnsFound = fscanf(g_pReflFile, "%le %le %le", &q_i, &q_f, &refl);

	      if (numColumnsFound < 2) {
		fgets(Buffer, CHAR_BUF_LENGTH, g_pReflFile);
		continue;
	      }
	      
	      if (innerCounter == 0 && outerCounter == 0) q_i_prev = q_i;
		
	      q_f_array[innerCounter] = q_f;
	      refl_array[innerCounter] = refl;

	      if (q_i_prev != q_i) {
		g_pTab_Qin_Qout[outerCounter] = calloc(innerCounter+2, sizeof(double));
		g_pTab_RoffSpec[outerCounter] = calloc(innerCounter, sizeof(double));
		g_pTab_Qin_Qout[outerCounter][0] = (double) innerCounter;
		g_pTab_Qin_Qout[outerCounter][1] = (double) q_i_prev;

		for (i = 0; i < innerCounter; i++) {
		  g_pTab_Qin_Qout[outerCounter][i+2] = q_f_array[i];
		  g_pTab_RoffSpec[outerCounter][i] = refl_array[i];
		}
		
		innerCounter=0;
		outerCounter++;
		q_i_prev = q_i;
		
	      }
	      else {
		q_i_prev = q_i;
		innerCounter++;
	      }

	    }

	    if (innerCounter > 0) {
	      g_pTab_Qin_Qout[outerCounter] = calloc(innerCounter+2, sizeof(double));
	      g_pTab_RoffSpec[outerCounter] = calloc(innerCounter, sizeof(double));
	      g_pTab_Qin_Qout[outerCounter][0] = (double) innerCounter;
	      g_pTab_Qin_Qout[outerCounter][1] =  q_i_prev;
	      
	      for (i = 0; i < innerCounter; i++) {
		g_pTab_Qin_Qout[outerCounter][i+2] = q_f_array[i];
		g_pTab_RoffSpec[outerCounter][i] = refl_array[i];
	      }
	      
	      outerCounter++;
	      
	    }
	    numQinPoints = outerCounter;

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

void CalculateThetaRange()
{
  // Calculate the theta range covered by the detector
  double theta = atan(detHeight/2./detDist);
  minTheta = 2.*g_dRotAngle - theta;
  maxTheta =  2.*g_dRotAngle + theta;
}

// For detectors close to the direct beam, deltaPhi is a function of theta
// Calculate corresponding deltaPhi for each trajectory individually.
void CalculatePhiRange(double theta, double* phiMin, double* phiMax, int* switchSign)
{
  double h0, h, h0Dist, hDist, largestDist;

  // Calculate the location at the detector which is hit by the direkt beam
  h0 = -detDist * tan(g_dRotAngle*2.);

  // Calculate the location that is hit by the trajectory with the angle of theta
  h = detDist * tan(theta - g_dRotAngle*2.);

  // Distance between end of detector and the direct beam position
  h0Dist = fabs(fabs(h0) - detHeight/2.);

  // Distance between direct beam and currect trajectory position
  hDist = fabs(h0 - h);

  largestDist = sqrt(pow(detWidth/2., 2) + pow(h0Dist, 2));

  if (fabs(h0) >= detHeight/2.) {
    if (largestDist >= hDist) {
      *phiMin = -acos(h0Dist/hDist);
      *phiMax = -*phiMin;
    }
    else {
      *phiMin = -asin(detWidth/2./hDist);
      *phiMax = -*phiMin;
    }
  }
  else {

    if (hDist <= h0Dist) {
      *phiMin = -M_PI;
      *phiMax = M_PI;
    }
    else if (hDist > h0Dist && hDist <= detWidth/2.) {

      *phiMin = (-1.)*(M_PI/2. + asin(h0Dist/hDist));
      *phiMax = -*phiMin;

    }
    else if (hDist > detWidth/2. && hDist <= (detHeight - h0Dist)) {

      *phiMin = (-1.)*(M_PI/2. - acos(detWidth/(2.*hDist)));
      *phiMax = -*phiMin;

    }
    else if (hDist > (detHeight - h0Dist) && hDist <= largestDist) {

      *phiMin = (-1.)*(M_PI/2. - acos(detWidth/(2.*hDist)));
      *phiMax = (-1.)*(acos((detHeight - h0Dist)/hDist));
      *switchSign = 1.;

    }
    else {
      *phiMin = 0.;
      *phiMax = 0.;
    }
  }

  //  fprintf(LogFilePtr,"theta: %f, h: %f, h0: %f, phiMin: %f, phiMax: %f \n", theta * 180./M_PI, h, h0, *phiMin* 180./M_PI, *phiMax* 180./M_PI);

}

// Calculate trajectory parameters after specular scattering
int ScatterSpecular(double scatteringAngle, Neutron* inputNeutron, Neutron* outputNeutron)
{

  
  double divy, divz, theta, dQ, dR;

  divy = (double) asin(inputNeutron->Vector[1] 
		       / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
			      inputNeutron->Vector[2]*inputNeutron->Vector[2]));
  
  divz = (double) asin(inputNeutron->Vector[2] 
		       / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
			      inputNeutron->Vector[1]*inputNeutron->Vector[1]));
  

  if (strcmp(g_sRotAxis, "Z")==0) 
    theta = (double) asin(scatteringAngle) - divy;
  
  else 
    theta = (double) asin(scatteringAngle) - divz;
  
  
  /* computes momentum transfer */
  dQ = 4.*M_PI*sin(theta)/outputNeutron->Wavelength;
  
  /* probability of reflection */
  if (g_nOption==2)
    dR = 1.0 ;           /* reference sample has reflectivity 1 */
  else
    dR = ReadReflect(dQ);
  
  g_dProbOut = g_dProbIn * dR ;
  
  if(g_dProbOut <= wei_min)
  return 0;
  
  //Convert back to global coordinate system and write neutron to the output stream
  TransformBackToGlobalSystemAndWriteNeutron(outputNeutron);

  return 1;

}

// Create new trajectories and calculate their parameters for offspecular scattering
void ScatterOffspecular(double scatteringAngle, Neutron* inputNeutron, Neutron* parentNeutron, Neutron* outputNeutron)
{

  double divy, divz, theta, dQ, dR;
  double currentQf = 0;
  short int offspecularRunning = 1;
  long int currentQfBin = 0;

  divy = (double) asin(inputNeutron->Vector[1] 
		       / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
			      inputNeutron->Vector[2]*inputNeutron->Vector[2]));
  
  divz = (double) asin(inputNeutron->Vector[2] 
		       / sqrt(inputNeutron->Vector[0]*inputNeutron->Vector[0] + 
			      inputNeutron->Vector[1]*inputNeutron->Vector[1]));
  

  if (strcmp(g_sRotAxis, "Z")==0) 
    theta = (double) asin(scatteringAngle) - divy;
  
  else 
    theta = (double) asin(scatteringAngle) - divz;
  
  /* computes momentum transfer */
  dQ = 2.*M_PI*sin(theta)/outputNeutron->Wavelength;
 
  //Loop through all q_f entries of the corresponding q_i value, 
  //create one neutron per entry
  while (offspecularRunning) {

    currentQfBin = FindQf(dQ, currentQfBin, &currentQf, &dR);
    
    if (currentQfBin < 0) {
      offspecularRunning = 0;
      break;
    }
    
    ScatterByQf(parentNeutron, outputNeutron, dQ, currentQf);
    
    g_dProbOut = g_dProbIn * dR ;			
    if(g_dProbOut <= wei_min)
      continue;

    //Convert back to global coordinate system and write neutron to the output stream
    TransformBackToGlobalSystemAndWriteNeutron(outputNeutron);
  }
  
  return;

}

// Scatters the neutron isotropically into a given detector
void ScatterIncoherent(Neutron* outputNeutron)
{

    double phiMin, phiMax, deltaPhi;
    int switchSign;
    double deltaTheta = maxTheta - minTheta;
    double realPhi, realTheta;
    double theta, phi;
    
    if (maxTheta > 0 && minTheta < 0) {
      deltaTheta = Max(maxTheta, fabs(minTheta));
      if (fabs(minTheta) > maxTheta)
	theta = MonteCarlo(minTheta, 0);
      else
	theta = MonteCarlo(0, maxTheta);
    } else
      theta = MonteCarlo(minTheta, maxTheta);
    
    switchSign = 0;
    
    CalculatePhiRange(theta, &phiMin, &phiMax, &switchSign);
    
    deltaPhi = (phiMax - phiMin)*(switchSign+1.);
    phi = MonteCarlo(phiMin, phiMax);
    
    if (switchSign) {
      double random = MonteCarlo(-1, 1);
      phi *= fabs(random)/random;
    }
    
    outputNeutron->Probability *= fabs(sin(theta))*deltaTheta*deltaPhi/M_PI*signalToBkgAreaFactor;

    realPhi = theta*sin(phi);
    realTheta = theta*cos(phi);

    outputNeutron->Vector[0] = cos(realTheta)*cos(realPhi);
    outputNeutron->Vector[1] = cos(realTheta)*sin(realPhi);
    outputNeutron->Vector[2] = sin(realTheta);
    outputNeutron->Color += 5000;
    
        /* makes depth correction to get back to the old frame for Depth != 0 */
    RotBackVector(RotMatrixCE, Depth) ;
    AddVector(outputNeutron->Position, Depth) ;
    
    /* computes neutron variables in the output frame */
    SubVector(outputNeutron->Position, TranslFoc) ;
    RotVector(RotMatrixFoc, outputNeutron->Position) ;  /* necessary only for user */
    RotVector(RotMatrixFoc, outputNeutron->Vector) ;    /* defined output frame    */
    
    /*	writes output binary file */
    NumOut++ ;
    WriteNeutron(outputNeutron) ;

    return;
    
}


// Finds the current Q_f value for the offspecular scattering
int FindQf(double Qin, int QfBin, double* Qf, double* refl)
{

  double dTQin1=0.0, dTQin2, dTQf1, dTQf2, dTQfd;
  double dLTRd, dLTRa, dLTRn;
  short  n=0;

  while (n+1 < numQinPoints  &&  g_pTab_Qin_Qout[n][1] < Qin)
    {	n++;
    }

  if (n == 0) {
    *refl=1;
    *Qf = 0;
    return -1;
  }


  if (n+1 < numQinPoints)
    {	/* linear extrapolation between neighbouring Qin and Qf bins in logarithmic scale */
      if (g_pTab_Qin_Qout[n+1][1] != g_pTab_Qin_Qout[n][1])
        {	
	  dTQin1     = g_pTab_Qin_Qout[n][1];
          dTQin2     = g_pTab_Qin_Qout[n+1][1];

	  dTQf1     = g_pTab_Qin_Qout[n][QfBin +2];
	  dTQf2     = g_pTab_Qin_Qout[n+1][QfBin +2];
	  if (Qin > dTQin1 && Qin < dTQin2) dTQfd = (dTQf1/(Qin - dTQin1)  + dTQf2/(dTQin2 - Qin)) / (1./(Qin - dTQin1) + 1./(dTQin2 - Qin));
	  else if (Qin == dTQin1) dTQfd = dTQf1;
	  else dTQfd = dTQf2;
	  *Qf = dTQfd;
	  if (g_pTab_RoffSpec[n][QfBin] == 0 || g_pTab_RoffSpec[n+1][QfBin] == 0) {
	    *refl = 0;
	    return -1;
	  }
          dLTRa    = log(g_pTab_RoffSpec[n][QfBin]);
          dLTRn    = log(g_pTab_RoffSpec[n+1][QfBin]);
          dLTRd    = dLTRa  +  (dLTRn-dLTRa ) / (dTQin2-dTQin1) * (Qin - dTQin1);
          *refl = exp(dLTRd);
        }
      else
        {	
	  *refl = g_pTab_RoffSpec[n+1][QfBin];
        }
    }
  else
    return -1;

  if ((QfBin+1) < g_pTab_Qin_Qout[n][0] && (QfBin+1) < g_pTab_Qin_Qout[n+1][0]) {
      return (QfBin+1);
  }
  else return -1;
  
  return -1;


}

// Determines the direction of the neutron at the scattering location such that
// the direction vector matches the requires Q_f
void ScatterByQf(Neutron* ParentNeutron, Neutron* Neutrons, double dQin, double dQf)
{

  VectorType nDir;  
  long double vDiff0;
  long double vDiff1;
  int i;
  short switchSign = 0;

  //  fprintf(LogFilePtr,"Direction in sample frame: %f %f %f\n", ParentNeutron->Vector[0], ParentNeutron->Vector[1], ParentNeutron->Vector[2]);

  for (i=0; i < 3; i++) nDir[i] = ParentNeutron->Vector[i];

  vDiff0 = nDir[0];
  nDir[0] *= dQf/dQin;
  if (fabs(nDir[0]) > fabs(vDiff0)) switchSign = 1;
  vDiff0 = vDiff0 - nDir[0];
  
  if (switchSign == 1) vDiff0 = fabs(vDiff0) * (-1.);
  else vDiff0 = fabs(vDiff0);

  if (strcmp(g_sRotAxis, "Z")==0) {

    FillRotMatrixZY(rotMatrixOffSpec2, 0, (g_dRotHoriz-M_PI_2));
    
    vDiff1 = -1.*fabs(nDir[1]) + sqrt(nDir[1]*nDir[1] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    if ((switchSign && nDir[2] < 0) || (!switchSign && nDir[2] < 0 && vDiff1 > 0)) vDiff1*=-1.;
    nDir[1] += vDiff1;
    
    RotBackVector(rotMatrixOffSpec2, nDir);

  }
  else {

    FillRotMatrixZY(rotMatrixOffSpec2, (g_dRotVert-M_PI_2), 0);

    vDiff1 = -1.*fabs(nDir[2]) + sqrt(nDir[2]*nDir[2] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    //if ((vDiff1*nDir[2] < 0 && switchSign == 0) || (vDiff1*nDir[2] > 0 && switchSign == 1)) vDiff1 *= -1.*fabs(nDir[2]) - sqrt(nDir[2]*nDir[2] + 2.*fabs(nDir[0])*vDiff0 - vDiff0*vDiff0);
    if ((switchSign && nDir[2] < 0) || (!switchSign && nDir[2] < 0 && vDiff1 > 0)) vDiff1*=-1.;
    nDir[2] += vDiff1;

    RotBackVector(rotMatrixOffSpec2, nDir);

  }

  for (i=0; i < 3; i++) Neutrons->Vector[i] = nDir[i];

  //  fprintf(LogFilePtr,"Direction in sample frame after re-orientation: %f %f %f %f %f\n", Neutrons->Vector[0], Neutrons->Vector[1], Neutrons->Vector[2], dQin, dQf);

  return;

}

// Neutron parameters are transformed back to the original coordinate system,
// taking into account a possible user outpur frame, and written to the stream.
void TransformBackToGlobalSystemAndWriteNeutron(Neutron* outputNeutron)
{

  outputNeutron->Probability = g_dProbOut;

 /* computes reflected direction in the frame of CE */
    outputNeutron->Vector[0] = -outputNeutron->Vector[0];
    
    /* computes neutron variables in the initial frame */
    RotBackVector(RotMatrixCE, outputNeutron->Position) ;
    RotBackVector(RotMatrixCE, outputNeutron->Vector) ;
    AddVector(outputNeutron->Position, PosCE) ;
    
    /* makes depth correction to get back to the old frame for Depth != 0 */
    RotBackVector(RotMatrixCE, Depth) ;
    AddVector(outputNeutron->Position, Depth) ;
    
    /* computes neutron variables in the output frame */
    SubVector(outputNeutron->Position, TranslFoc) ;
    RotVector(RotMatrixFoc, outputNeutron->Position) ;  /* necessary only for user */
    RotVector(RotMatrixFoc, outputNeutron->Vector) ;    /* defined output frame    */
    
    /*	writes output binary file */
    NumOut++ ;
    WriteNeutron(outputNeutron) ;
    
    return;
    
}
