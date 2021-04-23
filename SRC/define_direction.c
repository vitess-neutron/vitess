/*******************************************************************************************/
/* Tool DefineDirection:                                                                   */
/*   Converts between direction definitions in VITESS                                      */
/*                                                                                         */
/* The free non-commercial use of these routines is granted provided due credit is given   */
/* to the authors.                                                                         */
/*                                                                                         */
/* 1.0  Jul 2002  G. Zsigmond    initial version                                           */
/* 1.1  Mar 2020  K. Lieutenant  tidy up                                                   */
/*                                                                                         */
/* IMPORTANT NOTE   "Higher index" rotation convention:                                    */
/*   Positive rotation of frame means rotation of one positive axis                        */
/*   towards a higher index positive axis :       +X->+Y, +Y->+Z, +X->+Z                   */
/*******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "matrix.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
#define PI       3.14159265358979323846
#define DEG2RAD 57.29577951308


/******************************/
/** Prototypes               **/
/******************************/
static int    GetChar (const char* s);  // Reads string from stdin    
static double GetDbl  (const char* s);  // Reads double value from stdin    


/*********************************/
/** Global and Static Variables **/
/*********************************/

char   cMode, sBuffer[129];
int    i;
double theta, phi, angZ, angY, rotangX, rotangY, rotangZ, dir[3], dLength, MX[3][3], MY[3][3], MZ[3][3];


/******************************/
/** Program                  **/
/******************************/
int main(int argc, char **argv)
{
  _eModule=MCN_TOOL_DEF_DIR;
newdef:
  do
  {	
    cMode=GetChar("DEFINE DIRECTION \n\n"
                  "1) Converts between direction definitions in VITESS: \n\n"
                  "Cartesian: X = main propagation axis, \n"
                  "           Y = horizontal axis (left positive), \n" 
                  "           Z = vertical axis (upward positive). \n"
                  "Spherical: Theta = Angle with the +X axis,\n"
                  "           Phi   = Angle of the YZ projection with the +Y axis. \n"
                  "Euler:     How to transform vector into (1 0 0 )? \n"
                  "           Rotates +X into chosen direction i.e. +X''= direction. \n"
                  "           Alpha =  First frame rotation angle around the Z axis,\n"
                  "           Beta  =  Second frame rotation angle around the new Y axis. \n\n"
                  "2) Performs Euler frame rotations as next option.\n\n"
                  "IMPORTANT: Positive rotation of frame means rotation of one positive \n"
                  "           axis towards a higher index positive axis : +X->+Y, +Y->+Z, +X->+Z                 \n"
                  "\nWhich definition is given?                   Quit [q] \n"
                  "Cartesian    [c] \n"
                  "Spherical    [s] \n"
                  "Euler        [e] \n");
  } 
  while (cMode!='s' && cMode!='e' && cMode!='c'&& cMode!='q');

  if(cMode == 'q') goto fin;

  if(cMode == 'c')
  {
    dir[0] = GetDbl("\nX component             / [-] : ");
    dir[1] = GetDbl("Y component             / [-] : ");
    dir[2] = GetDbl("Z component             / [-] : ");

    dLength = LengthVector(dir);
    if(dLength==0.){printf("\n\n\n           ERROR! Vector length zero!\n\n\n"); goto newdef;}
    if(dLength!=1.) 
    {
      printf("\nVector length normalized to 1 by %f .\n",dLength) ;	
      for(i=0;i<3;i++) dir[i] *= 1./dLength ;
      printf("\nCartesian coordinates: \n"
      "X    : %f [-] \n"
      "Y    : %f [-] \n"
      "Z    : %f [-] \n", dir[0], dir[1], dir[2]);
    }

    CartesianToSpherical(dir, &theta, &phi);
    CartesianToEulerZY(dir, &angY,  &angZ);

    printf("\nSpherical coordinates: \n"
    "Theta: %f deg\n"
    "Phi  : %f deg\n",  DEG2RAD*theta, DEG2RAD*phi);
    printf("\nEuler coordinates: \n"
    "Alpha: %f deg\n"
    "Beta : %f deg\n",  DEG2RAD*angZ, DEG2RAD*angY);
  }

  if(cMode == 's')
  {
    theta = GetDbl("\nAngle with X axis                  / deg : ")/DEG2RAD;
    phi   = GetDbl("Angle of YZ projection with Y axis / deg : ")/DEG2RAD;

    SphericalToCartesian(dir, &theta, &phi);
    CartesianToEulerZY(dir, &angY,  &angZ);

    printf("\nEuler coordinates: \n"
    "Alpha: %f deg\n"
    "Beta : %f deg\n",  DEG2RAD*angZ, DEG2RAD*angY);
    printf("\nCartesian coordinates: \n"
    "X    : %f [-] \n"
    "Y    : %f [-] \n"
    "Z    : %f [-] \n", dir[0], dir[1], dir[2]);
  }

  if(cMode == 'e')
  {
    angZ = GetDbl("\nRotation angle around Z     / deg : ")/DEG2RAD;
    angY = GetDbl("Rotation angle around new Y / deg : ")/DEG2RAD;

    EulerToCartesianZY  (dir, &angY, &angZ);
    CartesianToSpherical(dir, &theta, &phi);

    printf("\nSpherical coordinates: \n"
    "Theta: %f deg\n"
    "Phi  : %f deg\n",  DEG2RAD*theta, DEG2RAD*phi);
    printf("\nCartesian coordinates: \n"
    "X    : %f [-] \n"
    "Y    : %f [-] \n"
    "Z    : %f [-] \n", dir[0], dir[1], dir[2]);
  }

  do 
  { 
    cMode=GetChar("\nNew definition? [d]   Rotation? [r]   Quit? [q]\n");
  } 
  while (cMode!='d' && cMode!='r' && cMode!='q');	

  if(cMode == 'd') goto newdef; 
  if(cMode == 'q') goto fin; 

  newrot:;
  do
  {	
    cMode=GetChar("\nGive sequence of Euler rotations about X, Y, Z:\n"
    "\nXYZ [1]        YZX [3]        ZXY [5] \n"
    "XZY [2]        YXZ [4]        ZYX [6]\n");
  } 
  while (cMode!='1' && cMode!='2' && cMode!='3'&& cMode!='4' && cMode!='5'&& cMode!='6');

	
  rotangX = GetDbl("\nGive rotation angle about X axis / deg : ")/DEG2RAD;
  rotangY = GetDbl("Give rotation angle about Y axis / deg : ")/DEG2RAD;
  rotangZ = GetDbl("Give rotation angle about Z axis / deg : ")/DEG2RAD;

  FillRotMatrixZ(MZ, rotangZ) ;
  FillRotMatrixY(MY, rotangY) ;
  FillRotMatrixX(MX, rotangX) ;

  if(cMode == '1') {RotVector(MX, dir); 	  RotVector(MY, dir); 	  RotVector(MZ, dir);}
  if(cMode == '2') {RotVector(MX, dir); 	  RotVector(MZ, dir); 	  RotVector(MY, dir);}
  if(cMode == '3') {RotVector(MY, dir); 	  RotVector(MZ, dir); 	  RotVector(MX, dir);}
  if(cMode == '4') {RotVector(MY, dir); 	  RotVector(MX, dir); 	  RotVector(MZ, dir);}
  if(cMode == '5') {RotVector(MZ, dir); 	  RotVector(MX, dir); 	  RotVector(MY, dir);}
  if(cMode == '6') {RotVector(MZ, dir); 	  RotVector(MY, dir); 	  RotVector(MX, dir);}


  /* gives output */
  dLength = LengthVector(dir);
  if(dLength!=1.) {for(i=0;i<3;i++) dir[i] *= 1./dLength ;}

  CartesianToSpherical(dir, &theta, &phi);
  CartesianToEulerZY(dir, &angY,  &angZ);

  printf("\nEuler coordinates: \n"
  "Alpha: %f deg\n"
  "Beta : %f deg\n",  DEG2RAD*angZ, DEG2RAD*angY);
  printf("\nSpherical coordinates: \n"
  "Theta: %f deg\n"
  "Phi  : %f deg\n",  DEG2RAD*theta, DEG2RAD*phi);
  printf("\nCartesian coordinates: \n"
  "X    : %f [-] \n"
  "Y    : %f [-] \n"
  "Z    : %f [-] \n", dir[0], dir[1], dir[2]);

  do 
  {
    cMode=GetChar("\nNew definition? [d]   New rotation? [r]   Quit? [q]\n");
  } 
  while (cMode!='d' && cMode!='r' && cMode!='q');	

  if(cMode == 'd') goto newdef; 
  if(cMode == 'r') goto newrot; 

  fin:;
  return 0;
}


/*******************************************************/
/** Reads different types of parameters from stdin    **/
/**   GetChar:   Reads string from stdin              **/
/**   GetDbl :   Reads double value from stdin        **/
/*******************************************************/
static
int GetChar (const char *s) 
{
  printf(s);
  fgets(sBuffer, 128, stdin);
  return sBuffer[0];
}

static
double GetDbl (const char *s) 
{
  printf(s);
  fgets(sBuffer, 128, stdin);   
  return atof(sBuffer);
}


