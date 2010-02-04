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

#ifndef RND_SIMPLE
 #include <gsl/gsl_rng.h>
 gsl_rng * vit_gsl_rng;
#endif

FILE* LogFilePtr;        /* pointer to the log file stream              */


/****************************************************************************************/
/*  Conversion between physical properties                                              */
/****************************************************************************************/

double ENERGY_FROM_LAMBDA(double x)
{
	return(81805.048 / x / x);   /* [Ang]   -> [ueV] */
}

double LAMBDA_FROM_ENERGY(double x)
{
	return(sqrt(81805.048 / x)); /* [ueV]   -> [Ang] */
}

double ENERGY_FROM_V(double x)
{
	return(0.5227033 * x * x);   /* [cm/ms] -> [ueV] */
}

double V_FROM_LAMBDA(double x)
{
	return(395.60346 / x);       /* [Ang]   -> [cm/ms] */
}

double LAMBDA_FROM_V(double x)
{
	return(395.60346 / x);       /* [cm/ms] -> [Ang] */
}

/****************************************************************************************/
/*  Random Functions                                                                    */
/****************************************************************************************/


double MonteCarlo(double x, double y)
{
   return (y - x)*Vran() + x;
}


/****************************************************************************************/
/*  General Functions                                                                   */
/****************************************************************************************/

/* computes square of a real value */

double sq(double Value)
{
	return Value * Value ;
}



/* calculates atan2 in the range (0, 2*M_PI) */

double atan0(double a, double b)
{
	if (b > 0.)
	  return (double) atan(a / b) ;

	if (b == 0.)
	  return M_PI_2 ;

	return (double) atan(a / b) + M_PI ;
}

/* rounds a value mathematically  */

double Round(double value)
{
	return floor(value + 0.5);
}


double RoundP(double value, int decimal)
{
	double f = pow(10, decimal);
	return Round(value * f) / f;
}


/* minimum and maximum of two double or long values */

long mini(long value1, long value2)
{
	if(value1 < value2) return value1 ;
	else                return value2 ;
}

long maxi(long value1, long value2)
{
	if(value1 > value2) return value1 ;
	else                return value2 ;
}

double Min(double value1, double value2)
{
	if(value1 < value2) return value1 ;
	else                return value2 ;
}

double Max(double value1, double value2)
{
	if(value1 > value2) return value1 ;
	else                return value2 ;
}



/* swap two values */

void Exchange(double* pValue1, double* pValue2)
{
	double dHelp;

	dHelp    = *pValue1 ;
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

double AngleVectors(VectorType v1, VectorType v2)
{
  double theta ;

  theta = ScalarProduct(v1, v2) / (double)sqrt(ScalarProduct(v1, v1)) / (double)sqrt(ScalarProduct(v2, v2)) ;
  return 180./M_PI * (double) acos(theta) ;
}

/* area of triangle from two vectors, G.Zs */


double Area(VectorType v1, VectorType v2)
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
		TempVec[i]=ScalarProduct(RotMatrix[i],Vector);
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
void FillRMatrixZY(double RotMatrix[3][3], double roty, double rotz)
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


/****************************************************************************************/
/*  General I/O Functions                                                               */
/****************************************************************************************/

/* fileOpen open file 'name' and gives pointer back
   in case of an opening error, a message is written to the LogFile */

FILE * fileOpen(const char *name, char *mode)
{
	FILE *f=NULL;

	f = fopen(name, mode);
	if (f==NULL)
	{	fprintf(LogFilePtr, "ERROR: Can't open %s!\n", name);
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



/****************************************************************************************/
/*  Functions for Reading of Input Data                                                 */
/****************************************************************************************/

/* ReadLine reads next line from file 'pFile' into string 'pLine' that is
     not empty and not a comment line (beginning with #)
	  returning TRUE if line is found and FALSE otherwise
   it strips comments at the end, leading and succeeding blanks, line feeds, tabs anc cr
   the maximal number of characters in the string must be given in 'nStrLen'
*/
#ifdef VERS26
int
ReadLine(FILE* pFile, char* pLine, int nStrLen)
{
	char *pComment;
	short k, kmax;

	strcpy(pLine, "");
	if (pFile!=NULL)
	{
		while(strlen(pLine)==0  && !feof(pFile))
		{
			fgets (pLine, nStrLen, pFile);

			/* delete line feeds, tabs and carriage returns */
			kmax = (short) strlen(pLine);
			for (k=0; k < kmax; k++)
			{	if (pLine[k]=='\n' || pLine[k]=='\t' || pLine[k]=='\r')
					pLine[k]=' ';
			}
			/* strip the comments and leading and succeeding blanks */
			pComment = strchr(pLine, '#');
			if (pComment != NULL)
				*pComment = '\0';
			while (pLine[0]==' ')
			{	StrgLShift(pLine,1);
			}
			while (pLine[strlen(pLine)-1]==' ')
			{	pLine[strlen(pLine)-1]='\0';
			}
		}
	}
	if (strlen(pLine) > 0)
		return TRUE;
	else
		return FALSE;
}
#else
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

#endif


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
  char comment[100], *c;
  c = fgets(comment, 100, fpt);
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


#ifdef VERS26
/* Shift string 'sStr' 'kWidth' bytes to the left */
void
StrgLShift(char* sStr, int kWidth)
{
	int k, ke;

	ke = strlen(sStr) - kWidth;

	for (k=0; k <= ke; k++)
		sStr[k] = sStr[k+kWidth];
}
#endif

/* Scan string 'sStr' and copy all values (but maximally 'nMax')
   to list 'pTab' of double values,  beginning with value number 'nStart'*/
long
StrgScanLF(const char* sStr, double* pTab, const int nMax, const int nStart)
{
	int    k, n=0;
	char   *pStr, sNumber[31];

	pStr = (char*) sStr;
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

