/*******************************************************************************/
/* Tool Generate Bispectral Extraction File:                                   */
/*  Generating input file for sm_ensemble module                               */
/*  describing n mirror plates surrounded by m  guide walls                    */
/*                                                                             */
/* 1.0  Dec 2012  C.Zendler  initial version                                   */
/* 1.1  Oct 2013  C.Zendler  add new file format                               */
/*******************************************************************************/
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string>
#include <vector>
#include <iostream>

extern "C" {
#include "init.h"
}

using namespace std;

double GetDouble(const char* pText);
string GetString(const char* pText);
void   GetChar(char* pString, const char* pText);


int main(int argc, char* argv[])
{
  const double PI=3.1415926535898;
  const int MaxNComponents=50;      // max mirr+wall components, set in sm_ensemble
  const double ThetaC=0.00173;      // crit. angle for 1 A neutroms
  const double Mu_Si=0.0048;        // attenuation coeff. for 1 A neutrons for wavelength dependent part
  const double MuInc_Si=0.030;      // attenuation coeff. for wavelength independent part
  const double Rav[7]={0.99, 0.93, 0.88, 0.78, 0.69, 0.61, 0.49}; //reflectivity for m=1...7: R_ave for Ni/Ti from  http://www.swissneutronics.ch/products/coatings.html

  /* input parameters */
  int             Nmirr=0;                                  // number of mirrors
  double          DistMirr=0,                               // distance of mirrors if equidistant
                  Width1Mirr=0,                             // y/z-length of mirror plates at entry position
                  Width2Mirr=0;                             // 2. y/z-length of mirror plates at exit position (if not rectangular)
  vector<double>  AngleMirr,                                // mirr inclination to x axis in hor./vert. direction
                  LengthMirr,                               // mirr length
                  CenterMirr,                               // x-coord. mirr centre
                  CenterYMirr,                              // y/z-coord. mirr centre
                  DMirr,                                    // mirror thickness
                  CoatingMirr;                              // coating (m-number)
  vector<string>  Name;                                     // mirr names
  vector<bool>    TransparentMirr,                          // tag transparent mirror vs. intransparent just reflecting 
                  HorPlane;                                 // mirror plane: horizontal or vertertical
  bool            sameAngleMirr=false,                      // if more than one mirror: parallel or not
                  newFileFormat=true;                       // new file format: only m_up, m_down needed
  string          helper,                                   // convert yes-no to bool
                  simpleShapeMirr;                          // rectangular vs trapezoidal shape 
  char            fileName[50]="MyBispectralExtraction.dat",
                  *pFullFileName;

  /* output parameters (see sm_ensemble description) */
  vector<double>  Y1, Y2, Y3, Y4, Z1, Z2, Z3, Z4, AngleHV, ThetaCSM, Rcsm, Mud, MuInc;

  FILE*   pFile;

  Init(argc, argv, VT_TOOL);

  printf("-----------------------------------------------------------------------------\n");
  printf("Generation of an input file for the 'sm_ensemble' module                     \n");
  printf("Version: 1.1                                                                 \n");
  printf("-----------------------------------------------------------------------------\n\n");
  printf("Input parameter help:  [ SEE ALSO HELP MANUAL ]                              \n");
  printf("- File format: new format includes only supermirror coating m, no R_csm etc  \n");
  printf("- Number of mirrors: transparent + intransparent (former 'guide walls')      \n");
  printf("- Mirror plane: h=horizontal= mirror in x-y-plane                            \n");
  printf("                v=vertical  = mirror in x-z-plane                            \n");
  printf("- Mirror inclination: clock-wise rotation for positive angle                 \n");
  printf("- same Mirror length: 'yes': the x-centre of the mirrors is half the length. \n");
  printf("     For other centre(s) choose 'no' and give (same) length for each mirror  \n");
  printf("- Smallest y/z-coordinate of mirror centre: for equidistant mirrors,         \n");
  printf("     give hor./vert. position of lowest mirror in hor./vert. direction       \n");
  printf("- Mirrors with different angles, lengths, centres:                           \n");
  printf("     give all values in same order (e.g. bottom to top)                      \n");
  printf("-----------------------------------------------------------------------------\n\n");
  
  ///////////////////////////////////////////////////////////////////////
  // read in parameters:  
  ///////////////////////////////////////////////////////////////////////
  cout<<"Choose file format (new/old): ";
  cin>>helper;
  while(helper!="new" && helper!="n" && helper=="old" && helper=="o"){
    cout<<"ERROR: invalid input, please answer 'new' or 'old'"<<endl;
    cin>>helper;
  }
  if(helper=="new" || helper=="n")
    newFileFormat=true;
  else if(helper=="old" || helper=="o")
    newFileFormat=false;

  cout<<"Number of mirrors: ";
  cin>>Nmirr;
  while(Nmirr>MaxNComponents || Nmirr<0){
    cout<<"ERROR: Please give number between 0 and "<<MaxNComponents<<": ";
    cin>>Nmirr;
  }
  
  if(Nmirr>0){
	

    if(Nmirr>1){
      cout<<"Equidistant mirrors of same length and with same inclination? (y/n): ";
      cin>>helper;
      while( helper!="y" && helper!="n" ){
	cout<<"ERROR: please choose yes ('y') or no ('n'): ";
	cin>>helper;
      }
      sameAngleMirr=(helper=="y");
    }
     
    if( sameAngleMirr ){

      cout<<"Mirror plane (h,v):";
      cin>>helper;
      while( helper!="h" && helper!="v" ){
	cout<<"ERROR: Please choose horizontal (h) or vertical (v) direction as mirror plane: ";
	cin>>helper;
      }
      HorPlane.push_back(helper=="h");

      cout<<"Transparent mirrors? (yes/no): ";
      cin>>helper;
      while(helper!="yes" && helper!="y" && helper=="no" && helper=="n"){
	cout<<"ERROR: please answer 'yes' or 'no'. Transparent means that attenuation in the chosen substrate material is taken into account. If 'no' is chosen, the mirror can only reflect neutrons, non-reflected neutrons are absorbed (as in guide walls)."<<endl;
	cin>>helper;
      }
      if(helper=="yes" || helper=="y")
	TransparentMirr.push_back(true);
      else 
	TransparentMirr.push_back(false);
      AngleMirr.push_back( GetDouble("Mirror inclinations w.r.t. x-axis (deg): ") );
      LengthMirr.push_back( GetDouble("Mirror lengths (cm): ") ); 
      cout<<"Distance between mirrors (cm): ";
      cin>>DistMirr;
      if( !HorPlane.at(0) )
	CenterYMirr.push_back( GetDouble("Give smallest y-coordinate of mirror centre (cm): ") );
      else
	CenterYMirr.push_back( GetDouble("Give smallest z-coordinate of mirror centre (cm): ") );
      double mNumber=GetDouble("Give mirror coatings m: ");
      while( mNumber>7 || mNumber<1 )
	mNumber=GetDouble("  Please choose m-number between 1 and 7: ") ;
      CoatingMirr.push_back( mNumber );
      Name.push_back("mirr1");
      CenterMirr.push_back( LengthMirr.at(0)/2 );
      if(TransparentMirr.at(0))
	DMirr.push_back( GetDouble("Mirror substrate thickness (cm): ") );
      else
	DMirr.push_back( 10 );
      for(int i=1; i<Nmirr; i++){
	HorPlane.push_back( HorPlane.at(0) );
	TransparentMirr.push_back( TransparentMirr.at(0) );
	AngleMirr.push_back( AngleMirr.at(0) );
	LengthMirr.push_back( LengthMirr.at(0) );
	CenterMirr.push_back( LengthMirr.at(i)/2 );
	CenterYMirr.push_back( CenterYMirr.at(i-1)+DistMirr );
	CoatingMirr.push_back( CoatingMirr.at(0) );
	DMirr.push_back( DMirr.at(0) );
	stringstream ss; ss<<(i+1); string NameIndex=ss.str();
	Name.push_back( "mirr"+NameIndex );
      }
    }
    else {
      for(int i=0; i<Nmirr; i++){
	if(Nmirr>1)cout<<" Parameters for "<<i+1<<". mirror:"<<endl;
	cout<<"Mirror plane (h,v):";
	cin>>helper;
	while( helper!="h" && helper!="v" ){
	  cout<<"ERROR: Please choose horizontal (h) or vertical (v) direction as mirror plane: ";
	  cin>>helper;
	}
	HorPlane.push_back(helper=="h");
	
	cout<<"Transparent mirror? (yes/no): ";
	cin>>helper;
	while(helper!="yes" && helper!="y" && helper=="no" && helper=="n"){
	  cout<<"ERROR: please answer 'yes' or 'no'. Transparent means that attenuation in the chosen substrate material is taken into account. If 'no' is chosen, the mirror can only reflect neutrons, non-reflected neutrons are absorbed (as in guide walls)."<<endl;
	  cin>>helper;
	}
	if(helper=="yes" || helper=="y")
	  TransparentMirr.push_back(true);
	else 
	  TransparentMirr.push_back(false);
	AngleMirr.push_back( GetDouble("Mirror inclination w.r.t. x-axis (deg): ") );
	LengthMirr.push_back( GetDouble("Mirror length (cm): ") );
	CenterMirr.push_back( GetDouble("Mirror centre in x (cm): ") );
	if( !HorPlane.at(i) )
	  CenterYMirr.push_back( GetDouble("Mirror centre in y (cm): ") );
	else
	  CenterYMirr.push_back( GetDouble("Mirror centre in z (cm): ") );
	double mNumber=GetDouble("Mirror coating (m-number): ");
	while( mNumber>7 || mNumber<1 )
	  mNumber=GetDouble("  Please choose m-number between 1 and 7: ") ;
	CoatingMirr.push_back( mNumber );
	if(TransparentMirr.at(i))
	  DMirr.push_back( GetDouble("Mirror substrate thickness (cm): ") );
	else
	  DMirr.push_back( 10 );
	stringstream ss; ss<<(i+1); string NameIndex=ss.str();
	Name.push_back( "mirr"+NameIndex );
      }
    }
    simpleShapeMirr=GetString("Are the mirror plates rectangular? (y/n) ");
    while( simpleShapeMirr!="y" && simpleShapeMirr!="n" ){
      simpleShapeMirr=GetString("ERROR: please choose yes ('y') or no ('n'): ");
    }
    if (simpleShapeMirr=="y"){
      Width1Mirr=GetDouble("Mirror extension perpendicular to x direction (cm): ");
      Width2Mirr=Width1Mirr;
    }
    else {
      Width1Mirr=GetDouble("1. width of mirror plates at smaller x (cm): ");
      Width2Mirr=GetDouble("2. width of mirror plates at larger x (cm): ");
    }
    GetChar(fileName, "Name of the file: ");
  }
  else{
    cout<<"What did you start this tool for?"<<endl;
    return 1;
  }
  
  
  ///////////////////////////////////////////////////////////////////////
  // sort/calculate parameters:  
  ///////////////////////////////////////////////////////////////////////
   
  for(int i=0; i<Nmirr; i++){
    if( !HorPlane.at(i) ){
      Y1.push_back( LengthMirr.at(i)/(2*cos(AngleMirr.at(i)*PI/180)) );
      Z1.push_back( Width1Mirr/2 );
      Y2.push_back( -Y1.at(i) );
      Z2.push_back( Width2Mirr/2 );
      Y3.push_back( -Y1.at(i) );
      Z3.push_back( -Z2.at(i) );
      Y4.push_back( Y1.at(i) );
      Z4.push_back( -Z1.at(i) );
    }
    else {
      Y1.push_back( Width1Mirr/2 );
      Z1.push_back( LengthMirr.at(i)/(2*cos(AngleMirr.at(i)*PI/180)) );
      Y2.push_back( -Y1.at(i) );
      Z2.push_back( Z1.at(i) );
      Y3.push_back( -Width2Mirr/2 );
      Z3.push_back( -Z2.at(i) );
      Y4.push_back( -Y3.at(i) );
      Z4.push_back( -Z1.at(i) );
    }
    AngleHV.push_back( 90-AngleMirr.at(i) );
    ThetaCSM.push_back( CoatingMirr.at(i)*ThetaC );
    if(!newFileFormat && TransparentMirr.at(i)==false){
      Mud.push_back( 100 ); 
      MuInc.push_back( 100 );
    }
    else{
      Mud.push_back( Mu_Si*DMirr.at(i) ); 
      MuInc.push_back( MuInc_Si*DMirr.at(i) );
    }
  
    double mNumber=CoatingMirr.at(i)*10;
    if((int)mNumber%10 == 0) //integer
      Rcsm.push_back( Rav[(int)CoatingMirr.at(i)-1] );
    else{ //extrapolate
      int m0=floor(CoatingMirr.at(i));
      if(m0<1)
	Rcsm.push_back( Rav[0] );
      else if(m0>=7)
	Rcsm.push_back( 0 );
      else
	Rcsm.push_back( Rav[m0-1]-(Rav[m0-1]-Rav[m0])*(CoatingMirr.at(i)-m0) );
    }
  }


  ///////////////////////////////////////////////////////////////////////
  // write sm_ensemble input file::  
  ///////////////////////////////////////////////////////////////////////
  
  pFullFileName = FullParName(fileName);
  pFile = fopen(pFullFileName, "w");
  
  if (pFile!=NULL){
    fprintf(pFile,"Input file for sm_ensemble, generated with Tool 'Generate Extraction System' v1.1: ");
    
    if(newFileFormat)
      fprintf(pFile,"new file format \n\n on   y1     z1     y2     z2      y3     z3     y4      z4      X      Y      Z    H/°   V/°    h/°   v/°   d/cm    m_up    m_down   Name \n");
    else
      fprintf(pFile,"old file format \n\n on   y1     z1     y2     z2      y3     z3     y4      z4      X      Y      Z    H/°   V/°    h/°   v/° Up: th_c th_csm  R_csm  µ*d   µ_inc*d  Down: th_c th_csm  R_csm   µ*d  µ_inc*d   Name \n");

    for(int i=0; i<Nmirr; i++){
      const char* ctypeName=Name.at(i).c_str();
      int MirrorUsage = (TransparentMirr.at(i)) ? 1 : 2;
      if( !HorPlane.at(i) ){
	if(newFileFormat)
	  fprintf(pFile, "%d  %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f  0.000  %3.2f  0.00  0.00  0.00   %3.3f %3.2f  %3.2f  -- %s -- \n", MirrorUsage, Y1.at(i), Z1.at(i), Y2.at(i), Z2.at(i), Y3.at(i), Z3.at(i), Y4.at(i), Z4.at(i), CenterMirr.at(i), CenterYMirr.at(i),AngleHV.at(i),DMirr.at(i),CoatingMirr.at(i),CoatingMirr.at(i),ctypeName);
	else
	  fprintf(pFile, "%d  %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f  0.000  %3.2f  0.00  0.00  0.00    %1.5f %1.5f %1.2f %3.5f %3.5f   %1.5f %1.5f %1.2f %3.5f %3.5f  -- %s -- \n", MirrorUsage, Y1.at(i), Z1.at(i), Y2.at(i), Z2.at(i), Y3.at(i), Z3.at(i), Y4.at(i), Z4.at(i), CenterMirr.at(i), CenterYMirr.at(i),AngleHV.at(i),ThetaC,ThetaCSM.at(i),Rcsm.at(i),Mud.at(i),MuInc.at(i),ThetaC,ThetaCSM.at(i),Rcsm.at(i),Mud.at(i),MuInc.at(i),ctypeName);
      }
      else {
	if(newFileFormat)
	  fprintf(pFile, "%d  %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f  0.000  %3.3f  0.00  %3.2f 0.00  0.00   %3.3f %3.2f  %3.2f  -- %s -- \n", MirrorUsage, Y1.at(i), Z1.at(i), Y2.at(i), Z2.at(i), Y3.at(i), Z3.at(i), Y4.at(i), Z4.at(i), CenterMirr.at(i), CenterYMirr.at(i),AngleHV.at(i),DMirr.at(i),CoatingMirr.at(i),CoatingMirr.at(i),ctypeName);
	else
	  fprintf(pFile, "%d  %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f  0.000  %3.3f  0.00  %3.2f 0.00  0.00    %1.5f %1.5f %1.2f %3.5f %3.5f   %1.5f %1.5f %1.2f %3.5f %3.5f  -- %s -- \n", MirrorUsage, Y1.at(i), Z1.at(i), Y2.at(i), Z2.at(i), Y3.at(i), Z3.at(i), Y4.at(i), Z4.at(i), CenterMirr.at(i), CenterYMirr.at(i),AngleHV.at(i),ThetaC,ThetaCSM.at(i),Rcsm.at(i),Mud.at(i),MuInc.at(i),ThetaC,ThetaCSM.at(i),Rcsm.at(i),Mud.at(i),MuInc.at(i),ctypeName);
      }
    }
    printf("\nOutput file has been generated: (%s)", pFullFileName);
    fclose(pFile);
  }
  else {
    printf("\nERROR: Output file could not be generated\n(%s)", pFullFileName);
  }
  
  cout<<"\n Type 'exit' to terminate: "<<endl;
  while (true){
    string c="";
    cin>>c;
    if (c=="exit") 
      break; 
    cout<<"You entered '"<<c<<"', type 'exit' to terminate:"<<endl;
  } 

  return 1;
}


double GetDouble(const char* pText)
{
  double dValue;
  printf("%s ", pText);
  scanf ("%lf", &dValue);

  return dValue;
}

string GetString(const char* pText)
{
  string sValue;
  printf("%s ", pText);
  cin >> sValue;

  return sValue;
}

 
void GetChar(char* pString, const char* pText)
{
  printf("%s ", pText);
  scanf ("%s", pString);
}
 
