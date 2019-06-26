/********************************************************************************************/
/*  VITESS module 'velselect'                                                               */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given to*/
/* the authors.                                                                             */
/*                                                                                          */
/* 1.0  Jun 1999  D. Wechsler							                                            */
/* 1.1  Jun 2001  K. Lieutenant  correction: memory allocation for pAngIn + SOFTABORT       */
/* 1.2  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                               */
/********************************************************************************************/

#include "general.h"
#include "init.h"
#include "softabort.h"

#include <stdlib.h>
#include <math.h>


double  rad, length, spacer, Rotn, Curve, DistOrigin;
double  globalz, neutAng, Velocity, Rotang, TrailingEdge, LeadingEdge, winwdth, winAng;
double  spacAng, t, deltaRot;
double* pAngIn;	
long winnum;

void  OwnInit(int argc, char *argv[]);

int main(int argc, char *argv[])
{


	long i,n;
	long BufferIndex;

	Neutron  Output;


	 /* initialisation, definitions see Input below */
	rad = 20.0;
	length = 100.0;
	winnum = 90;
	spacer =0.15;
	Rotn = 0.1;
	Curve = 45.0;
	DistOrigin = 15.0;
	BufferIndex = 0;


	/*Input*/

	Init(argc, argv, VT_VEL_SELECT);

	OwnInit(argc, argv);


	DECLARE_ABORT

	while(ReadNeutrons()!= 0)
	{
		for(i=0; i<NumNeutGot; i++)
		{
			Velocity = V_FROM_LAMBDA(InputNeutrons[i].Wavelength);
			globalz = DistOrigin + InputNeutrons[i].Position[2];  /* Distance axle velsel. neutron (z-direction)*/
			if (globalz<0.0) {fprintf(LogFilePtr," error1, wrong geometry"); exit(99);}

			/* calculation  of  angle: z-axis; center velsel.; neutron position */
		    neutAng = atan(InputNeutrons[i].Position[1]/globalz);
			if (fabs(neutAng) > M_PI) {fprintf(LogFilePtr," error2, wrong geometry"); exit(99);}

			/* Rotation angle of velsel. corresponding to neutron time */
			Rotang = 2.0*M_PI*Rotn*InputNeutrons[i].Time;

			/* all angles between -PI and PI; 0 corresponds to z-axis */
			while (Rotang >= M_PI) Rotang-=2.0*M_PI;
			while (Rotang < -M_PI) Rotang+=2.0*M_PI;

			/* Loop over all windows of the velsel. */
			for(n=0; n<winnum;n++)
			{
				CHECK

				/* angle region of window n [TrailingEdge;LeadingEdge] */
				TrailingEdge = Rotang + pAngIn[n] + spacAng;
				while (TrailingEdge >= M_PI) TrailingEdge-=2.0*M_PI;
				LeadingEdge = TrailingEdge + winAng;

				if ((neutAng >TrailingEdge)&&(neutAng<LeadingEdge))
				{	/* caclculation if exit window is hit */

					/* time for passing the velsel. */

					t= length/(Velocity*InputNeutrons[i].Vector[0]);


					globalz = DistOrigin + InputNeutrons[i].Position[2];

					/* old coordinates and test if neutron hits the velselect front*/

					if ((globalz*globalz +InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1])> rad*rad)
					  {  goto Getnewneutron;}
					/* new coordinates and new neutron angle and test if cylinder walls absorbed the neutron*/

					InputNeutrons[i].Position[0] += Velocity*t*InputNeutrons[i].Vector[0];
					InputNeutrons[i].Position[1] += Velocity*t*InputNeutrons[i].Vector[1];
					InputNeutrons[i].Position[2] += Velocity*t*InputNeutrons[i].Vector[2];

					InputNeutrons[i].Time+=t;
					globalz = DistOrigin + InputNeutrons[i].Position[2];

					if ((globalz*globalz +InputNeutrons[i].Position[1]*InputNeutrons[i].Position[1])> rad*rad)
					  {  goto Getnewneutron;}

					neutAng = atan(InputNeutrons[i].Position[1]/globalz);
					if (fabs(neutAng) > M_PI) {fprintf(LogFilePtr," error3, wrong geometry"); exit(99);}


					/* update TrailingEdge and Leading Edge for the channel under consideration*/

					deltaRot = (t*2.0*M_PI*Rotn) - Curve;

					if (fabs(deltaRot) >= M_PI) break;

					TrailingEdge+=deltaRot;
					while (TrailingEdge >= M_PI) TrailingEdge-=2.0*M_PI;
					while (TrailingEdge <= -M_PI) TrailingEdge+=2.0*M_PI;
					LeadingEdge= winAng+TrailingEdge;

					if ((neutAng >TrailingEdge)&&(neutAng<LeadingEdge)) {goto Transmission;}
					else { goto Getnewneutron;}
				}
				/* loop over windows continued */
			}

			Getnewneutron: continue; /* case of neutron blocked by spacers or absorbed within a channel */

			Transmission:

			InputNeutrons[i].Position[0]=0.0;

			Output = InputNeutrons[i];

			WriteNeutron(&Output);
		}
	}

	my_exit:
	free(pAngIn);

	Cleanup(length,0.0,0.0, 0.0,0.0);

	return(0);
}


void OwnInit(int argc, char *argv[])
{

  int i;
  bVisInstalled = TRUE;

  for(i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') 
	{
	  switch(argv[i][1])
	    {
	    case 's':
	      Rotn = atof(&argv[i][2]); /* number of velsel. rotations per second */
	      Rotn = Rotn/1000.0; /*conversion to ms */
	      break;

	    case 'l':
	      length = atof(&argv[i][2]); /* length of velsel. [cm] */
	      break;

	    case 'w':
	      winnum = atol(&argv[i][2]); /* number of windows */
	      break;

	    case 'd':
	      spacer = atof(&argv[i][2]); /* interslotspacing [cm] */
	      break;

	    case 'c':
	      Curve = atof(&argv[i][2]); /* curvature of velsel. channels [deg.] */
	      Curve=Curve*M_PI/180.0;
	      break;

	    case 'r':
	      rad = atof(&argv[i][2]); /* Radius of velsel. [cm] */
	      break;

	    case 'o':
	      DistOrigin = fabs(atof(&argv[i][2])); /* distance origin - axle of velsel. rotations [cm] */
	      break;								  /* origin = center of end of guide */

	    default:
	      fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	      exit(-1);
	      break;
	    }
	}
    }

  print_module_name("velselect 1.2a");
  
  /* definition of mean windowwidth, corresponding angle and angle with respect to interslotspacing */
  winwdth = (2.0*M_PI*DistOrigin/(double)winnum) -2.0*DistOrigin*asin(spacer/(2.0*DistOrigin));
  winAng= winwdth/DistOrigin;
  spacAng = 2.0*DistOrigin*asin(spacer/(2.0*DistOrigin))/DistOrigin;
  
  /* definition of window "coordinates" */
  pAngIn = calloc(winnum, sizeof(double));
  
  for (i=0; i<winnum; i++)
    {	
      pAngIn[i] = i*(winAng + spacAng);
    }
  
  // Visualisation of the velocity selector geometry
   
   if (bVisInstr)
     { 
       stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
       stGeometry.nCylinders = 1;
       
       stGeometry.pCylinder[0].Radius = rad;
       stGeometry.pCylinder[0].Length = length;
       stGeometry.pCylinder[0].vCntr[0]  = stGeometry.pCylinder[0].Length/2.;
       stGeometry.pCylinder[0].vCntr[1]  = 0;
       stGeometry.pCylinder[0].vCntr[2]  = DistOrigin;
       stGeometry.pCylinder[0].vSymAxis[0] = 1.;
       stGeometry.pCylinder[0].vSymAxis[1] = 0.;
       stGeometry.pCylinder[0].vSymAxis[2] = 0.;
       
       stGeometry.pDescr  = "vel_selector:grey";
       stGeometry.eModule = VT_VEL_SELECT;
  }    

  return;

}
