#ifndef MATHMATRIX_H
#define MATHMATRIX_H

/********************************************************************************************/
/*  VITESS module 'mathmatrix.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Aug 2011  D. Nekrassov  initial version                                              */
/********************************************************************************************/

// Matrix class: Add, subtract, multiply  3x3 matrices. Muliply a matrix by a 3-row vector.
#include <string>

using namespace std;

#include "mathvector.h"

class MathMatrix {

 public:

  double entries[3][3];

  MathMatrix();
  MathMatrix(double array[9]);
  MathMatrix(double matrix[3][3]);
  MathMatrix(double alpha, double beta, double gamma, string order = "xyz");
  virtual ~MathMatrix() {  };

  static MathMatrix* RotMatrixXFromVector(MathVector* v);
  MathMatrix* Transpose();
  void transpose();
  void swap(int i, int j);

  friend MathMatrix operator + (const MathMatrix& m1, const MathMatrix& m2);
  friend MathMatrix operator - (const MathMatrix& m1, const MathMatrix& m2);
  friend MathVector operator * (const MathMatrix& m1, const MathVector& v1);
  friend MathMatrix operator * (const MathMatrix& m1, const MathMatrix& m2);

  MathMatrix& operator += (MathMatrix const & m);
  MathMatrix& operator -= (MathMatrix const & m);
  
};


inline MathMatrix operator + (const MathMatrix& m1, const MathMatrix& m2) 
{
  double result[9] = {m1.entries[0][0] + m2.entries[0][0], m1.entries[0][1] + m2.entries[0][1], m1.entries[0][2] + m2.entries[0][2],  
		      m1.entries[1][0] + m2.entries[1][0], m1.entries[1][1] + m2.entries[1][1], m1.entries[1][2] + m2.entries[1][2],
                      m1.entries[2][0] + m2.entries[2][0], m1.entries[2][1] + m2.entries[2][1], m1.entries[2][2] + m2.entries[2][2]};
  return MathMatrix(result);
}

inline MathMatrix& MathMatrix::operator += (MathMatrix const &m) 
{
  entries[0][0] += m.entries[0][0]; entries[0][1] += m.entries[0][1]; entries[0][2] += m.entries[0][2];  
  entries[1][0] += m.entries[1][0]; entries[1][1] += m.entries[1][1]; entries[1][2] += m.entries[1][2];
  entries[2][0] += m.entries[2][0]; entries[2][1] += m.entries[2][1]; entries[2][2] += m.entries[2][2];
  return *this;
}

inline MathMatrix operator - (const MathMatrix& m1, const MathMatrix& m2) 
{
  double result[9] = {m1.entries[0][0] - m2.entries[0][0], m1.entries[0][1] - m2.entries[0][1], m1.entries[0][2] - m2.entries[0][2],  
		      m1.entries[1][0] - m2.entries[1][0], m1.entries[1][1] - m2.entries[1][1], m1.entries[1][2] - m2.entries[1][2],
                      m1.entries[2][0] - m2.entries[2][0], m1.entries[2][1] - m2.entries[2][1], m1.entries[2][2] - m2.entries[2][2]};
  return MathMatrix(result);
}

inline MathMatrix& MathMatrix::operator -= (MathMatrix const &m) 
{
  entries[0][0] -= m.entries[0][0]; entries[0][1] -= m.entries[0][1]; entries[0][2] -= m.entries[0][2];  
  entries[1][0] -= m.entries[1][0]; entries[1][1] -= m.entries[1][1]; entries[1][2] -= m.entries[1][2];
  entries[2][0] -= m.entries[2][0]; entries[2][1] -= m.entries[2][1]; entries[2][2] -= m.entries[2][2];
  return *this;
}


#endif
