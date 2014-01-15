#ifndef MATHFUNCTIONS_CPP
#define MATHFUNCTIONS_CPP
/********************************************************************************************/
/*  VITESS module 'mathfunctions.cpp'                                                       */
/*    functions written in C++ for various VITESS modules                                   */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Daniiel Nekrassov, Carolin Zendler, Michael Fromme, Klaus Lieutenant, Andreas Houben     */
/*                                                                                          */
/* 1.0  D. Nekrassov: Oct 2012,  initial version                                             */
/********************************************************************************************/


extern "C" {
#include "general.h"
#include "init.h"
}

#include "mathfunctions.h"


// extern FILE*    LogFilePtr;     /* stream to which things are logged */



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
// parameters: w1 - entrance width in cm
//             w2 - exit width in cm
//             length - length of ellipse in m!!!
//             dist - distance from exit to focal point in m!!!
//return values: longAxis and shortAxis are returned in m!
//               startPoint and endPoint are x coordinates for the entrance and exit of the ellipse 
//               in a coordinate system, of which the origin is in the center of the ellipse.
short CalculateEllipseParametersFromStartAndExitWidths(double w1, double w2, double length, double dist, 
		double &longAxis, double &shortAxis, double& startPoint, double &endPoint)
{

  double y1 = w1/200.;
  double y2 = w2/200.;

  double solutions[4];

  //  fprintf(LogFilePtr,"Check: %f %f %f \n", y1, y2, (y2 - y1)/y2);

  bool solutionFound = false;

  if (fabs((y2 - y1)/y2) < 1e-4) {

    double x = length/2.;
    double focalPoint = x + dist;

    double longAxis_2 = (0.5)*(y1*y1 + focalPoint*focalPoint +x*x) + sqrt(pow(y1*y1 + focalPoint*focalPoint + x*x, 2)/4. - x*x*focalPoint*focalPoint);
    longAxis = sqrt(longAxis_2);

    shortAxis = sqrt(y1*y1/(1. - x*x/(longAxis*longAxis)));
    
    double distDeviation1 = fabs((sqrt(longAxis*longAxis - shortAxis*shortAxis) - x) - dist) / dist;

    //    fprintf(LogFilePtr,"Check: %f %f %f  \n", longAxis, shortAxis, distDeviation1);

    if (distDeviation1 > 1e-2) {

      longAxis_2 = (0.5)*(y1*y1 + focalPoint*focalPoint + x*x) - sqrt(pow(y1*y1 + focalPoint*focalPoint + x*x, 2)/4. - x*x*focalPoint*focalPoint);
      double longAxisTemp = sqrt(longAxis_2);
      double shortAxisTemp = sqrt(y1*y1/(1. - x*x/(longAxis*longAxis)));

      double distDeviation2 = fabs((sqrt(longAxisTemp*longAxisTemp - shortAxisTemp*shortAxisTemp) - x) - dist) / dist;

      if (distDeviation2 < 1e-2) {
	longAxis = longAxisTemp;
	shortAxis = shortAxisTemp;
      }
      else {
	fprintf(LogFilePtr,"WARNING: Check ellipse parameters for correctness!\n");
      }
      
    }

    //     fprintf(LogFilePtr,"Check: %f %f \n", longAxis, shortAxis);

    startPoint = (-1.)*x;
    endPoint = x;

    solutionFound = true;
    
  }
  else {

    double y_t = y2*y2 - y1*y1;
    double A = length*length - y_t;
    double D_t = dist*dist - y2*y2;
    
    double e = y_t*y_t - 4.*dist*length*y_t - 4.*y2*y2*length*length;
    
    double a = 4.*dist*y_t*y_t - y_t*(2.*D_t*length + 8.*dist*dist*length - 2.*dist*A) - y2*y2*(8.*length*length*dist - 4.*length*A);
    double b = 6.*dist*dist*y_t*y_t - y_t*(4.*length*dist*D_t + 4.*dist*dist*dist*length - D_t*A  - 4.*dist*dist*A) - y2*y2*(A*A - 8.*dist*length*A + 4.*length*length*dist*dist);
    double c = 4.*y_t*y_t*dist*dist*dist - y_t*(2.*dist*dist*D_t*length - 2.*dist*D_t*A - 2.*dist*dist*dist*A) - y2*y2*(2.*dist*A*A - 4.*length*A*dist*dist);
    double d = y_t*y_t *dist*dist*dist*dist + dist*dist*D_t*A*y_t - y2*y2*dist*dist*A*A;
    
    a /= e;
    b /= e;
    c /= e;
    d /= e;
    
    SolveQuarticEquation(a, b, c, d, &solutions[0],false);
  

    for (int i = 0; i < 4; i++) {

      if (solutions[i] < -6665) continue;
    
      //      fprintf(LogFilePtr,"Solution Nr. %d: %f \n", i, solutions[i]);

      double x2 = solutions[i];
      double x1 = solutions[i] - length;

      double shortAxisTemp = sqrt( (x1*x1/(x2*x2)*y2*y2 - y1*y1) / (x1*x1/(x2*x2) - 1.));
      //      fprintf(LogFilePtr,"Short axis: %f \n", shortAxisTemp);
      if (shortAxisTemp != shortAxisTemp) continue;

      double longAxisTemp = sqrt(x1*x1)/sqrt(1. - y1*y1/(shortAxisTemp*shortAxisTemp));
      //      fprintf(LogFilePtr,"Long axis: %f \n", longAxisTemp);
      if (longAxisTemp != longAxisTemp) continue;

      double y1_calc = shortAxisTemp*sqrt(1. - x1*x1/(longAxisTemp*longAxisTemp));

      if (fabs(y1 - y1_calc)/y1_calc > 1e-4) continue;

      if ((fabs((sqrt(longAxisTemp*longAxisTemp - shortAxisTemp*shortAxisTemp) - x2) - dist) / dist) > 1e-4) continue;

      longAxis = longAxisTemp;
      shortAxis = shortAxisTemp;

      startPoint = x1;
      endPoint = x2;

      solutionFound = true;
      break;

    }
  }
  return solutionFound;

}


double CalculateEllipsePoint(double x, double longAxis, double shortAxis, double sign)
{

  double y = shortAxis*sqrt(1. - x*x/(longAxis*longAxis))*sign;
  return y;

}


extern "C" short C_CalculateEllipseParameters(double w1, double w2, double length, double dist, 
		double* longAxis, double* shortAxis, double* startPoint, double* endPoint)
{
  return CalculateEllipseParametersFromStartAndExitWidths(w1, w2, length, dist, *longAxis, *shortAxis, *startPoint, *endPoint);    
}
// Script assumes following equation: x^4 + a*x^3 + b*x^2 + c*x + d = 0
// double solutions*: pointer to array with 4 doubles.
void SolveQuarticEquation(double a, double b, double c, double d, double* solutions, short switchSign)
{

  double precision = 1e-15;

  //  if (switchSign) fprintf(LogFilePtr,"Sign switched! \n");

// substitution: x = y - a/4 
  // it follows: y^4 + alpha*y² + beta*y + gamma = 0 
  double alpha = (-1.)*(3./8.)*a*a + b;
  double beta = pow(a, 3)/8. - a*b/2. + c;
  double gamma = (-1.)*3.*pow(a, 4)/256. + a*a*b/16. - a*c/4. + d;

  // solution by introducing the cubic resolvent:
  // z³ - 2*alpha*z² + (alpha²-4gamma)z + beta² = 0 --> z³ - r*z² + sz + t
  double r = -2.*alpha;
  double s = (alpha*alpha - 4.*gamma);
  double t = beta*beta;

  // substitution z = y - r/3 
  // it follows: y³ + py + q = 0
  double p = (s - pow(r, 2)/3.);
  double q = (2.*pow(r, 3)/27. - r*s/3. + t);

  double Rvalue = pow(q/2., 2) + pow(p/3., 3);

  //  cout << "R=" << Rvalue << endl;

  double y1 = 0;
  double y2 = 0;
  double y3 = 0;
  double y2_i = 0;
  double y3_i = 0;

  //  bool Rpositive = true;

  // If Rvalue > 0 --> 1 real and 2 complex solutions
  // If Tvalue == 0 --> 2 real solutions
  // Solution by Cardano/Tartaglia
  if (Rvalue >= 0) {

    double Tvalue = sqrt(Rvalue);

    double u = pow(pow((-1.)*q/2. + Tvalue, 2), 1./6.);
    double v = pow(pow((-1.)*q/2. - Tvalue, 2), 1./6.);

    if (((-1.)*q/2. + Tvalue) < 0) u*=(-1.);
    if (((-1.)*q/2. - Tvalue) < 0) v*=(-1.);

    // cout << "u=" << u << "v=" << v << endl;

    y1 = u + v;
    y2 = (-1.)*(u+v)/2.;
    y3 = (-1.)*(u+v)/2.;

    // complex parts of solutions 2 & 3
    y2_i = (-1.)*((u-v)/2.)*sqrt(3.);
    y3_i = ((u-v)/2.)*sqrt(3.);

  }

  // If Rvalue < 0 --> 3 real solutions
  // "casus irreducibilis"
  else {
    
    //    Rpositive = false;

    double u = sqrt((-1.)*pow(p/3., 3));
    double w = acos((-1.)*q/(2.*u));

    // cout << "u=" << u << " cos(w)=" << cos(w) << endl;

    y1 =  2.*pow(pow(u, 2), 1./6.)* cos(w/3.);
    if (u < 0) y1*=(-1.);
    y2 =  2.*pow(pow(u, 2), 1./6.)* cos(w/3. + M_PI*2./3.);
    if (u < 0) y2*=(-1.);
    y3 =  2.*pow(pow(u, 2), 1./6.)* cos(w/3. + M_PI*4./3.);
    if (u < 0) y3*=(-1.);

    //    cout << "y1: " << y1 << " y2: " << y2 << " y3: " << y3 << endl;

    // Sort by value, lowest first
    if (y1 > y2) {
      double yTemp = y2;
      y2 = y1;
      y1 = yTemp;
    }

    if (y2 > y3) {
      double yTemp = y3;
      y3 = y2;
      y2 = yTemp;
    }

    if (y1 > y2) {
      double yTemp = y2;
      y2 = y1;
      y1 = yTemp;
    }

    //    cout << "y1: " << y1 << " y2: " << y2 << " y3: " << y3 << endl;

  }

  // reverse substitution z = y - r/3 
  double z1 = y1 - r/3.;
  double z2 = y2 - r/3.;
  double z2_i = y2_i;
  double z3 = y3 - r/3.;
  double z3_i = y3_i;

  // prepare to calculate sqrt(-z_1,2,3), take care of complex numbers
  z1*=-1.;
  z2*=-1.;
  z3*=-1.;
  z2_i*=-1.;
  z3_i*=-1.;

  double z2_phi = acos(z2/sqrt(z2_i*z2_i + z2*z2));
  if (z2_i < 0) z2_phi*=(-1.);
  double z3_phi = acos(z3/sqrt(z3_i*z3_i + z3*z3));
  if (z3_i < 0) z3_phi*=(-1.);

  //  cout << "phi2 " << z2_phi << " phi3 " << z3_phi << endl;

  double z2_sqrt = cos(z2_phi/2.)*sqrt(sqrt(z2_i*z2_i + z2*z2));
  double z2_sqrt_i = sin(z2_phi/2.)*sqrt(sqrt(z2_i*z2_i + z2*z2));

  double z3_sqrt = cos(z3_phi/2.)*sqrt(sqrt(z3_i*z3_i + z3*z3));
  double z3_sqrt_i = sin(z3_phi/2.)*sqrt(sqrt(z3_i*z3_i + z3*z3));

  double z1_sqrt = 0;
  double z1_sqrt_i = 0;

  if (z1 > 0) z1_sqrt = sqrt(z1);
  else z1_sqrt_i = sqrt((-1.)*z1);

  //  cout << "z1: " << z1_sqrt << " z2: " << z2_sqrt << " +i*" << z2_sqrt_i << " z3: " << z3_sqrt << " +i*" << z3_sqrt_i << endl;

  // Signs of the sqrt(z_1,2,3) must be chosen the way that sqrt(-z_1)*sqrt(-z_2)*sqrt(--z_3) = q (or -q for Rvalue < 0)
  double sign1 = 1.;
  double sign2 = 1.;
  double sign3 = -1.;

  //  if ((z2_sqrt_i*z3_sqrt_i) < 0) sign3 = -1.;

  if (beta < 0) {
      
    sign1 = -1.;
      
  }

  else {

    // if (!Rpositive) sign1 = -1.;

  }
  if (switchSign) sign1 *= -1.;
 
  // if ((Rpositive && ((z1_sqrt*sign1*z2_sqrt*sign2*z3_sqrt*sign3)/q < 0)) || (!Rpositive && (((z1_sqrt*sign1*z2_sqrt*sign2*z3_sqrt*sign3)/q > 0) ))) {
  //   sign1 *= -1.;
  //   fprintf(LogFilePtr,"Sign chanded, was wrong!");
  // }
    //    cout << "-q = " << beta*(-1.) << "  q from sqroots: " << z1_sqrt*sign1*(z2_sqrt*z3_sqrt*sign2*sign3 - z2_sqrt_i*z3_sqrt_i*sign2*sign3) << endl;
    //    cout << sign1 << "  " << sign2 << "  " << sign3 << endl;
    
    // y1 = (sqrt(-z_1) + sqrt(-z_2) + sqrt(-z_3))/2, beware of complex terms
  double y1_tilde = (z1_sqrt*sign1 + z2_sqrt*sign2 + z3_sqrt*sign3)/2.;
  double y1_tilde_i = (z1_sqrt_i*sign1 + z2_sqrt_i*sign2 + z3_sqrt_i*sign3)/2.;

   // y2 = (sqrt(-z_1) - sqrt(-z_2) - sqrt(-z_3))/2, beware of complex terms
  double y2_tilde = (z1_sqrt*sign1 - z2_sqrt*sign2 - z3_sqrt*sign3)/2.;
  double y2_tilde_i = (z1_sqrt_i*sign1 - z2_sqrt_i*sign2 - z3_sqrt_i*sign3)/2.;
    
   // y3 = (-sqrt(-z_1) + sqrt(-z_2) - sqrt(-z_3))/2, beware of complex terms
  double y3_tilde = ((-1.)*z1_sqrt*sign1 + z2_sqrt*sign2 - z3_sqrt*sign3)/2.;
  double y3_tilde_i = ((-1.)*z1_sqrt_i*sign1 + z2_sqrt_i*sign2 - z3_sqrt_i*sign3)/2.;
    
  // y4 = (-sqrt(-z_1) - sqrt(-z_2) + sqrt(-z_3))/2, beware of complex terms
  double y4_tilde = ((-1.)*z1_sqrt*sign1 - z2_sqrt*sign2 + z3_sqrt*sign3)/2.;
  double y4_tilde_i = ((-1.)*z1_sqrt_i*sign1 - z2_sqrt_i*sign2 + z3_sqrt_i*sign3)/2.;
    
  // reverse the substitution:  x = y - a/4 
  double x1 = y1_tilde - a/4.;
  double x2 = y2_tilde - a/4.;    
  double x3 = y3_tilde - a/4.;
  double x4 = y4_tilde - a/4.;


  // cout << x1 << " + i*" << y1_tilde_i << "  " << x2 << " + i*" << y2_tilde_i << "  "
  //      << x3 << " + i*" << y3_tilde_i << "  " << x4 << " + i*" << y4_tilde_i << endl;

  // // Check *real* solutions!
  // if ( y1_tilde_i == 0) CheckSolution(x1, a, b, c, d);
  // if ( y2_tilde_i == 0) CheckSolution(x2, a, b, c, d);
  // if ( y3_tilde_i == 0) CheckSolution(x3, a, b, c, d);
  // if ( y4_tilde_i == 0) CheckSolution(x4, a, b, c, d);

  //  bool solutionFound = false;

  if (solutions != 0) {

    if ( fabs(y1_tilde_i) < precision) {
      solutions[0] = x1;
      //      solutionFound = true;
    }
    else solutions[0] = -6666;
    if ( fabs(y2_tilde_i) < precision) {
      solutions[1] = x2;
      //      solutionFound = true;
    }
    else solutions[1] = -6666;
    if ( fabs(y3_tilde_i) < precision) {
      solutions[2] = x3;
      //       solutionFound = true;
    }
    else solutions[2] = -6666;
    if ( fabs(y4_tilde_i) < precision) {
      solutions[3] = x4;
      //       solutionFound = true;
    }
    else solutions[3] = -6666;
    
    //    return &solutions[0];

  }

  return;

}

double CheckSolution(double x, double a, double b, double c, double d)
{

  double y = pow(x, 4) + a*pow(x,3) + b*pow(x, 2) + c*x + d;

  if (fabs(y) > 1e-4) {

    double newX = ImprovePrecision(x, y, a, b, c, d);
    //    y = pow(newX, 4) + a*pow(newX, 3) + b*pow(newX, 2) + c*newX + d;
    return newX;
  }
  else {
    return x;
  }
}

double ImprovePrecision(double x, double y, double a, double b, double c, double d)
{

  double diff = 1.;
  double lastDiff = y;
  double lastX = x;
  double lastXWithDifferentSign = x;
  double factor = 1.;
  bool useDeltaX = true;

  int j = 0;

  while (j < 50) {

    double deltaX = x*0.0000001*factor;
    if (useDeltaX) x += deltaX;

    diff = pow(x, 4) + a*pow(x,3) + b*pow(x, 2) + c*x + d;

    if (fabs(diff) < 1e-4) break;

    if (useDeltaX) {
      if ((diff/lastDiff) > 0 && fabs(diff) < fabs(lastDiff)) {
	
	factor *= 2.;
	lastDiff = diff;
	
      }
      else if ((diff/lastDiff) > 0 && fabs(diff) > fabs(lastDiff)) {
	
	factor *= -2.;
	lastDiff = diff;
	
      }
      else if ((diff/lastDiff) < 0) {
	
	lastXWithDifferentSign = lastX;
	lastX = x;
	x = (x + lastXWithDifferentSign)/2.;
	useDeltaX = false;
	lastDiff = diff;
	
      }
    }
    else {

      if ((diff/lastDiff) < 0) {

	lastXWithDifferentSign = lastX;
	lastX = x;
	x = (x + lastXWithDifferentSign)/2.;
	lastDiff = diff;

      }
      else {

	lastX = x;
	x = (x + lastXWithDifferentSign)/2.;
	lastDiff = diff;

      }

    }

    j++;

  }

  return x;

}

double RandomLorentzian(double mean, double gamma)
{
  double rval, displ;
  rval = 2.*MonteCarlo(0, 1) - 1;
  displ = 0.5*gamma*tan(rval*M_PI_2);

  return (mean+displ);
}

#endif
