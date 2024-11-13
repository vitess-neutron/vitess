#ifndef MONOCHRCLASS_CPP
#define MONOCHRCLASS_CPP

#include "mathfunctions.h"
#include "monochrclass.h"
#include "convert.h"

#define DEL_ZETA_MAX 0.01   // iteration stops if step size is less than 0.01 deg rotation  
#define DEL_POS_MAX  0.01   //                                        or 0.02 cm  oscillation 

// constructor, sets all variables to zero
Monochromator::Monochromator()
{
  eModule=MCN_MONOCHROM;

  // Initialise member variables
  ParFileName  = NULL; 
  GeomFileName = NULL;
  
  pGeomFile = NULL;

  eGeomOption  = SINGLE_CE;
  eFocGeom     = NO_FOCUSING; 
  bTransm      = FALSE;
  bRndTof      = FALSE;
  eMonoMode    = REFL_MONO;
  eMonoMove    = VT_MONO_FIX;
  d_spr_option = LORENTZIAN; 
  nAreasPST    = 0;
  nRepete      = 1; 

  d_fwhm       = 0.0; 
  d_sigma      = 0.0;
  sigma1       = 0.0;
  sigma2       = 0.0;
  mu_abs       = 0.0;
  mu_scat      = 0.0;
  Reflectivity = 1.0; 
  SpcLayer     = 0.0;
  DeclLayer    = 0.0;
  Freq         = 0.0;
  Zeta0        = 0.0;
  AmplDop      = 0.0;
  AreaWidthPST = 0.0;
  RadiusPST    = 0.1;
  Period       = 0.0;
  omega        = 0.0;
  TrndMin      = 0.0;
  TrndMax      = 0.0;

  for (int i = 0; i < 2; i++) 
    mosaic_fwhm[i] = 0.0;
  
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
  OutHorU   = UNDEFINED;
  OutVertU  = UNDEFINED;
  OutHorR   = 0.0; 
  OutVertR  = 0.0; 
  OutHorT   = 0.0; 
  OutVertT  = 0.0; 

  eFrame    = VT_NO_FRAME;   // no decision about user defined frame  
  d_spacing = 0.0; 
  nOrderRefl= 0;

  for (int i = 0; i < 3; i++)
  {
    NumberCE[i] = 1;

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

    rDop[i]  = 0.0;
    vDop[i]  = 0.0;
  }

  PathLenTransSum=0.0;
  PathLenTrans = 0.0;
  PathLenRefl  = 0.0;
  maxDeviation = 1.0;
  peakWL       = 0.0;
  braggAngleTot= 0.0;
  axisPhi      = 0.0;
  mosRndmDir   = 0;
  
  currentNeutron = NULL;
  TOF    = -1.0;
  Prob   =  0.0;
  NumOut =  0; 

  DelZetaMax = DEL_ZETA_MAX;
  DelXmax    = DEL_POS_MAX;
}


// reads and checks input parameters and calculates depending values
void Monochromator::OwnInit(int argc, char* argv[])
{
  int    k=0;
  double m_cut=M_CUT;
  char   sHV[2][11]={"horizontal", "vertical"};  // mosaic_fwhm

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
	        sscanf(&argv[1][2], "%hd", &bTransm) ;
	        break;
        case 'K':
	        sscanf(&argv[1][2], "%hd", &bRndTof) ;
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
        case 'n':
	        sscanf(&argv[1][2], "%d", &nAreasPST) ;
	        break;

        case 'o':
          sscanf(&argv[1][2], "%lf", &SpcLayer);
          break;
        case 'J':
          sscanf(&argv[1][2], "%lf", &DeclLayer);
          break;
        case 'f':
	        sscanf(&argv[1][2], "%lf", &Freq) ;
          omega = 2.0 * M_PI * Freq / 1000.0;   // unit: [rad/ms]
          if (Freq > 0.0)
	          Period = 1000.0 / Freq;
          break;
        case 'p':
	        sscanf(&argv[1][2], "%lf", &Zeta0) ;
	        break;
        case 'Q':
	        sscanf(&argv[1][2], "%lf", &AmplDop) ;
	        break;
        case 'w':
	        sscanf(&argv[1][2], "%lf", &RadiusPST) ;
	        break;
        case 'q':
	        sscanf(&argv[1][2], "%lf", &AreaWidthPST) ;
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

        case 'I':
          sscanf(&argv[1][2], "%d", &NumberCE[0]);
          break;
        case 'H':
	        sscanf(&argv[1][2], "%d", &NumberCE[1]) ;
	        break;
        case 'V':
	        sscanf(&argv[1][2], "%d", &NumberCE[2]) ;
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
          OutHorU  = atof(&argv[1][2]);
          break;
        case 'U':
          OutVertU = atof(&argv[1][2]);
          break;

        default:
          Error2("unkown command option", argv[1]);
      }
    }
    argc--;
    argv++;
  }

  // Check max. number of layers
  if (NumberCE[0] > NUM_LAYERS)
    Error("Number of layers along the stacking direction x exceeds max. number, please increase NUM_LAYERS");

  // Checks for possible  options   (1 crystal element, geometry calculated or from file
  if((eGeomOption != SINGLE_CE) && (eGeomOption != CE_ARRAY_CALC) && (eGeomOption != CE_ARRAY_FILE))
    Error("No valid option for the gemetry of the monochromator system found!!");

  // Option 'vertical cylinder' assumes only 1 column
  if (eFocGeom==VERT_CYL && NumberCE[1] > 1)
    Error("Only 1 column allowed for vertical cylinder geometry");

  if (bRndTof==TRUE && (Freq==0.0 || eMonoMove==VT_MONO_FIX))
    Error("random TOF only makes sense for movement with a certain frequency");
  
  // checks mosaicity   
  for (k=0; k<2; k++) 
  {
    if (mosaic_fwhm[k] < m_cut) 
    {
      mosaic_fwhm[k]	= m_cut;
      fprintf(LogFilePtr, "WARNING: minimum mosaicity %1.1e was set to %s mosaic spread!\n", m_cut, sHV[k]);
    }
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

      // combines information from input and file, input parameters have priority
      if (eFrame==VT_NO_FRAME  && eFrm!=VT_NO_FRAME) eFrame = eFrm; 
      if (PosCE0[0] ==0.0 && x     !=0.0) PosCE0[0]= x     ;
      if (PosCE0[1] ==0.0 && y     !=0.0) PosCE0[1]= y     ;
      if (PosCE0[2] ==0.0 && z     !=0.0) PosCE0[2]= z     ;
      if (SrfcHor   ==0.0 && sfc_h !=0.0) SrfcHor  = sfc_h ;
      if (SrfcVert  ==0.0 && sfc_v !=0.0) SrfcVert = sfc_v ;
      if (BraggHor  ==0.0 && brg_h !=0.0) BraggHor = brg_h ;
      if (BraggVert ==0.0 && brg_v !=0.0) BraggVert= brg_v ;
      if (DimCE0[0] ==0.0 && thickn!=0.0) DimCE0[0]= thickn;
      if (DimCE0[1] ==0.0 && width !=0.0) DimCE0[1]= width ;
      if (DimCE0[2] ==0.0 && height!=0.0) DimCE0[2]= height;
      if (d_spacing ==0.0 && dsp   !=0.0) d_spacing = dsp  ;
      if (nOrderRefl==0   && n_ord !=0  ) nOrderRefl= n_ord;
      if (Transl[0] ==0.0 && out_x !=0.0) Transl[0]= out_x ;
      if (Transl[1] ==0.0 && out_y !=0.0) Transl[1]= out_y ;
      if (Transl[2] ==0.0 && out_z !=0.0) Transl[2]= out_z ;
      if (OutHorU ==UNDEFINED && out_h!=UNDEFINED) OutHorU =out_h;
      if (OutVertU==UNDEFINED && out_v!=UNDEFINED) OutVertU=out_v;

      // calculates values for output frame
      if (eFrame == VT_FRAME_STD)
      {
        AnglesOutputFrame(BraggHor, BraggVert, &OutHorR, &OutVertR);
        OutHorT = 0.0;     OutVertT = 0.0;
      }
      else if (eFrame == VT_FRAME_USER)
      {
        OutHorR = OutHorU; OutVertR = OutVertU;
        OutHorT = 0.0;     OutVertT = 0.0;
      }

      /* shifts output frame origin to center of focussing geometry */
      CopyVector(PosCE0, Transl);

      // initializes vectors to chosen CE element
      CopyVector(PosCE0, PosCE);
      CopyVector(DimCE0, DimCE);

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
  OutHorR   *= M_PI/180.0;
  OutVertR  *= M_PI/180.0;
  OutHorT   *= M_PI/180.0;
  OutVertT  *= M_PI/180.0;

  return;
}/* End ReadParameterFile */


// Determines the dependent parameters  
void Monochromator::calcPar()
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

  // here the different geometry options are treated
  if (eGeomOption == SINGLE_CE)     // single CE
  {
    NumberCE[0] = NumberCE[1] = NumberCE[2] = 1;
      
    CopyPosToVectors(0, 0, 0, PosCE0);
    CopyDimToVectors(0, 0, 0, DimCE0);

    std::vector <double> RotHorizVector;
    RotCEh_F[0].push_back(RotHorizVector);
    RotCEh_F[0][0].push_back(SrfcHor);
      
    std::vector <double> RotVertVector;
    RotCEv_F[0].push_back(RotVertVector);
    RotCEv_F[0][0].push_back(SrfcVert); 
  }
  else if (eGeomOption == CE_ARRAY_CALC)   // CE positions, dimensions and orientations calculated
  {
    char sText[21] = "";
    MonoFocus_ID2Txt(sText, eFocGeom);
    fprintf(LogFilePtr, "geometry: %s \n\n", sText);

    // calculate deviations from central CE
    if (eFocGeom==NO_FOCUSING) crys_geomNoFocus();
    if (eFocGeom==CONST_LMBD)  crys_geomLambda();
    if (eFocGeom==SPHERICAL )  crys_geomSphere();
    if (eFocGeom==VERT_CYL  )  crys_geomVertCyl();
    if (eFocGeom==DBL_FOC   )  crys_geomDoubleCyl();

    // add deviations of the CEs to position, size and orientation of the central CE
    writeFocData();
    addDev2Std();                         
  }
  else if (eGeomOption == CE_ARRAY_FILE)   // deviations of the CE positions, dimensions and orientations from file
  {
    readFocFile() ;
    addDev2Std();
  }

  /* computes rotation matrixes corresponding to CE(i,j) offset angles,
     a rotation about the vertical axix changes the rotation matrices, the other movement don't */
  if (eMonoMove==VT_MONO_ROT)
    fillRotMatrices(Zeta0*M_PI/180.0);
  else
    fillRotMatrices(0.0);


  /* computes rotation matrix corresponding to the output frame (focus direction) */
  FillRotMatrixZY(RotMatrixOut, OutVertR, OutHorR);

  /* computes parameters for rotation and oscillation */
  if (bRndTof)
  { TrndMin =-0.5*Period;
    TrndMax = 0.5*Period;
  }

  return; 
}


// Writes out important parameters 
void Monochromator::writePar()
{
  char   sTr[2][ 8]={"blocked",    "treated"},   // bTransm
         sMT[13]="",                             // eMonoType
         sMM[26]="",                             // eMonoMove
         sSO[11]="",                             // d_spr_option
         sText[21] = "";                         // geometry option

  MonoType_ID2Txt(sMT, eMonoMode);
  MonoMove_ID2Txt(sMM, eMonoMove);
  Distr_ID2Txt   (sSO, d_spr_option);
  MonoFocus_ID2Txt(sText, eFocGeom);

  /* monochromator parameters */
  fprintf(LogFilePtr, "  Mode: %s,  transmitted beam: %s\n",               sMT, sTr[bTransm]);
  if (eMonoMove != VT_MONO_FIX)
    fprintf(LogFilePtr, "  %-24.24s : %8.3f Hz %8.4f deg initial phase\n", sMM, Freq, Zeta0);
  if (eMonoMove == VT_MONO_PST)
    fprintf(LogFilePtr, " %2d windows of %5.2f deg width at %5.2f cm radius\n", nAreasPST, AreaWidthPST, RadiusPST);

  fprintf(LogFilePtr, "  attenuation   (scat, abs): %9.4f, %9.4f 1/cm\n",  mu_scat, mu_abs);
  fprintf(LogFilePtr, "  mosaic spread (hor, vert): %9.4f, %9.4f deg\n",   Degrees(mosaic_fwhm[0]), Degrees(mosaic_fwhm[1]));
  fprintf(LogFilePtr, "  d-spread %10s      : %13.4e Ang  \n",             sSO, d_fwhm);
  fprintf(LogFilePtr, "  reflectivity             : %9.4f \n",             Reflectivity);
  fprintf(LogFilePtr, "  repetition               : %4ld  \n",             nRepete);

  /* crystal parameters */
  fprintf(LogFilePtr, "data from parameter file   : '%s'\n", ParFileName);
  fprintf(LogFilePtr, "  order of reflection = %d,   d-spacing =%9.4f Ang\n",  nOrderRefl, d_spacing);
  fprintf(LogFilePtr, "  main position X, Y, Z    : %9.4f, %9.4f, %9.4f cm\n", PosCE0[0], PosCE0[1], PosCE0[2]);
  fprintf(LogFilePtr, "  thickness, width, height : %9.4f, %9.4f, %9.4f cm\n", DimCE0[0], DimCE0[1], DimCE0[2]);
  fprintf(LogFilePtr, "  Bragg angles   (hor,vert): %9.4f, %9.4f deg\n",       Degrees(BraggHor), Degrees(BraggVert));
  fprintf(LogFilePtr, "  Surface angles (hor,vert): %9.4f, %9.4f deg\n",       Degrees(SrfcHor),  Degrees(SrfcVert));
  fprintf(LogFilePtr, "braggTot =%9.4f, sigma1 = %f, sigma2 = %f, maxDev = %f\n", Degrees(M_PI_2 - braggAngleTot), sigma1, sigma2, Degrees(maxDeviation));
  fprintf(LogFilePtr, "Norm of the mosaic function: %1.4lf, peak wavelength %1.4lf \n", fNorm[0], peakWL) ;
  if (eFrame==VT_FRAME_USER) 
    fprintf(LogFilePtr,"user defined frame:\n") ;
  else  
    fprintf(LogFilePtr,"standard frame generation:\n") ;
  fprintf(LogFilePtr, "  output angles (hor, vert): %9.4f  %9.4f deg\n",        Degrees(OutHorR),  Degrees(OutVertR)); 
	fprintf(LogFilePtr, "  position X',Y',Z'        : %9.4f, %9.4f, %9.4f cm\n", Transl[0], Transl[1], Transl[2]);

  /* geometry parameters */
  fprintf(LogFilePtr, "initialised option: ") ;
  if (eGeomOption == SINGLE_CE)
  { fprintf(LogFilePtr,"'crystal_flat'\n");
  }
  else if (eGeomOption == CE_ARRAY_CALC)
  {
    fprintf(LogFilePtr, "geometry: %s \n\n", sText);    fprintf(LogFilePtr, "'crystal_focus'\n");
    fprintf(LogFilePtr, "  horizontal: number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg\n",                          NumberCE[1], RadH, GapH, DevH) ;
    fprintf(LogFilePtr, "  vertical  : number of CE = %2d,  radius = %6.1lf cm,  gap = %4.2lf cm,  var. orient. = %4.2lf deg,  min. angle = %.3lf deg\n", NumberCE[2], RadV, GapV, DevV, Psi0);
    fprintf(LogFilePtr, "  sequential: number of CE = %2d,                   spacing = %4.2lf cm \n",                                                     NumberCE[0], SpcLayer);
    fprintf(LogFilePtr, "  focus file:                '%s'\n", GeomFileName);
  }
  else if (eGeomOption == CE_ARRAY_FILE)
  {
    fprintf(LogFilePtr,"'crystal_focus_dat'\n");
    fprintf(LogFilePtr,"  number of CEs (stack, hor, vert): %2d %2d %2d \n", NumberCE[0], NumberCE[1], NumberCE[2]);
    fprintf(LogFilePtr,"  focus file:                '%s'\n", GeomFileName);
  }
}


// Reads the parameters from the geometry file */
void Monochromator::readFocFile()
{
  int	h=0, i=0, j=0;   
  char  sLine[CHAR_BUF_SMALL]="";
  double FocPar[8];
   
  pGeomFile = OpenInputFile(GeomFileName, FALSE, "r");

  if (pGeomFile==NULL)
  {
    Error2("focus file not found", GeomFileName);
  }     
  else
  { 
    // number of (layers,) columns and rows
    if (ColumnsInFile(pGeomFile) == 3)                 // ColumnsInFile must NOT be called after ReadLine() !!!
    { 
      ReadLine(pGeomFile, sLine, sizeof(sLine)-1);
      sscanf(sLine, "%d %d %d", &NumberCE[0], &NumberCE[1], &NumberCE[2]);
    }
    else
    { NumberCE[0]=1;
      ReadLine(pGeomFile, sLine, sizeof(sLine)-1);
      sscanf(sLine, "%d %d", &NumberCE[1], &NumberCE[2]);
    }

    // loop over all crystal elements (1 line for each element)
    // reads 3 position deviations, 3 size deviations and 2 orientation deviations in each line
    // and adds these values to those  given for the monochromator center or central element resp.
    for (h=0; h < NumberCE[0]; h++)      // loop over succeeding arrays
    { 
      for(i=0; i < NumberCE[1]; i++)     // loop over columns
      {
        std::vector <double> tempVector;
        PosCEx_F[h].push_back(tempVector);
        PosCEy_F[h].push_back(tempVector);
        PosCEz_F[h].push_back(tempVector);
        DimCEx_F[h].push_back(tempVector);
        DimCEy_F[h].push_back(tempVector);
        DimCEz_F[h].push_back(tempVector);
        RotCEh_F[h].push_back(tempVector);
        RotCEv_F[h].push_back(tempVector);
        
        for(j=0; j < NumberCE[2]; j++)   // loop over rows; each column is one vector 
        {
          ReadLine  (pGeomFile, sLine, sizeof(sLine)-1);
          StrgScanLF(sLine, FocPar, 8, 0);
          PosCEx_F[h][i].push_back(FocPar[0]); PosCEy_F[h][i].push_back(FocPar[1]); PosCEz_F[h][i].push_back(FocPar[2]);
          DimCEx_F[h][i].push_back(FocPar[3]); DimCEy_F[h][i].push_back(FocPar[4]); DimCEz_F[h][i].push_back(FocPar[5]);
          RotCEh_F[h][i].push_back(FocPar[6]); RotCEv_F[h][i].push_back(FocPar[7]);
        }
      }
    }
    fclose(pGeomFile);
  }  
  return;
}/* End ReadFocFile */


// fills the structure stGeometry for visualization
void Monochromator::setGeometry(const char* sColor)
{
  int    n=0,               // index for geometrical elements in figure
         h=0, i=0, j=0;     // columns (i, hor), rows (j, vert) and arrays (along beam)
  double RotMatrixCurrCE[3][3];
  VectorType DimCurrCE, PosCurrCE,
             normal = {1.0, 0.0, 0.0};

  InitRotMatrix(RotMatrixCurrCE);

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
	    stGeometry.pCuboid[0].vNormal[0]= 1.0;
	    stGeometry.pCuboid[0].vNormal[1]= tan(SrfcHor);
	    stGeometry.pCuboid[0].vNormal[2]= tan(SrfcVert);
    }
    else
    {	
      n=0;
	    stGeometry.nCuboids = NumberCE[0]*NumberCE[1]*NumberCE[2];
	    stGeometry.pCuboid  = (VtCuboid*) calloc(stGeometry.nCuboids, sizeof(VtCuboid));
      
        for (int h=0; h < NumberCE[0]; h++)
        {
          for (int i=0; i < NumberCE[1]; i++)
          {
            for (int j=0; j < NumberCE[2]; j++)
            {
              CopyVectorsToPos    (h, i, j, PosCurrCE);
              CopyVectorsToDim    (h, i, j, DimCurrCE);
              CopyMatricesToMatrix(h, i, j, RotMatrixCE_F, RotMatrixCurrCE);

              normal[0]=1.0; normal[1]=0.0; normal[2]=0.0; 
              RotBackVector(RotMatrixCurrCE, normal);

              stGeometry.pCuboid[n].Length = DimCurrCE[0];
              stGeometry.pCuboid[n].Width  = BlowUp * DimCurrCE[1];
              stGeometry.pCuboid[n].Height = BlowUp * DimCurrCE[2];
              stGeometry.pCuboid[n].vCntr[0] = PosCurrCE[0];
              stGeometry.pCuboid[n].vCntr[1] = PosCurrCE[1];
              stGeometry.pCuboid[n].vCntr[2] = PosCurrCE[2];
              stGeometry.pCuboid[n].vNormal[0] = normal[0];
              stGeometry.pCuboid[n].vNormal[1] = normal[1];
              stGeometry.pCuboid[n].vNormal[2] = normal[2];

              n++;
            }
          }
	    }
	  }
  }
  return;
}


// Calls Cleanup to write trajectory information and change to output co-ordinate system
void Monochromator::OwnCleanup()
{
  Cleanup(Transl[0], Transl[1], Transl[2], OutHorR, OutVertR);

  return;
} 


/*********************************************************************************************************************/

/* fills rotations matrices for the surface and the reflecting planes of the central CE (= monochromator) 
   and the surfaces of all CEs for a given Orientation of the rotating monochromator */
void Monochromator::fillRotMatrices(double MonoHor)
{
  int    h=0, i=0, j=0;     // columns (i, hor), rows (j, vert) and arrays (along beam)

  double rotH, rotV, RotMatrix[3][3] ;

  /* computes rotation matrixes corresponding to Bragg angles */
  FillRotMatrixZY(RotMatrixCE0, SrfcVert, SrfcHor+MonoHor) ;

  /* computes rotation matrixes corresponding to Bragg angles */
  FillRotMatrixZY(RotMatrixBragg, BraggVert, BraggHor+MonoHor) ;

  for (h=0; h < NumberCE[0]; h++)       /* step along beam */
  { for (i=0; i < NumberCE[1]; i++)
    { for (j=0; j < NumberCE[2]; j++)
      {
        rotH = RotCEh_F[h][i][j] + MonoHor;
        rotV = RotCEv_F[h][i][j];
        FillRotMatrixZY(RotMatrix, rotV, rotH);
        CopyMatrixToMatrices(h, i, j, RotMatrix, RotMatrixCE_F);
      }
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

  sigma1 = dSpacingSpreadParams[2];
  double x1_incr = sigma1/100.;
  if (x1_incr == 0) 
    x1_incr = 1.;

  sigma2 = fRndm[2];
  double x2_incr = sigma2/100.;
  if (x2_incr == 0) 
    x2_incr = 1.;

  double minSC = Min(sin(axisPhi), cos(axisPhi));
  maxDeviation = (1. + 2.*minSC - (minSC/sin(M_PI_2/2.)))*braggAngleTot;

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
  fNorm[0] = Min(Reflectivity/mean, 1.0);

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


/********************************************************************************************
* Monochromator::processNeutron
*
* In this function the neutron is propagated to the monochromator,
*                  reflected at the corresponding CE and weighted with the reflection probability
*
* in : pNeutIn : data of the incoming neutron in the frame of the module
* out: pNeutOut: data of the outgoing neutron in the frame of the module
*
* return: none
*********************************************************************************************/
void Monochromator::processNeutron(Neutron* pNeutIn)
{
  bool    bHit=false,     // flag: crystal element is hit by the neutron
          bPos=true;      // flag: the PST monochromator is in position at the time of arrival
  int     h=0,   hHit=0,  // layer (that is hit
          iHit=0,jHit=0;  // (expected) index of column and row of CE that is hit
  double  TofCE=UNDEFINED,// flight time from entry to CE surface      
          ReflProb=0.0;   // total weight of the reflected neutron trajectories  
  Neutron NeutCE,         // data of the neutron on the CE surface (in the frame of the module)
          NeutRefl,       // data of the reflected neutron         (in the frame of the module)
          NeutTrans,      // data of the transmitted neutron       (in the frame of the module)
          NeutTransOut,   // data of the transmitted neutron       (in the output frame)
          NeutReflOut;    // data of the reflected neutron         (in the output frame)
  
  // init
  InitNeutron(&NeutCE);
  InitNeutron(&NeutRefl);   InitNeutron(&NeutReflOut);
  InitNeutron(&NeutTrans);  InitNeutron(&NeutTransOut);

  // Initialize
  pNeutIn->Vector[0] = sqrt(1.0 - sq(pNeutIn->Vector[1]) - sq(pNeutIn->Vector[2]));
  if (bRndTof)
    pNeutIn->Time = MonteCarlo(TrndMin, TrndMax);
  PathLenTransSum = 0.0;

  // loop over all CE layers
  for (h=0; h < NumberCE[0]; h++)         // x-direction
  {
    // moves monochromator if applicable, selects CE on which the neutron is reflected and returns variables in the frame of CE
    // 'rotMonoAndSelectCE()' and 'oscMonoAndSelectCE' iterate already ation which CE is hit
    switch (eMonoMove)
    { 
      case VT_MONO_FIX: 
      case VT_MONO_PST:
        bHit = selectCE(iHit, jHit, pNeutIn, h);
        break;
      case VT_MONO_ROT:
        bHit = rotMonoAndSelectCE(iHit, jHit, TofCE, pNeutIn, h);
        break;
      case VT_MONO_OSC:
        bHit = oscMonoAndSelectCE(iHit, jHit, TofCE, pNeutIn, h);
        break;
      default:
        Error("Unkown type of monochromator movement");
    }

    // propagate neutron to the CE that is hit (or to the y-z-plane through the x position of the central element)
    propNeutron2CE(&NeutCE, pNeutIn, TofCE);

    // check if the monochromator is in position
    if (eMonoMove==VT_MONO_PST)
      bPos = checkPstPos(&NeutCE);

    if (bHit==true && bPos==true) 
    { 
      hHit = h;

      // reflects neutron and writes its data to output stream and trajectory file
      ReflProb = reflectNeutron(&NeutRefl, &NeutCE);

      pNeutIn->Probability -= ReflProb;
    }
    else  // no CE was found
    {
      WriteIAP(&NeutCE, VT_OUTSIDE);
    }
  }

  // for flat option (eGeomOption=SINGLE_CE) computes neutron direction in the "Bragg" frame keeping frame of CE for the position
  if (eGeomOption==SINGLE_CE)
    RotVector(RotMatrixBragg, NeutCE.Vector);   // Vector is now in the frame of the Bragg refl. plane

  // transmitted neutron
  if (bTransm==TRUE)
  {
    transmitNeutron(&NeutTrans, &NeutCE);
    WriteIAP(&NeutTrans, VT_PASSED);

    // computes neutron variables in the output frame
    transfIn2Out(&NeutTransOut,&NeutTrans);

    WriteNeutron(&NeutTransOut);
    NumOut++ ;
  }
  
  return;
}


// Propagate neutron to the monochromator surface
void Monochromator::propNeutron2CE(Neutron* pNeutOut, const Neutron* pNeutIn, const double TofCE)
{
    double DelX=0.0, V0=0.0, Vx=0.0, Tof=0.0, Path=0.0;

    // if TOF to CE is not given, calculate flight time to the current CE
    if (TofCE==UNDEFINED)
    {
      V0   = V_FROM_LAMBDA(pNeutIn->Wavelength);
      DelX = (PosCE[0] - 0.5 * DimCE[0]) - pNeutIn->Position[0];
      Vx   = V0 * pNeutIn->Vector[0];
      Tof  = DelX / Vx;
    }
    else
    { Tof = TofCE;
    }

    // Update total TOF and position 
    Path = V0 * Tof;

    CopyNeutron(pNeutIn, pNeutOut);
    pNeutOut->Time += Tof;

    for (int i = 0; i < 3; i++)
      pNeutOut->Position[i] += pNeutOut->Vector[i] * Path;

    return;
}

bool Monochromator::checkPstPos(const Neutron* pNeutIn)
{
  // for PST: check if one of the active areas is hit
  short m=0;             // index of the monochromator areas on the PST 
  bool  bArea = false;
  double Aj   = 0.0,      // Orientation of the center of the m-th area covered by the monochromtor [deg]
         Tj   = 0.0,      // Time at which this area is in the center of the beam     
         DelT = 0.5 * (AreaWidthPST / 360.0 * Period);  // half of the time that this area is in the beam

  for (m = 1; m <= nAreasPST; m++)
  {
    Aj = (double)(2 * m - 1 - nAreasPST) / 2.0 * 360.0 / nAreasPST + Zeta0;
    Tj = Aj / 360.0 * Period;
    if (fabs(pNeutIn->Time - Tj) < DelT)
      bArea = true;
  }

  return bArea;
}

double Monochromator::reflectNeutron(Neutron* pNeutRefl, const Neutron* pNeutIn)
{
  int     iRep=0,                // index of repetition
          iOrd=0,                // index of order 
          minOrd=0, maxOrd=0;    // min. and max. order treated
  double  DelX=0.0, V0=0.0, 
          Vx=0.0, Path=0.0,
          TotReflProb =0.0;
 /*      pNeutIn            // data of the neutron on the CE plane   (in the frame of the module)       */
  Neutron NeutPlCE,         // data of the neutron on the CE plane   (in the frame of the refl. plane)
          NeutPlCEM,        // data of the neutron on the CE plane   (in the frame of the moving monochromator)
          NeutReflCEM,      // data of the reflected neutron         (in the frame of the moving monochromator)
          NeutReflCE,       // data of the reflected neutron         (in the frame of the refl. plane)  
 /*      pNeutRefl,         // data of the reflected neutron         (in the frame of the module)       */
          NeutReflOut;      // data of the reflected neutron         (in the output frame)

  // init
  InitNeutron(&NeutPlCE);    InitNeutron(&NeutPlCEM);
  InitNeutron(&NeutReflCEM); InitNeutron(&NeutReflCE);
  InitNeutron(pNeutRefl);    InitNeutron(&NeutReflOut);

  // if order=-1 is given all orders are treated, loop is left if order gets too high 
  if (nOrderRefl == ANY_COLOR)
  { minOrd =   1;
    maxOrd = 100;
  }
  else
  { minOrd = nOrderRefl;
    maxOrd = nOrderRefl;
  }

  // transfers to the co-ordinate system of the (moving) monochromator
  switch (eMonoMove)
  {
    case VT_MONO_PST: 
      transfIn2CE(&NeutPlCE,  pNeutIn);
      transf2PST (&NeutPlCEM, &NeutPlCE, true);
      break;
    case VT_MONO_ROT: 
      transfIn2CE(&NeutPlCE,  pNeutIn);
      transf2RotZ(&NeutPlCEM, &NeutPlCE, true);
      break;
    case VT_MONO_OSC: 
      transf2DopplX(&NeutPlCEM, pNeutIn, true);
      break;
    default:
      transfIn2CE (&NeutPlCEM, pNeutIn);
  }

  // reflect 'nRepete' times for the wanted or all orders of reflection 
  for (iOrd = minOrd; iOrd <= maxOrd; iOrd++)
  {
    for (iRep = 0; iRep < nRepete; iRep++)
    { double arg,                   // N lambda / 2 d
             pi2_bragg,             // pi/2 - Bragg angle   
             d_rnd = d_spacing;     // randomly varied d-spacing within given range

      // if (iRep > 0) ChangeNeutronID(&NeutRefl);
      NeutPlCEM.Probability /= nRepete;
      
      /* random d-spacing */
      if (d_fwhm > 0)
      {
        if (d_spr_option == LORENTZIAN) d_rnd = RandomLorentzian(d_spacing, d_fwhm);
        if (d_spr_option == GAUSSIAN)   d_rnd = DistrGauss(d_spacing, d_sigma);
      }
      
       /* computes reflection angle corresponding to random d-spacindg */
       arg = NeutPlCEM.Wavelength * iOrd / 2.0 / d_rnd;
       if (arg >= 1.05) goto refl_exit; /* wavelength too large */
       if (arg >= 1.00) continue;       /* wavelength for the chosen d-spacing too large */
       pi2_bragg = acos(arg);
      
      // Here the reflection probability is calculated
      // and the neutron trajectory changes direction after reflection from a mosaic element.
      CopyNeutron(&NeutPlCEM, &NeutReflCEM);
      NeutReflCEM.Probability *= calcReflProbAndDir(NeutReflCEM.Vector, NeutPlCEM.Vector, pi2_bragg);
      

      switch (eMonoMove)
      {
        case VT_MONO_PST: 
          transf2PST (&NeutReflCE, &NeutReflCEM, false);
          transfCE2In(pNeutRefl,   &NeutReflCE);
          break;
        case VT_MONO_ROT: 
          transf2RotZ(&NeutReflCE, &NeutReflCEM, false);
          transfCE2In(pNeutRefl,   &NeutReflCE);
          break;
        case VT_MONO_OSC: 
          transf2DopplX(pNeutRefl, &NeutReflCEM, false);
          break;
        default:
          transfCE2In (pNeutRefl, &NeutReflCEM);
      }

      /* makes depth correction to get back to the old frame for Depth != 0 */
      RotBackVector(RotMatrixCE, Depth);
      AddVector(pNeutRefl->Position, Depth);

      if (pNeutRefl->Probability < wei_min)
        continue;
      
      // add reflection probability to total reflection probability   	
      if (bTransm == TRUE)
        TotReflProb += pNeutRefl->Probability;
      
      // trajectory visualization 
      if (iRep == 0 && pNeutRefl->Probability > wei_min)
      {
        pNeutRefl->Probability *= nRepete;
        WriteIAP(pNeutRefl, VT_REFLECTED);
      }
      
      // fills structure for reflected neutron in output frame
      transfIn2Out(&NeutReflOut, pNeutRefl);

      /* writes output binary file */
      WriteNeutron(&NeutReflOut);
      NumOut++;
    }
  } // end of loop over reflected neutrons

 refl_exit:
  return TotReflProb;
}

// Transmits neutron through the monochromator crystal
void Monochromator::transmitNeutron(Neutron* pNeutOut, const Neutron* pNeutIn)
{
  double v0, ToF, mu_tot, weight;

  v0 = V_FROM_LAMBDA(pNeutIn->Wavelength);
  ToF = PathLenTrans / v0;
  mu_tot = mu_scat + mu_abs * pNeutIn->Wavelength / 1.798;
  weight = exp(-1.0 * PathLenTransSum * mu_tot);

  CopyNeutron(pNeutIn, pNeutOut);

  ChangeNeutronID(pNeutOut);
  pNeutOut->Time += ToF;
  pNeutOut->Probability *= weight;

  for (int i = 0; i < 3; i++)
    pNeutOut->Position[i] += pNeutOut->Vector[i] * PathLenTrans;

  return;
}


/***********************************************************************
* Monochromator::selectCE             no movement of the monochromator
*              ::rotMonoAndSelectCE   rotation about a vertical axis
*              ::rotPSTAndSelectCE    rotation about an axis in the horizontal plane
*              ::oscMonoAndSelectCE   oscaillation along the x-axis (or close to it)
*
* searches for CE that is hit by neutron in layer h_in
*
* in : pNeutIn  : neutron data in the local co-ordinate system of the monochromator module
*      hIn      : index of the CE layer that is tested
* out: iHit,jHit: indices of the CE that is hit by the neutron
*                   iHit: column,  jHit: row,  hIn: layer   (0,0,0) is top, left, first to be hit
*
* calculated: PathLenTrans, PathLenTransSum
* determined: PosCE, RotMatrixCE,
*             DimCE, RotMatrixBragg
*
* return: CE hit? true or false
*************************************************************************/
bool Monochromator::selectCE(int& iHit, int& jHit, const Neutron* pNeutIn, const int hIn)
{
  bool       bFound = false;
  int        h=0,             // index of the CE element layers
             i=0,             // index of the CE element columns
             j=0,             // index of the CE element rows
             hStart, hEnd;    // first and last layer to be checked  
  VectorType pos, dir;        // position and direction of the neutron in the frame of the CE surface 
  VectorType Pos1, Pos2;      // intersection points with crystal element

  // init
  // CopyNeutron(pNeutIn, pNeutOut);

  if (hIn == -1) {hStart = 0;   hEnd = NumberCE[0];}
  else           {hStart = hIn; hEnd = hIn + 1;}

  // loop over all crystal elements
  for (h = hStart; h < hEnd; h++)         // x-direction
  { for (i = 0; i < NumberCE[1]; i++)      // y-direction
    { for (j = 0; j < NumberCE[2]; j++)    // z-direction
      {
        // copies position, size and orientation data for the current CE
        CopyVectorsToPos(h, i, j, PosCE);
        CopyVectorsToDim(h, i, j, DimCE);
        CopyMatricesToMatrix(h, i, j, RotMatrixCE_F, RotMatrixCE);

        // computes neutron variables in the frame of the CE surface
        transfIn2CE(pos, dir, pNeutIn->Position, pNeutIn->Vector);

        /* here computes the depth where the neutron meets the reflecting plane,
           equivalent to a parallel shift of a t=0 CE in the frame of CE */
        if (IntersectionWithRectangular(DimCE, pos, dir, Pos1, Pos2) == TRUE)
        {
          /* here we have the CE and initialise the values */
          bFound = true;
          PathLenTrans = DistVector(Pos1, Pos2);
          PathLenTransSum += PathLenTrans;

          // vector from CE surface to the plane of reflection
          Depth[0] = MonteCarlo(Pos1[0], Pos2[0]);
          Depth[1] = Depth[2] = 0.0 ;

          /*  // path lengths through the crystal
          if (eMonoMode == REFL_MONO)
            PathLenRefl = 2.0*(Depth[0]-Pos1[0]) * PathLenTrans/(Pos2[0]-Pos1[0]);  // reflection geometry, approximation, correct only for planes parallel to surface
          else
            PathLenRefl = PathLenTrans;                                             // transmission geometry approximation, correct only for planes parallel to surface

          SubVector (pos, Depth);                 // neutron position in the frame of the reflecting surface
          CopyVector(pos, pNeutInCE->Position);
          CopyVector(dir, pNeutInCE->Vector);   */

          /* NOTE: in focusing geometry the Bragg frame and CE frame are coincident*/
          if (eGeomOption != SINGLE_CE)
          {
            int p, q;

            for (p = 0; p < 3; p++)
            {
              for (q = 0; q < 3; q++)
                RotMatrixBragg[p][q] = RotMatrixCE[p][q];
            }
          }

          iHit = i;
          jHit = j;
          return bFound;
        }
      }
    }
  }

  // use monochromator center if no CE was found
  if (bFound == FALSE)
  {
      CopyVector(PosCE0, PosCE);
      CopyVector(DimCE0, DimCE);
  }

  return bFound;
}/* End SelectCE */


bool Monochromator::rotMonoAndSelectCE(int& iHit, int& jHit, double& ToFCE, const Neutron* pNeutIn, const int hIn) 
{
  bool   bHit=false;
  int    h=0,i=0,j=0,               //          columns (i, hor), rows (j, vert) and layer (along beam)
         hStart,hEnd,               //          first and last layer to be checked (usually only hIn)  
         hLast=-1,iLast=-1,jLast=-1;//          column, row and and layer, where it hit in the las (along beam)
  double zeta0, zetaN, zetaO,       //   [rad]  initial, current and previous value of the rotation phase
         DelZeta,                   //   [deg]  and their difference    
         tof =0.0,                  //   [ms]   TOF from the entrance to plane defined by the surface of the CE
         ToF0=pNeutIn->Time;        //   [ms]   TOF from the source to the entrance

  // init
  if (hIn==-1) {hStart = 0;   hEnd = NumberCE[0];}
  else         {hStart = hIn; hEnd = hIn + 1;}
  zeta0 = Zeta0 * M_PI / 180.0;

  // initial time estimation using monochromator center
  fillRotMatrices(zeta0);
  checkCE(ToFCE, MathVector(PosCE0), DimCE0, MathMatrix(RotMatrixCE0), pNeutIn);
  zetaN = zeta0 + omega * (ToF0+ToFCE);   // total TOF
  fillRotMatrices(zetaN);

  do
  { 
    zetaO=zetaN;
    
    // try if it hits the same element as in the previous step
    if (iLast>=0 && jLast>=0 && hLast>=0)
    {
      CopyVectorsToPos    (hLast, iLast, jLast, PosCE);
      CopyVectorsToDim    (hLast, iLast, jLast, DimCE);
      CopyMatricesToMatrix(hLast, iLast, jLast, RotMatrixCE_F, RotMatrixCE);

      bHit=checkCE(tof, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
      if (bHit)
      {
        ToFCE = tof;
        zetaN = zeta0 + omega * (ToF0+ToFCE);   // total TOF 
        fillRotMatrices(zetaN);
      }
    }

    if (bHit==false)
    { // loop over all crystal elements (in the layer)
      for (h=hStart; h < hEnd; h++)           // x-direction
      { for (i=0; i < NumberCE[1]; i++)       // y-direction
        { for (j=0; j < NumberCE[2]; j++)     // z-direction
          {
            // copies position, size and orientation data for the current CE
            CopyVectorsToPos    (h, i, j, PosCE);
            CopyVectorsToDim    (h, i, j, DimCE);
            CopyMatricesToMatrix(h, i, j, RotMatrixCE_F, RotMatrixCE);

            bHit = checkCE(tof, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
            if (bHit)
            {
                iHit  = i; jHit  = j;
                iLast = i; jLast = j; hLast = h;
                ToFCE = tof;
                zetaN = zeta0 + omega * (ToF0 + ToFCE);   // total TOF in sec
                fillRotMatrices(zetaN);
                goto check_rot;
            }
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


bool Monochromator::oscMonoAndSelectCE(int& iHit, int& jHit, double& ToFCE, const Neutron* pNeutIn, const int hIn)
{
  bool       bHit=false;
  VectorType vMF= {0.0,0.0,0.0};        // [cm/ms] speed of the neutron in the frame of the reflecting CE
  int        q=0,                       //         index for axes ((x,y,z),
             h=0,i=0,j=0,               //         columns (i, hor), rows (j, vert) and layer (along beam)
             hStart,hEnd,               //         first and last layer to be checked (usually only hIn)  
             hLast=-1,iLast=-1,jLast=-1;// column, row and and layer, where it hit in the las (along beam)
  double     v0=0.0,                    // [cm/ms] speed of the neutron in the frame of the module
             vDopMax,                   // [cm/ms] max. speed of the Doppler drive
             vModMF=0.0,                // [cm/ms] speed of the neutron in the frame of the moving monochromator
             zeta0=0.0, zeta=0.0,       //  [rad]  initial and current value of the oscillation phase      zeta = 0 means central position of the Doppler Drive
             xDopN=0.0, xDopO=0.0,      //  [cm]   current and previous value of the oscillation position
             DelX=0.0,                  //  [cm]     and their difference                                
             tof =0.0,                  //  [ms]   TOF from the entrance to plane defined by the surface of the CE
             ToF0=pNeutIn->Time;        //  [ms]   TOF from source to entrance

  // init
  if (hIn==-1) {hStart = 0;   hEnd = NumberCE[0];}
  else         {hStart = hIn; hEnd = hIn + 1;}
  zeta0 = Zeta0 * M_PI / 180.0;
   
  // initial time estimation using the monochromator center
  checkCE(ToFCE, MathVector(PosCE0), DimCE0, MathMatrix(RotMatrixCE0), pNeutIn);
  zeta = zeta0 + omega * (ToF0+ToFCE);   // total TOF
  xDopN = AmplDop * sin(zeta);

  do
  { 
    xDopO=xDopN;
    
    // try if it hits the same element as in the previous step
    if (iLast>=0 && jLast>=0 && hLast>=0)
    { CopyVectorsToPos    (hLast, iLast, jLast, PosCE) ;
      CopyVectorsToDim    (hLast, iLast, jLast, DimCE) ;
      CopyMatricesToMatrix(hLast, iLast, jLast, RotMatrixCE_F, RotMatrixCE) ;
      PosCE[0] += xDopN;

      bHit=checkCE(tof, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
      if (bHit)
      {
        ToFCE = tof;
        zeta  = zeta0 + omega * (ToF0+ToFCE); 
        xDopN = AmplDop * sin(zeta);
      }
    }

    if (bHit==false)
    { // loop over all crystal elements (in the layer)
      for (h=hStart; h < hEnd; h++)          // x-direction
      { for (i=0; i < NumberCE[1]; i++)      // y-direction
        { for (j=0; j < NumberCE[2]; j++)    // z-direction
          {
            // copies position, size and orientation data for the current CE
            CopyVectorsToPos    (h, i, j, PosCE);
            CopyVectorsToDim    (h, i, j, DimCE);
            CopyMatricesToMatrix(h, i, j, RotMatrixCE_F, RotMatrixCE);
            PosCE[0] += xDopN;

            bHit = checkCE(tof, MathVector(PosCE), DimCE, MathMatrix(RotMatrixCE), pNeutIn);
            if (bHit)
            {
                iHit  = i; jHit  = j;
                iLast = i; jLast = j; hLast = h;
                ToFCE = tof;
                zeta  = zeta0 + omega * (ToF0 + ToFCE);
                xDopN = AmplDop * sin(zeta);
                goto check_pos;
            }
          }
        }
      }
    }

  check_pos:
    DelX = fabs(xDopN-xDopO);
  }
  while (bHit && DelX > DelXmax);

  // determine position and speed of the Doppler drive for the time that the neutron hits the CE
  v0      = V_FROM_LAMBDA(pNeutIn->Wavelength);
  vDopMax = AmplDop * omega; 

  rDop[0] = AmplDop * sin(zeta) * cos(BraggHor) *cos(BraggVert);
  rDop[1] = AmplDop * sin(zeta) * sin(BraggHor) *cos(BraggVert);
  rDop[2] = AmplDop * sin(zeta) * sin(BraggVert);

  vDop[0] = vDopMax * cos(zeta) * cos(BraggHor) *cos(BraggVert);
  vDop[1] = vDopMax * cos(zeta) * sin(BraggHor) *cos(BraggVert);
  vDop[2] = vDopMax * cos(zeta) * sin(BraggVert);

  return bHit;
}


/***********************************************************************
* Monochromator::transf2RotMono
*                transf2PST
*                transf2DopplX 
*
* transfers the neutron parameters to the co-ordinate system of the moving monochromator or back
*
* out: pNeutOut : neutron parameters in the frame of the moving monochr. (at the time of reflection)
* in : pNeutIn  : neutron parameters in the fixed module frame
*      bForward : false: transformation back from moving system to static system, i.e. input and output are exchanged
*
* return: none
*************************************************************************/
void Monochromator::transf2RotZ(Neutron* pNeutOut, const Neutron* pNeutIn, const bool bForward)
{
  VectorType PosCEChop;                                 // position of the current CE in the frame of the central CE
  double x = 0.0, y = 0.0,                              // position of the neutron in the frame of the central CE
      vxMF = 0.0, vyMF = 0.0, vzMF = 0.0, vModMF = 0.0, // speed of the neutron in the frame of the rotating monochromator
        v0 = 0.0, angle = 0.0, radius = 0.0;

  CopyVector(PosCE, PosCEChop);
  SubVector(PosCEChop, PosCE0);
  RotVector(RotMatrixCE, PosCEChop);           // Vector to current CE is now in the frame of the central CE

  v0 = V_FROM_LAMBDA(pNeutIn->Wavelength);
  x = PosCEChop[0] + pNeutIn->Position[0];
  y = PosCEChop[1] + pNeutIn->Position[1];
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(x, y);

  vxMF = v0 * (pNeutIn->Vector[0]) - omega * radius * cos(angle);
  vyMF = v0 * (pNeutIn->Vector[1]) + omega * radius * sin(angle);
  vzMF = v0 * (pNeutIn->Vector[2]);
  vModMF = sqrt(sq(vxMF) + sq(vyMF) + sq(vzMF));


  CopyNeutron(pNeutIn, pNeutOut);
  pNeutOut->Vector[0] = vxMF / vModMF;
  pNeutOut->Vector[1] = vyMF / vModMF;
  pNeutOut->Vector[2] = vzMF / vModMF;
  pNeutOut->Wavelength = LAMBDA_FROM_V(vModMF);

  return;
}

void Monochromator::transf2PST(Neutron* pNeutOut, const Neutron* pNeutIn, const bool bForward)
{
    VectorType PosCEChop;                                 // position of the current CE in the frame of the central CE
      double dir=-1.0,          
             y  = 0.0, z = 0.0,                           // position of the neutron in the frame of the central CE
             v0_in = 0.0, v0_out = 0.0, 
             angle = 0.0, radius = 0.0;
    MathVector vMono,                                     // velocity of the monochromator
               vNeutIn, vNeutOut;                         // velocity of the neutron absolute and relative to the monochromator

    if (bForward) dir = 1.0;

    v0_in   = V_FROM_LAMBDA(pNeutIn->Wavelength);
    vNeutIn = MathVector(v0_in * pNeutIn->Vector[0], v0_in * pNeutIn->Vector[1], v0_in * pNeutIn->Vector[2]);

    CopyVector(PosCE, PosCEChop);
    SubVector(PosCEChop, PosCE0);
    RotVector(RotMatrixCE, PosCEChop);           // Vector to current CE is now in the frame of the central CE

    // velocity of the monochromator position that is hit by the neutron (in the chopper frame)
    y = PosCEChop[1] + pNeutIn->Position[1];
    z = PosCEChop[2] + pNeutIn->Position[2] + RadiusPST;
    radius = sqrt(sq(y) + sq(z));
    angle = atan2(y, z);
    vMono = MathVector(0.0, omega * radius * cos(angle), -omega * radius * sin(angle));  

    vNeutOut = vNeutIn - vMono * dir;
    v0_out   = vNeutOut.Mod();

    CopyNeutron(pNeutIn, pNeutOut);
    pNeutOut->Vector[0] = vNeutOut.getX() / v0_out;
    pNeutOut->Vector[1] = vNeutOut.getY() / v0_out;
    pNeutOut->Vector[2] = vNeutOut.getZ() / v0_out;
    pNeutOut->Wavelength = LAMBDA_FROM_V(v0_out);

    return;
}


void Monochromator::transf2DopplX(Neutron* pNeutOut, const Neutron* pNeutIn, const bool bForward)
{
  int     j=0;                    //         index of axes
  double  dir  =-1.0,          
          v_in = 0.0,             // [cm/ms] speed of the neutron in the frame of the moving monochromator
          v_out= 0.0;                // [cm/ms] speed of the neutron in the frame of the module
  VectorType vOut= {0.0,0.0,0.0};    // [cm/ms] speed of the neutron in the fixed module frame

  if (bForward) dir = 1.0;

  // determine speed in the fixed module frame
  v_in = V_FROM_LAMBDA(pNeutIn->Wavelength);
  for (j=0; j < 3; j++)
    vOut[j] = v_in * (pNeutIn->Vector[j]) - dir * vDop[j];
  v_out = LengthVector(vOut);

  CopyNeutron(pNeutIn, pNeutOut);
  pNeutOut->Wavelength = LAMBDA_FROM_V(v_out);

  for (j=0; j < 3; j++)
  { pNeutOut->Position[j] += rDop[j];
    pNeutOut->Vector[j]    = vOut[j]/v_out;
  }

  return;
}


/***********************************************************************
* Monochromator::checkCE
*
* checks if neutron hits a plane and calculates flight time until arrival
*
* in : pNeut : neutron parameters in the module frame
*      Mrot  : matrix to rotate from from module frame to CE frame
*      SizeCE: size of the current CE (thickness, width, height)
*      vPosCE: CE position in the module frame 
* out: Time  : TOF to hit the CE
*
* return: on CE? true or false
*************************************************************************/
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

/***********************************************************************
* Monochromator::isNeutInCE
*
* checks if neutron is on a crystal element (CE)
*
* in : vPosN : neutron position in the frame of the current CE
*      SizeCE: size of the current CE (thickness, width, height)
*
* return: on CE? true or false
*************************************************************************/
bool Monochromator::isNeutInCE(MathVector vPosN, const VectorType SizeCE)
{
  bool bIn = false;

  if (fabs(vPosN.getX()) <= 0.5*SizeCE[0] &&
      fabs(vPosN.getY()) <= 0.5*SizeCE[1] &&
      fabs(vPosN.getZ()) <= 0.5*SizeCE[2]   )
    bIn=true;

  return bIn;
}


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


/*********************************************************************************************************************/

/*******************************************************/
/* 'lambda-focusing' option                            */
/*******************************************************/
void Monochromator::crys_geomLambda()
{
  int    h=0,                    // index of the CE element layers
         i=0,                    // index of the CE element columns
         j=0;                    // index of the CE element rows
  double b, c, R1, R11, R2, R20,
         RotView[3][3],
         ShiftX = 0.0;           // shift of the layer relative to the center of the CE ensemble 
  double Psi, PsiC, Theta, DeltaTheta;

  VectorType Step_H, Step_V ;

  FillRotMatrixZY(RotMatrixCE, -SrfcVert, -SrfcHor) ;
  FillRotMatrixZY(RotView, 10.0 * M_PI / 180.0, -95.0 * M_PI / 180.0) ;

  if (RadV <= 0.0)
    Error("radius must be > 0");

  R20 = 0.0;

  /* computes CE parameters */
  b   = -DimCE0[2] * cos(M_PI_2 + SrfcVert);
  c   = - ( sq(RadV) + RadV * DimCE0[2] * cos(M_PI_2 + SrfcVert) );
  R2  = ( - b + sqrt(sq(b) - 4. * c) ) / 2.;
  R11 = sqrt( sq(RadV) + sq(DimCE0[2] / 2.) + RadV * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;

  PsiC =  acos( (sq(RadV) + sq(R11) - sq(DimCE0[2] / 2.)) / 2. / RadV / R11 )
        + acos( (sq(R11) + sq(R2) - sq(DimCE0[2] / 2.)) / 2. / R11 / R2 );

  for (h=0; h < NumberCE[0]; h++)	   /* step along beam, loop over arrays */
  {
    ShiftX = (2*h + 1 - NumberCE[0]) / 2.0 * SpcLayer;

    for (i=0; i < NumberCE[1]; i++)		/* step horizontal */
    {
      R2 = RadV /* initial R1 */ ;
      Psi = M_PI_2 - Radians(Psi0) + PsiC ;
      DeltaTheta = 2. * atan(DimCE0[1] / 2. / R2) ;
      Theta = ((NumberCE[1] - 1) / 2. - i) * DeltaTheta ;
   
      declareVectors(h);
   
      for (j=0; j < NumberCE[2]; j++)	/* step vertical */
      {
	    R1  = R2 ;
	    b   = - DimCE0[2] * cos(M_PI_2 + SrfcVert) ;
	    c   = - ( sq(R1) + R1 * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;
	    R2  = ( - b + sqrt(sq(b) - 4. * c) ) / 2.  ;
	    R11 = sqrt( sq(R1) + sq(DimCE0[2] / 2.) + R1 * DimCE0[2] * cos(M_PI_2 + SrfcVert) ) ;

	    Psi = Psi - acos( (sq(R1) + sq(R11) - sq(DimCE0[2] / 2.)) / 2. / R1 / R11 )
                  - acos( (sq(R11) + sq(R2) - sq(DimCE0[2] / 2.)) / 2. / R11 / R2 ) ;

	    /* computes x translation at the end */
	    if (fabs(Psi - M_PI_2) <= atan(DimCE0[2] / 2. / R2))
        R20 = R2 ;

	    /* output focussing geometry parameters */
	    PosCEx_F[h][i].push_back( R2 * sin(Psi) * cos(Theta) + ShiftX);
	    PosCEy_F[h][i].push_back( R2 * sin(Psi) * sin(Theta) );
	    PosCEz_F[h][i].push_back( R2 * cos(Psi) );


	    if (i!=0 && j!=0)
	    {
          Step_H[0] = PosCEx_F[h][i][j] - PosCEx_F[h][i - 1][j];
          Step_H[1] = PosCEy_F[h][i][j] - PosCEy_F[h][i - 1][j];
          Step_H[2] = PosCEz_F[h][i][j] - PosCEz_F[h][i - 1][j];
          Step_V[0] = PosCEx_F[h][i][j] - PosCEx_F[h][i][j - 1];
          Step_V[1] = PosCEy_F[h][i][j] - PosCEy_F[h][i][j - 1];
          Step_V[2] = PosCEz_F[h][i][j] - PosCEz_F[h][i][j - 1];

	      DimCEx_F[h][i].push_back(0.0) ;
	      DimCEy_F[h][i].push_back(0.9 * (LengthVector(Step_H) - DimCE0[1]));
	      DimCEz_F[h][i].push_back(0.9 * (LengthVector(Step_V) - DimCE0[2]));

	    } 
	    else 
        {
            DimCEx_F[h][i].push_back(0.0);
            DimCEy_F[h][i].push_back(0.0);
            DimCEz_F[h][i].push_back(0.0);
        }

	    RotCEh_F[h][i].push_back( (Theta) * 180. / M_PI       + MonteCarlo(-0.5*DevH, 0.5*DevH) + addLayerDecl(h));
	    RotCEv_F[h][i].push_back((M_PI_2 - Psi) * 180. / M_PI + MonteCarlo(-0.5*DevV, 0.5*DevV));

	  }
    }
  }

  rotateCEs();

  /* print to file */
  writeFocData();

  return ;
}


/*******************************************************/
/* 'sphere-focusing' option                            */
/*******************************************************/
void	Monochromator::crys_geomSphere()
{
  int     h=0,                        // index of the CE element layers
          i=0,                        // index of the CE element columns
          j=0,                        // index of the CE element rows
          q=0;                        // index: x, y, z              
  double  R,                          // radius of the sphere
          ShiftX = 0.0,               // shift of the layer relative to the center of the CE ensemble 
          Psi, Theta,                 // vertical and horizontal orientation of the crystal element
          DelPsi, DelTheta,           // Angular dfferences between neighboring rows/columns 
          RotView[3][3], 
          DistRows, DistCols;         // distance between 2 rows/columns (= element height/width  + gap)  ;
  VectorType	r, Step_H, Step_V ;

  DistRows = DimCE0[2] + GapV;   // vertical
  DistCols = DimCE0[1] + GapH;   // horizontal

  FillRotMatrixZY(RotMatrixCE, - SrfcVert, - SrfcHor) ; 
  FillRotMatrixZY(RotView, 10.0 * M_PI / 180.0, -95.0 * M_PI / 180.0) ;

  if (RadV <= 0.0)
    Error("radius must be > 0");

  R = RadV;                                     // = sqrt(sq(RadV) - sq(DistRows / 2.)) ; 
  DelPsi   = 2.0 * asin(0.5 * DistRows / R);    // vertical
  DelTheta = 2.0 * asin(0.5 * DistCols / R);    // horizontal

  /*computes CE parameters */
  for (h=0; h < NumberCE[0]; h++)	   /* step along beam, loop over arrays */
  {
    ShiftX = (2*h + 1 - NumberCE[0]) / 2.0 * SpcLayer;

    for(i=0; i < NumberCE[1];i++)		/* step horizontal */
    {
      Psi = M_PI_2 - Radians(Psi0);
      Theta = ((NumberCE[1]-1) / 2. - i) * DelTheta ;
     
      declareVectors(h);  
     
      for (j=0; j < NumberCE[2]; j++)	/* step vertical */
      {
        /* output focussing geometry parameters */
        r[0] = R * sin(Psi) * cos(Theta) - R + ShiftX;
        r[1] = R * sin(Psi) * sin(Theta);
        r[2] = R * cos(Psi);

        PosCEx_F[h][i].push_back(r[0]);
        PosCEy_F[h][i].push_back(r[1]);
        PosCEz_F[h][i].push_back(r[2]);

        if (i != 0 && j != 0)
        {
          Step_H[0] = PosCEx_F[h][i][j] - PosCEx_F[h][i-1][j];
          Step_H[1] = PosCEy_F[h][i][j] - PosCEy_F[h][i-1][j];
          Step_H[2] = PosCEz_F[h][i][j] - PosCEz_F[h][i-1][j];
          Step_V[0] = PosCEx_F[h][i][j] - PosCEx_F[h][i][j-1];
          Step_V[1] = PosCEy_F[h][i][j] - PosCEy_F[h][i][j-1];
          Step_V[2] = PosCEz_F[h][i][j] - PosCEz_F[h][i][j-1];

          DimCEx_F[h][i].push_back(0.0);
          DimCEy_F[h][i].push_back(0.9 * (LengthVector(Step_H) - DimCE0[1]));
          DimCEz_F[h][i].push_back(0.9 * (LengthVector(Step_V) - DimCE0[2]));
        }
        else
        {
          DimCEx_F[h][i].push_back(0.0);
          DimCEy_F[h][i].push_back(0.0);
          DimCEz_F[h][i].push_back(0.0);
        }

        RotCEh_F[h][i].push_back((Theta) * 180. / M_PI + MonteCarlo(-0.5 * DevH, 0.5 * DevH) + addLayerDecl(h));
        RotCEv_F[h][i].push_back((M_PI_2 - Psi) * 180. / M_PI + MonteCarlo(-0.5 * DevV, 0.5 * DevV));

        Psi = Psi - DelPsi;
      }
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
  int     h=0,                // index of the CE element layers
          j=0;                // index of the CE element rows
  double  R,                  // Radius
          ShiftX=0.0,         // shift of the layer relative to the center of the CE ensemble 
          Psi, DelPsi,        // vertical angles
          RotView[3][3], 
          DistRows;           // distance between 2 rows (= slab height + gap)  ;
  VectorType r={0.0,0.0,0.0}; // position of the CE relative to the center of the CE ensemble

  NumberCE[1] = 1;    // only 1 column
  DistRows = DimCE0[2] + GapV;

  FillRotMatrixZY(RotMatrixCE, -SrfcVert, -SrfcHor) ; 
  FillRotMatrixZY(RotView, 10.0 * M_PI / 180.0, -95.0 * M_PI / 180.0) ;

  if(RadV == 0.)
    Error("ERROR: radius must be > 0");

  R =  RadV;

  /*computes CE parameters */
  DelPsi = 2.0 * asin(0.5 * DistRows / R);    // vertical
  Psi    = M_PI_2 - Radians(Psi0);

  declareVectors(h);

  for (h=0; h < NumberCE[0]; h++)	   /* step along beam, loop over arrays */
  {
    ShiftX = (2*h + 1 - NumberCE[0]) / 2.0 * SpcLayer;

    for (j=0; j < NumberCE[2]; j++)	/* step vertical */
    {
      /* output focussing geometry parameters */
      r[0] = R * sin(Psi) - R + ShiftX;
      r[1] = 0.;
      r[2] = R * cos(Psi);

      DimCEx_F[h][0].push_back(0.0);
      PosCEx_F[h][0].push_back(r[0]);
      PosCEy_F[h][0].push_back(r[1]);
      PosCEz_F[h][0].push_back(r[2]);
      DimCEy_F[h][0].push_back(0.0);
      DimCEz_F[h][0].push_back(0.0);
      RotCEh_F[h][0].push_back(MonteCarlo(-0.5 * DevH, 0.5 * DevH) + addLayerDecl(h));
      RotCEv_F[h][0].push_back(MonteCarlo(-0.5 * DevV, 0.5 * DevV) + (M_PI_2 - Psi) * 180. / M_PI);

      Psi -= DelPsi;
    }
  }

  /*rotate CEs and print to file */
  rotateCEs();
  writeFocData();

  return ;
   
}/* End Crys_GeomVertCyl */


/*******************************************************/
/* 'double focussing cylinder' option                  */
/*******************************************************/
void   Monochromator::crys_geomDoubleCyl()   
{
  int    h=0,                 // index of the CE element layers
         i=0,                 // index of the CE element columns
         j=0;                 // index of the CE element rows
  double CE_Width =0.0,       /* width and height of one monochromator element */
         CE_Height=0.0,
         Psi  =0.0,           /* vertical angle to monochromator slab under consideration */
         Theta=0.0;           /* horizontal angle to monochromator slab under consideration */
  VectorType r={0.0,0.0,0.0}; /* position of the CE relative to the center of the CE ensemble */
  
  FillRotMatrixZY(RotMatrixCE, -SrfcVert, -SrfcHor) ; 
  
  CE_Width  = DimCE0[1];
  CE_Height = DimCE0[2];
  
  if (RadV==0.0 && RadH==0.0)
  	Warning("both radii are zero");
  
  /* computes CE parameters */
  for (h=0; h < NumberCE[0]; h++)	   /* step along beam, loop over arrays */
  {
    r[0] = (2*h + 1 - NumberCE[0]) / 2.0 * SpcLayer;

    for (i=0; i < NumberCE[1]; i++)	   /* step horizontal, loop over columns */
    {
      r[1] = (NumberCE[1] - 1 - 2 * i) / 2.0 * (CE_Width + GapH);
      if (RadH > 0.0)
          Theta = atan(r[1] / RadH);
      else
          Theta = 0.0;

      declareVectors(h);

      for (j=0; j < NumberCE[2]; j++)	/* step vertical,   loop over rows */
      {
        /* output focusing geometry parameters */
        r[2] = (NumberCE[2] - 1 - 2 * j) / 2.0 * (CE_Height + GapV);

        if (RadV > 0.0)
            Psi = atan(r[2] / RadV);
        else
            Psi = 0.0;

        PosCEx_F[h][i].push_back(r[0]);
        PosCEy_F[h][i].push_back(r[1]);
        PosCEz_F[h][i].push_back(r[2]);
        DimCEx_F[h][i].push_back(0.0);
        DimCEy_F[h][i].push_back(0.0);
        DimCEz_F[h][i].push_back(0.0);

        RotCEh_F[h][i].push_back(Theta * 180. / M_PI + MonteCarlo(-0.5 * DevH, 0.5 * DevH) + addLayerDecl(h));
        RotCEv_F[h][i].push_back(Psi   * 180. / M_PI + MonteCarlo(-0.5 * DevV, 0.5 * DevV));
      }
    }
  }
  
  rotateCEs();

  /* print to file */
  writeFocData();

  return;

}/* End crys_geomDoubleCyl */


/*******************************************************/
/* 'no focusing' option                                */
/*******************************************************/
void   Monochromator::crys_geomNoFocus()   
{
  int    h=0,                 // index of the CE element layers
         i=0,                 // index of the CE element columns
         j=0;                 // index of the CE element rows
  double CE_Width =0.0,       /* width and height of one monochromator element */
         CE_Height=0.0;
  VectorType r={0.0,0.0,0.0}; /* position of the CE relative to the center of the CE ensemble */
  
  FillRotMatrixZY(RotMatrixCE, -SrfcVert, -SrfcHor) ; 
  
  CE_Width  = DimCE0[1];
  CE_Height = DimCE0[2];
  
  /* computes CE parameters */
  for (h=0; h < NumberCE[0]; h++)	   /* step along beam, loop over arrays */
  {
    r[0] = (2*h + 1 - NumberCE[0]) / 2.0 * SpcLayer;

    for (i=0; i < NumberCE[1]; i++)	   /* step horizontal, loop over columns */
    {
      r[1] = (2*i + 1 - NumberCE[1]) / 2.0 * (CE_Width + GapH);

      declareVectors(h);

      for (j=0; j < NumberCE[2]; j++)	/* step vertical,   loop over rows */
      {
        /* output focusing geometry parameters */
        r[2] = (2*j + 1 - NumberCE[2]) / 2.0 * (CE_Height + GapV);

        PosCEx_F[h][i].push_back(r[0]);
        PosCEy_F[h][i].push_back(r[1]);
        PosCEz_F[h][i].push_back(r[2]);
        DimCEx_F[h][i].push_back(0.0);
        DimCEy_F[h][i].push_back(0.0);
        DimCEz_F[h][i].push_back(0.0);

        RotCEh_F[h][i].push_back(MonteCarlo(-0.5 * DevH, 0.5 * DevH) + addLayerDecl(h));
        RotCEv_F[h][i].push_back(MonteCarlo(-0.5 * DevV, 0.5 * DevV));
      }
    }
  }
  
  rotateCEs();

  /* print to file */
  writeFocData();

  return;

}/* End crys_geomDoubleCyl */


/********************************************************************/
/* rotateCEs()    rotates CE positions to module frame              */
/* addDev2Std()   adds the deviations of each CE to standard values */
/* addLayerDecl() adds declination of the layer                     */
/* writeFocData() writes calculated crystal data to file            */
/********************************************************************/
//
void Monochromator::rotateCEs()
{
  int i = 0,  // index of the CE element columns
      j = 0,  // index of the CE element rows
      h = 0;  // index of the CE element layers
  VectorType r;

  InitVector(r);
  for (h=0; h < NumberCE[0]; h++)       /* step along beam */
  { for (i=0; i < NumberCE[1]; i++)		/* step horizontal */
    { for (j=0; j < NumberCE[2]; j++)	/* step vertical */
	  {
	    /*PosCE_F[0][i][j] += R ;*/
	    /* rotate  */
	    CopyVectorsToPos(h, i, j, r) ;
	    RotVector       (RotMatrixCE, r) ;
	    CopyPosToVectors(h, i, j, r) ;
	  }
    }
  }

  return;
}


void Monochromator::addDev2Std()
{
  int i = 0,  // index of the CE element columns
      j = 0,  // index of the CE element rows
      h = 0;  // index of the CE element layers

  for (h=0; h < NumberCE[0]; h++)       /* step along beam */
  { for (i=0; i < NumberCE[1]; i++)     /* step horizontal */
    { for (j=0; j < NumberCE[2]; j++)   /* step vertical */
      {
        /* converts degs in radian etc. */
	      RotCEh_F[h][i][j] *= M_PI/180.0;
	      RotCEv_F[h][i][j] *= M_PI/180.0;

        RotCEh_F[h][i][j] += SrfcHor;
        RotCEv_F[h][i][j] += SrfcVert;

	      PosCEx_F[h][i][j] += PosCE0[0] ;
        PosCEy_F[h][i][j] += PosCE0[1];
        PosCEz_F[h][i][j] += PosCE0[2];
        DimCEx_F[h][i][j] += DimCE0[0];
        DimCEy_F[h][i][j] += DimCE0[1];
        DimCEz_F[h][i][j] += DimCE0[2];
	    }
    }
  }
  return;
}


double Monochromator::addLayerDecl(int hLayer)
{
  double DelDirHor = 0.0;

  if (NumberCE[0] > 1 && DeclLayer != 0.0)
    DelDirHor = DeclLayer * (2.0 * hLayer / (NumberCE[0] - 1.0) - 1.0);

  return DelDirHor;
}


void Monochromator::writeFocData()
{
  int i=0,  // index of the CE element columns
      j=0,  // index of the CE element rows
      h=0;  // index of the CE element layers

  // open geometry file to save calculated data
  pGeomFile = OpenOutputFile(GeomFileName, FALSE, "w") ;

  if (pGeomFile != NULL)
  { 
    fprintf(pGeomFile, "# layers colums rows\n");
    fprintf(pGeomFile, "   %3d    %3d   %3d\n\n", NumberCE[0], NumberCE[1], NumberCE[2]);

    fprintf(pGeomFile, "# deviations from the central crystal element in \n");
    fprintf(pGeomFile, "# x-pos     y-pos     z-pos    thickness   width     height   horizontal vertical orient.\n");
    fprintf(pGeomFile, "#  [cm]      [cm]      [cm]       [cm]      [cm]      [cm]      [deg]     [deg]          \n");

    for (h=0; h < NumberCE[0]; h++)       /* step along beam */
    { for (i=0; i < NumberCE[1]; i++)     /* step horizontal */
      { for (j=0; j < NumberCE[2]; j++)   /* step vertical */
        {
          fprintf(pGeomFile, "%9.6f %9.6f %9.6f  %9.6f %9.6f %9.6f  %9.6f %9.6f\n",
                  PosCEx_F[h][i][j], PosCEy_F[h][i][j], PosCEz_F[h][i][j], DimCEx_F[h][i][j], DimCEy_F[h][i][j], DimCEz_F[h][i][j], RotCEh_F[h][i][j], RotCEv_F[h][i][j]);
        }
        if (NumberCE[2] > 1) fprintf(pGeomFile, "\n");
      }
      if (NumberCE[1] > 1) fprintf(pGeomFile, "\n");
    }

    fclose(pGeomFile) ;
  }
  else
  {
    Warning("Geometry file could not be opened. Calculated data are not written to a file.");
  }

  return;
}



/*****************************************************************/
/** Co-ordinate transformations from input to CE frame and back **/
/*****************************************************************/
void Monochromator::transfIn2CE (Neutron* pNeutOut, const Neutron* pNeutIn)
{
    /* computes neutron variables in the CE frame */
    CopyNeutron(pNeutIn, pNeutOut);

    SubVector(pNeutOut->Position, PosCE);
    RotVector(RotMatrixCE,    pNeutOut->Position);
    RotVector(RotMatrixBragg, pNeutOut->Vector);
}

void Monochromator::transfCE2In (Neutron* pNeutOut, const Neutron* pNeutIn)
{
    /* computes neutron variables in the initial frame */
    CopyNeutron(pNeutIn, pNeutOut);

    RotBackVector(RotMatrixBragg, pNeutOut->Vector);
    RotBackVector(RotMatrixCE,    pNeutOut->Position);
    AddVector(pNeutOut->Position, PosCE);
}

void Monochromator::transfIn2Out(Neutron* pNeutOut, const Neutron* pNeutIn)
{
    /* computes neutron variables in the output frame */
    CopyNeutron(pNeutIn, pNeutOut);

    SubVector(pNeutOut->Position, Transl);
    RotVector(RotMatrixOut, pNeutOut->Position);
    RotVector(RotMatrixOut, pNeutOut->Vector);
}


void Monochromator::transfIn2CE(VectorType pos_CE, VectorType dir_CE, const VectorType pos_In, const VectorType dir_In)
{
    /* computes neutron variables in the CE frame */
    CopyVector(pos_In, pos_CE);
    CopyVector(dir_In, dir_CE);

    SubVector(pos_CE, PosCE);
    RotVector(RotMatrixCE,    pos_CE);
    RotVector(RotMatrixBragg, dir_CE);
}

void Monochromator::transfCE2In(VectorType pos_In, VectorType dir_In, const VectorType pos_CE, const VectorType dir_CE)
{
    /* computes neutron variables in the initial frame */
    CopyVector(pos_CE, pos_In);
    CopyVector(dir_CE, dir_In);

    RotBackVector(RotMatrixBragg, dir_In);
    RotBackVector(RotMatrixCE,    pos_In);
    AddVector(pos_In, PosCE);
}

void Monochromator::transfIn2Out(VectorType pos_Out, VectorType dir_Out, const VectorType pos_In, const VectorType dir_In)
{
    /* computes neutron variables in the output frame */
    CopyVector(pos_In, pos_Out);
    CopyVector(dir_In, dir_Out);

    SubVector(pos_Out, Transl);
    RotVector(RotMatrixOut, pos_Out);
    RotVector(RotMatrixOut, dir_Out);
}


/*********************************************************************************************************************/
// 4 functions to copy rotation matrices or vectors to arrays and get them out again 
//
void Monochromator::CopyMatricesToMatrix(int h, int i, int j, std::vector < std::vector<double> >  Matrix[3][3][NUM_LAYERS], double Result[3][3])
{
  int l = 0,  // column index of the rotation matrix
      m = 0;  // row index of the rotation matrix

  for (l=0; l < 3; l++) 
  { for (m=0; m < 3; m++) 
    {
      if (h < NumberCE[0] && i < Matrix[l][m][h].size() && j < Matrix[l][m][h][i].size())
	    Result[l][m] = Matrix[l][m][h][i][j] ;
	  else 
        Error("Trying to access non-existing matrix elements in CopyMatricesToMatrix") ;
    }
  }
  return;
}

void Monochromator::CopyMatrixToMatrices(int h, int i, int j, double Result[3][3], std::vector < std::vector<double> > Matrix[3][3][NUM_LAYERS])
{
  int l=0,  // column index of the rotation matrix
      m=0;  // row index of the rotation matrix

  std::vector<double> tempVector;

  for (int l=0; l < 3; l++) 
  { for (int m=0; m < 3; m++) 
    {
      while (Matrix[l][m][h].size() < i+1)
        Matrix[l][m][h].push_back(tempVector);
      while (Matrix[l][m][h][i].size() < j+1) 
        Matrix[l][m][h][i].push_back(0.0);

      Matrix[l][m][h][i][j] = Result[l][m];
    }
  }

  return;
}

void Monochromator::CopyVectorsToPos(int h, int i, int j, double Pos[3])
{
  if (h < NumberCE[0] && i < NumberCE[1] && j < NumberCE[2])
  { 
    Pos[0] = PosCEx_F[h][i][j];
    Pos[1] = PosCEy_F[h][i][j];
    Pos[2] = PosCEz_F[h][i][j];
  }
  else 
  {
    Error("Trying to access non - existing matrix elements in CopyVectorsToVector");
  }
  
  return;
}

void Monochromator::CopyVectorsToDim(int h, int i, int j, double Dim[3])
{
  if (h < NumberCE[0] && i < NumberCE[1] && j < NumberCE[2])
  {
      Dim[0] = DimCEx_F[h][i][j];
      Dim[1] = DimCEy_F[h][i][j];
      Dim[2] = DimCEz_F[h][i][j];
  }
  else
  {
      Error("Trying to access non - existing matrix elements in CopyVectorsToVector");
  }

  return;
}

void Monochromator::CopyPosToVectors(int h, int i, int j, double Pos[3])
{
  std::vector<double> tempVector;
    
  while (PosCEx_F[h].size() < i+1)
    PosCEx_F[h].push_back(tempVector);
  while (PosCEy_F[h].size() < i+1)
    PosCEy_F[h].push_back(tempVector);
  while (PosCEz_F[h].size() < i+1)
    PosCEz_F[h].push_back(tempVector);

  while (PosCEx_F[h][i].size() < j+1)
    PosCEx_F[h][i].push_back(0.0);
  while (PosCEy_F[h][i].size() < j+1)
    PosCEy_F[h][i].push_back(0.0);
  while (PosCEz_F[h][i].size() < j+1)
    PosCEz_F[h][i].push_back(0.0);

  PosCEx_F[h][i][j] = Pos[0];
  PosCEy_F[h][i][j] = Pos[1];
  PosCEz_F[h][i][j] = Pos[2];

  return;
}

void Monochromator::CopyDimToVectors(int h, int i, int j, double Dim[3])
{
    std::vector<double> tempVector;

    while (DimCEx_F[h].size() < i + 1)
        DimCEx_F[h].push_back(tempVector);
    while (DimCEy_F[h].size() < i + 1)
        DimCEy_F[h].push_back(tempVector);
    while (DimCEz_F[h].size() < i + 1)
        DimCEz_F[h].push_back(tempVector);

    while (DimCEx_F[h][i].size() < j + 1)
        DimCEx_F[h][i].push_back(0.0);
    while (DimCEy_F[h][i].size() < j + 1)
        DimCEy_F[h][i].push_back(0.0);
    while (DimCEz_F[h][i].size() < j + 1)
        DimCEz_F[h][i].push_back(0.0);

    DimCEx_F[h][i][j] = Dim[0];
    DimCEy_F[h][i][j] = Dim[1];
    DimCEz_F[h][i][j] = Dim[2];

    return;
}


// initializes arrays
void Monochromator::declareVectors(int h)
{
  std::vector <double> tempVector;

  PosCEx_F[h].push_back(tempVector);
  PosCEy_F[h].push_back(tempVector);
  PosCEz_F[h].push_back(tempVector);
  DimCEx_F[h].push_back(tempVector);
  DimCEy_F[h].push_back(tempVector);
  DimCEz_F[h].push_back(tempVector);
  RotCEh_F[h].push_back(tempVector);
  RotCEv_F[h].push_back(tempVector);
  
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
