/*********************************************************************************************/
/*  VITESS module source                                                                     */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/*  Visualize beam module via PGPLOT GRAPHIK LIBRARY                                         */
/*                                                                                           */
/* 1.1  Jan 2001  S. Manoshin    cpgplot visualisation                                       */
/* 1.1a Feb 2001  S. Manoshin    adjustement of visualisation 01.02.01 [MSA]                 */
/* 1.1b Feb 2001  S. Manoshin    visualisation only given wavelength range 02.02.01 [MSA]    */
/* 1.1c Feb 2001  S. Manoshin    calculation center of beam 02.02.01 [MSA]                   */
/* 1.1d Feb 2001  G. Zsigmond    visualisation time of arrival(wavelength) 07.02.01 [Zsi]    */
/* 1.1e Mar 2001  S. Manoshin    visualisation wavelength(time of flight) 02.03.01 [MSA]     */
/* 1.1f Mar 2001  S. Manoshin    circular or rectangle, wavelength, width, heigth            */    
/* 1.1g Mar 2001  S. Manoshin    Tested on the Linux OS                                      */    
/* 1.2  Jun 2001  G. Zsigmond    SOFTABORT                                                   */
/* 1.3  Apr 2002  S. Manoshin	   Correct scale for time, wavelength visualization	           */
/* 1.4  Feb 2004  S. Manoshin    Visualise only first 10000 trajectories                     */
/*                               Choose the output device : screen, file or both             */				
/*                               New external variable gselec, default values for variables  */
/* 1.5  Jul 2020  K. Lieutenant  tidy up, new central visualization parameters               */
/*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "general.h"
#include "init.h"
#include "softabort.h"
#include "cpgplot.h"


/************************************/
/** Definitions, structures, enums **/
/************************************/
/* MF: visualisation for Windows and generation of file for the picture */
#ifdef DO_WIN32 
#define  GDEV "visual.ps"
#else 
#define  GDEV "visual.png"
#endif 


/******************************/
/** Prototypes               **/
/******************************/
void  OwnInit(int argc, char *argv[]);      // reads input parameters and initializes global variables


/******************************/
/** Global Variables         **/
/******************************/
extern int gselec;            // -o   [-]   output option: 1: display only, 2: file only, 3: both    defined in cpgplot.c 

long   keyw=0,                // -k   [-]   0: visualization ALL wavelength  1:  visualization only (wavemin..wavemax) wavelength
       visualizetype=1;       // -R   [-]   1: circular YZ visualize, 2: rectangle YZ visualize, 3: visualize time of arrival(wavelength), 4: visualize wavelength(time of arrival)

double winradius = 10.0;      // -r   [cm]  radius of a circular window
double ywincenter=  0.0,      // -y   [cm]  horizontal position of the center of a circular window 
       zwincenter=  0.0;      // -z   [cm]  vertical position of the center of a circular window 
double wavemin   =  0.0,      // -m  [Ang]  minimal wavelength
       wavemax   =  1.0e25;   // -M  [Ang]  maximal wavelength
double timemin   =  0.0,      // -t   [ms]  minimal TOF considered 
       timemax   =  1.0e25;   // -T   [ms]  maximal TOF considered 
double widthmin  =-10.0,      // -w   [cm]  minimal horizontal position of a rectangular window  
       widthmax  = 10.0;      // -W   [cm]  maximal horizontal position of a rectangular window
double heightmin =-10.0,      // -h   [cm]  minimal vertical position of a rectangular window
       heightmax = 10.0;      // -H   [cm]  maximal vertical position of a rectangular window


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
  double CenterX=0.0, CenterY=0.0,  CenterZ=0.0, 
         SumProb=0.0, AveTimeOF=0.0;
  long   i=0, 
         number_vis_tr=0;  /* Current number of visualised trajectories */

  // initialisation
  // --------------
  _eModule = MCN_VISUAL;
  gselec   = 1 ; /* Activate visualisation device -screen */

  Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.5");
	OwnInit(argc, argv);

  bVisInstalled = FALSE;
  bBlowUp       = FALSE;
    
  DECLARE_ABORT
  
  // loop over all trajectories
  // --------------------------
  while((ReadNeutrons())!= 0)
  {
    for(i=0; i<NumNeutGot; i++) 
    {
      CHECK
	     
      /* IMPORTANT! Module does not disturb the normal of the processing type */     

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
        if (number_vis_tr <= BufferSize)
        {
          switch(visualizetype) 
          {
            case 1:
              if (((InputNeutrons[i].Wavelength >= wavemin)&&(InputNeutrons[i].Wavelength <= wavemax)) || (keyw == 0))
              {    
                cpgpt1((float)(InputNeutrons[i].Position[1]), (float)(InputNeutrons[i].Position[2]),-2);

                CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
                CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
                CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
                AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
                SumProb   += InputNeutrons[i].Probability;
              }
              break;
	    
            case 2:
              if (((InputNeutrons[i].Wavelength >= wavemin)&&(InputNeutrons[i].Wavelength <= wavemax)) || (keyw == 0))
              {
                cpgpt1((float)(InputNeutrons[i].Position[1]), (float)(InputNeutrons[i].Position[2]),-2);
	
                CenterX   += InputNeutrons[i].Probability*InputNeutrons[i].Position[0]; 
                CenterY   += InputNeutrons[i].Probability*InputNeutrons[i].Position[1]; 
                CenterZ   += InputNeutrons[i].Probability*InputNeutrons[i].Position[2]; 
                AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
                SumProb   += InputNeutrons[i].Probability;
              }
              break;
	    
            case 3:
              cpgpt1((float)(InputNeutrons[i].Wavelength), (float)(InputNeutrons[i].Time),-2);
	
              AveTimeOF += InputNeutrons[i].Probability*InputNeutrons[i].Time;
              SumProb   += InputNeutrons[i].Probability;
              break;    
	    
	    
            case 4:
              cpgpt1((float)(InputNeutrons[i].Time), (float)(InputNeutrons[i].Wavelength),-2);
	
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
  }

// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
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

  /* Do the general cleanup */
  Cleanup(0.0, 0.0, 0.0, 0.0, 0.0);
  
  /* Close pgplot window */
  cpgclos();    
    
  return 0;
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
  int i;
    
  /* module input parameters */
  
  for(i=1; i<argc; i++) 
  {
    if(argv[i][0]!='+') 
    {
      switch(argv[i][1]) 
      {
        case 'o':
          gselec = atol(&argv[i][2]);
          break;
        case 'k':
          keyw = atol(&argv[i][2]);
          break;
        case 'R':
          visualizetype = atol(&argv[i][2]);
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

        default:
          fprintf(LogFilePtr,"ERROR: unkown commandline option: %s\n",argv[i]);
          exit(-1);
          break;
      }
    }
  }
    
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
	  
      cpgenv(-2.0*(winradius),2.0*(winradius),-2.0*(winradius),2.0*(winradius),1,0);
      cpgsfs(2);
      cpgcirc(ywincenter,zwincenter,(winradius));
      cpgsch(1.2);
      cpglab("Y, cm","Z, cm","NEUTRON BEAM VISUALISATION");
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
		  
      cpgenv(2.0*(widthmin),2.0*(widthmax), 2.0*(heightmin),2.0*(heightmax),1,0);
	
      cpgmove(-widthmin, -heightmin);
      cpgdraw(widthmin,-heightmin );
      cpgdraw(widthmin, heightmin);
      cpgdraw(-widthmin, heightmin);
      cpgdraw(-widthmin, -heightmin); 
	
      cpgsch(1.2);
      cpglab("Y, cm","Z, cm","NEUTRON BEAM VISUALISATION");
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
	    
      cpgenv(1.0*(timemin),1.0*(timemax), 1.0*(wavemin),1.0*(wavemax),1,0);
	
      cpgmove(-wavemin, -timemin);
      cpgdraw(wavemin,-timemin );
      cpgdraw(wavemin, timemin);
      cpgdraw(-wavemin, timemin);
      cpgdraw(-wavemin, -timemin); 
	
      cpgsch(1.2);
      cpglab("TIME OF ARRIVAL, ms","WAVELENGTH, A","TIME CHARACTERISTIC OF BEAM");
      break;
	    
	
    default:
      fprintf(LogFilePtr,"ERROR: Unkown type of visualization");
      exit(-1);
      break;
  }    
} 
