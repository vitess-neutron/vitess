#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "init.h"
#include "random_sampler.h"

#define LSIZ 130 

/* Randomly selects a set of nSampleSize neutrons from the trajectory file inFile
 and stores it in an outFilePtr.  */
void randomSampleFile(char* inFile, FILE* outFilePtr, int nSampleSize)
{
    // Declare variables
    int i, idx, RSIZ, nRnd;
    char line[LSIZ]; // Single line buffer instead of 2D array
    int nRows = 0;

    // Read Input File and store in array
    FILE* fp = fopen(inFile, "r");
    RSIZ = LinesInFile(fp);
    fprintf(LogFilePtr, "-- Sampling from %s \n", inFile);
    fprintf(LogFilePtr, "-- %d out of %d trajectories will be read.\n", nSampleSize, RSIZ);
    char** fileLines = (char**)malloc(RSIZ * sizeof(char*)); // Allocate memory for file lines

    for (i=0; i<4; i++){
        fgets(line, LSIZ, fp); // Read and discard 4 header rows
    }

    while (fgets(line, LSIZ, fp))
    {
        fileLines[nRows] = (char*)malloc(LSIZ * sizeof(char)); // Allocate memory for line
        strcpy(fileLines[nRows], line); // Copy line to array
        nRows++;
    }
    fclose(fp);

    // Randomly sample the Original file using gsl.
    gsl_rng *r;
    const gsl_rng_type * T;
    gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);
    for (idx = 0; idx < nSampleSize; idx++)
    {
        nRnd = gsl_rng_get(r) % nRows;
        fprintf(outFilePtr, " %s", fileLines[nRnd]);
    }
    rewind(outFilePtr);
    gsl_rng_free(r);

    // Free memory allocated for file lines
    for (i = 0; i < nRows; i++) {
        free(fileLines[i]);
    }
    free(fileLines);
}