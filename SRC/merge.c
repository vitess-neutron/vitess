/*
Merge VITESS trajectory output data:

Read interaction event files f1,f2,..,fn

Events are sorted by id, first only per event file.

To keep memory usage at bay, we
- open all files at once
- read an event from e1 with id id1, and
- process all events with id1

We start with reading an event from f1, which got the ID id, and define
a trajectory with this event.
We read on from f1 and add all events, which have the same ID.
The move on to f2 and read events here.
If for event e2 from f2
- id == id2 add this to the trajectory t, and read on from f2
- id > id2 should be impossible, skip this event, and read on
- id < id2 move on to f3, keep e2

M. Fromme August 2011

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// maximal number of events with a given id
#define MAXTRAJ 1024

typedef struct {
  float pos[3];
  float lambda;
  int id;
  int color;
  int reason;
  int spin;
} t_traj_point;

int ntraj, // number of trajectories found
  maxtraj; // maximal number of points found for a processed trajectory

t_traj_point traj[MAXTRAJ];

#define MAXRBUF 64
int maxfile; // maximal index of input file
FILE *outf, *rf[MAXRBUF];

int isbuf[MAXRBUF]; // to indicate we have a point in the preread array to process
t_traj_point preread[MAXRBUF];

char *outfilename;

int outfilesel=1;
int svg_width = 800;
int svg_height = 600;
char *svg_line_color = "#448"; // dark blue

/*
   x along neutron beam
   z vertical axis
   y form a horizontal plane with x axis
   side view: y projection respective selection of x,z
   top view: z projection respective selection of x,y
*/

#define SIDEVIEW 0
#define TOPVIEW 1
int instrument_view; // default 0, view x,z


void usage () {
  printf("usage:\nmerge [option] outfile infile1 infile2 {infile}\n"
         "\twith a maximum of %d input files\n"
         "option may be\n"
         "-x\t\tto write a X3D file\n"
         "-s\t\tto write a SVG file, default\n",
         MAXRBUF);
  exit(0);
}


void myexit(const char *s) {
  printf ("%s!\n", s);
  exit (2);
}


t_traj_point *readPoint(int stage) {
  if (isbuf[stage]) {
    isbuf[stage] = 0;
  } else {
    if (1 != fread(preread + stage, sizeof(preread[0]), 1, rf[stage]))
      return 0;
  }
  return preread + stage;
}

void putBack(int stage) {
  isbuf[stage] = 1;
}


void addPoint(t_traj_point *rp) {
  ++maxtraj;
  if (maxtraj >= MAXTRAJ)
    myexit("trajectory with too many points encountered");
  memcpy(traj + maxtraj, rp, sizeof(*rp));
}

int startSVGFile(int width, int height) {
  int rc = fprintf(outf,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
          "     xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
          "     version=\"1.1\" baseProfile=\"full\"\n"
          "     viewBox=\"-10 -10 %d %d\">\n"
          "<defs>\n"
          "<script type=\"text/javascript\">\n"
          "function meldung(n){alert(\"trajectory \" + n)}\n"
          "</script>\n",
          svg_width, svg_height);
  return rc == 2;
}

int finishSVGFile() {
  int i;
  fprintf(outf, "</defs>\n");
  for (i=1; i<=ntraj; i++)
    fprintf(outf, "<use id=\"uset%d\" xlink:href=\"#t%d\" onclick=\"meldung(%d);\"/>\"\n",
            i,i,i);
  fprintf(outf, "</svg>\n");
  return 1;
}

char * pretty(float v, char *s) {
  // write a float value with without superfluous leading and trailing chars
  int i;
  char *q, *p = s;
  sprintf(s, "%12.2f", v);
  while (isspace(*p))
    ++p;
  if ((q = strchr(p, '.'))) {
    // remove trailing zeroes
    for (i = strlen(q)-1; i>0; i--)
      if (q[i] == '0')
        q[i] = 0;
      else
        break;
    if (q[i] == '.')
        q[i] = 0;
  }
  return p;
}

void writeSVGTrajectory () {
  int i;
  char ba[16], bb[16];
  float *pos;
  ++ntraj;
  fprintf(outf, "<g id=\"t%d\"><polyline fill=\"none\" stroke=\"%s\" stroke-width=\"0.5\" points=\"",
          ntraj, svg_line_color);
  for (i=0; i<= maxtraj; i++) {
    pos = &(traj[i].pos[0]);
    fprintf(outf,"%s,%s",
            pretty(pos[0], ba),
            pretty(pos[instrument_view == SIDEVIEW ? 2 : 1], bb));
    if (i<maxtraj) fputc(',', outf);
  }
  fprintf(outf, "\"/></g>\n");
}

void writeTextTrajectory() {
  int i;
  t_traj_point *p = traj;
  for (i=0; i<= maxtraj; i++) {
    printf("%d %g %g %g %d %d %g %d\n",
           p->id, p->pos[0], p->pos[1], p->pos[2],
           p->color, p->reason, p->lambda, p->spin);
    ++p;
  }

}


int main(int argc, char**argv) {

  int i, stage, tid;
  t_traj_point *rp;
  char *arg, *outfilename;

  if (argc <4)
    usage();

  ++argv;
  while ((arg = *argv++)) {
    if (*arg == '-') {
      switch (arg[1]) {
      case 's': outfilesel = 1; break;
      case 'x': outfilesel = 2; break;
      default : usage();
      }
    } else if (outfilename) {
      if (!(rf[maxfile] = fopen(arg, "r")))
        myexit("unable to open input file");
      ++maxfile;
    } else {
      outfilename = arg;
    }
  }

  if (!(outf = fopen(outfilename, "w")))
    myexit("unable to open output file");

  if (outfilesel == 1) {
    if (! startSVGFile(svg_width, svg_height))
      myexit("unable to write svg file");
  }


  while (1) {
    stage = 0;
    if (!(rp = readPoint(stage)))
      break;
    maxtraj = -1;
    tid = rp->id;
    addPoint(rp);
    while ((rp = readPoint(stage))) {
      if (tid == rp->id)
        addPoint(rp);
      else if (tid < rp->id)
        putBack(stage);
      break;
    }

    for (++stage; stage <= maxfile; ++stage) {
      while ((rp = readPoint(stage))) {
        if (tid < rp->id) {
          putBack(stage);
          break;
        }
        if (tid == rp->id)
          addPoint(rp);
      }
    }
    if (outfilesel == 1)
      writeSVGTrajectory();
    else
      writeTextTrajectory();
  }

  fclose(outf);
  for (i=0; i<=maxfile; i++)
    fclose(rf[i]);

  return 0;
}
