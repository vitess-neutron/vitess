//*******************************************************************************************
// VITESS helper application sortiap
// Sort trajectories from individual files to a result file.
// Assume the first file has all neutron ids.
// Feb 2012 M. Fromme
//********************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef union {
  char *t;
  float pos[2];  // only 2 coordinates for SVG
} t_point_union;

// t_point is for a list of positions in a trajectory, here stored as strings
typedef struct s_point {
  t_point_union u;
  struct s_point *next;
} t_point, *p_point;

int id_len, id_count;
char *id_buffer;
p_point * point_buffer;

int len_factor=1;
int svg_output;

FILE *outf;

int svg_width = 800;
int svg_height = 600;
const char *svg_line_color = "#448"; // dark blue

/* 
   x along neutron beam
   z vertical axis
   y forms a horizontal plane with x axis
   side view: y projection respective selection of x,z
   top view: z projection respective selection of x,y
*/

#define SIDEVIEW 0
#define TOPVIEW 1
int instrument_view; // default 0, view x,z

void usage() {
  printf("usage:\n"
         "sortiap {option} [-o outfile] infile {infile}\n"
         "\tinfile\tone ore more input file names\n"
         "\t-o outfile\tresult file, default stdout\n"
         "\toption\tmay be\n"
         "\t\t-s\tSVG sideview output, default text\n"
         "\t\t-S\tSVG topview output\n"
         );
  exit(0);
}

#define myexit(s) {fprintf(stderr,s); exit(2);}
#define myexit1(s,a) {fprintf(stderr,s,a); exit(2);}

int compIDs(const void* p1, const void* p2) {
  return memcmp(p1, p2, id_len);
}

int searchId(const char *id) {

  // binary search for a given id in a sorted array of ids
  int ihalf, ilow, ihigh, rc;

  ilow = 0;
  ihigh = id_count - 1;
  while (ilow <= ihigh) {
    ihalf = (ilow+ihigh) / 2;
    rc = memcmp(id, id_buffer + ihalf*id_len, id_len);
    if (rc == 0) return ihalf;
    if (rc < 0)
      ihigh = ihalf - 1; // move left
    else
      ilow = ihalf + 1;  // move right
  }
  myexit1("unknown id %s encountered\n", id);
  return -1;  // never
}

p_point newPoint(const char *data) {
  p_point p;
  p = calloc(1, sizeof(t_point));
  if (svg_output) {
    int color;
    float lambda, pos[3];
    if (5 != sscanf(data, "%d %f %f %f %f", &color, &lambda, pos, pos+1, pos+2))
        myexit1("insufficient point data %s\n", data);
    p->u.pos[0] = pos[0];
    p->u.pos[1] = pos[instrument_view == SIDEVIEW ? 2 : 1];
  } else {
    p->u.t = strdup(data);
  }

  return p;
}

void insertPoint(const char *ids, const char *point_data) {
  p_point lp, next_lp;

  // search neutron id in list
  int id = searchId(ids);

  if ((lp = point_buffer[id])) {
    // find last point of trajectory so far and append new point
    while ((next_lp = lp->next))
      lp = next_lp;
    lp->next = newPoint(point_data);
  } else {
    point_buffer[id] = newPoint(point_data);
  }
}

char * pretty(float v, char *s) {
  // write a float value with without superfluous leading or trailing characters 
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

void minMax(int range[2], float v) {
  if (v < range[0])  range[0] = v;
  if (v > range[1])  range[1] = v;
}

void writeSVG() {

  int id, i, ntraj;
  int count = 0;
  p_point p,q;
  char ba[16], bb[16];
  int c1[2], c2[2];
    
  // compute value range

  c1[0] = c2[0] = svg_width;
  c1[1] = c2[1] = 0;

  for (id=0; id<id_count; id++) 
    for (p = point_buffer[id]; p; p = p->next) {
      minMax(c1, p->u.pos[0]);
      minMax(c2, p->u.pos[1]);
    }

  // start SVG file
  fprintf(outf,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
          "     xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
          "     version=\"1.1\" baseProfile=\"full\"\n"
          "     viewBox=\"%d %d %d %d\">\n"
          "<defs>\n"
          "<script type=\"text/javascript\">\n"
          "function meldung(n){alert(\"trajectory \" + n)}\n"
          "</script>\n",
          c1[0]-10, c2[0]-10,
          c1[1]+10, c2[1]+10);

  ntraj = 0;

  for (id=0; id<id_count; id++) {

    p = q = point_buffer[id];

    while (p) {
      count++;
      if (!p->next) break;
      p = p->next;
    }
    if (count <= 1) continue; // a single point is of no interest

    ++ntraj;
    fprintf(outf, "<g id=\"t%d\"><polyline fill=\"none\" stroke=\"%s\" stroke-width=\"0.5\" points=\"",
            ntraj, svg_line_color);

    i = 0;
    for (p = q; p; p = p->next) {
      fprintf(outf,"%s,%s",
              pretty(p->u.pos[0], ba),
              pretty(p->u.pos[1], bb));
      if (++i < count) fputc(',', outf); 
    }
    fprintf(outf, "\"/></g>\n");
  }

  // finish SVG file
  fprintf(outf, "</defs>\n");
  for (i=1; i<=ntraj; i++)
    fprintf(outf, "<use id=\"uset%d\" xlink:href=\"#t%d\" onclick=\"meldung(%d);\"/>\"\n",
            i,i,i);
  fprintf(outf, "</svg>\n");
}

void writeTrajectory(int i) {
  static char idstring[32]; // static to terminate string by 0  
  int count = 0;
  p_point p,q;

  p = q = point_buffer[i];

  while (p) {
    count++;
    if (!p->next) break;
    p = p->next;
  }
  if (count <= 1) return; // a single point is of no interest

  memcpy(idstring, id_buffer + id_len*i, id_len);
  fprintf(outf, "%d %s\n", count, idstring);
  for (p = q; p; p = p->next)
    fprintf(outf, "%s\n", p->u.t);
}


int main (int argc, char **argv) {

  char *arg, *p, *bufp, *s;
  static char line[256];
  int i, id;
  FILE *f;
  static char *outfilename;
  static char *infilename[128];
  static int infilecount;

  ++argv; // skip program name

  while ((arg = *argv++)) {
    if (*arg == '-') {
      switch (arg[1]) {
      case 's': svg_output = 1; instrument_view = SIDEVIEW; break;
      case 'S': svg_output = 1; instrument_view = TOPVIEW; break;
      case 'o' : outfilename = *argv++;
        if (!outfilename || !*outfilename) usage();
        break;
      default: usage();
      }
    } else {
      if (infilecount >= 128) myexit("too many input files\n");
      infilename[infilecount++] = arg;
    }
  }

  if (infilecount < 1) usage();

  if (outfilename) {
    outf = fopen(outfilename, "w");
    if (!outf) myexit1("unable to write file %s\n", outfilename);
  } else
    outf = stdout;

  // count neutron ids from the first file
  f = fopen(infilename[0], "r");
  if (!f) myexit1("unable to read %s\n", infilename[0]);
  while (fgets(line,255,f)) {
    int len;
    if (strchr(line, '#')) continue;
    id_count++;
    if ((p = strchr(line, ' ')) || (p = strchr(line, '\t'))) {
      *p = 0;
      len = strlen(line);
      if (id_len) {
        if (len != id_len)
          myexit1("insufficient neutron ids in %s\n",infilename[0]);
      } else
        id_len = len;
    } else
      myexit1("insufficient input in %s\n",infilename[0]);
  }

  // read ids now
  id_buffer = bufp = malloc(id_len*id_count);
  rewind(f);
  while (fgets(line,255,f)) {
    if (strchr(line, '#')) continue;
    memcpy(bufp, line, id_len);
    bufp += id_len;
  }

  // sort ids
  qsort((void *)id_buffer, id_count, (size_t) id_len, compIDs);

  point_buffer = (p_point *) calloc(id_count, sizeof(p_point));

  for (i=0; i<infilecount; i++) {
    if (i==0)
      rewind(f);
    else if (! (f = fopen(infilename[i], "r")))
      myexit1("unable to read %s\n", infilename[i]);

    while (fgets(line,255,f)) {
      int slen;
      if ((p = strchr(line, '#'))) {
        const char *tests = "unit_length=";
        if ((s = strstr(p,tests))) {
          // length units
          s += strlen(tests);
          if (s[0] == 'c' && s[1] == 'm')
            len_factor = 100;
          else if (s[0] == 'm' && s[1] == 'm')
            len_factor = 1000;
        }
        continue;
      }
      slen = strlen(line);
      if (slen < id_len+2)
        myexit1("insufficient data in input line :%s:\n", line);
      line[slen-1] = 0;  // remove \n
      insertPoint(line, line + id_len + 1);
    }
    fclose(f);
  }

  // write trajectories
  if (svg_output)
    writeSVG();
  else
    for (id=0; id<id_count; id++)
      writeTrajectory(id);

  if (outf != stdout)
    fclose(outf);

  return 0;
}

