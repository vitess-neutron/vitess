#ifndef MATHMATRIX_CPP
#define MATHMATRIX_CPP

#include <iostream>

#include "mathmatrix.h"



MathMatrix::MathMatrix()
{

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {

      entries[i][j] = 0;

    }
  }

}


MathMatrix::MathMatrix(double array[9])
{

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {

      entries[i][j] = array[3*i + j];

    }
  }

}


MathMatrix::MathMatrix(double matrix[3][3])
{
  for (int i = 0; i < 3; i++) 
  {
    for (int j = 0; j < 3; j++) 
    {
      entries[i][j] = matrix[i][j];
    }
  }
}

MathMatrix::MathMatrix(double alpha, double beta, double gamma, string order)
{

  double calpha = cos(alpha);
  double salpha = sin(alpha);

  double cbeta = cos(beta);
  double sbeta = sin(beta);

  double cgamma = cos(gamma);
  double sgamma = sin(gamma);

  MathMatrix temp1;

  temp1.entries[0][0] = 1;
  temp1.entries[1][1] = calpha;
  temp1.entries[1][2] = -1.*salpha;
  temp1.entries[2][1] = salpha;
  temp1.entries[2][2] = calpha;

  MathMatrix temp2;

  temp2.entries[0][0] =  cbeta;
  temp2.entries[2][0] =  -1.*sbeta;
  temp2.entries[0][2] =  sbeta;
  temp2.entries[2][2] =  cbeta;
  temp2.entries[1][1] =  1;

  MathMatrix temp3;

  temp3.entries[0][0] =  cgamma;
  temp3.entries[0][1] =  -1.*sgamma;
  temp3.entries[1][0] =  sgamma;
  temp3.entries[1][1] =  cgamma;
  temp3.entries[2][2] =  1;

  MathMatrix* tempPointer[3];

  if (order.substr(0, 1) == "x") tempPointer[0]=&temp1;
  else if (order.substr(0, 1) == "y") tempPointer[0]=&temp2;
  else tempPointer[0]=&temp3;

  if (order.substr(1, 1) == "x") tempPointer[1]=&temp1;
  else if (order.substr(1, 1) == "y") tempPointer[1]=&temp2;
  else tempPointer[1]=&temp3;

  if (order.substr(2, 1) == "x") tempPointer[2]=&temp1;
  else if (order.substr(2, 1) == "y") tempPointer[2]=&temp2;
  else tempPointer[2]=&temp3;

  if (tempPointer[0] == tempPointer[1] || tempPointer[1] == tempPointer[2] || tempPointer[0] == tempPointer[2]) {

    for (int i = 0; i < 9; i++) entries[i/3][i%3]=0;
    std::cerr << "Wrong rotation order given: " << order << std::endl;

  }

  else {

    temp1 = (*tempPointer[0])*(*tempPointer[1])*(*tempPointer[2]);
    *this = temp1;
    
  }

}

MathVector operator * (const MathMatrix& m1, const MathVector& v1) 
{

  double result[3];
  
  for (int i = 0; i < 3; i++) {
    
    result[i] = m1.entries[i][0]*v1.x[0] + m1.entries[i][1]*v1.x[1] + m1.entries[i][2]*v1.x[2];

  }

  return MathVector(result[0], result[1], result[2]);

}


MathMatrix operator * (const MathMatrix& m1, const MathMatrix& m2) 
{

  double result[9];
  
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {

      result[i*3 + j] = m1.entries[i][0]*m2.entries[0][j] + m1.entries[i][1]*m2.entries[1][j] + m1.entries[i][2]*m2.entries[2][j];

    }
  }

  return MathMatrix(result);

}

// Computes a rotation matrix to rotate the coordinate system such that 
// the x-axis coincides with the x-component of the vector v
MathMatrix* MathMatrix::RotMatrixXFromVector(MathVector* v)
{

  double phi = v->Phi(); // rotation angle around z-axis
  double theta = v->Theta(); // here: rotation angle around y-axis

  MathMatrix* rotMatrix = new MathMatrix(0, 3.14159265/2. - theta, (-1.)*phi, "xyz");
    
  return rotMatrix;
}


MathMatrix* MathMatrix::Transpose()
{

  MathMatrix* transMatrix = new MathMatrix();

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
    
      transMatrix->entries[j][i] = entries[i][j];

    }
  }

  return transMatrix;
}

/*
void MathMatrix::transpose()
{
  int i,j;
  MathMatrix transMatrix;

  for (int i = 0; i < 3; i++) 
  { for (int j = 0; j < 3; j++) 
    {
      transMatrix.entries[j][i] = entries[i][j];
    }
  }

  for (int i = 0; i < 3; i++) 
  { for (int j = 0; j < 3; j++) 
    {
      entries[i][j] = transMatrix.entries[i][j];
    }
  }

  return;
}*/

void MathMatrix::transpose()
{
  swap(0,1);
  swap(0,2);
  swap(1,2);
  return;
}

void MathMatrix::swap(int i, int j)
{
  double    Aij=entries[i][j]; 
  entries[i][j]=entries[j][i]; 
  entries[j][i]=Aij;  

  return;
}


#endif
