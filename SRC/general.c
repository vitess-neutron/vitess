/********************************************************************************************/
/*  VITESS module 'general.c'                                                               */
/*    Elementary functions for all VITESS modules                                           */
/*                                                                                          */
/* The free non-commercial use of these routines is granted providing due credit is given   */
/* to the authors:                                                                          */
/* Friedrich Streffer, Géza Zsigmond, Dietmar Wechsler,                                     */
/* Michael Fromme, Klaus Lieutenant, Sergey Manoshin                                        */
/*                                                                                          */
/* Change: K.L.  2002 JAN, reorganized routines                                             */
/* Change: G.Zs. 2002 JUL, new routines                                                     */
/* Change: K.L.  2003 JAN, new functions 'ReadLine', 'StrgLShift', and 'StrgCopy'           */
/* Change: K.L.  2003 FEB, definitions of 'idum' and 'LogFilePtr' from init to general      */
/* Change: K.L.  2003 MAR, new function 'StrgScanLF', additional parameter in 'ReadLine'    */
/* Change: M.F.  2005 DEC, random number generators from GNU GSL                            */
/* Change: A.H.  2009 OCT, new routine: RoundP for rounding after given decimal position    */

#include "general.h"
#include "ctype.h"
#include "time.h"

#ifndef RND_SIMPLE
 #include "gsl/gsl_rng.h"
 gsl_rng * vit_gsl_rng;
#endif


#ifdef VT_WINDOWS
 char cSl = '\\';
#else
 char cSl = '/';
#endif

double gsl_ran_gaussian (const gsl_rng * r, const double sigma);

FILE* LogFilePtr;        /* pointer to the log file stream              */


/****************************************************************************************/
/*  Conversion between physical properties                                              */
/****************************************************************************************/

double ENERGY_FROM_LAMBDA(const double x)
{
	return(81805.048 / x / x);   /* [Ang]   -> [ueV] */
}

double LAMBDA_FROM_ENERGY(const double e)
{
	return(sqrt(81805.048 / e)); /* [ueV]   -> [Ang] */
}

double ENERGY_FROM_V(const double v)
{
	return(0.5227033 * v * v);   /* [cm/ms] -> [ueV] */
}

double V_FROM_ENERGY(const double e)
{
	return(sqrt(e / 0.5227033)); /* [ueV] -> [cm/ms] */
}

double LAMBDA_FROM_V(const double x)
{
	return(395.60346 / x);       /* [cm/ms] -> [Ang] */
}

double V_FROM_LAMBDA(const double x)
{
	return(395.60346 / x);       /* [Ang]   -> [cm/ms] */
}


/****************************************************************************************/
/*  Random Functions                                                                    */
/****************************************************************************************/

/* uniformly distributed random numbers in [x,y] */
double MonteCarlo(const double x, const double y)
{
   return (y - x)*Vran() + x;
}

/* random numbers of Gaussian distribution with standard deviation 'Sigma' around 'Center' */
double DistrGauss(double Center, double Sigma)
{
  return Center + gsl_ran_gaussian(vit_gsl_rng, Sigma);
}


/****************************************************************************************/
/*  General Functions                                                                   */
/****************************************************************************************/

/* computes square of a real value */

double sq(const double Value)
{
	return Value * Value ;
}


/* calculates atan2 in the range (0, 2*M_PI) */

double atan0(const double a, const double b)
{
	if (b > 0.)
	  return (double) atan(a / b) ;

	if (b == 0.)
	  return M_PI_2 ;

	return (double) atan(a / b) + M_PI ;
}

/* rounds a value mathematically  */

double Round(const double value)
{
	return floor(value + 0.5);
}


double RoundP(const double value, const int decimal)
{
	double f = pow(10.0, decimal);
	return Round(value * f) / f;
}


/* minimum and maximum of two double or long values */

long mini(const long value1, const long value2)
{
  return value1 < value2 ? value1 : value2;
}

long maxi(const long value1, const long value2)
{
  return value1 > value2 ? value1 : value2;
}

double Min(const double value1, const double value2)
{
  return value1 < value2 ? value1 : value2;
}

double Max(const double value1, const double value2)
{
  return value1 > value2 ? value1 : value2;
}


/* swap two values */

void Exchange(double* pValue1, double* pValue2)
{
  double dHelp;

  dHelp    = *pValue1;
  *pValue1 = *pValue2;
  *pValue2 = dHelp;
}


/* Calculation of solid angle from horizontal and vertical opening angle */
/*                                         */
/* dHorAngle : horizontal angle in radians */
/* dVertAngle: horizontal angle in radians */
/*                                         */
double SolidAngle(const double dHorAngle, const double dVertAngle)
{

  if (dVertAngle < 0.55)
    /* solution for small angles: Omega = 2 phi * 2(tan(theta)-tan³(theta)/3) */
    return 4 * dHorAngle  * (tan(dVertAngle) - pow(tan(dVertAngle),3)/3.0);
	
  if (dHorAngle < 0.55)
    /* solution for small angles: Omega = 2(tan(phi)-tan³(phi)/3) * 2 theta */
    return 4 * dVertAngle * (tan(dHorAngle)  - pow(tan(dHorAngle),3)/3.0);

  /* empirical approximation for large angles */
  return 4 * sqrt(dHorAngle * sin(dHorAngle) * dVertAngle * sin(dVertAngle));

}


// Calculation of reflectivity on supermirrors from wavelength and inclination angle
// either following quadratic SwissNeutronics description by Henrik Jacobsen (ReflSN)
// or using any reflectivity file (ReflFile)
//
// Lambda: wavelength        [Ang]
// Angle : inclination angle [deg]
// M     : official m value of the supermirror    (ReflSN only)
// Rdata : pointer to list of reflectivity values (ReflFile only)
//
double ReflSN(const double Lambda,    const double Angle,    const double M)
{
  double S,T, 
    M2,            // m'     : 'real' m value
    Q,             // Q      : momentum transfer of the reflection
    Qc,            // Q_c    : crit. momentum transfer 
    QcNi  =0.0217, // Q_c,Ni : crit. momentum transfer of nickel
    R0    =0.99,   // R_0    : reflectivity for 0 <= Q <= Q_c
    alphaQ=0.0,    //          slope Delta_R / Delta_Q
    betaQ =0.0,    //          quadratic term to describe R(q)
    W,             // W      : width of the cut-off  [1/Ang]
    R;             // R      : reflectivity

  Qc = QcNi*Min(M, 1.0);
  Q  = 4*M_PI*sin(M_PI/180.0*Angle)/Lambda;

  if (Q <= Qc)
  { R = R0;
  }
  else
  { 
    if (M <= 1.0)
    { R=0.0;
    }
    else
    {
      W  = 0.0022 - 0.0002*M;
      M2 = M*0.9853 + 0.1978;

      if (M > 3.0)
      { alphaQ =  5.0944 + 0.1204*M;
        betaQ  = 68.1137 - 7.6251*M;
      }
      else
      { alphaQ = M;
        betaQ  = 0.0;
      }
      T = 0.5 * (1.0 - tanh((Q - M2*QcNi) / W));
      S = (1.0 - alphaQ * (Q-Qc) + betaQ * sq(Q-Qc));
	    R = R0 * T * S ;
    }
  }
  return(R);
}


double ReflInterpol(const double Lambda, const double Angle, const double* Rdata, long MaxData)
{
  long   iw1;
  double w,         // angle/wavelength
         R=0.0;     // reflectivity

  w   = Angle*1000.0 / Lambda;
  iw1 = (long) floor(w);

  if ((iw1+1) < MaxData)
    R = Rdata[iw1] + (Rdata[iw1+1] - Rdata[iw1]) * (w - iw1);

  return(R);
}


/****************************************************************************************/
/*  Vector Functions                                                                    */
/****************************************************************************************/

/* 'Copy' copies the contents of Vector 'Src' to vector 'Dest'  */
/*                                                    */
void CopyVector(const VectorType Src, VectorType Dest)
{
    Dest[0] = Src[0];
    Dest[1] = Src[1];
    Dest[2] = Src[2];
}


/* 'MAXV' returns the number of the largest component of 'Vector': 0, 1 or 2  */
/*                                                                            */
long MAXV(const VectorType Vector)
{
  if( (fabs(Vector[0]) > fabs(Vector[1])) && (fabs(Vector[0]) > fabs(Vector[2])))
    return 0;
  if(fabs(Vector[1]) > fabs(Vector[2]))
    return 1;
  return 2;
}


/* 'LengthVector' returns the length of vector 'Vec'  */
/*                                                    */
double LengthVector(const VectorType Vec)
{
  //return sqrt(ScalarProduct(Vec,Vec));

  return sqrt(Vec[0]*Vec[0] + Vec[1]*Vec[1] + Vec[2]*Vec[2]);

}


/* 'NormVector' changes the vector length to 1  */
/*                                              */
short NormVector(VectorType Vector)
{
  long   i;
  double dLen = LengthVector(Vector);

  if (dLen==0.0)
    return FALSE;

  for(i=0;i<3;i++)
    Vector[i] /= dLen;

  return TRUE;
}


/* 'DistVector' calculates the distance between the points described by Vec1 and Vec2  */
/*                                                                                     */
double DistVector(const VectorType Vec1, const VectorType Vec2)
{
  VectorType Vhlp;
  
  CopyVector(Vec1, Vhlp) ;
  SubVector (Vhlp, Vec2);
  return LengthVector(Vhlp);
}


/* 'AddVector' adds 'Add' to 'Value' and returns 'Value'  */
/*                                                        */
void AddVector(VectorType Value, const VectorType Add)
{
  int i ;
  for (i=0;i<3;i++)
    Value[i] += Add[i] ;
}


/* 'SubVector' Substracts 'Sub' from 'Value' and returns 'Value' */
/*                                                             */
void SubVector(VectorType Value, const VectorType Sub)
{
  int i ;
  for(i=0;i<3;i++)
    Value[i] -=  Sub[i];
}


/* 'MultiplyByScalar' multiplies a vector by a scalar */
/*                                                    */
void MultiplyByScalar(VectorType Vector, const double Scalar)
{
  int i;
  for (i=0;i<3;i++)
    Vector[i] *= Scalar;
}


/* 'ScalarProduct' calculates the scalar product of two vectors 'v1' and 'v2' */
/*                                                                            */
double ScalarProduct(const VectorType v1, const VectorType v2)
{
  return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

/* angle between two vectors in degs */

double AngleVectors(const VectorType v1,  const VectorType v2)
{
  double theta;

  theta = ScalarProduct(v1, v2) / sqrt(ScalarProduct(v1, v1) * ScalarProduct(v2, v2)) ;
  return 180./M_PI * acos(theta) ;
}

/* area of triangle from two vectors, G.Zs */


double Area(const VectorType v1, const VectorType v2)
{
  double lv = LengthVector(v1) * LengthVector(v2);
  return lv * fabs(sin(acos( ScalarProduct(v1, v2) / lv)) /2.);

  //return LengthVector(v1) * LengthVector(v2) *
  //		fabs(sin(acos( ScalarProduct(v1, v2)/(LengthVector(v1) * LengthVector(v2)))) /2.);

}


/* 'RotVector' does essentially a Vector times matrix multiplication   */
/* in order to rotate the Vector. The rotation Matrix may be supplied  */
/* by e.g. 'RotMatrixX'                                                */
/* Author: F. Streffer.                                                */
void RotVector(double RotMatrix[3][3], VectorType Vector)
{
  VectorType TempVec;
  int        i;

  for(i=0;i<3;i++)
    TempVec[i] = ScalarProduct(RotMatrix[i],Vector);
  CopyVector(TempVec, Vector);
}

/* 'RotBackVector' rotates a vector, by multiplication of the Vector  */
/* with the invers of RotMatrix. E.g if RotMatrix is the same as in   */
/* 'RotVector' and is applied to the result of RotVector the original */
/* vector is restored.   (Remember det(RotMatrix)=1)                  */
/* Author: F. Streffer.                                               */
void RotBackVector(double RotMatrix[3][3], VectorType Vector)
{
  VectorType TempVec;
  int        i;

  for(i=0;i<3;i++)
    TempVec[i]=RotMatrix[0][i]*Vector[0]+RotMatrix[1][i]*Vector[1]+RotMatrix[2][i]*Vector[2];
  CopyVector(TempVec, Vector);
}


/* 'FillRMatrixZY' calculates a rotation matrix, which rotates a frame   */
/* at first about the z-axis by 'rotz' and then about the y-axis by 'roty' */
/*  Author: F. Streffer.                                                   */
/*  Change: G. Zs. 16 JUL 2002  rotation convention                        */
void FillRMatrixZY(double RotMatrix[3][3], const double roty, const double rotz)
{
  double sz, cz, sy, cy;
  long   i,j;

  sy= (double) sin(roty);
  cy= (double) cos(roty);
  sz= (double) sin(rotz);
  cz= (double) cos(rotz);

  /* now, fill the matrix */
  RotMatrix[0][0] =  cy*cz;
  RotMatrix[0][1] =  cy*sz;
  RotMatrix[0][2] =  sy;
  RotMatrix[1][0] = -sz;
  RotMatrix[1][1] =  cz;
  RotMatrix[1][2] =  0.0;
  RotMatrix[2][0] = -sy*cz;
  RotMatrix[2][1] = -sy*sz;
  RotMatrix[2][2] =  cy;

  /* cut off very small matrix elements */
  for(i=0; i<3; i++)
    for(j=0; j<3; j++)
      if(fabs(RotMatrix[i][j]) < 1e-12) RotMatrix[i][j] = 0.0;
}


/* 'CartesianToEulerZY' calculates Euler angles 'rotz' and 'roty'         */
/* to transfer the x-axis to 'Vector' by rotation around y- and z-axis ZY */
/* (cf. FillRotMatrixZY)                                                  */
/*  Author: G. Zsigmond                                                   */
void CartesianToEulerZY(VectorType Vector, double *roty, double *rotz)
{
  *rotz = (double) atan2( Vector[1] , Vector[0] ) ;

  *roty = (double) atan2( Vector[2] , ((double) cos(*rotz) * Vector[0] + (double) sin(*rotz) * Vector[1]) ) ;
}

/* Euler to cartesian - invers of previous                            */
/*  Author: G. Zsigmond                                               */

void EulerToCartesianZY(VectorType Vector, double *roty, double *rotz)
{
  double cos_roty = cos(*roty);
  Vector[0]= (double) cos_roty * (double) cos(*rotz) ;
  Vector[1]= (double) cos_roty * (double) sin(*rotz) ;
  Vector[2]= (double) sin(*roty) ;
}


/****************************************************************************************/
/*  General I/O Functions                                                               */
/****************************************************************************************/

/* fileOpen open file 'name' and gives pointer back
   in case of an opening error, a message is written to the LogFile */

FILE * fileOpen(const char *name, const char *mode)
{
  FILE *f;

  if (! (f = fopen(name, mode))) {
    fprintf(LogFilePtr, "ERROR: Can't open %s!\n", name);
    exit(-1);
  }
  return f;
}


void Error(const char *text)
{
  fprintf(LogFilePtr,"ERROR: %s!\n", text);
  exit(-1);
}


void Warning(const char *text)
{
  fprintf(LogFilePtr,"Warning: %s!\n", text);
}


void Abort()
{
  exit(-1);
}


/* Wait(time)
   remains 'time' sec in this function
*/
void Wait(float WaitTime)
{
  int   c1, c2;
  float DelT;  // time in sec

  c1=clock();

  do
  { c2=clock();
    DelT = ((float)(c2-c1))/CLOCKS_PER_SEC;
  }
  while (DelT < WaitTime);

  return;
}


/****************************************************************************************/
/*  Functions for Reading of Input Data                                                 */
/****************************************************************************************/

/* ReadLine reads next line from file 'pFile' into string 'pLine' that is
     not empty and not a comment line (beginning with #)
	  returning TRUE if line is found and FALSE otherwise
   it strips comments at the end, leading and succeeding blanks, line feeds, tabs anc cr
   the maximal number of characters in the string must be given in 'nStrLen'
*/

int
ReadLine(FILE* pFile, char* pLine, int nStrLen) {

  if (pFile)
    while (fgets (pLine, nStrLen, pFile)) {
      int v, k, kanf, kmax;

      /* substitute line feeds, tabs and carriage returns with blanks */
      for (k=0; (v = pLine[k]) && v != '#'; k++) {
	if (v=='\n' || v=='\t' || v=='\r')
	  pLine[k] = ' ';
      }
      if (k <= 0) continue;

      /* strip the comments and leading and succeeding blanks */
      for (kanf = 0; pLine[kanf] == ' '; kanf++) ;
      for (kmax = k-1; kmax >= kanf && pLine[kmax] == ' '; kmax--) ;
      if (kmax < kanf) continue;
      if (kanf == 0) {
	pLine[kmax+1] = 0;
      } else {
	for (k = 0; kanf <= kmax; k++, kanf++)
	  pLine[k] = pLine[kanf];
	pLine[k] = 0;
      }
      return TRUE;
    }

  *pLine = 0;
  return FALSE;
}


/*  ReadParString(FILE *fpt) reads one string value from parameter file */

void ReadParString(FILE *fpt, char *stringvar)
{
  fscanf(fpt,"%s", stringvar) ;
}


/*  ReadParF(FILE *fpt) reads one double value from parameter file */

double ReadParF(FILE *fpt)
{
  double value;
  return 1 == fscanf (fpt, "%lf", &value) ? value : 0.;
}


/*  ReadParI(FILE *fpt) reads one integer value from parameter file */

int ReadParI(FILE *fpt)
{
  int value;
  return fscanf(fpt,"%d", &value) == 1 ? value : 0;
}


/* ReadParComment(FILE *fpt) reads comment line */

void ReadParComment(FILE *fpt)
{
  char comment[100];
  fgets(comment, 100, fpt);
}


/**********************************************************/
/*  String Operations                                     */
/**********************************************************/

/* Copy 'nLen' bytes of 'sOrigin' into the new string 'sCopy' */
void
StrgCopy(char* sCopy, const char* sOrigin, int nLen)
{
	strncpy(sCopy, sOrigin, nLen);
	sCopy[nLen]='\0';
}


/* Shift string 'sStr' 'kWidth' bytes to the left */
void
StrgLShift(char* sStr, int kWidth)
{
	int k, ke;

	ke = strlen(sStr) - kWidth;

	for (k=0; k <= ke; k++)
		sStr[k] = sStr[k+kWidth];
}


/* Scan string 'sStr' and copy all values (but maximally 'nMax')
   to list 'pTab' of double values,  beginning with value number 'nStart'*/
long
StrgScanLF(const char* sStr, double* pTab, const int nMax, const int nStart)
{
	int k, n=0;
	const char *pStr;
	char sNumber[31];

	pStr = sStr;
	n   -= nStart;
	do
	{	/* search of beginning and end of 1st number of (remaining) string */
		k=0;
		/* step forward until first number or control character */
		while (isdigit(pStr[k])==0 && iscntrl(pStr[k])==0)
			k++;
		/* step forward until space-like or control character */
		while (isspace(pStr[k])==0 && iscntrl(pStr[k])==0)
			k++;

		/* separating first number and adding it to the list */
		if (k > 0)
		{
			StrgCopy(sNumber, pStr, k);
			if (n >= 0)
				pTab[n] = atof(sNumber);
			n++;
			pStr += k;
		}
	}
	while (n < nMax && k > 0);

	return(n);
}


/**************************************************************/
/* Change of the Slashes to the right ones, e.g. '\' to '/'   */
/**************************************************************/
void ChangeSlash(char* pStr)
{
	int k, klen;

	klen = strlen(pStr);
	for (k=0; k < klen; k++)
	{	if (pStr[k]=='/' || pStr[k]=='\\')
			pStr[k]=cSl;
	}
}
