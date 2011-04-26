/* M. Fromme HZB 2010
   parallel execution of a helper threads
*/

#include <stdlib.h>
#include <string.h>
#include "init.h"
#include "softabort.h"

static Neutron* OutNeutronsParallel;
//extern int      NThreads; // declared in init.h
static int      ChunkSize;

static void (*doProc)(int, int);  // task routine to be done in parallel

static int      outNbufSize, *outNcount;
static int      MCbufSize, *MCcount;
static double   *MCbuffer;
static float    maxmcusage;      // to report max. fill ratio of mc buffers
static int      doParBarrier;	 // 0 (back) or 1 (start)
static int      outstanding;     // number of threads not ready yet

static gsl_rng **vit_thread_gsl_rng;    // individual generators for threads 

static void doChunk(int thread_i);

#ifdef WIN32

#ifdef WIN32KNOWN
# include <windows.h>
# include <process.h>
#else
#define WIN32KNOWN 1
#endif

static HANDLE  hEvent1;
static HANDLE  hEvent2;
static HANDLE  hDone;
static HANDLE  hWorkMutex;      // to guard commonly written variables
static HANDLE  hDoneMutex;      // to indicate "work is done"
static HANDLE  hDebugMutex;     // to guard debug output of threads
static HANDLE  winThread[MAXWORKER];

void debugLock() {
  WaitForSingleObject( hDebugMutex, INFINITE);
}

void debugUnlock() {
  ReleaseMutex(hDebugMutex);
}

static int setDone () {
  int i, all_done;
  all_done = 0;

  WaitForSingleObject( hWorkMutex, INFINITE );
  if (outstanding) {
    outstanding--;
    if (outstanding == 0) all_done = 1;
    }
  ReleaseMutex(hWorkMutex);

  if (all_done) {
    // signal work is done to master
    WaitForSingleObject( hDoneMutex, INFINITE );
    SetEvent(hDone); 
    ReleaseMutex(hDoneMutex);
  }
  return i;
}

static void threadLoop (void *arg) {

  int toggle_barrier, my_thread_i;
  toggle_barrier = 0;
  my_thread_i = (int) arg;

  while(!finishSoftabort) { // finish from softabort.h

    // first wait at a barrier until work is available
    if ((toggle_barrier = !toggle_barrier)) {
      WaitForSingleObject( hEvent1, INFINITE ); // front barrier
    } else {
      WaitForSingleObject( hEvent2, INFINITE ); // back barrier
    }

    if (finishSoftabort) break;

    doChunk(my_thread_i);

    setDone();
  }
}

static int initParallel (int nworkers) {
  long i;
  if (nworkers <= 0) return 0;

  // these events must be reset (TRUE), and are unset initially (FALSE)
  hDone   = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEvent1 = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEvent2 = CreateEvent(NULL, TRUE, FALSE, NULL);

  hWorkMutex = CreateMutex( NULL, FALSE, NULL );  // Cleared, we do not 
  hDoneMutex = CreateMutex( NULL, FALSE, NULL );  // request these mutexes here

  hDebugMutex = CreateMutex( NULL, FALSE, NULL ); // request these mutexes here

  for (i=1; i<=nworkers; i++)
    winThread[i-1] = _beginthread( threadLoop, 0, (void *)i);

  return nworkers;
}

static void startHelpers () {

  // prepare the release at the right barrier
  WaitForSingleObject( hWorkMutex, INFINITE );
  
  // Toggle Barriers
  doParBarrier = !doParBarrier;

  outstanding = NThreads;
  ResetEvent(hDone);
  ReleaseMutex(hWorkMutex);
  
  if (doParBarrier) {
    ResetEvent(hEvent2);
    SetEvent(hEvent1);
  } else {
    ResetEvent(hEvent1);
    SetEvent(hEvent2);
  }
}

static void waitForHelpers() {
  WaitForSingleObject( hDone, INFINITE );
}

static void shutdownParallel() {
  finishSoftabort = 1;
  // break all brakes
  SetEvent(hEvent1);
  SetEvent(hEvent2);
  // wait for termination of all helper threads 
  WaitForMultipleObjects(NThreads, winThread, TRUE, INFINITE);
}

// end Windows
#else
// Linux

# include <sys/types.h>
# include <pthread.h>
#define LOCK(a) pthread_mutex_lock (&a)
#define UNLOCK(a) pthread_mutex_unlock (&a)
#define WAIT(a,b) pthread_cond_wait (&a, &b)
#define SIGNAL(a) pthread_cond_signal (&a)
#define BROADCAST(a) pthread_cond_broadcast (&a)
#define CANCEL(a) pthread_cancel(a)

#define WAITLOOP(a) LOCK(a##_m); \
 pthread_cleanup_push(unlock_##a##_m, NULL);\
 while(a##_hold) WAIT(a##_var,a##_m);\
 pthread_cleanup_pop(1);

#define DEFLOCK(a)\
 static pthread_mutex_t  a##_m  = PTHREAD_MUTEX_INITIALIZER;\
 static pthread_cond_t  a##_var = PTHREAD_COND_INITIALIZER;\
 static int a##_hold = 1;\
 static void unlock_##a##_m (void* arg) {UNLOCK(a##_m);}

DEFLOCK(start);
DEFLOCK(back);
DEFLOCK(done);

#undef DEFLOCK

static pthread_mutex_t work_m;  // mutex to guard more_to_do and outstanding
static pthread_mutex_t debug_m; // mutex to guard debug output

void debugLock()   { pthread_mutex_lock(&debug_m);}
void debugUnlock() { pthread_mutex_unlock(&debug_m);}

static void setDone () {
  int all_done = 0;
  pthread_mutex_lock(&work_m);
  if (outstanding) {
    outstanding--;
    if (outstanding == 0) all_done = 1;
    }
  pthread_mutex_unlock(&work_m);
  if (all_done) {
    // signal work is done to master
    LOCK(done_m);
    done_hold = 0;
    SIGNAL(done_var);
    UNLOCK(done_m);
  }
}

static void *threadLoop (void *arg) {

  int toggle_barrier;
  long my_thread_i;
  toggle_barrier = 0;
  my_thread_i = (long) arg;

  while(!finishSoftabort) { // finish from softabort.h

    // first wait at a barrier until work is available
    if ((toggle_barrier = !toggle_barrier)) {
      WAITLOOP(start); // front barrier
    } else {
      WAITLOOP(back); // back barrier
    }
    doChunk(my_thread_i);

    setDone();
  }
  return arg;
}


static int initParallel (int nworkers) {
  long i;
  static pthread_t threads[MAXWORKER+1];

  if (nworkers <= 0 || nworkers > MAXWORKER) return 0;
  pthread_mutex_init(&work_m, 0);

  for (i=1; i<=nworkers; i++)
    if (pthread_create(threads + i, NULL, threadLoop, (void *) i))
      return 0;

  return nworkers;
}


static void startHelpers () {
  // prepare the release at the right barrier
  if ((doParBarrier = !doParBarrier)) {
    LOCK(start_m);
    start_hold = 0;
    back_hold = 1;
  } else {
    LOCK(back_m);
    start_hold = 1;
    back_hold = 0;
  }
  outstanding = NThreads;
  done_hold = 1;  // so that we may wait at WAITLOOP(done)
  if (doParBarrier) {
    BROADCAST(start_var);
    UNLOCK(start_m);
  } else {
    BROADCAST(back_var);
    UNLOCK(back_m);
  }
}

static void waitForHelpers() {
  WAITLOOP(done);
}

static void shutdownParallel() { }

#endif  // Linux

static double myVran(int n) {
  if (vit_thread_gsl_rng)
    // generate a random number for thread n, n=0,1,..
    return gsl_rng_uniform (vit_thread_gsl_rng[n]);

  // else use a pre-computed random number
  int c = MCcount[n] - 1;
  if (c < 0) myExit2("insufficient random number storage (%d,%d) for threads, increase prefetch buffer size\n", n,MCbufSize);
  MCcount[n] = c;
  return MCbuffer[n*MCbufSize + c];
}

double VranPar(int thread_i) {
  thread_i--;
  if (thread_i < 0) return Vran(); // called from main thread
  return myVran(thread_i);
}

double MonteCarloPar(double x, double y, int thread_i) {
  return x + (y - x) * (thread_i <= 0 ? Vran() : myVran(thread_i-1));
}

/* Polar (Box-Mueller) method; See Knuth v2, 3rd ed, p122 */

double ran_gaussian_par (const double sigma, int thread_i)
{
  double x, y, r2;

  do
  {
    /* choose x,y in uniform square (-1,-1) to (+1,+1) */

    x = -1 + 2 * VranPar(thread_i);
    y = -1 + 2 * VranPar(thread_i);

    /* see if it is in the unit circle */
    r2 = x * x + y * y;
  }
  while (r2 > 1.0 || r2 == 0);

  /* Box-Muller transform */
  return sigma * y * sqrt (-2.0 * log (r2) / r2);
}

double DistrGaussPar(const double Center, const double Sigma, int thread_i)
{
  return Center + ran_gaussian_par(Sigma, thread_i);
}


void ran_dir_3d_par (double *p, int thread_i) {

  double s, a, x,y;

  // Copy of the routine gsl_ran_dir_3d in rng/sphere.c which uses preallocated random numbers.
  do
    {
      x = -1 + 2 * VranPar(thread_i);
      y = -1 + 2 * VranPar(thread_i);
      s = x*x + y*y;
    }
  while (s > 1.0);

  p[2] = -1 + 2 * s;   // z
  a = 2 * sqrt (1 - s);

  p[0] = x * a;
  p[1] = y * a;
}

static int createThreadBuffers() {
  if (outNbufSize*NThreads == 0) return 0;
  if ( ! (OutNeutronsParallel = (Neutron *) malloc(NThreads*outNbufSize*sizeof(Neutron))) ||
       ! (outNcount           = (int *)     calloc(NThreads, sizeof(int))))
    return 0;
  if (MCbufSize < 0) {
    // prepare individual random number generators per thread
    int n;
    extern long int VRandomSeed;
    vit_thread_gsl_rng = malloc(NThreads * sizeof(gsl_rng *));
    for (n=0; n<NThreads; n++) {
      gsl_rng * g;
      vit_thread_gsl_rng[n] = g = gsl_rng_alloc(gsl_rng_default);
      if (!g) 
        myExit("unable to create individual random number generators!\n");
      gsl_rng_set(g, VRandomSeed+n+1);
    } 
  }
  if (MCbufSize <= 0) return 1;
  return
    (MCbuffer = (double *) malloc(NThreads*MCbufSize*sizeof(double))) &&
    (MCcount  = (int *)    calloc(NThreads, sizeof(int)));
}

static int morefill;

static void fillMCbuffers() {
  // fills buffers to contain MCbufSize random numbers
  // MCcount[0] shows what is left for thread 1, which may use entries 0,1,...,MCcount[1]-1
  int c,i,n;
  if (MCbufSize <= 0) return;
  for (n=0; n<NThreads; n++)
    if ((c = MCcount[n]) < MCbufSize) {
      float ratio;
      double *v = MCbuffer + n*MCbufSize + c;
      for (i=c; i<MCbufSize; i++)
	*v++ = Vran();
      MCcount[n] = MCbufSize;
      if (morefill) {
	// do statistics on buffer usage, but not at the first call of fillMCbuffers
	ratio = (float)(MCbufSize - c) / MCbufSize;
	if (ratio > maxmcusage) maxmcusage = ratio;   
      }
    }
  morefill = 1;
}

void printMCStatistic(FILE *f) {
  if (NThreads <= 0) return; 
  if (MCbufSize > 0) 
    fprintf(f, "maximum usage of mc prefetch buffers %8.1f %%, buffer sizes %d, %d helper thread(s)\n",
	    100*maxmcusage, MCbufSize, NThreads);
  else
    fprintf(f, "used %d helper thread(s)\n", NThreads);
}


void WriteNeutronParallel(Neutron *OutNeutron, int thread_i) {
  if (NThreads <= 0 || thread_i <= 0)
    WriteNeutron(OutNeutron);
  else {
    int i,c;
    i = thread_i - 1;  // i will be 0 for thread 1
    if ((c = outNcount[i]) >= outNbufSize)
      myExit2("!!! temp buffer size %d too small for thread %d !!!\n", outNbufSize, thread_i);
    outNcount[i] = c+1;
    CopyNeutron(OutNeutron, OutNeutronsParallel + i*outNbufSize + c);
  }
}

static void flushParallelOutput() {
  int c,i,n;
  for (n=0; n<NThreads; n++)
    if ((c = outNcount[n])) {
      Neutron *neut = OutNeutronsParallel + n*outNbufSize;
      for (i=0; i<c; i++)
	WriteNeutron(neut++);
      outNcount[n] = 0;
    }
}	


static void doChunk(int thread_i) {
  int i,min_i,max_i;
  min_i = thread_i*ChunkSize;
  max_i = min_i + ChunkSize - 1;
  if (max_i >= NumNeutGot) max_i = NumNeutGot-1;
  for (i=min_i; i<=max_i; i++) {
    CHECK;
    doProc(i, thread_i);
  }
 my_exit:;
}

void processPipedNeutronsWithOutput(int nthreads, void (*p)(int, int), void (*po)(),
			  int maxnratio, int maxmc) {
  int maxchunksize;
  DECLARE_ABORT;

  if (nthreads <= 0) {
    // serial execution
    while (ReadNeutrons()) {
      int i;
      CHECK;
      for(i=0; i<NumNeutGot; i++)
        (*p)(i, 0);    
      if (po) 
        (*po)();
    }
    return;
  } 
  
  // parallel execution

  NThreads = initParallel(nthreads);
  if (nthreads != NThreads) myExit("could not create threads\n");

  doProc = p;    // remember what to do in threads

  if (maxnratio <= 0)
    maxnratio = 0;
  maxchunksize = 1 + BufferSize/(NThreads + 1);
  outNbufSize = maxnratio * maxchunksize;

  MCbufSize = maxmc * maxchunksize;

  if (! createThreadBuffers()) myExit("Couldn't allocate memory for temporary thread buffers\n");

  while (ReadNeutrons()) {
    CHECK;
    ChunkSize = 1 + NumNeutGot/(NThreads + 1);
    if (MCbufSize > 0) 
      fillMCbuffers();
    startHelpers();
    // the main thread does it's share doChunk(0)
    // parallel to helper threads which do doChunk(1),...
    doChunk(0);
    waitForHelpers();
    flushParallelOutput();
    if (po) 
      (*po)();
  }
  my_exit: ;
  shutdownParallel();
}
