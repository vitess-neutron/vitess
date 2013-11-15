#ifndef OPT_DEFS_H
#define OPT_DEFS_H

// definitions
// ------------
#define BUF_LEN 1024  // maximal length of strings read from file
#define FN_LEN    99  // maximal length of a filename (incl. path)
#define KW_LEN    49  // maximal length of a key word or title

#define IMAX    1024  // maximal number of measuring points
#define NMAX      16  // maximal number of fit parameters

#define CONT       1
#define READY      2

#define MAX_FIL   10  // Code must be changed, if MAX_FIL > 10   (sscanf(sFile) gener_fct 187f)
#define MAX_PAR   25  // Code must be changed, if MAX_PAR > 25   (sscanf(nModNo)  gener_fct 195ff,
#define MAX_SIM  100  // max. number of simulations in a fit step sscanf(sParId)  gener_fct 200ff,
#define MAX_MOD   40  //                                          sscanf(sParVal) gener_fct 254ff)

// enumerations
// ------------
typedef enum
{	VT_SYS_NN = 0,
	VT_WIN_NT = 1,
	VT_WIN_98 = 2,
	VT_UNIX   = 3,
	VT_LINUX  = 4
}
VtSystem;


typedef enum
{ VT_APPL_NN   = 0,
  VT_OPT_PC    = 1,
  VT_OPT_GRID  = 2,
  VT_FIT_PC    = 3
}
VtAppl;

typedef enum
{ VT_METHOD_NN  = 0,
  VT_OPT_GRAD   = 1,
  VT_OPT_GRAD_MC= 2,
  VT_METROPOLIS = 3,
  VT_SIMPLEX    = 4,
  VT_SWARM      = 5,
  VT_GENETIC    = 6
}
VtFitMethod;
#endif
