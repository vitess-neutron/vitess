#include <stdio.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "monochr_analyser.h"
#include "init.h"
#include "matrix.h"


/*********************************/
/*			Functions			 */
/*********************************/


/* probability of finding a mosaic piece with normal vector Mosaic.
Maximum probability amplitude = 1 */

double Mosaicity(VectorType Mosaic)
{
double thm, phm, fwhm_fact, arg, p ;

	thm = phm = 0. ; /* initialising */


	CartesianToSpherical(Mosaic, &thm, &phm) ;

	fwhm_fact = sq((double) cos(phm) / mosaic_fwhm[0]) + sq((double) sin(phm) / mosaic_fwhm[1]) ;

	if(thm != 0.)
	{

	arg = - sq( thm * (double) sqrt(8*log(2))) / 2 * fwhm_fact ;

	if(arg < - 100.) arg = -100. ;

	p = (double) exp(arg) ;

	}

	else	p = 1. ;

	return p ;


}/* End Mosaicity */


/* probability of finding a d-spacing in Lorentzian approximation.
Maximum probability amplitude = 1 */

double	dSpreadLorentzian(double d_ran)
{
double p ;

	if(d_fwhm == 0.)
	{
		p = 1. ;

	}else{

		p = sq(d_fwhm) / ( 4.*sq(d_ran - d_spacing) + sq(d_fwhm) ) ;
	}

	return p ;

}/* End dSpreadLorentzian */


/* probability of finding a d-spacing in Gaussian approximation.
Maximum probability amplitude = 1 */

double	dSpreadGaussian(double d_ran)
{
double p, argd ;

	if(d_fwhm == 0.)
	{
		p = 1. ;

	}else{

	argd = - sq( (d_ran - d_spacing) * (double) sqrt(8.*log(2.))) / 2. / sq(d_fwhm) ;

	if(argd < - 100.) argd = -100. ;

	p = (double) exp(argd) ;

	}

	return p ;

}/* End dSpreadGaussian */

