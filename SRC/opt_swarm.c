/******************************************************************************************/
/*  Optimization using Swarm algorithm                                                    */
/*  Time-Varying Acceleration Coefficients (TVAC) method (10.1109/TEVC.2004.826071)       */
/*                                                                                        */
/* 1.00   Nov 2013    C. Zendler    1st version                                           */
/******************************************************************************************/
#include "general.h"
#include "opt_fct.h"
#include "opt_vars.h"

#define MAX_BEES 300      // max. number of bees (individuals)

typedef struct
{
  double Position[NMAX+1];
  double Velocity[NMAX+1];
  double localBestPos[NMAX+1];
  double localBestFoM;
}
  SwarmIndividual;

/*********************************************/
/* global variables                          */
/*********************************************/
static double globalBestPos[NMAX+1]={0};
static short DEBUG = FALSE;                      

/*********************************************/
/* prototypes                                */
/*********************************************/
static short ReadIniFile(int *pNbees, int *pNparameters, int *pNsteps, int *pNfinish, double *pW0min, double *pW0max, double *pW1min, double *pW1max, double *pW2min, double *pW2max, short *pWrite, const char *sIniFile);
static double SetStartingPosition(SwarmIndividual *onebee, int BeeNo);
static double UpdatePosition(SwarmIndividual *onebee, const double w0_min, const double w0_max, const double w1_min, const double w1_max, const double w2_min, const double w2_max, const int nSteps, const int current);
inline void CopyPosition(double PosIn[], double PosOut[]);

/*****************************************************************************/
/* Swarm optimization routine                                                */
/*****************************************************************************/
short Swarm(){

  SwarmIndividual ThisBee[MAX_BEES+1];  // the swarm: array of individuals
  double globalBestFoM=1e99;            // inverse FoM: Factor/FOM read from Fcomm.dat, global best value
  double currentFoM=1e99;               // inverse FoM: Factor/FOM read from Fcomm.dat, current value
  int i,j,k,                            // counting variables
    last_change=0;                      // last iteration step with improvement
  FILE *fSearchProgress;                // print list for plotting: step, Factor/FoM, Par1...ParN  
  FILE *fBeeMovement;                   // TMP: print list for plotting: Par1 for bee1...N 
 
  /* input parameters with default values */
  double wMin_inertia = 0.4,    // min. inertia weight
    wMax_inertia = 0.9,         // max. inertia weight
    wMin_local = 0.5,           // min. weight individual best
    wMax_local = 2.5,           // max. weight individual best
    wMin_global = 0.5,          // min. weight global best
    wMax_global = 2.5;          // max. weight global best
  int Nbees = 10,               // number of individual bees 
    Nparameters = 1,            // number of parameters
    Nsteps = 150,               // max. number of optimization steps
    Nfinish = 10;               // number of optimization steps without improvement before finishing
  

  /* initialize */
  if(DEBUG)
    fprintf(LogFilePtr,"\n\n DEBUG: Starting initialization\n");
  
  fBeeMovement=fileOpen("Swarm_BeeMovement_Parameter1.dat","wt"); 
  ReadIniFile(&Nbees, &Nparameters, &Nsteps, &Nfinish, &wMin_inertia, &wMax_inertia, &wMin_local, &wMax_local, &wMin_global, &wMax_global, &eOut, sIniFile);

  if(eOut==3)
    DEBUG=TRUE;
  
  fprintf(LogFilePtr,"\n swarm algorithm 1.0 \n swarm size: %d individuals \n optimization steps: %d \n time dependent weights modified between (w_start,w_end): \n   w0: (%f,%f),  w1: (%f,%f),  w2: (%f,%f) \n\n",Nbees, Nsteps,wMax_inertia, wMin_inertia, wMax_local, wMin_local, wMin_global, wMax_global);

  for (j=1; j<=Nbees; j++){
    
    if(DEBUG)
      fprintf(LogFilePtr,"\n DEBUG: bee no %d: ",j);
    
    currentFoM = SetStartingPosition(&ThisBee[j],j);
    ThisBee[j].localBestFoM = currentFoM;
    CopyPosition(ThisBee[j].Position,ThisBee[j].localBestPos);
    if(currentFoM < globalBestFoM){
      globalBestFoM = currentFoM;
      CopyPosition(ThisBee[j].Position,globalBestPos);
    }
    
    if(DEBUG)
      (j<Nbees) ? fprintf(fBeeMovement,"%10.4f  ",ThisBee[j].Position[1]) : fprintf(fBeeMovement,"%10.4f  \n",ThisBee[j].Position[1]); 
    
  }
 
  fprintf(LogFilePtr,"\n (swarm) start global best (Factor/FoM): %10.4e \n\n",globalBestFoM);
  fSearchProgress=fileOpen("Swarm_GlobalBestValues.dat","wt");
  if(eOut>1){
    fprintf(fSearchProgress,"0  %10.5e   ",globalBestFoM);
    for(k=1; k<=nPar; k++){
      fprintf(fSearchProgress," %10.3f  ",globalBestPos[k]);  
    }
  }

  /**********************/
  /* start optimization */
  /**********************/
  if(DEBUG)
    fprintf(LogFilePtr,"\n\n DEBUG: Starting optimization loop ================================");

  for (i=1; i<=Nsteps; i++){
    for (j=1; j<=Nbees; j++){
      
      if(DEBUG)
	fprintf(LogFilePtr,"\n DEBUG: step %d, bee no %d: ",i,j);
      
      currentFoM = UpdatePosition(&ThisBee[j],wMin_inertia,wMax_inertia,wMin_local,wMax_local,wMin_global,wMax_global,Nsteps,i);
      
      if(DEBUG)
	(j<Nbees) ? fprintf(fBeeMovement,"%10.4f  ",ThisBee[j].Position[1]) : fprintf(fBeeMovement,"%10.4f  \n",ThisBee[j].Position[1]); 
      
      if(currentFoM < ThisBee[j].localBestFoM ){
	ThisBee[j].localBestFoM = currentFoM;
	CopyPosition(ThisBee[j].Position,ThisBee[j].localBestPos);
	if(ThisBee[j].localBestFoM < globalBestFoM){
	  globalBestFoM=ThisBee[j].localBestFoM;
	  CopyPosition(ThisBee[j].localBestPos,globalBestPos);
	  last_change=i;
	  fprintf(LogFilePtr,"\n (swarm) new global best: Factor/FoM: %10.4e in step %d \n\n",globalBestFoM,i);
	  if(eOut>1){
	    if(DEBUG)
	      fprintf(LogFilePtr," found by bee %d",j);
	    for(k=1; k<=nPar; k++)
	      fprintf(LogFilePtr,"       global best par. %d: %f \n",k,globalBestPos[k]);
	    fprintf(LogFilePtr,"\n");
	  }
	}
      }
    }

    if(eOut>1){
      fprintf(fSearchProgress,"\n %d  %10.5e   ",i,globalBestFoM);
      for(k=1; k<=nPar; k++){
	fprintf(fSearchProgress," %10.3f  ",globalBestPos[k]);  
      }
    }
    if(DEBUG){
      fprintf(LogFilePtr,"\n-------------------------\n");
    }  

    /* EXIT optimization loop if no improvement in last [Nfinish] steps */  
    if( i-last_change == Nfinish ){
      fprintf(LogFilePtr,"\n No improvement in last %d steps, finish optimization",Nfinish);
      i++;
      break;
    }

  }

  /* clean up and write result */
  fclose(fSearchProgress);
  fclose(fBeeMovement);
  if(eOut<3){
  remove("Swarm_BeeMovement_Parameter1.dat");
    if(eOut<2)
      remove("Swarm_GlobalBestValues.dat");
  }

  fprintf(LogFilePtr,"\n\n  optimization finished after %d steps \n global best Factor/FoM: %10.4e \n",i-1,globalBestFoM);
  printf("Optimization finished after %d steps \n global best Factor/FoM: %10.4e \n",i-1,globalBestFoM);
  for(k=1; k<=nPar; k++){
    fprintf(LogFilePtr,"  global best value for parameter %d: %f \n",k,globalBestPos[k]);
    printf("   global best value for parameter %d: %f \n",k,globalBestPos[k]);
  }
  
  return READY;
}

/*********************************************************************************************/
/* Function setting start values: 
   random position in parameter space, only first bee starts at user given start values (P00) 
   random velocity within +-StepSize (DelP)                                                  */
/*********************************************************************************************/
static double SetStartingPosition(SwarmIndividual *onebee, int BeeNo){
  double v_max;
  int h, i;
  for (h=1; h<=nPar; h++){
    if(BeeNo>1)
      onebee->Position[h]=MonteCarlo(Pmin[h],Pmax[h]);
    else
      onebee->Position[h]=P00[h];
    v_max=DelP[h];
    onebee->Velocity[h]=MonteCarlo(-v_max,v_max);
    if(DEBUG)
      fprintf(LogFilePtr,"\n DEBUG: starting velocity parameter %d: %f  \n        starting position parameter %d: %f ",h,onebee->Velocity[h],h,onebee->Position[h]);
  }

  for(i=1; i<=NMAX; i++){
    if(i<=nPar)
      arP[1][i]=onebee->Position[i];
    else
      arP[1][i]=0;
  }
  CalcAllFcts(1,1);

  if(arF[1][1]>1e20) //fom=0
    fprintf(LogFilePtr,"\n WARNING: figure of merit is 0, check optimization parameter range!");

  return arF[1][1];
}

/*******************************************************************************/
/* Function updating position and velocity values: 
   v_new = w0 v_old + w1 r1 (localBestPos - p_old) + w2 r2 (globalBest - p_old)
     with w0, w1, w2: weights
     r1, r2: random numbers in [0,1]
   p_new = p_old + v_new                                                       */
/*******************************************************************************/
static double UpdatePosition(SwarmIndividual *onebee, const double w0_min, const double w0_max, const double w1_min, const double w1_max, const double w2_min, const double w2_max, const int nSteps, const int current){
  double r1, r2, v_max, v_old, p_old, w0_i, w1_i, w2_i;
  int h, i;
  r1=MonteCarlo(0,1);
  r2=MonteCarlo(0,1);

  /* time dependence */
  w0_i=(w0_max-w0_min)*(nSteps-current)/nSteps+w0_min;
  w1_i=(w1_min-w1_max)*current/nSteps+w1_max;
  w2_i=(w2_max-w2_min)*current/nSteps+w2_min;
 
  for (h=1; h<=nPar; h++){
    v_old=onebee->Velocity[h];
    p_old=onebee->Position[h];
    onebee->Velocity[h] = w0_i*v_old + w1_i*r1*(onebee->localBestPos[h]-p_old) + w2_i*r2*(globalBestPos[h]-p_old); 
    v_max=0.5*fabs(Pmax[h]-Pmin[h]);
    if( fabs(onebee->Velocity[h]) > v_max )
      onebee->Velocity[h] = ((onebee->Velocity[h] > 0) ? v_max : -v_max);
    onebee->Position[h] = p_old + onebee->Velocity[h];

    /* Set p to threshold if outside range */
    if(onebee->Position[h]>Pmax[h])
      onebee->Position[h]=Pmax[h];
    else if (onebee->Position[h] < Pmin[h])
      onebee->Position[h]=Pmin[h];

    if(DEBUG)
      fprintf(LogFilePtr,"\n DEBUG: new velocity parameter %d: %f  \n        new position parameter %d: %f          ",h,onebee->Velocity[h],h,onebee->Position[h]);
  }

  for(i=1; i<=NMAX; i++){
    if(i<=nPar)
      arP[1][i]=onebee->Position[i];
    else
      arP[1][i]=0;
  }

  /* Run simulation with parameters arP, 
     calculate Factor/FOM and write into arF */
  CalcAllFcts(1,1);

  return arF[1][1];
}


/************************************************************************************************/
/* Function to read optimization control parameters from file 
   Input : sIniFile:  Name of the file 
   Output: *pNbees       : number of swarm individuals 
           *pNparameters : number of parameters
           *pNsteps      : number of optimization steps
           *pNfinish     : number of opt. steps without improvement (finishing condition)
           *pW0          : weight inertia term (w_inertia)
           *pW1          : weight individual best (w_local)
           *pW2          : weight global best (w_global)                                   
           *pWrite       : output level (1: default, 2: write parameter development, 3: debug)  */
/************************************************************************************************/
static short ReadIniFile(int *pNbees, int *pNparameters, int *pNsteps, int *pNfinish, double *pW0min, double *pW0max, double *pW1min, double *pW1max, double *pW2min, double *pW2max, short *pWrite, const char *sIniFile){
  short rc=TRUE,
        rp=TRUE;             
  FILE* pIniFile;
  char  sParameter[BUF_LEN+1], // content of the parameter
    sMessage[100],             // Warning messages
    cId;                       // character defining the parameter
        
  pIniFile = fileOpen(sIniFile, "r");

  if (pIniFile!=NULL) {       
    rp = ReadParameter(&cId, sParameter,  pIniFile);
    while (rp) 
      {
	switch (cId)
	  {
	  case 'b': *pNbees  = atoi(sParameter); 
	    if(*pNbees > MAX_BEES){
	      *pNbees = MAX_BEES;
	      fprintf(LogFilePtr, "\n WARNING: swarm size larger than max. size, set to maximum: %d individuals \n", MAX_BEES);
	    }
	    break;
	  case 's': *pNsteps  = atoi(sParameter); break;
	  case 'f': *pNfinish  = atoi(sParameter); break;
	  case 'w': *pW0min  = atof(sParameter); break;
	  case 'W': *pW0max  = atof(sParameter); break;
	  case 'u': *pW1min  = atof(sParameter); break;
	  case 'U': *pW1max  = atof(sParameter); break;
	  case 'v': *pW2min  = atof(sParameter); break;
	  case 'V': *pW2max  = atof(sParameter); break;
	  case 'x': *pWrite  = (short) atoi(sParameter); break;
	  default : sprintf(sMessage, "unknown parameter in '%s'", sIniFile);
	    Warning(sMessage);
	  }
	rp = ReadParameter(&cId, sParameter,  pIniFile);
      }
    fclose(pIniFile);
  }
  else
    {       
      Warning("WARNING (swarm): file containing control parameters could not be opened, default values are used");
      rc=FALSE;
    }

  if(*pW0min>*pW0max)
    fprintf(LogFilePtr,"\n WARNING: w0_min > w0_max  ==>  increasing velocity will slow down convergence \n");
  if(*pW1min>*pW1max)
    fprintf(LogFilePtr,"\n WARNING: w1_min > w1_max  ==>  cognitive part is increasing with time instead of decreasing! \n");
  if(*pW2min>*pW2max)
    fprintf(LogFilePtr,"\n WARNING: w2_min > w2_max  ==>  social part is decreasing with time instead of increasing! \n");

  return rc;
  
}

/*******************************************************************************************/
inline void CopyPosition(double PosIn[], double PosOut[]){
  int k;
  for(k=1; k<=nPar; k++){
    PosOut[k]=PosIn[k];
  }
}

