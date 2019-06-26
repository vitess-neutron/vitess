/***********************FUNCTION*FOR*MODULE*BENDER*************************/
/**************************FUNCTION***TEST*********************************/
/**This*function*is*not*correct*for*all*tests***WILL*BE*MODIFIED*LATER*****/

#include "init.h"

#define  N_SURF		601
#define  N_SURF_S	600
#define  N_SURF_M3	1801
#define  N_SURF_M3_S	1800 
 
/****************************************/ 
/** Structures (changed in March 2002) **/ 
/****************************************/ 
 
/* Structure for describe bender geometry */

typedef struct 
{ 
/* double CriticalAngles; 
  double CutoffAngles; */ 
  SurfaceSecond SurfLeft [N_SURF]; 
  SurfaceSecond SurfRight[N_SURF]; 
  SurfaceSecond SurfExit[N_SURF];
  SurfaceSecond SurfTopBottom[N_SURF]; 
} 
Bender; 
 



 
void GeometryTestBender(Bender BenderMy, 
    double XENR[N_SURF], double YENR[N_SURF], double XENL[N_SURF], double YENL[N_SURF], 
    double XEXR[N_SURF], double YEXR[N_SURF], double XEXL[N_SURF], double YEXL[N_SURF],
    double BenderEntranceHeight, double BenderExitHeight, double beta, 
    long NumberOfSurfaces)

{
 
  /************TEST***PART***OF***THE***BENDER***GEOMETRY***************/    

double temp1, temp2, temp3, temp4, temp5, temp6;
long i;
double nearzero=1e-8;
double ZENL[N_SURF], ZENR[N_SURF], ZEXL[N_SURF], ZEXR[N_SURF];
double XX, YY, ZZ;

  /* Test left and right plane and exit plane*/

fprintf(LogFilePtr,"GEOMETRY OF BENDER IS TESTING - BEGIN\n");
  
  for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {
	ZENL[i] = 0.0;
	ZENR[i] = 0.0;
	ZEXL[i] = 0.0;
	ZEXR[i] = 0.0;
    
      		temp1 =   XENL[i]*XENL[i]*BenderMy.SurfLeft[i].A +
		    	  XENL[i]*BenderMy.SurfLeft[i].B +
			  YENL[i]*YENL[i]*BenderMy.SurfLeft[i].C +
		   	  YENL[i]*BenderMy.SurfLeft[i].D +
	                  ZENL[i]*ZENL[i]*BenderMy.SurfLeft[i].E +
			  ZENL[i]*BenderMy.SurfLeft[i].F +
			  BenderMy.SurfLeft[i].W +
			  XENL[i]*YENL[i]*BenderMy.SurfLeft[i].P +
		          YENL[i]*ZENL[i]*BenderMy.SurfLeft[i].Q +
	                  XENL[i]*ZENL[i]*BenderMy.SurfLeft[i].R;
	
      		temp2 =   XEXL[i]*XEXL[i]*BenderMy.SurfLeft[i].A +
    			  XEXL[i]*BenderMy.SurfLeft[i].B +
			  YEXL[i]*YEXL[i]*BenderMy.SurfLeft[i].C +
			  YEXL[i]*BenderMy.SurfLeft[i].D +
			  ZEXL[i]*ZEXL[i]*BenderMy.SurfLeft[i].E +
			  ZEXL[i]*BenderMy.SurfLeft[i].F +
			  BenderMy.SurfLeft[i].W +
			  XEXL[i]*YEXL[i]*BenderMy.SurfLeft[i].P +
			  YEXL[i]*ZEXL[i]*BenderMy.SurfLeft[i].Q +
			  XEXL[i]*ZEXL[i]*BenderMy.SurfLeft[i].R;
	

      		  temp3 =   XENR[i]*XENR[i]*BenderMy.SurfRight[i].A +
	                    XENR[i]*BenderMy.SurfRight[i].B +
	                    YENR[i]*YENR[i]*BenderMy.SurfRight[i].C +
	                    YENR[i]*BenderMy.SurfRight[i].D +
	                    ZENR[i]*ZENR[i]*BenderMy.SurfRight[i].E +
			    ZENR[i]*BenderMy.SurfRight[i].F +
			    BenderMy.SurfRight[i].W +
			    XENR[i]*YENR[i]*BenderMy.SurfRight[i].P +
			    YENR[i]*ZENR[i]*BenderMy.SurfRight[i].Q +
			    XENR[i]*ZENR[i]*BenderMy.SurfRight[i].R;
	
	
      		  temp4 =   XEXR[i]*XEXR[i]*BenderMy.SurfRight[i].A +
			    XEXR[i]*BenderMy.SurfRight[i].B +
			    YEXR[i]*YEXR[i]*BenderMy.SurfRight[i].C +
			    YEXR[i]*BenderMy.SurfRight[i].D +
			    ZEXR[i]*ZEXR[i]*BenderMy.SurfRight[i].E +
	                    ZEXR[i]*BenderMy.SurfRight[i].F +
			    BenderMy.SurfRight[i].W +
			    XEXR[i]*YEXR[i]*BenderMy.SurfRight[i].P +
			    YEXR[i]*ZEXR[i]*BenderMy.SurfRight[i].Q +
			    XEXR[i]*ZEXR[i]*BenderMy.SurfRight[i].R;	
	
	
	  	  temp5  =  XEXR[i]*XEXR[i]*BenderMy.SurfExit[i].A +
		  	    XEXR[i]*BenderMy.SurfExit[i].B +
  			    YEXR[i]*YEXR[i]*BenderMy.SurfExit[i].C +
  			    YEXR[i]*BenderMy.SurfExit[i].D +
  		 	    ZEXR[i]*ZEXR[i]*BenderMy.SurfExit[i].E +
  			    ZEXR[i]*BenderMy.SurfExit[i].F +
  			    BenderMy.SurfExit[i].W +
  			    XEXR[i]*YEXR[i]*BenderMy.SurfExit[i].P +
  			    YEXR[i]*ZEXR[i]*BenderMy.SurfExit[i].Q +
  			    XEXR[i]*ZEXR[i]*BenderMy.SurfExit[i].R;       
  	  
  	  	  temp6  =  XEXL[i]*XEXL[i]*BenderMy.SurfExit[i].A +
  			    XEXL[i]*BenderMy.SurfExit[i].B +
  			    YEXL[i]*YEXL[i]*BenderMy.SurfExit[i].C +
  			    YEXL[i]*BenderMy.SurfExit[i].D +
  			    ZEXL[i]*ZEXL[i]*BenderMy.SurfExit[i].E +
  			    ZEXL[i]*BenderMy.SurfExit[i].F +
  			    BenderMy.SurfExit[i].W +
  			    XEXL[i]*YEXL[i]*BenderMy.SurfExit[i].P +
  			    YEXL[i]*ZEXL[i]*BenderMy.SurfExit[i].Q +
  			    XEXL[i]*ZEXL[i]*BenderMy.SurfExit[i].R;       
  	  
	if (fabs(temp1) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.1 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp2) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.2 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp3) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.3 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp4) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.4 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp5) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.5 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp6) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 1.6 of bender geometry is not passed. \n");
	}
	
//	fprintf(LogFilePtr,"Test left and right surfaces:  %f  %f  %f  %f\n",temp1,temp2,temp3,temp4);
//	fprintf(LogFilePtr,"Test exit surfaces rf  %f  %f \n",temp5,temp6);  
	

    }

	XX = 0.0;
	YY = 0.0;
	ZZ = (BenderEntranceHeight/2.0);

  temp1 =  XX*XX*BenderMy.SurfTopBottom[0].A +
    	   XX*BenderMy.SurfTopBottom[0].B +
	   YY*YY*BenderMy.SurfTopBottom[0].C +
    	   YY*BenderMy.SurfTopBottom[0].D +
    	   ZZ*ZZ*BenderMy.SurfTopBottom[0].E +
    	   ZZ*BenderMy.SurfTopBottom[0].F +
    	   	BenderMy.SurfTopBottom[0].W +
    	   XX*YY*BenderMy.SurfTopBottom[0].P +
    	   YY*ZZ*BenderMy.SurfTopBottom[0].Q +
    	   XX*ZZ*BenderMy.SurfTopBottom[0].R ;


	XX = 0.0;
	YY = 0.0;
	ZZ = (-BenderEntranceHeight/2.0);

 temp2 =   XX*XX*BenderMy.SurfTopBottom[1].A +
    	   XX*BenderMy.SurfTopBottom[1].B +
	   YY*YY*BenderMy.SurfTopBottom[1].C +
    	   YY*BenderMy.SurfTopBottom[1].D +
    	   ZZ*ZZ*BenderMy.SurfTopBottom[1].E +
    	   ZZ*BenderMy.SurfTopBottom[1].F +
    	   	BenderMy.SurfTopBottom[1].W +
    	   XX*YY*BenderMy.SurfTopBottom[1].P +
    	   YY*ZZ*BenderMy.SurfTopBottom[1].Q +
    	   XX*ZZ*BenderMy.SurfTopBottom[1].R ;
 
  
 	XX = 0.0;
	YY = 0.0;
	ZZ = (BenderExitHeight/2.0); 
 
 
 temp3 =   XX*XX*BenderMy.SurfTopBottom[0].A +
    	   XX*BenderMy.SurfTopBottom[0].B +
	   YY*YY*BenderMy.SurfTopBottom[0].C +
    	   YY*BenderMy.SurfTopBottom[0].D +
    	   ZZ*ZZ*BenderMy.SurfTopBottom[0].E +
    	   ZZ*BenderMy.SurfTopBottom[0].F +
    	   	BenderMy.SurfTopBottom[0].W +
    	   XX*YY*BenderMy.SurfTopBottom[0].P +
    	   YY*ZZ*BenderMy.SurfTopBottom[0].Q +
    	   XX*ZZ*BenderMy.SurfTopBottom[0].R ;    


	XX = 0.0;
	YY = 0.0;
	ZZ = (-BenderExitHeight/2.0); 

 temp4 =   XX*XX*BenderMy.SurfTopBottom[1].A +
    	   XX*BenderMy.SurfTopBottom[1].B +
	   YY*YY*BenderMy.SurfTopBottom[1].C +
    	   YY*BenderMy.SurfTopBottom[1].D +
    	   ZZ*ZZ*BenderMy.SurfTopBottom[1].E +
    	   ZZ*BenderMy.SurfTopBottom[1].F +
    	   	BenderMy.SurfTopBottom[1].W +
    	   XX*YY*BenderMy.SurfTopBottom[1].P +
    	   YY*ZZ*BenderMy.SurfTopBottom[1].Q +
    	   XX*ZZ*BenderMy.SurfTopBottom[1].R ;    


//	fprintf(LogFilePtr,"Test top bot surfaces:  %f  %f  %f  %f\n",temp1,temp2,temp3,temp4);
	
	if (fabs(temp1) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 2.1 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp2) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 2.2 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp3) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 2.3 of bender geometry is not passed. \n");
	}
	
	if (fabs(temp4) >= nearzero)
	{
	    fprintf(LogFilePtr,"WARNING!!! Test 2.4 of bender geometry is not passed. \n");
	}	
	
/*    fprintf(LogFilePtr,"Test top, bot surfaces  %f  %f  %f  %f \n",temp1,temp2,temp3,temp4); */

    /* Test the angle between entrance and exit planes */

    for(i = 1; i <= (NumberOfSurfaces-1); i++)
    {	if ((YEXL[i] - YEXR[i]) != 0.0)
	{
	    temp1 = (XEXL[i] - XEXR[i]) / (YEXL[i] - YEXR[i]);
	    temp1 = -1.0*atan(temp1);
	    if (fabs(beta-temp1)> nearzero)
	    {	
		fprintf(LogFilePtr,"Angle test %ld is not passed. \n",i);
	    }
//	    fprintf(LogFilePtr,"(Legth/Radius)  =  %f ; Calculated angle =  %f \n",beta,temp1); 
	}
    }	

fprintf(LogFilePtr,"GEOMETRY OF BENBER IS TESTING - END\n");

/***********END***TEST***PART**************/    

}
/************END***TEST***FUNCTION*********/


 
