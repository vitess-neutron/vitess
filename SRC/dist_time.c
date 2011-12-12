/****************************************************************************************/
/* Tool DistTimePlot:                                                                   */
/*  Plotting of a distance-time-diagram                                                 */
/*                                                                                      */
/* 1.0  Sep 2003  K. Lieutenant    initial version                                      */
/* 1.1  Nov 2003  K. Lieutenant    extension for choppers with more than 1 aperture,    */
/*                                 different frames, new plot option, etc.              */
/* 1.2  Mar 2004  K. Lieutenant    tool adapted to call from GUI                        */
/*      Dec 2011  M. Fromme        sleep some time to show graphic window               */
/****************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "init.h"
#include "cpgplot.h"


#define TRUE  1
#define FALSE 0

#define BUF_SIZE 255

#ifdef WIN32
# include <windows.h>
# define SLEEPSECONDS(a) Sleep(1000*a)
#else
# include <unistd.h>
# define SLEEPSECONDS(a) sleep(a)
#endif

static void  DrawLine   (const float fVeloc,  float fTime);
static float fMin       (float value1,  float value2);
static float fMax       (float value1,  float value2);
static short ReadChopper(int argc, char *argv[]);
static void  OwnInit    (int argc, char *argv[]);

char  sBuffer[BUF_SIZE+1];
const char *pTitle="  ",  // title of the plot
  *pFileName=0,  // Name of plot file
  *pFullName;    // name of plot file incl. path
float fTmax=10.0,   // max. time that shall be displayed
  fTp=1.0,      // pulse length
  fTPbeg=0.0,   // beginning, center and end of pulse
  fTPctr,
  fTPend,
  fTrep=50.0,   // repetition time of pulses
  fChopDist,    // Distances: source - (center of double) chopper
  fChopAngle,   // angle of chopper aperture
  fChopPhase,   // angle of center of chopper aperture at t=0 relative to guide center
  fDetDist=20.0;// Distance:  source - detector
short nFrameMin=1,  // number of first frame
  nNumWnd=1,    // number of apertures on chopper
  nNumOpen=1;   // number of chopper openings per cycle of source
int sleepsecs = 2;  // show graphic display some seconds before closing the graphic window


int main(int argc, char* argv[])
{
  float v,
    fTchop,          /* time between opening of 2 successive apertures */
    fWBDist=0.0,     /* Distances: source - (center of double) WB chopper  */
    fTopen,
    fTclose,         /* opening and closing time of chopper */
    fWBTchop=0.0,    /* time between opening of 2 successive apertures of the WB-Chopper */
    fWBTopen=0.0,
    fWBTclose=0.0,   /* opening and closing time of WB chopper */
    fVfast, fVslow,  /* velocities of fastest and slowest neutrons able to pass the WB chopper */
    fT0fast, fT0slow,/* times at which those neutrons start at the moderator */
    t;               /* starting time (run variable in loop) */
  short nNumFrms=1,  /* number of frames to be displayed */
    nOption=1,       /* Option: 1: beginning of frame defined by beginning of pulse,
                        end of frame defined by end of pulse
                        2: beginning of frame defined by end of pulse and vice versa */
    bSubFrame=FALSE, /* criterion: frame (to be displayed) defined by multi-aperture choppers */
    bDrawDet=TRUE,   /* criterion: draw line for detector */
    nNumChop=0,      /* number of choppers */
    nFr;             /* running variable in loop over frames */
  const char *GraphDev;	
#ifdef DO_WIN32
# define DEFAULTNAME "dist_time.ps"
#else
# define DEFAULTNAME "dist_time.png"
#endif

  /* get input data */
  /* -------------- */
  Init(argc, argv, VT_TOOL);
  OwnInit(argc, argv);
  nNumFrms = 1;
  nOption  = 1;
  bSubFrame= FALSE;
  bDrawDet = TRUE;
  fTPctr   = (float)(0.5*fTp);

  /* write result postscript or PNG file to parameter directory */
  if (pFileName == 0 || *pFileName == 0)
    pFileName = DEFAULTNAME;
  GraphDev = pFullName = FullParName(pFileName);

  /* initialize pgplot */
  /* ----------------- */
  if (cpgopen(GraphDev) < 1) {
    Error("plot device cannot be opened");
    exit(-1);
  }

  /* draw frame, titles and detector line */
  /* ------------------------------------ */
  cpgsfs(2);   /* size 2      */
  cpgslw(2);   /* linewidth 2 */
  v = bDrawDet ? 1.08F*fDetDist : fDetDist;
  cpgenv(0.0, fTmax,  0.0, v,  1, 0);
  cpglab("time [ms]","distance [m]", pTitle);

  /* draw detector axis */
  if (bDrawDet)  {
    cpgsci (2); /* colour 2 */
    cpgmove(0.0,  fDetDist);
    cpgdraw(fTmax,fDetDist);
  }

  /* draw choppers */
  /* ------------- */
  /* loop over choppers */
  while (ReadChopper(argc, argv))  {	
    nNumChop++;

    /* determine opening and closing times */
    fTchop  = fTrep/nNumOpen;
    fTopen  = fTchop * (fChopPhase-fChopAngle/2.0F)/(360.0F/nNumWnd) + fTchop;
    fTclose = fTchop * (fChopPhase+fChopAngle/2.0F)/(360.0F/nNumWnd);

    /* save data of WB chopper */
    if (fChopDist > fWBDist) {	
      if (bSubFrame == FALSE || nNumWnd > 1) {
        /* find first frame */
        while (fTopen <= (nFrameMin-1)*fTchop) {
          fTopen  += fTchop;
          fTclose += fTchop;
        }
        while (fTopen > nFrameMin*fTchop) {
          fTopen  -= fTchop;
          fTclose -= fTchop;
        }
        fWBDist   = fChopDist;
        fWBTchop  = fTchop;
        fWBTopen  = fTopen;
        fWBTclose = fTclose+fTchop;
      }
    }

    /* find first opening and closing time */
    while (fTopen <= 0) {
      fTopen  += fTchop;
      fTclose += fTchop;
    }
    while (fTopen > fTchop) {
      fTopen  -= fTchop;
      fTclose -= fTchop;
    }

    /* draw lines for time ranges that the chopper is closed */
    if (nNumWnd > 1)
      cpgsci(5);  /* colour 5 for multiple window choppers */
    else
      cpgsci(3);  /* colour 3 for choppers with 1 window   */
    do {
      cpgmove(fMax(0.0,   fTclose), fChopDist);
      cpgdraw(fMin(fTmax, fTopen),  fChopDist);
      fTopen  += fTchop;
      fTclose += fTchop;
    } while (fTclose < fTmax);
  }

  /* draw s-t-lines of neutrons if data of at least 1 chopper are given */
  /* ------------------------------------------------------------------ */
  if (nNumChop > 0 && fWBTchop > 0.0) {
    /* loop over frames to be displayed */
    for (nFr=0; nFr < nNumFrms; nFr++) {
      /* fTPctr  = GetFloat("center of pulse            [ms]  : "); */
      fTPbeg  = (float) (fTPctr - fTp/2.0);
      fTPend  = (float) (fTPctr + fTp/2.0);

      fTopen  = fWBTopen  + nFr*fWBTchop;
      fTclose = fWBTclose + nFr*fWBTchop;

      if (nOption==1) {
	fVfast  = fWBDist/(fTopen -fTPbeg);
        fVslow  = fWBDist/(fTclose-fTPend);
        fT0fast = fTPbeg-5*fTrep;
        fT0slow = fTPend-5*fTrep;
      } else {
        fVfast  = fWBDist/(fTopen -fTPend);
        fVslow  = fWBDist/(fTclose-fTPbeg);
        fT0fast = fTPend-5*fTrep;
        fT0slow = fTPbeg-5*fTrep;
      }

      cpgsci(4);
      for (t=fT0fast; t < fTmax; t+=fTrep)	
        DrawLine(fVfast, t);

      for (t=fT0slow; t < fTmax; t+=fTrep)
        DrawLine(fVslow, t);
    }
  }

  /* Close graphic window */
  /* -------------------- */

  // Sleep some seconds to let G2 routines show the on-screen display before closing.
  if (sleepsecs)
    SLEEPSECONDS(sleepsecs);

  cpgclos();

  fprintf(LogFilePtr, "Figure saved as %s\n", pFullName);

  return 1;
}

static
void OwnInit   (int argc, char *argv[])
{
  long   i;
  char  *arg=NULL;

  for(i=1; i<argc; i++) {
    if(argv[i][0]!='+') {
      arg=&argv[i][2];
      switch(argv[i][1]) {
      case 'D':
        /* distance source - detector [m] */
        fDetDist = (float) atof(arg);		
        argv[i][0]='+';
        break;
      case 'f':
        /* First desired frame */
        nFrameMin = (short) atoi(arg);
        argv[i][0]='+';
        if (nFrameMin < 0)
          Error("Frames that can be displayed are 0, 1, 2, ....");
        break;
      case 'F':
        /* Title of the figure */
        pFileName = arg;
        argv[i][0]='+';
        break;
      case 'R':
        /* repetition time of pulses [ms] */
        fTrep = (float) atof(arg);		
        argv[i][0]='+';
        break;
      case 'p':
        /* pulse length */
        fTp = (float) atof(arg);		
        argv[i][0]='+';
        break;
      case 's':
        sleepsecs = atoi(arg);
        if (sleepsecs < 0)
          sleepsecs = 0;
        else if (sleepsecs > 100)
          sleepsecs = 100;
        break;
      case 't':
        /* Time to be displayed      [ms] */
        fTmax = (float) atof(arg);			
        argv[i][0]='+';
        break;
      case 'T':
        /* Title of the figure */
        pTitle = arg;
        argv[i][0]='+';
        break;
      }
    }
  }
}

/*
  fChopAngle = GetFloat("Angle of opening           [deg]        : ");
  fChopPhase = GetFloat("Chopper phase              [deg]        : ");
  nNumOpen   = GetShort("No of aperture openings per cycle       : ");
  nNumWnd    = GetShort("No of apertures on chopper              : ");
*/

static
short ReadChopper(int argc, char *argv[])
{
  long   i;
  char  *arg=NULL;
  short bDist=FALSE, bAngle=FALSE, bPhase=FALSE,
    bOpen=FALSE, bWnd=FALSE;

  for(i=1; i<argc; i++) {
    if(argv[i][0]!='+') {
      arg=&argv[i][2];
      switch(argv[i][1]) {
      case 'a':
        /* Angle of opening   [deg] */
        fChopAngle = (float) atof(arg);		
        bAngle=TRUE;
        break;
      case 'C':
        /*Distance: source - chopper [m] */
        fChopDist = (float) atof(arg);		
        if (fChopDist==0.0)
          return FALSE;
        bDist=TRUE;
        break;
      case 'N':
        /* No of aperture openings per cycle */
        nNumOpen = (short) atoi(arg);
        bOpen=TRUE;
        break;
      case 'n':
        /* No of apertures on chopper */
        nNumWnd = (short) atoi(arg);
        bWnd=TRUE;
        break;
      case 'o':
        /* Chopper phase      [deg] */
        fChopPhase = (float) atof(arg);			
        bPhase=TRUE;
        break;

      default:
        sprintf(sBuffer, "Unknown command option: %s\n",argv[i]);
        Error(sBuffer);
        exit(-1);
        break;
      }
      argv[i][0]='+';
      if (bDist && bAngle && bPhase && bOpen && bWnd)
        return TRUE;
    }
  }
  return FALSE;
}

static
float fMin(float value1, float value2)
{
  return value1 < value2 ? value1 : value2;
}

static
float fMax(float value1, float value2)
{
  return value1 > value2 ? value1 : value2;
}


static
void DrawLine(const float fVeloc, float t)
{

  float fSmin, fSmax, // position for t=0, t=fTmax
    fTdet;            // time of arrial at chopper

  fTdet = t + fDetDist / fVeloc;
  if (fTdet > 0.0)  {
    if (t >= 0.0)
      cpgmove(t, 0.0);
    else {
      fSmin = fVeloc * (-t);
      cpgmove(0.0, fSmin);
    }

    if (fTdet <= fTmax)
      cpgdraw(fTdet, fDetDist);
    else {
      fSmax = fVeloc * (fTmax-t) ;
      cpgdraw(fTmax, fSmax);
    }
  }
}
