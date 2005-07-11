/****************************************************************************************/
/*  VITESS module DETECTOR                                                              */
/* The free non-commercial use of these routines is granted providing due credit        */
/* is given to the authors.                                                             */
/*                                                                                      */
/* 1.0                           initial version                                        */
/* 1.1  Jun 2001  G. Zsigmond    SOFTABORT                                              */
/* 1.2  Jan 2002  K. Lieutenant  reorganisation                                         */
/* 1.3  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                           */
/* 1.4  Feb 2004  K. Lieutenant  'message' and 'ERROR' included                         */
/* 1.5  Apr 2004  K. Lieutenant  probability divided by repetition                      */
/* 1.5a Dec 2004  K. Lieutenant  option 'no TOF' added                                  */
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "init.h"
#include "matrix.h"
#include "softabort.h"
#include "intersection.h"
#include "message.h"


#define VT_NO_TOF    0
#define VT_TOF_CALC  1
#define VT_TOF_BIN   2


/* global variables */
SampleType Detector;
int        geom=0;
long       Columns=1, 		 /* number of columns of the detector*/
           Rows=1;    		 /* number of rows of the detector*/
double     Thickness,
           distance;
double     MaxEfficiency;   /* Maximal efficieny of the detector */
long       GenNeutrons=10;
short      bMonitor,        /* option: only monitoring            */
           eTOF;            /* option: TOF in detector 0: no 1: calculated 2: binning */
double     Theta,dTheta,
           dWidth, 
           RotMatrix[3][3];
double     itheta, iphi;


/* function pointers */
long (*NeutronIntersectsDetector)(Neutron *Nin, SampleType *Detector, VectorType ISP[]);
void (*DetectorSpot)(VectorType SP, VectorType DetSpot, SampleType *Detector);


/* prototypes of local functions */
long NeutronIntersectsCubeDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[]);
long NeutronIntersectsCylDetector (Neutron *Nin, SampleType *Detector, VectorType ISP[]);

void NoDetSpot      (VectorType SP, VectorType DetSpot, SampleType *Detector);
void CubeDetSpot    (VectorType SP, VectorType DetSpot, SampleType *Detector);
void CylinderDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector);

void OwnInit   (int argc, char *argv[]);
void OwnCleanup();


int main(int argc, char *argv[])
{
	double LambdaProb;    /* detecting probability due to the wavelength of the neutron */
	VectorType ISP[2];    /* intersection points with the detector */
	double NSigma;        /*neutron crossection times density of scatterers in the detector*/
	long   NeutCount;
	double TimeTillScattering, LengthTillScattering;
	VectorType DetSpot, SP, vDir;
	long   i,j;
	double FullLengthInDetector;
	double ScatteringProb,
	       dRotY, dRotZ;
	Neutron OutNeutron;
	double  norm;  /* intensity norm factor */

	/* Initialize the program according to the parameters given   */
	Init(argc, argv, VT_DETECTOR);

	/* module specific initialization */
	OwnInit(argc, argv);

	print_module_name("detector 1.5a");

	/* Rotmatrix will rotate a Vector to a frame in which the middle of the */
	/* Detector sits on the x-axis */
	RotMatrixX(Detector.Direction, RotMatrix);

	NSigma=-log(1-MaxEfficiency)/Thickness;

	/* Get the neutrons from the file */
	DECLARE_ABORT

	while((ReadNeutrons())!= 0)
	{
		CHECK

		for(i=0; i<NumNeutGot; i++) 
		{
			CHECK

			/* First rotate the position and direction of the neutron to the detector frame */
			RotVector(RotMatrix, InputNeutrons[i].Position);
			RotVector(RotMatrix, InputNeutrons[i].Vector);

			if(NeutronIntersectsDetector(&(InputNeutrons[i]),&Detector,ISP)) 
			{
				/* determine the length of the path through the scintilator */
				FullLengthInDetector = DistVector(ISP[0], ISP[1]);

				/* intensity norm factor */
				norm = Thickness; //MaxEfficiency*FullLengthInDetector/(1-exp(-NSigma*Thickness));

				/* now the influence of the wavelength*/
				if(InputNeutrons[i].Wavelength < 5.0)
					LambdaProb = 0.5 + 0.1*InputNeutrons[i].Wavelength;
				else
					LambdaProb = 1.0;

				for(NeutCount=0; NeutCount<GenNeutrons; NeutCount++)
				{
					if (eTOF==VT_TOF_CALC)
					{	/* determine the scattering point in the scintilator */
						LengthTillScattering = MonteCarlo(0,FullLengthInDetector);
						ScatteringProb = NSigma*exp(-NSigma*LengthTillScattering) * norm * LambdaProb;
					}
					else
					{
						LengthTillScattering = 0.0;
						ScatteringProb = MaxEfficiency * LambdaProb;
					}
					if(bMonitor)
						ScatteringProb=1.0;

					for(j=0; j<3; j++)
						SP[j]= ISP[0][j] +LengthTillScattering*InputNeutrons[i].Vector[j];

					DetectorSpot(SP, DetSpot, &Detector);

					TimeTillScattering=DistVector(SP,InputNeutrons[i].Position)/
											 V_FROM_LAMBDA(InputNeutrons[i].Wavelength);

					/* everythings done, so rot back the vectors and put all together */
					RotBackVector(RotMatrix,DetSpot);
					RotBackVector(RotMatrix,SP);

					/* set output data of the neutron */
					OutNeutron             = InputNeutrons[i];
					OutNeutron.Time        = InputNeutrons[i].Time + TimeTillScattering;
					OutNeutron.Probability = InputNeutrons[i].Probability * ScatteringProb / GenNeutrons;
					CopyVector(SP, OutNeutron.Position);
					NormVector(DetSpot);
					CartesianToSpherical(DetSpot, &(OutNeutron.Vector[0]), &(OutNeutron.Vector[1]));

					OutNeutron.Vector[2] = LengthVector(DetSpot);

					WriteNeutron(&OutNeutron);
				} /* for */
			} /* if */
		}
	}

  my_exit:
	/* Do module specific cleanups */
	OwnCleanup();

	/* Do the general cleanup */
	SphericalToCartesian(vDir, &itheta, &iphi);
	CartesianToEulerZY  (vDir, &dRotY, &dRotZ);
	Cleanup(distance*vDir[0], distance*vDir[1], distance*vDir[2], dRotZ, dRotY);

	return 0;
}



long NeutronIntersectsCubeDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[])
{
	VectorType Position;
	double t[2];
	long i;

	/* move the origin of the coordinate system to the middle of the detector*/
	CopyVector(Nin->Position,Position);
	Position[0] -= Detector->Position[0]-Detector->SG.Cube.thickness/2.0;

	if(LineIntersectsCube(Position, Nin->Vector, &(Detector->SG.Cube), t)) 
	{
		for(i=0; i<3; i++)
		{	ISP[1][i] = Position[i]+t[1]*Nin->Vector[i];
			ISP[0][i] = Position[i]+t[0]*Nin->Vector[i];
		}

		/* transform the ISPs to the old coordinate system*/
		ISP[0][0] += Detector->Position[0]+Detector->SG.Cube.thickness/2.0;
		ISP[1][0] += Detector->Position[0]+Detector->SG.Cube.thickness/2.0;
		if(t[1]>=0.0) 
		{	
			return TRUE;
		} 
		else 
		{
			CountMessageID(DET_TRAJ_INSIDE, Nin->ID);
			return FALSE;
		}
	} 
	else 
	{	return FALSE;
	}
}



long NeutronIntersectsCylDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[])
{
	VectorType InISP[2], OutISP[2], Position, cut;
	double Int[2], Outt[2], ntheta, nphi, CTheta;
	long   i;

	CopyVector(Nin->Position, Position);
	SubVector (Position, Detector->Position);

	if(LineIntersectsCylinder(Position, Nin->Vector, &(Detector->SG.Cyl), Int)) 
	{
		for(i=0; i<3; i++)
		{	InISP[1][i] = Position[i]+Int[1]*Nin->Vector[i];
			InISP[0][i] = Position[i]+Int[0]*Nin->Vector[i];
		}
	}
	else
	{
		return FALSE;
	}

	CopyVector(InISP[1],cut);
	RotBackVector(RotMatrix,cut);
	NormVector(cut);
	CartesianToSpherical(cut, &ntheta, &nphi);

	/*x=r_kugel*cos(ntheta) = r_cyl*cos(CTheta);
	  y=r_kugel*sin(ntheta)*cos(nphi) = r_cyl*sin(CTheta);
	  z=r_kugel*sin(ntheta)*sin(nphi) = z;
	  ==> ...   */
	CTheta=atan(cos(nphi)*tan(ntheta));

	if ((ntheta>M_PI_2) && (CTheta<=0)) 
		CTheta= M_PI + CTheta;
	else if ((ntheta>M_PI_2) && (CTheta>0)) 
		CTheta= -M_PI + CTheta;

	if(fabs(CTheta-Theta) < dTheta || fabs(CTheta+2*M_PI-Theta) < dTheta) 
	{
		Detector->SG.Cyl.r += Thickness;
		if(LineIntersectsCylinder(Position, Nin->Vector, (CylinderType *)&(Detector->SG), Outt)) 
		{
			for(i=0; i<3; i++)
			{	OutISP[1][i] = Position[i]+Outt[1]*Nin->Vector[i];
				OutISP[0][i] = Position[i]+Outt[0]*Nin->Vector[i];
			}
		}
		else
		{
			return FALSE;
		}
		Detector->SG.Cyl.r-=Thickness;
		if( (fabs(InISP[1][0]) < Detector->SG.Cyl.height*0.99999/2.0) && (fabs(OutISP[1][0]) <Detector->SG.Cyl.height*0.99999/2.0)) 
		{
			CopyVector(InISP[1],ISP[0]);
			CopyVector(OutISP[1],ISP[1]);
			return TRUE;
		}
	}
	return FALSE;
}



void CubeDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector)
{
	double IComp, JComp, I,J;

	/* determine the I and J component of SP */
	IComp = SP[1]+Detector->SG.Cube.width/2.0;
	JComp = SP[2]+Detector->SG.Cube.height/2.0;
	I = floor(IComp*Columns/Detector->SG.Cube.width);
	J = floor(JComp*Rows/Detector->SG.Cube.height);

	DetSpot[0]=Detector->Position[0];
	DetSpot[1]=Detector->SG.Cube.width*(I+0.5-0.5*Columns)/Columns;
	DetSpot[2]=Detector->SG.Cube.height*(J+0.5-0.5*Rows)/Rows;
}



void CylinderDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector)
{
	double     STheta, SPhi, CTheta, SpotTheta,
	           archpos, JComp, I,J;
	VectorType nsp;

	CopyVector(SP,nsp);
	RotBackVector(RotMatrix,nsp);

	NormVector(nsp);
	CartesianToSpherical(nsp, &STheta, &SPhi);

	CTheta=atan(cos(SPhi)*tan(STheta));

	if ((STheta>M_PI_2) && (CTheta<=0)) 
		CTheta =  M_PI + CTheta;
	else if ((STheta>M_PI_2) && (CTheta>0)) 
		CTheta = -M_PI + CTheta;

	archpos= fabs(CTheta-(Theta+dTheta));  /*counted from 0 to radians of archlength*/
	I=floor(Columns*archpos/(2.0*dTheta));
	SpotTheta=Theta+dTheta -((I+0.5)/Columns)*(2.0*dTheta);

	JComp = nsp[2]+Detector->SG.Cyl.height/2.0;
	J = floor(JComp*Rows/Detector->SG.Cyl.height);

	DetSpot[0]=Detector->SG.Cyl.r*cos(SpotTheta);
	DetSpot[1]=Detector->SG.Cyl.r*sin(SpotTheta);
	DetSpot[2]=Detector->SG.Cyl.height*(J+0.5-0.5*Rows)/Rows;

	RotVector(RotMatrix,DetSpot);
}



void NoDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector)
{
	CopyVector(SP,DetSpot);
}



void  OwnInit(int argc, char *argv[])
{
	long i;
	int  optiontest=0;
	double dHeight=0.0, thickness=0.0;
	long NoDetGrid=FALSE;

	/* some default values */
	GenNeutrons=10;
	Columns = 1;
	Rows = 1;
	geom=0;
	bMonitor = FALSE;
	NoDetGrid= FALSE;
	eTOF     = VT_TOF_CALC;
	Detector.Direction[0]=0.0;
	Detector.Direction[1]=0.0;

	for(i=1; i<argc; i++)
	{
		if(argv[i][0]!='+')
		{	switch(argv[i][1])
			{	case 'G':
					if(strstr(&(argv[i][2]), "cyl")!=NULL) 
					{ geom=1;
					} 
					else 
					{
						if(strstr(&(argv[i][2]), "cub")!=NULL) 
						{ geom=2;
						} 
						else 
						{ Error("Unknown detector geometry");
						}
					}
					break;
				case 'h':
					sscanf(&(argv[i][2]),"%lf", &dHeight);
					optiontest |= 0x1;
					break;
				case 'w':
					sscanf(&(argv[i][2]),"%lf", &dWidth);
					optiontest |= 0x2;
					break;
				case 't':
					sscanf(&(argv[i][2]),"%lf", &thickness);
					if(thickness<=0.0) thickness=1e-4;
					optiontest |= 0x4;
					break;
				case 'e':
					sscanf(&(argv[i][2]),"%lf", &MaxEfficiency);
					if(MaxEfficiency>=1.0) MaxEfficiency=0.99999;
					optiontest |= 0x8;
					break;
				case 'T':
					/* Theta is the angle between the +x-axis and the vector*/
					sscanf(&(argv[i][2]),"%lf", &itheta);
					itheta*=M_PI/180.0;
					break;
				case 'P':
					/* Phi is the angle of the +y-axis and the projection of the vector to the yz-plane */
					sscanf(&(argv[i][2]),"%lf", &iphi);
					iphi*=M_PI/180.0;
					break;
				case 'D':
					sscanf(&(argv[i][2]),"%lf", &distance);
					optiontest |= 0x10;
					break;
				case 'c':
					sscanf(&(argv[i][2]),"%ld", &Columns);
					break;
				case 'r':
					sscanf(&(argv[i][2]),"%ld", &Rows);
					break;
				case 'A':
					sscanf(&(argv[i][2]),"%ld", &GenNeutrons);
					break;
				case 'o':
					sscanf(&(argv[i][2]),"%d", &eTOF);
					break;

				case 'M':
					if(argv[i][2]=='1') bMonitor=TRUE;
					break;
				case 'g':
					if(argv[i][2]=='0') NoDetGrid=TRUE;
					break;
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}

	if(geom!=0)
	{	if(geom==1)
		{	/* cylinder */
			/*if(optiontest != 31) {
			  fprintf(LogFilePtr,"For the cylindrical detector the -D, -t, -h, -w and -e options are mandatory!\n");
			  exit(-1);
			}*/
			Detector.SG.Cyl.r=distance;
			Detector.SG.Cyl.height = dHeight;
			Thickness = thickness;
			Detector.Direction[0]=0.0;
			Detector.Direction[1]=0.0;
			Detector.Direction[2]=1.0;

			Detector.Position[0]=0.0;
			Detector.Position[1]=0.0;
			Detector.Position[2]=0.0;

			Theta=itheta;
			if (cos(iphi) < 0.0) Theta=-Theta;
			dTheta=dWidth/(2.0*distance);

			NeutronIntersectsDetector=NeutronIntersectsCylDetector;
			DetectorSpot=CylinderDetSpot;
		} 
		else 
		{
			/* cube */
			/*if(optiontest != 31)
			{ fprintf(LogFilePtr,"For the cube type detector the -h, -w, -t, -e and -D options are mandatory!\n");
			  exit(-1);
			}*/
			Detector.SG.Cube.height = dHeight;
			Detector.SG.Cube.width =dWidth;
			Detector.SG.Cube.thickness = thickness;
			Detector.Direction[0]=cos(itheta);
			Detector.Direction[1]=sin(itheta)*cos(iphi);
			Detector.Direction[2]=sin(itheta)*sin(iphi);
			for(i=0; i<3;i++)
				if(fabs(Detector.Direction[i])<1e-5) Detector.Direction[i]=0.0;

			Detector.Position[0] = distance;
			Detector.Position[1] = 0.0;
			Detector.Position[2] = 0.0;
			Thickness=thickness;
			NeutronIntersectsDetector=NeutronIntersectsCubeDetector;
			DetectorSpot=CubeDetSpot;
		}
	} 
	else 
	{
		fprintf(LogFilePtr,"ERROR: You have to supply a detector geometry with -Gcub or -Gcyl!\n");
		exit(-1);
	}
	if(NoDetGrid) DetectorSpot=NoDetSpot;
}



void OwnCleanup()
{
  /* print error that might have occured many times */
	PrintMessage(DET_TRAJ_INSIDE, "", ON);

	fprintf(LogFilePtr," \n");

	/* set description for instrument plot */
	stPicture.dWPar  = dWidth;
	stPicture.dHPar  = dWidth/Columns;
	stPicture.dRPar  = distance;
	stPicture.eType  = (short) geom;
}

