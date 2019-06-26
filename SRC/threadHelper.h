/* M. Fromme HZB 2010
   parallel execution of a helper threads
*/
void   processPipedNeutronsWithOutput(int nthreads, void (*p)(int, int),  void (*po)(), int maxnratio, int maxmc);
#define processPipedNeutrons(a,b,c,d) processPipedNeutronsWithOutput(a,b,0,c,d)

void   WriteNeutronParallel(Neutron *n, int thread_i);
double MonteCarloPar(double x, double y, int thread_i);
double VranPar(int thread_i);
double DistrGaussPar(const double Center, const double Sigma, int thread_i);
double ran_gaussian_par (const double sigma, int thread_i);
void   ran_dir_3d_par (double *v, int thread_i);
void   printMCStatistic(FILE *f);
void   debugLock(), debugUnlock();

int    fwritePar(void *data, size_t s, int n, FILE *f);
void   initParWrite(size_t s, int n);
