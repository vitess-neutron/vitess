/* The free non-commercial use of these routines is granted */
/* providing due credit is given to the authors.            */
/* Author: Géza Zsigmond, last change JUL 2002              */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define	STRING_BUFFER 50


FILE *InputFilePtr;	
FILE *OutputFilePtr;	
char InputFileName[129], OutputFileName[129], yesno[2];    

void	ReadString(FILE *fpt, char stringvar[STRING_BUFFER]);
double	ReadF(FILE *fpt);
double	ReadE(FILE *fpt);
int		ReadI(FILE *fpt);
void	ReadComment(FILE *fpt);
double	sq(double Value);

long i, imax;
double xx[1000], yc[1000], xmean, x2mean, IntegralInt, sd, SD;



int main(int argc, char **argv)
{

	printf("Give output filename: \n"); scanf("%s", OutputFileName ) ; 

newcal:;

	if((OutputFilePtr = fopen(OutputFileName,"a"))==NULL) 
		{
		printf("Can't open %s for output!\n", OutputFileName);
		exit(-1);
		}

again:;
	printf("Give  input filename: \n"); scanf("%s", InputFileName ) ; 

	if((InputFilePtr=fopen(InputFileName,"r"))==NULL) 
		{
		printf("Can't open %s for input!\n", InputFileName);
		goto again;
		}/**/


	IntegralInt = 0.;
	xmean = 0. ;
	SD = 0;

	for(i=0;i<1001;i++)
	{
	
		if((xx[i] = ReadF(InputFilePtr))==0) goto stop_it1/**/; 

		imax = i;

		yc[i] = ReadE(InputFilePtr); 	ReadComment(InputFilePtr);
	
		xmean += xx[i] * yc[i] ; 

		IntegralInt += yc[i];

	}
	

stop_it1: ; printf(" \n");

		xmean *= 1./IntegralInt;

		for(i=0;i<(imax+1);i++)
		{
		
			
				SD += sq(xx[i] - xmean)* yc[i]/IntegralInt ; 

		}
		
			SD = (double) sqrt(SD);

			fprintf(OutputFilePtr, "%f   %f   %f\n", xmean, SD, IntegralInt);

		printf("mean,           SD,             SD^2,           integral\n%e   %e   %e   %e\n", xmean, SD, SD*SD, IntegralInt);

		fclose(InputFilePtr);
		fclose(OutputFilePtr);

		printf("\nNew calculation? [y/n]  \n"); scanf("%s", yesno ) ; 
		if(*yesno == 'y') goto newcal ;

}

/***********************************************************************/
/* some general procedures::


/*  ReadString(FILE *fpt) reads one string value from parameter file */

void ReadString(FILE *fpt, char stringvar[STRING_BUFFER])
{
	fscanf(fpt,"%s", stringvar ) ; 
	
	return ;
}


/*  ReadF(FILE *fpt) reads one double value from parameter file */

double ReadF(FILE *fpt)
{
double value ;
		value=0. ;
		fscanf(fpt,"%lf", &value ) ; 
		return value;
}

/*  ReadE(FILE *fpt) reads one double value from parameter file */

double ReadE(FILE *fpt)
{
double value ;
		value=0. ;
		fscanf(fpt,"%le", &value ) ; 
		return value;
}


/*  ReadI(FILE *fpt) reads one integer value from parameter file */

int ReadI(FILE *fpt)
{
int value ;
		value=0 ;
		fscanf(fpt,"%d", &value ) ; 
		return value;
}


/* ReadComment(FILE *fpt) reads comment line */

void ReadComment(FILE *fpt)
{
char comment[100], *c ;
c=fgets(comment, 100, fpt) ;
}


/* computes square of a real value */

double sq(double Value)
{
	return Value * Value ;
}



