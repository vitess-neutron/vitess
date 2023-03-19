/*********************************************************************************************/
/*  VITESS module 'beamstop'                                                                 */
/*                                                                                           */
/* This module simulates a beamstop of circular or rectangular shape                         */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Apr 2011  K. Lieutenant   initial version                                            */
/* 1.1  Jul 2019  K. Lieutenant   length compression for visualization                       */
/* 1.2  Jul 2021  K. Lieutenant   general changes for VITESS 3.5                             */
/* 1.3  Mar 2023  K. Lieutenant   function and visualization corrected                       */
/*********************************************************************************************/

#include "convert.h"
#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "convert.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and sets global parameters
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
VtShape eShape=FALSE;          // -R  [cm]   criterion: shape of window, 'circular' or 'rectangular' 
short   bProp=FALSE;           // -p  [cm]   criterion: propagate to beamstop  0: no,  1: yes
double  Width   =0.0,          // -W  [cm]   width of a rectangular beamstop 
        Height  =0.0,          // -H  [cm]   height of a rectangular beamstop 
        Radius  =0.0,          // -r  [cm]   radius of a circular beamstop  
        DistMove=0.0;          // -d  [cm]   distance between starting point and beamstop 
double  CenterY =0.0,          //            fixed
        CenterZ =0.0;          //            center of the beamstop position

// Variables determined from input parameters or trajectory data
short   bOnBeamstop=FALSE;     // criterion: beamstop hit or not 
Plane   Endpoint;              // vertical Plane through the position of the beamstop
                               //  (Endpoint.D = distance to end of free flight path along x-axis [cm]) 
double  VelocityReal=0.0;      // velocity of the neutron 


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  long    i=0;                      // index of trajectories
  double  DistCenter=0.0,           // distance between center of beamstop and point of striking of the neutron
          TimeOF =0.0,              // time of flight of the neutron to the window
          NewPosY=0.0, NewPosZ=0.0; // hor. and vert. position of neutron at slit
  Neutron TestNeutron;

  // Initialisation
  // --------------
  InitNeutron(&TestNeutron);

  _eModule=MCN_BEAMSTOP;

  Init(argc,argv, _eModule);
	PrintModuleName(_eModule, "1.2");
  OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = TRUE;

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
        if (InputNeutrons[i].Wavelength == 0.0) continue;
        VelocityReal = V_FROM_LAMBDA(InputNeutrons[i].Wavelength); 
        if (VelocityReal <= 0.0) continue;

        // 	Check if neutron would hit the beamstop 
        TestNeutron=InputNeutrons[i]; //memcpy(&TestNeutron, &InputNeutrons[i], sizeof(Neutron));

        if (keygrav == 1)
        {
          TimeOF = NeutronPlaneIntersectionGrav(&TestNeutron, Endpoint);
        }
        else
        {
          TimeOF = NeutronPlaneIntersection1(&TestNeutron, Endpoint);
        }

        /* If plane through beamstop surface is reached:                   */
        /*  Calculate position on beamstop  and  decide if beamstop is hit */
        if (TimeOF >= 0.0)
        { NewPosY = TestNeutron.Position[1];
          NewPosZ = TestNeutron.Position[2];

          if(eShape==VT_CIRCLE)
          {	
            DistCenter = sqrt(sq(NewPosY - CenterY) + sq(NewPosZ - CenterZ));
            if (DistCenter <= Radius)
              bOnBeamstop=TRUE;
            else
              bOnBeamstop=FALSE;
          }
          else if (eShape==VT_SQUARE)
          { 
            if (fabs(NewPosY - CenterY) <= 0.5*Width  &&  fabs(NewPosZ - CenterZ) <= 0.5*Height)
              bOnBeamstop=TRUE;
			      else
				      bOnBeamstop=FALSE;
          }
          else
          { Error("Unknown beamstop shape");
          }
        }
        else
        { bOnBeamstop=FALSE;
        }

        /* if beamstop is hit   : writeout new data set for 'progation'=yes */
        /* if beamstop is missed: writeout input data for ne */
        if (bOnBeamstop)
        { 
          if (bProp)
          { WriteNeutron(&InputNeutrons[i]);
            WriteIAP(&TestNeutron, VT_ABSORBED);
          }
        }
        else
        { WriteNeutron(&InputNeutrons[i]);
        }
      }
    }
  }	

  // Finish: print parameters, write geometry and instrument file, free memory
  // -------------------------------------------------------------------------
my_exit:
  if (eShape==VT_CIRCLE)
    fprintf(LogFilePtr, "Beamstop of %6.2f cm diameter in a distance of %7.2f cm \n",             2.0*Radius, DistMove);
  else if (eShape==VT_SQUARE) 
    fprintf(LogFilePtr, "Beamstop of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", Width, Height, DistMove);
  else
    Error("Unknown beamstop shape");
  
  SetGeometry("blue");
  Cleanup(DistMove,0.0,0.0, 0.0,0.0);	

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global parameters **/
/*******************************************************/
void  OwnInit(int argc, char* argv[])
{
	int i;
  char* arg;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '+')
		{
      arg=&argv[i][2];
			switch (argv[i][1])
			{
			  case 'p':
				  bProp = atoi(&argv[i][2]);        // criterion: propagate to beamstop  0: no,  1: yes
				  if (bProp == FALSE)
					  bOldFrame = TRUE;
				  break;
			  case 'R':
          if (strlen(arg) > 1)
				    eShape = Shape_Txt2ID(arg);     // text given   VITESS 4 
          else
            eShape = (VtShape) atoi(arg);   // ID given     VITESS 3
				  break;

			  case 'd':
				  DistMove = atof(&argv[i][2]);
				  break;

			  case 'r':
				  Radius = atof(&argv[i][2]);
				  break;

			  case 'W':
				  Width = atof(&argv[i][2]);
				  break;
			  case 'H':
				  Height = atof(&argv[i][2]);
				  break;

			  default:
				  fprintf(LogFilePtr, "ERROR: unknown command option: %s\n", argv[i]);
				  exit(-1);
				  break;
			}
		}
	}

	Endpoint.A =  1.0;
	Endpoint.B =  0.0;
	Endpoint.C =  0.0;
	Endpoint.D = -1.0 * DistMove;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the beamstop geometry
  if (bVisInstr)
  { 
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    if (eShape==VT_CIRCLE) 
    {
      stGeometry.pCircle =calloc(1, sizeof(VtCircle));
      stGeometry.nCircles=1; 
      
      stGeometry.pCircle[0].Radius    = BlowUp * Radius;
      stGeometry.pCircle[0].AngleBeg  = 0;
      stGeometry.pCircle[0].AngleEnd  = 359.99;
      stGeometry.pCircle[0].vCntr[0]  = DistMove;
      stGeometry.pCircle[0].vCntr[1]  = 0.0;
      stGeometry.pCircle[0].vCntr[2]  = 0.0;
      stGeometry.pCircle[0].vNormal[0]= 1.0;
      stGeometry.pCircle[0].vNormal[1]= 0.0;
      stGeometry.pCircle[0].vNormal[2]= 0.0;
    }
    else if (eShape==VT_SQUARE) 
    {
      stGeometry.pRectangle =calloc(1, sizeof(VtRectangle));
      stGeometry.nRectangles=1; 
      
      stGeometry.pRectangle[0].Width     = BlowUp * Width;
      stGeometry.pRectangle[0].Height    = BlowUp * Height;
      stGeometry.pRectangle[0].rotAngle  = 90.0;
      stGeometry.pRectangle[0].vCntr[0]  = DistMove;
      stGeometry.pRectangle[0].vCntr[1]  = 0.0;
      stGeometry.pRectangle[0].vCntr[2]  = 0.0;
      stGeometry.pRectangle[0].vNormal[0]= 1.0;
      stGeometry.pRectangle[0].vNormal[1]= 0.0;
      stGeometry.pRectangle[0].vNormal[2]= 0.0;
    }
  }

  return;
}
