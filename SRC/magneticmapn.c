#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"

void writemagneticmap(char *FieldFileName1, long i_x_max, long i_y_max,
                      long i_z_max, double depth1, double width1, double height1, double field_hom1[3]) 
{ 
  long i_x, i_y, i_z;
  double Dim[3], Posit[3], fieldsph[3] ;
  FILE *FieldMapFile;

  fprintf(LogFilePtr,"Fill file for homogenous magnetic field\n");

  FieldMapFile=fopen(FieldFileName1, "w");

  /* print number of matrix lines, columns */
  fprintf(FieldMapFile, " %d    %d    %d\n", i_x_max, i_y_max, i_z_max) ;

  /* inhomogeneous fieldsph */
  for (i_x=1; i_x<(i_x_max+1); i_x++) 
  { for (i_y=1; i_y<(i_y_max+1); i_y++) 
    { for (i_z=1; i_z<(i_z_max+1); i_z++) 
      {
        /* generate fieldsph domain size: uniform  */
        Dim[0] = depth1/i_x_max ; 
        Dim[1] = width1/i_y_max ; 
        Dim[2] = height1/i_z_max ; 

        /* generate position */
        Posit[0] = ((i_x -1)-(i_x_max /2 - 0.5)) * Dim[0] ;
        Posit[1] = ((i_y -1)-(i_y_max /2 - 0.5)) * Dim[1] ;
        Posit[2] = ((i_z -1)-(i_z_max /2 - 0.5)) * Dim[2] ;

        /* generate field vectors in spherical representation */
        CartesianToSpherical(field_hom1, &fieldsph[2], &fieldsph[1]) ;	
        fieldsph[0] = LengthVector(field_hom1) ;
			
        fprintf(LogFilePtr,"Magnetic field:   %f Oe=Gauss\n",fieldsph[0]) ;

        /* print out to file */
        fprintf(FieldMapFile, " %f    %f    %f    %f    %f    %f    %f    %f    %f \n", 
        Posit[0], Posit[1], Posit[2],
        Dim[0], Dim[1], Dim[2],
        fieldsph[0], fieldsph[1], fieldsph[2]);
      }
    }
  }

  fclose(FieldMapFile);

}
