#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "general.h"
#include "init.h"
#include "matrix.h"
#include "precessionfield.h"

void writemagneticmap()
{
  long i_x, i_y, i_z, i_x_max, i_y_max, i_z_max ;
  double Dim[3], Posit[3], fieldsph[3] ;

  FieldMapFile=fopen(FieldFileName, "w");

  /*    INPUT  */
  i_x_max = i_y_max = i_z_max = 2 ;

  /* print number of matrix lines, columns */
  fprintf(FieldMapFile, " %ld    %ld    %ld\n", i_x_max, i_y_max, i_z_max) ;

  /* inhomogeneous fieldsph */
  for(i_x=1;i_x<(i_x_max+1);i_x++)
  { for(i_y=1;i_y<(i_y_max+1);i_y++)
    { for(i_z=1;i_z<(i_z_max+1);i_z++)
      {
        /* generate fieldsph domain size: uniform  */
        Dim[0] = depth/i_x_max ;
        Dim[1] = width/i_y_max ;
        Dim[2] = height/i_z_max ;

        /* generate position */
        Posit[0] = ((i_x -1)-(i_x_max /2 - 0.5)) * Dim[0] ;
        Posit[1] = ((i_y -1)-(i_y_max /2 - 0.5)) * Dim[1] ;
        Posit[2] = ((i_z -1)-(i_z_max /2 - 0.5)) * Dim[2] ;

        /* generate field vectors in spherical representation */
        fieldsph[0] = LengthVector(field_hom) ;

        { VectorType fieldh;
          CopyVector(field_hom, fieldh);
          MultiplyByScalar(fieldh, 1./fieldsph[0]) ;

          CartesianToSpherical(fieldh, &fieldsph[2], &fieldsph[1]) ;
        }

        /* print out to file */
        fprintf(FieldMapFile, " %le    %le    %le    %le    %le    %le    %le    %le    %le \n",
                              Posit[0], Posit[1], Posit[2],
                              Dim[0], Dim[1], Dim[2],
                              fieldsph[0], fieldsph[1], fieldsph[2]);
      }
    }
  }

  fclose(FieldMapFile);
}
