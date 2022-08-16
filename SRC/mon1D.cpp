#ifndef MON1D_CPP
#define MON1D_CPP

#include "mon1D.h"


/******************************/
/** Constructor              **/
/******************************/
Mon1D::Mon1D()
{
  eModule = MCN_MONITOR1;

  for (int i = 0; i < 3; i++) 
  {
    dataArray[i] = NULL;
    dataArrayError[i]  = NULL;
    dataArrayCounts[i] = NULL;
    dataArrayPolWeights[i] = NULL;
    
    xMin[i] = -1;
    xMax[i] = -1;
    
    nBinsX[i] = 0;
    
    nTrajTot[i] = 0;
    IntTot  [i] = 0.0;  
    xBinSize[i] = 0.0;
  
    eParX[i] = NO_PAR;

    monSwitchedOn[i] = 0;

    fMonitor[i] = NULL;
  }

  fMonitorFilename = "NoFile";

  bMultFiles= false;
  nBunches  =  1;
  lambdaMin = -1;
  lambdaMax = -1;

  analysePol = 0;

  polAnalysisVector    = NULL;
  polAnalysisRotMatrix = NULL;  

  filterVarMin1 = -1.0e10;
  filterVarMin2 = -1.0e10;
  filterVarMax1 =  1.0e10;
  filterVarMax2 =  1.0e10;

  filterParam1 = NO_PAR;
  filterParam2 = NO_PAR;
  filterComb   = NO_FCOMB;

  // normalise = -1; 

  bWeight = 1;
  exclCounts = 0;

  sParName[NO_PAR   ] = "no_par";   sParUnit[NO_PAR   ] = "-";
  sParName[POS_X    ] = "pos_x";    sParUnit[POS_X    ] = "cm";
  sParName[POS_Y    ] = "pos_y";    sParUnit[POS_Y    ] = "cm";
  sParName[POS_Z    ] = "pos_z";    sParUnit[POS_Z    ] = "cm";
  sParName[DIV_Y    ] = "div_y";    sParUnit[DIV_Y    ] = "deg";
  sParName[DIV_Z    ] = "div_z";    sParUnit[DIV_Z    ] = "deg";
  sParName[LAMBDA   ] = "lambda";   sParUnit[LAMBDA   ] = "Ang";
  sParName[ENERGY   ] = "energy";   sParUnit[ENERGY   ] = "µeV"; 
  sParName[TIME     ] = "time";     sParUnit[TIME     ] = "ms"; 
  sParName[K_Y      ] = "k_y";      sParUnit[K_Y      ] = "1/Ang";
  sParName[K_Z      ] = "k_z";      sParUnit[K_Z      ] = "1/Ang"; 
  sParName[POS_R    ] = "pos_r";    sParUnit[POS_R    ] = "cm"; 
  sParName[POS_PHI  ] = "pos_phi";  sParUnit[POS_PHI  ] = "deg";
  sParName[DIR_PHI  ] = "dir_phi";  sParUnit[DIR_PHI  ] = "deg";
  sParName[DIR_THETA] = "dir_theta";sParUnit[DIR_THETA] = "deg";
  sParName[COL_VERT ] = "col_vert"; sParUnit[COL_VERT ] = ""; 
  sParName[COL_HOR  ] = "col_hor";  sParUnit[COL_HOR  ] = ""; 
  sParName[COLOR    ] = "color";    sParUnit[COLOR    ] = ""; 
}       


/**************************************************/
/** Reads input parameters and sets parameters   **/
/**************************************************/
void Mon1D::OwnInit(int argc, char* argv[])
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
	        eParX[0] = (VtMonPar)atoi(&argv[i][2]); // parameter to be shown on the 1st x axis, input parameter
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
	        eParX[1] = (VtMonPar)atoi(&argv[i][2]); // 2nd parameter to be shown on the x axis, input parameter
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
	        eParX[2] = (VtMonPar)atoi(&argv[i][2]); // 3rd parameter to be shown on the x axis, input parameter
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
	        filterParam1 = (VtMonPar)atoi(&argv[i][2]); // filter parameter 1, optional input parameter
	        break;

        case 'J':  
	        filterParam2 = (VtMonPar)atoi(&argv[i][2]); // filter parameter 2, optional input parameter
	        break;

        case 'C':  
          filterComb   = (VtFiltComb)atoi(&argv[i][2]); // filter combination (AND,OR)
          break;

        case 'p':
	        bWeight  = atof(&argv[i][2]);
	        /* p=1 means probabilities activated, else neutron weight is set to 1.0 */
	        break;

        case 'P':
	        analysePol = atoi(&argv[i][2]); // 1 if polarisation analysis desired, optional input parameter
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

  // check for needed file name
  if (fMonitorFilename=="")
    Error("you must define a MonitorOutputFile");

  // check if more than 1 file is wanted
  int numberFiles = 0;

  for (int ii = 0; ii < 3; ii++) 
  {
    if (eParX[ii] > 0) numberFiles++;
  }

  if (numberFiles > 1) 
    bMultFiles = true;

  // allocate memory and initialize
  if (bWeight != 1) bWeight = 0;

  for (int ii = 0; ii < 3; ii++) 
  {

    if (eParX[ii] < 1) continue;
    
    // Calculate the bin size for x- and y-axis
    xBinSize[ii] = (xMax[ii] - xMin[ii])/nBinsX[ii];
    
    // Allocate the memory for the monitor data
    dataArray[ii]       = (double*) malloc(nBinsX[ii] * sizeof(double));
    dataArrayError[ii]  = (double*) malloc(nBinsX[ii] * sizeof(double));
    dataArrayCounts[ii] =    (int*) malloc(nBinsX[ii] * sizeof(int));
    
    for (int i = 0; i < nBinsX[ii]; i++) 
    {
      dataArray      [ii][i]=0.0;
      dataArrayError [ii][i]=0.0;
      dataArrayCounts[ii][i]=0;
    }
    
    monSwitchedOn[ii] = 1;

    // If polarisation analysis is desired, here the rotation matrix and the weights container are defined
    if (analysePol) 
    {
      polAnalysisRotMatrix =  MathMatrix::RotMatrixXFromVector(polAnalysisVector);
      
      dataArrayPolWeights[ii] = (double*) malloc(nBinsX[ii] * sizeof(double));
      
      for (int i = 0; i < nBinsX[ii]; i++) 
      {
	      dataArrayPolWeights[ii][i]=0;
      }
    }
  }
  return;

}


/**************************************************/
/** Fill all monitors chosen                     **/
/**************************************************/
int Mon1D::FillMonitorArray(Neutron* n)
{
  int passed = 1;

  for (int i = 0; i < 3; i++) 
  {
    if (monSwitchedOn[i]) passed &= FillMonitor(n, i);
  }

  return passed;
}


/***********************************************************/
/** Fill the monitor with the data of the current neutron **/
/***********************************************************/
int Mon1D::FillMonitor(Neutron* pNeutr, int counter)
{
  short  bPar1=UNUSED, bPar2=UNUSED;
  double ParValue=0.0;
  
  // Find or calculate the parameter set for the x-axis, dismiss if outside the range
  double xValue = DetermineParameter(eParX[counter], pNeutr);  
  int iBin = (int)((xValue - xMin[counter])/xBinSize[counter]);
  if (iBin < 0 || iBin >= nBinsX[counter]) return 0;
  

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
    if (bWeight) dataArray[counter][iBin] += pNeutr->Probability;
    else dataArray[counter][iBin] += 1.0;
  }
  // If polarisation analysis required, include additional weight 
  // being the neutron spin component parallel to the analysis direction
  else 
  {
    MathVector spinVector (pNeutr->Spin[0], pNeutr->Spin[1], pNeutr->Spin[2]);
    MathVector spinVectorProj = (*polAnalysisRotMatrix)*spinVector;

    if (bWeight) 
    {
      dataArray[counter][iBin] += pNeutr->Probability*spinVectorProj.x[0];
      dataArrayPolWeights[counter][iBin] += pNeutr->Probability;
    }
    else 
    {
      dataArray[counter][iBin] += spinVectorProj.x[0];
      dataArrayPolWeights[counter][iBin] += 1.0;
    }
  }

  dataArrayCounts[counter][iBin]++;
  nTrajTot[counter]++;

  return 1;
}


/*******************************************************/
/** Determine, which parameter has to be calculated   **/
/*******************************************************/
double Mon1D::DetermineParameter(VtMonPar id, Neutron* pNeutr)
{

  // Return the parameter value identified by 'id'

  double paramValue = 0;
  
  MathVector neutronVector (pNeutr->Vector[0], pNeutr->Vector[1], pNeutr->Vector[2]);
  MathVector neutronPosition (pNeutr->Position[0], pNeutr->Position[1], pNeutr->Position[2]);
  MathVector neutronPositionProjYZ (pNeutr->Position[1], pNeutr->Position[2], 0);
  
  double divy = 0;
  double divz = 0;

  switch (id) 
  {
    case NO_PAR:  
      Error("parameter not defined");
      break;   

    case POS_X:  
      paramValue = neutronPosition.x[0];
      break;   
    case POS_Y:
      paramValue = pNeutr->Position[1]; // y-pos
      break;
    case POS_Z:
      paramValue = pNeutr->Position[2]; // z-pos
      break;
    
    case DIV_Y:   
      if (neutronVector.x[0] >= 0) 
        paramValue = atan2(neutronVector.x[1], sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI; //y divergence
      else 
        paramValue = atan2(neutronVector.x[1], -sqrt(sq(neutronVector.x[0]) + sq(neutronVector.x[2])))*180./M_PI;
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
      divy = neutronVector.Phi();
      paramValue = divy * 2. * M_PI / pNeutr->Wavelength; // ky: y component of the wave vector 
      break;
    case K_Z:
      neutronVector.x[1] = 0;
      if (neutronVector.x[2] > 0) 
        divz = M_PI/2. -  neutronVector.Theta(); 
      else 
        divz = M_PI/2. - (neutronVector.Theta() + M_PI);
      paramValue = divz * 2. * M_PI / pNeutr->Wavelength;  // kz: z component of the wave vector
      break;
    
    case POS_R:
      neutronPosition.x[0] = 0;
      paramValue = neutronPosition.Mod(); // r: projection of the position vector on the y-z plane
      break;
    case POS_PHI:
      paramValue = neutronPositionProjYZ.Phi()*180./M_PI;   // orientation of pos_r in the y-z plane (in a cylindrical coordinate system)
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
      paramValue = (pNeutr->Color - (pNeutr->Color%100) ) / 100;//  colorLR: number of reflections on left or right plane
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
void Mon1D::WriteOut(long iBnch)
{
  int    iBin=0, nBinPol[3]={0,0,0};
  double fNorm  = 1.0;               // ratio of total to processed bunches after treating current bunch
  string fullFileName;

  for (int ii = 0; ii < 3; ii++) 
  {
    if (!monSwitchedOn[ii]) continue;

    IntTot[ii] = 0.0;
    for (iBin = 0; iBin < nBinsX[ii]; iBin++) 
    {    
      if (analysePol) 
      { 
        IntTot[ii] += dataArrayPolWeights[ii][iBin];
        if (dataArrayCounts[ii][iBin] > 0 && dataArrayPolWeights[ii][iBin] > 0) 
        { 
          dataArrayError[ii][iBin] = dataArray[ii][iBin] / dataArrayPolWeights[ii][iBin] / sqrt(dataArrayCounts[ii][iBin]);
          nBinPol[ii]++;
        }
      }
      else
      {
        IntTot[ii] += dataArray[ii][iBin];
        if (dataArrayCounts[ii][iBin] > 0) 
          dataArrayError[ii][iBin] = dataArray[ii][iBin] / sqrt(dataArrayCounts[ii][iBin]);
      }
    }

    if (bMultFiles) 
      fullFileName = fMonitorFilename + "_" + sParName[eParX[ii]] + ".dat";
    else
      fullFileName = fMonitorFilename;
    
    fMonitor[ii] = OpenOutputFile(fullFileName.c_str(), TRUE, "w");
    if (fMonitor[ii]!=NULL)
    {
      if (iBnch > 0 && nBunches > 1)
        fNorm = (double) nBunches / (double) iBnch;

      // For polarisation analysis, divide the value in each bin by the sum of spin weights
      if (analysePol) 
      { 
        WriteHeader1DB(fMonitor[ii], "polarisation", ANY_COLOR, iBnch, nBunches, nBinsX[ii], IntTot[ii], nTrajTot[ii], sParName[eParX[ii]].c_str(), sParUnit[eParX[ii]].c_str());
        for (iBin = 0; iBin < nBinsX[ii]; iBin++) 
        { if (dataArrayCounts[ii][iBin] > 0 && dataArrayPolWeights[ii][iBin] > 0) 
            fprintf(fMonitor[ii],"%10.4f  %12.5e %12.5e  %7ld\n", ((xMin[ii] + xBinSize[ii]*iBin) + (xMin[ii] + xBinSize[ii]*(iBin+1.)))/2.0, 
	                                                                 dataArray[ii][iBin]/dataArrayPolWeights[ii][iBin], dataArrayError[ii][iBin], dataArrayCounts[ii][iBin]);
        }
      }
      else
      { 
        WriteHeader1DB(fMonitor[ii], "intensity",    ANY_COLOR, iBnch, nBunches, nBinsX[ii], IntTot[ii], nTrajTot[ii], sParName[eParX[ii]].c_str(), sParUnit[eParX[ii]].c_str());
        for (iBin = 0; iBin < nBinsX[ii]; iBin++) 
        { fprintf(fMonitor[ii],"%10.4f  %12.5e %12.5e  %7ld\n", ((xMin[ii] + xBinSize[ii]*iBin) + (xMin[ii] + xBinSize[ii]*(iBin+1.)))/2.0, 
	                                                              fNorm * dataArray[ii][iBin], fNorm * dataArrayError[ii][iBin], dataArrayCounts[ii][iBin]);
        }
      }

      fclose(fMonitor[ii]);
    }
  }
}


/******************************/
/** Free allocated memory    **/
/******************************/
void Mon1D::FreeMemory()
{
  for (int ii = 0; ii < 3; ii++) 
  {
    if (!monSwitchedOn[ii]) continue;

    // Give back the memory space
    free (dataArray[ii]);
    free (dataArrayError[ii]);
    free (dataArrayCounts[ii]);
    
    if (analysePol) 
    {
      free (dataArrayPolWeights[ii]);
    }
  }
  
  return;
}

#endif
