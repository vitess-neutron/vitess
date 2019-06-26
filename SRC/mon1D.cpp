#ifndef MON1D_CPP
#define MON1D_CPP


#include "mon1D.h"


Mon1D::Mon1D()
{

  for (int i = 0; i < 3; i++) {
  
    dataArray[i] = 0;
    dataArrayPolWeights[i] = 0;
    dataArrayError[i] = 0;
    dataArrayCounts[i] = 0;
    
    xMin[i] = -1;
    xMax[i] = -1;
    
    nBinsX[i] = 0;
    
    xBinSize[i] = 0;
  
    xParam[i] = -1;

    monSwitchedOn[i] = 0;

    fMonitor[i] = 0;

  }

  fMonitorFilename = "NoFile";

  lambdaMin = -1;
  lambdaMax = -1;

  analysePol = 0;

  polAnalysisVector = 0;
  polAnalysisRotMatrix = 0;  

  filterVarMin1 = -1;
  filterVarMin2 = -1;
  filterVarMax1 = -1;
  filterVarMax2 = -1;

  filterParam1 = -1;
  filterParam2 = -1;
  filterComb = -1;

  normalise = -1; 
  
  colour = -1;
  pWeight = 1;
  exclCounts = 0;

  weightTag[0] = "";
  weightTag[1] = "weight";

  sParameterNames[0] = "pos_y";
  sParameterNames[1] = "pos_z";
  sParameterNames[2] = "div_y";
  sParameterNames[3] = "div_z";
  sParameterNames[4] = "lambda";
  sParameterNames[5] = "energy"; 
  sParameterNames[6] = "time"; 
  sParameterNames[7] = "k_y";
  sParameterNames[8] = "k_z"; 
  sParameterNames[9] = "r"; 
  sParameterNames[10] = "phi";
  sParameterNames[11] = "col_vert"; 
  sParameterNames[12] = "col_hor"; 
  sParameterNames[13] ="color"; 

}


void Mon1D::Init(int argc, char* argv[])
{
  // Read the command line arguments
  for(int i=1; i<argc; i++)
    {
      if(argv[i][0]!='+') {
	switch(argv[i][1])
	  {
	  case 'O':	   
	    fMonitorFilename=&argv[i][2];
	    break;

	  case 'X':
	    xParam[0] = atoi(&argv[i][2]); // parameter to be shown on the 1st x axis, input parameter
	    break;
	  
	  case 'x':
	     nBinsX[0] = atol(&argv[i][2]); /* number of bins for the 1st horizontal axis */
	    break;

	  case 'w':
	    xMin[0] = atof(&argv[i][2]);		/* left edge position of 1st window */
	    break;

	  case 'W':
	    xMax[0] = atof(&argv[i][2]);		/* right edge position of 1st window */
	    break;

	 case 'Y':
	    xParam[1] = atoi(&argv[i][2]); // 2nd parameter to be shown on the x axis, input parameter
	    break;
	  
	  case 'y':
	     nBinsX[1] = atol(&argv[i][2]); /* number of bins for the 2nd  horizontal axis */
	    break;

	  case 'f':
	    xMin[1] = atof(&argv[i][2]);		/* left edge position of 2nd window */
	    break;

	  case 'F':
	    xMax[1] = atof(&argv[i][2]);		/* right edge position of 2nd window */
	    break;   

	  case 'Z':
	    xParam[2] = atoi(&argv[i][2]); // 3rd parameter to be shown on the x axis, input parameter
	    break;
	  
	  case 'z':
	     nBinsX[2] = atol(&argv[i][2]); /* number of bins for the 3rd  horizontal axis */
	    break;

	  case 'g':
	    xMin[2] = atof(&argv[i][2]);		/* left edge position of 3rd window */
	    break;

	  case 'G':
	    xMax[2] = atof(&argv[i][2]);		/* right edge position of 3rd window */
	    break;   
  

	  case 'I':  
	    filterParam1 = atoi(&argv[i][2]); // filter parameter 1, optional input parameter
	    break;

	  case 'J':  
	    filterParam2 = atoi(&argv[i][2]); // filter parameter 2, optional input parameter
	    break;

	  case 'C':  
            filterComb = atoi(&argv[i][2]); // filter combination (AND,OR)
            break;

	  case 'p':
	    pWeight  = atof(&argv[i][2]);
	    /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	    break;

	  case 'P':
	    analysePol = atof(&argv[i][2]); // 1 if polarisation analysis desired, optional input parameter
	    break;

	  case 'r':
	    if (!polAnalysisVector) polAnalysisVector = new MathVector(); // polarisation analysis vector, x component
	    polAnalysisVector->x[0] = atof(&argv[i][2]);
	    break;

	  case 's':
	    if (!polAnalysisVector) polAnalysisVector = new MathVector(); // polarisation analysis vector, y component
	    polAnalysisVector->x[1] = atof(&argv[i][2]);
	    break;
	    
	  case 't':
	    if (!polAnalysisVector) polAnalysisVector = new MathVector();  // polarisation analysis vector, z component
	    polAnalysisVector->x[2] = atof(&argv[i][2]);
	    break;
	    
	  case 'e':
	    if(argv[i][2]=='1')
	      exclCounts = 1;   /* if activated, only neutrons meeting the monitor conditions are considered further on */
	    break;
	  
	  case 'l':
        lambdaMin = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;

      case 'L':
        lambdaMax = atof(&argv[i][2]);   /* filter lambda, -1 means any */
        break;

      case 'u':
        filterVarMin1 = atof(&argv[i][2]);   /* minimum value of filter parameter 1 */
        break;

      case 'U':
        filterVarMax1 = atof(&argv[i][2]);   /* maximum value of filter parameter 1 */
        break;

      case 'v':
        filterVarMin2 = atof(&argv[i][2]);   /* minimum value of filter parameter 2 */
        break;

      case 'V':
        filterVarMax2 = atof(&argv[i][2]);   /* maximum value of filter parameter 2 */
        break;

	  default:
	    fprintf(LogFilePtr,"unknown commandline option: %s\n",argv[i]);
	    exit(-1);
	    break;
	  }
      }
    }

  if (fMonitorFilename=="")
    {
      fprintf(LogFilePtr,"\n you must define a MonitorOutputFile");
      exit(99);
    }


  bool multipleFiles = false;
  int numberFiles = 0;

  for (int ii = 0; ii < 3; ii++) {
    if (xParam[ii] > 0) numberFiles++;
  }

  if (numberFiles > 1) multipleFiles = true;

  if (pWeight != 1) pWeight = 0;

  for (int ii = 0; ii < 3; ii++) {

    if (xParam[ii] < 1) continue;

    string fullFileName = fMonitorFilename;
    if (multipleFiles) fullFileName = fullFileName + "_" + sParameterNames[xParam[ii]-1] + ".mon";
    
    if((fMonitor[ii] = fopen(fullFileName.c_str(), "w"))==NULL)
      {
	fprintf(LogFilePtr,"\nFile %s could not be opened for monitor1D output\n", fullFileName.c_str());
	exit(-1);
      }
    
    // Calculate the bin size for x- and y-axis
    xBinSize[ii] = (xMax[ii] - xMin[ii])/nBinsX[ii];
    
    // Allocate the memory for the monitor data
    dataArray[ii] = (double*) malloc(nBinsX[ii] * sizeof(double));
    dataArrayError[ii] = (double*) malloc(nBinsX[ii] * sizeof(double));
    dataArrayCounts[ii] = (int*) malloc(nBinsX[ii] * sizeof(int));
    
    for (int i = 0; i < nBinsX[ii]; i++) {
      
      dataArray[ii][i]=0;
      dataArrayCounts[ii][i]=0;
      
    }
    
    monSwitchedOn[ii] = 1;

    // If polarisation analysis is desired, here the rotation matrix and the weights container are defined
    if (analysePol) {
      
      polAnalysisRotMatrix =  MathMatrix::RotMatrixXFromVector(polAnalysisVector);
      
      dataArrayPolWeights[ii] = (double*) malloc(nBinsX[ii] * sizeof(double));
      
      for (int i = 0; i < nBinsX[ii]; i++) {
	
	dataArrayPolWeights[ii][i]=0;
	
      }
    }
  }
  return;

}


int Mon1D::FillMonitorArray(Neutron* n)
{
  
  int passed = 1;

  for (int i = 0; i < 3; i++) {

    if (monSwitchedOn[i]) passed &= FillMonitor(n, i);

  }

  return passed;

}
// Fill the monitor with the data of the neutron under study
int Mon1D::FillMonitor(Neutron* n, int counter)
{
  
  // Find or calculate the parameter set for the x-axis, dismiss if outside the range
  double xValue = DetermineParameter(xParam[counter], n);  
  int binX = (int)((xValue - xMin[counter])/xBinSize[counter]);
  if (binX < 0 || binX >= nBinsX[counter]) return 0;
  

  // Dismiss if outside the wavelength range, if defined
  if (lambdaMin >= 0 || lambdaMax > 0) {
    if (n->Wavelength < lambdaMin || n->Wavelength > lambdaMax) return 0;
  }

  // Dismiss if outside the range of filter parameter 1, if defined
  if (filterParam1 > 0 && (filterParam2 <= 0 || filterComb==1)) {
    double filterValue1 = DetermineParameter(filterParam1, n);
    if (filterValue1 < filterVarMin1 || filterValue1 > filterVarMax1) return 0;  
}

  // Dismiss if outside the range of filter parameter 2, if defined
  if (filterParam2 > 0 && (filterParam1 <= 0 || filterComb==1)) {
    double filterValue2 = DetermineParameter(filterParam2, n);
    if (filterValue2 < filterVarMin2 || filterValue2 > filterVarMax2) return 0;
  }

  // Dismiss if outside the range of filter parameter 1 and 2 (pass if fulfilled 1 OR 2)
  if (filterComb==0 && filterParam1 > 0 && filterParam2 > 0) {
    double filterValue1 = DetermineParameter(filterParam1, n);
    double filterValue2 = DetermineParameter(filterParam2, n);
    if ( (filterValue1 < filterVarMin1 || filterValue1 > filterVarMax1) && (filterValue2 < filterVarMin2 || filterValue2 > filterVarMax2)) return 0;  
}

  // Fill the monitor data if no polarisation analysis required
  if (!analysePol) {
    if (pWeight) dataArray[counter][binX] += n->Probability;
    else dataArray[counter][binX] += 1.0;
  }

  // If polarisation analysis required, include additional weight 
  // being the neutron spin component parallel to the analysis direction
  else {

    MathVector spinVector (n->Spin[0], n->Spin[1], n->Spin[2]);
    MathVector spinVectorProj = (*polAnalysisRotMatrix)*spinVector;

    if (pWeight) {
      dataArray[counter][binX] += n->Probability*spinVectorProj.x[0];
      dataArrayPolWeights[counter][binX] += n->Probability;
    }
    else {
      dataArray[counter][binX] += spinVectorProj.x[0];
      dataArrayPolWeights[counter][binX] += 1.0;
    }
  }

  dataArrayCounts[counter][binX]++;

  return 1;

}



double Mon1D::DetermineParameter(int id, Neutron* n)
{

  // Return the parameter value identified by 'id'

  double paramValue = 0;
  
  MathVector neutronVector (n->Vector[0], n->Vector[1], n->Vector[2]);
  MathVector neutronPosition (n->Position[0], n->Position[1], n->Position[2]);
  MathVector neutronPositionProjYZ (n->Position[1], n->Position[2], 0);
  
  double divy = 0;
  double divz = 0;

  switch (id) {
  case 1:
    paramValue = n->Position[1]; // y-pos
    break;
    
  case 2:
    paramValue = n->Position[2]; // z-pos
    break;
    
   case 3:   
    if ( neutronVector.x[0] >= 0) paramValue = atan2(neutronVector.x[1], sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI; //y divergence
    else paramValue = atan2(neutronVector.x[1], -sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI;
    break;
    
  case 4:
    paramValue = atan2(neutronVector.x[2], sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[1])))*180./M_PI; //z divergence
    break;
     

  case 5:
    paramValue = n->Wavelength; // wavelength
    break;
    
  case 6:
    paramValue = ENERGY_FROM_LAMBDA(n->Wavelength);  //energy
    break;
    
  case 7:
    paramValue = n->Time; // time
    break; 
    
  case 8:
    divy = neutronVector.Phi();
    paramValue = divy * 2. * M_PI / n->Wavelength; // ky: y component of the wave vector 
    break;
    
  case 9:
    neutronVector.x[1] = 0;
    if (neutronVector.x[2] > 0) divz = M_PI/2. - neutronVector.Theta(); 
    else divz = M_PI/2. - (neutronVector.Theta() + M_PI);
    paramValue = divz * 2. * M_PI / n->Wavelength;  // kz: z component of the wave vector
    break;
    
  case 10:
    neutronPosition.x[0] = 0;
    paramValue = neutronPosition.Mod(); // r: projection of the neutron vector on the y-z plane
    break;
    
  case 11:
    // phi angle of the r-phi cylindrical coordinate system corresponding to the y-z plane
    paramValue = neutronPositionProjYZ.Phi()*180./M_PI; 
    break;

  case 12:
    paramValue = (n->Color %100); //  colorTB: number of reflections at top or bottom plane
    break;

  case 13:
    paramValue = (n->Color - (n->Color%100) ) / 100;//  colorLR: number of reflections at left or right plane
    break;

  case 14:
    paramValue = (n->Color - (n->Color%100) ) / 100 + (n->Color %100); // color: number of reflections (colorTB+colorLR)
    break;
    
  case 15:  
    paramValue = neutronVector.PhiSc()*180./M_PI;
    break;

   case 16:  
    paramValue = neutronVector.ThetaSc()*180./M_PI;
    break;  

    case 17:  
    paramValue = neutronPosition.x[0];
    break;   

  default:
    fprintf(LogFilePtr,"unknown parameter ID: %d\n", id);
    exit(-1);
    break;

  }

  return paramValue;

}

// Write output file
void Mon1D::WriteOut()
{

  for (int ii = 0; ii < 3; ii++) {

    if (!monSwitchedOn[ii]) continue;

    for(int binx = 0; binx < nBinsX[ii]; binx++) {    
  
      if (dataArrayCounts[ii][binx]>0) 
	dataArrayError[ii][binx] = dataArray[ii][binx]*sqrt(1./dataArrayCounts[ii][binx]);
      
      // For polarisation analysis, divide the value in each bin by the sum of spin weights
      if (analysePol) 
	if (dataArrayPolWeights[ii][binx] > 0) dataArray[ii][binx]/=dataArrayPolWeights[ii][binx];	     
      
    }
    
 
    fprintf(fMonitor[ii],"#Monitor %s\n", weightTag[pWeight].c_str());
    fprintf(fMonitor[ii], "#x\ty\tDelta_y\tCounts\n");      
    for(int binx = 0; binx < nBinsX[ii]; binx++) {
      
      fprintf(fMonitor[ii],"%5.3f\t%5.3E\t%5.3E\t%d\n", ((xMin[ii] + xBinSize[ii]*binx) + (xMin[ii] + xBinSize[ii]*(binx+1.)))/2.0, 
	      dataArray[ii][binx], dataArrayError[ii][binx], dataArrayCounts[ii][binx]);     
      
    }
    fclose(fMonitor[ii]);
    
    // Give back the memory space
    free (dataArray[ii]);
    free (dataArrayError[ii]);
    free (dataArrayCounts[ii]);
    
    if (analysePol) {
      free (dataArrayPolWeights[ii]);
    }
    
  }
  
  return;
  
}


#endif
