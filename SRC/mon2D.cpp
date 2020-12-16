#ifndef MON2D_CPP
#define MON2D_CPP

#include "mon2D.h"


/******************************/
/** Constructor              **/
/******************************/
Mon2D::Mon2D()
{
  eModule = MCN_MONITOR2;
  fMonitorFilename = "NoFile";

  nBinsX = 0;
  nBinsY = 0;
  xParam = -1;
  yParam = -1;

  xMin = -1.0;
  xMax = -1.0;
  yMin = -1.0;
  yMax = -1.0;

  bWeight = 1;
  exclCounts = 0;
  format = NO_FORMAT;

  lambdaMin = -1;
  lambdaMax = -1;
  filterParam1 = -1;
  filterParam2 = -1;
  filterComb = -1;
  filterVarMin1 = -1.0;
  filterVarMin2 = -1.0;
  filterVarMax1 = -1.0;
  filterVarMax2 = -1.0;

  analysePol = 0;

  polAnalysisVector    = NULL;
  polAnalysisRotMatrix = NULL;  

  fMonitor = NULL;

  xBinSize = 0.0;
  yBinSize = 0.0;

  BinPosX   = NULL;
  BinPosY   = NULL;
  dataArray = NULL;
  dataArrayError  = NULL;
  dataArrayCounts = NULL;
  dataArrayPolWeights = NULL;
}


/**************************************************/
/** Reads input parameters and sets parameters   **/
/**************************************************/
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
	        bWeight  = atoi(&argv[i][2]);
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
          format = (VtFormat2D) atoi(&argv[i][2]);   /* file format for output, 0 = old matrix, 1 = new xyz, gnuplot readable */
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
    Error("you must define a MonitorOutputFile");
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
  BinPosX         = (double*) malloc((nBinsX+1) * sizeof(double));
  BinPosY         = (double*) malloc((nBinsY+1) * sizeof(double));
  dataArray       = (double*) malloc(nBinsY * nBinsX * sizeof(double));
  dataArrayError  = (double*) malloc(nBinsY * nBinsX * sizeof(double));
  dataArrayCounts =   (long*) malloc(nBinsY * nBinsX * sizeof(long));

  for (int i = 0; i <= nBinsX; i++)  BinPosX[i] = xMin + (xMax - xMin) * i / (double)nBinsX;
  for (int j = 0; j <= nBinsY; j++)  BinPosY[j] = yMin + (yMax - yMin) * j / (double)nBinsY;

  for (int i = 0; i < nBinsX; i++) 
  {
    for (int j = 0; j < nBinsY; j++)
    {
      int k = nBinsX * i + j; 
      dataArray[k]=0.;
      dataArrayError[k]=0.;
      dataArrayCounts[k]=0;
    }
  }

  // If polarisation analysis is desired, here the rotation matrix and the weights container are defined
  if (analysePol) 
  {
    polAnalysisRotMatrix =  MathMatrix::RotMatrixXFromVector(polAnalysisVector);
    
    dataArrayPolWeights = (double*) malloc(nBinsY * nBinsX * sizeof(double));
    
    for (int i = 0; i < nBinsX; i++) 
    {
      for (int j = 0; j < nBinsY; j++) 
      {
        int k = nBinsX * i + j; 
	    	dataArrayPolWeights[k]=0;
      }
    }
  }
  
  return;

}


/***********************************************************/
/** Fill the monitor with the data of the current neutron **/
/***********************************************************/
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

  int kBinXY = nBinsX * binX + binY; 

  // Dismiss if outside the wavelength range, if defined
  if (lambdaMin >= 0 || lambdaMax > 0) 
  {
    if (n->Wavelength < lambdaMin || n->Wavelength > lambdaMax) return 0;
  }

  // Dismiss if outside the range of filter parameter 1, if defined (independent of filter 2: combined with AND)
  if (filterParam1 > 0 && (filterParam2 <= 0 || filterComb==1)) 
  {
    double filterValue1 = DetermineParameter(filterParam1, n);
    if (filterValue1 < filterVarMin1 || filterValue1 > filterVarMax1) return 0;  
  }

  // Dismiss if outside the range of filter parameter 2, if defined (independent of filter 1: combined with AND)
  if (filterParam2 > 0 && (filterParam1 <= 0 || filterComb==1)) 
  {
    double filterValue2 = DetermineParameter(filterParam2, n);
    if (filterValue2 < filterVarMin2 || filterValue2 > filterVarMax2) return 0;
  }

  // Dismiss if outside the range of filter parameter 1 and 2 (pass if fulfilled 1 OR 2)
  if (filterComb==0 && filterParam1 > 0 && filterParam2 > 0) 
  {
    double filterValue1 = DetermineParameter(filterParam1, n);
    double filterValue2 = DetermineParameter(filterParam2, n);
    if ( (filterValue1 < filterVarMin1 || filterValue1 > filterVarMax1) && (filterValue2 < filterVarMin2 || filterValue2 > filterVarMax2)) return 0;  
  }

  // Fill the monitor data if no polarisation analysis required
  if (!analysePol) 
  {
    if (bWeight) dataArray[kBinXY] += n->Probability;
    else dataArray[kBinXY] += 1.0;
  }

  // If polarisation analysis required, include additional weight 
  // being the neutron spin component parallel to the analysis direction
  else 
  {
    MathVector spinVector (n->Spin[0], n->Spin[1], n->Spin[2]);
    MathVector spinVectorProj = (*polAnalysisRotMatrix)*spinVector;

    if (bWeight) 
    {
      dataArray[kBinXY] += n->Probability*spinVectorProj.x[0];
      dataArrayPolWeights[kBinXY] += n->Probability;
    }
    else 
    {
      dataArray[kBinXY] += spinVectorProj.x[0];
      dataArrayPolWeights[kBinXY] += 1.0;
    }
  }

  dataArrayCounts[kBinXY]++;

  return 1;
}


/*******************************************************/
/** Determine, which parameter has to be calculated   **/
/*******************************************************/
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


/******************************/
/** Write output file        **/
/******************************/
void Mon2D::WriteOut()
{
  char *sParX=NULL, *sParY=NULL;

  ParId2Text(sParX, xParam);
  ParId2Text(sParY, yParam);

  // For polarisation analysis, divide the value in each bin by the sum of spin weights
  for(int binx = 0; binx < nBinsX; binx++) 
  {
    for(int biny = 0; biny < nBinsY; biny++) 
    {
      int kBinXY = nBinsX * binx + biny; 
      if (dataArray[kBinXY] > 0) 
      {
        dataArrayError[kBinXY] = dataArray[kBinXY]*sqrt(1./dataArrayCounts[kBinXY]);
        if (analysePol && dataArrayPolWeights[kBinXY] > 0) 
        {
	        dataArray[kBinXY]/=dataArrayPolWeights[kBinXY];
        }
      }
    }
  }

  if (analysePol) 
    WriteHeader2D(fMonitor, format, "polarisation", bWeight,  nBinsX, sParX,           nBinsY, sParY);
  else
    WriteHeader2D(fMonitor, format, "Intensity",    bWeight,  nBinsX, sParX,           nBinsY, sParY);

  WriteOutput2D  (fMonitor, format,                 bWeight,  nBinsX, BinPosX, nBinsX, nBinsY, BinPosY,  dataArray, dataArrayError, dataArrayCounts);

  if (fMonitor!=NULL)
    fclose(fMonitor);
}


/***********************************/
/** Convert parameter ID to text  **/
/***********************************/
void Mon2D::ParId2Text(char* sParName, const int ePar)
{
  switch (ePar)
  { 
    case  0: strcpy(sParName, "pos_y");     break;
	  case  1: strcpy(sParName, "pos_z");     break;
	  case  2: strcpy(sParName, "div_y");     break;
	  case  3: strcpy(sParName, "div_z");     break;
	  case  4: strcpy(sParName, "lambda");    break;
	  case  5: strcpy(sParName, "energy");    break; 
	  case  6: strcpy(sParName, "time");      break; 
	  case  7: strcpy(sParName, "k_y");       break;
	  case  8: strcpy(sParName, "k_z");       break; 
	  case  9: strcpy(sParName, "pos_r");     break; 
	  case 10: strcpy(sParName, "pos_phi");   break;
	  case 11: strcpy(sParName, "col_vert");  break; 
	  case 12: strcpy(sParName, "col_hor");   break; 
	  case 13: strcpy(sParName, "color");     break; 
	  case 14: strcpy(sParName, "dir_phi");   break;
	  case 15: strcpy(sParName, "dir_theta"); break;
	  case 16: strcpy(sParName, "pos_x");     break;
  }
}


/******************************/
/** Free allocated memory    **/
/******************************/
void Mon2D::FreeMemory()
{
  // Give back the memory space
  free(dataArray);
  free(dataArrayError);
  free(dataArrayCounts);

  if (analysePol) 
  {
     free (dataArrayPolWeights);
  }

  return; 
}


#endif
