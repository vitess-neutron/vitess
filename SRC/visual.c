/*********************************************************************************************/
/*  VITESS module source                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*                 Visulalise beam module via PGPLOT GRAPHIK LIBRARY (version 1.1) 	     */
/*                 Writed by Manoshin Sergey manoshin@hmi.de Jan-Feb-Mar 2001
		     */
/*                    1. Include cpgplot visualisation  Jan 2001  
			     */
/*                    2. Include adjustement of visualisation 01.02.01 [MSA] 
		     */
/*                    3. Include visualisation only given wavelength range 02.02.01 [MSA] 
   */
/*                    4. Include calculation center of beam 02.02.01 [MSA]  
		     */
/*                    5. Include visualisation time of arrival(wavelength) 07.02.01 [Zsi] 
   */
/*                    6. Include visualisation wavelength(time of flight) 02.03.01 [MSA]     */
/*                  circular or rectangle, wavelength, width, heigth			     */    
/* 1.01             Tested on the Linux OS 						     */    
/*                                                                                           */
/* 1.02  June 2001  G. Zsigmond    SOFTABORT                                                 */
/* 1.03  April 2002 S. Manoshin	   Correct scale for time, wavelength visualization	     */
/* 1.04	 Feb.  2004 S. Manoshin    Visualise only first 10000 trajectories		     */
/*				   Choose the output device : screen, file or both	     */				
/*				   New external variable gselec 			     */
/*				   Default values for some variables			     */
/*********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "cpgplot.h"


  /* MF: visualisation for Windows and generation of file for the picture */
#ifdef DO_WIN32 
#define  GDEV "visual.ps"
#else 
#define  GDEV "visual.png"
#endif 



extern int   gselec; /* choose the output 1 - display only, 2 - file only,
			    3 - both, defined in cpgplot.c */

int main(int argc, char *argv[])
{
  long   i, keyw=0;
  double heightmin=-10.0, heightmax=10.0;
  double widthmin=-10.0, widthmax=10.0;
  double wavemin=0.0, wavemax=1e25;
  double winradius=10.0;
  double ywincenter=0.0, zwincenter=0.0;
  double timemin=0.0, timemax=1e25;
  double CenterX=0.0, CenterY=0.0,  CenterZ=0.0 , SumProb=0.0, AveTimeOF=0.0;
  
  long   number_vis_tr=0; /* Current number of visualised trajectories */
  
  long   visualizetype=1;
  

/* var visualizetype: 1 - circular YZ visualize, 2 - rectangle
YZ visualize, 3 - visualize time of arrival(wavelength),
4 - visualize wavelength(time of arrival)
*/

  /* Initialize the program according to the parameters given   */
  /* Input */
  
  Init(argc, argv, VT_VISUAL);

    print_module_name("visualize 1.02");

    gselec = 1 ; /* Activate visualisation device -screen */
    
    keyw = 0;
    
/* keyw = 0  visualization ALL wavelength
   keyw = 1  visualization only (wavemin..wavemax) wavelength */    
    
  /* module input parameters */
  
  for(i=1; i<argc; i++) 
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1]) 
      {

	case 'R':
	  visualizetype = atol(&argv[i][2]);
	  break;
	  
	case 'o':
	  gselec = atol(&argv[i][2]);
	  break;
	  
	  
	case 'r':
	  winradius = atof(&argv[i][2]);
	  break;  
	  
	case 'y':
	  ywincenter = atof(&argv[i][2]);  
	  break;
	  
	case 'z':
	  zwincenter = atof(&argv[i][2]);
	  break;  
	  
        case 'm':
	  wavemin = atof(&argv[i][2]);
	  break;   
	  
	case 'M':
	  wavemax = atof(&argv[i][2]);
	  break;   
	  
	case 't':
	  timemin = atof(&argv[i][2]);
	  break;   
	  
	case 'T':
	  timemax = atof(&argv[i][2]);
	  break;     
	  
	case 'h':
	  heightmin = atof(&argv[i][2]);
	  break;  
	  
        case 'H':
	  heightmax = atof(&argv[i][2]);
	  break;   
	  
	case 'w':
	  widthmin = atof(&argv[i][2]);
	  break;  
	  
        case 'W':
	  widthmax = atof(&argv[i][2]);
	  break;   
	  
	case 'k':
	  keyw = atol(&argv[i][2]);
	  break;

        default:
          fprintf(LogFilePtr,"ERROR: unkown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
  

/*    fprintf(LogFilePtr, "Kind of visualisation %d \n",visualizetype);
    fprintf(LogFilePtr, "Radius of circle %f \n",winradius);
    fprintf(LogFilePtr, "Center of circle %f  %f \n", ywincenter, zwincenter);
    fprintf(LogFilePtr, "Wave min max: %f  %f \n",wavemin, wavemax);  
    fprintf(LogFilePtr, "Time min max: %f  %f \n",timemin, timemax);
    fprintf(LogFilePtr, "Width min max: %f  %f \n",widthmin, widthmax);
    fprintf(LogFilePtr, "Height min max: %f  %f \n",heightmin, heightmax); */
    
    
     if (keyw == 1)
     {
          if (wavemin > wavemax)
          {
	      fprintf(LogFilePtr,"ERROR: Wavelength min and max values incorrect. Check please.  \n");    
	      exit(-1);
	  }
     }
  
  /* initialize pgplot with device GraphDev */
    if (!((gselec == 1)||(gselec == 2)||(gselec == 3)))
    {
	fprintf(LogFilePtr,"ERROR: Incorrect output device, correct option -o, value 1,2 or 3 \n");
	exit(-1);
    }
    
    
    if (gselec == 1)
    {
	fprintf(LogFilePtr,"Open visual output device - display\n");
    }
    
    if (gselec == 2)
    {
	fprintf(LogFilePtr,"Open visual output device - file \n");
    }
    
    if (gselec == 3)
    {
	fprintf(LogFilePtr,"Open visual output device - display+file \n");
    }

  
    if (cpgopen(GDEV) < 1) 
    {
	fprintf(LogFilePtr,"ERROR: I cannot open the graphical output device \n");    
	exit(-1);
    }  
    
    /* Define pgplot coordinate range of graph circular or rectangle */
    
    switch(visualizetype) 
    {

	case 1:
	
	  if (winradius <= 0.0)
	  {
		  fprintf(LogFilePtr,"ERROR: Raduis circular window values incorrect. Check please.  \n");    
		  exit(-1);
	  }
	  
	    cpgenv(-2.0*(winradius),2.0*(winradius),
	    -2.0*(winradius),2.0*(winradius),1,0);
	    cpgsfs(2);
	    cpgcirc(ywincenter,zwincenter,(winradius));
	    cpgsch(1.2);
	    cpglab("Y, cm","Z, cm","NEUTRON BEAM VISUALISATION");
	
	    CenterX   = 0.0; 
	    CenterY   = 0.0; 
	    CenterZ   = 0.0; 
	    SumProb   = 0.0;
	    AveTimeOF = 0.0;
	    break;

	
	case 2:
	
	  if (heightmin >= heightmax)
	  {
		  fprintf(LogFilePtr,"ERROR: Height min and max values incorrect. Check please. \n");    
 	      	  exit(-1);
	  }
  
	  if (widthmin >= widthmax)
	  {
		  fprintf(LogFilePtr,"ERROR: Width min and max values incorrect. Check please.  \n");    
		  exit(-1);
	  }
		  
	    cpgenv(2.0*(widthmin),2.0*(widthmax),
	    2.0*(heightmin),2.0*(heightmax),1,0);
	
	    cpgmove(-widthmin, -heightmin);
	    cpgdraw(widthmin,-heightmin );
	    cpgdraw(widthmin, heightmin);
    	    cpgdraw(-widthmin, heightmin);
	    cpgdraw(-widthmin, -heightmin); 
	
	    cpgsch(1.2);
	    cpglab("Y, cm","Z, cm","NEUTRON BEAM VISUALISATION");
	
	    CenterX   = 0.0; 
	    CenterY   = 0.0; 
	    CenterZ   = 0.0; 
	    SumProb   = 0.0;
	    AveTimeOF = 0.0;
	    break;

	    
	  case 3:
	  
	    if (timemin > timemax)
	    {
		  fprintf(LogFilePtr,"ERROR: Time min and max values incorrect. Check please.  \n");    
		  exit(-1);
	    }
	    
	   if (wavemin > wavemax)
	   {
		  fprintf(LogFilePtr,"ERROR: Wavelength min and max values incorrect. Check please.  \n");    
		  exit(-1);
	   }
	    
	    cpgenv(1.0*(wavemin),1.0*(wavemax),
	    1.0*(timemin),1.0*(timemax),1,0);
	
	    cpgmove(-wavemin, -timemin);
	    cpgdraw(wavemin,-timemin );
	    cpgdraw(wavemin, timemin);
    	    cpgdraw(-wavemin, timemin);
	    cpgdraw(-wavemin, -timemin); 
	
	    cpgsch(1.2);
	    cpglab("WAVELENGTH, A","TIME OF ARRIVAL, ms","TIME CHARACTERISTIC OF BEAM");
	
	    SumProb   = 0.0;
	    AveTimeOF = 0.0;
	    break;
	    
	  case 4:
	  
	    if (timemin > timemax)
	    {
		  fprintf(LogFilePtr,"ERROR: Time min and max values incorrect. Check please.  \n");    
		  exit(-1);
	    }
	    
	   if (wavemin > wavemax)
	   {
		  fprintf(LogFilePtr,"ERROR: Wavelength min and max values incorrect. Check please.  \n");    
		  exit(-1);
	   }
	    
	    cpgenv(1.0*(timemin),1.0*(timemax),
	    1.0*(wavemin),1.0*(wavemax),1,0);
	
	    cpgmove(-wavemin, -timemin);
	    cpgdraw(wavemin,-timemin );
	    cpgdraw(wavemin, timemin);
    	    cpgdraw(-wavemin, timemin);
	    cpgdraw(-wavemin, -timemin); 
	
	    cpgsch(1.2);
	    cpglab("TIME OF ARRIVAL, ms","WAVELENGTH, A","TIME CHARACTERISTIC OF BEAM");
	
	    SumProb   = 0.0;
	    AveTimeOF = 0.0;
	    break;
	    
	
	default:
            fprintf(LogFilePtr,"ERROR: Unkown type of visualization");
            exit(-1);
            break;
    
    }    

/*    fprintf(LogFilePtr,"Ok, window opened"); */
    
  /* Get the neutrons from the file */
DECLARE_ABORT
  
  while((ReadNeutrons())!= 0)
  {
CHECK    
    /* here you may do anything you like to the neutrons, but, please, */
    /* WriteNeutron for the output! Thank you			       */
    for(i=0; i<NumNeutGot; i++) 
    {
CHECK
     /*      fprintf(AsciiFile,"%.5e %.5e %.5e %.5e %.5e %.5e %.5e %.5e %.5e\n",
             InputNeutrons[i].Time, InputNeutrons[i].Wavelength,
             InputNeutrons[i].Probability, InputNeutrons[i].Position[0],
             InputNeutrons[i].Position[1], InputNeutrons[i].Position[2],
             InputNeutrons[i].Vector[0], InputNeutrons[i].Vector[1],
             InputNeutrons[i].Vector[2]);  */
             
	     
/* IMPORTANT! Module does not disturb the normal of the processing type */     


    if (number_vis_tr <= BufferSize)
    {
    
    switch(visualizetype) 
    {
	case 1:
		if (((InputNeutrons[i].Wavelength >= wavemin)
		&&(InputNeutrons[i].Wavelength <= wavemax))||(keyw == 0))
		{    
			cpgpt1((float)(InputNeutrons[i].Position[1]), 
			(float)(InputNeutrons[i].Position[2]),-2);

			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
			AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
			SumProb   += InputNeutrons[i].Probability;
		}
	    break;
	    
	case 2:
		if (((InputNeutrons[i].Wavelength >= wavemin)
		&&(InputNeutrons[i].Wavelength <= wavemax))||(keyw == 0))
		{
			cpgpt1((float)(InputNeutrons[i].Position[1]), 
    			(float)(InputNeutrons[i].Position[2]),-2);
	
			CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
			CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
			CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
			AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
			SumProb   += InputNeutrons[i].Probability;
		}
	    break;
	
	    
	case 3:
		cpgpt1((float)(InputNeutrons[i].Wavelength), 
    		(float)(InputNeutrons[i].Time),-2);
	
		AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
		SumProb   += InputNeutrons[i].Probability;
	
		break;    
	    
	    
	case 4:
		cpgpt1((float)(InputNeutrons[i].Time), 
    		(float)(InputNeutrons[i].Wavelength),-2);
	
		AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
		SumProb   += InputNeutrons[i].Probability;
	
		break;    
	    
    } /* end case */
    	
    } /* end if */
    
    number_vis_tr = number_vis_tr + 1;
	
/* Output neutrons in the pipe */    
     WriteNeutron(&(InputNeutrons[i]));
    
    }
  }

my_exit:
  /* Do the general cleanup */
  Cleanup(0.0, 0.0, 0.0, 0.0, 0.0);
  
  /* Close pgplot window */
  
  cpgclos();    
  
  /* Output center of beam */
  
  switch(visualizetype) 
    {
	case 1:
	
  	    if (SumProb != 0.0)
	    {
		    CenterX   = CenterX/SumProb; 
		    CenterY   = CenterY/SumProb;
		    CenterZ   = CenterZ/SumProb;
		    AveTimeOF = AveTimeOF/SumProb;
		
	    fprintf(LogFilePtr,"Center of Visualise Beam: X = %f cm Y = %f cm  Z = %f cm TOF = %f ms \n",CenterX, CenterY, CenterZ, AveTimeOF);
	    }
	    else
	    {
	    fprintf(LogFilePtr,"No required neutrons for visualize \n");
	    }
	    break;
	
	case 2:
	
	    if (SumProb != 0.0)
	    {
		    CenterX   = CenterX/SumProb; 
		    CenterY   = CenterY/SumProb;
		    CenterZ   = CenterZ/SumProb;
		    AveTimeOF = AveTimeOF/SumProb;
		
	    fprintf(LogFilePtr,"Center of Visualise Beam: X = %f cm  Y = %f cm  Z = %f cm TOF = %f ms \n",CenterX, CenterY, CenterZ, AveTimeOF);
	    }
	    else
	    {
	    fprintf(LogFilePtr,"No requred neutrons for visualize \n");
	    }
	    break;
	    
	case 3:
	
	    if (SumProb != 0.0)
	    {
		    AveTimeOF = AveTimeOF/SumProb;
	    fprintf(LogFilePtr,"Average TOF = %f  ms \n", AveTimeOF);
	    }
	    else
	    {
	    fprintf(LogFilePtr,"No requred neutrons for visualize \n");
	    }
	    break;    
	    
	    
	case 4:
	
	    if (SumProb != 0.0)
	    {
		    AveTimeOF = AveTimeOF/SumProb;
	    fprintf(LogFilePtr,"Average TOF = %f  ms \n", AveTimeOF);
	    }
	    else
	    {
	    fprintf(LogFilePtr,"No requred neutrons for visualize \n");
	    }
	    break;    
    }	
  
  return 0;
}

 
