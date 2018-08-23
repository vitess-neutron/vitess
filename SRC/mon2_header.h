#define BINSIZE 1001
static double* by;
static double* bz;
static double binyz[BINSIZE][BINSIZE];
static double binyzerror[BINSIZE][BINSIZE];
static int binyzcounts[BINSIZE][BINSIZE];

void printFloatItem(double v, FILE*f) {
  static char buf[16];
  int i;
  sprintf(buf, "%10.7f", v);
  // cut off trailing zeroes
  for (i=9; i>=0; i--)
    if (buf[i] != '0')
      break;
  buf[i+1] = 0;
  fputs(buf,f);
  fputc(' ',f);
}

#define PrintFloat(a) { double v = a; if (v == 0) fputs("0 ",fmonitor); else printFloatItem(v,fmonitor); }
#define PrintItem(f,a) { double v = a; if (v == 0) fputs("0 ",fmonitor); else fprintf(fmonitor,f,v); }
#define Newline fputc('\n',fmonitor)

int WriteOutput(FILE* fmonitor, int format, int pWeight, int nbiny, int nbinz,
                const char *xaxis, const char *yaxis) {

  int dy, dz;

  fprintf(fmonitor,"#Monitor %s %s :%d:%s:%d:%s\n",
          format==0  ? "matrix" : "xyz",
          pWeight==0 ? "" : "weight",
          nbiny, xaxis, nbinz, yaxis);

  if (format == 0) {

    for (dy = 0; dy<nbiny; dy++)
      PrintFloat((by[dy] + by[dy+1]) / 2.0);
    Newline;

    for (dz = 0; dz<nbinz; dz++) {
      PrintItem("%5.3f ", (bz[dz] + bz[dz+1]) / 2.0);
      for (dy = 0; dy<nbiny; dy++)
        PrintItem("%5.3E ", binyz[dy][dz]);
      Newline;
    }

  } else {

    fputs("#x  y  z\n", fmonitor);
    for (dz = 0; dz<nbinz; dz++) {
      double y, error, binc;
      y = (bz[dz]+bz[dz+1]) / 2.0;
      for (dy = 0; dy<nbiny; dy++) {
        int c = binyzcounts[dy][dz];
        PrintFloat((by[dy]+by[dy+1]) / 2.0);
        PrintFloat(y);
        if (c <= 0)
          fputs("0 0 0\n", fmonitor);
        else {
          binc  = binyz[dy][dz];
          error = binc <= 0 ? 0 : binc * sqrt(1./c);
          PrintItem("%5.3E ", binc);
          PrintItem("%5.3E ", error);
          fprintf(fmonitor, "%d\n", c);
        }
      }
      Newline;
    }
  }

  fclose(fmonitor);

  return 1;
}

#undef PrintItem
#undef Newline
