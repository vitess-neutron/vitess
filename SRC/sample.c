/********************************************************************************************/
/*  VITESS module 'sample.c'                                                                */
/*    Some functions for sample_powder, sample_sans, etc.                                   */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors.                                                                          */
/*                                                                                          */
/********************************************************************************************/

#include <string.h>

#include "init.h"
#include "intersection.h"
#include "sample.h"
#include "matrix.h"
#include "message.h"


/******************************/
/**   Global Variables       **/
/******************************/
double MuTot=0.0, /* total macroscopic scattering cross-section (= sigma_tot/UCV) [1/cm] */
       MuAbs=0.0; /* macroscopic absorption cross-section       (= sigma_abs/UCV) [1/cm] */


/****************************************************************/
/* 'InitSample':   initializes structure 'SampleType'           */
/****************************************************************/
void InitSample  (SampleType* pSample)
{
  pSample->Position [2] = pSample->Position [1] = pSample->Position [0] = 0.0;
  pSample->Direction[2] = pSample->Direction[1] = pSample->Direction[0] = 0.0;
  pSample->SG.Cube.thickness=0.0;
  pSample->SG.Cube.height   =0.0;
  pSample->SG.Cube.width    =0.0;
}


/****************************************************************/
/* 'ReadCube'                                                   */
/*   reads information about a sample of cubic geometry         */
/*                                                              */
/* SampleFile: Pointer to the File                        (in)  */
/* Sample    : Information about geometry and orientation (out) */
/****************************************************************/
void ReadCube(FILE *SampleFile, SampleType *Sample)
{
  char Buffer[CHAR_BUF_LENGTH];
  char *helpptr;

  /* get next line from SampleFile */
  if (fgets(Buffer,CHAR_BUF_LENGTH,SampleFile)!=NULL) 
  {
    /* first strip any comment                  */
    helpptr= strchr(Buffer, '#');
    sscanf(Buffer,"%lf %lf %lf", &(Sample->SG.Cube.thickness),
                                 &(Sample->SG.Cube.height),
                                 &(Sample->SG.Cube.width));
    if (Sample->SG.Cube.thickness==0.0 || Sample->SG.Cube.height==0.0 || Sample->SG.Cube.width==0.0)
      Error("One of the lengths of the (cuboid) sample is not given or assigned to zero");
    if(helpptr!=NULL) *helpptr='\0';

    if (fgets(Buffer,CHAR_BUF_LENGTH,SampleFile)!=NULL) 
    {
      sscanf(Buffer, "%lf %lf %lf",
      &(Sample->Direction[0]), &(Sample->Direction[1]), &(Sample->Direction[2]));
      if (NormVector(Sample->Direction)==FALSE)
      Error("Sample direction is missing");
    } 
    else 
    {
      fprintf(LogFilePtr, "ERROR: orientation of the sample not found in %s\n", SampleFileName);
      exit(-1);
    }
  } 
  else 
  {
    fprintf(LogFilePtr,"ERROR: height, width and thickness of the cube not found in %s!\n", SampleFileName);
    exit(-1);
  }

  Sample->Type=VT_CUBE;

  SetSampleGeometry(Sample);
}


/****************************************************************/
/* 'ReadCylinder'                                               */
/*   reads information about a sample of cylindric geometry     */
/*                                                              */
/* SampleFile: Pointer to the File                        (in)  */
/* Sample    : Information about geometry and orientation (out) */
/****************************************************************/
void ReadCylinder(FILE *SampleFile, SampleType *Sample)
{
  char Buffer[CHAR_BUF_LENGTH];
  char *helpptr;

  /* get next line from SampleFile */
  if(fgets(Buffer,CHAR_BUF_LENGTH,SampleFile)!=NULL) 
  {
    /* first strip any comment                  */
    helpptr= strchr(Buffer, '#');
    if(helpptr!=NULL) *helpptr='\0';

    sscanf(Buffer,"%lf %lf", &(Sample->SG.Cyl.r), &(Sample->SG.Cyl.height));
    if (Sample->SG.Cyl.r==0.0 || Sample->SG.Cyl.height==0.0)
      Error("Height or radius of the (cylindrical) sample is not given or assigned to zero");
    if(fgets(Buffer,CHAR_BUF_LENGTH,SampleFile)!=NULL) 
    {
      sscanf(Buffer, "%lf %lf %lf", &(Sample->Direction[0]), &(Sample->Direction[1]), &(Sample->Direction[2]));
      if (NormVector(Sample->Direction)==FALSE)
        Error("Sample direction is missing");
    } 
    else 
    {
      fprintf(LogFilePtr, "ERROR: orientation of the sample not found in %s\n", SampleFileName);
      exit(-1);
    }
  } 
  else 
  {
    fprintf(LogFilePtr,"ERROR: radius and height of the cylinder not found in %s!\n",
    SampleFileName);
    exit(-1);
  }

  Sample->Type=VT_CYL;

  SetSampleGeometry(Sample);
}


/****************************************************************/
/* 'ReadBall'                                                   */
/*   reads information about a sample of spheric geometry       */
/*                                                              */
/* SampleFile: Pointer to the File                        (in)  */
/* Sample    : Information about geometry and orientation (out) */
/****************************************************************/
void ReadBall(FILE *SampleFile, SampleType *Sample)
{
	char Buffer[CHAR_BUF_LENGTH];
	char *helpptr;

	if(fgets(Buffer,CHAR_BUF_LENGTH,SampleFile)!=NULL) 
	{
		/* first strip any comment                  */
		helpptr= strchr(Buffer, '#');
		if(helpptr!=NULL) *helpptr='\0';
		sscanf(Buffer,"%lf", &(Sample->SG.Ball.r));
		if (Sample->SG.Ball.r==0.0)
			Error("Radius of the (spherical) sample is not given or assign to zero");
		Sample->Direction[0] = 0.0;
		Sample->Direction[1] = 0.0;
		Sample->Direction[2] = 1.0;
	}

	Sample->Type=VT_SPHERE;

	SetSampleGeometry(Sample);

}


// Set sample geometry for visualisation
void SetSampleGeometry(SampleType *Sample)
{
  switch(Sample->Type) 
  {
    case VT_CUBE:
      stGeometry.pCuboid = calloc(1, sizeof(VtCuboid));
      stGeometry.nCuboids = 1; 
      
      stGeometry.pCuboid[0].Length = Sample->SG.Cube.thickness; 
      stGeometry.pCuboid[0].Width  = Sample->SG.Cube.width;
      stGeometry.pCuboid[0].Height = Sample->SG.Cube.height;
      stGeometry.pCuboid[0].vCntr[0]  = Sample->Position[0];
      stGeometry.pCuboid[0].vCntr[1]  = Sample->Position[1];
      stGeometry.pCuboid[0].vCntr[2]  = Sample->Position[2];
      stGeometry.pCuboid[0].vNormal[0]= Sample->Direction[0];
      stGeometry.pCuboid[0].vNormal[1]= Sample->Direction[1];
      stGeometry.pCuboid[0].vNormal[2]= Sample->Direction[2];
      break;
      
    case VT_CYL:
      stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
      stGeometry.nCylinders = 1;

      stGeometry.pCylinder[0].Radius = Sample->SG.Cyl.r;
      stGeometry.pCylinder[0].Length = Sample->SG.Cyl.height;
      stGeometry.pCylinder[0].vCntr[0]  = Sample->Position[0];
      stGeometry.pCylinder[0].vCntr[1]  = Sample->Position[1];
      stGeometry.pCylinder[0].vCntr[2]  = Sample->Position[2];
      stGeometry.pCylinder[0].vSymAxis[0] = Sample->Direction[0];
      stGeometry.pCylinder[0].vSymAxis[1] = Sample->Direction[1];
      stGeometry.pCylinder[0].vSymAxis[2] = Sample->Direction[2];
      break;

    case VT_HOL_CYL:
      stGeometry.pCylinder = calloc(1, sizeof(VtCylinder));
      stGeometry.nCylinders = 1;

      stGeometry.pCylinder[0].Radius = Sample->SG.HCyl.r_out;
      stGeometry.pCylinder[0].Length = Sample->SG.HCyl.h_out;
      stGeometry.pCylinder[0].vCntr[0]  = Sample->Position[0];
      stGeometry.pCylinder[0].vCntr[1]  = Sample->Position[1];
      stGeometry.pCylinder[0].vCntr[2]  = Sample->Position[2];
      stGeometry.pCylinder[0].vSymAxis[0] = Sample->Direction[0];
      stGeometry.pCylinder[0].vSymAxis[1] = Sample->Direction[1];
      stGeometry.pCylinder[0].vSymAxis[2] = Sample->Direction[2];
      break;

    case VT_SPHERE:
      stGeometry.pSphere = calloc(1, sizeof(VtSphere));
      stGeometry.nSpheres = 1;

      stGeometry.pSphere[0].Radius = Sample->SG.Ball.r;
      stGeometry.pSphere[0].vCntr[0]  = Sample->Position[0];
      stGeometry.pSphere[0].vCntr[1]  = Sample->Position[1];
      stGeometry.pSphere[0].vCntr[2]  = Sample->Position[2];
       

    default: 
      stGeometry.pCuboid = calloc(1, sizeof(VtCuboid));
      stGeometry.nCuboids = 1; 
      
      stGeometry.pCuboid[0].Length = 2.; 
      stGeometry.pCuboid[0].Width  = 2.;
      stGeometry.pCuboid[0].Height = 2.;
      stGeometry.pCuboid[0].vCntr[0]  = 1;
      stGeometry.pCuboid[0].vCntr[1]  = 0;
      stGeometry.pCuboid[0].vCntr[2]  = 0;
      stGeometry.pCuboid[0].vNormal[0]= 1.;
      stGeometry.pCuboid[0].vNormal[1]= 0.;
      stGeometry.pCuboid[0].vNormal[2]= 0.;
      break;
  }
  return;
}


/****************************************************************/
/* 'CompPair'                                                   */
/* This function is needed by qsort to sort the StrucFac Array  */
/* remember in StrucFac[0][0] should be the largest d-spacing   */
/****************************************************************/
int CompPair(const void* p1, const void* p2)
{
  DoublePair* pOne = (DoublePair *)p1;
  DoublePair* pTwo = (DoublePair *)p2;

  if(*pOne[0] <  *pTwo[0]) return +1;
  if(*pOne[0] == *pTwo[0]) return  0;

  return -1; // *pOne[0] >  *pTwo[0]
}


/****************************************************************/
/* 'ReadTilComment'                                             */
/* This function reads one line of the parameter file and       */
/* strips the comment at the end beginning with '#'             */
/****************************************************************/
int ReadTilComment(char* pBuffer, FILE* pSampleFile)
{
  char* pHelp;
 
  /* read line */
  if(fgets(pBuffer, CHAR_BUF_LENGTH, pSampleFile)!=NULL)
  {	
    /* strip any comment 			*/
    pHelp = strchr(pBuffer, '#');
    if(pHelp != NULL) 
      *pHelp = '\0';

    return TRUE;
  }
  else 
  {	
    return FALSE;
  }
}


/*******************************************************************/
/* 'ProcessNeutronToEnd'                                           */
/* This function calculates the output data of a neutron (incl.    */
/* total scattering probability) after the scattering in a sample  */
/*                                                                 */
/* Neut         : information about the current neutron            */
/* SP           : scattering point                                 */
/* Ls           : path length inside sample before scattering      */
/* DetFac       : Probability by detector coverage                 */
/* ScProb       : Probability of the scattering process itself     */
/* OutTheta,                                                       */
/* OutPhi       : New flight direction in the new coordinte system */
/* Sample       : holds geometry data about the sample             */
/* SampleRotNeut: is a matrix that rotates the old to the new      */
/*                neutron coordinate system                        */
/* RotMatrixSmpl: is a matrix that rotates the sample              */
/*                in its position (from the (1,0,0) direction      */
/*******************************************************************/
void ProcessNeutronToEnd(Neutron *Neut, VectorType SP, double Ls,
                        double DetFac, double ScProb, double ScTheta,
                        double ScPhi, SampleType *Sample,
                        double RotMatrixNeut[3][3], double RotMatrixSmpl[3][3])
{
  Neutron    OutNeut;
  double     Las, t, Atten, Lbs;
  VectorType OutISP[2];
  long       nisp;

  /* Determine the flight length of the neutron before scattering */
  Lbs = DistVector(Neut->Position, SP); /*including the distance to the sample*/

  /* initialize output data of the neutron */
  memcpy(&OutNeut, Neut, sizeof(Neutron));

  /* put the neutron to the scattering point */
  CopyVector(SP,OutNeut.Position);

  /* calculate the new direction in the neutron frame */
  OutNeut.Vector[0]= cos(ScTheta);
  OutNeut.Vector[1]= sin(ScTheta)*cos(ScPhi);
  OutNeut.Vector[2]= sin(ScTheta)*sin(ScPhi);

  /* bring the direction to the original co-ordinate system */
  RotBackVector(RotMatrixNeut, OutNeut.Vector);

  /* ok the neutron is at SP and has its new Direction */
  /* find the intersections with the sample walls      */
  if(NeutronIntersectsSample(&OutNeut, Sample, RotMatrixSmpl, OutISP, &nisp, VT_INSIDE)) 
  {
    /* Distance between SP and OutISP, Length atfter scattering */
    Las = DistVector(SP, OutISP[1]);

    /* ok, neutron leaves the sample at OutISP[1] */
    CopyVector(OutISP[1],OutNeut.Position);

    /* it takes t sec to travel through the sample */
    t = (Las+Lbs)/V_FROM_LAMBDA(OutNeut.Wavelength);

    /* and the neutron may be attenuated                               */
    /* also scale MuAbs for the neutrons velocity                        */
    /* MuAbs is proportional to 1/v, i.e. proportional to the wavelength */
    /* reference wavelength is usually 1.798 Ang                       */
    Atten = exp(-(Las+Ls)*(MuTot + MuAbs*OutNeut.Wavelength/1.798));

    /* now put all together  */
    OutNeut.Time        = Neut->Time+t;
    OutNeut.Probability = Neut->Probability*DetFac*Atten*ScProb;

    /* write the Neutron to the output file   */
    WriteNeutron(&OutNeut);
    WriteIAP(&OutNeut, VT_SCATTERED);
  } 
  else 
  { 
    /* Uhh, here is something terribly wrong */
    // Error("(internal): neutron leaves the sample without intersecting its wall");
    CountMessageID(ENV_TRAJ_OUTSIDE, OutNeut.ID);
  }
}


/****************************************************************/
/* 'NeutronIntersectsSample'                                    */
/*   tests whether the neutron hits the sample                  */
/*                                                              */
/* returns TRUE =1 if neutron intersects,                       */
/*         FALSE=0 otherwise                                    */
/*                                                              */
/* Nin             carries information about the current neutron*/
/* Sample          holds geometry data about the sample         */
/* SampleRotMatrix is a matrix that rotates the sample          */
/*                 in its position (from the (1,0,0) direction  */
/* ISP             denotes the intersection points              */
/*                 it is given in the input coordinate system   */
/* nisp            number of intersections to come (with t > 0) */
/****************************************************************/
long NeutronIntersectsSample(const Neutron *Nin, SampleType* pSample,
                             double SampleRotMatrix[3][3], VectorType ISP[2],
                             long* pNisp, VtDir eDir) 
{
  double t[2];
  VectorType Position, Direction;
  long j, rc=FALSE;

  CopyVector(Nin->Position, Position);
  CopyVector(Nin->Vector, Direction);

  /* Rotation of the sample to the neutron's frame  */
  if (pSample->Type != VT_SPHERE)
  {	
    RotBackVector(SampleRotMatrix, Position);
    RotBackVector(SampleRotMatrix, Direction);
  }

  /* Searching the distances t0 and t1 to the intersection points with the sample */
  switch (pSample->Type)
  {	
    case VT_CUBE   : rc=LineIntersectsCube     (Position, Direction, &(pSample->SG.Cube), t); break;
    case VT_CYL    : rc=LineIntersectsCylinder (Position, Direction, &(pSample->SG.Cyl ), t); break;
    case VT_SPHERE : rc=LineIntersectsSphere   (Position, Direction, &(pSample->SG.Ball), t); break;
    case VT_HOL_CYL: rc=LineIntersectsHollowCyl(Position, Direction, &(pSample->SG.HCyl), t, eDir); break;
    default:      Error("Sample type unknown");
  }

  if(rc==TRUE) 
  {	
    /*   calculating of the intersection points ISP0 and ISP1 
    t0: entering the sample, t1: leaving the sample        */
    for(j=0; j<3; j++)
    {	
      ISP[1][j]=Nin->Position[j]+t[1]*Nin->Vector[j];
      ISP[0][j]=Nin->Position[j]+t[0]*Nin->Vector[j];
    }
    /* calculating the number of intersection points to come
    (result of LineIntersects.... are ordered t0 < t1  */
    *pNisp = 0;
    if (t[1] >= 0.0) (*pNisp)++;
    if (t[0] >= 0.0) (*pNisp)++;
    return TRUE;
  }
  else
  {	
    return FALSE;
  }
}


/************************************************************************************/
/** Reads structure factor file                                                    **/
/**   tag 1: returns  d-spacing and |F|²   (for powder sample)                     **/
/**   tag 2: returns  d, h,k,l, and |F|²   (for single crystal sample)             **/
/**                                                                                **/
/** analyzes extension .str .lau and .laz to read parameters from the right column **/
/************************************************************************************/
int ReadStructureFile(const char* sStructFile, int tag, DoublePair* structFactorLookup[])
{
  FILE*   pStrucFile;
  long    NumLines, i, j;
  char    sBuffer[CHAR_BUF_LENGTH];
  int     lenFilename;
  int     maxColumn;
  double* parBuffer;
  
  /* first open the file, exit if opening fails */
  pStrucFile = OpenInputFile2(sStructFile, "structure factor data", "rt"); 

  /* count lines in file */
  NumLines = LinesInFile(pStrucFile);

  /* get memory for StrucFac */
  if (tag == 1) 
  {
    if((*structFactorLookup = (DoublePair*) calloc(NumLines, sizeof(DoublePair)))==NULL)
    { 
      fprintf(LogFilePtr,"ERROR: Can't allocate memory for structure factor data\n");
      exit(-1);
    }
  }
  else if (tag == 2) 
  {
    hVal = (double*) calloc(NumLines, sizeof(double));
    kVal = (double*) calloc(NumLines, sizeof(double));
    lVal = (double*) calloc(NumLines, sizeof(double));
    F2Val = (double*) calloc(NumLines, sizeof(double));
  }
  else 
  { 
    fprintf(LogFilePtr,"ERROR: Unknown sample type!\n");
    exit(-1);
  }

  /* get back to the start of the File */
  rewind(pStrucFile);
  
  lenFilename = strlen(sStructFile);

  if (strstr(sStructFile, ".str") == &sStructFile[lenFilename-4]) 
  {
    colD = 1;
    colF2 = 2;
    maxColumn = 2;
  }
  else if (strstr(sStructFile, ".laz") == &sStructFile[lenFilename-4]) 
  {
    colh = 1;
    colk = 2;
    coll = 3;
    colD = 6;
    colF = 13;
    colM = 17;
    maxColumn = 18;
  }
  else if (strstr(sStructFile, ".lau") == &sStructFile[lenFilename-4]) 
  {
    colh = 1;
    colk = 2;
    coll = 3;
    colM = 4;
    colD = 5;
    colF2 = 7 ;
    scaleF2 = 1./100.; // conversion from fm^2 to barn
    maxColumn = 7;
  }
  else if (((colD > 0 && tag==1) || (colh > 0 && colk > 0 && coll > 0 && tag == 2))  && (colF > 0 || colF2 > 0)) 
  {
    maxColumn = Max(Max(Max(Max(Max(colD, colF), colF2), colM), colDW), Max(colh, Max(colk, coll)));
  }
  else 
  {
    fprintf(LogFilePtr,"ERROR: Unknown structure file format! Use .dat, .str, .laz, .lau or specify the meaning of the individual columns!\n");
    exit(-1);
  }

  parBuffer = (double*) calloc(maxColumn, sizeof(double));

  /* and read the data */
     
  for(i=0; i < NumLines; i++)  
  { 
    char format[1024] = "%lf ";
    const char* sRemainBuffer;

    ReadLine(pStrucFile, sBuffer, sizeof(sBuffer)-1);        

    sscanf  (sBuffer, format, &parBuffer[0]);    
    sRemainBuffer = sBuffer;         

    for (j=1; j < maxColumn; j++) 
    {
      strncpy(&format[strlen(format)-4], "%*lf ", 5);
      strcat(format, "%lf ");
      sscanf(sRemainBuffer, format, &parBuffer[j]);
    }
      
    if (tag == 1) 
    {
      (*structFactorLookup)[i][0] = parBuffer[colD-1];      
      if (colF2 > 0)  
      {
        (*structFactorLookup)[i][1] = parBuffer[colF2 -1]*scaleF2;
      }
      else if (colF > 0)
      { (*structFactorLookup)[i][1] = sq(parBuffer[colF -1])*scaleF2;
      }

      if (colDW > 0) (*structFactorLookup)[i][1] *= parBuffer[colDW -1];
      // powder sample
      if (colM > 0) (*structFactorLookup)[i][1] *= parBuffer[colM -1];
      
      //      for (j=0; j < maxColumn; j++) fprintf(LogFilePtr,"%f ", parBuffer[j]); 
    }
    else 
    {
      hVal[i] = parBuffer[colh -1];
      kVal[i] = parBuffer[colk -1];
      lVal[i] = parBuffer[coll -1];
      
      if (colF2 > 0) 
        F2Val[i] = parBuffer[colF2 -1]*scaleF2;
      else 
        F2Val[i] = sq(parBuffer[colF -1])*scaleF2;
      
      if (colDW > 0) F2Val[i] *= parBuffer[colDW -1];
    }
  }

  fclose(pStrucFile);
  fprintf(LogFilePtr,"Read %ld lines from the structure factor file %s.\n", NumLines, sStructFile);

  if (tag == 1) 
  {
    qsort((void *)*structFactorLookup, (size_t) NumLines, sizeof(DoublePair), CompPair);
    /* Sum up all equal d-spacings */
    i=0;
    for(j=1; j<NumLines; j++)
    { if((*structFactorLookup)[j][0]!=(*structFactorLookup)[j-1][0])
      { i++;
        (*structFactorLookup)[i][0]=(*structFactorLookup)[j][0];
        (*structFactorLookup)[i][1]=(*structFactorLookup)[j][1];
      } 
      else
      { (*structFactorLookup)[i][1]+= (*structFactorLookup)[j][1];
      }
    }
    NumLines = i+1;
  }

  return NumLines;
}
