/*********************************************************************************************/
/*  VITESS module 'filter2D'                                                                     */
/*                                                                                           */
/* This module simulates a rectangular area in which the                                        */
/*                                                                                           */
/* The free non-commercial use of these routines is granted providing due credit is given to */
/* the authors.                                                                              */
/*                                                                                           */
/* 1.0  Sep 2021  K. Lieutenant   initial version (based on slit.c)                                           */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "convert.h"


/******************************/
/** Prototypes               **/
/******************************/
void   OwnInit   (int argc, char *argv[]);    // reads input parameters and initializes global variables
void   LoadTable ();                          // reads the filter table F(y,z) or F(div_y, div_z)
double GetFactor(double Y, double Z);         // returns the factor F(y,z) or F(div_y, div_z) for the given position or divergence
int    TabIndex (int i, int j);               // returns the index in the array for the matrix element A_ij


/******************************/
/** Global Variables         **/
/******************************/
// Input parameters
VtMon2Par ePar=NO_MON2_PAR;    // -P   [-]   parameter that is filtered (position or divergence)
char*     sFilterTable=NULL;   // -F   [-]   name of the file containing the filter table  
double    Ymin=0.0,            // -y   [cm]  minimal horizontal filter position
          Ymax=0.0,            // -Y   [cm]  maximal horizontal filter position 
          Zmin=0.0,            // -z   [cm]  minimal vertical filter position 
          Zmax=0.0;            // -Z   [cm]  maximal vertical filter position 

// Variables determined from input parameters or data from file
FILE*     pFilterFile=NULL;    //            pointer to the file containing the filter table  
int       nBinsHor =1,         //      [cm]  number of horizontal channels = number of matrix columns
          nBinsVert=1;         //      [cm]  number of vertical channels   = number of matrix rows
double*   aFilter=NULL;


/******************************/
/** Main Program             **/
/******************************/
int main(int argc, char *argv[])
{
	long  i=0;
  char   sParTxt[20]="";
	double PosY=0.0, PosZ=0.0;        // hor. and vert. position of neutron

  // initialisation
  // --------------
  _eModule = MCN_SLIT;

	Init(argc,argv, _eModule);
  PrintModuleName(_eModule, "1.0");
	OwnInit(argc, argv);
  LoadTable();

  bVisInstalled = FALSE;
  bLengthCmpr   = FALSE;

	DECLARE_ABORT

  // loop over all trajectories
  // --------------------------
	while (ReadNeutrons()!= 0)
	{
		for (i=0; i<NumNeutGot; i++)
		{
			CHECK

      // Only write out event if EOB line is found, otherwise process trajectory
      if (IsEOB(&(InputNeutrons[i]))==TRUE)
      {
        WriteNeutron(&(InputNeutrons[i]));
      }
      else
      { 
			  // Calculate  and  writeout new data set, if slit is hit
			  // -----------------------------------------------------
			  PosY = InputNeutrons[i].Position[1];
			  PosZ = InputNeutrons[i].Position[2];
			
			  if (PosY > Ymin && PosY < Ymax &&  PosZ > Zmin && PosZ < Zmax)
			  {	
				  InputNeutrons[i].Probability *= GetFactor(PosY, PosZ);

				  WriteNeutron(&InputNeutrons[i]);
			  }
      }
		}
	}	

// Finish: print parameters, write geometry and instrument file, free memory
// -----------------------------------------------------
my_exit:
  Mon2Par_ID2Txt(sParTxt, ePar);
	fprintf(LogFilePtr, "%d x %d %s filter of size %6.2f x %6.2f cm (W x H) using file '%s'\n", 
	                    nBinsHor, nBinsVert, sParTxt, Ymax-Ymin, Zmax-Zmin, sFilterTable);

  Cleanup(0.0, 0.0, 0.0, 0.0, 0.0);     // print intensity, write instrument.inf, free memory

  return(0);
}


/*******************************************************/
/** Reads input parameters and sets global variables  **/
/*******************************************************/
void  OwnInit(int argc, char *argv[])
{
	int i=0;

	for (i=1; i<argc; i++)
	{
		if (argv[i][0]!='+') 
		{
			switch(argv[i][1])
      {
        /* filter file name */
        case 'F':
          sFilterTable=&argv[i][2];
          break;
				case 'P':
					ePar = Mon2Par_Txt2ID(&argv[i][2]);
					break;

				case 'y':
					Ymin  = atof(&argv[i][2]);
					break;
				case 'Y':
					Ymax = atof(&argv[i][2]);
					break;
				case 'z':
					Zmin  = atof(&argv[i][2]);
					break;
				case 'Z':
					Zmax = atof(&argv[i][2]);
					break;

				case 'n':
					nBinsHor = atoi(&argv[i][2]);
					break;
				case 'N':
					nBinsVert = atoi(&argv[i][2]);
					break;
      
				default:
					fprintf(LogFilePtr,"ERROR: unknown command option: %s\n",argv[i]);
					exit(-1);
					break;
			}
		}
	}
}


/*********************************************************************/
/* reads the filter table F(y,z) or F(div_y, div_z)                  */
/*********************************************************************/
void LoadTable()
{
  char sBuffer[CHAR_BUF_LENGTH];

  /* If there is a structure factor file go and load the file */
  if (sFilterTable!=NULL) 
  {
    /* opens distribution file */
    pFilterFile = OpenInputFile(sFilterTable, FALSE, "rt");
    if (pFilterFile!=NULL) 
    {
      /* reads number of lines, allocates memory and then reads data */
      nBinsHor  = ColumnsInFile(pFilterFile);
      nBinsVert = LinesInFile(pFilterFile);
      aFilter  = calloc(nBinsHor*nBinsVert, sizeof(double));

      for (int i=0; i < nBinsVert; i++)
      {  
        ReadLine  (pFilterFile, sBuffer, CHAR_BUF_LENGTH);
        StrgScanLF(sBuffer, &(aFilter[i*nBinsHor]), nBinsHor, 0);
      }

      /* closes trace file */
      fclose(pFilterFile) ;
    } 
    else 
    {	
      fprintf(LogFilePtr, "\nERROR: Can't open %s to read structure factor file\n", sFilterTable);
      exit (-1);
    }
  }
}


/*************************************************************************************/
/* returns the factor F(y,z) or F(div_y, div_z) for the given position or divergence */
/*************************************************************************************/
double GetFactor(double Y, double Z)
{
  int    i=0, j=0;
  double fact=0.0;

  if (Y > Ymin && Y < Ymax && Z > Zmin && Z < Zmax)
  {
    i = floor((Z - Zmin)*nBinsVert/(Zmax - Zmin));
    j = floor((Y - Ymin)*nBinsHor /(Ymax - Ymin));
    fact = aFilter[TabIndex(i,j)];
  }

  return fact;
}

/***************************************************************/
/* returns the index in the array for the matrix element A_ij  */
/***************************************************************/
int TabIndex(int i, int j)
{
  int k = i * nBinsHor + j;
  return k;
}


	    

      
 



      



