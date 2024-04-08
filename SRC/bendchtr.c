/***********************FUNCTION*FOR*MODULE*BENDER********************************************************************************/
/*********************************************************************************************************************************/
/************FUNCTION*FOR*MOVING*NEUTRON*INSIDE*CHANNEL*OF*BENDER*****************************************************************/
/*********************************************************************************************************************************/

#ifdef VT_GRAPH
# include "cpgplot.h"
  extern int do_visualise; /* default : no visualisation */
#endif
 
#include "intersection.h"
#include "init.h"
#include "bender.h"

void gsl_ran_dir_3d (const gsl_rng * r, double * x, double * y, double * z);
 


double PathThroughChannelGravOrder2(Neutron *ThisNeutron, Bender BenderMy, BenderChannel ThisBenderCh, long numberch, long NumberOfSurfaces,
                                    double weight_min, double disabut,
                                    double reflectivitylup[1000], double reflectivityrup[1000], double reflectivitytbup[1000], 
                                    double reflectivityldo[1000], double reflectivityrdo[1000], double reflectivitytbdo[1000], 
                                    double surfacerough, long key_grav, long keypol, long qspin,
                                    double entrancediscenter[N_SURF], double exitdiscenter[N_SURF], double spacer) 
{ 
  /************************************************************************************/ 
  /* This routine calculates the trajectory a neutron follows through a simple        */ 
  /* neutron bender. It accepts two structured variables; a pointer to a neutron      */ 
  /* structure and a simple Bender structure. This latter consists simply of four     */ 
  /* infinite planes describing the two walls floor and ceiling of the bender and a   */ 
  /* fifth infinite plane at the exit of the bender. The structure has an assosciated */ 
  /* critical angle; any neutron that intercepts a wall at an angle greater than this */ 
  /* is absorbed.                                                                     */ 
  /* Neutron flight by parabolic trajectories with GRAVITY                            */ 
  /* Significant Rewrited by Manoshin Sergey Feb 2001                                 */ 
  /* Note! The function is return Time Of Flight                                      */ 
  /************************************************************************************/ 
 
  int i, PreviousCollision, ThisCollision=4, datanumber; 
  long R1;
  double angular,degangular; 
  double  TimeOF, TimeOF1, TimeOFmin; 
  double  TimeOFTotal=0.0; 
  double  AP, BP, CP, DOTP, FP, VelocityReal; 
  double  VX, VY, VZ; 
  double signl, signr, signt, signb; 
  double chance;
  /* KL: new variable */
  double stepp; 
  /* Local copy of neutrons for cylce.. */ 
  Neutron TempNeutron, TempNeutron1, NearestNeutron; 
 
 
#ifdef VT_GRAPH 
  if (do_visualise) 
  { 
    double tempx, tempy; 
 
    /* visual path begin */ 
    cpgsci(6); 
    tempx = ThisNeutron->Position[0]; 
    tempy = ThisNeutron->Position[1]; 
    cpgmove(tempx,tempy); 
    /* visual path end */ 
  } 
#endif 
 
		    	
  /* COPY THE CURRENT CHANNEL, CHOOSED */		    	
  /* numberch - number of current(entrance) channel of bender */
  /* left	plane */	    	
	    
  ThisBenderCh.Surf[2].A = BenderMy.SurfLeft[numberch].A;
  ThisBenderCh.Surf[2].B = BenderMy.SurfLeft[numberch].B;
  ThisBenderCh.Surf[2].C = BenderMy.SurfLeft[numberch].C;
  ThisBenderCh.Surf[2].D = BenderMy.SurfLeft[numberch].D;
  ThisBenderCh.Surf[2].E = BenderMy.SurfLeft[numberch].E;
  ThisBenderCh.Surf[2].F = BenderMy.SurfLeft[numberch].F;
  ThisBenderCh.Surf[2].W = BenderMy.SurfLeft[numberch].W;
  ThisBenderCh.Surf[2].P = BenderMy.SurfLeft[numberch].P;
  ThisBenderCh.Surf[2].Q = BenderMy.SurfLeft[numberch].Q;
  ThisBenderCh.Surf[2].R = BenderMy.SurfLeft[numberch].R;
			
		
  /* right plane */	
			
  ThisBenderCh.Surf[3].A = BenderMy.SurfRight[numberch].A;
  ThisBenderCh.Surf[3].B = BenderMy.SurfRight[numberch].B;
  ThisBenderCh.Surf[3].C = BenderMy.SurfRight[numberch].C;
  ThisBenderCh.Surf[3].D = BenderMy.SurfRight[numberch].D;
  ThisBenderCh.Surf[3].E = BenderMy.SurfRight[numberch].E;
  ThisBenderCh.Surf[3].F = BenderMy.SurfRight[numberch].F;
  ThisBenderCh.Surf[3].W = BenderMy.SurfRight[numberch].W;
  ThisBenderCh.Surf[3].P = BenderMy.SurfRight[numberch].P;
  ThisBenderCh.Surf[3].Q = BenderMy.SurfRight[numberch].Q;
  ThisBenderCh.Surf[3].R = BenderMy.SurfRight[numberch].R;
	    
	    
  /* exit plane */ 

  ThisBenderCh.Surf[4].A = BenderMy.SurfExit[numberch].A;
  ThisBenderCh.Surf[4].B = BenderMy.SurfExit[numberch].B;
  ThisBenderCh.Surf[4].C = BenderMy.SurfExit[numberch].C;
  ThisBenderCh.Surf[4].D = BenderMy.SurfExit[numberch].D;
  ThisBenderCh.Surf[4].E = BenderMy.SurfExit[numberch].E;
  ThisBenderCh.Surf[4].F = BenderMy.SurfExit[numberch].F;
  ThisBenderCh.Surf[4].W = BenderMy.SurfExit[numberch].W;
  ThisBenderCh.Surf[4].P = BenderMy.SurfExit[numberch].P;
  ThisBenderCh.Surf[4].Q = BenderMy.SurfExit[numberch].Q;
  ThisBenderCh.Surf[4].R = BenderMy.SurfExit[numberch].R; 
			
			
  /*   Define the stepp for given channel	*/
	    
  /* KL: correction fabs(x - y) instead of fabs(fabs(x) - fabs(y))  
  and calculation in one equation instead of three          
  KL: size of 'stepp' decreased;  
  possible ERROR: result depends on size of 'stepp' (see example BEND_DIR)  
  reason unknown                                                          */ 
		   
  stepp = 0.01*(fabs(exitdiscenter    [numberch+1] - exitdiscenter    [numberch]) +
                fabs(entrancediscenter[numberch+1] - entrancediscenter[numberch]) - spacer);

  /* Convert stepp in time ms */
	    
  VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength)); 	
	    
  if (VelocityReal != 0.0)
  {
    stepp = stepp/VelocityReal;
  }
  else
  {
    fprintf(LogFilePtr,"WARNING! Neutron Velocity is ZERO!!!, Wavelength is INFINITY! \n");
    return(-1);
  }
	    
 
 
  /***********************************************************************************/ 
  /* The main loop here is continuous: the neutron will continue to bounce around,   */ 
  /* until it is absorbed or intercepts with the exit plane.                         */ 
  /***********************************************************************************/ 
 
  /*	fprintf(LogFilePtr,"weight min  %e \n",weight_min);*/ 
 
  R1 = 1;
  PreviousCollision = 6; 
 
  while(GUIDEFLIGHT) 
  { 
    TimeOFmin = 99999999999999999999999.9; 
 
    /***********************************************************************************/ 
    /* Loop through all five planes....                                                */ 
    /***********************************************************************************/ 
    for(i=0;i<5;i++) 
    { 
      /***********************************************************************************/ 
      /* If the plane currently indexed is the one that the neutron has just hit, return */ 
      /* contol to the top of the loop and look at the next plane.                       */ 
      /***********************************************************************************/ 
      //			if(i==PreviousCollision) continue; 
 
      /***********************************************************************************/ 
      /* Find the point where this neutron trajectory intercepts the currently indexed   */ 
      /* plane  WITH GRAVITY      	                               */ 
      /***********************************************************************************/ 
 
      /* Save current neutron, becouse function NeutronPlaneIntersectionGrav have 
         modify Neutron data structure  */		 
 
      CopyNeutron(ThisNeutron, &TempNeutron); 
 
      TimeOF=NeutronSurfaceSecIntersectionGr(&TempNeutron, ThisBenderCh.Surf[i], key_grav);		 
 
      /*	Intercept = NeutronPlaneIntersection(*ThisNeutron, ThisGuide.Wall[i]); */ 
      //      	fprintf(LogFilePtr,"num = %d time = %e \n",i, TimeOF); 
      /***********************************************************************************/ 
      /* If this intercept point is behind the neutrons current position, pass control to*/ 
      /* the top of the loop: OR Time of flight <= 0.0, Fixed Manoshin Sergey 19.02.00   */ 
      /***********************************************************************************/ 
      
      /* ILLEGAL NEUTRON PARAMETERS */
      
      if (TempNeutron.Position[0] < ThisNeutron->Position[0]) continue; 
      if (TempNeutron.Vector[0] < 0.0) continue;	 
      if (TimeOF <= 0.0) continue;				 
 
      /***********************************************************************************/ 
      /* If this calculated distance is not the shortest so far, return to the top of the*/ 
      /* loop.  TimeOF -> min                                                            */ 
      /***********************************************************************************/ 
      if(TimeOF > TimeOFmin) continue; 
 
      /***********************************************************************************/ 
      /* The intercept of the neutron with this wall is the nearest so far, so accept it */ 
      /* temporarily.                                                                    */ 
      /***********************************************************************************/ 
 
      CopyNeutron(&TempNeutron, &NearestNeutron); 
      TimeOFmin = TimeOF; 
      ThisCollision = i; 
    } 
 
    //    	fprintf(LogFilePtr,"Number plane of coll: %d \n",ThisCollision); 
	
    /***********************************************************************************/ 
    /* Having looped through all five planes, the current values of NearestNeutron,    */ 
    /* TimeOFmin and ThisCollision, reflect the coordinates, distance and index        */ 
    /* of the neutrons interaction with a bender wall. If this bender wall is index 4    */ 
    /*(ie: the exit	window) reset the neutron coordinates to this point, add the path  */ 
    /* length to this point to the running total and return that total.                */ 
    /***********************************************************************************/ 
 
 
    /* Neutrons going in the exit of the bender channel */
 
    if(ThisCollision == 4) 
    { 
 
      /* Illegal velocity */
      if(NearestNeutron.Vector[0]<0.0)  return(-1.0);
 
      /*  This feature is reject neutrons, which make reflection near edges (exit) of bender */			 
      if (disabut > 0.0) 
      { 
        VelocityReal = (double)(V_FROM_LAMBDA(NearestNeutron.Wavelength)); 
        if (TimeOFmin*VelocityReal <= disabut)  return(-1.0); 
      } 
 
      /*	fprintf(LogFilePtr,"Velocity real= %f \n", VelocityReal); */ 
 
      if (NearestNeutron.Probability <= weight_min)  return(-1.0); 
 
 
      ThisNeutron->Position[0] = NearestNeutron.Position[0]; 
      ThisNeutron->Position[1] = NearestNeutron.Position[1]; 
      ThisNeutron->Position[2] = NearestNeutron.Position[2]; 
      ThisNeutron->Vector[2] = NearestNeutron.Vector[2]; 
      ThisNeutron->Probability = NearestNeutron.Probability; 
      ThisNeutron->Wavelength = NearestNeutron.Wavelength; 
 
      TimeOFTotal =  TimeOFTotal + TimeOFmin; 
 
  #ifdef VT_GRAPH 
      if (do_visualise) 
      { 
        double tempx, tempy; 
        /* visual path begin */ 
        tempx = ThisNeutron->Position[0]; 
        tempy = ThisNeutron->Position[1];
        cpgdraw(tempx,tempy); 
        /* visual path end */ 
      } 
  #endif 
 
      return TimeOFTotal; 
    } 
 
    /***********************************************************************************/ 
    /* If the angle of intersection of the flight path and the bender wall exceed the   */ 
    /* critical angle of the bender, the neutron is absorbed.                           */ 
    /***********************************************************************************/ 
 
    /* normal vector in surface which will be reflected */ 
  
    AP = 2.0*ThisBenderCh.Surf[ThisCollision].A*NearestNeutron.Position[0] 
           + ThisBenderCh.Surf[ThisCollision].B 
           + ThisBenderCh.Surf[ThisCollision].P*NearestNeutron.Position[1] 
           + ThisBenderCh.Surf[ThisCollision].R*NearestNeutron.Position[2]; 
      
      
    BP = 2.0*ThisBenderCh.Surf[ThisCollision].C*NearestNeutron.Position[1] 
           + ThisBenderCh.Surf[ThisCollision].D 
           + ThisBenderCh.Surf[ThisCollision].P*NearestNeutron.Position[0] 
           + ThisBenderCh.Surf[ThisCollision].Q*NearestNeutron.Position[2]; 
      
      
    CP = 2.0*ThisBenderCh.Surf[ThisCollision].E*NearestNeutron.Position[2] 
           + ThisBenderCh.Surf[ThisCollision].F 
           + ThisBenderCh.Surf[ThisCollision].Q*NearestNeutron.Position[1] 
           + ThisBenderCh.Surf[ThisCollision].R*NearestNeutron.Position[0]; 
 
 
    /* Normalize normale vector to the reflection plane */	 
 
    FP = sqrt(AP*AP + BP*BP + CP*CP); 
 
    if (FP == 0.0) return(-1.0); 
 
    AP = AP/FP; 
    BP = BP/FP; 
    CP = CP/FP; 
 
 
    /* influence of rough surface */		 
 
    if (surfacerough != 0.0) 
    { 
      // len = vector3rand(&VX, &VY, &VZ);
      gsl_ran_dir_3d( vit_gsl_rng, &VX, &VY, &VZ);

      /*fprintf(LogFilePtr,"vx vy vz %f  %f  %f  %f  %f \n",VX,VY,VZ,len,surfacerough);*/		 
      AP = AP + surfacerough*VX; 
      BP = BP + surfacerough*VY; 
      CP = CP + surfacerough*VZ; 
 
      /* Renormalize normale vector to the reflection plane */	 
 
      FP = sqrt(AP*AP + BP*BP + CP*CP); 
 
      if (FP == 0.0) return(-1.0); 
 
      AP = AP/FP; 
      BP = BP/FP; 
      CP = CP/FP; 
    }	 
 
    chance = Vran();
 
    angular=fabs(NeutronPlaneAngle2(&NearestNeutron, AP, BP, CP)); 
 
    /* Convert from radian to degree */ 
 
    degangular = angular*360.0/(2.0*M_PI); 
 
    datanumber =  (int)(degangular*1000.0/(NearestNeutron.Wavelength)); 
 
    if (datanumber > 999) return(-1.0); 
 
    /*Choose the reflectivity file */	 
    
    switch(ThisCollision) 
    {	 
      /* top plane */ 
      case 0: 
        if (keypol == 1) 
        { 
          if(NearestNeutron.Spin[qspin] == 1.0) 
          { 
            NearestNeutron.Probability *= reflectivitytbup[datanumber]; 
          } 
          if(NearestNeutron.Spin[qspin] == -1.0) 
          { 
            NearestNeutron.Probability *= reflectivitytbdo[datanumber]; 
          } 
        } 
        else 
        { 
          NearestNeutron.Probability *= reflectivitytbup[datanumber]; 
        }		 
        break; 
	 
      /* bottom plane */	 
      case 1:			 
        if (keypol == 1) 
        { 
          if(NearestNeutron.Spin[qspin] == 1.0)			 
          { 
            NearestNeutron.Probability *= reflectivitytbup[datanumber]; 
          } 
          if(NearestNeutron.Spin[qspin] == -1.0) 
          { 
            NearestNeutron.Probability *= reflectivitytbdo[datanumber]; 
          } 
        } 
        else 
        { 
          NearestNeutron.Probability *= reflectivitytbup[datanumber]; 
        }		 
        break;				 
	 
      /* right plane */ 
      case 2: 
        if (keypol == 1) 
        { 
          if(NearestNeutron.Spin[qspin] == 1.0) 
          { 
            NearestNeutron.Probability *= reflectivitylup[datanumber]; 
          } 
          if(NearestNeutron.Spin[qspin] == -1.0) 
          { 
            NearestNeutron.Probability *= reflectivityldo[datanumber]; 
          } 
        } 
        else 
        { 
          NearestNeutron.Probability *= reflectivitylup[datanumber];		 
        }		 
        break; 
	 
      /* left plane */	 
      case 3: 
        if (keypol == 1) 
        { 
          if(NearestNeutron.Spin[qspin] == 1.0) 
          { 
            NearestNeutron.Probability *= reflectivityrup[datanumber]; 
          } 
          if(NearestNeutron.Spin[qspin] == -1.0) 
          { 
            NearestNeutron.Probability *= reflectivityrdo[datanumber]; 
          } 
        } 
        else 
        { 
          NearestNeutron.Probability *= reflectivityrup[datanumber]; 
        }		 
        break; 
	 
      default: 
        fprintf(LogFilePtr,"No such plane!\n"); 
        break; 
    }			 
 
 	  /* Reject neutron with small probability */
 	  if (NearestNeutron.Probability <= weight_min)  return(-1.0); 
 
    /***********************************************************************************/ 
    /*  Calculate the trajectory of the reflected neutron.                             */ 
    /***********************************************************************************/ 
    /* Make reflection */	 
 
 
    DOTP = AP*NearestNeutron.Vector[0] + BP*NearestNeutron.Vector[1] + 
      CP*NearestNeutron.Vector[2]; 
 
    ThisNeutron->Vector[0] = NearestNeutron.Vector[0] - 2.0*DOTP*AP; 
    ThisNeutron->Vector[1] = NearestNeutron.Vector[1] - 2.0*DOTP*BP; 
    ThisNeutron->Vector[2] = NearestNeutron.Vector[2] - 2.0*DOTP*CP;		 
 
    //	fprintf(LogFilePtr,"Make reflection\n"); 
 
    /***********************************************************************************/ 
    /* Reset the neutron coordinates to coordinates of the collision, add the path     */ 
    /* length to this point to the running total, keep track of which plane has just   */ 
    /* been hit and then return to the top of the top of the loop and find the next    */ 
    /* collision                                                                       */ 
    /***********************************************************************************/ 
 
    ThisNeutron->Position[0] = NearestNeutron.Position[0]; 
    ThisNeutron->Position[1] = NearestNeutron.Position[1]; 
    ThisNeutron->Position[2] = NearestNeutron.Position[2]; 
    ThisNeutron->Wavelength = NearestNeutron.Wavelength;  
    ThisNeutron->Probability = NearestNeutron.Probability; 
 
 
    TimeOFTotal =  TimeOFTotal + TimeOFmin ; 
    PreviousCollision = ThisCollision; 
 
 
#ifdef VT_GRAPH 
    if (do_visualise) 
    { 
      double tempx, tempy; 
      /* visual path begin */ 
      tempx = ThisNeutron->Position[0]; 
      tempy = ThisNeutron->Position[1]; 
      cpgdraw(tempx,tempy); 
      cpgsci(7); 
    } 
#endif 
 
    /*	angular1=fabs(NeutronPlaneAngle2(ThisNeutron, AP, BP, CP));		 
	 
    fprintf(LogFilePtr,"angle inc = %f, refl = %f \n",angular,angular1); */ 
 
    CopyNeutron(ThisNeutron, &TempNeutron1); 
    
    TimeOF1=NeutronSurfaceSecIntersectionGr(&TempNeutron1, ThisBenderCh.Surf[4], key_grav);		 
 
     /* CHECK incorrect flight of neutron, move neutron after reflection on small distance 
     and check inside channel or no , no - exit(-1) */
     
    if (TimeOF1 > stepp) 
    {
      VelocityReal = (double)(V_FROM_LAMBDA(ThisNeutron->Wavelength)); 
      ThisNeutron->Position[0] = ThisNeutron->Position[0] + VelocityReal*stepp*(ThisNeutron->Vector[0]); 
      ThisNeutron->Position[1] = ThisNeutron->Position[1] + VelocityReal*stepp*(ThisNeutron->Vector[1]); 
      ThisNeutron->Position[2] = ThisNeutron->Position[2] + VelocityReal*stepp*(ThisNeutron->Vector[2]); 
	
      /* Include gravity */ 
      if (key_grav == 1) 
      {	 
        ThisNeutron->Position[2] = ThisNeutron->Position[2] - 0.5*(G*1.0e-4)*stepp*stepp; 
        ThisNeutron->Vector[2]   = ThisNeutron->Vector[2]   -    ((G*1.0e-4)*stepp/VelocityReal);	 
      }	 
      TimeOFTotal = TimeOFTotal + stepp; 
 
 
      /* check the incorrect path of neutron */ 
      signl = 
        ThisBenderCh.Surf[2].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[2].B*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[2].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[2].D*ThisNeutron->Position[1] + 
        ThisBenderCh.Surf[2].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[2].F*ThisNeutron->Position[2] + 
        ThisBenderCh.Surf[2].W + 
        ThisBenderCh.Surf[2].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[2].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[2].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
 
 
 
      signr = 
        ThisBenderCh.Surf[3].A*ThisNeutron->Position[0]*ThisNeutron->Position[0]+ 
        ThisBenderCh.Surf[3].B*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[3].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[3].D*ThisNeutron->Position[1] + 
        ThisBenderCh.Surf[3].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[3].F*ThisNeutron->Position[2] + 
        ThisBenderCh.Surf[3].W + 
        ThisBenderCh.Surf[3].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[3].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[3].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
 
 
      signt = 
        ThisBenderCh.Surf[0].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[0].B*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[0].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[0].D*ThisNeutron->Position[1] + 
        ThisBenderCh.Surf[0].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[0].F*ThisNeutron->Position[2] + 
        ThisBenderCh.Surf[0].W + 
        ThisBenderCh.Surf[0].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[0].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[0].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
 
      signb = 
        ThisBenderCh.Surf[1].A*ThisNeutron->Position[0]*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[1].B*ThisNeutron->Position[0] + 
        ThisBenderCh.Surf[1].C*ThisNeutron->Position[1]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[1].D*ThisNeutron->Position[1] + 
        ThisBenderCh.Surf[1].E*ThisNeutron->Position[2]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[1].F*ThisNeutron->Position[2] + 
        ThisBenderCh.Surf[1].W + 
        ThisBenderCh.Surf[1].P*ThisNeutron->Position[0]*ThisNeutron->Position[1]+ 
        ThisBenderCh.Surf[1].Q*ThisNeutron->Position[1]*ThisNeutron->Position[2]+ 
        ThisBenderCh.Surf[1].R*ThisNeutron->Position[2]*ThisNeutron->Position[0] ; 
 
        //fprintf(LogFilePtr,"signs l = %f r = %f \n",signl,signr); 

#ifdef VT_GRAPH 
    if (do_visualise) 
    { 
      double tempx, tempy; 
      /* visual path begin */ 
      tempx = ThisNeutron->Position[0]; 
      tempy = ThisNeutron->Position[1]; 
      cpgdraw(tempx,tempy); 
      cpgsci(7); 
    } 
#endif 

      /* particle outside channel */ 
      /* KL: Warning: neutron must move from the surface, otherwise sign is a very low value, 
      that can be above or below zero. Therefore, comparison: > 1e-x may be better than > 0.0 */ 
	       
      if ((signl*signr) > 0.0) 
        return(-1.0); 
 
      if ((signt*signb) > 0.0) 
        return(-1.0); 
     } 
  } 
} 





 
