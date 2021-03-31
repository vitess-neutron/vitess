/*********************************************************************************************/
/*  VITESS module 'window'                                                               */
/*                                                                                           */
/* This module simulates an aperture or window of circular or rectangular shape              */
/*   (attenuation inside and non-ideal absorption outside the window can be considered)      */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*       June 1999  D. Wechsler                                                              */
/* 1.00  Feb  2001  S. Manoshin     include of gravity effect                                */
/* 1.01  June 2001  K. Lieutenant   parameter S to simulate a beamstop + SOFTABORT           */
/* 1.02  Jan  2002  K. Lieutenant   reorganisation                                           */
/* 2.00  Jun  2003  S. Manoshin     Add material for window frame                            */
/* 2.10  Mar  2004  S. Manoshin     Add material for inner part of window                    */
/* 2.21  Jul  2004  S. Manoshin     Corrected some bugs for thick window                     */
/* 2.22  Jan  2002  K. Lieutenant   correction:  position after beamstop                     */
/* 2.23  May  2010  A. Houben       "Rotation" of square window by counter rot of neutron pos*/
/* 2.24  Apr  2012  A. Houben       Treat only neutrons with a given color and phi angle     */
/* 2.25  Aug  2019  K. Lieutenant   tidy up and compression option for visualization         */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "bender_inter_data.h"
#include "matrix.h"
#include "message.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);    // reads input parameters and initializes global variables
void  EvalInput();                        // Analyses input parameters and prepares attenuation
void  SetGeometry(char* sColor);          // fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
short  bCircularWindow=TRUE,  // -R  [-]   Flag: kind of window, TRUE: circular, FALSE rectangular  
       bBeamStop=FALSE,       // -S  [-]   Flag: beamstop        TRUE: beamstop, FALSE normal window
       bRemoveOtherCol=TRUE,  // -d  [-]   Flag: Neutrons that are not treated are removed
       TreatColor = -1;       // -f  [-]   Treat only neutrons with this color
double DistMove =0.0;         // -l  [cm]  Distance from origin to the window (along the x-axis) 
double heightmin=0.0,         // -h  [cm]  z-coordinate: bottom of rectangular window            
       heightmax=0.0,         // -H  [cm]  z-coordinate: top of rectangular window               
       widthmin =0.0,         // -w  [cm]  y-coordinate: lower frame value of rectangular window 
       widthmax =0.0,         // -W  [cm]  y-coordinate: higher frame value of rectangular window
       winradius=0.0,         // -r  [cm]  radius of circular window                             
       ywincenter=0.0,        // -y  [cm]  y coordinate: center of window                        
       zwincenter=0.0,        // -z  [cm]  z coordinate: center of window                        
       rotang = 0.0;          // -A  [rad] Rotation angle (input parameter in [deg])
double minPhi=-1.0,           // -p  [deg] min. and 
       maxPhi=370.0;          // -P  [deg] max. angle in y-z-plane
double ThicknessO=0.0,        // -t  [cm]  thickness of the frame material    
       ThicknessI=0.0;        // -T  [cm]  thickness of the pane material      
char	*sTransFileNameO=NULL;  // -C   [-]  file describing the transmission of the window frame material
char	*sTransFileNameI=NULL;  // -m   [-]  file describing the transmission of the material in the open part of window
long   KeymaterialO=6;        // -c   [-]  Window frame material: 0 - from file, 1 - gadolinium, 2 - cadmium,  3 - Bor10,
	                            //                                  4 - Eu,        5 - Silicon,    6 - ideal absorber

// Variables determined from input parameters or trajectory data
long   KeymaterialI=1;        //           Window pane material:  0 - from file  1 - no(default) 
FILE	*pTransFileO=NULL;      //           pointer to window frame transmission file 
FILE  *pTransFileI=NULL;      //           pointer to window pane transmission file 
double WavO[MAX_MU],          //           wavelength and attenuation values of the frame material 
       MuO [MAX_MU],          
       WavI[MAX_MU],          //           wavelength and attenuation values of the pane material 
       MuI [MAX_MU];          
long   nValFO=0;              //           number of attenuation values in file (for window frame material)
long   nValFI=0;              //           number of attenuation values in file (for window pane material)
Plane  EndPoint,              //           Planes through window for zero thickness and
       EndPointO,             //             end of the Outer and end of the Inner wall
       EndPointI;             //           EndPoint.D: distance to window along x-axis         [cm]  


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
	short   bOutOfWindow=FALSE;
	long    i=0;               // index of trajectories

	double  DistSquared=0.0;   // distance from point of hitting to center of window   
	double  TimeOF,            // TOF of neutron from origin to window
          TOF3;              // TOF of neutron to pass through window material
	double  NewPositionY=0.0,  // neutron position in co-ordinate system  rotated by 'rotang'
          NewPositionZ=0.0, 
          Phi=0.0;           // phi angle of the current trajectory
	double  CenterX=0.0,       // center of beam at window
          CenterY=0.0, 
          CenterZ=0.0, 
          SumProb=0.0;
  double  VelocityReal=0.0,  // velocity of the current neutron
          N_Wavelength,      // wavelength of the current neutron
          mu=0.0,            // attenuation coefficient of the material, that the neutron traverses
          prob=0.0;          // probability of traversing the material

  Neutron Output;            // trajectory as it is written to the output

  InitNeutron(&Output);

  // Initialisation
  // --------------
	_eModule=MCN_WINDOW;

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "2.25");
  OwnInit(argc, argv);
  MsgInit();
  EvalInput();

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bLengthCmpr = TRUE;
  
	DECLARE_ABORT

  // Loop over all trajectories
  // --------------------------
  while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK
      
      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        // Remove neutrons with wrong color
        // --------------------------------
        if ((TreatColor >= 0) && (InputNeutrons[i].Color != TreatColor)) 
        {
          Output = InputNeutrons[i];
          if (!bRemoveOtherCol) 
          {
    	      WriteIAP(&Output, VT_EXITED);
    	      WriteNeutron(&Output);
          }
          continue;
        }
			
        // Remove neutrons with wrong direction ro wavelength
        // --------------------------------------------------
			  if (bOldFrame==FALSE)
        {
			    if (InputNeutrons[i].Vector[0] <= 0.0) continue;
			    if (InputNeutrons[i].Wavelength == 0.0) continue;
			    VelocityReal = (double)(V_FROM_LAMBDA(InputNeutrons[i].Wavelength));
			    if (VelocityReal <= 0.0) continue;
			  } 
			  else 
        {
			    Output = InputNeutrons[i];			
			  }
			
        // Write intersection point
        // ------------------------
			  WriteIAP(&InputNeutrons[i], VT_ENTERED);

        // 	Move neutron to window with or without gravity effect and calculate Time of Flight (ms)
        // ----------------------------------------------------------------------------------------
			  if (keygrav == 1)
			    TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], EndPoint);
			  else
				  TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], EndPoint);
      
			  // Calculate average position
        // --------------------------
			  InputNeutrons[i].Time += (double)TimeOF;
			  CenterX  += InputNeutrons[i].Probability*InputNeutrons[i].Position[0];
			  CenterY  += InputNeutrons[i].Probability*InputNeutrons[i].Position[1];
			  CenterZ  += InputNeutrons[i].Probability*InputNeutrons[i].Position[2];
			  SumProb  += InputNeutrons[i].Probability;
			  TOF3 = 0.0;

			  // window test
        // -----------
        if (rotang != 0.0) 
        {   /*x' = x cos f - y sin f
			        y' = y cos f + x sin f */
				  NewPositionY = InputNeutrons[i].Position[1] * cos(-rotang) - InputNeutrons[i].Position[2] * sin(-rotang);
				  NewPositionZ = InputNeutrons[i].Position[2] * cos(-rotang) + InputNeutrons[i].Position[1] * sin(-rotang);
			  } 
        else
        {
				  NewPositionY = InputNeutrons[i].Position[1];
				  NewPositionZ = InputNeutrons[i].Position[2];
			  }

        if (minPhi >= 0.0 && maxPhi <= 360.0) 
        { 
          Phi	= (double)atan2(InputNeutrons[i].Vector[1], InputNeutrons[i].Vector[2])*180.0/M_PI+180.;
          if (Phi < minPhi || Phi > maxPhi)
          {
	          WriteIAP(&InputNeutrons[i], VT_ABSORBED);
	          continue;
	        }
        }

        // Test if window is hit
        if (bCircularWindow==TRUE)
        {	DistSquared =  (NewPositionY - ywincenter)*(NewPositionY - ywincenter)
                       + (NewPositionZ - zwincenter)*(NewPositionZ - zwincenter);
          if (winradius*winradius < DistSquared)
            bOutOfWindow=TRUE;
          else
          bOutOfWindow=FALSE;
        }
        else
        {	if ((widthmin  > NewPositionY) || (widthmax < NewPositionY) ||
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
            TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPointI);
          }
          else
          {
            TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPointI);
          }

          if (KeymaterialI == 0)
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
            InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
          }

          if (bOldFrame==FALSE) 
          {
            WriteIAP(&InputNeutrons[i], VT_EXITED);
            InputNeutrons[i].Position[0]=0.0;
            InputNeutrons[i].Time += (double)TOF3 ;
            Output = InputNeutrons[i];
          } 
          else 
          {
            InputNeutrons[i]=Output;
            WriteIAP(&InputNeutrons[i], VT_EXITED);
          }
          WriteNeutron(&Output);
        }
        else /* else, if hitting beamstop or out of window */
        {
          if (KeymaterialO != 6)
          {
            if (keygrav == 1)
            {
              TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPointO);
            }
            else
            {
              TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPointO);
            }

            /* Attenuation during pass through window material */
            N_Wavelength = InputNeutrons[i].Wavelength;
            mu = Interpolation(N_Wavelength,KeymaterialO,WavO,MuO,nValFO);
            if (mu == -10000.0)
            { // sprintf(sBuffer, "Attenuation coefficient of window frame material could not be determined for wavelength %6.3f Ang", N_Wavelength);
              // Error(sBuffer);
              CountMessageID(WNDO_L_RANGE_TOO_SMALL, InputNeutrons[i].ID);
            }
            prob = exp(-mu*TOF3*VelocityReal);
            InputNeutrons[i].Probability = InputNeutrons[i].Probability*prob;
            InputNeutrons[i].Time += TOF3;
            InputNeutrons[i].Position[0]=DistMove;
            Output = InputNeutrons[i];
            if (InputNeutrons[i].Probability <= wei_min) 
            {
              WriteIAP(&Output, VT_ABSORBED);
            }
            else 
            {
              WriteIAP(&Output, VT_EXITED);
              WriteNeutron(&Output);
            }
          }
          else 
          {
            if (keygrav == 1)
            {
              TOF3 = NeutronPlaneIntersectionGrav(&InputNeutrons[i] , EndPointO);
            }
            else
            {
              TOF3 = NeutronPlaneIntersection1(&InputNeutrons[i] , EndPointO);
            }
            InputNeutrons[i].Probability = 0.;
            InputNeutrons[i].Position[0]=DistMove;
            InputNeutrons[i].Time += TOF3;
            WriteIAP(&InputNeutrons[i], VT_ABSORBED);
          }
        }
      }
    }
  }

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
my_exit:
	if (bCircularWindow)
	  fprintf(LogFilePtr,"Window of %6.2f cm diameter in a distance of %7.2f cm \n", 2.0*winradius, DistMove);
	else
	  fprintf(LogFilePtr,"Window of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", 
	                     widthmax-widthmin, heightmax-heightmin, DistMove);
  PrintMessage(WNDI_L_RANGE_TOO_SMALL, sTransFileNameI, ON);
  PrintMessage(WNDO_L_RANGE_TOO_SMALL, sTransFileNameO, ON);

	if (SumProb != 0.0)
	{ CenterX   = CenterX/SumProb;
		CenterY   = CenterY/SumProb;
		CenterZ   = CenterZ/SumProb;
		fprintf(LogFilePtr,"Center of beam: X = %f cm Y = %f cm Z = %f cm \n",CenterX, CenterY, CenterZ);
	}
	else
	{ fprintf(LogFilePtr,"No neutrons at the exit of this module \n");
	}

  if (TreatColor >= 0) fprintf(LogFilePtr,"Only neutrons with color %hd are treated \n", TreatColor);

	fprintf(LogFilePtr," \n");

  // fills the structure stGeometry for visualization
	SetGeometry("blue");

  Cleanup(Max(ThicknessO, ThicknessI)+DistMove, 0.0, 0.0, 0.0, 0.0);

	return(0);
}


/**************************************************************/
/** Reads input parameters and sets them as global variables **/
/**************************************************************/
void  OwnInit(int argc, char *argv[])
{
	int   i=0, j=0;
	short bOFrame=FALSE; /* default for window 'new frame' */

  // initialize
	bOldFrame = -1;      /* no default for frame in general */

  InitPlane(&EndPoint);
  InitPlane(&EndPointI);
  InitPlane(&EndPointO);

  for(j=0; j<MAX_MU; j++)
	{	
		WavO[j] = 0.0;
		MuO [j] = 0.0;
		WavI[j] = 0.0;
		MuI [j] = 0.0;
	}

  // read parameters
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
				/* in cm, outer material */
				break;
			case 'T':
				ThicknessI = atof(&argv[i][2]);
				/* in cm, inner material */
				break;
      case 'f':
				sscanf(&(argv[i][2]),"%hd", &TreatColor);
				break;
      case 'd':
				sscanf(&(argv[i][2]),"%hd", &bRemoveOtherCol);
				break;
      case 'p':
				minPhi = atof(&argv[i][2]);
				/* in deg, min angle in yz plane */
				break;
      case 'P':
				maxPhi = atof(&argv[i][2]);
        if (maxPhi < 0.0)
          maxPhi = 370.0;
				/* in deg, max angle in yz plane */
				break;

			default:
				fprintf(LogFilePtr,"ERROR: unknown commandline option: %s\n",argv[i]);
				exit(-1);
				break;
			}
		}
	}

	// take default value for frame, if not explicitely set
	if (bOldFrame==-1)
		bOldFrame=bOFrame;

  if (maxPhi < minPhi)
    Error("Maximal phi angle must not be smaller than minimal phi angle");

  // Fill structures defining the planes
	EndPoint.D  = -1.0 * DistMove;
	EndPointI.D = -1.0 *(DistMove + ThicknessI);
	EndPointO.D = -1.0 *(DistMove + ThicknessO);

  return;
}


/********************************************************/
/** Analyses input parameters and prepares attenuation **/
/********************************************************/
void EvalInput()
{
  char sLine[CHAR_BUF_SMALL]="";
	long i=0,                        // index of arrays for wavelength and attenuation 
	     nVal=0;                   // number of wavelength and attenuation values in the array

  // Checks
  // ------
	if (DistMove < 0.0 && bOldFrame == FALSE)
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
		fprintf(LogFilePtr,"WARNING: It is recommeded to have outer and inner thickness EQUAL! \n");
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
  { case 0: fprintf(LogFilePtr, "Transmission characteristics read from file %s\n", sTransFileNameO); break;
    case 1: fprintf(LogFilePtr, "Gadolinium \n"); Gadolinium(WavO, MuO, &nVal); break;
    case 2: fprintf(LogFilePtr, "Cadmium    \n"); Cadmium   (WavO, MuO, &nVal); break;
    case 3: fprintf(LogFilePtr, "Bor10      \n"); Bor10     (WavO, MuO, &nVal); break;
    case 4: fprintf(LogFilePtr, "Eu         \n"); Eu        (WavO, MuO, &nVal); break;
    case 5: fprintf(LogFilePtr, "Silicon    \n"); Silicon   (WavO, MuO, &nVal); break;
    case 6: fprintf(LogFilePtr, "Ideal absorber \n");                           break;
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
      pTransFileO = OpenInputFile(sTransFileNameO, FALSE, "r");
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
          if (WavO[i+1] < WavO[i])
          {
            fprintf(LogFilePtr,"ERROR: incorrect data in the transmission file '%s' of the window frame (outer window)\n", sTransFileNameO);
            fprintf(LogFilePtr,"The wavelength values must be in increasing order! \n");
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

    pTransFileI = OpenInputFile(sTransFileNameI,FALSE, "r");
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
        if (WavI[i+1] < WavI[i]) 
        {
          fprintf(LogFilePtr,"ERROR: incorrect data in the transmission file0 '%s' of the window pane (inner window)\n", sTransFileNameI);
          fprintf(LogFilePtr,"The wavelength values must be in increasing order! \n");
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
void SetGeometry(char* sColor)
{
  
 // Geometry data
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    if (bCircularWindow) 
    {  
      stGeometry.pHolCyl = calloc(1, sizeof(VtHolCyl));
      stGeometry.nHolCyls = 1;

      stGeometry.pHolCyl[0].InnerRadius = winradius;
      stGeometry.pHolCyl[0].Radius      = winradius * 3.0;
      stGeometry.pHolCyl[0].Length      = Max(ThicknessO, ThicknessI)/CmprFact;
      stGeometry.pHolCyl[0].vCntr[0]    = (DistMove + stGeometry.pHolCyl[0].Length/2.)/CmprFact;
      stGeometry.pHolCyl[0].vCntr[1]    = ywincenter;
      stGeometry.pHolCyl[0].vCntr[2]    = zwincenter;
      stGeometry.pHolCyl[0].vSymAxis[0] = 1.0;
      stGeometry.pHolCyl[0].vSymAxis[1] = 0.0;
      stGeometry.pHolCyl[0].vSymAxis[2] = 0.0;
    }
    else 
    {
      ywincenter = (widthmax  + widthmin) /2.0;
      zwincenter = (heightmax + heightmin)/2.0;

      stGeometry.pHull = calloc(1, sizeof(VtHull));
      stGeometry.nHulls = 1; 

      stGeometry.pHull[0].Length    = Max(ThicknessO, ThicknessI)/CmprFact;
      stGeometry.pHull[0].WidthIn   = (widthmax  - widthmin);
      stGeometry.pHull[0].WidthOut  = (widthmax  - widthmin) * 3.0;
      stGeometry.pHull[0].HeightIn  = (heightmax - heightmin);
      stGeometry.pHull[0].HeightOut = (heightmax - heightmin) * 3.0;
      stGeometry.pHull[0].vCntr[0]  = (DistMove + stGeometry.pHull[0].Length/2.)/CmprFact;
      stGeometry.pHull[0].vCntr[1]  = ywincenter;
      stGeometry.pHull[0].vCntr[2]  = zwincenter;
      stGeometry.pHull[0].vNormal[0]= 1.0;
      stGeometry.pHull[0].vNormal[1]= 0.0;
      stGeometry.pHull[0].vNormal[2]= 0.0;
      stGeometry.pHull[0].rotAngle  = rotang * 180.0/M_PI;

      // FillRotMatrixZY(rotMatrixPi2, 0, M_PI_2);
      // RotVector(rotMatrixPi2, stGeometry.pCuboid[0].vNormal);
    }
  }
}
