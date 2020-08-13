/*********************************************************************************************/
/*  VITESS module 'multiple windows'                                                         */
/*                                                                                           */
/* This module simulates a disc containing several windows of circular or rectangular shape  */
/*   (attenuation inside and non-ideal absorption outside the windows can be considered)     */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*  Modified by Manoshin Sergey in Jan 2001 for include gravity effect                       */	
/* Significant revision: multiple circular windows (only) window 05.02.01                    */
/* Include multi circle windows (<100) possibility  07 Feb 2001                              */
/*                                                                                           */
/* 1.00  Feb  2001  S. Manoshin     initial version                                          */	
/* 1.01  June 2001  K. Lieutenant   SOFTABORT                                                */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation, center of beam                           */
/* 2.00  Jun  2003  S. Manoshin     Add material for window frame                            */
/* 2.10  Mar  2004  S. Manoshin     Add material for inner part of window                    */
/* 2.21  Jul  2004  S. Manoshin	    Corrected bug for thick window                           */
/* 2.22  Dec  2007  K. Lieutenant   Option to use rectangular windows added                  */
/* 2.23  Aug  2019  K. Lieutenant   tidy up and visualiation                                 */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"
#include "message.h"


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
void  OwnInit    (int argc, char *argv[]);         // reads input parameters and initializes global variables
short ReadWndFile();                              
void  EvalInput  ();
void  SetGeometry(char* sColor, int nHoles);       // fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
McCompID  _eModule=MCN_WND_MULT;

Plane	    Endpoint,         // Planes through window for zero thickness and
          EndPointO,        // end of the Outer and end of the Inner wall
	        EndPointI;        // Endpoint.D: distance to window along x-axis        [cm]

VtWndGeom eShape=VT_AUTO_SHAPE, // kind of window: defined by input file, circular or rectangular
          eWinShape[101];  
char*     CollFileName=NULL;
double    Distance=0.0,         // Distance from origin to the window (along the x-axis)  [cm]
          OuterRadius=200.0; 
double    winradius [101],      // radii of circular windows                              [cm]
          ywincenter[101],      // y coordinates: centers of windows                      [cm]
          zwincenter[101],      // z coordinates: centers of windows                      [cm]
          winwidth  [101],      // widths of rectangular windows                          [cm]
          winheight [101];      // heights of rectangular windows                         [cm]

long      KeymaterialO=6,       // Window frame material: 0 - from file, 1 - gadolinium, 2 - cadmium,  3 - Bor10,
	                              //                        4 - Eu,        5 - Silicon,    6 - ideal absorber
          KeymaterialI=1;       // Absorption of open part of window, 1 -no(default) 0 - from file

char  *sTransFileNameO=NULL;    // file describing the transmission of the window frame material
FILE  *pTransFileO=NULL; 
double WavO[MAX_MU],            // lambda and µ-values and thickness of the frame material 
       MuO [MAX_MU], 
       ThicknessO=0.0;
long   nValFO=0;                // number of attenuation values in file (for window frame material)

char  *sTransFileNameI=NULL;    // file describing the transmission of the material in the open part of window
FILE  *pTransFileI=NULL; 
double WavI[MAX_MU],            // lambda and µ-values and thickness of the inner material 
       MuI [MAX_MU], 
       ThicknessI=0.0;
long   nValFI=0;                // number of attenuation values in file (for window pane material)


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
	long	  i, j;
	long 	  NumberOfHoles;
	long	  key_abs=0;        // key absorb: 1 - absorption,  0 - transmission

	double  DistSquared=0.0;  // distance from the point of impact to the center of window   
	double  TimeOF=0.0,       // TOF of neutron from origin to window
          TOF3  =0.0;       // TOF of neutron to pass through window material
	double  NewPositionY=0.0, // neutron position in co-ordinate system  rotated by 'rotang'
          NewPositionZ=0.0; 
	double  CenterX=0.0,      // center of beam at window
          CenterY=0.0, 
          CenterZ=0.0, 
          SumProb=0.0;
  double  VelocityReal=0.0, // velocity of the current neutron
          N_Wavelength,     // wavelength of the current neutron
          mu=0.0,           // attenuation coefficient of the material, that the neutron traverses
          prob=0.0;         // probability of traversing the material

  Neutron Output;           // trajectory as it is written to the output

 	
  /******************************************/
  /** Initialisation and Parameter Input   **/
  /******************************************/
  memset(&Output, '\0', sizeof(Neutron));
  
  for(j=0; j<=100; j++)
  {
    ywincenter[j] = 0.0; 
    zwincenter[j] = 0.0;
    winradius [j] = 0.0; 
    winwidth  [j] = 0.0; 
    winheight [j] = 0.0; 
    eWinShape [j] = VT_AUTO_SHAPE;
  }	
  
  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "2.23");
  OwnInit(argc, argv);
  MsgInit();
  EvalInput();

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = TRUE;
		
  // Read Input data from window file: Number of lines equals number of holes in multiple window
  // -------------------------------------------------------------------------------------------
  NumberOfHoles = ReadWndFile();

  DECLARE_ABORT

  /******************************/
  /** Loop over trajectories   **/
  /******************************/
  while(ReadNeutrons()!= 0)
  {
    for(i=0; i<NumNeutGot; i++)
    {
      CHECK
      
      // Remove neutrons with wrong direction or wavelength
      // --------------------------------------------------
      if (InputNeutrons[i].Vector[0] <= 0.0) continue;
      if (InputNeutrons[i].Wavelength == 0.0) continue;
      VelocityReal = V_FROM_LAMBDA(InputNeutrons[i].Wavelength); 
      if (VelocityReal <= 0.0) continue;
			
      // Write intersection point
      // ------------------------
			WriteIAP(&InputNeutrons[i], VT_ENTERED);
			
      // 	Move neutron to window with or without gravity effect and calculate Time of Flight (ms)
      // -----------------------------------------------------------------------------
      if (keygrav == 1)
        TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
      else
        TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);

			// Calculate average position
      // --------------------------
      InputNeutrons[i].Time += TimeOF;
      CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
      CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
      CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
      SumProb   += InputNeutrons[i].Probability;

      // window test
      // -----------
      NewPositionY = InputNeutrons[i].Position[1];
      NewPositionZ = InputNeutrons[i].Position[2];
      DistSquared = NewPositionY*NewPositionY + NewPositionZ*NewPositionZ;
	    TOF3 = 0.0;
				
      if (DistSquared <= OuterRadius*OuterRadius) 
      {
	      key_abs = 1;
		
        for(j=1; j<=NumberOfHoles; j++) 
        {
          if (eWinShape[j]==VT_CIRCLE)
          {	DistSquared =  (NewPositionY - ywincenter[j])*(NewPositionY - ywincenter[j]) 
                         + (NewPositionZ - zwincenter[j])*(NewPositionZ - zwincenter[j]);
            if (DistSquared <= winradius[j]*winradius[j]) 
              key_abs = 0;
          }
          else
          {	if (fabs(NewPositionY - ywincenter[j]) < 0.5*winwidth [j]  && 
                fabs(NewPositionZ - zwincenter[j]) < 0.5*winheight[j]    ) 
              key_abs = 0;
          }
        }
		
        if (key_abs == 1) // absorbed
        {
          if (KeymaterialO == 6)
            continue;
				
          if (keygrav == 1)
            TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPointO);
          else
            TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPointO);
  			
          /* Attenuation during pass of window material */
          N_Wavelength = InputNeutrons[i].Wavelength;    
          mu = Interpolation(N_Wavelength, KeymaterialO, WavO, MuO, nValFO);
          if (mu == -10000.0)
          { // sprintf(sBuffer, "Attenuation coefficient of plate material could not be determined for wavelength %6.3f Ang", N_Wavelength);
            // Error(sBuffer);
            CountMessageID(WNDO_L_RANGE_TOO_SMALL, InputNeutrons[i].ID);
          }
          prob = exp(-mu*TOF3*VelocityReal);	
          InputNeutrons[i].Probability *= prob;
          InputNeutrons[i].Time        += TOF3;	
        }
	    }
		
      if (key_abs == 0) // passed through hole or outside plate
      {
		    /* case of transmission neutron */
        if (keygrav == 1)
          TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPointI);
        else
          TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPointI);

        if ((KeymaterialI == 0)&&(DistSquared <= OuterRadius*OuterRadius))				 
        {
	        /* Attenuation during pass of open window material */
          N_Wavelength = InputNeutrons[i].Wavelength;    
          mu = Interpolation(N_Wavelength,KeymaterialI,WavI,MuI,nValFI);
          if (mu == -10000.0)
          { // sprintf(sBuffer, "Attenuation coefficient of window pane material could not be determined for wavelength %6.3f Ang", N_Wavelength);
            // Error(sBuffer);
            CountMessageID(WNDI_L_RANGE_TOO_SMALL, InputNeutrons[i].ID);
          }
          prob = exp(-mu*TOF3*VelocityReal);	
          InputNeutrons[i].Probability *= prob;
          InputNeutrons[i].Time        += TOF3;	
        }
      }    	 
					
      InputNeutrons[i].Time += (double)TOF3;							
      InputNeutrons[i].Position[0] = 0.0;	
      Output = InputNeutrons[i];
      WriteNeutron(&Output);
    }
  }

/******************************************************************************/
/* Finish: print parameters, write geometry and instrument file, free memory  */
/******************************************************************************/
my_exit:

	fprintf(LogFilePtr,"\n Distance between plane x=0 and window plane  %f cm  \n", fabs(Endpoint.D));
  
  if(keygrav == 1)
    fprintf(LogFilePtr,"Gravity is enabled \n");
  else
    fprintf(LogFilePtr,"Gravity is disabled \n");

  PrintMessage(WNDI_L_RANGE_TOO_SMALL, sTransFileNameI, ON);
  PrintMessage(WNDO_L_RANGE_TOO_SMALL, sTransFileNameO, ON);

  if (SumProb != 0.0)
  {
    CenterX   = CenterX/SumProb;
    CenterY   = CenterY/SumProb; 
    CenterZ   = CenterZ/SumProb; 
    fprintf(LogFilePtr,"Center of beam at the windows: X = %f cm Y = %f cm Z = %f cm \n\n",CenterX, CenterY, CenterZ );
  }
  else
  {
     fprintf(LogFilePtr,"No neutrons at the exit of this module \n\n");
  }

  // fills the structure stGeometry for visualization
  SetGeometry("blue", NumberOfHoles);

  Cleanup(Max(ThicknessO, ThicknessI)+Distance, 0.0, 0.0, 0.0, 0.0);
	
  return(0);
}


/**************************************************************/
/** Reads input parameters and sets them as global variables **/
/**************************************************************/
void  OwnInit   (int argc, char *argv[])
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
					eShape = (VtWndGeom) atol(&argv[i][2]);  // Shape of the individuals apertures: 0 different,  1: circular, 2: rectangular
					break;

				case 'c':
					KeymaterialO = atol(&argv[i][2]);  /* Material of nemder channels: 0 - from file, 1 - gadolinium, 2 - cadmium, 3 -Bor10, 4 - Eu, 5 - Silicon, 6 - ideal absorber */
					break;
				case 'C':
					sTransFileNameO=&argv[i][2]; 
					break;
				case 'm':
					sTransFileNameI=&argv[i][2]; 
					break;
							
				case 't':
					ThicknessO = atof(&argv[i][2]);
					break;
				case 'T':
					ThicknessI = atof(&argv[i][2]);
					break;

				default:
					fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}
  
  Endpoint.A =  1.0;
  Endpoint.B =  0.0;
  Endpoint.C =  0.0;
  Endpoint.D = -1.0* Distance;
	
  EndPointI.A =  1.0;
  EndPointI.B =  0.0;
  EndPointI.C =  0.0;
  EndPointI.D = -1.0*(Distance + ThicknessI);
	
  EndPointO.A =  1.0;
  EndPointO.B =  0.0;
  EndPointO.C =  0.0;
  EndPointO.D = -1.0*(Distance + ThicknessO);	
}
				
					
/*******************************************************/
/** Reads the structure stGeometry for visualization  **/
/*******************************************************/
short ReadWndFile()
{
	char  sLine[256];
	short j, Nholes;            // index over holes and number of holes
	FILE  *coll_file;

	if (CollFileName !=NULL)
	{	if( (coll_file = fopen(CollFileName,"r"))==NULL)
		{
			fprintf(LogFilePtr, "ERROR: File %s could not be opened to read window data \n", CollFileName);
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
		fprintf(LogFilePtr,"\n ERROR: No window file name given \n");
		exit(-1);
	}

	Nholes = j;

	fprintf(LogFilePtr,"\n Number of holes: %d", Nholes);
	fprintf(LogFilePtr,"\n Outer radius of the window: %7.2lf cm", OuterRadius);


	for(j = 1; j <= Nholes; j++) {
	  if (eWinShape[j]==VT_CIRCLE)
	    fprintf(LogFilePtr,"\n Collimator data: center Y = %6.2lf cm  Z = %6.2lf cm radius = %6.2lf cm", 
		    ywincenter[j], zwincenter[j], winradius[j]);
	  else 
	    fprintf(LogFilePtr,"\n Collimator data: center Y = %6.2lf cm  Z = %6.2lf cm  winwidth = %6.2lf cm  winheight= %6.2lf cm", 
		    ywincenter[j], zwincenter[j], winwidth[j], winheight[j]);
	} 
	fprintf(LogFilePtr,"\n") ;

	return Nholes;
}
	

/********************************************************/
/** Analyses input parameters and prepares attenuation **/
/********************************************************/
void  EvalInput()
{
  char sLine[CHAR_BUF_SMALL]="";
	long i, j,                     // indices of arrays for wavelength and attenuation 
	     nVal=0;                   // number of wavelength and attenuation values in the array
  
  // Checks
  // ------
	if (Distance < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Length of space must be >= 0.0 !!!\n");
		exit(-1);
	}
		
	if (ThicknessO < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Thickness of window < 0.0 !!!");
		exit(-1);
	}

	if (ThicknessI < 0.0)
	{
		fprintf(LogFilePtr,"ERROR: Thickness of open part of the window < 0.0 !!!");
		exit(-1);
	}
		
	if (ThicknessO != ThicknessI)
	{
		fprintf(LogFilePtr,"WARNING! It is recommended to have outer and inner thickness EQUAL \n");
	}
	
	// Init
  // ----
	for(j=0; j<MAX_MU; j++)
	{	
		WavO[j] = 0.0;
		MuO [j] = 0.0;
		WavI[j] = 0.0;
		MuI [j] = 0.0;
	}
	
	// Case: zero thickness
  // --------------------
	if (ThicknessO == 0.0)
	{
		KeymaterialO = 6;
	}
	if (ThicknessI == 0.0)
	{
		KeymaterialI = 1;
	}
	
	// output text
  // -----------
  fprintf(LogFilePtr,"Window frame material: ");

  switch (KeymaterialO)
  { case 0:  fprintf(LogFilePtr, "Transmission characteristics read from file %s\n", sTransFileNameO); break;
    case 1:  fprintf(LogFilePtr, "Gadolinium \n"); Gadolinium(WavO, MuO, &nVal); break;
    case 2:  fprintf(LogFilePtr, "Cadmium    \n"); Cadmium   (WavO, MuO, &nVal); break;
    case 3:  fprintf(LogFilePtr, "Bor10      \n"); Bor10     (WavO, MuO, &nVal); break;
    case 4:  fprintf(LogFilePtr, "Eu         \n"); Eu        (WavO, MuO, &nVal); break;
    case 5:  fprintf(LogFilePtr, "Silicon    \n"); Silicon   (WavO, MuO, &nVal); break;
    case 6:  fprintf(LogFilePtr, "Ideal absorber \n");                           break;
    default: fprintf(LogFilePtr, "\n"); Error("No valid value for material ID (option -c)");
  }

  if (ThicknessO > 0)
    fprintf(LogFilePtr,"Thickness of outer material of window %f cm \n", ThicknessO);
  if (ThicknessI > 0)
    fprintf(LogFilePtr,"Thickness of inner material of window %f cm \n", ThicknessI);
    
  // window frame material from file
  // -------------------------------
  if (KeymaterialO == 0)
  {
    // Read transmission file for window frame
    if (sTransFileNameO !=NULL)
    {
      pTransFileO = fopen(sTransFileNameO,"r");
      if (pTransFileO!=NULL)  
      { 
        i=0;
        while (ReadLine(pTransFileO, sLine, CHAR_BUF_SMALL-1) > 0) 
        { i++;
          sscanf(sLine, "%lf %lf", &WavO[i], &MuO[i]);
        }
        nVal  =i;
        nValFO=nVal;
        fclose(pTransFileO);
	    
        /* check the input data */	    
        for(i = 1; i <= (nValFO-1); i++)
        {
          if (WavO[i+1] <= WavO[i]) 
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in transmission file of the window frame \n");
            fprintf(LogFilePtr,"The wavelength values (1st column) must be in ascending order!!! \n");
            exit(-1);
          }    
        }
      }
      else
      { Error("Transmission file could not be opened");
      }
    }		
    else
    { 
      Error("No file name given describing the transmission of the window frame\n");
    }
  }	

  if (KeymaterialO >= 0 && KeymaterialO < 6) 
    fprintf(LogFilePtr, "Usable wavelength range: %6.2f - %6.2f Ang \n", WavI[1], WavI[nVal]);

  // window pane material from file
  // ------------------------------
	if (sTransFileNameI != NULL) 
	{
	  fprintf(LogFilePtr,"Material transmission characteristics of window pane read from file:  %s \n", sTransFileNameI);
	  KeymaterialI = 0; /* activate this material */

    pTransFileI = fopen(sTransFileNameI,"r");
    if (pTransFileI!=NULL)  
    { i=0;
      while (ReadLine(pTransFileI, sLine, CHAR_BUF_SMALL-1) > 0) 
      { i++;
        sscanf(sLine, "%lf %lf", &WavI[i], &MuI[i]);
      }
      nValFI=i;
      fclose(pTransFileI);
	
      /* check the input data */	    
      for(i = 1; i <= (nValFI-1); i++)
      {
        if (WavI[i+1] <= WavI[i]) 
        {
          fprintf(LogFilePtr,"ERROR: incorrect data in open transmission file of the window \n");
          fprintf(LogFilePtr,"The wavelength values (1st column) must be in ascending order!!! \n");
          exit(-1);
        }    
      }
    }
    else
    { Error("Transmission file could not be opened");
    }
	    
    fprintf(LogFilePtr, "Usable wavelength range: %6.2f - %6.2f Ang \n", WavI[1], WavI[nValFI]);
  }	
  return;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void  SetGeometry(char* sColor, int nHoles)
{
  int j,
      nHolesC=0,    // number or circular holes
      nHolesR=0;    // number of rectangular holes

  // Geometry data
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;
   
    for (j=0; j < nHoles; j++)
    { 
      if (eWinShape[j]==VT_CIRCLE)
        nHolesC++;
      else
        nHolesR++;
    }

    stGeometry.nHolCyls = nHolesC+1;
    stGeometry.pHolCyl  = calloc(nHolesC+1, sizeof(VtHolCyl));
    stGeometry.nHulls   = nHolesR; 
    stGeometry.pHull    = calloc(nHolesR,   sizeof(VtHull));

    // whole plate
    stGeometry.pHolCyl[0].Radius      = OuterRadius;
    stGeometry.pHolCyl[0].InnerRadius = OuterRadius * 0.9;
    stGeometry.pHolCyl[0].Length      = Max(ThicknessO, ThicknessI)/CmprFact;
    stGeometry.pHolCyl[0].vCntr[0]    = (Distance + stGeometry.pHolCyl[0].Length/2.)/CmprFact;
    stGeometry.pHolCyl[0].vCntr[1]    = 0.0;
    stGeometry.pHolCyl[0].vCntr[2]    = 0.0;
    stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
    stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
    stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;

    for (j=0; j < nHoles; j++)
    { 
      if (eWinShape[j]==VT_CIRCLE)
      {
        stGeometry.pHolCyl[0].InnerRadius = winradius[j];
        stGeometry.pHolCyl[0].Radius      = winradius[j] * 1.1;
        stGeometry.pHolCyl[0].Length      = Max(ThicknessO, ThicknessI)/CmprFact;
        stGeometry.pHolCyl[0].vCntr[0]    = (Distance + stGeometry.pHolCyl[0].Length/2.)/CmprFact;
        stGeometry.pHolCyl[0].vCntr[1]    = ywincenter[j];
        stGeometry.pHolCyl[0].vCntr[2]    = zwincenter[j];
        stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
        stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
        stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;
      }
      else
      {
        stGeometry.pHull[0].WidthIn   = winwidth [j];
        stGeometry.pHull[0].WidthOut  = winwidth [j] * 1.1;
        stGeometry.pHull[0].HeightIn  = winheight[j];
        stGeometry.pHull[0].HeightOut = winheight[j] * 1.1;
        stGeometry.pHull[0].Length    = Max(ThicknessO, ThicknessI)/CmprFact;
        stGeometry.pHull[0].vCntr[0]  = (Distance + stGeometry.pHull[0].Length/2.)/CmprFact;
        stGeometry.pHull[0].vCntr[1]  = ywincenter[j];
        stGeometry.pHull[0].vCntr[2]  = zwincenter[j];
        stGeometry.pHull[0].vNormal[0]= 1.0;
        stGeometry.pHull[0].vNormal[1]= 0.0;
        stGeometry.pHull[0].vNormal[2]= 0.0;
      }
    }
  }
  return;
}
