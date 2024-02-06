#ifndef MONOCHRCLASS_CPP
#define MONOCHRCLASS_CPP

#include "mathfunctions.h"
#include "monochrclass.h"
#include "convert.h"

#define DEL_ZETA_MAX 0.01

// constructor, sets all variables to zero
Monochromator::Monochromator()
{
  eModule=MCN_MONOCHROM;

  // Initialise member variables
  ParFileName  = ""; 
  GeomFileName = "";
  
  pGeomFile = NULL;

  eGeomOption  = SINGLE_CE;
  eFocGeom     = NO_FOCUSING; 
  bTransm      = FALSE;
  eMonoMode    = REFL_MONO;
  eMonoMove    = VT_MONO_FIX;
  d_spr_option = LORENTZIAN; 
  nRepete      = 1; 

  d_fwhm       = 0.0; 
  d_sigma      = 0.0;
  mu_abs       = 0.0;
  mu_scat      = 0.0;
  Reflectivity = 1.0; 
  Freq         = 0.0;
  RadiusPST    = 0.1;
  Zeta0        = 0.0;

  for (int i = 0; i < 2; i++) 
  { NumberCE   [i] = 1;
    mosaic_fwhm[i] = 0.0;
  }
  DevH  = 0.0; 
  DevV  = 0.0;
  GapH  = 0.0; 
  GapV  = 0.0;
  RadH  = 0.0;
  RadV  = 0.0;
  Psi0  = 0.0;

  SrfcHor   = 0.0; 
  SrfcVert  = 0.0; 
  BraggHor  = 0.0; 
  BraggVert = 0.0;
  OutHor    = 0.0; 
  OutVert   = 0.0; 

  eFrame    = VT_NO_FRAME;   // no decision about user defined frame  
  d_spacing = 0.0; 
  nOrderRefl= 0;

  for (int i = 0; i < 3; i++)
  {
    Transl[i] = 0.0; 
    PosCE0[i] = 0.0; 
    DimCE0[i] = 0.0; 

    Depth[i]  = 0.0;
    PosCE[i]  = 0.0; 
    DimCE[i]  = 0.0; 

    dSpacingSpreadParams  [i] = 0.0; 
    horMosaicSpreadParams [i] = 0.0; 
    vertMosaicSpreadParams[i] = 0.0;

    for (int j = 0; j < 3; j++)
    {  RotMatrixCE   [i][j] = 0.0; 
       RotMatrixCE0  [i][j] = 0.0; 
       RotMatrixBragg[i][j] = 0.0; 
       RotMatrixOut  [i][j] = 0.0; 
    }

    fNorm[i] = 0.0;
    fRndm[i] = 0.0;

    Pos [i]  = 0.0; 
    Dir [i]  = 0.0; 
  }

  PathLenTrans = 0.0;
  PathLenRefl  = 0.0;
  maxDeviation = 1.0;
  peakWL       = 0.0;
  braggAngleTot= 0.0;
  axisPhi      = 0.0;
  mosRndmDir   = 0;
  
  currentNeutron = NULL;
  TOF    = -1.0; 
  Prob   = -1.0; 
  NumOut =  0; 

  DelZetaMax = DEL_ZETA_MAX;
}


// reads and checks input parameters and calculates depending values
void Monochromator::OwnInit(int argc, char* argv[])
{
  int    k=0;
  double m_cut=M_CUT;
  char   sHV[2][11]={"horizontal", "vertical"},  // mosaic_fwhm
         sTr[2][ 8]={"blocked",    "treated"},   // bTransm
         sMT[13]="",                             // eMonoType
         sMM[26]="",                             // eMonoMove
         sSO[11]="";                             // d_spr_option

  while(argc>1)
  {
		if (argv[1][0]!='+') 
		{
      switch(argv[1][1])
      {
        // Main Window
        // -----------
        case 'P':
	        ParFileName=&argv[1][2];
	        break;
        case 'G':
	        GeomFileName=&argv[1][2];
	        break;

        case 'O':
	        eGeomOption = (VtMonoArrange) atoi(&argv[1][2]);
	        break;
        case 'X':
	        eMonoMode   = (VtMonoType) atoi(&argv[1][2]);
	        break;
        case 'B':
	        sscanf(&argv[1][2], "%d", &bTransm) ;
	        break;
        case 'd':
	        d_spr_option = (VtDistr) atoi(&argv[1][2]);
	        break;
        case 'b':
	        eMonoMove   = (VtMonoMove) atoi(&argv[1][2]);
	        break;
        case 'g':
	        eFocGeom    = (VtMonoFocus) atoi(&argv[1][2]);
	        break;

        case 'A':
	        sscanf(&argv[1][2], "%d", &nRepete) ;
	        break;
        case 'f':
	        sscanf(&argv[1][2], "%lf", &Freq) ;
	        break;
        case 'p':
	        sscanf(&argv[1][2], "%lf", &Zeta0) ;
	        break;
        case 'w':
	        sscanf(&argv[1][2], "%lf", &RadiusPST) ;
	        break;

        case 'D':
	        sscanf(&argv[1][2], "%lf", &d_fwhm) ;
	        break;
        case 'c':
	        sscanf(&argv[1][2], "%lf", &mu_scat) ;
	        break;
        case 'C':
	        sscanf(&argv[1][2], "%lf", &mu_abs) ;
	        break;
        case 'R':
	        sscanf(&argv[1][2], "%lf", &Reflectivity) ;
	        break;

        case 'm':
	        sscanf(&argv[1][2], "%lf", &mosaic_fwhm[0]) ;
	        break;
        case 'M':
	        sscanf(&argv[1][2], "%lf", &mosaic_fwhm[1]) ;
	        break;
        case 't':
	        sscanf(&argv[1][2], "%lf", &DevH) ;
	        break;
        case 'T':
	        sscanf(&argv[1][2], "%lf", &DevV) ;
	        break;
        case 'h':
	        sscanf(&argv[1][2], "%lf", &GapH) ;
	        break;
        case 'v':
	        sscanf(&argv[1][2], "%lf", &GapV) ;
	        break;
        case 'H':
	        sscanf(&argv[1][2], "%d", &NumberCE[0]) ;
	        break;
        case 'V':
	        sscanf(&argv[1][2], "%d", &NumberCE[1]) ;
	        break;

        case 'r':
	        sscanf(&argv[1][2], "%lf", &RadV) ;
	        break;
        case 'a':
	        sscanf(&argv[1][2], "%lf", &Psi0) ;
	        break;
        case 's':
	        sscanf(&argv[1][2], "%lf", &RadH) ;
	        break;

        // Parameter file or main window
        // -----------------------------
        // position and size of the monochromator element
        case 'x':
          PosCE0[0] = atof(&argv[1][2]);
          break;
        case 'y':
          PosCE0[1] = atof(&argv[1][2]);
          break;
        case 'z':
          PosCE0[2] = atof(&argv[1][2]);
          break;

        case 'i':
          DimCE0[0] = atof(&argv[1][2]);
          break;
        case 'j':
          DimCE0[1] = atof(&argv[1][2]);
          break;
        case 'k':
          DimCE0[2] = atof(&argv[1][2]);
          break;

        // information about the reflecting plane
        case 'l':
          BraggHor = atof(&argv[1][2]);
          break;
        case 'L':
          BraggVert = atof(&argv[1][2]);
          break;

        case 'e':
          SrfcHor = atof(&argv[1][2]);
          break;
        case 'E':
          SrfcVert = atof(&argv[1][2]);
          break;

        case 'S':
          d_spacing = atof(&argv[1][2]);
          break;
        case 'N':
          nOrderRefl = atoi(&argv[1][2]);
          break;

        // Output frame 
        case 'F':
          eFrame = (VtFrameGen) atoi(&argv[1][2]);
          break;

        case 'W':
          Transl[0] = atof(&argv[1][2]);
          break;
        case 'Y':
          Transl[1] = atof(&argv[1][2]);
          break;
        case 'Z':
          Transl[2] = atof(&argv[1][2]);
          break;

        case 'u':
          OutHor  = atof(&argv[1][2]);
          break;
        case 'U':
          OutVert = atof(&argv[1][2]);
          break;

        default:
          Error2("unkown command option", argv[1]);
          exit(-1);
      }
    }
    argc--;
    argv++;
  }

  // Checks for possible  options   (1 crystal element, geometry calculated or from file
  if((eGeomOption != SINGLE_CE) && (eGeomOption != CE_ARRAY_CALC) && (eGeomOption != CE_ARRAY_FILE))
    Error("No valid option for the gemetry of the monochromator system found!!");

  // Option 'vertical cylinder' assumes only 1 column
  if (eFocGeom==VERT_CYL && NumberCE[0] > 1)
    Error("Only 1 column allowed for vertical cylinder geometry");
  
  // checks mosaicity   
  for (k=0; k<2; k++) 
  {
    if (mosaic_fwhm[k] < m_cut) 
    {
      mosaic_fwhm[k]	= m_cut;
      fprintf(LogFilePtr, "WARNING: minimum mosaicity %1.1e was set to %s mosaic spread!\n", m_cut, sHV[k]);
    }
  }

  MonoType_ID2Txt(sMT, eMonoMode);
  MonoMove_ID2Txt(sMM, eMonoMove);
  Distr_ID2Txt   (sSO, d_spr_option);

  fprintf(LogFilePtr, "  Mode: %s,  transmitted beam: %s\n",               sMT, sTr[bTransm]);
  if (eMonoMode!=VT_MONO_FIX)
    fprintf(LogFilePtr, "  %24.24s : %9.4f Hz %9.4f deg\n\n",              sMM, Freq, Zeta0);
  fprintf(LogFilePtr, "  attenuation   (scat, abs): %9.4f, %9.4f 1/cm\n",  mu_scat, mu_abs);
  fprintf(LogFilePtr, "  mosaic spread (hor, vert): %9.4f, %9.4f\n",       mosaic_fwhm[0], mosaic_fwhm[1]);
  fprintf(LogFilePtr, "  d-spread %10s      : %13.4e      \n",             sSO, d_fwhm);
  fprintf(LogFilePtr, "  reflectivity             : %9.4f \n",             Reflectivity);
  fprintf(LogFilePtr, "  repetition               : %4ld  \n",             nRepete);
  
  /* prints to log file */
  fprintf(LogFilePtr, "initialised option: ") ;
  if (eGeomOption == SINGLE_CE)
  { fprintf(LogFilePtr,"'crystal_flat'\n");
  }
  else if (eGeomOption == CE_ARRAY_CALC)
  {
    fprintf(LogFilePtr,"'crystal_focus'\n");
    fprintf(LogFilePtr,"  horizontal: number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg\n",                          NumberCE[0], RadH, GapH, DevH) ;
    fprintf(LogFilePtr,"  vertical  : number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg,  min. angle = %.3lf deg\n", NumberCE[1], RadV, GapV, DevV, Psi0);
    fprintf(LogFilePtr,"  focus file:                '%s'\n", GeomFileName);
  }
  else if (eGeomOption == CE_ARRAY_FILE)
  {
    fprintf(LogFilePtr,"'crystal_focus_dat'\n");
    fprintf(LogFilePtr,"  number of CE = %d, %d (h.,v.)\n", NumberCE[0], NumberCE[1]);
    fprintf(LogFilePtr,"  focus file:                '%s'\n", GeomFileName);
  }

  /* converts degs in radian etc. */
  mosaic_fwhm[0]  *= M_PI/180.0;
  mosaic_fwhm[1]  *= M_PI/180.0;
  d_fwhm          *= d_spacing;
  d_sigma          = d_fwhm/sqrt(8.0*log(2.0));
}


// Reads monochromator parameters and combines with them input parameters 
void Monochromator::setMonochrPar()
{
  FILE*  pFile = NULL;
  char   sLine[CHAR_BUF_SMALL]="";
  int    nLen=sizeof(sLine)-1, 
         iFrm  =-1,  n_ord=0;
  double x     =0.0, y    =0.0, z     =0.0,
         sfc_h =0.0, sfc_v=0.0,
         brg_h =0.0, brg_v=0.0,
         thickn=0.0, width=0.0, height=0.0,
         dsp   =0.0, 
         m_rng =0.0, d_rng=0.0,
         out_x =0.0, out_y=0.0, out_z =0.0, 
         out_h =0.0, out_v=0.0;
  VtFrameGen eFrm=VT_NO_FRAME; 

  /* Opens the parameter file if a file name is given */
  if (ParFileName!=NULL)
	{
    pFile = OpenInputFile(ParFileName, FALSE, "r");

    /* Reads the parameters if the file could be opened */
    if (pFile != NULL)
    { 
      /* reads from file by using ReadParF(Par_Crys) and ReadParComment(Par_Crys) */
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &x,     &y,     &z);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &sfc_h, &sfc_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &brg_h, &brg_v);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &thickn,&width, &height);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %d",      &dsp,   &n_ord);
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &m_rng, &d_rng);    // data not used in new monochromator module, kept to be able to use the same file
      if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%d",          &iFrm); 
      eFrm = (VtFrameGen) iFrm; 
      if (eFrm==VT_FRAME_USER)
      { 
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf %lf", &out_x,  &out_y,  &out_z);
        if (ReadLine(pFile, sLine, nLen)) sscanf(sLine, "%lf %lf",     &out_h,  &out_v);
      }

      fprintf(LogFilePtr, "data from parameter file   : '%s'\n",                ParFileName) ;

      // combines information from input and file, input parameters have priority
      if (eFrame==VT_NO_FRAME  && eFrm!=VT_NO_FRAME) eFrame = eFrm; 
      if (PosCE0[0]==0.0 && x     !=0.0) PosCE0[0]= x     ;
      if (PosCE0[1]==0.0 && y     !=0.0) PosCE0[1]= y     ;
      if (PosCE0[2]==0.0 && z     !=0.0) PosCE0[2]= z     ;
      if (SrfcHor  ==0.0 && sfc_h !=0.0) SrfcHor  = sfc_h ;
      if (SrfcVert ==0.0 && sfc_v !=0.0) SrfcVert = sfc_v ;
      if (BraggHor ==0.0 && brg_h !=0.0) BraggHor = brg_h ;
      if (BraggVert==0.0 && brg_v !=0.0) BraggVert= brg_v ;
      if (DimCE0[0]==0.0 && thickn!=0.0) DimCE0[0]= thickn;
      if (DimCE0[1]==0.0 && width !=0.0) DimCE0[1]= width ;
      if (DimCE0[2]==0.0 && height!=0.0) DimCE0[2]= height;
      if (d_spacing==0.0 && dsp   !=0.0) d_spacing = dsp  ;
      if (nOrderRefl==0  && n_ord !=0  ) nOrderRefl= n_ord;
      if (Transl[0]==0.0 && out_x !=0.0) Transl[0]= out_x ;
      if (Transl[1]==0.0 && out_y !=0.0) Transl[1]= out_y ;
      if (Transl[2]==0.0 && out_z !=0.0) Transl[2]= out_z ;
      if (OutHor   ==0.0 && out_h !=0.0) OutHor   = out_h ;
      if (OutVert  ==0.0 && out_v !=0.0) OutVert  = out_v ;

      // calculates values for output frame, unless transmission is treated
      if (eFrame==VT_FRAME_STD)
      { if (bTransm==FALSE)  
        {
          /* angles corresponding to the output frame: */
          AnglesOutputFrame(BraggHor, BraggVert, &OutHor, &OutVert);

          /* shifts output frame origin to center of focussing geometry */
          CopyVector(PosCE0, Transl) ;
        }
        else
        { Transl[0]=Transl[1]=Transl[2]=0.0;
          OutHor=OutVert=0.0;
        }
      }

      // initializes vectors to chosen CE element
      CopyVector(PosCE0, PosCE);
      CopyVector(DimCE0, DimCE);

      /* print parameters to log file for verification */
      fprintf(LogFilePtr, "  order of reflection = %d,   d-spacing =%9.4f\n",   nOrderRefl, d_spacing);
      fprintf(LogFilePtr, "  main position X, Y, Z    : %9.4f, %9.4f, %9.4f\n", PosCE0[0], PosCE0[1], PosCE0[2]);
      fprintf(LogFilePtr, "  thickness, width, height : %9.4f, %9.4f, %9.4f\n", DimCE0[0], DimCE0[1], DimCE0[2]);
      fprintf(LogFilePtr, "  Bragg angles   (hor,vert): %9.4f, %9.4f\n",        BraggHor,  BraggVert);
      fprintf(LogFilePtr, "  Surface angles (hor,vert): %9.4f, %9.4f\n",        SrfcHor,   SrfcVert);

      if (eFrame==VT_FRAME_USER) 
        fprintf(LogFilePtr,"user defined frame:\n") ;
      else  
        fprintf(LogFilePtr,"standard frame generation:\n") ;
      fprintf(LogFilePtr, "  output angles (hor, vert): %9.4f  %9.4f\n",        OutHor,    OutVert); 
	    fprintf(LogFilePtr, "  position X',Y',Z'        : %9.4f, %9.4f, %9.4f\n", Transl[0], Transl[1], Transl[2]);

      fclose(pFile);
    }
    else
    {	
      fprintf(LogFilePtr, "WARNING: Cannot open sample file %s\n", ParFileName);
    }
  }

  /* converts degs in radian etc. */
  SrfcHor   *= M_PI/180.0;
  SrfcVert	*= M_PI/180.0;
  BraggHor  *= M_PI/180.0;
  BraggVert *= M_PI/180.0;
  OutHor    *= M_PI/180.0;
  OutVert   *= M_PI/180.0;

  return;
}/* End ReadParameterFile */


// Determines the dependent parameters and write out important parameters 
void Monochromator::calcAndWritePar()
{
  // Filling the array of the d_spacing spread parameters 
  // for normalisation calculation and randomising
  dSpacingSpreadParams[0] = 1.;
  dSpacingSpreadParams[1] = d_spacing;
  if (d_spr_option == LORENTZIAN) dSpacingSpreadParams[2] = d_fwhm;
  else  dSpacingSpreadParams[2] = d_sigma;

  horMosaicSpreadParams[0] = 1.;
  horMosaicSpreadParams[1] = 0.;
  horMosaicSpreadParams[2] = mosaic_fwhm[0]/sqrt(8.*log(2.));

  vertMosaicSpreadParams[0] = 1.;
  vertMosaicSpreadParams[1] = 0.;
  vertMosaicSpreadParams[2] = mosaic_fwhm[1]/sqrt(8.*log(2.));

  // Copy Mosaic spread parameters to fRndm and fNorm depending on hor. and vert. Bragg angle
  FindPeakLambdaSortMosaicityDistr(BraggHor, BraggVert);

  // This function takes care of a proper normalisation of the fNorm mosaicity Gaussian 
  // to comply with the reflectivity value provided by the user
  NormFunction();

  fprintf(LogFilePtr,"Norm of the mosaic function: %1.4lf, peak wavelength %1.4lf \n", fNorm[0], peakWL) ;

  // here the the different geometry options are treated
  if (eGeomOption == SINGLE_CE)     // single CE
  {
    NumberCE[0] = NumberCE[1] = 1 ;
      
    CopyVectorToVectors(0, 0, PosCE0, PosCE_F) ;
    CopyVectorToVectors(0, 0, DimCE0, DimCE_F) ;

    std::vector <double> RotHorizVector;
    RotCEhor_F.push_back(RotHorizVector);
    RotCEhor_F[0].push_back(SrfcHor);
      
    std::vector <double> RotVertVector;
    RotCEvert_F.push_back(RotVertVector);
    RotCEvert_F[0].push_back(SrfcVert); 
  }
  else if (eGeomOption == CE_ARRAY_CALC)   // CE positions, dimensions and orientations calculated
  {
    if (eFocGeom==CONST_LMBD) crys_geomLambda();
    if (eFocGeom==SPHERICAL ) crys_geomSphere();
    if (eFocGeom==VERT_CYL  ) crys_geomVertCyl();
    if (eFocGeom==DBL_FOC   ) crys_geomDoubleCyl();
    addDev2Std();
  }
  else if (eGeomOption == CE_ARRAY_FILE)     // CE positions, dimensions and orientations from file
  {
    readFocFile() ;
    addDev2Std();
  }
  
  
  /* computes rotation matrixes corresponding to CE(i,j) offset angles */
  fillRotMatrices(Zeta0*M_PI/180.0);

  /* computes rotation matrix corresponding to the output frame (focus direction) */
  FillRotMatrixZY(RotMatrixOut, OutVert, OutHor);

  
  return; 
}


// Reads the parameters from the geometry file */
void Monochromator::readFocFile()
{
  int	i, j;
  
  pGeomFile = OpenInputFile(GeomFileName, FALSE, "r");

  if (pGeomFile==NULL)
	{
	  NumberCE[0]=0; 
    NumberCE[1]=0;
	  fprintf(LogFilePtr,"ERROR: focus file '%s' not found!\n", GeomFileName);
    exit(0);
	}     
  else
  { // reads the from file by using ReadParF() and ReadParComment()
    // number of columns and rows
    NumberCE[0] = ReadParI(pGeomFile) ; NumberCE[1] = ReadParI(pGeomFile) ; ReadParComment(pGeomFile) ;

    // loop over all crystal elements (1 line for each element)
    // reads 3 position deviations, 3 size deviations and 2 orientation deviations in each line
    // and adds these values to those  given for the monochromator center or central element resp.
    for(i=0; i<NumberCE[0]; i++)
    {
      std::vector <double> tempVector;
      PosCE_F[0].push_back(tempVector);
      PosCE_F[1].push_back(tempVector);
      PosCE_F[2].push_back(tempVector);
      DimCE_F[0].push_back(tempVector);
      DimCE_F[1].push_back(tempVector);
      DimCE_F[2].push_back(tempVector);
      RotCEhor_F.push_back(tempVector);
      RotCEvert_F.push_back(tempVector);

      for(j=0; j<NumberCE[1]; j++)
	    {
	      PosCE_F[0][i].push_back(ReadParF(pGeomFile));   PosCE_F[1][i].push_back(ReadParF(pGeomFile)); PosCE_F[2][i].push_back(ReadParF(pGeomFile));
	      DimCE_F[0][i].push_back(ReadParF(pGeomFile));   DimCE_F[1][i].push_back(ReadParF(pGeomFile)); DimCE_F[2][i].push_back(ReadParF(pGeomFile));
	      RotCEhor_F  [i].push_back(ReadParF(pGeomFile)); RotCEvert_F [i].push_back(ReadParF(pGeomFile));
	    }
    }

    fclose(pGeomFile);
  }
  return;
}/* End ReadFocFile */


// fills the structure stGeometry for visualization
void Monochromator::setGeometry(char* sColor)
{
  int k=0;  // index for geometrical elements in figure

  /* fills structure for instrument visalization */
  if (bVisInstr)
  { 
	  sprintf(sVisDescrpt, "%s:%s", sModuleName, sColor);
    stGeometry.pDescr  = sVisDescrpt;
    stGeometry.eModule = eModule;

    if(eGeomOption == SINGLE_CE) 
    {
      // Visualisation of the monochromator geometry
	    stGeometry.pCuboid  = (VtCuboid*) calloc(1, sizeof(VtCuboid));
	    stGeometry.nCuboids = 1; 
	
	    stGeometry.pCuboid[0].Length    = DimCE0[0]; 
	    stGeometry.pCuboid[0].Width     = BlowUp * DimCE0[1];
	    stGeometry.pCuboid[0].Height    = BlowUp * DimCE0[2];
	    stGeometry.pCuboid[0].vCntr[0]  = PosCE0[0];
	    stGeometry.pCuboid[0].vCntr[1]  = PosCE0[1];
	    stGeometry.pCuboid[0].vCntr[2]  = PosCE0[2];
	    stGeometry.pCuboid[0].vNormal[0]= 1.;
	    stGeometry.pCuboid[0].vNormal[1]= tan(SrfcHor);
	    stGeometry.pCuboid[0].vNormal[2]= tan(SrfcVert);
    }
    else
    {	
      k=0;
	    stGeometry.nCuboids = NumberCE[0]*NumberCE[1];
	    stGeometry.pCuboid = (VtCuboid*) calloc(stGeometry.nCuboids, sizeof(VtCuboid));
      
	    for (int i=0; i < NumberCE[0]; i++) 
      {
	      for (int j=0; j < NumberCE[1]; j++) 
        {
	        VectorType DimCurrCE, PosCurrCE;
	        double RotMatrixCurrCE[3][3];

	        CopyVectorsToVector (i, j, PosCE_F, PosCurrCE) ;
	        CopyVectorsToVector (i, j, DimCE_F, DimCurrCE) ;
	        CopyMatricesToMatrix(i, j, RotMatrixCE_F, RotMatrixCurrCE) ;

	        VectorType normal={1, 0, 0};
	        RotBackVector(RotMatrixCurrCE , normal);

	        stGeometry.pCuboid[k].Length    = DimCurrCE[0]; 
	        stGeometry.pCuboid[k].Width     = BlowUp * DimCurrCE[1];
	        stGeometry.pCuboid[k].Height    = BlowUp * DimCurrCE[2];
	        stGeometry.pCuboid[k].vCntr[0]  = PosCurrCE[0];
	        stGeometry.pCuboid[k].vCntr[1]  = PosCurrCE[1];
	        stGeometry.pCuboid[k].vCntr[2]  = PosCurrCE[2];
	        stGeometry.pCuboid[k].vNormal[0]= normal[0];
	        stGeometry.pCuboid[k].vNormal[1]= normal[1];
	        stGeometry.pCuboid[k].vNormal[2]= normal[2];

	        k++;
	      }
	    }
	  }
  }
  return;
}


// Calls Cleanup to write trajectory information and change to output co-ordinate system
void Monochromator::OwnCleanup()
{
  if (bTransm) 
    Cleanup(0, 0, 0, 0, 0);
  else 
    Cleanup(Transl[0], Transl[1], Transl[2], OutHor, OutVert);

  return;
} 


/*********************************************************************************************************************/

/* fills rotations matrices for the surface and the reflecting planes of the central CE (= monochromator) 
   and the surfaces of all CEs for a given Orientation of the rotating monochromator */
void Monochromator::fillRotMatrices(double MonoHor)
{
  int i, j ;
  double rotH, rotV, RotMatrix[3][3] ;

  /* computes rotation matrixes corresponding to Bragg angles */
  FillRotMatrixZY(RotMatrixCE0, SrfcVert, SrfcHor+MonoHor) ;

  /* computes rotation matrixes corresponding to Bragg angles */
  FillRotMatrixZY(RotMatrixBragg, BraggVert, BraggHor+MonoHor) ;

  for(i=0; i < NumberCE[0]; i++)
  {
    for(j=0; j < NumberCE[1]; j++)
      {
	      rotH = RotCEhor_F[i][j] + MonoHor;
	      rotV = RotCEvert_F[i][j] ;
	      FillRotMatrixZY(RotMatrix, rotV, rotH) ;
	      CopyMatrixToMatrices(i, j, RotMatrix, RotMatrixCE_F) ;
      }
  }

  return;
}

/* calculates angles corresponding to the output frame from the Bragg angles */
void Monochromator::AnglesOutputFrame(double RotH, double RotV, double *AnglFocH, double *AnglFocV)
{
  double RotMatrix[3][3];

  FillRotMatrixZY(RotMatrix, M_PI/180.*RotV, M_PI/180.*RotH) ;

  VectorType n={1, 0, 0};
  RotVector(RotMatrix, n) ; /* components of a vector parallel to X in the frame of the CE */
  n[0] *= -1. ; /* reflection on the CE */
  RotBackVector(RotMatrix, n) ;  /* new components in the frame of input */

  CartesianToEulerZY(n, AnglFocV, AnglFocH) ;

  if(*AnglFocH == - M_PI) *AnglFocH = M_PI ;
  if(*AnglFocV == - M_PI) *AnglFocV = M_PI ;
  *AnglFocH	*= 180./M_PI ;
  *AnglFocV	*= 180./M_PI ;

  return;
}

// This function calculates the peak wavelength based on the horizontal and vertical
// Bragg angles and the given d spacing. In addition, the phi angle between the
// instrument axis (direction of incoming trajectories) and normal of the CE
// being (1,0,0) determines which mosaicity distribution (vertical or horizontal) 
// is used for randomisation and which as a look-up for the absolute norm.
// See the help file for further details
void Monochromator::FindPeakLambdaSortMosaicityDistr(double RotH, double RotV)
{

  double tempMatrix[3][3];

  FillRotMatrixZY(tempMatrix, RotV, RotH) ;

  VectorType n={1, 0, 0};
  VectorType n2={1, 0, 0};

  RotVector(tempMatrix, n) ; /* components of a vector parallel to X in the frame of the CE */

  braggAngleTot = AngleVectors(n, n2)/180.*M_PI;
  peakWL = cos(braggAngleTot) * 2. * d_spacing;

  if (braggAngleTot > 1e-4) {
    double thetaTemp = 0;
    CartesianToSpherical(n, &thetaTemp, &axisPhi);
    if (axisPhi > M_PI_2) axisPhi = fabs(M_PI - axisPhi);
  }
  else {
    braggAngleTot = 1e-4;
    axisPhi = 0;
  }

  double sinP = sin(axisPhi);
  double cosP = cos(axisPhi);

 if (sinP >= cosP) {
   CopyVector(horMosaicSpreadParams, fRndm);
   CopyVector(vertMosaicSpreadParams, fNorm);
   mosRndmDir = 1;
 }
 else {
   CopyVector(vertMosaicSpreadParams, fRndm);
   CopyVector(horMosaicSpreadParams, fNorm);
   mosRndmDir = 2;
 }

 return;

}

// This function is called in Init() and calculates the proper normalisation of the mosaicity
// distribution function that is used to look up the absolute norm after reflection of a trajectory
void Monochromator::NormFunction()
{

  const int nBins=500;   // number of bins in array y[]
  int       index=  0;   // index in array y

  double expo  = 0.0,    // exponent in Gaussian intensity distribution
         ytemp = 0.0,    // intensity for ?
         x21   = 0.0,    // ? 
         sum   = 0.0,    // sum of the intensities * ? 
         weight= 0.0,    // sum of the intensities
         mean  = 0.0;    // mean reflectivity before normalization

  double y[nBins];
  for (int i = 0; i < nBins; i++) y[i] = 0.0;

  double sigma1 = dSpacingSpreadParams[2];
  double x1_incr = sigma1/100.;
  if (x1_incr == 0) 
    x1_incr = 1.;

  double sigma2 = fRndm[2];
  double x2_incr = sigma2/100.;
  if (x2_incr == 0) 
    x2_incr = 1.;

  double minSC = Min(sin(axisPhi), cos(axisPhi));
  maxDeviation = (1. + 2.*minSC - (minSC/sin(M_PI_2/2.)))*braggAngleTot;

  fprintf(LogFilePtr,"braggTot =%9.4f, sigma1 = %f, sigma2 = %f, maxDev = %f\n", (M_PI_2 - braggAngleTot)*180./M_PI, sigma1, sigma2, maxDeviation*180./M_PI);

  for (double x1 = d_spacing - 2.975*sigma1; x1 <= d_spacing + 2.975*sigma1; x1 += x1_incr) 
  {
    double prob_dspacing = 1.;

    if (sigma1 > 0) 
    {
      if(d_spr_option == LORENTZIAN) prob_dspacing =  sq(sigma1) / ( 4.*sq(x1 - d_spacing) + sq(sigma1) ) ;
      else prob_dspacing = dSpacingSpreadParams[0]*exp(-sq(x1 - dSpacingSpreadParams[1])/(2.*sq(sigma1)));   
    }
    double braggAngleDev = acos(peakWL/(2.*x1)) - acos(peakWL/(2.*d_spacing));

    if ((braggAngleTot - braggAngleDev) < 1e-5) continue;

    for (double x2 = -4.995*sigma2; x2 <= 4.995*sigma2; x2 += x2_incr)
    {
      if (fabs(x2) > maxDeviation) continue;

      ytemp = prob_dspacing * fRndm[0]*exp(-sq(x2 - fRndm[1])/(2.*sq(sigma2)));
      x21 = 0;
      
      DetermineMosaicAngle(braggAngleDev, x2, x21);

      expo = -sq(x21 - fNorm[1]) / (2.*sq(fNorm[2]));   
      index = mini((long)(fNorm[0] * exp(expo) * nBins), nBins-1);
      y[index] += ytemp;      
    }
  }

  for (int i = 0; i < nBins; i++) 
  {
    double x = 1./(2.*nBins) + (1./nBins)*((double) i);
    sum    += x*y[i];
    weight +=   y[i];
  }

  if (weight > 0) 
    mean = sum/weight;
  else 
    mean = 1.;

  // Here the reflectivity is incorporated into the norm of the 
  // mosaicity Gaussian that is used to look up the absolute weight
  // of a reflected neutron.
  fNorm[0] = Reflectivity/mean;

  return;

} 

/* During the normalisation calculation, this function is used to determine the value of the mosaic angle
 perpendicular to the one that is scanned in the Norm() method. 
 Input parameters: angleDiff = angular deviation from the given bragg angle due to the d_spacing spread
                   mosaicAngle1 = First mosaic angle from the scan in the Norm() method
                   mosaicAngle2 = Is filled with the solution found by the method 
 See the help file for further details. */                   
void  Monochromator::DetermineMosaicAngle(double angleDiff, double mosaicAngle1, double &mosaicAngle2)
{

  double r1 = tan(braggAngleTot + angleDiff) - tan(angleDiff); 
  double r2 = tan(braggAngleTot + angleDiff);
  double x = fabs(sin(mosaicAngle1));
  double gamma = 0;
  if (mosaicAngle1 < 0) gamma = M_PI/2. + fabs(axisPhi);
  else gamma = M_PI/2. - fabs(axisPhi);
		   
  double c_sq = r1*r1 + x*x - 2*r1*x*cos(gamma);  
  double alpha = acos(-1.*(r1*r1 - c_sq - x*x)/(2.*sqrt(c_sq)*x));
  double beta = 0;
  if (alpha <= M_PI/2) beta = M_PI/2. - alpha;// - fabs(theta);
  else beta = M_PI*3./2. - alpha;// - fabs(theta);

  double y1, y2;

  if ((c_sq*cos(beta)*cos(beta) - c_sq + r2*r2) > 0) {
    y1 = sqrt(c_sq)*cos(beta) + sqrt(c_sq*cos(beta)*cos(beta) - c_sq + r2*r2);
    y2 = sqrt(c_sq)*cos(beta) - sqrt(c_sq*cos(beta)*cos(beta) - c_sq + r2*r2);
  }
  else {
    y1 =  sqrt(c_sq)*cos(beta);
    y2 = y1;
  }
   
  double phi1, phi2;

  if (y1 <= 1) phi1 = asin(y1*cos(mosaicAngle1));
  else phi1 = M_PI;

  if (fabs(y2) <= 1) phi2 = asin(y2*cos(mosaicAngle1));
  else phi2 = M_PI;

  mosaicAngle2 = Min(fabs(phi1), fabs(phi2));

  if (sqrt(c_sq) < r2) mosaicAngle2 = fabs(mosaicAngle2)*(-1.);
  else mosaicAngle2 = fabs(mosaicAngle2);

  return;
  
}


/*********************************************************************************************************************/

// In this function the trajectory is propagated to the monochromator,
// reflected off the corresponding CE and weighted with the reflection probability
void Monochromator::processNeutron(Neutron* pNeutIn)
{
  bool    bHit=false;
  int     iRep=0,             // index of repetition
          kHit=0, lHit=0,     // (expected) index of column and row of CE that is hit
          iOrd=0,             // index of order 
          minOrd=0, maxOrd=0; // min. and max. order treated
  double  DelX=0.0, V0=0.0, Vx=0.0, Path=0.0;
  Neutron NeutInCE,     // data of the incoming neutron          (in the frame of the refl. plane
          NeutCES,      // data of the neutron on the CE surface (in the frame of the module)
          NeutPlCE,     // data of the neutron on the CE plane   (in the frame of the refl. plane)
          NeutPlRCE,    // data of the neutron on the CE plane   (in the frame of the rotating monochromator)
          NeutReflRCE,  // data of the reflected neutron         (in the frame of the rotating monochromator)
          NeutRefl,     // data of the reflected neutron         (in the frame of the module)
          NeutTrans,    // data of the transmitted neutron       (in the frame of the module)
          NeutTransOut, // data of the transmitted neutron       (in the output frame = module frame)
          NeutReflOut;  // data of the reflected neutron         (in the output frame)
  
  // init
  InitNeutron(&NeutInCE);   InitNeutron(&NeutCES);
  InitNeutron(&NeutPlCE);   
  InitNeutron(&NeutPlRCE);  InitNeutron(&NeutReflRCE);
  InitNeutron(&NeutRefl);   InitNeutron(&NeutReflOut);
  InitNeutron(&NeutTrans);  InitNeutron(&NeutTransOut);

  // Set the vector magnitude to unity
  pNeutIn->Vector[0] = sqrt(1.0 - sq(pNeutIn->Vector[1]) - sq(pNeutIn->Vector[2])) ;

  // moves monochromator if applicable, selects CE on which the neutron is reflected and returns variables in the frame of CE
  // 'rotateMonochr()' and 'translateMonoX' return already information which CE is hit
  // 'selectCE()' is needed to calculate the neutron parameters in the CE frame
  switch (eMonoMove)
  { 
    case VT_MONO_FIX: 
    case VT_MONO_PST: 
      bHit = selectCE(pNeutIn, &NeutInCE, 0, 0);
      break;
    case VT_MONO_ROT:
      bHit = rotateMonochr(kHit, lHit, pNeutIn);
      if (bHit) 
      { bHit = selectCE(pNeutIn, &NeutInCE, kHit, lHit);
        if (!bHit)                                    // check all CEs if the last rotation step changed the CE 
          bHit = selectCE(pNeutIn, &NeutInCE, 0, 0);  // which is in fact very unlikely
      }
      break;
    case VT_MONO_OSC:
      Error("Oscillating monochromator not yet realized");
      /*bHit = translateMonoX(kHit, lHit, pNeutIn);
      if (bHit) 
      { bHit = selectCE(pNeutIn, &NeutInCE, kHit, lHit);
        if (!bHit)                                    // check all CEs if the last rotation step changed the CE 
          bHit = selectCE(pNeutIn, &NeutInCE, 0, 0);  // which is in fact very unlikely
      }*/
      break;
  }

  if (bHit==false)  // no CE was found
  {
    // treatment of transmitted neutron
    if (bTransm==TRUE) 
    { 
      // write neutron with initial porperties    
      // (alternatively a propagation to the exit plane of the monochromator would be possible)
      CopyNeutron (pNeutIn, &NeutTrans);
      WriteNeutron(&NeutTrans);
      NumOut++;
    }
    // neutron is lost if transmitted neutrons are not regarded
    else
    {
      propNeutron2CE(pNeutIn, &NeutCES);
      WriteIAP(&NeutCES, VT_OUTSIDE);
    }
  }
  else
  {
    // Create a new trajectory for the transmitted neutrons and propagate it to the crystal element
    if (bTransm)
    { propNeutron2CE (pNeutIn, &NeutTrans);
      ChangeNeutronID(&NeutTrans);
    }

    // If a CE was found,  'NeutInCE' contains now position and direction of the incoming neutron in the frame of the reflecting crystal element.
    // Now 'NeutPlCE' is calculated which contains the properties of the neutron when arriving at the point of reflection 
    // (in the frame of the reflecting crystal element)
    DelX = 0.0 - NeutInCE.Position[0];
    V0   = V_FROM_LAMBDA(NeutInCE.Wavelength);
    Vx   = V0 * NeutInCE.Vector[0];
    TOF  = DelX / Vx;
    Path = V0 * TOF;

    CopyNeutron(&NeutInCE, &NeutPlCE);

    NeutPlCE.Time += TOF;
    for (int i = 0; i < 3; i++)
      NeutPlCE.Position[i] += NeutPlCE.Vector[i] * Path; //

    // for flat option (eGeomOption=SINGLE_CE) computes neutron direction in the "Bragg" frame keeping frame of CE for the position
    if (eGeomOption==SINGLE_CE)
    { RotBackVector(RotMatrixCE,    NeutPlCE.Vector);   // Vector is now back to the original frame
      RotVector    (RotMatrixBragg, NeutPlCE.Vector);   // Vector is now in the frame of the Bragg refl. plane
    }

    // if order=-1 is given all orders are treated, loop is left if order gets too high 
    if (nOrderRefl==ANY_COLOR)
    { minOrd = 1;
      maxOrd = 100;
    }
    else
    { minOrd = nOrderRefl;
      maxOrd = nOrderRefl;
    }

    for (iOrd=minOrd; iOrd <= maxOrd; iOrd++) 
    {
      for (iRep=0; iRep < nRepete; iRep++) 
      {
        double arg,                   // N lambda / 2 d
               pi2_bragg,             // pi/2 - Bragg angle   
               d_rnd = d_spacing;     // randomly varied d-spacing within given range

        // if (iRep > 0) ChangeNeutronID(&NeutRefl);
        NeutPlCE.Probability /= nRepete;

         /* random d-spacing */     
        if (d_fwhm > 0) 
        {
          if (d_spr_option==LORENTZIAN) d_rnd = RandomLorentzian(d_spacing, d_fwhm) ;
          if (d_spr_option==GAUSSIAN)   d_rnd = DistrGauss(d_spacing, d_sigma);  
        }

        if (eMonoMove==VT_MONO_PST)
          rotateMonoYZ(&NeutPlCE, &NeutPlRCE, Freq);
        else
          CopyNeutron(&NeutPlCE, &NeutPlRCE);
                 
        /* computes reflection angle corresponding to random d-spacindg */
        arg = NeutPlRCE.Wavelength * iOrd / 2.0 / d_rnd ;
        if (arg >= 1.05) return;   /* wavelength too large */
        if (arg >= 1.00) continue; /* wavelength for the chosen d-spacing too large */
        pi2_bragg = acos(arg) ;

        // Here the reflection probability is calculated
        // and the neutron trajectory changes direction after reflection from a mosaic element.
        CopyNeutron(&NeutPlRCE, &NeutReflRCE);
        NeutReflRCE.Probability *= calcReflProbAndDir(NeutReflRCE.Vector, NeutPlRCE.Vector, pi2_bragg);   

        if (eMonoMove==VT_MONO_PST)
          rotateMonoYZ(&NeutReflRCE, &NeutRefl, -Freq); 
        else
          CopyNeutron(&NeutReflRCE, &NeutRefl);

        if (NeutRefl.Probability < wei_min) 
          continue;

        // subtract reflection probability from transmission probability   	
        if (bTransm==TRUE)
          NeutTrans.Probability -= NeutRefl.Probability;

        /* computes neutron variables in the initial frame */
        RotBackVector(RotMatrixBragg, NeutRefl.Vector);
        RotBackVector(RotMatrixCE,    NeutRefl.Position) ;
        AddVector    (NeutRefl.Position, PosCE) ;

        /* makes depth correction to get back to the old frame for Depth != 0 */
        RotBackVector(RotMatrixCE, Depth) ;
        AddVector    (NeutRefl.Position, Depth) ;

        // fills structure for reflected neutron in output frame
        CopyNeutron(&NeutRefl, &NeutReflOut);

        // trajectory visualization 
        if (iRep == 0 && Prob > wei_min)
        {
          NeutRefl.Probability *= nRepete;
          WriteIAP(&NeutRefl, VT_REFLECTED); 
        }

        // Rotate the Vectors to the output frame 
        if (bTransm==FALSE)
        { SubVector(NeutReflOut.Position, Transl);
          RotVector(RotMatrixOut, NeutReflOut.Position);
          RotVector(RotMatrixOut, NeutReflOut.Vector);
        }

        /* writes output binary file */
        WriteNeutron(&NeutReflOut) ;
        NumOut++ ;
      }
    } // end of loop over reflected neutrons

    // transmitted neutron
    if (bTransm==TRUE)
    {
      transmitNeutron(&NeutTrans, &NeutTransOut);
      WriteIAP(&NeutTransOut, VT_PASSED);
      WriteNeutron(&NeutTransOut);
      NumOut++ ;
    }
  }
  return;
}

// for a rotating monochromator: rotate and 
bool Monochromator::rotateMonochr(int& kLast, int& lLast, const Neutron* pNeutIn)
{
  bool   bHit=false;
  int    k=0,l=0;                // indices for columns (k, hor) and rows (l, vert)
  double omega,                  // angular frequency of the monochromator rotation
         zeta0, zetaN, zetaO,    // initial, current and previous value of the rotation phase [rad]
         DelZeta,                // and their difference                                      [deg]
         ToFM=0.0,               // TOF from entrance to monochromator
         ToF0=pNeutIn->Time;     // TOF from source to entrance

  // init
  omega = 2.0*M_PI*Freq;
  zeta0 = Zeta0 * M_PI / 180.0;
  kLast = lLast = -1;
   
  // initial time estimation from monochromator center
  checkCE(ToFM, MathVector(PosCE0), DimCE0, MathMatrix(RotMatrixCE0), pNeutIn);
  zetaN = zeta0 + omega * (ToF0+ToFM)/1000.0;   // total TOF in sec
  fillRotMatrices(zetaN);

  do
  { 
    zetaO=zetaN;
    
    // try if it hits the same element as in the previous step
    if (kLast >=0 && lLast >= 0)
    { CopyVectorsToVector (kLast, lLast, PosCE_F, PosCE) ;
      CopyVectorsToVector (kLast, lLast, DimCE_F, DimCE) ;
      CopyMatricesToMatrix(kLast, lLast, RotMatrixCE_F, RotMatrixCE) ;

      bHit=checkCE(ToFM, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
      if (bHit)
      {
        zetaN = zeta0 + omega * (ToF0+ToFM)/1000.0;   // total TOF in sec
        fillRotMatrices(zetaN);
      }
    }

    if (bHit==false)
    { // loop over all crystal elements
      for (k=0; k < NumberCE[0]; k++) 
      {
        for (l=0; l < NumberCE[1]; l++) 
        {
          // copies position, size and orientation data for the current CE
          CopyVectorsToVector (k, l, PosCE_F, PosCE) ;
          CopyVectorsToVector (k, l, DimCE_F, DimCE) ;
          CopyMatricesToMatrix(k, l, RotMatrixCE_F, RotMatrixCE) ;

          bHit=checkCE(ToFM, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
          if (bHit)
          {
            kLast=k; lLast=l;
            zetaN = zeta0 + omega * (ToF0+ToFM)/1000.0;   // total TOF in sec
            fillRotMatrices(zetaN);
            goto check_rot;
          }
        }
      }
    }

  check_rot:
    DelZeta = fabs(zetaN-zetaO) * 180.0 / M_PI;
  }

  while (bHit && DelZeta > DelZetaMax);

  return bHit;
}

void Monochromator::rotateMonoYZ(Neutron* pNeutIn, Neutron* pNeutRot, double freq)
{
  VectorType PosCEChop;                            // position of the current CE in the frame of the central CE
  double y=0.0, z=0.0,                             // position of the neutron in the frame of the central CE
         vxMF=0.0, vyMF=0.0, vzMF=0.0, vModMF=0.0, // speed of the neutron in the frame of the rotating monochromator
         v0=0.0,  omega=0.0, angle=0.0, radius=0.0;

  CopyVector(PosCE,       PosCEChop);
  SubVector (PosCEChop,   PosCE0);
  RotVector (RotMatrixCE, PosCEChop);           // Vector to current CE is now in the frame of the central CE

  omega  = 2.0 * M_PI *freq; 
  v0     = V_FROM_LAMBDA(pNeutIn->Wavelength);
  y      = PosCEChop[1] + pNeutIn->Position[1];
  z      = RadiusPST + PosCEChop[2] + pNeutIn->Position[2];
  radius = sqrt(sq(y) + sq(z));
  angle  = atan2(y, z);

  vxMF   = v0*(pNeutIn->Vector[0]);
  vyMF   = v0*(pNeutIn->Vector[1]) - omega*radius*cos(angle)/1000.0;   // factor 1000 for cm/s -> cm/ms
  vzMF   = v0*(pNeutIn->Vector[2]) + omega*radius*sin(angle)/1000.0;
  vModMF = sqrt(sq(vxMF) + sq(vyMF) + sq(vzMF));

  CopyNeutron(pNeutIn, pNeutRot);
  pNeutRot->Vector[0]  = vxMF/vModMF;
  pNeutRot->Vector[1]  = vyMF/vModMF;
  pNeutRot->Vector[2]  = vzMF/vModMF;
  pNeutRot->Wavelength = LAMBDA_FROM_V(vModMF);
}

// void Monochromator::translateMonoX(Neutron* pNeutIn, Neutron* pNeutRot,double freq)
// {
//   double omega,
// 	 vxMF, vyMF, vzMF, V0, vModMF, radiusPST;
//
//   CopyNeutron(pNeutIn, pNeutRot);
//
//   omega = freq; // 2*M_1_PI*freq;
//   V0 = V_FROM_LAMBDA(pNeutIn->Wavelength);
//
//   vxMF = V0*(pNeutIn->Vector[0]) + omega*(pNeutIn->Position[0]);
//   vyMF = V0*(pNeutIn->Vector[1]);
//   vzMF = V0*(pNeutIn->Vector[2]);
//
//   vModMF = sqrt(sq(vxMF) + sq(vyMF) + sq(vzMF));
//
//   pNeutRot->Vector[0] = vxMF/vModMF;
//   pNeutRot->Vector[1] = vyMF/vModMF;
//   pNeutRot->Vector[2] = vzMF/vModMF;
//   pNeutRot->Wavelength = LAMBDA_FROM_V(vModMF);
// }

// checks if neutron hits a plane and calculates flight time until arrival   [ms]
bool Monochromator::checkCE(double& Time, const MathVector vPosCE, const VectorType SizeCE, const MathMatrix Mrot, const Neutron* pNeut)
{
  bool       bHit=false;
  double     t,          // time parameter
             dist;       // Distance between plane of reflection and incoming neutron
  MathVector vPos,                                        // neutron position in monochromator plane
             vPosRel,                                     // neutron position in the CE frame
             vNmlCE,                                      // plane normal 
             vPos0  = MathVector(pNeut->Position),        // initial neutron position 
             vDir0  = MathVector(pNeut->Vector),          // initial neutron fight direction
             vUnit  = MathVector(1.0, 0.0, 0.0);          // unit vector
  MathMatrix MrotInv= Mrot;

  Time = 0.0;
  MrotInv.transpose();     // rotation matrices are defined to rotate the frame,
                           // to rotate a vector, the inverse matrix must be used, 
                           // which is identical with the transposed matrix for rotation matrices
  vNmlCE = MrotInv * vUnit;

  dist = vPosCE * vNmlCE; 
  if (checkPlaneIntersect(vPos0, vDir0, vNmlCE, dist, t))
  { 
    vPos = vPos0 + vDir0*t;
    Time = t / V_FROM_LAMBDA(pNeut->Wavelength);

    vPosRel = Mrot * (vPos - vPosCE);
    bHit = isNeutInCE(vPosRel, SizeCE);
  }

  return bHit;
}


bool Monochromator::isNeutInCE(MathVector vPosN, const VectorType SizeCE)
{
  bool bIn = false;

  if (fabs(vPosN.getX()) <= 0.5*SizeCE[0] &&
      fabs(vPosN.getY()) <= 0.5*SizeCE[1] &&
      fabs(vPosN.getZ()) <= 0.5*SizeCE[2]   )
    bIn=true;

  return bIn;
}

/***********************************************************************
* Monochromator::selectCE
*
* in : pNeutIn  : neutron data in the local co-ordinate system of the monochromator module
* out: pNeutInCE: same neutron data in the co-ordinate system of the reflecting plane
*
* calculated: PathLenTrans, PathLenRefl
* determined: PosCE, RotMatrixCE, 
*             DimCE, RotMatrixBragg
*
* return: CE hit? true or false
*************************************************************************/
bool  Monochromator::selectCE(const Neutron* pNeutIn, Neutron* pNeutInCE, int kStart, int lStart)
{
  bool       bFound=false;
  int		     k, l;            // indices of crystal elements
  VectorType pos, dir;        // position and direction of the neutron in the frame of the CE surface 
  VectorType Pos1, Pos2;      // intersection points with crystal element

  // init
  CopyNeutron(pNeutIn, pNeutInCE);

  // loop over all crystal elements
  for (k=kStart; k<NumberCE[0]; k++) 
  {
    for (l=lStart; l<NumberCE[1]; l++) 
    {
      // copies position, size and orientation data for the current CE
      CopyVectorsToVector (k, l, PosCE_F, PosCE) ;
      CopyVectorsToVector (k, l, DimCE_F, DimCE) ;
      CopyMatricesToMatrix(k, l, RotMatrixCE_F, RotMatrixCE) ;

      // computes neutron variables in the frame of the CE surface
      CopyVector(pNeutIn->Position, pos) ;
      CopyVector(pNeutIn->Vector,   dir) ;
      SubVector(pos, PosCE) ;
      RotVector(RotMatrixCE, pos) ;
      RotVector(RotMatrixCE, dir) ;

      /* here computes the depth where the neutron meets the reflecting plane, 
         equivalent to a parallel shift of a t=0 CE in the frame of CE */
      if (IntersectionWithRectangular(DimCE, pos, dir, Pos1, Pos2) == TRUE)
      { 
        /* here we have the CE and initialise the values */
        bFound=true ;

        // vector from CE surface to the plane of reflection
        Depth[0] = MonteCarlo(Pos1[0], Pos2[0]) ;
        Depth[1] = Depth[2] = 0.0 ;

        // path lengths through the crystal
        PathLenTrans = DistVector(Pos1, Pos2);
        if (eMonoMode==REFL_MONO) 
          PathLenRefl = 2.0*(Depth[0]-Pos1[0]) * PathLenTrans/(Pos2[0]-Pos1[0]);  // reflection geometry, approximation, correct only for planes parallel to surface
        else 
          PathLenRefl = PathLenTrans;                                             // transmission geometry approximation, correct only for planes parallel to surface

        SubVector (pos, Depth);                 // neutron position in the frame of the reflecting surface
        CopyVector(pos, pNeutInCE->Position);
        CopyVector(dir, pNeutInCE->Vector);

        /* NOTE: in focusing geometry the Bragg frame and CE frame are coincident*/
        if (eGeomOption != SINGLE_CE) 
        { int p, q;
			
          for(p=0; p<3; p++) 
          { for(q=0; q<3; q++) 
              RotMatrixBragg[p][q] = RotMatrixCE[p][q];
          }
        }

        return bFound;
      }
    }
  }

  // use monochromator center if no CE was found
  CopyVector(PosCE0, PosCE);
  CopyVector(DimCE0, DimCE);

  return bFound;

}/* End SelectCE */

// This function calculates the reflection probability for a neutron trajectory.
// In processNeutron(), the d_spacing and therefore the corresponding Bragg angle has been determined 
// by randomisation. In this function, the mosaicity in one dimension (fRndm) is randomised,
// while the mosaicity in the perpendicular direction is determined such that the Bragg condition
// between the direction of the mosaic element normal and the neutron trajectory is satisfied. This is done 
// by solving a system consisting of 3 equations:
// Eq. 1: sin(mosaicAngle1) = y/x (if the randomisation direction is horizontal) or sin(mosaicAngle1) = z (vertical)
// Here, mosaicAngle1 is the randomised mosaicity angle and x, y, z are vector components of the mosaic piece normal.
// Eq. 2: Mag(x, y, z) = 1 // The magnitude of the mosaic piece normal is 1
// Eq. 3: (x, y, z)*(x_n, y_n, z_n) = cos(pi/2 - braggAngle)
// where (x_n, y_n, z_n) is the neutron direction vector in the frame of the CE normal.
// Out of 2 possible solutions for the mosaic vector the one with the smaller mosaic angle is taken,
// since the other one gives an angle of the order of the bragg angle.
double Monochromator::calcReflProbAndDir(VectorType DirOut, const VectorType DirIn, const double pi2_braggAngle)
{

  double tanM, tanMtilde, sinM, cosM, cBragg, alpha, beta, gamma;
  double mosaicAngle1, theta, phi;
  double norm=0.0;  

  double x1, x2;
  double angle11, angle12, angle21, angle22;

  double nTries = 1.;
  double maxNTries = (fRndm[2]/(maxDeviation/braggAngleTot*pi2_braggAngle))*5.;

  if (maxNTries < 1) maxNTries = 2;

  double x_n = DirIn[0];
  double y_n = DirIn[1];
  double z_n = DirIn[2];

  VectorType mosaicVector={1, 0, 0};

  while (nTries <= maxNTries) 
  {
    if (mosRndmDir == 1) 
    {
      mosaicAngle1 = DistrGauss(fRndm[1], fRndm[2]);


      tanM = tan(mosaicAngle1);
      tanMtilde = 1 + tanM*tanM;
      cBragg = cos(pi2_braggAngle);
    
      alpha = 2.*tanM*x_n*y_n/(z_n*z_n) + x_n*x_n/(z_n*z_n) + tanM*tanM*y_n*y_n/(z_n*z_n) + tanMtilde;
      beta = -1.*(cBragg*x_n/(z_n*z_n) + cBragg*tanM*y_n/(z_n*z_n));
      gamma = cBragg*cBragg/(z_n*z_n) - 1;
    
      x1 = (-1.*beta + sqrt(beta*beta - alpha*gamma))/alpha;
      x2 = (-1.*beta - sqrt(beta*beta - alpha*gamma))/alpha;

      double y1 = x1*tanM;
      double y2 = x2*tanM;
      
      double z11, z12;
      if (x1*x1 + y1*y1 < 1) 
      {
        z11 = sqrt(1. - x1*x1 - y1*y1);
        z12 = -sqrt(1. - x1*x1 - y1*y1);
      }
      else 
      {
        z11 = 0;
        z12 = 0;
      }
           
      VectorType vMos11 = {x1, y1, z11};
      VectorType vMos12 = {x1, y1, z12};      
      angle11 = AngleVectors(DirIn, vMos11)/180.*M_PI;
      angle12 = AngleVectors(DirIn, vMos12)/180.*M_PI;
      
      if (fabs(angle11 - pi2_braggAngle) < 1e-5) 
      {
        phi = atan2(z11,sqrt(x1*x1 + y1*y1));
        CopyVector(vMos11, mosaicVector);
      }
      else if (fabs(angle12 - pi2_braggAngle) < 1e-5) 
      {
        phi = atan2(z12,sqrt(x1*x1 + y1*y1));
        CopyVector(vMos12, mosaicVector);
      }
      else 
      {
        phi = 20.*fNorm[2];
      }

      // Use another solution if the resulting second mosaic angle
      // is too large, most probably there is a solution with a 
      // smaller mosaic angle that leads to a higher weight factor
      if (fabs(phi) > 10.*fNorm[2]) 
      {
        double z21, z22;
        if (x2*x2 + y2*y2 < 1) 
        {
	        z21 = sqrt(1. - x2*x2 - y2*y2);
	        z22 = -sqrt(1. - x2*x2 - y2*y2);
        }
        else {
	        z21 = 0;
	        z22 = 0;
        }
      
        VectorType vMos21 = {x2, y2, z21};
        VectorType vMos22 = {x2, y2, z22};      
        angle21 = AngleVectors(DirIn, vMos21)/180.*M_PI;
        angle22 = AngleVectors(DirIn, vMos22)/180.*M_PI;
      
        if (fabs(angle21 - pi2_braggAngle) < 1e-5) 
        {
	        phi = atan2(z21,sqrt(x2*x2 + y2*y2));
	        CopyVector(vMos21, mosaicVector);
        }
        else if (fabs(angle22 - pi2_braggAngle) < 1e-5)
        {
	        phi = atan2(z22,sqrt(x2*x2 + y2*y2));
	        CopyVector(vMos22, mosaicVector);
        }
        else {
	        nTries++;
	        continue;
        }
      
      }

      theta = mosaicAngle1;
      // Use the normalisation distribution defined in the NormFunction()
      // method to find a weight for the trajectory taking into account
      // the reflectivity value provided by the user
      norm = fNorm[0]*exp(-sq(phi - fNorm[1])/(2.*sq(fNorm[2])));
      break;
    }
    else  // mosRndmDir = 2
    {

      mosaicAngle1 = DistrGauss(fRndm[1], fRndm[2]);

      sinM = sin(mosaicAngle1);
      cosM = cos(mosaicAngle1);
      cBragg = cos(pi2_braggAngle);
    
      alpha = 1. + x_n*x_n/(y_n*y_n);
      beta =  sinM*x_n*z_n/(y_n*y_n) - cBragg*x_n/(y_n*y_n);
      gamma = cBragg*cBragg/(y_n*y_n) - cosM*cosM - 2.*cBragg*sinM*z_n/(y_n*y_n) + sinM*sinM*z_n*z_n/(y_n*y_n);
    
      x1 = (-1.*beta + sqrt(beta*beta - alpha*gamma))/alpha;
      x2 = (-1.*beta - sqrt(beta*beta - alpha*gamma))/alpha;

      double z1 = sinM;

      double y11, y12;
      if (z1*z1 + x1*x1 < 1) {
        y11 = sqrt(1 - z1*z1 - x1*x1);
        y12 = -sqrt(1 - z1*z1 - x1*x1);
      }
      else {
        y11 = 0;
        y12 = 0;
      }
    
      VectorType vMos11 = {x1, y11, z1};
      VectorType vMos12 = {x1, y12, z1};      
      angle11 = AngleVectors(DirIn, vMos11)/180.*M_PI;
      angle12 = AngleVectors(DirIn, vMos12)/180.*M_PI;

      if (fabs(angle11 - pi2_braggAngle) < 1e-5) 
      {
        theta = atan2(y11,x1);
        CopyVector(vMos11, mosaicVector);
      }
      else if (fabs(angle12 - pi2_braggAngle) < 1e-5) 
      {
        theta = atan2(y12,x1);
        CopyVector(vMos12, mosaicVector);
      }
      else 
      {
        theta = 20.*fNorm[2];
      }


      if (fabs(theta) > 10.*fNorm[2]) 
      {
        double y21, y22;
        if (z1*z1 + x2*x2 < 1) {
	        y21 = sqrt(1 - z1*z1 - x2*x2);
	        y22 = -sqrt(1 - z1*z1 - x2*x2);
        }
        else 
        {
	        y21 = 0;
	        y22 = 0;
        }
      
        VectorType vMos21 = {x2, y21, z1};
        VectorType vMos22 = {x2, y22, z1};      
        angle21 = AngleVectors(DirIn, vMos21)/180.*M_PI;
        angle22 = AngleVectors(DirIn, vMos22)/180.*M_PI;

        if (fabs(angle21 - pi2_braggAngle) < 1e-5) 
        {
	        theta = atan2(y21,x2);
	        CopyVector(vMos21, mosaicVector);
        }
        else if (fabs(angle22 - pi2_braggAngle) < 1e-5) 
        {
	        theta = atan2(y22,x2);
	        CopyVector(vMos22, mosaicVector);
        }
        else 
        {
	        nTries++;
	        continue;
        }
      
      }

      phi = mosaicAngle1;
      // Use the normalisation distribution defined in the NormFunction()
      // method to find a weight for the trajectory taking into account
      // the reflectivity value provided by the user
      norm = fNorm[0]*exp(-sq(theta - fNorm[1])/(2.*sq(fNorm[2])));
      break;
    }
  }

  // Here the new neutron direction is determined
  double mosaicMatrix[3][3];
  RotMatrixX(mosaicVector, mosaicMatrix);
  CopyVector(DirIn, DirOut);
  RotVector(mosaicMatrix, DirOut);

  DirOut[0] *= -1.;
  RotBackVector(mosaicMatrix, DirOut);

#if DEBUG
   double tempTh, tempPh;
   CartesianToSpherical(mosaicVector, &tempTh, &tempPh);
//  DEBUG_OUT("Direction of the mosaic vector: tempTh = %f, tempPh = %f, neutronVec:    %f %f %f %f   %f \n", 
 //   	  tempTh*180./M_PI, tempPh*180./M_PI, angle11*180./M_PI, angle12*180./M_PI, angle21*180./M_PI, angle22*180./M_PI, nTries); 
#endif

  if (nTries < maxNTries) 
    return (norm/nTries);
  else 
    return 0.0;
}

// Propagate neutron to the monochromator surface
void Monochromator::propNeutron2CE(const Neutron* pNeutIn, Neutron* pNeutOut)
{
  double DelX, V0, Vx, ToF, Path;
  
  DelX = (PosCE[0] - 0.5*DimCE[0]) - pNeutIn->Position[0];
  V0   = V_FROM_LAMBDA(pNeutIn->Wavelength);
  Vx   = V0 * pNeutIn->Vector[0];
  ToF  = DelX / Vx;
  Path = V0 * ToF;

  CopyNeutron(pNeutIn, pNeutOut);
  pNeutOut->Time += ToF;

  for (int i = 0; i < 3; i++)
    pNeutOut->Position[i] += pNeutOut->Vector[i] * Path;

  return;
}

// Transmits neutron through the monochromator crystal
void Monochromator::transmitNeutron(const Neutron* pNeutIn, Neutron* pNeutOut)
{
  double V0, ToF, mu_tot, weight;
  
  V0     = V_FROM_LAMBDA(pNeutIn->Wavelength);
  ToF    = PathLenTrans / V0;
  mu_tot = mu_scat + mu_abs * pNeutIn->Wavelength/1.798;
  weight = exp (-1.0 * PathLenTrans * mu_tot);
  
  CopyNeutron(pNeutIn, pNeutOut);

  pNeutOut->Time += ToF;
  pNeutOut->Probability *= weight;

  for (int i = 0; i < 3; i++)
    pNeutOut->Position[i] += pNeutOut->Vector[i] * PathLenTrans;

  return;
}

/*********************************************************************************************************************/

/*******************************************************/
/* 'lambda-focusing' option                            */
/*******************************************************/
void Monochromator::crys_geomLambda()
{

  int		m, j, k ;
  double	b, c, R1, R11, R2, R20, RotView[3][3] ;
  double	Psi, PsiC, Theta, DeltaTheta;

  VectorType	r, Step_H, Step_V ;

  fprintf(LogFilePtr,"geometry: vertical lambda focussing\n\n") ;

  FillRotMatrixZY(RotMatrixCE, - SrfcVert, - SrfcHor) ;
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if (RadV == 0.)
  {
    fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;
    exit(0) ;
  }


  Psi0 *= M_PI / 180. ;
  R20 = 0. ;

  /* computes CE parameters */
  b   = -DimCE0[2] * cos(M_PI_2 + SrfcVert);
  c   = - ( sq(RadV) + RadV * DimCE0[2] * cos(M_PI_2 + SrfcVert) );
  R2  = ( - b + sqrt(sq(b) - 4. * c) ) / 2.;
  R11 = sqrt( sq(RadV) + sq(DimCE0[2] / 2.) + RadV * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;

  PsiC =  acos( (sq(RadV) + sq(R11) - sq(DimCE0[2] / 2.)) / 2. / RadV / R11 )
        + acos( (sq(R11) + sq(R2) - sq(DimCE0[2] / 2.)) / 2. / R11 / R2 );

  for (m=0; m < NumberCE[0]; m++)		/* step horizontal */
  {
    R2 = RadV /* initial R1 */ ;
    Psi = M_PI_2 - Psi0 + PsiC ;
    DeltaTheta = 2. * atan(DimCE0[1] / 2. / R2) ;
    Theta = ((NumberCE[0] - 1) / 2. - m) * DeltaTheta ;

    declareVectors();

    for (j=0; j < NumberCE[1]; j++)	/* step vertical */
    {
	    R1  = R2 ;
	    b   = - DimCE0[2] * cos(M_PI_2 + SrfcVert) ;
	    c   = - ( sq(R1) + R1 * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;
	    R2  = ( - b + sqrt(sq(b) - 4. * c) ) / 2.  ;
	    R11 = sqrt( sq(R1) + sq(DimCE0[2] / 2.) + R1 * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;

	    Psi = Psi	- acos( (sq(R1) + sq(R11) - sq(DimCE0[2] / 2.)) / 2. / R1 / R11 )
	                  - acos( (sq(R11) + sq(R2) - sq(DimCE0[2] / 2.)) / 2. / R11 / R2 ) ;

	    /* computes x translation at the end */
	    if (fabs(Psi - M_PI_2) <= atan(DimCE0[2] / 2. / R2))
        R20 = R2 ;

	    /* output focussing geometry parameters */
	    PosCE_F[0][m].push_back( R2 * sin(Psi) * cos(Theta) );
	    PosCE_F[1][m].push_back( R2 * sin(Psi) * sin(Theta) );
	    PosCE_F[2][m].push_back( R2 * cos(Psi) );


	    if (m!=0 && j!=0)
	    {
        for(k=0;k<3;k++)
	      {
		      Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
		      Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
	      }

	      DimCE_F[0][m].push_back(0.0) ;
	      DimCE_F[1][m].push_back(0.9 * (LengthVector(Step_H) - DimCE0[1]));
	      DimCE_F[2][m].push_back(0.9 * (LengthVector(Step_V) - DimCE0[2]));

	    } 
	    else 
      {
	      for(k=0;k<3;k++) DimCE_F[k][m].push_back(0.) ;
	    }

	    RotCEhor_F[m].push_back( (Theta) * 180. / M_PI           + MonteCarlo(-0.5*DevH, 0.5*DevH));
	    RotCEvert_F [m].push_back((M_PI_2 - Psi) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));

	  }
  }

  for(m=0; m < NumberCE[0]; m++)		/* step horizontal */
  {
    for(j=0; j < NumberCE[1]; j++)	/* step vertical */
    {
	    PosCE_F[0][m][j] -= R20 ;/**/

	    /* rotate  */
	    CopyVectorsToVector(m, j, PosCE_F, r) ;
	    RotVector(RotMatrixCE, r) ;
	    CopyVectorToVectors(m, j, r, PosCE_F) ;
    }
  }

  /* print to file */
  writeFocData();

  return ;
}


/*******************************************************/
/* 'sphere-focusing' option                            */
/*******************************************************/
void	Monochromator::crys_geomSphere()
{
  int	    m, j, k, q ;
  double  R,                          // radius of the sphere
          Psi, Theta,                 // vertical and horizontal orientation of the crystal element
          DelPsi, DeltaTheta,         // Angular dfferences between neighboring rows/columns 
          RotView[3][3], 
          DistRows, DistCols;         // distance between 2 rows/columns (= element height/width  + gap)  ;
  VectorType	r, Step_H, Step_V ;

  fprintf(LogFilePtr,"geometry: focussing sphere\n\n") ;

  DistRows = DimCE0[2] + GapV;   // vertical
  DistCols = DimCE0[1] + GapH;   // horizontal

  FillRotMatrixZY(RotMatrixCE, - SrfcVert, - SrfcHor) ; 
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if (RadV == 0.)
  {
    fprintf(LogFilePtr,"\nERROR: radius must be > 0 !\n") ;
    exit(0) ;
  }

  Psi0		= Psi0 * M_PI / 180. ;
  R = RadV;                        // = sqrt(sq(RadV) - sq(DistRows / 2.)) ; 
  DelPsi = 2.0 * asin(0.5 * DistRows / R);    // vertical
  DeltaTheta = 2.0 * asin(0.5 * DistCols / R);    // horizontal

  /*computes CE parameters */
  for(m = 0;m<NumberCE[0];m++)		/* step horizontal */
  {
    Psi = M_PI_2 - Psi0;
    Theta = ((NumberCE[0]-1) / 2. - m) * DeltaTheta ;

    declareVectors();  

    for(j = 0;j<NumberCE[1];j++)	/* step vertical */
	  {
	    /* output focussing geometry parameters */
	    r[0] = R * sin(Psi) * cos(Theta) - R ;
	    r[1] = R * sin(Psi) * sin(Theta) ;
	    r[2] = R * cos(Psi) ;

	    for(q=0;q<3;q++) PosCE_F[q][m].push_back(r[q]) ;

	    if( (m != 0) && (j != 0) )
	    {
	      for(k=0;k<3;k++)
		    {
		      Step_H[k] = PosCE_F[k][m][j] - PosCE_F[k][m-1][j] ;
		      Step_V[k] = PosCE_F[k][m][j] - PosCE_F[k][m][j-1] ;
		    }

	      DimCE_F[0][m].push_back(0.) ;	
	      DimCE_F[1][m].push_back(0.9 * (LengthVector(Step_H) - DimCE0[1]));	
	      DimCE_F[2][m].push_back(0.9 * (LengthVector(Step_V) - DimCE0[2]));
	    } 
	    else 
      {
	      for(k=0;k<3;k++) DimCE_F[k][m].push_back( 0.) ;	
	    }

	    RotCEhor_F[m].push_back((Theta) * 180. / M_PI            + MonteCarlo(-0.5*DevH, 0.5*DevH));
	    RotCEvert_F[m].push_back( (M_PI_2 - Psi) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));

 	    Psi = Psi - DelPsi;
    }
  }

  rotateCEs();

  /* print to file */
  writeFocData();

  return ;
}/* End Crys_GeomSphere */


/*******************************************************/
/* 'vertical cylinder-focusing' option                 */
/*******************************************************/
void   Monochromator::crys_geomVertCyl()
{
  int		m, j, k, q ;
  double	R,                // Radius
          Psi, DelPsi,      // vertical angles
          RotView[3][3], 
          DistRows;         // distance between 2 rows (= slab height + gap)  ;
  VectorType	r ;

  fprintf(LogFilePtr,"geometry: vertically focussing cylinder\n\n") ;

  DistRows = DimCE0[2] + GapV;

  FillRotMatrixZY(RotMatrixCE, - SrfcVert, - SrfcHor) ; 
  FillRotMatrixZY(RotView, 10 * M_PI / 180., -95 * M_PI / 180.) ;

  if(RadV == 0.)
    Error("ERROR: radius must be > 0");

  Psi0 = Psi0 * M_PI / 180. ;
  R =  RadV;

  /*computes CE parameters */
  m = 0;		// only 1 column
  DelPsi = 2.0 * asin(0.5 * DistRows / R);    // vertical
  Psi    = M_PI_2 - Psi0;

  declareVectors();

  for (j=0; j<NumberCE[1]; j++)	/* step vertical */
  {
    /* output focussing geometry parameters */
    r[0] = R * sin(Psi) - R ;
    r[1] = 0. ;
    r[2] = R * cos(Psi) ;

    for(q=0;q<3;q++) PosCE_F[q][m].push_back (r[q]) ;

    for(k=0;k<3;k++) DimCE_F[k][m].push_back ( 0.) ;	

    RotCEhor_F[m].push_back(MonteCarlo(-0.5*DevH, 0.5*DevH));
    RotCEvert_F[m].push_back (MonteCarlo(-0.5*DevV, 0.5*DevV) + (M_PI_2 - Psi) * 180. / M_PI );

    Psi -= DelPsi ;
  }

  /*rotate CEs and print to file */
  NumberCE[0]=1;
  rotateCEs();
  writeFocData();

  return ;

}/* End Crys_GeomVertCyl */


/*******************************************************/
/* 'double focussing cylinder' option                  */
/*******************************************************/
void   Monochromator::crys_geomDoubleCyl()
{

	int	    m, j, k, q ; 
	double	    CE_Width,    /* width and height of one monochromator element */
	            CE_Height,
	            Psi,         /* vertical angle to monochromator slab under consideration */
	            Theta;       /* horizontal angle to monochromator slab under consideration */

	VectorType  r;

	fprintf(LogFilePtr,"geometry: cylinder focussing vertically and horizontally\n\n") ;

	FillRotMatrixZY(RotMatrixCE, - SrfcVert, - SrfcHor) ; 

	CE_Width  = DimCE0[1];
	CE_Height = DimCE0[2];

	if(RadV == 0.0 && RadH==0.0)
		Warning("both radii are zero");

	/*computes CE parameters */
	for(m = 0;m<NumberCE[0];m++)	   /* step horizontal, loop over columns */
	{
		r[0] =  0.0 ;
		r[1] = (NumberCE[0] - 1 - 2*m)/2.0 * (CE_Width + GapH);
		if (RadH > 0.0)	Theta = atan(r[1] / RadH);
		else Theta = 0.0;

		declareVectors();
		
		for(j = 0;j<NumberCE[1];j++)	/* step vertical,   loop over rows */
		{
			/* output focusing geometry parameters */
			r[2] = (NumberCE[1] - 1 - 2*j)/2.0 * (CE_Height + GapV);
		
			if (RadV > 0.0)
				Psi = atan(r[2] / RadV);
			else
				Psi = 0.0;

			for(q=0;q<3;q++) PosCE_F[q][m].push_back( r[q]) ;
			for(k=0;k<3;k++) DimCE_F[k][m].push_back(0.0 );	

			RotCEhor_F [m].push_back(Theta * 180./M_PI + MonteCarlo(-0.5*DevH, 0.5*DevH));
			RotCEvert_F[m].push_back(Psi   * 180./M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));		}
	}

  rotateCEs();

	/* print to file */
  writeFocData();

	return ;

}/* End crys_geomDoubleCyl */


/******************************************************************/
/* rotateCEs() rotates CE positions to module frame               */
/* addDev2Std() adds the deviations of each CE to standard values */
/* writeFocData() writes calculated crystal data to file          */
/******************************************************************/
//
void Monochromator::rotateCEs()
{
  int m, j;
  VectorType r;

  for(m=0; m < NumberCE[0]; m++)		/* step horizontal */
  { for(j=0; j < NumberCE[1]; j++)	/* step vertical */
	  {
	    /*PosCE_F[0][m][j] += R ;*/
	    /* rotate  */
	    CopyVectorsToVector(m, j, PosCE_F, r) ;
	    RotVector(RotMatrixCE, r) ;
	    CopyVectorToVectors(m, j, r, PosCE_F) ;
	  }
  }

  return;
}

void Monochromator::addDev2Std()
{
  int i,j,k;  // indices

  for(i=0; i<NumberCE[0]; i++)
  {
    for(j=0; j<NumberCE[1]; j++)
	  {
	    /* converts degs in radian etc. */
	    RotCEhor_F[i][j] *= M_PI/180. ;
	    RotCEvert_F [i][j] *= M_PI/180. ;

	    /* adds main parameters */
	    for(k=0;k<3;k++)
	    {
	      PosCE_F[k][i][j] += PosCE0[k] ;
	      DimCE_F[k][i][j] += DimCE0[k] ;
	    }

	    RotCEhor_F[i][j] += SrfcHor ;
	    RotCEvert_F [i][j] += SrfcVert ;
	  }
  }
  return;
}

void Monochromator::writeFocData()
{
  int m, j;

  // open geometry file to save calculated data
  pGeomFile = OpenOutputFile(GeomFileName, FALSE, "w") ;

  if (pGeomFile != NULL)
  { 
    fprintf(pGeomFile,"%d %d\n", NumberCE[0], NumberCE[1]) ;/**/

    for (m=0; m < NumberCE[0]; m++)		/* step horizontal */
    { for (j=0; j < NumberCE[1]; j++)	/* step vertical */
	    {
	      fprintf(pGeomFile,"%8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f %8.6f\n",
                          PosCE_F[0][m][j], PosCE_F[1][m][j], PosCE_F[2][m][j], DimCE_F[0][m][j], DimCE_F[1][m][j], DimCE_F[2][m][j], RotCEhor_F[m][j], RotCEvert_F[m][j] ) ;
	    }
    }

    fclose(pGeomFile) ;
  }
  else
  {
    Warning("Geometry file could not be opened. Calculated data are not written to a file.");
  }

  return;
}


/*********************************************************************************************************************/
// 4 functions to copy rotation matrices or vectors to arrays and get them out again 
//
void Monochromator::CopyMatricesToMatrix(int i, int j, std::vector < std::vector<double> >  Matrix[3][3], double Result[3][3])
{

  for(int k = 0; k < 3; k++) {
    for(int l = 0; l < 3; l++) {
      if (Matrix[k][l].size() >= (i+1)) {
	if (Matrix[k][l][i].size() >= (j+1))
	  Result[k][l] = Matrix[k][l][i][j] ;
	else {
	  fprintf(LogFilePtr,"1 Trying to access non-existing matrix elements in CopyMatricesToMatrix! Abort! \n") ;
	  exit(-1);
	}
      }
      else {
	fprintf(LogFilePtr,"2 Trying to access non-existing matrix elements in CopyMatricesToMatrix! Abort! \n") ;
	exit(-1);
      }
    }
  }
  return;
}

void Monochromator::CopyMatrixToMatrices(int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3])
{

  for(int k = 0;k < 3; k++) {
    for(int l = 0;l < 3; l++) {
      while (Matrix[k][l].size() < (i+1)) {
	std::vector<double> tempVector;
	Matrix[k][l].push_back(tempVector);
      }
      while (Matrix[k][l][i].size() < (j+1)) Matrix[k][l][i].push_back(0.);
      Matrix[k][l][i][j] = Result[k][l] ;
    }
  }

  return;

}

void Monochromator::CopyVectorsToVector(int i, int j, std::vector < std::vector<double> >  Vector[3], double Result[3])
{
  
  for(int k = 0; k < 3; k++) {
    if (Vector[k].size() >= (i+1)) {
      if (Vector[k][i].size() >= (j+1))
	Result[k] = Vector[k][i][j] ;
      else {
	  fprintf(LogFilePtr,"3 Trying to access non-existing matrix elements in CopyVectorsToVector! Abort! \n") ;
	  exit(-1);
      }
    }
    else {
      fprintf(LogFilePtr,"4 Trying to access non-existing matrix elements in CopyVectorsToVector! Abort! \n") ;
      exit(-1);
    }
  }
  
  return;

}

void Monochromator::CopyVectorToVectors(int i, int j, double Vector[3], std::vector < std::vector<double> >  Result[3])
{

  for(int k = 0; k < 3; k++) {
    while (Result[k].size() < (i+1)) {
      std::vector<double> tempVector;
      Result[k].push_back(tempVector);
    }
    while (Result[k][i].size() < (j+1)) Result[k][i].push_back(0.);
    Result[k][i][j] = Vector[k] ;
  }

  return;

}

// initializes arrays
void Monochromator::declareVectors()
{

  std::vector <double> tempVector;
  PosCE_F[0].push_back(tempVector);
  PosCE_F[1].push_back(tempVector);
  PosCE_F[2].push_back(tempVector);
  DimCE_F[0].push_back(tempVector);
  DimCE_F[1].push_back(tempVector);
  DimCE_F[2].push_back(tempVector);
  RotCEhor_F.push_back(tempVector);
  RotCEvert_F.push_back(tempVector);
  
  return;

}

// calculates intersection point of straight line through point with plane
bool Monochromator::checkPlaneIntersect(const MathVector vLineOffset,  const MathVector vLineDir,
                                        const MathVector vPlaneNormal, const double PlaneDistance, double& t)
{
  bool   rc;
  double sp = vLineDir * vPlaneNormal;

  if (sp==0.0)
  { rc=false;
    t = 0.0;
  }
  else
  { rc= true;
    t = (PlaneDistance - (vLineOffset * vPlaneNormal))/sp;
  }

  return rc;
}


#endif


