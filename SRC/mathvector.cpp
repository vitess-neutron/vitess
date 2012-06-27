
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

  double X = 0;
  double Y = 0;
  double Z = 0;

  // Rotation around the x-axis
  X = x[0];
  Y = cos(alphaX)*x[1] - sin(alphaX)*x[2];
  Z = sin(alphaX)*x[1] + cos(alphaX)*x[2];

  // Rotation around the y-axis
  X = cos(alphaY)*X + sin(alphaY)*Z;
  Y = Y;
  Z = (-1.)*sin(alphaY)*X + cos(alphaY)*Z;

   // Rotation around the z-axis
  X = cos(alphaZ)*X - sin(alphaZ)*Y;
  Y = sin(alphaZ)*X + cos(alphaZ)*Y;
  Z = Z;


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
