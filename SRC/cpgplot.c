#include "cpgplot.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <g2.h>

#ifdef DO_X11
#include <g2_X11.h>
# include <unistd.h>
#endif
#ifdef DO_GIF
# include <g2_GIF.h>
#endif
#ifdef DO_PS
# include <g2_PS.h>
#endif
#ifdef DO_WIN32
# include <g2_win32.h>
#endif

#define WINX 775
#define WINY 575
#define M 20

static int dev,gdev;
int gselec = 3;

#define DD(f) fprintf(stderr, f)
#define DD1(f,a) fprintf(stderr, f,a)
#define DD2(f,a,b) fprintf(stderr, f,a,b)
#define DD3(f,a,b,c) fprintf(stderr, f,a,b,c)
#define DD4(f,a,b,c,d) fprintf(stderr, f,a,b,c,d)

#define OD(f)
#define OD1(f,a)
#define OD2(f,a,b)
#define OD3(f,a,b,c)
#define OD4(f,a,b,c,d)

#if defined(DO_GIF) || defined(DO_PS)
# define G0(c) {if (gselec & 1) c(dev); if (gselec & 2) c(gdev);}
# define G1(c,a) {if (gselec & 1) c(dev,a); if (gselec & 2) c(gdev,a);}
# define G2(c,a,b) {if (gselec & 1) c(dev,a,b); if (gselec & 2) c(gdev,a,b);}
# define G3(c,a,b,e) {if (gselec & 1) c(dev,a,b,e); if (gselec & 2) c(gdev,a,b,e);}
# define G4(c,a,b,e,f) {if (gselec & 1) c(dev,a,b,e,f); if (gselec & 2) c(gdev,a,b,e,f);}
#else
# define G0(c) c(dev)
# define G1(c,a) c(dev,a)
# define G2(c,a,b) c(dev,a,b)
# define G3(c,a,b,e) c(dev,a,b,e)
# define G4(c,a,b,e,f) c(dev,a,b,e,f)
#endif



static float tpos[3][2] = {
  {0.5*WINX, 0.5*M},      /* pos. (x,y) of heading of x-axis */
  {2.0*M, WINY-M},    /* pos. (x,y) of heading of y-axis */
  {0.6*WINX, WINY-M}  /* pos. (x,y) of title             */
};

static float xx0,yy0, xf=1,yf=1;

#define XSC(x) (x*xf+xx0)
#define YSC(y) (y*yf+yy0)

static float xsc(float x)
{
  x = XSC(x);
  if (x < 0) return 0.0;
  if (x > WINX) return (float) WINX;
  return x;
}

static float ysc(float y)
{
  y = YSC(y);
  if (y < 0) return 0.0;
  if (y > WINY) return (float) WINY;
  return y;
}

static float AxisInc(const float z)
{
  double expo=0, man, dif, delta;

  dif = z;
  while (dif > 10.) {dif /= 10; expo++;}
  while (dif <  1.) {dif *= 10; expo--;}
  if      (dif < 2.) man=0.1;
  else if (dif < 4.) man=0.2;
  else if (dif < 5.) man=0.4;
  else               man=0.5;
  delta = man*pow(10.0,expo);
  return ((float) delta);
}

static void GetFormat(char* format, double beg, double ampl)
{
  int    expo=0, nks=0, vks;
  double dif, rest=beg-floor(beg);

  while (rest > 1e-2 && nks < 4)
  { nks++;
    rest *= 10.0;
    rest -= floor(rest); 
  }

  dif = beg+ampl;
  while (dif >= 10.0) {dif /= 10.0; expo++;}
  while (dif <   1.0) {dif *= 10.0; expo--;}

  if (nks < (1-expo)) nks = 1-expo;
  if (nks==0)
    vks = 1;
  else
    vks = nks+2;

  sprintf(format, "%%%d.%df", vks, nks);
}




#define L 0.2
#define G 0.8
static double farbe[8][3] = {
  {1,0,0},
  {L,G,L},
  {0,0,1},
  {1,0,1},
  {0,1,1},
  {1,1,0},
  {1,G,0},
  {G,1,0}
};
#undef L
#undef G

static int myC[8], gC[8];

int cpgopen(const char *device)
{
  int i;

#ifdef DO_X11
  dev = g2_open_X11(WINX, WINY);
#elif DO_WIN32
  dev = g2_open_win32(WINX, WINY,"VITESS",0);
#endif

  // eigene Farben
  for (i=0; i<8; i++) {
    myC[i] = g2_ink(dev, farbe[i][0], farbe[i][1], farbe[i][2]);
    OD2("Farbe %i: %i\n", i, myC[i]);
  }

#ifdef DO_GIF
  gdev = g2_open_GIF(device, WINX, WINY);
  // eigene Farben
  for (i=0; i<8; i++) {
    gC[i] = g2_ink(gdev, farbe[i][0], farbe[i][1], farbe[i][2]);
    OD2("Farbe %i: %i\n", i, gC[i]);
  }
#elif DO_PS
  gdev = g2_open_PS(device, g2_A4, g2_PS_land);
  // eigene Farben
  for (i=0; i<8; i++) {
    gC[i] = g2_ink(gdev, farbe[i][0], farbe[i][1], farbe[i][2]);
  }
#endif
  return 1;
}

void cpgclos (void)
{
  // close device
  G0(g2_flush);
  G0(g2_close);
  OD("g2 closed\n");
}


void cpgenv(float xmin, float xmax, float ymin, float ymax, int just, int axis)
{
  // set evironment frame (xmin,xmax), (ymin,ymax)
  // just,axis immer 1,0

  float ax,ay, bx,by, x,xd,xv, y,yd,yv;
  short i;
  char  str[20], format[11];

  xf  = (float) 0.8*WINX/(xmax-xmin);
  xx0 = (float)(-xf*xmin + 0.15*WINX);

  yf  = (float)0.8*WINY/(ymax-ymin);
  yy0 = (float)(-yf*ymin + 0.11*WINY);

  // frame
  // if (!axis) return;
  G1(g2_pen, 1);
  ax = XSC(xmin);
  bx = XSC(xmax);
  ay = YSC(ymin);
  by = YSC(ymax);
  G2(g2_move,ax,ay);
  G2(g2_line_to,bx,ay);
  G2(g2_line_to,bx,by);
  G2(g2_line_to,ax,by);
  G2(g2_line_to,ax,ay);

  /* x-axis */
  /* ------ */
  xd = AxisInc(xmax-xmin);
  GetFormat(format, xmin, xmax-xmin);
  for (x=xmin,i=0; x<=xmax; x+=xd,i++) {
    xv = xsc(x);
    G2(g2_move,xv,ay);
    if (2*(i/2)==i) {
      sprintf(str, format, x);
      G2(g2_line_to,xv,ay+8);
      G3(g2_string, xv-4*strlen(str),ay-25, str);
      G2(g2_move,   xv,by-8);
    }
    else {
      G2(g2_line_to,xv,ay+4);
      G2(g2_move,   xv,by-4);
    }
    G2(g2_line_to,xv,by);
  }

  /* y-axis */
  /* ------ */
  yd = AxisInc(ymax-ymin);
  GetFormat(format, ymin, ymax-ymin);
  for (y=ymin,i=0; y<=ymax; y+=yd,i++) {
    yv = ysc(y);
    G2(g2_move,ax,yv);
    if (2*(i/2)==i) {
      sprintf(str, format, y);
      G2(g2_line_to,ax+8, yv);
      G3(g2_string, ax-10*(strlen(str)+1),yv-7, str);
      G2(g2_move,   bx-8, yv);
    }
    else {
      G2(g2_line_to,ax+4,yv);
      G2(g2_move,   bx-4,yv);
    }
    G2(g2_line_to,bx,yv);
  }
}

void cpgdraw(float x, float y)
{
  // move with pen down to
  float xv,yv;
  xv = xsc(x);
  yv = ysc(y);
  G2(g2_line_to,xv, yv);
  OD4("g2_line_to: %g,%g\t%g,%g\n", x,y, xv,yv);
}

void cpglab(const char *xlbl, const char *ylbl, const char *toplbl)
{
  // 3 labels x label, y label, top label
  G3(g2_string, tpos[0][0]-4*strlen(xlbl), tpos[0][1], ((char*) xlbl));
  G3(g2_string, tpos[1][0], tpos[1][1], ((char*) ylbl));
  G3(g2_string, tpos[2][0]-4*strlen(toplbl), tpos[2][1], ((char*) toplbl));
}

void cpgmove(float x, float y)
{
  // move pen up to (x,y)
  float xv, yv;
  xv = xsc(x);
  yv = ysc(y);
  G2(g2_move,xv, yv);

  OD4("g2_move: %g,%g\t%g,%g\n", x,y, xv, yv);
}


void cpgpt1(float x, float y, int symbol)
{
  // draw symbol at (xpt, ypt)
  float xl,xr,yl,yr;
  x = XSC(x);
  y = YSC(y);
  if (x < 0 || y < 0 || x > WINX || y > WINY) return;
  OD3("pt: %g,%g %i\n", x,y, symbol);
  switch (symbol) {
  case 23: // cross
    xl = x-1;
    xr = x+1;
    yl = y-1;
    yr = y+1;
    G4(g2_line, xl,xr, yl,yr);
    G4(g2_line, xl,xr, yr,yl);
    break;
  case 22: // circle
    G3(g2_circle, x, y, 3);
    break;
  default: // point
    G2(g2_plot, x,y);
  }
}

void cpgsch(float size)
{
  // set character height, immer 1.2
}

#define MYCOL(i) ((i<2||i>26) ? 1 : (i<8 ? myC[i-2] : i))
#define GCOL(i) ((i<2||i>26) ? 1 : (i<8 ? gC[i-2] : i))
void cpgsci(int ci)
{
  // set color index
  G0(g2_flush);
  g2_pen(dev, MYCOL(ci));
#if defined(DO_GIF) || defined(DO_PS)
  g2_pen(gdev, GCOL(ci));
#endif
  OD2("pen: %i %i\n",ci,i);
}

void cpgcirc(float xcenter, float ycenter, float radius)
{
  float x,y;
  x = xsc(xcenter);
  y = ysc(ycenter);
  G3(g2_circle, x, y, radius);
}

void cpgsfs(int fs)
{
  float ffs = (float)(fs*10.0);
  G1(g2_set_font_size, ffs);
}

void cpgslw(int lw)
{
  float llw = (float)(lw*1.0);
  G1(g2_set_line_width, llw);
}
