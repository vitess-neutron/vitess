// merge VITESS spectra of the same kind
// M. Fromme HZB Oct. 2013

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

static void usage (void) {
  printf("\nusage:\n"
         "merge_spectra [option] resultfile infile1 infile2 [infile-n]\n\n"
         "Merges two or more input VITESS spectra from text files to a result file.\n"
         "option may be\n"
         "-f\t\tforce overwrite, per default resultfile must not exist\n"
         "-r rootdir\troot directory for all input files\n"
         "-n filename\tif given, all infile parameters are treated as directory names\n"
         "\t\twhere each directory contains this file\n"
         "-v\t\tbe verbose\n"
         "\n");
  exit(0);
}

#define NL "\n"
#define myexit1(s,a) {printf("\n" s "!\n" , a); exit(2); }
#define myexit2(s,a,b) {printf("\n" s "!\n" , a,b); exit(2); }
#define myexit4(s,a,b,c,d) {printf("\n" s "!\n" , a,b,c,d); exit(2); }

static char *ofn;    // output file name
static char **input_filename;    // array of input file names

// MAXS should be big enough to read a full matrix input line with 1+ycount values
// where ycount normally is <= 1000
#define MAXS 409600
static char buffer[MAXS];

static int skip_lines, rlines, cols, matrix,
  maxcols, number_count, infile_count, verbose,
  is_mergeable_text, is_monitor, is_xyz, weight;
static char *infilename, *rootdir;

static double *linevals, *values;

void resetToData(FILE *f) {
  int i;
  fseek(f, 0, SEEK_SET);
  for (i=0; i<skip_lines; i++)
    fgets(buffer, MAXS-1, f);
}

static FILE * openInFile(char *name) {
  FILE *f;
  char *fname = buffer;

  if (rootdir) {
    if (infilename)
      sprintf(buffer, "%s/%s/%s", rootdir, name, infilename);
    else
      sprintf(buffer, "%s/%s", rootdir, name);
  } else if (infilename)
    sprintf(buffer, "%s/%s", name, infilename);
  else
    fname = name;
  if (verbose)
    printf("open input file %s\n", fname);

  f = fopen(fname, "r");
  if (!f) myexit1("unable to read file %s", fname);
  return f;
}

static int colsInLine(const char *s) {
  // compute the number of columns in line s
  int v, c = 0, was_space = 1;
  while (1) {
    v = *s++;
    if (v == 0 || v == '\n') break;
    if (v == '\t' || isspace(v))
      was_space = 1;
    else if (!isprint(v))
      break;
    else {
      if (was_space)
        c++;
      was_space = 0;
    }
  }
  return c;
}

static void readValues(int cols, const char *s, double **vp) {
  double *p = *vp;
  int n;
  while (cols > 0) {
    if (1 == sscanf(s, "%lf%n", p, &n)) {
      cols--;
      s += n;
      p++;
    } else {
      myexit1("can not read real number in %s", s);
    }
  }
  *vp = p;
}


static int parseHeader(FILE *f) {

  char *s, *c;

  while ((s = fgets(buffer, MAXS-1, f))) {
    if (strchr(s,'#')) {
      skip_lines++;
      if (strstr(s,"Trajectories"))
        is_mergeable_text = 1;
      else if (strstr(s,"Monitor")) {
        is_monitor = 1;
        is_xyz = ((c = strstr(s,"x ")) != 0) &&
          ((c = strstr(c+2,"y ")) != 0) && (strchr(c,'z') != 0);
        weight = (strstr(s,"weight") != 0) || (strstr(s,"mean") != 0);
        matrix = strstr(s,"matrix") != 0;
      } else if (((c = strstr(s,"x ")) != 0) &&
                 ((c = strstr(c+2,"y ")) != 0) && (strchr(c,'z') != 0))
        is_xyz = 1;
      continue;
    }
    // we found the first non-comment line
    if (is_mergeable_text) break;
    if (is_xyz)
      cols = 5;
    else {
      cols = colsInLine(s);
      if (cols > 16) matrix = 1;
      if (matrix) {
        // mon2D format 0
        // skip this line with cols x-axis tic values
        skip_lines++;
        break;
      } else if (cols != 2 && cols != 4) {
        // unknown spectrum type
        return 0;
      }
    }
    break;
  }
  if (verbose)
    printf("cols %d  matrix %d  is_xzy %d weight %d\n", cols, matrix, is_xyz, weight);

  return 1;
}

static void mergeTextFiles(FILE *fo) {
  FILE *f;
  int i;

  // copy first input file contents to output file
  f = openInFile(input_filename[0]);
  while (fgets(buffer,MAXS-1, f))
    fputs(buffer, fo);
  fclose(f);

  // add other files contents, skipping headers
  for (i=1; i<infile_count; i++) {
    int j;
    f = openInFile(input_filename[i]);
    for (j=0; j<skip_lines; j++)
      if (!fgets(buffer,MAXS-1, f)) myexit1("unable to read %s",input_filename[i]);
    while (fgets(buffer,MAXS-1, f))
      fputs(buffer, fo);
    fclose(f);
  }
}

static void prettyItem(FILE *fo, double v, int nl) {

  if (v == 0)
    fputs(nl ? "0\n" : "0 ", fo);
  else
    fprintf(fo, nl ? "%g\n" : "%g ", v);
}

static void prettyPrint(FILE *fo, int c, double *vp) {
  while (--c > 0)
    prettyItem(fo, *vp++, 0);
  prettyItem(fo, *vp, 1);
}

static void mergeFormFiles(FILE *fo) {

  // merge other files
  FILE *f;
  int infile, n, i,j;
  double mean, factor, *vp;

  for (infile=1; infile < infile_count; infile++) {
    f = openInFile(input_filename[infile]);
    // skip header lines
    for (i=0; i<skip_lines; i++) {
      if (! fgets(buffer,MAXS-1, f))
        myexit1("error reading header of %s",input_filename[infile]);
      if (infile == 1)
        fputs(buffer, fo);   // copy header lines to outfile
    }

    // merge data
    n = i = 0;
    while (fgets(buffer,MAXS-1, f))
      if (buffer[0] && buffer[0] != '\n') {
        i++;
        vp = linevals;
        readValues(maxcols, buffer, &vp);

        if (matrix) {
          // first item (y value) will not be changed but checked
          if (values[n] != linevals[0])
            myexit4("inconsistent matrix x values %g %g, n %d  i %d", values[n], linevals[0], n,i);
          for (j=1; j<=cols; j++)
            values[n+j] += linevals[j];
        } else if (is_xyz) {
          // ignore x,y
          // increment 3. and 5. column, 4. column error value computed later
          values[n+2] += linevals[2];
          values[n+4] += linevals[4];
        } else if (cols == 2) {
          values[n+1] += linevals[1];
        } else if (cols == 4) {
          // 2. column weighted count
          values[n+1] += linevals[1];
          // 3. column computed later
          // 4. column raw count
          values[n+3] += linevals[3];
        }
        n += maxcols;
      }

    fclose(f);

    // copy merged data to outfile

    factor = 1.0 / infile_count;

    if (matrix) {

      for (n=0; n < number_count; n += maxcols) {
        // first value is y axis tic-value, followed by cols counts
        // normalize counts to average
        for (j=1; j<cols; j++)
          values[n+j] *= factor;
        prettyPrint(fo, maxcols, values + n);
      }

    } else if (cols == 2) {

      for (n=0; n < number_count; n += 2) {
        values[n+1] *= factor;
        prettyPrint(fo, 2, values + n);
      }

    } else if (cols == 4) {

      for (n=0; n < number_count; n += 4) {
        if (values[n+3] <= 0)
          fprintf(fo, "%g 0 0 0\n", values[n]);
        else {
          mean = values[n+1] * factor;
          // 2. column = mean value  if weight
          if (weight) values[n+1] = mean;
          // 3. column = statistical error
          values[n+2] = mean * sqrt(1.0 / values[n+3]);
          prettyPrint(fo, 4, values + n);
        }
      }

    } else if (is_xyz) {

      // keep 1. column (x), 2. column (y) and 5. column (absolute count)
      for (n=0; n < number_count; n += 5)
        if (values[n+4] <= 0)
          fprintf(fo, "%g %g 0 0 0\n", values[n], values[n+1]);
        else {
          mean = values[n+2] * factor;
          // 3. column = mean value  if weight
          if (weight) values[n+2] = mean;
          // 4. column = statistical error
          values[n+3] = mean * sqrt(1.0 / values[n+4]);
          prettyPrint(fo, 5, values + n);
        }

    } else {
      myexit1("unexpected cols %d!", cols);
    }
  }
}


static void mergeFiles() {

  double *vp;
  FILE *f, *fo;

  // read first spectrum

  f = openInFile(input_filename[0]);

  if (!parseHeader(f))
    myexit1("error with header of %s",  input_filename[0]);

  if (verbose)
    printf("%d header lines,  %d lines,  %d cols  of input  %s\n",
           skip_lines, rlines, cols, input_filename[0]);

  if (!(fo = fopen(ofn, "w"))) myexit1("unable to open output file %s", ofn);

  if (is_mergeable_text) {

    fclose(f);
    mergeTextFiles(fo);

  } else {

    // count rest lines
    resetToData(f);
    rlines = 0;
    while (fgets(buffer,MAXS-1,f))
      if (buffer[0] && buffer[0] != '\n')
        rlines++;

    if (matrix)
      // We have cols y values as part of the header lines,
      // following are lines with a x value and cols counts
      maxcols = 1 + cols; // x value + cols counts
    else
      maxcols = cols;

    number_count = maxcols * rlines;
    vp = values = (double*) malloc(number_count*sizeof(double));
    linevals = (double*) malloc(maxcols*sizeof(double));

    // now read numbers
    resetToData(f);

    while (fgets(buffer,MAXS-1, f))
      if (buffer[0] && buffer[0] != '\n')
        readValues(maxcols, buffer, &vp);

    fclose(f);

    mergeFormFiles(fo);

  }

  fclose(fo);
}


int main (int argc, char **argv) {
  static int force_overwrite;
  int slen;
  char *arg;
  FILE *f;

  if (argc < 4) usage();

  input_filename = (char **) calloc(argc, sizeof(char*));

  argv++;
  while ((arg = *argv++)) {
    if (arg[0] == '-') {
      switch (arg[1]) {
      case 'f':
        force_overwrite = 1; break;
      case 'n':
        infilename = *argv++; break;
      case 'r':
        rootdir = *argv++; break;
      case 'v':
        verbose = 1; break;
      default:
        usage();
      }
    } else if (ofn) {
      slen = strlen(arg);
      if (slen > 1 && arg[slen-1] == '/')
        arg[slen-1] = 0; // no slashes at end of file- or directory-names
      input_filename[infile_count++] = arg;
    } else {
      ofn = arg;
    }
  }
  if (!ofn || infile_count < 2) usage();

  if (rootdir) {
    slen = strlen(rootdir);
    if (rootdir[0] != '/' || slen < 2)
      myexit1("directory rootdir %s must be given with an absolute Path", rootdir);
    if (rootdir[slen-1] == '/')
      rootdir[slen-1] = 0; // remove trailing slash
  }
  if (!force_overwrite && (f=fopen(ofn,"r"))) {
    fclose(f);
    printf("output file %s already exists, use -f option!\n", ofn);
    usage();
  }

  mergeFiles();

  return 0;
}
