#ifndef MATHVECTOR_H
#define MATHVECTOR_H

/********************************************************************************************/
/*  VITESS module 'mathvector.h'                                                            */
/*                                                                                          */
/* The free non-commercial use of these routines is granted                                 */
/* providing due credit is given to the authors.                                            */
/* 1.0 Jul 2011  D. Nekrassov  initial version                                              */
/********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// This class describes a mathematical three component vector
class MathVector {

 public:

  double x[3];
    
  MathVector();
  MathVector(double x, double y, double z=0);
  virtual ~MathVector() { };
  
  friend MathVector operator + (const MathVector& v1, const MathVector& v2);
  friend MathVector operator - (const MathVector& v1, const MathVector& v2);
  friend double operator * (const MathVector& v1, const MathVector& v2);
  friend MathVector operator * (const MathVector& v, const double c);
  friend double operator / (const MathVector& v1, const MathVector& v2);
  friend MathVector operator / (const MathVector& v, const double c);

  MathVector& operator += (MathVector const & v);
  MathVector& operator -= (MathVector const & v);
  MathVector& operator *= (const double c);
  MathVector& operator /= (const double c);

  friend bool operator == (const MathVector& v1, const MathVector& v2);

  MathVector Rotate(double alphaX=0, double alphaY=0, double alphaZ=0);

  double Mod();
  MathVector Unit();
  double Phi();
  double Theta();

  void Array(double* arr);


};



inline MathVector operator + (const MathVector& v1, const MathVector& v2) {return MathVector(v1.x[0] + v2.x[0], v1.x[1] + v2.x[1], v1.x[2] + v2.x[2]);}

inline MathVector operator - (const MathVector& v1, const MathVector& v2) {return MathVector(v1.x[0] - v2.x[0], v1.x[1] - v2.x[1], v1.x[2] - v2.x[2]);}

inline MathVector operator * (const MathVector& v, const double c) {return MathVector(v.x[0]*c, v.x[1]*c, v.x[2]*c);}
inline double operator * (const MathVector& v1, const MathVector& v2)  {return (v1.x[0]*v2.x[0] + v1.x[1]*v2.x[1] + v1.x[2]*v2.x[2]);}

inline MathVector operator / (const MathVector& v, const double c) {return MathVector(v.x[0]/c, v.x[1]/c, v.x[2]/c);}
inline double operator / (const MathVector& v1, const MathVector& v2)  {return (v1.x[0]/v2.x[0] + v1.x[1]/v2.x[1] + v1.x[2]/v2.x[2]);}

inline MathVector& MathVector::operator += (MathVector const & v) {x[0] += v.x[0]; x[1] += v.x[1]; x[2] += v.x[2]; return *this;}

inline MathVector& MathVector::operator -= (MathVector const & v) {x[0] -= v.x[0]; x[1] -= v.x[1]; x[2] -= v.x[2]; return *this;}

inline MathVector& MathVector::operator *= (const double c) {x[0] *= c; x[1] *= c; x[2] *= c; return *this;}

inline MathVector& MathVector::operator /= (const double c) {x[0] /= c; x[1] /= c; x[2] /= c; return *this;}

inline bool operator == (const MathVector& v1, const MathVector& v2) {return (v1.x[0] == v2.x[0] && v1.x[1] == v2.x[1] && v1.x[2] == v2.x[2]);}


#endif
