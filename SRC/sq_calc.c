/**************************************************/
/*  Last change:      BB  17 Jan 2002   10:14 am  */
/*  Transformed to C: KL  16 Jul 2002             */
/*                                                */
/*   program pyhsm                                */
/*                                                */
/*  CALCULATION OF A STRUCTURE FACTOR WITH THE    */
/*      PERCUS-YEVICK HARD SPHERE MODEL           */
/*                                                */
/**************************************************/

#include "general.h"

/**************************************************/
/* static variables                               */
/**************************************************/
static double alf,bet,gam,sca,sca0;


/**************************************************/
/* local functions                                */
/**************************************************/
void scalp(long nin, double q, double sig);


double pyshm(long nin, double q, double sig, double ro)
{
  double S, sig3, capis, pf, den, usden, ho, hop, sz;

  sig3 = pow(sig,3);
  capis= 4.0*M_PI*sig3;
  pf   = M_PI*ro*sig3/6.0;
  den  = pow(1.0-pf, 4);
  usden= 1.0/den;
  ho   = sq(1.+2.0*pf);
  hop  = sq(1.+0.5*pf);
  alf  =-ho*usden;
  bet  = 6.0*pf*hop*usden;
  gam  =-0.5*pf*ho *usden;

  scalp(nin, q, sig);
    sca  = capis*sca;
    sca0 = capis*sca0;

  sz = 1./(1.-ro*sca0);
    S  = 1./(1.-ro*sca);

  return S;
}


void pyshmFileGen(char* text, long n, long nin, double q0, double dq, double sig, double ro,
           FILE* pOut)
{
  double S[800], a[800],
         q, sig3, capis, pf, den, usden, ho, hop, sz;
  long   i,ij;

  sig3 = pow(sig,3);
  capis= 4.0*M_PI*sig3;
  pf   = M_PI*ro*sig3/6.0;
  den  = pow(1.0-pf, 4);
  usden= 1.0/den;
  ho   = sq(1.+2.0*pf);
  hop  = sq(1.+0.5*pf);
  alf  =-ho*usden;
  bet  = 6.0*pf*hop*usden;
  gam  =-0.5*pf*ho *usden;

  for (ij=1; ij<=n; ij++)
  {  q = q0 + dq*(float)(ij-1);
    scalp(nin, q, sig);
        sca  = capis*sca;
        sca0 = capis*sca0;
    // fprintf(pOut,"%12.6e  %12.6e\n", q, sca);
    sz    = 1./(1.-ro*sca0);
         S[ij] = 1./(1.-ro*sca);
        a[ij] = q;
  }

  if (pOut != NULL)
  {  fprintf(pOut,"\n%s\n", text);
    // fprintf(pOut,"S(0)= %12.6e\n", sz');
    for(i=1; i<=n; i++)
    {  fprintf(pOut,"%12.6e  %12.6e\n", a[i],S[i]);
    }
  }

  return;
}

void scalp(long nin, double q, double sig)
{
  long i;
  double dnin, f, cuf, rap, abgam;

  sca  = 0.0;
  sca0 = 0.0;
  dnin = 1.0/((float)nin);

  for (i=1; i<=nin; i++)
  {  f    = dnin*(float)i;
    cuf  = f*q*sig;
    rap  = sin(cuf)/cuf;
    abgam= alf + f*bet + gam*pow(f,3);
    sca  = sca + dnin*sq(f)*rap*abgam;
    sca0 = sca0+ dnin*sq(f)*abgam;
  }

  return;
}
