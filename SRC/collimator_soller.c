
/********************************************************************************************/
/*  VITESS module 'collimator_virtual'                                                      */
/*                                                                                          */
/* This module simulates a filter acting on neutrons like a radial or Soller collimator     */
/*   depending on the parameter 'angular collimation', it does not propagate the neutrons   */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Sep  1999  D. Wechsler    initial version                                           */
/* 1.1  Jan  2004  K. Lieutenant  changes for 'instrument.dat'                              */
/* 1.2  Aug  2019  K. Lieutenant  subroutines and visualisation included                    */
/* 1.3  Aug  2023  K. Lieutenant  visualisation completed                                   */
/********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "softabort.h"
#include "matrix.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
// #define MAX_ANG        10000   //       maximal number of collimation centers
#define STD_COLL_DIST     50   // [cm]  virtual radius of the radial collimator for visualization 
#define STD_COLL_THICK     5   // [cm]  virtual thickness of the radial collimator for visualization 
#define STD_COLL_HEIGHT   10   // [cm]  virtual height of the collimator for visualization 
#define STD_COLL_WIDTH    10   // [cm]  virtual width of the collimator for visualization 
#define STD_COLL_LENGTH   20   // [cm]  virtual length of the collimator for visualization 
#define MAX_CHANNELS      20.0


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit    (int argc, char *argv[]);       // reads input parameters and sets global parameters
void  SetGeometry(char* sColor);                 // fills the structure stGeometry for visualization


/******************************/
/** Global variables         **/
/******************************/
// Input parameters
short   bAngColl=FALSE;      // -k         flag: angular collimation
long    nAngles =1;          // -n         number of collimation channels = Angle grid
double  PeakTransm=1.0,      // -e         maximal probability for passing through the collimator (considers effectively the blocking due to the width of collimator blades
	      HorCollDiv=0.0,      // -d  [deg]  allowed divergence FWHM; always with respect to y-direction
	      AngSpacing=0.0,      // -a  [deg]  angular distance between two collimation centers
        AngleMin  =0.0;      // -m  [deg]  minimum of angle range 

// Variables determined from input parameters or trajectory data
double  AngleMax  =0.0;      //     [deg]  maximum of angle range 
double* Angle;               //     [deg] (pointer to) array of angles of maximal transition


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
	long	  i=0,               //       index of trajectories
          j=0;               //       index of angles
	short   bTransmit=FALSE;   //       flag: neutron has passed through collimator
	double  CollimProb=0.0,    //       transmission probability 
          HorDiv=0.0;        // [deg] horizontal divergence of the neutron trajectory
	Neutron Output;            //       trajectory written to the output (to be read by the next module)

  InitNeutron(&Output);

	// Reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_COLL_VIRT;

  Init(argc, argv, _eModule);
  if (bAngColl)
	  print_module_name("Virtual angular Collimator 1.3");
  else
	  print_module_name("Virtual Soller Collimator 1.3");
  OwnInit(argc,argv);


  // array of angles of maximal transition:
  // allocate memory, initialize, fill array
  // ---------------------------------------
  Angle  = (double*) calloc(nAngles, sizeof(double));
  for (j=0; j < nAngles; j++) 
    Angle[j]=0.0;

	if (bAngColl==TRUE && nAngles > 1) // case radial collimator	
	{ 
    Angle[0] = AngleMin;
		for(j=1; j < nAngles; j++)
			Angle[j] = Angle[j-1] + 2.0*HorCollDiv + AngSpacing;
  
    AngleMax = Angle[nAngles - 1];
	}
	else // case linear collimator in beamline direction (angle=0.0)
	{	
    nAngles  = 1; 
    AngleMax = AngleMin;

    if (bVisInstr) bBlowUp = TRUE;
	}

  bVisInstalled = TRUE;

	DECLARE_ABORT;

	// Loop over trajectories
  // ----------------------
	while (ReadNeutrons()!= 0)
	{
		CHECK;
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK;

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
			  HorDiv = Degrees(atan2(InputNeutrons[i].Vector[1],InputNeutrons[i].Vector[0]));

			  bTransmit=FALSE;

			  for(j=0;j<nAngles; j++)
			  {
				  if (fabs(Angle[j]-HorDiv) < HorCollDiv)
				  { 	
            CollimProb = PeakTransm*(1.0 - (fabs(Angle[j]-HorDiv) / HorCollDiv));
					  bTransmit=TRUE;
					  break;
				  }
			  }

			  if (bTransmit) 
			  {	
  			  Output = InputNeutrons[i];
          Output.Probability *= CollimProb;
          WriteIAP(&Output, VT_PASSED);
			  }
			  else 
			  {	
          WriteIAP(&InputNeutrons[i], VT_ABSORBED);
          continue;
			  }

			  WriteNeutron(&Output);
      }
		}
	}

// Finish: print parameters, write geometry and instrument file, free memory
// -------------------------------------------------------------------------
my_exit:
  if (bAngColl)
    fprintf(LogFilePtr, "Horizontal angular collimation using %ld centers of %6.2f deg FWHM from %6.2f to %6.2f deg \n", 
                        nAngles, HorCollDiv, AngleMin, AngleMax);
  else
    fprintf(LogFilePtr, "Soller collimator along the beam axis of %6.2f deg FWHM\n", HorCollDiv);

  SetGeometry("blue");

  Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return(0);
}


/***********************************************************************************/
/* OwnInit:                                                                        */
/* This routine reads the parameter values and checks them                         */
/***********************************************************************************/
void OwnInit   (int argc, char *argv[])
{
  long  i;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') {

			switch(argv[i][1])
			{
				case 'k':
					bAngColl = atol(&argv[i][2]); 	/* bAngColl =1 ==> angular collimation is activated */
					break;

				case 'd':
					HorCollDiv = atof(&argv[i][2]); /* allowed divergence FWHM[deg]; always with respect to y-direction*/
					break;

				case 'e':
					PeakTransm = atof(&argv[i][2]); /* maximal probability for passing through the collimator (considers effectively the blocking due to width of collimator spacers */
					break;

				case 'm': 
					AngleMin = atof(&argv[i][2]);   /* minimum of angle range [deg] */
					break;

				case 'n': 
					nAngles = atol(&argv[i][2]);    /* number of collimation channels = Angle grid */
					break;

				case 'a':
					AngSpacing = atof(&argv[i][2]); /* angular distance between collimation channels, only needed for the case of angular collimation [deg]*/
					break;

				default:
				  fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
				  exit(-1);
				  break;
			}
		}
	}
  return;
}


/***********************************************************************************/
/* SetGeometry:                                                                    */
/*  fills the structure stGeometry for visualization                               */
/***********************************************************************************/
void SetGeometry(char* sColor)
{
 // Geometry data
	if (bVisInstr)
  {
    int    j=0, k=0, l=0,          //        indices of collimation centers, blades per collimation center and the combination
           nBlades=4,              //        number of blades per radial collimation center
           nChannels=1;            //        number of channels in the Soller collimator
    double ry =0.0, rz=0.0,        // [rad]  directions of the incoming beam
           Xi =0.0,                // [deg]  direction to the center of the collimator
           radius=STD_COLL_DIST,   // [cm]   virtual radius of the radial collimator for visualization
           length=STD_COLL_THICK,  // [cm]   virtual length of the collimator for visualization
           dist  =1.0;             // [cm]   channel width = blade distance for the linear collimator

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    if (AngleMax > AngleMin)
    { 
      nBlades=4;
      stGeometry.nCylSlices  = 2; 
      stGeometry.pCylSlice   = (VtCylSlice*) calloc(stGeometry.nCylSlices, sizeof(VtCylSlice));
      stGeometry.nRectangles = nBlades*nAngles; 
      stGeometry.pRectangle  = (VtRectangle*) calloc(stGeometry.nRectangles, sizeof(VtRectangle));

      // get direction to the center of the collimator
      RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
      if (rz < 0) rz += 2.*M_PI;

      Xi = 0.5*(AngleMax+AngleMin) + rz/M_PI*180.0;
	      
      stGeometry.pCylSlice[0].Radius     =  radius-0.5*length; 
      stGeometry.pCylSlice[0].Width      = (radius-0.5*length)*(AngleMax-AngleMin)/180.0*M_PI;
      stGeometry.pCylSlice[0].Height     = STD_COLL_HEIGHT;
      stGeometry.pCylSlice[0].vCntr[0]   = 0.0;
      stGeometry.pCylSlice[0].vCntr[1]   = 0.0;
      stGeometry.pCylSlice[0].vCntr[2]   = 0.0;
      stGeometry.pCylSlice[0].vSymAxis[0]= 0;
      stGeometry.pCylSlice[0].vSymAxis[1]= 0;
      stGeometry.pCylSlice[0].vSymAxis[2]= 1;
      stGeometry.pCylSlice[0].OpenAngle  = AngleMax-AngleMin;
      stGeometry.pCylSlice[0].Phi        = Xi;

      stGeometry.pCylSlice[1].Radius     =  radius+0.5*length; 
      stGeometry.pCylSlice[1].Width      = (radius+0.5*length)*(AngleMax-AngleMin)/180.0*M_PI;
      stGeometry.pCylSlice[1].Height     = STD_COLL_HEIGHT;
      stGeometry.pCylSlice[1].vCntr[0]   = 0.0;
      stGeometry.pCylSlice[1].vCntr[1]   = 0.0;
      stGeometry.pCylSlice[1].vCntr[2]   = 0.0;
      stGeometry.pCylSlice[1].vSymAxis[0]= 0;
      stGeometry.pCylSlice[1].vSymAxis[1]= 0;
      stGeometry.pCylSlice[1].vSymAxis[2]= 1;
      stGeometry.pCylSlice[1].OpenAngle  = AngleMax-AngleMin;
      stGeometry.pCylSlice[1].Phi        = Xi;

      for (j=0; j < nAngles; j++)
      { 
        for (k=0; k < nBlades; k++)
        { 
          l = nBlades*j + k; 
          Xi = Angle[j] + rz/M_PI*180.0 + HorCollDiv*(2*k-(nBlades-1))/(nBlades-1);

          stGeometry.pRectangle[l].Width     = length;
          stGeometry.pRectangle[l].rotAngle  = 0.0;
          stGeometry.pRectangle[l].Height    = STD_COLL_HEIGHT;
          stGeometry.pRectangle[l].vCntr[0]  = radius*cos(Radians(Xi));
          stGeometry.pRectangle[l].vCntr[1]  = radius*sin(Radians(Xi));
          stGeometry.pRectangle[l].vCntr[2]  = 0.0;
          stGeometry.pRectangle[l].vNormal[0]= -sin(Radians(Xi));
          stGeometry.pRectangle[l].vNormal[1]=  cos(Radians(Xi));
          stGeometry.pRectangle[l].vNormal[2]= 0.0;
        }
      }
    }
    else
    {
      stGeometry.nHulls      = 1; 
      stGeometry.pHull       = (VtHull*) calloc(stGeometry.nHulls, sizeof(VtHull));
      stGeometry.pHull[0].WidthIn   = STD_COLL_WIDTH;
      stGeometry.pHull[0].WidthOut  = STD_COLL_WIDTH;
      stGeometry.pHull[0].HeightIn  = STD_COLL_HEIGHT;
      stGeometry.pHull[0].HeightOut = STD_COLL_HEIGHT;
      stGeometry.pHull[0].Length    = STD_COLL_LENGTH;
      stGeometry.pHull[0].vCntr[0]  = 0.0;
      stGeometry.pHull[0].vCntr[1]  = 0.0;
      stGeometry.pHull[0].vCntr[2]  = 0.0;
      stGeometry.pHull[0].vNormal[0]= 1.0;
      stGeometry.pHull[0].vNormal[1]= 0.0;
      stGeometry.pHull[0].vNormal[2]= 0.0;
      stGeometry.pHull[0].rotAngle  = 0.0;

      dist = stGeometry.pHull[0].Length * tan(Radians(HorCollDiv));
      if (dist < stGeometry.pHull[0].WidthIn/MAX_CHANNELS)
        dist = stGeometry.pHull[0].WidthIn/MAX_CHANNELS;
      nChannels = (int)(Round(stGeometry.pHull[0].WidthIn/dist));
      stGeometry.nRectangles = nChannels+1; 
      stGeometry.pRectangle  = (VtRectangle*) calloc(stGeometry.nRectangles, sizeof(VtRectangle));
      for (k=0; k <= nChannels; k++)
      { 
        stGeometry.pRectangle[k].Width     = STD_COLL_LENGTH;
        stGeometry.pRectangle[k].Height    = STD_COLL_HEIGHT;
        stGeometry.pRectangle[k].rotAngle  = 0.0;
        stGeometry.pRectangle[k].vCntr[0]  = 0.0;
        stGeometry.pRectangle[k].vCntr[1]  =(k - 0.5*nChannels)*dist;
        stGeometry.pRectangle[k].vCntr[2]  = 0.0;
        stGeometry.pRectangle[k].vNormal[0]= 0.0;
        stGeometry.pRectangle[k].vNormal[1]= 1.0;
        stGeometry.pRectangle[k].vNormal[2]= 0.0;
      }
    }
  }
}