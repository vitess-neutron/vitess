#ifndef MON2D_CPP
#define MON2D_CPP


#include "mon2D.h"


Mon2D::Mon2D()
{
  eModule = MCN_MONITOR2;

  dataArray = 0;
  dataArrayPolWeights = 0;

  xMin = -1;
  xMax = -1;
  yMin = -1;
  yMax = -1;

  nBinsX = 0;
  nBinsY = 0;

  xBinSize = 0;
  yBinSize = 0;

  xParam = -1;
  yParam = -1;

  fMonitor = 0;
  fMonitorFilename = "NoFile";

  weightTag[0] = "";
  weightTag[1] = "weight";

  formatTag[0] = "matrix";
  formatTag[1] = "xyz";

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

  pWeight = 1;
  exclCounts = 0;
  format = -1;
//  normalise = -1;

}


void Mon2D::OwnInit(int argc, char* argv[])
{
  // Read the command line arguments
  for (int i=1; i<argc; i++)
  {
    if(argv[i][0]!='+') 
    {
    	switch(argv[i][1])
	    {
	      case 'O':
	        fMonitorFilename=&argv[i][2];
	        break;

	      case 'X':
	        xParam = atoi(&argv[i][2]); // parameter to be shown on the x axis, input parameter
	        break;
	      case 'Y':
	        yParam = atoi(&argv[i][2]); // parameter to be shown on the y axis, input parameter

	      case 'x':
	         nBinsX = atol(&argv[i][2]); /* number of bins horizontal axis */
	        break;
	      case 'y':
	         nBinsY = atol(&argv[i][2]); /* number of bins vertical axis */
	        break;

	      case 'h':
	        yMin =  atof(&argv[i][2]);   /* bottom position window */
	        break;
	      case 'w':
	        xMin = atof(&argv[i][2]);		/* left edge position window */
	        break;

	      case 'H':
	        yMax =  atof(&argv[i][2]);   /* top position window */
	        break;
	      case 'W':
	        xMax = atof(&argv[i][2]);		/* right edge position window */
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
	
          case 'F':
          format = atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz, gnuplot readable */
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
  else
  {	
    fMonitor = OpenOutputFile(fMonitorFilename.c_str(), FALSE, "w");
    if (fMonitor ==NULL)
	  {
	    fprintf(LogFilePtr,"\nFile %s could not be opened for monitor2D output\n", fMonitorFilename.c_str());
	    exit(-1);
	  }
  }

  // Calculate the bin size for x- and y-axis
  xBinSize = (xMax - xMin)/nBinsX;
  yBinSize = (yMax - yMin)/nBinsY;

  // Allocate the memory for the monitor data
  dataArray       = (double**) malloc(nBinsX * sizeof(double*));
  dataArrayError  = (double**) malloc(nBinsX * sizeof(double*));
  dataArrayCounts =    (int**) malloc(nBinsX * sizeof(int*));
  for (int i = 0; i < nBinsX; i++) 
  {
    dataArray[i]       = (double*) malloc(nBinsY * sizeof(double));
    dataArrayError[i]  = (double*) malloc(nBinsY * sizeof(double));
    dataArrayCounts[i] =    (int*) malloc(nBinsY * sizeof(int));
  }

  for (int i = 0; i < nBinsX; i++) 
  {
    for (int j = 0; j < nBinsY; j++)
    {
      dataArray[i][j]=0.;
      dataArrayError[i][j]=0.;
      dataArrayCounts[i][j]=0;

    }
  }

  // If polarisation analysis is desired, here the rotation matrix and the weights container are defined
  if (analysePol) 
  {
    polAnalysisRotMatrix =  MathMatrix::RotMatrixXFromVector(polAnalysisVector);
    
    dataArrayPolWeights = (double**) malloc(nBinsX * sizeof(double*));
    for (int i = 0; i < nBinsX; i++) dataArrayPolWeights[i] = (double*) malloc(nBinsY * sizeof(double));
    
    for (int i = 0; i < nBinsX; i++) 
    {
      for (int j = 0; j < nBinsY; j++) 
      {
	    	dataArrayPolWeights[i][j]=0;
      }
    }
  }
  
  return;

}

// Fill the monitor with the data of the neutron under study
int Mon2D::FillMonitor(Neutron* n)
{
  
  // Find or calculate the parameter set for the x-axis, dismiss if outside the range
  double xValue = DetermineParameter(xParam, n);  
  int binX = (int)((xValue - xMin)/xBinSize);
  if (xValue < xMin || xValue > xMax) return 0;
  
  // Find or calculate the parameter set for the y-axis, dismiss if outside the range
  double yValue = DetermineParameter(yParam, n);  
  int binY = (int)((yValue - yMin)/yBinSize);
  if (yValue < yMin || yValue > yMax) return 0;

  // Dismiss if outside the wavelength range, if defined
  if (lambdaMin >= 0 || lambdaMax > 0) {
    if (n->Wavelength < lambdaMin || n->Wavelength > lambdaMax) return 0;
  }

  // Dismiss if outside the range of filter parameter 1, if defined (independent of filter 2: combined with AND)
  if (filterParam1 > 0 && (filterParam2 <= 0 || filterComb==1)) {
    double filterValue1 = DetermineParameter(filterParam1, n);
    if (filterValue1 < filterVarMin1 || filterValue1 > filterVarMax1) return 0;  
}

  // Dismiss if outside the range of filter parameter 2, if defined (independent of filter 1: combined with AND)
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
    if (pWeight) dataArray[binX][binY] += n->Probability;
    else dataArray[binX][binY] += 1.0;
  }

  // If polarisation analysis required, include additional weight 
  // being the neutron spin component parallel to the analysis direction
  else {

    MathVector spinVector (n->Spin[0], n->Spin[1], n->Spin[2]);
    MathVector spinVectorProj = (*polAnalysisRotMatrix)*spinVector;

    if (pWeight) {
      dataArray[binX][binY] += n->Probability*spinVectorProj.x[0];
      dataArrayPolWeights[binX][binY] += n->Probability;
    }
    else {
      dataArray[binX][binY] += spinVectorProj.x[0];
      dataArrayPolWeights[binX][binY] += 1.0;
    }
  }

  dataArrayCounts[binX][binY]++;

  return 1;

}


double Mon2D::DetermineParameter(int id, Neutron* n)
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
void Mon2D::WriteOut()
{
  // For polarisation analysis, divide the value in each bin by the sum of spin weights
  for(int binx = 0; binx < nBinsX; binx++) 
  {
    for(int biny = 0; biny < nBinsY; biny++) 
    {
      if (dataArray[binx][biny] > 0) 
      {
        dataArrayError[binx][biny] = dataArray[binx][biny]*sqrt(1./dataArrayCounts[binx][biny]);
        if (analysePol && dataArrayPolWeights[binx][biny] > 0) {
	        dataArray[binx][biny]/=dataArrayPolWeights[binx][biny];
        }
      }
    }
  }

 
  fprintf(fMonitor, "#Monitor %s %s\n", formatTag[format].c_str(), weightTag[pWeight].c_str());
  
  switch (format) 
  {
    case 0: // matrix format
      for(int binx = 0; binx < nBinsX; binx++)
      {
	        fprintf(fMonitor,"%5.3f\t",((xMin + xBinSize*binx) + (xMin + xBinSize*(binx+1.)))/2.0);
      }
      for(int biny = 0; biny < nBinsY; biny++)
      {
	      fprintf(fMonitor,"\n %5.3f\t",((yMin + yBinSize*biny) + (yMin + yBinSize*(biny+1.)))/2.0);
	      for(int binx = 0; binx < nBinsX; binx++)
	      {
	        fprintf(fMonitor,"%5.3E\t",dataArray[binx][biny]);
	      }
      }
      break;

    case 1: // xyz format
      fprintf(fMonitor, "#x  y  z\n");
      for(int biny = 0; biny < nBinsY; biny++) 
      {
        for(int binx = 0; binx < nBinsX; binx++) 
        {
		      fprintf(fMonitor,"%5.3f\t%5.3f\t%5.3E\t%5.3E\t%d\n", 
		      ((xMin + xBinSize*binx) + (xMin + xBinSize*(binx+1.)))/2.0, 
		      ((yMin + yBinSize*biny) + (yMin + yBinSize*(biny+1.)))/2.0,
		      dataArray[binx][biny], dataArrayError[binx][biny], dataArrayCounts[binx][biny]);
        }
        fprintf(fMonitor, "\n");
      }
      break;
  }
  if (fMonitor!=NULL)
    fclose(fMonitor);
}


void Mon2D::FreeMemory()
{
  // Give back the memory space
  for (int i = 0; i < nBinsX; i++) 
  {
    free(dataArray[i]);
    free(dataArrayError[i]);
    free(dataArrayCounts[i]);
  }
  free (dataArray);
  free(dataArrayError);
  free(dataArrayCounts);

  if (analysePol) 
  {
     for (int i = 0; i < nBinsX; i++) free(dataArrayPolWeights[i]);
     free (dataArrayPolWeights);
  }

  return; 
}


#endif
