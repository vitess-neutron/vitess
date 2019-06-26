/*********************************************************************************************/
/*  VITESS module EVAL_ELAST                                                                 */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0                            initial version                                            */
/* 1.1  Jan 2001  K. Lieutenant  time of evaluation                                          */
/* 1.2  Nov 2001  K. Lieutenant  SOFTABORT                                                   */
/* 1.3  Jan 2002  K. Lieutenant  Cleanup(), log. binning, Writing out of the neutron data    */
/* 1.4  Nov 2003  K. Lieutenant  evaluation dependent on colour                              */
/* 1.5  Jan 2004  K. Lieutenant  changes for 'instrument.dat'                                */
/* 1.6  Feb 2004  K. Lieutenant  'FullParName' + ERROR included; check of 'kind' out of loop */
/* 1.7  Nov 2005  K. Lieutenant  transformation direction -> scattering angles added         */
/* 1.7a Jun 2009  A. Houben      increased NCENTER from 100 to 200                           */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "init.h"
#include "general.h"
#include "matrix.h"
#include "softabort.h"

#define BINS   5000
#define NCENTER 200


/* globale variable */
int   probactiv=TRUE,        /* probactiv=1 means probabilities activated, 
                                else neutron weight is set to 1.0         */
      TOF = FALSE,           /* TRUE : time of flight instrument */
      deadspotactive=FALSE,  /* TRUE : deadspot exists */
      bExclCount =FALSE,     /* TRUE : only neutrons complying with the evaluate requirements
                                       are written to the output      */
      bLogBinning=FALSE;     /* TRUE : binning increases exponentially 
                                FALSE: linear binning                  */

long  nbins,                 /* number of bins */
      nColour,               /* colour necessary for the trajectory to be regarded
                                colour 0 means: all trajectories are regarded  */
      kind;                  /* 1= d-spacing; 2=momentum transfer q; 3=scattering angle */

double referenceWavelength,  /* reference Wavelength for crystal monochromator (or mechanical velocity
                                  selector) instrument                                                 */
       deadspotangle=0,      /* excludes all neutrons with a scattering angle < deadspotangle [deg] */
       Flightpath=0,         /* length of neutron flight path [cm] */
       TimeOffset=0,         /* global shift of the neutron time t= t-TimeOffset [ms] */
       m,M,                  /* lower and upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
       dLogProz=0.0,         /* percentage of increase to next bin      */
       dDelLambda,           /* difference between wavelength calculated from TOF and true wavelength  */
       dEvalTimeMin=-1.0e10, /* minimal and maximal time for evaluation */
       dEvalTimeMax= 1.0e10;


FILE *fspectra=NULL, *ftotcounts=NULL, *finfofile=NULL;


void OwnInit   (int argc, char *argv[]);


int main(int argc, char *argv[])
{
	long	i,j,k, 
	  bcnt[BINS+1],      /* number of trajectories contributing to count rate */
	  leftedge, rightedge;

	double bintc, binterval=1.0,
	  bpost[BINS+1],     /* limits of the bins                                */
	  bint[BINS+1],      /* count rate of a bin                               */
	  center[NCENTER], totcenter[NCENTER], range[NCENTER],
	  time, lambda, 
	  TwoTheta, TwoThetaDeg, Phi, 
	  qValue, dspacing,
	  prob=0;

	int ibin;


	/* Initialisation */
	Init   (argc, argv, VT_EVAL_ELAST);
	print_module_name("eval_elast 1.7a");
	OwnInit(argc, argv);

	switch (kind) 
	{
		case 1: fprintf(LogFilePtr, "Option: d-spacing\n"); break;
		case 2: fprintf(LogFilePtr, "Option: momentum transfer Q\n"); break;
		case 3: fprintf(LogFilePtr, "Option: scattering angle\n");break;
		case 4: fprintf(LogFilePtr, "Option: wavelength difference\n");break;
		default:Error("Wrong value for evaluation parameter\n");
	}

	memset(totcenter, 0, NCENTER*sizeof(double));
	memset(bpost,     0, BINS*sizeof(double));
	memset(bint,      0, BINS*sizeof(double));
	bintc = 0;

	/* Construction of the Bins */
	/* logarithmic */
	if (bLogBinning)
	{	
		bpost[0] = m;

		for(ibin = 1; bpost[ibin-1] < M; ibin++)
		{
			bpost[ibin] = bpost[ibin-1] * (1.0 + dLogProz/100.);
			bint [ibin] = 0.0;
			bcnt [ibin] = 0;
		}
		nbins = ibin-1;
	}
	/* linear */
	else
	{	binterval = (M - m) / (double)nbins;
		
		for(ibin = 0; ibin<=nbins; ibin++)
		{
			bpost[ibin] = m + binterval*ibin;
			bint [ibin] = 0.0;
			bcnt [ibin] = 0;
		}
	}

	/* Processing of the Neutrons */
	DECLARE_ABORT

	while (ReadNeutrons())
	{	for(i=0; i<NumNeutGot; i++)
		{
			CHECK

			CartesianToSpherical(InputNeutrons[i].Vector, &TwoTheta, &Phi);
			prob     = probactiv ? InputNeutrons[i].Probability : 1.0;
			time     = InputNeutrons[i].Time - TimeOffset;
			lambda   = TOF ? 395.60346/(Flightpath/time) : referenceWavelength;
			// TwoTheta = InputNeutrons[i].Vector[0];

			/* Writing out all neutrons, if 'exclusive counts = no' is set */
			if (bExclCount==FALSE)		
				WriteNeutron(&InputNeutrons[i]);

			/* trajectories within deadspot */
			if (deadspotactive && TwoTheta <= deadspotangle) continue;

			/* traj. out of time of evaluation */
			if (time < dEvalTimeMin || time > dEvalTimeMax) continue;

			/* exclude traj. with wrong colour: (nColour=0 means: all colours accepted) */
			if (nColour!=0 && nColour!=InputNeutrons[i].Color) continue;

			/* Writing out the neutrons that comply with the requirements, 
			   if 'exclusive counts = yes' is set */
			if (bExclCount==TRUE)		
				WriteNeutron(&InputNeutrons[i]);

			switch (kind) 
			{
				case 1: /* dspacing */
					dspacing = lambda / (2.0 * sin(TwoTheta/2.0));
					for(ibin = 0; ibin<nbins; ibin++)
					{	if (bpost[ibin] <= dspacing && dspacing < bpost[ibin+1])
						{
							bcnt[ibin]++;
							bint[ibin] = bint[ibin] + prob;
							bintc = bintc + prob;
							break;
						}
					}
					break;

				case 2: /* q-range */
					qValue = (4.0*M_PI/lambda)*sin(TwoTheta/2.0);
					for(ibin = 0; ibin<nbins; ibin++)
					{	if (bpost[ibin] <= qValue && qValue < bpost[ibin+1])
						{
							bcnt[ibin]++;
							bint[ibin] = bint[ibin] + prob;
							bintc = bintc + prob;
							break;
						}
					}
					break;

				case 3:	/* scattering angle */
					TwoThetaDeg = TwoTheta*180.0/M_PI;
					for(ibin = 0; ibin<nbins; ibin++)
					{	if (bpost[ibin] <= TwoThetaDeg && TwoThetaDeg < bpost[ibin+1])
						{
							bcnt[ibin]++;
							bint[ibin] = bint[ibin] + prob;
							bintc = bintc + prob;
							break;
						}
					}
					break;

				case 4: /* lambda-diff */
					dDelLambda = lambda - InputNeutrons[i].Wavelength;
					for(ibin = 0; ibin<nbins; ibin++)
					{	if (bpost[ibin] <= dDelLambda && dDelLambda < bpost[ibin+1])
						{
							bcnt[ibin]++;
							bint[ibin] = bint[ibin] + prob;
							bintc = bintc + prob;
							break;
						}
					}
					break;
			}
		}
	}

	my_exit:


	/* Output of Results */
	fprintf(LogFilePtr, "total neutron count rate within binning: %11.4e n/s \n", bintc);

	/* Spectrum */
	if (fspectra != NULL)
	{
		double bmid;

		for(ibin = 0; ibin<(nbins); ibin++)
		{	
			/* if (fabs(bint[ibin]) < 1E-40)
				bint[ibin] = 0.0; */
			if (bLogBinning)
				bmid = sqrt(bpost[ibin]*bpost[ibin+1]);
			else
				bmid = (bpost[ibin]+bpost[ibin+1])/2.0;
			fprintf(fspectra,"%12g %12g %7ld\n", bmid, bint[ibin], bcnt[ibin]);
		}
		fclose(fspectra);
	}

	if (ftotcounts != NULL && binterval > 0)
	{ 
		if (finfofile == NULL)
		{
			Error("you must define a spectra information file to obtain integrated counts");
		}

		fprintf(LogFilePtr,"\n binintervalls = integration range for each selected point:");

		i = 0;
		while(!feof(finfofile))
		if (2 == fscanf(finfofile, "%lf %lf", &center[i], &range[i]))
		i++;
		else
		break;
		fclose (finfofile);

		for (j=0; j<i; j++)
		{
			leftedge =  (long)floor( (center[j] - (range[j]/2.0) -m)/binterval );
			rightedge = (long)floor( (center[j] + (range[j]/2.0) -m)/binterval);
	  				  
			fprintf(LogFilePtr,"\n [%ld, %ld]",leftedge, rightedge);
				  
			for (k=leftedge; k<=rightedge; k++)
			totcenter[j] += bint[k];
		}

		/* writeout */
		for(j = 0; j<i; j++)
		{ 
			if (fabs(totcenter[j]) < 1E-40) totcenter[j] = 0.0;
			fprintf(ftotcounts,"%7.7f\t%11.7E\n", center[j], totcenter[j]);
		}
		fclose(ftotcounts);
	}  /* end totcounts */


	/*Cleanup*/
	stPicture.eType = (short) kind;  
	fprintf(LogFilePtr,"\n");
	Cleanup(0.0,0.0,0.0, 0.0,0.0);

	return 0;
}


void OwnInit(int argc, char *argv[])
{
	long   i;
	double winp;
	char * arg;

	for(i=1; i<argc; i++) 
	{
		arg = argv[i];
		if (*arg !='+') 
		{
			arg += 2;
			switch(arg[-1]) 
			{
				case 'o':
					if(fspectra = fopen(FullParName(arg),"w"))
						break;
					fprintf(LogFilePtr,"\nERROR: File %s could not be opened for spectra output\n",arg);
					exit(-1);
					  
				case 'O':
					if (ftotcounts = fopen(FullParName(arg),"w"))
						break;
					fprintf(LogFilePtr,"\nERROR: File %s could not be opened for integrated output\n",arg);
					exit(-1);

				case 'I':
					/* info file for generating integrated output */
					if (finfofile = fopen(FullParName(arg),"r"))
						break;
					fprintf(LogFilePtr,"\nERROR: File %s could not be opened \n",arg);
					exit(-1);

				case 'n':
					nbins = atol(arg); /* number of bins */
					if (nbins <= BINS)
						break;
					fprintf(LogFilePtr,"\nERROR: number of bins must be <= %d", BINS);
					exit(99);

				case 'k':
					kind = atol(arg); /* 1= d-spacing; 2=momentum transfer q; 3=scattering angle */
					break;

				case 'w':
					winp = atof(arg);
					if (winp == 1.0) TOF = TRUE; /* time of flight instrument */
					break;

				case 'r':
					referenceWavelength = atof(arg); /* reference Wavelength for crystal monochromator */
					break;                           /* (or mechanical velocity selector) instrument   */

				case 'e':
					dEvalTimeMin = atof(arg);        /* minimal time for evaluation */
					break;

				case 'E':
					dEvalTimeMax = atof(arg);        /* maximal time for evaluation */
					break;


				case 'C':
					nColour = atol(arg);       /*  excludes all neutrons with diff. Colour, if nColour > 0 */
					break;

				case 'd':
					deadspotangle = M_PI*atof(arg)/180.0; /* excludes all neutrons with a           */
					deadspotactive = TRUE;                /* scattering angle < deadspotangle [deg] */
					break;


				case 'm':
					m = atof(arg);   /* lower bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;

				case 'M':
					M = atof(arg);   /* upper bound of d-spacing, q or theta range [A], [1/A], [deg]*/
					break;

				case 'R':
					dLogProz    = atof(arg);       /* percentage of increase to next bin */
					if (dLogProz!=0.0)
						bLogBinning = TRUE;
					break;


				case 'c':
					if(atol(arg)==1)        /* if activated, only neutrons complying with the  */
						bExclCount = TRUE;   /* evaluate requirements are considered further on */
					break;


				case 'l':
					Flightpath = atof(arg);  /* length of neutron flight path [cm] */
					if (Flightpath <= 0.0)
						Error("you must define a flight path > 0.0");
					break;

				case 'T':
					TimeOffset = atof(arg); /* global shift of the neutron time t= t-TimeOffset [ms] */
					break;

				case 'p':
					probactiv = atoi(arg);
					/* probactiv=1 means probabilities activated, else neutron weight is set to 1.0 */
					break;

				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n", argv[i]);
					exit(-1);
					break;
			}
		}
	}

	/* checks */
	if (bLogBinning && m==0.0)
		Error("lower bound value must not be zero for logarithmic binning"); 
}

