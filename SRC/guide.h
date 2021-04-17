#ifndef GUIDE_H
#define GUIDE_H

/******************************/
/** Definitions              **/
/******************************/

#include "defines.h"

#define INDEX(x,y,p) (x*(nbinsY)+y   +  (p+1)*nbinsX*nbinsY )

#define MAX_GAMMA_NUM 19

// binning keys
#define KeyNone            0
#define iKeyMode           1
#define iKeyMode0          2
#define iKeyMode5          3
#define iKeyMode10         4
#define dKeyRefCount       5
#define dKeyRefCountY      6
#define dKeyRefCountZ      7
#define iKeyThisCollision  8
#define dKeydegangular     9
#define dKeym             10
#define dKeyreflectivity  11
#define dKeyDivY          12
#define dKeyDivZ          13
#define iKeyColor         14
#define dKeyTime          15
#define dKeyWavelength    16
#define dKeyProbability   17
#define dKeyPositionX     18
#define dKeyPositionY     19
#define dKeyPositionZ     20
#define dKeyVectorX       21
#define dKeyVectorY       22
#define dKeyVectorZ       23
#define dKeySpinX         24
#define dKeySpinY         25
#define dKeySpinZ         26


/******************************/
/** Structures and Enums     **/
/******************************/

typedef struct
{
  Neutron    neutron;
  VtGdeWall  ThisCollision;
  double     degangular;
  double     m;
  double     reflectivity;
  double     DivY;
  double     DivZ;
  int        Mode;  // 0 = Scattered, 5 = GW_EXIT (not saved), 10 = Died
}
NeutronEx;

typedef struct
{
  double    X;
  double    Y;
  NeutronEx ndata;
  long      Counts;
  double    RefCount;   /* Counts the number of reflections.
                           If the last reflection did not occur for any reason,
                           RefCount is increased by one, and multiplied by -1 */
  double    RefCountY;  /* Number of reflection on horizontal guide planes */
  double    RefCountZ;  /* Number of reflection on vertical guide planes */
  int       Mode10;
  int       Mode5;
  int       Mode0;
  double    ProbSum;
} 
BINDATA;

static int allocNeutrons = 16; // allocacte ReflCond.neutrons in this chunk size,
                               // choose a number big enough to avoid frequent reallocations
static int allocText = 1023;   // allocate text buffers for ReflCond.Output in chunks of this size+1 

typedef struct
{
  int       RefCount;       // Count of reflections:
                            // If the last reflection did not occur for any reason,
                            // RefCount is increased by one, and multiplied by -1
  int       RefCountY;      // Number of reflection on horizontal guide planes
  int       RefCountZ;      // Number of reflection on vertical guide planes
  char      *Output;        // One line of text for each reflection will be stored to this
  int       alloc_text;     // allocated bytes for text output
  int       insert_at;      // text position, where to insert text in Output
  NeutronEx *neutrons;      // neutron trajectory states
  int       alloc_neutrons; // allocated space for neutrons, in multiples of allocNeutrons
  int       cneutrons;      // number of stored neutron trajectory states
}
ReflCond;

typedef struct
{
  Plane *Wall;
}
NeutronGuide;

typedef struct
{
  double MValue;
  FILE   *pfile;
  char   *filename;
  double *Rdata;
  long   maxdata;
  double area;
}
ReflFile;

typedef struct
{
  double Xpce, Ypce, Zpce, // list of x-pos., width and height at beginning and end of pieces
    Wchan;                 // list of widths of channel at beginning and end of each piece
  ReflFile **RData;        // use GW_TOP, GW_BOTTOM, GW_LEFT, GW_RIGHT, etc
}
GuidePiece;
  
typedef struct
{
  double gammaInt[MAX_GAMMA_NUM];     //Intensities of gammas of Ni and Ti
  double neutronInt;                  //Intensity of the transmitted neutron 
}
Escaping_MCPL_particles;


typedef  double(*GetVal)(ReflCond *RefOut, int cNeut);

#endif
