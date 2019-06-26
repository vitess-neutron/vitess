#ifndef PYSHM_H
#define PYSHM_H

double pyshm(long nin, double q, double sig, double ro);
void   pyshmFileGen(char* text, long n, long nin, double q0, double dq, 
					double sig, double ro, FILE* pOut);

#endif
