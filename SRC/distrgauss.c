#include "init.h"

double gsl_ran_gaussian (const gsl_rng * r, const double sigma);

double DistrGauss(double Module, double Sigma)
{
  return Module + gsl_ran_gaussian( vit_gsl_rng, Sigma);
}
