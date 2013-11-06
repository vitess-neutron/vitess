
#include "mathvector.h"


MathVector::MathVector()
{

  x[0] = 0;
  x[1] = 0;
  x[2] = 0;

}


MathVector::MathVector(double x0, double y0, double z0)
{
  x[0] = x0;
  x[1] = y0;
  x[2] = z0;
  
}


MathVector MathVector::Rotate(double alphaX, double alphaY, double alphaZ)
{

  double X, Y, Z, sinV, cosV;

  // Rotation around the x-axis
  sinV = sin(alphaX);
  cosV = cos(alphaX);
  X = x[0];
  Y = cosV*x[1] - sinV*x[2];
  Z = sinV*x[1] + cosV*x[2];

  // Rotation around the y-axis
  sinV = sin(alphaY);
  cosV = cos(alphaY);
  X = cosV*X + sinV*Z;
  // Y = Y;
  Z = - sinV*X + cosV*Z;

   // Rotation around the z-axis
  sinV = sin(alphaZ);
  cosV = cos(alphaZ);
  X = cosV*X - sinV*Y;
  Y = sinV*X + cosV*Y;
  // Z = Z;

  return MathVector(X, Y, Z);

}



double MathVector::Mod()
{
  
  return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);

}

MathVector MathVector::Unit()
{

  double mod = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);

  return MathVector(x[0]/mod, x[1]/mod, x[2]/mod);

}


double MathVector::Phi()
{
  
  if (x[0] == 0) {
    if (x[1] == 0) return 0;
    else if (x[1] > 0) return 3.14159265/2;
    else return (-1.)*3.14159265/2;
  }
  
  else return atan(x[1]/x[0]);

}


double MathVector::Theta()
{

  if (x[2] == 0) return 3.14159265/2;

  else return atan(sqrt(x[0]*x[0] + x[1]*x[1])/x[2]);

}


void MathVector::Array(double* arr)
{

  arr[0] = x[0];
  arr[1] = x[1];
  arr[2] = x[2];

  return;

}
