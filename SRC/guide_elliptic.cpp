#ifndef GUIDE_ELLIPTIC_CPP
#define GUIDE_ELLIPTIC_CPP

/********************************************************************************************/
/*  VITESS module 'guide_elliptic.cpp'                                                      */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Oct 2012  D. Nekrassov  initial version                                              */
/* 1.1 Nov 2013  D. Nekrassov  added m-values as input parameters                           */
/********************************************************************************************/

#include <math.h>

#define thetaCNi 0.099138

#include "guide_elliptic.h"


int main(int argc, char *argv[])
{
 
  long	i, BufferIndex;
  
  int registered = 0;

  BufferIndex = 0;
  registered=0;

  Init(argc, argv, VT_GUIDE);
  print_module_name("guide elliptic 1.0");

  OwnInit(argc, argv);

  /*************************************************************/
  DECLARE_ABORT;

  int neutronsLost = 0;

  while(ReadNeutrons()!= 0)
  {
  CHECK;
  for(i=0; i<NumNeutGot; i++)
	{
	  CHECK;

	  //	  fprintf(LogFilePtr,"Neutron number read: %d, wavelength %f", i, InputNeutrons[i].Wavelength);

	  registered=ProcessNeutron(&InputNeutrons[i]);

	  if(registered==1) 
	    WriteNeutron(&(InputNeutrons[i]));
	  else neutronsLost++;
	}
  }
  
  fprintf(LogFilePtr,"Neutrons lost straight: %d \n", neutronsKilledStraight);
  fprintf(LogFilePtr,"Neutrons lost parabolic: %d \n", neutronsKilledParabolic);
  fprintf(LogFilePtr,"Simultaneous collisions: %d \n", simultaneousCollisions);
  fprintf(LogFilePtr,"Bad neutrons: %d \n", badNeutrons);

 my_exit:

  OwnCleanup();

  Cleanup(lengthGuide*100.,0.0,0.0, 0.0,0.0);
  
  return 1;

}


void OwnInit(int argc, char *argv[])
{

   for(int i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {
	  case 'O':
	    if((fShapeFilePointer = fopen(&argv[i][2],"w"))==NULL)  //File with shape informations
	      {
		fprintf(LogFilePtr,"\nFile %s could not be opened for elliptic guide shape output\n",&argv[i][2]);
		exit(-1);
	      }
	    shapeFile=&argv[i][2];
	    break;

	  case 'V':
	    shapeVer = atoi(&argv[i][2]); // Vertical guide shape: 0=constant, 1=straight, 2=elliptic 
	    break;
	  
	  case 'H':
	    shapeHor = atoi(&argv[i][2]); // Horizontal guide shape: 0=constant, 1=straight, 2=elliptic
	    break; 

	  case 'a':
	    longAxisHor = atof(&argv[i][2]); //Length of long axis in horizontal plane in m
	    break;

	  case 'A':
	    longAxisVer = atof(&argv[i][2]); //Length of long axis in vertical plane in m
	    break;

	  case 'b':
	    shortAxisHor =  atof(&argv[i][2]); //Length of short axis in horizontal plane in m
	    break;
	  case 'B':
	    shortAxisVer = atof(&argv[i][2]);	//Length of short axis in vertical plane in m
	    break;

	  case 'u':
	    startHeight =  atof(&argv[i][2]);   // Entrance height in cm
	    //startHeight /= 100.; // convert in m
	    break;
	  case 'U':
	    endHeight = atof(&argv[i][2]);	// Exit height in cm
	    //endHeight /= 100.; // convert in m
	    break;

	  case 'w':  
	    startWidth = atof(&argv[i][2]); // Entrance width in cm
	    //startWidth /= 100.; // convert in m
	    break;

	  case 'W':  
	    endWidth = atof(&argv[i][2]); // Exit width in cm
	    //endWidth /= 100.; // convert in m
	    break;

	  case 'l':
	    lengthGuide  = atof(&argv[i][2]); // Guide length in m
	    break;

	 case 'd':
	    distToFocusHor  = atof(&argv[i][2]); // Distance to focal point from guide exit
	    break; 

	 case 'D':
	    distToFocusVer  = atof(&argv[i][2]); // Distancs to focal point from guide exit
	    break; 

	  case 'C':
	    changeColor = atoi(&argv[i][2]); // Modify color with each reflection
	    break;

	  case 'i':  /* left plane */
	    if( (fReflFileLeftPointer = fopen(FullParName(&argv[i][2]),"r"))==NULL) //Reflectivity file left plane
          {	fprintf(LogFilePtr,"ERROR: File %s containing coating of left plane could not be opened\n",&argv[i][2]);
            exit(-1);
          }
          reflFileNameLeft=(std::string) &argv[i][2];
	  reflContainer[0].pfile = fReflFileLeftPointer;
	  reflContainer[0].filename = reflFileNameLeft.c_str();
	  LoadReflFile(&reflContainer[0]);
          break;

	  case 'e':  /* left plane */
	    mNumber[0] = atof(&argv[i][2]);  	    
	    break;

        case 'I':  /* right plane */
          if( (fReflFileRightPointer = fopen(FullParName(&argv[i][2]),"r"))==NULL) //Reflectivity file right plane
          {	fprintf(LogFilePtr,"ERROR: File %s containing coating of right plane could not be opened\n",&argv[i][2]);
            exit(-1);
          }
          reflFileNameRight=(std::string) &argv[i][2];
	  reflContainer[1].pfile = fReflFileRightPointer;
	  reflContainer[1].filename = reflFileNameRight.c_str();
	  LoadReflFile(&reflContainer[1]);
          break;
	  
	  case 'E':  /* right plane */
	    mNumber[1] = atof(&argv[i][2]);  
	    break;
	    
	  case 'j':    /* top plane */
          if( (fReflFileTopPointer = fopen(FullParName(&argv[i][2]),"r"))==NULL) //Reflectivity file top plane
          {
            fprintf(LogFilePtr,"ERROR: File %s containing coating of top plane could not be opened\n",&argv[i][2]);
            exit(-1);
          }
          reflFileNameTop=(std::string) &argv[i][2];
	  reflContainer[2].pfile = fReflFileTopPointer;
	  reflContainer[2].filename = reflFileNameTop.c_str();
	  LoadReflFile(&reflContainer[2]);
          break;

	  case 'f':  /* top plane */
	    mNumber[2] = atof(&argv[i][2]);  
	    break;
	    
        case 'J':    /* bottom plane */
          if( (fReflFileBottomPointer = fopen(FullParName(&argv[i][2]),"r"))==NULL)  //Reflectivity file bottom plane
          {
            fprintf(LogFilePtr,"ERROR: File %s containing coating of bottom plane could not be opened\n",&argv[i][2]);
            exit(-1);
          }
          reflFileNameBottom=(std::string) &argv[i][2];
	  reflContainer[3].pfile = fReflFileBottomPointer;
	  reflContainer[3].filename = reflFileNameBottom.c_str();
	  LoadReflFile(&reflContainer[3]);
          break;

        case 'F':  /* bottom plane */
	  mNumber[3] = atof(&argv[i][2]);  
	  break;

	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;
	  }
      }
    }

   if (!reflContainer[0].pfile)
     FillReflContainer(&reflContainer[0], mNumber[0]);
   if (!reflContainer[1].pfile)
     FillReflContainer(&reflContainer[1], mNumber[1]);
   if (!reflContainer[2].pfile)
     FillReflContainer(&reflContainer[2], mNumber[2]);
   if (!reflContainer[3].pfile)
     FillReflContainer(&reflContainer[3], mNumber[3]);
   

   if (distToFocusVer < 0) distToFocusVer = distToFocusHor;

   //Check if enough parameters were given
   //Treat horizontal plane first
   if (shapeHor == 2) { // We are dealing with an ellipse

     if (longAxisHor > 0 && shortAxisHor > 0 && lengthGuide > 0) {

       if (startWidth > 0 || endWidth > 0) {
	 fprintf(LogFilePtr,"Ambiguous input for horizontal plane, please specify a) either entrance AND exit parameters or b) the size of the axes. \n");
	 fprintf(LogFilePtr,"Long axis: %f, start width: %f, end width: %f \n", longAxisHor, startWidth, endWidth);
	 exit(-1);
       }
       else {
	 double x = lengthGuide/2.;
	 startPoint = x*(-1.);
	 endPoint = x;
	 startWidth = (shortAxisHor*sqrt(1. - x*x/(longAxisHor*longAxisHor))*2.)*100.;
	 endWidth = startWidth;
       }

     }
     else if (lengthGuide > 0 && startWidth > 0 && endWidth > 0 && distToFocusHor > 0) {
     
       if (longAxisHor > 0 || shortAxisHor > 0) {
	 fprintf(LogFilePtr,"Ambiguous input for horizontal plane, please specify either a) entrance/exit parameters or b) the size of the axes.");
	 fprintf(LogFilePtr,"Lond axis: %f, short axis: %f", longAxisHor, shortAxisHor);
	 exit(-1);
       }
       else {
          
	 if (!CalculateEllipseParametersFromStartAndExitWidths(startWidth, endWidth, lengthGuide, distToFocusHor, longAxisHor, shortAxisHor, startPoint, endPoint)) {
	   fprintf(LogFilePtr,"Unable to determine horizontal ellipse parameters from input!");
	   exit (-1);
	 }

	 fprintf(LogFilePtr,"Found horizontal ellipse parameters according to user input: long axis: %f m, short axis: %f m \n", longAxisHor, shortAxisHor);

       }
     }
   }
   else if (shapeHor == 0 || shapeHor == 1) { // We are dealing with a constant or straight guide 

     slopeStraightHor = (endWidth - startWidth) / (200.*lengthGuide);
     fprintf(LogFilePtr,"Horizontal shape is straight with start width %f cm, end width %f cm, and a slope of %f \n", startWidth, endWidth, slopeStraightHor);
     if (shapeHor == 0 && slopeStraightHor != 0) {
       fprintf(LogFilePtr,"Horizontal shape is supposed to be constant but entrance and exit widths differ!");
       exit(-1);
     }
     startPoint = -1.*lengthGuide/2.;
     endPoint = lengthGuide/2.;     
   }

   //Check if enough parameters were given
   //Treat the vertical plane now
   if (shapeVer == 2) { // we are dealing with an ellipse

     if (longAxisVer > 0 && shortAxisVer > 0 && lengthGuide > 0) {

       if (startHeight > 0 || endHeight > 0) {
	 fprintf(LogFilePtr,"Ambiguous input for vertical plane, please specify either a) entrance/exit parameters or b) the size of the axes.");
	 exit(-1);
       }
       else {
	 double x = lengthGuide/2.;
	 startHeight = (shortAxisVer*sqrt(1. - x*x/(longAxisVer*longAxisVer))*2.)*100.;
	 endHeight = startHeight;
       }

     }
     else if (lengthGuide > 0 && startHeight > 0 && endHeight > 0 && distToFocusVer > 0) {
     
       if (longAxisVer > 0 || shortAxisVer > 0) {
	 fprintf(LogFilePtr,"Ambiguous input for vertical plane, please specify either a) entrance/exit parameters or b) the size of the axes.");
	 exit(-1);
       }
       else {
 
	 // Needed in case the vertical ellipse has another coordinate system
	 double startPointTemp = 0.;
	 double endPointTemp = 0.;

	 if (!CalculateEllipseParametersFromStartAndExitWidths(startHeight, endHeight, lengthGuide, distToFocusVer, longAxisVer, shortAxisVer, startPointTemp, endPointTemp)) {
	   fprintf(LogFilePtr,"Unable to determine vertical ellipse parameters from input!");
	   exit (-1);
	 }

	 //Shift of the vertical wrt the horizontal coordinate system
	 vertOffset = startPointTemp - startPoint;
       
	 fprintf(LogFilePtr,"Found vertical ellipse parameters according to user input: long axis: %f m, short axis: %f m, shift of x-axis: %f m \n", longAxisVer, shortAxisVer, vertOffset);

       }
     }
   }
   else if (shapeVer == 0 || shapeVer == 1) { // We are dealing with a constant or straight guide 

     slopeStraightVer = (endHeight - startHeight) / (200.*lengthGuide);
     fprintf(LogFilePtr,"Vertical shape is straight with start height %f cm, end height %f cm, and a slope of %f \n", startHeight, endHeight, slopeStraightVer);
     if (shapeVer == 0 && slopeStraightVer != 0) {
       fprintf(LogFilePtr,"Vertical shape is supposed to be constant but entrance and exit widths differ!");
       exit(-1);
     }
   }
  

   SetGeometryData(); 

   return;

}



// Use 4 equations to determine ellipse parameters
// x1^2 = longAxis*longAxis - longAxis*longAxis/(shortAxis*shortAxis)*y1*y1
// x2^2 = longAxis*longAxis - longAxis*longAxis/(shortAxis*shortAxis)*y2*y2
// (x2 + dist)^2 = longAxis*longAxis - shortAxis*shortAxis
// length = x2 -x1
//
// The general case is solved by a quartic equation giving possible solutions for x2.
//
// Special case: y1=y2 --> x1 = -x2 = length/2.
//


void   LoadReflFile(ReflFile *pReflFile)
{
  // if (pReflFile->maxdata > 0) return;
  
  long   count = 0, i = 0, nLines = 0;
  char   sBuffer[512]="";

  if (pReflFile!=NULL) {
    if (pReflFile->filename != NULL) {
      if (pReflFile->pfile == NULL) pReflFile->pfile = fopen(FullParName(pReflFile->filename), "r");
      if (pReflFile->pfile != NULL) {
        nLines = LinesInFile(pReflFile->pfile);
        pReflFile->maxdata = nLines * 10;
        pReflFile->Rdata = (double*) calloc(pReflFile->maxdata, sizeof(double));
        for(count=0; count < nLines; count++) {
          ReadLine(pReflFile->pfile, sBuffer, sizeof(sBuffer)-1);
          i += StrgScanLF(sBuffer, &pReflFile->Rdata[10*count], pReflFile->maxdata-10*count, 0);
        }
        fclose(pReflFile->pfile);
      }
    }
  }

  return;
}

// Fill reflectivity arrays by calculating the reflectivity usind the formula
// for the Swiss Neutronics supermirrors
void FillReflContainer(ReflFile* reflStruct, double m)
{

  if (m < 0) {
    fprintf(LogFilePtr,"m-Value below 0 is given! Module stops!");
    exit(-1);
  }

  double lambda = 1./thetaCNi;
  reflStruct->maxdata = (int) (m*100. + 200);
  reflStruct->Rdata = (double*) calloc(reflStruct->maxdata, sizeof(double));

  for (int i = 0; i < reflStruct->maxdata; i++) reflStruct->Rdata[i] = ReflSN(lambda, (double)i*0.01, m);

  return;

}

int ProcessNeutron(Neutron* n)
{ 
  
  // Initialise some parameters in the beginning
  bool endReached = false;
  double xMin = startPoint;
  double tof = 0.;
  //  double startPosition = n->Position[0];
  n->Position[0] = xMin;
  
  // Check if neutron misses the guide entrance
  if (n->Position[1] >= (startWidth / 2.) || n->Position[1] <= ((-1.)*startWidth / 2.)) {

    WriteIAPEllGuide(n, VT_OUTSIDE);

    return 0;
  }
  if (n->Position[2] >= (startHeight / 2.) || n->Position[2] <= ((-1.)*startHeight / 2.)) {

    WriteIAPEllGuide(n, VT_OUTSIDE);
    
    return 0;
  }
  

  //Sanity check
  if (n->Vector[0] == 0 || n->Wavelength == 0 || n->Probability == 0) return 0;
  if (ISNAN(n->Position[0]) || ISNAN(n->Position[1]) ||
	  ISNAN(n->Position[2])) return 0;

  
  // Propagate the trajectory in x-y and x-z plane
  // Calculate where the first interaction occurs
  // Since guide changes the divergence, in particular n->Vector[0], 
  // propagation in perpendicular plane is affected.
  
  WriteIAPEllGuide(n, VT_ENTERED);

  // Calculate next intersection point; 
  // sum up the travelled distance in x direction
  while (!endReached) {
    
    if (n->Vector[0] == 0) return 0;

    Neutron nTemp1;
    Neutron nTemp2;

    // Create two temporary trajectories for horizontal/vertical propagation
    CopyNeutron(n, &nTemp1);
    CopyNeutron(n, &nTemp2);

    // Find out which one collides first with the guide
    double distTempX1 = 0;
    endReached = PropagateStraightTrajectory(&nTemp1, distTempX1, xMin, 1, shapeHor);
    double distTempX2 = 0;

    // Shift the coordinate system for the vertical plane
    if (keygrav == 1)  endReached &= PropagateParabolicTrajectory(&nTemp2, distTempX2, xMin+vertOffset, 2, shapeVer);
    else {
       // Shift the coordinate system for the vertical plane
      endReached &= PropagateStraightTrajectory(&nTemp2, distTempX2, xMin+vertOffset, 2, shapeVer);
      if (nTemp1.Probability < wei_min && nTemp2.Probability < wei_min) neutronsKilledStraight--;
    }
    
    // For neutrons reflecting off guide at the very beginning the parabolic case can fail. 
    // Approximate by straight trajectory, should be fine within first 2% of the size of the major axis, e.g. within first 1.5m for major axis = 75m
    if (keygrav && shapeVer == 2 && (fabs(nTemp1.Position[2]/100.) > fabs(CalculateGuidePoint(nTemp1.Position[0], 2, 1))) &&
	((distTempX2 - distTempX1) > 1e-5) && (n->Position[0] < (startPoint + 0.02*longAxisVer))) {
      CopyNeutron(n, &nTemp2);
      endReached &= PropagateStraightTrajectory(&nTemp2, distTempX2, xMin+vertOffset, 2, shapeVer);
      if (nTemp1.Probability < wei_min && nTemp2.Probability < wei_min) neutronsKilledStraight--;
    }     

    if (!endReached) {
      // If reflection locations for both dimensions are closer together than 10^-5 m, ignore the difference, there are usually just a few
      if (fabs(distTempX1 - distTempX2) < 1e-5) {

	n->Position[0] = nTemp1.Position[0];
	n->Position[1] = CalculateGuidePoint(nTemp1.Position[0], 1, fabs(nTemp1.Position[1])/nTemp1.Position[1]);
	n->Position[2] = CalculateGuidePoint(nTemp1.Position[0], 2, fabs(nTemp2.Position[2])/nTemp2.Position[2]);
	n->Vector[1] = nTemp1.Vector[1];
	n->Vector[2] = nTemp2.Vector[2];
	n->Vector[0] = sqrt(1. - nTemp1.Vector[1]*nTemp1.Vector[1] - nTemp2.Vector[2]*nTemp2.Vector[2]);
	tof += (nTemp1.Position[0] - xMin)*100./(n->Vector[0]*V_FROM_LAMBDA(n->Wavelength));

	if (changeColor) n->Color += 101;
	if (n->Probability <= wei_min)  {
	  WriteIAPEllGuide(n, VT_ABSORBED);
	  return 0;
	}
	WriteIAPEllGuide(n, VT_REFLECTED);
	WriteIAPEllGuide(n, VT_REFLECTED);
	
	xMin = nTemp1.Position[0];
	simultaneousCollisions++;

      }
      
      // Reflection takes place first in horizontal plane
      else if (distTempX1 < distTempX2) {	
	 
	// Here something went wrong, first reflection takes place in the horizontal plane,
	// but in vertical the trajectory already left the guide!
	if (fabs(nTemp1.Position[2]/100.) > fabs(CalculateGuidePoint(nTemp1.Position[0], 2, 1))) {

#if DEBUG	  
	  double ellipseAtLastCollision = CalculateGuidePoint(nTemp1.Position[0], 2, fabs( n->Position[2])/ n->Position[2])*100.;
	  DEBUG_OUT("Bad neutrons from y-reflection: dist1 %f, vert ellipse at last collision: %f", distTempX1, ellipseAtLastCollision);
	  if (distTempX1 > 0) {
	    DEBUG_OUT("Coordinates for bad neutrons from y-reflection: x %f, y %f, z %f, z from ellipse %f, dir_x %f, dir_y %f, dir_z %f \n",  nTemp1.Position[0], nTemp1.Position[1], nTemp1.Position[2],
		       ellipseAtLastCollision, nTemp1.Vector[0], nTemp1.Vector[1], nTemp1.Vector[2]);
	     DEBUG_OUT("Coordinates for bad neutrons from z-reflection: x %f, y %f, z %f, dir_x %f, dir_y %f, dir_z %f \n", nTemp2.Position[0], nTemp2.Position[1], nTemp2.Position[2],
			nTemp2.Vector[0], nTemp2.Vector[1], nTemp2.Vector[2]);	  
	     ellipseAtLastCollision = CalculateGuidePoint(n->Position[0], 2, fabs( n->Position[2])/ n->Position[2])*100.;	  
	     double x = n->Position[0];
	     double slope = 0;
	     if (shapeVer == 2 ) slope = (-1.)*fabs(n->Position[2])/n->Position[2]*shortAxisVer/(longAxisVer*longAxisVer)*x/sqrt(1. - x*x/(longAxisVer*longAxisVer))*n->Vector[0];	  
	     DEBUG_OUT("Coordinates for bad neutrons before: x %f, y %f, z %f, z from ellipse %f, dir_x %f, dir_y %f, dir_z %f, slope of ellipse %f \n", n->Position[0], n->Position[1], n->Position[2],
			ellipseAtLastCollision, n->Vector[0], n->Vector[1], n->Vector[2], slope);
	  }
#endif

	  badNeutrons++;
	  return 0;

	}

	// Update ToF and other variables according to the new collision point
	tof += (nTemp1.Position[0] - xMin)*100./(n->Vector[0]*V_FROM_LAMBDA(n->Wavelength));
	xMin = nTemp1.Position[0];
	CopyNeutron(&nTemp1, n);
	if (changeColor) n->Color += 1; // Use +1 for color for horizontal reflection
	
	// Check if neutron got absorbed
	if (n->Probability <= wei_min)  {
	  WriteIAPEllGuide(n, VT_ABSORBED);
	  return 0;
	}
	WriteIAPEllGuide(n, VT_REFLECTED);
	
      }

      // Reflection takes place first in vertical plane
      else {

	// Here something went wrong, first reflection takes place in the vertical plane,
	// but in horizontal the trajectory already left the guide!
	if (fabs(nTemp2.Position[1]/100.) > fabs(CalculateGuidePoint(nTemp2.Position[0], 1, 1))) {
	  double guideAtLastCollision = CalculateGuidePoint(nTemp2.Position[0], 1, 1)*100.;
#if DEBUG
	  DEBUG_OUT("Bad neutrons from y-reflection: dist1 %f, dist2 %f, y-position at dist2: %f", distTempX1, distTempX2, nTemp2.Position[1]/100.);
#endif
	  badNeutrons++;
	  return 0;
	}

	// Update ToF and other variables according to the new collision point
	tof += (nTemp2.Position[0] - xMin)*100./(n->Vector[0]*V_FROM_LAMBDA(n->Wavelength));
	xMin = nTemp2.Position[0];
	CopyNeutron(&nTemp2, n);
	if (changeColor) n->Color += 100; // Use +100 for color for horizontal reflection

	// Check if neutron got absorbed
	if (n->Probability <= wei_min) {
	  WriteIAPEllGuide(n, VT_ABSORBED);
	  return 0;
	}
	WriteIAPEllGuide(n, VT_REFLECTED);

      }
    }

    // Trajectory reached the guide exit
    else {      

      n->Position[0] = endPoint;
      n->Position[1] = nTemp1.Position[1];
      n->Position[2] = nTemp2.Position[2];
      WriteIAPEllGuide(n, VT_EXITED);
      n->Position[0] = 0.; //startPosition + lengthGuide*100.;
      n->Vector[2] = nTemp2.Vector[2]; // Take into account change of direction due to gravity since last reflection
      tof += (endPoint - xMin)*100./(n->Vector[0]*V_FROM_LAMBDA(n->Wavelength));
      n->Time += tof;
      
    }    

  }


  return 1;

}


// Calculate next intersection point for  straight trajectories, choose between planes: 1 for x-y, 2 for x-z plane
// For the x-z plane, take into account a possible offset between the coordinate systems (only for ellipses)
bool PropagateStraightTrajectory(Neutron* n, double &dist, double xMin, int plane, int shape)
{

  //  fprintf(LogFilePtr,"Propagate straight trajectory! \n");

   // Read direction and position of the neutron
  MathVector neutronVector (n->Vector[0], n->Vector[1], n->Vector[2]);
  MathVector neutronPosition (xMin, n->Position[1]/100., n->Position[2]/100.); // convert from cm to m

  // Create a straight equation from trajectory: y = mx + b
  double m = neutronVector.x[plane] / neutronVector.x[0];
  double b = m*xMin*(-1.) + neutronPosition.x[plane];

 
  // // Check whether the neutron hits the exit in this plane:
  // double posAtExit = m*endPoint + b;
  // if ((fabs(posAtExit) < endWidth/2 && plane == 1) ||  (fabs(posAtExit) < endHeight/2 && plane == 2)) {
  //   n->Position[plane] = (m*endPoint + b)*100.;
  //   dist = endPoint - xMin;
  //   return 1;
  // }

  // Initialise ellipse parameters
  double longAxis = longAxisHor;
  double shortAxis = shortAxisHor;
  double slopeFromShape = slopeStraightHor;
  double shapeWidthAtZero = startWidth/200. - slopeFromShape*startPoint;

  if (plane == 2) {

    longAxis = longAxisVer;
    shortAxis = shortAxisVer;
    slopeFromShape = slopeStraightVer;
    shapeWidthAtZero = startHeight/200. - slopeFromShape*startPoint;
  }

  // Find the intersection point either with an elliptic or a straight guide (shape)
  double x,y;
  if (shape == 2) IntersectStraightTrajectoryWithEllipse(longAxis, shortAxis, b, m, xMin + pow(10., -8.), x, y);
  else IntersectTrajectoryWithLinearShape(slopeFromShape, shapeWidthAtZero, b, m, 0, xMin + pow(10., -8.), x, y);

  // Take into account a possible offset of the vertical coordinate system
  if (plane == 2) {
    xMin -= vertOffset;
    x -= vertOffset;
    neutronPosition.x[0] -= vertOffset;
    b += m*vertOffset;
  }

  // There was some unknown problem, currently such cases do not occur
  if (x == -6666) {
    dist = endPoint - xMin;
    return 1;
  }

  // Trajectory leaves the guide without further reflection
  if (x > endPoint) {
    n->Position[plane] = (m*endPoint + b)*100.;
    dist = endPoint - xMin;
    n->Position[0] = endPoint;
    return 1;
  }

  // Trajectory hits the guide
  else{

    double newSlope = 0;
    if (shape == 2) {
      if (y > 0)  newSlope = CalculateAngleAfterReflectionEllipse(longAxis, shortAxis, m, 0, x, plane, true);
      else newSlope = CalculateAngleAfterReflectionEllipse(longAxis, shortAxis, m, 0, x, plane, false);
    }
    else {
      if (y > 0) newSlope =  CalculateAngleAfterReflectionLinear(slopeFromShape, m, 0, x, true);
      else newSlope = CalculateAngleAfterReflectionLinear(slopeFromShape, m, 0, x, false);
    }

    // Very rarely the slope is wrongly calculated!
     if (newSlope != newSlope) {
      dist = 0;
      n->Probability = 0;
      //      fprintf(LogFilePtr,"Slope straight: %f \n", newSlope);
      return 0;
    }

     // From the slope of the straight representing the neutron trajectory,
     // calculate the angle after reflection and thus the new slope
    double slopeBefore = m;

    double reflAngle = atan(fabs(newSlope - slopeBefore)/2.)/M_PI*180.;
    double totalAngle = atan(newSlope - slopeBefore);

    // Find the right reflection file
    if (y >= 0 && plane == 1) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[0].maxdata > datanumber) n->Probability *= reflContainer[0].Rdata[datanumber];
      else n->Probability = 0;
    }
    else if (y < 0 && plane == 1) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
       if (reflContainer[1].maxdata > datanumber) n->Probability *= reflContainer[1].Rdata[datanumber];
       else n->Probability = 0;
    }
    else if (y >= 0 && plane == 2) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[2].maxdata > datanumber) n->Probability *= reflContainer[2].Rdata[datanumber];
      else n->Probability = 0; 
    }
    else if (y < 0 && plane == 2) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[3].maxdata > datanumber) n->Probability *= reflContainer[3].Rdata[datanumber];
      else n->Probability = 0;
    }
    else n->Probability = 0;

    // Log the number of trajectories absorbed
    if (n->Probability == 0) neutronsKilledStraight++;

    // Distance wrt the x-axis between the last and current collision point
    dist = x - neutronPosition.x[0];

    // Rotate the trajectory after collision
    MathVector neutronVectorAfterReflection (0, 0, 0);
    if (plane == 1) neutronVectorAfterReflection = neutronVector.Rotate(0, 0, totalAngle);
    else neutronVectorAfterReflection = neutronVector.Rotate(0, totalAngle*(-1.), 0);

    // fprintf(LogFilePtr,"Neutron vector before reflection straight: %f %f %f\n", neutronVector.x[0],  neutronVector.x[1], neutronVector.x[2]);
    // fprintf(LogFilePtr,"Neutron vector after reflection straight: %f %f %f\n", neutronVectorAfterReflection.x[0],  
    // 	    neutronVectorAfterReflection.x[1], neutronVectorAfterReflection.x[2]);
    

    // Propagate also the perpendicular direction
    
    int otherplane = 0;
    if (plane == 1) otherplane = 2;
    else otherplane = 1;

    double a1 = n->Vector[otherplane] / n->Vector[0];
    double a2 = keygrav*(-1.)*0.5*G/pow(V_FROM_LAMBDA(n->Wavelength)*10., 2);
    a1 -= 2.*a2*xMin;
    double a0 = neutronPosition.x[otherplane] - a1*xMin - a2*xMin*xMin;
    n->Position[otherplane] = (a2*x*x + a1*x + a0)*100.; //convert from m to cm
    n->Vector[otherplane] = (a1 + 2.*a2*x)*n->Vector[0];    

    n->Vector[0] = neutronVectorAfterReflection.x[0];
    n->Vector[plane] = neutronVectorAfterReflection.x[plane];

    n->Position[0] = x; // note: position still in m!
    n->Position[plane] = y*100.; //convert from m to cm

    if (n->Vector[0] <= 0) n->Probability = 0;

    return 0;

  }

  return 1;

}

// Calculate next intersection point for parabolic trajectories, choose between planes: 1 for x-y, 2 for x-z plane
// For the x-z plane, take into account a possible offset between the coordinate systems (only for ellipses)
bool PropagateParabolicTrajectory(Neutron* n, double &dist, double xMin, int plane, int shape)
{
  
  //  fprintf(LogFilePtr,"Propagate parabolic trajectory! \n");
   // Read direction and position of the neutron
  MathVector neutronVector (n->Vector[0], n->Vector[1], n->Vector[2]);
  MathVector neutronPosition (xMin, n->Position[1]/100., n->Position[2]/100.); // convert from cm to m

  // Create a parabolic equation from trajectory: y = a2x^2 + a1x + a0, use G=9.81m/s^2 for a2.
  // use m and s as units
  double a1 = neutronVector.x[plane] / neutronVector.x[0];

  double a2 = (-1.)*0.5*G/pow(V_FROM_LAMBDA(n->Wavelength)*10., 2);
  a1 -= 2.*a2*xMin;
  double a0 = neutronPosition.x[plane] - a1*xMin - a2*xMin*xMin;

   // Initialise ellipse parameters
  double longAxis = longAxisHor;
  double shortAxis = shortAxisHor;
  double slopeFromShape = slopeStraightHor;
  double shapeWidthAtZero = startWidth/200. - slopeFromShape*startPoint;

  if (plane == 2) {

    longAxis = longAxisVer;
    shortAxis = shortAxisVer;
    slopeFromShape = slopeStraightVer;
    shapeWidthAtZero = startHeight/200. - slopeFromShape*startPoint;

  }

  // Find the intersection point either with an elliptic or a straight guide (shape)
  double x,y;

   if (shape == 2) IntersectParabolicTrajectoryWithEllipse(longAxis, shortAxis, a0, a1, a2, xMin + pow(10., -5.), x, y, false);
   else IntersectTrajectoryWithLinearShape(slopeFromShape, shapeWidthAtZero, a0, a1, a2, xMin + pow(10., -8.), x, y);

   if (plane==2) {
     xMin -= vertOffset;
     x -= vertOffset;
   }
   
   // There was some unknown problem, currently such cases do not occur
   if (x == -6666) {
     //     fprintf(LogFilePtr,"Coordinates for nan neutrons: x %f, y %f, z %f, dir_x %f, dir_y %f, dir_z %f \n", n->Position[0], n->Position[1], n->Position[2],
     //	     n->Vector[0], n->Vector[1], n->Vector[2]);
     dist = endPoint - xMin;
     return 1;
   }
   
  // Trajectory leaves the guide without further reflection
   if (x >= endPoint) {
     double endPointTemp = endPoint;
     if (plane==2) endPointTemp += vertOffset;
     double slope = a1  + 2.*a2*endPointTemp;
     n->Vector[plane] = slope*(n->Vector[0]);
     n->Position[plane] = (a0 + a1*endPointTemp + a2*endPointTemp*endPointTemp)*100.; //  convert from m to cm
     dist = endPoint - xMin;
     n->Position[0] = endPoint;
     return 1;
   }

  // Trajectory hits the guide
  else {

    double newSlope = 0;
    if (shape == 2) {
      if (y > 0)  newSlope = CalculateAngleAfterReflectionEllipse(longAxis, shortAxis, a1, a2, x, plane, true);
      else newSlope = CalculateAngleAfterReflectionEllipse(longAxis, shortAxis, a1, a2, x, plane, false);
    }
    else {
      if (y > 0) newSlope =  CalculateAngleAfterReflectionLinear(slopeFromShape, a1, a2, x, true);
      else newSlope = CalculateAngleAfterReflectionLinear(slopeFromShape, a1, a2, x, false);
    }
      
    // Very rarely the slope is wrongly calculated!
    if ((newSlope != newSlope)) {
      dist = 0;
      n->Probability = 0;
      //      fprintf(LogFilePtr,"Slope parabolic: %f \n", newSlope);
      return 0;
    }

    // switch back to vertical system for calculation of the trajectory angle
    if (plane==2) {
      x += vertOffset;
    }

   // From the slope of the parabula representing the neutron trajectory at the collision position,
    // calculate the angle after reflection and thus the new slope 
    double slopeBefore = a1 + 2.*a2*x;
    neutronVector.x[plane] = slopeBefore*(neutronVector.x[0]);
    double reflAngle = atan(fabs(newSlope - slopeBefore)/2.)/M_PI*180.;
    double totalAngle = atan(newSlope - slopeBefore);

    // Find the right reflection file
    if (y >= 0 && plane == 1) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[0].maxdata > datanumber) n->Probability *= reflContainer[0].Rdata[datanumber];
      else n->Probability = 0;
    }
    else if (y < 0 && plane == 1) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[1].maxdata > datanumber) n->Probability *= reflContainer[1].Rdata[datanumber];
      else n->Probability = 0;
    }
    else if (y >= 0 && plane == 2) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[2].maxdata > datanumber) n->Probability *= reflContainer[2].Rdata[datanumber];
      else n->Probability = 0;
    }
    else if (y < 0 && plane == 2) {
      int datanumber =  (int)(reflAngle*1000.0/(n->Wavelength));
      if (reflContainer[3].maxdata > datanumber) n->Probability *= reflContainer[3].Rdata[datanumber];
      else n->Probability = 0;
    }
    else n->Probability = 0;

     // Log the number of trajectories absorbed
    if (n->Probability == 0) neutronsKilledParabolic++;

    // Distance wrt the x-axis between the last and current collision point
    dist = x - neutronPosition.x[0];
    
    // Rotate the trajectory after collision
    MathVector neutronVectorAfterReflection (0, 0, 0);
    if (plane == 1) neutronVectorAfterReflection = neutronVector.Rotate(0, 0, totalAngle);
    else  neutronVectorAfterReflection = neutronVector.Rotate(0, totalAngle*(-1.), 0);
    

    // Propagate also perpendicular direction    
    int otherplane = 0;
    if (plane == 1) otherplane = 2;
    else {
      otherplane = 1;
      xMin += vertOffset;
    }

    double m = n->Vector[otherplane] / n->Vector[0];
    double b = m*xMin*(-1.) + neutronPosition.x[otherplane];  
    n->Position[otherplane] = (m*x + b)*100.; //convert from m to cm  

    n->Vector[0] = neutronVectorAfterReflection.x[0];
    n->Vector[plane] = neutronVectorAfterReflection.x[plane];
    n->Position[0] = x; // note: position still in m!
    // correct the position for the coordinate system offset
    if (plane == 2) n->Position[0] -= vertOffset;
    n->Position[plane] = y*100.; //convert from m to cm

    if (n->Vector[0] <= 0) n->Probability = 0;

    return 0;

  }

  return 1;

}


// Find intersection point by solving the quadratic equation:
// (mx + b)^2 = shortAxis*shortAxis*(1 - x*x/longAxis*longAxis)
void IntersectStraightTrajectoryWithEllipse(double longAxis, double shortAxis, double b, double m, double xMin, double &x, double &y)
{
  
  double ellipse = 1.;

  double p_half = (m*b)/(ellipse*shortAxis*shortAxis/(longAxis*longAxis) + m*m);
  double q = (b*b - shortAxis*shortAxis)/(ellipse*shortAxis*shortAxis/(longAxis*longAxis) + m*m);
  
  if ((p_half*p_half - q) < 0) {
    x = -6666;
    y = 0;
    return;
  }
  
  //  double xIntersectMin = (-1.)*p_half - sqrt(p_half*p_half - q);
  double xIntersectMax = (-1.)*p_half + sqrt(p_half*p_half - q);

  if (xIntersectMax > xMin) {
    x = xIntersectMax;
    y = b + m*xIntersectMax;
  }
  else {
    x = longAxis;
    y = 0;
    return; 
  }
  
  return;


}

// Find intersection point by solving the quartic equation:
// (a2*x^2 + a1*x + a0)^2 = shortAxis*shortAxis*(1 - x*x/longAxis*longAxis)
void IntersectParabolicTrajectoryWithEllipse(double longAxis, double shortAxis, double a0, double a1, double a2, double xMin, double& x, double& y, bool switchSign)
{

  double ellipse = 1.;//1.;

  double a = 2.*a1/a2;
  double b = (a1*a1 + 2.*a0*a2 + (shortAxis*shortAxis*ellipse/(longAxis*longAxis)))/(a2*a2);
  double c = 2.*a0*a1/(a2*a2);
  double d = (a0*a0 - shortAxis*shortAxis)/(a2*a2);

  double solutions[4];
  SolveQuarticEquation(a, b, c, d, &solutions[0], switchSign);


  double minSolution = longAxis;

  for (int i = 0; i < 4; i++) {
    if (solutions[i] > xMin && solutions[i] < (endPoint + vertOffset)) {

      if (solutions[i] < minSolution) minSolution = solutions[i];

    }
  }

  //  if (minSolution < endPoint) minSolution = CheckSolution(minSolution, a, b, c, d);

  x = minSolution;
  y = a0 + a1*x + a2*x*x;

  if (y > 0) y =  CalculateEllipsePoint(x, longAxis, shortAxis, 1.);
  else y =  CalculateEllipsePoint(x, longAxis, shortAxis, -1.);

  return;
  
}


// Simple case: Intersect a parabolic or straight (a2=0) trajectory with a straight guide
void IntersectTrajectoryWithLinearShape(double slopeFromShape, double shapeWidthAtZero, double a0, double a1, double a2, double xMin, double &x, double &y)
{


  if (a2 == 0) {

    double x1 = (shapeWidthAtZero - a0)/(a1 - slopeFromShape);
    double x2 = ((-1.)*shapeWidthAtZero - a0)/(a1 + slopeFromShape);

    if (x1 > xMin && x2 <= xMin) {
      x = x1;
      y = a1*x1 + a0;
    }
    else if (x2 > xMin && x1 <= xMin) {
      x = x2;
      y = a1*x2 + a0;
    }
    else if (x1 > xMin && x2 > xMin) {
      x = Min(x1, x2);
      y = a1*x + a0;
    }
	else  if (x1 <= xMin && x2 <= xMin){
		x = lengthGuide;
		y = 0;
	}
	 else {
      fprintf(LogFilePtr,"WTF? x1 = %f & x2 = %f xMin = %f slope = %f \n", x1, x2, xMin, a0);
      exit(-1);
    }	
  }

  else {

    double x_0[4];

    double p1 = (a1 - slopeFromShape) / a2;
    double q1 = (a0 - shapeWidthAtZero) / a2;
    x_0[0] = -p1/2. + sqrt(pow(p1/2., 2) - q1);
    x_0[1] = -p1/2. - sqrt(pow(p1/2., 2) - q1);

    double p2 = (a1 + slopeFromShape) / a2;
    double q2 = (a0 + shapeWidthAtZero) / a2;
    x_0[2] = -p2/2. + sqrt(pow(p2/2., 2) - q2);
    x_0[3] = -p2/2. - sqrt(pow(p2/2., 2) - q2);

    double minPos = endPoint + lengthGuide;

    for (int i = 1; i < 4; i++) {
      if (x_0[i] > xMin && x_0[i] < minPos) minPos = x_0[i];
    }

    x = minPos;
    y = a2*x*x + a1*x + a0;

  }
  
  return;

}


// Calculate the trajectory angle after collision with an elliptic guide
double CalculateAngleAfterReflectionEllipse(double longAxis, double shortAxis, double a1, double a2, double x, int plane, bool positive)
{  

  if (plane == 2) x += vertOffset;

  double a = longAxis;
  double b = shortAxis;
  double sign = 1.;
  if (!positive) sign = -1.;

  double slope = (-1.)*sign*b/(a*a)*x/sqrt(1. - x*x/(a*a));

  a1 = a1 + 2.*a2*x;
  
  if (positive) a1 = a1 - 2.*fabs(a1 - slope);
  else a1 = a1 + 2.*fabs(a1 - slope);
  
  //  if (a1 != a1) fprintf(LogFilePtr,"Crazy slope: %f %f %f\n", slope, x, a);

  return a1;

}

// Calculate the trajectory angle after collision with a straight guide
double CalculateAngleAfterReflectionLinear(double slopeFromShape, double a1, double a2, double x, bool positive)
{

  a1 = a1 + 2.*a2*x;

  if (positive) a1 = a1 - 2.*fabs(a1 - slopeFromShape);
  else a1 = a1 + 2.*fabs(a1 + slopeFromShape);

  return a1;

}

// Calculate the halfwidth/height at a certain position expressed in internal coordinates
// with the origin at the centre of the (horizontal) ellipse 
// In case the vertical ellipse has an offset in the coordinate system, it is accounted for in this function
double CalculateGuidePoint (double x, int dir, double sign)
{

  int shape = 0;
  double slope = 0;
  double y = 0;

  if (dir == 1) {
    shape = shapeHor;
    if (shape < 2) {
      slope = slopeStraightHor;
      y = (startWidth/200. + (x - startPoint)*slope)*sign;
    }
    else {
      y = CalculateEllipsePoint(x, longAxisHor, shortAxisHor, sign);
    }
  }
  else if (dir == 2) {
     shape = shapeVer;
     x += vertOffset;
    if (shape < 2) {
      slope = slopeStraightVer;
      y = (startHeight/200. + (x - startPoint)*slope)*sign;
    }
    else {
      y = CalculateEllipsePoint(x, longAxisVer, shortAxisVer, sign);
    }
  }
  else {
    fprintf(LogFilePtr,"Wrong direction for guide shape! \n");
    exit(-1);
  }

  return y;

}


// Some log writing...
void OwnCleanup()
{

  fprintf(LogFilePtr,"Elliptic parameters used from input: \n");
  fprintf(LogFilePtr,"Long axis: %f m, short axis %f m, in horizontal plane \n", longAxisHor, shortAxisHor);
  fprintf(LogFilePtr,"Long axis: %f m, short axis %f m, in vertical plane \n", longAxisVer, shortAxisVer);
  fprintf(LogFilePtr,"Length of guide: %f , xLow: %f , xHigh: %f vertOffset %f \n", lengthGuide, startPoint, endPoint, vertOffset);
  fprintf(LogFilePtr,"Start width: %f cm, start height %f cm, end width %f cm, end height %f cm\n", startWidth, startHeight, endWidth, endHeight);


  double trueX = startPoint;
  double shiftedX = 0;
  double step = 0.5;

  fprintf(fShapeFilePointer, "# x[m] \t width[m] \t height[m] \n");

  while (trueX <= endPoint) {

    fprintf(fShapeFilePointer, " %f \t %f \t %f \n", shiftedX, CalculateGuidePoint(trueX, 1, 1)*2. , CalculateGuidePoint(trueX, 2, 1)*2.);
    shiftedX += step;
    trueX += step;

  }

  if ((trueX - step) < endPoint) fprintf(fShapeFilePointer, " %f \t %f \t %f \n", endPoint - startPoint, CalculateGuidePoint(endPoint, 1, 1)*2. , CalculateGuidePoint(endPoint, 2, 1)*2.);

  fclose (fShapeFilePointer);

  return;

}

void SetGeometryData()
{


  bVisInstalled = TRUE;
 // Geometry data
  if (bVisInstr) {

    if (shapeHor==2 && shapeVer==2 && vertOffset == 0)  {      
      
      stGeometry.pEllipsoid = (VtEllipsoid*) calloc(1, sizeof(VtEllipsoid));
      stGeometry.nEllipsoids = 1;
      stGeometry.pEllipsoid[0].vCntr[0] = (0 - startPoint)*100.;
      stGeometry.pEllipsoid[0].vCntr[1] = 0.;
      stGeometry.pEllipsoid[0].vCntr[2] = 0.;

      stGeometry.pEllipsoid[0].vSymAxis[0] = 1;
      stGeometry.pEllipsoid[0].vSymAxis[1] = 0.;
      stGeometry.pEllipsoid[0].vSymAxis[2] = 0.;
      
      stGeometry.pEllipsoid[0].Length = longAxisHor*200.;
      stGeometry.pEllipsoid[0].Width = shortAxisHor*200.;
      stGeometry.pEllipsoid[0].Height = shortAxisVer*200.;
      stGeometry.pEllipsoid[0].Xlow = startPoint*100.;
      stGeometry.pEllipsoid[0].Xhigh = endPoint*100.;

    }
    else if (shapeHor < 2 && shapeVer < 2) {

      stGeometry.pHull = (VtHull*) calloc(1, sizeof(VtEllipsoid));
      stGeometry.nHulls = 1;

      stGeometry.pHull[0].vCntr[0] = (0 - startPoint)*100.;
      stGeometry.pHull[0].vCntr[1] = 0.;
      stGeometry.pHull[0].vCntr[2] = 0.;

      stGeometry.pHull[0].vNormal[0] = 1;
      stGeometry.pHull[0].vNormal[1] = 0.;
      stGeometry.pHull[0].vNormal[2] = 0.;
      
      stGeometry.pHull[0].Length = lengthGuide*100.;
      stGeometry.pHull[0].WidthIn = startWidth;
      stGeometry.pHull[0].WidthOut = endWidth;
      stGeometry.pHull[0].HeightIn = startHeight;
      stGeometry.pHull[0].HeightOut = endHeight;

    }

    else {

      stGeometry.nHulls = (int)(lengthGuide*100.) / 50 + 1;
      stGeometry.pHull = (VtHull*) calloc(stGeometry.nHulls, sizeof(VtHull));

      for (int i = 0; i < (stGeometry.nHulls - 1); i++) {

	stGeometry.pHull[i].vCntr[0] = i*50. + 25.;
	stGeometry.pHull[i].vCntr[1] = 0.;
	stGeometry.pHull[i].vCntr[2] = 0.;

	stGeometry.pHull[i].vNormal[0] = 1;
	stGeometry.pHull[i].vNormal[1] = 0.;
	stGeometry.pHull[i].vNormal[2] = 0.;	

	stGeometry.pHull[i].Length = 50.;
	stGeometry.pHull[i].WidthIn = 200.*CalculateGuidePoint(startPoint + 0.5*i, 1);
	stGeometry.pHull[i].WidthOut = 200.*CalculateGuidePoint(startPoint + 0.5*(i+1.), 1);
	stGeometry.pHull[i].HeightIn = 200.*CalculateGuidePoint(startPoint + 0.5*i, 2);
	stGeometry.pHull[i].HeightOut = 200.*CalculateGuidePoint(startPoint + 0.5*(i+1.), 2);

      }

      double remainingDist = double ((int)(lengthGuide*100.) % 50);
      if (remainingDist == 0) {
	stGeometry.nHulls--;
      }
      else {
	stGeometry.pHull[stGeometry.nHulls - 1].vCntr[0] = (stGeometry.nHulls - 1)*50. + remainingDist/2.;
	stGeometry.pHull[stGeometry.nHulls - 1].vCntr[1] = 0.;
	stGeometry.pHull[stGeometry.nHulls - 1].vCntr[2] = 0.;
	
	stGeometry.pHull[stGeometry.nHulls - 1].vNormal[0] = 1;
	stGeometry.pHull[stGeometry.nHulls - 1].vNormal[1] = 0.;
	stGeometry.pHull[stGeometry.nHulls - 1].vNormal[2] = 0.;	
	
	stGeometry.pHull[stGeometry.nHulls - 1].Length = remainingDist;
	stGeometry.pHull[stGeometry.nHulls - 1].WidthIn = 200.*CalculateGuidePoint(startPoint + 0.5*(stGeometry.nHulls - 1), 1);
	stGeometry.pHull[stGeometry.nHulls - 1].WidthOut = 200.*CalculateGuidePoint(endPoint, 1);
	stGeometry.pHull[stGeometry.nHulls - 1].HeightIn = 200.*CalculateGuidePoint(startPoint + 0.5*(stGeometry.nHulls - 1), 2);
      stGeometry.pHull[stGeometry.nHulls - 1].HeightOut = 200.*CalculateGuidePoint(endPoint, 2);
      }
    }
    
    stGeometry.pDescr  = "elliptic guide:yellow";
    stGeometry.eModule = VT_GUIDE;

  }
    
  return;

}

// Function needed since all positions are in [cm] for trajectory visualisation
void WriteIAPEllGuide(Neutron *n, VtReason eReason)
{

  double xPosTemp = n->Position[0];
  n->Position[0] = (n->Position[0] - startPoint)*100.;
  WriteIAP(n, eReason);
  n->Position[0] = xPosTemp;

  return;

}


#endif
