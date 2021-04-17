/*********************************************************************************************/
/*  VITESS module 'radial collimator'                                                        */
/*                                                                                           */
/* This module simulates a radial, oscillating collimator with absorbing channels            */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  May 2008  K. Lieutenant   initial version                                            */
/* 1.1a Nov 2012  K. Lieutenant   visualization, part 1                                      */
/* 1.2  Jul 2019  K. Lieutenant   completion of visualization (no length compression)        */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "matrix.h"
#include "intersection.h"


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit(int argc, char *argv[]);                                                                          // Reads input parameters and initializes global variables
short  AdvanceToCylinderSurface(double* pTime, Neutron* pNeutron, double Distance, double Height, double Speed); // calculates the flight time to the entrance or exit wall and the point of impact
void   SetGeometry(char* sColor);                                                                                        // fills the structure stGeometry for visualization

int    DetermineChannel(double Angle, double AngleMin, double AngleSep, double Radius);                          // determines the channel as a function of horizontal position


/******************************/
/** Global Variables         **/
/******************************/
VtOscill  _eOscColl=VT_OSC_OFF;  /* option: oscillating collimator  
                                      0: no             
                                      1: yes, but only random phase   */
long   nChannels=1;           /* number of collimator channels                        */

double // frequency =0.0,        /* oscillation frequency of the collimator              */
       // phase     =0.0,        /* phase of the oscillation at t=0                      */
       Distance  =0.0,        /* distance origin - beginning of the collimator in cm  */
       Length    =0.0,        /* length of the collimator in cm                       */
       EntrHeight=0.0,        /* entrance height of the collimator in deg             */
       ExitHeight=0.0,        /* exit height of the collimator in deg                 */
       AngCentre =0.0,        /* direction to the center of the collimator in deg     */
       AngWidth  =0.0,        /* angular width of the collimator in deg               */
       OscWidth  =0.0,        /* angular width of the oscillation in deg              */
       BladeWidth=0.0,        /* thickness of a blade (separating 2 channels)         */
       AngSep    =0.0,        /* angular separation of the collimator channels in deg */
       AngDead   =0.0,        /* angle covered by one blade at the entrance (seen from the sample centre) */
       AngMin    =0.0,        /* minimal angle to collimator entrance in deg          */
       AngMax    =0.0,        /* minimal angle to collimator entrance in deg          */
       AngCntrAct=0.0,        /* actual dir. to the coll. centre in deg (due to osc.width) */
       AngMinAct =0.0;        /* actual minimal angle to collimator entrance in deg        */


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char *argv[])
{
	long     i;
	int      iChanIn, iChanOut;   /*         channels of entrance and exit of the neutron                    */
  short    bHit1, bHit2;        /* booleans: cylinder hit within collimator height                         */
	double   VelocityReal,        /* [cm/ms] velocity of the neutron                                         */
           ToF1, ToF2,          /* [ms]    times of flight to the collimator entrance and through it resp. */
	         NewAngH, NewPosZ;    /*         hor. [deg] and vert. position [cm] of the neutron               */
	      // Theta, Phi;          /* [rad]   horizontal and vertical angle towards centre of collimator      */
	      // RotY,  RotZ,         /* [rad]   corresponding Euler angles                                      */
	// VectorType vDir;              /*         direction to the collimator centre in cartesian co-ordinates    */
	Neutron    OutNeutron, EnterNeutron;

	// reading of input data and initilisation
  // ---------------------------------------
  _eModule=MCN_COLL_RADIAL;

  Init(argc,argv, _eModule);
	PrintModuleName(_eModule, "1.2");
	OwnInit(argc, argv);

  bVisInstalled = TRUE;
  bLengthCmpr   = FALSE;

	AngCntrAct  = AngCentre;
	AngMinAct   = AngMin;

	DECLARE_ABORT

	// loop over trajectories
  // ----------------------
	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			CHECK

      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
			  // Move neutron to the beginning of the collimator and determine position
			  // ----------------------------------------------------------------------			
			  if (InputNeutrons[i].Wavelength <= 0.0) continue;
			  VelocityReal = V_FROM_LAMBDA(InputNeutrons[i].Wavelength); 

			  OutNeutron = InputNeutrons[i];

			  bHit1 = AdvanceToCylinderSurface(&ToF1, &OutNeutron, Distance, EntrHeight, VelocityReal); 

			  if (bHit1)
			  {	
				  if (_eOscColl==VT_RND_PHASE)
				  {	AngCntrAct = MonteCarlo(AngCentre-0.5*OscWidth, AngCentre+0.5*OscWidth);
					  AngMinAct  = AngCntrAct - 0.5*AngWidth;
				  }
				  NewAngH = 180.0/M_PI*atan2(OutNeutron.Position[1], OutNeutron.Position[0]);
				  NewPosZ = OutNeutron.Position[2];
          CopyNeutron(&OutNeutron, &EnterNeutron);
				
				  // Follow neutron through the collimator if it enters into one of the channels
  			  // ---------------------------------------------------------------------------			
				  if (fabs(NewAngH-AngCntrAct) < 0.5*AngWidth  &&  fabs(NewPosZ) < 0.5*EntrHeight)
				  {	
					  // find out entrance channel   (channel = 0 means 'blade position')
					  iChanIn = DetermineChannel(NewAngH, AngMinAct, AngSep, Distance);

					  if (iChanIn > 0)
					  {	
						  bHit2 = AdvanceToCylinderSurface(&ToF2, &OutNeutron, Distance+Length, ExitHeight, VelocityReal); 

						  if (bHit2)
						  {
							  NewAngH = 180.0/M_PI*atan2(OutNeutron.Position[1], OutNeutron.Position[0]);
							  NewPosZ = OutNeutron.Position[2];

							  // Writeout new data set, if neutron leaves inside the exit area through the same channel
        			  // ----------------------------------------------------------------------			
							  if (fabs(NewAngH-AngCntrAct) < 0.5*AngWidth  &&  fabs(NewPosZ) < 0.5*ExitHeight)
							  {	
								  // find out exit channel   (channel = 0 means 'blade position')
								  iChanOut = DetermineChannel(NewAngH, AngMinAct, AngSep, Distance+Length);

								  if (iChanIn==iChanOut)
								  {
									  OutNeutron.Time += (ToF1 + ToF2);
									  // WriteNeutron(&OutNeutron);
									  WriteNeutron(&InputNeutrons[i]);
                    // WriteIAP(&OutNeutron, VT_EXITED);
								  }
                  else
                  { // estimate point inside the collimator for absorption
                    // might be exchanged by the position where it hits the blade
            			  // ----------------------------------------------------------------------			
                    double prc;
                    int k,
                        N2 = 2 * abs(iChanOut - iChanIn); 
                    prc = (double) (N2-1)/N2;
                    for (k=0; k < 3; k++)
                      OutNeutron.Position[k] -= prc*(OutNeutron.Position[k] - EnterNeutron.Position[k]);
                    WriteIAP(&OutNeutron, VT_ABSORBED);
                  }
							  }
						  } // bHit2
					  }
				  }
			  } // bHit1
      }
		}
	}	

// Finish: print parameters, write geometry and instrument file, free memory
// -------------------------------------------------------------------------
 my_exit:
	if (_eOscColl==VT_RND_PHASE)
		fprintf(LogFilePtr, "oscillating amplitude %6.2f deg (phase chosen randomly) \n", OscWidth);
	fprintf(LogFilePtr, "%ld channels from %6.2f to %6.2f deg     \n", nChannels, AngMin, AngMax);
	fprintf(LogFilePtr, "%6.2f cm long, %7.2f cm from the sample \n", Length, Distance);

	// Instrument visualization
	SetGeometry("blue");

	/* Do the general cleanup */
	/* Theta = M_PI/180.0*AngCentre;
	Phi   = 0.0;
	DistExit = Distance + Length;

	SphericalToCartesian(vDir, &Theta, &Phi);
	CartesianToEulerZY  (vDir, &RotY,  &RotZ);
	Cleanup(DistExit*vDir[0], DistExit*vDir[1], DistExit*vDir[2], RotZ, RotY); */
	Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
	int i;
	char  *arg=NULL;

	bOldFrame = TRUE;                        // module uses frame of the previous module, i.e. the sample 

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+') 
		{
			arg=&argv[i][2];
			switch(argv[i][1])
      	{
				case 'd':
					Distance   = atof(arg);      // distance origin - beginning of the collimator in cm 
					break;

				case 'l':
					Length     = atof(arg);      // length of the collimator in cm
					break;
				case 'h':
					EntrHeight = atof(arg);      // entrance height of the collimator in deg 
					break;
				case 'H':
					ExitHeight = atof(arg);      // exit height of the collimator in deg 
					break;

				case 'a':
					AngCentre  = atof(arg);      // direction to the center of the collimator in deg 
					break;
				case 'w':
					AngWidth   = atof(arg);      // angular width of the collimator in deg 
					break;
				case 'n':
					nChannels  = atol(arg);      // bender: No. of channels 
					break;
				case 's':
					BladeWidth =  atof(arg);     // width of bender channel border in cm 
					break;

				case 'o':
					OscWidth   = atof(arg);      //  oscillation width in deg
					if (OscWidth > 0.0)
						_eOscColl = VT_RND_PHASE;
					break;
				/* 
				case 'O':                       //  0: fixed   
					eOscColl = (short) atol(arg);//  1: oscillating, phase not relevant 
					break;                       //  2: oscillating, phase considered 
				case 'f':
					frequency  = atof(arg);      //  frequency in 1/s 
					break;
				case 'p':                   
					phase      = atol(arg);      //  phase in deg at t=0 
					break;   */
      
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}

	AngMin  = AngCentre - 0.5*AngWidth;
	AngMax  = AngCentre + 0.5*AngWidth;

	AngSep  = AngWidth/nChannels;
	AngDead = BladeWidth/Distance*180.0/M_PI;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
 // Geometry data
	if (bVisInstr)
  {
    int    i;
    double ry, rz,   // [rad]  directions of the incoming beam
           phi,      // [rad]  direction to the center of a blade
           Xi;       // [deg]  direction to the center of the collimator

    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    stGeometry.nCylSlices = 2; 
    stGeometry.pCylSlice  = (VtCylSlice*) calloc(2, sizeof(VtCylSlice));
    stGeometry.nHulls     = nChannels+1; 
    stGeometry.pHull      = (VtHull*) calloc(nChannels+1, sizeof(VtHull));

    // get direction to the center of the collimator
    RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
    if (rz < 0) rz += 2.*M_PI;
    fprintf(LogFilePtr,"For the radial collimator ry %f, rz %f", ry, rz);
    Xi = AngCentre + rz/M_PI*180.0;
	      
    stGeometry.pCylSlice[0].Radius     = Distance; 
    stGeometry.pCylSlice[0].Width      = Distance*(AngWidth+OscWidth)/180.0*M_PI;
    stGeometry.pCylSlice[0].Height     = EntrHeight;
    stGeometry.pCylSlice[0].vCntr[0]   = 0.;
    stGeometry.pCylSlice[0].vCntr[1]   = 0.;
    stGeometry.pCylSlice[0].vCntr[2]   = 0.;
    stGeometry.pCylSlice[0].vSymAxis[0]= 0;
    stGeometry.pCylSlice[0].vSymAxis[1]= 0;
    stGeometry.pCylSlice[0].vSymAxis[2]= 1;
    stGeometry.pCylSlice[0].OpenAngle  = AngWidth+OscWidth;
    stGeometry.pCylSlice[0].Phi        = Xi;
	      
    stGeometry.pCylSlice[1].Radius     = Distance+Length; 
    stGeometry.pCylSlice[1].Width      =(Distance+Length)*(AngWidth+OscWidth)/180.0*M_PI;
    stGeometry.pCylSlice[1].Height     = ExitHeight;
    stGeometry.pCylSlice[1].vCntr[0]   = 0.;
    stGeometry.pCylSlice[1].vCntr[1]   = 0.;
    stGeometry.pCylSlice[1].vCntr[2]   = 0.;
    stGeometry.pCylSlice[1].vSymAxis[0]= 0;
    stGeometry.pCylSlice[1].vSymAxis[1]= 0;
    stGeometry.pCylSlice[1].vSymAxis[2]= 1;
    stGeometry.pCylSlice[1].OpenAngle  = AngWidth+OscWidth;
    stGeometry.pCylSlice[1].Phi        = Xi;

    for (i=0; i <= nChannels; i++)
    { 
      phi= (AngMin + i*AngSep)*M_PI/180.0 + rz;

      stGeometry.pHull[i].WidthIn   = BladeWidth;
      stGeometry.pHull[i].WidthOut  = BladeWidth;
      stGeometry.pHull[i].HeightIn  = EntrHeight;
      stGeometry.pHull[i].HeightOut = ExitHeight;
      stGeometry.pHull[i].Length    = Length;
      stGeometry.pHull[i].vCntr[0]  = (Distance+Length/2.)*cos(phi);
      stGeometry.pHull[i].vCntr[1]  = (Distance+Length/2.)*sin(phi);
      stGeometry.pHull[i].vCntr[2]  = 0.0;
      stGeometry.pHull[i].vNormal[0]= cos(phi);
      stGeometry.pHull[i].vNormal[1]= sin(phi);
      stGeometry.pHull[i].vNormal[2]= 0.0;
      stGeometry.pHull[i].rotAngle  = 0.0;
    }

  }
}

	    
/***********************************************************************************/
/* PathThroughColl:                                                                */
/*  calculates the flight time to entrance or exit wall and the point of impact    */
/***********************************************************************************/
short AdvanceToCylinderSurface(double* pTime, Neutron* pNeutron, double Dist, double Height, double Speed)
{
	CylinderType stCyl;
	VectorType   vISP;		         // intersection point
	double       t[2]={0.0,0.0},   // times to reach the surface
	             RotMatrix[3][3];
	short        i, bHit;
	Neutron      stNeutron;

	stCyl.height = Height;
	stCyl.r      = Dist;

	/* co-ordinate transformation as cylinder is supposed to lie in x-direction */ 
	stNeutron = *pNeutron; 
	/* entweder
	pNeutron->Position[0] = stNeutron->Position[2]; pNeutron->Position[2] = -stNeutron->Position[0];
	pNeutron->Vector  [0] = stNeutron->Vector  [2]; pNeutron->Vector  [2] = -stNeutron->Vector  [0];
	pNeutron->Spin    [0] = stNeutron->Spin    [2]; pNeutron->Spin    [2] = -stNeutron->Spin    [0]; 
	   oder */
   FillRotMatrixY(RotMatrix, M_PI_2);
   RotVector     (RotMatrix, pNeutron->Position);
   RotVector     (RotMatrix, pNeutron->Vector);
   RotVector     (RotMatrix, pNeutron->Spin);

	
	bHit   = (short) LineIntersectsCylinder(pNeutron->Position, pNeutron->Vector, &stCyl, t);
	if (bHit)
	{
		for(i=0; i<3; i++)
		{	vISP[i] = pNeutron->Position[i] + t[1]*pNeutron->Vector[i]; // t[1] > t[0] determines the intersection point in flight direction
		}
		*pTime = DistVector(vISP, pNeutron->Position) / Speed; 
		CopyVector(vISP, pNeutron->Position);
	}
	else
	{	*pTime = 0.0;
	}			

	/* back transformation */
	stNeutron=*pNeutron; 
	/* entweder
	pNeutron->Position[0] = -stNeutron->Position[2]; pNeutron->Position[2] = stNeutron->Position[0];
	pNeutron->Vector  [0] = -stNeutron->Vector  [2]; pNeutron->Vector  [2] = stNeutron->Vector  [0];
	pNeutron->Spin    [0] = -stNeutron->Spin    [2]; pNeutron->Spin    [2] = stNeutron->Spin    [0]; 
	   oder */
   RotBackVector(RotMatrix, pNeutron->Position);
   RotBackVector(RotMatrix, pNeutron->Vector);
   RotBackVector(RotMatrix, pNeutron->Spin);

	return bHit;   
}


/***********************************************************************************/
/* DetermineChannel:                                                               */
/*  determines the channel as a function of horizontal position                    */
/***********************************************************************************/
int DetermineChannel(double angle, double angle_min, double angle_sep, double radius)
{
	int    iChan=0;
	double Channel,
	       ChanDist;  // distance between 2 channels

	ChanDist = radius * angle_sep * M_PI/180.0;
	Channel  = (angle - angle_min) / angle_sep;

	if (fabs(Channel - Round(Channel)) >= 0.5*BladeWidth/ChanDist)
		iChan = (int) floor(Channel) + 1;

	return iChan;
} 

