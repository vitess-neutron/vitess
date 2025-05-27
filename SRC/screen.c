/*********************************************************************************************/
/*  VITESS module 'screen'                                                                   */
/*                                                                                           */
/* This module simulates propagation to a rectangular or cylindrical area (about z-axis)     */
/*         and saved binned PSD data                                                         */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Oct 2023  K. Lieutenant  initial version                                             */
/* 1.0a Jan 2025  K. Lieutenant  no suppression of backscattering + better criterion for wall*/
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "intersection.h"
#include "mon2_header.h"


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and initializes global variables
void  SetGeometry(char* sColor);            // fills the structure stGeometry for visualization
void  UpdateMon(long iBnch);                // Updates monitor output file 


/******************************/
/** Global Variables         **/
/******************************/
char*  OutFileName=NULL;       // -O   [-]   output file name to store the 2D PSD data
VtDetGeom  eGeom;              // -G   [-]   geometry 1: cyl     2: flat   
VtFormat2D eFormat = MATRIX;   // -F    [-]   file format for output:  MATRIX: 2D matrix  XYZ: xyz  MATR_CMPT: 2D matrix compact  XYZ_CMPT xyz compact
double Width =0.0,             // -w   [cm]  width of the (rectangular) area 
       Height=0.0,             // -h   [cm]  height of the area 
       AngleMin=0.0,           // -a  [deg]  min. angle in horizontal plane
       AngleMax=0.0,           // -A  [deg]  max. angle in horizontal plane
       Distance=0.0;           // -D   [cm]  distance between starting point and area
long   nBinsY  =1,             // -y   [-]   number of bins in horizontal direction
       nBinsZ  =1;             // -z   [-]   number of bins in vertical direction

// Variables that are fixed or determined from input parameters or trajectory data
short    bProbactiv=TRUE;      //      [-]   flag Display  : YES: Probability weight   NO: number of trajectories
double   WidthMin  =0.0,       //      [cm]  horizontal range of the screen
         WidthMax  =0.0,       
         HeightMin =0.0,       //      [cm]  vertical range of the screen
         HeightMax =0.0;        
Plane    Endpoint;             //      [cm]  Endpoint.D: distance to end of free flight path along x-axis [cm]
long     nBunches = 1;         //            number of bunches started
double*  BinPosY  = NULL;      //            edges of the bins of the first parameter
double*  BinPosZ  = NULL;      //            edges of the bins of the second parameter
double** IntYZ    = NULL;      //            intensity within a bin (in 2 dimensions) 
double** IntYZError= NULL;     //            standard deviation of this intensity 
long  ** nTrajYZ   = NULL;     //            number of trajectories within a bin
long     nTrajTot =0;          //            total number of traj. within monitor limits
double   TotInt   =0.0;        //            total intensitiy within monitor limits


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
	long   i=0,
         iY=0, jZ=0,                // indices of matrix
         iBnch=0;                   // current bunch
	double Velocity=0.0,              // velocity of the neutron    
         TimeOF=0.0,                // time of flight of the neutron to the window 
         Angle=0.0;                 // angle of detection

  // Initialisation
  // --------------
  _eModule = MCN_SCREEN;

	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.0a");
	OwnInit(argc, argv);

  bVisInstalled = TRUE;
  if (bVisInstr) 
    bBlowUp = TRUE;

  nBunches = ReadNumBnch();

	DECLARE_ABORT

  // Loop over all trajectories
  // --------------------------
	while (ReadNeutrons()!= 0)
	{
		for (i=0; i<NumNeutGot; i++)
		{
			CHECK

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        iBnch++;
        UpdateMon(iBnch);
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
			  // 	Check parameters of the trajectory 
			  if (InputNeutrons[i].Wavelength == 0.0) continue;
			  Velocity = (V_FROM_LAMBDA(InputNeutrons[i].Wavelength)); 
			  if (Velocity <= 0.0) continue;

			  if (eGeom==VT_DET_FLAT)
        {
			    // 	Move neutron to end of the area and calculate Time of Flight (ms)
			    if (keygrav == 1)
				    TimeOF = NeutronPlaneIntersectionGrav(&InputNeutrons[i], Endpoint);
			    else
				    TimeOF = NeutronPlaneIntersection1(&InputNeutrons[i], Endpoint);

			    // Calculate  and  writeout new data set, if area is hit
			    if (fabs(InputNeutrons[i].Position[1]) < 0.5*Width  &&  fabs(InputNeutrons[i].Position[2]) < 0.5*Height)
			    {	
	          iY = (int)floor(nBinsY*(InputNeutrons[i].Position[1]-WidthMin) /(WidthMax -WidthMin));
	          jZ = (int)floor(nBinsZ*(InputNeutrons[i].Position[2]-HeightMin)/(HeightMax-HeightMin));
			
	          if (iY >= 0 && iY < nBinsY && jZ >= 0 && jZ < nBinsZ)
	          {	
	            nTrajYZ[iY][jZ]++;
              nTrajTot++;
	            IntYZ  [iY][jZ]+= InputNeutrons[i].Probability;
	            TotInt         += InputNeutrons[i].Probability;
	          }
				    InputNeutrons[i].Time += TimeOF;
            WriteIAP(&InputNeutrons[i], VT_DETECTED);

				    // InputNeutrons[i].Position[0]=0.0;

				    WriteNeutron(&InputNeutrons[i]);
			    }
          else
          { WriteIAP(&InputNeutrons[i], VT_OUT_OF_WND);
          }
        }
        else if (eGeom==VT_DET_CYL)
        {
          VectorType CylDetector={0.0,0.0,0.0}, 
                     Pos1={0.0,0.0,0.0}, Pos2={0.0,0.0,0.0};
          CylDetector[0] = 2.0*Distance;
          CylDetector[2] = Height;

          // Calculate intersection point with vertical cylinder centered around the sample center and scattering angle
          IntersectionWithCylinder(CylDetector, InputNeutrons[i].Position, InputNeutrons[i].Vector, Pos1, Pos2);
          TimeOF = DistVector(Pos2, InputNeutrons[i].Position)/Velocity;
          Angle  = Degrees(atan2(Pos2[1], Pos2[0]));

			    // Set as new data set, if cylinder wall is hit
          if (fabs(Pos2[2]) < 0.49999*Height && Angle > AngleMin && Angle < AngleMax)
          { 
            CopyVector(Pos2, InputNeutrons[i].Position);

	          iY = (int)floor(nBinsY*(Angle-AngleMin) /(AngleMax -AngleMin));
	          jZ = (int)floor(nBinsZ*(InputNeutrons[i].Position[2]-HeightMin)/(HeightMax-HeightMin));
			
	          if (iY >= 0 && iY < nBinsY && jZ >= 0 && jZ < nBinsZ)
	          {	
	            nTrajYZ[iY][jZ]++;
              nTrajTot++;
	            IntYZ  [iY][jZ]+= InputNeutrons[i].Probability;
	            TotInt         += InputNeutrons[i].Probability;
	          }

				    InputNeutrons[i].Time += TimeOF;

            WriteIAP(&InputNeutrons[i], VT_DETECTED);
				    WriteNeutron(&InputNeutrons[i]);
			    }
          else
          { WriteIAP(&InputNeutrons[i], VT_OUT_OF_WND);
          }
        }
      }
		}
	}	

// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
  // writes final monitor output
  UpdateMon(nBunches);  

  // write to log file
  if (eGeom==VT_DET_CYL)
  { fprintf(LogFilePtr, "Cylindrical screen of %6.2f cm  height and %6.2f cm radius\n",  Height, Distance);
    fprintf(LogFilePtr, "      centered around %6.2f deg   covering %6.2f deg \n", (AngleMax+AngleMin)/2.0, AngleMax-AngleMin);
  }
  else if (eGeom==VT_DET_FLAT)
  { fprintf(LogFilePtr, "Rectangular screen of size %6.2f x %6.2f cm (W x H) in a distance of %7.2f cm \n", Width, Height, Distance);
  }

  // write geometry data for visualization
  SetGeometry("cyan");                      

  // print intensity, write instrument.inf, free memory
  // the origin of the co-ordinate system remains at the sample  
  Cleanup(0.0,0.0,0.0, 0.0,0.0);    

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
	int i=0, 
      iY=0, jZ=0;   // indices of matrix

  InitPlane(&Endpoint);

	for (i=1; i<argc; i++)
	{
		if (argv[i][0]!='+') 
		{
			switch(argv[i][1])
      {
        case 'O':
          OutFileName=(&argv[i][2]);
          break;
        case 'G':
          eGeom = (VtDetGeom) atoi(&argv[i][2]); // 1: cylindrical   2: flat
          break;
        case 'F':
          eFormat = (VtFormat2D) atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz, gnuplot readable */
          break;

				case 'w':
					Width  = atof(&argv[i][2]);
					break;
				case 'h':
					Height = atof(&argv[i][2]);
					break;
				case 'a':
					AngleMin  = atof(&argv[i][2]);
					break;
				case 'A':
					AngleMax  = atof(&argv[i][2]);
					break;
				case 'D':
					Distance = atof(&argv[i][2]);
					break;

	      case 'y':
	        nBinsY = atol(&argv[i][2]); /* number of bins y-direction */
	        break;
	      case 'z':
	        nBinsZ = atol(&argv[i][2]); /* number of bins, z-direction */
	        break;
      
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}

  // Check geometry
  if (eGeom!=VT_DET_CYL && eGeom!=VT_DET_FLAT)
    Error("Wrong value for geometry of the screen");

  // Define plane through flat detector
	Endpoint.A =  1.0;
	Endpoint.D = -1.0*Distance;
  
  // Allocate memory for the monitor data
  BinPosY    = (double*)  malloc((nBinsY+1) * sizeof(double));
  BinPosZ    = (double*)  malloc((nBinsZ+1) * sizeof(double));
  IntYZ      = (double**) malloc(nBinsY * sizeof(double*));
  IntYZError = (double**) malloc(nBinsY * sizeof(double*));
  nTrajYZ    =   (long**) malloc(nBinsY * sizeof(long*));

  for (iY=0; iY < nBinsY; iY++) 
  {
    IntYZ     [iY] = (double*) malloc(nBinsZ * sizeof(double));
    IntYZError[iY] = (double*) malloc(nBinsZ * sizeof(double));
    nTrajYZ   [iY] = (long*)   malloc(nBinsZ * sizeof(long));
  }

  // initializes arrays
  if (eGeom==VT_DET_CYL)
  { for (iY=0; iY <= nBinsY; iY++) 
      BinPosY[iY] = AngleMin  +  (AngleMax-AngleMin)  * iY / (double)nBinsY;
  }
  else 
  { WidthMin = -Width/2.0; 
    WidthMax =  Width/2.0;
    for (iY=0; iY <= nBinsY; iY++)
      BinPosY[iY] = WidthMin  +  (WidthMax-WidthMin)  * iY / (double)nBinsY;
  }

  HeightMin = -Height/2.0; 
  HeightMax =  Height/2.0;
  for (jZ=0; jZ <= nBinsZ; jZ++) 
    BinPosZ[jZ] = HeightMin + (HeightMax-HeightMin) * jZ / (double)nBinsZ;

  for(iY=0; iY < nBinsY; iY++)
  { for(jZ=0; jZ < nBinsZ; jZ++)
	  {
	    IntYZ[iY][jZ] = 0.0;
	    IntYZError[iY][jZ] = 0.0;
	    nTrajYZ   [iY][jZ] = 0;
	  }
  }

  return;
}


/*******************************************************/
/** fills the structure stGeometry for visualization  **/
/*******************************************************/
void SetGeometry(char* sColor)
{
  // Visualisation of the slit geometry
  if (bVisInstr)
  {
    sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  =  sVisDescrpt;
    stGeometry.eModule = _eModule;

    if (eGeom==VT_DET_FLAT)
    { 
      stGeometry.pRectangle = calloc(1, sizeof(VtRectangle));
      stGeometry.nRectangles = 1;

      stGeometry.pRectangle[0].Width      = BlowUp * Width;
      stGeometry.pRectangle[0].Height     = BlowUp * Height;
      stGeometry.pRectangle[0].rotAngle   = 0.0;
      stGeometry.pRectangle[0].vCntr[0]   = Distance;
      stGeometry.pRectangle[0].vCntr[1]   = 0.0;
      stGeometry.pRectangle[0].vCntr[2]   = 0.0;
      stGeometry.pRectangle[0].vNormal[0] = 1.0;
      stGeometry.pRectangle[0].vNormal[1] = 0.0;
      stGeometry.pRectangle[0].vNormal[2] = 0.0;
    }
    if (eGeom==VT_DET_CYL)
    { 
      stGeometry.nCylSlices = 1; 
      stGeometry.pCylSlice = (VtCylSlice*) calloc(stGeometry.nCylSlices, sizeof(VtCylSlice));

      stGeometry.pCylSlice[0].Radius     = Distance; 
      stGeometry.pCylSlice[0].Width      = Width;
      stGeometry.pCylSlice[0].Height     = BlowUp * Height;
      stGeometry.pCylSlice[0].Phi        =(AngleMax + AngleMin)/2.0;
      stGeometry.pCylSlice[0].OpenAngle  = Min(AngleMax - AngleMin, 359.9);
      stGeometry.pCylSlice[0].vCntr[0]   = 0.0;
      stGeometry.pCylSlice[0].vCntr[1]   = 0.0;
      stGeometry.pCylSlice[0].vCntr[2]   = 0.0;
      stGeometry.pCylSlice[0].vSymAxis[0]= 0.0;
      stGeometry.pCylSlice[0].vSymAxis[1]= 0.0;
      stGeometry.pCylSlice[0].vSymAxis[2]= 1.0;
    }
  }

  return;
}


/*******************************************************/
/**  Updates main monitor output file                 **/
/*******************************************************/
void UpdateMon(long iBnch)
{
  int    iBinY=0, jBinZ=0;            // matrix indices
  double f_norm  = 1.0;               // ratio of total to processed bunches after treating current bunch
  FILE*  fMonitor= NULL;              // pointer to output file

  // opens monitor file
  fMonitor = OpenOutputFile(OutFileName, TRUE, "wt");

  if (fMonitor)
  {
    // normalize according number of bunches simulated
    if (iBnch > 0 && nBunches > 1)
      f_norm = (double) nBunches / (double) iBnch;

    // calculate standard deviation
    for (iBinY = 0; iBinY < nBinsY; iBinY++) 
    { for (jBinZ = 0; jBinZ < nBinsZ; jBinZ++) 
      {
        if (nTrajYZ[iBinY][jBinZ] > 0) 
          IntYZError[iBinY][jBinZ] = IntYZ[iBinY][jBinZ] / sqrt(nTrajYZ[iBinY][jBinZ]);
        else 
          IntYZError[iBinY][jBinZ] = 0.0;
      }
    } 

    // writes header and data
    if (eGeom==VT_DET_CYL)
      WriteHeader2DB(fMonitor, FALSE, eFormat, "Intensity", bProbactiv, iBnch, nBunches, TotInt, nTrajTot,   
                     nBinsY, "scat_angle [deg]", AngleMin,  AngleMax,    
                     nBinsZ, "pos_z [cm]", HeightMin, HeightMax);
    else
      WriteHeader2DB(fMonitor, FALSE, eFormat, "Intensity", bProbactiv, iBnch, nBunches, TotInt, nTrajTot,   
                     nBinsY, "pos_y [cm]", WidthMin,  WidthMax,    
                     nBinsZ, "pos_z [cm]", HeightMin, HeightMax);

    WriteOutput2DB(fMonitor, eFormat, bProbactiv,  
                   nBinsY, BinPosY,   nBinsZ, BinPosZ,  f_norm,
                   IntYZ, IntYZError, nTrajYZ);

    fclose(fMonitor);
  }
}
