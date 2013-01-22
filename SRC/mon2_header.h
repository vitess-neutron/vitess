

#define BINSIZE 1001
static double* by;
static double* bz;
static double binyz[BINSIZE][BINSIZE];
static double binyzerror[BINSIZE][BINSIZE];
static int binyzcounts[BINSIZE][BINSIZE];



int WriteOutput(FILE* fmonitor, int format, int nbiny, int nbinz)
{

  int dy, dz;

  switch (format) {
	case 0:
	  fprintf(fmonitor,"#Monitor\n");
	  for(dy = 0; dy<nbiny; dy++)
		{
		  fprintf(fmonitor,"%10.7f\t",(by[dy]+by[dy+1])/2.0);
		}
	  for(dz = 0; dz<nbinz; dz++)
		{
		  fprintf(fmonitor,"\n %5.3f\t",(bz[dz]+bz[dz+1])/2.0);
		  for(dy = 0; dy<nbiny; dy++)
		{
		  fprintf(fmonitor,"%5.3E\t",binyz[dy][dz]);
		}
		}
	  break;
    case 1:
      fprintf(fmonitor,"#Monitor\n");
	  fprintf(fmonitor, "#x  y  z\n");
	  for(dz = 0; dz<nbinz; dz++) {
		  for(dy = 0; dy<nbiny; dy++) {

		    if (binyz[dy][dz]>0) {
		      binyzerror[dy][dz] = binyz[dy][dz]*sqrt(1./binyzcounts[dy][dz]);
		    }

		    fprintf(fmonitor,"%10.7f\t%10.7f\t%5.3E\t%5.3E\t%d\n", (by[dy]+by[dy+1])/2.0, (bz[dz]+bz[dz+1])/2.0, 
			    binyz[dy][dz],  binyzerror[dy][dz],  binyzcounts[dy][dz]);
		  }
		  fprintf(fmonitor, "\n");
	  }
	break;
  }
  fclose(fmonitor);

  return 1;

}
