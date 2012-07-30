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
#include <math.h>

#ifdef _MSC_VER
# include <float.h>
# define M_PI            3.14159265358979323846  /* pi */
# define M_PI_2          1.57079632679489661923  /* pi/2 */
# define ISNAN(x) _isnan(x)
#else
# define ISNAN(x) isnan(x)
#endif

typedef union {
  char *t;
  float pos[3];  // xyz coordinates, only x and second for SVG
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
int output_type;  // 0 text,  1 svg,  2 x3d, 3 x3d with geometry
char *geom_file;
int ids_from_all_files;

float xwlow = 0, xwhigh = 100;  // x range for SVG output, %
float scale2 = 1;             // scale factor for second dimension, SVG output

FILE *outf;

char *x3d_option_filename;
float viewport[3][2] = {{-1e5,1e5},{-1e5,1e5},{-1e5,1e5}};
int annotationLabels=1;        // if set, annotate X3D model with module names

int svg_width = 800;
int svg_height = 600;
const char *svg_line_color = "#448"; // dark blue
char strokeWB[16], *strokeWS;  // stroke width format

const char
// line green
  *LineAppearance="<Material diffuseColor='0 1 0'/>",
// circle gray
  *CircleAppearance="<Material diffuseColor='.7 .7 .7' specularColor='.2 .2 .2'/>",
  *CUBEMAT="<Material diffuseColor='.9 .9 0' emissiveColor='.1 .1 .33' transparency='.5'/>",
// rectangle, blue
  *RECTMAT="<Material diffuseColor='0.1 0.1 0.9' emissiveColor='.1 .1 .33' transparency='.5'/>",
  *TRIANGLEMAT="<Material diffuseColor='0.1 0.1 0.9' emissiveColor='.1 .1 .33' transparency='.5'/>",
  *CYLMAT="<Material diffuseColor='.9 .9 .1' emissiveColor='.1 .1 .9' transparency='.5'/>",
  *SPHEREMAT="<Material diffuseColor='.9 .1 .1' emissiveColor='.1 .1 .33' transparency='.5'/>",
  *HULLMAT="<Material diffuseColor='.3 .3 1' emissiveColor='.1 .1 .33' transparency='.5'/>",
  *ELLIPSMAT="<Material diffuseColor='.3 .1 .3' emissiveColor='.1 .1 .33' transparency='.5'/>",
  *ELLIPS2MAT= "<Material diffuseColor='0.2 0.6 0.5' emissiveColor='.1 .1 .33' transparency='.3'/>",

  *LABELMAT="<Material diffuseColor='1 1 .3' emissiveColor='.33 .33 .1'/>",
  *FONTSTYLE="<FontStyle DEF='label_font' family='\"SANS\"' justify='\"MIDDLE\" \"MIDDLE\"' size='.1'/>"
;

// Type enumeration of geometric elements

typedef enum {
  GT_Line      = 1, // line individual shape def, but common appearance
  GT_Rectangle = 2,
  GT_Circle    = 3,
  GT_Cuboid    = 4,
  GT_Cylinder  = 5,
  GT_Sphere    = 6,
  GT_Hull      = 7, // hull of a pyramid section, no top and bottom plane
                    // given by length, bottom width,height and top width,height
  GT_HollowCylinder = 8,  // cylinder without inner cylinder
  GT_Hull6          = 9,  // hull, given by length, bottom w1,w2,h, top w1,w2,h
  GT_Hull8          = 10, // hull, given by length, bottom w1,w2,h1,h2, top w1,w2,h1,h2
  GT_Ellipsoid      = 11, // cut of an ellipsoid
  GT_OpenRectangle  = 12, // rectangle with spare inner rectangle
  GT_Triangle       = 13, // triangle given by 3 * x,y,z coordinates
  GT_CylSlice       = 14  // slice from a cylinder hull
} GType;

// GTMAX must be highest number of GType
#define GTMAX 14

/*
   x along neutron beam
   z vertical axis
   y forms a horizontal plane with x axis
   side view: y projection respective selection of x,z
   top view: z projection respective selection of x,y
*/

int xz_view; // default 0, view x,y

#define toRad(a) (float)(a*(M_PI/180.0))

void usage() {
  printf("usage:\n"
         "sortiap {option} [-o outfile] {infile}\n"
         "\tinfile\t\tzero ore more names of input files with trajectories\n"
         "\t-o outfile\tresult file, default stdout\n"
         "\n\toption\tmay be\n"
         "\t-a\tread ids from all input files, default is to assume all ids\n\t\t\tare to be seen in the first file\n"
         "\t-f setupfile\n\t\tload a file with x3d options\n"
         "\t-s\tSVG x,y output\n"
         "\t-S geometry\tSVG output with instrument geometry\n"
         "\t-z\toption for SVG to output x,z coordinates\n"
         "\t-x\tX3D output\n"
         "\t-X geometry\tX3D output, with geometry file insert\n"
         "\t--xlow val\trestrict x range, default 0 %%\n"
         "\t--xhigh val\trestrict x range, default 100 %%\n"
         "\t--scale val\tscale factor for second dimension, default 1\n"
         "sortiap sorts point output from VITESS trajectory runs.\n"
         "The default is plain text output for further processing.\n"
         );
  exit(0);
}

#define myexit(s) {fputs(s,stderr); exit(2);}
#define myexit1(s,a) {fprintf(stderr,s,a); exit(2);}
#define myexit2(s,a,b) {fprintf(stderr,s,a,b); exit(2);}

void setVF(float *v, const char *s) {
  if (1 > sscanf(s, "%f", v))
    myexit("unable to read x3d option from file\n");
}

void setVI(int *v, const char *s) {
  if (1 > sscanf(s, "%d", v))
    myexit("unable to read x3d option from file\n");
}

#define GVF(a) if (strcmp(line+1, #a)==0) setVF(&a,p)
#define GVI(a,b) if (strcmp(line+1, #a)==0) setVI(&(b),p)
#define GVV(a,b) if (strcmp(line+1, #a)==0) setVF(&(b),p)
#define GVS(a,b) if (strcmp(line+1, #a)==0) b=strdup(p)

void parseX3dOptionFile() {
  FILE *xf;
  char *p, *q, line[256];
  int v;
  if (! x3d_option_filename) return;
  xf = fopen(x3d_option_filename, "r");
  while (fgets(line, 255, xf)) {
    if ((p = strchr(line, '#')))
      *p = 0;
    if (! (p = strchr(line, '='))) continue;
    *p++ = 0;
    q = line;
    while ((v = *q))
      *q++ = tolower(v);
    // chomp, reduce to printable characters
    q = p;
    while ((v = *q) && isprint(v))
      ++q;
    *q = 0;
    switch (line[0]) {
    case 'c':
      GVS(ylmat,CYLMAT);
      GVS(ircleappearance,CircleAppearance);
      break;
    case 'e':
      GVS(llipsmat,ELLIPSMAT);
      GVS(llips2mat,ELLIPS2MAT);
      break;
    case 'f':
      GVS(ontstyle,FONTSTYLE);
      break;
    case 'h':
      GVS(ullmat,HULLMAT);
      break;
    case 'l':
      GVI(abels,annotationLabels);
      GVS(abelmat,LABELMAT);
      GVS(ineappearance,LineAppearance);
      break;
    case 'r':
      GVS(ectmat,RECTMAT);
      break;
    case 's':
      GVS(pheremat,SPHEREMAT);
      break;
    case 't':
      GVS(rianglemat,TRIANGLEMAT);
      break;
    case 'x':
      GVV(low,  viewport[0][0]);
      GVV(high, viewport[0][1]);
      break;
    case 'y':
      GVV(low,  viewport[1][0]);
      GVV(high, viewport[1][1]);
      break;
    case 'z':
      GVV(low,  viewport[2][0]);
      GVV(high, viewport[2][1]);
      break;
    default: ;
    }
  }
  fclose(xf);
}

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
  int color;
  float lambda, weight;
  p_point p = calloc(1, sizeof(t_point));
  if (output_type) {
    // SVG or X3D
    if (6 != sscanf(data, "%d %f %f %f %f %f", &color, &lambda, &weight,
                    &(p->u.pos[0]), &(p->u.pos[1]), &(p->u.pos[2])))
      myexit1("insufficient point data %s\n", data);
    // special for SVG
    if (output_type == 1 && xz_view == 1)
      p->u.pos[1] = p->u.pos[2];
  } else
    // text
    p->u.t = strdup(data);

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

char * sS(float v, char *s, const char *format);

#define sS2(a,b) sS(a,b, "%12.2f")
#define sS3(a,b) sS(a,b, "%12.3f")
#define sS4(a,b) sS(a,b, "%12.4f")
#define sS5(a,b) sS(a,b, "%12.5f")

char * sS(float v, char *s, const char *format) {
  // convert a float value to text, without superfluous leading or trailing characters
  int i;
  char *q, *p = s;
  sprintf(s, format, v);
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
    // restrict leading 0. to .
    if (p[0] == '0' && p[1] == '.')
      ++p;
    // restrict leading -0. to -.
    else if (p[0] == '-' && p[1] == '0' && p[2] == '.') {
      ++p;
      *p = '-';
    }
  }
  // -0 becomes 0
  if (p[0] == '-' && p[1] == '0' && p[2] == 0) {
    p[0] = '0';
    p[1] = 0;
  }
  return p;
}

void minMax(float range[2], float v) {
  if (v < range[0])  range[0] = v;
  if (v > range[1])  range[1] = v;
}

float fMod(float a, float m) {
  if (m < 0) m = -m;
  while (a < 0) a += m;
  while (a > m) a -= m;
  return a;
}

void genColor (int n, char *s) {
  float ang, r,g,b, v;
  char b1[16], b2[16], b3[16];
  ang = fMod((float)n, 360.0f);
  r = g = b = 0.0f;
  if (ang <= 120) {
    g = v = ang / 120.0f;
    r = 1.0f - v;
  } else if (ang <= 240) {
    b = v = (ang - 120.0f) / 120.0f;
    g = 1.0f - v;
  } else {
    r = v = (ang - 240.0f) / 120.0f;
    b = 1.0f - v;
  }
  sprintf(s, "%s %s %s",
          sS2(r, b1),
          sS2(g, b2),
          sS2(b, b3));
}

#define CrossProd(a1,a2,a3, u1,u2,u3, v1,v2,v3) a1 = u2*v3 - u3*v2; a2 = u3*v1 - u1*v3; a3 = u1*v2 - u2*v1;
#define ScalProd(u1,u2,u3, v1,v2,v3) u1*v1 + u2*v2 + u3*v3

void rotString (float u1, float u2, float u3,
                float v1, float v2, float v3,
                char *s) {
  // Compute the rotation to transform the normalized vector (u1,u2,u3) to (v1,v2,v3) :
  // The cross product defines a perpendicular axis, we take that as rotation axis.
  // The scalar product defines the angle.

  char s1[16], s2[16], s3[16], s4[16];
  double a1,a2,a3, ang;

  // normalize v
  float v, vfactor;
  v = v1*v1 + v2*v2 + v3*v3;
  if (v == 0) {
    strcpy(s, "1 0 0 0");
    return;
  }
  vfactor = (float)(1.0 / sqrt(v));
  v1 *= vfactor;
  v2 *= vfactor;
  v3 *= vfactor;

  CrossProd(a1, a2, a3, u1,u2,u3, v1,v2,v3);
  ang = acos(ScalProd(u1,u2,u3, v1,v2,v3));
  sprintf(s, "%s %s %s %s",
          sS5((float)a1, s1), sS5((float)a2, s2),
          sS5((float)a3, s3), sS5((float)ang, s4));
}


const char *x3d_old[GTMAX+1], *x3d_new[GTMAX+1];

void defineMaterials () {
  static char buf[256];
  sprintf(buf, "<Appearance>%s</Appearance>", LineAppearance);
  x3d_old[GT_Line] = x3d_new[GT_Line] = strdup(buf);

  sprintf(buf, "<Appearance>%s</Appearance>", CircleAppearance);
  x3d_old[GT_Circle] = x3d_new[GT_Circle] = strdup(buf);

  x3d_old[GT_Cuboid] = "<Shape USE='cuboid'/>";
  sprintf(buf, "<Shape DEF='cuboid'><Box/><Appearance>%s</Appearance></Shape>", CUBEMAT);
  x3d_new[GT_Cuboid] = strdup(buf);

  x3d_old[GT_Cylinder] = "<Shape USE='cylinder'/>";
  sprintf(buf, "<Shape DEF='cylinder'><Cylinder/><Appearance>%s</Appearance></Shape>", CYLMAT);
  x3d_new[GT_Cylinder] = strdup(buf);

  x3d_old[GT_Sphere] = "<Shape USE='sphere'/>";
  sprintf(buf, "<Shape DEF='sphere'><Sphere/><Appearance>%s</Appearance></Shape>", SPHEREMAT);
  x3d_new[GT_Sphere] = strdup(buf);

  x3d_old[GT_Triangle] = "<Appearance USE='triangle'/>";
  sprintf(buf, "<Appearance DEF='triangle'>%s</Appearance>", TRIANGLEMAT);
  x3d_new[GT_Triangle] = strdup(buf);

  x3d_old[GT_Rectangle] = "<Shape USE='rectangle'/>";
  sprintf(buf, "<Shape DEF='rectangle'><Rectangle2D/><Appearance>%s</Appearance></Shape>", RECTMAT);
  x3d_new[GT_Rectangle] = strdup(buf);

}

void drawTriangle(float fa[9], const char *use) {
  int i;
  static char b1[16];
  fprintf (outf, "<Shape>%s<IndexedFaceSet coordIndex='0 1 2' solid='false'><Coordinate point='%s", use, sS5(fa[0], b1));
  for (i=1; i<9; i++)
    fprintf (outf, " %s", sS5(fa[i], b1));
  fputs ("'/></IndexedFaceSet></Shape>\n", outf);
}

void drawSimpleShape(float x, float y, float width, float height, const char *shape) {

  static char b1[16], b2[16], b3[16], b4[16];

  fprintf (outf, "<Transform scale='%s %s 1' translation='%s %s 0'>%s</Transform>\n",
           sS5(width/2.0f, b1), sS5(height/2.0f, b2),
           sS5(x, b3), sS5(y, b4),
           shape);
}

void drawRectangle(float *fa, const char *shape, const char *rots, float width, float height, float angle) {

  static char pre[48], b1[16], b2[16], b3[16], b4[16], b5[16];
  const char *post;

  if (angle != 0) {
    // the rectangle should be rotated by angle first
    // (0 0 1) is the orientation of 2D objects
    sprintf(pre, "<Transform rotation='0 0 1 %s'>", sS5(toRad(angle), b1));
    post = "</Transform>";
  } else {
    pre[0] = 0;
    post = "";
  }
  fprintf (outf, "<Transform scale='%s %s 1' rotation='%s' translation='%s %s %s'>%s%s%s</Transform>\n",
           sS5(width/2.0f, b1), sS5(height/2.0f, b2), rots,
           sS5(fa[0], b3), sS5(fa[1], b4), sS5(fa[2], b5),
           pre, shape, post);
}


void drawHollowCylinderShape(float inner_r) {

  const char *nuse, *use;
  char b[16], ss[256];
  static const char *ouse;

  if (ouse) {
    nuse = use = ouse;
  } else {
    sprintf(ss, "<Appearance DEF='cylcolor'>%s</Appearance>", ELLIPS2MAT);
    nuse = ss;
    ouse = use = "<Appearance USE='cylcolor'/>";
  }
  fprintf( outf,
          "<Transform rotation='1 0 0 1.57' translation='0 -1 0'>"
          "<Shape DEF='ring'>%s<Disk2D innerRadius='%s' outerRadius='1' solid='false'/></Shape>"
          "</Transform>"
          "<Shape>%s<Cylinder top='false' bottom='false' solid='false'/></Shape>"
          "<Shape>%s<Cylinder radius='0.5' top='false' bottom='false' solid='false'/></Shape>"
          "<Transform rotation='1 0 0 1.57' translation='0 1 0'><Shape USE='ring'/></Transform>\n",
          nuse, sS3(inner_r, b), use, use);
}

void printPoint(float x, float y) {
  char b[16];
  fputs(sS3(x,b), outf);
  fputc(' ', outf);
  fputs(sS3(y,b), outf);
  fputc(' ', outf);
}

void rotPoint(float *x, float *y, float sina, float cosa) {
  float a,b;
  a = *x;
  b = *y;
  *x = cosa*a - sina*b;
  *y = sina*a + cosa*b;
}


float toAng(float a) {
  if (a < 0) {
    return (float) (M_PI - acos(-a));
  }
  return (float) acos(a);
}


#define ROTATIONS 63

void drawCylSlice (float dirang, float openang) {
  float x, y, deltang, a, sina, cosa;

  // assure sensible angles
  if (dirang < 0   || dirang > 360) return;
  if (openang <= 0 || openang > 180) return;

  // compute first point of a line along a circular arc
  a = toRad(dirang - openang/2);
  sina = (float) sin(a);
  cosa = (float) cos(a);
  x = 1.0f;
  y = 0.0f;
  rotPoint(&x,&y, sina, cosa);

  // define the cylinder slice by extrusion of a circular arc line
  fputs("<Extrusion solid='false' beginCap='false' endCap='false' crossSection='", outf);
  if (openang < 2) {
    // use only 2 points, if the slice is smaller than 2 degree
    printPoint(x,y);
    a = toRad(openang);
    sina = (float) sin(a);
    cosa = (float) cos(a);
    rotPoint(&x,&y, sina, cosa);
    printPoint(x,y);
  } else {
    // one point per degree
    int j, npoints;
    npoints = (int) openang;
    deltang = openang / npoints;
    a = toRad(deltang);
    sina = (float) sin(a);
    cosa = (float) cos(a);
    for (j=0; j<=npoints; j++) {
      printPoint(x,y);
      rotPoint(&x,&y, sina, cosa);
    }
  }

  // extrusion spine along y axis
  fputs("' spine='0 -1 0 0 1 0' orientation='0 1 0 0 0 1 0 0'/>\n", outf);
}

void drawEllipsoidShape (float xlow, float xhigh) {
  float x, y, d, deltang, ang, sina, cosa, lowang,highang;
  static float scale[ROTATIONS];
  int i,j, cuts;
  char b1[16];

  // assure -1 < xlow < xhigh < 1
  if (xlow <= -1.0 || xlow >= xhigh || xhigh >= 1.0) return;

  lowang = toAng(xlow);
  highang = toAng(xhigh);

  d = lowang - highang;
  cuts = (int)(1 + d*(ROTATIONS + 1)/(2 * M_PI));

  deltang = d / cuts;

  ang = (float)((2*M_PI) / (ROTATIONS+1));
  sina = (float) sin(ang);
  cosa = (float) cos(ang);

  // we define the ellipsoid surface by extrusion of a circular plane
  fputs("<Extrusion solid='false' beginCap='false' endCap='false' crossSection='", outf);
  x = 1;
  y = 0;
  // cross section points on a unit circle
  for (j=0; j<=ROTATIONS; j++) {
    printPoint(x,y);
    rotPoint(&x,&y, sina, cosa);
  }
  printPoint(x,y);

  // spine points along x axis
  fputs("' spine='", outf);
  x = xlow;
  scale[0] = (float) sqrt(1.0 - x*x);
  ang = lowang;
  fprintf(outf, "%s 0 0 ", sS3(x,b1));
  for (i=1; i<=cuts; i++) {
    ang -= deltang;
    x = ang <= 0 ? 1 : (float) cos(ang);
    fprintf(outf, "%s 0 0 ", sS3(x,b1));
    scale[i] = (float) sqrt(1.0 - x*x);
  }

  // scale is an array of radius values for the corresponding y,z cuts
  fputs("' scale='", outf);
  for (i=0; i<=cuts; i++) {
    char *s = sS3(scale[i], b1);
    fprintf(outf, "%s %s ", s,s);
  }

  // the orientation is always the positive x direction
  fputs("' orientation='", outf);
  for (i=0; i<=cuts; i++)
    fputs("1 0 0 0 ", outf);
  fputs("'/>\n", outf);
}

int outOfView(float *fa) {
  int i;
  for (i=0; i<3; i++) {
    float v = fa[i];
    if (v < viewport[i][0] || v > viewport[i][1])
      return 1;
  }
  return 0;
}

void restrictPoint(float *fa) {
  int i;
  for (i=0; i<3; i++) {
    float v = fa[i];
    if (v < viewport[i][0])
      fa[i] = viewport[i][0];
    else if (v > viewport[i][1])
      fa[i] = viewport[i][1];
  }
}

#define MAXARGS 16

int parseGeomItem(FILE *gf, char *line, float fa[MAXARGS], int *ngeom, char **mods) {
  int vtype, len, slen, nargs, rc;
  char *rs, *p;

 skip_me:
  while ((rs = fgets(line,255,gf))) {
    if (strchr(line, '#')) continue;
    if ((p = strchr(line, ' ')) || (p = strchr(line, '\t')))
      break;
  }
  if (!rs) return 0;

  ++(*ngeom);
  *p++ = 0;
  len = strlen(line);
  vtype = -1;
  switch (*rs++) {
  case 'C':
    if (0 == strcmp(rs, "ircle")) {
      vtype = GT_Circle;  nargs = 9;
    } else if (0 == strcmp(rs, "uboid")) {
      vtype = GT_Cuboid;  nargs = 9;
    } else if (0 == strcmp(rs, "ylinder")) {
      vtype = GT_Cylinder;  nargs = 8;
    } else if (0 == strcmp(rs, "ylSlice")) {
      vtype = GT_CylSlice;  nargs = 11;
    }
    break;
  case 'E':
    if (0 == strcmp(rs, "llipsoid")) {
      vtype = GT_Ellipsoid;  nargs = 11;
    }
    break;
  case 'H':
    if (0 == strcmp(rs, "ull")) {
      vtype = GT_Hull;  nargs = 11;
    } else if (0 == strcmp(rs, "ull6")) {
      vtype = GT_Hull6;  nargs = 13;
    } else if (0 == strcmp(rs, "ull8")) {
      vtype = GT_Hull8;  nargs = 15;
    } else if (0 == strcmp(rs, "ollowCylinder")) {
      vtype = GT_HollowCylinder;  nargs = 9;
    }
    break;
  case 'L':
    if (0 == strcmp(rs, "ine")) {
      vtype = GT_Line;  nargs = 6;
    }
    break;
  case 'O':
    if (0 == strcmp(rs, "penRectangle")) {
      vtype = GT_OpenRectangle;  nargs = 10;
    }
    break;
  case 'R':
    if (0 == strcmp(rs, "ectangle")) {
      vtype = GT_Rectangle;  nargs = 9;
    }
    break;
  case 'S':
    if (0 == strcmp(rs, "phere")) {
      vtype = GT_Sphere;  nargs = 4;
    }
    break;
  case 'T':
    if (0 == strcmp(rs, "riangle")) {
      vtype = GT_Triangle;  nargs = 9;
    }
    break;
  }
  if (vtype < 0)
    return vtype;

  switch (nargs) {
  case  4: rc = sscanf(p, "%f %f %f %f%n", fa,fa+1,fa+2,fa+3, &slen); break;
  case  6: rc = sscanf(p, "%f %f %f %f %f %f%n", fa,fa+1,fa+2,fa+3,fa+4,fa+5, &slen); break;
  case  8: rc = sscanf(p, "%f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7, &slen); break;
  case  9: rc = sscanf(p, "%f %f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7,fa+8, &slen); break;
  case 10: rc = sscanf(p, "%f %f %f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7,fa+8,fa+9, &slen); break;
  case 11: rc = sscanf(p, "%f %f %f %f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7,fa+8,fa+9,fa+10, &slen); break;
  case 13: rc = sscanf(p, "%f %f %f %f %f %f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7,fa+8,fa+9,fa+10,fa+11,fa+12, &slen); break;
  case 15: rc = sscanf(p, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f%n",
                       fa,fa+1,fa+2,fa+3,fa+4,fa+5,fa+6,fa+7,fa+8,fa+9,fa+10,fa+11,fa+12,fa+13,fa+14, &slen); break;
  default: rc = 0;
  }
  if (rc != nargs) return -1;

  // mods is the zero terminated module name string
  p += slen;
  while (isspace(*p)) ++p;
  *mods = p;
  while (isprint(*p)) ++p;
  *p = 0;

  // apply viewport, if specified
  if (x3d_option_filename &&
      (outOfView(fa) || (vtype ==  GT_Line && outOfView(fa+3))))
    goto skip_me;
  return vtype;
}

#define swapYZ(y,z) v=fa[y]; fa[y]=fa[z]; fa[z] = -v
#define swapWH(w,h) v=fa[w]; fa[w]=fa[h]; fa[h] = v

static void vitessToX3Dcoordinates(float *fa, int vtype) {

  // Both VITESS and X3D use a right handed coordinate system, but
  // the VITESS y axis becomes the negative X3D z axis, and
  // the VITESS z axis becomes the X3D y axis :
  // x_x3d = x
  // y_x3d = z
  // z_x3d = -y
  // We use swapYZ to transform 3d coordinates, and swapWH to transform
  // width and height values.

  float v;

  swapYZ(1,2); // first point, always present

  if (vtype == GT_Line || vtype == GT_Triangle) {
    swapYZ(4,5); // 2. point
    if (vtype == GT_Triangle) {
      swapYZ(7,8); // 3. point
    }
  } else if (vtype != GT_Sphere) {
    // all these have a direction vector in elements 3,4,5
    swapYZ(4,5); // direction

    switch (vtype) {
    case GT_OpenRectangle:
      swapWH(8,9);
      // fall through
    case GT_Rectangle:
      swapWH(6,7);
      break;
    case GT_Ellipsoid:
    case GT_Cuboid:
    case GT_CylSlice:
      swapWH(7,8);
      break;
    case GT_Hull:
      swapWH(7,9);
      swapWH(8,10);
      break;
    default : ;
    }
  }

        
}

void geom2X3D(char *fn) {

  // parse a geometry file from VITESS to X3D output

  FILE *gf;
  char *mods, *scales;
  const char *shape;
  static char line[256], trans[64], scaleb[64], rots[64],
    b1[16], b2[16], b3[16], b4[16], b5[16], b6[16],
    used_before[GTMAX+1];  // denotes if a base geometric element has been defined so far
  static float fa[MAXARGS];
  int vtype, vvtype, ngeom = 0, firstmodule=1;

  defineMaterials();

  if (! *fn || ! (gf = fopen(fn, "r")))
    myexit("unable to read geometry file\n");

  // parse input of geometry file to X3D output

  while ((vtype = parseGeomItem(gf, line, fa, &ngeom, &mods))) {

    if (vtype < 0)
      myexit2("unknown geometry item in %s line\n%s\n", fn, line);

    vitessToX3Dcoordinates(fa, vtype);

    // first 3 values give x,y,z position
    sprintf(trans, "%s %s %s",
            sS5(fa[0], b1), sS5(fa[1], b2), sS5(fa[2], b3));
    
    // GT_Rectangle and GT_OpenRectangle use the same shape
    vvtype = vtype == GT_OpenRectangle ? GT_Rectangle : vtype;

    shape = used_before[vvtype] ? x3d_old[vvtype] : x3d_new[vvtype];
    used_before[vvtype] = 1;

    switch (vtype) {
    case GT_Line:
    fprintf(outf, "<Shape DEF='%s-%d'>%s<LineSet vertexCount='2' colorPerVertex='false'><Coordinate point='"
            "%s %s %s %s %s %s'/><Color color='1 0 0 1 0 0'/></LineSet></Shape>\n",
            mods, ngeom, shape,
            sS5(fa[0], b1), sS5(fa[1], b2), sS5(fa[2], b3),
            sS5(fa[3], b4), sS5(fa[4], b5), sS5(fa[5], b6) );
      break;
    case GT_Rectangle:
      // Orientation of 2D objects is 0,0,1
      rotString(0, 0, 1, fa[3], fa[4], fa[5], rots);
      drawRectangle(fa, shape, rots, fa[6], fa[7], fa[8]);
      break;
    case GT_OpenRectangle:
      // draw open rectangle as four adjacent rectangles
      { float smallw,smallh, w1,w2, h1,h2, xshift, yshift;
        // common rotation and translation for all 4 part rectangles
        rotString(0, 0, 1, fa[3], fa[4], fa[5], rots);
        fprintf (outf, "<Transform rotation='%s' translation='%s %s %s'>",
                 rots, sS5(fa[0], b3), sS5(fa[1], b4), sS5(fa[2], b5));
        w1 = fa[6]; w2 = fa[8];
        h1 = fa[7]; h2 = fa[9];
        smallw = (w1-w2)/2;
        xshift = (w2+smallw)/2;
        drawSimpleShape(-xshift, 0, smallw, h1, shape);
        shape = x3d_old[vvtype]; // at least now
        drawSimpleShape(xshift, 0, smallw, h1, shape);
        smallh = (h1-h2)/2;
        yshift = (h2+smallh)/2;
        drawSimpleShape(0, -yshift, w2, smallh, shape);
        drawSimpleShape(0, yshift, w2, smallh, shape);
        fputs ("</Transform>\n", outf);
      }
      break;
    case GT_Circle:
      /// X3D ArcClose2D has default radius 1 in x,y plane
      rotString(0, 0, 1, fa[3], fa[4], fa[5], rots);
      fprintf (outf,
               "<Transform rotation='%s' translation='%s'><Shape DEF='circle-%d'>%s"
               "<ArcClose2D closureType='PIE' radius='%s' startAngle='%s' endAngle='%s'/>"
               "</Shape></Transform>\n",
               rots, trans, ngeom, shape,
               sS5(fa[6], b1), sS5(toRad(fa[7]), b2), sS5(toRad(fa[8]), b3) );
      break;
    case GT_Cuboid:
      rotString(0, 1, 0, fa[3], fa[4], fa[5], rots);
      fprintf (outf, "<Transform scale='%s %s %s' translation='%s'>%s</Transform>\n",
               sS5(fa[6]/2.0f, b1), sS5(fa[7]/2.0f, b2), sS5(fa[9]/2.0f, b3), trans, shape);
      break;
    case GT_Cylinder:
      // X3D Cylinder has default orientation 0 1 0
      // 0 1 2  Ort
      // 3 4 5  Richtung
      // 6 7    Länge Radius
      scales = sS5(fa[7], scaleb);
      rotString(0, 1, 0, fa[3], fa[4], fa[5], rots);
      fprintf (outf, "<Transform scale='%s %s %s' rotation='%s' translation='%s'>%s</Transform>\n",
               scales, sS5(fa[6]/2.0f, b1), scales, rots, trans, shape);
      break;
    case GT_Sphere:
      scales = sS5(fa[3], scaleb);
      fprintf (outf, "<Transform scale='%s %s %s' translation='%s'>%s</Transform>\n",
               scales, scales, scales, trans, shape);
      break;
    case GT_Hull:
      // 6   Länge
      // 7   Eingangsbreite
      // 8   Ausgangsbreite
      // 9   Eingangshöhe
      // 10  Ausgangshöhe
      rotString(0, 1, 0, fa[3], fa[4], fa[5], rots);
      // direction has _4_ parameters, vector + angle
      fprintf (outf, "<Transform scale='%s 1 1' rotation='%s' translation='%s'>"
               "<Shape><Appearance>%s</Appearance>"
               "<Extrusion solid='false' beginCap='false' endCap='false' "
               "spine='0 -1 0 0 1 0' direction='0 1 0 0 0 1 0 0' "
               "scale='%s %s %s %s'/></Shape></Transform>\n",
               sS5(fa[6]/2.0f, b1), rots, trans, HULLMAT,
               sS5(fa[7]/2.0f, b2), sS5(fa[9]/2.0f, b3),
               sS5(fa[8]/2.0f, b4), sS5(fa[10]/2.0f, b5) );
      break;
    case GT_Hull6:
    case GT_Hull8:
      // not yet implemented
      break;
    case GT_Ellipsoid:
      rotString(1, 0, 0, fa[3], fa[4], fa[5], rots);
      fprintf (outf, "<Transform scale='%s %s %s' rotation='%s' translation='%s %s %s'><Shape>",
               sS5(fa[6]/2.0f, b1), sS5(fa[7]/2.0f, b2), sS5(fa[8]/2.0f, b3),
               rots,
               sS5(fa[0], b4), sS5(fa[1], b5), sS5(fa[2], b6) );
      drawEllipsoidShape(fa[9], fa[10]);
      fprintf (outf, "<Appearance>%s</Appearance></Shape></Transform>\n", ELLIPSMAT);
      break;
    case GT_CylSlice:
      rotString(0, 1, 0, fa[3], fa[4], fa[5], rots);
      fprintf (outf, "<Transform scale='%s %s %s' rotation='%s' translation='%s %s %s'><Shape>",
               sS5(fa[6]/2.0f, b1), sS5(fa[7]/2.0f, b2), sS5(fa[8]/2.0f, b3),
               rots,
               sS5(fa[0], b4), sS5(fa[1], b5), sS5(fa[2], b6) );
      drawCylSlice(fa[9], fa[10]);
      fprintf (outf, "<Appearance>%s</Appearance></Shape></Transform>\n", RECTMAT);
      break;
    case GT_HollowCylinder:
      rotString(0, 1, 0, fa[3], fa[4], fa[5], rots);
      scales = sS5(fa[7]/2.0f, b2);
      fprintf (outf, "<Transform scale='%s %s %s' rotation='%s' translation='%s %s %s'>",
               sS5(fa[6]/2.0f, b1), scales, scales,
               rots,
               sS5(fa[0], b4), sS5(fa[1], b5), sS5(fa[2], b6));
      drawHollowCylinderShape(fa[8]);
      fputs ("</Transform>\n", outf);
      break;
    case GT_Triangle:
      drawTriangle(fa, shape);
      break;
    }

    if (!annotationLabels) continue;

    // show module name
    if (firstmodule) {
      fprintf (outf, "<Transform translation='%s'><Billboard><Shape>"
               "<Appearance DEF='label_appearance'>%s</Appearance>"
               "<Text string='%s'>%s</Text></Shape></Billboard></Transform>\n",
               trans, LABELMAT, mods, FONTSTYLE);
      firstmodule = 0;
    } else
      fprintf (outf, "<Transform translation='%s'><Billboard><Shape>"
               "<Appearance USE='label_appearance'/>"
               "<Text string='%s'><FontStyle USE='label_font'/></Text></Shape></Billboard></Transform>\n",
               trans, mods);
    
  }
  fclose(gf);
}


void writeX3D() {
  int ntraj, count, id, mat;
  p_point p,q, first_p, last_p;
  static char buf[32], material_known[32];

  // start X3D file
  fputs( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 3.0//EN\" \"http://www.web3d.org/specifications/x3d-3.0.dtd\">\n"
         "<X3D profile='Interactive' version='3.0' xmlns:xsd='http://www.w3.org/2001/XMLSchema-instance' xsd:noNamespaceSchemaLocation='http://www.web3d.org/specifications/x3d-3.0.xsd'>\n"
         "<head>\n"
         "<meta content='VITESS experiment geometry plus trajectories' name='editors'/>\n"
         "</head>\n"
         "<Scene>\n"
         "<NavigationInfo type='\"EXAMINE\" \"ANY\"'/>\n",
         outf
         );

  if (geom_file)
    geom2X3D(geom_file);

  ntraj = 0;

  for (id=0; id<id_count; id++) {

    p = q = point_buffer[id];
    count = 0;
    last_p = 0;
    if (x3d_option_filename) {
      first_p = 0;
      while (p) {
        if (count) {
          last_p = p;
          ++count;
          if (outOfView(p->u.pos)) {
            // add a last point, but restrict to bounds
            restrictPoint(p->u.pos);
            break;
          }
        } else if (!outOfView(p->u.pos)) {
          count = 1;
          first_p = last_p = p;
        }
        if (!p->next) break;
        p = p->next;
      }
    } else {
      first_p = p;
      while (p) {
        ++count;
        if (!p->next) break;
        p = p->next;
      }
    }
    if (count <= 1) continue; // a single point is of no interest

    ++ntraj;
    mat = ntraj % 32;
    if (material_known[mat])
      fprintf(outf,
              "<Shape DEF='L-%d'><Appearance><Material USE='M-%d'/></Appearance>"
              "<LineSet vertexCount='%d'><Coordinate point='",
            ntraj, mat, count);
    else {
      genColor(mat, buf);
      fprintf(outf, "<Shape DEF='L-%d'>"
              "<Appearance><Material DEF='M-%d' diffuseColor='0 0 0' emissiveColor='%s'/></Appearance>"
              "<LineSet vertexCount='%d'><Coordinate point='",
              ntraj, mat, buf, count);
      material_known[mat] = 1;
    }

    for (p = first_p; p; p = p->next) {
      // transform VITESS coordinates to X3D coordinates
      fputs(sS4(p->u.pos[0], buf), outf);   // x_x3d = x
      putc(' ', outf);
      fputs(sS4(p->u.pos[2], buf), outf);   // y_x3d = z
      putc(' ', outf);
      fputs(sS4(- p->u.pos[1], buf), outf); // z_x3d = -y
      putc(' ', outf);

      if (p == last_p)
        break;
    }
    fseek(outf, -1, SEEK_CUR);
    fputs("'/></LineSet></Shape>\n", outf);
  }
  fputs("</Scene></X3D>\n", outf);
}

float SquarePoints[][3] = { {-1,-1,0}, {1,-1,0}, {1,1,0}, {-1,1,0} };
int SquareLines[][2] = { {0,1}, {1,2}, {2,3}, {3,0} };

float CubePoints[][3] = { {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
                          {-1,-1,1},  {1,-1,1},  {1,1,1},  {-1,1,1}  };
int CubeLines[][2] = { {0,1}, {1,2}, {2,3}, {3,0},  {0,4}, {1,5}, {2,6}, {3,7},  {4,5}, {5,6}, {6,7}, {7,4} };

void drawSVGCircle (float a, float b, float radius, const char *color) {
  char *x, *y, *r, buf[64];
  x = sS2(a, buf);
  y = sS2(b, buf+16);
  r = sS2(radius, buf+32);

  fprintf(outf, "<circle cx=\"%s\" cy=\"%s\" r=\"%s\" fill=\"%s\"/>\n",
          x, y, r, color);
}

void drawSVGL (float a1, float a2, float b1, float b2, const char *color) {
  char *x1, *x2, *y1, *y2, buf[64];
  x1 = sS2(a1, buf);
  x2 = sS2(b1, buf+16);
  y1 = sS2(scale2*a2, buf+32);
  y2 = sS2(scale2*b2, buf+48);

  if (0 == strcmp(x1, x2) && 0 == strcmp(y1,y2))
    return;  // ignore lines reduced to a point

  fprintf(outf, "<line x1=\"%s\" y1=\"%s\" x2=\"%s\" y2=\"%s\" "
          "style=\"stroke:%s;stroke-width:%s;stroke-linecap:round\"/>\n",
          x1, y1, x2, y2, color, strokeWS);
}

void drawSVGLine(float a[3], float b[3], const char *color) {
  int yind = xz_view ? 2 : 1;
  drawSVGL(a[0], a[yind], b[0], b[yind], color);
}


#define setM(a) memset(a, 0, 16*sizeof(a[0][0]))

void matMul (float res[4][4], float a[4][4], float b[4][4]) {
  int i,j;
  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      res[i][j] = a[i][0]*b[0][j] + a[i][1]*b[1][j] + a[i][2]*b[2][j] + a[i][3]*b[3][j];
}

void setScaledM(float m[4][4], float xsize, float ysize, float zsize) {
  setM(m);
  m[0][0] = xsize;
  m[1][1] = ysize;
  m[2][2] = zsize;
  m[3][3] = 1;
}

void applyDirM(float res[4][4], float m[4][4], float n[3], float angle) {
  float fcosa, cosa, sina, r[4][4];
  setM(r);
  sina = (float) sin(angle);
  cosa =(float) cos(angle);
  fcosa = 1.0f - cosa;
  r[0][0] = n[0]*n[0]*fcosa + cosa;
  r[0][1] = n[0]*n[1]*fcosa - n[2]*sina;
  r[0][2] = n[0]*n[2]*fcosa + n[1]*sina;
  r[1][0] = n[1]*n[0]*fcosa + n[2]*sina;
  r[1][1] = n[1]*n[1]*fcosa + cosa;
  r[1][2] = n[1]*n[2]*fcosa + n[0]*sina;
  r[2][0] = n[2]*n[0]*fcosa - n[1]*sina;
  r[2][1] = n[2]*n[1]*fcosa + n[0]*sina;
  r[2][2] = n[2]*n[2]*fcosa + cosa;
  r[3][3] = 1;

  matMul(res, r, m);
}

void normVec(float v[3]) {
  float d = 1.0f / (float) sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] *= d;
  v[1] *= d;
  v[2] *= d;
}

void setTransM(float m[4][4], float x, float y, float z) {
  m[0][3] = x;
  m[1][3] = y;
  m[2][3] = z;
}

void transPoint(float res[3], float t[4][4], float p[3]) {
  int i;
  for (i=0; i<3; i++)
    res[i] = t[i][0]*p[0] + t[i][1]*p[1] + t[i][2]*p[2] + t[i][3];
}

#define SWAP(a,b,v) v=a; a=b; b=v
#define IsEqual(a,b) (fabs(a-b) < 1.e-6)

void addLine(float x1, float y1, float x2, float y2,
             float x[16][2], float y[16][2], int *known) {
  int k, maxk;
  float v;

  // ignore points
  if (IsEqual(x1,x2) && IsEqual(y1,y2)) return;

  // ignore doubles like (x,y), (u,v) and (u,v), (x,y)
  // sort lines to x, y order
  if (x1>x2 || (x1==x2 && y1>y2)) {
    SWAP(x1,x2,v);
    SWAP(y1,y2,v);
  }
  maxk = *known;
  if (maxk >= 16) return;
  for (k=0; k<maxk; k++)
    if (x[k][0] == x1 && x2 == x[k][1] &&
        y[k][0] == y1 && y2 == y[k][1])
      return;
  x[k][0] = x1;
  x[k][1] = x2;
  y[k][0] = y1;
  y[k][1] = y2;
  *known = maxk+1;
}


void drawSVGLines (float t[4][4], int n, float point[][3], int line[][2], const char *color) {
  float p1[3], p2[3];
  float x[16][2], y[16][2];
  int i, known, yind;
  known=0;
  yind = xz_view ? 2 : 1;

  for (i=0; i<n; i++) {
    transPoint(p1, t, &(point[line[i][0]][0]));
    transPoint(p2, t, &(point[line[i][1]][0]));
    // printf("%2d addline %g\t%g\t%g\t%g\n", i, p1[0], p1[yind], p2[0], p2[yind]);
    addLine(p1[0], p1[yind], p2[0], p2[yind], x, y, &known);
  }
  // printf("%d to %d\n", n,known);
  for (i=0; i<known; i++)
    drawSVGL(x[i][0], y[i][0], x[i][1], y[i][1], color);
}

void geom2SVG(char *fn) {

  // parse a geometry file from VITESS to SVG output

  FILE *gf;
  char *mods;
  static char line[256];
  static float fa[MAXARGS], n[3], mat[4][4], mat1[4][4];
  int vtype, ngeom = 0;

  if (! *fn || ! (gf = fopen(fn, "r")))
    myexit("unable to read geometry file\n");

  // parse input of geometry file to X3D output

  while ((vtype = parseGeomItem(gf, line, fa, &ngeom, &mods))) {
    if (vtype < 0)
      myexit1("unknown geometry item in %s\n", fn);

    // use z as second coordinate for xz-view
    if (xz_view) fa[1] = fa[2];

    ++ngeom;
    switch (vtype) {
    case 1: // line
      drawSVGLine(fa, fa+3, "green");
      break;
    case 2: // rectangle
    case 4: // cuboid
    case 5: // cylinder
    case 7: // hull = box without top and bottom
      if (vtype == 5) {
        // draw the enclosing cuboid
        //            half length, radius, radius
        setScaledM(mat1, fa[7]/2.0f, fa[6], fa[6]);
      } else {
        //  each half of    length  width      height
        setScaledM(mat1, fa[6]/2.0f, fa[7]/2.0f, fa[9]/2.0f);
      }
      // we rotate, so that the z axis becomes the given orientation (ox,oy,oz)
      // that is we rotate around (0,0,1) x (ox,oy,oz) by the scalar product (0,0,1)*(ox,oy,oz)
      CrossProd(n[0],n[1],n[2], 0,0,1, fa[3],fa[4],fa[5]);
      normVec(n);
      // scalar product (0,0,oz) = (0,0, fa[5]) here
      applyDirM(mat, mat1, n, fa[5]);
      setTransM(mat, fa[0], fa[1], fa[2]);
      if (vtype == 2)
        drawSVGLines(mat, 4, SquarePoints, SquareLines, "red");
      else
        drawSVGLines(mat, 12, CubePoints, CubeLines, (vtype == 5 ? "yellow" : "blue"));
      break;
    case 3: // circle
      // draw a line, if direction is x axis
      if (fa[3] != 1 || fa[4] != 0 || fa[5] != 0) continue;
      drawSVGL(fa[0], -fa[6], fa[0], fa[6], "red");
      break;
    case 6: // sphere
      drawSVGCircle(fa[0], fa[xz_view ? 2 : 1], fa[3], "red");
     break;
    }
  }
  fclose(gf);
}


void writeSVG() {

  int id, i, ntraj;
  int count = 0;
  p_point p,q;
  char ba[16], bb[16], bc[16], bd[16];
  float c1[2], c2[2], v, xlow, xhigh, xdelta, ydelta, d;

  // compute value range

  c1[0] = c2[0] = (float) svg_width;
  c1[1] = c2[1] = 0;

  for (id=0; id<id_count; id++)
    for (p = point_buffer[id]; p; p = p->next) {
      minMax(c1, p->u.pos[0]);
      v = p->u.pos[1] *= scale2;
      minMax(c2, v);
    }

  // We may scale and restrict the visualized part:
  // scale : factor for the second dimension, default 1
  // restrict : xwlow, xwhigh in %, relative to [min, max]

  xlow  = c1[0];
  if (xwlow == 0 && xwhigh == 100) {
    xhigh = c1[1];
  } else {
    float xlen, xfactor;
    xlen = c1[1] - c1[0];
    xfactor = xlen / 100.0f;
    xhigh = xlow + xwhigh*xfactor;
    xlow += xwlow*xfactor;

    for (id=0; id<id_count; id++) {
      p_point newlist, nold;
      float v1;
      newlist = nold = 0;
      for (p = point_buffer[id]; p; p = p->next) {
        v1 = p->u.pos[0];
        if (v1 < xlow || v1 > xhigh) continue;
        if (newlist) {
          nold->next = p;
          nold = p;
        } else
          newlist = nold = p;
      }
      if (nold)
        nold->next = 0;
      point_buffer[id] = newlist;
    }
  }

  xdelta =  (xhigh - xlow) / 100;
  // to have comparable dimensions, we force y values

  ydelta = (c2[1] - c2[0]) / 10;
  d = c2[1] - c2[0] + 2*ydelta;
  strokeWS = sS5(d/100, strokeWB);

  // start SVG file
  fprintf(outf,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
          "<svg xmlns=\"http://www.w3.org/2000/svg\"\n"
          "     xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:ev=\"http://www.w3.org/2001/xml-events\"\n"
          "     version=\"1.1\" baseProfile=\"full\"\n"
          "     viewBox=\"%s %s %s %s\">\n"
          "<defs><script type=\"text/javascript\">"
          "function meldung(n){alert(\"trajectory \" + n)}"
          "</script></defs>\n",
          sS2(xlow -xdelta, ba), sS2(c2[0]-ydelta, bb),
          sS2(xhigh+xdelta, bc), sS2(c2[1]+ydelta, bd) );

  if (geom_file)
    geom2SVG(geom_file);

  ntraj = 0;

  for (id=0; id<id_count; id++) {

    p = q = point_buffer[id];
    count = 0;
    while (p) {
      count++;
      if (!p->next) break;
      p = p->next;
    }
    if (count <= 1) continue; // a single point is of no interest

    ++ntraj;
    fprintf(outf, "<g id=\"t%d\"><polyline fill=\"none\" stroke=\"%s\" stroke-width=\"%s\" points=\"",
             ntraj, svg_line_color, strokeWS);

    i = 0;
    for (p = q; p; p = p->next) {
      float a,b;
      char *pa,*pb;
      a = p->u.pos[0];
      b = p->u.pos[1];
      pa = sS3(p->u.pos[0], ba);
      pb = sS3(b, bb);
      fprintf(outf,"%s,%s", pa, pb);
      if (++i < count) fputc(',', outf);
    }
    fputs("\"/></g>\n", outf);
  }

  // finish SVG file
  for (i=1; i<=ntraj; i++)
    fprintf(outf, "<use id=\"uset%d\" xlink:href=\"#t%d\" onclick=\"meldung(%d);\"/>\"\n",
            i,i,i);
  fputs("</svg>\n", outf);
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
  int i, id, maxIdFile;
  FILE *f;
  static char *outfilename;
  static char *infilename[128];
  static int infilecount;
  float fv;

  ++argv; // skip program name

  while ((arg = *argv++)) {
    if (*arg == '-') {
      switch (arg[1]) {
      case '-':
        p = arg+2;
        arg = *argv++;
        if (!arg || 1 != sscanf(arg, "%f", &fv))
          usage();
        if (0 == strcmp("xlow", p))
          xwlow = fv;
        else if (0 == strcmp("xhigh", p))
          xwhigh = fv;
        else if (0 == strcmp("scale", p))
          scale2 = fv;
        else
          usage();
        break;
      case 'a': ids_from_all_files = 1; break;
      case 'f' : x3d_option_filename = *argv++;
        if (!x3d_option_filename || !*x3d_option_filename) usage();
        break;
      case 'o' : outfilename = *argv++;
        if (!outfilename || !*outfilename) usage();
        break;
      case 's': output_type = 1; break;
      case 'S': output_type = 1; geom_file = *argv++; break;
      case 'x': output_type = 2; break;
      case 'X': output_type = 3; geom_file = *argv++; break;
      case 'z': xz_view = 1; break;
      default: usage();
      }
    } else {
      if (infilecount >= 128) myexit("too many input files\n");
      infilename[infilecount++] = arg;
    }
  }

  if (infilecount < 1 && (output_type < 1 || output_type > 3))
    usage();

  if (outfilename) {
    outf = fopen(outfilename, "w");
    if (!outf) myexit1("unable to write file %s\n", outfilename);
  } else
    outf = stdout;

  if (output_type != 1) parseX3dOptionFile();

  if (infilecount < 1) {
    // just transform the experiment geometry
    if (output_type == 1)
      writeSVG();
    else
      writeX3D();
    if (outf != stdout)
      fclose(outf);
    return 0;
  }

  maxIdFile = ids_from_all_files ? infilecount-1 : 0;

  // count neutron ids from files

  for (i=0; i<=maxIdFile; i++) {
    if (! (f = fopen(infilename[i], "r")))
      myexit1("unable to read %s\n", infilename[i]);
    while (fgets(line,255,f)) {
      int len;
      if (strchr(line, '#')) continue;
      id_count++;
      if ((p = strchr(line, ' ')) || (p = strchr(line, '\t'))) {
        *p = 0;
        len = strlen(line);
        if (id_len) {
          if (len != id_len)
            myexit1("insufficient neutron ids in %s\n",infilename[i]);
        } else
          id_len = len;
      } else
        myexit1("insufficient input in %s\n",infilename[i]);
    }
    fclose(f);
  }

  // read ids now
  id_buffer = bufp = malloc(id_len*id_count);

  for (i=0; i<=maxIdFile; i++) {
    if (! (f = fopen(infilename[i], "r")))
      myexit1("unable to read %s\n", infilename[i]);
    while (fgets(line,255,f)) {
      if (strchr(line, '#')) continue;
      memcpy(bufp, line, id_len);
      bufp += id_len;
    }
    fclose(f);
  }

  // sort ids
  qsort((void *)id_buffer, id_count, (size_t) id_len, compIDs);

  if (maxIdFile > 0) {
    // eliminate double id entries
    int remain = 1;
    int gap = 0;
    char *n;
    p = id_buffer;
    n = p + id_len;
    for (i=1; i<id_count; i++) {
      if (0 == memcmp(p, n, id_len))
        gap = 1;
      else {
        ++remain;
        p += id_len;
        if (gap)
          memcpy(p, n, id_len);
      }
      n += id_len;
    }
    if (2*remain < id_count) {
      n = realloc(id_buffer, remain*id_len);
      id_buffer = n;
    }
    id_count = remain;
  }

  point_buffer = (p_point *) calloc(id_count, sizeof(p_point));

  for (i=0; i<infilecount; i++) {
    if (! (f = fopen(infilename[i], "r")))
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
  switch (output_type) {
  case 1:
    writeSVG();
    break;
  case 2:
  case 3:
    writeX3D();
    break;
  default:
    for (id=0; id<id_count; id++)
      writeTrajectory(id);
  }

  if (outf != stdout)
    fclose(outf);

  return 0;
}
