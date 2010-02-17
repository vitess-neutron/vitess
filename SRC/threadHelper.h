/* M. Fromme HZB 2010
   parallel execution of a helper threads
*/
void processPipedNeutrons(int nthreads, void (*p)(int, int),
			  int maxnratio, int maxmc);

void WriteNeutronParallel(Neutron *n, int thread_i);
double MonteCarloPar(double x, double y, int thread_i);
double VranPar(int thread_i);
void ran_dir_3d_par (double *v, int thread_i);
void printMCStatistic(FILE *f);
void debugLock(), debugUnlock();
