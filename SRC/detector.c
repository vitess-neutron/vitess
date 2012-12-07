#/****************************************************************************************/
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
/* 1.6  Nov 2005  K. Lieutenant  direction instead of scattering angles written         */
/*                               correction: grid for cylindrical geometry              */
/* 1.6a Dec 2009  A. Houben      Detect only neutrons with a given color and add        */
/*                               a value to the color property after detection          */
/* 1.7  Jul 2012  A. Houben      Cylindrical detector with constant angle in phi        */
/*                               (pixel size will vary with phi)                        */
/* 1.7a Jul 2012  A. Houben      Wavelength dependant probability is read from file     */
/* 1.8  Oct 2012  D. Nekrassov                                                          */
/*              & K. Lieutenant  visualization                                          */
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

typedef struct
{
  double Lambda, Eff;
}
  EffData;


typedef struct
{
  FILE    *pfile;
  char    *filename;
  EffData *data;
  long    maxdata;
}
  EffFile;

/* global variables */
SampleType Detector;
int        geom=0;           // detector geometry: 1: cylinder   2: flat
short      phimode=0;      /* 0 = const. pixel size; 1 = const Delta phi */
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
double     itheta, iphi, constphi;
short      DetectColor = -1, AddColor = -1; // Detect only neutrons with a given color and assign a new color after detection

EffFile Eff = {0};

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

double GetLambdaProbFromEff(const double lambda);
static FILE * tryOpen(const char *fn, const char *s);

int main(int argc, char *argv[])
{
  double LambdaProb;    /* detecting probability due to the wavelength of the neutron */
  VectorType ISP[2];    /* intersection points with the detector */
  double NSigma;        /*neutron crossection times density of scatterers in the detector*/
  long   NeutCount;
  double TimeTillScattering, LengthTillScattering;
  VectorType DetSpot, SP, vDir, vShift;
  long   i,j;
  double FullLengthInDetector;
  double ScatteringProb,
    dRotY, dRotZ;
  Neutron OutNeutron;
  Neutron WorkNeutron;
  double  norm;  /* intensity norm factor */

  /* Initialize the program according to the parameters given   */
  Init(argc, argv, VT_DETECTOR);
  print_module_name("detector 1.8");

  /* module specific initialization */
  OwnInit(argc, argv);
  SphericalToCartesian(vDir, &itheta, &iphi);
  CopyVector(vDir, vShift);
  MultiplyByScalar(vShift, distance);

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

      // Use a copy to work on
      WorkNeutron = InputNeutrons[i];
	      
      /* First rotate the position and direction of the neutron to the detector frame */
      RotVector(RotMatrix, WorkNeutron.Position);
      RotVector(RotMatrix, WorkNeutron.Vector);
	       

      if(NeutronIntersectsDetector(&(WorkNeutron),&Detector,ISP)) 
      {
		  
        /* determine the length of the path through the scintilator */
        FullLengthInDetector = DistVector(ISP[0], ISP[1]);
		  
        /* intensity norm factor */
        norm = Thickness; //MaxEfficiency*FullLengthInDetector/(1-exp(-NSigma*Thickness));

        /* now the influence of the wavelength*/
        if (Eff.maxdata > 0) {
            LambdaProb = GetLambdaProbFromEff(WorkNeutron.Wavelength);
          } else {
            if(WorkNeutron.Wavelength < 5.0) {
                LambdaProb = 0.5 + 0.1*WorkNeutron.Wavelength;
              } else {
                LambdaProb = 1.0;
              }
          }

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
          { ScatteringProb       = 1.0;
            LengthTillScattering = 0.0;
          }

          for(j=0; j<3; j++)
            SP[j]= ISP[0][j] +LengthTillScattering*WorkNeutron.Vector[j];

          DetectorSpot(SP, DetSpot, &Detector);

          TimeTillScattering=DistVector(SP,WorkNeutron.Position)/
          V_FROM_LAMBDA(WorkNeutron.Wavelength);

          /* everythings done, so rot back the vectors and put all together 
          and set output data of the neutron */
          OutNeutron             = WorkNeutron;
          OutNeutron.Time        = WorkNeutron.Time + TimeTillScattering;
          OutNeutron.Probability = WorkNeutron.Probability * ScatteringProb / GenNeutrons;
		      		     		      
          if (bMonitor)
          {
            RotBackVector(RotMatrix, SP);
            CopyVector(SP, OutNeutron.Position);
          }
          else
          {	
            RotBackVector(RotMatrix,DetSpot);
            CopyVector(DetSpot, OutNeutron.Position);
            NormVector(DetSpot);
            CopyVector(DetSpot, OutNeutron.Vector);
          }
		      
          // write interaction point - only once per incoming trajectory
          if (NeutCount < 1)  
          { if (AddColor > 0) OutNeutron.Color += AddColor;
            WriteIAP(&OutNeutron, VT_DETECTED);
          }

          // write out neutrons that shall be detected
          if ((DetectColor < 1) || (InputNeutrons[i].Color == DetectColor)) 
          {
            // the neutron co-ordinates are adapted to the shift of the co-ordinate for a flat detector (see below)
            if (geom==2)
              SubVector(OutNeutron.Position, vShift);
            if (AddColor > 0) OutNeutron.Color += AddColor;
              WriteNeutron(&OutNeutron);			  
          }
        } /* loop count */
      } 
      else /* if neutron does not intersect detector */ 
      {
        VectorType Pos1, Pos2;  // intersection points of trajectory with cylinder
 
        if (geom==2) // flat
        { 
          RotBackVector(RotMatrix, WorkNeutron.Vector);
          RotBackVector(RotMatrix, WorkNeutron.Position);
          PlaneLineIntersect(WorkNeutron.Position, WorkNeutron.Vector, vDir, distance, WorkNeutron.Position);
        }
        else
        { 
          IntersectionWithInfiniteCylinder(distance, WorkNeutron.Position, WorkNeutron.Vector, Pos1, Pos2);
          if (Pos1[0]*WorkNeutron.Vector[0] > 0)
            CopyVector(Pos1, WorkNeutron.Position);
          else
            CopyVector(Pos2, WorkNeutron.Position);
          RotBackVector(RotMatrix, WorkNeutron.Vector);
          RotBackVector(RotMatrix, WorkNeutron.Position);
        }
        WriteIAP(&WorkNeutron, VT_OUTSIDE);
      }
    } //for(i=0; i<NumNeutGot; i++)
  } //while((ReadNeutrons())!= 0)

 my_exit:
  /* Do module specific cleanups */
  OwnCleanup();

  /* Do the general cleanup */
  // the origin of the co-ordinate system is shifted to the center of a flat detector
  // for a cylindric detector it remains at the sample  
  CartesianToEulerZY  (vDir, &dRotY, &dRotZ);
  if (geom==2)
    Cleanup(vShift[0], vShift[1], vShift[2], dRotZ, dRotY);
  else
    Cleanup(0.0,0.0,0.0, 0.0,0.0);

  return 0;
}

double GetLambdaProbFromEff(const double lambda) {
  int i;

  if (lambda < Eff.data[0].Lambda) {
    if (lambda < 5.)
      return 0.5 + 0.1*lambda;  //What should happen if not Eff.data for given wavelength?
    else
      return Eff.data[0].Eff;  //What should happen if not Eff.data for given wavelength?
  } else {
    for (i = 1; i<=Eff.maxdata; i++) {
      if (lambda < Eff.data[i].Lambda)
        return Eff.data[i].Eff + (Eff.data[i].Eff-Eff.data[i-1].Eff)/(Eff.data[i].Lambda-Eff.data[i-1].Lambda) * (lambda-Eff.data[i].Lambda);
    }
    if (i>Eff.maxdata) return Eff.data[i].Eff;  //What should happen if not Eff.data for given wavelength?
  }
  return -1.;
}

long NeutronIntersectsCubeDetector(Neutron *Nin, SampleType *Detector, VectorType ISP[])
{
	VectorType Position;
	double t[2];
	long i;
	//char* form = "%c%c%09lu %c %5d  %7.3f %8.5f %11.3e  %8.4f %8.4f %8.4f  %9.6f %9.6f %9.6f   %4.1f %4.1f %4.1f\n";

	/* move the origin of the coordinate system to the middle of the detector*/
	CopyVector(Nin->Position,Position);
	Position[0] -= Detector->Position[0]-Detector->SG.Cube.thickness/2.0;

	/*if (fabs(Nin->Position[0]) == 0.5 || fabs(Nin->Position[1]) == 0.5 || fabs(Nin->Position[2]) == 0.5)
		fprintf(LogFilePtr, "", "");*/
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
			/*fprintf(LogFilePtr, form,
			  Nin->ID.IDGrp[0], Nin->ID.IDGrp[1], Nin->ID.IDNo,          
			  Nin->Debug,       Nin->Color,       
			  Nin->Time,        Nin->Wavelength,  Nin->Probability,
			  Nin->Position[0], Nin->Position[1], Nin->Position[2], 
			  Nin->Vector[0],   Nin->Vector[1],   Nin->Vector[2], 
			  Nin->Spin[0],     Nin->Spin[1],     Nin->Spin[2]);*/
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
	           archpos, JComp, I,J, SPHeight;
	VectorType nsp;

	CopyVector(SP,nsp);
	RotBackVector(RotMatrix,nsp);
	SPHeight = nsp[2];

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

	DetSpot[0]=Detector->SG.Cyl.r*cos(SpotTheta);
	DetSpot[1]=Detector->SG.Cyl.r*sin(SpotTheta);

	if (phimode == 0) {
	    JComp = SPHeight + Detector->SG.Cyl.height/2.0;
	    J = floor(JComp*Rows/Detector->SG.Cyl.height);

	    DetSpot[2]=Detector->SG.Cyl.height*(J+0.5-0.5*Rows)/Rows;
	  } else {
	    JComp = atan2(Detector->SG.Cyl.r, SPHeight);
	    J = floor((JComp-M_PI/2.)/constphi+0.5)*constphi+M_PI/2.;
	
	    DetSpot[2]=tan(M_PI/2.-J)*Detector->SG.Cyl.r;
	  }
        
	RotVector(RotMatrix,DetSpot);
}



void NoDetSpot(VectorType SP, VectorType DetSpot, SampleType *Detector)
{
	CopyVector(SP,DetSpot);
}



void  OwnInit(int argc, char *argv[])
{
  long i;
  int  optiontest=0, iv;
  double dHeight=0.0, thickness=0.0;
  long NoDetGrid=FALSE;
  char sBuffer[512];
  long count = 0;

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
    {
      switch(argv[i][1]) 
      { case 'G':
	        if      (strstr(&(argv[i][2]), "cyl")!=NULL) geom=1; 
          else if (strstr(&(argv[i][2]), "cub")!=NULL) geom=2;
	        else    Error("Unknown detector geometry");
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

        case 'E':    /* efficiency file */
           Eff.pfile = tryOpen( (Eff.filename = &argv[i][2]), "efficiency file");
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

        case 'p':
	  /* Delta Phi */
	  sscanf(&(argv[i][2]),"%hd", &phimode);
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
          sscanf(&(argv[i][2]),"%d", &iv);
          eTOF = iv;
          break;

        case 'M':
          if(argv[i][2]=='1') bMonitor=TRUE;
          break;

        case 'g':
          if(argv[i][2]=='0') NoDetGrid=TRUE;
          break;

        case 'C':
          sscanf(&(argv[i][2]),"%hd", &DetectColor);
          break;

  	    case 'S':
          sscanf(&(argv[i][2]),"%hd", &AddColor);
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
	  constphi = atan(dHeight/2./distance)*2./Rows;

	  NeutronIntersectsDetector=NeutronIntersectsCylDetector;
	  DetectorSpot=CylinderDetSpot;

	  bVisInstalled = TRUE;
	  // Geometry data
	  if (bVisInstr)
	    { 
	      double ry, rz;
	      stGeometry.pCylSlice = (VtCylSlice*) calloc(1, sizeof(VtCylSlice));
	      stGeometry.nCylSlices = 1; 
	      
	      stGeometry.pCylSlice[0].Radius = distance; 
	      stGeometry.pCylSlice[0].Width  = dWidth;
	      stGeometry.pCylSlice[0].Height = dHeight;
	      stGeometry.pCylSlice[0].vCntr[0]  = 0.;
	      stGeometry.pCylSlice[0].vCntr[1]  = 0.;
	      stGeometry.pCylSlice[0].vCntr[2]  = 0.;
	      stGeometry.pCylSlice[0].vSymAxis[0]= 0;
	      stGeometry.pCylSlice[0].vSymAxis[1]= 0;
	      stGeometry.pCylSlice[0].vSymAxis[2]= 1;
	      stGeometry.pCylSlice[0].Phi = (iphi+itheta)/M_PI*180.;
	      stGeometry.pCylSlice[0].OpenAngle = dWidth/(2.*M_PI*distance)*360.;
	      
	      RotMatrixToAnglesZY(RotMatrixM, &ry, &rz);
	      fprintf(LogFilePtr,"For the cylindrical detector ry %f, rz %f", ry, rz);
	      if (rz < 0) rz += 2.*M_PI;
	      stGeometry.pCylSlice[0].Phi += rz/M_PI*180.;

	      stGeometry.pDescr  = "detector:cyan";
	      stGeometry.eModule = VT_DETECTOR;
	    }

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

	  bVisInstalled = TRUE;
	  // Geometry data
	  if (bVisInstr)
	    { 
	      stGeometry.pCuboid = (VtCuboid*) calloc(1, sizeof(VtCuboid));
	      stGeometry.nCuboids = 1; 
	      
	      
	      fprintf(LogFilePtr,"For the cylindrical detector iphi %f, itheta %f", iphi, itheta);

	      stGeometry.pCuboid[0].Length = thickness; 
	      stGeometry.pCuboid[0].Width  = dWidth;
	      stGeometry.pCuboid[0].Height = dHeight;
	      stGeometry.pCuboid[0].vCntr[0]  = distance*cos(itheta);
	      stGeometry.pCuboid[0].vCntr[1]  = distance*sin(itheta);
	      stGeometry.pCuboid[0].vCntr[2]  = 0;
	      stGeometry.pCuboid[0].vNormal[0]= cos(itheta);
	      stGeometry.pCuboid[0].vNormal[1]= sin(itheta)*cos(iphi);
	      stGeometry.pCuboid[0].vNormal[2]=  sin(itheta)*sin(iphi);
	      
	      stGeometry.pDescr  = "detector:cyan";
	      stGeometry.eModule = VT_DETECTOR;
	    }
	}
    } 
  else 
    {
      fprintf(LogFilePtr,"ERROR: You have to supply a detector geometry with -Gcub or -Gcyl!\n");
      exit(-1);
    }
  if(NoDetGrid) DetectorSpot=NoDetSpot;
  
  if (Eff.pfile!=NULL) {
      Eff.maxdata = LinesInFile(Eff.pfile);
      Eff.data = calloc(Eff.maxdata-1, sizeof(EffData));
      for(count=0; count < Eff.maxdata; count++) {
        ReadLine(Eff.pfile, sBuffer, sizeof(sBuffer)-1);
        sscanf(sBuffer, "%lf %lf", &Eff.data[count].Lambda, &Eff.data[count].Eff);
      }
      fclose(Eff.pfile);
    }
}

static FILE * tryOpen(const char *fn, const char *s) {
  FILE *f;
  char *fulln = FullParName(fn);
  f = fopen(fulln, "r");
  if (f) 
    return f;
  myExit2("ERROR: File %s containing %s could not be opened\n", fulln, s);
  return 0;
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

