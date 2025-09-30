#ifndef MON2D_CPP
#define MON2D_CPP

#include "mon2D.h"
#include "convert.h"


/******************************/
/** Constructor              **/
/******************************/
Mon2D::Mon2D()
{
  eModule = MCN_MONITOR2;
  fMonitorFilename = "NoFile";

  nBinsX = 0;
  nBinsY = 0;
  xParam = NO_PAR;
  yParam = NO_PAR;

  xMin = -1.0;
  xMax = -1.0;
  yMin = -1.0;
  yMax = -1.0;

  bWeight = TRUE;
  exclCounts = 0;
  format = NO_2D_FORMAT;

  lambdaMin = -1;
  lambdaMax = -1;
  filterParam1 = NO_PAR;
  filterParam2 = NO_PAR;
  filterComb   = NO_FCOMB;
  filterVarMin1 = -1.0e10;
  filterVarMin2 = -1.0e10;
  filterVarMax1 =  1.0e10;
  filterVarMax2 =  1.0e10;

  analysePol = 0;

  polAnalysisVector    = NULL;
  polAnalysisRotMatrix = NULL;  

  fMonitor = NULL;

  nBunches = 1;
  nTrajTot = 0;
  IntTot   = 0.0;  

  xBinSize = 0.0;
  yBinSize = 0.0;

  BinPosX   = NULL;
  BinPosY   = NULL;
  dataArray       = NULL;
  dataArrayPol    = NULL;
  dataArrayError  = NULL;
  dataArrayCounts = NULL;
  dataArrayPolWeights = NULL;
}


/**************************************************/
/** Reads input parameters and sets parameters   **/
/**************************************************/
void Mon2D::OwnInit(int argc, char* argv[])
{
  nBunches = ReadNumBnch();

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
	        xParam = (VtMonPar)atoi(&argv[i][2]); // parameter to be shown on the x axis, input parameter
	        break;
	      case 'Y':
	        yParam = (VtMonPar)atoi(&argv[i][2]); // parameter to be shown on the y axis, input parameter

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
	        filterParam1 = (VtMonPar)atoi(&argv[i][2]); // filter parameter 1, optional input parameter
	        break;

	      case 'J':  
	        filterParam2 = (VtMonPar)atoi(&argv[i][2]); // filter parameter 2, optional input parameter
	        break;

	      case 'C':  
	        filterComb = (VtFiltComb)atoi(&argv[i][2]); // filter combination (AND,OR)
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
    Error("you must define a MonitorOutputFile");

  // Calculate the bin size for x- and y-axis
  xBinSize = (xMax - xMin)/nBinsX;
  yBinSize = (yMax - yMin)/nBinsY;

  // Allocate the memory for the monitor data
  BinPosX         = (double*)  malloc((nBinsX+1) * sizeof(double));
  BinPosY         = (double*)  malloc((nBinsY+1) * sizeof(double));

  dataArray       = (double**) malloc(nBinsX * sizeof(double*));
  dataArrayError  = (double**) malloc(nBinsX * sizeof(double*));
  dataArrayCounts =   (long**) malloc(nBinsX * sizeof(long*));

  for (int i=0; i < nBinsX; i++) 
  {
    dataArray[i]       = (double*) malloc(nBinsY * sizeof(double));
    dataArrayError[i]  = (double*) malloc(nBinsY * sizeof(double));
    dataArrayCounts[i] = (long*)   malloc(nBinsY * sizeof(long));
  }

  for (int i=0; i <= nBinsX; i++)  BinPosX[i] = xMin + (xMax - xMin) * i / (double)nBinsX;
  for (int j=0; j <= nBinsY; j++)  BinPosY[j] = yMin + (yMax - yMin) * j / (double)nBinsY;

  for (int i=0; i < nBinsX; i++) 
  { for (int j=0; j < nBinsY; j++)
    {
      dataArray      [i][j]=0.0;
      dataArrayError [i][j]=0.0;
      dataArrayCounts[i][j]=0;
    }
  }

  // If polarisation analysis is desired, here the rotation matrix and the weights container are defined
  if (analysePol) 
  {
    polAnalysisRotMatrix = MathMatrix::RotMatrixXFromVector(polAnalysisVector);
    
    dataArrayPolWeights = (double**) malloc(nBinsX * sizeof(double*));
    dataArrayPol        = (double**) malloc(nBinsX * sizeof(double*));

    for (int i=0; i < nBinsX; i++) 
    { dataArrayPolWeights[i] = (double*) malloc(nBinsY * sizeof(double));
      dataArrayPol       [i] = (double*) malloc(nBinsY * sizeof(double));
    }
    
    for (int i=0; i < nBinsX; i++) 
    { for (int j=0; j < nBinsY; j++)
      {
	    	dataArrayPolWeights[i][j]=0.0;
	    	dataArrayPol       [i][j]=0.0;
      }
    }
  }
  
  return;
}


/***********************************************************/
/** Fill the monitor with the data of the current neutron **/
/***********************************************************/
int Mon2D::FillMonitor(Neutron* pNeutr)
{
  short  bPar1=UNUSED, bPar2=UNUSED;
  double ParValue=0.0;
  
  // Find or calculate the parameter set for the x-axis, dismiss if outside the range
  double xValue = DetermineParameter(xParam, pNeutr);  
  int iBinX = (int)((xValue - xMin)/xBinSize);
  if (xValue < xMin || xValue >= xMax) return 0;
  
  // Find or calculate the parameter set for the y-axis, dismiss if outside the range
  double yValue = DetermineParameter(yParam, pNeutr);  
  int jBinY = (int)((yValue - yMin)/yBinSize);
  if (yValue < yMin || yValue >= yMax) return 0;

  // Dismiss if outside the wavelength range, if defined
  if (lambdaMin >= 0 || lambdaMax > 0) 
  {
    if (pNeutr->Wavelength < lambdaMin || pNeutr->Wavelength > lambdaMax) return 0;
  }

  // Dismiss if outside the range(s) considering the combination (AND or OR)
  if (filterParam1 > NO_PAR)
  { 
    ParValue = DetermineParameter(filterParam1, pNeutr);
    if (ParValue < filterVarMin1 || ParValue > filterVarMax1)
      bPar1=FALSE;  
    else
      bPar1=TRUE;
  }
  if (filterParam2 > NO_PAR)
  { 
    ParValue = DetermineParameter(filterParam2, pNeutr);
    if (ParValue < filterVarMin2 || ParValue > filterVarMax2)
      bPar2=FALSE;  
    else
      bPar2=TRUE;
  }
  if (filterComb==AND_AND_AND) 
  { if (bPar1==FALSE || bPar2==FALSE)
      return FALSE;
  }
  else // OR
  { if (!(bPar1==TRUE || bPar2==TRUE || bPar1==UNUSED && bPar2==UNUSED))
      return FALSE;
  }

  // Fill the monitor data if no polarisation analysis required
  if (!analysePol) 
  {
    if (bWeight) dataArray[iBinX][jBinY] += pNeutr->Probability;
    else dataArray[iBinX][jBinY] += 1.0;
  }

  // If polarisation analysis required, include additional weight 
  // being the neutron spin component parallel to the analysis direction
  else 
  {
    MathVector spinVector (pNeutr->Spin[0], pNeutr->Spin[1], pNeutr->Spin[2]);
    MathVector spinVectorProj = (*polAnalysisRotMatrix)*spinVector;

    if (bWeight) 
    {
      dataArray[iBinX][jBinY] += pNeutr->Probability*spinVectorProj.x[0];
      dataArrayPolWeights[iBinX][jBinY] += pNeutr->Probability;
    }
    else 
    {
      dataArray[iBinX][jBinY] += spinVectorProj.x[0];
      dataArrayPolWeights[iBinX][jBinY] += 1.0;
    }
  }

  dataArrayCounts[iBinX][jBinY]++;
  nTrajTot++;
  IntTot += pNeutr->Probability;

  return 1;
}


/*******************************************************/
/** Determine, which parameter has to be calculated   **/
/*******************************************************/
double Mon2D::DetermineParameter(VtMonPar id, Neutron* pNeutr)
{

  // Return the parameter value identified by 'id'

  double paramValue = 0;
  
  MathVector neutronVector  (pNeutr->Vector[0],   pNeutr->Vector[1],   pNeutr->Vector[2]);
  MathVector neutronPosition(pNeutr->Position[0], pNeutr->Position[1], pNeutr->Position[2]);
  MathVector neutronPositionProjYZ(pNeutr->Position[1], pNeutr->Position[2], 0);
  MathVector neutronPositionProjXY(pNeutr->Position[0], pNeutr->Position[1], 0);
  
  double divy = 0;
  double divz = 0;

  switch (id) 
  {
    case NO_PAR:  
      Error("parameter not defined");
      break;   
  
    case POS_X:  
      paramValue = pNeutr->Position[0]; // x-pos
      break;   
    case POS_Y:
      paramValue = pNeutr->Position[1]; // y-pos
      break;
    case POS_Z:
      paramValue = pNeutr->Position[2]; // z-pos
      break;
    
    case DIV_Y:   
      if ( neutronVector.x[0] >= 0) paramValue = atan2(neutronVector.x[1], sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI; //y divergence
      else paramValue = atan2(neutronVector.x[1], -sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI;
      break;
    case DIV_Z:
      paramValue = atan2(neutronVector.x[2], sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[1])))*180./M_PI; //z divergence
      break;
    
    case LAMBDA:
      paramValue = pNeutr->Wavelength; // wavelength
      break;
    case ENERGY:
      paramValue = ENERGY_FROM_LAMBDA(pNeutr->Wavelength);  //energy
      break;
    case TIME:
      paramValue = pNeutr->Time; // time
      break;
    
    case K_Y:
      divy = neutronVector.DivY();
      paramValue = divy * 2. * M_PI / pNeutr->Wavelength;  // ky: y component of the wave vector 
      break;
    case K_Z:
      divz = neutronVector.DivZ();
      paramValue = divz * 2. * M_PI / pNeutr->Wavelength;  // kz: z component of the wave vector
      break;
    
    case POS_R:
      neutronPosition.x[0] = 0;
      paramValue = neutronPosition.Mod(); // r: projection of the neutron vector on the y-z plane
      break;
    case POS_PHI:
      // phi angle of the r-phi cylindrical coordinate system corresponding to the y-z plane
      paramValue = neutronPositionProjYZ.Phi()*180./M_PI; 
      break;
    case POS_THETA:
      // orientation of pos_r in the x-y plane (in a cylindrical coordinate system)
      paramValue = neutronPositionProjXY.Phi()*180./M_PI;   
      break;
    
    case DIR_PHI:  
      paramValue = neutronVector.PhiSc()*180./M_PI;
      break;
    case DIR_THETA:  
      paramValue = neutronVector.ThetaSc()*180./M_PI;
      break;

    case COL_VERT:
      paramValue = (pNeutr->Color %100); //  colorTB: number of reflections at top or bottom plane
      break;
    case COL_HOR:
      paramValue = (pNeutr->Color - (pNeutr->Color%100) ) / 100;//  colorLR: number of reflections at left or right plane
      break;
    case COLOR:
      paramValue = (pNeutr->Color - (pNeutr->Color%100) ) / 100 + (pNeutr->Color %100); // color: number of reflections (colorTB+colorLR)
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
void Mon2D::WriteOut(long iBnch)
{
  char sParX[20]="", 
       sParY[20]="";
  double fNorm  = 1.0;               // ratio of total to processed bunches after treating current bunch

  ParId2Text(sParX, xParam);
  ParId2Text(sParY, yParam);

  // For polarisation analysis, divide the value in each bin by the sum of spin weights
  for (int iBinX = 0; iBinX < nBinsX; iBinX++) 
  { for (int jBinY = 0; jBinY < nBinsY; jBinY++) 
    {
      if (analysePol)
      { if (dataArrayCounts[iBinX][jBinY] > 0  && dataArrayPolWeights[iBinX][jBinY] > 0) 
        { dataArrayPol  [iBinX][jBinY] = dataArray   [iBinX][jBinY] / dataArrayPolWeights[iBinX][jBinY];
          dataArrayError[iBinX][jBinY] = dataArrayPol[iBinX][jBinY] / sqrt(dataArrayCounts[iBinX][jBinY]);
        }
        else
        { dataArrayPol  [iBinX][jBinY] = 0.0;
          dataArrayError[iBinX][jBinY] = 0.0;
        }
      }
      else
      { if (dataArrayCounts[iBinX][jBinY] > 0) 
          dataArrayError[iBinX][jBinY] = dataArray[iBinX][jBinY] / sqrt(dataArrayCounts[iBinX][jBinY]);
        else 
          dataArrayError[iBinX][jBinY] = 0.0;
      }
    }
  }

  fMonitor = OpenOutputFile(fMonitorFilename.c_str(), TRUE, "w");
  if (fMonitor!=NULL)
	{
    if (iBnch > 0 && nBunches > 1)
      fNorm = (double) nBunches / (double) iBnch;

    if (analysePol) 
    { WriteHeader2DB(fMonitor, FALSE, format, "polarisation", bWeight, iBnch, nBunches, IntTot, nTrajTot,  
                     nBinsX, sParX, xMin, xMax,  
                     nBinsY, sParY, yMin, yMax);
      WriteOutput2DB(fMonitor,        format,                 bWeight,  nBinsX, BinPosX,  nBinsY, BinPosY,  fNorm,  
                     dataArrayPol, dataArrayError, dataArrayCounts);
    }
    else
    { WriteHeader2DB(fMonitor, FALSE, format, "Intensity",    bWeight, iBnch, nBunches, IntTot, nTrajTot,  
                     nBinsX, sParX, xMin, xMax,  
                     nBinsY, sParY, yMin, yMax);
      WriteOutput2DB(fMonitor,        format,                 bWeight,  nBinsX, BinPosX,  nBinsY, BinPosY,  fNorm,  
                     dataArray, dataArrayError, dataArrayCounts);
    }
    fclose(fMonitor);
  }

  return;
}


/***********************************/
/** Convert parameter ID to text  **/
/***********************************/
void Mon2D::ParId2Text(char* sParName, const VtMonPar ePar)
{
  MonPar_ID2Txt(sParName, ePar);
  switch (ePar)
  { 
  /*case NO_PAR   : strcpy(sParName, "no_par");        break;
	  case POS_X    : strcpy(sParName, "pos_x/cm");      break;
    case POS_Y    : strcpy(sParName, "pos_y/cm");      break;
	  case POS_Z    : strcpy(sParName, "pos_z/cm");      break;
	  case DIV_Y    : strcpy(sParName, "div_y/deg");     break;
	  case DIV_Z    : strcpy(sParName, "div_z/deg");     break;
	  case LAMBDA   : strcpy(sParName, "lambda/Ang");    break;
	  case ENERGY   : strcpy(sParName, "energy/µeV");    break; 
	  case TIME     : strcpy(sParName, "time/ms");       break; 
	  case K_Y      : strcpy(sParName, "k_y/(1/Ang)");   break;
	  case K_Z      : strcpy(sParName, "k_z/(1/Ang)");   break; 
	  case POS_R    : strcpy(sParName, "pos_r/cm");      break; 
	  case POS_PHI  : strcpy(sParName, "pos_phi/deg");   break;
	  case DIR_PHI  : strcpy(sParName, "dir_phi/deg");   break;
	  case DIR_THETA: strcpy(sParName, "dir_theta/deg"); break;
	  case COL_VERT : strcpy(sParName, "col_vert");      break; 
	  case COL_HOR  : strcpy(sParName, "col_hor");       break; 
	  case COLOR    : strcpy(sParName, "color");         break; 
    default: strcpy(sParName, ""); */
	  case POS_X    : 
    case POS_Y    : 
    case POS_Z    : 
	  case POS_R    : strcat(sParName, " [cm]");    break;
    case DIV_Y    : 
	  case DIV_Z    : 
	  case POS_PHI  : 
    case DIR_PHI  : 
	  case DIR_THETA: strcat(sParName, " [deg]");   break;
	  case LAMBDA   : strcat(sParName, " [Ang]");   break;
	  case ENERGY   : strcat(sParName, " [µeV]");   break; 
	  case TIME     : strcat(sParName, " [ms]");    break; 
	  case K_Y      : 
	  case K_Z      : strcat(sParName, " [1/Ang]"); break; 
  }
}


/******************************/
/** Free allocated memory    **/
/******************************/
void Mon2D::FreeMemory()
{
  // Give back the memory space
  for (int i=0; i < nBinsX; i++) {
    free(dataArray[i]);
    free(dataArrayError[i]);
    free(dataArrayCounts[i]);
  }
  free(dataArray);
  free(dataArrayError);
  free(dataArrayCounts);
  free(BinPosX);
  free(BinPosY);

  if (analysePol) 
  {
    for (int i=0; i < nBinsX; i++) {
      free(dataArrayPolWeights[i]);
      free(dataArrayPol[i]);
    }
    free(dataArrayPolWeights);
    free(dataArrayPol);
  }
  if (polAnalysisVector) delete polAnalysisVector;
  if (polAnalysisRotMatrix) delete polAnalysisRotMatrix;

  return; 
}

#endif
